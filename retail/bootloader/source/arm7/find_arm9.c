#include <stddef.h> // NULL
#include "nds_header.h"
#include "find.h"
#include "debug_file.h"

//#define memset __builtin_memset

//
// Subroutine function signatures ARM9
//

// Module params
static const u32 moduleParamsSignature[2] = {0xDEC00621, 0x2106C0DE};

// Card read
static const u32 cardReadEndSignature[2]            = {0x04100010, 0x040001A4}; // SDK < 4
static const u32 cardReadEndSignatureAlt[2]         = {0x040001A4, 0x04100010};
static const u16 cardReadEndSignatureThumb[4]       = {0x01A4, 0x0400, 0x0200, 0x0000};
static const u16 cardReadEndSignatureThumb5[8]      = {0xFE00, 0xFFFF, 0xE120, 0x0213, 0x01A4, 0x0400, 0x0010, 0x0410}; // SDK 5
static const u16 cardReadEndSignatureThumb5Alt1[8]  = {0xFE00, 0xFFFF, 0x9940, 0x0214, 0x01A4, 0x0400, 0x0010, 0x0410}; // SDK 5
static const u16 cardReadEndSignatureThumb5Alt2[4]  = {0x01A4, 0x0400, 0xFE00, 0xFFFF};                                 // SDK 5
static const u32 cardReadStartSignature[1]          = {0xE92D4FF0};
static const u32 cardReadStartSignatureAlt[1]       = {0xE92D4070};
static const u32 cardReadStartSignature5[1]         = {0xE92D4FF8};                                                     // SDK 5
static const u16 cardReadStartSignatureThumb[2]     = {0xB5F8, 0xB082};
static const u16 cardReadStartSignatureThumbAlt[2]  = {0xB5F0, 0xB083};
static const u16 cardReadStartSignatureThumb5[1]    = {0xB5F0};                                                         // SDK 5
static const u16 cardReadStartSignatureThumb5Alt[1] = {0xB5F8};                                                         // SDK 5

// Card read cached
static const u32 cardReadCachedEndSignature1[4]   = {0xE5950020, 0xE3500000, 0x13A00001, 0x03A00000}; // SDK <= 2
static const u32 cardReadCachedStartSignature1[2] = {0xE92D4030, 0xE24DD004};
static const u32 cardReadCachedEndSignature3[4]   = {0xE5950024, 0xE3500000, 0x13A00001, 0x03A00000}; // SDK 3
//               cardReadCachedStartSignature3
static const u32 cardReadCachedEndSignature4[4]   = {0xE5940024, 0xE3500000, 0x13A00001, 0x03A00000}; // SDK >= 4
static const u32 cardReadCachedStartSignature4[2] = {0xE92D4038, 0xE59F407C}; // SDK >= 4
  
//static const u32 instructionBHI[1] = {0x8A000001};

// Card pull out
static const u32 cardPullOutSignature1[5]         = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011, 0x1A00000D}; // SDK <= 3
static const u32 cardPullOutSignature4[4]         = {0xE92D4008, 0xE201003F, 0xE3500011, 0x1A00000D}; // SDK >= 4
static const u32 cardPullOutSignature5[4]         = {0xE92D4010, 0xE201003F, 0xE3500011, 0x1A000012}; // SDK 5
static const u32 cardPullOutSignature5Alt[4]      = {0xE92D4038, 0xE201003F, 0xE3500011, 0x1A000011}; // SDK 5
static const u16 cardPullOutSignatureThumb[4]     = {0xB508, 0x203F, 0x4008, 0x2811};
static const u16 cardPullOutSignatureThumbAlt[4]  = {0xB500, 0xB081, 0x203F, 0x4001};
static const u16 cardPullOutSignatureThumb5[4]    = {0xB510, 0x203F, 0x4008, 0x2811};                 // SDK 5
static const u16 cardPullOutSignatureThumb5Alt[4] = {0xB538, 0x203F, 0x4008, 0x2811};                 // SDK 5

//static const u32 cardSendSignature[7] = {0xE92D40F0, 0xE24DD004, 0xE1A07000, 0xE1A06001, 0xE1A01007, 0xE3A0000E, 0xE3A02000};

// Force to power off
//static const u32 forceToPowerOffSignature[4] = {0xE92D4000, 0xE24DD004, 0xE59F0028, 0xE28D1000};

// Card id
static const u32 cardIdEndSignature[2]            = {0x040001A4, 0x04100010};
static const u32 cardIdEndSignature5[4]           = {0xE8BD8010, 0x02FFFAE0, 0x040001A4, 0x04100010}; // SDK 5
static const u32 cardIdEndSignature5Alt[3]        = {0x02FFFAE0, 0x040001A4, 0x04100010};             // SDK 5
static const u16 cardIdEndSignatureThumb[6]       = {0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410};
static const u16 cardIdEndSignatureThumb5[8]      = {0xFAE0, 0x02FF, 0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410}; // SDK 5
static const u32 cardIdStartSignature[1]          = {0xE92D4000};
static const u32 cardIdStartSignatureAlt1[1]      = {0xE92D4008};
static const u32 cardIdStartSignatureAlt2[1]      = {0xE92D4010};
static const u32 cardIdStartSignature5[2]         = {0xE92D4010, 0xE3A050B8}; // SDK 5
static const u32 cardIdStartSignature5Alt[1]      = {0xE92D4038};             // SDK 5
static const u16 cardIdStartSignatureThumb[2]     = {0xB500, 0xB081};
static const u16 cardIdStartSignatureThumbAlt1[2] = {0xB508, 0x202E};
static const u16 cardIdStartSignatureThumbAlt2[2] = {0xB508, 0x20B8};
static const u16 cardIdStartSignatureThumbAlt3[2] = {0xB510, 0x24B8};

// Card read DMA
static const u32 cardReadDmaEndSignature[2]         = {0x01FF8000, 0x000001FF};
static const u16 cardReadDmaEndSignatureThumbAlt[4] = {0x8000, 0x01FF, 0x0000, 0x0200};
static const u32 cardReadDmaStartSignature[1]       = {0xE92D4FF8};
static const u32 cardReadDmaStartSignatureAlt1[1]   = {0xE92D47F0};
static const u32 cardReadDmaStartSignatureAlt2[1]   = {0xE92D4FF0};
static const u32 cardReadDmaStartSignature5[1]      = {0xE92D43F8}; // SDK 5
static const u16 cardReadDmaStartSignatureThumb1[1] = {0xB5F0}; // SDK <= 2
static const u16 cardReadDmaStartSignatureThumb3[1] = {0xB5F8}; // SDK >= 3

// Random patch
static const u32 randomPatchSignature[4]        = {0xE3500000, 0x1597002C, 0x10406004, 0x03E06000};
static const u32 randomPatchSignature5First[4]  = {0xE92D43F8, 0xE3A04000, 0xE1A09001, 0xE1A08002}; // SDK 5
static const u32 randomPatchSignature5Second[3] = {0xE59F003C, 0xE590001C, 0xE3500000};             // SDK 5

// Mpu cache
static const u32 mpuInitRegion0Signature[1] = {0xEE060F10};
static const u32 mpuInitRegion0Data[1]      = {0x4000033};
static const u32 mpuInitRegion1Signature[1] = {0xEE060F11};
static const u32 mpuInitRegion1Data1[1]     = {0x200002D}; // SDK <= 3
static const u32 mpuInitRegion1Data4[1]     = {0x200002D}; // SDK >= 4
//static const u32 mpuInitRegion1DataAlt[1]   = {0x200002B};
static const u32 mpuInitRegion2Signature[1] = {0xEE060F12};
static const u32 mpuInitRegion2Data1[1]     = {0x27C0023}; // SDK != 3 (Previously: SDK <= 2)
static const u32 mpuInitRegion2Data3[1]     = {0x27E0021}; // SDK 3 (Previously: SDK >= 3)
static const u32 mpuInitRegion3Signature[1] = {0xEE060F13};
static const u32 mpuInitRegion3Data[1]      = {0x8000035};

// Mpu cache init
static const u32 mpuInitCache[1] = {0xE3A00042};

static const u32 operaRamSignature[2]        = {0x097FFFFE, 0x09000000};

   
 
// Init Heap
static const initHeapStartSignature2[3]        = {0xE3500006, 0x908FF100, 0xEA000012};
static const initHeapStartSignature2alt[3]        = {0xE3500006, 0xE3500006, 0xEA000012};
static const initHeapStartSignature3[3]        = {0xE92D4000, 0xE24DD004, 0xE3500006};
static const initHeapStartSignature4[3]        = {0xE92D4008, 0x908FF100, 0x908FF100};
static const u32 initHeapStartSignature4Alt[4]     = {0xE92D4008, 0xE3500006, 0x908FF100, 0xEA00001C};
static const u16 initHeapStartSignatureThumb4Alt[3]     = {0xB508, 0x2806, 0xD824};
static const initHeapEndSignature[2]        = {0x27FF000, 0x37F8000};


u32* findModuleParamsOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findModuleParamsOffset:\n");

	u32* moduleParamsOffset = findOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		moduleParamsSignature, 2
	);
	if (moduleParamsOffset) {
		dbg_printf("Module params offset found: ");
	} else {
		dbg_printf("Module params offset not found\n");
	}

	if (moduleParamsOffset) {
		dbg_hexa((u32)moduleParamsOffset);
		dbg_printf("\n");
	}

	return moduleParamsOffset;
}

u32* findCardReadEndOffsetType0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardReadEndOffsetType0:\n");

	const char* romTid = getRomTid(ndsHeader);

	u32* cardReadEndOffset = NULL;
	if (strncmp(romTid, "UOR", 3) != 0 && (moduleParams->sdk_version < 0x4000000 || moduleParams->sdk_version > 0x5000000)) {
		cardReadEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			cardReadEndSignature, 2
		);
	}
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read end (type 0) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadEndOffsetType1(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardReadEndOffsetType1:\n");

	const char* romTid = getRomTid(ndsHeader);

	u32* cardReadEndOffset = NULL;
	if (strncmp(romTid, "UOR", 3) == 0) { // Start at 0x3800 for "WarioWare: DIY"
		//readType = 1;
		cardReadEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination + 0x3800, 0x00300000,//ndsHeader->arm9binarySize,
			cardReadEndSignatureAlt, 2
		);
	} else {
		//readType = 1;
		cardReadEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			cardReadEndSignatureAlt, 2
		);
	}
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end alt (type 1) found: ");
	} else {
		dbg_printf("ARM9 Card read end alt (type 1) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardReadEndOffsetThumb:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb, 4
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end thumb found: ");
	} else {
		dbg_printf("ARM9 Card read end thumb not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

// SDK 5
u16* findCardReadEndOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findCardReadEndOffsetThumb5Type0:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb5, 8
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end SDK 5 thumb (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read end SDK 5 thumb (type 0) not found\n");
	}

    if (!cardReadEndOffset) {
        cardReadEndOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			cardReadEndSignatureThumb5Alt1, 8
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end SDK 5 thumb alt 1 (type 0) found: ");
		} else {
			dbg_printf("ARM9 Card read end SDK 5 thumb alt 1 (type 0) not found\n");
		}
    }

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

// SDK 5
u16* findCardReadEndOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	dbg_printf("findCardReadEndOffsetThumb5Type1:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb5Alt2, 4
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end SDK 5 thumb alt 2 (type 1) found: ");
	} else {
		dbg_printf("ARM9 Card read end SDK 5 thumb alt 2 (type 1) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadStartOffsetType0(const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetType0:\n");

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

u32* findCardReadStartOffsetType1(const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetType1:\n");

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

// SDK 5
u32* findCardReadStartOffset5(const module_params_t* moduleParams, const u32* cardReadEndOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffset5:\n");

	u32* cardReadStartOffset = findOffsetBackwards(
		(u32*)cardReadEndOffset, 0x120,
		cardReadStartSignature5, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start SDK 5 found: ");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 not found\n");
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

// SDK 5
u16* findCardReadStartOffsetThumb5Type0(const module_params_t* moduleParams, const u16* cardReadEndOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetThumb5Type0:\n");

	u16* cardReadStartOffset = findOffsetBackwardsThumb(
		cardReadEndOffset, 0xD0,
		cardReadStartSignatureThumb5, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start SDK 5 thumb (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 thumb (type 0) not found\n");
	}

	if (cardReadStartOffset) {
		dbg_hexa((u32)cardReadStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

// SDK 5
u16* findCardReadStartOffsetThumb5Type1(const module_params_t* moduleParams, const u16* cardReadEndOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetThumb5Type1:\n");

	u16* cardReadStartOffset = findOffsetBackwardsThumb(
		(u16*)cardReadEndOffset, 0xD0,
		cardReadStartSignatureThumb5Alt, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start SDK 5 thumb alt (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 thumb alt (type 0) not found\n");
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

	const u32* cardReadCachedEndSignature = cardReadCachedEndSignature1;
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
		dbg_printf("Card read cached end found: ");
	} else {
		dbg_printf("Card read cached end not found\n");
		//cardReadFound = false;
	}

	if (cardReadCachedEndOffset) {
		dbg_hexa((u32)cardReadCachedEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadCachedEndOffset;
}

u32* findCardReadCachedStartOffset(const module_params_t* moduleParams, const u32* cardReadCachedEndOffset) {
	if (!cardReadCachedEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadCachedStartOffset:\n");

	const u32* cardReadCachedStartSignature = cardReadCachedStartSignature1;
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

	//if (!usesThumb) {
	
	u32* cardPullOutOffset = 0;
	if (moduleParams->sdk_version > 0x5000000) {
		// SDK 5
		cardPullOutOffset = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			cardPullOutSignature5, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler SDK 5 found: ");
		} else {
			dbg_printf("Card pull out handler SDK 5 not found\n");
		}

		if (!cardPullOutOffset) {
			// SDK 5
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				cardPullOutSignature5Alt, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 5 alt found: ");
			} else {
				dbg_printf("Card pull out handler SDK 5 alt not found\n");
			}
		}
	} else {
		if (moduleParams->sdk_version < 0x3000000) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				cardPullOutSignature1, 5
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler found: ");
			} else {
				dbg_printf("Card pull out handler not found\n");
			}
		}

		if (!cardPullOutOffset && moduleParams->sdk_version < 0x4000000) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				cardPullOutSignature1, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler alt found: ");
			} else {
				dbg_printf("Card pull out handler alt not found\n");
			}
		}

		if (!cardPullOutOffset) {
			// SDK 4
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				cardPullOutSignature4, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 4 found: ");
			} else {
				dbg_printf("Card pull out handler SDK 4 not found\n");
			}
		}
	}

	if (cardPullOutOffset) {
		dbg_hexa((u32)cardPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader) {
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

// SDK 5
u16* findCardPullOutOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findCardPullOutOffsetThumbType0:\n");
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//, ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb5, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler SDK 5 thumb (type 0) found: ");
	} else {
		dbg_printf("Card pull out handler SDK 5 thumb (type 0) not found\n");
	}

	if (cardPullOutOffset) {
		dbg_hexa((u32)cardPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

// SDK 5
u16* findCardPullOutOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	dbg_printf("findCardPullOutOffsetThumbType1:\n");
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, 0x00300000,//, ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb5Alt, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler SDK 5 thumb alt (type 1) found: ");
	} else {
		dbg_printf("Card pull out handler SDK 5 thumb alt (type 1) not found\n");
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
		dbg_printf("Force to power off handler found: ");
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

u32* findCardIdEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdEndOffset:\n");

	u32* cardIdEndOffset = NULL;

	if (moduleParams->sdk_version > 0x5000000) {
		// SDK 5
		cardIdEndOffset = findOffsetBackwards(
			(u32*)cardReadEndOffset, 0x800,
			cardIdEndSignature5, 4
		);
		if (cardIdEndOffset) {
			dbg_printf("Card ID end SDK 5 found: ");
		} else {
			dbg_printf("Card ID end SDK 5 not found\n");
		}

		if (!cardIdEndOffset) {
			// SDK 5
			cardIdEndOffset = findOffset(
				(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
				cardIdEndSignature5Alt, 3
			);
			if (cardIdEndOffset) {
				dbg_printf("Card ID end SDK 5 alt found: ");
			} else {
				dbg_printf("Card ID end SDK 5 alt not found\n");
			}
		}
	} else {
		cardIdEndOffset = findOffset(
			(u32*)cardReadEndOffset + 0x10, ndsHeader->arm9binarySize,
			cardIdEndSignature, 2
		); //if (!usesThumb) {
		if (!cardIdEndOffset) {
			cardIdEndOffset = findOffset(
				(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
				cardIdEndSignature, 2
			);
		}
		if (cardIdEndOffset) {
			dbg_printf("Card ID end found: ");
		} else {
			dbg_printf("Card ID end not found\n");
		}
	}

	if (cardIdEndOffset) {
		dbg_hexa((u32)cardIdEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdEndOffset;
}

u16* findCardIdEndOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u16* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdEndOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardIdEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		cardIdEndSignatureThumb, 6
	);
	if (cardIdEndOffset) {
		dbg_printf("Card ID end thumb found: ");
	} else {
		dbg_printf("Card ID end thumb not found\n");
	}

	if (!cardIdEndOffset) {
		// SDK 5
		if (moduleParams->sdk_version > 0x5000000) {
			cardIdEndOffset = findOffsetThumb(
				(u16*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
				cardIdEndSignatureThumb5, 8
			);
			if (cardIdEndOffset) {
				dbg_printf("Card ID end SDK 5 thumb found: ");
			} else {
				dbg_printf("Card ID end SDK 5 thumb not found\n");
			}
		}
	}

	if (cardIdEndOffset) {
		dbg_hexa((u32)cardIdEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdEndOffset;
}

u32* findCardIdStartOffset(const module_params_t* moduleParams, const u32* cardIdEndOffset) {
	if (!cardIdEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdStartOffset:\n");

	u32* cardIdStartOffset = NULL;

	if (moduleParams->sdk_version > 0x5000000) {
		// SDK 5
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignature5, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start SDK 5 found: ");
		} else {
			dbg_printf("Card ID start SDK 5 not found\n");
		}

		if (!cardIdStartOffset) {
			// SDK 5
			if (moduleParams->sdk_version > 0x5000000) {
				cardIdStartOffset = findOffsetBackwards(
					(u32*)cardIdEndOffset, 0x100,
					cardIdStartSignature5Alt, 1
				);
				if (cardIdStartOffset) {
					dbg_printf("Card ID start SDK 5 alt 1 found: ");
				} else {
					dbg_printf("Card ID start SDK 5 alt 1 not found\n");
				}
			}
		}
	} else {
		cardIdStartOffset = findOffsetBackwards(
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
				dbg_printf("Card ID start alt 1 found: ");
			} else {
				dbg_printf("Card ID start alt 1 not found\n");
			}
		}
	}

	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignatureAlt2, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start alt 2 found: ");
		} else {
			dbg_printf("Card ID start alt 2 not found\n");
		}
	}

	if (cardIdStartOffset) {
		dbg_hexa((u32)cardIdStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdStartOffset;
}

u16* findCardIdStartOffsetThumb(const module_params_t* moduleParams, const u16* cardIdEndOffset) {
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

u32* findCardReadDmaStartOffset(const module_params_t* moduleParams, const u32* cardReadDmaEndOffset) {
	if (!cardReadDmaEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadDmaStartOffset:\n");

	//if (!usesThumb) {

	u32* cardReadDmaStartOffset = NULL;

	if (moduleParams->sdk_version > 0x5000000) {
		cardReadDmaStartOffset = findOffsetBackwards(
			(u32*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignature5, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start SDK 5 found: ");
		} else {
			dbg_printf("Card read DMA start SDK 5 not found\n");
		}
	} else {
		cardReadDmaStartOffset = findOffsetBackwards(
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
		dbg_printf("Card read DMA start thumb SDK < 3 found: ");
	} else {
		dbg_printf("Card read DMA start thumb SDK < 3 not found\n");
	}

	if (!cardReadDmaStartOffset) {
		cardReadDmaStartOffset = findOffsetBackwardsThumb(
			(u16*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignatureThumb3, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start thumb SDK >= 3 found: ");
		} else {
			dbg_printf("Card read DMA start thumb SDK >= 3 not found\n");
		}
	}

	if (cardReadDmaStartOffset) {
		dbg_hexa((u32)cardReadDmaStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaStartOffset;
}

const u32* getMpuInitRegionSignature(u32 patchMpuRegion) {
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

	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);

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

	const u32* mpuInitRegion1Data = mpuInitRegion1Data1;
	const u32* mpuInitRegion2Data = mpuInitRegion2Data1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		mpuInitRegion2Data = mpuInitRegion2Data3;
	} else if (moduleParams->sdk_version > 0x4000000) {
		mpuInitRegion1Data = mpuInitRegion1Data4;
	}

	const u32* mpuInitRegionData = mpuInitRegion1Data;
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

u32* findHeapPointerOffset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	dbg_printf("findHeapPointerOffset:\n");
    
    const u32* startSig = initHeapStartSignature4;
    const u16* startSigThumb = initHeapStartSignatureThumb4Alt;
    if (moduleParams->sdk_version > 0x2010000 && moduleParams->sdk_version < 0x4000000) {
        startSig = initHeapStartSignature3;
    } else if (moduleParams->sdk_version < 0x2010000) {
        startSig = initHeapStartSignature2;
    }

	u32* initHeapStart = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,
		startSig, 3
	);
	if (initHeapStart) {
		dbg_printf("Init Heap Start found: ");
	} else {
		dbg_printf("Init Heap Start not found\n");
        if (moduleParams->sdk_version < 0x4000000) return 0;
	}
	if (moduleParams->sdk_version > 0x4000000) {
		if (!initHeapStart) {
		startSig = initHeapStartSignature4Alt;
		initHeapStart = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,
			startSig, 4
		);
		if (initHeapStart) {
			dbg_printf("Init Heap Start alt found: ");
		} else {
			dbg_printf("Init Heap Start alt not found\n");
		}
		}

		if (!initHeapStart) {
		initHeapStart = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, 0x00300000,
			startSigThumb, 3
		);
		if (initHeapStart) {
			dbg_printf("Init Heap Start thumb alt found: ");
		} else {
			dbg_printf("Init Heap Start thumb alt not found\n");
			return 0;
		}
		}
	}

    dbg_hexa((u32)initHeapStart);
	dbg_printf("\n");
	
    
    u32* initHeapEnd = findOffset(
        initHeapStart, 0x200,
		initHeapEndSignature, 2
	);
    if (initHeapEnd) {
		dbg_printf("Init Heap End found: ");
	} else {
		dbg_printf("Init Heap End not found\n\n");
        return 0;
	}
    
    dbg_hexa((u32)initHeapEnd);
	dbg_printf("\n");
    dbg_printf("heapPointer: ");
    u32* heapPointer = initHeapEnd-5;
    if (moduleParams->sdk_version < 0x2010000) {
        heapPointer = initHeapEnd-3;
    }
        
    dbg_hexa((u32)heapPointer);
	dbg_printf("\n");
    
	return heapPointer;
}

u32* findRandomPatchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRandomPatchOffset:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		randomPatchSignature, 4
	);
	if (randomPatchOffset) {
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

// SDK 5
u32* findRandomPatchOffset5First(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findRandomPatchOffset5First:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		randomPatchSignature5First, 4
	);
	if (randomPatchOffset) {
		dbg_printf("Random patch SDK 5 first found: ");
	} else {
		dbg_printf("Random patch SDK 5 first not found\n");
	}

	if (randomPatchOffset) {
		dbg_hexa((u32)randomPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return randomPatchOffset;
}

// SDK 5
u32* findRandomPatchOffset5Second(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findRandomPatchOffset5Second:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
        randomPatchSignature5Second, 3
	);
	if (randomPatchOffset) {
		dbg_printf("Random patch SDK 5 second found: ");
	} else {
		dbg_printf("Random patch SDK 5 second not found\n");
	}

	if (randomPatchOffset) {
		dbg_hexa((u32)randomPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return randomPatchOffset;
}

u32* findOperaRamOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version > 0x5000000) {
		return NULL;
	}

	dbg_printf("findOperaRamOffset:\n");

	u32* operaRamOffset = findOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
        operaRamSignature, 2
	);
	if (operaRamOffset) {
		dbg_printf("Opera RAM found: ");
	} else {
		dbg_printf("Opera RAM not found\n");
	}

	if (operaRamOffset) {
		dbg_hexa((u32)operaRamOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return operaRamOffset;
}
