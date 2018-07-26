#include <string.h> // memset
#include <stdlib.h> // malloc
#include "card_finder.h"
#include "debugToFile.h"

//#define memset __builtin_memset

extern u32 ROM_TID;

//
// Subroutine function signatures ARM9
//

// Module params
u32 moduleParamsSignature[2] = {0xDEC00621, 0x2106C0DE};

// Card read
u32 a9cardReadSignature[2]            = {0x04100010, 0x040001A4};
u32 a9cardReadSignatureAlt[2]         = {0x040001A4, 0x04100010};
u16 a9cardReadSignatureThumb[4]       = {0x01A4, 0x0400, 0x0200, 0x0000};
u32 cardReadStartSignature[1]         = {0xE92D4FF0};
u32 cardReadStartSignatureAlt[1]      = {0xE92D4070};
u16 cardReadStartSignatureThumb[2]    = {0xB5F8, 0xB082};
u16 cardReadStartSignatureThumbAlt[2] = {0xB5F0, 0xB083};

// Card read cached
u32 cardReadCachedStartSignature1[2] = {0xE92D4030, 0xE24DD004};
u32 cardReadCachedEndSignature1[4]   = {0xE5950020, 0xE3500000, 0x13A00001, 0x03A00000};

u32 cardReadCachedEndSignature3[4]   = {0xE5950024, 0xE3500000, 0x13A00001, 0x03A00000};

u32 cardReadCachedStartSignature4[2] = {0xE92D4038, 0xE59F407C};
u32 cardReadCachedEndSignature4[4]   = {0xE5940024, 0xE3500000, 0x13A00001, 0x03A00000};
  
//u32 a9instructionBHI[1] = {0x8A000001};

// Card pull out
u32 cardPullOutSignature1[4]        = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011};
u32 cardPullOutSignature4[4]        = {0xE92D4008, 0xE201003F, 0xE3500011, 0x1A00000D};
u16 cardPullOutSignatureThumb[4]    = {0xB508, 0x203F, 0x4008, 0x2811};
u16 cardPullOutSignatureThumbAlt[4] = {0xB500, 0xB081, 0x203F, 0x4001};

//u32 a9cardSendSignature[7] = {0xE92D40F0, 0xE24DD004, 0xE1A07000, 0xE1A06001, 0xE1A01007, 0xE3A0000E, 0xE3A02000};

// Force to power off
//u32 forceToPowerOffSignature[4] = {0xE92D4000, 0xE24DD004, 0xE59F0028, 0xE28D1000};

// Card id
u32 a9cardIdSignature[2]             = {0x040001A4, 0x04100010};
u16 a9cardIdSignatureThumb[6]        = {0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410};
u32 cardIdStartSignature[1]          = {0xE92D4000};
u32 cardIdStartSignatureAlt1[1]      = {0xE92D4008};
u32 cardIdStartSignatureAlt2[1]      = {0xE92D4010};
u16 cardIdStartSignatureThumb[2]     = {0xB500, 0xB081};
u16 cardIdStartSignatureThumbAlt1[2] = {0xB508, 0x202E};
u16 cardIdStartSignatureThumbAlt2[2] = {0xB508, 0x20B8};
u16 cardIdStartSignatureThumbAlt3[2] = {0xB510, 0x24B8};

// Card read DMA
u32 cardReadDmaStartSignature[1]       = {0xE92D4FF8};
u32 cardReadDmaStartSignatureAlt1[1]    = {0xE92D47F0};
u32 cardReadDmaStartSignatureAlt2[1]   = {0xE92D4FF0};
u16 cardReadDmaStartSignatureThumb1[1] = {0xB5F0};
u16 cardReadDmaStartSignatureThumb3[1] = {0xB5F8};

u32 cardReadDmaEndSignature[2]         = {0x01FF8000, 0x000001FF};
u16 cardReadDmaEndSignatureThumbAlt[4] = {0x8000, 0x01FF, 0x0000, 0x0200};

// Arena low
//u32 arenaLowSignature[4] = {0xE1A00100, 0xE2800627, 0xE2800AFF, 0xE5801DA0};

// Random patch
u32 randomPatch[4] = {0xE3500000, 0x1597002C, 0x10406004, 0x03E06000};

// Mpu cache
u32 mpuInitRegion0Signature[1] = {0xEE060F10};
u32 mpuInitRegion0Data[1]      = {0x4000033};
u32 mpuInitRegion1Signature[1] = {0xEE060F11};
u32 mpuInitRegion1Data1[1]     = {0x200002D};
u32 mpuInitRegion1Data4[1]     = {0x200002D}; // sdk >= 4 version
u32 mpuInitRegion1DataAlt[1]   = {0x200002B};
u32 mpuInitRegion2Signature[1] = {0xEE060F12};
u32 mpuInitRegion2Data1[1]     = {0x27C0023}; // sdk < 3 version
u32 mpuInitRegion2Data3[1]     = {0x27E0021}; // sdk >= 3 version
u32 mpuInitRegion3Signature[1] = {0xEE060F13};
u32 mpuInitRegion3Data[1]      = {0x8000035};

// Mpu cache init
u32 mpuInitCache[1] = {0xE3A00042};

//int readType = 0;

module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer) {
	dbg_printf("Looking for moduleparams\n");
	u32* moduleparams = findOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		moduleParamsSignature, 2
	);
	*(vu32*)(0x2800008) = ((u32)moduleparams - 0x8);
	if (!moduleparams) {
		dbg_printf("No moduleparams?\n");
		*(vu32*)(0x2800010) = 1;
		moduleparams = malloc(0x100);
		memset(moduleparams, 0, 0x100);
		((module_params_t*)((u32)moduleparams - 0x1C))->compressed_static_end = 0;
		switch (donorSdkVer) {
			case 0:
			default:
				break;
			case 1:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x1000500;
				break;
			case 2:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x2001000;
				break;
			case 3:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x3002001;
				break;
			case 4:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x4002001;
				break;
			case 5:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x5003001;
				break;
		}
	}
	return (module_params_t*)((u32)moduleparams - 0x1C);
}

u32* findCardReadEndOffset0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardReadEndOffset0:\n");

	u32* cardReadEndOffset = NULL;
	if (ROM_TID != 0x45524F55 && moduleParams->sdk_version < 0x4000000) {
		cardReadEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			a9cardReadSignature, 2
		);
	}
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read end (type 0) not found\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadEndOffset1(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardReadEndOffset1:\n");

	u32* cardReadEndOffset = NULL;
	if (ROM_TID == 0x45524F55) { // Start at 0x3800 for "WarioWare: DIY (USA)"
		//readType = 1;
		cardReadEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination + 0x3800, 0x00300000,//ndsHeader->arm9binarySize,
			a9cardReadSignatureAlt, 2
		);
	} else {
		//readType = 1;
		cardReadEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			a9cardReadSignatureAlt, 2
		);
	}
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end alt (type 1) found\n");
	} else {
		dbg_printf("ARM9 Card read end alt (type 1) not found\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardReadEndOffsetThumb:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		a9cardReadSignatureThumb, 4
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end thumb found\n");
	} else {
		dbg_printf("ARM9 Card read end thumb not found\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadStartOffset0(const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffset0:\n");

	//if (readType != 1) {

	u32* cardReadStartOffset = findOffsetBackwards(
		cardReadEndOffset, 0x118,
		cardReadStartSignature, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read start (type 0) not found\n");
	}

	if (cardReadStartOffset) {
		dbg_hexa((u32)cardReadStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardReadStartOffset1(const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffset1:\n");

	//if (readType == 1) {
	u32* cardReadStartOffset = findOffsetBackwards(
		cardReadEndOffset, 0x118,
		cardReadStartSignatureAlt, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start alt (type 1) found: ");
	} else {
		dbg_printf("ARM9 Card read start alt (type 1) not found\n");
	}

	if (cardReadStartOffset) {
		dbg_hexa((u32)cardReadStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u16* findCardReadStartOffsetThumb(const u16* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetThumb:\n");

	u16* cardReadStartOffset = findOffsetBackwardsThumb(
		cardReadEndOffset, 0xC0,
		cardReadStartSignatureThumb, 2
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start thumb found: ");
	} else {
		dbg_printf("ARM9 Card read start thumb not found\n");
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwardsThumb(
			cardReadEndOffset, 0xC0,
			cardReadStartSignatureThumbAlt, 2
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start thumb alt found: ");
		} else {
			dbg_printf("ARM9 Card read start thumb alt not found\n");
		}
	}

	if (cardReadStartOffset) {
		dbg_hexa((u32)cardReadStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardReadCachedEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardReadCachedEndOffset:\n");

	u32* cardReadCachedEndSignature = cardReadCachedEndSignature1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardReadCachedEndSignature = cardReadCachedEndSignature3;
	} else if (moduleParams->sdk_version > 0x4000000) {
		cardReadCachedEndSignature = cardReadCachedEndSignature4;
	}
	
	u32* cardReadCachedEndOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardReadCachedEndSignature, 4
	);
	if (cardReadCachedEndOffset) {
		dbg_printf("Card read cached end found\n");
	} else {
		dbg_printf("Card read cached end not found\n");
		//cardReadFound = false;
	}

	dbg_printf("\n");
	return cardReadCachedEndOffset;
}

u32* findCardReadCachedStartOffset(const module_params_t* moduleParams, const u32* cardReadCachedEndOffset) {
	if (!cardReadCachedEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadCachedStartOffset:\n");

	u32* cardReadCachedStartSignature = cardReadCachedStartSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		cardReadCachedStartSignature = cardReadCachedStartSignature4;
	}
	
	u32* cardReadCachedStartOffset = findOffsetBackwards(
		cardReadCachedEndOffset, 0xFF,
		cardReadCachedStartSignature, 2
	);
	if (cardReadCachedStartOffset) {
		dbg_printf("Card read cached start found: ");
	} else {
		dbg_printf("Card read cached start not found\n");
		//cardReadFound = false;
	}

	if (cardReadCachedStartOffset) {
		dbg_hexa((u32)cardReadCachedStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadCachedStartOffset;
}

u32* findCardPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardPullOutOffset:\n");

	u32* cardPullOutSignature = cardPullOutSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		cardPullOutSignature = cardPullOutSignature4;
	}

	//if (!usesThumb) {
	
	u32* cardPullOutOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardPullOutSignature, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler found: ");
	} else {
		dbg_printf("Card pull out handler not found\n");
	}

	if (cardPullOutOffset) {
		dbg_hexa((u32)cardPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardPullOutOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler thumb found: ");
	} else {
		dbg_printf("Card pull out handler thumb not found\n");
	}

	if (!cardPullOutOffset) {
		cardPullOutOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			cardPullOutSignatureThumbAlt, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler thumb alt found: ");
		} else {
			dbg_printf("Card pull out handler thumb alt not found\n");
		}
	}

	if (cardPullOutOffset) {
		dbg_hexa((u32)cardPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

/*u32* findForceToPowerOffOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findForceToPowerOffOffset:\n");

	u32 forceToPowerOffOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		forceToPowerOffSignature, 4
	);
	if (forceToPowerOffOffset) {
		dbg_printf("Force to power off handler: ");
	} else {
		dbg_printf("Force to power off handler not found\n");
	}

	if (forceToPowerOffOffset) {
		dbg_hexa((u32)forceToPowerOffOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return forceToPowerOffOffset;
}*/

u32* findCardIdEndOffset(const tNDSHeader* ndsHeader, const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdEndOffset:\n");

	u32* cardIdEndOffset = findOffset(
		(u32*)cardReadEndOffset + 0x10, ndsHeader->arm9binarySize,
		a9cardIdSignature, 2
	);

	//if (!usesThumb) {
	
	if (!cardIdEndOffset) {
		cardIdEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
			a9cardIdSignature, 2
		);
	}

	if (cardIdEndOffset) {
		dbg_printf("Card ID end found\n");
	} else {
		dbg_printf("Card ID end not found\n");
	}

	dbg_printf("\n");
	return cardIdEndOffset;
}

u16* findCardIdEndOffsetThumb(const tNDSHeader* ndsHeader, const u16* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdEndOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardIdEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		a9cardIdSignatureThumb, 6
	);
	if (cardIdEndOffset) {
		dbg_printf("Card ID end thumb found\n");
	} else {
		dbg_printf("Card ID end thumb not found\n");
	}

	dbg_printf("\n");
	return cardIdEndOffset;
}

u32* findCardIdStartOffset(const u32* cardIdEndOffset) {
	if (!cardIdEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdStartOffset:\n");

	u32* cardIdStartOffset = findOffsetBackwards(
		(u32*)cardIdEndOffset, 0x100,
		cardIdStartSignature, 1
	);
	if (cardIdStartOffset) {
		dbg_printf("Card ID start found: ");
	} else {
		dbg_printf("Card ID start not found\n");
	}

	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignatureAlt1, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start alt 1 not found\n");
		} else {
			dbg_printf("Card ID start alt 1 found: ");
		}
	}
	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignatureAlt2, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start alt 2 not found\n");
		} else {
			dbg_printf("Card ID start alt 2 found: ");
		}
	}

	if (cardIdStartOffset) {
		dbg_hexa((u32)cardIdStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdStartOffset;
}

u16* findCardIdStartOffsetThumb(const u16* cardIdEndOffset) {
	if (!cardIdEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdStartOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardIdStartOffset = findOffsetBackwardsThumb(
		(u16*)cardIdEndOffset, 0x40,
		cardIdStartSignatureThumb, 2
	);
	if (cardIdStartOffset) {
		dbg_printf("Card ID start thumb found: ");
	} else {
		dbg_printf("Card ID start thumb not found\n");
	}

	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwardsThumb(
			(u16*)cardIdEndOffset, 0x40,
			cardIdStartSignatureThumbAlt1, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start thumb alt 1 found: ");
		} else {
			dbg_printf("Card ID start thumb alt 1 not found\n");
		}
	}
	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwardsThumb(
			(u16*)cardIdEndOffset, 0x40,
			cardIdStartSignatureThumbAlt2, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start thumb alt 2 found: ");
		} else {
			dbg_printf("Card ID start thumb alt 2 not found\n");
		}
	}
	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwardsThumb(
			(u16*)cardIdEndOffset, 0x40,
			cardIdStartSignatureThumbAlt3, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start thumb alt 3 found: ");
		} else {
			dbg_printf("Card ID start thumb alt 3 not found\n");
		}
	}

	if (cardIdStartOffset) {
		dbg_hexa((u32)cardIdStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdStartOffset;
}

u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardReadDmaEndOffset:\n");

	u32* cardReadDmaEndOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardReadDmaEndSignature, 2
	);
	if (cardReadDmaEndOffset) {
		dbg_printf("Card read DMA end found: ");
	} else {
		dbg_printf("Card read DMA end not found\n");
	}

	if (cardReadDmaEndOffset) {
		dbg_hexa((u32)cardReadDmaEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaEndOffset;
}

u16* findCardReadDmaEndOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardReadDmaEndOffsetThumb:\n");

	//if (usesThumb) {

	u16* cardReadDmaEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardReadDmaEndSignatureThumbAlt, 4
	);
	if (cardReadDmaEndOffset) {
		dbg_printf("Card read DMA end thumb alt found: ");
	} else {
		dbg_printf("Card read DMA end thumb alt not found\n");
	}

	if (cardReadDmaEndOffset) {
		dbg_hexa((u32)cardReadDmaEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaEndOffset;
}

u32* findCardReadDmaStartOffset(const u32* cardReadDmaEndOffset) {
	if (!cardReadDmaEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadDmaStartOffset:\n");

	//if (!usesThumb) {

	u32* cardReadDmaStartOffset = findOffsetBackwards(
		(u32*)cardReadDmaEndOffset, 0x200,
		cardReadDmaStartSignature, 1
	);
	if (cardReadDmaStartOffset) {
		dbg_printf("Card read DMA start found: ");
	} else {
		dbg_printf("Card read DMA start not found\n");
	}

	if (!cardReadDmaStartOffset) {
		cardReadDmaStartOffset = findOffsetBackwards(
			(u32*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignatureAlt1, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start alt 1 found: ");
		} else {
			dbg_printf("Card read DMA start alt 1 not found\n");
		}
	}
	if (!cardReadDmaStartOffset) {
		cardReadDmaStartOffset = findOffsetBackwards(
			(u32*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignatureAlt2, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start alt 2 found: ");
		} else {
			dbg_printf("Card read DMA start alt 2 not found\n");
		}
	}

	if (cardReadDmaStartOffset) {
		dbg_hexa((u32)cardReadDmaStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaStartOffset;
}

u16* findCardReadDmaStartOffsetThumb(const u16* cardReadDmaEndOffset) {
	if (!cardReadDmaEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadDmaStartOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardReadDmaStartOffset = findOffsetBackwardsThumb(
		(u16*)cardReadDmaEndOffset, 0x100,
		cardReadDmaStartSignatureThumb1, 1
	);
	if (cardReadDmaStartOffset) {
		dbg_printf("Card read DMA start thumb 1 found: ");
	} else {
		dbg_printf("Card read DMA start thumb 1 not found\n");
	}

	if (!cardReadDmaStartOffset) {
		cardReadDmaStartOffset = findOffsetBackwardsThumb(
			(u16*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignatureThumb3, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start thumb 3 found: ");
		} else {
			dbg_printf("Card read DMA start thumb 3 not found\n");
		}
	}

	if (cardReadDmaStartOffset) {
		dbg_hexa((u32)cardReadDmaStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaStartOffset;
}

u32* getMpuInitRegionSignature(u32 patchMpuRegion) {
	switch (patchMpuRegion) {
		case 0: return mpuInitRegion0Signature;
		case 1: return mpuInitRegion1Signature;
		case 2: return mpuInitRegion2Signature;
		case 3: return mpuInitRegion3Signature;
	}
	return mpuInitRegion1Signature;
}

u32* findMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion) {
	dbg_printf("findMpuStartOffset:\n");

	u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);

	u32* mpuStartOffset = findOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		mpuInitRegionSignature, 1
	);
	if (mpuStartOffset) {
		dbg_printf("Mpu init found: ");
	} else {
		dbg_printf("Mpu init not found\n");
	}

	if (mpuStartOffset) {
		dbg_hexa((u32)mpuStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return mpuStartOffset;
}

u32* findMpuDataOffset(const module_params_t* moduleParams, u32 patchMpuRegion, const u32* mpuStartOffset) {
	if (!mpuStartOffset) {
		return NULL;
	}

	dbg_printf("findMpuDataOffset:\n");

	u32* mpuInitRegion1Data = mpuInitRegion1Data1;
	u32* mpuInitRegion2Data = mpuInitRegion2Data1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		mpuInitRegion2Data = mpuInitRegion2Data3;
	} else if (moduleParams->sdk_version > 0x4000000) {
		mpuInitRegion1Data = mpuInitRegion1Data4;
	}

	u32* mpuInitRegionData = mpuInitRegion1Data;
	switch (patchMpuRegion) {
		case 0:
			mpuInitRegionData = mpuInitRegion0Data;
			break;
		case 1:
			mpuInitRegionData = mpuInitRegion1Data;
			break;
		case 2:
			mpuInitRegionData = mpuInitRegion2Data;
			break;
		case 3:
			mpuInitRegionData = mpuInitRegion3Data;
			break;
	}
	
	u32* mpuDataOffset = findOffset(
		mpuStartOffset, 0x100,
		mpuInitRegionData, 1
	);
	if (!mpuDataOffset) {
		// Try to find it
		for (int i = 0; i < 0x100; i++) {
			mpuDataOffset += i;
			if ((*mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
				break;
			}
		}
	}
	if (mpuDataOffset) {
		dbg_printf("Mpu data found: ");
	} else {
		dbg_printf("Mpu data not found\n");
	}

	if (mpuDataOffset) {
		dbg_hexa((u32)mpuDataOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return mpuDataOffset;
}

u32* findMpuInitCacheOffset(const u32* mpuStartOffset) {
	dbg_printf("findMpuInitCacheOffset:\n");

	u32* mpuInitCacheOffset = findOffset(
		mpuStartOffset, 0x100,
		mpuInitCache, 1
	);
	if (mpuInitCacheOffset) {
		dbg_printf("Mpu init cache found: ");
	} else {
		dbg_printf("Mpu init cache not found\n");
	}

	if (mpuInitCacheOffset) {
		dbg_hexa((u32)mpuInitCacheOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return mpuInitCacheOffset;
}

/*u32* findArenaLowOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findArenaLowOffset:\n");

	u32* arenaLowOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		arenaLowSignature, 4
	);
	if (arenaLowOffset) {
		dbg_printf("Arena low found: ");
	} else {
		dbg_printf("Arena low not found\n");
	}

	if (arenaLowOffset) {
		dbg_hexa((u32)arenaLowOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return arenaLowOffset;
}*/

u32* findRandomPatchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRandomPatchOffset:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		randomPatch, 4
	);
	if (!randomPatchOffset) {
		dbg_printf("Random patch found: ");
	} else {
		dbg_printf("Random patch not found\n");
	}

	if (randomPatchOffset) {
		dbg_hexa((u32)randomPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return randomPatchOffset;
}
