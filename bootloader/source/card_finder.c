#include <string.h>
#include <stdlib.h>
#include <nds/system.h>
#include <nds/memory.h>
#include "card_finder.h"
#include "debugToFile.h"

extern u32 ROM_TID;

bool cardReadFound = false;
bool usesThumb = false;
bool sdk5 = false;

static int readType = 0;

//
// Subroutine function signatures ARM7
//

u32 j_HaltSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00007BAF};
u32 j_HaltSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B7A3};
u32 j_HaltSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B837};
u32 j_HaltSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B92B};
u32 j_HaltSignature1Alt4[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000BAEB};
u32 j_HaltSignature1Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803B93};
u32 j_HaltSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DAF};
u32 j_HaltSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DB3};
u32 j_HaltSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ECB};
u32 j_HaltSignature1Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F13};
u32 j_HaltSignature1Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03804133};
u32 j_HaltSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800F7F};
u32 j_HaltSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x038010F3};
u32 j_HaltSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x038011BF};
u32 j_HaltSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803597};
u32 j_HaltSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038040C3};
u32 j_HaltSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038042AB};
u32 j_HaltSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x038042AF};
u32 j_HaltSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x0380433F};
u32 j_HaltSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x038043E3};
u32 j_HaltSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804503};
u32 j_HaltSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038045BF};
u32 j_HaltSignature3Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x0380538B};
u32 j_HaltSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x0380064B};
u32 j_HaltSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x038008C3};
u32 j_HaltSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x038008CF};
u32 j_HaltSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x0380356F};
u32 j_HaltSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038036BF};
u32 j_HaltSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038037D3};
u32 j_HaltSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E7F};
u32 j_HaltSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803EBF};
u32 j_HaltSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804503};

u32 swi12Signature[1]                 = {0x4770DF12}; // LZ77UnCompReadByCallbackWrite16bit
u32 j_GetPitchTableSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004721};
u32 j_GetPitchTableSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BB9};
u32 j_GetPitchTableSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BC9};
u32 j_GetPitchTableSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BE5};
u32 j_GetPitchTableSignature1Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803BE9};
u32 j_GetPitchTableSignature1Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E05};
u32 j_GetPitchTableSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E09};
u32 j_GetPitchTableSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F21};
u32 j_GetPitchTableSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804189};
u32 j_GetPitchTableSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800FD5};
u32 j_GetPitchTableSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801149};
u32 j_GetPitchTableSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801215};
u32 j_GetPitchTableSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804119};
u32 j_GetPitchTableSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804301};
u32 j_GetPitchTableSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804305};
u32 j_GetPitchTableSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804395};
u32 j_GetPitchTableSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804439};
u32 j_GetPitchTableSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804559};
u32 j_GetPitchTableSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804615};
u32 j_GetPitchTableSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038053E1};
u32 j_GetPitchTableSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x038006A1};
u32 j_GetPitchTableSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800919};
u32 j_GetPitchTableSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800925};
u32 j_GetPitchTableSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035C5};
u32 j_GetPitchTableSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035ED};
u32 j_GetPitchTableSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803715};
u32 j_GetPitchTableSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803829};
u32 j_GetPitchTableSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ED5};
u32 j_GetPitchTableSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F15};
u32 swiGetPitchTableSignature5[4]     = {0x781A4B06, 0xD3030791, 0xD20106D1, 0x1A404904};


//
// Subroutine function signatures ARM9
//

u32 moduleParamsSignature[2] = {0xDEC00621, 0x2106C0DE};

u32 a9cardReadSignature[2]             = {0x04100010, 0x040001A4};
u32 a9cardReadSignatureAlt[2]          = {0x040001A4, 0x04100010};
u16 a9cardReadSignatureThumb[4]        = {0x01A4, 0x0400, 0x0200, 0x0000};
u32 cardReadStartSignature[1]          = {0xE92D4FF0};
u32 cardReadStartSignatureAlt[1]       = {0xE92D4070};
u16 cardReadStartSignatureThumb[2]     = {0xB5F8, 0xB082};
u16 cardReadStartSignatureThumbAlt1[2] = {0xB5F0, 0xB083};

u32 a9cardIdSignature[2]             = {0x040001A4, 0x04100010};
u16 a9cardIdSignatureThumb[6]        = {0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410};
u32 cardIdStartSignature[1]          = {0xE92D4000};
u32 cardIdStartSignatureAlt[1]       = {0xE92D4008};
u32 cardIdStartSignatureAlt2[1]      = {0xE92D4010};
u16 cardIdStartSignatureThumb[2]     = {0xB500, 0xB081};
u16 cardIdStartSignatureThumbAlt[2]  = {0xB508, 0x202E};
u16 cardIdStartSignatureThumbAlt2[2] = {0xB508, 0x20B8};
u16 cardIdStartSignatureThumbAlt3[2] = {0xB510, 0x24B8};
  
//u32 a9instructionBHI[1] = {0x8A000001};

u32 cardPullOutSignature1[4]         = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011};
u32 cardPullOutSignature4[4]         = {0xE92D4008, 0xE201003F, 0xE3500011, 0x1A00000D};
u16 cardPullOutSignatureThumb[4]     = {0xB508, 0x203F, 0x4008, 0x2811};
u16 cardPullOutSignatureThumbAlt1[4] = {0xB500, 0xB081, 0x203F, 0x4001};

//u32 a9cardSendSignature[7] = {0xE92D40F0, 0xE24DD004, 0xE1A07000, 0xE1A06001, 0xE1A01007, 0xE3A0000E, 0xE3A02000};

u32 cardCheckPullOutSignature1[4] = {0xE92D4018, 0xE24DD004, 0xE59F204C, 0xE1D210B0};
u32 cardCheckPullOutSignature3[4] = {0xE92D4000, 0xE24DD004, 0xE59F002C, 0xE1D000B0};

//u32 forceToPowerOffSignature[4] = {0xE92D4000, 0xE24DD004, 0xE59F0028, 0xE28D1000};

u32 cardReadCachedStartSignature1[2] = {0xE92D4030, 0xE24DD004};
u32 cardReadCachedEndSignature1[4]   = {0xE5950020, 0xE3500000, 0x13A00001, 0x03A00000};

u32 cardReadCachedEndSignature3[4]   = {0xE5950024, 0xE3500000, 0x13A00001, 0x03A00000};

u32 cardReadCachedStartSignature4[2] = {0xE92D4038, 0xE59F407C};
u32 cardReadCachedEndSignature4[4]   = {0xE5940024, 0xE3500000, 0x13A00001, 0x03A00000};

u32 cardReadDmaStartSignature[1]       = {0xE92D4FF8};
u32 cardReadDmaStartSignatureAlt[1]    = {0xE92D47F0};
u32 cardReadDmaStartSignatureAlt2[1]   = {0xE92D4FF0};
u16 cardReadDmaStartSignatureThumb1[1] = {0xB5F0};
u16 cardReadDmaStartSignatureThumb3[1] = {0xB5F8};

u32 cardReadDmaEndSignature[2]         = {0x01FF8000, 0x000001FF};
u16 cardReadDmaEndSignatureThumbAlt[4] = {0x8000, 0x01FF, 0x0000, 0x0200};

u32 aRandomPatch[4]       = {0xE3500000, 0x1597002C, 0x10406004, 0x03E06000};
u32 sleepPatch[2]         = {0x0A000001, 0xE3A00601};
u16 sleepPatchThumb[2]    = {0xD002, 0x4831};
u16 sleepPatchThumbAlt[2] = {0xD002, 0x0440};

// irqEnable
u32 irqEnableStartSignature1[4] = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0};
u32 irqEnableStartSignature4[4] = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020};

//u32 arenaLowSignature[4] = {0xE1A00100, 0xE2800627, 0xE2800AFF, 0xE5801DA0};

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

u32 mpuInitCache[1] = {0xE3A00042};

// Look @find and return the position of it.
u32 getOffset(u32* addr, size_t size, u32* find, size_t lenofFind, int direction) {
	u32* debug = (u32*)0x037D0000;
	u32* end = addr + size/sizeof(u32);
	debug[3] = (u32)end;
	for (; addr != end; addr += direction) {
		bool found = true;

		for (int i = 0; i < lenofFind; i++) {
			if (addr[i] != find[i]) {
				found = false;
				break;
			}
		}

		if (found) {
			return (u32)addr;
		}
	}

	return (u32)NULL;
}

u32 getOffsetThumb(u16* addr, size_t size, u16* find, size_t lenofFind, int direction) {
	for (u16* end = addr + size/sizeof(u16); addr != end; addr += direction) {
		bool found = true;

		for (int i = 0; i < lenofFind; i++) {
			if (addr[i] != find[i]) {
				found = false;
				break;
			}
		}

		if (found) {
			return (u32)addr;
		}
	}

	return (u32)NULL;
}

module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer) {
	dbg_printf("Looking for moduleparams\n");
	u32 moduleparams = getOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		(u32*)moduleParamsSignature, 2,
		1
	);
	*(vu32*)(0x2800008) = (moduleparams - 0x8);
	if (!moduleparams) {
		dbg_printf("No moduleparams?\n");
		*(vu32*)(0x2800010) = 1;
		moduleparams = (u32)malloc(0x100);
		memset((u32*)moduleparams, 0, 0x100);
		((module_params_t*)(moduleparams - 0x1C))->compressed_static_end = 0;
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
	return (module_params_t*)(moduleparams - 0x1C);
}

u32 getCardReadEndOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	u32 cardReadEndOffset = 0;
	if (ROM_TID == 0x45524F55) {
		// Start at 0x3800 for "WarioWare: DIY (USA)"
		readType = 1;
		cardReadEndOffset = getOffset(
			(u32*)ndsHeader->arm9destination + 0x3800, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)a9cardReadSignatureAlt, 2,
			1
		);
	} else if (moduleParams->sdk_version < 0x4000000) {
		cardReadEndOffset = getOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)a9cardReadSignature, 2,
			1
		);
	}
	if (!cardReadEndOffset && readType != 1) {
		dbg_printf("Card read end not found. Trying alt\n");
		readType = 1;
		cardReadEndOffset = getOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)a9cardReadSignatureAlt, 2,
			1
		);
		if (*(u32*)(cardReadEndOffset - 4) == 0xFFFFFE00) {
			dbg_printf("Found thumb\n");
			cardReadEndOffset -= 4;
			usesThumb = true;
		}
	}
	if (!cardReadEndOffset) {
		dbg_printf("Card read end alt not found. Trying thumb\n");
		usesThumb = true;
		cardReadEndOffset = getOffsetThumb(
			(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u16*)a9cardReadSignatureThumb, 4,
			1
		);
	}
	if (!cardReadEndOffset) {
		dbg_printf("Thumb card read end not found\n");
	}
	return cardReadEndOffset;
}

u32 getCardReadStartOffset(const tNDSHeader* ndsHeader, u32 cardReadEndOffset) {
	u32 cardReadStartOffset = 0;
	if (readType == 1) {
		cardReadStartOffset = getOffset(
			(u32*)cardReadEndOffset, -0x118,
			(u32*)cardReadStartSignatureAlt, 1,
			-1
		);
	} else {
		cardReadStartOffset = getOffset(
			(u32*)cardReadEndOffset, -0x118,
			(u32*)cardReadStartSignature, 1,
			-1
		);
	}
	if (!cardReadStartOffset) {
		dbg_printf("Card read start not found. Trying thumb\n");
		cardReadStartOffset = getOffsetThumb(
			(u16*)cardReadEndOffset, -0xC0,
			(u16*)cardReadStartSignatureThumb, 2,
			-1
		);
	}
	if (!cardReadStartOffset) {
		dbg_printf("Thumb card read start not found\n");
		cardReadStartOffset = getOffsetThumb(
			(u16*)cardReadEndOffset, -0xC0,
			(u16*)cardReadStartSignatureThumbAlt1, 2,
			-1
		);
	}
	if (!cardReadStartOffset) {
		dbg_printf("Thumb card read start alt 1 not found\n");
	} else {
		dbg_printf("Arm9 Card read:\t");
		dbg_hexa(cardReadStartOffset);
		dbg_printf("\n");
		cardReadFound = true;
	}
	return cardReadStartOffset;
}

u32 getCardPullOutOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	u32* cardPullOutSignature = cardPullOutSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		cardPullOutSignature = cardPullOutSignature4;
	}
	
	u32 cardPullOutOffset = 0;
	if (usesThumb) {
		cardPullOutOffset = getOffsetThumb(
			(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u16*)cardPullOutSignatureThumb, 4,
			1
		);
		if (!cardPullOutOffset) {
			dbg_printf("Thumb card pull out handler not found. Trying alt\n");
			cardPullOutOffset = getOffsetThumb(
				(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				(u16*)cardPullOutSignatureThumbAlt1, 4,
				1
			);
		}
		if (!cardPullOutOffset) {
			dbg_printf("Thumb card pull out handler alt not found\n");
		}
	} else {
		cardPullOutOffset = getOffset(
			(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)cardPullOutSignature, 4,
			1
		);
		if (!cardPullOutOffset) {
			dbg_printf("Card pull out handler not found\n");
		} else {
			dbg_printf("Card pull out handler:\t");
			dbg_hexa(cardPullOutOffset);
			dbg_printf("\n");
		}
	}
	return cardPullOutOffset;
}

/*u32 getForceToPowerOffOffset(const tNDSHeader* ndsHeader) {
	u32 forceToPowerOffOffset = getOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		(u32*)forceToPowerOffSignature, 4,
		1
	);
	if (!forceToPowerOffOffset) {
		dbg_printf("Force to power off handler not found\n");
	} else {
		dbg_printf("Force to power off handler:\t");
		dbg_hexa(forceToPowerOffOffset);
		dbg_printf("\n");
	}
	return forceToPowerOffOffset;
}*/

u32 getCardReadCachedEndOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	u32* cardReadCachedEndSignature = cardReadCachedEndSignature1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardReadCachedEndSignature = cardReadCachedEndSignature3;
	} else if (moduleParams->sdk_version > 0x4000000) {
		cardReadCachedEndSignature = cardReadCachedEndSignature4;
	}
	
	u32 cardReadCachedEndOffset = getOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		(u32*)cardReadCachedEndSignature, 4,
		1
	);
	if (!cardReadCachedEndOffset) {
		dbg_printf("Card read cached end not found\n");
		//cardReadFound = false;
	}
	return cardReadCachedEndOffset;
}

u32 getCardReadCachedStartOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams, u32 cardReadCachedEndOffset) {
	u32* cardReadCachedStartSignature = cardReadCachedStartSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		cardReadCachedStartSignature = cardReadCachedStartSignature4;
	}
	
	u32 cardReadCachedStartOffset = getOffset(
		(u32*)cardReadCachedEndOffset, -0xFF,
		(u32*)cardReadCachedStartSignature, 2,
		-1
	);
	if (!cardReadCachedStartOffset) {
		dbg_printf("Card read cached start not found\n");
		//cardReadFound = false;
	} else {
		dbg_printf("Card read cached :\t");
		dbg_hexa(cardReadCachedStartOffset);
		dbg_printf("\n");
	}
	return cardReadCachedStartOffset;
}

u32 getCardIdEndOffset(const tNDSHeader* ndsHeader, u32 cardReadEndOffset) {
	u32 cardIdEndOffset = getOffset(
		(u32*)cardReadEndOffset + 0x10, ndsHeader->arm9binarySize,
		(u32*)a9cardIdSignature, 2,
		1
	);
	if (usesThumb) {
		if (!cardIdEndOffset) {
			cardIdEndOffset = getOffsetThumb(
				(u16*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
				(u16*)a9cardIdSignatureThumb, 6,
				1
			);
		}
	} else {
		if (!cardIdEndOffset) {
			cardIdEndOffset = getOffset(
				(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
				(u32*)a9cardIdSignature, 2,
				1
			);
		}
	}
	if (!cardIdEndOffset) {
		dbg_printf("Card id end not found\n");
	}
	return cardIdEndOffset;
}

u32 getCardIdStartOffset(const tNDSHeader* ndsHeader, u32 cardIdEndOffset) {
	if (!cardIdEndOffset) {
		return 0;
	}
	u32 cardIdStartOffset = 0;
	cardIdStartOffset = getOffset(
		(u32*)cardIdEndOffset, -0x100,
		(u32*)cardIdStartSignature, 1,
		-1
	);
	if (usesThumb) {
		if (!cardIdStartOffset) {
			cardIdStartOffset = getOffsetThumb(
				(u16*)cardIdEndOffset, -0x40,
				(u16*)cardIdStartSignatureThumb, 2,
				-1
			);
		}
		if (!cardIdStartOffset) {
			cardIdStartOffset = getOffsetThumb(
				(u16*)cardIdEndOffset, -0x40,
				(u16*)cardIdStartSignatureThumbAlt, 2,
				-1
			);
		}
		if (!cardIdStartOffset) {
			cardIdStartOffset = getOffsetThumb(
				(u16*)cardIdEndOffset, -0x40,
				(u16*)cardIdStartSignatureThumbAlt2, 2,
				-1
			);
		}
		if (!cardIdStartOffset) {
			cardIdStartOffset = getOffsetThumb(
				(u16*)cardIdEndOffset, -0x40,
				(u16*)cardIdStartSignatureThumbAlt3, 2,
				-1
			);
		}
	} else {
		if (!cardIdStartOffset) {
			cardIdStartOffset = getOffset(
				(u32*)cardIdEndOffset, -0x100,
				(u32*)cardIdStartSignatureAlt, 1,
				-1
			);
		}
		if (!cardIdStartOffset) {
			cardIdStartOffset = getOffset(
				(u32*)cardIdEndOffset, -0x100,
				(u32*)cardIdStartSignatureAlt2, 1,
				-1
			);
		}
	}
	if (!cardIdStartOffset) {
		dbg_printf("Card id start not found\n");
	} else {
		dbg_printf("Card id :\t");
		dbg_hexa(cardIdStartOffset);
		dbg_printf("\n");
	}
	return cardIdStartOffset;
}

u32 getCardReadDmaEndOffset(const tNDSHeader* ndsHeader) {
	u32 cardReadDmaEndOffset = getOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		(u32*)cardReadDmaEndSignature, 2,
		1
	);
	if (!cardReadDmaEndOffset) {
		dbg_printf("Card read dma end not found\n");
	}
	if (usesThumb) {
		if (!cardReadDmaEndOffset) {
			dbg_printf("Trying thumb alt\n");
			cardReadDmaEndOffset = getOffsetThumb(
				(u16*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				(u16*)cardReadDmaEndSignatureThumbAlt, 4,
				1
			);
		}
		if (!cardReadDmaEndOffset) {
			dbg_printf("Thumb card read dma end alt not found\n");
		}
	}
	return cardReadDmaEndOffset;
}

u32 getCardReadDmaStartOffset(const tNDSHeader* ndsHeader, u32 cardReadDmaEndOffset) {
	if (!cardReadDmaEndOffset) {
		return 0;
	}
	u32 cardReadDmaStartOffset = 0;
	dbg_printf("Card read dma end :\t");
	dbg_hexa(cardReadDmaEndOffset);
	dbg_printf("\n");
	if (usesThumb) {
		cardReadDmaStartOffset = getOffsetThumb(
			(u16*)cardReadDmaEndOffset, -0x100,
			(u16*)cardReadDmaStartSignatureThumb1, 1,
			-1
		);
		if (!cardReadDmaStartOffset) {
			dbg_printf("Thumb card read dma start 1 not found\n");
			cardReadDmaStartOffset = getOffsetThumb(
				(u16*)cardReadDmaEndOffset, -0x200,
				(u16*)cardReadDmaStartSignatureThumb3, 1,
				-1
			);
		}
		if (!cardReadDmaStartOffset) {
			dbg_printf("Thumb card read dma start 3 not found\n");
		}
	} else {
		cardReadDmaStartOffset = getOffset(
			(u32*)cardReadDmaEndOffset, -0x200,
			(u32*)cardReadDmaStartSignature, 1,
			-1
		);
		if (!cardReadDmaStartOffset) {
			dbg_printf("Card read dma start not found\n");
			cardReadDmaStartOffset = getOffset(
				(u32*)cardReadDmaEndOffset, -0x200,
				(u32*)cardReadDmaStartSignatureAlt, 1,
				-1
			);
		}
		if (!cardReadDmaStartOffset) {
			dbg_printf("Card read dma start alt not found\n");
			cardReadDmaStartOffset = getOffset(
				(u32*)cardReadDmaEndOffset, -0x200,
				(u32*)cardReadDmaStartSignatureAlt2, 1,
				-1
			);
		}
		if (!cardReadDmaStartOffset) {
			dbg_printf("Card read dma start alt2 not found\n");
		}
	}
	if (cardReadDmaStartOffset) {
		dbg_printf("Card read dma :\t");
		dbg_hexa(cardReadDmaStartOffset);
		dbg_printf("\n");
	}
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

u32 getMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion) {
	u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);

	u32 mpuStartOffset = getOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		mpuInitRegionSignature, 1,
		1
	);
	if (!mpuStartOffset) {
		dbg_printf("Mpu init not found\n");
	}
	return mpuStartOffset;
}

u32 getMpuDataOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams, u32 mpuStartOffset, u32 patchMpuRegion) {
	if (!mpuStartOffset) {
		return 0;
	}

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
	
	u32 mpuDataOffset = 0;
	mpuDataOffset = getOffset(
		(u32*)mpuStartOffset, 0x100,
		mpuInitRegionData, 1,
		1
	);
	if (!mpuDataOffset) {
		// Try to find it
		for (int i = 0; i < 0x100; i++) {
			mpuDataOffset += i;
			if ((*(u32*)mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
				break;
			}
		}
	}
	if (!mpuDataOffset) {
		dbg_printf("Mpu data not found\n");
	} else {
		dbg_printf("Mpu data :\t");
		dbg_hexa((u32)mpuDataOffset);
		dbg_printf("\n");
	}
	return mpuDataOffset;
}

u32 getMpuInitCacheOffset(const tNDSHeader* ndsHeader, u32 mpuStartOffset) {
	u32 mpuInitCacheOffset = getOffset(
		(u32*)mpuStartOffset, 0x100,
		(u32*)mpuInitCache, 1,
		1
	);
	if (!mpuInitCacheOffset) {
		dbg_printf("Mpu init cache not found\n");
	}
	return mpuInitCacheOffset;
}

/*u32 getArenaLowOffset(const tNDSHeader* ndsHeader) {
	u32 arenaLowOffset = getOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		(u32*)arenaLowSignature, 4,
		1
	);
	if (!arenaLowOffset) {
		dbg_printf("Arena low not found\n");
	} else {
		dbg_printf("Arena low found\n");
	}
	return arenaLowOffset;
}*/

u32 getRandomPatchOffset(const tNDSHeader* ndsHeader) {
	u32 randomPatchOffset = getOffset(
		(u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
		(u32*)aRandomPatch, 4,
		1
	);
	/*if (!randomPatchOffset) {
		dbg_printf("Random patch not found\n");
	}*/
	return randomPatchOffset;
}

u32 getSwiHaltOffset(const tNDSHeader* ndsHeader) {
	u32 swiHaltOffset = getOffset(
		(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
		(u32*)j_HaltSignature1, 4,
		1
	);
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt1, 4,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 1 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt2, 4,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 2 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt3, 4,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 3 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt4, 4,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 4 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt5, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 5 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt6, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 6 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt7, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 7 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt8, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 8 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt9, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 9 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature1Alt10, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 10 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt1, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 1 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt2, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 2 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt3, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 3 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt4, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 4 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt5, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 5 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt6, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 6 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt7, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 7 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt8, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 8 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt9, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 9 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt10, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 10 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature3Alt11, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 11 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt1, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 1 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt2, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 2 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt3, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 3 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt4, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 4 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt5, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 5 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt6, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 6 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt7, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 7 not found\n");
		swiHaltOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			(u32*)j_HaltSignature4Alt8, 3,
			1
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 8 not found\n");
	} else {
		dbg_printf("swiHalt call found\n");
	}
	return swiHaltOffset;
}

u32 getSwi12Offset(const tNDSHeader* ndsHeader) {
	u32 swi12Offset = getOffset(
		(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
		(u32*)swi12Signature, 1,
		1
	);
	if (!swi12Offset) {
		dbg_printf("swi 0x12 call not found\n");
	} else {
		dbg_printf("swi 0x12 call found\n");
	}
	return swi12Offset;
}

u32 getSwiGetPitchTableOffset(const tNDSHeader* ndsHeader) {
	u32 swiGetPitchTableOffset = getOffset(
		(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
		(u32*)j_GetPitchTableSignature1, 4,
		1
	);
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt1, 4,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 1 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt2, 4,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 2 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt3, 4,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 3 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt4, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 4 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt5, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 5 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt6, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 6 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt7, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 7 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature1Alt8, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 8 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt1, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 1 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt2, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 2 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt3, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 3 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt4, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 4 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt5, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 5 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt6, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 6 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt7, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 7 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt8, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 8 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt9, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 9 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature3Alt10, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 10 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt1, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 1 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt2, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 2 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt3, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 3 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt4, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 4 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt5, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 5 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt6, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 6 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt7, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 7 not found\n");
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)j_GetPitchTableSignature4Alt8, 3,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 8 not found\n");
		sdk5 = true;
		swiGetPitchTableOffset = getOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			(u32*)swiGetPitchTableSignature5, 4,
			1
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable call SDK5 not found\n");
	} else {
		dbg_printf("swiGetPitchTable call found\n");
	}
	return swiGetPitchTableOffset;
}

u32 getSleepPatchOffset(const tNDSHeader* ndsHeader) {
	u32 sleepPatchOffset = getOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		(u32*)sleepPatch, 2,
		1
	);
	if (!sleepPatchOffset) {
		dbg_printf("Sleep patch not found. Trying thumb\n");
		usesThumb = true;
		sleepPatchOffset = getOffsetThumb(
			(u16*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
			(u16*)sleepPatchThumb, 2,
			1
		);
	} else {
		dbg_printf("Sleep patch found\n");
	}
	if (!sleepPatchOffset) {
		dbg_printf("Thumb sleep patch not found. Trying alt\n");
		sleepPatchOffset = getOffsetThumb(
			(u16*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
			(u16*)sleepPatchThumbAlt, 2,
			1
		);
	}
	if (!sleepPatchOffset) {
		dbg_printf("Thumb sleep patch alt not found\n");
	} else {
		dbg_printf("Thumb sleep patch found\n");
	}
	return sleepPatchOffset;
}

u32 getCardCheckPullOutOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
    u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	u32 cardCheckPullOutOffset = getOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//ndsHeader->arm9binarySize,
		(u32*)cardCheckPullOutSignature, 4,
		1
	);
	if (!cardCheckPullOutOffset) {
		dbg_printf("Card check pull out not found\n");
	} else {
		dbg_printf("Card check pull out found\n");
	}
	return cardCheckPullOutOffset;
}

u32 getCardIrqEnableOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
    u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32 cardIrqEnableOffset = getOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		(u32*)irqEnableStartSignature, 4,
		1
	);
	if (!cardIrqEnableOffset) {
		dbg_printf("irq enable not found\n");
	} else {
		dbg_printf("irq enable found\n");
	}
	return cardIrqEnableOffset;
}
