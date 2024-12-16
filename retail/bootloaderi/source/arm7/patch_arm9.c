#include <string.h>
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
#include "value_bits.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"
#include "debug_file.h"
#include "dmaTwl.h"
#include "tonccpy.h"

#include "igm_text.h"
#include "locations.h"

extern u8 gameOnFlashcard;
extern u8 saveOnFlashcard;
extern u16 a9ScfgRom;

extern bool gbaRomFound;
extern bool dsiModeConfirmed;

bool isPawsAndClaws(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AQ2", 3) == 0 // Paws & Claws: Pet Resort
	 || strncmp(romTid, "YMU", 3) == 0 // Paws & Claws: Pet Vet 2
	) {
		return true;
	}

	return false;
}

static void fixForDifferentBios(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, bool usesThumb, bool usesCloneboot) {
	if (!usesCloneboot) {
		u32* swi12Offset = patchOffsetCache.a9Swi12Offset;
		if (!patchOffsetCache.a9Swi12Offset) {
			swi12Offset = a9_findSwi12Offset(ndsHeader);
			if (swi12Offset) {
				patchOffsetCache.a9Swi12Offset = swi12Offset;
			}
		}

		// swi 0x12 call
		if (swi12Offset && (u8)a9ScfgRom == 1 && (!(REG_SCFG_ROM & BIT(1)) || dsiModeConfirmed)) {
			// Patch to call swi 0x02 instead of 0x12
			u32* swi12Patch = ce9->patches->swi02;
			tonccpy(swi12Offset, swi12Patch, 0x4);
		}

		dbg_printf("swi12 location : ");
		dbg_hexa((u32)swi12Offset);
		dbg_printf("\n\n");
	}

	bool ROMisDsiEnhanced = (ndsHeader->unitCode > 0);

	u32* dsiModeCheckOffset = patchOffsetCache.dsiModeCheckOffset;
	u32* dsiModeCheck2Offset = patchOffsetCache.dsiModeCheck2Offset;
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

	if (dsiModeCheckOffset && ROMisDsiEnhanced) {
		// Patch to return as DS BIOS
		if (dsiModeConfirmed && (u8)a9ScfgRom != 1) {
			dsiModeCheckOffset[7] = 0x2FFFD00;
			dbg_printf("Running DSi mode with DS BIOS set in REG_SCFG_ROM\n");

			if (dsiModeCheck2Offset) {
				if (dsiModeCheck2Offset[0] == 0xE59F0014) {
					dsiModeCheck2Offset[7] = 0x2FFFD00;
				} else {
					dsiModeCheck2Offset[usesThumb ? 22/sizeof(u16) : 18] = 0x2FFFD00;
				}
			}
		} /*else if (!dsiModeConfirmed && extendedMemoryConfirmed && !(REG_SCFG_ROM & BIT(1))) {
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
		}*/
	}

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
		}
	}

	if (cardHashInitOffset) {
		if ((u32)cardHashInitOffset < *(u32*)0x02FFE1C8) {
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

/* static void patchCardRomInit(u32* cardReadEndOffset, bool usesThumb) {
	u32* cardRomInitOffset = patchOffsetCache.cardRomInitOffset;
	if (!patchOffsetCache.cardRomInitOffset) {
		cardRomInitOffset = usesThumb ? (u32*)findCardRomInitOffsetThumb((u16*)cardReadEndOffset) : findCardRomInitOffset(cardReadEndOffset);
		if (cardRomInitOffset) {
			patchOffsetCache.cardRomInitOffset = cardRomInitOffset;
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
} */

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
			cardReadEndOffset = findCardReadEndOffsetType1(ndsHeader, moduleParams, startOffset);
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

	/*if (ndsHeader->unitCode > 0 && dsiModeConfirmed) {
		cardReadStartOffset = patchOffsetCache.cardReadHashOffset;
		if (!patchOffsetCache.cardReadHashOffset) {
			cardReadStartOffset = findCardReadHashOffset();
			if (cardReadStartOffset) {
				patchOffsetCache.cardReadHashOffset = cardReadStartOffset;
			} else {
				return false;
			}
		}

		if (cardReadStartOffset) {
			cardReadPatch = ce9->patches->card_read_arm9;
			tonccpy(cardReadStartOffset, cardReadPatch, 0x2C);
		}

		dbg_printf("cardReadHash location : ");
		dbg_hexa(cardReadStartOffset);
		dbg_printf("\n\n");
	}*/
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
	u32* cardPullOutPatch = (usesThumb ? ce9->thumbPatches->card_pull_out_arm9 : ce9->patches->card_pull_out_arm9);
	tonccpy(cardPullOutOffset, cardPullOutPatch, usesThumb ? 0x2 : 0x30);
    dbg_printf("cardPullOut location : ");
    dbg_hexa((u32)cardPullOutOffset);
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
	tonccpy(forceToPowerOffOffset, cardPullOutPatch, 0x4);
}*/

static bool patchCardId(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32* cardReadEndOffset) {
	if (!isPawsAndClaws(ndsHeader) && !cardReadEndOffset) {
		return true;
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
	} else if (isSdk5(moduleParams)) {
		return false;
	}

	return true;
}

void patchGbaSlotInit_cont(const tNDSHeader* ndsHeader, bool usesThumb, bool searchAgainForThumb) {
	extern u32 oldArm7mbk;
	if (REG_SCFG_EXT != 0 || ndsHeader->unitCode == 0 || !dsiModeConfirmed || (oldArm7mbk == 0x080037C0 && *(u32*)0x02FFE1A0 == 0x080037C0) || *(u32*)0x02FFE1A0 == 0x00403000) {
		return;
	}

	// Card ID
	u32* gbaSlotInitOffset = patchOffsetCache.gbaSlotInitOffset;
	if (!patchOffsetCache.gbaSlotInitChecked) {
		if (usesThumb) {
			gbaSlotInitOffset = (u32*)findGbaSlotInitOffsetThumb(ndsHeader);
		} else {
			gbaSlotInitOffset = findGbaSlotInitOffset(ndsHeader);
		}
		if (!gbaSlotInitOffset && searchAgainForThumb) {
			gbaSlotInitOffset = (u32*)findGbaSlotInitOffsetThumb(ndsHeader);
			if (gbaSlotInitOffset) {
				usesThumb = true;
				patchOffsetCache.a9IsThumb = usesThumb;
			}
		}
		if (gbaSlotInitOffset) {
			patchOffsetCache.gbaSlotInitOffset = gbaSlotInitOffset;
		}
		patchOffsetCache.gbaSlotInitChecked = true;
	} else if (searchAgainForThumb) {
		usesThumb = patchOffsetCache.a9IsThumb;
	}

	if (gbaSlotInitOffset) {
        // Patch
		if (usesThumb) {
			*(u16*)gbaSlotInitOffset = 0x4770;	// bx lr
		} else {
			*gbaSlotInitOffset = 0xE12FFF1E;	// bx lr
		}

		dbg_printf("gbaSlotInit location : ");
		dbg_hexa((u32)gbaSlotInitOffset);
		dbg_printf("\n\n");
	}
}

static void patchGbaSlotInit(const tNDSHeader* ndsHeader, bool usesThumb) {
	patchGbaSlotInit_cont(ndsHeader, usesThumb, false);
}

/*static void patchCardRefresh(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	// Card refresh
	u32* cardRefreshOffset = patchOffsetCache.cardRefreshOffset;
	if (!patchOffsetCache.cardRefreshOffset) {
		cardRefreshOffset = findCardRefreshOffset(ndsHeader, moduleParams, usesThumb);
		if (cardRefreshOffset) {
			patchOffsetCache.cardRefreshOffset = cardRefreshOffset;
		}
	}

	if (cardRefreshOffset) {
        // Patch
		if (usesThumb) {
			u16* cardRefreshOffsetThumb = (u16*)cardRefreshOffset;
			*cardRefreshOffsetThumb = 0x4770;	// bx lr
		} else {
			*cardRefreshOffset = 0xE12FFF1E;	// bx lr
		}
		dbg_printf("cardRefresh location : ");
		dbg_hexa(cardRefreshOffset);
		dbg_printf("\n\n");
	}
}*/

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

static bool patchCardEndReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version)
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man
	 || (strncmp(romTid, "V2G", 3) == 0 && !dsiModeConfirmed) // Mario vs. Donkey Kong: Mini-Land Mayhem (DS mode)
	 || !cardReadDMA) return false;

    u32* offset = patchOffsetCache.cardEndReadDmaOffset;
    u32 offsetDmaHandler = patchOffsetCache.dmaHandlerOffset;
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
		offset = findCardEndReadDma(ndsHeader,moduleParams,usesThumb,patchOffsetCache.cardReadDmaEndOffset,&offsetDmaHandler);
		if (offset) {
			patchOffsetCache.cardEndReadDmaOffset = offset;
			patchOffsetCache.dmaHandlerOffset = offsetDmaHandler;
		}
		patchOffsetCache.cardEndReadDmaChecked = true;
	  }
    if(offset) {
      dbg_printf("\nNDMA CARD READ METHOD ACTIVE\n");
    dbg_printf("cardEndReadDma location : ");
    dbg_hexa((u32)offset);
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

// bool setDmaPatched = false;
static bool patchCardSetDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing
	 || strncmp(romTid, "CP3", 3) == 0	// Viva Pinata
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version)
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man
	 || (strncmp(romTid, "V2G", 3) == 0 && !dsiModeConfirmed) // Mario vs. Donkey Kong: Mini-Land Mayhem (DS mode)
	 || !cardReadDMA) return false;

	dbg_printf("\npatchCardSetDma\n");           

    u32* setDmaoffset = patchOffsetCache.cardSetDmaOffset;
    if (!patchOffsetCache.cardSetDmaChecked) {
		setDmaoffset = findCardSetDma(ndsHeader,moduleParams,usesThumb);
		if (setDmaoffset) {
			patchOffsetCache.cardSetDmaOffset = setDmaoffset;
		}
		patchOffsetCache.cardSetDmaChecked = true;
    }
    if(setDmaoffset) {
      dbg_printf("\nNDMA CARD SET METHOD ACTIVE\n");       
    dbg_printf("cardSetDma location : ");
    dbg_hexa((u32)setDmaoffset);
    dbg_printf("\n\n");
      u32* cardSetDmaPatch = (usesThumb ? ce9->thumbPatches->card_set_dma_arm9 : ce9->patches->card_set_dma_arm9);
	  tonccpy(setDmaoffset, cardSetDmaPatch, 0x30);
	  // setDmaPatched = true;

      return true;  
    }

    return false; 
}

static void patchReset(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {    
	const char* romTid = getRomTid(ndsHeader);
	if (ndsHeader->unitCode == 0 && (strcmp(romTid, "NTRJ") == 0 || strncmp(romTid, "HND", 3) == 0 || strncmp(romTid, "HNE", 3) == 0)) {
		u32* offset = patchOffsetCache.srlStartOffset9;

		if (!patchOffsetCache.srlStartOffsetChecked) {
			offset = findSrlStartOffset9(ndsHeader);
			if (offset) patchOffsetCache.srlStartOffset9 = offset;
			patchOffsetCache.srlStartOffsetChecked = true;
		}

		if (offset) {
			// Patch
			tonccpy(offset, ce9->patches->reset_arm9, 0x40);
			dbg_printf("srlStart location : ");
			dbg_hexa((u32)offset);
			dbg_printf("\n\n");
		}
	}

	u32* offset = patchOffsetCache.resetOffset;

    if (!patchOffsetCache.resetChecked) {
		offset = findResetOffset(ndsHeader, moduleParams, (bool)patchOffsetCache.srlStartOffset9);
		if (offset) patchOffsetCache.resetOffset = offset;
		patchOffsetCache.resetChecked = true;
	}

	if (!offset) {
		return;
	}

	// Patch
	tonccpy(offset, ce9->patches->reset_arm9, 0x40);
	dbg_printf("reset location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

void patchResetTwl(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {    
	if (ndsHeader->unitCode == 0 || !dsiModeConfirmed || (patchOffsetCache.resetOffset && !isDSiWare)) {
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
		*nandTmpJumpFuncOffset = generateA7Instr((int)nandTmpJumpFuncOffset, (int)ce9->patches->reset_arm9);
	} else if (nandTmpJumpFuncOffset[-3] == 0xE8BD8008 && nandTmpJumpFuncOffset[-1] == 0x02FFE230) { // DEBUG
		nandTmpJumpFuncOffset[-4] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (4 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		dbg_printf("Reset (TWL) patched!\n");
	} else if (nandTmpJumpFuncOffset[-2] == 0xE8BD8008 && nandTmpJumpFuncOffset[-1] == 0x02FFE230) {
		nandTmpJumpFuncOffset[-3] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (3 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		dbg_printf("Reset (TWL) patched!\n");
		if (nandTmpJumpFuncOffset[-13] == 0xE12FFF1C) {
			nandTmpJumpFuncOffset[-12] = (u32)ce9->patches->reset_arm9;
			dbg_printf("Exit-to-menu patched!\n");
		}
	} else if (nandTmpJumpFuncOffset[-2] == 0xE12FFF1C) {
		nandTmpJumpFuncOffset[-1] = (u32)ce9->patches->reset_arm9;
		dbg_printf("Exit-to-menu patched!\n");
	} else if (nandTmpJumpFuncOffset[-15] == 0xE8BD8008 && nandTmpJumpFuncOffset[-14] == 0x02FFE230) {
		nandTmpJumpFuncOffset[-16] = generateA7Instr((int)(((u32)nandTmpJumpFuncOffset) - (15 * sizeof(u32))), (int)ce9->patches->reset_arm9);
		dbg_printf("Reset (TWL) patched!\n");
		if (nandTmpJumpFuncOffset[-26] == 0xE12FFF1C) {
			nandTmpJumpFuncOffset[-25] = (u32)ce9->patches->reset_arm9;
			dbg_printf("Exit-to-menu patched!\n");
		}
	} else if (nandTmpJumpFuncOffset[-15] == 0xE12FFF1C) {
		nandTmpJumpFuncOffset[-14] = (u32)ce9->patches->reset_arm9;
		dbg_printf("Exit-to-menu patched!\n");
	}
	if (nandTmpJumpFuncOffset[-2] >= 0x02000000 && nandTmpJumpFuncOffset[-2] < 0x02400000 && nandTmpJumpFuncOffset[-1] == 0x02FFE230) {
		ce9->intr_vblank_orig_return = nandTmpJumpFuncOffset[-2];
		nandTmpJumpFuncOffset[-1] = 0x02FFC230;
	}
	dbg_printf("nandTmpJumpFunc location : ");
	dbg_hexa((u32)nandTmpJumpFuncOffset);
	dbg_printf("\n\n");
}

static bool getSleep(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32 ROMinRAM) {
	/*bool ROMsupportsDsiMode = (ndsHeader->unitCode > 0 && dsiModeConfirmed);
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AH9", 3) == 0 // Tony Hawk's American Sk8land
	) {
		return false;
	} else {*/
		if (gameOnFlashcard && !ROMinRAM) {
			return false;
		} else if (ROMinRAM) {
			/*if (!cardReadDMA || extendedMemoryConfirmed || ROMsupportsDsiMode)*/ return false;
		} else {
			if (/* !cardDmaImprove &&*/ !asyncCardRead) return false;
		}
	//}

	// Work-around for minorly unstable card read DMA inplementation
	u32* offset = patchOffsetCache.sleepFuncOffset;
	if (!patchOffsetCache.sleepChecked) {
		offset = findSleepOffset(ndsHeader, moduleParams, usesThumb, &patchOffsetCache.sleepFuncIsThumb);
		if (offset) {
			patchOffsetCache.sleepFuncOffset = offset;
		}
		patchOffsetCache.sleepChecked = true;
	}
	if (!offset) {
		return false;
	}
	if (patchOffsetCache.sleepFuncIsThumb) {
		ce9->thumbPatches->sleepRef = offset;
	} else {
		ce9->patches->sleepRef = offset;
	}
	dbg_printf("sleep location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
	return true;
}

bool a9PatchCardIrqEnable(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	const char* romTid = getRomTid(ndsHeader);

	if (strncmp(romTid, "AWD", 3) == 0 // Diddy Kong Racing - Fix corrupted 3D model bug
	 || strncmp(romTid, "CP3", 3) == 0 // Viva Pinata - Fix touch and model rendering bug
	 || strncmp(romTid, "BO5", 3) == 0 // Golden Sun: Dark Dawn - Fix black screen on boot
	 || strncmp(romTid, "Y8L", 3) == 0 // Golden Sun: Dark Dawn (Demo Version) - Fix black screen on boot
	 || strncmp(romTid, "B8I", 3) == 0 // Spider-Man: Edge of Time - Fix white screen on boot
	 || strncmp(romTid, "TAM", 3) == 0 // The Amazing Spider-Man - Fix white screen on boot
	) {
		extern bool igmAccessible;
		igmAccessible = false;
		return true;
	}

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

static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion) {
	if (patchMpuRegion == 2 || ndsHeader->unitCode > 0) return;

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

	if (!isSdk5(moduleParams) && unpatchedFuncs->mpuInitRegionOldData != 0x200002B) {
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

void patchMpu2(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const bool usesCloneboot) {
	if (moduleParams->sdk_version > 0x5000000 && (ndsHeader->unitCode == 0 || !dsiModeConfirmed)) {
		return;
	}

	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	// Find the mpu init
	u32* mpuStartOffset = patchOffsetCache.mpuStartOffset2;
	u32* mpuDataOffset = patchOffsetCache.mpuDataOffset2;
	if (!patchOffsetCache.mpuStartOffset2) {
		mpuStartOffset = findMpuStartOffset(ndsHeader, 2);
	}
	if (mpuStartOffset) {
		dbg_printf("Mpu start 2: ");
		dbg_hexa((u32)mpuStartOffset);
		dbg_printf("\n\n");
	}
	if (!patchOffsetCache.mpuDataOffset2) {
		mpuDataOffset = findMpuDataOffset(moduleParams, 2, mpuStartOffset);
	}
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
			unpatchedFuncs->mpuDataOffset2 = mpuDataOffset;
			unpatchedFuncs->mpuInitRegionOldData2 = *mpuDataOffset;
			// *mpuDataOffset = (ndsHeader->unitCode > 0 && dsiModeConfirmed) ? 0 : (PAGE_128K | 0x027E0000 | 1);
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

		u32 mpuInitOffsetInSrl = (u32)mpuInitOffset;
		mpuInitOffsetInSrl -= (u32)ndsHeader->arm9destination;

		if (mpuInitOffsetInSrl >= 0 && mpuInitOffsetInSrl < 0x4000 && usesCloneboot) {
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

	patchOffsetCache.mpuStartOffset2 = mpuStartOffset;
	patchOffsetCache.mpuDataOffset2 = mpuDataOffset;
	patchOffsetCache.mpuInitOffset2 = mpuInitOffset;
}

void patchMpuChange(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 oldArm7mbk;
	if (moduleParams->sdk_version < 0x5000000 || oldArm7mbk == 0x080037C0) {
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

void patchMpuInitTwl(const tNDSHeader* ndsHeader) {
	if (ndsHeader->unitCode == 0 || !dsiModeConfirmed) {
		return;
	}

	u32* offset = patchOffsetCache.mpuInitEndTwl;
	if (!patchOffsetCache.mpuInitEndTwl) {
		offset = findMpuInitTwlEnd(patchOffsetCache.heapPointerOffset);
		if (offset) {
			patchOffsetCache.mpuInitEndTwl = offset;
		}
	}

	if (!offset) {
		return;
	}

	if (offset[-2] == 0xE12FFF1C) {
		offset[-3] = 0xE12FFF1E; // bx lr
	} else {
		bool dsiEnhanced = (offset[-1] == 0x027FF000);

		if (offset[0] == 0xE1A00100) {
			offset[dsiEnhanced ? -3 : -2] = 0xE1A00000; // nop
		} else {
			u16* thumbOffset = (u16*)offset;
			thumbOffset[dsiEnhanced ? -5 : -3] = 0x46C0; // nop
			thumbOffset[dsiEnhanced ? -4 : -2] = 0x46C0; // nop
		}
	}

	dbg_printf("Mpu init end TWL: ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

/*static bool patchCartExist(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	// Slot-2 cart exist
	u32* offset = patchOffsetCache.cartExistOffset;
	if (!patchOffsetCache.cartExistOffsetChecked) {
		offset = findCartExistOffset(ndsHeader, usesThumb);
		if (offset) {
			patchOffsetCache.cartExistOffset = offset;
		}
		patchOffsetCache.cartExistOffsetChecked = true;
	}
	if (!offset) {
		return false;
	}

	// Patch
	if (usesThumb) {
		u16* thumbOffset = (u16*)offset;

		thumbOffset[0] = 0x2001;	// movs r0, #1
		thumbOffset[1] = 0x4770;	// bx lr
	} else {
		offset[0] = 0xE3A00001;	// mov r0, #1
		offset[1] = 0xE12FFF1E;	// bx lr
	}
    dbg_printf("cartExist location : ");
    dbg_hexa((u32)offset);
    dbg_printf("\n\n");
	return true;
}

static void patchCartInfoInitConstant(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	// Slot-2 cart info init
	u32* offset = patchOffsetCache.cartInfoInitConstantOffset;
	if (!patchOffsetCache.cartInfoInitConstantOffset) {
		offset = findCartInfoInitConstantOffset(ndsHeader, moduleParams, usesThumb);
		if (offset) {
			patchOffsetCache.cartInfoInitConstantOffset = offset;
		}
	}
	if (!offset) {
		return;
	}

	// Patch
	*offset = 0x027FFA00;
    dbg_printf("cartInfoInitConstant location : ");
    dbg_hexa((u32)offset);
    dbg_printf("\n\n");
}

static void patchCartRead(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	// Slot-2 cart read
	u32* offset = patchOffsetCache.cartReadOffset;
	if (!patchOffsetCache.cartReadOffsetChecked) {
		offset = findCartReadOffset(ndsHeader, usesThumb);
		if (offset) {
			patchOffsetCache.cartReadOffset = offset;
		}
		patchOffsetCache.cartReadOffsetChecked = true;
	}
	if (!offset) {
		return;
	}

	// Patch
	u32* cartReadPatch = (usesThumb ? ce9->thumbPatches->cart_read : ce9->patches->cart_read);
	tonccpy(offset, cartReadPatch, 0x40);
    dbg_printf("cartRead location : ");
    dbg_hexa((u32)offset);
    dbg_printf("\n\n");
}*/

static void patchWaitSysCycles(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32* offset = patchOffsetCache.waitSysCyclesOffset;

	if (isSdk5(moduleParams)) {
		if (!patchOffsetCache.waitSysCyclesOffset) {
			offset = findWaitSysCyclesOffset(ndsHeader);
			if (offset) {
				patchOffsetCache.waitSysCyclesOffset = offset;
			}
		}

		if ((ndsHeader->unitCode == 0 && (boostCpu || dsiModeConfirmed)) || (ndsHeader->unitCode > 0 && boostCpu && !dsiModeConfirmed)) {
			if (offset[0] == 0xE92D4008) {
				offset[1] = 0xE1A00100; // mov r0, r0, lsl#2
				if (ndsHeader->unitCode > 0) {
					offset[2] = 0xE1A00000; // nop
					offset[3] = 0xE1A00000; // nop
					offset[4] = 0xE1A00000; // nop
					offset[5] = 0xE1A00000; // nop
					offset[6] = 0xE1A00000; // nop
					offset[7] = 0xE1A00000; // nop
				}
			} else {
				u16* offsetThumb = (u16*)offset;
				offsetThumb[1] = 0x0080; // lsls r0, r0, #2
				if (ndsHeader->unitCode > 0) {
					offsetThumb[2] = 0x46C0; // nop
					offsetThumb[3] = 0x46C0; // nop
					offsetThumb[4] = 0x46C0; // nop
					offsetThumb[5] = 0x46C0; // nop
					offsetThumb[6] = 0x46C0; // nop
					offsetThumb[7] = 0x46C0; // nop
					offsetThumb[8] = 0x46C0; // nop
				}
			}

			if (dsiModeConfirmed) {	
				*(u32*)0x027FEFF4 = (u32)offset;
			}
		}

		dbg_printf("waitSysCycles location : ");
		dbg_hexa((u32)offset);
		dbg_printf("\n\n");
		return;
	}

	if (!patchOffsetCache.waitSysCyclesOffset) {
		offset = findWaitCpuCyclesOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.waitSysCyclesOffset = offset;
		}
	}

	if (boostCpu || dsiModeConfirmed) {
		offset[0] = 0xE59F3000; // ldr r3, =waitSysCycles
		offset[1] = 0xE12FFF13; // bx r3
		offset[2] = (u32)ce9->patches->waitSysCycles;

		if (dsiModeConfirmed) {	
			*(u32*)((u32)INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED + 4) = (u32)ce9->patches->waitSysCycles;
		}
	}

	dbg_printf("waitCpuCycles location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

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

void patchHiHeapPointer(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	extern u8 consoleModel;
	extern u32 cheatSizeTotal;
	const bool ROMsupportsDsiMode = (ndsHeader->unitCode > 0 && dsiModeConfirmed);
	const bool cheatsEnabled = (cheatSizeTotal > 4 && cheatSizeTotal <= 0x8000);
	const char* romTid = getRomTid(ndsHeader);

	if (moduleParams->sdk_version < 0x2008000 || !dsiModeConfirmed || strncmp(romTid, "UBR", 3) == 0) {
		return;
	}

	u32* heapPointer = patchOffsetCache.heapPointerOffset;
	if (*patchOffsetCache.heapPointerOffset != (ROMsupportsDsiMode ? 0x13A007BE : 0x023E0000)
	 && *patchOffsetCache.heapPointerOffset != (ROMsupportsDsiMode ? 0xE3A007BE : 0x023E0000)
	 && *patchOffsetCache.heapPointerOffset != (ROMsupportsDsiMode ? 0x048020BE : 0x023E0000)) {
		patchOffsetCache.heapPointerOffset = 0;
	}
	if (!patchOffsetCache.heapPointerOffset) {
		heapPointer = findHeapPointer2Offset(moduleParams, ndsHeader);
	}
    if(heapPointer) {
		patchOffsetCache.heapPointerOffset = heapPointer;
	} else {
		dbg_printf("Heap pointer not found\n");
		return;
	}

    dbg_printf("hi heap end: ");
	dbg_hexa((u32)heapPointer);
    dbg_printf("\n\n");

	if (ROMsupportsDsiMode && ((gameOnFlashcard && consoleModel == 0 && !cheatsEnabled) || consoleModel > 0) && (u8)a9ScfgRom == 1) {
		return;
	}

	u32* oldheapPointer = (u32*)*heapPointer;

	dbg_printf("old hi heap value: ");
	dbg_hexa((u32)oldheapPointer);
	dbg_printf("\n\n");

	const u32 dsiHiHeap = ((gameOnFlashcard && consoleModel == 0 && !cheatsEnabled) || consoleModel > 0) ? 0x02F70000 : retail_CACHE_ADRESS_START_TWLSDK_SMALL;
	bool newHiHeapReported = false;

	if (ROMsupportsDsiMode) {
		/* if (!dsiWramAccess) {
			// DSi WRAM not mapped to ARM9
			// DSi mode title loaded on DSi/3DS
			switch (*heapPointer) {
				case 0x13A007BE:
					*heapPointer = (u32)0x13A007BA; // MOVNE R0, #0x2E80000
					break;
				case 0xE3A007BE:
					*heapPointer = (u32)0xE3A007BA; // MOV R0, #0x2E80000
					break;
				case 0x048020BE:
					*heapPointer = (u32)0x048020BA; // MOVS R0, #0x2E80000
					break;
			}
		} else { */
			// DSi mode title loaded on DSi from SD card, or DSi/3DS with external DSi BIOS files loaded
			if (!gameOnFlashcard && (strncmp(romTid, "IRB", 3) == 0 || strncmp(romTid, "IRA", 3) == 0 || strncmp(romTid, "IRE", 3) == 0 || strncmp(romTid, "IRD", 3) == 0)) {
				// Pokemon Black & White 1&2
				switch (*heapPointer) {
					case 0x13A007BE:
						*heapPointer = (u32)0x13A0062D; // MOVNE R0, #0x2D00000
						break;
					case 0xE3A007BE:
						*heapPointer = (u32)0xE3A0062D; // MOV R0, #0x2D00000
						break;
					case 0x048020BE:
						*heapPointer = (u32)0x048020B4; // MOVS R0, #0x2D00000
						break;
				}
			} else if (!gameOnFlashcard && strncmp(romTid, "V2G", 3) != 0 && strncmp(romTid, "DD3", 3) != 0) {
				switch (*heapPointer) {
					case 0x13A007BE:
						*heapPointer = (u32)0x13A0062F; // MOVNE R0, #0x2F00000
						break;
					case 0xE3A007BE:
						*heapPointer = (u32)0xE3A0062F; // MOV R0, #0x2F00000
						break;
					case 0x048020BE:
						*heapPointer = (u32)0x048020BC; // MOVS R0, #0x2F00000
						break;
				}
			} else switch (*heapPointer) {
				case 0x13A007BE: {
					// *heapPointer = (u32)0x13A007BD; // MOVNE R0, #0x2F40000
					heapPointer[-1] = 0xE3500001; // cmp r0, #1
					for (int i = 0xA0/sizeof(u32); i < 0xE0/sizeof(u32); i++) {
						if (heapPointer[i] == 0x023E0000) {
							dbg_printf("hi heap end: ");
							dbg_hexa((u32)heapPointer+(i*sizeof(u32)));
							dbg_printf("\n\n");

							heapPointer[i] = dsiHiHeap;

							dbg_printf("new hi heap value: ");
							dbg_hexa(heapPointer[i]);
							dbg_printf("\n\n");

							newHiHeapReported = true;
							break;
						}
					}
				}	break;
				case 0xE3A007BE: {
					// *heapPointer = (u32)0xE3A007BD; // MOV R0, #0x2F40000
					const bool debuggerSdk = (heapPointer[1] == 0xEA000043);
					u32 addr = (u32)heapPointer;

					if (debuggerSdk) { // debuggerSdk
						*(u32*)(addr) = 0xE59F0134; // ldr r0, =0x????????
						*(u32*)(addr+0x28) = 0xE3560001; // cmp r6, #1
						*(u32*)(addr+0x30) = 0xE3A00627; // mov r0, #0x2700000

						*(u32*)(addr+0x58) = 0xE3A00C00; // mov r0, #*(u32*)(addr+0x13C)
						if (*(u32*)(addr+0x13C) != 0) {
							for (u32 i = 0; i < *(u32*)(addr+0x13C); i += 0x100) {
								*(u32*)(addr+0x58) += 1;
							}
						}

						*(u32*)(addr+0x13C) = dsiHiHeap;
					} else {
						*(u32*)(addr) = 0xE59F0094; // ldr r0, =0x2F60000

						*(u32*)(addr+0x40) = 0xE3A01C00; // mov r1, #*(u32*)(addr+0x9C)
						if (*(u32*)(addr+0x9C) != 0) {
							for (u32 i = 0; i < *(u32*)(addr+0x9C); i += 0x100) {
								*(u32*)(addr+0x40) += 1;
							}
						}

						*(u32*)(addr+0x9C) = dsiHiHeap;
					}
					dbg_printf("new hi heap value: ");
					dbg_hexa(*(u32*)(addr+(debuggerSdk ? 0x13C : 0x9C)));
					dbg_printf("\n\n");

					newHiHeapReported = true;
				}	break;
				case 0x048020BE: {
					u16* offsetThumb = (u16*)heapPointer;
					if (offsetThumb[-2] == 0x2800) { // cmp r0, #0
						offsetThumb[-2] = 0x2801; // cmp r0, #1
						for (int i = 0x60/sizeof(u32); i < 0xA0/sizeof(u32); i++) {
							if (heapPointer[i] == 0x023E0000) {
								dbg_printf("hi heap end: ");
								dbg_hexa((u32)heapPointer+(i*sizeof(u32)));
								dbg_printf("\n\n");

								heapPointer[i] = dsiHiHeap;

								dbg_printf("new hi heap value: ");
								dbg_hexa(heapPointer[i]);
								dbg_printf("\n\n");

								newHiHeapReported = true;
								break;
							}
						}
					} else {
						*heapPointer = (u32)0x048020BD; // MOVS R0, #0x2F40000
					}
				}	break;
			}
		// }
	} else {
		*heapPointer = 0x027C0000;
	}

	if (!newHiHeapReported) {
		dbg_printf("new hi heap value: ");
		dbg_hexa((u32)*heapPointer);
		dbg_printf("\n\n");
	}
    dbg_printf(ROMsupportsDsiMode ? "Hi Heap Shrink Successful\n\n" : "Hi Heap Grow Successful\n\n");
}

void patchHiHeapPointerDSiWare(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	u32* heapPointer = patchOffsetCache.heapPointerOffset;
	if (*patchOffsetCache.heapPointerOffset != 0x13A007BE
	 && *patchOffsetCache.heapPointerOffset != 0xE3A007BE
	 && *patchOffsetCache.heapPointerOffset != 0x048020BE) {
		patchOffsetCache.heapPointerOffset = 0;
	}
	if (!patchOffsetCache.heapPointerOffset) {
		heapPointer = findHeapPointer2Offset(moduleParams, ndsHeader);
	}
    if(heapPointer) {
		patchOffsetCache.heapPointerOffset = heapPointer;
	} else {
		dbg_printf("Heap pointer not found\n");
		return;
	}

    dbg_printf("hi heap end: ");
	dbg_hexa((u32)heapPointer);
    dbg_printf("\n\n");

	u32* oldheapPointer = (u32*)*heapPointer;

	dbg_printf("old hi heap value: ");
	dbg_hexa((u32)oldheapPointer);
	dbg_printf("\n\n");

	// DSi WRAM not mapped to ARM9
	switch (*heapPointer) {
		case 0x13A007BE:
			*heapPointer = (u32)0x13A0062F; // MOVNE R0, #0x2F00000
			break;
		case 0xE3A007BE:
			*heapPointer = (u32)0xE3A0062F; // MOV R0, #0x2F00000
			break;
		case 0x048020BE:
			*heapPointer = (u32)0x048020BC; // MOVS R0, #0x2F00000
			break;
	}

    dbg_printf("Hi Heap Shrink Successful\n\n");
}

/* void patchA9Mbk(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool standAlone) {
	if (dsiWramAccess) {
		return;
	}

	u32* mbkWramBOffset = patchOffsetCache.mbkWramBOffset;
	if (!patchOffsetCache.mbkWramBOffset) {
		if (standAlone) {
			mbkWramBOffset = findMbkWramBOffsetBoth(ndsHeader, moduleParams, (bool*)&patchOffsetCache.a9IsThumb);
		} else {
			if (patchOffsetCache.a9IsThumb) {
				mbkWramBOffset = (u32*)findMbkWramBOffsetThumb(ndsHeader, moduleParams);
			} else {
				mbkWramBOffset = findMbkWramBOffset(ndsHeader, moduleParams);
			}
		}
		if (mbkWramBOffset) {
			patchOffsetCache.mbkWramBOffset = mbkWramBOffset;
		}
	}
	if (mbkWramBOffset) {
		if (patchOffsetCache.a9IsThumb) {
			u16* offsetThumb = (u16*)mbkWramBOffset;

			// WRAM-B
			offsetThumb[0] = 0x20BB; // MOVS R0, #0x2EC0000
			offsetThumb[1] = 0x0480;
			offsetThumb[2] = 0x4770; // bx lr

			// WRAM-C
			offsetThumb[14] = 0x20BA; // MOVS R0, #0x2E80000
			offsetThumb[15] = 0x0480;
			offsetThumb[16] = 0x4770; // bx lr
		} else {
			// WRAM-B
			mbkWramBOffset[0]  = 0xE3A007BB; // MOV R0, #0x2EC0000
			mbkWramBOffset[1]  = 0xE12FFF1E; // bx lr

			// WRAM-C
			mbkWramBOffset[10] = 0xE3A007BA; // MOV R0, #0x2E80000
			mbkWramBOffset[11] = 0xE12FFF1E; // bx lr
		}
	}
	if (!mbkWramBOffset) {
		return;
	}

	dbg_printf("mbkWramBCGet location : ");
	dbg_hexa((u32)mbkWramBOffset);
	dbg_printf("\n\n");
} */

void patchSharedFontPath(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const ltd_module_params_t* ltdModuleParams) {
	if (!isDSiWare) {
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

	if (REG_SCFG_ROM & BIT(9)) {
		extern u32 iUncompressedSizei;

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
				return;
			}

			dbg_printf("fileIoRead location : ");
			dbg_hexa((u32)fileIoRead);
			dbg_printf("\n\n");

			/*dbg_printf("moduleParams->static_bss_end : ");
			dbg_hexa((u32)moduleParams->static_bss_end);
			dbg_printf("\n\n");

			dbg_printf("ltdModuleParams->arm9i_offset : ");
			dbg_hexa((u32)ltdModuleParams->arm9i_offset);
			dbg_printf("\n\n");*/

			tonccpy(moduleParams->static_bss_end, ltdModuleParams->arm9i_offset, iUncompressedSizei);

			//bool armFound = false;
			u32* arm9idst = moduleParams->static_bss_end;
			for (u32 i = 0; i < iUncompressedSizei/4; i++) {
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
						}
					}
				} /*else if ((u8)a9ScfgRom != 1
						&& (arm9idst[i] == 0x4770DF20 || arm9idst[i] == 0x4770DF21 || arm9idst[i] == 0x4770DF22 || arm9idst[i] == 0x4770DF23
						 || arm9idst[i] == 0x4770DF24 || arm9idst[i] == 0x4770DF25 || arm9idst[i] == 0x4770DF26 || arm9idst[i] == 0x4770DF27
						 || arm9idst[i] == 0x4770DF28 || arm9idst[i] == 0x4770DF29)) {
					// Stub out DSi BIOS functions
					arm9idst[i] = 0x47702001;
				}*/
			}
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

			tonccpy(moduleParams->static_bss_end, ltdModuleParams->arm9i_offset, iUncompressedSizei);

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
			for (u32 i = 0; i < iUncompressedSizei/4; i++) {
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

		tonccpy(ltdModuleParams->arm9i_offset, moduleParams->static_bss_end, iUncompressedSizei);
		toncset(moduleParams->static_bss_end, 0, iUncompressedSizei);
	} else {
		extern bool i2cBricked;
		if (!i2cBricked) {
			extern int sharedFontRegion;
			if ((sharedFontRegion == 2 && ndsHeader->gameCode[3] == 'K')
			 || (sharedFontRegion == 1 && ndsHeader->gameCode[3] == 'C')
			 || (sharedFontRegion == 0 && ndsHeader->gameCode[3] != 'C' && ndsHeader->gameCode[3] != 'K')) {
				return;
			}
		}

		const char* twlFontPath = "sdmc:/_nds/nds-bootstrap/TWLFontTable.dat";
		const char* chnFontPath = "sdmc:/_nds/nds-bootstrap/CHNFontTable.dat";
		const char* korFontPath = "sdmc:/_nds/nds-bootstrap/KORFontTable.dat";
		const u32 newFontPathOffset = 0x02FFDC00;

		bool found = false;
		extern u32 iUncompressedSize;
		extern u32 iUncompressedSizei;
		u32* arm9idst = (u32*)*(u32*)0x02FFE1C8;
		for (u32 i = 0; i < iUncompressedSizei/4; i++) {
			if (arm9idst[i] == (u32)offset) {
				arm9idst[i] = newFontPathOffset;
				found = true;
			}
		}

		if (!found) {
			u32* arm9dst = (u32*)ndsHeader->arm9destination;
			for (u32 i = 0; i < iUncompressedSize/4; i++) {
				if (arm9dst[i] == (u32)offset) {
					arm9dst[i] = newFontPathOffset;
					found = true;
				}
			}
		}

		if (!found) {
			return;
		}

		if (ndsHeader->gameCode[3] == 'K') {
			tonccpy((u32*)newFontPathOffset, korFontPath, strlen(korFontPath));
		} else if (ndsHeader->gameCode[3] == 'C') {
			tonccpy((u32*)newFontPathOffset, chnFontPath, strlen(chnFontPath));
		} else {
			tonccpy((u32*)newFontPathOffset, twlFontPath, strlen(twlFontPath));
		}
	}
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
		} else if (*(u8*)(srci+3) == 0xFB) { // If blx instruction...
			// then fix it
			u32* offsetByBlx = getOffsetFromBLX((u32*)srci);
			setBLX(dsti, (u32)offsetByBlx);
		}
		srci += 4;
		dsti += 4;
	}
}

/* void relocate_ce9(u32 default_location, u32 current_location, u32 size) {
    dbg_printf("relocate_ce9\n");

    u32 location_sig[1] = {default_location};

    u32* firstCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!firstCardLocation) {
		return;
	}
    dbg_printf("firstCardLocation ");
	dbg_hexa((u32)firstCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*firstCardLocation);
    dbg_printf("\n\n");

    *firstCardLocation = current_location;

	u32* armReadCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!armReadCardLocation) {
		return;
	}
    dbg_printf("armReadCardLocation ");
	dbg_hexa((u32)armReadCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadCardLocation);
    dbg_printf("\n\n");

    *armReadCardLocation = current_location;

    u32* thumbReadCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!thumbReadCardLocation) {
		return;
	}
    dbg_printf("thumbReadCardLocation ");
	dbg_hexa((u32)thumbReadCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadCardLocation);
    dbg_printf("\n\n");

    *thumbReadCardLocation = current_location;

    u32* armReadDmaCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!armReadDmaCardLocation) {
		return;
	}
    dbg_printf("armReadCardDmaLocation ");
	dbg_hexa((u32)armReadDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadDmaCardLocation);
    dbg_printf("\n\n");

    *armReadDmaCardLocation = current_location;

    u32* armReadCartLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!armReadCartLocation) {
		return;
	}
    dbg_printf("armReadCartLocation ");
	dbg_hexa((u32)armReadCartLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armReadCartLocation);
    dbg_printf("\n\n");

    *armReadCartLocation = current_location;

    u32* armSetDmaCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!armSetDmaCardLocation) {
		return;
	}
    dbg_printf("armSetDmaCardLocation ");
	dbg_hexa((u32)armSetDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armSetDmaCardLocation);
    dbg_printf("\n\n");

    *armSetDmaCardLocation = current_location;

    u32* thumbReadDmaCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!thumbReadDmaCardLocation) {
		return;
	}
    dbg_printf("thumbReadCardDmaLocation ");
	dbg_hexa((u32)thumbReadDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadDmaCardLocation);
    dbg_printf("\n\n");

    *thumbReadDmaCardLocation = current_location;

    u32* thumbSetDmaCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!thumbSetDmaCardLocation) {
		return;
	}
    dbg_printf("thumbSetDmaCardLocation ");
	dbg_hexa((u32)thumbSetDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbSetDmaCardLocation);
    dbg_printf("\n\n");

    *thumbSetDmaCardLocation = current_location;

    u32* armIrqEnableLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!armIrqEnableLocation) {
		return;
	}
    dbg_printf("armIrqEnableLocation ");
	dbg_hexa((u32)armIrqEnableLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*armIrqEnableLocation);
    dbg_printf("\n\n");

    *armIrqEnableLocation = current_location;

    u32* thumbIrqEnableLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!thumbIrqEnableLocation) {
		return;
	}
    dbg_printf("thumbIrqEnableLocation ");
	dbg_hexa((u32)thumbIrqEnableLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbIrqEnableLocation);
    dbg_printf("\n\n");

    *thumbIrqEnableLocation = current_location;

    u32* thumbResetLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!thumbResetLocation) {
		return;
	}
    dbg_printf("thumbResetLocation ");
	dbg_hexa((u32)thumbResetLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbResetLocation);
    dbg_printf("\n\n");

    *thumbResetLocation = current_location;

    u32* pdashReadLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!pdashReadLocation) {
		return;
	}
    dbg_printf("pdashReadLocation ");
	dbg_hexa((u32)pdashReadLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*pdashReadLocation);
    dbg_printf("\n\n");

    *pdashReadLocation = current_location;

    u32* thumbReadSlot2Location = findOffset((u32*)current_location, size, location_sig, 1);
	if (!thumbReadSlot2Location) {
		return;
	}
    dbg_printf("thumbReadSlot2Location ");
	dbg_hexa((u32)thumbReadSlot2Location);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadSlot2Location);
    dbg_printf("\n\n");

    *thumbReadSlot2Location = current_location;

	u32* ipcSyncHandlerLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!ipcSyncHandlerLocation) {
		return;
	}
    dbg_printf("ipcSyncHandlerLocation ");
	dbg_hexa((u32)ipcSyncHandlerLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*ipcSyncHandlerLocation);
    dbg_printf("\n\n");

    *ipcSyncHandlerLocation = current_location;

	u32* resetLocation = findOffset((u32*)current_location, size, location_sig, 1);
	if (!resetLocation) {
		return;
	}
    dbg_printf("resetLocation ");
	dbg_hexa((u32)resetLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*resetLocation);
    dbg_printf("\n\n");

    *resetLocation = current_location;

    u32* globalCardLocation = findOffset((u32*)current_location, size, location_sig, 1);
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
    ce9->patches->waitSysCycles = (u32*)((u32)ce9->patches->waitSysCycles - default_location + current_location);
    ce9->patches->cart_read = (u32*)((u32)ce9->patches->cart_read - default_location + current_location);
    ce9->patches->cacheFlushRef = (u32*)((u32)ce9->patches->cacheFlushRef - default_location + current_location);
    ce9->patches->cardEndReadDmaRef = (u32*)((u32)ce9->patches->cardEndReadDmaRef - default_location + current_location);
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
    ce9->thumbPatches->cart_read = (u32*)((u32)ce9->patches->cart_read - default_location + current_location);
    ce9->thumbPatches->cacheFlushRef = (u32*)((u32)ce9->thumbPatches->cacheFlushRef - default_location + current_location);
    ce9->thumbPatches->cardEndReadDmaRef = (u32*)((u32)ce9->thumbPatches->cardEndReadDmaRef - default_location + current_location);
    ce9->thumbPatches->sleepRef = (u32*)((u32)ce9->thumbPatches->sleepRef - default_location + current_location);
    ce9->thumbPatches->reset_arm9 = (u32*)((u32)ce9->thumbPatches->reset_arm9 - default_location + current_location);
} */

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

	if (sdPatchEntry) {
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
	} else
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
		tonccpy((u32*)0x020E2BF0, nandWritePatch, 0x40);

		//u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
		u32* nandReadPatch = ce9->patches->nand_read_arm9;
		tonccpy((u32*)0x020E2F3C, nandReadPatch, 0x40);
	} else
	// Nintendo DS Guide (World)
	if (strncmp(romTid, "UGD", 3) == 0) {
		//u32 gNandInit(void* data)
		*(u32*)(0x02009298) = 0xe3a00001; //mov r0, #1
		*(u32*)(0x0200929C) = 0xe12fff1e; //bx lr

		//u32 gNandResume(void)
		*(u32*)(0x020098C8) = 0xe3a00000; //mov r0, #0
		*(u32*)(0x020098CC) = 0xe12fff1e; //bx lr

		//u32 gNandState(void)
		*(u32*)(0x02009AA8) = 0xe1a00000; //nop
		*(u32*)(0x02009AB0) = 0x06600000;

		//u32 gNandError(void)
		*(u32*)(0x02009AB4) = 0xe3a00000; //mov r0, #0
		*(u32*)(0x02009AB8) = 0xe12fff1e; //bx lr

		//u32 gNandWrite(void* memory,void* flash,u32 size,u32 dma_channel)
		u32* nandWritePatch = ce9->patches->nand_write_arm9;
		tonccpy((u32*)0x0200961C, nandWritePatch, 0x40);

		//u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
		u32* nandReadPatch = ce9->patches->nand_read_arm9;
		tonccpy((u32*)0x02009940, nandReadPatch, 0x40);
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
	extern u8 consoleModel;

	// Opera RAM patch (ARM9)
	*(u32*)0x02003D48 = 0xC3E0000;
	*(u32*)0x02003D4C = 0xC3E0004;

	*(u32*)0x02010FF0 = 0xC3E0000;
	*(u32*)0x02010FF4 = 0xC3E00CE;

	*(u32*)0x020112AC = 0xC3E0080;

	*(u32*)0x020402BC = 0xC3E00C2;
	*(u32*)0x020402C0 = 0xC3E00C0;
	if (consoleModel > 0) {
		*(u32*)0x020402CC = 0xD7FFFFE;
		*(u32*)0x020402D0 = 0xD000000;
		*(u32*)0x020402D4 = 0xD1FFFFF;
		*(u32*)0x020402D8 = 0xD3FFFFF;
		*(u32*)0x020402DC = 0xD7FFFFF;
		*(u32*)0x020402E0 = 0xDFFFFFF;	// ???
		dma_twlFill32(0, 0xFFFFFFFF, (u32*)0xD000000, 0x800000);		// Fill fake MEP with FFs
	} else {
		*(u32*)0x020402CC = 0xCFFFFFE;
		*(u32*)0x020402D0 = 0xC800000;
		*(u32*)0x020402D4 = 0xC9FFFFF;
		*(u32*)0x020402D8 = 0xCBFFFFF;
		*(u32*)0x020402DC = 0xCFFFFFF;
		*(u32*)0x020402E0 = 0xD7FFFFF;	// ???
		dma_twlFill32(0, 0xFFFFFFFF, (u32*)0xC800000, 0x800000);		// Fill fake MEP with FFs
	}
}

/*static void setFlushCache(cardengineArm9* ce9, u32 patchMpuRegion, bool usesThumb) {
	//if (!usesThumb) {
	ce9->patches->needFlushDCCache = (patchMpuRegion == 1);
}*/

u32 patchCardNdsArm9(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const ltd_module_params_t* ltdModuleParams, u32 ROMinRAM, u32 patchMpuRegion, const bool usesCloneboot) {

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
	} else if (strncmp(romTid, "UGD", 3) == 0) { // Start at 0x2010000 for "Nintendo DS Guide"
		startOffset = (u32)ndsHeader->arm9destination + 0x10000;
	}

    dbg_printf("startOffset : ");
    dbg_hexa(startOffset);
    dbg_printf("\n\n");

	patchMpu(ndsHeader, moduleParams, patchMpuRegion);
	patchMpu2(ndsHeader, moduleParams, usesCloneboot);
	patchMpuChange(ndsHeader, moduleParams);
	patchMpuInitTwl(ndsHeader);

	patchWaitSysCycles(ce9, ndsHeader, moduleParams);

	if (isPawsAndClaws(ndsHeader)) {
		patchCardId(ce9, ndsHeader, moduleParams, false, NULL); // Patch card ID first
	}

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &sdk5ReadType, &cardReadEndOffset, startOffset)) {
		dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

	fixForDifferentBios(ce9, ndsHeader, usesThumb, usesCloneboot);

	if (ndsHeader->unitCode > 0 && dsiModeConfirmed) {
		// patchCardRomInit(cardReadEndOffset, usesThumb);

		if (!patchCardHashInit(ndsHeader, moduleParams, usesThumb)) {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}

		// patchA9Mbk(ndsHeader, moduleParams, false);
		patchSharedFontPath(ce9, ndsHeader, moduleParams, ltdModuleParams);
	}

	if (strncmp(romTid, "V2G", 3) == 0 && !dsiModeConfirmed) {
		// try to patch card read a second time
		dbg_printf("patch card read a second time\n");
		dbg_printf("startOffset : 0x02030000\n\n");
		if (!patchCardReadMvDK4(0x02030000)) {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
	}

	patchCardReadPdash(ce9, ndsHeader);

    // made obsolete by tonccpy
	//patchCardReadCached(ce9, ndsHeader, moduleParams, usesThumb);

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, sdk5ReadType, &cardPullOutOffset);

	//patchCardTerminateForPullOut(ce9, usesThumb, ndsHeader, moduleParams, cardPullOutOffset);

	patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	//patchForceToPowerOff(ce9, ndsHeader, usesThumb);

	if (!isPawsAndClaws(ndsHeader)) {
		if (!patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset)) {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
	}

	patchGbaSlotInit(ndsHeader, usesThumb);

	//patchCardRefresh(ndsHeader, moduleParams, usesThumb);

	if (getSleep(ce9, ndsHeader, moduleParams, usesThumb, ROMinRAM)) {
		patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
	} else {
		if (!patchCardSetDma(ce9, ndsHeader, moduleParams, usesThumb)) {
			patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
		}
		if (!patchCardEndReadDma(ce9, ndsHeader, moduleParams, usesThumb) && (ndsHeader->unitCode == 0 || !dsiModeConfirmed)) {
			randomPatch(ndsHeader, moduleParams);
			randomPatch5Second(ndsHeader, moduleParams);
		}
	}

	//patchDownloadplay(ndsHeader);

    //patchSleep(ce9, ndsHeader, moduleParams, usesThumb);

	patchReset(ce9, ndsHeader, moduleParams);
	patchResetTwl(ce9, ndsHeader, moduleParams);

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch();
	} /*else if (gbaRomFound) {
		if (patchCartExist(ndsHeader, moduleParams, usesThumb)) {
			patchCartInfoInitConstant(ndsHeader, moduleParams, usesThumb);

			patchCartRead(ce9, ndsHeader, moduleParams, usesThumb);
		}
	}*/

	nandSavePatch(ce9, ndsHeader, moduleParams);

	//setFlushCache(ce9, patchMpuRegion, usesThumb);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
