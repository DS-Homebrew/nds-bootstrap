#include <stdio.h>
#include <nds/ndstypes.h>
#include <nds/debug.h>
#include <nds/system.h>
#include "debug_file.h"
#include "hook.h"
#include "common.h"
#include "patch.h"
#include "find.h"
#include "nds_header.h"
#include "cardengine_header_arm9.h"

#define b_saveOnFlashcard BIT(0)
#define b_extendedMemory BIT(1)
#define b_ROMinRAM BIT(2)
#define b_dsiMode BIT(3)
#define b_enableExceptionHandler BIT(4)
#define b_isSdk5 BIT(5)
#define b_overlaysInRam BIT(6)
#define b_cacheFlushFlag BIT(7)


static const int MAX_HANDLER_LEN = 50;
static const int MAX_HANDLER_LEN_ALT = 0x200;

// same as arm7
static const u32 handlerStartSig[5] = {
	0xe92d4000, 	// push {lr}
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe28cce21,		// add  ip, ip, #0x210
	0xe51c1008,		// ldr	r1, [ip, #-8]
	0xe3510000		// cmp	r1, #0
};

// alt gsun
static const u32 handlerStartSigAlt[5] = {
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe5bc2208,		// 
	0xe1ec00d8,		//
	0xe3520000		// cmp	r2, #0
};

// same as arm7
static const u32 handlerEndSig[4] = {
	0xe59f1008, 	// ldr  r1, [pc, #8]	(IRQ Vector table address)
	0xe7910100,		// ldr  r0, [r1, r0, lsl #2]
	0xe59fe004,		// ldr  lr, [pc, #4]	(IRQ return address)
	0xe12fff10		// bx   r0
};

// alt gsun
static const u32 handlerEndSigAlt[4] = {
	0xe59f100C, 	// 
	0xe5813000,		// 
	0xe5813004,		// 
	0xeaffffb8		// 
};

static u32* hookInterruptHandler(const u32* start, size_t size) {
	// Find the start of the handler
	u32* addr = findOffset(
		start, size,
		handlerStartSig, 5
	);
	if (!addr) {
        addr = findOffset(
    		start, size,
    		handlerStartSigAlt, 4
    	);
        if (!addr) {
            dbg_printf("ERR_HOOK_9 : handlerStartSig\n");
    		return NULL;
        }
	}
    
    dbg_printf("handlerStartSig\n");
    dbg_hexa(addr);
    dbg_printf("\n");

	// Find the end of the handler
	u32* addr2 = findOffset(
		addr, MAX_HANDLER_LEN*sizeof(u32),
		handlerEndSig, 4
	);
	if (!addr2) {
        addr2 = findOffset(
    		addr, MAX_HANDLER_LEN_ALT*sizeof(u32),
    		handlerEndSigAlt, 4
    	);
        if (!addr2) {
            dbg_printf("ERR_HOOK_9 : handlerEndSig\n");
    		return NULL;
        }
    }
     
    dbg_printf("handlerEndSig\n");
    dbg_hexa(addr2);
    dbg_printf("\n");

	// Now find the IRQ vector table
	// Make addr point to the vector table address pointer within the IRQ handler
	addr = addr2 + sizeof(handlerEndSig)/sizeof(handlerEndSig[0]);

	// Use relative and absolute addresses to find the location of the table in RAM
	u32 tableAddr = addr[0];
    dbg_printf("tableAddr\n");
    dbg_hexa(tableAddr);
    dbg_printf("\n");
    
	u32 returnAddr = addr[1];
    dbg_printf("returnAddr\n");
    dbg_hexa(returnAddr);
    dbg_printf("\n");
    
	u32* actualReturnAddr = addr + 2;
	u32* actualTableAddr = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);

	// The first entry in the table is for the Vblank handler, which is what we want
	return tableAddr;
	// 2     LCD V-Counter Match
}

int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 saveCluster,
	u8 saveOnFlashcard,
	u32 cacheBlockSize,
	u8 extendedMemory,
	u8 ROMinRAM,
	u8 dsiMode, // SDK 5
	u8 enableExceptionHandler,
	u8 consoleModel
) {
	nocashMessage("hookNdsRetailArm9");

	ce9->fileCluster            = fileCluster;
	ce9->saveCluster            = saveCluster;
	if (saveOnFlashcard) {
		ce9->valueBits |= b_saveOnFlashcard;
	}
	if (extendedMemory) {
		ce9->valueBits |= b_extendedMemory;
	}
	if (ROMinRAM) {
		ce9->valueBits |= b_ROMinRAM;
	}
	if (dsiMode) {
		ce9->valueBits |= b_dsiMode; // SDK 5
	}
	if (enableExceptionHandler) {
		ce9->valueBits |= b_enableExceptionHandler;
	}
	if (isSdk5(moduleParams)) {
		ce9->valueBits |= b_isSdk5;
	}
	ce9->consoleModel           = consoleModel;
	if (extendedMemory) {
		ce9->romLocation = ROM_LOCATION_EXT;
	} else if (consoleModel > 0) {
		ce9->romLocation = ((dsiMode || isSdk5(moduleParams)) ? ROM_SDK5_LOCATION : ROM_LOCATION);
	} else {
		ce9->romLocation = ROM_LOCATION;
	}

	if (!ROMinRAM) {
		//extern bool gbaRomFound;
		bool runOverlayCheck = true;
		ce9->cacheBlockSize = cacheBlockSize;
		const char* romTid = getRomTid(ndsHeader);
		if (isSdk5(moduleParams)) {
			if (consoleModel > 0) {
				ce9->romLocation = dev_CACHE_ADRESS_START_SDK5;
				ce9->cacheAddress = dev_CACHE_ADRESS_START_SDK5;
				ce9->cacheSlots = dev_CACHE_ADRESS_SIZE_SDK5/cacheBlockSize;
			} else if ((strncmp(romTid, "BKW", 3) == 0)
					|| (strncmp(romTid, "VKG", 3) == 0)) {
				ce9->romLocation = CACHE_ADRESS_START_low;
				ce9->cacheAddress = CACHE_ADRESS_START_low;
				ce9->cacheSlots = retail_CACHE_ADRESS_SIZE_low/cacheBlockSize;
			} else {
				ce9->romLocation = CACHE_ADRESS_START;
				ce9->cacheAddress = CACHE_ADRESS_START;
				ce9->cacheSlots = retail_CACHE_ADRESS_SIZE_SDK5/cacheBlockSize;
			}
		} else {
			if (strncmp(romTid, "UBR", 3) == 0) {
				runOverlayCheck = false;
				ce9->romLocation = CACHE_ADRESS_START_low;
				ce9->cacheAddress = CACHE_ADRESS_START_low;
				ce9->cacheSlots = retail_CACHE_ADRESS_SIZE_low/cacheBlockSize;
			} else {
				ce9->romLocation = (dsiMode ? dev_CACHE_ADRESS_START_SDK5 : CACHE_ADRESS_START);
				ce9->cacheAddress = ce9->romLocation;

				if (consoleModel > 0 && dsiMode /*&& !gbaRomFound*/) {
					ce9->cacheSlots = dev_CACHE_ADRESS_SIZE_SDK5/cacheBlockSize;
				} else {
					ce9->cacheSlots = retail_CACHE_ADRESS_SIZE/cacheBlockSize;
				}

			}
		}
	  if (runOverlayCheck) {
		extern u32 overlaysSize;

		if (overlaysSize <= /*(consoleModel>0 ? 0x1800000 :*/ 0x800000) {
			for (u32 i = 0; i < overlaysSize; i += cacheBlockSize) {
				ce9->cacheAddress += cacheBlockSize;
				ce9->cacheSlots--;
			}
			ce9->valueBits |= b_overlaysInRam;
		}
	  }

		if(strncmp(romTid, "CLJ", 3) == 0) {
			ce9->valueBits |= b_cacheFlushFlag;
		}
	}

	extern u32 iUncompressedSize;

    u32* tableAddr = patchOffsetCache.a9IrqHookOffset;
 	if (!tableAddr) {
		tableAddr = hookInterruptHandler((u32*)ndsHeader->arm9destination, iUncompressedSize);
	}
   
    if (!tableAddr) {
		dbg_printf("ERR_HOOK_9\n");
		return ERR_HOOK;
	}
    
    dbg_printf("hookLocation arm9: ");
	dbg_hexa((u32)tableAddr);
	dbg_printf("\n\n");
	patchOffsetCache.a9IrqHookOffset = tableAddr;

    /*u32* vblankHandler = hookLocation;
    u32* dma0Handler = hookLocation + 8;
    u32* dma1Handler = hookLocation + 9;
    u32* dma2Handler = hookLocation + 10;
    u32* dma3Handler = hookLocation + 11;
    u32* ipcSyncHandler = hookLocation + 16;
    u32* cardCompletionIrq = hookLocation + 19;*/
    
    ce9->irqTable   = tableAddr;

	nocashMessage("ERR_NONE");
	return ERR_NONE;
}
