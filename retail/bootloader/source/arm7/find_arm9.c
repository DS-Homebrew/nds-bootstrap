#include <stddef.h> // NULL
#include "patch.h"
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
static const u32 cardReadEndSignature3Elab[3]       = {0x04100010, 0x040001A4, 0xE92D4FF0}; // SDK 3
static const u32 cardReadEndSignatureAlt[2]         = {0x040001A4, 0x04100010};
static const u32 cardReadEndSignatureSdk2Alt[3]     = {0x040001A4, 0x04100010, 0xE92D000F}; // SDK 2
static const u32 cardReadEndSignatureAlt2[3]        = {0x040001A4, 0x040001A1, 0x04100010};
static const u16 cardReadEndSignatureThumb[4]       = {0x01A4, 0x0400, 0x0200, 0x0000};
static const u16 cardReadEndSignatureThumb5[4]      = {0x01A4, 0x0400, 0xFE00, 0xFFFF};                                 // SDK 5
static const u16 cardReadEndSignatureThumb5Alt1[5]  = {0x01A4, 0x0400, 0x0010, 0x0410, 0xB510};                         // SDK 5
static const u32 cardReadStartSignature[1]          = {0xE92D4FF0};
static const u32 cardReadStartSignatureAlt[1]       = {0xE92D47F0};
static const u32 cardReadStartSignatureAlt2[1]      = {0xE92D4070};
static const u32 cardReadStartSignature5[1]         = {0xE92D4FF8};                                                     // SDK 5
static const u32 cardReadStartSignature5Alt[4]      = {0xE92D4010};													// SDK 5.5
static const u32 cardReadCheckSignatureMvDK4[3]     = {0xE5C02289, 0xE5C02288, 0xE5D0028A};
static const u16 cardReadStartSignatureThumb[2]     = {0xB5F8, 0xB082};
static const u16 cardReadStartSignatureThumbAlt[2]  = {0xB5F0, 0xB083};
static const u16 cardReadStartSignatureThumb5[1]    = {0xB5F0};                                                         // SDK 5
static const u16 cardReadStartSignatureThumb5Alt[1] = {0xB5F8};                                                         // SDK 5

//static const u32 instructionBHI[1] = {0x8A000001};

// Card pull out
static const u32 cardPullOutSignature1[4]         = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011}; // SDK <= 3
static const u32 cardPullOutSignature1Elab[5]     = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011, 0x1A00000F}; // SDK 2
static const u32 cardPullOutSignature2Alt[4]      = {0xE92D000F, 0xE92D4030, 0xE24DD004, 0xE59D0014}; // SDK 2
static const u32 cardPullOutSignature4[4]         = {0xE92D4008, 0xE201003F, 0xE3500011, 0x1A00000D}; // SDK >= 4
static const u32 cardPullOutSignature5[4]         = {0xE92D4010, 0xE201003F, 0xE3500011, 0x1A000012}; // SDK 5
static const u32 cardPullOutSignature5Alt[4]      = {0xE92D4038, 0xE201003F, 0xE3500011, 0x1A000011}; // SDK 5
static const u16 cardPullOutSignatureThumb[5]     = {0xB508, 0x203F, 0x4008, 0x2811, 0xD10E};
static const u16 cardPullOutSignatureThumbAlt[4]  = {0xB500, 0xB081, 0x203F, 0x4001};
static const u16 cardPullOutSignatureThumbAlt2[4] = {0xB5F8, 0x203F, 0x4008, 0x2811};
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
static const u16 cardIdEndSignatureThumbAlt[6]    = {0xFFFF, 0xF8FF, 0x0000, 0xA700, 0xE000, 0xFFFF};
static const u16 cardIdEndSignatureThumb5[8]      = {0xFAE0, 0x02FF, 0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410}; // SDK 5
static const u32 cardIdStartSignature[1]          = {0xE92D4000};
static const u32 cardIdStartSignatureAlt1[1]      = {0xE92D4008};
static const u32 cardIdStartSignatureAlt2[1]      = {0xE92D4010};
static const u32 cardIdStartSignature5[2]         = {0xE92D4010, 0xE3A050B8}; // SDK 5
static const u32 cardIdStartSignature5Alt[2]      = {0xE92D4038, 0xE3A050B8}; // SDK 5
static const u16 cardIdStartSignatureThumb[2]     = {0xB500, 0xB081};
static const u16 cardIdStartSignatureThumbAlt1[2] = {0xB508, 0x202E};
static const u16 cardIdStartSignatureThumbAlt2[2] = {0xB508, 0x20B8};
static const u16 cardIdStartSignatureThumbAlt3[2] = {0xB510, 0x24B8};

// Card read DMA
static const u32 cardReadDmaEndSignature[2]          = {0x01FF8000, 0x000001FF};
static const u32 cardReadDmaEndSignatureSdk2Alt[2]   = {0x01FF8000, 0xE92D4030}; // SDK 2
static const u16 cardReadDmaEndSignatureThumbAlt[4]  = {0x8000, 0x01FF, 0x0000, 0x0200};
static const u32 cardReadDmaStartSignature[1]        = {0xE92D4FF8};
static const u32 cardReadDmaStartSignatureSdk2Alt[1] = {0xE92D4070};
static const u32 cardReadDmaStartSignatureAlt1[1]    = {0xE92D47F0};
static const u32 cardReadDmaStartSignatureAlt2[1]    = {0xE92D4FF0};
static const u32 cardReadDmaStartSignature5[1]       = {0xE92D43F8}; // SDK 5
static const u16 cardReadDmaStartSignatureThumb1[1]  = {0xB5F0}; // SDK <= 2
static const u16 cardReadDmaStartSignatureThumb3[1]  = {0xB5F8}; // SDK >= 3

// Random patch
static const u32 randomPatchSignature[4]        = {0xE3500000, 0x1597002C, 0x10406004, 0x03E06000};
// static const u32 randomPatchSignature5First[4]  = {0xE92D43F8, 0xE3A04000, 0xE1A09001, 0xE1A08002}; // SDK 5
static const u32 randomPatchSignature5Second[3] = {0xE59F003C, 0xE590001C, 0xE3500000};             // SDK 5

// irq enable
static const u32 irqEnableStartSignature1[4]        = {0xE59FC028, 0xE3A01000, 0xE1DC30B0, 0xE59F2020};					// SDK <= 3
static const u32 irqEnableStartSignature2Alt[4]     = {0xE92D000F, 0xE92D4030, 0xE24DD004, 0xEBFFFFDB};					// SDK 2
static const u32 irqEnableStartSignature4[4]        = {0xE59F3024, 0xE3A01000, 0xE1D320B0, 0xE1C310B0};					// SDK >= 4
static const u32 irqEnableStartSignatureThumb[5]    = {0x4D07B430, 0x2100882C, 0x4B068029, 0x1C11681A, 0x60194301};		// SDK <= 3
static const u32 irqEnableStartSignatureThumbAlt[4] = {0x4C07B418, 0x88232100, 0x32081C22, 0x68118021};					// SDK >= 3

// Mpu cache
static const u32 mpuInitRegion0Signature[1] = {0xEE060F10};
static const u32 mpuInitRegion0Data[1]      = {0x4000033};
static const u32 mpuInitRegion1Signature[1] = {0xEE060F11};
static const u32 mpuInitRegion1Data1[1]     = {0x200002D}; // SDK <= 4
static const u32 mpuInitRegion1DataAlt[1]   = {0x200002B};
//static const u32 mpuInitRegion1Data5[1]     = {0x2000031}; // SDK 5
static const u32 mpuInitRegion2Signature[1] = {0xEE060F12};
static const u32 mpuInitRegion2Data1[1]     = {0x27C0023}; // SDK <= 2
static const u32 mpuInitRegion2Data3[1]     = {0x27E0021}; // SDK >= 2 (Late)
//static const u32 mpuInitRegion2Data5[1]     = {0x27FF017}; // SDK 5
static const u32 mpuInitRegion3Signature[1] = {0xEE060F13};
static const u32 mpuInitRegion3Data[1]      = {0x8000035};
static const u32 mpuFlagsSetSignature[4]    = {0xE3A0004A, 0xEE020F30, 0xE3A0004A, 0xEE020F10}; // SDK 5
static const u32 mpuChangeRegion1Signature[3]         = {0xE3A00001, 0xE3A01402, 0xE3A0202A};
static const u32 mpuChangeRegion1SignatureAlt[3]      = {0x03A0202C, 0xE3A00001, 0xE3A01402};
static const u16 mpuChangeRegion1SignatureThumb[3]    = {0x2001, 0x0609, 0x222A};
static const u16 mpuChangeRegion1SignatureThumbAlt[3] = {0x2001, 0x0621, 0x222A};

// Init Heap
static const u32 initHeapEndSignature1[2]              = {0x27FF000, 0x37F8000};
static const u32 initHeapEndSignature5[2]              = {0x2FFF000, 0x37F8000};
static const u32 initHeapEndFuncSignature[1]           = {0xE12FFF1E};
static const u32 initHeapEndFunc2Signature[2]          = {0xE12FFF1E, 0x023E0000};
static const u32 initHeapEndFuncSignatureAlt[1]        = {0xE8BD8008};
static const u32 initHeapEndFunc2SignatureAlt1[2]      = {0xE8BD8008, 0x023E0000};
static const u32 initHeapEndFunc2SignatureAlt2[2]      = {0xE8BD8010, 0x023E0000};
static const u16 initHeapEndFuncSignatureThumb[1]      = {0xBD08};
static const u16 initHeapEndFuncSignatureThumbAlt[1]   = {0x4718};
static const u32 initHeapEndFunc2SignatureThumb[2]     = {0xBD082000, 0x023E0000};
static const u32 initHeapEndFunc2SignatureThumbAlt1[2] = {0x46C04718, 0x023E0000};
static const u32 initHeapEndFunc2SignatureThumbAlt2[2] = {0xBD082010, 0x023E0000};
static const u32 initHeapEndFunc2SignatureThumbAlt3[2] = {0xBD102000, 0x023E0000};

// Reset
static const u32 resetSignature2[4]     = {0xE92D4030, 0xE24DD004, 0xE59F1090, 0xE1A05000}; // sdk2
static const u32 resetSignature2Alt1[4] = {0xE92D000F, 0xE92D4010, 0xEB000026, 0xE3500000}; // sdk2
static const u32 resetSignature2Alt2[4] = {0xE92D4010, 0xE59F1078, 0xE1A04000, 0xE1D100B0}; // sdk2
static const u32 resetSignature3[4]     = {0xE92D4010, 0xE59F106C, 0xE1A04000, 0xE1D100B0}; // sdk3
static const u32 resetSignature3Alt[4]  = {0xE92D4010, 0xE59F1068, 0xE1A04000, 0xE1D100B0}; // sdk3
static const u32 resetSignature3Mb[1]   = {0xE92D4008}; // sdk3
static const u32 resetSignature4[4]     = {0xE92D4070, 0xE59F10A0, 0xE1A04000, 0xE1D100B0}; // sdk4
static const u32 resetSignature4Alt[4]  = {0xE92D4010, 0xE59F1084, 0xE1A04000, 0xE1D100B0}; // sdk4
static const u32 resetSignature5[4]     = {0xE92D4038, 0xE59F1054, 0xE1A05000, 0xE1D100B0}; // sdk5
static const u32 resetSignature5Alt1[4] = {0xE92D4010, 0xE59F104C, 0xE1A04000, 0xE1D100B0}; // sdk2 and sdk5
static const u32 resetSignature5Alt2[4] = {0xE92D4010, 0xE59F1088, 0xE1A04000, 0xE1D100B0}; // sdk5
static const u32 resetSignature5Alt3[4] = {0xE92D4038, 0xE59F106C, 0xE1A05000, 0xE1D100B0}; // sdk5
static const u32 resetSignature5Alt4[4] = {0xE92D4038, 0xE59F1090, 0xE1A05000, 0xE1D100B0}; // sdk5

static const u32 resetConstant[1]       = {RESET_PARAM};
static const u32 resetConstantMb[1]     = {0x027FFE34};
static const u32 resetConstant5[1]      = {RESET_PARAM_SDK5};

// Reset (TWL)
static const u32 nandTmpJumpFuncStart30[1]  = {0xE92D000F};
static const u32 nandTmpJumpFuncStart3[1]   = {0xE92D4008};
static const u32 nandTmpJumpFuncStart4[1]   = {0xE92D4010};

static const u32 nandTmpJumpFuncConstant[1] = {0x02FFDFC0};

// Panic
// TODO : could be a good idea to catch the call to Panic function and store the message somewhere


extern u32 iUncompressedSize;

u32* findModuleParamsOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findModuleParamsOffset:\n");

	extern u32 srlAddr;
	u32* moduleParamsOffset = NULL;
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 1) {
		patchOffsetCache.moduleParamsOffset = 0;
	} else {
		moduleParamsOffset = patchOffsetCache.moduleParamsOffset;
	}
	if (srlAddr > 0 || !moduleParamsOffset) {
		moduleParamsOffset = findOffset(
			(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
			moduleParamsSignature, 2
		);
		if (moduleParamsOffset) {
			dbg_printf("Module params offset found: ");
			patchOffsetCache.moduleParamsOffset = moduleParamsOffset;
		} else {
			dbg_printf("Module params offset not found\n");
		}
	} else {
		dbg_printf("Module params offset restored: ");
	}

	if (moduleParamsOffset) {
		dbg_hexa((u32)moduleParamsOffset);
		dbg_printf("\n");
	}

	return moduleParamsOffset;
}

u32* findCardReadEndOffsetType0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	dbg_printf("findCardReadEndOffsetType0:\n");

	const char* romTid = getRomTid(ndsHeader);

	u32* cardReadEndOffset = NULL;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4008000) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignature3Elab, 3
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end (type 0) elaborate found: ");
		} else {
			dbg_printf("ARM9 Card read end (type 0) elaborate not found\n");
		}
	}

	if (!cardReadEndOffset && moduleParams->sdk_version < 0x2008000) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignatureSdk2Alt, 3
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end SDK 2 alt found: ");
		} else {
			dbg_printf("ARM9 Card read end SDK 2 alt not found\n");
		}
	}

	if (!cardReadEndOffset && strncmp(romTid, "UOR", 3) != 0 && (moduleParams->sdk_version < 0x4008000 || moduleParams->sdk_version > 0x5000000)) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignature, 2
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end (type 0) short found: ");
		} else {
			dbg_printf("ARM9 Card read end (type 0) short not found\n");
		}
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadEndOffsetType1(const tNDSHeader* ndsHeader, u32 startOffset) {
	dbg_printf("findCardReadEndOffsetType1:\n");

	// const char* romTid = getRomTid(ndsHeader);

	u32* cardReadEndOffset = NULL;
	//readType = 1;
	cardReadEndOffset = findOffset(
		(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
		cardReadEndSignatureAlt, 2
	);


	if (!cardReadEndOffset) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignatureAlt2, 3
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

u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader, u32 startOffset) {
	dbg_printf("findCardReadEndOffsetThumb:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
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
u16* findCardReadEndOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findCardReadEndOffsetThumb5Type1:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb5, 4
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end SDK 5 thumb (type 1) found: ");
	} else {
		dbg_printf("ARM9 Card read end SDK 5 thumb (type 1) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

// SDK 5
u16* findCardReadEndOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	dbg_printf("findCardReadEndOffsetThumb5Type0:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb5Alt1, 5
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end SDK 5 thumb alt 1 (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read end SDK 5 thumb alt 1 (type 0) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadStartOffsetType0(const module_params_t* moduleParams, const u32* cardReadEndOffset) {
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
		dbg_printf("ARM9 Card read start (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read start (type 0) not found\n");
	}

	if (!cardReadStartOffset && moduleParams->sdk_version < 0x2008000) {
		cardReadStartOffset = findOffsetBackwards(
			cardReadEndOffset, 0x118,
			cardReadStartSignatureAlt, 1
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start alt 1 (type 0) found\n");
		} else {
			dbg_printf("ARM9 Card read start alt 1 (type 0) not found\n");
		}
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
		cardReadStartSignatureAlt2, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start alt 2 (type 1) found\n");
	} else {
		dbg_printf("ARM9 Card read start alt 2 (type 1) not found\n");
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
		dbg_printf("ARM9 Card read start SDK 5 found\n");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 not found\n");
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwards(
			(u32*)cardReadEndOffset, 0x120,
			cardReadStartSignature5Alt, 1
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start SDK 5.5 found\n");
		} else {
			dbg_printf("ARM9 Card read start SDK 5.5 not found\n");
		}
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardReadCheckOffsetMvDK4(u32 startOffset) {
	dbg_printf("findCardReadCheckOffsetMvDK4:\n");

	u32* cardReadStartOffset = findOffset(
		(u32*)startOffset, 0x20000,
		cardReadCheckSignatureMvDK4, 3
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read check (MvDK4) found\n");
	} else {
		dbg_printf("ARM9 Card read check (MvDK4) not found\n");
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
		cardReadEndOffset, 0x120,
		cardReadStartSignatureThumb, 2
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start thumb found\n");
	} else {
		dbg_printf("ARM9 Card read start thumb not found\n");
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwardsThumb(
			cardReadEndOffset, 0xC0,
			cardReadStartSignatureThumbAlt, 2
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start thumb alt found\n");
		} else {
			dbg_printf("ARM9 Card read start thumb alt not found\n");
		}
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
		dbg_printf("ARM9 Card read start SDK 5 thumb (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 thumb (type 0) not found\n");
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
		dbg_printf("ARM9 Card read start SDK 5 thumb alt (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 thumb alt (type 0) not found\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardPullOutOffset:\n");

	//if (!usesThumb) {
	
	u32* cardPullOutOffset = 0;
	if (moduleParams->sdk_version > 0x5000000) {
		// SDK 5
		cardPullOutOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardPullOutSignature5, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler SDK 5 found\n");
		} else {
			dbg_printf("Card pull out handler SDK 5 not found\n");
		}

		if (!cardPullOutOffset) {
			// SDK 5
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature5Alt, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 5 alt found\n");
			} else {
				dbg_printf("Card pull out handler SDK 5 alt not found\n");
			}
		}
	} else {
		if (moduleParams->sdk_version > 0x2008000 && moduleParams->sdk_version < 0x3000000) {
			// SDK 2
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature1Elab, 5
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 2 elaborate found\n");
			} else {
				dbg_printf("Card pull out handler SDK 2 elaborate not found\n");
			}
		}

		if (!cardPullOutOffset && moduleParams->sdk_version < 0x4000000) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature1, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler found\n");
			} else {
				dbg_printf("Card pull out handler not found\n");
			}
		}

		if (!cardPullOutOffset && moduleParams->sdk_version < 0x2008000) {
			// SDK 2
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature2Alt, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 2 alt found\n");
			} else {
				dbg_printf("Card pull out handler SDK 2 alt not found\n");
			}
		}

		if (!cardPullOutOffset) {
			// SDK 4
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature4, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 4 found\n");
			} else {
				dbg_printf("Card pull out handler SDK 4 not found\n");
			}
		}
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardPullOutOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb, 5
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler thumb found\n");
	} else {
		dbg_printf("Card pull out handler thumb not found\n");
	}

	if (!cardPullOutOffset) {
		cardPullOutOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardPullOutSignatureThumbAlt, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler thumb alt found\n");
		} else {
			dbg_printf("Card pull out handler thumb alt not found\n");
		}
	}

	if (!cardPullOutOffset) {
		cardPullOutOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardPullOutSignatureThumbAlt2, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler thumb alt 2 found\n");
		} else {
			dbg_printf("Card pull out handler thumb alt 2 not found\n");
		}
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
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb5, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler SDK 5 thumb (type 0) found\n");
	} else {
		dbg_printf("Card pull out handler SDK 5 thumb (type 0) not found\n");
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
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb5Alt, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler SDK 5 thumb alt (type 1) found\n");
	} else {
		dbg_printf("Card pull out handler SDK 5 thumb alt (type 1) not found\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

/*u32* findForceToPowerOffOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findForceToPowerOffOffset:\n");

	u32 forceToPowerOffOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
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

	if (isSdk5(moduleParams)) {
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
				(u32*)ndsHeader->arm9destination, iUncompressedSize,
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
			(u32*)cardReadEndOffset + 0x10, iUncompressedSize,
			cardIdEndSignature, 2
		); //if (!usesThumb) {
		if (!cardIdEndOffset) {
			cardIdEndOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,
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
		(u16*)ndsHeader->arm9destination, iUncompressedSize,
		cardIdEndSignatureThumb, 6
	);
	if (cardIdEndOffset) {
		dbg_printf("Card ID end thumb found: ");
	} else {
		dbg_printf("Card ID end thumb not found\n");
	}

	if (!cardIdEndOffset && moduleParams->sdk_version < 0x5000000) {
		// SDK <= 4
		cardIdEndOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardIdEndSignatureThumbAlt, 6
		);
		if (cardIdEndOffset) {
			dbg_printf("Card ID end thumb alt found: ");
		} else {
			dbg_printf("Card ID end thumb alt not found\n");
		}
	}

	if (!cardIdEndOffset && isSdk5(moduleParams)) {
		// SDK 5
		cardIdEndOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardIdEndSignatureThumb5, 8
		);
		if (cardIdEndOffset) {
			dbg_printf("Card ID end SDK 5 thumb found: ");
		} else {
			dbg_printf("Card ID end SDK 5 thumb not found\n");
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

	if (isSdk5(moduleParams)) {
		// SDK 5
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x200,
			cardIdStartSignature5, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start SDK 5 found\n");
		} else {
			dbg_printf("Card ID start SDK 5 not found\n");
		}

		if (!cardIdStartOffset) {
			cardIdStartOffset = findOffsetBackwards(
				(u32*)cardIdEndOffset, 0x200,
				cardIdStartSignature5Alt, 2
			);
			if (cardIdStartOffset) {
				dbg_printf("Card ID start SDK 5 alt 1 found\n");
			} else {
				dbg_printf("Card ID start SDK 5 alt 1 not found\n");
			}
		}
	} else {
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignature, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start found\n");
		} else {
			dbg_printf("Card ID start not found\n");
		}
	}

	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignatureAlt1, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start alt 1 found\n");
		} else {
			dbg_printf("Card ID start alt 1 not found\n");
		}
	}

	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwards(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignatureAlt2, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start alt 2 found\n");
		} else {
			dbg_printf("Card ID start alt 2 not found\n");
		}
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
		(u16*)cardIdEndOffset, 0x50,
		cardIdStartSignatureThumb, 2
	);
	if (cardIdStartOffset) {
		dbg_printf("Card ID start thumb found\n");
	} else {
		dbg_printf("Card ID start thumb not found\n");
	}

	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwardsThumb(
			(u16*)cardIdEndOffset, 0x50,
			cardIdStartSignatureThumbAlt1, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start thumb alt 1 found\n");
		} else {
			dbg_printf("Card ID start thumb alt 1 not found\n");
		}
	}
	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwardsThumb(
			(u16*)cardIdEndOffset, 0x50,
			cardIdStartSignatureThumbAlt2, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start thumb alt 2 found\n");
		} else {
			dbg_printf("Card ID start thumb alt 2 not found\n");
		}
	}
	if (!cardIdStartOffset) {
		cardIdStartOffset = findOffsetBackwardsThumb(
			(u16*)cardIdEndOffset, 0x50,
			cardIdStartSignatureThumbAlt3, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start thumb alt 3 found\n");
		} else {
			dbg_printf("Card ID start thumb alt 3 not found\n");
		}
	}

	dbg_printf("\n");
	return cardIdStartOffset;
}

u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardReadDmaEndOffset:\n");

	u32* cardReadDmaEndOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		cardReadDmaEndSignature, 2
	);
	if (cardReadDmaEndOffset) {
		dbg_printf("Card read DMA end found: ");
	} else {
		dbg_printf("Card read DMA end not found\n");
	}

	if (!cardReadDmaEndOffset && moduleParams->sdk_version < 0x2008000) {
		cardReadDmaEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardReadDmaEndSignatureSdk2Alt, 2
		);
		if (cardReadDmaEndOffset) {
			dbg_printf("Card read DMA end SDK 2 alt found: ");
		} else {
			dbg_printf("Card read DMA end SDK 2 alt not found\n");
		}
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
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
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

		if (!cardReadDmaStartOffset && moduleParams->sdk_version < 0x2008000) {
			cardReadDmaStartOffset = findOffsetBackwards(
				(u32*)cardReadDmaEndOffset, 0x200,
				cardReadDmaStartSignatureSdk2Alt, 1
			);
			if (cardReadDmaStartOffset) {
				dbg_printf("Card read DMA start SDK 2 alt found: ");
			} else {
				dbg_printf("Card read DMA start SDK 2 alt not found\n");
			}
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
		(u16*)cardReadDmaEndOffset, 0x200,
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

u32* a9FindCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumb) {
	dbg_printf("findCardIrqEnableOffset:\n");
	
	const u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (moduleParams->sdk_version > 0x4008000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32* cardIrqEnableOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
		irqEnableStartSignature, 4
	);
	if (cardIrqEnableOffset) {
		dbg_printf("irq enable found: ");
	} else {
		dbg_printf("irq enable not found\n");
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version < 0x2008000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature2Alt, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 2 alt found: ");
		} else {
			dbg_printf("irq enable SDK 2 alt not found\n");
		}
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4008000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature4, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 4 found: ");
		} else {
			dbg_printf("irq enable SDK 4 not found\n");
		}
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version < 0x4008000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignatureThumb, 5
		);
		if (cardIrqEnableOffset) {
			*usesThumb = true;
			dbg_printf("irq enable thumb found: ");
		} else {
			dbg_printf("irq enable thumb not found\n");
		}
	}

	if (!cardIrqEnableOffset) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignatureThumbAlt, 4
		);
		if (cardIrqEnableOffset) {
			*usesThumb = true;
			dbg_printf("irq enable thumb alt found: ");
		} else {
			dbg_printf("irq enable thumb alt not found\n");
		}
	}

	if (cardIrqEnableOffset) {
		dbg_hexa((u32)cardIrqEnableOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIrqEnableOffset;
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
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuInitRegionSignature, 1
	);
	if (mpuStartOffset) {
		dbg_printf("Mpu init found\n");
	} else {
		dbg_printf("Mpu init not found\n");
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
	if (moduleParams->sdk_version >= 0x2008000) {
		mpuInitRegion2Data = mpuInitRegion2Data3;
	}
	/*if (moduleParams->sdk_version > 0x5000000) {
		mpuInitRegion1Data = mpuInitRegion1Data5;
	}*/

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
		dbg_printf("Mpu data found\n");
	} else {
		dbg_printf("Mpu data not found\n");
	}

	dbg_printf("\n");
	return mpuDataOffset;
}

u32* findMpuDataOffsetAlt(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuDataOffsetAlt:\n");

	u32* mpuDataOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuInitRegion1DataAlt, 1
	);
	if (mpuDataOffset) {
		dbg_printf("Mpu data found\n");
	} else {
		dbg_printf("Mpu data not found\n");
	}

	dbg_printf("\n");
	return mpuDataOffset;
}

u32* findMpuFlagsSetOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuFlagsSet:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuFlagsSetSignature, 4
	);
	if (offset) {
		dbg_printf("Mpu flags set found\n");
	} else {
		dbg_printf("Mpu flags set not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findMpuChange(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuChange:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuChangeRegion1Signature, 3
	);
	if (offset) {
		dbg_printf("Mpu change found\n");
	} else {
		dbg_printf("Mpu change not found\n");
	}

	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			mpuChangeRegion1SignatureAlt, 3
		);
		if (offset) {
			dbg_printf("Mpu change alt found\n");
		} else {
			dbg_printf("Mpu change alt not found\n");
		}
	}

	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			mpuChangeRegion1SignatureThumb, 3
		);
		if (offset) {
			dbg_printf("Mpu change thumb found\n");
		} else {
			dbg_printf("Mpu change thumb not found\n");
		}
	}

	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			mpuChangeRegion1SignatureThumbAlt, 3
		);
		if (offset) {
			dbg_printf("Mpu change thumb alt found\n");
		} else {
			dbg_printf("Mpu change thumb alt not found\n");
		}
	}

	dbg_printf("\n");
	return offset;
}

u32* findHeapPointerOffset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	dbg_printf("findHeapPointerOffset:\n");
    
	const u32* initHeapEndSignature = initHeapEndSignature1;
	if (moduleParams->sdk_version > 0x5000000) {
		initHeapEndSignature = initHeapEndSignature5;
	}

    u32* initHeapEnd = findOffset(
        (u32*)ndsHeader->arm9destination, iUncompressedSize,
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

	u32* initEndFunc = findOffsetBackwards(
		(u32*)initHeapEnd, 0x40,
		initHeapEndFuncSignature, 1
	);
	if (!initEndFunc) {
		initEndFunc = findOffsetBackwards(
			(u32*)initHeapEnd, 0x40,
			initHeapEndFuncSignatureAlt, 1
		);
	}
    u32* heapPointer = initEndFunc + 1;
    
	if (!initEndFunc) {
		u16* initEndFuncThumb = findOffsetBackwardsThumb(
			(u16*)initHeapEnd, 0x40,
			initHeapEndFuncSignatureThumb, 1
		);
		if (!initEndFuncThumb) {
			initEndFuncThumb = findOffsetBackwardsThumb(
				(u16*)initHeapEnd, 0x40,
				initHeapEndFuncSignatureThumbAlt, 1
			);
		}
        heapPointer = (u32*)((u16*)initEndFuncThumb+1);
	}
    
    dbg_hexa((u32)heapPointer);
	dbg_printf("\n");

	return heapPointer;
}

u32* findHeapPointer2Offset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	dbg_printf("findHeapPointer2Offset:\n");
    
	u32* initEndFunc = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		initHeapEndFunc2Signature, 2
	);
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureAlt1, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureAlt2, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumb, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumbAlt1, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumbAlt2, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumbAlt3, 2
		);
	}
    u32* heapPointer = initEndFunc + 1;
    
    dbg_hexa((u32)heapPointer);
	dbg_printf("\n");

	return heapPointer;
}

u32* findRandomPatchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRandomPatchOffset:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
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
u32* findRandomPatchOffset5Second(const tNDSHeader* ndsHeader) {
	dbg_printf("findRandomPatchOffset5Second:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
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

u32* findResetOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* softResetMb) {
	dbg_printf("findResetOffset\n");
    const u32* resetSignature = resetSignature2;

    if (moduleParams->sdk_version > 0x4008000 && moduleParams->sdk_version < 0x5000000) { 
        resetSignature = resetSignature4;
    }
    if (moduleParams->sdk_version > 0x5000000) {
        resetSignature = resetSignature5;
    }

    u32 * resetOffset = NULL;

	if ((memcmp(getRomTid(ndsHeader), "NTRJ", 4) == 0) && (moduleParams->sdk_version < 0x5000000)) {
		u32* resetConstOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			resetConstantMb, 1
		);

		if (resetConstOffset) {
			resetOffset = findOffsetBackwards(
				resetConstOffset, 0x80,
				resetSignature3Mb, 1
			);
			if (!resetOffset) {
				resetOffset = findOffsetBackwards(
					resetConstOffset, 0x80,
					resetSignature3, 1
				);
			}

			if (resetOffset) {
				dbg_printf("Reset found\n");
				dbg_printf("\n");
				*softResetMb = true;
				return resetOffset;
			} else {
				dbg_printf("Reset not found\n");
			}
		}
	}

  	resetOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		resetSignature, 4
	);
	
	if (!resetOffset) {
		if (moduleParams->sdk_version > 0x2000000 && moduleParams->sdk_version < 0x2008000) {
			resetOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature5Alt1, 4
			);
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature2Alt1, 4
				);
			}
		} else if (moduleParams->sdk_version < 0x4008000) {
			if (moduleParams->sdk_version > 0x2008000 && moduleParams->sdk_version < 0x3000000) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature2Alt2, 4
				);
			}
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature3, 4
				);
			}
			if (moduleParams->sdk_version > 0x3000000) {
				if (!resetOffset) {
					resetOffset = findOffset(
						(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
						resetSignature3Alt, 4
					);
				}
			}
		} else if (moduleParams->sdk_version > 0x4008000 && moduleParams->sdk_version < 0x5000000) {
			resetOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature4Alt, 4
			);
		} else if (moduleParams->sdk_version > 0x5000000) {
			resetOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature5Alt1, 4
			);
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature5Alt2, 4
				);
			}
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature5Alt3, 4
				);
			}
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature5Alt4, 4
				);
			}
		}
	}
    
    if (resetOffset) {
		dbg_printf("Reset found: ");
        dbg_hexa((u32)resetOffset);
		dbg_printf("\n");
    } 
    
    while(resetOffset!=NULL) {
    	u32* resetEndOffset = findOffset(
    		resetOffset, 0x200,
    		(isSdk5(moduleParams) ? resetConstant5 : resetConstant), 1
    	);
        if (resetEndOffset) {
    		dbg_printf("Reset constant found: ");
            dbg_hexa((u32)resetEndOffset);
    		dbg_printf("\n");
            break;
        } 
        
      	resetOffset = findOffset(
				resetOffset+1, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature, 4
			);
        if (resetOffset) {
		    dbg_printf("Reset found: ");
            dbg_hexa((u32)resetOffset);
    		dbg_printf("\n");
        } 
    } 
    
	if (resetOffset) {
		dbg_printf("Reset found\n");
	} else {
		dbg_printf("Reset not found\n");
	}

	dbg_printf("\n");
	return resetOffset;
}

u32* findNandTmpJumpFuncOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findNandTmpJumpFuncOffset\n");
	
	u32* endOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		nandTmpJumpFuncConstant, 1
	);

	u32* offset = NULL;
	if (moduleParams->sdk_version < 0x5008000) {
		offset = findOffsetBackwards(
			endOffset, 0x400,
			nandTmpJumpFuncStart30, 1
		);
	} else {
		offset = findOffsetBackwards(
			endOffset, 0x60,
			nandTmpJumpFuncStart3, 1
		);

		if (!offset) {
			offset = findOffsetBackwards(
				endOffset, 0x60,
				nandTmpJumpFuncStart4, 1
			);
		}
	}

	if (offset) {
		dbg_printf("nandTmpJumpFunc found\n");
	} else {
		dbg_printf("nandTmpJumpFunc not found\n");
	}

	dbg_printf("\n");
	return offset;
}
