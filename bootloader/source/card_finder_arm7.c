#include "card_finder.h"
#include "debugToFile.h"

//
// Subroutine function signatures ARM7
//

u32 swi12Signature[1] = {0x4770DF12}; // LZ77UnCompReadByCallbackWrite16bit

u32 swiGetPitchTableSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004721};
u32 swiGetPitchTableSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BB9};
u32 swiGetPitchTableSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BC9};
u32 swiGetPitchTableSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BE5};
u32 swiGetPitchTableSignature1Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803BE9};
u32 swiGetPitchTableSignature1Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E05};
u32 swiGetPitchTableSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E09};
u32 swiGetPitchTableSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F21};
u32 swiGetPitchTableSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804189};
u32 swiGetPitchTableSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800FD5};
u32 swiGetPitchTableSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801149};
u32 swiGetPitchTableSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801215};
u32 swiGetPitchTableSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804119};
u32 swiGetPitchTableSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804301};
u32 swiGetPitchTableSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804305};
u32 swiGetPitchTableSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804395};
u32 swiGetPitchTableSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804439};
u32 swiGetPitchTableSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804559};
u32 swiGetPitchTableSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804615};
u32 swiGetPitchTableSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038053E1};
u32 swiGetPitchTableSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x038006A1};
u32 swiGetPitchTableSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800919};
u32 swiGetPitchTableSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800925};
u32 swiGetPitchTableSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035C5};
u32 swiGetPitchTableSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035ED};
u32 swiGetPitchTableSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803715};
u32 swiGetPitchTableSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803829};
u32 swiGetPitchTableSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ED5};
u32 swiGetPitchTableSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F15};
u32 swiGetPitchTableSignature5[4]      = {0x781A4B06, 0xD3030791, 0xD20106D1, 0x1A404904};

u32 swiHaltSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00007BAF};
u32 swiHaltSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B7A3};
u32 swiHaltSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B837};
u32 swiHaltSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B92B};
u32 swiHaltSignature1Alt4[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000BAEB};
u32 swiHaltSignature1Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803B93};
u32 swiHaltSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DAF};
u32 swiHaltSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DB3};
u32 swiHaltSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ECB};
u32 swiHaltSignature1Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803F13};
u32 swiHaltSignature1Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03804133};
u32 swiHaltSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800F7F};
u32 swiHaltSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x038010F3};
u32 swiHaltSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x038011BF};
u32 swiHaltSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803597};
u32 swiHaltSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038040C3};
u32 swiHaltSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038042AB};
u32 swiHaltSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x038042AF};
u32 swiHaltSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x0380433F};
u32 swiHaltSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x038043E3};
u32 swiHaltSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804503};
u32 swiHaltSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038045BF};
u32 swiHaltSignature3Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x0380538B};
u32 swiHaltSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x0380064B};
u32 swiHaltSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x038008C3};
u32 swiHaltSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x038008CF};
u32 swiHaltSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x0380356F};
u32 swiHaltSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038036BF};
u32 swiHaltSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038037D3};
u32 swiHaltSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E7F};
u32 swiHaltSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803EBF};
u32 swiHaltSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804503};

// Sleep patch
u32 sleepPatch[2]         = {0x0A000001, 0xE3A00601};
u16 sleepPatchThumb[2]    = {0xD002, 0x4831};
u16 sleepPatchThumbAlt[2] = {0xD002, 0x0440};

// Card check pull out
u32 cardCheckPullOutSignature1[4] = {0xE92D4018, 0xE24DD004, 0xE59F204C, 0xE1D210B0};
u32 cardCheckPullOutSignature3[4] = {0xE92D4000, 0xE24DD004, 0xE59F002C, 0xE1D000B0};

// irq enable
u32 irqEnableStartSignature1[4] = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0};
u32 irqEnableStartSignature4[4] = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020};

//bool sdk5 = false;

u32* findSwi12Offset(const tNDSHeader* ndsHeader) {
	u32* swi12Offset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
		swi12Signature, 1
	);
	if (swi12Offset) {
		dbg_printf("swi 0x12 call found\n");
	} else {
		dbg_printf("swi 0x12 call not found\n");
	}
	return swi12Offset;
}

u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader) {
	u32* swiGetPitchTableOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
		swiGetPitchTableSignature1, 4
	);
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt1, 4
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 1 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt2, 4
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 2 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt3, 4
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 3 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt4, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 4 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt5, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 5 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt6, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 6 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt7, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 7 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature1Alt8, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 8 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt1, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 1 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt2, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 2 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt3, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 3 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt4, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 4 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt5, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 5 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt6, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 6 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt7, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 7 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt8, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 8 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt9, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 9 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature3Alt10, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 10 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt1, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 1 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt2, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 2 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt3, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 3 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt4, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 4 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt5, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 5 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt6, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 6 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt7, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 7 not found\n");
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature4Alt8, 3
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 8 not found\n");
		//sdk5 = true;
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00010000,//ndsHeader->arm7binarySize,
			swiGetPitchTableSignature5, 4
		);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable call SDK5 not found\n");
	} else {
		dbg_printf("swiGetPitchTable call found\n");
	}
	return swiGetPitchTableOffset;
}

u32* findSwiHaltOffset(const tNDSHeader* ndsHeader) {
	u32* swiHaltOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
		swiHaltSignature1, 4
		);
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt1, 4
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 1 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt2, 4
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 2 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt3, 4
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 3 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt4, 4
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 4 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt5, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 5 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt6, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 6 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt7, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 7 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt8, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 8 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt9, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 9 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature1Alt10, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 10 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt1, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 1 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt2, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 2 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt3, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 3 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt4, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 4 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt5, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 5 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt6, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 6 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt7, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 7 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt8, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 8 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt9, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 9 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt10, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 10 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature3Alt11, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 11 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt1, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 1 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt2, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 2 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt3, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 3 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt4, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 4 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt5, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 5 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt6, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 6 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt7, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 7 not found\n");
		swiHaltOffset = findOffset(
			(u32*)ndsHeader->arm7destination, 0x00002000,//ndsHeader->arm7binarySize,
			swiHaltSignature4Alt8, 3
		);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 8 not found\n");
	} else {
		dbg_printf("swiHalt call found\n");
	}
	return swiHaltOffset;
}

u32* findSleepPatchOffset(const tNDSHeader* ndsHeader) {
	u32* sleepPatchOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		sleepPatch, 2
	);
	if (sleepPatchOffset) {
		dbg_printf("Sleep patch found\n");
	} else {
		dbg_printf("Sleep patch not found\n");
	}
	return sleepPatchOffset;
}

u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("Trying thumb\n");
	u16* sleepPatchOffset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		sleepPatchThumb, 2
	);
	if (!sleepPatchOffset) {
		dbg_printf("Thumb sleep patch not found. Trying alt\n");
		sleepPatchOffset = findOffsetThumb(
			(u16*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
			sleepPatchThumbAlt, 2
		);
	}
	if (sleepPatchOffset) {
		dbg_printf("Thumb sleep patch found\n");
	} else {
		dbg_printf("Thumb sleep patch alt not found\n");
	}
	return sleepPatchOffset;
}

u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
	if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	u32* cardCheckPullOutOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//ndsHeader->arm9binarySize,
		cardCheckPullOutSignature, 4
	);
	if (cardCheckPullOutOffset) {
		dbg_printf("Card check pull out found\n");
	} else {
		dbg_printf("Card check pull out not found\n");
	}
	return cardCheckPullOutOffset;
}

u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32* cardIrqEnableOffset = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
		irqEnableStartSignature, 4
	);
	if (cardIrqEnableOffset) {
		dbg_printf("irq enable found\n");
	} else {
		dbg_printf("irq enable not found\n");
	}
	return cardIrqEnableOffset;
}
