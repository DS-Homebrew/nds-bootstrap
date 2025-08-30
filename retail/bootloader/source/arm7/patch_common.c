/*
	NitroHax -- Cheat tool for the Nintendo DS
	Copyright (C) 2008  Michael "Chishm" Chisholm

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//#include <stddef.h>
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"
#include "patch.h"
#include "common.h"
#include "locations.h"
#include "tonccpy.h"
#include "loading_screen.h"
#include "debug_file.h"

u16 patchOffsetCacheFilePrevCrc = 0;
u16 patchOffsetCacheFileNewCrc = 0;

patchOffsetCacheContents patchOffsetCache;

extern u32 srlAddr;

static inline void doubleNopT(u32 addr) {
	*(u16*)(addr)   = 0x46C0;
	*(u16*)(addr+2) = 0x46C0;
}

void patchBinary(cardengineArm9* ce9, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	extern u32 accessControl;
	if (ndsHeader->unitCode == 3 || (accessControl & BIT(4))) {
		extern void patchDSiModeToDSMode(cardengineArm9* ce9, const tNDSHeader* ndsHeader);
		patchDSiModeToDSMode(ce9, ndsHeader);
		return;
	}

	const char* romTid = getRomTid(ndsHeader);

#ifdef LOADERTYPE0
	// Animal Crossing: Wild World
	if ((strncmp(romTid, "ADM", 3) == 0 || strncmp(romTid, "A62", 3) == 0) && !extendedMemory) {
		int instancesPatched = 0;
		u32 addrOffset = (u32)ndsHeader->arm9destination;
		while (instancesPatched < 3) {
			if (*(u32*)addrOffset >= 0x023FF000 && *(u32*)addrOffset < 0x023FF020) {
				extern u8 _io_dldi_size;
				if (_io_dldi_size == 0x0E) {
					*(u32*)addrOffset = 0x023DC000;
				} else if (_io_dldi_size == 0x0F) {
					*(u32*)addrOffset = 0x023D8000;
				} else {
					//*(u32*)addrOffset -= 0x2000;
					*(u32*)addrOffset = CARDENGINE_ARM9_LOCATION_DLDI_ALT2;
				}
				instancesPatched++;
			}
			addrOffset += 4;
			if (addrOffset > (u32)ndsHeader->arm9destination+ndsHeader->arm9binarySize) break;
		}
	}

	// The World Ends With You (USA/Europe)
	else if (strcmp(romTid, "AWLE") == 0 || strcmp(romTid, "AWLP") == 0) {
		*(u32*)0x203E7B0 = 0;
	}

	// Subarashiki Kono Sekai - It's a Wonderful World (Japan)
	else if (strcmp(romTid, "AWLJ") == 0) {
		*(u32*)0x203F114 = 0;
	}

	// Miami Nights - Singles in the City (USA)
	else if (strcmp(romTid, "AVWE") == 0) {
		// Fix not enough memory error
		*(u32*)0x0204CCCC = 0xe1a00000; //nop
	}

	// Miami Nights - Singles in the City (Europe)
	else if (strcmp(romTid, "AVWP") == 0) {
		// Fix not enough memory error
		*(u32*)0x0204CDBC = 0xe1a00000; //nop
	}

	// Castlevania - Portrait of Ruin (USA)
	else if (strcmp(romTid, "ACBE") == 0) {
		*(u32*)0x02007910 = 0xeb02508e;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025052;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}

	// Akumajou Dracula - Gallery of Labyrinth (Japan)
	else if (strcmp(romTid, "ACBJ") == 0) {
		if (ndsHeader->romversion == 0) {
			*(u32*)0x02007910 += 5;
			*(u32*)0x02007918 += 0xd0000000; // bne -> b
			*(u32*)0x02007a00 += 5;
			*(u32*)0x02007a08 += 0xe0000000; // ldreq -> ldr
			*(u32*)0x02007a0c += 0xe0000000; // ldreq -> ldr
			*(u32*)0x02007a10 += 0xe0000000; // mlaeq -> mla
			*(u32*)0x02007a14 += 0xe0000000; // beq -> b
		} else {
			*(u32*)0x0200753C += 5;
			*(u32*)0x02007544 += 0xd0000000; // bne -> b
			*(u32*)0x02007624 += 5;
			*(u32*)0x0200762C += 0xd0000000; // bne -> b
		}
	}

	// Castlevania - Portrait of Ruin (Europe) (En,Fr,De,Es,It)
	else if (strcmp(romTid, "ACBP") == 0) {
		*(u32*)0x02007b00 = 0xeb025370;
		*(u32*)0x02007b08 = 0xea000004;
		*(u32*)0x02007bf0 = 0xeb025334;
		*(u32*)0x02007bf8 = 0xe59f1030;
		*(u32*)0x02007bfc = 0xe59f0028;
		*(u32*)0x02007c00 = 0xe0281097;
		*(u32*)0x02007c04 = 0xea000003;
	}

	// Catan (Europe) (En,De)
	else if (strcmp(romTid, "CN7P") == 0) {
		*(u32*)0x02000bc0 = 0xe3540000;
		*(u32*)0x02000bc4 = 0x12441001;
		*(u32*)0x02000bc8 = 0x112fff1e;
		*(u32*)0x02000bcc = 0xe28dd070;
		*(u32*)0x02000bd0 = 0xe3a00000;
		*(u32*)0x02000bd4 = 0xe8bd8ff8;
		*(u32*)0x0207a9f0 = 0xebfe1872;
	}

	// De Kolonisten van Catan (Netherlands)
	else if (strcmp(romTid, "CN7H") == 0) {
		*(u32*)0x02000bc0 = 0xe3540000;
		*(u32*)0x02000bc4 = 0x12441001;
		*(u32*)0x02000bc8 = 0x112fff1e;
		*(u32*)0x02000bcc = 0xe28dd070;
		*(u32*)0x02000bd0 = 0xe3a00000;
		*(u32*)0x02000bd4 = 0xe8bd8ff8;
		*(u32*)0x0207af40 = 0xebfe271e;
	} else
#endif

	// Art Style: DIGIDRIVE (USA) (child.srl)
	// Art Style: INTERSECT (Europe, Australia) (child.srl)
	if (strcmp(romTid, "NTRJ") == 0 && ndsHeader->headerCRC16 == 0x53E2 && srlAddr > 0) {
		*(u32*)0x0200118C = 0x021BD754; // Boot to "mode_select" instead of "connection2"
		for (int i = 0; i < 6; i++) { // Disable bugged description text
			u32* offset1 = (u32*)0x02019C90;
			u32* offset2 = (u32*)0x02021F78;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
		}
		/* for (int i = 0; i < 4; i++) { // Disable bugged title description text
			u32* offset1 = (u32*)0x02019D1C;
			u32* offset2 = (u32*)0x02019D68;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
		} */
		// *(u32*)0x0201CB80 = 0xE1A00000; // nop
		*(u32*)0x0201EFBC = 0xE3A00000; // mov r0, #0
		for (int i = 0; i < 11; i++) { // Disable bugged ranking code
			u32* offset = (u32*)0x0201F050;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x02021338 = 0xE1A00000; // nop
		*(u32*)0x02021498 = 0xE1A00000; // nop
		*(u32*)0x020215F8 = 0xE1A00000; // nop
		*(u32*)0x02021644 += 0x30000000; // blt -> b
		*(u32*)0x0202175C = 0xE1A00000; // nop
		*(u32*)0x020218BC = 0xE1A00000; // nop
		*(u32*)0x02022140 = 0xE1A00000; // nop
	}

	// Art Style: DIGIDRIVE (Japan) (child.srl)
	else if (strcmp(romTid, "NTRJ") == 0 && ndsHeader->headerCRC16 == 0x681E && srlAddr > 0) {
		*(u32*)0x02001024 = 0x021A0948; // Boot to "mode_select" instead of "connection2"
		for (int i = 0; i < 6; i++) { // Disable bugged description text
			u32* offset1 = (u32*)0x02019B40;
			u32* offset2 = (u32*)0x0202230C;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
		}
		*(u32*)0x0201F368 = 0xE3A00000; // mov r0, #0
		for (int i = 0; i < 11; i++) { // Disable bugged ranking code
			u32* offset = (u32*)0x0201F3FC;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x020216CC = 0xE1A00000; // nop
		*(u32*)0x0202182C = 0xE1A00000; // nop
		*(u32*)0x0202198C = 0xE1A00000; // nop
		*(u32*)0x020219D8 += 0x30000000; // blt -> b
		*(u32*)0x02021AF0 = 0xE1A00000; // nop
		*(u32*)0x02021C50 = 0xE1A00000; // nop
	}

#ifdef LOADERTYPE0
	// Ideyou Sukeno: Kenkou Maja DSi (Japan) (main_rom.srl)
	else if (strcmp(romTid, "NTRJ") == 0 && ndsHeader->headerCRC16 == 0xCD01 && srlAddr > 0) {
		*(u32*)0x02000BEC = 0xE3A00001; // mov r0, #1 (Do not wait for other consoles to connect)
	}

	// Power Rangers - Samurai (USA) (En,Fr,Es)
	else if (strcmp(romTid, "B3NE") == 0) {
		*(u32*)0x02060608 = 0xe3a00001; //mov r0, #1
	}

	// Power Rangers - Samurai (Europe) (En,Fr,De,Es,It)
	else if (strcmp(romTid, "B3NP") == 0) {
		*(u32*)0x02060724 = 0xe3a00001; //mov r0, #1
	}

	// Learn with Pokemon - Typing Adventure (Europe)
	// TODO: Relocate new code to 0x023FC400
	/*else if (strcmp(romTid, "UZPP") == 0) {
		*(u32*)0x02000560 = 0xe92d401f;
		*(u32*)0x02000564 = 0xe28f0024;
		*(u32*)0x02000568 = 0xe5901000;
		*(u32*)0x0200056c = 0xe3510001;
		*(u32*)0x02000570 = 0x08bd801f;
		*(u32*)0x02000574 = 0xe5912000;
		*(u32*)0x02000578 = 0xe5903004;
		*(u32*)0x0200057c = 0xe1520003;
		*(u32*)0x02000580 = 0x05904008;
		*(u32*)0x02000584 = 0x05814000;
		*(u32*)0x02000588 = 0xe280000c;
		*(u32*)0x0200058c = 0xeafffff5;
		*(u32*)0x02000590 = 0x020f7c48;
		*(u32*)0x02000594 = 0x0000af81;
		*(u32*)0x02000598 = 0x0000a883;
		*(u32*)0x0200059c = 0x020f83f4;
		*(u32*)0x020005a0 = 0x0000b975;
		*(u32*)0x020005a4 = 0x0000c127;
		*(u32*)0x020005a8 = 0x02105498;
		*(u32*)0x020005ac = 0x02105179;
		*(u32*)0x020005b0 = 0x0200162d;
		*(u32*)0x020005b4 = 0x0210c030;
		*(u32*)0x020005b8 = 0x0210bd11;
		*(u32*)0x020005bc = 0x0200162d;
		*(u32*)0x020005c0 = 0x021022b4;
		*(u32*)0x020005c4 = 0x02101f95;
		*(u32*)0x020005c8 = 0x0200162d;
		*(u32*)0x020005cc = 0x021022d0;
		*(u32*)0x020005d0 = 0x02101ff9;
		*(u32*)0x020005d4 = 0x0200162d;
		*(u32*)0x020005d8 = 0x0210c058;
		*(u32*)0x020005dc = 0x0210be25;
		*(u32*)0x020005e0 = 0x0200162d;
		*(u32*)0x020005e4 = 0x00000001;
		*(u32*)0x020009f8 = 0xeafffed8;
		*(u32*)0x0200147c = 0x46c02800;
		*(u32*)0x02004d30 = 0xf9d8f3f7; // ldmia r8, {r0-r2,r4-r9,r12-pc}
		*(u32*)0x02018f6c = 0x00001000; // andeq r1, r0, r0
		*(u32*)0x02019658 = 0xfcd2f3e2; // ldc2, cp3, crf, [r2], {e2}
		*(u32*)0x0205b24c = 0x63212100; // msrvs cpsr_c, #0
		*(u32*)0x02383f28 = 0xebb000fc; // bl 00f84320
		*(u32*)0x023fc000 = 0x480db4ff; // stmdami sp, {r0-r7,r10,r12-sp,pc}
		*(u32*)0x023fc004 = 0x1c016800; // stcne cp8, cr6, [r1], {0}
		*(u32*)0x023fc008 = 0x31f031f0; // ldrshcc r3, [r0, #10]!
		*(u32*)0x023fc00c = 0x1c0c7809; // stcne cp8, cr7, [r12], {9}
		*(u32*)0x023fc010 = 0x43512278; // cmpmi r1, #80000007
		*(u32*)0x023fc014 = 0x300c1840; // andcc r1, r12, r0, asr #10
		*(u32*)0x023fc018 = 0xf41e2100; // ldr r2, [lr], -#100!
		*(u32*)0x023fc01c = 0x1c01facd; // stcne cpa, crf, [r1], {cd}
		*(u32*)0x023fc020 = 0x02122210; // andeqs r2, r2, #1
		*(u32*)0x023fc024 = 0x1c204354; // stcne ,cp3 cr4, [r0], #-150
		*(u32*)0x023fc028 = 0x6018a355; // andvss r10, r8, r5, asr r3
		*(u32*)0x023fc02c = 0x609a6059; // addvss r6, r10, r9, asr r0
		*(u32*)0x023fc030 = 0x60dc2401; // sbcvss r2, r12, r1, lsl #8
		*(u32*)0x023fc034 = 0x4718bcff; //
		*(u32*)0x023fc038 = 0x020c30dc; // andeq r3 ,r12, #dc
		*(u32*)0x023fc03c = 0xe2810001; // add r0 , r1, #1
		*(u32*)0x023fc040 = 0xe92d401f; // stmdb sp!, {r0-r4,lr}
		*(u32*)0x023fc044 = 0xe59f4140; // ldr r4, 023fc18c
		*(u32*)0x023fc048 = 0xe3540001; // cmp r4, #1
		*(u32*)0x023fc04c = 0x1a000005; // bne 023fc068
		*(u32*)0x023fc050 = 0xe59f0128; // ldr r0, 023fc180
		*(u32*)0x023fc054 = 0xe59f1128; // ldr r1, 023fc184
		*(u32*)0x023fc058 = 0xe59f2128; // ldr r2, 023fc188
		*(u32*)0x023fc05c = 0xe28fe06c; // add lr, pc, #6c
		*(u32*)0x023fc060 = 0xe59f3074; // ldr r3, 023fc0dc
		*(u32*)0x023fc064 = 0xe12fff13; // bx r3
		*(u32*)0x023fc068 = 0xe3540002; // cmp r4, #2
		*(u32*)0x023fc06c = 0x1a000017; // bne 023fc0d0
		*(u32*)0x023fc070 = 0xe59f0108; // ldr r0, 023fc180
		*(u32*)0x023fc074 = 0xe59f1108; // ldr r1, 023fc184
		*(u32*)0x023fc078 = 0xe59f2108; // ldr r2, 023fc188
		*(u32*)0x023fc07c = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc080 = 0xe59f3058; // ldr r3, 023fc0e0
		*(u32*)0x023fc084 = 0xe12fff13; // bx r3
		*(u32*)0x023fc088 = 0xe59f0100; // ldr r0, 023fc190
		*(u32*)0x023fc08c = 0xe59f1100; // ldr r1, 023fc194
		*(u32*)0x023fc090 = 0xe59f2100; // ldr r2, 023fc198
		*(u32*)0x023fc094 = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc098 = 0xe59f3040; // ldr r3, 023fc0e0
		*(u32*)0x023fc09c = 0xe12fff13; // bx r3
		*(u32*)0x023fc0a0 = 0xe59f00f8; // ldr r0, 023fc1a0
		*(u32*)0x023fc0a4 = 0xe59f10f8; // ldr r1, 023fc1a4
		*(u32*)0x023fc0a8 = 0xe59f20f8; // ldr r2, 023fc1a8
		*(u32*)0x023fc0ac = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc0b0 = 0xe59f3028; // ldr r3, 023fc0e0
		*(u32*)0x023fc0b4 = 0xe12fff13; // bx r3
		*(u32*)0x023fc0b8 = 0xe59f00f0; // ldr r0, 023fc1b0
		*(u32*)0x023fc0bc = 0xe59f10f0; // ldr r1, 023fc1b4
		*(u32*)0x023fc0c0 = 0xe59f20f0; // ldr r2, 023fc1b8
		*(u32*)0x023fc0c4 = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc0c8 = 0xe59f3010; // ldr r3, 023fc0e0
		*(u32*)0x023fc0cc = 0xe12fff13; // bx r3
		*(u32*)0x023fc0d0 = 0xe3a04000; // mov r4, #0
		*(u32*)0x023fc0d4 = 0xe58f40b0; // str r4, [pc, #b0]
		*(u32*)0x023fc0d8 = 0xe8bd801f; // ldmia sp!, {r0-r4,pc}
		*(u32*)0x023fc0dc = 0x038040b5; // orreq r4, r0, #b5
		*(u32*)0x023fc0e0 = 0x03804071; // orreq r4, r0, #71
		*(u32*)0x023fc0e4 = 0xf505b500; // str r11, [r5, -#500]
		*(u32*)0x023fc0e8 = 0xb4fffc6b; // ldrblt pc, [pc], #46b!
		*(u32*)0x023fc0ec = 0x4d164c15; // ldcmi cpc, cr4, [r6, #-54]
		*(u32*)0x023fc0f0 = 0x350c682d; // strcc r6, [r12, -@2d]
		*(u32*)0x023fc0f4 = 0x2000a622; // andcs r10, r0, r2, lsr #c
		*(u32*)0x023fc0f8 = 0x1c286030; // stcne, cp0, cr6, [r8], #-c0
		*(u32*)0x023fc0fc = 0xf41e2100; // ldr r2, [lr], -#100
		*(u32*)0x023fc100 = 0x6070fa5b; // rsbvss pc, r0, r11, asr r10
		*(u32*)0x023fc104 = 0x613460b4; // ldrhvs r6, [r4, -r4]!
		*(u32*)0x023fc108 = 0x1c283578; // stcne cp5, cr3, [r8], #-1e0
		*(u32*)0x023fc10c = 0xf41e2100; // ldr r2, [lr], #-100
		*(u32*)0x023fc110 = 0x6170fa53; // cmnvs r0, r3, asr r10
		*(u32*)0x023fc114 = 0x006061b4; //
		*(u32*)0x023fc118 = 0x35786230; // ldrbcc r6, [r8, -#230]!
		*(u32*)0x023fc11c = 0x21001c28; //
		*(u32*)0x023fc120 = 0xfa4af41e; // blx 0331d1a0
		*(u32*)0x023fc124 = 0x62b46270; // adcvss r6, r4, #7
		*(u32*)0x023fc128 = 0x19000060; // stmdbne r0, {r5-r6}
		*(u32*)0x023fc12c = 0x35786330; // ldrbcc r6, [r8, -#330]!
		*(u32*)0x023fc130 = 0x21001c28; //
		*(u32*)0x023fc134 = 0xfa40f41e; // blx 0309d1b4
		*(u32*)0x023fc138 = 0x63b46370; // movvss r6, #c0000001
		*(u32*)0x023fc13c = 0x60f42402; // rscvss r2, r4, r2, lsl #8
		*(u32*)0x023fc140 = 0xbd00bcff; // stclt cpc ,crb, [r0-#-3fc]
		*(u32*)0x023fc144 = 0x00001000; // andeq r1, r0, r0
		*(u32*)0x023fc148 = 0x020c30dc; // andeq r3, r12, #dc
	}*/

    // Pokemon Dash (Japan)
	//else if (strcmp(romTid, "APDJ") == 0) {
		// *(u32*)0x0206AE70 = 0xE3A00000; //mov r0, #0
        // *(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		// *(u32*)0x0206AE74 = 0xe12fff1e; //bx lr

        // *(u32*)0x02000B94 = 0xE1A00000; //nop

		// *(u32*)0x020D5010 = 0xe12fff1e; //bx lr
	//}

    // Pokemon Dash
	//else if (strcmp(romTid, "APDE") == 0 || strcmp(romTid, "APDP") == 0) {
        /*unsigned char pdash_patch_chars[64] =
        {
          0xFE, 0x40, 0x2D, 0xE9,
          0x28, 0x10, 0xA0, 0xE3,
          0x00, 0x20, 0xA0, 0xE3,
          0x24, 0x30, 0x9F, 0xE5,
          0x02, 0x40, 0x90, 0xE7,
          0x02, 0x40, 0x83, 0xE7,
          0x04, 0x20, 0x82, 0xE2,
          0x01, 0x00, 0x52, 0xE1,
          0xFA, 0xFF, 0xFF, 0x1A,
          0x10, 0x30, 0x9F, 0xE5,
          0x33, 0xFF, 0x2F, 0xE1,
          0xFE, 0x80, 0xBD, 0xE8,
          0x01, 0x00, 0xA0, 0xE3,
          0x1E, 0xFF, 0x2F, 0xE1,
          0x00, 0xA6, 0x0D, 0x02,              d
          0x78, 0x47, 0x0A, 0x02
        };

        //6D38C
        PatchMem(KArm9,s32(ii+1),0xe1a05000); //mov r5, r0
        PatchMem(KArm9,s32(ii+2),0xe1a00001); //mov r0, r1
        PatchMem(KArm9,s32(ii+3),0xe28fe004); //adr lr, [pc, #4]
        PatchMem(KArm9,s32(ii+4),0xe51ff004); //ldr pc, [pc, #-4]
        PatchMem(KArm9,s32(ii+5),(u32)iDmaFuncs.iFunc2);
        PatchMem(KArm9,s32(ii+6),0xe1a00005); //mov r0, r5
        PatchMem(KArm9,s32(ii+7),0xe28ff048); //adr pc, xxx  jump+48 (12*4)
        //6D3FC
        PatchMem(KArm9,s32(ii+28),0xe1a00000); //nop

        // r0 : ROMCTRL
        // r1 : ROMCTRL
        // r2 : ...
        // r3 : ...
        // r4 : DST
        // r5 : SRC
        // r6 : LEN
        // ..
        // r10 : cardstruct

        for(int i =0; i<64; i++) {
            *(((u8*)0x0206D2C4)+i) = pdash_patch_chars[i];
        }*/

        // *((u32*)0x02000BB0) = 0xE1A00000; //nop

		// *(u32*)0x0206D2C4 = 0xE3A00000; //mov r0, #0
        // *(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		// *(u32*)0x0206D2C8 = 0xe12fff1e; //bx lr

		// *(u32*)0x020D5010 = 0xe12fff1e; //bx lr
	//}

    /* // Pokemon Dash (Kiosk Demo)
	else if (strcmp(romTid, "A24E") == 0) {
        *(u32*)0x02000BB0 = 0xE1A00000; //nop
	}

    // Pokemon Dash (Korea)
	else if (strcmp(romTid, "APDK") == 0) {
        *(u32*)0x02000C14 = 0xE1A00000; //nop
	}*/

	// Golden Sun: Dark Dawn (USA, Australia)
	else if (strcmp(romTid, "BO5E") == 0) {
		// setBEQ(0x02003CA0, 0x02003C30); // Skip a block of DSProtect code branches
		setBL(0x0200AC08, (u32)ce9->patches->gsdd_fix);
		*(u32*)0x02FFF000 = 0x021F7720;
	}

	// Golden Sun: Dark Dawn (Europe)
	else if (strcmp(romTid, "BO5P") == 0) {
		// setBEQ(0x02003CDC, 0x02003C6C); // Skip a block of DSProtect code branches
		setBL(0x0200AC44, (u32)ce9->patches->gsdd_fix);
		*(u32*)0x02FFF000 = 0x021F78C0;
	}

	// Ougon no Taiyou: Shikkoku Naru Yoake (Japan)
	else if (strcmp(romTid, "BO5J") == 0) {
		// setBEQ(0x02003C7C, 0x02003C0C); // Skip a block of DSProtect code branches
		setBL(0x0200ABE4, (u32)ce9->patches->gsdd_fix);
		*(u32*)0x02FFF000 = 0x021F7500;
	}

	// Tony Hawk's Motion (USA)
	// Tony Hawk's Motion (Europe)
	/* else if (strncmp(romTid, "CTW", 3) == 0) {
		// Remove Motion Pak checks
		*(u16*)0x02002490 = 0;
		*(u32*)0x0202A834 = 0;
		*(u16*)0x0202A842 = 0;
		*(u16*)0x0202A844 = 0;
		*(u32*)0x02039BA8 = 0;
		*(u16*)0x02069EC8 = 0;
	} */

	// Tropix! Your Island Getaway
    else if (strcmp(romTid, "CTXE") == 0) {
		extern u32 baseChipID;
		u32 cardIdFunc[2] = {0x47704800, baseChipID};

		const u16* branchCode1 = generateA7InstrThumb(0x020BA666, 0x020BA670);
		tonccpy((void*)0x020BA666, branchCode1, 0x4);

		tonccpy((void*)0x020BA670, cardIdFunc, 0x8);

		const u16* branchCode2 = generateA7InstrThumb(0x020BA66A, 0x020BA6C0);
		tonccpy((void*)0x020BA66A, branchCode2, 0x4);

		tonccpy((void*)0x020BA728, patchOffsetCache.cardSetDmaOffset, 0xC);

		const u16* branchCode3 = generateA7InstrThumb(0x020BA70C, 0x020BA728);
		tonccpy((void*)0x020BA70C, branchCode3, 0x4);
		*(u16*)0x020BA710 = 0xBDF8;

		const u16* branchCode4 = generateA7InstrThumb(0x020BAAA2, 0x020BAAAC);
		tonccpy((void*)0x020BAAA2, branchCode4, 0x4);

		tonccpy((void*)0x020BAAAC, cardIdFunc, 0x8);

		const u16* branchCode5 = generateA7InstrThumb(0x020BAAA6, 0x020BAAFC);
		tonccpy((void*)0x020BAAA6, branchCode5, 0x4);

		const u16* branchCode6 = generateA7InstrThumb(0x020BAC5C, 0x020BAC64);
		tonccpy((void*)0x020BAC5C, branchCode6, 0x4);

		tonccpy((void*)0x020BAC64, cardIdFunc, 0x8);

		const u16* branchCode7 = generateA7InstrThumb(0x020BAC60, 0x020BACB6);
		tonccpy((void*)0x020BAC60, branchCode7, 0x4);
	}
#endif

	// Shantae DSi (03/06/09 build)
	else if (strcmp(romTid, "AIPE") == 0 && ndsHeader->headerCRC16 == 0x700E && !extendedMemory) {
		*(u32*)0x02046BFC -= 2;
		*(u32*)0x02046DE4 -= 2;
	}

	// Shantae DSi (04/01/09 build)
	else if (strcmp(romTid, "NTRJ") == 0 && ndsHeader->headerCRC16 == 0xAC4C && !extendedMemory) {
		*(u32*)0x0203FB20 -= 2;
		*(u32*)0x0203FD14 -= 2;
	}
}

void rsetA7Cache(void)
{
	patchOffsetCache.a7BinSize = 0;
	patchOffsetCache.a7IsThumb = 0;
	patchOffsetCache.wramEndAddrOffset = 0;
	patchOffsetCache.wramClearOffset = 0;
	patchOffsetCache.ramClearOffset = 0;
	patchOffsetCache.ramClearChecked = 0;
	patchOffsetCache.a7Swi12Offset = 0;
	patchOffsetCache.swiGetPitchTableOffset = 0;
	patchOffsetCache.swiGetPitchTableChecked = 0;
	patchOffsetCache.sleepPatchOffset = 0;
	patchOffsetCache.sleepInputWriteOffset = 0;
	patchOffsetCache.postBootOffset = 0;
	patchOffsetCache.a7CardIrqEnableOffset = 0;
	patchOffsetCache.a7IrqHandlerOffset = 0;
	patchOffsetCache.savePatchType = 0;
	patchOffsetCache.relocateStartOffset = 0;
	patchOffsetCache.relocateValidateOffset = 0;
	patchOffsetCache.a7CardReadEndOffset = 0;
	patchOffsetCache.a7JumpTableFuncOffset = 0;
	patchOffsetCache.a7JumpTableType = 0;
}

u32 patchCardNds(
	cardengineArm7* ce7,
	cardengineArm9* ce9,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 patchMpuRegion,
	const bool usesCloneboot,
	u32 saveFileCluster,
	u32 saveSize
) {
	dbg_printf("patchCardNds\n\n");

	if (isSdk5(moduleParams)) {
		dbg_printf("[SDK 5]\n\n");
	}

	u32 errorCodeArm9 = patchCardNdsArm9(ce9, ndsHeader, moduleParams, patchMpuRegion, usesCloneboot);

	//if (cardReadFound || ndsHeader->fatSize == 0) {
	if (errorCodeArm9 == ERR_NONE || ndsHeader->fatSize == 0) {
		return patchCardNdsArm7(ce7, ndsHeader, moduleParams, saveFileCluster);
	}

	return ERR_LOAD_OTHR;
}
