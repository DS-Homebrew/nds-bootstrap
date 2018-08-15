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

static const u32 swiHaltSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00007BAF};
static const u32 swiHaltSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B7A3};
static const u32 swiHaltSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B837};
static const u32 swiHaltSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B92B};
static const u32 swiHaltSignature1Alt4[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000BAEB};
static const u32 swiHaltSignature1Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803B93};
static const u32 swiHaltSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DAF};
static const u32 swiHaltSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DB3};
static const u32 swiHaltSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ECB};
static const u32 swiHaltSignature1Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F13};
static const u32 swiHaltSignature1Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03804133};
static const u32 swiHaltSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800F7F};
static const u32 swiHaltSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x038010F3};
static const u32 swiHaltSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x038011BF};
static const u32 swiHaltSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803597};
static const u32 swiHaltSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038040C3};
static const u32 swiHaltSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038042AB};
static const u32 swiHaltSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x038042AF};
static const u32 swiHaltSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x0380433F};
static const u32 swiHaltSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x038043E3};
static const u32 swiHaltSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804503};
static const u32 swiHaltSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038045BF};
static const u32 swiHaltSignature3Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x0380538B};
static const u32 swiHaltSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x0380064B};
static const u32 swiHaltSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x038008C3};
static const u32 swiHaltSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x038008CF};
static const u32 swiHaltSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x0380356F};
static const u32 swiHaltSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038036BF};
static const u32 swiHaltSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038037D3};
static const u32 swiHaltSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E7F};
static const u32 swiHaltSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803EBF};
static const u32 swiHaltSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804503};
static const u32 swiHaltSignature5[3]      = {0xE59FC000, 0xE12FFF1C, 0x037FB2DB}; // SDK 5
static const u32 swiHaltSignature5Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x037FB51F}; // SDK 5
static const u32 swiHaltSignature5Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x037FB5E3}; // SDK 5
static const u32 swiHaltSignature5Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x037FB6FB}; // SDK 5
static const u16 swiHaltSignatureThumb[2]  = {0xDF06, 0x4770};                     // SDK 5
//static const u16 swiHaltSignatureThumb5[4] = {0x004B, 0x4718, 0xB463, 0x037F};     // SDK 5

// Sleep patch
static const u32 sleepPatch[2]         = {0x0A000001, 0xE3A00601};
static const u16 sleepPatchThumb[2]    = {0xD002, 0x4831};
static const u16 sleepPatchThumbAlt[2] = {0xD002, 0x0440};

// Card check pull out
static const u32 cardCheckPullOutSignature1[4] = {0xE92D4018, 0xE24DD004, 0xE59F204C, 0xE1D210B0}; // SDK != 3
static const u32 cardCheckPullOutSignature3[4] = {0xE92D4000, 0xE24DD004, 0xE59F002C, 0xE1D000B0}; // SDK 3

// irq enable
static const u32 irqEnableStartSignature1[4]    = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0}; // SDK <= 3
static const u32 irqEnableStartSignature4[4]    = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020}; // SDK >= 4
static const u32 irqEnableStartSignature4Alt[4] = {0xE92D4010, 0xE1A04000, 0xEBFFFFE9, 0xE59FC020}; // SDK 5

//static bool sdk5 = false;

u32* findSwi12Offset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSwi12Offset:\n");

	u32* swi12Offset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
		swi12Signature, 1
	);
	if (swi12Offset) {
		dbg_printf("swi 0x12 call found: \n");
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

	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}

	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
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
	}
	if (!swiGetPitchTableOffset) {
		//sdk5 = true;
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
	}

	if (swiGetPitchTableOffset) {
		dbg_hexa((u32)swiGetPitchTableOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return swiGetPitchTableOffset;
}

u32* findSwiHaltOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSwiHaltOffset:\n");

	u32* swiHaltOffset = NULL;
	
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1, 4
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt1, 4
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 1 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 1 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt2, 4
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 2 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 2 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt3, 4
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 3 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 3 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt4, 4
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 4 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 4 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt5, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 5 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 5 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt6, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 6 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 6 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt7, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 7 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 7 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt8, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 8 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 8 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt9, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 9 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 9 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version < 0x3000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature1Alt10, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK <= 2 call alt 10 found: ");
			} else {
				dbg_printf("swiHalt SDK <= 2 call alt 10 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt1, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 1 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 1 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt2, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 2 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 2 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt3, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 3 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 3 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt4, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 4 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 4 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt5, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 5 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 5 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt6, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 6 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 6 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt7, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 7 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 7 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt8, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 8 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 8 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt9, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 9 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 9 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt10, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 10 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 10 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature3Alt11, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 3 call alt 11 found: ");
			} else {
				dbg_printf("swiHalt SDK 3 call alt 11 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt1, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 1 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 1 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt2, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 2 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 2 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt3, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 3 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 3 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt4, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 4 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 4 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt5, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 5 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 5 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt6, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 6 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 6 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt7, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 7 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 7 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x4000000 && moduleParams->sdk_version < 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
				swiHaltSignature4Alt8, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 4 call alt 8 found: ");
			} else {
				dbg_printf("swiHalt SDK 4 call alt 8 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				swiHaltSignature5, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 5 call found: ");
			} else {
				dbg_printf("swiHalt SDK 5 call not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				swiHaltSignature5Alt1, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 5 call alt 1 found: ");
			} else {
				dbg_printf("swiHalt SDK 5 call alt 1 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				swiHaltSignature5Alt2, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 5 call alt 2 found: ");
			} else {
				dbg_printf("swiHalt SDK 5 call alt 2 not found\n");
			}
		}
	}
	if (!swiHaltOffset) {
		if (moduleParams->sdk_version > 0x5000000) {
			swiHaltOffset = findOffset(
				(u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				swiHaltSignature5Alt3, 3
			);
			if (swiHaltOffset) {
				dbg_printf("swiHalt SDK 5 call alt 3 found: ");
			} else {
				dbg_printf("swiHalt SDK 5 call alt 3 not found\n");
			}
		}
	}

	if (swiHaltOffset) {
		dbg_hexa((u32)swiHaltOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return swiHaltOffset;
}

// SDK 5
u16* findSwiHaltOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findSwiHaltOffsetThumb:\n");

	u16* swiHaltOffset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
		swiHaltSignatureThumb, 2
	);
	if (swiHaltOffset) {
		dbg_printf("swiHalt SDK 5 call thumb found: ");
	} else {
		dbg_printf("swiHalt SDK 5 call thumb not found\n");
	}

	if (swiHaltOffset) {
		dbg_hexa((u32)swiHaltOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return swiHaltOffset;
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

u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardCheckPullOutOffset:\n");
	
	const u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
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
            irqEnableStartSignature4Alt, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable alt found: ");
		} else {
			dbg_printf("irq enable alt not found\n");
		}
	}

	if (cardIrqEnableOffset) {
		dbg_hexa((u32)cardIrqEnableOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIrqEnableOffset;
}
