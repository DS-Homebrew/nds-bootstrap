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

extern u32 gameOnFlashcard;
extern u32 saveOnFlashcard;

extern bool gbaRomFound;

static void fixForDsiBios(const cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
	u32* swi12Offset = patchOffsetCache.a9Swi12Offset;
	if (!patchOffsetCache.a9Swi12Offset) {
		swi12Offset = a9_findSwi12Offset(ndsHeader);
		if (swi12Offset) {
			patchOffsetCache.a9Swi12Offset = swi12Offset;
		}
	}

	if (!(REG_SCFG_ROM & BIT(1))) {
		// swi 0x12 call
		if (swi12Offset) {
			// Patch to call swi 0x02 instead of 0x12
			u32* swi12Patch = ce9->patches->swi02;
			memcpy(swi12Offset, swi12Patch, 0x4);
		}
	}
}

static bool patchCardRead(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumbPtr, int* readTypePtr, int* sdk5ReadTypePtr, u32** cardReadEndOffsetPtr) {
	bool usesThumb = patchOffsetCache.a9IsThumb;
	int readType = 0;
	int sdk5ReadType = 0; // SDK 5

	// Card read
	// SDK 5
	//dbg_printf("Trying SDK 5 thumb...\n");
	u32* cardReadEndOffset = patchOffsetCache.cardReadEndOffset;
	if (!patchOffsetCache.cardReadEndOffset) {
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type0(ndsHeader, moduleParams);
		if (cardReadEndOffset) {
			usesThumb = true;
			patchOffsetCache.a9IsThumb = usesThumb;
		}
		if (!cardReadEndOffset) {
			// SDK 5
			cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type1(ndsHeader, moduleParams);
			if (cardReadEndOffset) {
				sdk5ReadType = 1;
				usesThumb = true;
				patchOffsetCache.a9IsThumb = usesThumb;
			}
		}
		if (!cardReadEndOffset) {
			//dbg_printf("Trying thumb...\n");
			cardReadEndOffset = (u32*)findCardReadEndOffsetThumb(ndsHeader);
			if (cardReadEndOffset) {
				usesThumb = true;
				patchOffsetCache.a9IsThumb = usesThumb;
			}
		}
		if (!cardReadEndOffset) {
			cardReadEndOffset = findCardReadEndOffsetType0(ndsHeader, moduleParams);
		}
		if (!cardReadEndOffset) {
			//dbg_printf("Trying alt...\n");
			cardReadEndOffset = findCardReadEndOffsetType1(ndsHeader);
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
				cardReadStartOffset = findCardReadStartOffsetType0(cardReadEndOffset);
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
	memcpy(cardReadStartOffset, cardReadPatch, usesThumb ? (isSdk5(moduleParams) ? 0xB0 : 0xA0) : 0xE0); // 0xE0 = 0xF0 - 0x08
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
	memcpy(cardPullOutOffset, cardPullOutPatch, usesThumb ? 0x2 : 0x30);
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
		dbg_printf("Found cardId\n\n");
                
        // Patch
		u32* cardIdPatch = (usesThumb ? ce9->thumbPatches->card_id_arm9 : ce9->patches->card_id_arm9);

		cardIdPatch[usesThumb ? 1 : 2] = getChipId(ndsHeader, moduleParams);
		memcpy(cardIdStartOffset, cardIdPatch, usesThumb ? 0x8 : 0xC);
	}
}

static void patchCardReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	// Card read dma
	u32* cardReadDmaStartOffset = patchOffsetCache.cardReadDmaOffset;
	if (!patchOffsetCache.cardReadDmaChecked) {
		cardReadDmaStartOffset = NULL;
		u32* cardReadDmaEndOffset = NULL;
		if (usesThumb) {
			//dbg_printf("Trying thumb alt...\n");
			cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
		}
		if (!cardReadDmaEndOffset) {
			cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader);
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
	memcpy(cardReadDmaStartOffset, cardReadDmaPatch, 0x40);
}

static void patchCardEndReadDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	bool dmaAllowed = false;
    const char* romTid = getRomTid(ndsHeader);
	static const char list[][4] = {
        "YGX",  // GTA Chinatown Wars // works
        "C32",	// Ace Attorney Investigations: Miles Edgeworth // works
        "A3P",	// Anpanman to Touch de Waku Waku Training // sdk5
        "YBA",  // Bomberman 2 // works
        "TBR",  // Brave // sdk5
        "YR9",  // Castlevania OE // works
        //"ACV",  // Castlevania DOS // black screen issue to be investigated
        "AMH",  // Metroid Prime Hunters // TODO : freeze issue to be investigated
        "AFF",  // FF3 // works
        "AXF",  // FFXII // works
        "BO5",  // Golden Sun // sdk5
        "Y8L",  // Golden Sun Demo // sdk5
        "AWI",  // Hotel Dusk // works
		"YEE",	// Inazuma Eleven
		"BEE",	// Inazuma Eleven 2 - Firestorm
		"BEB",	// Inazuma Eleven 2 - Blizzard
		"BEZ",	// Inazuma Eleven 3 - Bomb Blast
		"BE8",	// Inazuma Eleven 3 - Lightning Bolt
		"BOE",	// Inazuma Eleven 3 - Team Ogre Attacks!
        "C6C",  // Infinite Space // works
        "A5F",  // Layton: Curious Village // works
        "YLT",  // Layton: Pandora's Box // works
        "C3J",  // Layton: Unwound Future // works
        "BLF",  // Layton: Last Specter // sdk5
        //"APD",  // Pokemon Dash // TODO : freeze issue to be investigated
        "ADA",  // Pokemon Diamond // works
        "APA",  // Pokemon Pearl // works
        "CPU",  // Pokemon Platinum // works
        "IPK",  // Pokemon HeartGold // works
        "IPG",  // Pokemon SoulSilver // works
        "IRB",  // Pokemon Black // sdk5
        "IRA",  // Pokemon White // sdk5
        "IRE",  // Pokemon Black 2 // sdk5
        "IRD",  // Pokemon White 2 // sdk5
        "BR4",  // Runaway: A Twist of Fate // works, fixes sound cracking
        "BZ3",  // SaGa 3 // works
        "YT7",  // SEGA Superstars Tennis
        "CSN",  // Sonic Chronicles: The Dark BrotherHood
        "BXS",  // Sonic Colors // sdk5
        "A3Y",  // Sonic Rush Adventure // works, but title screen has some flickers (if not using sleep method)
        "CB6",  // Space Bust-A-Move // works, fixes lags
        "ASF",  // Star Fox Command // works
        "YG4",  // Suikoden: Tierkreis // works
        "YUT",  // Ultimate Mortal Kombat
        "A8Q",  // Theme Park // works
        "AH9",  // Tony Hawk's American Sk8land // works, fixes crashing
        "AWA",  // Wario: Master of Disguise // works
    };

	for (unsigned int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
		if (memcmp(romTid, list[i], 3) == 0) {
			// Found a match.
			dmaAllowed = true;
			break;
		}
	}

  if (dmaAllowed) {
    u32* offset = patchOffsetCache.cardEndReadDmaOffset;
	  if (!patchOffsetCache.cardEndReadDmaChecked) {
		offset = findCardEndReadDma(ndsHeader,moduleParams,usesThumb);
		if (offset) patchOffsetCache.cardEndReadDmaOffset = offset;
		patchOffsetCache.cardEndReadDmaChecked = true;
	  }
    if(offset) {
      dbg_printf("\nNDMA CARD READ ARM9 METHOD ACTIVE\n");
      if(!isSdk5(moduleParams)) {
        // SDK1-4        
        if(usesThumb) {
            u16* thumbOffset = (u16*)offset;
            thumbOffset--;
            *thumbOffset = 0xB5F8; // push	{r3-r7, lr} 
            ce9->thumbPatches->cardEndReadDmaRef = thumbOffset;
          } else {
            u32* armOffset = (u32*)offset;
            armOffset--;
            *armOffset = 0xE92D40F8; // STMFD SP!, {R3-R7,LR}
            ce9->patches->cardEndReadDmaRef = armOffset;
        }
      } else {
        // SDK5 
        if(usesThumb) {
            u16* thumbOffset = (u16*)offset;
            while(*thumbOffset!=0xB508) { // push	{r3, lr}
                *thumbOffset = 0x46C0; // NOP
                thumbOffset--;
            }
            ce9->thumbPatches->cardEndReadDmaRef = thumbOffset;
        } else  {
            u32* armOffset = (u32*)offset;
            armOffset--;
            *armOffset = 0xE92D4008; // STMFD SP!, {R3,LR}
            ce9->patches->cardEndReadDmaRef = armOffset;
        }  
      }  
    }
  }
}

static bool patchCardSetDma(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
    dbg_printf("\npatchCardSetDma\n");           

	bool dmaAllowed = false;
    const char* romTid = getRomTid(ndsHeader);
	static const char list[][4] = {
        "YGX",  // GTA Chinatown Wars // works
        "C32",	// Ace Attorney Investigations: Miles Edgeworth // works
        "A3P",	// Anpanman to Touch de Waku Waku Training // sdk5
        "YBA",  // Bomberman 2 // works
        "TBR",  // Brave // sdk5
        "YR9",  // Castlevania OE // works
        //"ACV",  // Castlevania DOS // black screen issue to be investigated
        "AMH",  // Metroid Prime Hunters // TODO : freeze issue to be investigated
        "AFF",  // FF3 // works
        "AXF",  // FFXII // works
        "BO5",  // Golden Sun // sdk5
        "Y8L",  // Golden Sun Demo // sdk5
        "AWI",  // Hotel Dusk // works
		"YEE",	// Inazuma Eleven
		"BEE",	// Inazuma Eleven 2 - Firestorm
		"BEB",	// Inazuma Eleven 2 - Blizzard
		"BEZ",	// Inazuma Eleven 3 - Bomb Blast
		"BE8",	// Inazuma Eleven 3 - Lightning Bolt
		"BOE",	// Inazuma Eleven 3 - Team Ogre Attacks!
        //"C6C",  // Infinite Space // freezes after SEGA logo
        "A5F",  // Layton: Curious Village // works
        "YLT",  // Layton: Pandora's Box // works
        "C3J",  // Layton: Unwound Future // works
        "BLF",  // Layton: Last Specter // sdk5
        //"APD",  // Pokemon Dash // TODO : freeze issue to be investigated
        "ADA",  // Pokemon Diamond // works
        "APA",  // Pokemon Pearl // works
        "CPU",  // Pokemon Platinum // works
        "IPK",  // Pokemon HeartGold // works
        "IPG",  // Pokemon SoulSilver // works
        "IRB",  // Pokemon Black // sdk5
        "IRA",  // Pokemon White // sdk5
        "IRE",  // Pokemon Black 2 // sdk5
        "IRD",  // Pokemon White 2 // sdk5
        "BR4",  // Runaway: A Twist of Fate // works, fixes sound cracking
        "BZ3",  // SaGa 3 // works
        "YT7",  // SEGA Superstars Tennis
        "CSN",  // Sonic Chronicles: The Dark BrotherHood
        "BXS",  // Sonic Colors // sdk5
        "A3Y",  // Sonic Rush Adventure // works, but title screen has some flickers (if not using sleep method)
        "CB6",  // Space Bust-A-Move // works, fixes lags
        "ASF",  // Star Fox Command // works
        "YG4",  // Suikoden: Tierkreis // works
        "YUT",  // Ultimate Mortal Kombat
        "A8Q",  // Theme Park // works
        "AH9",  // Tony Hawk's American Sk8land // works, fixes crashing
        "AWA",  // Wario: Master of Disguise // works
    };

	for (unsigned int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
		if (memcmp(romTid, list[i], 3) == 0) {
			// Found a match.
			dmaAllowed = true;
			break;
		}
	}

  if (dmaAllowed) {
    u32* setDmaoffset = patchOffsetCache.cardReadDmaOffset;
    if (!patchOffsetCache.cardSetDmaChecked) {
		setDmaoffset = findCardSetDma(ndsHeader,moduleParams,usesThumb);
		if (setDmaoffset) patchOffsetCache.cardReadDmaOffset = setDmaoffset;
		patchOffsetCache.cardSetDmaChecked = true;
    }
    if(setDmaoffset) {
      dbg_printf("\nNDMA CARD SET ARM9 METHOD ACTIVE\n");       
      u32* cardSetDmaPatch = (usesThumb ? ce9->thumbPatches->card_set_dma_arm9 : ce9->patches->card_set_dma_arm9);
	  memcpy(setDmaoffset, cardSetDmaPatch, 0x30);
    
      return true;  
    }
  }

    return false; 
}


static void patchMpu(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	extern u32 gameOnFlashcard;
    const char* romTid = getRomTid(ndsHeader);

	if (moduleParams->sdk_version > 0x5000000 && !gameOnFlashcard
	&& strncmp(romTid, "KPF", 3) != 0	// Pop Island: Paperfield
	) {
		return;
	}

	if (patchOffsetCache.patchMpuRegion != patchMpuRegion) {
		patchOffsetCache.patchMpuRegion = 0;
		patchOffsetCache.mpuStartOffset = 0;
		patchOffsetCache.mpuDataOffset = 0;
		patchOffsetCache.mpuInitCacheOffset = 0;
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
			case 2:
				mpuNewDataAccess  = 0x15111111;
				mpuNewInstrAccess = 0x5111111;
				mpuAccessOffset   = 6;
				break;
			case 3:
				mpuInitRegionNewData = PAGE_8M | 0x03000000 | 1;
				mpuNewInstrAccess    = 0x5111111;
				mpuAccessOffset      = 5;
				break;
		}

		*(vu32*)((u32)ndsHeader + 0x200) = (vu32)mpuDataOffset;
		*(vu32*)((u32)ndsHeader + 0x204) = (vu32)*mpuDataOffset;

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
	}

	// Patch out all further mpu reconfiguration
	dbg_printf("patchMpuSize: ");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n\n");
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	u32* mpuInitOffset = mpuStartOffset;
	while (mpuInitOffset && patchMpuSize) {
		u32 patchSize = ndsHeader->arm9binarySize;
		if (patchMpuSize > 1) {
			patchSize = patchMpuSize;
		}
		mpuInitOffset = findOffset(
			//(u32*)((u32)mpuStartOffset + 4), patchSize,
			mpuInitOffset + 1, patchSize,
			mpuInitRegionSignature, 1
		);
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
	}

	patchOffsetCache.patchMpuRegion = patchMpuRegion;
	patchOffsetCache.mpuStartOffset = mpuStartOffset;
	patchOffsetCache.mpuDataOffset = mpuDataOffset;
	patchOffsetCache.mpuInitCacheOffset = mpuInitCacheOffset;
}

/*static void patchSlot2Exist(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool *usesThumb) {
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
		*(slot2ExistEndOffset + 3) = 0x02400000;
		*(slot2ExistEndOffset + 4) = 0x024000CE;
	} else {
		*(slot2ExistEndOffset + 3) = 0x024000CE;

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

u32* patchHeapPointer(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
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
        dbg_printf("ERROR: Wrong heap pointer\n");
        dbg_printf("heap pointer value: ");
	    dbg_hexa(*heapPointer);
		dbg_printf("\n\n");
        return 0;
    } else if (!patchOffsetCache.heapPointerOffset) {
		patchOffsetCache.heapPointerOffset = heapPointer;
		patchOffsetCacheChanged = true;
	}

    u32* oldheapPointer = (u32*)*heapPointer;

    dbg_printf("old heap pointer: ");
	dbg_hexa((u32)oldheapPointer);
    dbg_printf("\n\n");

	*heapPointer = *heapPointer + (isSdk5(moduleParams) ? 0x3000 : 0x1800); // shrink heap by 6 KB (or for SDK5, 12 KB)

    dbg_printf("new heap pointer: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Heap Shrink Sucessfull\n\n");

    return oldheapPointer;
}

void patchHeapPointer2(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	if (moduleParams->sdk_version <= 0x2007FFF
	|| isSdk5(moduleParams)) {
		return;
	}

	u32* heapPointer = NULL;
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 0
	 || patchOffsetCache.heapPointerOffset != 0x023E0000) {
		patchOffsetCache.heapPointerOffset = 0;
	} else {
		heapPointer = patchOffsetCache.heapPointerOffset;
	}
	if (!patchOffsetCache.heapPointerOffset) {
		heapPointer = findHeapPointer2Offset(moduleParams, ndsHeader);
	}
    if(!heapPointer || *heapPointer<0x02000000 || *heapPointer>0x03000000) {
        dbg_printf("ERROR: Wrong heap pointer\n");
        dbg_printf("heap pointer value: ");
	    dbg_hexa(*heapPointer);    
		dbg_printf("\n\n");
        return;
    } else if (!patchOffsetCache.heapPointerOffset) {
		patchOffsetCache.heapPointerOffset = heapPointer;
		patchOffsetCacheChanged = true;
	}

    u32* oldheapPointer = (u32*)*heapPointer;

    dbg_printf("old heap end pointer: ");
	dbg_hexa((u32)oldheapPointer);
    dbg_printf("\n\n");

	*heapPointer = 0x023DC000; // shrink heap by 16KB

    dbg_printf("new heap 2 pointer: ");
	dbg_hexa((u32)*heapPointer);
    dbg_printf("\n\n");
    dbg_printf("Heap 2 Shrink Sucessfull\n\n");
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
    
    u32* thumbReadDmaCardLocation =  findOffset(current_location, size, location_sig, 1);
	if (!thumbReadDmaCardLocation) {
		return;
	}
    dbg_printf("thumbReadCardDmaLocation ");
	dbg_hexa((u32)thumbReadDmaCardLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbReadDmaCardLocation);
    dbg_printf("\n\n");
    
    *thumbReadDmaCardLocation = current_location;
    
    u32* thumbSetDmaCardLocation =  findOffset(current_location, size, location_sig, 1);
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
    
    u32* thumbWriteNandLocation =  findOffset(current_location, size, location_sig, 1);
	if (!thumbWriteNandLocation) {
		return;
	}
    dbg_printf("thumbWriteNandLocation ");
	dbg_hexa((u32)thumbWriteNandLocation);
    dbg_printf(" : ");
    dbg_hexa((u32)*thumbWriteNandLocation);
    dbg_printf("\n\n");
    
    *thumbWriteNandLocation = current_location;
    
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
    ce9->patches->card_pull_out_arm9 = (u32*)((u32)ce9->patches->card_pull_out_arm9 - default_location + current_location);
    ce9->patches->card_id_arm9 = (u32*)((u32)ce9->patches->card_id_arm9 - default_location + current_location);
    ce9->patches->card_dma_arm9 = (u32*)((u32)ce9->patches->card_dma_arm9 - default_location + current_location);
    ce9->patches->card_set_dma_arm9 = (u32*)((u32)ce9->patches->card_set_dma_arm9 - default_location + current_location);
    ce9->patches->nand_read_arm9 = (u32*)((u32)ce9->patches->nand_read_arm9 - default_location + current_location);
    ce9->patches->nand_write_arm9 = (u32*)((u32)ce9->patches->nand_write_arm9 - default_location + current_location);
    ce9->patches->cardStructArm9 = (u32*)((u32)ce9->patches->cardStructArm9 - default_location + current_location);
    ce9->patches->card_pull = (u32*)((u32)ce9->patches->card_pull - default_location + current_location);
    ce9->patches->cacheFlushRef = (u32*)((u32)ce9->patches->cacheFlushRef - default_location + current_location);
    ce9->patches->terminateForPullOutRef = (u32*)((u32)ce9->patches->terminateForPullOutRef - default_location + current_location);
    ce9->patches->pdash_read = (u32*)((u32)ce9->patches->pdash_read - default_location + current_location);
    ce9->patches->ipcSyncHandlerRef = (u32*)((u32)ce9->patches->ipcSyncHandlerRef - default_location + current_location);
    ce9->thumbPatches->card_read_arm9 = (u32*)((u32)ce9->thumbPatches->card_read_arm9 - default_location + current_location);
    ce9->thumbPatches->card_pull_out_arm9 = (u32*)((u32)ce9->thumbPatches->card_pull_out_arm9 - default_location + current_location);
    ce9->thumbPatches->card_id_arm9 = (u32*)((u32)ce9->thumbPatches->card_id_arm9 - default_location + current_location);
    ce9->thumbPatches->card_dma_arm9 = (u32*)((u32)ce9->thumbPatches->card_dma_arm9 - default_location + current_location);
    ce9->thumbPatches->card_set_dma_arm9 = (u32*)((u32)ce9->thumbPatches->card_set_dma_arm9 - default_location + current_location);
    ce9->thumbPatches->nand_read_arm9 = (u32*)((u32)ce9->thumbPatches->nand_read_arm9 - default_location + current_location);
    ce9->thumbPatches->nand_write_arm9 = (u32*)((u32)ce9->thumbPatches->nand_write_arm9 - default_location + current_location);
    ce9->thumbPatches->cardStructArm9 = (u32*)((u32)ce9->thumbPatches->cardStructArm9 - default_location + current_location);
    ce9->thumbPatches->card_pull = (u32*)((u32)ce9->thumbPatches->card_pull - default_location + current_location);
    ce9->thumbPatches->cacheFlushRef = (u32*)((u32)ce9->thumbPatches->cacheFlushRef - default_location + current_location);
    ce9->thumbPatches->terminateForPullOutRef = (u32*)((u32)ce9->thumbPatches->terminateForPullOutRef - default_location + current_location);
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
static void randomPatch5First(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	// Random patch SDK 5 first
	u32* randomPatchOffset5First = patchOffsetCache.randomPatch5Offset;
	if (!patchOffsetCache.randomPatch5Checked) {
		randomPatchOffset5First = findRandomPatchOffset5First(ndsHeader);
		if (randomPatchOffset5First) {
			patchOffsetCache.randomPatch5Offset = randomPatchOffset5First;
		}
		patchOffsetCache.randomPatch5Checked = true;
	}
	if (!randomPatchOffset5First) {
		return;
	}
	// Patch
	*randomPatchOffset5First = 0xE3A00000;
	//*(u32*)((u32)randomPatchOffset5First + 4) = 0xE12FFF1E;
	*(randomPatchOffset5First + 1) = 0xE12FFF1E;
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
    
    // WarioWare D.I.Y. (USA)
	if (strcmp(romTid, "UORE") == 0) {
		sdPatchEntry = 0x2002c04; 
	}
    // WarioWare D.I.Y. (Europe)"
    if (strcmp(romTid, "UORP") == 0) {
		sdPatchEntry = 0x2002ca4; 
	}
    // WarioWare D.I.Y. (Japan)
    if (strcmp(romTid, "UORJ") == 0) {
		sdPatchEntry = 0x2002be4; 
	}
    
    if(sdPatchEntry) {   
      //u32 gNandInit(void* data)
      *((u32*)(sdPatchEntry+0x50c+0)) = 0xe3a00001; //mov r0, #1
      *((u32*)(sdPatchEntry+0x50c+4)) = 0xe12fff1e; //bx lr
      
      //u32 gNandWait(void)
      *((u32*)(sdPatchEntry+0xc9c+0)) = 0xe12fff1e; //bx lr
      
      //u32 gNandState(void)
      *((u32*)(sdPatchEntry+0xeb0+0)) = 0xe3a00003; //mov r0, #3
      *((u32*)(sdPatchEntry+0xeb0+4)) = 0xe12fff1e; //bx lr
      
      //u32 gNandError(void)
      *((u32*)(sdPatchEntry+0xec8+0)) = 0xe3a00000; //mov r0, #0
      *((u32*)(sdPatchEntry+0xec8+4)) = 0xe12fff1e; //bx lr

      //u32 gNandWrite(void* memory,void* flash,u32 size,u32 dma_channel)
      u32* nandWritePatch = ce9->patches->nand_write_arm9;
      memcpy(sdPatchEntry+0x958, nandWritePatch, 0x40);
         
      //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
      u32* nandReadPatch = ce9->patches->nand_read_arm9;
      memcpy(sdPatchEntry+0xd24, nandReadPatch, 0x40);
    } else {  
        // Jam with the Band (Europe)
        if (strcmp(romTid, "UXBP") == 0) {
          	//u32 gNandInit(void* data)
            *((u32*)(0x020613cc+0)) = 0xe3a00001; //mov r0, #1
            *((u32*)(0x020613cc+4)) = 0xe12fff1e; //bx lr
            
            //u32 gNandResume(void)
            *((u32*)(0x02061a4c+0)) = 0xe3a00000; //mov r0, #0
            *((u32*)(0x02061a4c+4)) = 0xe12fff1e; //bx lr
            
            //u32 gNandError(void)
            *((u32*)(0x02061c24+0)) = 0xe3a00000; //mov r0, #0
            *((u32*)(0x02061c24+4)) = 0xe12fff1e; //bx lr
      
            //u32 gNandWrite(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandWritePatch = ce9->patches->nand_write_arm9;
            memcpy(0x0206176c, nandWritePatch, 0x40);
               
            //u32 gNandRead(void* memory,void* flash,u32 size,u32 dma_channel)
            u32* nandReadPatch = ce9->patches->nand_read_arm9;
            memcpy(0x02061ac4, nandReadPatch, 0x40);
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

static void operaRamPatch(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
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

	fixForDsiBios(ce9, ndsHeader);

	if (!patchCardRead(ce9, ndsHeader, moduleParams, &usesThumb, &readType, &sdk5ReadType, &cardReadEndOffset)) {
		dbg_printf("ERR_LOAD_OTHR\n\n");
		return ERR_LOAD_OTHR;
	}

    // made obsolete by tonccpy
	//patchCardReadCached(ce9, ndsHeader, moduleParams, usesThumb);

	patchCardPullOut(ce9, ndsHeader, moduleParams, usesThumb, sdk5ReadType, &cardPullOutOffset);

	//patchCardTerminateForPullOut(ce9, usesThumb, ndsHeader, moduleParams, cardPullOutOffset);

	patchCacheFlush(ce9, usesThumb, cardPullOutOffset);

	//patchForceToPowerOff(ce9, ndsHeader, usesThumb);

	patchCardId(ce9, ndsHeader, moduleParams, usesThumb, cardReadEndOffset);

	bool cardSetDmaPatched = false;
	if (!gameOnFlashcard || (gameOnFlashcard && ROMinRAM)) {
		cardSetDmaPatched = patchCardSetDma(ce9, ndsHeader, moduleParams, usesThumb);
	}

	if (!cardSetDmaPatched) {
		patchCardReadDma(ce9, ndsHeader, moduleParams, usesThumb);
	}

	patchMpu(ndsHeader, moduleParams, patchMpuRegion, patchMpuSize);

	//patchDownloadplay(ndsHeader);

    //patchSleep(ce9, ndsHeader, moduleParams, usesThumb);

	if (!gameOnFlashcard || (gameOnFlashcard && ROMinRAM)) {
		patchCardEndReadDma(ce9, ndsHeader, moduleParams, usesThumb);
	}

	if (gameOnFlashcard && !ROMinRAM) {
		patchHeapPointer2(moduleParams, ndsHeader);
	}

	randomPatch(ndsHeader, moduleParams);

	randomPatch5First(ndsHeader, moduleParams);

	randomPatch5Second(ndsHeader, moduleParams);

	const char* romTid = getRomTid(ndsHeader);

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch(ndsHeader, moduleParams);
	} /*else if (gbaRomFound) {
		patchSlot2Exist(ce9, ndsHeader, moduleParams, &slot2usesThumb);

		//patchSlot2Read(ce9, ndsHeader, moduleParams, &slot2usesThumb);
	}*/

	nandSavePatch(ce9, ndsHeader, moduleParams);

	patchCardReadPdash(ce9, ndsHeader);

	setFlushCache(ce9, patchMpuRegion, usesThumb);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
