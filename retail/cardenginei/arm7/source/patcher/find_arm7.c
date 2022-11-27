#include <stddef.h> // NULL
#include "patch.h"
#include "find.h"
//#include "debug_file.h"

//
// Subroutine function signatures ARM7
//

static const u32 relocateStartSignature[1] = {0x027FFFFA};

static const u32 nextFunctiontSignature[1] = {0xE92D4000};
// static const u32 relocateValidateSignature[1] = {0x400010C};

static const u32 swi12Signature[1] = {0x4770DF12}; // LZ77UnCompReadByCallbackWrite16bit

static const u16 swiGetPitchTableSignatureThumb[4]    = {0xB570, 0x1C05, 0x2400, 0x4248};
static const u16 swiGetPitchTableSignatureThumbAlt[4] = {0xB570, 0x1C05, 0x4248, 0x2103};
static const u32 swiGetPitchTableSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004721};
static const u32 swiGetPitchTableSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004ACD};
static const u32 swiGetPitchTableSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BB9};
static const u32 swiGetPitchTableSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BC9};
static const u32 swiGetPitchTableSignature1Alt4[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BE5};
static const u32 swiGetPitchTableSignature1Alt5[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004CA5};
static const u32 swiGetPitchTableSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x038039D5};
static const u32 swiGetPitchTableSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803BE9};
static const u32 swiGetPitchTableSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E05};
static const u32 swiGetPitchTableSignature1Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E09};
static const u32 swiGetPitchTableSignature1Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03803F21};
static const u32 swiGetPitchTableSignature1Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x03804189};
static const u32 swiGetPitchTableSignature1Alt12[3] = {0xE59FC000, 0xE12FFF1C, 0x038049D5};
static const u32 swiGetPitchTableSignature1Alt13[3] = {0xE59FC000, 0xE12FFF1C, 0x03804BE9};
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
static const u32 swiGetPitchTableSignature3Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x03805485};
static const u32 swiGetPitchTableSignature3Alt12[3] = {0xE59FC000, 0xE12FFF1C, 0x038055A5};
static const u32 swiGetPitchTableSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x038006A1};
static const u32 swiGetPitchTableSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800811};
static const u32 swiGetPitchTableSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800919};
static const u32 swiGetPitchTableSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800925};
static const u32 swiGetPitchTableSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035C5};
static const u32 swiGetPitchTableSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035ED};
static const u32 swiGetPitchTableSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803715};
static const u32 swiGetPitchTableSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803829};
static const u32 swiGetPitchTableSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ED5};
static const u32 swiGetPitchTableSignature4Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F15};

// Sleep patch
static const u32 sleepPatch[2]         = {0x0A000001, 0xE3A00601};
static const u16 sleepPatchThumb[2]    = {0xD002, 0x4831};
static const u16 sleepPatchThumbAlt[2] = {0xD002, 0x0440};

// Card check pull out
static const u32 cardCheckPullOutSignature1[4] = {0xE92D4000, 0xE24DD004, 0xE59F00B4, 0xE5900000}; // Pokemon Dash, early sdk2
static const u32 cardCheckPullOutSignature2[4] = {0xE92D4018, 0xE24DD004, 0xE59F204C, 0xE1D210B0}; // SDK != 3
static const u32 cardCheckPullOutSignature3[4] = {0xE92D4000, 0xE24DD004, 0xE59F002C, 0xE1D000B0}; // SDK 3

// irq enable
static const u32 irqEnableStartSignature1[4]      = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0}; // SDK <= 3
static const u32 irqEnableStartSignature4[4]      = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020}; // SDK >= 4

bool a7GetReloc(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 vAddrOfRelocSrc;
	extern u32 relocDestAtSharedMem;

	// Find the relocation signature
    u32 relocationStart = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		relocateStartSignature, 1
	);
	if (!relocationStart) {
		//dbg_printf("Relocation start not found\n");
		return false;
	}

    // Validate the relocation signature
	u32 forwardedRelocStartAddr = relocationStart + 4;
	while (!*(u32*)forwardedRelocStartAddr || *(u32*)forwardedRelocStartAddr < 0x02000000 || *(u32*)forwardedRelocStartAddr > 0x03000000) {
		forwardedRelocStartAddr += 4;
	}
	vAddrOfRelocSrc = *(u32*)(forwardedRelocStartAddr + 8);
    
    /*dbg_printf("forwardedRelocStartAddr\n");
    dbg_hexa(forwardedRelocStartAddr);   
    dbg_printf("\nvAddrOfRelocSrc\n");
    dbg_hexa(vAddrOfRelocSrc);
    dbg_printf("\n");*/
	
	// Sanity checks
	u32 relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
	u32 relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
	if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
		//dbg_printf("Error in relocation checking method 1\n");

		// Find the beginning of the next function
		u32 nextFunction = (u32)findOffset(
			(u32*)relocationStart, ndsHeader->arm7binarySize,
			nextFunctiontSignature, 1
		);
	
		// Validate the relocation signature
		forwardedRelocStartAddr = nextFunction - 0x14;
		
		// Validate the relocation signature
		vAddrOfRelocSrc = *(u32*)(nextFunction - 0xC);
		
		// Sanity checks
		relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
		relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
		if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
			//dbg_printf("Error in relocation checking method 2\n");
			return false;
		}
	}

	// Get the remaining details regarding relocation
	u32 valueAtRelocStart = *(u32*)forwardedRelocStartAddr;
	relocDestAtSharedMem = *(u32*)valueAtRelocStart;
	if (relocDestAtSharedMem != 0x37F8000) { // Shared memory in RAM
		// Try again
		vAddrOfRelocSrc += *(u32*)(valueAtRelocStart + 4);
		relocDestAtSharedMem = *(u32*)(valueAtRelocStart + 0xC);
		if (relocDestAtSharedMem != 0x37F8000) {
			//dbg_printf("Error in finding shared memory relocation area\n");
			return false;
		}
	}

	/*dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");
	dbg_printf("Relocation dst: ");
	dbg_hexa(relocDestAtSharedMem);
	dbg_printf("\n");*/

	return true;
}

u32* a7_findSwi12Offset(const tNDSHeader* ndsHeader) {
	//dbg_printf("findSwi12Offset:\n");

	u32* swi12Offset = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
		swi12Signature, 1
	);
	/*if (swi12Offset) {
		dbg_printf("swi 0x12 call found\n");
	} else {
		dbg_printf("swi 0x12 call not found\n");
	}

	dbg_printf("\n");*/
	return swi12Offset;
}

u16* findSwiGetPitchTableThumbBranchOffset(const tNDSHeader* ndsHeader) {
	//dbg_printf("findSwiGetPitchTableThumbOffset:\n");

	u16* offset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		swiGetPitchTableSignatureThumb, 4
	);
	if (!offset) {
		offset = findOffsetThumb(
			(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			swiGetPitchTableSignatureThumbAlt, 4
		);
	}

	/*if (offset) {
		dbg_printf("swiGetPitchTable thumb branch found\n");
	} else {
		dbg_printf("swiGetPitchTable thumb branch not found\n");
	}

	dbg_printf("\n");*/
	return offset;
}

u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	//dbg_printf("findSwiGetPitchTableOffset:\n");

	u32* swiGetPitchTableOffset = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
		swiGetPitchTableSignature1, 4
	);
	/*if (swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK <= 2 call found\n");
	} else {
		dbg_printf("swiGetPitchTable SDK <= 2 call not found\n");
	}*/

	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt1, 4
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 1 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 1 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt2, 4
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 2 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 2 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt3, 4
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 3 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 3 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt4, 4
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 4 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 4 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt5, 4
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 5 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt6, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 6 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 6 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt7, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 7 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 7 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt8, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 8 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 8 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt9, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 9 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 9 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt10, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 10 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 10 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt11, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 11 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 11 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt12, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 12 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 12 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt13, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 13 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 13 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt1, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 1 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 1 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt2, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 2 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 2 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt3, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 3 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 3 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt4, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 4 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 4 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt5, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 5 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt6, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 6 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 6 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt7, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 7 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 7 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt8, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 8 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 8 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt9, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 9 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 9 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt10, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 10 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 10 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt11, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 11 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 11 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt12, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 12 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 12 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt1, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 1 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 1 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt2, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 2 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 2 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt3, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 3 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 3 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt4, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 4 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 4 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt5, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 5 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt6, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 6 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 6 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt7, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 7 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 7 not found\n");
		}*/
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt8, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 8 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 8 not found\n");
		}*/
	}

	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize > 0x10000 ? 0x10000 : ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt9, 3
		);
		/*if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 9 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 9 not found\n");
		}*/
	}

	//dbg_printf("\n");
	return swiGetPitchTableOffset;
}

u32* findSleepPatchOffset(const tNDSHeader* ndsHeader) {
	//dbg_printf("findSleepPatchOffset:\n");

	u32* sleepPatchOffset = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		sleepPatch, 2
	);
	/*if (sleepPatchOffset) {
		dbg_printf("Sleep patch found: ");
	} else {
		dbg_printf("Sleep patch not found\n");
	}

	if (sleepPatchOffset) {
		dbg_hexa((u32)sleepPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");*/
	return sleepPatchOffset;
}

u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader) {
	//dbg_printf("findSleepPatchOffsetThumb:\n");
	
	u16* sleepPatchOffset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		sleepPatchThumb, 2
	);
	/*if (sleepPatchOffset) {
		dbg_printf("Thumb sleep patch thumb found: ");
	} else {
		dbg_printf("Thumb sleep patch thumb not found\n");
	}*/

	if (!sleepPatchOffset) {
		sleepPatchOffset = findOffsetThumb(
			(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			sleepPatchThumbAlt, 2
		);
		/*if (sleepPatchOffset) {
			dbg_printf("Thumb sleep patch thumb alt found: ");
		} else {
			dbg_printf("Thumb sleep patch thumb alt not found\n");
		}*/
	}

	/*if (sleepPatchOffset) {
		dbg_hexa((u32)sleepPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");*/
	return sleepPatchOffset;
}

u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	//dbg_printf("findCardCheckPullOutOffset:\n");
	
	const u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
    if (moduleParams->sdk_version > 0x2004FFF && moduleParams->sdk_version < 0x3000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature2;
    } else if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	u32* cardCheckPullOutOffset = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		cardCheckPullOutSignature, 4
	);
	/*if (cardCheckPullOutOffset) {
		dbg_printf("Card check pull out found: ");
	} else {
		dbg_printf("Card check pull out not found\n");
	}

	if (cardCheckPullOutOffset) {
		dbg_hexa((u32)cardCheckPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");*/
	return cardCheckPullOutOffset;
}

u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	//dbg_printf("findCardIrqEnableOffset:\n");
	
	const u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32* cardIrqEnableOffset = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		irqEnableStartSignature, 4
	);
	/*if (cardIrqEnableOffset) {
		dbg_printf("irq enable found\n");
	} else {
		dbg_printf("irq enable not found\n");
	}*/

	if (!cardIrqEnableOffset && moduleParams->sdk_version < 0x4000000) {
		// SDK 4
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
            irqEnableStartSignature4, 4
		);
		/*if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 4 found\n");
		} else {
			dbg_printf("irq enable SDK 4 not found\n");
		}*/
	}

	//dbg_printf("\n");
	return cardIrqEnableOffset;
}
