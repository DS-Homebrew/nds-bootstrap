#include <string.h>
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
#include "cardengine_header_arm9.h"
#include "debug_file.h"
#include "tonccpy.h"

//#define memcpy __builtin_memcpy // memcpy

extern u16 gameOnFlashcard;
extern u16 saveOnFlashcard;
extern u16 a9ScfgRom;
extern u8 cardReadDMA;

extern bool gbaRomFound;
extern bool dsiModeConfirmed;

static void fixForDifferentBios(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, bool usesThumb) {
	bool ROMisDsiEnhanced = (ndsHeader->unitCode > 0);

	u32* swi12Offset = patchOffsetCache.a9Swi12Offset;
	u32* dsiModeCheckOffset = patchOffsetCache.dsiModeCheckOffset;
	u32* dsiModeCheck2Offset = patchOffsetCache.dsiModeCheck2Offset;
	if (!patchOffsetCache.a9Swi12Offset) {
		swi12Offset = a9_findSwi12Offset(ndsHeader);
		if (swi12Offset) {
			patchOffsetCache.a9Swi12Offset = swi12Offset;
		}
	}
	if (ROMisDsiEnhanced) {
		if (!patchOffsetCache.dsiModeCheckOffset) {
			dsiModeCheckOffset = findDsiModeCheckOffset(ndsHeader);
			if (dsiModeCheckOffset) patchOffsetCache.dsiModeCheckOffset = dsiModeCheckOffset;
		}
		if (!patchOffsetCache.dsiModeCheck2Checked) {
			dsiModeCheck2Offset = findDsiModeCheck2Offset(dsiModeCheckOffset, usesThumb);
			if (dsiModeCheck2Offset) patchOffsetCache.dsiModeCheck2Offset = dsiModeCheck2Offset;
			patchOffsetCache.dsiModeCheck2Checked = true;
		}
	}

	// swi 0x12 call
	if (swi12Offset && (u8)a9ScfgRom == 1 && (!(REG_SCFG_ROM & BIT(1)) || dsiModeConfirmed)) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = ce9->patches->swi02;
		memcpy(swi12Offset, swi12Patch, 0x4);
	}
	if (dsiModeCheckOffset && ROMisDsiEnhanced) {
		// Patch to return as DS BIOS
		if (dsiModeConfirmed && (u8)a9ScfgRom != 1) {
			dsiModeCheckOffset[7] = 0x2EFFFD0;
			dbg_printf("Running DSi mode with DS BIOS\n");

			if (dsiModeCheck2Offset) {
				if (dsiModeCheck2Offset[0] == 0xE59F0014) {
					dsiModeCheck2Offset[7] = 0x2EFFFD0;
				} else {
					dsiModeCheck2Offset[usesThumb ? 22/sizeof(u16) : 18] = 0x2EFFFD0;
				}
			}
		} else if (!dsiModeConfirmed && extendedMemoryConfirmed && !(REG_SCFG_ROM & BIT(1))) {
			dsiModeCheckOffset[0] = 0xE3A00001;	// mov r0, #1
			dsiModeCheckOffset[1] = 0xE12FFF1E;	// bx lr

			if (dsiModeCheck2Offset) {
				if (dsiModeCheck2Offset[0] == 0xE59F0014 || !usesThumb) {
					dsiModeCheck2Offset[0] = 0xE3A00000;	// mov r0, #0
					dsiModeCheck2Offset[1] = 0xE12FFF1E;	// bx lr
				} else {
					u16* dsiModeCheck2OffsetThumb = (u16*)dsiModeCheck2Offset;
					dsiModeCheck2OffsetThumb[0] = 0x2000;	// movs r0, #0
					dsiModeCheck2OffsetThumb[1] = 0x4770;	// bx lr
				}
			}
		}
	}

    dbg_printf("swi12 location : ");
    dbg_hexa((u32)swi12Offset);
    dbg_printf("\n\n");
	if (ROMisDsiEnhanced) {
		dbg_printf("dsiModeCheck location : ");
		dbg_hexa((u32)dsiModeCheckOffset);
		dbg_printf("\n\n");
		if (dsiModeCheck2Offset) {
			dbg_printf("dsiModeCheck2 location : ");
			dbg_hexa((u32)dsiModeCheck2Offset);
			dbg_printf("\n\n");
		}
	}
}

static bool patchCardHashInit(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	u32* cardHashInitOffset = patchOffsetCache.cardHashInitOffset;
	if (!patchOffsetCache.cardHashInitOffset) {
		cardHashInitOffset = usesThumb ? (u32*)findCardHashInitOffsetThumb(ndsHeader, moduleParams) : findCardHashInitOffset(ndsHeader, moduleParams);
		if (cardHashInitOffset) {
			patchOffsetCache.cardHashInitOffset = cardHashInitOffset;
			patchOffsetCacheChanged = true;
		}
	}

	if (cardHashInitOffset) {
		if (cardHashInitOffset < *(u32*)0x02FFE1C8) {
			if (usesThumb) {
				u16* cardHashInitOffsetThumb = (u16*)cardHashInitOffset;
				cardHashInitOffsetThumb[-2] = 0xB003;	// ADD SP, SP, #0xC
				cardHashInitOffsetThumb[-1] = 0xBD78;	// POP {R3-R6,PC}
			} else if (*cardHashInitOffset == 0xE280101F) {
				cardHashInitOffset[-2] = 0xE28DD00C;	// ADD SP, SP, #0xC
				cardHashInitOffset[-1] = 0xE8BD8078;	// LDMFD SP!, {R3-R6,PC}
			} else {
				cardHashInitOffset[-1] = 0xE3A00000;	// mov r0, #0
			}
		} else if (usesThumb) {
			*(u16*)cardHashInitOffset = 0x4770;	// bx lr
		} else {
			*cardHashInitOffset = 0xE12FFF1E;	// bx lr
		}
	} else {
		return false;
	}

    dbg_printf("cardHashInit location : ");
    dbg_hexa((u32)cardHashInitOffset);
    dbg_printf("\n\n");
	return true;
}

static void patchCardRomInit(u32* cardReadEndOffset, bool usesThumb) {
	u32* cardRomInitOffset = patchOffsetCache.cardRomInitOffset;
	if (!patchOffsetCache.cardRomInitOffset) {
		cardRomInitOffset = usesThumb ? (u32*)findCardRomInitOffsetThumb((u16*)cardReadEndOffset) : findCardRomInitOffset(cardReadEndOffset);
		if (cardRomInitOffset) {
			patchOffsetCache.cardRomInitOffset = cardRomInitOffset;
			patchOffsetCacheChanged = true;
		}
	}

	if (cardRomInitOffset) {
		u16* cardRomInitOffsetThumb = (u16*)cardRomInitOffset;
		if (usesThumb && cardRomInitOffsetThumb[0] == 0xB578) {
			cardRomInitOffsetThumb[7] = 0x2800;
		} else if (usesThumb) {
			cardRomInitOffsetThumb[6] = 0x2800;
		} else if (cardRomInitOffset[0] == 0xE92D4078 && cardRomInitOffset[1] == 0xE24DD00C) {
			cardRomInitOffset[6] = 0xE3500000;
		} else {
			cardRomInitOffset[5] = 0xE3500000;
		}
	}

    dbg_printf("cardRomInit location : ");
    dbg_hexa((u32)cardRomInitOffset);
    dbg_printf("\n\n");
}

static bool patchCardRead(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumbPtr, int* readTypePtr, int* sdk5ReadTypePtr, u32** cardReadEndOffsetPtr, bool useCache, u32 startOffset) {
	bool usesThumb = patchOffsetCache.a9IsThumb;
	int readType = 0;
	int sdk5ReadType = 0; // SDK 5

	// Card read
	// SDK 5
	//dbg_printf("Trying SDK 5 thumb...\n");
    u32* cardReadEndOffset = 0;
    if (useCache) cardReadEndOffset = patchOffsetCache.cardReadEndOffset;
	if (!patchOffsetCache.cardReadEndOffset || !useCache) {
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
		if (cardReadEndOffset && useCache) {
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
    u32* cardReadStartOffset = 0;
    if (useCache) cardReadStartOffset = patchOffsetCache.cardReadStartOffset;
	if (!patchOffsetCache.cardReadStartOffset || !useCache) {
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
		if (cardReadStartOffset && useCache) {
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
	memcpy(cardReadStartOffset, cardReadPatch, usesThumb ? (isSdk5(moduleParams) ? 0xB0 : 0xA0) : 0xE0); // 0xE0 = 0xF0 - 0x08
    dbg_printf("cardRead location : ");
    dbg_hexa(cardReadStartOffset);
    dbg_printf("\n");
    dbg_hexa((u32)ce9);
    dbg_printf("\n\n");

	/*if (ndsHeader->unitCode > 0 && dsiModeConfirmed) {
		cardReadStartOffset = patchOffsetCache.cardReadHashOffset;
		if (!patchOffsetCache.cardReadHashOffset) {
			cardReadStartOffset = findCardReadHashOffset();
			if (cardReadStartOffset) {
				patchOffsetCache.cardReadHashOffset = cardReadStartOffset;
				patchOffsetCacheChanged = true;
			} else {
				return false;
			}
		}

		if (cardReadStartOffset) {
			cardReadPatch = ce9->patches->card_read_arm9;
			memcpy(cardReadStartOffset, cardReadPatch, 0x2C);
		}

		dbg_printf("cardReadHash location : ");
		dbg_hexa(cardReadStartOffset);
		dbg_printf("\n\n");
	}*/
	return true;
}

/*static bool patchCardReadMvDK4(cardengineArm9* ce9, u32 startOffset) {
	u32* cardReadStartOffset = findCardReadStartOffsetMvDK4(startOffset);
	if (!cardReadStartOffset) {
		return false;
	}

	u32* cardReadPatch = ce9->patches->card_dma_arm9;
	memcpy(cardReadStartOffset, cardReadPatch, 0x60);
    dbg_printf("cardReadDma location : ");
    dbg_hexa(cardReadStartOffset);
    dbg_printf("\n\n");
	return true;
}*/

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
	u32* cardPullOutPatch = (usesThumb ? ce9->thumbPatches->card_pull_out_arm9 : ce9->patches->card_pull_out_arm9);
	memcpy(cardPullOutOffset, cardPullOutPatch, usesThumb ? 0x2 : 0x30);
    dbg_printf("cardPullOut location : ");
    dbg_hexa(cardPullOutOffset);
    dbg_printf("\n\n");
}

/*static void patchCardTerminateForPullOut(cardengineArm9* ce9, bool usesThumb, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32* cardPullOutOffset) {
	if (!cardPullOutOffset) {
		return;
	}

	u32* cardTerminateForPullOutOffset = findCardTerminateForPullOutOffset(ndsHeader, moduleParams);

	// Patch
	u32* cardTerminateForPullOutPatch = (usesThumb ? ce9->thumbPatches->terminateForPullOutRef : ce9->patches->terminateForPullOutRef);
	*cardTerminateForPullOutPatch = (u32)cardTerminateForPullOutOffset;
}*/

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
	memcpy(forceToPowerOffOffset, cardPullOutPatch, 0x4);
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
		memcpy(cardIdStartOffset, cardIdPatch, usesThumb ? 0x8 : 0xC);
		dbg_printf("cardId location : ");
		dbg_hexa(cardIdStartOffset);
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
		if (cardReadDmaEndOffset) {
			patchOffsetCache.cardReadDmaEndOffset = cardReadDmaEndOffset;
		}
		patchOffsetCache.cardReadDmaChecked = true;
		patchOffsetCacheChanged = true;
	}
	if (!cardReadDmaStartOffset) {
		return;
	}
	// Patch
	u32* cardReadDmaPatch = (usesThumb ? ce9->thumbPatches->card_dma_arm9 : ce9->patches->card_dma_arm9);
	memcpy(cardReadDmaStartOffset, cardReadDmaPatch, 0x40);
    dbg_printf("cardReadDma location : ");
    dbg_hexa(cardReadDmaStartOffset);
    dbg_printf("\n\n");
}

static bool patchCardEndReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32 ROMinRAM) {
	if (ndsHeader->unitCode == 0 || !dsiModeConfirmed) {
		const char* romTid = getRomTid(ndsHeader);

		if (strncmp(romTid, "ACV", 3) == 0
		 || strncmp(romTid, "AJS", 3) == 0
		 || strncmp(romTid, "AWI", 3) == 0
		 || strncmp(romTid, "AJU", 3) == 0
		 || strncmp(romTid, "AWD", 3) == 0
		 || strncmp(romTid, "CP3", 3) == 0
		 || strncmp(romTid, "BO5", 3) == 0
		 || strncmp(romTid, "Y8L", 3) == 0
		 || strncmp(romTid, "B8I", 3) == 0
		 || strncmp(romTid, "TAM", 3) == 0
		 || (gameOnFlashcard && !ROMinRAM) || !cardReadDMA) return false;
	}

    u32* offset = patchOffsetCache.cardEndReadDmaOffset;
	  if (!patchOffsetCache.cardEndReadDmaChecked) {
		if (!patchOffsetCache.cardReadDmaEndOffset) {
			u32* cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader, moduleParams);
			if (!cardReadDmaEndOffset && usesThumb) {
				cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
			}
			if (cardReadDmaEndOffset) {
				patchOffsetCache.cardReadDmaEndOffset = cardReadDmaEndOffset;
			}
		}
		offset = findCardEndReadDma(ndsHeader,moduleParams,usesThumb,patchOffsetCache.cardReadDmaEndOffset);
		if (offset) patchOffsetCache.cardEndReadDmaOffset = offset;
		patchOffsetCache.cardEndReadDmaChecked = true;
		patchOffsetCacheChanged = true;
	  }
    if(offset) {
      dbg_printf("\nNDMA CARD READ METHOD ACTIVE\n");
    dbg_printf("cardEndReadDma location : ");
    dbg_hexa(offset);
    dbg_printf("\n\n");
      if(!isSdk5(moduleParams)) {
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
            ce9->thumbPatches->cardEndReadDmaRef = thumbOffset;
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
			} else {
				*armOffset = 0xE92D40F8; // STMFD SP!, {R3-R7,LR}
			}
            ce9->patches->cardEndReadDmaRef = armOffset;
        }
      } else {
        // SDK5 
        if(usesThumb) {
            u16* thumbOffset = (u16*)offset;
            while(*thumbOffset!=0xB508) { // push	{r3, lr}
                thumbOffset--;
            }
            ce9->thumbPatches->cardEndReadDmaRef = thumbOffset;
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
static bool patchCardSetDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32 ROMinRAM) {
	bool ROMsupportsDsiMode = (ndsHeader->unitCode > 0 && dsiModeConfirmed);

	if (ROMsupportsDsiMode && !gameOnFlashcard && !ROMinRAM) {
		return false;
	}
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "ACV", 3) == 0
	 || strncmp(romTid, "AJS", 3) == 0
	 || strncmp(romTid, "AWI", 3) == 0
	 || strncmp(romTid, "AJU", 3) == 0
	 || strncmp(romTid, "AWD", 3) == 0
	 || strncmp(romTid, "CP3", 3) == 0
	 || strncmp(romTid, "BO5", 3) == 0
	 || strncmp(romTid, "Y8L", 3) == 0
	 || strncmp(romTid, "B8I", 3) == 0
	 || strncmp(romTid, "TAM", 3) == 0
	 || (gameOnFlashcard && !ROMsupportsDsiMode && !ROMinRAM) || !cardReadDMA) return false;

	dbg_printf("\npatchCardSetDma\n");           

    u32* setDmaoffset = patchOffsetCache.cardSetDmaOffset;
    if (!patchOffsetCache.cardSetDmaChecked) {
		setDmaoffset = findCardSetDma(ndsHeader,moduleParams,usesThumb);
		if (setDmaoffset) {
			patchOffsetCache.cardSetDmaOffset = setDmaoffset;
		}
		patchOffsetCache.cardSetDmaChecked = true;
		patchOffsetCacheChanged = true;
    }
    if(setDmaoffset) {
      dbg_printf("\nNDMA CARD SET METHOD ACTIVE\n");       
    dbg_printf("cardSetDma location : ");
    dbg_hexa(setDmaoffset);
    dbg_printf("\n\n");
      u32* cardSetDmaPatch = (usesThumb ? ce9->thumbPatches->card_set_dma_arm9 : ce9->patches->card_set_dma_arm9);
	  memcpy(setDmaoffset, cardSetDmaPatch, 0x30);
	  setDmaPatched = true;

      return true;  
    }

    return false; 
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
		memcpy(reset, resetPatch, 0x40);
		dbg_printf("reset location : ");
		dbg_hexa((u32)reset);
		dbg_printf("\n\n");
	}
}

static bool getSleep(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "A5F", 3) == 0
	 || strncmp(romTid, "B3R", 3) == 0
	 || strncmp(romTid, "B5P", 3) == 0
	 || strncmp(romTid, "BE8", 3) == 0
	 || strncmp(romTid, "BEB", 3) == 0
	 || strncmp(romTid, "BEE", 3) == 0
	 || strncmp(romTid, "BEZ", 3) == 0
	 || strncmp(romTid, "BLF", 3) == 0
	 || strncmp(romTid, "BOE", 3) == 0
	 || strncmp(romTid, "C3J", 3) == 0
	 || strncmp(romTid, "CLJ", 3) == 0
	 || strncmp(romTid, "Y49", 3) == 0
	 || strncmp(romTid, "Y6Z", 3) == 0
	 || strncmp(romTid, "Y9B", 3) == 0
	 || strncmp(romTid, "YEE", 3) == 0
	 || strncmp(romTid, "YEL", 3) == 0
	 || strncmp(romTid, "YEW", 3) == 0
	 || strncmp(romTid, "YLT", 3) == 0
	|| !patchOffsetCache.cardIdOffset
	) return false;

	// Work-around for buggy card read DMA and/or screen flickers during loading
   u32* offset = patchOffsetCache.sleepFuncOffset;
	if (!patchOffsetCache.sleepChecked) {
		offset = findSleepOffset(ndsHeader, moduleParams, usesThumb, &patchOffsetCache.sleepFuncIsThumb);
		if (offset) {
			patchOffsetCache.sleepFuncOffset = offset;
		}
		patchOffsetCache.sleepChecked = true;
		patchOffsetCacheChanged = true;
	}
	if (offset) {
		if (patchOffsetCache.sleepFuncIsThumb) {
			ce9->thumbPatches->sleepRef = offset;
		} else {
			ce9->patches->sleepRef = offset;
		}
		dbg_printf("sleep location : ");
		dbg_hexa(offset);
		dbg_printf("\n\n");
	}
	return offset ? true : false;
}

static bool a9PatchCardIrqEnable(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AJS", 3) == 0 // Jump Super Stars - Fix white screen on boot
	 || strncmp(romTid, "AJU", 3) == 0 // Jump Ultimate Stars - Fix white screen on boot
	 || strncmp(romTid, "AWD", 3) == 0	// Diddy Kong Racing - Fix corrupted 3D model bug
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata - Fix touch and model rendering bug
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version) - Fix black screen on boot
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time - Fix white screen on boot
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man - Fix white screen on boot
	) return true;

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
	memcpy(cardIrqEnableOffset, cardIrqEnablePatch, usesThumb ? 0x20 : 0x30);
    dbg_printf("cardIrqEnable location : ");
    dbg_hexa(cardIrqEnableOffset);
    dbg_printf("\n\n");
	return true;
}

static bool mpuInitCachePatched = false;

static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	if (patchMpuRegion == 2
	|| (ndsHeader->unitCode > 0 && dsiModeConfirmed)) return;

	if (patchOffsetCache.patchMpuRegion != patchMpuRegion) {
		patchOffsetCache.patchMpuRegion = 0;
		patchOffsetCache.mpuStartOffset = 0;
		patchOffsetCache.mpuDataOffset = 0;
		patchOffsetCache.mpuInitOffset = 0;
		patchOffsetCacheChanged = true;
	}

	// Find the mpu init
	u32* mpuStartOffset = patchOffsetCache.mpuStartOffset;
	u32* mpuDataOffset = patchOffsetCache.mpuDataOffset;
	if (!patchOffsetCache.mpuStartOffset) {
		mpuStartOffset = findMpuStartOffset(ndsHeader, patchMpuRegion);
	}
	if (!patchOffsetCache.mpuDataOffset) {
		mpuDataOffset = findMpuDataOffset(moduleParams, patchMpuRegion, mpuStartOffset);
	}
	dbg_printf("patchMpuSize: ");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n\n");
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

		*mpuDataOffset = mpuInitRegionNewData;

		if (mpuAccessOffset) {
			if (mpuNewInstrAccess) {
				mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
			}
			if (mpuNewDataAccess) {
				mpuDataOffset[mpuAccessOffset + 1] = mpuNewDataAccess;
			}
		}
	}

	// Find the mpu cache init
	u32* mpuInitCacheOffset = patchOffsetCache.mpuInitCacheOffset;
	if (!patchOffsetCache.mpuInitCacheOffset) {
		mpuInitCacheOffset = findMpuInitCacheOffset(mpuStartOffset);
	}
	if (mpuInitCacheOffset) {
		*mpuInitCacheOffset = 0xE3A00046;
		mpuInitCachePatched = true;
		patchOffsetCache.mpuInitCacheOffset = mpuInitCacheOffset;
	}

	// Patch out all further mpu reconfiguration
	dbg_printf("patchMpuSize: ");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n\n");
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	u32* mpuInitOffset = patchOffsetCache.mpuInitOffset;
	if (!mpuInitOffset) {
		mpuInitOffset = (u32*)mpuStartOffset;
	}
	extern u32 iUncompressedSize;
	u32 patchSize = iUncompressedSize;
	if (patchMpuSize > 1) {
		patchSize = patchMpuSize;
	}
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

	patchOffsetCache.patchMpuRegion = patchMpuRegion;
	patchOffsetCache.mpuStartOffset = mpuStartOffset;
	patchOffsetCache.mpuDataOffset = mpuDataOffset;
	patchOffsetCache.mpuInitOffset = mpuInitOffset;
}

static void patchMpu2(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x2008000 || (moduleParams->sdk_version > 0x5000000 && ndsHeader->unitCode == 0)) {
		return;
	}

	// Find the mpu init
	u32* mpuStartOffset = patchOffsetCache.mpuStartOffset2;
	u32* mpuDataOffset = patchOffsetCache.mpuDataOffset2;
	if (!patchOffsetCache.mpuStartOffset2) {
		mpuStartOffset = findMpuStartOffset(ndsHeader, 2);
	}
	if (!patchOffsetCache.mpuDataOffset2) {
		mpuDataOffset = findMpuDataOffset(moduleParams, 2, mpuStartOffset);
	}
	if (mpuDataOffset) {
		// Change the region 2 configuration

		//force DSi mode settings. THESE TOOK AGES TO FIND. -s2k
		if(ndsHeader->unitCode > 0 && dsiModeConfirmed){
			u32 mpuNewDataAccess     = 0x15111111;
			u32 mpuNewInstrAccess    = 0x5111111;
			u32 mpuInitRegionNewData = PAGE_32M | 0x0C000000 | 1;
			int mpuAccessOffset      = 3;

			*mpuDataOffset = mpuInitRegionNewData;

		//if (mpuAccessOffset) {
			//if (mpuNewInstrAccess) {
				mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
			//}
			//if (mpuNewDataAccess) {
				mpuDataOffset[mpuAccessOffset + 1] = mpuNewDataAccess;
			//}
		//}
		} else {
			//Original code made loading slow, so new code is used
			*mpuDataOffset = 0x27FF017; // SDK 5 value
		}

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
	}

	// Find the mpu cache init
	u32* mpuInitCacheOffset = patchOffsetCache.mpuInitCacheOffset;
	if (!mpuInitCachePatched) {
		mpuInitCacheOffset = findMpuInitCacheOffset(mpuStartOffset);
	}
	if (mpuInitCacheOffset) {
		*mpuInitCacheOffset = 0xE3A00046;
		mpuInitCachePatched = true;
		patchOffsetCache.mpuInitCacheOffset = mpuInitCacheOffset;
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
		dbg_printf("Mpu2 init: ");
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

/*static void patchSlot2Exist(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool *usesThumb) {
	if (isSdk5(moduleParams)) {
		return;
	}

	// Slot-2 exist
	//u32* slot2ExistEndOffset = patchOffsetCache.slot2ExistEndOffset;
	u32* slot2ExistEndOffset = NULL;
	//if (!patchOffsetCache.slot2ExistEndOffset) {
		//slot2ExistEndOffset = NULL;
		slot2ExistEndOffset = findSlot2ExistEndOffset(ndsHeader, &usesThumb);
		if (slot2ExistEndOffset) {
			//patchOffsetCache.slot2ExistOffset = slot2ExistEndOffset;
		}
	//}
	if (!slot2ExistEndOffset) {
		return;
	}

	// Patch
	if (usesThumb) {
		*(slot2ExistEndOffset + 3) = 0x0D000000;
		*(slot2ExistEndOffset + 4) = 0x0D0000CE;
	} else {
		*(slot2ExistEndOffset + 3) = 0x0D0000CE;

		int instancesPatched = 0;
		for (int i = 0x100/4; i > 0; i--) {
			if (*(slot2ExistEndOffset - i) == 0xE3A00302) {
				*(slot2ExistEndOffset - i) = *ce9->patches->slot2_exists_fix;
				instancesPatched++;
			}
			if (instancesPatched == 2) break;
		}
	}
}*/

/*static void patchSlot2Read(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool *usesThumb) {
	// Slot-2 read
	//u32* slot2ReadOffset = patchOffsetCache.slot2ReadOffset;
	u32* slot2ReadOffset = NULL;
	//if (!patchOffsetCache.slot2ReadOffset) {
		//slot2ReadStartOffset = NULL;
		slot2ReadOffset = findSlot2ReadOffset(ndsHeader, &usesThumb);
		if (slot2ReadOffset) {
			//patchOffsetCache.slot2ReadOffset = slot2ReadOffset;
		}
	//}
	if (!slot2ReadOffset) {
		return;
	}

	// Patch
	u32* slot2ReadPatch = (usesThumb ? ce9->thumbPatches->slot2_read : ce9->patches->slot2_read);
	memcpy(slot2ReadOffset, slot2ReadPatch, 0x40);
}*/

/*u32* patchLoHeapPointer(const module_params_t* moduleParams, const tNDSHeader* ndsHeader, bool ROMinRAM) {
	u32* heapPointer = NULL;
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 0
	 || patchOffsetCache.heapPointerOffset == 0x023E0000) {
		patchOffsetCache.heapPointerOffset = 0;
	} else {
		heapPointer = patchOffsetCache.heapPointerOffset;
	}
	if (!patchOffsetCache.heapPointerOffset) {
		heapPointer = findHeapPointerOffset(moduleParams, ndsHeader);
	}
    if (!heapPointer || *heapPointer<0x02000000 || *heapPointer>0x03000000) {
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

	*heapPointer += 0x2000; // shrink heap by 8KB

    dbg_printf("new lo heap pointer: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Lo Heap Shrink Sucessfull\n\n");

    return oldheapPointer;
}*/

u32* patchHiHeapPointer(const module_params_t* moduleParams, const tNDSHeader* ndsHeader, bool ROMinRAM) {
	if (moduleParams->sdk_version < 0x2008000) {
		return;
	}

	bool ROMsupportsDsiMode = (ndsHeader->unitCode>0 && dsiModeConfirmed);

	u32* heapPointer = NULL;
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 0
	 || (*patchOffsetCache.heapPointerOffset != (ROMsupportsDsiMode ? 0x13A007BE : 0x023E0000)
	  && *patchOffsetCache.heapPointerOffset != (ROMsupportsDsiMode ? 0xE3A007BE : 0x023E0000)
	  && *patchOffsetCache.heapPointerOffset != (ROMsupportsDsiMode ? 0x048020BE : 0x023E0000))) {
		patchOffsetCache.heapPointerOffset = 0;
	} else {
		heapPointer = patchOffsetCache.heapPointerOffset;
	}
	if (!patchOffsetCache.heapPointerOffset) {
		heapPointer = findHeapPointer2Offset(moduleParams, ndsHeader);
	}
    if(heapPointer) {
		patchOffsetCache.heapPointerOffset = heapPointer;
		patchOffsetCacheChanged = true;
	} else {
		dbg_printf("Heap pointer not found\n");
		return NULL;
	}

    u32* oldheapPointer = (u32*)*heapPointer;

    dbg_printf("old hi heap end pointer: ");
	dbg_hexa((u32)oldheapPointer);
    dbg_printf("\n\n");

	if (ROMsupportsDsiMode) {
		switch (*heapPointer) {
			case 0x13A007BE:
				*heapPointer = (u32)0x13A007BB; /* MOVNE R0, #0x2EC0000 */
				break;
			case 0xE3A007BE:
				*heapPointer = (u32)0xE3A007BB; /* MOV R0, #0x2EC0000 */
				break;
			case 0x048020BE:
				*heapPointer = (u32)0x048020BB; /* MOVS R0, #0x2EC0000 */
				break;
		}
	} else {
		*heapPointer = (u32)CARDENGINEI_ARM9_CACHED_LOCATION_ROMINRAM;
	}

    dbg_printf("new hi heap pointer: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Hi Heap Shrink Sucessfull\n\n");

    return *heapPointer;
}

void relocate_ce9(u32 default_location, u32 current_location, u32 size) {
    dbg_printf("relocate_ce9\n");

    u32 location_sig[1] = {default_location};

    u32* firstCardLocation = findOffset(current_location, size, location_sig, 1);
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

    u32* thumbReadCardLocation = findOffset(current_location, size, location_sig, 1);
	if (!thumbReadCardLocation) {
		return;
	}
    dbg_printf("thumbReadCardLocation ");
	dbg_hexa((u32)thumbReadCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadCardLocation);
    dbg_printf("\n\n");

    *thumbReadCardLocation = current_location;

    u32* armReadDmaCardLocation = findOffset(current_location, size, location_sig, 1);
	if (!armReadDmaCardLocation) {
		return;
	}
    dbg_printf("armReadCardDmaLocation ");
	dbg_hexa((u32)armReadDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadDmaCardLocation);
    dbg_printf("\n\n");

    *armReadDmaCardLocation = current_location;

    u32* armReadSlot2Location = findOffset(current_location, size, location_sig, 1);
	if (!armReadSlot2Location) {
		return;
	}
    dbg_printf("armReadSlot2Location ");
	dbg_hexa((u32)armReadSlot2Location);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadSlot2Location);
    dbg_printf("\n\n");

    *armReadSlot2Location = current_location;

    u32* armSetDmaCardLocation = findOffset(current_location, size, location_sig, 1);
	if (!armSetDmaCardLocation) {
		return;
	}
    dbg_printf("armSetDmaCardLocation ");
	dbg_hexa((u32)armSetDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armSetDmaCardLocation);
    dbg_printf("\n\n");

    *armSetDmaCardLocation = current_location;

    u32* thumbReadDmaCardLocation = findOffset(current_location, size, location_sig, 1);
	if (!thumbReadDmaCardLocation) {
		return;
	}
    dbg_printf("thumbReadCardDmaLocation ");
	dbg_hexa((u32)thumbReadDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadDmaCardLocation);
    dbg_printf("\n\n");

    *thumbReadDmaCardLocation = current_location;

    u32* thumbSetDmaCardLocation = findOffset(current_location, size, location_sig, 1);
	if (!thumbSetDmaCardLocation) {
		return;
	}
    dbg_printf("thumbSetDmaCardLocation ");
	dbg_hexa((u32)thumbSetDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbSetDmaCardLocation);
    dbg_printf("\n\n");

    *thumbSetDmaCardLocation = current_location;

    u32* armReadNandLocation = findOffset(current_location, size, location_sig, 1);
	if (!armReadNandLocation) {
		return;
	}
    dbg_printf("armReadNandLocation ");
	dbg_hexa((u32)armReadNandLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadNandLocation);
    dbg_printf("\n\n");

    *armReadNandLocation = current_location;

    u32* thumbReadNandLocation = findOffset(current_location, size, location_sig, 1);
	if (!thumbReadNandLocation) {
		return;
	}
    dbg_printf("thumbReadNandLocation ");
	dbg_hexa((u32)thumbReadNandLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadNandLocation);
    dbg_printf("\n\n");

    *thumbReadNandLocation = current_location;

    u32* armWriteNandLocation = findOffset(current_location, size, location_sig, 1);
	if (!armWriteNandLocation) {
		return;
	}
    dbg_printf("armWriteNandLocation ");
	dbg_hexa((u32)armWriteNandLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armWriteNandLocation);
    dbg_printf("\n\n");

    *armWriteNandLocation = current_location;

    u32* thumbWriteNandLocation = findOffset(current_location, size, location_sig, 1);
	if (!thumbWriteNandLocation) {
		return;
	}
    dbg_printf("thumbWriteNandLocation ");
	dbg_hexa((u32)thumbWriteNandLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbWriteNandLocation);
    dbg_printf("\n\n");

    *thumbWriteNandLocation = current_location;

    u32* armIrqEnableLocation = findOffset(current_location, size, location_sig, 1);
	if (!armIrqEnableLocation) {
		return;
	}
    dbg_printf("armIrqEnableLocation ");
	dbg_hexa((u32)armIrqEnableLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armIrqEnableLocation);
    dbg_printf("\n\n");

    *armIrqEnableLocation = current_location;

    u32* thumbIrqEnableLocation = findOffset(current_location, size, location_sig, 1);
	if (!thumbIrqEnableLocation) {
		return;
	}
    dbg_printf("thumbIrqEnableLocation ");
	dbg_hexa((u32)thumbIrqEnableLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbIrqEnableLocation);
    dbg_printf("\n\n");

    *thumbIrqEnableLocation = current_location;

    u32* pdashReadLocation = findOffset(current_location, size, location_sig, 1);
	if (!pdashReadLocation) {
		return;
	}
    dbg_printf("pdashReadLocation ");
	dbg_hexa((u32)pdashReadLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*pdashReadLocation);
    dbg_printf("\n\n");

    *pdashReadLocation = current_location;

    u32* thumbReadSlot2Location = findOffset(current_location, size, location_sig, 1);
	if (!thumbReadSlot2Location) {
		return;
	}
    dbg_printf("thumbReadSlot2Location ");
	dbg_hexa((u32)thumbReadSlot2Location);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadSlot2Location);
    dbg_printf("\n\n");

    *thumbReadSlot2Location = current_location;

	u32* ipcSyncHandlerLocation = findOffset(current_location, size, location_sig, 1);
	if (!ipcSyncHandlerLocation) {
		return;
	}
    dbg_printf("ipcSyncHandlerLocation ");
	dbg_hexa((u32)ipcSyncHandlerLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*ipcSyncHandlerLocation);
    dbg_printf("\n\n");

    *ipcSyncHandlerLocation = current_location;

	u32* resetLocation = findOffset(current_location, size, location_sig, 1);
	if (!resetLocation) {
		return;
	}
    dbg_printf("resetLocation ");
	dbg_hexa((u32)resetLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*resetLocation);
    dbg_printf("\n\n");

    *resetLocation = current_location;

    u32* globalCardLocation = findOffset(current_location, size, location_sig, 1);
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
    ce9->patches->card_irq_enable = (u32*)((u32)ce9->patches->card_irq_enable - default_location + current_location);
    ce9->patches->card_pull_out_arm9 = (u32*)((u32)ce9->patches->card_pull_out_arm9 - default_location + current_location);
    ce9->patches->card_id_arm9 = (u32*)((u32)ce9->patches->card_id_arm9 - default_location + current_location);
    ce9->patches->card_dma_arm9 = (u32*)((u32)ce9->patches->card_dma_arm9 - default_location + current_location);
    ce9->patches->card_set_dma_arm9 = (u32*)((u32)ce9->patches->card_set_dma_arm9 - default_location + current_location);
    ce9->patches->nand_read_arm9 = (u32*)((u32)ce9->patches->nand_read_arm9 - default_location + current_location);
    ce9->patches->nand_write_arm9 = (u32*)((u32)ce9->patches->nand_write_arm9 - default_location + current_location);
    ce9->patches->cardStructArm9 = (u32*)((u32)ce9->patches->cardStructArm9 - default_location + current_location);
    ce9->patches->card_pull = (u32*)((u32)ce9->patches->card_pull - default_location + current_location);
    ce9->patches->slot2_exists_fix = (u32*)((u32)ce9->patches->slot2_exists_fix - default_location + current_location);
    ce9->patches->slot2_read = (u32*)((u32)ce9->patches->slot2_read - default_location + current_location);
    ce9->patches->cacheFlushRef = (u32*)((u32)ce9->patches->cacheFlushRef - default_location + current_location);
    ce9->patches->sleepRef = (u32*)((u32)ce9->patches->sleepRef - default_location + current_location);
    ce9->patches->swi02 = (u32*)((u32)ce9->patches->swi02 - default_location + current_location);
    ce9->patches->reset_arm9 = (u32*)((u32)ce9->patches->reset_arm9 - default_location + current_location);
    ce9->patches->pdash_read = (u32*)((u32)ce9->patches->pdash_read - default_location + current_location);
    ce9->patches->vblankHandlerRef = (u32*)((u32)ce9->patches->vblankHandlerRef - default_location + current_location);
    ce9->patches->ipcSyncHandlerRef = (u32*)((u32)ce9->patches->ipcSyncHandlerRef - default_location + current_location);
    ce9->thumbPatches->card_read_arm9 = (u32*)((u32)ce9->thumbPatches->card_read_arm9 - default_location + current_location);
    ce9->thumbPatches->card_irq_enable = (u32*)((u32)ce9->thumbPatches->card_irq_enable - default_location + current_location);
    ce9->thumbPatches->card_pull_out_arm9 = (u32*)((u32)ce9->thumbPatches->card_pull_out_arm9 - default_location + current_location);
    ce9->thumbPatches->card_id_arm9 = (u32*)((u32)ce9->thumbPatches->card_id_arm9 - default_location + current_location);
    ce9->thumbPatches->card_dma_arm9 = (u32*)((u32)ce9->thumbPatches->card_dma_arm9 - default_location + current_location);
    ce9->thumbPatches->card_set_dma_arm9 = (u32*)((u32)ce9->thumbPatches->card_set_dma_arm9 - default_location + current_location);
    ce9->thumbPatches->nand_read_arm9 = (u32*)((u32)ce9->thumbPatches->nand_read_arm9 - default_location + current_location);
    ce9->thumbPatches->nand_write_arm9 = (u32*)((u32)ce9->thumbPatches->nand_write_arm9 - default_location + current_location);
    ce9->thumbPatches->cardStructArm9 = (u32*)((u32)ce9->thumbPatches->cardStructArm9 - default_location + current_location);
    ce9->thumbPatches->card_pull = (u32*)((u32)ce9->thumbPatches->card_pull - default_location + current_location);
    ce9->thumbPatches->slot2_read = (u32*)((u32)ce9->patches->slot2_read - default_location + current_location);
    ce9->thumbPatches->cacheFlushRef = (u32*)((u32)ce9->thumbPatches->cacheFlushRef - default_location + current_location);
    ce9->thumbPatches->sleepRef = (u32*)((u32)ce9->thumbPatches->sleepRef - default_location + current_location);
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
		u32* randomPatchOffset = patchOffsetCache.randomPatchOffset;
		if (!patchOffsetCache.randomPatchChecked) {
			randomPatchOffset = findRandomPatchOffset(ndsHeader);
			if (randomPatchOffset) {
				patchOffsetCache.randomPatchOffset = randomPatchOffset;
			}
			patchOffsetCache.randomPatchChecked = true;
			patchOffsetCacheChanged = true;
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
		patchOffsetCacheChanged = true;
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
      memcpy((u8*)sdPatchEntry+0x958, nandWritePatch, 0x40);

      //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
      u32* nandReadPatch = ce9->patches->nand_read_arm9;
      memcpy((u8*)sdPatchEntry+0xD24, nandReadPatch, 0x40);
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
            memcpy((u32*)0x0206176C, nandWritePatch, 0x40);

            //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandReadPatch = ce9->patches->nand_read_arm9;
            memcpy((u32*)0x02061AC4, nandReadPatch, 0x40);
    	} else
        // Face Training (Europe)
        if (strcmp(romTid, "USKV") == 0) {
          	//u32 gNandInit(void* data)
            *(u32*)(0x020E2AEC) = 0xe3a00001; //mov r0, #1
            *(u32*)(0x020E2AF0) = 0xe12fff1e; //bx lr

            //u32 gNandResume(void)
            *(u32*)(0x020E2EC0) = 0xe3a00000; //mov r0, #0
            *(u32*)(0x020E2EC4) = 0xe12fff1e; //bx lr

            //u32 gNandError(void)
            *(u32*)(0x020E3150) = 0xe3a00000; //mov r0, #0
            *(u32*)(0x020E3154) = 0xe12fff1e; //bx lr

            //u32 gNandWrite(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandWritePatch = ce9->patches->nand_write_arm9;
            memcpy((u32*)0x020E2BF0, nandWritePatch, 0x40);

            //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandReadPatch = ce9->patches->nand_read_arm9;
            memcpy((u32*)0x020E2F3C, nandReadPatch, 0x40);
    	}
	}
}

static void patchCardReadPdash(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
    u32 sdPatchEntry = 0;
    
	const char* romTid = getRomTid(ndsHeader);
    
    // Pokemon Dash USA
	if (strcmp(romTid, "APDE") == 0) {
		sdPatchEntry = 0x206CFE8;
        // TODO : try to target 206CFE8, more similar to cardread
        // r0 cardstruct 218A6E0 ptr 20D6120
        // r1 src
        // r2 dst
        // r3 len
        // return r0=number of time executed ?? 206D1A8
	}
    
    if(sdPatchEntry) {   
     	// Patch
    	u32* pDashReadPatch = ce9->patches->pdash_read;
    	memcpy(sdPatchEntry, pDashReadPatch, 0x40);   
    }
}

static void operaRamPatch(const tNDSHeader* ndsHeader) {
	extern int consoleModel;

	// Opera RAM patch (ARM9)
	*(u32*)0x02003D48 = 0x2400000;
	*(u32*)0x02003D4C = 0x2400004;

	*(u32*)0x02010FF0 = 0x2400000;
	*(u32*)0x02010FF4 = 0x24000CE;

	*(u32*)0x020112AC = 0x2400080;

	*(u32*)0x020402BC = 0x24000C2;
	*(u32*)0x020402C0 = 0x24000C0;
	if (consoleModel > 0) {
		*(u32*)0x020402CC = 0xD7FFFFE;
		*(u32*)0x020402D0 = 0xD000000;
		*(u32*)0x020402D4 = 0xD1FFFFF;
		*(u32*)0x020402D8 = 0xD3FFFFF;
		*(u32*)0x020402DC = 0xD7FFFFF;
		*(u32*)0x020402E0 = 0xDFFFFFF;	// ???
		toncset((char*)0xD000000, 0xFF, 0x800000);		// Fill fake MEP with FFs
	} else {
		*(u32*)0x020402CC = 0x2FFFFFE;
		*(u32*)0x020402D0 = 0x2800000;
		*(u32*)0x020402D4 = 0x29FFFFF;
		*(u32*)0x020402D8 = 0x2BFFFFF;
		*(u32*)0x020402DC = 0x2FFFFFF;
		*(u32*)0x020402E0 = 0xD7FFFFF;	// ???
		toncset((char*)0x2800000, 0xFF, 0x800000);		// Fill fake MEP with FFs
	}

	// Opera RAM patch (ARM7)
	*(u32*)0x0238C7BC = 0x2400000;
	*(u32*)0x0238C7C0 = 0x24000CE;

	//*(u32*)0x0238C950 = 0x2400000;
}

static void setFlushCache(cardengineArm9* ce9, u32 patchMpuRegion, bool usesThumb) {
	//if (!usesThumb) {
	ce9->patches->needFlushDCCache = (patchMpuRegion == 1);
}

u32 patchCardNdsArm9(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 ROMinRAM, u32 patchMpuRegion, u32 patchMpuSize) {

	bool usesThumb;
	bool slot2usesThumb = false;
	int readType;
	int sdk5ReadType; // SDK 5
	u32* cardReadEndOffset;
	u32* cardPullOutOffset;

	a9PatchCardIrqEnable(ce9, ndsHeader, moduleParams);

    const char* romTid = getRomTid(ndsHeader);

    u32 startOffset = ndsHeader->arm9destination;
    if (strncmp(romTid, "UOR", 3) == 0) { // Start at 0x2003800 for "WarioWare: DIY"
        startOffset = ndsHeader->arm9destination + 0x3800;
    } else if (strncmp(romTid, "UXB", 3) == 0) { // Start at 0x2080000 for "Jam with the Band"
        startOffset = ndsHeader->arm9destination + 0x80000;        
    } else if (strncmp(romTid, "USK", 3) == 0) { // Start at 0x20E8000 for "Face Training"
        startOffset = ndsHeader->arm9destination + 0xE4000;        
    }

    dbg_printf("startOffset : ");
    dbg_hexa(startOffset);
    dbg_printf("\n\n");

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &sdk5ReadType, &cardReadEndOffset, true, startOffset)) {
		dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

	fixForDifferentBios(ce9, ndsHeader, usesThumb);

	if (ndsHeader->unitCode > 0 && dsiModeConfirmed) {
		patchCardRomInit(cardReadEndOffset, usesThumb);

		if (!patchCardHashInit(ndsHeader, moduleParams, usesThumb)) {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
	}

   /*if (strncmp(romTid, "V2G", 3) == 0) {
        // try to patch card read DMA a second time
        dbg_printf("patch card read a second time\n");
        dbg_printf("startOffset : 0x02030000\n\n");
	   	if (!patchCardReadMvDK4(ce9, 0x02030000)) {
    		dbg_printf("ERR_LOAD_OTHR\n\n");
    		return ERR_LOAD_OTHR;
    	}
	}*/

    // made obsolete by tonccpy
	//patchCardReadCached(ce9, ndsHeader, moduleParams, usesThumb);

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, sdk5ReadType, &cardPullOutOffset);

	//patchCardTerminateForPullOut(ce9, usesThumb, ndsHeader, moduleParams, cardPullOutOffset);

	patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	//patchForceToPowerOff(ce9, ndsHeader, usesThumb);

	patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset);

	if (getSleep(ce9, ndsHeader, moduleParams, usesThumb)) {
		patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
	} else {
		if (!patchCardSetDma(ce9, ndsHeader, moduleParams, usesThumb, ROMinRAM)) {
			patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
		}
		if (!patchCardEndReadDma(ce9, ndsHeader, moduleParams, usesThumb, ROMinRAM)) {
			randomPatch(ndsHeader, moduleParams);
			randomPatch5Second(ndsHeader, moduleParams);
		}
	}

	patchMpu(ndsHeader, moduleParams, patchMpuRegion, patchMpuSize);
	patchMpu2(ndsHeader, moduleParams);

	//patchDownloadplay(ndsHeader);

    //patchSleep(ce9, ndsHeader, moduleParams, usesThumb);

	patchReset(ce9, ndsHeader, moduleParams);

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch(ndsHeader);
	} /*else if (gbaRomFound) {
		//patchSlot2Exist(ce9, ndsHeader, moduleParams, &slot2usesThumb);

		//patchSlot2Read(ce9, ndsHeader, moduleParams, &slot2usesThumb);
	}*/

	nandSavePatch(ce9, ndsHeader, moduleParams);

	patchCardReadPdash(ce9, ndsHeader);
    
	setFlushCache(ce9, patchMpuRegion, usesThumb);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
