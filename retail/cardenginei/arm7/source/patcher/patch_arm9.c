#include <string.h>
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
//#include "value_bits.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"
//#include "debug_file.h"
#include "tonccpy.h"

#define gameOnFlashcard BIT(0)
#define ROMinRAM BIT(3)
#define asyncCardRead BIT(14)

extern u32 valueBits;
extern u16 scfgRomBak;

extern vu32* volatile sharedAddr;

static void fixForDifferentBios(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, bool usesThumb) {
	if (scfgRomBak & BIT(1)) {
		return;
	}
	u32* swi12Offset = a9_findSwi12Offset(ndsHeader);

	// swi 0x12 call
	if (swi12Offset) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = ce9->patches->swi02;
		tonccpy(swi12Offset, swi12Patch, 0x4);
	}

	/*dbg_printf("swi12 location : ");
	dbg_hexa((u32)swi12Offset);
	dbg_printf("\n\n");*/
}

static bool patchCardRead(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumbPtr, int* readTypePtr, u32** cardReadEndOffsetPtr, u32 startOffset) {
	bool usesThumb = false;
	int readType = 0;

	// Card read
    u32* cardReadEndOffset = (u32*)findCardReadEndOffsetThumb(ndsHeader, startOffset);
	if (cardReadEndOffset) {
		usesThumb = true;
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
				//dbg_printf("Found thumb\n\n");
				--cardReadEndOffset;
				usesThumb = true;
			}
		}
	}
	*usesThumbPtr = usesThumb;
	*readTypePtr = readType;
	*cardReadEndOffsetPtr = cardReadEndOffset;
	if (!cardReadEndOffset) { // Not necessarily needed
		return false;
	}
    u32* cardReadStartOffset = (u32*)findCardReadStartOffsetThumb((u16*)cardReadEndOffset);
	if (!cardReadStartOffset) {
		if (readType == 0) {
			cardReadStartOffset = findCardReadStartOffsetType0(moduleParams, cardReadEndOffset);
		} else {
			cardReadStartOffset = findCardReadStartOffsetType1(cardReadEndOffset);
		}
	}
	if (!cardReadStartOffset) {
		return false;
	}
	//cardReadFound = true;

	// Card struct
	u32** cardStruct = (u32**)(cardReadEndOffset - 1);
	//u32* cardStructPatch = (usesThumb ? ce9->thumbPatches->cardStructArm9 : ce9->patches->cardStructArm9);
	if (moduleParams->sdk_version > 0x3000000) {
		// Save card struct
		sharedAddr[4] = (u32)(*cardStruct + 7);
		//*cardStructPatch = (u32)(*cardStruct + 7); // Cache management alternative
	} else {
		// Save card struct
		sharedAddr[4] = (u32)(*cardStruct + 6);
		//*cardStructPatch = (u32)(*cardStruct + 6); // Cache management alternative
	}

	// Patch
	u32* cardReadPatch = (usesThumb ? ce9->thumbPatches->card_read_arm9 : ce9->patches->card_read_arm9);
	tonccpy(cardReadStartOffset, cardReadPatch, usesThumb ? 0xA0 : 0xE0); // 0xE0 = 0xF0 - 0x08
    /*dbg_printf("cardRead location : ");
    dbg_hexa((u32)cardReadStartOffset);
    dbg_printf("\n");
    dbg_hexa((u32)ce9);
    dbg_printf("\n\n");*/
	return true;
}

static bool patchCardPullOut(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32** cardPullOutOffsetPtr) {
	// Card pull out
	u32* cardPullOutOffset = NULL;
	if (usesThumb) {
		//dbg_printf("Trying SDK 5 thumb...\n");
		if (!cardPullOutOffset) {
			//dbg_printf("Trying thumb...\n");
			cardPullOutOffset = (u32*)findCardPullOutOffsetThumb(ndsHeader);
		}
	} else {
		cardPullOutOffset = findCardPullOutOffset(ndsHeader, moduleParams);
	}
	*cardPullOutOffsetPtr = cardPullOutOffset;
	if (!cardPullOutOffset) {
		return false;
	}

	// Patch
	u32* cardPullOutPatch = (usesThumb ? ce9->thumbPatches->card_pull_out_arm9 : ce9->patches->card_pull_out_arm9);
	tonccpy(cardPullOutOffset, cardPullOutPatch, usesThumb ? 0x2 : 0x30);
    /*dbg_printf("cardPullOut location : ");
    dbg_hexa((u32)cardPullOutOffset);
    dbg_printf("\n\n");*/
	return true;
}

/*static void patchCacheFlush(cardengineArm9* ce9, bool usesThumb, u32* cardPullOutOffset) {
	if (!cardPullOutOffset) {
		return;
	}

	// Patch
	u32* cacheFlushPatch = (usesThumb ? ce9->thumbPatches->cacheFlushRef : ce9->patches->cacheFlushRef);
	*cacheFlushPatch = (u32)cardPullOutOffset + 13;
}*/

static void patchCardId(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return;
	}

	// Card ID
	u32* cardIdStartOffset = NULL;
	u32* cardIdEndOffset = NULL;
	if (usesThumb) {
		cardIdEndOffset = (u32*)findCardIdEndOffsetThumb(ndsHeader, moduleParams, (u16*)cardReadEndOffset);
		cardIdStartOffset = (u32*)findCardIdStartOffsetThumb(moduleParams, (u16*)cardIdEndOffset);
	} else {
		cardIdEndOffset = findCardIdEndOffset(ndsHeader, moduleParams, cardReadEndOffset);
		cardIdStartOffset = findCardIdStartOffset(moduleParams, cardIdEndOffset);
	}

	if (cardIdStartOffset) {
        // Patch
		u32* cardIdPatch = (usesThumb ? ce9->thumbPatches->card_id_arm9 : ce9->patches->card_id_arm9);
		tonccpy(cardIdStartOffset, cardIdPatch, usesThumb ? 0x8 : 0xC);
		/*dbg_printf("cardId location : ");
		dbg_hexa((u32)cardIdStartOffset);
		dbg_printf("\n\n");*/
	}
}

static void patchCardReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	// Card read dma
	u32* cardReadDmaStartOffset = NULL;
	u32* cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader, moduleParams);
	if (!cardReadDmaEndOffset && usesThumb) {
		cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
	}
	if (usesThumb) {
		cardReadDmaStartOffset = (u32*)findCardReadDmaStartOffsetThumb((u16*)cardReadDmaEndOffset);
	} else {
		cardReadDmaStartOffset = findCardReadDmaStartOffset(moduleParams, cardReadDmaEndOffset);
	}
	if (!cardReadDmaStartOffset) {
		return;
	}
	// Patch
	u32* cardReadDmaPatch = (usesThumb ? ce9->thumbPatches->card_dma_arm9 : ce9->patches->card_dma_arm9);
	tonccpy(cardReadDmaStartOffset, cardReadDmaPatch, 0x40);
    /*dbg_printf("cardReadDma location : ");
    dbg_hexa((u32)cardReadDmaStartOffset);
    dbg_printf("\n\n");*/
}

static bool patchCardEndReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	/*const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars
	 || strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version)
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man
	 || !cardReadDMA) return false;*/

    u32* offset = NULL;
    u32 offsetDmaHandler = 0;
	u32* cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader, moduleParams);
	if (!cardReadDmaEndOffset && usesThumb) {
		cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
	}
	offset = findCardEndReadDma(ndsHeader,moduleParams,usesThumb,cardReadDmaEndOffset,&offsetDmaHandler);
    if(offset) {
      /*dbg_printf("\nNDMA CARD READ METHOD ACTIVE\n");
    dbg_printf("cardEndReadDma location : ");
    dbg_hexa((u32)offset);
    dbg_printf("\n\n");*/
        // SDK1-4        
        if(usesThumb) {
            u16* thumbOffset = (u16*)offset;
            u16* thumbOffsetStartFunc = (u16*)offset;
			bool lrOnly = false;
            for (int i = 0; i <= 18; i++) {
                thumbOffsetStartFunc--;
				if (*thumbOffsetStartFunc==0xB5F0) {
					//dbg_printf("\nthumbOffsetStartFunc : ");
					//dbg_hexa(*thumbOffsetStartFunc);
					lrOnly = true;
					break;
				}
            }
            thumbOffset--;
			if (lrOnly) {
				thumbOffset--;
				thumbOffset[0] = *thumbOffsetStartFunc; // push	{r4-r7, lr}
				thumbOffset[1] = 0xB081; // SUB SP, SP, #4
			} else {
				*thumbOffset = 0xB5F8; // push	{r3-r7, lr}
			}
            ce9->thumbPatches->cardEndReadDmaRef = (u32*)thumbOffset;
        } else {
            u32* armOffset = (u32*)offset;
            u32* armOffsetStartFunc = (u32*)offset;
			bool lrOnly = false;
            for (int i = 0; i <= 16; i++) {
                armOffsetStartFunc--;
				if (*armOffsetStartFunc==0xE92D4000 || *armOffsetStartFunc==0xE92D4030 || *armOffsetStartFunc==0xE92D40F0) {
					//dbg_printf("\narmOffsetStartFunc : ");
					//dbg_hexa(*armOffsetStartFunc);
					lrOnly = true;
					break;
				}
            }
            armOffset--;
			if (lrOnly) {
				armOffset--;
				armOffset[0] = *armOffsetStartFunc; // "STMFD SP!, {LR}", "STMFD SP!, {R4,R5,LR}", or "STMFD SP!, {R4-R7,LR}"
				armOffset[1] = 0xE24DD004; // SUB SP, SP, #4
				if (offsetDmaHandler && *armOffsetStartFunc==0xE92D40F0) {
					offsetDmaHandler += 14*sizeof(u32); // Relocation fix
				}
			} else {
				*armOffset = 0xE92D40F8; // STMFD SP!, {R3-R7,LR}
			}
            ce9->patches->cardEndReadDmaRef = offsetDmaHandler==0 ? armOffset : (u32*)offsetDmaHandler;
        }
	  return true;
    }
	return false;
}

bool setDmaPatched = false;
static bool patchCardSetDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	/*const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars
	 || strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version)
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man
	 || !cardReadDMA) return false;*/

	//dbg_printf("\npatchCardSetDma\n");           

    u32* setDmaoffset = findCardSetDma(ndsHeader,moduleParams,usesThumb);
    if(setDmaoffset) {
      /*dbg_printf("\nNDMA CARD SET METHOD ACTIVE\n");       
    dbg_printf("cardSetDma location : ");
    dbg_hexa((u32)setDmaoffset);
    dbg_printf("\n\n");*/
      u32* cardSetDmaPatch = (usesThumb ? ce9->thumbPatches->card_set_dma_arm9 : ce9->patches->card_set_dma_arm9);
	  tonccpy(setDmaoffset, cardSetDmaPatch, 0x30);
	  setDmaPatched = true;

      return true;  
    }

    return false; 
}

bool softResetMb = false;
static void patchReset(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	softResetMb = false;
	u32* reset = findResetOffset(ndsHeader, moduleParams, &softResetMb);

	if (!reset) {
		return;
	}

	// Patch
	u32* resetPatch = ce9->patches->reset_arm9;
	tonccpy(reset, resetPatch, 0x40);
	/*dbg_printf("reset location : ");
	dbg_hexa((u32)reset);
	dbg_printf("\n\n");*/
}

static bool getSleep(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	/*bool ROMsupportsDsiMode = (ndsHeader->unitCode > 0 && dsiModeConfirmed);
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AH9", 3) == 0 // Tony Hawk's American Sk8land
	) {
		return false;
	} else {*/
		if ((valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM)) {
			return false;
		} else if (valueBits & ROMinRAM) {
			/*if (!cardReadDMA || extendedMemoryConfirmed || ROMsupportsDsiMode)*/ return false;
		} else {
			if (/* !cardDmaImprove &&*/ !(valueBits & asyncCardRead)) return false;
		}
	//}

	bool sleepFuncIsThumb = false;
	// Work-around for minorly unstable card read DMA inplementation
	u32* offset = findSleepOffset(ndsHeader, moduleParams, usesThumb, &sleepFuncIsThumb);
	if (offset) {
		if (sleepFuncIsThumb) {
			ce9->thumbPatches->sleepRef = offset;
		} else {
			ce9->patches->sleepRef = offset;
		}
		/*dbg_printf("sleep location : ");
		dbg_hexa((u32)offset);
		dbg_printf("\n\n");*/
	}
	return offset ? true : false;
}

bool a9PatchCardIrqEnable(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	/*const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars - Fix white screen on boot
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars - Fix white screen on boot
	 || strncmp(romTid, "AWD", 3) == 0	// Diddy Kong Racing - Fix corrupted 3D model bug
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata - Fix touch and model rendering bug
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version) - Fix black screen on boot
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time - Fix white screen on boot
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man - Fix white screen on boot
	) return true;*/

	bool usesThumb = false;

	// Card irq enable
	u32* cardIrqEnableOffset = a9FindCardIrqEnableOffset(ndsHeader, moduleParams, &usesThumb);
	if (!cardIrqEnableOffset) {
		return false;
	}
	u32* cardIrqEnablePatch = (usesThumb ? ce9->thumbPatches->card_irq_enable : ce9->patches->card_irq_enable);
	tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, usesThumb ? 0x20 : 0x30);
    /*dbg_printf("cardIrqEnable location : ");
    dbg_hexa((u32)cardIrqEnableOffset);
    dbg_printf("\n\n");*/
	return true;
}

static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion) {
	//if (patchMpuRegion == 2 || isSdk5(moduleParams)) return;

	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	// Find the mpu init
	u32* mpuStartOffset = findMpuStartOffset(ndsHeader, patchMpuRegion);
	/*if (mpuStartOffset) {
		dbg_printf("Mpu start: ");
		dbg_hexa((u32)mpuStartOffset);
		dbg_printf("\n\n");
	}*/
	u32* mpuDataOffset = findMpuDataOffset(moduleParams, patchMpuRegion, mpuStartOffset);
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

		/*dbg_printf("Mpu data: ");
		dbg_hexa((u32)mpuDataOffset);
		dbg_printf("\n\n");*/
	}

	// Patch out all further mpu reconfiguration
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	u32* mpuInitOffset = (u32*)mpuStartOffset;
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
		/*dbg_printf("Mpu init: ");
		dbg_hexa((u32)mpuInitOffset);
		dbg_printf("\n\n");*/

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

	//if (!isSdk5(moduleParams)) {
		u32* mpuDataOffsetAlt = findMpuDataOffsetAlt(ndsHeader);
		if (mpuDataOffsetAlt) {
			unpatchedFuncs->mpuDataOffsetAlt = mpuDataOffsetAlt;
			unpatchedFuncs->mpuInitRegionOldDataAlt = *mpuDataOffsetAlt;

			*mpuDataOffsetAlt = PAGE_32M | 0x02000000 | 1;

			/*dbg_printf("Mpu data alt: ");
			dbg_hexa((u32)mpuDataOffsetAlt);
			dbg_printf("\n\n");*/
		}
	//}
}

static void patchMpu2(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x2008000 || moduleParams->sdk_version > 0x5000000) {
		return;
	}

	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	// Find the mpu init
	u32* mpuStartOffset = findMpuStartOffset(ndsHeader, 2);
	/*if (mpuStartOffset) {
		dbg_printf("Mpu start 2: ");
		dbg_hexa((u32)mpuStartOffset);
		dbg_printf("\n\n");
	}*/
	u32* mpuDataOffset = findMpuDataOffset(moduleParams, 2, mpuStartOffset);
	if (mpuDataOffset) {
		// Change the region 2 configuration

		//force DSi mode settings. THESE TOOK AGES TO FIND. -s2k
		/*if(ndsHeader->unitCode > 0 && dsiModeConfirmed){
			u32 mpuNewDataAccess     = 0x15111111;
			u32 mpuNewInstrAccess    = 0x5111111;
			u32 mpuInitRegionNewData = PAGE_32M | 0x0C000000 | 1;
			int mpuAccessOffset      = 3;

			unpatchedFuncs->mpuDataOffset2 = mpuDataOffset;
			unpatchedFuncs->mpuInitRegionOldData2 = *mpuDataOffset;
			*mpuDataOffset = mpuInitRegionNewData;

		//if (mpuAccessOffset) {
			unpatchedFuncs->mpuAccessOffset = mpuAccessOffset;
			//if (mpuNewInstrAccess) {
				unpatchedFuncs->mpuOldInstrAccess = mpuDataOffset[mpuAccessOffset];
				mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
			//}
			//if (mpuNewDataAccess) {
				unpatchedFuncs->mpuOldDataAccess = mpuDataOffset[mpuAccessOffset + 1];
				mpuDataOffset[mpuAccessOffset + 1] = mpuNewDataAccess;
			//}
		//}
		} else {*/
			//Original code made loading slow, so new code is used
			unpatchedFuncs->mpuDataOffset2 = mpuDataOffset;
			unpatchedFuncs->mpuInitRegionOldData2 = *mpuDataOffset;
			*mpuDataOffset = PAGE_128K | 0x027E0000 | 1;
		//}

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

		/*dbg_printf("Mpu data 2: ");
		dbg_hexa((u32)mpuDataOffset);
		dbg_printf("\n\n");*/
	}

	// Patch out all further mpu reconfiguration
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(2);
	u32* mpuInitOffset = (u32*)mpuStartOffset;
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
		/*dbg_printf("Mpu init 2: ");
		dbg_hexa((u32)mpuInitOffset);
		dbg_printf("\n\n");*/

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
}

static void randomPatch(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	//const char* romTid = getRomTid(ndsHeader);

	// Random patch
	if (moduleParams->sdk_version > 0x3000000
	/*&& strncmp(romTid, "AKT", 3) != 0  // Doctor Tendo
	&& strncmp(romTid, "ACZ", 3) != 0  // Cars
	&& strncmp(romTid, "ABC", 3) != 0  // Harvest Moon DS
	&& strncmp(romTid, "AWL", 3) != 0*/) // TWEWY
	{
		u32* randomPatchOffset = findRandomPatchOffset(ndsHeader);
		if (randomPatchOffset) {
			// Patch
			//*(u32*)((u32)randomPatchOffset + 0xC) = 0x0;
			*(randomPatchOffset + 3) = 0x0;
		}
	}
}

/*static void setFlushCache(cardengineArm9* ce9, u32 patchMpuRegion, bool usesThumb) {
	ce9->patches->needFlushDCCache = (patchMpuRegion == 1);
}*/

u32 patchCardNdsArm9(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion) {

	bool usesThumb;
	//bool slot2usesThumb = false;
	int readType;
	u32* cardReadEndOffset;
	u32* cardPullOutOffset;

	a9PatchCardIrqEnable(ce9, ndsHeader, moduleParams);

    u32 startOffset = (u32)ndsHeader->arm9destination;

    /*dbg_printf("startOffset : ");
    dbg_hexa(startOffset);
    dbg_printf("\n\n");*/

	patchMpu(ndsHeader, moduleParams, patchMpuRegion);
	patchMpu2(ndsHeader, moduleParams);

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &cardReadEndOffset, startOffset)) {
		//dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

	fixForDifferentBios(ce9, ndsHeader, usesThumb);

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, &cardPullOutOffset);

	//patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset);

	if (getSleep(ce9, ndsHeader, moduleParams, usesThumb)) {
		patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
	} else {
		if (!patchCardSetDma(ce9, ndsHeader, moduleParams, usesThumb)) {
			patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
		}
		if (!patchCardEndReadDma(ce9, ndsHeader, moduleParams, usesThumb)) {
			randomPatch(ndsHeader, moduleParams);
		}
	}

    //patchSleep(ce9, ndsHeader, moduleParams, usesThumb);

	patchReset(ce9, ndsHeader, moduleParams);

	//setFlushCache(ce9, patchMpuRegion, usesThumb);

	//dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
