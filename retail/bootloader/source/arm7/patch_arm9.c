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
#include "value_bits.h"

#define FEATURE_SLOT_GBA			0x00000010
#define FEATURE_SLOT_NDS			0x00000020

//bool cardReadFound = false; // patch_common.c

bool isPawsAndClaws(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AQ2", 3) == 0 // Paws & Claws: Pet Resort
	 || strncmp(romTid, "YMU", 3) == 0 // Paws & Claws: Pet Vet 2
	) {
		return true;
	}

	return false;
}

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

static bool patchCardReadMvDK4(u32 startOffset) {
	u32* offset = findCardReadCheckOffsetMvDK4(startOffset);
	if (!offset) {
		return false;
	}

	//offset[2] = 0xE3A00001; // mov r0, #1
	offset[3] = 0xE1A00000; // nop
	offset[4] += 0xD0000000; // bne to b

	dbg_printf("cardReadMvDK4 location : ");
	dbg_hexa((u32)offset);
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
	if (!isPawsAndClaws(ndsHeader) && !cardReadEndOffset) {
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
		extern u32 baseChipID;
		u32* cardIdPatch = (usesThumb ? ce9->thumbPatches->card_id_arm9 : ce9->patches->card_id_arm9);

		cardIdPatch[usesThumb ? 1 : 2] = baseChipID;
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
	tonccpy(cardReadDmaStartOffset, cardReadDmaPatch, usesThumb ? 4 : 8);
    dbg_printf("cardReadDma location : ");
    dbg_hexa((u32)cardReadDmaStartOffset);
    dbg_printf("\n\n");
}

static void patchReset(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {    
    u32* reset = patchOffsetCache.resetOffset;

    if (!patchOffsetCache.resetChecked) {
		reset = findResetOffset(ndsHeader,moduleParams, (bool*)&patchOffsetCache.resetMb);
		if (reset) patchOffsetCache.resetOffset = reset;
		patchOffsetCache.resetChecked = true;
	}

	if (!reset) {
		return;
	}

	// Patch
	u32* resetPatch = ce9->patches->reset_arm9;
	tonccpy(reset, resetPatch, 0x40);
	dbg_printf("reset location : ");
	dbg_hexa((u32)reset);
	dbg_printf("\n\n");
}

static void patchResetTwl(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 accessControl;
	if (!(accessControl & BIT(4))) {
		return;
	}

	u32* nandTmpJumpFuncOffset = patchOffsetCache.nandTmpJumpFuncOffset;

    if (!patchOffsetCache.nandTmpJumpFuncChecked) {
		nandTmpJumpFuncOffset = findNandTmpJumpFuncOffset(ndsHeader, moduleParams);
		if (nandTmpJumpFuncOffset) patchOffsetCache.nandTmpJumpFuncOffset = nandTmpJumpFuncOffset;
		patchOffsetCache.nandTmpJumpFuncChecked = true;
	}

	if (!nandTmpJumpFuncOffset) {
		return;
	}
	extern u32 generateA7Instr(int arg1, int arg2);

	// Patch
	if (moduleParams->sdk_version < 0x5008000) {
		nandTmpJumpFuncOffset[0] = 0xE59F0000; // ldr r0, =0xFFFFFFFF
		nandTmpJumpFuncOffset[1] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) + (1 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		nandTmpJumpFuncOffset[2] = 0xFFFFFFFF;
	} else if (nandTmpJumpFuncOffset[-2] == 0xE8BD8008 && nandTmpJumpFuncOffset[-1] == 0x02FFE230) {
		nandTmpJumpFuncOffset[-11] = 0xE59F0000; // ldr r0, =0
		nandTmpJumpFuncOffset[-10] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (10 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		nandTmpJumpFuncOffset[-9] = 0;
		dbg_printf("Reset (TWL) patched!\n");
		if (nandTmpJumpFuncOffset[-13] == 0xE12FFF1C) {
			nandTmpJumpFuncOffset[-17] = 0xE59F0000; // ldr r0, =0xFFFFFFFF
			nandTmpJumpFuncOffset[-16] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (16 * sizeof(u32))), (int)ce9->patches->reset_arm9);
			nandTmpJumpFuncOffset[-15] = 0xFFFFFFFF;
			dbg_printf("Exit-to-menu patched!\n");
		}
	} else if (nandTmpJumpFuncOffset[-2] == 0xE12FFF1C) {
		nandTmpJumpFuncOffset[-6] = 0xE59F0000; // ldr r0, =0xFFFFFFFF
		nandTmpJumpFuncOffset[-5] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (5 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		nandTmpJumpFuncOffset[-4] = 0xFFFFFFFF;
		dbg_printf("Exit-to-menu patched!\n");
	} else if (nandTmpJumpFuncOffset[-15] == 0xE8BD8008 && nandTmpJumpFuncOffset[-14] == 0x02FFE230) {
		nandTmpJumpFuncOffset[-24] = 0xE59F0000; // ldr r0, =0
		nandTmpJumpFuncOffset[-23] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (23 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		nandTmpJumpFuncOffset[-22] = 0;
		dbg_printf("Reset (TWL) patched!\n");
		if (nandTmpJumpFuncOffset[-26] == 0xE12FFF1C) {
			nandTmpJumpFuncOffset[-20] = 0xE59F0000; // ldr r0, =0xFFFFFFFF
			nandTmpJumpFuncOffset[-19] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (19 * sizeof(u32))), (int)ce9->patches->reset_arm9);
			nandTmpJumpFuncOffset[-18] = 0xFFFFFFFF;
			dbg_printf("Exit-to-menu patched!\n");
		}
	} else if (nandTmpJumpFuncOffset[-15] == 0xE12FFF1C) {
		nandTmpJumpFuncOffset[-19] = 0xE59F0000; // ldr r0, =0xFFFFFFFF
		nandTmpJumpFuncOffset[-18] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (5 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		nandTmpJumpFuncOffset[-17] = 0xFFFFFFFF;
		dbg_printf("Exit-to-menu patched!\n");
	}
	dbg_printf("nandTmpJumpFunc location : ");
	dbg_hexa((u32)nandTmpJumpFuncOffset);
	dbg_printf("\n\n");
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
	tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, usesThumb ? 0x18 : 0x30);
    dbg_printf("cardIrqEnable location : ");
    dbg_hexa((u32)cardIrqEnableOffset);
    dbg_printf("\n\n");
	return true;
}

static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	if (!extendedMemory2 || patchMpuRegion == 2 || ndsHeader->unitCode > 0) {
		return;
	}

	if (patchOffsetCache.patchMpuRegion != patchMpuRegion) {
		patchOffsetCache.patchMpuRegion = 0;
		patchOffsetCache.mpuStartOffset = 0;
		patchOffsetCache.mpuDataOffset = 0;
		patchOffsetCache.mpuInitOffset = 0;
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
		if (!isSdk5(moduleParams)) {
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
		}

		dbg_printf("Mpu data: ");
		dbg_hexa((u32)mpuDataOffset);
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
	if (((moduleParams->sdk_version < 0x2008000) && !extendedMemory2) || moduleParams->sdk_version > 0x5000000) {
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

void patchMpuFlagsSet(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 _io_dldi_features;
	extern u32 arm7mbk;
	if ((_io_dldi_features & FEATURE_SLOT_NDS) || moduleParams->sdk_version < 0x5000000 || arm7mbk == 0x080037C0) {
		return;
	}

	u32* offset = patchOffsetCache.mpuDataOffset2;
	if (!patchOffsetCache.mpuDataOffset2) {
		offset = findMpuFlagsSetOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.mpuDataOffset2 = offset;
		}
	}

	if (!offset) {
		return;
	}

	// Disable MPU cache and write buffer for the Slot-2 region
	offset[0] = 0xE3A00042; // mov r0, #0x42
	offset[2] = 0xE3A00042; // mov r0, #0x42
	offset[4] = 0xE3A00002; // mov r0, #0x02

	dbg_printf("Mpu flags set: ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

void patchMpuChange(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 arm7mbk;
	if (!extendedMemory2 || moduleParams->sdk_version < 0x5000000 || arm7mbk == 0x080037C0) {
		return;
	}

	u32* offset = patchOffsetCache.mpuChangeOffset;
	if (!patchOffsetCache.mpuChangeOffset) {
		offset = findMpuChange(ndsHeader);
		if (offset) {
			patchOffsetCache.mpuChangeOffset = offset;
		}
	}

	if (!offset) {
		return;
	}

	if (offset[0] == 0xE3A00001 || offset[0] == 0x03A0202C) {
		offset[3] = 0xE1A00000; // nop
	} else {
		u16* thumbOffset = (u16*)offset;
		thumbOffset[3] = 0x46C0; // nop
		thumbOffset[4] = 0x46C0; // nop
	}

	dbg_printf("Mpu change: ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
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
	extern bool ce9Alt;
	extern bool ce9AltLargeTable;
	extern u32 arm7mbk;
	extern u32 accessControl;
	extern u32 fatTableAddr;
	const char* romTid = getRomTid(ndsHeader);

	extern u32 cheatSize;
	extern u32 apPatchSize;
	const u32 cheatSizeTotal = cheatSize+(apPatchIsCheat ? apPatchSize : 0);

	const bool nandAccess = (accessControl & BIT(4)); // isDSiWare

	if ((!nandAccess && extendedMemory2)
	|| moduleParams->sdk_version < 0x2008000
	|| (ce9Alt && !ce9AltLargeTable && cheatSizeTotal <= 4)
	|| strncmp(romTid, "VSO", 3) == 0
	|| arm7mbk == 0x080037C0) {
		return;
	}

	u32* heapPointer = patchOffsetCache.heapPointer2Offset;
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
	}

    dbg_printf("hi heap end: ");
	dbg_hexa((u32)heapPointer);
    dbg_printf("\n\n");

    u32* oldheapPointer = (u32*)*heapPointer;

    dbg_printf("old hi heap value: ");
	dbg_hexa((u32)oldheapPointer);
    dbg_printf("\n\n");

	if (ce9Alt && !ce9AltLargeTable) {
		*heapPointer = CHEAT_ENGINE_LOCATION_B4DS-0x400000;
	} else {
		*heapPointer = (fatTableAddr < 0x023C0000 || fatTableAddr >= (u32)ce9) ? (u32)ce9 : fatTableAddr; // shrink heap by FAT table size + ce9 binary size
	}
	if (nandAccess && extendedMemory2) {
		*heapPointer = CARDENGINE_ARM9_LOCATION_DLDI_EXTMEM;
	}

    dbg_printf("new hi heap value: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Hi Heap Shrink Sucessfull\n\n");
}

u32 relocateBssPart(const tNDSHeader* ndsHeader, u32 bssEnd, u32 bssPartStart, u32 bssPartEnd, u32 newPartStart) {
	extern u32 iUncompressedSize;

	u32 subtract = bssPartEnd-bssPartStart;
	if (newPartStart < bssPartStart) {
		subtract = bssPartStart-newPartStart;
	}

	u32* addr = (u32*)ndsHeader->arm9destination;
	for (u32 i = 0; i < iUncompressedSize/4; i++) {
		if (addr[i] >= bssPartStart && addr[i] < bssPartEnd) {
			addr[i] -= bssPartStart;
			addr[i] += newPartStart;
		} else if (bssPartEnd != bssEnd && addr[i] >= bssPartEnd && addr[i] < bssEnd) {
			addr[i] -= subtract;
		}
	}

	return subtract;
}

void patchHiHeapDSiWare(u32 addr, u32 heapEnd) {
	//*(u32*)(addr) = opCode; // mov r0, #0x????????
	if (*(u32*)(addr+4) == 0xEA000043) { // debuggerSdk
		*(u32*)(addr) = 0xE59F0134; // ldr r0, =0x????????
		*(u32*)(addr+0x28) = 0xE3560001; // cmp r6, #1
		*(u32*)(addr+0x30) = 0xE3A00627; // mov r0, #0x2700000

		*(u32*)(addr+0x58) = 0xE3A00C00; // mov r0, #*(u32*)(addr+0x13C)
		if (*(u32*)(addr+0x13C) != 0) {
			for (u32 i = 0; i < *(u32*)(addr+0x13C); i += 0x100) {
				*(u32*)(addr+0x58) += 1;
			}
		}

		*(u32*)(addr+0x13C) = heapEnd;
		return;
	}

	*(u32*)(addr) = 0xE59F0094; // ldr r0, =0x????????
	*(u32*)(addr+0x24) = 0xE3500001; // cmp r0, #1
	*(u32*)(addr+0x2C) = 0x13A00627; // movne r0, #0x2700000

	*(u32*)(addr+0x40) = 0xE3A01C00; // mov r1, #*(u32*)(addr+0x9C)
	if (*(u32*)(addr+0x9C) != 0) {
		for (u32 i = 0; i < *(u32*)(addr+0x9C); i += 0x100) {
			*(u32*)(addr+0x40) += 1;
		}
	}

	/*if (*(u32*)(addr+0x9C) == 0) {
		*(u32*)(addr+0x40) = 0xE3A01000; // mov r1, #0
	} else if (*(u32*)(addr+0x9C) == 0x800) {
		*(u32*)(addr+0x40) = 0xE3A01B02; // mov r1, #0x800
	}*/

	*(u32*)(addr+0x9C) = heapEnd;
}

void patchHiHeapDSiWareThumb(u32 addr, u32 newCodeAddr, u32 heapEnd) {
	*(u16*)(newCodeAddr) = 0x4800; // movs r0, #0x????????
	*(u16*)(newCodeAddr+2) = 0x4770; // bx lr
	*(u32*)(newCodeAddr+4) = heapEnd;

	setBLThumb(addr, newCodeAddr);

	*(u16*)(addr+0x1A) = 0x2801; // cmp r0, #1
	*(u16*)(addr+0x22) = 0x2027; // movne r0, #0x2700000
}

void patchUserSettingsReadDSiWare(u32 addr) {
	if (*(u16*)(addr) == 0x4806) { // ldr r0, =0x2FFFDFC (THUMB)
		*(u16*)(addr) = 0x46C0; // nop
		*(u16*)(addr+4) = 0xBD38; // POP {R3-R5,PC}
	} else if (*(u16*)(addr) == 0x4805) { // ldr r0, =0x2FFFDFC (THUMB)
		*(u16*)(addr) = 0x46C0; // nop
		*(u16*)(addr+4) = 0xBD10; // POP {R4,PC}
	} else if (*(u32*)(addr) == 0xE594117C) { // ldr r1, [r4,#0x17C]
		*(u32*)(addr) = 0xE5C50054; // strb r0, [r5,#0x54]
		*(u32*)(addr+4) = 0xE1D406B4; // ldrh r0, [r4,#0x64]
		*(u32*)(addr+8) = 0xE1A00E80; // mov r0, r0,lsl#29
		*(u32*)(addr+0xC) = 0xE1A00EA0; // mov r0, r0,lsr#29
	} else {
		*(u32*)(addr) -= 4; // ldr r0, =0x2FFFC80
		*(u32*)(addr+8) = 0xE5C41054; // strb r1, [r4,#0x54]
		*(u32*)(addr+0xC) = 0xE1D006B4; // ldrh r0, [r0,#0x64]
		*(u32*)(addr+0x10) = 0xE1A00E80; // mov r0, r0,lsl#29
		*(u32*)(addr+0x14) = 0xE1A00EA0; // mov r0, r0,lsr#29
	}
}

void patchInitDSiWare(u32 addr, u32 heapEnd) {
	bool debuggerSdk = false;
	u32* func = getOffsetFromBL((u32*)(addr+0x10));
	if (func[8] == 0xE92D4010) { // STMFD SP!, {R4,LR}
		debuggerSdk = true;
		func[6] = 0xE1A00000; // nop
		patchHiHeapDSiWare(((u32)func+0x300), heapEnd);
	} else {
		bool heapInitPatched = false;
		for (int i = 0; i < 64; i++) {	
			if (func[i] == 0xE1A00100 && !heapInitPatched) {
				func[i-2] = 0xE1A00000; // nop
				i += 16;
				heapInitPatched = true;
			}

			if (func[i] == 0xE3A007BE) {
				patchHiHeapDSiWare(((u32)func+(i*sizeof(u32))), heapEnd);
				break;
			}
		}
	}

	func = getOffsetFromBL((u32*)(addr+0x20));
	func[3] = 0xE1A00000; // nop

	if (debuggerSdk) {
		*(u32*)(addr+0x3C) = 0xE1A00000; // nop
		*(u32*)(addr+0x40) = 0xE1A00000; // nop
		*(u32*)(addr+0x4C) = 0xE1A00000; // nop
	} else {
		*(u32*)(addr+0x8C) = 0xE1A00000; // nop
		*(u32*)(addr+0x90) = 0xE1A00000; // nop
		*(u32*)(addr+0x9C) = 0xE1A00000; // nop
	}
}

void patchVolumeGetDSiWare(u32 addr) {
	*(u32*)(addr) = 0xE59F0004; // ldr r0, =0x02FFFDF0
	*(u32*)(addr+4) = 0xE5900000; // ldr r0, [r0]
	*(u32*)(addr+8) = 0xE12FFF1E; // bx lr
	*(u32*)(addr+0xC) = 0x02FFFDF0;
}

void patchTwlSleepMode(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 arm7mbk;
	if (arm7mbk != 0x080037C0 || moduleParams->sdk_version < 0x5010000) {
		return;
	}

	u32* offset = patchOffsetCache.twlSleepModeEndOffset;
	if (!patchOffsetCache.twlSleepModeEndChecked) {
		offset = findTwlSleepModeEndOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.twlSleepModeEndOffset = offset;
		}
		patchOffsetCache.twlSleepModeEndChecked = true;
	}

	if (!offset) {
		return;
	}

	dbg_printf("twlSleepModeEnd location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");

	if (*offset == 0xE2855001 || *offset == 0xE2866001) {
		offset--;
		*offset = 0xE1A00000; // nop
	} else {
		u16* offsetThumb = (u16*)offset;
		offsetThumb--;
		offsetThumb--;
		offsetThumb[0] = 0x46C0; // nop
		offsetThumb[1] = 0x46C0; // nop
	}
}

bool useSharedFont = false;

void patchSharedFontPath(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	//extern u32 accessControl;
	extern u32 arm9ibinarySize;
	if (/* !(accessControl & BIT(4)) || *(u32*)0x023B8000 != 0 */ !useSharedFont) {
		toncset((u32*)0x023B8000, 0, arm9ibinarySize > 0x8000 ? 0x8000 : arm9ibinarySize);
		return;
	}

	u32* offset = patchOffsetCache.sharedFontPathOffset;
	if (!patchOffsetCache.sharedFontPathChecked) {
		offset = findSharedFontPathOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.sharedFontPathOffset = offset;
		}
		patchOffsetCache.sharedFontPathChecked = true;
	}

	if (!offset) {
		return;
	}

	dbg_printf("sharedFontPath location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");

	const u32* dsiSaveOpen = ce9->patches->dsiSaveOpen;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;

	u32* fileIoOpen = patchOffsetCache.fileIoOpenOffset;
	if (!patchOffsetCache.fileIoOpenOffset) {
		fileIoOpen = findFileIoOpenOffset(ndsHeader, moduleParams);
		if (fileIoOpen) {
			patchOffsetCache.fileIoOpenOffset = fileIoOpen;
		}
	}
	if (!fileIoOpen) {
		toncset((u32*)0x023B8000, 0, arm9ibinarySize);
		return;
	}

	dbg_printf("fileIoOpen location : ");
	dbg_hexa((u32)fileIoOpen);
	dbg_printf("\n\n");

	if (*(u16*)fileIoOpen != 0xB5F8) { // ARM
		u32* fileIoClose = patchOffsetCache.fileIoCloseOffset;
		if (!patchOffsetCache.fileIoCloseOffset) {
			fileIoClose = findFileIoCloseOffset(fileIoOpen);
			if (fileIoClose) {
				patchOffsetCache.fileIoCloseOffset = fileIoClose;
			}
		}
		if (!fileIoClose) {
			toncset((u32*)0x023B8000, 0, arm9ibinarySize);
			return;
		}

		dbg_printf("fileIoClose location : ");
		dbg_hexa((u32)fileIoClose);
		dbg_printf("\n\n");

		u32* fileIoSeek = patchOffsetCache.fileIoSeekOffset;
		if (!patchOffsetCache.fileIoSeekOffset) {
			fileIoSeek = findFileIoSeekOffset(fileIoClose, moduleParams);
			if (fileIoSeek) {
				patchOffsetCache.fileIoSeekOffset = fileIoSeek;
			}
		}
		if (!fileIoSeek) {
			toncset((u32*)0x023B8000, 0, arm9ibinarySize);
			return;
		}

		dbg_printf("fileIoSeek location : ");
		dbg_hexa((u32)fileIoSeek);
		dbg_printf("\n\n");

		u32* fileIoRead = patchOffsetCache.fileIoReadOffset;
		if (!patchOffsetCache.fileIoReadOffset) {
			fileIoRead = findFileIoReadOffset(fileIoSeek, moduleParams);
			if (fileIoRead) {
				patchOffsetCache.fileIoReadOffset = fileIoRead;
			}
		}
		if (!fileIoRead) {
			toncset((u32*)0x023B8000, 0, arm9ibinarySize);
			return;
		}

		dbg_printf("fileIoRead location : ");
		dbg_hexa((u32)fileIoRead);
		dbg_printf("\n\n");

		extern u32* findLtdModuleParamsOffset(const tNDSHeader* ndsHeader);
		ltd_module_params_t* ltdModuleParams = (ltd_module_params_t*)(findLtdModuleParamsOffset(ndsHeader) - 4);

		u32 iUncompressedSizei = arm9ibinarySize;

		if (ltdModuleParams->compressed_static_end) {
			dbg_printf("arm9i is compressed");
			extern u32 decompressIBinary(unsigned char *pak_buffer, unsigned int pak_len);
			iUncompressedSizei = decompressIBinary((unsigned char*)0x023B8000, arm9ibinarySize);
		} else {
			dbg_printf("arm9i is not compressed");
		}
		dbg_printf("\n\n");

		tonccpy(moduleParams->static_bss_end, (u32*)0x023B8004, iUncompressedSizei-4);
		toncset((u32*)0x023B8000, 0, iUncompressedSizei);

		//bool armFound = false;
		//bool dsiBiosFuncsIn9i = false;
		bool relocOffsets = (strncmp(ndsHeader->gameCode, "KPT", 3) == 0);
		int zerosFound = 0;
		u32* arm9idst = moduleParams->static_bss_end;
		for (u32 i = 0; i < (iUncompressedSizei-4)/4; i++) {
			if (arm9idst[i] == (u32)offset) {
				for (int i2 = 0; i2 < 0x100/4; i2++) {
					u32* blOffset = (arm9idst + i - i2);
					u32* fileIoPtr = getOffsetFromBL(blOffset);
					if (fileIoPtr == fileIoOpen) {
						setBL((u32)blOffset, (u32)dsiSaveOpen);

						//dbg_printf("fileIoOpen bl found : ");
						//dbg_hexa((u32)blOffset);
						//dbg_printf("\n\n");
						//armFound = true;
					} else if (fileIoPtr == fileIoClose) {
						setBL((u32)blOffset, (u32)dsiSaveClose);

						//dbg_printf("fileIoClose bl found : ");
						//dbg_hexa((u32)blOffset);
						//dbg_printf("\n\n");
						//armFound = true;
					} else if (fileIoPtr == fileIoSeek) {
						setBL((u32)blOffset, (u32)dsiSaveSeek);

						//dbg_printf("fileIoSeek bl found : ");
						//dbg_hexa((u32)blOffset);
						//dbg_printf("\n\n");
						//armFound = true;
					} else if (fileIoPtr == fileIoRead) {
						setBL((u32)blOffset, (u32)dsiSaveRead);

						//dbg_printf("fileIoRead bl found : ");
						//dbg_hexa((u32)blOffset);
						//dbg_printf("\n\n");
						//armFound = true;
					} /*else if ((u32)fileIoPtr >= ((u32)ndsHeader->arm9destination)+ndsHeader->arm9binarySize && (u32)fileIoPtr < ((u32)moduleParams->static_bss_end)+0x7FFC) {
						*blOffset = 0xE3A00001; // mov r0, #1
					}*/
				}
			} else if (relocOffsets && arm9idst[i] >= ((u32)moduleParams->static_bss_end)+0x30000 && arm9idst[i] < ((u32)moduleParams->static_bss_end)+0x40000) {
				arm9idst[i] -= 0x30000;
			} else if (arm9idst[i] == 0x4770DF20 || arm9idst[i] == 0x4770DF21 || arm9idst[i] == 0x4770DF22 || arm9idst[i] == 0x4770DF23
					|| arm9idst[i] == 0x4770DF24 || arm9idst[i] == 0x4770DF25 || arm9idst[i] == 0x4770DF26 || arm9idst[i] == 0x4770DF27
					|| arm9idst[i] == 0x4770DF28 || arm9idst[i] == 0x4770DF29) {
				// Stub out DSi BIOS functions
				arm9idst[i] = 0x47702001;
				//dsiBiosFuncsIn9i = true;
			} else if (arm9idst[i] == 0) {
				zerosFound++;
				if (zerosFound == 0x200/4) {
					break;
				}
			}
		}
		/*if (!dsiBiosFuncsIn9i) {
			extern u32 iUncompressedSize;
			u32* arm9dst = ndsHeader->arm9destination;
			for (u32 i = 0; i < iUncompressedSize/4; i++) {
				if (arm9dst[i] == 0x4770DF20 || arm9dst[i] == 0x4770DF21 || arm9dst[i] == 0x4770DF22 || arm9dst[i] == 0x4770DF23
				 || arm9dst[i] == 0x4770DF24 || arm9dst[i] == 0x4770DF25 || arm9dst[i] == 0x4770DF26 || arm9dst[i] == 0x4770DF27
				 || arm9dst[i] == 0x4770DF28 || arm9dst[i] == 0x4770DF29) {
					// Stub out DSi BIOS functions
					arm9dst[i] = 0x47702001;
				}
			}
		}*/
		/*if (!armFound) {
			const u32 dsiSaveOpenT = 0x02000200;
			const u32 dsiSaveCloseT = 0x02000210;
			const u32 dsiSaveSeekT = 0x02000220;
			const u32 dsiSaveReadT = 0x02000230;

			bool openBlFound = false;
			bool closeBlFound = false;
			bool seekBlFound = false;
			bool readBlFound = false;

			u16* arm9idstT = (u16*)moduleParams->static_bss_end;
			for (u32 i = 0; i < iUncompressedSizei/4; i++) {
				if (arm9idst[i] == (u32)offset) {
					for (int i2 = 0; i2 < 0xC0/2; i2++) {
						u16* blOffset = (arm9idstT + (i*2) - i2);
						u32* fileIoPtr = (u32*)getOffsetFromBLXThumb(blOffset); // TODO: Implement getOffsetFromBLXThumb
						if (fileIoPtr == fileIoOpen) {
							setBLThumb((u32)blOffset, dsiSaveOpenT);

							//dbg_printf("fileIoOpen bl found : ");
							//dbg_hexa((u32)blOffset);
							//dbg_printf("\n\n");
							openBlFound = true;
						} else if (fileIoPtr == fileIoClose) {
							setBLThumb((u32)blOffset, dsiSaveCloseT);

							//dbg_printf("fileIoClose bl found : ");
							//dbg_hexa((u32)blOffset);
							//dbg_printf("\n\n");
							closeBlFound = true;
						} else if (fileIoPtr == fileIoSeek) {
							setBLThumb((u32)blOffset, dsiSaveSeekT);

							//dbg_printf("fileIoSeek bl found : ");
							//dbg_hexa((u32)blOffset);
							//dbg_printf("\n\n");
							seekBlFound = true;
						} else if (fileIoPtr == fileIoRead) {
							setBLThumb((u32)blOffset, dsiSaveReadT);

							//dbg_printf("fileIoRead bl found : ");
							//dbg_hexa((u32)blOffset);
							//dbg_printf("\n\n");
							readBlFound = true;
						}
					}
				}
			}
			if (openBlFound) {
				*(u16*)dsiSaveOpenT = 0x4778; // bx pc
				tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);
			}
			if (closeBlFound) {
				*(u16*)dsiSaveCloseT = 0x4778; // bx pc
				tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);
			}
			if (seekBlFound) {
				*(u16*)dsiSaveSeekT = 0x4778; // bx pc
				tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);
			}
			if (readBlFound) {
				*(u16*)dsiSaveReadT = 0x4778; // bx pc
				tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);
			}
		}*/
	} else { // THUMB
		toncset((u32*)0x023B8000, 0, 0x8000);

		u16* fileIoClose = (u16*)patchOffsetCache.fileIoCloseOffset;
		if (!patchOffsetCache.fileIoCloseOffset) {
			fileIoClose = findFileIoCloseOffsetThumb((u16*)fileIoOpen);
			if (fileIoClose) {
				patchOffsetCache.fileIoCloseOffset = (u32*)fileIoClose;
			}
		}
		if (!fileIoClose) {
			return;
		}

		dbg_printf("fileIoClose location : ");
		dbg_hexa((u32)fileIoClose);
		dbg_printf("\n\n");

		u16* fileIoSeek = (u16*)patchOffsetCache.fileIoSeekOffset;
		if (!patchOffsetCache.fileIoSeekOffset) {
			fileIoSeek = findFileIoSeekOffsetThumb(fileIoClose);
			if (fileIoSeek) {
				patchOffsetCache.fileIoSeekOffset = (u32*)fileIoSeek;
			}
		}
		if (!fileIoSeek) {
			return;
		}

		dbg_printf("fileIoSeek location : ");
		dbg_hexa((u32)fileIoSeek);
		dbg_printf("\n\n");

		u16* fileIoRead = (u16*)patchOffsetCache.fileIoReadOffset;
		if (!patchOffsetCache.fileIoReadOffset) {
			fileIoRead = findFileIoReadOffsetThumb(fileIoSeek, moduleParams);
			if (fileIoRead) {
				patchOffsetCache.fileIoReadOffset = (u32*)fileIoRead;
			}
		}
		if (!fileIoRead) {
			return;
		}

		dbg_printf("fileIoRead location : ");
		dbg_hexa((u32)fileIoRead);
		dbg_printf("\n\n");

		return; // getOffsetFromBLThumb currently doesn't get backward offsets correctly

		tonccpy(moduleParams->static_bss_end, (u32*)0x023B8004, 0x7FFC);
		toncset((u32*)0x023B8000, 0, 0x8000);

		const u32 dsiSaveOpenT = 0x02000200;
		const u32 dsiSaveCloseT = 0x02000210;
		const u32 dsiSaveSeekT = 0x02000220;
		const u32 dsiSaveReadT = 0x02000230;

		bool openBlFound = false;
		bool closeBlFound = false;
		bool seekBlFound = false;
		bool readBlFound = false;

		u32* arm9idst = moduleParams->static_bss_end;
		u16* arm9idstT = (u16*)moduleParams->static_bss_end;
		for (u32 i = 0; i < 0x7FFC/4; i++) {
			if (arm9idst[i] == (u32)offset) {
				for (int i2 = 0; i2 < 0x100/2; i2++) {
					u16* blOffset = (arm9idstT + (i*2) - i2);
					u16* fileIoPtr = getOffsetFromBLThumb(blOffset);
					if (fileIoPtr == (u16*)fileIoOpen) {
						setBLThumb((u32)blOffset, dsiSaveOpenT);

						dbg_printf("fileIoOpen bl found : ");
						dbg_hexa((u32)blOffset);
						dbg_printf("\n\n");
						openBlFound = true;
					} else if (fileIoPtr == fileIoClose) {
						setBLThumb((u32)blOffset, dsiSaveCloseT);

						dbg_printf("fileIoClose bl found : ");
						dbg_hexa((u32)blOffset);
						dbg_printf("\n\n");
						closeBlFound = true;
					} else if (fileIoPtr == fileIoSeek) {
						setBLThumb((u32)blOffset, dsiSaveSeekT);

						dbg_printf("fileIoSeek bl found : ");
						dbg_hexa((u32)blOffset);
						dbg_printf("\n\n");
						seekBlFound = true;
					} else if (fileIoPtr == fileIoRead) {
						setBLThumb((u32)blOffset, dsiSaveReadT);

						dbg_printf("fileIoRead bl found : ");
						dbg_hexa((u32)blOffset);
						dbg_printf("\n\n");
						readBlFound = true;
					}
				}
			}
		}
		if (openBlFound) {
			*(u16*)dsiSaveOpenT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);
		}
		if (closeBlFound) {
			*(u16*)dsiSaveCloseT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);
		}
		if (seekBlFound) {
			*(u16*)dsiSaveSeekT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);
		}
		if (readBlFound) {
			*(u16*)dsiSaveReadT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);
		}
	}
}

void patchTwlFontLoad(u32 heapAllocAddr, u32 newCodeAddr) {
	extern bool expansionPakFound;
	extern bool sharedFontInMep;
	if (expansionPakFound) {
		extern u32 clusterCache;
		extern u32* twlFontHeapAlloc;
		extern u32 twlFontHeapAllocSize;

		tonccpy((u32*)newCodeAddr, twlFontHeapAlloc, twlFontHeapAllocSize);
		*(u32*)newCodeAddr = clusterCache-0x200000;
		setBL(heapAllocAddr, newCodeAddr+4);

		sharedFontInMep = true;
		return;
	}

	extern u32* twlFontHeapAllocNoMep;

	tonccpy((u32*)newCodeAddr, twlFontHeapAllocNoMep, 0xC);
	setBL(heapAllocAddr, newCodeAddr);
}

void codeCopy(u32* dst, u32* src, u32 len) {
	tonccpy(dst, src, len);

	u32 srci = (u32)src;
	u32 dsti = (u32)dst;
	for (int i = 0; i < len/sizeof(u32); i++) {
		if (*(u8*)(srci+3) == 0xEB) { // If bl instruction...
			// then fix it
			u32* offsetByBl = getOffsetFromBL((u32*)srci);
			setBL(dsti, (u32)offsetByBl);
		}
		srci += 4;
		dsti += 4;
	}
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

static void patchCardReadPdash(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
    u32 sdPatchEntry = 0;
    
	const char* romTid = getRomTid(ndsHeader);
	if (strncmp(romTid, "APD", 3) != 0 && strncmp(romTid, "A24", 3) != 0) return;

	if (strncmp(romTid, "A24", 3) == 0) { // Kiosk Demo
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0206DA88 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206DFCC = 0; // Fix card set DMA function find

			sdPatchEntry = 0x206DB28;
		}
	} else if (ndsHeader->gameCode[3] == 'E') {
		*(u32*)0x0206CF48 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206D48C = 0; // Fix card set DMA function find

		sdPatchEntry = 0x206CFE8;
        // target 206CFE8, more similar to cardread
        // r0 cardstruct 218A6E0 ptr 20D6120
        // r1 src
        // r2 dst
        // r3 len
        // return r0=number of time executed ?? 206D1A8
	} else if (ndsHeader->gameCode[3] == 'P') {
		*(u32*)0x0206CF60 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206D4A4 = 0; // Fix card set DMA function find

		sdPatchEntry = 0x206D000;
	} else if (ndsHeader->gameCode[3] == 'J') {
		*(u32*)0x0206AAF4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206B038 = 0; // Fix card set DMA function find

		sdPatchEntry = 0x206AB94;
	} else if (ndsHeader->gameCode[3] == 'K') {
		*(u32*)0x0206DFCC = 0xE12FFF1E; // bx lr
		*(u32*)0x0206E510 = 0; // Fix card set DMA function find

		sdPatchEntry = 0x206E06C;
	}

    if(sdPatchEntry) {   
     	// Patch
    	u32* pDashReadPatch = ce9->patches->pdash_read;
    	tonccpy((u32*)sdPatchEntry, pDashReadPatch, 0x40);   
    }
}

static void operaRamPatch(void) {
	if (dsDebugRam || !extendedMemory2) {
		return;
	}

	extern u32 ROMinRAM;

	// Opera RAM patch (ARM9)
	*(u32*)0x02003D48 = 0xC400000;
	*(u32*)0x02003D4C = 0xC400004;

	*(u32*)0x02010FF0 = 0xC400000;
	*(u32*)0x02010FF4 = 0xC4000CE;

	*(u32*)0x020112AC = 0xC400080;

	*(u32*)0x020402BC = 0xC4000C2;
	*(u32*)0x020402C0 = 0xC4000C0;
	if (ROMinRAM) {
		*(u32*)0x020402CC = 0xD7FFFFE;
		*(u32*)0x020402D0 = 0xD000000;
		*(u32*)0x020402D4 = 0xD1FFFFF;
		*(u32*)0x020402D8 = 0xD3FFFFF;
		*(u32*)0x020402DC = 0xD7FFFFF;
		*(u32*)0x020402E0 = 0xDFFFFFF;	// ???
		toncset((char*)0xD000000, 0xFF, 0x800000);		// Fill fake MEP with FFs
	} else {
		*(u32*)0x020402CC = 0xCFFFFFE;
		*(u32*)0x020402D0 = 0xC800000;
		*(u32*)0x020402D4 = 0xC9FFFFF;
		*(u32*)0x020402D8 = 0xCBFFFFF;
		*(u32*)0x020402DC = 0xCFFFFFF;
		*(u32*)0x020402E0 = 0xD7FFFFF;	// ???
		toncset((char*)0xC800000, 0xFF, 0x800000);		// Fill fake MEP with FFs
	}
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

	if (ndsHeader->unitCode > 0) {
		extern u32 donorFileTwlCluster;
		extern u32 arm7mbk;
		extern u32 accessControl;

		if (((accessControl & BIT(4))
		   || (strncmp(romTid, "DME", 3) == 0 && extendedMemory2)
		   || (strncmp(romTid, "DMD", 3) == 0 && extendedMemory2)
		   || strncmp(romTid, "DMP", 3) == 0
		   || (strncmp(romTid, "DHS", 3) == 0 && extendedMemory2)
		)	&& arm7mbk == 0x080037C0 && donorFileTwlCluster != CLUSTER_FREE) {
			if (moduleParams->sdk_version > 0x5050000) {
				*(u32*)(startOffset+0x838) = 0xE1A00000; // nop
				*(u32*)(startOffset+0x99C) = 0xE1A00000; // nop
			} else if (moduleParams->sdk_version > 0x5020000 && moduleParams->sdk_version < 0x5050000) {
				*(u32*)(startOffset+0x98C) = 0xE1A00000; // nop
			}

			patchTwlSleepMode(ndsHeader, moduleParams);
		}

		patchSharedFontPath(ce9, ndsHeader, moduleParams);
	}

	patchMpu(ndsHeader, moduleParams, patchMpuRegion, patchMpuSize);
	patchMpu2(ndsHeader, moduleParams);
	patchMpuFlagsSet(ndsHeader, moduleParams);
	patchMpuChange(ndsHeader, moduleParams);

	patchHiHeapPointer(ce9, moduleParams, ndsHeader);

	if (isPawsAndClaws(ndsHeader)) {
		patchCardId(ce9, ndsHeader, moduleParams, false, NULL); // Patch card ID first
	}

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &sdk5ReadType, &cardReadEndOffset, startOffset)) {
		dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

	if (strncmp(romTid, "V2G", 3) == 0) {
		// try to patch card read a second time
		dbg_printf("patch card read a second time\n");
		dbg_printf("startOffset : 0x02030000\n\n");
		if (!patchCardReadMvDK4(0x02030000)) {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
	}

	patchCardReadPdash(ce9, ndsHeader);

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, sdk5ReadType, &cardPullOutOffset);

	patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	//patchForceToPowerOff(ce9, ndsHeader, usesThumb);

	if (!isPawsAndClaws(ndsHeader)) {
		patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset);
	}

	patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);

	//patchDownloadplay(ndsHeader);

	patchReset(ce9, ndsHeader, moduleParams);
	patchResetTwl(ce9, ndsHeader, moduleParams);

	randomPatch(ndsHeader, moduleParams);
	randomPatch5Second(ndsHeader, moduleParams);

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch();
	}

	nandSavePatch(ce9, ndsHeader, moduleParams);
    
	setFlushCache(ce9, patchMpuRegion, usesThumb);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
