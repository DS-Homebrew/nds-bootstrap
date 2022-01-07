#include <string.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"
#include "debug_file.h"
#include "tonccpy.h"

//bool cardReadFound = false; // patch_common.c

static bool patchCardRead(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumbPtr, int* readTypePtr, int* sdk5ReadTypePtr, u32** cardReadEndOffsetPtr, u32 startOffset) {
	bool usesThumb = patchOffsetCache.a9IsThumb;
	int readType = 0;
	int sdk5ReadType = 0; // SDK 5

	// Card read
	// SDK 5
	//dbg_printf("Trying SDK 5 thumb...\n");
    u32* cardReadEndOffset = patchOffsetCache.cardReadEndOffset;
	if (!patchOffsetCache.cardReadEndOffset) {
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type1(ndsHeader, moduleParams, startOffset);
		if (cardReadEndOffset) {
			sdk5ReadType = 1;
			usesThumb = true;
			patchOffsetCache.a9IsThumb = usesThumb;
		}
		if (!cardReadEndOffset) {
			// SDK 5
			cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type0(ndsHeader, moduleParams, startOffset);
			if (cardReadEndOffset) {
				sdk5ReadType = 0;
				usesThumb = true;
				patchOffsetCache.a9IsThumb = usesThumb;
			}
		}
		if (!cardReadEndOffset) {
			//dbg_printf("Trying thumb...\n");
			cardReadEndOffset = (u32*)findCardReadEndOffsetThumb(ndsHeader, startOffset);
			if (cardReadEndOffset) {
				usesThumb = true;
				patchOffsetCache.a9IsThumb = usesThumb;
			}
		}
		if (!cardReadEndOffset) {
			cardReadEndOffset = findCardReadEndOffsetType0(ndsHeader, moduleParams, startOffset);
		}
		if (!cardReadEndOffset) {
			//dbg_printf("Trying alt...\n");
			cardReadEndOffset = findCardReadEndOffsetType1(ndsHeader, startOffset);
			if (cardReadEndOffset) {
				readType = 1;
				if (*(cardReadEndOffset - 1) == 0xFFFFFE00) {
					dbg_printf("Found thumb\n\n");
					--cardReadEndOffset;
					usesThumb = true;
					patchOffsetCache.a9IsThumb = usesThumb;
				}
			}
		}
		if (cardReadEndOffset) {
			patchOffsetCache.cardReadEndOffset = cardReadEndOffset;
		}
	}
	*usesThumbPtr = usesThumb;
	*readTypePtr = readType;
	*sdk5ReadTypePtr = sdk5ReadType; // SDK 5
	*cardReadEndOffsetPtr = cardReadEndOffset;
	if (!cardReadEndOffset) { // Not necessarily needed
		return false;
	}
	u32* cardReadStartOffset = patchOffsetCache.cardReadStartOffset;
	if (!patchOffsetCache.cardReadStartOffset) {
		// SDK 5
		//dbg_printf("Trying SDK 5 thumb...\n");
		if (sdk5ReadType == 0) {
			cardReadStartOffset = (u32*)findCardReadStartOffsetThumb5Type0(moduleParams, (u16*)cardReadEndOffset);
		} else {
			cardReadStartOffset = (u32*)findCardReadStartOffsetThumb5Type1(moduleParams, (u16*)cardReadEndOffset);
		}
		if (!cardReadStartOffset) {
			//dbg_printf("Trying thumb...\n");
			cardReadStartOffset = (u32*)findCardReadStartOffsetThumb((u16*)cardReadEndOffset);
		}
		if (!cardReadStartOffset) {
			//dbg_printf("Trying SDK 5...\n");
			cardReadStartOffset = (u32*)findCardReadStartOffset5(moduleParams, cardReadEndOffset);
		}
		if (!cardReadStartOffset) {
			if (readType == 0) {
				cardReadStartOffset = findCardReadStartOffsetType0(moduleParams, cardReadEndOffset);
			} else {
				cardReadStartOffset = findCardReadStartOffsetType1(cardReadEndOffset);
			}
		}
		if (cardReadStartOffset) {
			patchOffsetCache.cardReadStartOffset = cardReadStartOffset;
		}
	}
	if (!cardReadStartOffset) {
		return false;
	}
	//cardReadFound = true;

	// Card struct
	u32** cardStruct = (u32**)(cardReadEndOffset - 1);
	u32* cardStructPatch = (usesThumb ? ce9->thumbPatches->cardStructArm9 : ce9->patches->cardStructArm9);
	if (moduleParams->sdk_version > 0x3000000) {
		// Save card struct
		ce9->cardStruct0 = (u32)(*cardStruct + 7);
		*cardStructPatch = (u32)(*cardStruct + 7); // Cache management alternative
	} else {
		// Save card struct
		ce9->cardStruct0 = (u32)(*cardStruct + 6);
		*cardStructPatch = (u32)(*cardStruct + 6); // Cache management alternative
	}

	// Patch
	u32* cardReadPatch = (usesThumb ? ce9->thumbPatches->card_read_arm9 : ce9->patches->card_read_arm9);
	tonccpy(cardReadStartOffset, cardReadPatch, usesThumb ? (isSdk5(moduleParams) ? 0xB0 : 0xA0) : 0xE0); // 0xE0 = 0xF0 - 0x08
    dbg_printf("cardRead location : ");
    dbg_hexa((u32)cardReadStartOffset);
    dbg_printf("\n");
    dbg_hexa((u32)ce9);
    dbg_printf("\n\n");
	return true;
}

static void patchCardPullOut(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, int sdk5ReadType, u32** cardPullOutOffsetPtr) {
	// Card pull out
	u32* cardPullOutOffset = patchOffsetCache.cardPullOutOffset;
	if (!patchOffsetCache.cardPullOutOffset) {
		cardPullOutOffset = NULL;
		if (usesThumb) {
			//dbg_printf("Trying SDK 5 thumb...\n");
			if (sdk5ReadType == 0) {
				cardPullOutOffset = (u32*)findCardPullOutOffsetThumb5Type0(ndsHeader, moduleParams);
			} else {
				cardPullOutOffset = (u32*)findCardPullOutOffsetThumb5Type1(ndsHeader, moduleParams);
			}
			if (!cardPullOutOffset) {
				//dbg_printf("Trying thumb...\n");
				cardPullOutOffset = (u32*)findCardPullOutOffsetThumb(ndsHeader);
			}
		} else {
			cardPullOutOffset = findCardPullOutOffset(ndsHeader, moduleParams);
		}
		if (cardPullOutOffset) {
			patchOffsetCache.cardPullOutOffset = cardPullOutOffset;
		}
	}
	*cardPullOutOffsetPtr = cardPullOutOffset;
	if (!cardPullOutOffset) {
		return;
	}

	// Patch
	u32* cardPullOutPatch = (usesThumb ? ce9->thumbPatches->card_pull : ce9->patches->card_pull);
	tonccpy(cardPullOutOffset, cardPullOutPatch, 0x4);
    dbg_printf("cardPullOut location : ");
    dbg_hexa((u32)cardPullOutOffset);
    dbg_printf("\n\n");
}

static void patchCacheFlush(cardengineArm9* ce9, bool usesThumb, u32* cardPullOutOffset) {
	if (!cardPullOutOffset) {
		return;
	}

	// Patch
	u32* cacheFlushPatch = (usesThumb ? ce9->thumbPatches->cacheFlushRef : ce9->patches->cacheFlushRef);
	*cacheFlushPatch = (u32)cardPullOutOffset + 13;
}

/*static void patchForceToPowerOff(cardengineArm9* ce9, const tNDSHeader* ndsHeader, bool usesThumb) {
	// Force to power off
	u32* forceToPowerOffOffset = findForceToPowerOffOffset(ndsHeader);
	if (!forceToPowerOffOffset) {
		return;
	}
	// Patch
	u32* cardPullOutPatch = (usesThumb ? ce9->thumbPatches->card_pull : ce9->patches->card_pull);
	tonccpy(forceToPowerOffOffset, cardPullOutPatch, 0x4);
}*/

static void patchCardId(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return;
	}

	// Card ID
	u32* cardIdStartOffset = patchOffsetCache.cardIdOffset;
	if (!patchOffsetCache.cardIdChecked) {
		cardIdStartOffset = NULL;
		u32* cardIdEndOffset = NULL;
		if (usesThumb) {
			cardIdEndOffset = (u32*)findCardIdEndOffsetThumb(ndsHeader, moduleParams, (u16*)cardReadEndOffset);
			cardIdStartOffset = (u32*)findCardIdStartOffsetThumb(moduleParams, (u16*)cardIdEndOffset);
		} else {
			cardIdEndOffset = findCardIdEndOffset(ndsHeader, moduleParams, cardReadEndOffset);
			cardIdStartOffset = findCardIdStartOffset(moduleParams, cardIdEndOffset);
		}
		if (cardIdStartOffset) {
			patchOffsetCache.cardIdOffset = cardIdStartOffset;
		}
		patchOffsetCache.cardIdChecked = true;
	}

	if (cardIdStartOffset) {
        // Patch
		u32* cardIdPatch = (usesThumb ? ce9->thumbPatches->card_id_arm9 : ce9->patches->card_id_arm9);

		cardIdPatch[usesThumb ? 1 : 2] = getChipId(ndsHeader, moduleParams);
		tonccpy(cardIdStartOffset, cardIdPatch, usesThumb ? 0x8 : 0xC);
		dbg_printf("cardId location : ");
		dbg_hexa((u32)cardIdStartOffset);
		dbg_printf("\n\n");
	}
}

static void patchCardReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	// Card read dma
	u32* cardReadDmaStartOffset = patchOffsetCache.cardReadDmaOffset;
	if (!patchOffsetCache.cardReadDmaChecked) {
		cardReadDmaStartOffset = NULL;
		u32* cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader, moduleParams);
		if (!cardReadDmaEndOffset && usesThumb) {
			cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
		}
		if (usesThumb) {
			cardReadDmaStartOffset = (u32*)findCardReadDmaStartOffsetThumb((u16*)cardReadDmaEndOffset);
		} else {
			cardReadDmaStartOffset = findCardReadDmaStartOffset(moduleParams, cardReadDmaEndOffset);
		}
		if (cardReadDmaStartOffset) {
			patchOffsetCache.cardReadDmaOffset = cardReadDmaStartOffset;
		}
		patchOffsetCache.cardReadDmaChecked = true;
	}
	if (!cardReadDmaStartOffset) {
		return;
	}
	// Patch
	u32* cardReadDmaPatch = (usesThumb ? ce9->thumbPatches->card_dma_arm9 : ce9->patches->card_dma_arm9);
	tonccpy(cardReadDmaStartOffset, cardReadDmaPatch, 0x40);
    dbg_printf("cardReadDma location : ");
    dbg_hexa((u32)cardReadDmaStartOffset);
    dbg_printf("\n\n");
}

static void patchReset(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {    
    u32* reset = patchOffsetCache.resetOffset;

    if (!patchOffsetCache.resetChecked) {
		reset = findResetOffset(ndsHeader,moduleParams);
		if (reset) patchOffsetCache.resetOffset = reset;
		patchOffsetCache.resetChecked = true;
	}

	if (reset) {
		// Patch
		u32* resetPatch = ce9->patches->reset_arm9;
		tonccpy(reset, resetPatch, 0x40);
		dbg_printf("reset location : ");
		dbg_hexa((u32)reset);
		dbg_printf("\n\n");
	}
}

static bool patchCardIrqEnable(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	bool usesThumb = patchOffsetCache.a9CardIrqIsThumb;

	// Card irq enable
	u32* cardIrqEnableOffset = patchOffsetCache.a9CardIrqEnableOffset;
	if (!patchOffsetCache.a9CardIrqEnableOffset) {
		cardIrqEnableOffset = a9FindCardIrqEnableOffset(ndsHeader, moduleParams, &usesThumb);
		if (cardIrqEnableOffset) {
			patchOffsetCache.a9CardIrqEnableOffset = cardIrqEnableOffset;
			patchOffsetCache.a9CardIrqIsThumb = usesThumb;
		}
	}
	if (!cardIrqEnableOffset) {
		return false;
	}
	u32* cardIrqEnablePatch = (usesThumb ? ce9->thumbPatches->card_irq_enable : ce9->patches->card_irq_enable);
	tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, usesThumb ? 0x20 : 0x30);
    dbg_printf("cardIrqEnable location : ");
    dbg_hexa((u32)cardIrqEnableOffset);
    dbg_printf("\n\n");
	return true;
}

static bool mpuInitCachePatched = false;

static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	if (ndsHeader->unitCode == 3 || !extendedMemory2 || patchMpuRegion == 2) {
		return;
	}

	if (patchOffsetCache.patchMpuRegion != patchMpuRegion) {
		patchOffsetCache.patchMpuRegion = 0;
		patchOffsetCache.mpuStartOffset = 0;
		patchOffsetCache.mpuDataOffset = 0;
		patchOffsetCache.mpuInitOffset = 0;
		patchOffsetCacheChanged = true;
	}

	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	// Find the mpu init
	u32* mpuStartOffset = patchOffsetCache.mpuStartOffset;
	u32* mpuDataOffset = patchOffsetCache.mpuDataOffset;
	if (!patchOffsetCache.mpuStartOffset) {
		mpuStartOffset = findMpuStartOffset(ndsHeader, patchMpuRegion);
	}
	if (mpuStartOffset) {
		dbg_printf("Mpu start: ");
		dbg_hexa((u32)mpuStartOffset);
		dbg_printf("\n\n");
	}
	if (!patchOffsetCache.mpuDataOffset) {
		mpuDataOffset = findMpuDataOffset(moduleParams, patchMpuRegion, mpuStartOffset);
	}
	if (mpuDataOffset) {
		// Change the region 1 configuration

		u32 mpuInitRegionNewData = PAGE_32M | 0x02000000 | 1;
		u32 mpuNewDataAccess     = 0;
		u32 mpuNewInstrAccess    = 0;
		int mpuAccessOffset      = 0;
		switch (patchMpuRegion) {
			case 0:
				mpuInitRegionNewData = PAGE_128M | 0x00000000 | 1;
				break;
			case 3:
				mpuInitRegionNewData = PAGE_32M | 0x08000000 | 1;
				mpuNewInstrAccess    = 0x5111111;
				mpuAccessOffset      = 5;
				break;
		}

		unpatchedFuncs->mpuDataOffset = mpuDataOffset;
		unpatchedFuncs->mpuInitRegionOldData = *mpuDataOffset;
		*mpuDataOffset = mpuInitRegionNewData;

		if (mpuAccessOffset) {
			unpatchedFuncs->mpuAccessOffset = mpuAccessOffset;
			if (mpuNewInstrAccess) {
				unpatchedFuncs->mpuOldInstrAccess = mpuDataOffset[mpuAccessOffset];
				mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
			}
			if (mpuNewDataAccess) {
				unpatchedFuncs->mpuOldDataAccess = mpuDataOffset[mpuAccessOffset + 1];
				mpuDataOffset[mpuAccessOffset + 1] = mpuNewDataAccess;
			}
		}

		dbg_printf("Mpu data: ");
		dbg_hexa((u32)mpuDataOffset);
		dbg_printf("\n\n");
	}

	// Find the mpu cache init
	u32* mpuInitCacheOffset = patchOffsetCache.mpuInitCacheOffset;
	if (!patchOffsetCache.mpuInitCacheOffset) {
		mpuInitCacheOffset = findMpuInitCacheOffset(mpuStartOffset);
	}
	if (mpuInitCacheOffset) {
		unpatchedFuncs->mpuInitCacheOffset = mpuInitCacheOffset;
		unpatchedFuncs->mpuInitCacheOld = *mpuInitCacheOffset;
		*mpuInitCacheOffset = 0xE3A00046;
		mpuInitCachePatched = true;
		patchOffsetCache.mpuInitCacheOffset = mpuInitCacheOffset;

		dbg_printf("Mpu init cache: ");
		dbg_hexa((u32)mpuInitCacheOffset);
		dbg_printf("\n\n");
	}

	// Patch out all further mpu reconfiguration
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	u32* mpuInitOffset = patchOffsetCache.mpuInitOffset;
	if (!mpuInitOffset) {
		mpuInitOffset = (u32*)mpuStartOffset;
	}
	extern u32 iUncompressedSize;
	u32 patchSize = iUncompressedSize;
	if (mpuInitOffset == mpuStartOffset) {
		mpuInitOffset = findOffset(
			//(u32*)((u32)mpuStartOffset + 4), patchSize,
			mpuInitOffset + 1, patchSize,
			mpuInitRegionSignature, 1
		);
	}
	if (mpuInitOffset) {
		dbg_printf("Mpu init: ");
		dbg_hexa((u32)mpuInitOffset);
		dbg_printf("\n\n");

		*mpuInitOffset = 0xE1A00000; // nop

		// Try to find it
		/*for (int i = 0; i < 0x100; i++) {
			mpuDataOffset += i;
			if ((*mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
				*mpuDataOffset = PAGE_32M | 0x02000000 | 1;
				break;
			}
			if (i == 100) {
				*mpuStartOffset = 0xE1A00000;
			}
		}*/
	}

	if (!isSdk5(moduleParams)) {
		u32* mpuDataOffsetAlt = patchOffsetCache.mpuDataOffsetAlt;
		if (!patchOffsetCache.mpuDataOffsetAlt) {
			mpuDataOffsetAlt = findMpuDataOffsetAlt(ndsHeader);
			if (mpuDataOffsetAlt) {
				patchOffsetCache.mpuDataOffsetAlt = mpuDataOffsetAlt;
			}
		}
		if (mpuDataOffsetAlt) {
			unpatchedFuncs->mpuDataOffsetAlt = mpuDataOffsetAlt;
			unpatchedFuncs->mpuInitRegionOldDataAlt = *mpuDataOffsetAlt;

			*mpuDataOffsetAlt = PAGE_32M | 0x02000000 | 1;

			dbg_printf("Mpu data alt: ");
			dbg_hexa((u32)mpuDataOffsetAlt);
			dbg_printf("\n\n");
		}
	}

	patchOffsetCache.patchMpuRegion = patchMpuRegion;
	patchOffsetCache.mpuStartOffset = mpuStartOffset;
	patchOffsetCache.mpuDataOffset = mpuDataOffset;
	patchOffsetCache.mpuInitOffset = mpuInitOffset;
}

static void patchMpu2(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x2008000 || moduleParams->sdk_version > 0x5000000) {
		return;
	}

	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	// Find the mpu init
	u32* mpuStartOffset = patchOffsetCache.mpuStartOffset2;
	if (mpuStartOffset) {
		dbg_printf("Mpu start 2: ");
		dbg_hexa((u32)mpuStartOffset);
		dbg_printf("\n\n");
	}
	u32* mpuDataOffset = patchOffsetCache.mpuDataOffset2;
	if (!patchOffsetCache.mpuStartOffset2) {
		mpuStartOffset = findMpuStartOffset(ndsHeader, 2);
	}
	if (!patchOffsetCache.mpuDataOffset2) {
		mpuDataOffset = findMpuDataOffset(moduleParams, 2, mpuStartOffset);
	}
	if (mpuDataOffset) {
		// Change the region 2 configuration (Makes loading slow, so new code is used)

		/*u32 mpuInitRegionNewData = PAGE_32M | 0x02000000 | 1;
		u32 mpuNewDataAccess = 0x15111111;
		u32 mpuNewInstrAccess = 0x5111111;
		int mpuAccessOffset = 6;

		*mpuDataOffset = mpuInitRegionNewData;

		if (mpuNewInstrAccess) {
			mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
		}
		if (mpuNewDataAccess) {
			mpuDataOffset[mpuAccessOffset + 1] = mpuNewDataAccess;
		}*/
		unpatchedFuncs->mpuDataOffset2 = mpuDataOffset;
		unpatchedFuncs->mpuInitRegionOldData2 = *mpuDataOffset;
		*mpuDataOffset = 0;

		dbg_printf("Mpu data 2: ");
		dbg_hexa((u32)mpuDataOffset);
		dbg_printf("\n\n");
	}

	// Find the mpu cache init
	u32* mpuInitCacheOffset = patchOffsetCache.mpuInitCacheOffset;
	if (!mpuInitCachePatched) {
		mpuInitCacheOffset = findMpuInitCacheOffset(mpuStartOffset);
	}
	if (mpuInitCacheOffset && !mpuInitCachePatched) {
		unpatchedFuncs->mpuInitCacheOffset = mpuInitCacheOffset;
		unpatchedFuncs->mpuInitCacheOld = *mpuInitCacheOffset;
		*mpuInitCacheOffset = 0xE3A00046;
		mpuInitCachePatched = true;
		patchOffsetCache.mpuInitCacheOffset = mpuInitCacheOffset;

		dbg_printf("Mpu init cache: ");
		dbg_hexa((u32)mpuInitCacheOffset);
		dbg_printf("\n\n");
	}

	// Patch out all further mpu reconfiguration
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(2);
	u32* mpuInitOffset = patchOffsetCache.mpuInitOffset2;
	if (!mpuInitOffset) {
		mpuInitOffset = (u32*)mpuStartOffset;
	}
	extern u32 iUncompressedSize;
	u32 patchSize = iUncompressedSize;
	if (mpuInitOffset == mpuStartOffset) {
		mpuInitOffset = findOffset(
			//(u32*)((u32)mpuStartOffset + 4), patchSize,
			mpuInitOffset + 1, patchSize,
			mpuInitRegionSignature, 1
		);
	}
	if (mpuInitOffset) {
		dbg_printf("Mpu init 2: ");
		dbg_hexa((u32)mpuInitOffset);
		dbg_printf("\n\n");

		*mpuInitOffset = 0xE1A00000; // nop

		// Try to find it
		/*for (int i = 0; i < 0x100; i++) {
			mpuDataOffset += i;
			if ((*mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
				*mpuDataOffset = PAGE_32M | 0x02000000 | 1;
				break;
			}
			if (i == 100) {
				*mpuStartOffset = 0xE1A00000;
			}
		}*/
	}

	patchOffsetCache.mpuStartOffset2 = mpuStartOffset;
	patchOffsetCache.mpuDataOffset2 = mpuDataOffset;
	patchOffsetCache.mpuInitOffset2 = mpuInitOffset;
}

/*u32* patchLoHeapPointer(const module_params_t* moduleParams, const tNDSHeader* ndsHeader, u32 saveSize) {
	if (moduleParams->sdk_version < 0x2008000) {
		return 0;
	}

	u32* heapPointer = NULL;
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 1) {
		patchOffsetCache.heapPointerOffset = 0;
	} else {
		heapPointer = patchOffsetCache.heapPointerOffset;
	}
	if (!patchOffsetCache.heapPointerOffset) {
		heapPointer = findHeapPointerOffset(moduleParams, ndsHeader);
	}
    if(!heapPointer || *heapPointer<0x02000000 || *heapPointer>0x03000000) {
        dbg_printf("ERROR: Wrong lo heap pointer\n");
        dbg_printf("heap pointer value: ");
	    dbg_hexa(*heapPointer);    
		dbg_printf("\n\n");
        return 0;
    } else if (!patchOffsetCache.heapPointerOffset) {
		patchOffsetCache.heapPointerOffset = heapPointer;
		patchOffsetCacheChanged = true;
	}

    u32* oldheapPointer = (u32*)*heapPointer;

    dbg_printf("old lo heap pointer: ");
	dbg_hexa((u32)oldheapPointer);
    dbg_printf("\n\n");
    
	u32 shrinkSize = 0;
	switch (ndsHeader->deviceSize) {
		case 0x09:
		default:
			shrinkSize = 0x2000;	// 0x4000000
			break;
		case 0x0A:
			shrinkSize = 0x4000;	// 0x8000000
			break;
	}

	*heapPointer += shrinkSize; // shrink heap by FAT table cache size

    dbg_printf("new lo heap pointer: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Lo Heap Shrink Sucessfull\n\n");

    return oldheapPointer;
}*/

void patchHiHeapPointer(cardengineArm9* ce9, const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	extern u32 arm7mbk;
	extern bool expansionPakFound;
	const char* romTid = getRomTid(ndsHeader);

	if (extendedMemory2
	|| moduleParams->sdk_version < 0x2008000
	|| strncmp(romTid, "VSO", 3) == 0
	|| arm7mbk == 0x080037C0) {
		return;
	}

	u32* heapPointer = NULL;
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 1) {
		patchOffsetCache.heapPointer2Offset = 0;
	} else {
		heapPointer = patchOffsetCache.heapPointer2Offset;
	}
	if (!patchOffsetCache.heapPointer2Offset) {
		heapPointer = findHeapPointer2Offset(moduleParams, ndsHeader);
	}
	if(!heapPointer || *heapPointer<0x02000000 || *heapPointer>0x03000000) {
        dbg_printf("ERROR: Wrong heap pointer\n");
        dbg_printf("heap pointer value: ");
	    dbg_hexa(*heapPointer);    
		dbg_printf("\n\n");
        return;
    } else if (!patchOffsetCache.heapPointer2Offset) {
		patchOffsetCache.heapPointer2Offset = heapPointer;
		patchOffsetCacheChanged = true;
	}

    u32* oldheapPointer = (u32*)*heapPointer;

    dbg_printf("old hi heap end pointer: ");
	dbg_hexa((u32)oldheapPointer);
    dbg_printf("\n\n");

	*heapPointer = expansionPakFound ? (u32)ce9 : 0x023C0000; // shrink heap by 128KB (or ce9 binary size, if expansion pak is found)

    dbg_printf("new hi heap pointer: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Hi Heap Shrink Sucessfull\n\n");
}

void patchHiHeapDSiWare(u32 addr, u32 opCode) {
	*(u32*)(addr) = opCode; // mov r0, #0x????????
	*(u32*)(addr+0x24) = 0xE3500001; // cmp r0, #1
	*(u32*)(addr+0x2C) = 0x13A00627; // movne r0, #0x2700000
}

void patchHiHeapDSiWareThumb(u32 addr, u16 opCode1, u16 opCode2) {
	*(u16*)(addr) = opCode1; // movs r0, #0x????????
	*(u16*)(addr+0x2) = opCode2;
	*(u16*)(addr+0x1A) = 0x2801; // cmp r0, #1
	*(u16*)(addr+0x22) = 0x2027; // movne r0, #0x2700000
}

/*void relocate_ce9(u32 default_location, u32 current_location, u32 size) {
    dbg_printf("relocate_ce9\n");
    
    u32 location_sig[1] = {default_location};
    
    u32* firstCardLocation =  findOffset(current_location, size, location_sig, 1);
	if (!firstCardLocation) {
		return;
	}
    dbg_printf("firstCardLocation ");
	dbg_hexa((u32)firstCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*firstCardLocation);
    dbg_printf("\n\n");
    
    *firstCardLocation = current_location;
    
	u32* armReadCardLocation = findOffset(current_location, size, location_sig, 1);
	if (!armReadCardLocation) {
		return;
	}
    dbg_printf("armReadCardLocation ");
	dbg_hexa((u32)armReadCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadCardLocation);
    dbg_printf("\n\n");
    
    *armReadCardLocation = current_location;
    
    u32* thumbReadCardLocation =  findOffset(current_location, size, location_sig, 1);
	if (!thumbReadCardLocation) {
		return;
	}
    dbg_printf("thumbReadCardLocation ");
	dbg_hexa((u32)thumbReadCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadCardLocation);
    dbg_printf("\n\n");
    
    *thumbReadCardLocation = current_location;
    
    u32* globalCardLocation =  findOffset(current_location, size, location_sig, 1);
	if (!globalCardLocation) {
		return;
	}
    dbg_printf("globalCardLocation ");
	dbg_hexa((u32)globalCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*globalCardLocation);
    dbg_printf("\n\n");
    
    *globalCardLocation = current_location;
    
    // fix the header pointer
    cardengineArm9* ce9 = (cardengineArm9*) current_location;
    ce9->patches = (cardengineArm9Patches*)((u32)ce9->patches - default_location + current_location);
    
    dbg_printf(" ce9->patches ");
	dbg_hexa((u32) ce9->patches);
    dbg_printf("\n\n");
    
    ce9->thumbPatches = (cardengineArm9ThumbPatches*)((u32)ce9->thumbPatches - default_location + current_location);
    ce9->patches->card_read_arm9 = (u32*)((u32)ce9->patches->card_read_arm9 - default_location + current_location);
    ce9->patches->card_pull_out_arm9 = (u32*)((u32)ce9->patches->card_pull_out_arm9 - default_location + current_location);
    ce9->patches->card_id_arm9 = (u32*)((u32)ce9->patches->card_id_arm9 - default_location + current_location);
    ce9->patches->card_dma_arm9 = (u32*)((u32)ce9->patches->card_dma_arm9 - default_location + current_location);
    ce9->patches->cardStructArm9 = (u32*)((u32)ce9->patches->cardStructArm9 - default_location + current_location);
    ce9->patches->card_pull = (u32*)((u32)ce9->patches->card_pull - default_location + current_location);
    ce9->patches->cacheFlushRef = (u32*)((u32)ce9->patches->cacheFlushRef - default_location + current_location);
    ce9->patches->readCachedRef = (u32*)((u32)ce9->patches->readCachedRef - default_location + current_location);
    ce9->thumbPatches->card_read_arm9 = (u32*)((u32)ce9->thumbPatches->card_read_arm9 - default_location + current_location);
    ce9->thumbPatches->card_pull_out_arm9 = (u32*)((u32)ce9->thumbPatches->card_pull_out_arm9 - default_location + current_location);
    ce9->thumbPatches->card_id_arm9 = (u32*)((u32)ce9->thumbPatches->card_id_arm9 - default_location + current_location);
    ce9->thumbPatches->card_dma_arm9 = (u32*)((u32)ce9->thumbPatches->card_dma_arm9 - default_location + current_location);
    ce9->thumbPatches->cardStructArm9 = (u32*)((u32)ce9->thumbPatches->cardStructArm9 - default_location + current_location);
    ce9->thumbPatches->card_pull = (u32*)((u32)ce9->thumbPatches->card_pull - default_location + current_location);
    ce9->thumbPatches->cacheFlushRef = (u32*)((u32)ce9->thumbPatches->cacheFlushRef - default_location + current_location);
    ce9->thumbPatches->readCachedRef = (u32*)((u32)ce9->thumbPatches->readCachedRef - default_location + current_location);
}*/

static void randomPatch(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	const char* romTid = getRomTid(ndsHeader);

	// Random patch
	if (moduleParams->sdk_version > 0x3000000
	&& strncmp(romTid, "AKT", 3) != 0  // Doctor Tendo
	&& strncmp(romTid, "ACZ", 3) != 0  // Cars
	&& strncmp(romTid, "ABC", 3) != 0  // Harvest Moon DS
	&& strncmp(romTid, "AWL", 3) != 0) // TWEWY
	{
		u32* randomPatchOffset = patchOffsetCache.randomPatchOffset;
		if (!patchOffsetCache.randomPatchChecked) {
			randomPatchOffset = findRandomPatchOffset(ndsHeader);
			if (randomPatchOffset) {
				patchOffsetCache.randomPatchOffset = randomPatchOffset;
			}
			patchOffsetCache.randomPatchChecked = true;
		}
		if (randomPatchOffset) {
			// Patch
			//*(u32*)((u32)randomPatchOffset + 0xC) = 0x0;
			*(randomPatchOffset + 3) = 0x0;
		}
	}
}

static u32 iSpeed = 1;

static u32 CalculateOffset(u16* anAddress,u32 aShift)
{
  u32 ptr=(u32)(anAddress+2+aShift);
  ptr&=0xfffffffc;
  ptr+=(anAddress[aShift]&0xff)*sizeof(u32);
  return ptr;
}

void patchDownloadplayArm(u32* aPtr)
{
	*(aPtr) = 0xe59f0000; //ldr r0, [pc]
	*(aPtr+1) = 0xea000000; //b pc
	*(aPtr+2) = iSpeed;
}

void patchDownloadplay(const tNDSHeader* ndsHeader)
{
  u16* top=(u16*)ndsHeader->arm9destination;
  u16* bottom=(u16*)(ndsHeader->arm9destination+0x300000);
  for(u16* ii=top;ii<bottom;++ii)
  {
    //31 6E ?? 48 0? 43 3? 66
    //ldr     r1, [r6,#0x60]
    //ldr     r0, =0x406000
    //orrs    rx, ry
    //str     rx, [r6,#0x60]
    //3245 - 42 All-Time Classics (Europe) (En,Fr,De,Es,It) (Rev 1)
    if(ii[0]==0x6e31&&(ii[1]&0xff00)==0x4800&&(ii[2]&0xfff6)==0x4300&&(ii[3]&0xfffe)==0x6630)
    {
      u32 ptr=CalculateOffset(ii,1);
		*(ii+2) = 0x46c0; //nop
		*(ii+3) = 0x6630; //str r0, [r6,#0x60]
		*(u32*)(ptr) = iSpeed;
      break;
    }
    //01 98 01 6E ?? 48 01 43 01 98 01 66
    //ldr     r0, [sp,#4]
    //ldr     r1, [r0,#0x60]
    //ldr     r0, =0x406000
    //orrs    r1, r0
    //ldr     r0, [sp,#4]
    //str     r1, [r0,#0x60]
    else if(ii[0]==0x9801&&ii[1]==0x6e01&&(ii[2]&0xff00)==0x4800&&ii[3]==0x4301&&ii[4]==0x9801&&ii[5]==0x6601)
    {
      u32 ptr=CalculateOffset(ii,2);
		*(ii+3) = 0x1c01; //mov r1, r0
		*(u32*)(ptr) = iSpeed;
      break;
    }
    else if(0==(((u32)ii)&1))
    {
      u32* buffer32=(u32*)ii;
      //60 00 99 E5 06 0A 80 E3 01 05 80 E3 60 00 89 E5
      //ldr     r0, [r9,#0x60]
      //orr     r0, r0, #0x6000
      //orr     r0, r0, #0x400000
      //str     r0, [r9,#0x60]
      //1606 - Cars - Mater-National Championship (USA)
      //2119 - Nanostray 2 (USA)
      //4512 - Might & Magic - Clash of Heroes (USA) (En,Fr,Es)
      if(buffer32[0]==0xe5990060&&buffer32[1]==0xe3800a06&&buffer32[2]==0xe3800501&&buffer32[3]==0xe5890060)
      {
        patchDownloadplayArm(buffer32);
        break;
      }
      //60 10 99 E5 ?? 0? 9F E5 00 00 81 E1 60 00 89 E5
      //ldr     r1, [r9,#0x60]
      //ldr     r0, =0x406000
      //orr     r0, r1, r0
      //str     r0, [r9,#0x60]
      //3969 - Power Play Pool (Europe) (En,Fr,De,Es,It)
      else if(buffer32[0]==0xe5991060&&(buffer32[1]&0xfffff000)==0xe59f0000&&buffer32[2]==0xe1810000&&buffer32[3]==0xe5890060)
      {
        patchDownloadplayArm(buffer32);
        break;
      }
      //60 20 8? E2 00 ?0 92 E5 ?? 0? 9F E5 00 00 81 E1 00 00 82 E5
      //add     r2, rx, #0x60
      //ldr     ry, [r2]
      //ldr     rz, =0x406000
      //orr     r0, ry, rz
      //str     r0, [r2]
      //0118 - GoldenEye - Rogue Agent (Europe)
      else if((buffer32[0]&0xfff0ffff)==0xe2802060&&(buffer32[1]&0xffffefff)==0xe5920000&&(buffer32[2]&0xffffe000)==0xe59f0000&&(buffer32[3]&0xfffefffe)==0xe1800000&&buffer32[4]==0xe5820000)
      {
        patchDownloadplayArm(buffer32+1);
        break;
      }
    }
  }
}

// SDK 5
static void randomPatch5Second(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	// Random patch SDK 5 second
	u32* randomPatchOffset5Second = patchOffsetCache.randomPatch5SecondOffset;
	if (!patchOffsetCache.randomPatch5SecondChecked) {
		randomPatchOffset5Second = findRandomPatchOffset5Second(ndsHeader);
		if (randomPatchOffset5Second) {
			patchOffsetCache.randomPatch5SecondOffset = randomPatchOffset5Second;
		}
		patchOffsetCache.randomPatch5SecondChecked = true;
	}
	if (!randomPatchOffset5Second) {
		return;
	}
	// Patch
	*randomPatchOffset5Second = 0xE3A00000;
	//*(u32*)((u32)randomPatchOffset5Second + 4) = 0xE12FFF1E;
	*(randomPatchOffset5Second + 1) = 0xE12FFF1E;
}

static void nandSavePatch(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
    u32 sdPatchEntry = 0;

	const char* romTid = getRomTid(ndsHeader);

    // WarioWare: D.I.Y. (USA)
	if (strcmp(romTid, "UORE") == 0) {
		sdPatchEntry = 0x2002c04; 
	}
    // WarioWare: Do It Yourself (Europe)
    if (strcmp(romTid, "UORP") == 0) {
		sdPatchEntry = 0x2002ca4; 
	}
    // Made in Ore (Japan)
    if (strcmp(romTid, "UORJ") == 0) {
		sdPatchEntry = 0x2002be4; 
	}

    if(sdPatchEntry) {   
      //u32 gNandInit(void* data)
      *(u32*)((u8*)sdPatchEntry+0x50C) = 0xe3a00001; //mov r0, #1
      *(u32*)((u8*)sdPatchEntry+0x510) = 0xe12fff1e; //bx lr

      //u32 gNandWait(void)
      *(u32*)((u8*)sdPatchEntry+0xC9C) = 0xe12fff1e; //bx lr

      //u32 gNandState(void)
      *(u32*)((u8*)sdPatchEntry+0xEB0) = 0xe3a00003; //mov r0, #3
      *(u32*)((u8*)sdPatchEntry+0xEB4) = 0xe12fff1e; //bx lr

      //u32 gNandError(void)
      *(u32*)((u8*)sdPatchEntry+0xEC8) = 0xe3a00000; //mov r0, #0
      *(u32*)((u8*)sdPatchEntry+0xECC) = 0xe12fff1e; //bx lr

      //u32 gNandWrite(void* memory,void* flash,u32 size,u32 dma_channel)
      u32* nandWritePatch = ce9->patches->nand_write_arm9;
      tonccpy((u8*)sdPatchEntry+0x958, nandWritePatch, 0x40);

      //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
      u32* nandReadPatch = ce9->patches->nand_read_arm9;
      tonccpy((u8*)sdPatchEntry+0xD24, nandReadPatch, 0x40);
    } else {
        // Jam with the Band (Europe)
        if (strcmp(romTid, "UXBP") == 0) {
          	//u32 gNandInit(void* data)
            *(u32*)(0x020613CC) = 0xe3a00001; //mov r0, #1
            *(u32*)(0x020613D0) = 0xe12fff1e; //bx lr

            //u32 gNandResume(void)
            *(u32*)(0x02061A4C) = 0xe3a00000; //mov r0, #0
            *(u32*)(0x02061A50) = 0xe12fff1e; //bx lr

            //u32 gNandError(void)
            *(u32*)(0x02061C24) = 0xe3a00000; //mov r0, #0
            *(u32*)(0x02061C28) = 0xe12fff1e; //bx lr

            //u32 gNandWrite(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandWritePatch = ce9->patches->nand_write_arm9;
            tonccpy((u32*)0x0206176C, nandWritePatch, 0x40);

            //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandReadPatch = ce9->patches->nand_read_arm9;
            tonccpy((u32*)0x02061AC4, nandReadPatch, 0x40);
    	}
	}
}

static void operaRamPatch(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (dsDebugRam || !extendedMemory2) {
		return;
	}

	// Opera RAM patch (ARM9)
	*(u32*)0x02003D48 = 0x2400000;
	*(u32*)0x02003D4C = 0x2400004;

	*(u32*)0x02010FF0 = 0x2400000;
	*(u32*)0x02010FF4 = 0x24000CE;

	*(u32*)0x020112AC = 0x2400080;

	*(u32*)0x020402BC = 0x24000C2;
	*(u32*)0x020402C0 = 0x24000C0;
	*(u32*)0x020402CC = 0x2FFFFFE;
	*(u32*)0x020402D0 = 0x2800000;
	*(u32*)0x020402D4 = 0x29FFFFF;
	*(u32*)0x020402D8 = 0x2BFFFFF;
	*(u32*)0x020402DC = 0x2FFFFFF;
	*(u32*)0x020402E0 = 0xD7FFFFF;	// ???
	toncset((char*)0x2800000, 0xFF, 0x800000);		// Fill fake MEP with FFs

	// Opera RAM patch (ARM7)
	*(u32*)0x0238C7BC = 0x2400000;
	*(u32*)0x0238C7C0 = 0x24000CE;

	//*(u32*)0x0238C950 = 0x2400000;
}

static void setFlushCache(cardengineArm9* ce9, u32 patchMpuRegion, bool usesThumb) {
	//if (!usesThumb) {
	ce9->patches->needFlushDCCache = (patchMpuRegion == 1);
}

u32 patchCardNdsArm9(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	bool usesThumb;
	int readType;
	int sdk5ReadType; // SDK 5
	u32* cardReadEndOffset;
	u32* cardPullOutOffset;

	if (!patchCardIrqEnable(ce9, ndsHeader, moduleParams)) {
		dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

    const char* romTid = getRomTid(ndsHeader);

    u32 startOffset = (u32)ndsHeader->arm9destination;
    if (strncmp(romTid, "UOR", 3) == 0) { // Start at 0x2003800 for "WarioWare: DIY"
        startOffset = (u32)ndsHeader->arm9destination + 0x3800;
    } else if (strncmp(romTid, "UXB", 3) == 0) { // Start at 0x2080000 for "Jam with the Band"
        startOffset = (u32)ndsHeader->arm9destination + 0x80000;        
    } else if (strncmp(romTid, "USK", 3) == 0) { // Start at 0x20E8000 for "Face Training"
        startOffset = (u32)ndsHeader->arm9destination + 0xE4000;        
    }

    dbg_printf("startOffset : ");
    dbg_hexa(startOffset);
    dbg_printf("\n\n");

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &sdk5ReadType, &cardReadEndOffset, startOffset)) {
		dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, sdk5ReadType, &cardPullOutOffset);

	patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	//patchForceToPowerOff(ce9, ndsHeader, usesThumb);

	patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset);

	patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);

	patchMpu(ndsHeader, moduleParams, patchMpuRegion, patchMpuSize);
	patchMpu2(ndsHeader, moduleParams);

	//patchDownloadplay(ndsHeader);

	patchHiHeapPointer(ce9, moduleParams, ndsHeader);

	patchReset(ce9, ndsHeader, moduleParams);

	randomPatch(ndsHeader, moduleParams);
	randomPatch5Second(ndsHeader, moduleParams);

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch(ndsHeader, moduleParams);
	}

	nandSavePatch(ce9, ndsHeader, moduleParams);
    
	setFlushCache(ce9, patchMpuRegion, usesThumb);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
