#include <stdio.h>
#include <nds/ndstypes.h>
#include <nds/system.h>
#include "debug_file.h"
#include "hook.h"
#include "common.h"
#include "patch.h"
#include "find.h"
#include "nds_header.h"
#include "cardengine_header_arm9.h"
#include "value_bits.h"
#include "nocashMessage.h"

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
#define b_useSharedRam BIT(13)
#define b_cloneboot BIT(14)
#define b_isDlp BIT(15)
#define b_bypassExceptionHandler BIT(16)
#define b_fntFatCached BIT(17)
#define b_waitForPreloadToFinish BIT(18)
#define b_resetOnFirstException BIT(19)


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

void configureRomMap(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const u32 romStart, const u8 dsiMode) {
	extern u32 getRomLocation(const tNDSHeader* ndsHeader, const bool isESdk2, const bool isSdk5, const bool dsiBios);

	const u32 romLocation = getRomLocation(ndsHeader, (ce9->valueBits & b_eSdk2), (ce9->valueBits & b_isSdk5), (ce9->valueBits & b_dsiBios));
	ce9->romLocation = romLocation;
	ce9->romLocation -= romStart;

	if (ndsHeader->unitCode > 0 && dsiMode) {
		// ROM map is not used for TWL games in DSi mode
		return;
	}

	extern u32 romMapLines;
	extern u32 romMap[7][3];

	ce9->romMapLines = romMapLines;
	for (int i = 0; i < 7; i++) {
		for (int i2 = 0; i2 < 3; i2++) {
			ce9->romMap[i][i2] = romMap[i][i2];
		}
	}
}

int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 saveCluster,
	u32 saveSize,
	u8 saveOnFlashcard,
	u32 cacheBlockSize,
	u8 ROMinRAM,
	u8 dsiMode, // SDK 5
	u8 enableExceptionHandler,
	s32 mainScreen,
	u8 consoleModel,
	bool usesCloneboot
) {
	nocashMessage("hookNdsRetailArm9");

	extern bool sharedWramEnabled;
	extern bool scfgBios9i(void);
	extern u32 iUncompressedSize;
	extern u32 overlaysSize;
	extern bool overlayPatch;
	extern u32 baseFntOff;
	extern u32 baseFatOff;
	extern u32 baseFatSize;
	extern u32 romPaddingSize;
	extern u32 dataToPreloadAddr;
	extern u32 dataToPreloadSize;
	// extern u32 dataToPreloadFrame;
	extern bool romLocationAdjust(const tNDSHeader* ndsHeader, const bool laterSdk, const bool isSdk5, const bool dsiBios, u32* romLocation);
	extern bool dataToPreloadFound(const tNDSHeader* ndsHeader);
	const char* romTid = getRomTid(ndsHeader);
	const bool laterSdk = ((moduleParams->sdk_version >= 0x2008000 && moduleParams->sdk_version != 0x2012774) || moduleParams->sdk_version == 0x20029A8);

	ce9->fileCluster            = fileCluster;
	ce9->saveCluster            = saveCluster;
	ce9->saveSize               = saveSize;
	if (saveOnFlashcard) {
		ce9->valueBits |= b_saveOnFlashcard;
	}
	if (ROMinRAM) {
		ce9->valueBits |= b_ROMinRAM;
	}
	if (!laterSdk) {
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
	/* if (strncmp(romTid, "CBB", 3) == 0 // Big Bang Mini
	 || strncmp(romTid, "AWV", 3) == 0 // Nervous Brickdown
	) {
		ce9->valueBits |= b_cacheDisabled; // Disable card data cache for specific games
	} */
	if (scfgBios9i()) {
		ce9->valueBits |= b_dsiBios;
	}
	if (asyncCardRead) {
		ce9->valueBits |= b_asyncCardRead;
	}
	if (sharedWramEnabled) {
		ce9->valueBits |= b_useSharedRam;
	}
	if (usesCloneboot) {
		ce9->valueBits |= b_cloneboot;
	}
	if (strncmp(romTid, "HND", 3) == 0) {
		ce9->valueBits |= b_isDlp;
	}
	if (strncmp(romTid, "AZE", 3) == 0) { // Zelda: Phantom Hourglass
		ce9->valueBits |= b_bypassExceptionHandler;
	}
	if (!ROMinRAM && dsiWramAccess && !dsiWramMirrored && (ndsHeader->unitCode == 0 || !dsiModeConfirmed) && baseFatSize != 0) {
		const u32 fntFatSize = (baseFatOff-baseFntOff)+baseFatSize;
		if (fntFatSize <= 0x80000) {
			ce9->fntSrc = baseFntOff;
			ce9->fntFatSize = fntFatSize;
			ce9->valueBits |= b_fntFatCached;
		}
	}
	ce9->mainScreen             = mainScreen;
	ce9->overlaysSrc            = (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) ? (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize) : ndsHeader->arm9overlaySource;
	ce9->overlaysSize           = overlaysSize;
	ce9->romPaddingSize         = romPaddingSize;
	ce9->consoleModel           = consoleModel;

	if (!ROMinRAM) {
		//extern bool gbaRomFound;
		extern u8 gameOnFlashcard;
		bool runOverlayCheck = overlayPatch;
		u32 dataToPreloadSizeAligned = 0;
		ce9->cacheBlockSize = cacheBlockSize;
		if (ce9->overlaysSrc) {
			ce9->overlaysSrcAlign = (ce9->overlaysSrc/ce9->cacheBlockSize)*ce9->cacheBlockSize;
			for (u32 i = ce9->overlaysSrcAlign; i < ndsHeader->arm7romOffset; i+= ce9->cacheBlockSize) {
				ce9->overlaysSizeAlign += ce9->cacheBlockSize;
			}
		}
		if (ndsHeader->unitCode > 0 && dsiMode) {
			extern u32 cheatSizeTotal;
			const bool cheatsEnabled = (cheatSizeTotal > 4 && cheatSizeTotal <= 0x8000);
			const bool specialTitle = (strncmp(romTid, "V2G", 3) == 0 || strncmp(romTid, "DD3", 3) == 0);
			const bool pkmnTitle = (strncmp(romTid, "IRB", 3) == 0 || strncmp(romTid, "IRA", 3) == 0 || strncmp(romTid, "IRE", 3) == 0 || strncmp(romTid, "IRD", 3) == 0);

			ce9->cacheAddress = (consoleModel > 0 ? dev_CACHE_ADRESS_START_TWLSDK : (cheatsEnabled ? retail_CACHE_ADRESS_START_TWLSDK_CHEAT : retail_CACHE_ADRESS_START_TWLSDK));
			if (consoleModel == 0 && !gameOnFlashcard) {
				if (pkmnTitle) {
					ce9->cacheAddress = (cheatsEnabled ? retail_CACHE_ADRESS_START_TWLSDK_LARGE_CHEAT : retail_CACHE_ADRESS_START_TWLSDK_LARGE);
				} else if (specialTitle) {
					ce9->cacheAddress = (cheatsEnabled ? retail_CACHE_ADRESS_START_TWLSDK_SMALL_CHEAT : retail_CACHE_ADRESS_START_TWLSDK_SMALL);
				}
			}
			ce9->romLocation = ce9->cacheAddress;
			ce9->cacheSlots = (consoleModel > 0 ? (cheatsEnabled ? dev_CACHE_ADRESS_SIZE_TWLSDK_CHEAT : dev_CACHE_ADRESS_SIZE_TWLSDK) : (cheatsEnabled ? retail_CACHE_ADRESS_SIZE_TWLSDK_CHEAT : retail_CACHE_ADRESS_SIZE_TWLSDK))/cacheBlockSize;
			if (consoleModel == 0 && !gameOnFlashcard) {
				if (pkmnTitle) {
					ce9->cacheSlots = (cheatsEnabled ? retail_CACHE_ADRESS_SIZE_TWLSDK_LARGE_CHEAT : retail_CACHE_ADRESS_SIZE_TWLSDK_LARGE)/cacheBlockSize;
				} else if (specialTitle) {
					ce9->cacheSlots = (cheatsEnabled ? retail_CACHE_ADRESS_SIZE_TWLSDK_SMALL_CHEAT : retail_CACHE_ADRESS_SIZE_TWLSDK_SMALL)/cacheBlockSize;
				}
			}
		} else {
			extern bool hasVramWifiBinary;
			const u32 start = (hasVramWifiBinary ? CACHE_ADRESS_START_ALT : CACHE_ADRESS_START);
			u32 size = 0;
			if (strncmp(romTid, "UBR", 3) == 0) {
				runOverlayCheck = false;
				ce9->cacheAddress = start;
				size = retail_CACHE_ADRESS_SIZE_BROWSER;
			} else {
				ce9->cacheAddress = (dsiMode ? CACHE_ADRESS_START_DSIMODE : start);
				if (dsiMode) {
					size = (consoleModel > 0 ? dev_CACHE_ADRESS_SIZE_DSIMODE : retail_CACHE_ADRESS_SIZE_DSIMODE);
				} else {
					size = (consoleModel > 0 ? dev_CACHE_ADRESS_SIZE : retail_CACHE_ADRESS_SIZE);
				}
			}
			ce9->romLocation = ce9->cacheAddress;
			ce9->cacheSlots = size/cacheBlockSize;
		}
		if (dataToPreloadFound(ndsHeader)) {
			//ce9->romLocation[1] = ce9->romLocation[0]+dataToPreloadSize[0];
			// ce9->romLocation -= dataToPreloadAddr[0];
			//ce9->romLocation[1] -= dataToPreloadAddr[1];
			configureRomMap(ce9, ndsHeader, dataToPreloadAddr, dsiMode);
			for (u32 i = 0; i < dataToPreloadSize/*+dataToPreloadSize[1]*/; i += cacheBlockSize) {
				ce9->cacheAddress += cacheBlockSize;
				romLocationAdjust(ndsHeader, laterSdk, (ce9->valueBits & b_isSdk5), (ce9->valueBits & b_dsiBios), &ce9->cacheAddress);
				dataToPreloadSizeAligned += cacheBlockSize;
			}
			ce9->cacheSlots -= dataToPreloadSizeAligned/cacheBlockSize;
			ce9->romPartSrc = dataToPreloadAddr;
			//ce9->romPartSrc[1] = dataToPreloadAddr[1];
			ce9->romPartSize = dataToPreloadSize;
			//ce9->romPartSize[1] = dataToPreloadSize[1];
			/* if (dataToPreloadFrame) {
				ce9->valueBits |= b_waitForPreloadToFinish;
			} */
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
		if (strncmp(romTid, "UBR", 3) == 0 || iUncompressedSize > 0x26C000) {
			ce9->valueBits |= b_slowSoftReset;
		}
		if (!dataToPreloadFound(ndsHeader) && (strncmp(romTid, "YPT", 3) == 0)) { // Puppy Palace
			ce9->valueBits |= b_resetOnFirstException;
		}

		if (ndsHeader->unitCode == 0 || !dsiMode) {
			u32* cacheAddressTable = (u32*)(!laterSdk ? CACHE_ADDRESS_TABLE_LOCATION2 : CACHE_ADDRESS_TABLE_LOCATION);
			u32 addr = ce9->cacheAddress;

			for (int slot = 0; slot < ce9->cacheSlots; slot++) {
				romLocationAdjust(ndsHeader, laterSdk, (ce9->valueBits & b_isSdk5), (ce9->valueBits & b_dsiBios), &addr);
				cacheAddressTable[slot] = addr;
				addr += cacheBlockSize;
			}
		}
	} else {
		u32 romOffset = 0;
		if (usesCloneboot) {
			romOffset = 0x4000;
		} else if (ndsHeader->arm9overlaySource == 0 || ndsHeader->arm9overlaySize == 0) {
			romOffset = (ndsHeader->arm7romOffset + ndsHeader->arm7binarySize);
		} else if (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) {
			romOffset = (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
		} else {
			romOffset = ndsHeader->arm9overlaySource;
		}
		configureRomMap(ce9, ndsHeader, romOffset, dsiMode);
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

int hookNdsRetailArm9Mini(cardengineArm9* ce9, const tNDSHeader* ndsHeader, s32 mainScreen, u8 consoleModel) {
	ce9->mainScreen             = mainScreen;
	ce9->consoleModel           = consoleModel;
	ce9->valueBits |= b_enableExceptionHandler;

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
