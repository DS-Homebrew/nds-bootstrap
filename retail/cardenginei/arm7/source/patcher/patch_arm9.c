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

bool isPawsAndClaws(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AQ2", 3) == 0 // Paws & Claws: Pet Resort
	 || strncmp(romTid, "YMU", 3) == 0 // Paws & Claws: Pet Vet 2
	) {
		return true;
	}

	return false;
}

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

static bool patchCardRead(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumbPtr, int* readTypePtr, int* sdk5ReadTypePtr, u32** cardReadEndOffsetPtr, u32 startOffset) {
	bool usesThumb = false;
	int readType = 0;
	int sdk5ReadType = 0; // SDK 5

	// Card read
	// SDK 5
	//dbg_printf("Trying SDK 5 thumb...\n");
    u32* cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type1(ndsHeader, moduleParams, startOffset);
	if (cardReadEndOffset) {
		sdk5ReadType = 1;
		usesThumb = true;
	}
	if (!cardReadEndOffset) {
		// SDK 5
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type0(ndsHeader, moduleParams, startOffset);
		if (cardReadEndOffset) {
			sdk5ReadType = 0;
			usesThumb = true;
		}
	}
	if (!cardReadEndOffset) {
		//dbg_printf("Trying thumb...\n");
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb(ndsHeader, startOffset);
		if (cardReadEndOffset) {
			usesThumb = true;
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
				// dbg_printf("Found thumb\n\n");
				--cardReadEndOffset;
				usesThumb = true;
			}
		}
	}
	*usesThumbPtr = usesThumb;
	*readTypePtr = readType;
	*sdk5ReadTypePtr = sdk5ReadType; // SDK 5
	*cardReadEndOffsetPtr = cardReadEndOffset;
	if (!cardReadEndOffset) { // Not necessarily needed
		return false;
	}
    u32* cardReadStartOffset = NULL;
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
	tonccpy(cardReadStartOffset, cardReadPatch, usesThumb ? ((moduleParams->sdk_version > 0x5000000) ? 0xB0 : 0xA0) : 0xE0); // 0xE0 = 0xF0 - 0x08
    /* dbg_printf("cardRead location : ");
    dbg_hexa((u32)cardReadStartOffset);
    dbg_printf("\n");
    dbg_hexa((u32)ce9);
    dbg_printf("\n\n"); */
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

	/* dbg_printf("cardReadMvDK4 location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n"); */
	return true;
}

static void patchCardPullOut(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, int sdk5ReadType, u32** cardPullOutOffsetPtr) {
	// Card pull out
	u32* cardPullOutOffset = NULL;
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
	*cardPullOutOffsetPtr = cardPullOutOffset;
	if (!cardPullOutOffset) {
		return;
	}

	// Patch
	u32* cardPullOutPatch = (usesThumb ? ce9->thumbPatches->card_pull_out_arm9 : ce9->patches->card_pull_out_arm9);
	tonccpy(cardPullOutOffset, cardPullOutPatch, usesThumb ? 0x2 : 0x30);
    /* dbg_printf("cardPullOut location : ");
    dbg_hexa((u32)cardPullOutOffset);
    dbg_printf("\n\n"); */
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
	if (!isPawsAndClaws(ndsHeader) && !cardReadEndOffset) {
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
		/* dbg_printf("cardId location : ");
		dbg_hexa((u32)cardIdStartOffset);
		dbg_printf("\n\n"); */
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
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars
	 || strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version)
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man
	 || strncmp(romTid, "V2G", 3) == 0 // Mario vs. Donkey Kong: Mini-Land Mayhem (DS mode)
	/* || !cardReadDMA*/) return false;

    u32* offset = NULL;
    u32 offsetDmaHandler = 0;
	u32* cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader, moduleParams);
	if (!cardReadDmaEndOffset && usesThumb) {
		cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
	}
	offset = findCardEndReadDma(ndsHeader,moduleParams,usesThumb,cardReadDmaEndOffset,&offsetDmaHandler);
    if(offset) {
     /* dbg_printf("\nNDMA CARD READ METHOD ACTIVE\n");
    dbg_printf("cardEndReadDma location : ");
    dbg_hexa((u32)offset);
    dbg_printf("\n\n"); */
      if(moduleParams->sdk_version < 0x5000000) {
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
      } else {
        // SDK5 
        if(usesThumb) {
            u16* thumbOffset = (u16*)offset;
            while(*thumbOffset!=0xB508) { // push	{r3, lr}
                thumbOffset--;
            }
            ce9->thumbPatches->cardEndReadDmaRef = (u32*)thumbOffset;
            thumbOffset[1] = 0x46C0; // NOP
            thumbOffset[2] = 0x46C0; // NOP
            thumbOffset[3] = 0x46C0; // NOP
            thumbOffset[4] = 0x46C0; // NOP
        } else  {
            u32* armOffset = (u32*)offset;
            armOffset--;
			*armOffset = 0xE92D4008; // STMFD SP!, {R3,LR}
            ce9->patches->cardEndReadDmaRef = armOffset;
        }  
	  }
	  return true;
    }
	return false;
}

bool setDmaPatched = false;
static bool patchCardSetDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars
	 || strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version)
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man
	 || strncmp(romTid, "V2G", 3) == 0 // Mario vs. Donkey Kong: Mini-Land Mayhem (DS mode)
	/* || !cardReadDMA*/) return false;

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
	const char* romTid = getRomTid(ndsHeader);
	if (strcmp(romTid, "NTRJ") == 0 || strncmp(romTid, "HND", 3) == 0 || strncmp(romTid, "HNE", 3) == 0) {
		u32* offset = findSrlStartOffset9(ndsHeader);

		if (offset) {
			// Patch
			tonccpy(offset, ce9->patches->reset_arm9, 0x40);
			/* dbg_printf("srlStart location : ");
			dbg_hexa((u32)offset);
			dbg_printf("\n\n"); */
			softResetMb = true;
		}
	}

	u32* offset = findResetOffset(ndsHeader, moduleParams, softResetMb);

	if (!offset) {
		return;
	}

	// Patch
	tonccpy(offset, ce9->patches->reset_arm9, 0x40);
	/* dbg_printf("reset location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n"); */
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
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars - Fix white screen on boot
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars - Fix white screen on boot
	 || strncmp(romTid, "AWD", 3) == 0	// Diddy Kong Racing - Fix corrupted 3D model bug
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata - Fix touch and model rendering bug
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn - Fix black screen on boot
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version) - Fix black screen on boot
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time - Fix white screen on boot
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man - Fix white screen on boot
	) return true;

	bool usesThumb = false;

	// Card irq enable
	u32* cardIrqEnableOffset = a9FindCardIrqEnableOffset(ndsHeader, moduleParams, &usesThumb);
	if (!cardIrqEnableOffset) {
		return false;
	}
	u32* cardIrqEnablePatch = (usesThumb ? ce9->thumbPatches->card_irq_enable : ce9->patches->card_irq_enable);
	tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, usesThumb ? 0x18 : 0x30);
    /*dbg_printf("cardIrqEnable location : ");
    dbg_hexa((u32)cardIrqEnableOffset);
    dbg_printf("\n\n");*/
	return true;
}

static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion) {
	if (patchMpuRegion == 2 || ndsHeader->unitCode > 0) return;

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
		if (moduleParams->sdk_version < 0x5000000) {
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

	if (moduleParams->sdk_version < 0x5000000 && unpatchedFuncs->mpuInitRegionOldData != 0x200002B) {
		u32* mpuDataOffsetAlt = findMpuDataOffsetAlt(ndsHeader);
		if (mpuDataOffsetAlt) {
			unpatchedFuncs->mpuDataOffsetAlt = mpuDataOffsetAlt;
			unpatchedFuncs->mpuInitRegionOldDataAlt = *mpuDataOffsetAlt;

			*mpuDataOffsetAlt = PAGE_32M | 0x02000000 | 1;

			/* dbg_printf("Mpu data alt: ");
			dbg_hexa((u32)mpuDataOffsetAlt);
			dbg_printf("\n\n"); */
		}
	}
}

static void patchMpu2(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (ndsHeader->unitCode > 0) {
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
	if (mpuDataOffset && (moduleParams->sdk_version < 0x5000000)) {
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
			*mpuDataOffset = 0;
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

		u32 mpuInitOffsetInSrl = (u32)mpuInitOffset;
		mpuInitOffsetInSrl -= (u32)ndsHeader->arm9destination;

		if (mpuInitOffsetInSrl >= 0 && mpuInitOffsetInSrl < 0x4000) {
			unpatchedFuncs->mpuInitOffset2 = mpuInitOffset;
		}
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

void patchMpuChange(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	u32* offset = findMpuChange(ndsHeader);

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

	/* dbg_printf("Mpu change: ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n"); */
}

static void randomPatch(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	const char* romTid = getRomTid(ndsHeader);

	// Random patch
	if (moduleParams->sdk_version > 0x3000000
	&& strncmp(romTid, "AKT", 3) != 0  // Doctor Tendo
	&& strncmp(romTid, "ACZ", 3) != 0  // Cars
	&& strncmp(romTid, "ABC", 3) != 0  // Harvest Moon DS
	&& strncmp(romTid, "AWL", 3) != 0) // TWEWY
	{
		u32* randomPatchOffset = findRandomPatchOffset(ndsHeader);
		if (randomPatchOffset) {
			// Patch
			//*(u32*)((u32)randomPatchOffset + 0xC) = 0x0;
			*(randomPatchOffset + 3) = 0x0;
		}
	}
}

// SDK 5
static void randomPatch5Second(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	// Random patch SDK 5 second
	u32* randomPatchOffset5Second = findRandomPatchOffset5Second(ndsHeader);
	if (!randomPatchOffset5Second) {
		return;
	}
	// Patch
	*randomPatchOffset5Second = 0xE3A00000;
	//*(u32*)((u32)randomPatchOffset5Second + 4) = 0xE12FFF1E;
	*(randomPatchOffset5Second + 1) = 0xE12FFF1E;
}

/*static void setFlushCache(cardengineArm9* ce9, u32 patchMpuRegion, bool usesThumb) {
	ce9->patches->needFlushDCCache = (patchMpuRegion == 1);
}*/

u32 patchCardNdsArm9(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion) {

	bool usesThumb;
	//bool slot2usesThumb = false;
	int readType;
	int sdk5ReadType; // SDK 5
	u32* cardReadEndOffset;
	u32* cardPullOutOffset;

	a9PatchCardIrqEnable(ce9, ndsHeader, moduleParams);

    const char* romTid = getRomTid(ndsHeader);

    u32 startOffset = (u32)ndsHeader->arm9destination;
    if (strncmp(romTid, "UOR", 3) == 0) { // Start at 0x2003800 for "WarioWare: DIY"
        startOffset = (u32)ndsHeader->arm9destination + 0x3800;
    } else if (strncmp(romTid, "UXB", 3) == 0) { // Start at 0x2080000 for "Jam with the Band"
        startOffset = (u32)ndsHeader->arm9destination + 0x80000;
    } else if (strncmp(romTid, "USK", 3) == 0) { // Start at 0x20E8000 for "Face Training"
        startOffset = (u32)ndsHeader->arm9destination + 0xE4000;
    }

    /*dbg_printf("startOffset : ");
    dbg_hexa(startOffset);
    dbg_printf("\n\n");*/

	patchMpu(ndsHeader, moduleParams, patchMpuRegion);
	patchMpu2(ndsHeader, moduleParams);
	patchMpuChange(ndsHeader, moduleParams);

	if (isPawsAndClaws(ndsHeader)) {
		patchCardId(ce9, ndsHeader, moduleParams, false, NULL); // Patch card ID first
	}

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &sdk5ReadType, &cardReadEndOffset, startOffset)) {
		//dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

	fixForDifferentBios(ce9, ndsHeader, usesThumb);

	if (strncmp(romTid, "V2G", 3) == 0) {
		// try to patch card read a second time
		// dbg_printf("patch card read a second time\n");
		// dbg_printf("startOffset : 0x02030000\n\n");
		if (!patchCardReadMvDK4(0x02030000)) {
			// dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
	}

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, sdk5ReadType, &cardPullOutOffset);

	//patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	if (!isPawsAndClaws(ndsHeader)) {
		patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset);
	}

	if (getSleep(ce9, ndsHeader, moduleParams, usesThumb)) {
		patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
	} else {
		if (!patchCardSetDma(ce9, ndsHeader, moduleParams, usesThumb)) {
			patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
		}
		if (!patchCardEndReadDma(ce9, ndsHeader, moduleParams, usesThumb)) {
			randomPatch(ndsHeader, moduleParams);
			randomPatch5Second(ndsHeader, moduleParams);
		}
	}

    //patchSleep(ce9, ndsHeader, moduleParams, usesThumb);

	patchReset(ce9, ndsHeader, moduleParams);

	//setFlushCache(ce9, patchMpuRegion, usesThumb);

	//dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
