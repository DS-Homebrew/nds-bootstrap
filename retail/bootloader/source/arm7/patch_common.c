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

u16 patchOffsetCacheFileVersion = 30;	// Change when new functions are being patched, some offsets removed,
										// the offset order changed, and/or the function signatures changed (not added)

patchOffsetCacheContents patchOffsetCache;

u16 patchOffsetCacheFilePrevCrc = 0;
u16 patchOffsetCacheFileNewCrc = 0;

static inline void doubleNopT(u32 addr) {
	*(u16*)(addr)   = 0x46C0;
	*(u16*)(addr+2) = 0x46C0;
}

void patchDSiModeToDSMode(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
	extern u32 donorFileTwlCluster;	// SDK5 (TWL)
	extern u32 fatTableAddr;
	const char* romTid = getRomTid(ndsHeader);
	extern void patchHiHeapDSiWare(u32 addr, u32 heapEnd);
	extern void patchHiHeapDSiWareThumb(u32 addr, u16 opCode1, u16 opCode2);

	const u32 heapEnd = (fatTableAddr < 0x023C0000 || fatTableAddr >= CARDENGINE_ARM9_LOCATION_DLDI) ? CARDENGINE_ARM9_LOCATION_DLDI : fatTableAddr;

	if (donorFileTwlCluster == 0) {
		return;
	}

	// Patch DSi-Exclusives to run in DS mode

	// Nintendo DSi XL Demo Video (USA)
	// Requires 8MB of RAM
	if (strcmp(romTid, "DMEE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x02008DD8 = 0xE1A00000; // nop
		*(u32*)0x02008EF4 = 0xE1A00000; // nop
		*(u32*)0x02008F08 = 0xE1A00000; // nop
		*(u32*)0x0200BC58 = 0xE1A00000; // nop
		*(u32*)0x0200D778 = 0xE1A00000; // nop
		*(u32*)0x0200EFF4 = 0xE1A00000; // nop
		*(u32*)0x0200EFF8 = 0xE1A00000; // nop
		*(u32*)0x0200F004 = 0xE1A00000; // nop
		*(u32*)0x0200F148 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0200F1A4, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020107FC = 0xE1A00000; // nop
		*(u32*)0x02010800 = 0xE1A00000; // nop
		*(u32*)0x02010804 = 0xE1A00000; // nop
		*(u32*)0x02010808 = 0xE1A00000; // nop
	}

	// Nintendo DSi XL Demo Video: Volume 2 (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "DMDE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x02008E04 = 0xE1A00000; // nop
		*(u32*)0x02008F14 = 0xE1A00000; // nop
		*(u32*)0x02008F28 = 0xE1A00000; // nop
		*(u32*)0x0200BB3C = 0xE1A00000; // nop
		*(u32*)0x0200D55C = 0xE1A00000; // nop
		*(u32*)0x0200ED80 = 0xE1A00000; // nop
		*(u32*)0x0200ED84 = 0xE1A00000; // nop
		*(u32*)0x0200ED90 = 0xE1A00000; // nop
		*(u32*)0x0200EEF0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0200EF4C, 0x02700000); // mov r0, #0x2700000
	}

	// NOE Movie Player: Volume 1 (Europe)
	else if (strcmp(romTid, "DMPP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02009ED8 = 0xE1A00000; // nop
		*(u32*)0x0200D0D8 = 0xE1A00000; // nop
		*(u32*)0x0200EE00 = 0xE1A00000; // nop
		*(u32*)0x02010630 = 0xE1A00000; // nop
		*(u32*)0x02010634 = 0xE1A00000; // nop
		*(u32*)0x02010640 = 0xE1A00000; // nop
		*(u32*)0x020107A0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020107FC, heapEnd); // mov r0, #0x23C0000
	}

	// Picture Perfect Hair Salon (USA)
	// Hair Salon (Europe/Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "DHSE") == 0 || strcmp(romTid, "DHSV") == 0) && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x02005108 = 0xE1A00000; // nop
		*(u32*)0x0200517C = 0xE1A00000; // nop
		*(u32*)0x02005190 = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x0200F2B0 = 0xE1A00000; // nop
		*(u32*)0x0200F3DC = 0xE1A00000; // nop
		*(u32*)0x0200F3F0 = 0xE1A00000; // nop
		*(u32*)0x02012840 = 0xE1A00000; // nop
		*(u32*)0x02017F34 = 0xE1A00000; // nop
		*(u32*)0x02019A0C = 0xE1A00000; // nop
		*(u32*)0x02019A10 = 0xE1A00000; // nop
		*(u32*)0x02019A1C = 0xE1A00000; // nop
		*(u32*)0x02019B60 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019BBC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201E6CC = 0xE1A00000; // nop
	}

	// Patch DSiWare to run in DS mode

	// 1950s Lawn Mower Kids (USA)
	// 1950s Lawn Mower Kids (Europe, Australia)
	else if (strcmp(romTid, "K95E") == 0 || strcmp(romTid, "K95V") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02019608 = 0xE1A00000; // nop
		*(u32*)0x02015A08 = 0xE1A00000; // nop
		*(u32*)0x0201D8BC = 0xE1A00000; // nop
		*(u32*)0x0201F658 = 0xE1A00000; // nop
		*(u32*)0x0201F65C = 0xE1A00000; // nop
		*(u32*)0x0201F668 = 0xE1A00000; // nop
		*(u32*)0x0201F7C8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F824, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02020DD8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023C1C = 0xE1A00000; // nop
		doubleNopT(0x020471B0);
		*(u16*)0x0204713C = 0x4770; // bx lr
		*(u16*)0x0204715C = 0x4770; // bx lr
	}

	// GO Series: 10 Second Run (USA)
	// GO Series: 10 Second Run (Europe)
	// Sound either does not play or stop too quickly
	else if (strcmp(romTid, "KJUE") == 0 || strcmp(romTid, "KJUP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005888 = 0xE12FFF1E; // bx lr
		// Real hardware fix: Disable option font
		{
			*(u32*)0x02005A68 = 0xE12FFF1E; // bx lr
			*(u32*)0x02005A9C = 0xE12FFF1E; // bx lr
			*(u32*)0x02005B74 = 0xE12FFF1E; // bx lr
			*(u32*)0x02005BA0 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x020150FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201588C = 0xE1A00000; // nop
		*(u32*)0x0201589C = 0xE1A00000; // nop
		*(u32*)0x020158A8 = 0xE1A00000; // nop
		*(u32*)0x020158B4 = 0xE1A00000; // nop
		*(u32*)0x02015968 = 0xE1A00000; // nop
		*(u32*)0x02015970 = 0xE1A00000; // nop
		*(u32*)0x02015980 = 0xE1A00000; // nop
		*(u32*)0x02015A60 = 0xE1A00000; // nop
		*(u32*)0x02015A98 = 0xE1A00000; // nop
		*(u32*)0x02018B4C = 0xE1A00000; // nop
		*(u32*)0x020193A0 = 0xE1A00000; // nop
		*(u32*)0x020193A4 = 0xE1A00000; // nop
		*(u32*)0x020193B4 = 0xE1A00000; // nop
		*(u32*)0x020193E0 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
		*(u32*)0x02019D20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02030A88 = 0xE1A00000; // nop
		*(u32*)0x02034224 = 0xE1A00000; // nop
		*(u32*)0x02037F24 = 0xE1A00000; // nop
		*(u32*)0x02039CCC = 0xE1A00000; // nop
		*(u32*)0x02039CD0 = 0xE1A00000; // nop
		*(u32*)0x02039CDC = 0xE1A00000; // nop
		*(u32*)0x02039E3C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02039E98, heapEnd); // mov r0, #0x23C0000
		//*(u32*)0x02039FCC = 0x02115860;
		*(u32*)0x0203B3CC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		//*(u32*)0x0203B77C = 0xE12FFF1E; // bx lr
		/* *(u32*)0x0203B7D4 = 0xE1A00000; // nop
		*(u32*)0x0203B7D8 = 0xE1A00000; // nop
		*(u32*)0x0203B7DC = 0xE1A00000; // nop
		*(u32*)0x0203B7E0 = 0xE1A00000; // nop */
		*(u32*)0x0203E7D0 = 0xE1A00000; // nop
	}

	// 101 Pinball World (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KIIE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02093EB0 = 0xE1A00000; // nop
		*(u32*)0x02097220 = 0xE1A00000; // nop
		*(u32*)0x0209B3E4 = 0xE1A00000; // nop
		*(u32*)0x0209D190 = 0xE1A00000; // nop
		*(u32*)0x0209D194 = 0xE1A00000; // nop
		*(u32*)0x0209D1A0 = 0xE1A00000; // nop
		*(u32*)0x0209D300 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0209D35C, heapEnd); // mov r0, #0x2700000
		*(u32*)0x0209E8F0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020A1D28 = 0xE1A00000; // nop
		*(u32*)0x020B7B0C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7B30 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7B54 = 0xE3A00001; // mov r0, #1
	}

	// 101 Pinball World (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KIIP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0208DC80 = 0xE1A00000; // nop
		*(u32*)0x02090FF0 = 0xE1A00000; // nop
		*(u32*)0x020951B4 = 0xE1A00000; // nop
		*(u32*)0x02096F60 = 0xE1A00000; // nop
		*(u32*)0x02096F64 = 0xE1A00000; // nop
		*(u32*)0x02096F70 = 0xE1A00000; // nop
		*(u32*)0x020970D0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0209712C, heapEnd); // mov r0, #0x2700000
		*(u32*)0x020986C0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0209BAF8 = 0xE1A00000; // nop
		*(u32*)0x020B0888 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B08AC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B08D0 = 0xE3A00001; // mov r0, #1
	}

	// 99Bullets (USA)
	else if (strcmp(romTid, "K99E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02005144 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005160 = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x0204F5B0 = 0xE1A00000; // nop
		*(u32*)0x02053278 = 0xE1A00000; // nop
		*(u32*)0x0205B090 = 0xE1A00000; // nop
		*(u32*)0x02068228 = 0xE1A00000; // nop
		*(u32*)0x0206A09C = 0xE1A00000; // nop
		*(u32*)0x0206A0A0 = 0xE1A00000; // nop
		*(u32*)0x0206A0AC = 0xE1A00000; // nop
		*(u32*)0x0206A20C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206A268, heapEnd); // mov r0, #0x23C0000
	}

	// 99Bullets (Europe)
	else if (strcmp(romTid, "K99P") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x0200512C = 0xE1A00000; // nop
		*(u32*)0x02005140 = 0xE1A00000; // nop
		*(u32*)0x02005144 = 0xE1A00000; // nop
		*(u32*)0x02005148 = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x0205FCF4 = 0xE1A00000; // nop
		*(u32*)0x0206488C = 0xE1A00000; // nop
		*(u32*)0x0206B1C0 = 0xE1A00000; // nop
		*(u32*)0x0206D048 = 0xE1A00000; // nop
		*(u32*)0x0206D04C = 0xE1A00000; // nop
		*(u32*)0x0206D058 = 0xE1A00000; // nop
		*(u32*)0x0206D1B8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206D214, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02071114 = 0xE1A00000; // nop
	}

	// 99Bullets (Japan)
	else if (strcmp(romTid, "K99J") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02005144 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005160 = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x02063618 = 0xE1A00000; // nop
		*(u32*)0x02069F60 = 0xE1A00000; // nop
		*(u32*)0x0206BDF0 = 0xE1A00000; // nop
		*(u32*)0x0206BDF4 = 0xE1A00000; // nop
		*(u32*)0x0206BE00 = 0xE1A00000; // nop
		*(u32*)0x0206BF60 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206BFBC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020726F0 = 0xE1A00000; // nop
	}

	// 99Moves (USA)
	else if (strcmp(romTid, "K9WE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02061478 = 0xE1A00000; // nop
		*(u32*)0x02067D7C = 0xE1A00000; // nop
		*(u32*)0x02069C0C = 0xE1A00000; // nop
		*(u32*)0x02069C10 = 0xE1A00000; // nop
		*(u32*)0x02069C1C = 0xE1A00000; // nop
		*(u32*)0x02069D7C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02069DD8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0206DCD8 = 0xE1A00000; // nop
		*(u32*)0x0207050C = 0xE1A00000; // nop
	}

	// 99Moves (Europe)
	else if (strcmp(romTid, "K9WP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x020614C8 = 0xE1A00000; // nop
		*(u32*)0x02067DCC = 0xE1A00000; // nop
		*(u32*)0x02069C5C = 0xE1A00000; // nop
		*(u32*)0x02069C60 = 0xE1A00000; // nop
		*(u32*)0x02069C6C = 0xE1A00000; // nop
		*(u32*)0x02069DCC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02069E28, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0206DD28 = 0xE1A00000; // nop
		*(u32*)0x0207055C = 0xE1A00000; // nop
	}

	// 99Seconds (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KXTE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02061590 = 0xE1A00000; // nop
		*(u32*)0x02067EC0 = 0xE1A00000; // nop
		*(u32*)0x02069D50 = 0xE1A00000; // nop
		*(u32*)0x02069D54 = 0xE1A00000; // nop
		*(u32*)0x02069D60 = 0xE1A00000; // nop
		*(u32*)0x02069EC0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02069E28, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0206DE34 = 0xE1A00000; // nop
		*(u32*)0x02070668 = 0xE1A00000; // nop
	}

	// 99Seconds (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KXTP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x020615E0 = 0xE1A00000; // nop
		*(u32*)0x02067F10 = 0xE1A00000; // nop
		*(u32*)0x02069DA0 = 0xE1A00000; // nop
		*(u32*)0x02069DA4 = 0xE1A00000; // nop
		*(u32*)0x02069DB0 = 0xE1A00000; // nop
		*(u32*)0x02069F10 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02069F6C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0206DE84 = 0xE1A00000; // nop
		*(u32*)0x020706B8 = 0xE1A00000; // nop
	}

	// Absolute Baseball (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KE9E") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x0200F890 = 0xE1A00000; // nop
		*(u32*)0x02013454 = 0xE1A00000; // nop
		*(u32*)0x02018230 = 0xE1A00000; // nop
		*(u32*)0x02019FDC = 0xE1A00000; // nop
		*(u32*)0x02019FE0 = 0xE1A00000; // nop
		*(u32*)0x02019FEC = 0xE1A00000; // nop
		*(u32*)0x0201A14C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201A1A8, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201B42C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201E7D8 = 0xE1A00000; // nop
		*(u32*)0x0205FAD0 = 0xE1A00000; // nop
		*(u32*)0x02072554 = 0xE3A00000; // mov r0, #0
	}

	// Absolute BrickBuster (USA)
	// Crashes after starting a game mode
	// Requires 8MB of RAM
	/*else if (strcmp(romTid, "K6QE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020053E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02055B74 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02055B78 = 0xE12FFF1E; // bx lr
		*(u32*)0x02055C48 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02055C4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0205CD8C = 0xE1A00000; // nop
		*(u32*)0x02060D94 = 0xE1A00000; // nop
		*(u32*)0x0206BB40 = 0xE1A00000; // nop
		*(u32*)0x0206DA58 = 0xE1A00000; // nop
		*(u32*)0x0206DA5C = 0xE1A00000; // nop
		*(u32*)0x0206DA68 = 0xE1A00000; // nop
		*(u32*)0x0206DBAC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206DC08, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0206EE64 = 0xE1A00000; // nop
		*(u32*)0x0206EE6C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0206EEEC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206EEF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206EEF8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0206EEFC = 0xE12FFF1E; // bx lr
		*(u32*)0x02072668 = 0xE1A00000; // nop
	}*/

	// Ace Mathician (USA)
	else if (strcmp(romTid, "KQKE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x02005128 = 0xE1A00000; // nop
		*(u32*)0x0200512C = 0xE1A00000; // nop
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200513C = 0xE1A00000; // nop
		*(u32*)0x0200DFD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02010084 = 0xE1A00000; // nop
		*(u32*)0x02030344 = 0xE1A00000; // nop
		*(u32*)0x020335B0 = 0xE1A00000; // nop
		*(u32*)0x02036444 = 0xE1A00000; // nop
		*(u32*)0x020381E0 = 0xE1A00000; // nop
		*(u32*)0x020381E4 = 0xE1A00000; // nop
		*(u32*)0x020381F0 = 0xE1A00000; // nop
		*(u32*)0x02038350 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020383AC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02039B74 = 0xE1A00000; // nop
		*(u32*)0x02039B78 = 0xE1A00000; // nop
		*(u32*)0x02039B7C = 0xE1A00000; // nop
		*(u32*)0x02039B80 = 0xE1A00000; // nop
		*(u32*)0x0203C5DC = 0xE1A00000; // nop
	}

	// Ace Mathician (Europe, Australia)
	else if (strcmp(romTid, "KQKV") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x02005128 = 0xE1A00000; // nop
		*(u32*)0x0200512C = 0xE1A00000; // nop
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200513C = 0xE1A00000; // nop
		*(u32*)0x0200DFD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02030354 = 0xE1A00000; // nop
		*(u32*)0x020335C0 = 0xE1A00000; // nop
		*(u32*)0x02036454 = 0xE1A00000; // nop
		*(u32*)0x020381F0 = 0xE1A00000; // nop
		*(u32*)0x020381F4 = 0xE1A00000; // nop
		*(u32*)0x02038200 = 0xE1A00000; // nop
		*(u32*)0x02038360 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020383BC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02039B84 = 0xE1A00000; // nop
		*(u32*)0x02039B88 = 0xE1A00000; // nop
		*(u32*)0x02039B8C = 0xE1A00000; // nop
		*(u32*)0x02039B90 = 0xE1A00000; // nop
		*(u32*)0x0203C5EC = 0xE1A00000; // nop
	}

	// Advanced Circuits (USA)
	// Advanced Circuits (Europe, Australia)
	else if (strncmp(romTid, "KAC", 3) == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02011298 = 0xE1A00000; // nop
		*(u32*)0x02014738 = 0xE1A00000; // nop
		*(u32*)0x02018468 = 0xE1A00000; // nop
		*(u32*)0x0201A248 = 0xE1A00000; // nop
		*(u32*)0x0201A24C = 0xE1A00000; // nop
		*(u32*)0x0201A258 = 0xE1A00000; // nop
		*(u32*)0x0201A3B8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201A414, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201B758 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201EBE8 = 0xE1A00000; // nop
		*(u32*)0x0202CDA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D490 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02053F30 = 0xE1A00000; // nop
			*(u32*)0x02053F90 = 0xE1A00000; // nop
			*(u32*)0x02054920 = 0xE12FFF1E; // bx lr
		} else if (ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x02053F58 = 0xE1A00000; // nop
			*(u32*)0x02053FB8 = 0xE1A00000; // nop
			*(u32*)0x020548C0 = 0xE12FFF1E; // bx lr
		}
	}

	// Ah! Heaven (USA)
	// Ah! Heaven (Europe)
	else if (strcmp(romTid, "K5HE") == 0 || strcmp(romTid, "K5HP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200F778 = 0xE1A00000; // nop
		*(u32*)0x02012A58 = 0xE1A00000; // nop
		*(u32*)0x02016388 = 0xE1A00000; // nop
		*(u32*)0x02018124 = 0xE1A00000; // nop
		*(u32*)0x02018128 = 0xE1A00000; // nop
		*(u32*)0x02018134 = 0xE1A00000; // nop
		*(u32*)0x02018294 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020182F0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02019538 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201C150 = 0xE1A00000; // nop
		*(u32*)0x0201FD04 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02029C68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02029D14 = 0xE12FFF1E; // bx lr
	}

	// AiRace: Tunnel (USA)
	// Requires 8MB of RAM
	// Crashes after selecting a stage
	/*else if (strcmp(romTid, "KATE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u16*)0x0202A3D2 = 0x46C0; // nop
		*(u16*)0x0202A3D4 = 0x46C0; // nop
		*(u16*)0x0202A59C = 0x46C0; // nop
		*(u16*)0x0202A59E = 0x46C0; // nop
		*(u32*)0x02032AF0 = 0xE1A00000; // nop
		*(u16*)0x02042042 = 0x46C0; // nop
		*(u16*)0x02042044 = 0x46C0; // nop
		*(u16*)0x02042048 = 0x46C0; // nop
		*(u16*)0x0204204A = 0x46C0; // nop
		*(u32*)0x020420F4 = 0xE1A00000; // nop
		*(u32*)0x02048AC0 = 0xE1A00000; // nop
		*(u32*)0x0204C1C8 = 0xE1A00000; // nop
		*(u32*)0x02056798 = 0xE1A00000; // nop
		*(u32*)0x02058628 = 0xE1A00000; // nop
		*(u32*)0x0205862C = 0xE1A00000; // nop
		*(u32*)0x02058638 = 0xE1A00000; // nop
		*(u32*)0x02058798 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020587F4, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02059B68 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205D21C = 0xE1A00000; // nop
	}*/

	// G.G. Series: All Breaker (USA)
	// G.G. Series: All Breaker (Japan)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "K27E") == 0 || strcmp(romTid, "K27J") == 0) && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200D71C = 0xE1A00000; // nop
		*(u32*)0x0204E880 = 0xE1A00000; // nop
		*(u32*)0x02052814 = 0xE1A00000; // nop
		*(u32*)0x02058BE8 = 0xE1A00000; // nop
		*(u32*)0x0205AA84 = 0xE1A00000; // nop
		*(u32*)0x0205AA88 = 0xE1A00000; // nop
		*(u32*)0x0205AA94 = 0xE1A00000; // nop
		*(u32*)0x0205ABF4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0205AC50, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0205FA64 = 0xE1A00000; // nop
	}

	// AlphaBounce (USA)
	// Does not boot
	/*else if (strcmp(romTid, "KALE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020187B8 = 0xE1A00000; // nop
		*(u32*)0x0201BC4C = 0xE1A00000; // nop
		*(u32*)0x0201EA34 = 0xE1A00000; // nop
		*(u32*)0x0201EAE8 = 0xE1A00000; // nop
		*(u32*)0x0201EC08 = 0xE1A00000; // nop
		*(u32*)0x0201EC84 = 0xE1A00000; // nop
		*(u32*)0x0201ED08 = 0xE1A00000; // nop
		*(u32*)0x0201FBC8 = 0xE1A00000; // nop
		*(u32*)0x0201FC38 = 0xE1A00000; // nop
		*(u32*)0x0201FD4C = 0xE1A00000; // nop
		*(u32*)0x0201FDB4 = 0xE1A00000; // nop
		*(u32*)0x0201FE34 = 0xE1A00000; // nop
		*(u32*)0x0201FE98 = 0xE1A00000; // nop
		*(u32*)0x0201FF50 = 0xE1A00000; // nop
		*(u32*)0x0201FFC0 = 0xE1A00000; // nop
		*(u32*)0x020226A0 = 0xE1A00000; // nop
		*(u32*)0x02024DD4 = 0xE1A00000; // nop
		*(u32*)0x02024DD8 = 0xE1A00000; // nop
		*(u32*)0x02024DE4 = 0xE1A00000; // nop
		*(u32*)0x02024F44 = 0xE1A00000; // nop
		*(u32*)0x02024FA0 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02024FC4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02024FCC = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020299E0 = 0xE1A00000; // nop
		*(u32*)0x020B0600 = 0xE1A00000; // nop
		*(u32*)0x020B0604 = 0xE1A00000; // nop
		*(u32*)0x020B060C = 0xE1A00000; // nop
	}*/

	// Anonymous Notes 1: From The Abyss (USA)
	// Crashes on reading save data after touching START
	/*else if (strcmp(romTid, "KVIE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200E74C = 0xE1A00000; // nop
		*(u32*)0x02011C48 = 0xE1A00000; // nop
		*(u32*)0x02016A5C = 0xE1A00000; // nop
		*(u32*)0x020188E0 = 0xE1A00000; // nop
		*(u32*)0x020188E4 = 0xE1A00000; // nop
		*(u32*)0x020188F0 = 0xE1A00000; // nop
		*(u32*)0x02018A50 = 0xE1A00000; // nop
		*(u32*)0x02018AAC = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02018AD0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02018AD8 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201D5C4 = 0xE1A00000; // nop
		*(u32*)0x02023DB0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02023DB4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0209E2CC = 0xE1A00000; // nop
		*(u32*)0x0209E2E0 = 0xE1A00000; // nop
		*(u32*)0x0209E2F4 = 0xE1A00000; // nop
		*(u32*)0x020CE830 = 0xE12FFF1E; // bx lr
		*(u32*)0x020CFB78 = 0xE1A00000; // nop
		*(u32*)0x020CFCD0 = 0xE1A00000; // nop
	}*/

	// Antipole (USA)
	// Does not boot due to lack of memory
	/*else if (strcmp(romTid, "KJHE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x020333F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02035704 = 0xE1A00000; // nop
		*(u32*)0x020357E0 = 0xE1A00000; // nop
		*(u32*)0x0203BAD8 = 0xE1A00000; // nop
		*(u32*)0x0203D9E4 = 0xE1A00000; // nop
		*(u32*)0x02056BD0 = 0xE1A00000; // nop
		*(u32*)0x020577A4 = 0xE1A00000; // nop
		*(u32*)0x020581AC = 0xE1A00000; // nop
		*(u32*)0x02058260 = 0xE1A00000; // nop
		*(u32*)0x02058300 = 0xE1A00000; // nop
		*(u32*)0x02058380 = 0xE1A00000; // nop
		*(u32*)0x020583FC = 0xE1A00000; // nop
		*(u32*)0x02058480 = 0xE1A00000; // nop
		*(u32*)0x02058988 = 0xE1A00000; // nop
		*(u32*)0x02058A44 = 0xE1A00000; // nop
		*(u32*)0x02058AF0 = 0xE1A00000; // nop
		*(u32*)0x02058B84 = 0xE1A00000; // nop
		*(u32*)0x02058C18 = 0xE1A00000; // nop
		*(u32*)0x02058CAC = 0xE1A00000; // nop
		*(u32*)0x02058D40 = 0xE1A00000; // nop
		*(u32*)0x02058DD4 = 0xE1A00000; // nop
		*(u32*)0x02058E68 = 0xE1A00000; // nop
		*(u32*)0x02058EFC = 0xE1A00000; // nop
		*(u32*)0x02059020 = 0xE1A00000; // nop
		*(u32*)0x02059084 = 0xE1A00000; // nop
		*(u32*)0x0205914C = 0xE1A00000; // nop
		*(u32*)0x020591BC = 0xE1A00000; // nop
		*(u32*)0x02059248 = 0xE1A00000; // nop
		*(u32*)0x020592B8 = 0xE1A00000; // nop
		*(u32*)0x02059340 = 0xE1A00000; // nop
		*(u32*)0x020593B0 = 0xE1A00000; // nop
		*(u32*)0x020594C4 = 0xE1A00000; // nop
		*(u32*)0x0205952C = 0xE1A00000; // nop
		*(u32*)0x020595AC = 0xE1A00000; // nop
		*(u32*)0x02059610 = 0xE1A00000; // nop
		*(u32*)0x020596C8 = 0xE1A00000; // nop
		*(u32*)0x02059738 = 0xE1A00000; // nop
		*(u32*)0x0205D874 = 0xE1A00000; // nop
		*(u32*)0x0205D878 = 0xE1A00000; // nop
		*(u32*)0x0205D884 = 0xE1A00000; // nop
		*(u32*)0x0205DA40 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0205DA64 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0205DA6C = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0205F090 = 0xE1A00000; // nop
		*(u32*)0x0205F094 = 0xE1A00000; // nop
		*(u32*)0x0205F098 = 0xE1A00000; // nop
		*(u32*)0x0205F09C = 0xE1A00000; // nop
		*(u32*)0x02061984 = 0xE1A00000; // nop
	}*/

	// Army Defender (USA)
	// Army Defender (Europe)
	else if (strncmp(romTid, "KAY", 3) == 0) {
		*(u32*)0x020051BC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020051C0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005204 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005208 = 0xE12FFF1E; // bx lr
		*(u32*)0x020426BC = 0xE1A00000; // nop
		*(u32*)0x02045CC4 = 0xE1A00000; // nop
		*(u32*)0x0204B010 = 0xE1A00000; // nop
		*(u32*)0x0204CECC = 0xE1A00000; // nop
		*(u32*)0x0204CED0 = 0xE1A00000; // nop
		*(u32*)0x0204CEDC = 0xE1A00000; // nop
		*(u32*)0x0204D020 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204D07C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0204E3E0 = 0xE1A00000; // nop
		*(u32*)0x0204E3E8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02051AB8 = 0xE1A00000; // nop
	}

	// Art Style: AQUIA (USA)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAE") == 0) {
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		*(u32*)0x0203BB4C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203BB50 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203BC18 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203BC1C = 0xE12FFF1E; // bx lr
		*(u32*)0x02051E00 = 0xE1A00000; // nop
		*(u32*)0x02054CD8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02054CDC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020583BC = 0xE1A00000; // nop
		*(u32*)0x02062C58 = 0xE1A00000; // nop
		*(u32*)0x02064A48 = 0xE1A00000; // nop
		*(u32*)0x02064A4C = 0xE1A00000; // nop
		*(u32*)0x02064A58 = 0xE1A00000; // nop
		*(u32*)0x02064B9C = 0xE1A00000; // nop
		*(u32*)0x02064BA0 = 0xE1A00000; // nop
		*(u32*)0x02064BA4 = 0xE1A00000; // nop
		*(u32*)0x02064BA8 = 0xE1A00000; // nop
		/* *(u32*)0x02064B9C = generateA7Instr(0x02064B9C, 0x020665C4); // bl 0x020665C4
		{
			*(u32*)0x020665C4 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020665C8 = 0xE3A01402; // mov r1, #0x2000000
			*(u32*)0x020665CC = 0xE3A0202A; // mov r2, #0x2A
			*(u32*)0x020665D0 = generateA7Instr(0x020665D0, 0x02065214); // bl 0x02065214
			*(u32*)0x020665D4 = 0xE59F100C; // ldr r1, =0x27FF000
			*(u32*)0x020665D8 = 0xE3A00002; // mov r0, #2
			*(u32*)0x020665DC = 0xE3A02016; // mov r2, #0x16
			*(u32*)0x020665E0 = generateA7Instr(0x020665E0, 0x02065214); // bl 0x02065214
			*(u32*)0x020665E4 = 0xE8BD8008; // LDMFD SP!, {R3,PC}
			*(u32*)0x020665E8 = 0x027FF000;
		} */
		patchHiHeapDSiWare(0x02064C04, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x020660BC = 0xE1A00000; // nop
		*(u32*)0x020660C4 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Art Style: AQUITE (Europe, Australia)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAV") == 0) {
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		*(u32*)0x0203BC5C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203BC60 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203BD28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203BD2C = 0xE12FFF1E; // bx lr
		*(u32*)0x02051F10 = 0xE1A00000; // nop
		*(u32*)0x02054DE8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02054DEC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020584CC = 0xE1A00000; // nop
		*(u32*)0x02062D68 = 0xE1A00000; // nop
		*(u32*)0x02064B58 = 0xE1A00000; // nop
		*(u32*)0x02064B5C = 0xE1A00000; // nop
		*(u32*)0x02064B58 = 0xE1A00000; // nop
		*(u32*)0x02064CAC = 0xE1A00000; // nop
		*(u32*)0x02064CB0 = 0xE1A00000; // nop
		*(u32*)0x02064CB4 = 0xE1A00000; // nop
		*(u32*)0x02064CB8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02064D14, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x020661CC = 0xE1A00000; // nop
		*(u32*)0x020661D4 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Art Style: AQUARIO (Japan)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAJ") == 0) {
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		*(u32*)0x0203E250 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203E254 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203E324 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203E328 = 0xE12FFF1E; // bx lr
		*(u32*)0x0205446C = 0xE1A00000; // nop
		*(u32*)0x02057344 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02057348 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0205AA28 = 0xE1A00000; // nop
		*(u32*)0x020652C4 = 0xE1A00000; // nop
		*(u32*)0x020670B4 = 0xE1A00000; // nop
		*(u32*)0x020670B8 = 0xE1A00000; // nop
		*(u32*)0x020670C4 = 0xE1A00000; // nop
		*(u32*)0x02067208 = 0xE1A00000; // nop
		*(u32*)0x0206720C = 0xE1A00000; // nop
		*(u32*)0x02067210 = 0xE1A00000; // nop
		*(u32*)0x02067214 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02067270, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
	}

	// Everyday Soccer (USA)
	// DS Download Play requires 8MB of RAM
	else if (strcmp(romTid, "KAZE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x0200DC9C = 0xE1A00000; // nop
		*(u32*)0x02068414 = 0xE1A00000; // nop
		*(u32*)0x0206CA14 = 0xE1A00000; // nop
		*(u32*)0x02078504 = 0xE1A00000; // nop
		*(u32*)0x0207A470 = 0xE1A00000; // nop
		*(u32*)0x0207A474 = 0xE1A00000; // nop
		*(u32*)0x0207A480 = 0xE1A00000; // nop
		*(u32*)0x0207A5E0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0207A63C, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x0207A770 = 0x02299500;
		*(u32*)0x0207BCB4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207BCB8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BCC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207BCC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F4C4 = 0xE1A00000; // nop
	}

	// ARC Style: Everyday Football (Europe, Australia)
	// DS Download Play requires 8MB of RAM
	else if (strcmp(romTid, "KAZV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x0200DD70 = 0xE1A00000; // nop
		*(u32*)0x020684E8 = 0xE1A00000; // nop
		*(u32*)0x0206CAE8 = 0xE1A00000; // nop
		*(u32*)0x020785D8 = 0xE1A00000; // nop
		*(u32*)0x0207A544 = 0xE1A00000; // nop
		*(u32*)0x0207A548 = 0xE1A00000; // nop
		*(u32*)0x0207A554 = 0xE1A00000; // nop
		*(u32*)0x0207A6B4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0207A710, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x0207A844 = 0x02299500;
		*(u32*)0x0207BD88 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207BD8C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BD94 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207BD98 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F598 = 0xE1A00000; // nop
	}

	// ARC Style: Soccer! (Japan)
	// ARC Style: Soccer! (Korea)
	else if (strcmp(romTid, "KAZJ") == 0 || strcmp(romTid, "KAZK") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x0200DD70 = 0xE1A00000; // nop
		*(u32*)0x020683C8 = 0xE1A00000; // nop
		*(u32*)0x0206C9C8 = 0xE1A00000; // nop
		*(u32*)0x020784B8 = 0xE1A00000; // nop
		*(u32*)0x0207A424 = 0xE1A00000; // nop
		*(u32*)0x0207A428 = 0xE1A00000; // nop
		*(u32*)0x0207A434 = 0xE1A00000; // nop
		*(u32*)0x0207A594 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0207A5F0, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x0207A724 = 0x022993E0;
		*(u32*)0x0207BC68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207BC6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BC74 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207BC78 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F478 = 0xE1A00000; // nop
	}

	// Asphalt 4: Elite Racing (USA)
	// Does not boot (Black screens)
	/*else if (strcmp(romTid, "KA4E") == 0) {
		*(u32*)0x020050E0 = 0xE1A00000; // nop
		*(u32*)0x02031E08 = 0xE1A00000; // nop
		*(u32*)0x0204FA6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207951C = 0xE1A00000; // nop
		*(u32*)0x0207952C = 0xE1A00000; // nop
		*(u32*)0x0207954C = 0xE1A00000; // nop
		*(u32*)0x02079554 = 0xE1A00000; // nop
		*(u32*)0x0207955C = 0xE1A00000; // nop
		*(u32*)0x02079564 = 0xE1A00000; // nop
		*(u32*)0x02079578 = 0xE1A00000; // nop
		*(u32*)0x02079580 = 0xE1A00000; // nop
		*(u32*)0x02079588 = 0xE1A00000; // nop
		*(u32*)0x02079590 = 0xE1A00000; // nop
		*(u32*)0x020795B4 = 0xE1A00000; // nop
		*(u32*)0x0207ACB8 = 0xE1A00000; // nop
		*(u32*)0x0207B840 = 0xE1A00000; // nop
		*(u32*)0x0207B868 = 0xE1A00000; // nop
		*(u32*)0x0208FCC4 = 0xE1A00000; // nop
		*(u32*)0x0209868C = 0xE1A00000; // nop
		*(u32*)0x0209A5E4 = 0xE1A00000; // nop
		*(u32*)0x0209A5E8 = 0xE1A00000; // nop
		*(u32*)0x0209A5F4 = 0xE1A00000; // nop
		*(u32*)0x0209A738 = 0xE1A00000; // nop
	}*/

	// G.G. Series: Assault Buster (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KABE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200D83C = 0xE1A00000; // nop
		*(u32*)0x0204F59C = 0xE1A00000; // nop
		*(u32*)0x02053530 = 0xE1A00000; // nop
		*(u32*)0x02059980 = 0xE1A00000; // nop
		*(u32*)0x0205B81C = 0xE1A00000; // nop
		*(u32*)0x0205B820 = 0xE1A00000; // nop
		*(u32*)0x0205B82C = 0xE1A00000; // nop
		*(u32*)0x0205B98C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0205B9E8, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020607FC = 0xE1A00000; // nop
	}

	// G.G. Series: Assault Buster (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KABJ") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200A29C = 0xE1A00000; // nop
		*(u32*)0x020427A0 = 0xE1A00000; // nop
		*(u32*)0x020466A0 = 0xE1A00000; // nop
		*(u32*)0x0204C830 = 0xE1A00000; // nop
		*(u32*)0x0204E6C4 = 0xE1A00000; // nop
		*(u32*)0x0204E6C8 = 0xE1A00000; // nop
		*(u32*)0x0204E6D4 = 0xE1A00000; // nop
		*(u32*)0x0204E834 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204E890, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02053548 = 0xE1A00000; // nop
	}

	// Aura-Aura Climber (USA)
	else if (strcmp(romTid, "KSRE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005164 = 0xE1A00000; // nop
		*(u32*)0x020104A0 = 0xE1A00000; // nop
		*(u32*)0x02010508 = 0xE1A00000; // nop
		*(u32*)0x02026760 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203F500 = 0xE1A00000; // nop
		*(u32*)0x02042F10 = 0xE1A00000; // nop
		*(u32*)0x02049420 = 0xE1A00000; // nop
		*(u32*)0x0204B27C = 0xE1A00000; // nop
		*(u32*)0x0204B280 = 0xE1A00000; // nop
		*(u32*)0x0204B28C = 0xE1A00000; // nop
		*(u32*)0x0204B3EC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204B448, extendedMemory2 ? 0x02700000 : heapEnd);
		*(u32*)0x0204C52C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
	}

	// Aura-Aura Climber (Europe, Australia)
	else if (strcmp(romTid, "KSRV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005164 = 0xE1A00000; // nop
		*(u32*)0x0201066C = 0xE1A00000; // nop
		*(u32*)0x020106D4 = 0xE1A00000; // nop
		*(u32*)0x020265A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203F580 = 0xE1A00000; // nop
		*(u32*)0x02042F90 = 0xE1A00000; // nop
		*(u32*)0x020494A0 = 0xE1A00000; // nop
		*(u32*)0x0204B2FC = 0xE1A00000; // nop
		*(u32*)0x0204B300 = 0xE1A00000; // nop
		*(u32*)0x0204B30C = 0xE1A00000; // nop
		*(u32*)0x0204B46C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204B4C8, extendedMemory2 ? 0x02700000 : heapEnd);
		*(u32*)0x0204C5AC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
	}

	// Big Bass Arcade (USA)
	// Locks up on the first shown logos
	/*else if (strcmp(romTid, "K9GE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005120 = 0xE1A00000; // nop
		*(u32*)0x0200D83C = 0xE1A00000; // nop
		*(u32*)0x02010DDC = 0xE1A00000; // nop
		*(u32*)0x020168C0 = 0xE1A00000; // nop
		*(u32*)0x020186E8 = 0xE1A00000; // nop
		*(u32*)0x020186EC = 0xE1A00000; // nop
		*(u32*)0x020186F8 = 0xE1A00000; // nop
		*(u32*)0x02018858 = 0xE1A00000; // nop
		*(u32*)0x0203AF58 = 0xE12FFF1E; // bx lr
	}*/

	// BlayzBloo: Super Melee Brawlers Battle Royale (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBZE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0206B93C = 0xE1A00000; // nop
		*(u32*)0x0206F438 = 0xE1A00000; // nop
		*(u32*)0x02075718 = 0xE1A00000; // nop
		*(u32*)0x02077620 = 0xE1A00000; // nop
		*(u32*)0x02077624 = 0xE1A00000; // nop
		*(u32*)0x02077630 = 0xE1A00000; // nop
		*(u32*)0x02077790 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020777EC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0207BE88 = 0xE1A00000; // nop
	}

	// Bloons TD 4 (USA)
	// Unknown bug causes game to crash after popping the last bloon
	// Requires 8MB of RAM
	/*else if (strcmp(romTid, "KUVE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x0201C584 = 0xE1A00000; // nop
		*(u32*)0x0201FF40 = 0xE1A00000; // nop
		*(u32*)0x02023CF8 = 0xE1A00000; // nop
		*(u32*)0x02025A94 = 0xE1A00000; // nop
		*(u32*)0x02025A98 = 0xE1A00000; // nop
		*(u32*)0x02025AA4 = 0xE1A00000; // nop
		*(u32*)0x02025C04 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02025C60, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0202A344 = 0xE1A00000; // nop
	}*/

	// Bomberman Blitz (USA)
	// Bomberman Blitz (Europe, Australia)
	// Itsudemo Bomberman (Japan)
	// Controls must be reset in the Options menu to play the game properly
	else if (strncmp(romTid, "KBB", 3) == 0) {
		*(u32*)0x02008988 = 0xE1A00000; // nop
		*(u32*)0x0200C280 = 0xE1A00000; // nop
		*(u32*)0x02012524 = 0xE1A00000; // nop
		*(u32*)0x020146C4 = 0xE1A00000; // nop
		*(u32*)0x020146C8 = 0xE1A00000; // nop
		*(u32*)0x020146D4 = 0xE1A00000; // nop
		*(u32*)0x02014818 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02014874, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02015B60 = 0xE1A00000; // nop
		*(u32*)0x02015B68 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02015B88 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015B8C = 0xE12FFF1E; // bx lr
		*(u32*)0x02015B94 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015B98 = 0xE12FFF1E; // bx lr
		*(u32*)0x02015BB8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015BBC = 0xE12FFF1E; // bx lr
		*(u32*)0x02015BCC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015BD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02015BDC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02015BE0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02019670 = 0xE1A00000; // nop
		*(u32*)0x0201B8BC = 0xE3A00003; // mov r0, #3
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0204351C = 0xE1A00000; // nop
			*(u32*)0x02043528 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			*(u32*)0x020437AC = 0xE3A00001; // mov r0, #1
			*(u32*)0x020437B0 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FC20 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02098818 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0209881C = 0xE12FFF1E; // bx lr
		} else if (ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x020435E8 = 0xE1A00000; // nop
			*(u32*)0x020435F4 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			*(u32*)0x02043878 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0204387C = 0xE12FFF1E; // bx lr
		} else if (ndsHeader->gameCode[3] == 'J') {
			*(u32*)0x02043248 = 0xE1A00000; // nop
			*(u32*)0x02043254 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			*(u32*)0x020434D8 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020434DC = 0xE12FFF1E; // bx lr
		}
	}

	// Art Style: BOXLIFE (USA)
	else if (strcmp(romTid, "KAHE") == 0) {
		*(u32*)0x0202FBD0 = 0xE1A00000; // nop
		*(u32*)0x020355D8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020355DC = 0xE12FFF1E; // bx lr
		*(u32*)0x020356C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020356C8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02035DC0 = 0xE1A00000; // nop
		*(u32*)0x02035DD0 = 0xE1A00000; // nop
		*(u32*)0x02035DE0 = 0xE1A00000; // nop
		*(u32*)0x02036060 = 0xE1A00000; // nop
		*(u32*)0x0203606C = 0xE1A00000; // nop
		*(u32*)0x02036088 = 0xE1A00000; // nop
		*(u32*)0x02055990 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02055994 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02058F84 = 0xE1A00000; // nop
		*(u32*)0x02060EE0 = 0xE1A00000; // nop
		*(u32*)0x02062D18 = 0xE1A00000; // nop
		*(u32*)0x02062D1C = 0xE1A00000; // nop
		*(u32*)0x02062D28 = 0xE1A00000; // nop
		*(u32*)0x02062E88 = 0xE1A00000; // nop
		*(u32*)0x02062E8C = 0xE1A00000; // nop
		*(u32*)0x02062E90 = 0xE1A00000; // nop
		*(u32*)0x02062E94 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062EF0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02064224 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
	}

	// Art Style: BOXLIFE (Europe, Australia)
	else if (strcmp(romTid, "KAHV") == 0) {
		*(u32*)0x0202FB18 = 0xE1A00000; // nop
		*(u32*)0x02035220 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02035224 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203530C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02035310 = 0xE12FFF1E; // bx lr
		*(u32*)0x02035A08 = 0xE1A00000; // nop
		*(u32*)0x02035A18 = 0xE1A00000; // nop
		*(u32*)0x02035A28 = 0xE1A00000; // nop
		*(u32*)0x02035CA8 = 0xE1A00000; // nop
		*(u32*)0x02035CB4 = 0xE1A00000; // nop
		*(u32*)0x02035CD0 = 0xE1A00000; // nop
		*(u32*)0x02055694 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02055698 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02058C88 = 0xE1A00000; // nop
		*(u32*)0x02060BE4 = 0xE1A00000; // nop
		*(u32*)0x02062A1C = 0xE1A00000; // nop
		*(u32*)0x02062A20 = 0xE1A00000; // nop
		*(u32*)0x02062A2C = 0xE1A00000; // nop
		*(u32*)0x02062B8C = 0xE1A00000; // nop
		*(u32*)0x02062B90 = 0xE1A00000; // nop
		*(u32*)0x02062B94 = 0xE1A00000; // nop
		*(u32*)0x02062B98 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062BF4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02063F28 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
	}

	// Art Style: Hacolife (Japan)
	else if (strcmp(romTid, "KAHJ") == 0) {
		*(u32*)0x0202F148 = 0xE1A00000; // nop
		*(u32*)0x0203456C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02034570 = 0xE12FFF1E; // bx lr
		*(u32*)0x02034658 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203465C = 0xE12FFF1E; // bx lr
		*(u32*)0x02034D54 = 0xE1A00000; // nop
		*(u32*)0x02034D64 = 0xE1A00000; // nop
		*(u32*)0x02034D74 = 0xE1A00000; // nop
		*(u32*)0x02034FF4 = 0xE1A00000; // nop
		*(u32*)0x02035000 = 0xE1A00000; // nop
		*(u32*)0x0203501C = 0xE1A00000; // nop
		*(u32*)0x02054C10 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02054C14 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020583A8 = 0xE1A00000; // nop
		*(u32*)0x02060A64 = 0xE1A00000; // nop
		*(u32*)0x020628C4 = 0xE1A00000; // nop
		*(u32*)0x020628C8 = 0xE1A00000; // nop
		*(u32*)0x020628D4 = 0xE1A00000; // nop
		*(u32*)0x02062A18 = 0xE1A00000; // nop
		*(u32*)0x02062A1C = 0xE1A00000; // nop
		*(u32*)0x02062A20 = 0xE1A00000; // nop
		*(u32*)0x02062A24 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062A80, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02063DE4 = 0xE1A00000; // nop
		*(u32*)0x02063DEC = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Bugs'N'Balls (USA)
	// Bugs'N'Balls (Europe)
	else if (strncmp(romTid, "KKQ", 3) == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201B334 = 0xE1A00000; // nop
		*(u32*)0x0201E7B0 = 0xE1A00000; // nop
		*(u32*)0x02022F3C = 0xE1A00000; // nop
		*(u32*)0x02024D6C = 0xE1A00000; // nop
		*(u32*)0x02024D70 = 0xE1A00000; // nop
		*(u32*)0x02024D7C = 0xE1A00000; // nop
		*(u32*)0x02024EDC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02024F38, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0202637C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02029104 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02062F20 = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'P') {
			*(u32*)0x02064B7C = 0xE1A00000; // nop
		}
	}

	// Cake Ninja (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K2JE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008C4C = 0xE1A00000; // nop
		*(u32*)0x02008DE4 = 0xE1A00000; // nop
		*(u32*)0x02057938 = 0xE1A00000; // nop
		*(u32*)0x0205B0FC = 0xE1A00000; // nop
		*(u32*)0x02060CAC = 0xE1A00000; // nop
		*(u32*)0x02062CB0 = 0xE1A00000; // nop
		*(u32*)0x02062CB4 = 0xE1A00000; // nop
		*(u32*)0x02062CC0 = 0xE1A00000; // nop
		*(u32*)0x02062E20 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062E7C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02064424 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020679A0 = 0xE1A00000; // nop
	}

	// Cake Ninja 2 (Europe)
	// Locks up on the game's logo
	/*else if (strcmp(romTid, "K2NP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008880 = 0xE1A00000; // nop
		*(u32*)0x02008A88 = 0xE1A00000; // nop
		*(u32*)0x02077508 = 0xE1A00000; // nop
		*(u32*)0x0207ACCC = 0xE1A00000; // nop
		*(u32*)0x020808A8 = 0xE1A00000; // nop
		*(u32*)0x020828AC = 0xE1A00000; // nop
		*(u32*)0x020828B0 = 0xE1A00000; // nop
		*(u32*)0x020828BC = 0xE1A00000; // nop
		*(u32*)0x02082A1C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02082A78, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02084020 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0208759C = 0xE1A00000; // nop
	}*/

	// Cake Ninja: XMAS (USA)
	// Locks up on Please Wait screen(?)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYNE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008406 = 0xE1A00000; // nop
		*(u32*)0x0200846C = 0xE1A00000; // nop
		*(u32*)0x02050348 = 0xE1A00000; // nop
		*(u32*)0x02053B0C = 0xE1A00000; // nop
		*(u32*)0x020597E8 = 0xE1A00000; // nop
		*(u32*)0x0205B7EC = 0xE1A00000; // nop
		*(u32*)0x0205B7F0 = 0xE1A00000; // nop
		*(u32*)0x0205B7FC = 0xE1A00000; // nop
		*(u32*)0x0205B95C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062E7C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020604DC = 0xE1A00000; // nop
	}

	// Calculator (USA)
	else if (strcmp(romTid, "KCYE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201F6A8 = 0xE1A00000; // nop
		*(u32*)0x020239D8 = 0xE1A00000; // nop
		*(u32*)0x02027270 = 0xE1A00000; // nop
		*(u32*)0x02029038 = 0xE1A00000; // nop
		*(u32*)0x0202903C = 0xE1A00000; // nop
		*(u32*)0x02029048 = 0xE1A00000; // nop
		*(u32*)0x020291A8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02029204, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0202A590 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202DA48 = 0xE1A00000; // nop
		*(u32*)0x0203ED20 = 0xE1A00000; // nop
		*(u32*)0x0204D1FC = 0xE12FFF1E; // bx lr
	}

	// Calculator (Europe, Australia)
	else if (strcmp(romTid, "KCYV") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201503C = 0xE1A00000; // nop
		*(u32*)0x0201937C = 0xE1A00000; // nop
		*(u32*)0x0201CC14 = 0xE1A00000; // nop
		*(u32*)0x0201E9DC = 0xE1A00000; // nop
		*(u32*)0x0201E9E0 = 0xE1A00000; // nop
		*(u32*)0x0201E9EC = 0xE1A00000; // nop
		*(u32*)0x0201EB4C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201EBA8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201FF34 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020233EC = 0xE1A00000; // nop
		*(u32*)0x020346D0 = 0xE1A00000; // nop
		*(u32*)0x02042B6C = 0xE12FFF1E; // bx lr
	}

	// Candle Route (USA)
	// Candle Route (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "K9YE") == 0 || strcmp(romTid, "K9YP") == 0) && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020175F4 = 0xE1A00000; // nop
		*(u32*)0x0201ADD4 = 0xE1A00000; // nop
		*(u32*)0x0201FE10 = 0xE1A00000; // nop
		*(u32*)0x02021CA0 = 0xE1A00000; // nop
		*(u32*)0x02021CA4 = 0xE1A00000; // nop
		*(u32*)0x02021CB0 = 0xE1A00000; // nop
		*(u32*)0x02021E10 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02021E6C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02023108 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202661C = 0xE1A00000; // nop
		*(u32*)0x0207DEB8 = 0xE1A00000; // nop
		*(u32*)0x0207DEBC = 0xE1A00000; // nop
		*(u32*)0x0207DEC0 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x020AE44C = 0xE1A00000; // nop

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020AE76C;
				offset[i] = 0xE1A00000; // nop
			}
		} else {
			*(u32*)0x020AE4F0 = 0xE1A00000; // nop

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020AE810;
				offset[i] = 0xE1A00000; // nop
			}
		}
	}

	// Castle Conqueror (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KCNE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201D8B8 = 0xE1A00000; // nop
		*(u32*)0x0201D8BC = 0xE1A00000; // nop
		*(u32*)0x0201D8C8 = 0xE1A00000; // nop
		*(u32*)0x0201DA28 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201DA84, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201ECBC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02021FA4 = 0xE1A00000; // nop
		*(u32*)0x02024740 = 0xE1A00000; // nop
		*(u32*)0x02027A00 = 0xE1A00000; // nop
		*(u32*)0x0202B438 = 0xE1A00000; // nop
		*(u32*)0x0204AFD8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204B084 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204B44C = 0xE12FFF1E; // bx lr
	}

	// Castle Conqueror (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KCNP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02016340 = 0xE1A00000; // nop
		*(u32*)0x02019694 = 0xE1A00000; // nop
		*(u32*)0x0201D330 = 0xE1A00000; // nop
		*(u32*)0x0201F0CC = 0xE1A00000; // nop
		*(u32*)0x0201F0D0 = 0xE1A00000; // nop
		*(u32*)0x0201F0DC = 0xE1A00000; // nop
		*(u32*)0x0201F23C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F298, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020204E0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023960 = 0xE1A00000; // nop
		*(u32*)0x0203A5A4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203A7D4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203AB6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0203B134 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203BA14 = 0xE12FFF1E; // bx lr
	}

	// Castle Conqueror: Against (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KQNE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02015B2C = 0xE1A00000; // nop
		*(u32*)0x02018DA4 = 0xE1A00000; // nop
		*(u32*)0x0201CE90 = 0xE1A00000; // nop
		*(u32*)0x0201EC3C = 0xE1A00000; // nop
		*(u32*)0x0201EC40 = 0xE1A00000; // nop
		*(u32*)0x0201EC4C = 0xE1A00000; // nop
		*(u32*)0x0201EDAC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201EE08, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020200E4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023594 = 0xE1A00000; // nop
		*(u32*)0x0206ACC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206AEC8 = 0xE12FFF1E; // bx lr
	}

	// Castle Conqueror: Against (Europe, Australia)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KQNV") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02015B2C = 0xE1A00000; // nop
		*(u32*)0x02018E0C = 0xE1A00000; // nop
		*(u32*)0x0201CEF8 = 0xE1A00000; // nop
		*(u32*)0x0201ECA4 = 0xE1A00000; // nop
		*(u32*)0x0201ECA8 = 0xE1A00000; // nop
		*(u32*)0x0201ECB4 = 0xE1A00000; // nop
		*(u32*)0x0201EE14 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201EE70, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0202014C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020235FC = 0xE1A00000; // nop
		*(u32*)0x02041BF8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02041DFC = 0xE12FFF1E; // bx lr
	}

	// Castle Conqueror: Heroes (USA)
	// Castle Conqueror: Heroes (Japan)
	else if (strcmp(romTid, "KC5E") == 0 || strcmp(romTid, "KC5J") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02017744 = 0xE1A00000; // nop
		*(u32*)0x0201AB7C = 0xE1A00000; // nop
		*(u32*)0x0201E754 = 0xE1A00000; // nop
		*(u32*)0x020205D0 = 0xE1A00000; // nop
		*(u32*)0x020205D4 = 0xE1A00000; // nop
		*(u32*)0x020205E0 = 0xE1A00000; // nop
		*(u32*)0x02020740 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202079C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020208D0 -= 0x30000;
		*(u32*)0x02021A68 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02024730 = 0xE1A00000; // nop
	}

	// Castle Conqueror: Heroes (Europe, Australia)
	else if (strcmp(romTid, "KC5V") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02017670 = 0xE1A00000; // nop
		*(u32*)0x0201AAA8 = 0xE1A00000; // nop
		*(u32*)0x0201E680 = 0xE1A00000; // nop
		*(u32*)0x020204FC = 0xE1A00000; // nop
		*(u32*)0x02020500 = 0xE1A00000; // nop
		*(u32*)0x0202050C = 0xE1A00000; // nop
		*(u32*)0x0202066C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020206C8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020207FC = 0x022C9BA0;
		*(u32*)0x02021994 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202465C = 0xE1A00000; // nop
	}

	// Castle Conqueror: Heroes 2 (USA)
	// Castle Conqueror: Heroes 2 (Europe, Australia)
	// Castle Conqueror: Heroes 2 (Japan)
	// Requires 8MB of RAM
	else if (strncmp(romTid, "KXC", 3) == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02013170 = 0xE1A00000; // nop
		*(u32*)0x020164C4 = 0xE1A00000; // nop
		*(u32*)0x0201A118 = 0xE1A00000; // nop
		*(u32*)0x0201BEB4 = 0xE1A00000; // nop
		*(u32*)0x0201BEB8 = 0xE1A00000; // nop
		*(u32*)0x0201BEC4 = 0xE1A00000; // nop
		*(u32*)0x0201C024 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C080, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201D2C8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02020710 = 0xE1A00000; // nop
	}

	// Castle Conqueror: Revolution (USA)
	// Castle Conqueror: Revolution (Europe, Australia)
	// Requires 8MB of RAM
	else if (strncmp(romTid, "KQN", 3) == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02015B4C = 0xE1A00000; // nop
		*(u32*)0x02018E2C = 0xE1A00000; // nop
		*(u32*)0x0201CF18 = 0xE1A00000; // nop
		*(u32*)0x0201ECC4 = 0xE1A00000; // nop
		*(u32*)0x0201ECC8 = 0xE1A00000; // nop
		*(u32*)0x0201ECD4 = 0xE1A00000; // nop
		*(u32*)0x0201EE34 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C080, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020201DC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202368C = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x020642A4 = 0xE12FFF1E; // bx lr
			*(u32*)0x020644BC = 0xE12FFF1E; // bx lr
		} else if (ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x02066CA8 = 0xE12FFF1E; // bx lr
			*(u32*)0x02066EC0 = 0xE12FFF1E; // bx lr
		}
	}

	// Cave Story (USA)
	else if (strcmp(romTid, "KCVE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02005980 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005A68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005B60 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200A12C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0207342C = 0xE1A00000; // nop
		*(u32*)0x0207654C = 0xE1A00000; // nop
		*(u32*)0x0207A454 = 0xE1A00000; // nop
		*(u32*)0x0207C20C = 0xE1A00000; // nop
		*(u32*)0x0207C210 = 0xE1A00000; // nop
		*(u32*)0x0207C21C = 0xE1A00000; // nop
		*(u32*)0x0207C37C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0207C3D8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0207D758 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02080F68 = 0xE1A00000; // nop
	}

	// Chronos Twins: One Hero in Two Times (USA)
	// Overlay-related crash
	/*else if (strcmp(romTid, "K9TE") == 0) {
		*(u32*)0x0200B7AC = 0xE1A00000; // nop
		*(u32*)0x0200F400 = 0xE1A00000; // nop
		*(u32*)0x02013488 = 0xE1A00000; // nop
		*(u32*)0x0201526C = 0xE1A00000; // nop
		*(u32*)0x02015270 = 0xE1A00000; // nop
		*(u32*)0x0201527C = 0xE1A00000; // nop
		*(u32*)0x020153C0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201541C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02019000 = 0xE1A00000; // nop
	}*/

	// Chuck E. Cheese's Alien Defense Force (USA)
	else if (strcmp(romTid, "KUQE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200BB6C = 0xE1A00000; // nop
		*(u32*)0x0200F008 = 0xE1A00000; // nop
		*(u32*)0x02012354 = 0xE1A00000; // nop
		*(u32*)0x020140F0 = 0xE1A00000; // nop
		*(u32*)0x020140F4 = 0xE1A00000; // nop
		*(u32*)0x02014100 = 0xE1A00000; // nop
		*(u32*)0x02014260 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020142BC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02015504 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201829C = 0xE1A00000; // nop
		*(u32*)0x0201B9E4 = 0xE1A00000; // nop

		// Skip Manual screen
		for (int i = 0; i < 4; i++) {
			u32* offset = (u32*)0x0202D43C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Chuck E. Cheese's Arcade Room (USA)
	else if (strcmp(romTid, "KUCE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02013978 = 0xE1A00000; // nop
		*(u32*)0x02016E14 = 0xE1A00000; // nop
		*(u32*)0x0201A2B0 = 0xE1A00000; // nop
		*(u32*)0x0201C04C = 0xE1A00000; // nop
		*(u32*)0x0201C050 = 0xE1A00000; // nop
		*(u32*)0x0201C05C = 0xE1A00000; // nop
		*(u32*)0x0201C1BC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C218, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201D460 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020201F8 = 0xE1A00000; // nop
		*(u32*)0x02032550 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x020459F0 = 0xE1A00000; // nop
	}

	// Color Commando (USA)
	// Color Commando (Europe) (Rev 0)
	else if (strcmp(romTid, "KXFE") == 0 || (strcmp(romTid, "KXFP") == 0 && ndsHeader->romversion == 0)) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x0200AD6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200CF04 = 0xE1A00000; // nop
		*(u32*)0x0202D958 = 0xE1A00000; // nop
		*(u32*)0x02030BC4 = 0xE1A00000; // nop
		*(u32*)0x02033A58 = 0xE1A00000; // nop
		*(u32*)0x020357F4 = 0xE1A00000; // nop
		*(u32*)0x020357F8 = 0xE1A00000; // nop
		*(u32*)0x02035804 = 0xE1A00000; // nop
		*(u32*)0x02035964 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020359C0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02036D70 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02039BF0 = 0xE1A00000; // nop
	}

	// Color Commando (Europe) (Rev 1)
	else if (strcmp(romTid, "KXFP") == 0 && ndsHeader->romversion == 1) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x0200AD6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200CF04 = 0xE1A00000; // nop
		*(u32*)0x0202D884 = 0xE1A00000; // nop
		*(u32*)0x02030AF0 = 0xE1A00000; // nop
		*(u32*)0x02033984 = 0xE1A00000; // nop
		*(u32*)0x02035720 = 0xE1A00000; // nop
		*(u32*)0x02035724 = 0xE1A00000; // nop
		*(u32*)0x02035730 = 0xE1A00000; // nop
		*(u32*)0x02035890 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020358EC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02036C9C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02039B1C = 0xE1A00000; // nop
	}

	// CuteWitch! runner (USA)
	// CuteWitch! runner (Europe)
	// Stage music doesn't play on retail consoles
	else if (strncmp(romTid, "K32", 3) == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201B8CC = 0xE1A00000; // nop
		*(u32*)0x0201ED48 = 0xE1A00000; // nop
		*(u32*)0x020234D4 = 0xE1A00000; // nop
		*(u32*)0x02025304 = 0xE1A00000; // nop
		*(u32*)0x02025308 = 0xE1A00000; // nop
		*(u32*)0x02025314 = 0xE1A00000; // nop
		*(u32*)0x02025474 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020254D0, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x02026978 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02029700 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02062068 = 0xE12FFF1E; // bx lr
		} else if (ndsHeader->gameCode[3] == 'P') {
			*(u32*)0x02093AA4 = 0xE12FFF1E; // bx lr
		}
	}

	// Crash-Course Domo (USA)
	else if (strcmp(romTid, "KDCE") == 0) {
		*(u16*)0x0200DF38 = 0x2001; // movs r0, #1
		*(u16*)0x0200DF3A = 0x4770; // bx lr
		*(u16*)0x020153C4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		*(u16*)0x02015418 = 0x46C0; // nop
		doubleNopT(0x02023C72);
		doubleNopT(0x02025EC2);
		doubleNopT(0x02028B44);
		doubleNopT(0x0202A0FE);
		doubleNopT(0x0202A102);
		doubleNopT(0x0202A10E);
		doubleNopT(0x0202A1F2);
		patchHiHeapDSiWareThumb(0x0202A230, 0x208F, 0x0480); // movs r0, #0x23C0000
		doubleNopT(0x0202B2B6);
		*(u16*)0x0202B2BA = 0x46C0; // nop
		*(u16*)0x0202B2BC = 0x46C0; // nop
		doubleNopT(0x0202B2BE);
		doubleNopT(0x0202D372);
	}

	// Dark Void Zero (USA)
	// Dark Void Zero (Europe, Australia)
	else if (strcmp(romTid, "KDVE") == 0 || strcmp(romTid, "KDVV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02018A3C = 0xE1A00000; // nop
		*(u32*)0x02018A4C = 0xE1A00000; // nop
		*(u32*)0x02046DD8 = 0xE1A00000; // nop
		*(u32*)0x0204CC24 = 0xE1A00000; // nop
		*(u32*)0x0204EE80 = 0xE1A00000; // nop
		*(u32*)0x0204EF18 = 0xE1A00000; // nop
		*(u32*)0x0204EF1C = 0xE1A00000; // nop
		*(u32*)0x0204EF28 = 0xE1A00000; // nop
		*(u32*)0x0204F06C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204F0C8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0205052C = 0xE1A00000; // nop
		*(u32*)0x02050534 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02052BD4 = 0xE1A00000; // nop
		*(u32*)0x02052C00 = 0xE1A00000; // nop
		*(u32*)0x02054EF8 = 0xE1A00000; // nop
		*(u32*)0x02058A24 = 0xE1A00000; // nop
		*(u32*)0x02059C44 = 0xE1A00000; // nop
		*(u16*)0x020851A4 = 0x46C0; // nop
		*(u16*)0x020851A6 = 0x46C0; // nop
		*(u32*)0x020891BC = 0xE1A00000; // nop
		*(u32*)0x0208AE4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0208B008 = 0xE12FFF1E; // bx lr
	}

	// Dairojo! Samurai Defenders (USA)
	else if (strcmp(romTid, "KF3E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200DDB4 = 0xE1A00000; // nop
		*(u32*)0x02011580 = 0xE1A00000; // nop
		*(u32*)0x0201BCD4 = 0xE1A00000; // nop
		*(u32*)0x0201DB50 = 0xE1A00000; // nop
		*(u32*)0x0201DB54 = 0xE1A00000; // nop
		*(u32*)0x0201DB60 = 0xE1A00000; // nop
		*(u32*)0x0201DCC0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201DD1C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201EFDC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EFE0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EFE8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EFEC = 0xE12FFF1E; // bx lr
		*(u32*)0x02022418 = 0xE1A00000; // nop
	}

	// GO Series: Defense Wars (USA)
	// GO Series: Defence Wars (Europe)
	else if (strcmp(romTid, "KWTE") == 0 || strcmp(romTid, "KWTP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200722C = 0xE1A00000; // nop
		*(u32*)0x0200B350 = 0xE1A00000; // nop
		*(u32*)0x02049F68 = 0xE1A00000; // nop
		*(u32*)0x0204DC94 = 0xE1A00000; // nop
		*(u32*)0x020537F4 = 0xE1A00000; // nop
		*(u32*)0x020555D4 = 0xE1A00000; // nop
		*(u32*)0x020555D8 = 0xE1A00000; // nop
		*(u32*)0x020555E4 = 0xE1A00000; // nop
		*(u32*)0x02055744 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020557A0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02056A24 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205A134 = 0xE1A00000; // nop

		// Manual screen
		/* *(u32*)0x0200CB0C = 0xE1A00000; // nop
		*(u32*)0x0200CB10 = 0xE1A00000; // nop
		*(u32*)0x0200CB34 = 0xE1A00000; // nop
		*(u32*)0x0200CB50 = 0xE1A00000; // nop
		*(u32*)0x0200CB80 = 0xE1A00000; // nop */

		// Skip
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200CC98;
			offset[i] = 0xE1A00000; // nop
		}

		/*for (int i = 0; i < 10; i++) {
			u32* offset = (u32*)0x0203B3A0;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x0203CAD4 = 0xEB006798; // bl 0x205693C
		for (int i = 0; i < 6; i++) {
			u32* offset = (u32*)0x0203CD60;
			offset[i] = 0xE1A00000; // nop
		}*/
	}

	// DotMan (USA)
	else if (strcmp(romTid, "KHEE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005358 = 0xE1A00000; // nop
		*(u32*)0x0200E038 = 0xE1A00000; // nop
		*(u32*)0x0201158C = 0xE1A00000; // nop
		*(u32*)0x02014F78 = 0xE1A00000; // nop
		*(u32*)0x02016D0C = 0xE1A00000; // nop
		*(u32*)0x02016D10 = 0xE1A00000; // nop
		*(u32*)0x02016D1C = 0xE1A00000; // nop
		*(u32*)0x02016E7C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016ED8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018120 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201ADA8 = 0xE1A00000; // nop
		*(u32*)0x0201D1A0 = 0xE1A00000; // nop

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02022600;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (Europe)
	else if (strcmp(romTid, "KHEP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005370 = 0xE1A00000; // nop
		*(u32*)0x0200E074 = 0xE1A00000; // nop
		*(u32*)0x02011650 = 0xE1A00000; // nop
		*(u32*)0x02015050 = 0xE1A00000; // nop
		*(u32*)0x02016DEC = 0xE1A00000; // nop
		*(u32*)0x02016DF0 = 0xE1A00000; // nop
		*(u32*)0x02016DFC = 0xE1A00000; // nop
		*(u32*)0x02016F5C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016FB8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018200 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201AE88 = 0xE1A00000; // nop
		*(u32*)0x0201D280 = 0xE1A00000; // nop

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020226DC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (Japan)
	else if (strcmp(romTid, "KHEJ") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005358 = 0xE1A00000; // nop
		*(u32*)0x0200E014 = 0xE1A00000; // nop
		*(u32*)0x02011568 = 0xE1A00000; // nop
		*(u32*)0x02014F54 = 0xE1A00000; // nop
		*(u32*)0x02016CE8 = 0xE1A00000; // nop
		*(u32*)0x02016CEC = 0xE1A00000; // nop
		*(u32*)0x02016CF8 = 0xE1A00000; // nop
		*(u32*)0x02016E58 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016EB4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020180FC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201AD84 = 0xE1A00000; // nop
		*(u32*)0x0201D080 = 0xE1A00000; // nop

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0202248C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Dr. Mario Express (USA)
	// A Little Bit of... Dr. Mario (Europe, Australia)
	else if (strcmp(romTid, "KD9E") == 0 || strcmp(romTid, "KD9V") == 0) {
		*(u32*)0x020103C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02013A08 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02019DF4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201B724 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201B728 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201BC0C = 0xE1A00000; // nop
		*(u32*)0x0201BC10 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BC28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BD70 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BD88 = 0xE1A00000; // nop (Leave MPU region 1 untouched)
		*(u32*)0x0201BE0C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BE3C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BF10 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BF40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201CF08 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D2A8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020248C4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02025CD4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0203D488 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203D48C = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02044B00 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02058F68 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02058F6C = 0xE12FFF1E; // bx lr
			*(u32*)0x0205990C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206F430 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0207347C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020736DC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0207401C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02074054 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		} else {
			*(u32*)0x02044A9C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02058E58 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02058E5C = 0xE12FFF1E; // bx lr
			*(u32*)0x020597FC = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206F320 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0207336C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020735CC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02073F0C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02073F44 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		}
	}

	// Chotto Dr. Mario (Japan)
	else if (strcmp(romTid, "KD9J") == 0) {
		*(u32*)0x020052B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02010B08 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02013E58 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201A244 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BB74 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201BB78 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201C05C = 0xE1A00000; // nop
		*(u32*)0x0201C060 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C078 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C1C0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C1D8 = 0xE1A00000; // nop (Leave MPU region 1 untouched)
		*(u32*)0x0201C25C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C28C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C360 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C390 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D358 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D6F8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02024CF4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02026104 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202D3B4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202D644 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202DFB8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0202DFF0 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		*(u32*)0x020584B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020584B8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0205F6F0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207356C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02073570 = 0xE12FFF1E; // bx lr
		*(u32*)0x02073F10 = 0xE3A00000; // mov r0, #0
	}

	// Dragon's Lair (USA)
	else if (strcmp(romTid, "KDLE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop
		*(u32*)0x02012064 = 0xE1A00000; // nop
		*(u32*)0x02012068 = 0xE1A00000; // nop
		for (int i = 0; i < 5; i++) {
			u32* offset1 = (u32*)0x020132C0;
			u32* offset2 = (u32*)0x020135FC;
			u32* offset3 = (u32*)0x02013A44;
			u32* offset4 = (u32*)0x02014DA8;
			u32* offset5 = (u32*)0x02016134;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
			offset3[i] = 0xE1A00000; // nop
			offset4[i] = 0xE1A00000; // nop
			offset5[i] = 0xE1A00000; // nop
		}
		*(u32*)0x0202FACC = 0xE1A00000; // nop
		*(u32*)0x0202FC00 = 0xE1A00000; // nop
		*(u32*)0x0202FC14 = 0xE1A00000; // nop
		*(u32*)0x02033044 = 0xE1A00000; // nop
		*(u32*)0x02036A4C = 0xE1A00000; // nop
		*(u32*)0x02038868 = 0xE1A00000; // nop
		*(u32*)0x0203886C = 0xE1A00000; // nop
		*(u32*)0x02038878 = 0xE1A00000; // nop
		*(u32*)0x020389BC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02038A18, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x02038B4C = 0x02084600;
		*(u32*)0x0203A0D0 = 0xE1A00000; // nop
		*(u32*)0x0203A0D8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0203A53C = 0xE1A00000; // nop
		*(u32*)0x0203A540 = 0xE1A00000; // nop
		*(u32*)0x0203A544 = 0xE1A00000; // nop
		*(u32*)0x0203A548 = 0xE1A00000; // nop
		*(u32*)0x0203D0A4 = 0xE1A00000; // nop
	}

	// Dragon's Lair (Europe, Australia)
	else if (strcmp(romTid, "KDLV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop
		*(u32*)0x0201205C = 0xE1A00000; // nop
		*(u32*)0x02012060 = 0xE1A00000; // nop
		for (int i = 0; i < 5; i++) {
			u32* offset1 = (u32*)0x020132B4;
			u32* offset2 = (u32*)0x020135F0;
			u32* offset3 = (u32*)0x02013A38;
			u32* offset4 = (u32*)0x02014D9C;
			u32* offset5 = (u32*)0x02016128;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
			offset3[i] = 0xE1A00000; // nop
			offset4[i] = 0xE1A00000; // nop
			offset5[i] = 0xE1A00000; // nop
		}
		*(u32*)0x0202FAC0 = 0xE1A00000; // nop
		*(u32*)0x0202FBF4 = 0xE1A00000; // nop
		*(u32*)0x0202FC08 = 0xE1A00000; // nop
		*(u32*)0x02033038 = 0xE1A00000; // nop
		*(u32*)0x02036A40 = 0xE1A00000; // nop
		*(u32*)0x0203885C = 0xE1A00000; // nop
		*(u32*)0x02038860 = 0xE1A00000; // nop
		*(u32*)0x0203886C = 0xE1A00000; // nop
		*(u32*)0x020389B0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02038A0C, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x02038B40 = 0x02084600;
		*(u32*)0x0203A0C4 = 0xE1A00000; // nop
		*(u32*)0x0203A0CC = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0203A530 = 0xE1A00000; // nop
		*(u32*)0x0203A534 = 0xE1A00000; // nop
		*(u32*)0x0203A538 = 0xE1A00000; // nop
		*(u32*)0x0203A53C = 0xE1A00000; // nop
		*(u32*)0x0203D098 = 0xE1A00000; // nop
	}

	// Dragon's Lair II: Time Warp (USA)
	else if (strcmp(romTid, "KLYE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop
		*(u32*)0x020171CC = 0xE1A00000; // nop
		*(u32*)0x020171D0 = 0xE1A00000; // nop
		*(u32*)0x02033EE0 = 0xE1A00000; // nop
		*(u32*)0x02037300 = 0xE1A00000; // nop
		*(u32*)0x0203AB00 = 0xE1A00000; // nop
		*(u32*)0x0203C8C0 = 0xE1A00000; // nop
		*(u32*)0x0203C8C4 = 0xE1A00000; // nop
		*(u32*)0x0203C8D0 = 0xE1A00000; // nop
		*(u32*)0x0203CA30 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203CA8C, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0203CBC0 = 0x02089260;
		*(u32*)0x0203E13C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02041040 = 0xE1A00000; // nop
	}

	// Dragon's Lair II: Time Warp (Europe, Australia)
	// Crashes on company logos (Cause unknown)
	else if (strcmp(romTid, "KLYV") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050EC = 0xE1A00000; // nop
		*(u32*)0x020051E0 = 0xE1A00000; // nop
		*(u32*)0x020171E8 = 0xE1A00000; // nop
		*(u32*)0x020171EC = 0xE1A00000; // nop
		*(u32*)0x02033F64 = 0xE1A00000; // nop
		*(u32*)0x02036AE8 = 0xE1A00000; // nop
		*(u32*)0x0203740C = 0xE1A00000; // nop
		*(u32*)0x0203AC20 = 0xE1A00000; // nop
		*(u32*)0x0203C9E8 = 0xE1A00000; // nop
		*(u32*)0x0203C9EC = 0xE1A00000; // nop
		*(u32*)0x0203C9F8 = 0xE1A00000; // nop
		*(u32*)0x0203CB58 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203CBB4, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0203CCE8 = 0x020894E0;
		*(u32*)0x0203E264 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02041168 = 0xE1A00000; // nop
	}

	// DS WiFi Settings
	else if (strcmp(romTid, "B88A") == 0) {
		const u16* branchCode = generateA7InstrThumb(0x020051F4, (int)ce9->thumbPatches->reset_arm9);

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u16*)0x020051F4 = branchCode[0];
		*(u16*)0x020051F6 = branchCode[1];
		*(u32*)0x02005224 = 0xFFFFFFFF;
		*(u16*)0x0202FAEE = 0x46C0; // nop
		*(u16*)0x0202FAF0 = 0x46C0; // nop
		*(u16*)0x02031B3E = 0x46C0; // nop
		*(u16*)0x02031B40 = 0x46C0; // nop
		*(u16*)0x020344A0 = 0x46C0; // nop
		*(u16*)0x020344A2 = 0x46C0; // nop
		*(u16*)0x02036532 = 0x46C0; // nop
		*(u16*)0x02036534 = 0x46C0; // nop
		*(u16*)0x02036536 = 0x46C0; // nop
		*(u16*)0x02036538 = 0x46C0; // nop
		*(u16*)0x02036542 = 0x46C0; // nop
		*(u16*)0x02036544 = 0x46C0; // nop
		*(u16*)0x02036626 = 0x46C0; // nop
		*(u16*)0x02036628 = 0x46C0; // nop
		*(u16*)0x020374D8 = 0x2001; // movs r0, #1
		*(u16*)0x020374DA = 0x4770; // bx lr
		*(u16*)0x02037510 = 0x2000; // movs r0, #0
		*(u16*)0x02037512 = 0x4770; // bx lr
		*(u16*)0x0203B490 = 0x2003; // movs r0, #3
		*(u16*)0x0203B492 = 0x4770; // bx lr
	}

	// GO Series: Earth Saver (USA)
	else if (strcmp(romTid, "KB8E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x02005534 = 0xE1A00000; // nop
		*(u32*)0x0200A3D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200A898 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200B800 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02036398 = 0xE1A00000; // nop
		*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0204BFE8 = 0xE1A00000; // nop
		*(u32*)0x0204F548 = 0xE1A00000; // nop
		*(u32*)0x02054440 = 0xE1A00000; // nop
		*(u32*)0x02056228 = 0xE1A00000; // nop
		*(u32*)0x0205622C = 0xE1A00000; // nop
		*(u32*)0x02056238 = 0xE1A00000; // nop
		*(u32*)0x02056398 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020563F4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02057678 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02057AAC = 0xE1A00000; // nop
		*(u32*)0x02057AB0 = 0xE1A00000; // nop
		*(u32*)0x02057AB4 = 0xE1A00000; // nop
		*(u32*)0x02057AB8 = 0xE1A00000; // nop
		*(u32*)0x0205ABF8 = 0xE1A00000; // nop

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014BEC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Famicom Wars DS: Ushinawareta Hikari (Japan)
	else if (strcmp(romTid, "Z2EJ") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02015BC4 = 0xE1A00000; // nop
		*(u32*)0x020197B8 = 0xE1A00000; // nop
		*(u32*)0x0201F670 = 0xE1A00000; // nop
		*(u32*)0x020216B8 = 0xE1A00000; // nop
		*(u32*)0x020216BC = 0xE1A00000; // nop
		*(u32*)0x020216C8 = 0xE1A00000; // nop
		*(u32*)0x02021828 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02021884, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020219B8 = 0x02257500;
		*(u32*)0x02022C1C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C70 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02022C74 = 0xE12FFF1E; // bx lr
		*(u32*)0x02026A94 = 0xE1A00000; // nop
	}

	// Fieldrunners (USA)
	// Fieldrunners (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KFDE") == 0 || strcmp(romTid, "KFDV") == 0) && extendedMemory2) {
		*(u32*)0x0205828C = 0xE1A00000; // nop
		*(u32*)0x0205B838 = 0xE1A00000; // nop
		*(u32*)0x0205F8FC = 0xE1A00000; // nop
		*(u32*)0x020616E0 = 0xE1A00000; // nop
		*(u32*)0x020616E4 = 0xE1A00000; // nop
		*(u32*)0x020616F0 = 0xE1A00000; // nop
		*(u32*)0x02061834 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02061890, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020619C4 -= 0x30000;
		*(u32*)0x02062CF0 = 0xE1A00000; // nop
		*(u32*)0x02062CF8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02063280 = 0xE1A00000; // nop
		*(u32*)0x02063284 = 0xE1A00000; // nop
		*(u32*)0x02063288 = 0xE1A00000; // nop
		*(u32*)0x0206328C = 0xE1A00000; // nop
		*(u32*)0x020663D4 = 0xE1A00000; // nop
	}

	// Flashlight (USA)
	else if (strcmp(romTid, "KFSE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0201A200 = 0xE1A00000; // nop
		*(u32*)0x0201D1F0 = 0xE1A00000; // nop
		*(u32*)0x020201CC = 0xE1A00000; // nop
		*(u32*)0x02021F7C = 0xE1A00000; // nop
		*(u32*)0x02021F80 = 0xE1A00000; // nop
		*(u32*)0x02021F8C = 0xE1A00000; // nop
		*(u32*)0x020220EC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02022148, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020233AC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020265C4 = 0xE1A00000; // nop
	}

	// Flashlight (Europe)
	else if (strcmp(romTid, "KFSP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201A10C = 0xE1A00000; // nop
		*(u32*)0x0201D0FC = 0xE1A00000; // nop
		*(u32*)0x020200D8 = 0xE1A00000; // nop
		*(u32*)0x02021E88 = 0xE1A00000; // nop
		*(u32*)0x02021E8C = 0xE1A00000; // nop
		*(u32*)0x02021E98 = 0xE1A00000; // nop
		*(u32*)0x02021FF8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02022054, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020232B8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020264D0 = 0xE1A00000; // nop
	}

	// Flipper (USA)
	// Music will not play on retail consoles
	else if (strcmp(romTid, "KFPE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x0200DE64 = 0xE1A00000; // nop
		*(u32*)0x02031F9C = 0xE1A00000; // nop
		*(u32*)0x020351FC = 0xE1A00000; // nop
		*(u32*)0x020384E4 = 0xE1A00000; // nop
		*(u32*)0x0203A278 = 0xE1A00000; // nop
		*(u32*)0x0203A27C = 0xE1A00000; // nop
		*(u32*)0x0203A288 = 0xE1A00000; // nop
		*(u32*)0x0203A3E8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203A444, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0203B7E4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0203E664 = 0xE1A00000; // nop
	}

	// Flipper (Europe)
	// Music will not play on retail consoles
	else if (strcmp(romTid, "KFPP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x0200DECC = 0xE1A00000; // nop
		*(u32*)0x02032008 = 0xE1A00000; // nop
		*(u32*)0x02035268 = 0xE1A00000; // nop
		*(u32*)0x02038550 = 0xE1A00000; // nop
		*(u32*)0x0203A2E4 = 0xE1A00000; // nop
		*(u32*)0x0203A2E8 = 0xE1A00000; // nop
		*(u32*)0x0203A2F4 = 0xE1A00000; // nop
		*(u32*)0x0203A454 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203A4B0, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0203B8C0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0203E740 = 0xE1A00000; // nop
	}

	// Flipper 2: Flush the Goldfish (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KKNE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051EC = 0xE1A00000; // nop
		*(u32*)0x02005208 = 0xE1A00000; // nop
		*(u32*)0x02005220 = 0xE1A00000; // nop
		*(u32*)0x02005224 = 0xE1A00000; // nop
		*(u32*)0x02005228 = 0xE1A00000; // nop
		*(u32*)0x0200522C = 0xE1A00000; // nop
		*(u32*)0x02005230 = 0xE1A00000; // nop
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02008160 = 0xE1A00000; // nop
		*(u32*)0x0203A338 = 0xE1A00000; // nop
		*(u32*)0x0203D5A4 = 0xE1A00000; // nop
		*(u32*)0x0204044C = 0xE1A00000; // nop
		*(u32*)0x0204220C = 0xE1A00000; // nop
		*(u32*)0x02042210 = 0xE1A00000; // nop
		*(u32*)0x0204221C = 0xE1A00000; // nop
		*(u32*)0x0204237C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020423D8, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020423D8 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x020423FC = 0xE3500001; // cmp r0, #1
		*(u32*)0x02042404 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020437F8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02046678 = 0xE1A00000; // nop
	}

	// Frogger Returns (USA)
	else if (strcmp(romTid, "KFGE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020117D4 = 0xE1A00000; // nop
		*(u32*)0x020152F0 = 0xE1A00000; // nop
		*(u32*)0x0201A220 = 0xE1A00000; // nop
		*(u32*)0x0201C040 = 0xE1A00000; // nop
		*(u32*)0x0201C044 = 0xE1A00000; // nop
		*(u32*)0x0201C050 = 0xE1A00000; // nop
		*(u32*)0x0201C1B0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C20C, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02020C60 = 0xE1A00000; // nop

		// Skip Manual screen
		*(u32*)0x0204B968 = 0xE1A00000; // nop
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0204B98C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Game & Watch: Ball (USA, Europe)
	// Softlocks after a miss or exiting gameplay
	else if (strcmp(romTid, "KGBO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201007C = 0xE1A00000; // nop
		*(u32*)0x020138C0 = 0xE1A00000; // nop
		*(u32*)0x020177FC = 0xE1A00000; // nop
		*(u32*)0x020196AC = 0xE1A00000; // nop
		*(u32*)0x020196B0 = 0xE1A00000; // nop
		*(u32*)0x020196BC = 0xE1A00000; // nop
		*(u32*)0x02019800 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201985C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E328 = 0xE1A00000; // nop
		/* *(u32*)0x02033AB4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02034594 = 0xE12FFF1E; // bx lr
		*(u32*)0x020348B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02034D34 = 0xE12FFF1E; // bx lr */
		*(u32*)0x02035078 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Ball (Japan)
	// Softlocks after a miss or exiting gameplay
	else if (strcmp(romTid, "KGBJ") == 0) {
		*(u32*)0x02010024 = 0xE1A00000; // nop
		*(u32*)0x02013804 = 0xE1A00000; // nop
		*(u32*)0x0201765C = 0xE1A00000; // nop
		*(u32*)0x02019500 = 0xE1A00000; // nop
		*(u32*)0x02019504 = 0xE1A00000; // nop
		*(u32*)0x02019510 = 0xE1A00000; // nop
		*(u32*)0x02019654 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020196B0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E07C = 0xE1A00000; // nop
		*(u32*)0x02034BC8 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Chef (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGCO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02011B84 = 0xE1A00000; // nop
		*(u32*)0x020153C8 = 0xE1A00000; // nop
		*(u32*)0x020194F4 = 0xE1A00000; // nop
		*(u32*)0x0201B3A4 = 0xE1A00000; // nop
		*(u32*)0x0201B3A8 = 0xE1A00000; // nop
		*(u32*)0x0201B3B4 = 0xE1A00000; // nop
		*(u32*)0x0201B4F8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201B554, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020204E8 = 0xE1A00000; // nop
		*(u32*)0x0202F0FC = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Chef (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGCJ") == 0) {
		*(u32*)0x02011B2C = 0xE1A00000; // nop
		*(u32*)0x0201530C = 0xE1A00000; // nop
		*(u32*)0x02019354 = 0xE1A00000; // nop
		*(u32*)0x0201B1F8 = 0xE1A00000; // nop
		*(u32*)0x0201B1FC = 0xE1A00000; // nop
		*(u32*)0x0201B208 = 0xE1A00000; // nop
		*(u32*)0x0201B34C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201B3A8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0202023C = 0xE1A00000; // nop
		*(u32*)0x0202EE9C = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Donkey Kong Jr. (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGDO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201007C = 0xE1A00000; // nop
		*(u32*)0x020138C0 = 0xE1A00000; // nop
		*(u32*)0x02017A78 = 0xE1A00000; // nop
		*(u32*)0x02019928 = 0xE1A00000; // nop
		*(u32*)0x0201992C = 0xE1A00000; // nop
		*(u32*)0x02019938 = 0xE1A00000; // nop
		*(u32*)0x02019A7C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019AD8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E998 = 0xE1A00000; // nop
		*(u32*)0x0202D860 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Donkey Kong Jr. (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGDJ") == 0) {
		*(u32*)0x02010024 = 0xE1A00000; // nop
		*(u32*)0x02013804 = 0xE1A00000; // nop
		*(u32*)0x020178D8 = 0xE1A00000; // nop
		*(u32*)0x0201977C = 0xE1A00000; // nop
		*(u32*)0x02019780 = 0xE1A00000; // nop
		*(u32*)0x0201978C = 0xE1A00000; // nop
		*(u32*)0x020198D0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201992C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E6EC = 0xE1A00000; // nop
		*(u32*)0x0202D600 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Flagman (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGGO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020104C8 = 0xE1A00000; // nop
		*(u32*)0x02013D0C = 0xE1A00000; // nop
		*(u32*)0x02017C48 = 0xE1A00000; // nop
		*(u32*)0x02019AF8 = 0xE1A00000; // nop
		*(u32*)0x02019AFC = 0xE1A00000; // nop
		*(u32*)0x02019B08 = 0xE1A00000; // nop
		*(u32*)0x02019C4C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019CA8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E774 = 0xE1A00000; // nop
		*(u32*)0x0202D520 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Flagman (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGGJ") == 0) {
		*(u32*)0x02010470 = 0xE1A00000; // nop
		*(u32*)0x02013C50 = 0xE1A00000; // nop
		*(u32*)0x02017AA8 = 0xE1A00000; // nop
		*(u32*)0x0201994C = 0xE1A00000; // nop
		*(u32*)0x02019950 = 0xE1A00000; // nop
		*(u32*)0x0201995C = 0xE1A00000; // nop
		*(u32*)0x02019AA0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019AFC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E4C8 = 0xE1A00000; // nop
		*(u32*)0x0202D2C0 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Helmet (USA, Europe)
	// Game & Watch: Judge (USA, Europe)
	// Game & Watch: Manhole (USA, Europe)
	// Helmet & Manhole: Softlocks after 3 misses or exiting gameplay
	// Judge: Softlocks after limit is reached or exiting gameplay
	else if (strcmp(romTid, "KGHO") == 0 || strcmp(romTid, "KGJO") == 0 || strcmp(romTid, "KGMO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201007C = 0xE1A00000; // nop
		*(u32*)0x020138C0 = 0xE1A00000; // nop
		*(u32*)0x020177FC = 0xE1A00000; // nop
		*(u32*)0x020196AC = 0xE1A00000; // nop
		*(u32*)0x020196B0 = 0xE1A00000; // nop
		*(u32*)0x020196BC = 0xE1A00000; // nop
		*(u32*)0x02019800 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201985C, heapEnd); // mov r0, #0x23C0000
		if (strcmp(romTid, "KGHO") == 0 || strcmp(romTid, "KGMO") == 0) {
			*(u32*)0x0201E71C = 0xE1A00000; // nop
			*(u32*)0x0202D5E4 = 0xE12FFF1E; // bx lr
		} else {
			*(u32*)0x0201E328 = 0xE1A00000; // nop
			*(u32*)0x0202D158 = 0xE12FFF1E; // bx lr
		}
	}

	// Game & Watch: Helmet (Japan)
	// Game & Watch: Judge (Japan)
	// Game & Watch: Manhole (Japan)
	// Helmet & Manhole: Softlocks after 3 misses or exiting gameplay
	// Judge: Softlocks after limit is reached or exiting gameplay
	else if (strcmp(romTid, "KGHJ") == 0 || strcmp(romTid, "KGJJ") == 0 || strcmp(romTid, "KGMJ") == 0) {
		*(u32*)0x02010024 = 0xE1A00000; // nop
		*(u32*)0x02013804 = 0xE1A00000; // nop
		*(u32*)0x0201765C = 0xE1A00000; // nop
		*(u32*)0x02019500 = 0xE1A00000; // nop
		*(u32*)0x02019504 = 0xE1A00000; // nop
		*(u32*)0x02019510 = 0xE1A00000; // nop
		*(u32*)0x02019654 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020196B0, heapEnd); // mov r0, #0x23C0000
		if (strcmp(romTid, "KGHJ") == 0 || strcmp(romTid, "KGMJ") == 0) {
			*(u32*)0x0201E470 = 0xE1A00000; // nop
			*(u32*)0x0202D384 = 0xE12FFF1E; // bx lr
		} else {
			*(u32*)0x0201E07C = 0xE1A00000; // nop
			*(u32*)0x0202CEF8 = 0xE12FFF1E; // bx lr
		}
	}

	// Game & Watch: Mario's Cement Factory (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGFO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02011B84 = 0xE1A00000; // nop
		*(u32*)0x020153C8 = 0xE1A00000; // nop
		*(u32*)0x02019580 = 0xE1A00000; // nop
		*(u32*)0x0201B430 = 0xE1A00000; // nop
		*(u32*)0x0201B434 = 0xE1A00000; // nop
		*(u32*)0x0201B440 = 0xE1A00000; // nop
		*(u32*)0x0201B584 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201B5E0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02020574 = 0xE1A00000; // nop
		*(u32*)0x0202F188 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Mario's Cement Factory (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGFJ") == 0) {
		*(u32*)0x02011B2C = 0xE1A00000; // nop
		*(u32*)0x02015250 = 0xE1A00000; // nop
		*(u32*)0x020193E0 = 0xE1A00000; // nop
		*(u32*)0x0201B284 = 0xE1A00000; // nop
		*(u32*)0x0201B288 = 0xE1A00000; // nop
		*(u32*)0x0201B294 = 0xE1A00000; // nop
		*(u32*)0x0201B3D8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201B434, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020202C8 = 0xE1A00000; // nop
		*(u32*)0x0202EF28 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Vermin (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGVO") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201007C = 0xE1A00000; // nop
		*(u32*)0x020138C0 = 0xE1A00000; // nop
		*(u32*)0x020177FC = 0xE1A00000; // nop
		*(u32*)0x020196AC = 0xE1A00000; // nop
		*(u32*)0x020196B0 = 0xE1A00000; // nop
		*(u32*)0x020196BC = 0xE1A00000; // nop
		*(u32*)0x02019800 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201985C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E328 = 0xE1A00000; // nop
		*(u32*)0x0202D0D4 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Vermin (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGVJ") == 0) {
		*(u32*)0x02010024 = 0xE1A00000; // nop
		*(u32*)0x02013804 = 0xE1A00000; // nop
		*(u32*)0x0201765C = 0xE1A00000; // nop
		*(u32*)0x02019500 = 0xE1A00000; // nop
		*(u32*)0x02019504 = 0xE1A00000; // nop
		*(u32*)0x02019510 = 0xE1A00000; // nop
		*(u32*)0x02019654 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020196B0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E07C = 0xE1A00000; // nop
		*(u32*)0x0202CE74 = 0xE12FFF1E; // bx lr
	}

	// Glory Days: Tactical Defense (USA)
	// Glory Days: Tactical Defense (Europe)
	else if (strcmp(romTid, "KGKE") == 0 || strcmp(romTid, "KGKP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x0200B488 = 0xE1A00000; // nop
		*(u32*)0x02017128 = 0xE1A00000; // nop
		*(u32*)0x02018F94 = 0xE1A00000; // nop
		*(u32*)0x02018F98 = 0xE1A00000; // nop
		*(u32*)0x02018FA4 = 0xE1A00000; // nop
		*(u32*)0x02019104 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019160, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201D83C = 0xE1A00000; // nop
		if (strcmp(romTid, "KGKE") == 0) {
			for (int i = 0; i < 12; i++) {
				u32* offset = (u32*)0x0206710C;
				offset[i] = 0xE1A00000; // nop
			}
			*(u32*)0x020671B4 = 0xE1A00000; // nop
			for (int i = 0; i < 10; i++) {
				u32* offset = (u32*)0x02075514;
				offset[i] = 0xE1A00000; // nop
			}
		} else {
			for (int i = 0; i < 12; i++) {
				u32* offset = (u32*)0x02067264;
				offset[i] = 0xE1A00000; // nop
			}
			*(u32*)0x0206730C = 0xE1A00000; // nop
			for (int i = 0; i < 10; i++) {
				u32* offset = (u32*)0x0207566C;
				offset[i] = 0xE1A00000; // nop
			}
		}
	}

	// Hard-Hat Domo (USA)
	else if (strcmp(romTid, "KDHE") == 0) {
		*(u16*)0x0200D060 = 0x2001; // movs r0, #1
		*(u16*)0x0200D062 = 0x4770; // bx lr
		*(u16*)0x020140AC = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02024C7E);
		doubleNopT(0x02022A2E);
		doubleNopT(0x02027900);
		doubleNopT(0x02028EBA);
		doubleNopT(0x02028EBE);
		doubleNopT(0x02028ECA);
		doubleNopT(0x02028FAE);
		patchHiHeapDSiWareThumb(0x02028FEC, 0x208F, 0x0480); // movs r0, #0x23C0000
		doubleNopT(0x0202A076);
		*(u16*)0x0202A078 = 0x46C0;
		*(u16*)0x0202A07A = 0x46C0;
		doubleNopT(0x0202A07E);
		doubleNopT(0x0202C176);
	}

	// Heathcliff: Spot On (USA)
	else if (strcmp(romTid, "K6SE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005280 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020052C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201615C = 0xE1A00000; // nop
		*(u32*)0x02019EDC = 0xE1A00000; // nop
		*(u32*)0x0201F084 = 0xE1A00000; // nop
		*(u32*)0x02020FC8 = 0xE1A00000; // nop
		*(u32*)0x02020FCC = 0xE1A00000; // nop
		*(u32*)0x02020FD8 = 0xE1A00000; // nop
		*(u32*)0x02021138 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02021194, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02022500 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022504 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202250C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02022510 = 0xE12FFF1E; // bx lr
		*(u32*)0x02025AA8 = 0xE1A00000; // nop
		*(u32*)0x0205B604 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205B608 = 0xE12FFF1E; // bx lr
	}

	// Hellokids: Vol. 1: Coloring and Painting! (USA)
	// Loops in some code during white screens
	/*else if (strcmp(romTid, "KKIE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200B888 = 0xE3A02001; // mov r2, #1
		*(u32*)0x02028700 = 0xE1A00000; // nop
		*(u32*)0x0202F7F4 = 0xE1A00000; // nop
		*(u32*)0x02034310 = 0xE1A00000; // nop
		*(u32*)0x020361A8 = 0xE1A00000; // nop
		*(u32*)0x020361AC = 0xE1A00000; // nop
		*(u32*)0x020361B8 = 0xE1A00000; // nop
		*(u32*)0x02036318 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02036374, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02037868 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02039C48 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02039C80 = 0xE1A00000; // nop
		*(u32*)0x0203B674 = 0xE1A00000; // nop
	}*/

	// Invasion of the Alien Blobs (USA)
	// Branches to DSi code in ITCM?
	/*else if (strcmp(romTid, "KBTE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201BF84 = 0xE1A00000; // nop
		*(u32*)0x0201BF94 = 0xE1A00000; // nop
		*(u32*)0x0201BFB0 = 0xE1A00000; // nop
		*(u32*)0x02021214 = 0xE1A00000; // nop
		*(u32*)0x020224EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020224F0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02023498 = 0xE1A00000; // nop
		*(u32*)0x0203C79C = 0xE1A00000; // nop
		*(u32*)0x020404A8 = 0xE1A00000; // nop
		*(u32*)0x02046608 = 0xE1A00000; // nop
		*(u32*)0x02048444 = 0xE1A00000; // nop
		*(u32*)0x02048448 = 0xE1A00000; // nop
		*(u32*)0x02048454 = 0xE1A00000; // nop
		*(u32*)0x020485B4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02048610, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02049894 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0204CDE0 = 0xE1A00000; // nop
	}*/

	// JellyCar 2 (USA)
	else if (strcmp(romTid, "KJYE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02006334 = 0xE1A00000; // nop
		*(u32*)0x0200634C = 0xE1A00000; // nop
		*(u32*)0x02019F94 = 0xE1A00000; // nop (Skip Manual screen, Part 1)
		for (int i = 0; i < 11; i++) { // Skip Manual screen, Part 2
			u32* offset = (u32*)0x0201A028;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x020B4694 = 0xE1A00000; // nop
		*(u32*)0x020B7F60 = 0xE1A00000; // nop
		*(u32*)0x020BC368 = 0xE1A00000; // nop
		*(u32*)0x020BE0FC = 0xE1A00000; // nop
		*(u32*)0x020BE100 = 0xE1A00000; // nop
		*(u32*)0x020BE10C = 0xE1A00000; // nop
		*(u32*)0x020BE26C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020BE2C8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020BE3FC = 0x0213D220;
		*(u32*)0x020BF7D0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020C31BC = 0xE1A00000; // nop
	}

	// Jump Trials (USA)
	// Does not work on real hardware
	/*else if (strcmp(romTid, "KJPE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x0201BD6C = 0xE1A00000; // nop
		*(u32*)0x0201BD9C = 0xE1A00000; // nop
		*(u32*)0x0201BDBC = 0xE1A00000; // nop
		*(u32*)0x0201E5D0 = 0xE1A00000; // nop
		*(u32*)0x0201E5E0 = 0xE1A00000; // nop
		*(u32*)0x0201E61C = 0xE1A00000; // nop
		*(u32*)0x0201E88C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E890 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EA40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EA44 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203AA5C = 0xE1A00000; // nop
		*(u32*)0x0203ED64 = 0xE1A00000; // nop
		*(u32*)0x02045AFC = 0xE1A00000; // nop
		*(u32*)0x02047938 = 0xE1A00000; // nop
		*(u32*)0x0204793C = 0xE1A00000; // nop
		*(u32*)0x02047948 = 0xE1A00000; // nop
		*(u32*)0x02047AA8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02047B04, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02048D88 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0204C1F0 = 0xE1A00000; // nop
	}*/

	// Jump Trials Extreme (USA)
	// Does not work on real hardware
	/*else if (strcmp(romTid, "KZCE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x0201EAD0 = 0xE1A00000; // nop
		*(u32*)0x0201EB00 = 0xE1A00000; // nop
		*(u32*)0x0201EB20 = 0xE1A00000; // nop
		*(u32*)0x02021334 = 0xE1A00000; // nop
		*(u32*)0x02021344 = 0xE1A00000; // nop
		*(u32*)0x02021380 = 0xE1A00000; // nop
		*(u32*)0x020215F0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020215F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020217A4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020217A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203D7C0 = 0xE1A00000; // nop
		*(u32*)0x02041AC8 = 0xE1A00000; // nop
		*(u32*)0x020488C0 = 0xE1A00000; // nop
		*(u32*)0x0204A6FC = 0xE1A00000; // nop
		*(u32*)0x0204A700 = 0xE1A00000; // nop
		*(u32*)0x0204A70C = 0xE1A00000; // nop
		*(u32*)0x0204A86C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204A8C8, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x0204BB4C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0204EFB4 = 0xE1A00000; // nop
	}*/

	// Kung Fu Dragon (USA)
	// Kung Fu Dragon (Europe)
	else if (strcmp(romTid, "KT9E") == 0 || strcmp(romTid, "KT9P") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x020051D4 = 0xE1A00000; // nop
		*(u32*)0x02005310 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0200E8F4 = 0xE1A00000; // nop
		*(u32*)0x02011D90 = 0xE1A00000; // nop
		*(u32*)0x0201580C = 0xE1A00000; // nop
		*(u32*)0x020175A8 = 0xE1A00000; // nop
		*(u32*)0x020175AC = 0xE1A00000; // nop
		*(u32*)0x020175B8 = 0xE1A00000; // nop
		*(u32*)0x02017718 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02017774, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020189BC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201BD60 = 0xE1A00000; // nop
		*(u32*)0x0201D8EC = 0xE1A00000; // nop
		*(u32*)0x02024994 = 0xE1A00000; // nop
		*(u32*)0x02024A4C = 0xE1A00000; // nop
	}

	// Akushon Gemu: Tobeyo!! Dorago! (Japan)
	else if (strcmp(romTid, "KT9J") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x020051D8 = 0xE1A00000; // nop
		*(u32*)0x020052F0 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0200E8C8 = 0xE1A00000; // nop
		*(u32*)0x02011D64 = 0xE1A00000; // nop
		*(u32*)0x020157E0 = 0xE1A00000; // nop
		*(u32*)0x0201757C = 0xE1A00000; // nop
		*(u32*)0x02017580 = 0xE1A00000; // nop
		*(u32*)0x0201758C = 0xE1A00000; // nop
		*(u32*)0x020176EC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02017748, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018990 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201BD34 = 0xE1A00000; // nop
		*(u32*)0x0201D8C0 = 0xE1A00000; // nop
		*(u32*)0x020248BC = 0xE1A00000; // nop
		*(u32*)0x02024974 = 0xE1A00000; // nop
	}

	// Kyara Pasha!: Hello Kitty (Japan)
	// Shows Japanese error screen (Possibly related to camera)
	/*else if (strcmp(romTid, "KYKJ") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051E4 -= 4;
		*(u32*)0x020051EC = 0xE1A00000; // nop
		*(u32*)0x020052CC = 0xE1A00000; // nop
		*(u32*)0x0201EA20 = 0xE1A00000; // nop
		*(u32*)0x0201EA24 = 0xE1A00000; // nop
		*(u32*)0x0201EA3C = 0xE8BD80F8; // LDMFD SP!, {R3-R7,PC}
		*(u32*)0x0203F004 = 0xE1A00000; // nop
		*(u32*)0x02046130 = 0xE1A00000; // nop
		*(u32*)0x0204B7C4 = 0xE1A00000; // nop
		*(u32*)0x0204DE30 = 0xE1A00000; // nop
		*(u32*)0x0204DE34 = 0xE1A00000; // nop
		*(u32*)0x0204DE40 = 0xE1A00000; // nop
		*(u32*)0x0204DFA0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204DFFC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0204F350 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205321C = 0xE1A00000; // nop
	}*/

	// The Legend of Zelda: Four Swords: Anniversary Edition (USA)
	// The Legend of Zelda: Four Swords: Anniversary Edition (Europe, Australia)
	// Zelda no Densetsu: 4-tsu no Tsurugi: 25th Kinen Edition (Japan)
	// Some graphics are glitched/missing
	// Requires 8MB of RAM (though crashes on black screens due to code at 0x02018794)
	/*else if ((strcmp(romTid, "KQ9E") == 0 || strcmp(romTid, "KQ9V") == 0 || strcmp(romTid, "KQ9J") == 0) && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x02012AAC = 0xE1A00000; // nop
		*(u32*)0x020166D4 = 0xE1A00000; // nop
		*(u32*)0x020185C8 = 0xE1A00000; // nop
		*(u32*)0x020185CC = 0xE1A00000; // nop
		*(u32*)0x020185D8 = 0xE1A00000; // nop
		*(u32*)0x02018738 = 0xE1A00000; // nop
		*(u32*)0x02018794 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x020187B8 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020187C0 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020188C8 -= 0x30000;
		*(u32*)0x02019968 = 0xE12FFF1E; // bx lr
		*(u32*)0x02019974 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201D01C = 0xE1A00000; // nop
		if (strcmp(romTid, "KQ9E") == 0) {
			*(u32*)0x02082A3C = 0xE1A00000; // nop
			*(u32*)0x02082A58 = 0xE1A00000; // nop
			*(u32*)0x020A44FC = 0xE1A00000; // nop
			*(u32*)0x020A467C = 0xE1A00000; // nop
		} else if (strcmp(romTid, "KQ9V") == 0) {
			*(u32*)0x02082A5C = 0xE1A00000; // nop
			*(u32*)0x02082A78 = 0xE1A00000; // nop
			*(u32*)0x020A451C = 0xE1A00000; // nop
			*(u32*)0x020A469C = 0xE1A00000; // nop
		} else {
			*(u32*)0x020829F8 = 0xE1A00000; // nop
			*(u32*)0x02082A14 = 0xE1A00000; // nop
			*(u32*)0x020A44B8 = 0xE1A00000; // nop
			*(u32*)0x020A4638 = 0xE1A00000; // nop
		}
	}*/

	// Libera Wing (Europe)
	// Black screens
	/*else if (strcmp(romTid, "KLWP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02016138 = 0xE1A00000; // nop
		*(u32*)0x020195A8 = 0xE1A00000; // nop
		*(u32*)0x0201ED70 = 0xE1A00000; // nop
		*(u32*)0x02020B90 = 0xE1A00000; // nop
		*(u32*)0x02020B94 = 0xE1A00000; // nop
		*(u32*)0x02020BA0 = 0xE1A00000; // nop
		*(u32*)0x02020D00 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02020D5C, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02020E90 = 0x020D7F40;
		*(u32*)0x02024FD0 = 0xE1A00000; // nop
		*(u32*)0x02044668 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204585C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02045860 = 0xE12FFF1E; // bx lr
		*(u32*)0x020563F8 = 0xE1A00000; // nop
	}*/

	// Little Twin Stars (Japan)
	// Locks up(?) after confirming age and left/right hand
	else if (strcmp(romTid, "KQ3J") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050DC = 0xE1A00000; // nop
		*(u32*)0x020050F0 = 0xE1A00000; // nop
		*(u32*)0x02005200 = 0xE1A00000; // nop
		*(u32*)0x0200523C = 0xE1A00000; // nop
		*(u32*)0x020162C4 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x0203DEF4 = 0xE1A00000; // nop
		*(u32*)0x020417D0 = 0xE1A00000; // nop
		*(u32*)0x020460B8 = 0xE1A00000; // nop
		*(u32*)0x02047F94 = 0xE1A00000; // nop
		*(u32*)0x02047F98 = 0xE1A00000; // nop
		*(u32*)0x02047FA4 = 0xE1A00000; // nop
		*(u32*)0x02048104 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02048160, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02048294 = 0x0221A980;
		*(u32*)0x020495B0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0204CB3C = 0xE1A00000; // nop
	}

	// Lola's Alphabet Train (USA)
	else if (strcmp(romTid, "KLKE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x02024D24 = 0xE1A00000; // nop
		*(u32*)0x02028FAC = 0xE1A00000; // nop
		*(u32*)0x0202CA60 = 0xE1A00000; // nop
		*(u32*)0x0202E83C = 0xE1A00000; // nop
		*(u32*)0x0202E840 = 0xE1A00000; // nop
		*(u32*)0x0202E84C = 0xE1A00000; // nop
		*(u32*)0x0202E9AC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202EA08, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0202FDB4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02033218 = 0xE1A00000; // nop
	}

	// Lola's Alphabet Train (Europe)
	else if (strcmp(romTid, "KLKP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x02024CE4 = 0xE1A00000; // nop
		*(u32*)0x02028F6C = 0xE1A00000; // nop
		*(u32*)0x0202CA20 = 0xE1A00000; // nop
		*(u32*)0x0202E7FC = 0xE1A00000; // nop
		*(u32*)0x0202E800 = 0xE1A00000; // nop
		*(u32*)0x0202E80C = 0xE1A00000; // nop
		*(u32*)0x0202E96C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202E9C8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0202FD74 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020331D8 = 0xE1A00000; // nop
	}

	// Magical Whip (USA)
	else if (strcmp(romTid, "KWME") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200E5D4 = 0xE1A00000; // nop
		*(u32*)0x02011A18 = 0xE1A00000; // nop
		*(u32*)0x020155B0 = 0xE1A00000; // nop
		*(u32*)0x02017344 = 0xE1A00000; // nop
		*(u32*)0x02017348 = 0xE1A00000; // nop
		*(u32*)0x02017354 = 0xE1A00000; // nop
		*(u32*)0x020174B4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02017510, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018758 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201BAFC = 0xE1A00000; // nop
		*(u32*)0x0201D4F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02026E94 = 0xE1A00000; // nop
		*(u32*)0x02030288 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Magical Whip (Europe)
	else if (strcmp(romTid, "KWMP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200E610 = 0xE1A00000; // nop
		*(u32*)0x02011ADC = 0xE1A00000; // nop
		*(u32*)0x02015688 = 0xE1A00000; // nop
		*(u32*)0x02017424 = 0xE1A00000; // nop
		*(u32*)0x02017428 = 0xE1A00000; // nop
		*(u32*)0x02017434 = 0xE1A00000; // nop
		*(u32*)0x02017594 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020175F0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018838 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201BBDC = 0xE1A00000; // nop
		*(u32*)0x0201D5D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02026F74 = 0xE1A00000; // nop
		*(u32*)0x02030368 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Magnetic Joe (USA)
	else if (strcmp(romTid, "KJOE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02013344 = 0xE1A00000; // nop
		*(u32*)0x0201705C = 0xE1A00000; // nop
		*(u32*)0x0201B0B0 = 0xE1A00000; // nop
		*(u32*)0x0201CF48 = 0xE1A00000; // nop
		*(u32*)0x0201CF4C = 0xE1A00000; // nop
		*(u32*)0x0201CF58 = 0xE1A00000; // nop
		*(u32*)0x0201D0B8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201D114, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201D248 = 0x020C5360;
		*(u32*)0x0201E658 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E65C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E664 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E668 = 0xE12FFF1E; // bx lr
		*(u32*)0x02021464 = 0xE1A00000; // nop
		*(u32*)0x02027058 = 0xE1A00000; // nop
		*(u32*)0x02027328 = 0xE1A00000; // nop
		*(u32*)0x02036964 = 0xE1A00000; // nop
		*(u32*)0x02036980 = 0xE1A00000; // nop
		*(u32*)0x020369F8 = 0xE1A00000; // nop
		*(u32*)0x02036A30 = 0xE1A00000; // nop (Skip
		*(u32*)0x02036A34 = 0xE1A00000; // nop  Manual screen)
		*(u32*)0x02036AAC = 0xE1A00000; // nop
		*(u32*)0x0204E5AC = 0xE3A00002; // mov r0, #2
		*(u32*)0x0204E62C = 0xE1A00000; // nop
		*(u32*)0x0205DAAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205DAB0 = 0xE12FFF1E; // bx lr
	}

	// Mario Calculator (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KWFE") == 0 && extendedMemory2) {
		*(u16*)0x0200504E = 0x46C0; // nop
		*(u16*)0x02005050 = 0x46C0; // nop
		*(u16*)0x02010E64 = 0x7047; // bx lr
		*(u16*)0x020117D4 = 0x7047; // bx lr
		*(u16*)0x020346F8 = 0xB003; // ADD SP, SP, #0xC
		*(u16*)0x020346FA = 0xBD78; // POP {R3-R6,PC}
		*(u16*)0x020369D4 = 0x46C0; // nop
		*(u16*)0x020369D6 = 0x46C0; // nop
		*(u16*)0x0203B08C = 0x46C0; // nop
		*(u16*)0x0203B08E = 0x46C0; // nop
		*(u16*)0x0203C5DA = 0x46C0; // nop
		*(u16*)0x0203C5DC = 0x46C0; // nop
		*(u16*)0x0203C5DE = 0x46C0; // nop
		*(u16*)0x0203C5E0 = 0x46C0; // nop
		*(u16*)0x0203C5EA = 0x46C0; // nop
		*(u16*)0x0203C5EC = 0x46C0; // nop
		*(u16*)0x0203C6D6 = 0x46C0; // nop
		*(u16*)0x0203C6D8 = 0x46C0; // nop
		*(u16*)0x0203C714 = 0x209C; // movs r0, #0x2700000
		*(u16*)0x0203C72E = 0x2801; // cmp r0, #1
		*(u16*)0x0203C736 = 0x2027; // movs r0, #0x2700000
		//*(u32*)0x0203C7EC = 0x02090140;
		*(u16*)0x020474DA = 0x46C0; // nop
		*(u16*)0x020474E6 = 0x46C0; // nop
		*(u16*)0x020474F0 = 0x46C0; // nop
	}

	// Mario vs. Donkey Kong: Minis March Again! (USA)
	// Does not boot
	/*else if (strcmp(romTid, "KDME") == 0) {
		*(u32*)0x0202E6F8 = 0xE1A00000; // nop
		*(u32*)0x0202E788 = 0xE1A00000; // nop
		*(u32*)0x020343E0 = 0xE1A00000; // nop
		*(u32*)0x020343E4 = 0xE1A00000; // nop
		*(u32*)0x020343F4 = 0xE1A00000; // nop
		*(u32*)0x02034408 = 0xE1A00000; // nop
		*(u32*)0x0203448C = 0xE1A00000; // nop
		*(u32*)0x02059770 = 0xE1A00000; // nop
		*(u32*)0x0205A72C = 0xE1A00000; // nop
		*(u32*)0x020612B8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020612BC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02064F80 = 0xE1A00000; // nop
		*(u32*)0x0206C780 = 0xE1A00000; // nop
		*(u32*)0x0206F7AC = 0xE1A00000; // nop
		*(u32*)0x0206F7B0 = 0xE1A00000; // nop
		*(u32*)0x0206F7BC = 0xE1A00000; // nop
		*(u32*)0x0206F900 = 0xE1A00000; // nop
		*(u32*)0x0206F904 = 0xE1A00000; // nop
		*(u32*)0x0206F908 = 0xE1A00000; // nop
		*(u32*)0x0206F90C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206F968, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02071390 = 0xE1A00000; // nop
		*(u32*)0x02071394 = 0xE1A00000; // nop
		*(u32*)0x02071398 = 0xE1A00000; // nop
		*(u32*)0x0207139C = 0xE1A00000; // nop
	}*/

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0) {
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02014BDC;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x0201A38C;

		*(u32*)0x0200B0A0 = 0xE1A00000; // nop
		*(u32*)0x02014BDC = generateA7Instr(0x02014BDC, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		*(u32*)0x0201A38C = generateA7Instr(0x0201A38C, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
		*(u32*)0x0204D3C4 = 0xE1A00000; // nop
		*(u32*)0x02051124 = 0xE1A00000; // nop
		*(u32*)0x020566E8 = 0xE1A00000; // nop
		*(u32*)0x020585BC = 0xE1A00000; // nop
		*(u32*)0x020585C0 = 0xE1A00000; // nop
		*(u32*)0x020585CC = 0xE1A00000; // nop
		*(u32*)0x02058710 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0205876C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020599A0 = 0xE1A00000; // nop
		*(u32*)0x020599A8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0205C93C = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Europe, Australia)
	else if (strcmp(romTid, "KMGV") == 0) {
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201528C;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x0201AA44;

		*(u32*)0x0200B3A8 = 0xE1A00000; // nop
		*(u32*)0x0201528C = generateA7Instr(0x0201528C, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		*(u32*)0x0201AA44 = generateA7Instr(0x0201AA44, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
		*(u32*)0x0204D504 = 0xE1A00000; // nop
		*(u32*)0x02050F30 = 0xE1A00000; // nop
		*(u32*)0x020564F4 = 0xE1A00000; // nop
		*(u32*)0x020583E4 = 0xE1A00000; // nop
		*(u32*)0x020583E8 = 0xE1A00000; // nop
		*(u32*)0x020583F4 = 0xE1A00000; // nop
		*(u32*)0x02058538 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02058594, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020597C8 = 0xE1A00000; // nop
		*(u32*)0x020597D0 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0205C88C = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Japan)
	else if (strcmp(romTid, "KMGJ") == 0) {
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02014718;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x02019C54;

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200B184 = 0xE1A00000; // nop
		*(u32*)0x02014718 = generateA7Instr(0x02014718, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		*(u32*)0x02019C54 = generateA7Instr(0x02019C54, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
		*(u32*)0x0204B538 = 0xE1A00000; // nop
		*(u32*)0x0204EEB4 = 0xE1A00000; // nop
		*(u32*)0x02053EA8 = 0xE1A00000; // nop
		*(u32*)0x02055D34 = 0xE1A00000; // nop
		*(u32*)0x02055D38 = 0xE1A00000; // nop
		*(u32*)0x02055D44 = 0xE1A00000; // nop
		*(u32*)0x02055EA4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02055F00, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02057118 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205A99C = 0xE1A00000; // nop
	}

	// Mighty Milky Way (USA)
	// Mighty Milky Way (Europe)
	// Mighty Milky Way (Japan)
	// Audio doesn't play on retail consoles
	else if (strncmp(romTid, "KWY", 3) == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 1;
		ce9->rumbleForce[1] = 1;

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200545C = 0xE1A00000; // nop
		*(u32*)0x020054B0 = 0xE1A00000; // nop
		*(u32*)0x020054C8 = 0xE1A00000; // nop
		*(u32*)0x020054CC = 0xE1A00000; // nop
		*(u32*)0x020054D0 = 0xE1A00000; // nop
		*(u32*)0x020054D4 = 0xE1A00000; // nop
		*(u32*)0x020054DC = 0xE1A00000; // nop
		*(u32*)0x020054E4 = 0xE1A00000; // nop
		*(u32*)0x020057AC = 0xE12FFF1E; // bx lr
		*(u32*)0x02005A20 = 0xE1A00000; // nop
		*(u32*)0x02005A24 = 0xE1A00000; // nop
		*(u32*)0x02005A28 = 0xE1A00000; // nop
		*(u32*)0x02005A2C = 0xE1A00000; // nop
		*(u32*)0x02005A30 = 0xE1A00000; // nop
		*(u32*)0x02005A38 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020072CC = 0xE3A00901; // mov r0, #0x4000 (Shrink sound heap from 1MB to 16KB: Disables music)
		}
		if (ndsHeader->gameCode[3] == 'J') {
			ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201D008;
			ce9->patches->rumble_arm9[1][3] = *(u32*)0x020275F8;
			if (!extendedMemory2) {
				// Disable loading title music
				*(u32*)0x02012930 = 0xE1A00000; // nop
				*(u32*)0x02012934 = 0xE1A00000; // nop
				*(u32*)0x02012938 = 0xE1A00000; // nop
				*(u32*)0x0201293C = 0xE1A00000; // nop
				*(u32*)0x02012940 = 0xE1A00000; // nop
				*(u32*)0x02012944 = 0xE1A00000; // nop
				*(u32*)0x02012964 = 0xE1A00000; // nop
				*(u32*)0x02012980 = 0xE1A00000; // nop
				*(u32*)0x02012DC4 = 0xE1A00000; // nop
				*(u32*)0x02012DE0 = 0xE1A00000; // nop
			}
			*(u32*)0x0201D008 = generateA7Instr(0x0201D008, (int)ce9->patches->rumble_arm9[0]); // Rumble when Luna gets shocked
			*(u32*)0x020275F8 = generateA7Instr(0x020275F8, (int)ce9->patches->rumble_arm9[1]); // Rumble when planet is destroyed
			*(u32*)0x02064FB0 = 0xE1A00000; // nop
			*(u32*)0x02068924 = 0xE1A00000; // nop
			*(u32*)0x0206CE5C = 0xE1A00000; // nop
			*(u32*)0x0206ECE8 = 0xE1A00000; // nop
			*(u32*)0x0206ECEC = 0xE1A00000; // nop
			*(u32*)0x0206ECF8 = 0xE1A00000; // nop
			*(u32*)0x0206EE58 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x0206EEB4, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
			*(u32*)0x0206EFE8 -= 0x30000;
			*(u32*)0x020700CC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
			*(u32*)0x02070500 = 0xE1A00000; // nop
			*(u32*)0x02070504 = 0xE1A00000; // nop
			*(u32*)0x02070508 = 0xE1A00000; // nop
			*(u32*)0x0207050C = 0xE1A00000; // nop
			*(u32*)0x02070518 = 0xE1A00000; // nop (Enable error exception screen)
			*(u32*)0x02073A08 = 0xE1A00000; // nop
		} else {
			ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201CFB0;
			ce9->patches->rumble_arm9[1][3] = *(u32*)0x0202750C;
			if (!extendedMemory2) {
				// Disable loading title music
				*(u32*)0x020128E4 = 0xE1A00000; // nop
				*(u32*)0x020128E8 = 0xE1A00000; // nop
				*(u32*)0x020128EC = 0xE1A00000; // nop
				*(u32*)0x020128F0 = 0xE1A00000; // nop
				*(u32*)0x020128F4 = 0xE1A00000; // nop
				*(u32*)0x020128F8 = 0xE1A00000; // nop
				*(u32*)0x02012918 = 0xE1A00000; // nop
				*(u32*)0x02012934 = 0xE1A00000; // nop
				*(u32*)0x02012D7C = 0xE1A00000; // nop
				*(u32*)0x02012D98 = 0xE1A00000; // nop
			}
			*(u32*)0x0201CFB0 = generateA7Instr(0x0201CFB0, (int)ce9->patches->rumble_arm9[0]); // Rumble when Luna gets shocked
			*(u32*)0x0202750C = generateA7Instr(0x0202750C, (int)ce9->patches->rumble_arm9[1]); // Rumble when planet is destroyed
			*(u32*)0x02064E34 = 0xE1A00000; // nop
			*(u32*)0x020687A8 = 0xE1A00000; // nop
			*(u32*)0x0206CCE0 = 0xE1A00000; // nop
			*(u32*)0x0206EB6C = 0xE1A00000; // nop
			*(u32*)0x0206EB70 = 0xE1A00000; // nop
			*(u32*)0x0206EB7C = 0xE1A00000; // nop
			*(u32*)0x0206ECDC = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x0206ED38, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
			*(u32*)0x0206EE6C -= 0x30000;
			*(u32*)0x0206FF50 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
			*(u32*)0x02070384 = 0xE1A00000; // nop
			*(u32*)0x02070388 = 0xE1A00000; // nop
			*(u32*)0x0207038C = 0xE1A00000; // nop
			*(u32*)0x02070390 = 0xE1A00000; // nop
			*(u32*)0x0207039C = 0xE1A00000; // nop (Enable error exception screen)
			*(u32*)0x0207388C = 0xE1A00000; // nop
		}
	}

	// Mixed Messages (USA)
	// Mixed Messages (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KMME") == 0 || strcmp(romTid, "KMMV") == 0) && extendedMemory2) {
		*(u32*)0x020036D8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020036DC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02006E74 = 0xE1A00000; // nop
		*(u32*)0x0200B8FC = 0xE1A00000; // nop
		*(u32*)0x0200B900 = 0xE1A00000; // nop
		*(u32*)0x0200B90C = 0xE1A00000; // nop
		*(u32*)0x0200BA50 = 0xE1A00000; // nop
		*(u32*)0x0200BA54 = 0xE1A00000; // nop
		*(u32*)0x0200BA58 = 0xE1A00000; // nop
		*(u32*)0x0200BA5C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0200BAB8, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0200CD44 = 0xE1A00000; // nop
		*(u32*)0x0200CD4C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0200F694 = 0xE1A00000; // nop
		*(u32*)0x0202A65C = 0xE1A00000; // nop
		*(u32*)0x02031A40 = 0xE3A00008; // mov r0, #8
		*(u32*)0x02031A44 = 0xE12FFF1E; // bx lr
		*(u32*)0x020337FC = 0xE1A00000; // nop
		*(u32*)0x02033B00 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Monster Buster Club (USA)
	else if (strcmp(romTid, "KXBE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0207F058 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F05C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F138 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F13C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F4FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F63C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F640 = 0xE12FFF1E; // bx lr
		*(u32*)0x02094318 = 0xE1A00000; // nop
		*(u32*)0x02097954 = 0xE1A00000; // nop
		*(u32*)0x0209AB24 = 0xE1A00000; // nop
		*(u32*)0x0209C8E4 = 0xE1A00000; // nop
		*(u32*)0x0209C8E8 = 0xE1A00000; // nop
		*(u32*)0x0209C8F4 = 0xE1A00000; // nop
		*(u32*)0x0209CA54 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0209CAB0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0209DEA8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020A0C08 = 0xE1A00000; // nop
	}

	// Monster Buster Club (Europe)
	else if (strcmp(romTid, "KXBP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0207EF64 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207EF68 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F048 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F414 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F418 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F558 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F55C = 0xE12FFF1E; // bx lr
		*(u32*)0x0209424C = 0xE1A00000; // nop
		*(u32*)0x02097888 = 0xE1A00000; // nop
		*(u32*)0x0209AA58 = 0xE1A00000; // nop
		*(u32*)0x0209C818 = 0xE1A00000; // nop
		*(u32*)0x0209C81C = 0xE1A00000; // nop
		*(u32*)0x0209C828 = 0xE1A00000; // nop
		*(u32*)0x0209C988 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0209C9E4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0209DDDC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020A0B3C = 0xE1A00000; // nop
	}

	// Mr. Brain (Japan)
	else if (strcmp(romTid, "KMBJ") == 0) {
		*(u32*)0x020054EC = 0xE1A00000; // nop
		*(u32*)0x02005504 = 0xE1A00000; // nop
		*(u32*)0x0200648C = 0xE1A00000; // nop
		*(u32*)0x0200949C = 0xE1A00000; // nop
		*(u32*)0x02025AB4 = 0xE1A00000; // nop
		*(u32*)0x02029804 = 0xE1A00000; // nop
		*(u32*)0x0203065C = 0xE1A00000; // nop
		*(u32*)0x020325FC = 0xE1A00000; // nop
		*(u32*)0x02032600 = 0xE1A00000; // nop
		*(u32*)0x0203260C = 0xE1A00000; // nop
		*(u32*)0x02032750 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020327AC, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02033AC4 = 0xE1A00000; // nop
		*(u32*)0x02033ACC = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02036ED0 = 0xE1A00000; // nop
	}

	// Music on: Playing Piano (USA)
	// Sprite graphics and font missing
	else if (strcmp(romTid, "KICE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200AFFC = 0xE1A00000; // nop
		*(u32*)0x0200E08C = 0xE1A00000; // nop
		*(u32*)0x0200E0DC = 0xE1A00000; // nop
		*(u32*)0x0200E0FC = 0xE1A00000; // nop
		*(u32*)0x0200E10C = 0xE1A00000; // nop
		*(u32*)0x0200E138 = 0xE1A00000; // nop
		*(u32*)0x0201D9F4 = 0xE1A00000; // nop
		*(u32*)0x02020EC8 = 0xE1A00000; // nop
		*(u32*)0x020219D8 = 0xE1A00000; // nop
		*(u32*)0x02021A74 = 0xE1A00000; // nop
		*(u32*)0x02021B28 = 0xE1A00000; // nop
		*(u32*)0x02021BDC = 0xE1A00000; // nop
		*(u32*)0x02021C7C = 0xE1A00000; // nop
		*(u32*)0x02021CFC = 0xE1A00000; // nop
		*(u32*)0x02021D78 = 0xE1A00000; // nop
		*(u32*)0x02021DFC = 0xE1A00000; // nop
		*(u32*)0x02021E9C = 0xE1A00000; // nop
		*(u32*)0x02021F58 = 0xE1A00000; // nop
		*(u32*)0x02022094 = 0xE1A00000; // nop
		*(u32*)0x020220F8 = 0xE1A00000; // nop
		*(u32*)0x020221C0 = 0xE1A00000; // nop
		*(u32*)0x02022230 = 0xE1A00000; // nop
		*(u32*)0x020222BC = 0xE1A00000; // nop
		*(u32*)0x0202232C = 0xE1A00000; // nop
		*(u32*)0x020223B4 = 0xE1A00000; // nop
		*(u32*)0x02022424 = 0xE1A00000; // nop
		*(u32*)0x02022538 = 0xE1A00000; // nop
		*(u32*)0x020225A0 = 0xE1A00000; // nop
		*(u32*)0x02022620 = 0xE1A00000; // nop
		*(u32*)0x02022684 = 0xE1A00000; // nop
		*(u32*)0x0202273C = 0xE1A00000; // nop
		*(u32*)0x020227AC = 0xE1A00000; // nop
		*(u32*)0x0202A754 = 0xE1A00000; // nop
		*(u32*)0x02025F9C = 0xE1A00000; // nop
		*(u32*)0x02025FA0 = 0xE1A00000; // nop
		*(u32*)0x02025FAC = 0xE1A00000; // nop
		*(u32*)0x0202610C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02026168, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02027574 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202A754 = 0xE1A00000; // nop
	}

	// Neko Reversi (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KNVJ") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02010A04 = 0xE1A00000; // nop
		*(u32*)0x020140A8 = 0xE1A00000; // nop
		*(u32*)0x02017B64 = 0xE1A00000; // nop
		*(u32*)0x02019900 = 0xE1A00000; // nop
		*(u32*)0x02019904 = 0xE1A00000; // nop
		*(u32*)0x02019910 = 0xE1A00000; // nop
		*(u32*)0x02019A70 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019ACC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201AE44 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201E4B4 = 0xE1A00000; // nop
		*(u32*)0x0203FCA4 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Nintendo DSi + Internet (Japan)
	// Nintendo DSi + Internet (USA)
	else if (strcmp(romTid, "K2DJ") == 0 || strcmp(romTid, "K2DE") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x0200599C = 0xE1A00000; // nop
		*(u32*)0x020059A8 = 0xE1A00000; // nop
		*(u32*)0x020059B8 = 0xE1A00000; // nop
		*(u32*)0x020059C4 = 0xE1A00000; // nop
		*(u32*)0x0200AB2C = 0xE1A00000; // nop
		*(u32*)0x0200DB70 = 0xE1A00000; // nop
		*(u32*)0x0200FED4 = 0xE1A00000; // nop
		*(u32*)0x02011B0C = 0xE1A00000; // nop
		*(u32*)0x02011BA4 = 0xE1A00000; // nop
		*(u32*)0x02011BA8 = 0xE1A00000; // nop
		*(u32*)0x02011BB4 = 0xE1A00000; // nop
		*(u32*)0x02011CF8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02011D54, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02015D50 = 0xE1A00000; // nop
	}

	// Nintendo DSi + Internet (Europe)
	else if (strcmp(romTid, "K2DP") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x020059AC = 0xE1A00000; // nop
		*(u32*)0x020059B8 = 0xE1A00000; // nop
		*(u32*)0x020059C8 = 0xE1A00000; // nop
		*(u32*)0x020059D4 = 0xE1A00000; // nop
		*(u32*)0x0200ADE0 = 0xE1A00000; // nop
		*(u32*)0x0200DE24 = 0xE1A00000; // nop
		*(u32*)0x02010188 = 0xE1A00000; // nop
		*(u32*)0x02011DB0 = 0xE1A00000; // nop
		*(u32*)0x02011E48 = 0xE1A00000; // nop
		*(u32*)0x02011E4C = 0xE1A00000; // nop
		*(u32*)0x02011E58 = 0xE1A00000; // nop
		*(u32*)0x02011F9C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02011FF8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02016048 = 0xE1A00000; // nop
	}

	// Nintendo DSi + Internet (Australia)
	else if (strcmp(romTid, "K2DU") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x020059AC = 0xE1A00000; // nop
		*(u32*)0x020059B8 = 0xE1A00000; // nop
		*(u32*)0x020059C8 = 0xE1A00000; // nop
		*(u32*)0x020059D4 = 0xE1A00000; // nop
		*(u32*)0x0200AB70 = 0xE1A00000; // nop
		*(u32*)0x0200DBB4 = 0xE1A00000; // nop
		*(u32*)0x0200FF18 = 0xE1A00000; // nop
		*(u32*)0x02011B40 = 0xE1A00000; // nop
		*(u32*)0x02011BD8 = 0xE1A00000; // nop
		*(u32*)0x02011BDC = 0xE1A00000; // nop
		*(u32*)0x02011BE8 = 0xE1A00000; // nop
		*(u32*)0x02011D2C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02011D88, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02015DD8 = 0xE1A00000; // nop
	}

	// Nintendoji (Japan)
	// Audio does not play, requires more patches to get past the save screen
	else if (strcmp(romTid, "K9KJ") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005160 = 0xE3A01601; // mov r1, #0x100000
		*(u32*)0x020051C0 = 0xE1A00000; // nop
		*(u32*)0x020051C4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x02005538 = 0xE1A00000; // nop
		*(u32*)0x0200554C = 0xE1A00000; // nop
		*(u32*)0x02018C08 = 0xE1A00000; // nop
		*(u32*)0x0201A9F8 = 0xE1A00000; // nop
		*(u32*)0x0201A9FC = 0xE1A00000; // nop
		*(u32*)0x0201AA08 = 0xE1A00000; // nop
		*(u32*)0x0201AB68 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201ABC4, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201E7B0 = 0xE1A00000; // nop
		*(u32*)0x0209EEB8 = 0xE1A00000; // nop
	}

	// Number Battle
	// Shows "WiFi connection information erased" message on boot
	else if (strcmp(romTid, "KSUE") == 0) {
		*(u32*)0x02005330 = 0xE1A00000; // nop
		*(u32*)0x02005EA4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02005EA8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005FA0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02005FA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02006130 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02006134 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200619C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020061A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02006384 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020063F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200657C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020065CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02006B68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200CB10 = 0xE1A00000; // nop
		*(u32*)0x0200CB20 = 0xE1A00000; // nop
		*(u32*)0x0200CBF8 = 0xE1A00000; // nop
		*(u32*)0x02021C20 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x020225C0 = 0xE1A00000; // nop
		*(u32*)0x020225CC = 0xE1A00000; // nop
		*(u32*)0x02022620 = 0xE1A00000; // nop
		*(u32*)0x02065870 = 0xE1A00000; // nop
		*(u32*)0x02065874 = 0xE1A00000; // nop
		*(u32*)0x0206588C = 0xE1A00000; // nop
		*(u32*)0x020A9EAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020CE79C = 0xE1A00000; // nop
		*(u32*)0x020D2A3C = 0xE1A00000; // nop
		*(u32*)0x020DA2B4 = 0xE1A00000; // nop
		*(u32*)0x020DD3C0 = 0xE1A00000; // nop
		*(u32*)0x020DD3C4 = 0xE1A00000; // nop
		*(u32*)0x020DD3D0 = 0xE1A00000; // nop
		*(u32*)0x020DD514 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020DD570, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020DD6A4 = 0x0234F020;
		*(u32*)0x020DEA44 = 0xE1A00000; // nop
		*(u32*)0x020DEA4C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020DEA6C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEA70 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEA78 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEA7C = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEA9C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEAA0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEAB0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEAB4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEAC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DEAC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEF00 = 0xE1A00000; // nop
		*(u32*)0x020DEF04 = 0xE1A00000; // nop
		*(u32*)0x020DEF08 = 0xE1A00000; // nop
		*(u32*)0x020DEF0C = 0xE1A00000; // nop
		*(u32*)0x020E256C = 0xE1A00000; // nop
		*(u32*)0x020E4774 = 0xE3A00003; // mov r0, #3
	}

	// Odekake! Earth Seeker (Japan)
	// Black screens after company logos
	// Seemingly not possible to fix the cause? (Fails to read or write save)
	/*else if (strcmp(romTid, "KA7J") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020107E8 = 0xE1A00000; // nop
		*(u32*)0x02014350 = 0xE1A00000; // nop
		*(u32*)0x02018864 = 0xE1A00000; // nop
		*(u32*)0x0201A6B0 = 0xE1A00000; // nop
		*(u32*)0x0201A6B4 = 0xE1A00000; // nop
		*(u32*)0x0201A6C0 = 0xE1A00000; // nop
		*(u32*)0x0201A820 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201A87C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201BD30 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201C4A4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201C4A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F500 = 0xE1A00000; // nop
		*(u32*)0x02022DA4 = 0xE1A00000; // nop
		*(u32*)0x0202B070 = 0xE1A00000; // nop
		*(u32*)0x0202B078 = 0xE1A00000; // nop
		*(u32*)0x0202B080 = 0xE1A00000; // nop
		//*(u32*)0x02036694 = 0xE12FFF1E; // bx lr
		*(u32*)0x020366AC = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020366C4 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x020366E4 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x02036708 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x02036738 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02036C94 = 0xE1A00000; // nop
		*(u32*)0x0205D920 = 0xE1A00000; // nop
	}*/

	// Paul's Shooting Adventure (USA)
	else if (strcmp(romTid, "KPJE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200FF94 = 0xE1A00000; // nop
		*(u32*)0x0201362C = 0xE1A00000; // nop
		*(u32*)0x02017F20 = 0xE1A00000; // nop
		*(u32*)0x02019CB4 = 0xE1A00000; // nop
		*(u32*)0x02019CB8 = 0xE1A00000; // nop
		*(u32*)0x02019CC4 = 0xE1A00000; // nop
		*(u32*)0x02019E24 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019E80, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201B104 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201E40C = 0xE1A00000; // nop
		*(u32*)0x0203A20C = 0xE12FFF1E; // bx lr
	}

	// Paul's Shooting Adventure 2 (USA)
	else if (strcmp(romTid, "KUSE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02016008 = 0xE1A00000; // nop
		*(u32*)0x02019E58 = 0xE1A00000; // nop
		*(u32*)0x0201F9B0 = 0xE1A00000; // nop
		*(u32*)0x02021874 = 0xE1A00000; // nop
		*(u32*)0x02021878 = 0xE1A00000; // nop
		*(u32*)0x02021884 = 0xE1A00000; // nop
		*(u32*)0x020219E4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02021A40, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02022E98 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02022EB4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022EB8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022EC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02022EC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02026298 = 0xE1A00000; // nop
		*(u32*)0x0202A968 = 0xE1A00000; // nop
		*(u32*)0x0202A980 = 0xE1A00000; // nop
		*(u32*)0x0203A730 = 0xE3A00001; // mov r0, #1
	}

	// Petit Computer (USA)
	// Does not boot (black screens, seems to rely on code from DSi binaries)
	/*else if (strcmp(romTid, "KNAE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200523C = 0xE1A00000; // nop
		*(u32*)0x0200EB90 = 0xE1A00000; // nop
		*(u32*)0x0200EB98 = 0xE1A00000; // nop
		*(u32*)0x0200EBB4 = 0xE1A00000; // nop
		*(u32*)0x0200EBBC = 0xE1A00000; // nop
		*(u32*)0x0200EBF0 = 0xE1A00000; // nop
		*(u32*)0x0200EC08 = 0xE1A00000; // nop
		*(u32*)0x0200EC30 = 0xE1A00000; // nop
		*(u32*)0x02086F5C = 0xE1A00000; // nop
		*(u32*)0x0208B504 = 0xE1A00000; // nop
		*(u32*)0x0208BCBC = 0xE1A00000; // nop
		*(u32*)0x0208BCD8 = 0xE1A00000; // nop
		*(u32*)0x0208BF4C = 0xE1A00000; // nop
		*(u32*)0x0208C288 = 0xE1A00000; // nop
		*(u32*)0x0208C2A0 = 0xE1A00000; // nop
		*(u32*)0x0208C8D8 = 0xE1A00000; // nop
		*(u32*)0x0208C974 = 0xE1A00000; // nop
		*(u32*)0x0208CA28 = 0xE1A00000; // nop
		*(u32*)0x0208CADC = 0xE1A00000; // nop
		*(u32*)0x0208CB7C = 0xE1A00000; // nop
		*(u32*)0x0208CBFC = 0xE1A00000; // nop
		*(u32*)0x0208CC78 = 0xE1A00000; // nop
		*(u32*)0x0208CCFC = 0xE1A00000; // nop
		*(u32*)0x0208CD9C = 0xE1A00000; // nop
		*(u32*)0x0208CE58 = 0xE1A00000; // nop
		*(u32*)0x0208CF14 = 0xE1A00000; // nop
		*(u32*)0x0208CFD0 = 0xE1A00000; // nop
		*(u32*)0x0208D08C = 0xE1A00000; // nop
		*(u32*)0x0208D148 = 0xE1A00000; // nop
		*(u32*)0x0208D204 = 0xE1A00000; // nop
		*(u32*)0x0208D2C0 = 0xE1A00000; // nop
		*(u32*)0x0208D36C = 0xE1A00000; // nop
		*(u32*)0x0208D400 = 0xE1A00000; // nop
		*(u32*)0x0208D494 = 0xE1A00000; // nop
		*(u32*)0x0208D528 = 0xE1A00000; // nop
		*(u32*)0x0208D5BC = 0xE1A00000; // nop
		*(u32*)0x0208D650 = 0xE1A00000; // nop
		*(u32*)0x0208D6E4 = 0xE1A00000; // nop
		*(u32*)0x0208D778 = 0xE1A00000; // nop
		*(u32*)0x0208D89C = 0xE1A00000; // nop
		*(u32*)0x0208D900 = 0xE1A00000; // nop
		*(u32*)0x0208D9C8 = 0xE1A00000; // nop
		*(u32*)0x0208DA38 = 0xE1A00000; // nop
		*(u32*)0x0208DAC4 = 0xE1A00000; // nop
		*(u32*)0x0208DB34 = 0xE1A00000; // nop
		*(u32*)0x0208DBBC = 0xE1A00000; // nop
		*(u32*)0x0208DC2C = 0xE1A00000; // nop
		*(u32*)0x0208DD40 = 0xE1A00000; // nop
		*(u32*)0x0208DE28 = 0xE1A00000; // nop
		*(u32*)0x0208DE8C = 0xE1A00000; // nop
		*(u32*)0x0208DF44 = 0xE1A00000; // nop
		*(u32*)0x0208DFB4 = 0xE1A00000; // nop
		*(u32*)0x02090994 = 0xE1A00000; // nop
		*(u32*)0x02092A74 = 0xE1A00000; // nop
		*(u32*)0x02092A78 = 0xE1A00000; // nop
		*(u32*)0x02092A84 = 0xE1A00000; // nop
		*(u32*)0x02092BE4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02092C40, heapEnd); // mov r0, #0x23C0000
	}*/

	// Phantasy Star 0 Mini (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KPSJ") == 0 && extendedMemory2) {
		*(u32*)0x02007FA8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02007FAC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0200CC88 = 0xE1A00000; // nop
		*(u32*)0x02015F6C = 0xE1A00000; // nop
		*(u32*)0x0202D9DC = 0xE1A00000; // nop
		*(u32*)0x0202D9E0 = 0xE1A00000; // nop
		*(u32*)0x0202D9EC = 0xE1A00000; // nop
		*(u32*)0x0202DB30 = 0xE1A00000; // nop
		*(u32*)0x0202DB34 = 0xE1A00000; // nop
		*(u32*)0x0202DB38 = 0xE1A00000; // nop
		*(u32*)0x0202DB3C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202DB98, 0x02700000); // mov r0, #0x2700000
		//*(u32*)0x0202DCCC = 0x0218B0A0;
		*(u16*)0x0209F778 = 0x46C0; // nop
		*(u16*)0x0209F77A = 0x46C0; // nop
	}

	// Art Style: PiCTOBiTS (USA)
	else if (strcmp(romTid, "KAPE") == 0) {
		*(u32*)0x0200518C = 0xE1A00000; // nop
		*(u32*)0x02005198 = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x020059E4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020059E8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020395E0 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020395E4 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203CC10 = 0xE1A00000; // nop
		*(u32*)0x0204153C = 0xE1A00000; // nop
		*(u32*)0x020432A4 = 0xE1A00000; // nop
		*(u32*)0x020432A8 = 0xE1A00000; // nop
		*(u32*)0x020432B4 = 0xE1A00000; // nop
		*(u32*)0x020433F8 = 0xE1A00000; // nop
		*(u32*)0x020433FC = 0xE1A00000; // nop
		*(u32*)0x02043400 = 0xE1A00000; // nop
		*(u32*)0x02043404 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02043460, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02044804 = 0xE1A00000; // nop
		*(u32*)0x0204480C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Art Style: PiCOPiCT (Europe, Australia)
	else if (strcmp(romTid, "KAPV") == 0) {
		*(u32*)0x0200518C = 0xE1A00000; // nop
		*(u32*)0x02005198 = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x020059E4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020059E8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02039658 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0203965C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203CC88 = 0xE1A00000; // nop
		*(u32*)0x020415B0 = 0xE1A00000; // nop
		*(u32*)0x0204331C = 0xE1A00000; // nop
		*(u32*)0x02043320 = 0xE1A00000; // nop
		*(u32*)0x0204332C = 0xE1A00000; // nop
		*(u32*)0x02043470 = 0xE1A00000; // nop
		*(u32*)0x02043474 = 0xE1A00000; // nop
		*(u32*)0x02043478 = 0xE1A00000; // nop
		*(u32*)0x0204347C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020434D8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0204487C = 0xE1A00000; // nop
		*(u32*)0x02044884 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Art Style: PiCOPiCT (Japan)
	else if (strcmp(romTid, "KAPJ") == 0) {
		*(u32*)0x02005194 = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A4 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x020051B0 = 0xE1A00000; // nop
		*(u32*)0x02005A8C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005A90 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203990C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02039910 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203CF3C = 0xE1A00000; // nop
		*(u32*)0x02041868 = 0xE1A00000; // nop
		*(u32*)0x020435B4 = 0xE1A00000; // nop
		*(u32*)0x020435B8 = 0xE1A00000; // nop
		*(u32*)0x020435C4 = 0xE1A00000; // nop
		*(u32*)0x02043708 = 0xE1A00000; // nop
		*(u32*)0x0204370C = 0xE1A00000; // nop
		*(u32*)0x02043710 = 0xE1A00000; // nop
		*(u32*)0x02043714 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02043770, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0204487C = 0xE1A00000; // nop
		*(u32*)0x02044884 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Picture Perfect: Pocket Stylist (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KHRE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		for (int i = 0; i < 7; i++) {
			u32* offset = (u32*)0x02005194;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x020051B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200D14C = 0xE1A00000; // nop
		*(u32*)0x0201300C = 0xE1A00000; // nop
		*(u32*)0x02013144 = 0xE1A00000; // nop
		*(u32*)0x02013158 = 0xE1A00000; // nop
		*(u32*)0x02016F98 = 0xE1A00000; // nop
		*(u32*)0x0201D1DC = 0xE1A00000; // nop
		*(u32*)0x0201F0E0 = 0xE1A00000; // nop
		*(u32*)0x0201F0E4 = 0xE1A00000; // nop
		*(u32*)0x0201F0F0 = 0xE1A00000; // nop
		*(u32*)0x0201F250 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F2AC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201F3E0 = 0x022B3440;
		*(u32*)0x02020614 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023D6C = 0xE1A00000; // nop
		*(u32*)0x02044080 = 0xE1A00000; // nop
		*(u32*)0x020440A0 = 0xE1A00000; // nop
		*(u32*)0x0205C7A8 = 0xE1A00000; // nop
	}

	// Pinball Pulse: The Ancients Beckon (USA)
	// Incomplete/broken patch
	/*else if (strcmp(romTid, "KZPE") == 0) {
		*(u32*)0x02004988 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x02008240 = 0xE12FFF1E; // bx lr
		for (int i = 0; i < 19; i++) {
			u32* offset1 = (u16*)0x02013614;
			u32* offset2 = (u16*)0x020136C4;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
		}
		*(u32*)0x02013D2C = 0xE1A00000; // nop
		*(u32*)0x0202C6E0 = 0xE1A00000; // nop
		*(u32*)0x02049058 = 0xE1A00000; // nop
		*(u32*)0x020655AC = 0xE1A00000; // nop
		*(u32*)0x02067530 = 0xE1A00000; // nop
		*(u32*)0x0206ABA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02087214 = 0xE1A00000; // nop
		*(u32*)0x0208A6E8 = 0xE1A00000; // nop
		*(u32*)0x0208DFCC = 0xE1A00000; // nop
		*(u32*)0x0208FE18 = 0xE1A00000; // nop
		*(u32*)0x0208FE1C = 0xE1A00000; // nop
		*(u32*)0x0208FE28 = 0xE1A00000; // nop
		*(u32*)0x0208FF6C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0208FFC8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0209147C = 0xE1A00000; // nop
		*(u32*)0x02091480 = 0xE1A00000; // nop
		*(u32*)0x02091484 = 0xE1A00000; // nop
		*(u32*)0x02091488 = 0xE1A00000; // nop
	}*/

	// GO Series: Portable Shrine Wars (USA)
	// GO Series: Portable Shrine Wars (Europe)
	else if (strcmp(romTid, "KOQE") == 0 || strcmp(romTid, "KOQP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020073E0 = 0xE1A00000; // nop
		*(u32*)0x0200E004 = 0xE1A00000; // nop  (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204E1B8 = 0xE1A00000; // nop
		*(u32*)0x02051F6C = 0xE1A00000; // nop
		*(u32*)0x02057C84 = 0xE1A00000; // nop
		*(u32*)0x02059A6C = 0xE1A00000; // nop
		*(u32*)0x02059A70 = 0xE1A00000; // nop
		*(u32*)0x02059A7C = 0xE1A00000; // nop
		*(u32*)0x02059BDC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02059C38, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0205AEBC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205E5CC = 0xE1A00000; // nop

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200DE78;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Art Style: precipice (USA)
	else if (strcmp(romTid, "KAKE") == 0) {
		*(u32*)0x02007768 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200776C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200B370 = 0xE1A00000; // nop
		*(u32*)0x0200B37C = 0xE1A00000; // nop
		*(u32*)0x0200B3AC = 0xE1A00000; // nop
		*(u32*)0x0200B3B0 = 0xE1A00000; // nop
		*(u32*)0x0200B3B8 = 0xE1A00000; // nop
		*(u32*)0x0203B6AC = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0203B6B0 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203EAC0 = 0xE1A00000; // nop
		*(u32*)0x02045C18 = 0xE1A00000; // nop
		*(u32*)0x02047A0C = 0xE1A00000; // nop
		*(u32*)0x02047A10 = 0xE1A00000; // nop
		*(u32*)0x02047A1C = 0xE1A00000; // nop
		*(u32*)0x02047B60 = 0xE1A00000; // nop
		*(u32*)0x02047B64 = 0xE1A00000; // nop
		*(u32*)0x02047B68 = 0xE1A00000; // nop
		*(u32*)0x02047B6C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02047BC8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02048F2C = 0xE1A00000; // nop
		*(u32*)0x02048F34 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Art Style: KUBOS (Europe, Australia)
	else if (strcmp(romTid, "KAKV") == 0) {
		*(u32*)0x02007768 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200776C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200B370 = 0xE1A00000; // nop
		*(u32*)0x0200B37C = 0xE1A00000; // nop
		*(u32*)0x0200B3AC = 0xE1A00000; // nop
		*(u32*)0x0200B3B0 = 0xE1A00000; // nop
		*(u32*)0x0200B3B8 = 0xE1A00000; // nop
		*(u32*)0x0203B6A4 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0203B6A8 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203EAB8 = 0xE1A00000; // nop
		*(u32*)0x02045C10 = 0xE1A00000; // nop
		*(u32*)0x02047A04 = 0xE1A00000; // nop
		*(u32*)0x02047A08 = 0xE1A00000; // nop
		*(u32*)0x02047A14 = 0xE1A00000; // nop
		*(u32*)0x02047B58 = 0xE1A00000; // nop
		*(u32*)0x02047B5C = 0xE1A00000; // nop
		*(u32*)0x02047B60 = 0xE1A00000; // nop
		*(u32*)0x02047B64 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02047BC0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02048F24 = 0xE1A00000; // nop
		*(u32*)0x02048F2C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Pro-Putt Domo (USA)
	else if (strcmp(romTid, "KDPE") == 0) {
		*(u16*)0x020106BC = 0x2001; // movs r0, #1
		*(u16*)0x020106BE = 0x4770; // bx lr
		*(u16*)0x020179F4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02026262);
		doubleNopT(0x020284B2);
		doubleNopT(0x0202B2D4);
		doubleNopT(0x0202C88E);
		doubleNopT(0x0202C892);
		doubleNopT(0x0202C89E);
		doubleNopT(0x0202C982);
		patchHiHeapDSiWareThumb(0x0202C9C0, 0x208F, 0x0480); // movs r0, #0x23C0000
		doubleNopT(0x0202DA1E);
		*(u16*)0x0202DA22 = 0x46C0; // nop
		*(u16*)0x0202DA24 = 0x46C0; // nop
		doubleNopT(0x0202DA26);
		doubleNopT(0x0202FADA);
	}

	// Puzzle League: Express (USA)
	else if (strcmp(romTid, "KPNE") == 0) {
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02023614 = 0xE3A00001; // mov r0, #1 (Hide volume icon in gameplay)
		*(u32*)0x0203604C = 0xE1A00000; // nop
		*(u32*)0x0205663C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02056640 = 0xE12FFF1E; // bx lr
		*(u32*)0x02056A28 = 0xE12FFF1E; // bx lr
		*(u32*)0x02064DD0 = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020ACF54 = 0xE1A00000; // nop
		*(u32*)0x020B1334 = 0xE1A00000; // nop
		*(u32*)0x020BE08C = 0xE1A00000; // nop
		*(u32*)0x020C0004 = 0xE1A00000; // nop
		*(u32*)0x020C0008 = 0xE1A00000; // nop
		*(u32*)0x020C0014 = 0xE1A00000; // nop
		*(u32*)0x020C0158 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020C01B4, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x020C1668 = 0xE1A00000; // nop
		*(u32*)0x020C1670 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020C52F8 = 0xE1A00000; // nop
	}

	// A Little Bit of... Puzzle League (Europe, Australia)
	else if (strcmp(romTid, "KPNV") == 0) {
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02023634 = 0xE3A00001; // mov r0, #1 (Hide volume icon in gameplay)
		*(u32*)0x02036FA0 = 0xE1A00000; // nop
		*(u32*)0x020575FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02057600 = 0xE12FFF1E; // bx lr
		*(u32*)0x020579E8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02065DE4 = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020AE94C = 0xE1A00000; // nop
		*(u32*)0x020BD2DC = 0xE1A00000; // nop
		*(u32*)0x020BFA84 = 0xE1A00000; // nop
		*(u32*)0x020C19FC = 0xE1A00000; // nop
		*(u32*)0x020C1A00 = 0xE1A00000; // nop
		*(u32*)0x020C1A0C = 0xE1A00000; // nop
		*(u32*)0x020C1B50 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020C1BAC, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x020C3060 = 0xE1A00000; // nop
		*(u32*)0x020C3068 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020C6CF0 = 0xE1A00000; // nop
	}

	// Chotto Panel de Pon (Japan)
	else if (strcmp(romTid, "KPNJ") == 0) {
		*(u32*)0x02005068 = 0xE1A00000; // nop
		*(u32*)0x02005070 = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x02023404 = 0xE3A00001; // mov r0, #1 (Hide volume icon in gameplay)
		*(u32*)0x02035BB8 = 0xE1A00000; // nop
		*(u32*)0x02056128 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205612C = 0xE12FFF1E; // bx lr
		*(u32*)0x02056514 = 0xE12FFF1E; // bx lr
		*(u32*)0x02064FEC = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020ADCCC = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020ADCD0 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020B21BC = 0xE1A00000; // nop
		*(u32*)0x020C0B54 = 0xE1A00000; // nop
		*(u32*)0x020C29C0 = 0xE1A00000; // nop
		*(u32*)0x020C29C4 = 0xE1A00000; // nop
		*(u32*)0x020C29D0 = 0xE1A00000; // nop
		*(u32*)0x020C2B14 = 0xE1A00000; // nop
		*(u32*)0x020C2B18 = 0xE1A00000; // nop
		*(u32*)0x020C2B1C = 0xE1A00000; // nop
		*(u32*)0x020C2B20 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020C2B7C, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x020C4030 = 0xE1A00000; // nop
		*(u32*)0x020C4038 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Quick Fill Q (USA)
	// Quick Fill Q (Europe)
	else if (strcmp(romTid, "KUME") == 0 || strcmp(romTid, "KUMP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02010300 = 0xE1A00000; // nop
		*(u32*)0x020139A4 = 0xE1A00000; // nop
		*(u32*)0x02017460 = 0xE1A00000; // nop
		*(u32*)0x020191FC = 0xE1A00000; // nop
		*(u32*)0x02019200 = 0xE1A00000; // nop
		*(u32*)0x0201920C = 0xE1A00000; // nop
		*(u32*)0x0201936C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020193C8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201A740 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201DDB0 = 0xE1A00000; // nop
		*(u32*)0x02040240 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rabi Laby (USA)
	// Rabi Laby (Europe)
	else if (strcmp(romTid, "KLBE") == 0 || strcmp(romTid, "KLBP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop
		*(u32*)0x020053A8 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0200DC38 = 0xE1A00000; // nop
		*(u32*)0x020110D4 = 0xE1A00000; // nop
		*(u32*)0x020149F0 = 0xE1A00000; // nop
		*(u32*)0x0201678C = 0xE1A00000; // nop
		*(u32*)0x02016790 = 0xE1A00000; // nop
		*(u32*)0x0201679C = 0xE1A00000; // nop
		*(u32*)0x020168FC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016958, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02017BA0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201A848 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0202663C = 0xE1A00000; // nop
			*(u32*)0x020266F8 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020266CC = 0xE1A00000; // nop
			*(u32*)0x02026788 = 0xE1A00000; // nop
		}
	}

	// Akushon Pazuru: Rabi x Rabi (Japan)
	else if (strcmp(romTid, "KLBJ") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005190 = 0xE1A00000; // nop
		*(u32*)0x02005360 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0200DBAC = 0xE1A00000; // nop
		*(u32*)0x020148C8 = 0xE1A00000; // nop
		*(u32*)0x0201665C = 0xE1A00000; // nop
		*(u32*)0x02016660 = 0xE1A00000; // nop
		*(u32*)0x0201666C = 0xE1A00000; // nop
		*(u32*)0x020167CC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016828, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02017A70 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201A718 = 0xE1A00000; // nop
		*(u32*)0x02026BD0 = 0xE1A00000; // nop
		*(u32*)0x02026CB4 = 0xE1A00000; // nop
	}

	// Rabi Laby 2 (USA)
	// Rabi Laby 2 (Europe)
	// Akushon Pazuru: Rabi x Rabi Episodo 2 (Japan)
	else if (strncmp(romTid, "KLV", 3) == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051E8 = 0xE1A00000; // nop
		*(u32*)0x0200540C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0200DAE4 = 0xE1A00000; // nop
		*(u32*)0x02010F80 = 0xE1A00000; // nop
		*(u32*)0x0201489C = 0xE1A00000; // nop
		*(u32*)0x02016638 = 0xE1A00000; // nop
		*(u32*)0x0201663C = 0xE1A00000; // nop
		*(u32*)0x02016648 = 0xE1A00000; // nop
		*(u32*)0x020167A8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016804, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02017A4C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201A6F4 = 0xE1A00000; // nop
	}

	// Robot Rescue (USA)
	else if (strcmp(romTid, "KRTE") == 0) {
		*(u32*)0x0200C2DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2E0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C39C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C570 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C574 = 0xE12FFF1E; // bx lr
		*(u32*)0x020108A4 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202A484 = 0xE1A00000; // nop
		*(u32*)0x0202D844 = 0xE1A00000; // nop
		*(u32*)0x02031480 = 0xE1A00000; // nop
		*(u32*)0x02033264 = 0xE1A00000; // nop
		*(u32*)0x02033268 = 0xE1A00000; // nop
		*(u32*)0x02033274 = 0xE1A00000; // nop
		*(u32*)0x020333B8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02033414, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02033548 = 0x020AC1C0;
		*(u32*)0x0203481C = 0xE1A00000; // nop
		*(u32*)0x02034824 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02037D48 = 0xE1A00000; // nop
	}

	// Robot Rescue (Europe, Australia)
	else if (strcmp(romTid, "KRTV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200C084 = 0xE1A00000; // nop
		*(u32*)0x0200C08C = 0xE1A00000; // nop
		*(u32*)0x0200C2CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C388 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C38C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C550 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C554 = 0xE12FFF1E; // bx lr
		*(u32*)0x02010C30 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202A56C = 0xE1A00000; // nop
		*(u32*)0x0202D7CC = 0xE1A00000; // nop
		*(u32*)0x020311B4 = 0xE1A00000; // nop
		*(u32*)0x02032F48 = 0xE1A00000; // nop
		*(u32*)0x02032F4C = 0xE1A00000; // nop
		*(u32*)0x02032F58 = 0xE1A00000; // nop
		*(u32*)0x020330B8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02033114, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02033248 = 0x020A8160;
		*(u32*)0x020344FC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02037954 = 0xE1A00000; // nop
	}

	// ARC Style: Robot Rescue (Japan)
	else if (strcmp(romTid, "KRTJ") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200F218 = 0xE1A00000; // nop
		*(u32*)0x0200F220 = 0xE1A00000; // nop
		*(u32*)0x0200F460 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F464 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F51C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F520 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F6E4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F6E8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02013BC8 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202D3E0 = 0xE1A00000; // nop
		*(u32*)0x02030640 = 0xE1A00000; // nop
		*(u32*)0x02034028 = 0xE1A00000; // nop
		*(u32*)0x02035DBC = 0xE1A00000; // nop
		*(u32*)0x02035DC0 = 0xE1A00000; // nop
		*(u32*)0x02035DCC = 0xE1A00000; // nop
		*(u32*)0x02035F2C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02035F88, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x020360BC = 0x020AE580;
		*(u32*)0x02037370 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0203A7C8 = 0xE1A00000; // nop
	}

	// Robot Rescue 2 (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KRRE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200C0AC = 0xE1A00000; // nop
		*(u32*)0x0200C0B4 = 0xE1A00000; // nop
		*(u32*)0x0200C2F4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C3B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C578 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C57C = 0xE12FFF1E; // bx lr
		*(u32*)0x02010888 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02033384 = 0xE1A00000; // nop
		*(u32*)0x02036678 = 0xE1A00000; // nop
		*(u32*)0x0203A098 = 0xE1A00000; // nop
		*(u32*)0x0203BE34 = 0xE1A00000; // nop
		*(u32*)0x0203BE38 = 0xE1A00000; // nop
		*(u32*)0x0203BE44 = 0xE1A00000; // nop
		*(u32*)0x0203BFA4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203C000, 0x02F00000); // mov r0, #0x2F00000 (mirrors to 0x2700000 on debug DS units)
		*(u32*)0x0203C134 = 0x020BEAA0;
		*(u32*)0x0203D3F8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02040850 = 0xE1A00000; // nop
	}

	// Robot Rescue 2 (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KRRP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200C0AC = 0xE1A00000; // nop
		*(u32*)0x0200C0B4 = 0xE1A00000; // nop
		*(u32*)0x0200C2F4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C3B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C578 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C57C = 0xE12FFF1E; // bx lr
		*(u32*)0x02010BE4 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02036A54 = 0xE1A00000; // nop
		*(u32*)0x0203A474 = 0xE1A00000; // nop
		*(u32*)0x0203C20C = 0xE1A00000; // nop
		*(u32*)0x0203C210 = 0xE1A00000; // nop
		*(u32*)0x0203C214 = 0xE1A00000; // nop
		*(u32*)0x0203C380 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203C3DC, 0x02F00000); // mov r0, #0x2F00000 (mirrors to 0x2700000 on debug DS units)
		*(u32*)0x0203C510 = 0x020BF400;
		*(u32*)0x0203D7D4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02040C2C = 0xE1A00000; // nop
	}

	// Rock-n-Roll Domo (USA)
	else if (strcmp(romTid, "KD6E") == 0) {
		*(u16*)0x02010164 = 0x2001; // movs r0, #1
		*(u16*)0x02010166 = 0x4770; // bx lr
		*(u16*)0x02016514 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02024D86);
		doubleNopT(0x02026FD6);
		doubleNopT(0x02029C58);
		doubleNopT(0x0202B212);
		doubleNopT(0x0202B216);
		doubleNopT(0x0202B222);
		doubleNopT(0x0202B306);
		patchHiHeapDSiWareThumb(0x0202B344, 0x208F, 0x0480); // movs r0, #0x23C0000
		doubleNopT(0x0202C3A2);
		*(u16*)0x0202C3A6 = 0x46C0;
		*(u16*)0x0202C3A8 = 0x46C0;
		doubleNopT(0x0202C3AA);
		doubleNopT(0x0202E45E);
	}

	// Shantae: Risky's Revenge (USA)
	// Requires 8MB of RAM, crashes after first battle with 4MB of RAM
	// BGM is disabled to stay within RAM limitations
	else if (strcmp(romTid, "KS3E") == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleForce[0] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02026F68;

		*(u32*)0x0200498C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0201FBA0 = 0xE12FFF1E; // bx lr
			/* *(u32*)0x0201FD3C = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FDA8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FE14 = 0xE12FFF1E; // bx lr */
		}
		*(u32*)0x0201FC20 = 0xE12FFF1E; // bx lr (Disable loading sdat file)
		*(u32*)0x02026F68 = generateA7Instr(0x02026F68, (int)ce9->patches->rumble_arm9[0]); // Rumble when hair is whipped
		*(u32*)0x02092050 = 0xE1A00000; // nop
		*(u32*)0x02092078 = 0xE3A05001; // mov r5, #1
		*(u32*)0x02092B94 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DE420 = 0xE1A00000; // nop
		*(u32*)0x020DE548 = 0xE1A00000; // nop
		*(u32*)0x020DE55C = 0xE1A00000; // nop
		*(u32*)0x020E20C4 = 0xE1A00000; // nop
		*(u32*)0x020E616C = 0xE1A00000; // nop
		*(u32*)0x020E7F64 = 0xE1A00000; // nop
		*(u32*)0x020E7F68 = 0xE1A00000; // nop
		*(u32*)0x020E7F74 = 0xE1A00000; // nop
		*(u32*)0x020E80D4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020E8130, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x020E8264 = 0x02186C60;
		*(u32*)0x020E9348 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020E977C = 0xE1A00000; // nop
		*(u32*)0x020E9780 = 0xE1A00000; // nop
		*(u32*)0x020E9784 = 0xE1A00000; // nop
		*(u32*)0x020E9788 = 0xE1A00000; // nop
		*(u32*)0x020E9794 = 0xE1A00000; // nop (Enable error exception screen)
		*(u32*)0x020ECBEC = 0xE1A00000; // nop
	}

	// Shantae: Risky's Revenge (Europe)
	// Requires 8MB of RAM, crashes after first battle with 4MB of RAM
	// BGM is disabled to stay within RAM limitations
	else if (strcmp(romTid, "KS3P") == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleForce[0] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x020271E0;

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0201FE18 = 0xE12FFF1E; // bx lr
			/* *(u32*)0x0201FFB4 = 0xE12FFF1E; // bx lr
			*(u32*)0x02020020 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202008C = 0xE12FFF1E; // bx lr */
		}
		*(u32*)0x0201FE98 = 0xE12FFF1E; // bx lr (Disable loading sdat file)
		*(u32*)0x020271E0 = generateA7Instr(0x020271E0, (int)ce9->patches->rumble_arm9[0]); // Rumble when hair is whipped
		*(u32*)0x020922D4 = 0xE1A00000; // nop
		*(u32*)0x020922FC = 0xE3A05001; // mov r5, #1
		*(u32*)0x02092FC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DE810 = 0xE1A00000; // nop
		*(u32*)0x020DE938 = 0xE1A00000; // nop
		*(u32*)0x020DE94C = 0xE1A00000; // nop
		*(u32*)0x020E253C = 0xE1A00000; // nop
		*(u32*)0x020E6600 = 0xE1A00000; // nop
		*(u32*)0x020E8400 = 0xE1A00000; // nop
		*(u32*)0x020E8404 = 0xE1A00000; // nop
		*(u32*)0x020E8410 = 0xE1A00000; // nop
		*(u32*)0x020E8570 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020E85CC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x020E8700 = 0x02190100;
		*(u32*)0x020E97E4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020E9C18 = 0xE1A00000; // nop
		*(u32*)0x020E9C1C = 0xE1A00000; // nop
		*(u32*)0x020E9C20 = 0xE1A00000; // nop
		*(u32*)0x020E9C24 = 0xE1A00000; // nop
		*(u32*)0x020E9C30 = 0xE1A00000; // nop (Enable error exception screen)
		*(u32*)0x020ED088 = 0xE1A00000; // nop
	}

	// Soul of Darkness (USA)
	// Does not boot: Black screens
	/*else if (strcmp(romTid, "KSKE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050F4 = 0xE1A00000; // nop
		*(u32*)0x0204CD4C = 0xE1A00000; // nop
		*(u32*)0x0204CDB8 = 0xE1A00000; // nop
		*(u32*)0x02088804 = 0xE1A00000; // nop
		*(u32*)0x0208BF00 = 0xE1A00000; // nop
		*(u32*)0x02090884 = 0xE1A00000; // nop
		*(u32*)0x02092714 = 0xE1A00000; // nop
		*(u32*)0x02092718 = 0xE1A00000; // nop
		*(u32*)0x02092724 = 0xE1A00000; // nop
		*(u32*)0x02092884 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020928E0, heapEnd); // mov r0, #0x23C0000
	}*/

	// Space Ace (USA)
	// Freezes after clearing high scores
	else if (strcmp(romTid, "KA6E") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop
		*(u32*)0x02005DD0 = 0xE1A00000; // nop
		*(u32*)0x02016458 = 0xE1A00000; // nop
		*(u32*)0x0201645C = 0xE1A00000; // nop
		*(u32*)0x02033768 = 0xE1A00000; // nop
		*(u32*)0x02033890 = 0xE1A00000; // nop
		*(u32*)0x020338A4 = 0xE1A00000; // nop
		*(u32*)0x02036B88 = 0xE1A00000; // nop
		*(u32*)0x0203A348 = 0xE1A00000; // nop
		*(u32*)0x0203C108 = 0xE1A00000; // nop
		*(u32*)0x0203C10C = 0xE1A00000; // nop
		*(u32*)0x0203C118 = 0xE1A00000; // nop
		*(u32*)0x0203C278 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203C2D4, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0203C408 = 0x02088940;
		*(u32*)0x0203D984 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0203DDC8 = 0xE1A00000; // nop
		*(u32*)0x0203DDCC = 0xE1A00000; // nop
		*(u32*)0x0203DDD0 = 0xE1A00000; // nop
		*(u32*)0x0203DDD4 = 0xE1A00000; // nop
		*(u32*)0x0203E9B4 = 0xE1A00000; // nop
		*(u32*)0x02040888 = 0xE1A00000; // nop
	}

	// Space Invaders Extreme Z (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEVJ") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02017904 = 0xE1A00000; // nop
		*(u32*)0x0201B794 = 0xE1A00000; // nop
		*(u32*)0x020207B0 = 0xE1A00000; // nop
		*(u32*)0x020225D8 = 0xE1A00000; // nop
		*(u32*)0x020225DC = 0xE1A00000; // nop
		*(u32*)0x020225E8 = 0xE1A00000; // nop
		*(u32*)0x02022748 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020227A4, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020228D8 = 0x0213CC60;
		*(u32*)0x02023A9C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02026EA0 = 0xE1A00000; // nop
		*(u32*)0x020E3E4C = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E3E50 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E43A4 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E43A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E4624 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E4628 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E4854 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E4858 = 0xE12FFF1E; // bx lr
	}

	// Spotto! (USA)
	// Does not boot: Issue unknown
	/*else if (strcmp(romTid, "KSPE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02012D80 = 0xE1A00000; // nop
		*(u32*)0x02022AB4 = 0xE1A00000; // nop
		*(u32*)0x02026038 = 0xE1A00000; // nop
		*(u32*)0x0202C280 = 0xE1A00000; // nop
		*(u32*)0x0202D6B8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202D6BC = 0xE12FFF1E; // bx lr
		*(u32*)0x0202E0E0 = 0xE1A00000; // nop
		*(u32*)0x0202E0E4 = 0xE1A00000; // nop
		*(u32*)0x0202E0F0 = 0xE1A00000; // nop
		*(u32*)0x0202E250 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202E2AC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02031EB4 = 0xE1A00000; // nop
		*(u32*)0x0204CA50 = 0xE1A00000; // nop
		*(u32*)0x0204CA74 = 0xE1A00000; // nop
		*(u32*)0x020558C4 = 0xE1A00000; // nop
		*(u32*)0x020558F4 = 0xE1A00000; // nop
		*(u32*)0x020558FC = 0xE1A00000; // nop
		*(u32*)0x02055FAC = 0xE3A00000; // mov r0, #0
	}*/

	// Sudoku (USA)
	// Sudoku (USA) (Rev 1)
	else if (strcmp(romTid, "K4DE") == 0) {
		if (ndsHeader->romversion == 1) {
			*(u32*)0x0200698C = 0xE1A00000; // nop
			*(u32*)0x0203701C = 0xE3A00001; // mov r0, #1
			*(u32*)0x02037020 = 0xE12FFF1E; // bx lr
			*(u32*)0x020B7CA4 = 0xE1A00000; // nop
			*(u32*)0x020BB4B8 = 0xE1A00000; // nop
			*(u32*)0x020C0D34 = 0xE1A00000; // nop
			*(u32*)0x020C2B18 = 0xE1A00000; // nop
			*(u32*)0x020C2B1C = 0xE1A00000; // nop
			*(u32*)0x020C2B28 = 0xE1A00000; // nop
			*(u32*)0x020C2C6C = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x020C2CC8, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x020C40E4 = 0xE1A00000; // nop
			*(u32*)0x020C40EC = 0xE8BD8010; // LDMFD SP!, {R4,PC}
			*(u32*)0x020C7E08 = 0xE1A00000; // nop
		} else {
			*(u32*)0x0200695C = 0xE1A00000; // nop
			*(u32*)0x0203609C = 0xE3A00001; // mov r0, #1
			*(u32*)0x020360A0 = 0xE12FFF1E; // bx lr
			*(u32*)0x020A0670 = 0xE1A00000; // nop
			*(u32*)0x020A3E84 = 0xE1A00000; // nop
			*(u32*)0x020A9700 = 0xE1A00000; // nop
			*(u32*)0x020AB4E4 = 0xE1A00000; // nop
			*(u32*)0x020AB4E8 = 0xE1A00000; // nop
			*(u32*)0x020AB4F4 = 0xE1A00000; // nop
			*(u32*)0x020AB638 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x020AB694, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x020ACAB0 = 0xE1A00000; // nop
			*(u32*)0x020ACAB8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
			*(u32*)0x020B0664 = 0xE1A00000; // nop
		}
	}

	// Sudoku (Europe, Australia) (Rev 1)
	else if (strcmp(romTid, "K4DV") == 0) {
		*(u32*)0x0200695C = 0xE1A00000; // nop
		*(u32*)0x020360E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020360EC = 0xE12FFF1E; // bx lr
		*(u32*)0x020A06BC = 0xE1A00000; // nop
		*(u32*)0x020A3ED0 = 0xE1A00000; // nop
		*(u32*)0x020A974C = 0xE1A00000; // nop
		*(u32*)0x020AB530 = 0xE1A00000; // nop
		*(u32*)0x020AB534 = 0xE1A00000; // nop
		*(u32*)0x020AB540 = 0xE1A00000; // nop
		*(u32*)0x020AB684 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020AB6E0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020ACAFC = 0xE1A00000; // nop
		*(u32*)0x020ACB04 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020B06B0 = 0xE1A00000; // nop
	}

	// Sudoku 4Pockets (USA)
	// Sudoku 4Pockets (Europe)
	else if (strcmp(romTid, "K4FE") == 0 || strcmp(romTid, "K4FP") == 0) {
		*(u32*)0x02004C4C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02013030 = 0xE1A00000; // nop
		*(u32*)0x0201680C = 0xE1A00000; // nop
		*(u32*)0x020197DC = 0xE1A00000; // nop
		*(u32*)0x0201B58C = 0xE1A00000; // nop
		*(u32*)0x0201B590 = 0xE1A00000; // nop
		*(u32*)0x0201B59C = 0xE1A00000; // nop
		*(u32*)0x0201B6FC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201B758, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02020014 = 0xE1A00000; // nop
		*(u32*)0x0202E888 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202E88C = 0xE12FFF1E; // bx lr
	}

	// Tori to Mame (Japan)
	// Does not boot: Crashes on black screens
	/*else if (strcmp(romTid, "KP6J") == 0) {
		*(u32*)0x020217B0 = 0xE12FFF1E; // bx lr (Skip NTFR file loading from TWLNAND)
	}*/

	// Touch Solitaire (USA)
	// Requires 8MB of RAM (but why?)
	else if (strcmp(romTid, "KSLE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		doubleNopT(0x0200D78A);
		doubleNopT(0x0200D90A);
		doubleNopT(0x0200D916);
		doubleNopT(0x0200DA26);
		doubleNopT(0x0200E15A);
		doubleNopT(0x0201AD4A);
		doubleNopT(0x0201D28A);
		doubleNopT(0x02020F94);
		doubleNopT(0x02022552);
		doubleNopT(0x02022556);
		doubleNopT(0x02022562);
		doubleNopT(0x02022646);
		patchHiHeapDSiWareThumb(0x02022684, 0x209C, 0x0480); // movs r0, #0x2700000
		*(u16*)0x020233DE = 0x46C0; // nop
		*(u16*)0x020233E2 = 0xBD38; // POP {R3-R5,PC}
		doubleNopT(0x020236CC);
		*(u16*)0x020236D0 = 0x46C0; // nop
		*(u16*)0x020236D2 = 0x46C0; // nop
		doubleNopT(0x020236D4);
		*(u16*)0x02025690 = 0x2001; // mov r0, #1
		*(u16*)0x02025692 = 0x4770; // bx lr
		*(u16*)0x02025698 = 0x2000; // mov r0, #0
		*(u16*)0x0202569A = 0x4770; // bx lr
		doubleNopT(0x02025756);
	}

	// Trajectile (USA)
	// Requires 8MB of RAM
	// Crashes after loading a stage
	/*else if (strcmp(romTid, "KDZE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020408B8 = 0xE1A00000; // nop
		*(u32*)0x020408BC = 0xE1A00000; // nop
		doubleNopT(0x02040D24);
		doubleNopT(0x020B4B40);
		doubleNopT(0x020B9216);
		for (int i = 0; i < 36; i++) {
			u16* offset = (u16*)0x020B9298;
			offset[i] = 0x46C0; // nop
		}
		*(u32*)0x020C560C = 0xE1A00000; // nop
		*(u32*)0x020C751C = 0xE1A00000; // nop
		*(u32*)0x020C75B4 = 0xE1A00000; // nop
		*(u32*)0x020C75B8 = 0xE1A00000; // nop
		*(u32*)0x020C75C4 = 0xE1A00000; // nop
		*(u32*)0x020C7708 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020C7764, 0x02700000); // mov r0, #0x2700000
		doubleNopT(0x020CF4BA);
		doubleNopT(0x020D515E);
	}*/

	// Wakugumi: Monochrome Puzzle (Europe, Australia)
	else if (strcmp(romTid, "KK4V") == 0) {
		*(u32*)0x02005A38 = 0xE1A00000; // nop
		*(u32*)0x0204F240 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02050114 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020668F8 = 0xE1A00000; // nop
		*(u32*)0x0206A538 = 0xE1A00000; // nop
		*(u32*)0x0206EA28 = 0xE1A00000; // nop
		*(u32*)0x02070804 = 0xE1A00000; // nop
		*(u32*)0x02070808 = 0xE1A00000; // nop
		*(u32*)0x02070814 = 0xE1A00000; // nop
		*(u32*)0x02070958 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020709B4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02071D84 = 0xE1A00000; // nop
		*(u32*)0x02071D8C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020754C0 = 0xE1A00000; // nop
	}

	// White-Water Domo (USA)
	else if (strcmp(romTid, "KDWE") == 0) {
		*(u16*)0x0200C918 = 0x2001; // movs r0, #1
		*(u16*)0x0200C91A = 0x4770; // bx lr
		*(u16*)0x02013B10 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x020223BE);
		doubleNopT(0x0202460E);
		doubleNopT(0x020272C4);
		doubleNopT(0x0202887E);
		doubleNopT(0x02028882);
		doubleNopT(0x0202888E);
		doubleNopT(0x02028972);
		patchHiHeapDSiWareThumb(0x020289B0, 0x208F, 0x0480); // movs r0, #0x23C0000
		doubleNopT(0x02029A36);
		*(u16*)0x02029A3A = 0x46C0;
		*(u16*)0x02029A3C = 0x46C0;
		doubleNopT(0x02029A3E);
		doubleNopT(0x0202BAF2);
	}

	// Art Style: ZENGAGE (USA)
	else if (strcmp(romTid, "KASE") == 0) {
		*(u32*)0x0200E000 = 0xE1A00000; // nop
		*(u32*)0x0200E080 = 0xE1A00000; // nop
		*(u32*)0x0200E1E8 = 0xE1A00000; // nop
		*(u32*)0x0200E290 = 0xE1A00000; // nop
		*(u32*)0x0200EBC8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200EBCC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201CAAC = 0xE1A00000; // nop
		*(u32*)0x0201CAB0 = 0xE1A00000; // nop
		*(u32*)0x0201CAC0 = 0xE1A00000; // nop
		*(u32*)0x0201CE74 = 0xE1A00000; // nop
		*(u32*)0x0201CE80 = 0xE1A00000; // nop
		*(u32*)0x0201CEA8 = 0xE1A00000; // nop
		*(u32*)0x0201D474 = 0xE1A00000; // nop
		*(u32*)0x0201D48C = 0xE1A00000; // nop
		*(u32*)0x0201D538 = 0xE1A00000; // nop
		*(u32*)0x02035228 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0203522C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02038820 = 0xE1A00000; // nop
		*(u32*)0x0203F0B8 = 0xE1A00000; // nop
		*(u32*)0x02040E7C = 0xE1A00000; // nop
		*(u32*)0x02040E80 = 0xE1A00000; // nop
		*(u32*)0x02040E8C = 0xE1A00000; // nop
		*(u32*)0x02040FEC = 0xE1A00000; // nop
		*(u32*)0x02040FF0 = 0xE1A00000; // nop
		*(u32*)0x02040FF4 = 0xE1A00000; // nop
		*(u32*)0x02040FF8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02041054, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02042388 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
	}

	// Art Style: NEMREM (Europe, Australia)
	else if (strcmp(romTid, "KASV") == 0) {
		*(u32*)0x0200E000 = 0xE1A00000; // nop
		*(u32*)0x0200E080 = 0xE1A00000; // nop
		*(u32*)0x0200E1E8 = 0xE1A00000; // nop
		*(u32*)0x0200E290 = 0xE1A00000; // nop
		*(u32*)0x0200EBC8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200EBCC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201C7A0 = 0xE1A00000; // nop
		*(u32*)0x0201C7A4 = 0xE1A00000; // nop
		*(u32*)0x0201C7B4 = 0xE1A00000; // nop
		*(u32*)0x0201CB5C = 0xE1A00000; // nop
		*(u32*)0x0201CB68 = 0xE1A00000; // nop
		*(u32*)0x0201CB90 = 0xE1A00000; // nop
		*(u32*)0x0201D164 = 0xE1A00000; // nop
		*(u32*)0x0201D17C = 0xE1A00000; // nop
		*(u32*)0x0201D228 = 0xE1A00000; // nop
		*(u32*)0x02034CBC = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02034CC0 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020382B4 = 0xE1A00000; // nop
		*(u32*)0x0203EB4C = 0xE1A00000; // nop
		*(u32*)0x02040910 = 0xE1A00000; // nop
		*(u32*)0x02040914 = 0xE1A00000; // nop
		*(u32*)0x02040920 = 0xE1A00000; // nop
		*(u32*)0x02040A80 = 0xE1A00000; // nop
		*(u32*)0x02040A84 = 0xE1A00000; // nop
		*(u32*)0x02040A88 = 0xE1A00000; // nop
		*(u32*)0x02040A8C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02040AE8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02041E1C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
	}
}

void patchBinary(cardengineArm9* ce9, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	if (ndsHeader->unitCode == 3) {
		patchDSiModeToDSMode(ce9, ndsHeader);
		return;
	}

	const char* romTid = getRomTid(ndsHeader);

	// Trauma Center: Under the Knife (USA)
	if (strcmp(romTid, "AKDE") == 0) {
		*(u32*)0x2007434 = 0;
	}

	// Trauma Center: Under the Knife (Europe)
	else if (strcmp(romTid, "AKDP") == 0) {
		*(u32*)0x20A6B90 = 0;
	}

	// Chou Shittou Caduceus (Japan)
	else if (strcmp(romTid, "AKDJ") == 0 && ndsHeader->romversion == 1) {
		*(u32*)0x20CCB18 = 0;
	}

	// Animal Crossing: Wild World
	else if ((strncmp(romTid, "ADM", 3) == 0 || strncmp(romTid, "A62", 3) == 0) && !extendedMemory2) {
		int instancesPatched = 0;
		u32 addrOffset = (u32)ndsHeader->arm9destination;
		while (instancesPatched < 3) {
			if(*(u32*)addrOffset >= 0x023FF000 && *(u32*)addrOffset < 0x023FF020) { 
				*(u32*)addrOffset -= 0x2000;
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
	
	// 0735 - Castlevania - Portrait of Ruin (USA)
	else if (strcmp(romTid, "ACBE") == 0) {
		*(u32*)0x02007910 = 0xeb02508e;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025052;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0676 - Akumajou Dracula - Gallery of Labyrinth (Japan)
	else if (strcmp(romTid, "ACBJ") == 0) {
		*(u32*)0x02007910 = 0xeb0250b0;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025074;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0881 - Castlevania - Portrait of Ruin (Europe) (En,Fr,De,Es,It)
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
	else if (strcmp(romTid, "UZPP") == 0) {
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
	}

	// WarioWare: DIY (USA)
	else if (strcmp(romTid, "UORE") == 0) {
		*(u32*)0x02003114 = 0xE12FFF1E; //bx lr
	}
	// WarioWare: Do It Yourself (Europe)
	else if (strcmp(romTid, "UORP") == 0) {
		*(u32*)0x020031B4 = 0xE12FFF1E; //bx lr
	}
	// Made in Ore (Japan)
	else if (strcmp(romTid, "UORJ") == 0) {
		*(u32*)0x020030F4 = 0xE12FFF1E; //bx lr
	}

    // Pokemon Dash (Japan)
	//else if (strcmp(romTid, "APDJ") == 0) {
		//*(u32*)0x0206AE70 = 0xE3A00000; //mov r0, #0
        //*(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		//*(u32*)0x0206AE74 = 0xe12fff1e; //bx lr
        
        //*(u32*)0x02000B94 = 0xE1A00000; //nop

		//*(u32*)0x020D5010 = 0xe12fff1e; //bx lr
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
        
        //*((u32*)0x02000BB0) = 0xE1A00000; //nop 
    
		//*(u32*)0x0206D2C4 = 0xE3A00000; //mov r0, #0
        //*(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		//*(u32*)0x0206D2C8 = 0xe12fff1e; //bx lr
        
		//*(u32*)0x020D5010 = 0xe12fff1e; //bx lr
	//}

    /* // Pokemon Dash (Kiosk Demo)
	else if (strcmp(romTid, "A24E") == 0) {
        *(u32*)0x02000BB0 = 0xE1A00000; //nop
	}

    // Pokemon Dash (Korea)
	else if (strcmp(romTid, "APDK") == 0) {
        *(u32*)0x02000C14 = 0xE1A00000; //nop
	}*/


    // Golden Sun
    /*else if (strcmp(romTid, "BO5E") == 0) {
        // patch "refresh" function
        *(u32*)0x204995C = 0xe12fff1e; //bx lr
        *(u32*)0x20499C4 = 0xe12fff1e; //bx lr
    }*/

	// Tropix! Your Island Getaway
    else if (strcmp(romTid, "CTXE") == 0) {
		extern u32 baseChipID;
		u32 cardIdFunc[2] = {0, 0};
		tonccpy(cardIdFunc, ce9->thumbPatches->card_id_arm9, 0x4);
		cardIdFunc[1] = baseChipID;

		const u16* branchCode1 = generateA7InstrThumb(0x020BA666, 0x020BA670);

		*(u16*)0x020BA666 = branchCode1[0];
		*(u16*)0x020BA668 = branchCode1[1];

		tonccpy((void*)0x020BA670, cardIdFunc, 0x8);

		const u16* branchCode2 = generateA7InstrThumb(0x020BA66A, 0x020BA6C0);

		*(u16*)0x020BA66A = branchCode2[0];
		*(u16*)0x020BA66C = branchCode2[1];

		/*tonccpy((void*)0x020BA728, ce9->thumbPatches->card_set_dma_arm9, 0x30);

		const u16* branchCode3 = generateA7InstrThumb(0x020BA70C, 0x020BA728);

		*(u16*)0x020BA70C = branchCode3[0];
		*(u16*)0x020BA70E = branchCode3[1];
		*(u16*)0x020BA710 = 0xBDF8;*/

		const u16* branchCode4 = generateA7InstrThumb(0x020BAAA2, 0x020BAAAC);

		*(u16*)0x020BAAA2 = branchCode4[0];
		*(u16*)0x020BAAA4 = branchCode4[1];

		tonccpy((void*)0x020BAAAC, cardIdFunc, 0x8);

		const u16* branchCode5 = generateA7InstrThumb(0x020BAAA6, 0x020BAAFC);

		*(u16*)0x020BAAA6 = branchCode5[0];
		*(u16*)0x020BAAA8 = branchCode5[1];

		const u16* branchCode6 = generateA7InstrThumb(0x020BAC5C, 0x020BAC64);

		*(u16*)0x020BAC5C = branchCode6[0];
		*(u16*)0x020BAC5E = branchCode6[1];

		tonccpy((void*)0x020BAC64, cardIdFunc, 0x8);

		const u16* branchCode7 = generateA7InstrThumb(0x020BAC60, 0x020BACB6);

		*(u16*)0x020BAC60 = branchCode7[0];
		*(u16*)0x020BAC62 = branchCode7[1];
	}

	// DSiWare containing Cloneboot

	// Art Style: BASE 10 (USA)
	else if (strcmp(romTid, "KADE") == 0) {
		*getOffsetFromBL((u32*)0x020074A8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074BC) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074D8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074E8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x02007500) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020075C4) = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D25C = 0xEB00007C; // bl 0x0202D454 (Skip manual screen)
		//*(u32*)0x0202D2EC = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0202D314 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203A288 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02056724 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02059F88 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064FC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020668F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020668FC = 0xE12FFF1E; // bx lr
		*(u32*)0x02066E50 = 0xE1A00000; // nop
		*(u32*)0x02066E54 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066E6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066FB4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067050 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067080 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067154 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067184 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0206865C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020686B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068A98 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02070068 = 0xE3A00000; // mov r0, #0
	}

	// Art Style: CODE (Europe, Australia)
	else if (strcmp(romTid, "KADV") == 0) {
		*getOffsetFromBL((u32*)0x020074A8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074BC) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074D8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074E8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x02007500) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020075C4) = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D288 = 0xEB00007C; // bl 0x0202D480 (Skip manual screen)
		*(u32*)0x0203A318 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020567B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205A018 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02065050 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066988 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206698C = 0xE12FFF1E; // bx lr
		*(u32*)0x02066EE0 = 0xE1A00000; // nop
		*(u32*)0x02066EE4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066EFC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067044 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020670E0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067110 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020671E4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067214 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020686EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068740 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068B28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020700F8 = 0xE3A00000; // mov r0, #0
	}

	// Art Style: DECODE (Japan)
	else if (strcmp(romTid, "KADJ") == 0) {
		*getOffsetFromBL((u32*)0x020074B0) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074C4) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074E0) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074F0) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x02007508) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020075C8) = 0xE12FFF1E; // bx lr
		*(u32*)0x0202E2AC = 0xEB000071; // bl 0x0202E478 (Skip manual screen)
		*(u32*)0x0203B148 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020575B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205AE14 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067784 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02067788 = 0xE12FFF1E; // bx lr
		*(u32*)0x02067CDC = 0xE1A00000; // nop
		*(u32*)0x02067CE0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067CF8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067E40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067EDC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067F0C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067FE0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068010 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02069480 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020694C0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02070E08 = 0xE3A00000; // mov r0, #0
	}
}

static bool rsetA7CacheDone = false;

void rsetA7Cache(void)
{
	if (rsetA7CacheDone) return;

	patchOffsetCache.a7BinSize = 0;
	patchOffsetCache.a7IsThumb = 0;
	patchOffsetCache.wramEndAddrOffset = 0;
	patchOffsetCache.wramClearOffset = 0;
	patchOffsetCache.ramClearOffset = 0;
	patchOffsetCache.ramClearChecked = 0;
	patchOffsetCache.sleepPatchOffset = 0;
	patchOffsetCache.postBootOffset = 0;
	patchOffsetCache.a7CardIrqEnableOffset = 0;
	patchOffsetCache.a7IrqHandlerOffset = 0;
	patchOffsetCache.savePatchType = 0;
	patchOffsetCache.relocateStartOffset = 0;
	patchOffsetCache.relocateValidateOffset = 0;
	patchOffsetCache.a7CardReadEndOffset = 0;
	patchOffsetCache.a7JumpTableFuncOffset = 0;
	patchOffsetCache.a7JumpTableType = 0;

	rsetA7CacheDone = true;
}

void rsetPatchCache(void)
{
	extern u32 srlAddr;

	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 1) {
		if (srlAddr == 0 && !esrbScreenPrepared) pleaseWaitOutput();
		u32* moduleParamsOffset = patchOffsetCache.moduleParamsOffset;
		toncset(&patchOffsetCache, 0, sizeof(patchOffsetCacheContents));
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.type = 1;	// 0 = Regular, 1 = B4DS, 2 = HB
		patchOffsetCache.moduleParamsOffset = moduleParamsOffset;
		rsetA7CacheDone = true;
	}
}

u32 patchCardNds(
	cardengineArm7* ce7,
	cardengineArm9* ce9,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 patchMpuRegion,
	u32 patchMpuSize,
	u32 saveFileCluster,
	u32 saveSize
) {
	dbg_printf("patchCardNds\n\n");

	bool sdk5 = isSdk5(moduleParams);
	if (sdk5) {
		dbg_printf("[SDK 5]\n\n");
	}

	u32 errorCodeArm9 = patchCardNdsArm9(ce9, ndsHeader, moduleParams, patchMpuRegion, patchMpuSize);
	
	//if (cardReadFound || ndsHeader->fatSize == 0) {
	if (errorCodeArm9 == ERR_NONE || ndsHeader->fatSize == 0) {
		return patchCardNdsArm7(ce7, ndsHeader, moduleParams, saveFileCluster);
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}
