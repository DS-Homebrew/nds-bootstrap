#include <stddef.h> // NULL
#include "find.h"
#include "debug_file.h"

//
// Subroutine function signatures ARM7
//

static const u32 swi12Signature[1] = {0x4770DF12}; // LZ77UnCompReadByCallbackWrite16bit

static const u32 swiGetPitchTableSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004721};
static const u32 swiGetPitchTableSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BB9};
static const u32 swiGetPitchTableSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BC9};
static const u32 swiGetPitchTableSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BE5};
static const u32 swiGetPitchTableSignature1Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803BE9};
static const u32 swiGetPitchTableSignature1Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E05};
static const u32 swiGetPitchTableSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E09};
static const u32 swiGetPitchTableSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F21};
static const u32 swiGetPitchTableSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804189};
static const u32 swiGetPitchTableSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800FD5};
static const u32 swiGetPitchTableSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801149};
static const u32 swiGetPitchTableSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801215};
static const u32 swiGetPitchTableSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804119};
static const u32 swiGetPitchTableSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804301};
static const u32 swiGetPitchTableSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804305};
static const u32 swiGetPitchTableSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804395};
static const u32 swiGetPitchTableSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804439};
static const u32 swiGetPitchTableSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804559};
static const u32 swiGetPitchTableSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804615};
static const u32 swiGetPitchTableSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038053E1};
static const u32 swiGetPitchTableSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x038006A1};
static const u32 swiGetPitchTableSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800919};
static const u32 swiGetPitchTableSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800925};
static const u32 swiGetPitchTableSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035C5};
static const u32 swiGetPitchTableSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035ED};
static const u32 swiGetPitchTableSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803715};
static const u32 swiGetPitchTableSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803829};
static const u32 swiGetPitchTableSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ED5};
static const u32 swiGetPitchTableSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F15};
static const u32 swiGetPitchTableSignature5[4]      = {0x781A4B06, 0xD3030791, 0xD20106D1, 0x1A404904};

// Sleep patch
static const u32 sleepPatch[2]         = {0x0A000001, 0xE3A00601};
static const u16 sleepPatchThumb[2]    = {0xD002, 0x4831};
static const u16 sleepPatchThumbAlt[2] = {0xD002, 0x0440};

// RAM clear
static const u32 ramClearSignature[2] = {0x02FFC000, 0x02FFF000};

// Card check pull out
static const u32 cardCheckPullOutSignature1[4] = {0xE92D4000, 0xE24DD004, 0xE59F00B4, 0xE5900000}; // Pokemon Dash, early sdk2
static const u32 cardCheckPullOutSignature2[4] = {0xE92D4018, 0xE24DD004, 0xE59F204C, 0xE1D210B0}; // SDK != 3
static const u32 cardCheckPullOutSignature3[4] = {0xE92D4000, 0xE24DD004, 0xE59F002C, 0xE1D000B0}; // SDK 3

// irq enable
static const u32 irqEnableStartSignature1[4]    = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0}; // SDK <= 3
static const u32 irqEnableStartSignature4[4]    = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020}; // SDK >= 4
static const u32 irqEnableStartSignature4Alt1[4] = {0xE92D4010, 0xE1A04000, 0xEBFFFFE9, 0xE59FC020}; // SDK 5
static const u32 irqEnableStartSignature4Alt2[4] = {0xE92D4010, 0xE1A04000, 0xEB00122B, 0xE59F2030}; // SDK 5

//static bool sdk5 = false;

u32* findSwi12Offset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSwi12Offset:\n");

	u32* swi12Offset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
		swi12Signature, 1
	);
	if (swi12Offset) {
		dbg_printf("swi 0x12 call found: ");
	} else {
		dbg_printf("swi 0x12 call not found\n");
	}

	if (swi12Offset) {
		dbg_hexa((u32)swi12Offset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return swi12Offset;
}

u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSwiGetPitchTableOffset:\n");

	u32* swiGetPitchTableOffset = NULL;

	if (moduleParams->sdk_version > 0x5000000) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature5, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable call SDK 5 found: ");
		} else {
			dbg_printf("swiGetPitchTable call SDK 5 not found\n");
		}
	}

	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call not found\n");
		}
	}

	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt1, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 1 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 1 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt2, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 2 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 2 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt3, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 3 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 3 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 4 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 4 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt5, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 5 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 5 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt6, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 6 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 6 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt7, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 7 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 7 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt8, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 8 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 8 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt1, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 1 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 1 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt2, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 2 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 2 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt3, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 3 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 3 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 4 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 4 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt5, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 5 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 5 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt6, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 6 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 6 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt7, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 7 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 7 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt8, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 8 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 8 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt9, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 9 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 9 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt10, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 10 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 10 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt1, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 1 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 1 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt2, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 2 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 2 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt3, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 3 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 3 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 4 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 4 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt5, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 5 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 5 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt6, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 6 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 6 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt7, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 7 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 7 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt8, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 8 found: ");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 8 not found\n");
		}
	}

	if (swiGetPitchTableOffset) {
		dbg_hexa((u32)swiGetPitchTableOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return swiGetPitchTableOffset;
}

u32* findSleepPatchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSleepPatchOffset:\n");

	u32* sleepPatchOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		sleepPatch, 2
	);
	if (sleepPatchOffset) {
		dbg_printf("Sleep patch found: ");
	} else {
		dbg_printf("Sleep patch not found\n");
	}

	if (sleepPatchOffset) {
		dbg_hexa((u32)sleepPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return sleepPatchOffset;
}

u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findSleepPatchOffsetThumb:\n");
	
	u16* sleepPatchOffset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		sleepPatchThumb, 2
	);
	if (sleepPatchOffset) {
		dbg_printf("Thumb sleep patch thumb found: ");
	} else {
		dbg_printf("Thumb sleep patch thumb not found\n");
	}

	if (!sleepPatchOffset) {
		sleepPatchOffset = findOffsetThumb(
			(u16*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
			sleepPatchThumbAlt, 2
		);
		if (sleepPatchOffset) {
			dbg_printf("Thumb sleep patch thumb alt found: ");
		} else {
			dbg_printf("Thumb sleep patch thumb alt not found\n");
		}
	}

	if (sleepPatchOffset) {
		dbg_hexa((u32)sleepPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return sleepPatchOffset;
}

u32* findRamClearOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRamClearOffset:\n");

	u32* ramClearOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00030000,
		ramClearSignature, 2
	);
	if (ramClearOffset) {
		dbg_printf("RAM clear found: ");
		dbg_hexa((u32)ramClearOffset);
		dbg_printf("\n");
	} else {
		dbg_printf("RAM clear not found\n");
	}

	dbg_printf("\n");
	return ramClearOffset;
}

u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardCheckPullOutOffset:\n");
	
	const u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
    if (moduleParams->sdk_version > 0x2004FFF && moduleParams->sdk_version < 0x3000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature2;
    } else if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	u32* cardCheckPullOutOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//ndsHeader->arm9binarySize,
		cardCheckPullOutSignature, 4
	);
	if (cardCheckPullOutOffset) {
		dbg_printf("Card check pull out found: ");
	} else {
		dbg_printf("Card check pull out not found\n");
	}

	if (cardCheckPullOutOffset) {
		dbg_hexa((u32)cardCheckPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardCheckPullOutOffset;
}

u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardIrqEnableOffset:\n");
	
	const u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32* cardIrqEnableOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		irqEnableStartSignature, 4
	);
	if (cardIrqEnableOffset) {
		dbg_printf("irq enable found: ");
	} else {
		dbg_printf("irq enable not found\n");
	}

	if (!cardIrqEnableOffset) {
		// SDK 5
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature4Alt1, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable alt 1 found: ");
		} else {
			dbg_printf("irq enable alt 1 not found\n");
		}
	}

	if (!cardIrqEnableOffset) {
		// SDK 5
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature4Alt2, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable alt 2 found: ");
		} else {
			dbg_printf("irq enable alt 2 not found\n");
		}
	}

	if (cardIrqEnableOffset) {
		dbg_hexa((u32)cardIrqEnableOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIrqEnableOffset;
}
