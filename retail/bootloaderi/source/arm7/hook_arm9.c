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
#include "value_bits.h"

#define b_saveOnFlashcard BIT(0)
#define b_ROMinRAM BIT(1)
#define b_eSdk2 BIT(2)
#define b_dsiMode BIT(3)
#define b_enableExceptionHandler BIT(4)
#define b_isSdk5 BIT(5)
#define b_overlaysCached BIT(6)
#define b_cacheFlushFlag BIT(7)
#define b_cardReadFix BIT(8)
#define b_cacheDisabled BIT(9)
#define b_slowSoftReset BIT(10)
#define b_dsiBios BIT(11)
#define b_asyncCardRead BIT(12)
#define b_softResetMb BIT(13)
#define b_cloneboot BIT(14)


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

// alt black sigil
static const u32 handlerStartSigAlt[5] = {
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe28cce21,		//
	0xe51c1008,		//
	0xe14f0000		//
};

// alt gsun
static const u32 handlerStartSigAlt5[5] = {
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

// alt black sigil
static const u32 handlerEndSigAlt[4] = {
	0xe8bd5003, 	// 
	0xe14c10b8,		// 
	0xe169f000,		// 
	0xe12fff1e		// 
};

// alt gsun
static const u32 handlerEndSigAlt5[4] = {
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
			addr = findOffset(
				start, size,
				handlerStartSigAlt5, 4
			);
        }
        if (!addr) {
            dbg_printf("ERR_HOOK_9 : handlerStartSig\n");
    		return NULL;
        }
	}

    dbg_printf("handlerStartSig\n");
    dbg_hexa((u32)addr);
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
        if (addr2) {
			addr2 = addr2 + 1;
		} else {
			addr2 = findOffset(
				addr, MAX_HANDLER_LEN_ALT*sizeof(u32),
				handlerEndSigAlt5, 4
			);
        }
        if (!addr2) {
            dbg_printf("ERR_HOOK_9 : handlerEndSig\n");
    		return NULL;
        }
    }

    dbg_printf("handlerEndSig\n");
    dbg_hexa((u32)addr2);
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

	//u32* actualReturnAddr = addr + 2;
	//u32* actualTableAddr = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);

	// The first entry in the table is for the Vblank handler, which is what we want
	return (u32*)tableAddr;
	// 2     LCD V-Counter Match
}

int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 saveCluster,
	u32 saveSize,
	u16 saveOnFlashcard,
	u32 cacheBlockSize,
	u8 extendedMemory,
	u8 ROMinRAM,
	u8 dsiMode, // SDK 5
	u8 enableExceptionHandler,
	u8 consoleModel,
	bool usesCloneboot
) {
	nocashMessage("hookNdsRetailArm9");

	extern u32 iUncompressedSize;
	extern u32 overlaysSize;
	extern bool overlayPatch;
	extern u32 dataToPreloadAddr[2];
	extern u32 dataToPreloadSize[2];
	const char* romTid = getRomTid(ndsHeader);

	ce9->fileCluster            = fileCluster;
	ce9->saveCluster            = saveCluster;
	ce9->saveSize               = saveSize;
	if (saveOnFlashcard) {
		ce9->valueBits |= b_saveOnFlashcard;
	}
	if (ROMinRAM) {
		ce9->valueBits |= b_ROMinRAM;
	}
	if (moduleParams->sdk_version < 0x2008000) {
		ce9->valueBits |= b_eSdk2;
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
	/*if (strncmp(romTid, "CAY", 3) == 0 // Army Men: Soldiers of Misfortune
	 || strncmp(romTid, "B7F", 3) == 0 // The Magic School Bus: Oceans
	) {
		ce9->valueBits |= b_cacheDisabled; // Disable card data cache for specific games
	}*/
	if (strncmp(romTid, "YV5", 3) == 0 // Dragon Quest V: Hand of the Heavenly Bride
	) {
		ce9->valueBits |= b_cacheDisabled; // Disable card data cache for specific games
	}
	if (!(REG_SCFG_ROM & BIT(9))) {
		ce9->valueBits |= b_dsiBios;
	}
	if (asyncCardRead) {
		ce9->valueBits |= b_asyncCardRead;
	}
	if (patchOffsetCache.resetMb) {
		ce9->valueBits |= b_softResetMb;
	}
	if (usesCloneboot) {
		ce9->valueBits |= b_cloneboot;
	}
	ce9->overlaysSize           = overlaysSize;
	ce9->consoleModel           = consoleModel;
	if (extendedMemory) {
		ce9->romLocation[0] = (moduleParams->sdk_version < 0x2008000) ? ROM_LOCATION_EXT_SDK2 : ROM_LOCATION_EXT;
	} else if (consoleModel > 0) {
		ce9->romLocation[0] = ((dsiMode || isSdk5(moduleParams)) ? ROM_SDK5_LOCATION : ROM_LOCATION);
	} else {
		ce9->romLocation[0] = ROM_LOCATION;
	}

	if (!ROMinRAM) {
		//extern bool gbaRomFound;
		bool runOverlayCheck = overlayPatch;
		u32 dataToPreloadSizeAligned = 0;
		ce9->cacheBlockSize = cacheBlockSize;
		if (isSdk5(moduleParams)) {
			if (consoleModel > 0) {
				ce9->romLocation[0] = dev_CACHE_ADRESS_START_SDK5;
				ce9->cacheAddress = dev_CACHE_ADRESS_START_SDK5;
				ce9->cacheSlots = ((ndsHeader->unitCode > 0 && dsiModeConfirmed) ? dev_CACHE_ADRESS_SIZE_TWLSDK : dev_CACHE_ADRESS_SIZE_SDK5)/cacheBlockSize;
			} else if (ndsHeader->unitCode > 0 && dsiModeConfirmed) {
				ce9->romLocation[0] = retail_CACHE_ADRESS_START_TWLSDK;
				if (strncmp(romTid, "DD3", 3) == 0) {
					ce9->cacheAddress = 0x02000000;
					ce9->cacheSlots = 0x4000/cacheBlockSize;
				} else {
					ce9->cacheAddress = retail_CACHE_ADRESS_START_TWLSDK;
					ce9->cacheSlots = retail_CACHE_ADRESS_SIZE_TWLSDK/cacheBlockSize;
				}
			} else {
				ce9->romLocation[0] = CACHE_ADRESS_START;
				ce9->cacheAddress = CACHE_ADRESS_START;
				ce9->cacheSlots = retail_CACHE_ADRESS_SIZE_SDK5/cacheBlockSize;
			}
		} else {
			if (strncmp(romTid, "UBR", 3) == 0) {
				runOverlayCheck = false;
				ce9->romLocation[0] = CACHE_ADRESS_START_low;
				ce9->cacheAddress = CACHE_ADRESS_START_low;
				ce9->cacheSlots = retail_CACHE_ADRESS_SIZE_low/cacheBlockSize;
			} else {
				ce9->romLocation[0] = (consoleModel>0&&dsiMode ? dev_CACHE_ADRESS_START_SDK5 : CACHE_ADRESS_START);
				ce9->cacheAddress = ce9->romLocation[0];
				if (consoleModel > 0 /*&&!gbaRomFound*/) {
					ce9->cacheSlots = (dsiMode ? dev_CACHE_ADRESS_SIZE_SDK5 : dev_CACHE_ADRESS_SIZE)/cacheBlockSize;
				} else {
					ce9->cacheSlots = retail_CACHE_ADRESS_SIZE/cacheBlockSize;
				}
			}
		}
		if (dataToPreloadSize[0] > 0 && (dataToPreloadSize[0]+dataToPreloadSize[1]) < (consoleModel>0 ? (isSdk5(moduleParams) ? 0xF00000 : 0x1700000) : (ndsHeader->unitCode > 0 && dsiModeConfirmed ? 0x400000 : 0x700000))) {
			ce9->romLocation[1] = ce9->romLocation[0]+dataToPreloadSize[0];
			ce9->romLocation[0] -= dataToPreloadAddr[0];
			ce9->romLocation[1] -= dataToPreloadAddr[1];
			for (u32 i = 0; i < dataToPreloadSize[0]+dataToPreloadSize[1]; i += cacheBlockSize) {
				ce9->cacheAddress += cacheBlockSize;
				dataToPreloadSizeAligned += cacheBlockSize;
			}
			ce9->cacheSlots -= dataToPreloadSizeAligned/cacheBlockSize;
			ce9->romPartSrc[0] = dataToPreloadAddr[0];
			ce9->romPartSrc[1] = dataToPreloadAddr[1];
			ce9->romPartSize[0] = dataToPreloadSize[0];
			ce9->romPartSize[1] = dataToPreloadSize[1];
		}
		if (runOverlayCheck && overlaysSize <= 0x700000) {
			/*extern u8 gameOnFlashcard;
			if (!gameOnFlashcard && (consoleModel > 0 || !dsiModeConfirmed || (ndsHeader->unitCode == 0 && dsiModeConfirmed))) {
				if (cacheBlockSize == 0) {
					ce9->cacheAddress += (overlaysSize/4)*4;
				} else
				for (u32 i = 0; i < overlaysSize; i += cacheBlockSize) {
					ce9->cacheAddress += cacheBlockSize;
					ce9->cacheSlots--;
				}
			}*/
			ce9->valueBits |= b_overlaysCached;
		}

		if (strncmp(romTid, "CLJ", 3) == 0 || strncmp(romTid, "IPK", 3) == 0 || strncmp(romTid, "IPG", 3) == 0) {
			ce9->valueBits |= b_cacheFlushFlag;
		}

		if (strncmp(romTid, "IPK", 3) == 0 || strncmp(romTid, "IPG", 3) == 0) {
			ce9->valueBits |= b_cardReadFix;
		}
		if (strncmp(romTid, "UBR", 3) == 0 || iUncompressedSize > 0x280000) {
			ce9->valueBits |= b_slowSoftReset;
		}
	} else {
		ce9->romLocation[0] -= usesCloneboot ? 0x8000 : (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
	}

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

int hookNdsRetailArm9Mini(cardengineArm9* ce9, const tNDSHeader* ndsHeader, u8 consoleModel) {
	ce9->consoleModel           = consoleModel;

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

    ce9->irqTable   = tableAddr;

	nocashMessage("ERR_NONE");
	return ERR_NONE;
}
