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
	extern bool expansionPakFound;
	extern u32 donorFileTwlCluster;	// SDK5 (TWL)
	extern u32 fatTableAddr;
	const char* romTid = getRomTid(ndsHeader);
	extern void patchHiHeapDSiWare(u32 addr, u32 heapEnd);
	extern void patchHiHeapDSiWareThumbOld(u32 addr, u16 opCode1, u16 opCode2);
	extern void patchHiHeapDSiWareThumb(u32 addr, u32 newCodeAddr, u32 heapEnd);

	const u32 heapEnd = (fatTableAddr < 0x023C0000 || fatTableAddr >= CARDENGINE_ARM9_LOCATION_DLDI) ? CARDENGINE_ARM9_LOCATION_DLDI : fatTableAddr;

	if (donorFileTwlCluster == 0) {
		return;
	}

	const u32* dsiSaveGetResultCode = ce9->patches->dsiSaveGetResultCode;
	const u32* dsiSaveCreate = ce9->patches->dsiSaveCreate;
	const u32* dsiSaveDelete = ce9->patches->dsiSaveDelete;
	const u32* dsiSaveGetInfo = ce9->patches->dsiSaveGetInfo;
	const u32* dsiSaveSetLength = ce9->patches->dsiSaveSetLength;
	const u32* dsiSaveOpen = ce9->patches->dsiSaveOpen;
	const u32* dsiSaveOpenR = ce9->patches->dsiSaveOpenR;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveGetLength = ce9->patches->dsiSaveGetLength;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;
	const u32* dsiSaveWrite = ce9->patches->dsiSaveWrite;

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

	// 40-in-1: Explosive Megamix (USA)
	else if (strcmp(romTid, "K45E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200DFB8 = 0xE1A00000; // nop
		/* *(u32*)0x0200DFCC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200DFD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E2D0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E2D4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E408 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E40C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E54C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E550 = 0xE12FFF1E; // bx lr */
		//*(u32*)0x0200E010 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0200E0D4, (u32)dsiSaveCreate);
		setBL(0x0200E0E4, (u32)dsiSaveOpen);
		setBL(0x0200E11C, (u32)dsiSaveSetLength);
		setBL(0x0200E138, (u32)dsiSaveWrite);
		setBL(0x0200E14C, (u32)dsiSaveClose);
		*(u32*)0x0200E304 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E39C, (u32)dsiSaveOpen);
		setBL(0x0200E3D0, (u32)dsiSaveRead);
		setBL(0x0200E3E4, (u32)dsiSaveClose);
		*(u32*)0x0200E440 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E470 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E494, 0x0200E4D4);
		*(u32*)0x0200E4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E578 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020FD244 = 0xE1A00000; // nop
		tonccpy((u32*)0x020FDDC8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x021008F8 = 0xE1A00000; // nop
		*(u32*)0x02103BB8 = 0xE1A00000; // nop
		*(u32*)0x02105A3C = 0xE1A00000; // nop
		*(u32*)0x02105A40 = 0xE1A00000; // nop
		*(u32*)0x02105A4C = 0xE1A00000; // nop
		*(u32*)0x02105BAC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02105C08, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x021070C0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x021070DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x021070E0 = 0xE12FFF1E; // bx lr
		*(u32*)0x021070E8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x021070EC = 0xE12FFF1E; // bx lr
		*(u32*)0x021074E0 = 0xE1A00000; // nop
		*(u32*)0x021074E4 = 0xE1A00000; // nop
		*(u32*)0x021074E8 = 0xE1A00000; // nop
		*(u32*)0x021074EC = 0xE1A00000; // nop
		*(u32*)0x0210A528 = 0xE1A00000; // nop
		*(u32*)0x0210BEC0 = 0xE3A00003; // mov r0, #3
	}

	// 40-in-1: Explosive Megamix (Europe)
	else if (strcmp(romTid, "K45P") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200DF68 = 0xE1A00000; // nop
		/* *(u32*)0x0200DF7C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200DF80 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E280 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E284 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E3B8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E3BC = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E4FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E500 = 0xE12FFF1E; // bx lr */
		//*(u32*)0x0200DFC0 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0200E084, (u32)dsiSaveCreate);
		setBL(0x0200E094, (u32)dsiSaveOpen);
		setBL(0x0200E0CC, (u32)dsiSaveSetLength);
		setBL(0x0200E0E8, (u32)dsiSaveWrite);
		setBL(0x0200E0FC, (u32)dsiSaveClose);
		*(u32*)0x0200E2B4 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E34C, (u32)dsiSaveOpen);
		setBL(0x0200E380, (u32)dsiSaveRead);
		setBL(0x0200E394, (u32)dsiSaveClose);
		*(u32*)0x0200E3F0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E420 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E444, 0x0200E484);
		*(u32*)0x0200E4A8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E528 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020FC8B4 = 0xE1A00000; // nop
		tonccpy((u32*)0x020FD438, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020FFF68 = 0xE1A00000; // nop
		*(u32*)0x02103228 = 0xE1A00000; // nop
		*(u32*)0x02105A3C = 0xE1A00000; // nop
		*(u32*)0x02105A40 = 0xE1A00000; // nop
		*(u32*)0x02105A4C = 0xE1A00000; // nop
		*(u32*)0x0210521C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02105278, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02106730 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0210674C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02106750 = 0xE12FFF1E; // bx lr
		*(u32*)0x02106758 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0210675C = 0xE12FFF1E; // bx lr
		*(u32*)0x02106B50 = 0xE1A00000; // nop
		*(u32*)0x02106B54 = 0xE1A00000; // nop
		*(u32*)0x02106B58 = 0xE1A00000; // nop
		*(u32*)0x02106B5C = 0xE1A00000; // nop
		*(u32*)0x02109B98 = 0xE1A00000; // nop
		*(u32*)0x0210B530 = 0xE3A00003; // mov r0, #3
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
		*(u32*)0x02011E1C = 0xE1A00000; // nop
		*(u32*)0x02011E50 = 0xE1A00000; // nop
		*(u32*)0x02013E8C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02013EA8 = 0xE1A00000; // nop
		*(u32*)0x02013EDC = 0xE1A00000; // nop
		*(u32*)0x02013EE4 = 0xE1A00000; // nop
		*(u32*)0x02013F58 = 0xE1A00000; // nop
		*(u32*)0x02013F5C = 0xE1A00000; // nop
		*(u32*)0x02013F60 = 0xE1A00000; // nop
		*(u32*)0x02013FE4 = 0xE1A00000; // nop
		*(u32*)0x02013FE8 = 0xE1A00000; // nop
		*(u32*)0x02013FEC = 0xE1A00000; // nop
		setBL(0x02031FE8, (u32)dsiSaveOpen);
		setBL(0x02032000, (u32)dsiSaveGetLength);
		setBL(0x02032010, (u32)dsiSaveSeek);
		setBL(0x02032020, (u32)dsiSaveWrite);
		setBL(0x02032028, (u32)dsiSaveClose);
		setBL(0x02032098, (u32)dsiSaveOpen);
		setBL(0x020320B0, (u32)dsiSaveGetLength);
		setBL(0x020320C4, (u32)dsiSaveSeek);
		setBL(0x020320D4, (u32)dsiSaveRead);
		setBL(0x020320DC, (u32)dsiSaveClose);
		setBL(0x02032154, (u32)dsiSaveCreate);
		setBL(0x02032180, (u32)dsiSaveOpen);
		setBL(0x020321BC, (u32)dsiSaveWrite);
		setBL(0x020321CC, (u32)dsiSaveClose);
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
		*(u32*)0x02010EAC = 0xE1A00000; // nop
		*(u32*)0x02010EE0 = 0xE1A00000; // nop
		*(u32*)0x02012F1C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012F38 = 0xE1A00000; // nop
		*(u32*)0x02012F6C = 0xE1A00000; // nop
		*(u32*)0x02012F74 = 0xE1A00000; // nop
		*(u32*)0x02012FE8 = 0xE1A00000; // nop
		*(u32*)0x02012FEC = 0xE1A00000; // nop
		*(u32*)0x02012FF0 = 0xE1A00000; // nop
		*(u32*)0x02013074 = 0xE1A00000; // nop
		*(u32*)0x02013078 = 0xE1A00000; // nop
		*(u32*)0x0201307C = 0xE1A00000; // nop
		setBL(0x020310C8, (u32)dsiSaveOpen);
		setBL(0x020310E0, (u32)dsiSaveGetLength);
		setBL(0x020310F0, (u32)dsiSaveSeek);
		setBL(0x02031100, (u32)dsiSaveWrite);
		setBL(0x02031108, (u32)dsiSaveClose);
		setBL(0x02031178, (u32)dsiSaveOpen);
		setBL(0x02031090, (u32)dsiSaveGetLength);
		setBL(0x020311A4, (u32)dsiSaveSeek);
		setBL(0x020311B4, (u32)dsiSaveRead);
		setBL(0x020311BC, (u32)dsiSaveClose);
		setBL(0x02031234, (u32)dsiSaveCreate);
		setBL(0x02031260, (u32)dsiSaveOpen);
		setBL(0x0203129C, (u32)dsiSaveWrite);
		setBL(0x020312AC, (u32)dsiSaveClose); 
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
		*(u32*)0x02010F2C = 0xE1A00000; // nop
		*(u32*)0x02010F60 = 0xE1A00000; // nop
		setBL(0x02012E48, (u32)dsiSaveCreate);
		*(u32*)0x02012E5C = 0xE1A00000; // nop
		*(u32*)0x02012E68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012EF8 = 0xE1A00000; // nop
		*(u32*)0x02012EFC = 0xE1A00000; // nop
		*(u32*)0x02012F00 = 0xE1A00000; // nop
		setBL(0x02012F0C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012F28 = 0xE1A00000; // nop
		*(u32*)0x02012F34 = 0xE3A00001; // mov r0, #1
		setBL(0x02030FCC, (u32)dsiSaveOpen);
		setBL(0x02030FE4, (u32)dsiSaveGetLength);
		setBL(0x02030FF4, (u32)dsiSaveSeek);
		setBL(0x02031004, (u32)dsiSaveWrite);
		setBL(0x0203100C, (u32)dsiSaveClose);
		setBL(0x0203107C, (u32)dsiSaveOpen);
		setBL(0x02031094, (u32)dsiSaveGetLength);
		setBL(0x020310A8, (u32)dsiSaveSeek);
		setBL(0x020310B8, (u32)dsiSaveRead);
		setBL(0x020310C0, (u32)dsiSaveClose);
		setBL(0x020311C8, (u32)dsiSaveCreate);
		setBL(0x02031164, (u32)dsiSaveOpen);
		setBL(0x020311A0, (u32)dsiSaveWrite);
		setBL(0x020311B0, (u32)dsiSaveClose);
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
	// 99Moves (Europe)
	else if (strcmp(romTid, "K9WE") == 0 || strcmp(romTid, "K9WP") == 0) {
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
		*(u32*)0x02010CE8 = 0xE1A00000; // nop
		*(u32*)0x02010D1C = 0xE1A00000; // nop
		setBL(0x02012BD4, (u32)dsiSaveCreate);
		*(u32*)0x02012BE8 = 0xE1A00000; // nop
		*(u32*)0x02012BF4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012C84 = 0xE1A00000; // nop
		*(u32*)0x02012C88 = 0xE1A00000; // nop
		*(u32*)0x02012C8C = 0xE1A00000; // nop
		setBL(0x02012C98, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012CB4 = 0xE1A00000; // nop
		*(u32*)0x02012CC0 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x02031820, (u32)dsiSaveOpen);
			setBL(0x02031838, (u32)dsiSaveGetLength);
			setBL(0x02031848, (u32)dsiSaveSeek);
			setBL(0x02031858, (u32)dsiSaveWrite);
			setBL(0x02031860, (u32)dsiSaveClose);
			setBL(0x020318D0, (u32)dsiSaveOpen);
			setBL(0x020318E8, (u32)dsiSaveGetLength);
			setBL(0x020318FC, (u32)dsiSaveSeek);
			setBL(0x0203190C, (u32)dsiSaveRead);
			setBL(0x02031914, (u32)dsiSaveClose);
			setBL(0x0203198C, (u32)dsiSaveCreate);
			setBL(0x020319B8, (u32)dsiSaveOpen);
			setBL(0x020319F4, (u32)dsiSaveWrite);
			setBL(0x02031A04, (u32)dsiSaveClose);
			*(u32*)0x02061478 = 0xE1A00000; // nop
			*(u32*)0x02067D7C = 0xE1A00000; // nop
			*(u32*)0x02069C0C = 0xE1A00000; // nop
			*(u32*)0x02069C10 = 0xE1A00000; // nop
			*(u32*)0x02069C1C = 0xE1A00000; // nop
			*(u32*)0x02069D7C = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02069DD8, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x0206DCD8 = 0xE1A00000; // nop
			*(u32*)0x0207050C = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x02031870, (u32)dsiSaveOpen);
			setBL(0x02031888, (u32)dsiSaveGetLength);
			setBL(0x02031898, (u32)dsiSaveSeek);
			setBL(0x020318A8, (u32)dsiSaveWrite);
			setBL(0x020318B0, (u32)dsiSaveClose);
			setBL(0x02031920, (u32)dsiSaveOpen);
			setBL(0x02031938, (u32)dsiSaveGetLength);
			setBL(0x0203196C, (u32)dsiSaveSeek);
			setBL(0x0203195C, (u32)dsiSaveRead);
			setBL(0x02031964, (u32)dsiSaveClose);
			setBL(0x020319DC, (u32)dsiSaveCreate);
			setBL(0x02031A08, (u32)dsiSaveOpen);
			setBL(0x02031A44, (u32)dsiSaveWrite);
			setBL(0x02031A54, (u32)dsiSaveClose);
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
	}

	// 99Seconds (USA)
	// 99Seconds (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KXTE") == 0 || strcmp(romTid, "KXTP") == 0) && extendedMemory2) {
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
		*(u32*)0x02010CE4 = 0xE1A00000; // nop
		*(u32*)0x02010D18 = 0xE1A00000; // nop
		setBL(0x02011918, (u32)dsiSaveCreate);
		*(u32*)0x0201192C = 0xE1A00000; // nop
		*(u32*)0x02011938 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020119CC = 0xE1A00000; // nop
		*(u32*)0x020119D0 = 0xE1A00000; // nop
		*(u32*)0x020119D4 = 0xE1A00000; // nop
		setBL(0x020119E0, (u32)dsiSaveGetResultCode);
		*(u32*)0x020119FC = 0xE1A00000; // nop
		*(u32*)0x02011A08 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x020302D4, (u32)dsiSaveOpen);
			setBL(0x020302EC, (u32)dsiSaveGetLength);
			setBL(0x020302FC, (u32)dsiSaveSeek);
			setBL(0x0203030C, (u32)dsiSaveWrite);
			setBL(0x02030314, (u32)dsiSaveClose);
			setBL(0x02030384, (u32)dsiSaveOpen);
			setBL(0x0203039C, (u32)dsiSaveGetLength);
			setBL(0x020303B0, (u32)dsiSaveSeek);
			setBL(0x020303C0, (u32)dsiSaveRead);
			setBL(0x020303C8, (u32)dsiSaveClose);
			setBL(0x02030440, (u32)dsiSaveCreate);
			setBL(0x0203046C, (u32)dsiSaveOpen);
			setBL(0x020304A8, (u32)dsiSaveWrite);
			setBL(0x020304B8, (u32)dsiSaveClose);
			*(u32*)0x02061590 = 0xE1A00000; // nop
			*(u32*)0x02067EC0 = 0xE1A00000; // nop
			*(u32*)0x02069D50 = 0xE1A00000; // nop
			*(u32*)0x02069D54 = 0xE1A00000; // nop
			*(u32*)0x02069D60 = 0xE1A00000; // nop
			*(u32*)0x02069EC0 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02069F1C, 0x02700000); // mov r0, #0x2700000
			*(u32*)0x0206DE34 = 0xE1A00000; // nop
			*(u32*)0x02070668 = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x02030324, (u32)dsiSaveOpen);
			setBL(0x0203033C, (u32)dsiSaveGetLength);
			setBL(0x0203034C, (u32)dsiSaveSeek);
			setBL(0x0203035C, (u32)dsiSaveWrite);
			setBL(0x02030364, (u32)dsiSaveClose);
			setBL(0x020303D4, (u32)dsiSaveOpen);
			setBL(0x020303EC, (u32)dsiSaveGetLength);
			setBL(0x02030400, (u32)dsiSaveSeek);
			setBL(0x02030410, (u32)dsiSaveRead);
			setBL(0x02030418, (u32)dsiSaveClose);
			setBL(0x02030490, (u32)dsiSaveCreate);
			setBL(0x020304BC, (u32)dsiSaveOpen);
			setBL(0x020304F8, (u32)dsiSaveWrite);
			setBL(0x02030508, (u32)dsiSaveClose);
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
	}

	// Absolute Baseball (USA)
	// Audio doesn't play on retail consoles
	// Extra fixes required for it to work on real hardware
	/*else if (strcmp(romTid, "KE9E") == 0) {
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
		patchHiHeapDSiWare(0x0201A1A8, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201B42C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201E7D8 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0201D0FC = 0xE12FFF1E; // bx lr
			*(u32*)0x0201D124 = 0xE12FFF1E; // bx lr
			*(u32*)0x02038E5C = 0xE1A00000; // nop
		}
		*(u32*)0x0205FAD0 = 0xE1A00000; // nop
		*(u32*)0x02072554 = 0xE3A00001; // mov r0, #0
	}*/

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
		/* setBL(0x0201E3C0, (u32)dsiSaveCreate);
		setBL(0x0201E3E8, (u32)dsiSaveOpen);
		*(u32*)0x0201E41C = 0xE1A00000; // nop
		setBL(0x0201E428, (u32)dsiSaveCreate);
		setBL(0x0201E438, (u32)dsiSaveOpen);
		setBL(0x0201E468, (u32)dsiSaveOpen);
		setBL(0x0201E540, (u32)dsiSaveRead);
		setBL(0x0201E560, (u32)dsiSaveSeek);
		setBL(0x0201E578, (u32)dsiSaveWrite);
		setBL(0x0201E588, (u32)dsiSaveSeek); */
		*(u32*)0x0201FD04 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02029C68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02029D14 = 0xE12FFF1E; // bx lr
		/* setBL(0x02029CFC, (u32)dsiSaveClose);
		setBL(0x02029DB8, (u32)dsiSaveClose);
		setBL(0x02029DCC, (u32)dsiSaveClose); */
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
		*(u32*)0x0205C1B4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
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

	// Amakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF2J") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200D658 = 0xE1A00000; // nop
		*(u32*)0x02010BD8 = 0xE1A00000; // nop
		*(u32*)0x02015CC4 = 0xE1A00000; // nop
		*(u32*)0x02017A20 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02017AC4 = 0xE1A00000; // nop
		*(u32*)0x02017AC8 = 0xE1A00000; // nop
		*(u32*)0x02017AD4 = 0xE1A00000; // nop
		*(u32*)0x02017C18 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02017C74, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018ED4 = 0xE1A00000; // nop
		*(u32*)0x02018EDC = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0201BD08 = 0xE1A00000; // nop
		setBL(0x0203C1C8, (u32)dsiSaveOpen);
		setBL(0x0203C1F4, (u32)dsiSaveRead);
		setBL(0x0203C204, (u32)dsiSaveClose);
		setBL(0x0203C220, (u32)dsiSaveClose);
		*(u32*)0x0203C274 = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x0203C2B0 = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x0203C2BC, (u32)dsiSaveCreate);
		setBL(0x0203C2CC, (u32)dsiSaveOpen);
		setBL(0x0203C2F8, (u32)dsiSaveSetLength);
		setBL(0x0203C308, (u32)dsiSaveClose);
		setBL(0x0203C32C, (u32)dsiSaveWrite);
		setBL(0x0203C33C, (u32)dsiSaveClose);
		setBL(0x0203C358, (u32)dsiSaveClose);
	}

	// Anne's Doll Studio: Antique Collection (USA)
	// Anne's Doll Studio: Antique Collection (Europe)
	// Anne's Doll Studio: Princess Collection (USA)
	// Anne's Doll Studio: Princess Collection (Europe)
	else if (strcmp(romTid, "KY8E") == 0 || strcmp(romTid, "KY8P") == 0
		   || strcmp(romTid, "K2SE") == 0 || strcmp(romTid, "K2SP") == 0) {
		/*if (!extendedMemory2) {
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				u32* addr = (u32*)ndsHeader->arm9destination;
				if (addr[i] >= 0x022D0000 && addr[i] < 0x02360000) {
					addr[i] -= 0x200000;
				}
			}
		}*/

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x02011374 = 0xE1A00000; // nop
		*(u32*)0x020151A0 = 0xE1A00000; // nop
		*(u32*)0x0201A2A4 = 0xE1A00000; // nop
		*(u32*)0x0201C040 = 0xE1A00000; // nop
		*(u32*)0x0201C044 = 0xE1A00000; // nop
		*(u32*)0x0201C050 = 0xE1A00000; // nop
		*(u32*)0x0201C1B0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C20C, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		/*if (!extendedMemory2) {
			*(u32*)0x0201C340 -= 0x240000;
		}*/
		*(u32*)0x0201D654 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02020B1C = 0xE1A00000; // nop
		setBL(0x0202A164, (u32)dsiSaveGetResultCode);
		setBL(0x0202A288, (u32)dsiSaveOpen);
		setBL(0x0202A2BC, (u32)dsiSaveRead);
		setBL(0x0202A2E4, (u32)dsiSaveClose);
		setBL(0x0202A344, (u32)dsiSaveOpen);
		setBL(0x0202A38C, (u32)dsiSaveWrite);
		setBL(0x0202A3AC, (u32)dsiSaveClose);
		setBL(0x0202A3F0, (u32)dsiSaveCreate);
		setBL(0x0202A44C, (u32)dsiSaveDelete);
		*(u32*)0x0202C608 = 0xE1A00000; // nop
		if (strncmp(romTid, "KY8", 3) == 0) {
			*(u32*)0x0202F97C = 0xE1A00000; // nop
			*(u32*)0x0202F998 = 0xE1A00000; // nop
			*(u32*)0x02030018 = 0xE12FFF1E; // bx lr
			*(u32*)0x020310D0 = 0xE1A00000; // nop
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x0203B89C = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAFC = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BB00 = 0xE12FFF1E; // bx lr
			} else {
				*(u32*)0x0203B844 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAA4 = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BAA8 = 0xE12FFF1E; // bx lr
			}
		} else {
			*(u32*)0x0202F978 = 0xE1A00000; // nop
			*(u32*)0x0202F994 = 0xE1A00000; // nop
			*(u32*)0x02030014 = 0xE12FFF1E; // bx lr
			*(u32*)0x020310CC = 0xE1A00000; // nop
			*(u32*)0x0203B678 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x0203B8D8 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x0203B8DC = 0xE12FFF1E; // bx lr
		}
	}

	// Anne's Doll Studio: Gothic Collection (USA)
	else if (strcmp(romTid, "K54E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x02011270 = 0xE1A00000; // nop
		*(u32*)0x0201509C = 0xE1A00000; // nop
		*(u32*)0x0201A1A0 = 0xE1A00000; // nop
		*(u32*)0x0201BF3C = 0xE1A00000; // nop
		*(u32*)0x0201BF40 = 0xE1A00000; // nop
		*(u32*)0x0201BF4C = 0xE1A00000; // nop
		*(u32*)0x0201C0AC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C108, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201D550 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02020A18 = 0xE1A00000; // nop
		*(u32*)0x02033850 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x02033AB0 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x02033AB4 = 0xE12FFF1E; // bx lr
		setBL(0x02035614, (u32)dsiSaveGetResultCode);
		setBL(0x02035738, (u32)dsiSaveOpen);
		setBL(0x0203576C, (u32)dsiSaveRead);
		setBL(0x02035794, (u32)dsiSaveClose);
		setBL(0x020357F4, (u32)dsiSaveOpen);
		setBL(0x0203583C, (u32)dsiSaveWrite);
		setBL(0x0203585C, (u32)dsiSaveClose);
		setBL(0x020358A0, (u32)dsiSaveCreate);
		setBL(0x020358FC, (u32)dsiSaveDelete);
		*(u32*)0x02037AC4 = 0xE1A00000; // nop
		*(u32*)0x0203AE68 = 0xE1A00000; // nop
		*(u32*)0x0203AE84 = 0xE1A00000; // nop
		*(u32*)0x0203B69C = 0xE12FFF1E; // bx lr
		*(u32*)0x0203C7C0 = 0xE1A00000; // nop
	}

	// Anne's Doll Studio: Lolita Collection (USA)
	// Anne's Doll Studio: Lolita Collection (Europe)
	else if (strcmp(romTid, "KLQE") == 0 || strcmp(romTid, "KLQP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020112A0 = 0xE1A00000; // nop
		*(u32*)0x020150CC = 0xE1A00000; // nop
		*(u32*)0x0201A1D0 = 0xE1A00000; // nop
		*(u32*)0x0201BF6C = 0xE1A00000; // nop
		*(u32*)0x0201BF70 = 0xE1A00000; // nop
		*(u32*)0x0201BF7C = 0xE1A00000; // nop
		*(u32*)0x0201C0DC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C138, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201D580 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02020A48 = 0xE1A00000; // nop
		*(u32*)0x020337B0 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x02033A10 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x02033A14 = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x020355C4, (u32)dsiSaveGetResultCode);
			setBL(0x020356E8, (u32)dsiSaveOpen);
			setBL(0x0203571C, (u32)dsiSaveRead);
			setBL(0x02035744, (u32)dsiSaveClose);
			setBL(0x020357A4, (u32)dsiSaveOpen);
			setBL(0x020357EC, (u32)dsiSaveWrite);
			setBL(0x0203580C, (u32)dsiSaveClose);
			setBL(0x02035850, (u32)dsiSaveCreate);
			setBL(0x020358AC, (u32)dsiSaveDelete);
			*(u32*)0x02037A74 = 0xE1A00000; // nop
			*(u32*)0x0203AE0C = 0xE1A00000; // nop
			*(u32*)0x0203AE28 = 0xE1A00000; // nop
			*(u32*)0x0203B640 = 0xE12FFF1E; // bx lr
			*(u32*)0x0203C6F4 = 0xE1A00000; // nop
		} else {
			setBL(0x02035570, (u32)dsiSaveGetResultCode);
			setBL(0x02035694, (u32)dsiSaveOpen);
			setBL(0x020356C8, (u32)dsiSaveRead);
			setBL(0x020356F0, (u32)dsiSaveClose);
			setBL(0x02035750, (u32)dsiSaveOpen);
			setBL(0x020357EC, (u32)dsiSaveWrite);
			setBL(0x02035798, (u32)dsiSaveClose);
			setBL(0x020357FC, (u32)dsiSaveCreate);
			setBL(0x02035858, (u32)dsiSaveDelete);
			*(u32*)0x02037A20 = 0xE1A00000; // nop
			*(u32*)0x0203ADB8 = 0xE1A00000; // nop
			*(u32*)0x0203ADD4 = 0xE1A00000; // nop
			*(u32*)0x0203B5EC = 0xE12FFF1E; // bx lr
			*(u32*)0x0203C6A0 = 0xE1A00000; // nop
		}
	}

	// Anne's Doll Studio: Tokyo Collection (USA)
	else if (strcmp(romTid, "KSQE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x02011344 = 0xE1A00000; // nop
		*(u32*)0x02015170 = 0xE1A00000; // nop
		*(u32*)0x0201A274 = 0xE1A00000; // nop
		*(u32*)0x0201C010 = 0xE1A00000; // nop
		*(u32*)0x0201C014 = 0xE1A00000; // nop
		*(u32*)0x0201C020 = 0xE1A00000; // nop
		*(u32*)0x0201C180 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C1DC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201D624 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02020AEC = 0xE1A00000; // nop
		setBL(0x02027F34, (u32)dsiSaveGetResultCode);
		setBL(0x02028058, (u32)dsiSaveOpen);
		setBL(0x0202808C, (u32)dsiSaveRead);
		setBL(0x020280B4, (u32)dsiSaveClose);
		setBL(0x02028114, (u32)dsiSaveOpen);
		setBL(0x0202815C, (u32)dsiSaveWrite);
		setBL(0x0202817C, (u32)dsiSaveClose);
		setBL(0x020281C0, (u32)dsiSaveCreate);
		setBL(0x0202821C, (u32)dsiSaveDelete);
		*(u32*)0x0202A3E4 = 0xE1A00000; // nop
		*(u32*)0x0202D760 = 0xE1A00000; // nop
		*(u32*)0x0202D77C = 0xE1A00000; // nop
		*(u32*)0x0202DF84 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202F05C = 0xE1A00000; // nop
		*(u32*)0x0203A534 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x0203A794 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x0203A798 = 0xE12FFF1E; // bx lr
	}

	// Anonymous Notes 1: From The Abyss (USA & Europe)
	// Anonymous Notes 2: From The Abyss (USA & Europe)
	else if ((strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0)
	  && ndsHeader->gameCode[3] != 'J') {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200E74C = 0xE1A00000; // nop
		*(u32*)0x02011C48 = 0xE1A00000; // nop
		*(u32*)0x02016A5C = 0xE1A00000; // nop
		*(u32*)0x020188E0 = 0xE1A00000; // nop
		*(u32*)0x020188E4 = 0xE1A00000; // nop
		*(u32*)0x020188F0 = 0xE1A00000; // nop
		*(u32*)0x02018A50 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02018AAC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02019F18 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201D5C4 = 0xE1A00000; // nop
		setBL(0x02024220, (u32)dsiSaveOpen);
		setBL(0x02024258, (u32)dsiSaveCreate);
		setBL(0x02024268, (u32)dsiSaveGetResultCode);
		setBL(0x02024290, (u32)dsiSaveOpen);
		setBL(0x020242BC, (u32)dsiSaveGetLength);
		setBL(0x020242D8, (u32)dsiSaveSetLength);
		setBL(0x02024328, (u32)dsiSaveWrite);
		setBL(0x02024338, (u32)dsiSaveClose);
		setBL(0x02024358, (u32)dsiSaveClose);
		setBL(0x020243A0, (u32)dsiSaveOpen);
		setBL(0x020243D8, (u32)dsiSaveSeek);
		setBL(0x020243E8, (u32)dsiSaveClose);
		setBL(0x02024400, (u32)dsiSaveWrite);
		setBL(0x02024410, (u32)dsiSaveClose);
		setBL(0x02024420, (u32)dsiSaveClose);
		setBL(0x02024460, (u32)dsiSaveOpen);
		setBL(0x0202448C, (u32)dsiSaveGetLength);
		setBL(0x020244AC, (u32)dsiSaveSeek);
		setBL(0x020244BC, (u32)dsiSaveClose);
		setBL(0x020244D4, (u32)dsiSaveRead);
		setBL(0x020244E4, (u32)dsiSaveClose);
		setBL(0x020244F4, (u32)dsiSaveClose);
		*(u32*)0x02024510 = 0xE1A00000; // nop
		*(u32*)0x02024530 = 0xE1A00000; // nop
		*(u32*)0x0202457C = 0xE1A00000; // nop
		if (ndsHeader->gameCode[2] == 'I') {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x0209E2CC = 0xE1A00000; // nop
				*(u32*)0x0209E2E0 = 0xE1A00000; // nop
				*(u32*)0x0209E2F4 = 0xE1A00000; // nop
				*(u32*)0x020CFB78 = 0xE1A00000; // nop
				*(u32*)0x020CFCD0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				*(u32*)0x0209E0DC = 0xE1A00000; // nop
				*(u32*)0x0209E0F0 = 0xE1A00000; // nop
				*(u32*)0x0209E104 = 0xE1A00000; // nop
				*(u32*)0x020CF988 = 0xE1A00000; // nop
				*(u32*)0x020CFAE0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		} else if (ndsHeader->gameCode[2] == '2') {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x0209E300 = 0xE1A00000; // nop
				*(u32*)0x0209E314 = 0xE1A00000; // nop
				*(u32*)0x0209E328 = 0xE1A00000; // nop
				*(u32*)0x020D071C = 0xE1A00000; // nop
				*(u32*)0x020D0874 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				*(u32*)0x0209E0EC = 0xE1A00000; // nop
				*(u32*)0x0209E100 = 0xE1A00000; // nop
				*(u32*)0x0209E114 = 0xE1A00000; // nop
				*(u32*)0x020D0508 = 0xE1A00000; // nop
				*(u32*)0x020D0660 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		}
	}

	// Anonymous Notes 1: From The Abyss (Japan)
	// Anonymous Notes 2: From The Abyss (Japan)
	else if (strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200506C = 0xE1A00000; // nop
		*(u32*)0x0200E6DC = 0xE1A00000; // nop
		*(u32*)0x02011C9C = 0xE1A00000; // nop
		*(u32*)0x02016D70 = 0xE1A00000; // nop
		*(u32*)0x02018C48 = 0xE1A00000; // nop
		*(u32*)0x02018C4C = 0xE1A00000; // nop
		*(u32*)0x02018C58 = 0xE1A00000; // nop
		*(u32*)0x02018D9C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02018DF8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201A278 = 0xE1A00000; // nop
		*(u32*)0x0201A280 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0201DB28 = 0xE1A00000; // nop
		setBL(0x0202481C, (u32)dsiSaveOpen);
		setBL(0x02024854, (u32)dsiSaveCreate);
		setBL(0x02024864, (u32)dsiSaveGetResultCode);
		setBL(0x0202488C, (u32)dsiSaveOpen);
		setBL(0x020248B8, (u32)dsiSaveGetLength);
		setBL(0x020248D4, (u32)dsiSaveSetLength);
		setBL(0x02024924, (u32)dsiSaveWrite);
		setBL(0x02024934, (u32)dsiSaveClose);
		setBL(0x02024954, (u32)dsiSaveClose);
		setBL(0x0202499C, (u32)dsiSaveOpen);
		setBL(0x020249D4, (u32)dsiSaveSeek);
		setBL(0x020249E4, (u32)dsiSaveClose);
		setBL(0x020249FC, (u32)dsiSaveWrite);
		setBL(0x02024A0C, (u32)dsiSaveClose);
		setBL(0x02024A1C, (u32)dsiSaveClose);
		setBL(0x02024A5C, (u32)dsiSaveOpen);
		setBL(0x02024A88, (u32)dsiSaveGetLength);
		setBL(0x02024AA8, (u32)dsiSaveSeek);
		setBL(0x02024AB8, (u32)dsiSaveClose);
		setBL(0x02024AD0, (u32)dsiSaveRead);
		setBL(0x02024AE0, (u32)dsiSaveClose);
		setBL(0x02024AF0, (u32)dsiSaveClose);
		*(u32*)0x02024B0C = 0xE1A00000; // nop
		*(u32*)0x02024B2C = 0xE1A00000; // nop
		*(u32*)0x02024B78 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[2] == 'I') {
			*(u32*)0x0209E0A4 = 0xE1A00000; // nop
			*(u32*)0x0209E0B8 = 0xE1A00000; // nop
			*(u32*)0x0209E0CC = 0xE1A00000; // nop
			*(u32*)0x020CF970 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		} else if (ndsHeader->gameCode[2] == '2') {
			*(u32*)0x0209E0D0 = 0xE1A00000; // nop
			*(u32*)0x0209E0E4 = 0xE1A00000; // nop
			*(u32*)0x0209E0F8 = 0xE1A00000; // nop
			*(u32*)0x020D050C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
	}

	// Anonymous Notes 3: From The Abyss (USA & Japan)
	// Anonymous Notes 4: From The Abyss (USA & Japan)
	else if (strncmp(romTid, "KV3", 3) == 0 || strncmp(romTid, "KV4", 3) == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005084 = 0xE1A00000; // nop
		*(u32*)0x0200E778 = 0xE1A00000; // nop
		*(u32*)0x02011C74 = 0xE1A00000; // nop
		*(u32*)0x02016A88 = 0xE1A00000; // nop
		*(u32*)0x0201890C = 0xE1A00000; // nop
		*(u32*)0x02018910 = 0xE1A00000; // nop
		*(u32*)0x0201891C = 0xE1A00000; // nop
		*(u32*)0x02018A7C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02018AD8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02019F44 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02019F60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02019F64 = 0xE12FFF1E; // bx lr
		*(u32*)0x02019F6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02019F70 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201D5F0 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[2] == '3') {
			if (ndsHeader->gameCode[3] == 'E') {
				setBL(0x0202424C, (u32)dsiSaveOpen);
				setBL(0x02024284, (u32)dsiSaveCreate);
				setBL(0x02024294, (u32)dsiSaveGetResultCode);
				setBL(0x020242BC, (u32)dsiSaveOpen);
				setBL(0x020242E8, (u32)dsiSaveGetLength);
				setBL(0x02024304, (u32)dsiSaveSetLength);
				setBL(0x02024354, (u32)dsiSaveWrite);
				setBL(0x02024364, (u32)dsiSaveClose);
				setBL(0x02024384, (u32)dsiSaveClose);
				setBL(0x020243CC, (u32)dsiSaveOpen);
				setBL(0x02024404, (u32)dsiSaveSeek);
				setBL(0x02024414, (u32)dsiSaveClose);
				setBL(0x0202442C, (u32)dsiSaveWrite);
				setBL(0x0202443C, (u32)dsiSaveClose);
				setBL(0x0202444C, (u32)dsiSaveClose);
				setBL(0x0202448C, (u32)dsiSaveOpen);
				setBL(0x020244B8, (u32)dsiSaveGetLength);
				setBL(0x020244D8, (u32)dsiSaveSeek);
				setBL(0x020244E8, (u32)dsiSaveClose);
				setBL(0x02024500, (u32)dsiSaveRead);
				setBL(0x02024510, (u32)dsiSaveClose);
				setBL(0x02024520, (u32)dsiSaveClose);
				*(u32*)0x0202453C = 0xE1A00000; // nop
				*(u32*)0x0202455C = 0xE1A00000; // nop
				*(u32*)0x020245A8 = 0xE1A00000; // nop
				*(u32*)0x0209E250 = 0xE1A00000; // nop
				*(u32*)0x0209E264 = 0xE1A00000; // nop
				*(u32*)0x0209E278 = 0xE1A00000; // nop
				*(u32*)0x020D0120 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				setBL(0x02024270, (u32)dsiSaveOpen);
				setBL(0x020242A8, (u32)dsiSaveCreate);
				setBL(0x020242B8, (u32)dsiSaveGetResultCode);
				setBL(0x020242E0, (u32)dsiSaveOpen);
				setBL(0x0202430C, (u32)dsiSaveGetLength);
				setBL(0x02024328, (u32)dsiSaveSetLength);
				setBL(0x02024378, (u32)dsiSaveWrite);
				setBL(0x02024388, (u32)dsiSaveClose);
				setBL(0x020243A8, (u32)dsiSaveClose);
				setBL(0x020243F0, (u32)dsiSaveOpen);
				setBL(0x02024428, (u32)dsiSaveSeek);
				setBL(0x02024438, (u32)dsiSaveClose);
				setBL(0x02024450, (u32)dsiSaveWrite);
				setBL(0x02024460, (u32)dsiSaveClose);
				setBL(0x02024470, (u32)dsiSaveClose);
				setBL(0x020244B0, (u32)dsiSaveOpen);
				setBL(0x020244DC, (u32)dsiSaveGetLength);
				setBL(0x020244FC, (u32)dsiSaveSeek);
				setBL(0x0202450C, (u32)dsiSaveClose);
				setBL(0x02024524, (u32)dsiSaveRead);
				setBL(0x02024534, (u32)dsiSaveClose);
				setBL(0x02024544, (u32)dsiSaveClose);
				*(u32*)0x02024560 = 0xE1A00000; // nop
				*(u32*)0x02024580 = 0xE1A00000; // nop
				*(u32*)0x020245CC = 0xE1A00000; // nop
				*(u32*)0x0209DB18 = 0xE1A00000; // nop
				*(u32*)0x0209DB2C = 0xE1A00000; // nop
				*(u32*)0x0209DB40 = 0xE1A00000; // nop
				*(u32*)0x020CF8AC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		} else if (ndsHeader->gameCode[2] == '4') {
			if (ndsHeader->gameCode[3] == 'E') {
				setBL(0x0202425C, (u32)dsiSaveOpen);
				setBL(0x02024294, (u32)dsiSaveCreate);
				setBL(0x020242A4, (u32)dsiSaveGetResultCode);
				setBL(0x020242CC, (u32)dsiSaveOpen);
				setBL(0x020242F8, (u32)dsiSaveGetLength);
				setBL(0x02024314, (u32)dsiSaveSetLength);
				setBL(0x02024364, (u32)dsiSaveWrite);
				setBL(0x02024374, (u32)dsiSaveClose);
				setBL(0x02024394, (u32)dsiSaveClose);
				setBL(0x020243DC, (u32)dsiSaveOpen);
				setBL(0x02024414, (u32)dsiSaveSeek);
				setBL(0x02024424, (u32)dsiSaveClose);
				setBL(0x0202443C, (u32)dsiSaveWrite);
				setBL(0x0202444C, (u32)dsiSaveClose);
				setBL(0x0202445C, (u32)dsiSaveClose);
				setBL(0x0202449C, (u32)dsiSaveOpen);
				setBL(0x020244C8, (u32)dsiSaveGetLength);
				setBL(0x020244E8, (u32)dsiSaveSeek);
				setBL(0x020244F8, (u32)dsiSaveClose);
				setBL(0x02024510, (u32)dsiSaveRead);
				setBL(0x02024520, (u32)dsiSaveClose);
				setBL(0x02024530, (u32)dsiSaveClose);
				*(u32*)0x0202454C = 0xE1A00000; // nop
				*(u32*)0x0202456C = 0xE1A00000; // nop
				*(u32*)0x020245B8 = 0xE1A00000; // nop
				*(u32*)0x0209F72C = 0xE1A00000; // nop
				*(u32*)0x0209F740 = 0xE1A00000; // nop
				*(u32*)0x0209F754 = 0xE1A00000; // nop
				*(u32*)0x020D0FFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				setBL(0x02024280, (u32)dsiSaveOpen);
				setBL(0x020242B8, (u32)dsiSaveCreate);
				setBL(0x020242C8, (u32)dsiSaveGetResultCode);
				setBL(0x020242F0, (u32)dsiSaveOpen);
				setBL(0x0202431C, (u32)dsiSaveGetLength);
				setBL(0x02024338, (u32)dsiSaveSetLength);
				setBL(0x02024388, (u32)dsiSaveWrite);
				setBL(0x02024398, (u32)dsiSaveClose);
				setBL(0x020243B8, (u32)dsiSaveClose);
				setBL(0x02024400, (u32)dsiSaveOpen);
				setBL(0x02024438, (u32)dsiSaveSeek);
				setBL(0x02024448, (u32)dsiSaveClose);
				setBL(0x02024460, (u32)dsiSaveWrite);
				setBL(0x02024470, (u32)dsiSaveClose);
				setBL(0x02024480, (u32)dsiSaveClose);
				setBL(0x020244C0, (u32)dsiSaveOpen);
				setBL(0x020244EC, (u32)dsiSaveGetLength);
				setBL(0x0202450C, (u32)dsiSaveSeek);
				setBL(0x0202451C, (u32)dsiSaveClose);
				setBL(0x02024524, (u32)dsiSaveRead);
				setBL(0x02024534, (u32)dsiSaveClose);
				setBL(0x02024544, (u32)dsiSaveClose);
				*(u32*)0x02024570 = 0xE1A00000; // nop
				*(u32*)0x02024590 = 0xE1A00000; // nop
				*(u32*)0x020245DC = 0xE1A00000; // nop
				*(u32*)0x0209EDEC = 0xE1A00000; // nop
				*(u32*)0x0209EE00 = 0xE1A00000; // nop
				*(u32*)0x0209EE14 = 0xE1A00000; // nop
				*(u32*)0x020D0590 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		}
	}

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

	// Art Style: AQUIA (USA)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAE") == 0) {
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		//*(u32*)0x0203BB4C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203BB50 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0203BC18 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203BC1C = 0xE12FFF1E; // bx lr
		setBL(0x0203BBE4, (u32)dsiSaveOpen);
		setBL(0x0203BC08, (u32)dsiSaveClose);
		*(u32*)0x0203BC2C = 0xE1A00000; // nop
		*(u32*)0x0203BC4C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203BC70 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203BC90, (u32)dsiSaveCreate);
		setBL(0x0203BCA0, (u32)dsiSaveOpen);
		setBL(0x0203BCC8, (u32)dsiSaveWrite);
		setBL(0x0203BCE0, (u32)dsiSaveClose);
		setBL(0x0203BD2C, (u32)dsiSaveOpen);
		setBL(0x0203BD54, (u32)dsiSaveRead);
		setBL(0x0203BD80, (u32)dsiSaveClose);
		setBL(0x0203BE70, (u32)dsiSaveOpen);
		setBL(0x0203BE98, (u32)dsiSaveWrite);
		setBL(0x0203BEB4, (u32)dsiSaveClose);
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
		//*(u32*)0x0203BC5C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203BC60 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0203BD28 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203BD2C = 0xE12FFF1E; // bx lr
		setBL(0x0203BCF4, (u32)dsiSaveOpen);
		setBL(0x0203BD18, (u32)dsiSaveClose);
		*(u32*)0x0203BD3C = 0xE1A00000; // nop
		*(u32*)0x0203BD5C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203BD80 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203BDA0, (u32)dsiSaveCreate);
		setBL(0x0203BDB0, (u32)dsiSaveOpen);
		setBL(0x0203BDC8, (u32)dsiSaveWrite);
		setBL(0x0203BDF0, (u32)dsiSaveClose);
		setBL(0x0203BE3C, (u32)dsiSaveOpen);
		setBL(0x0203BE64, (u32)dsiSaveRead);
		setBL(0x0203BE90, (u32)dsiSaveClose);
		setBL(0x0203BF80, (u32)dsiSaveOpen);
		setBL(0x0203BFA8, (u32)dsiSaveWrite);
		setBL(0x0203BFC4, (u32)dsiSaveClose);
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
		//*(u32*)0x0203E250 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203E254 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0203E324 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203E328 = 0xE12FFF1E; // bx lr
		setBL(0x0203E2F0, (u32)dsiSaveOpen);
		setBL(0x0203E314, (u32)dsiSaveClose);
		*(u32*)0x0203E334 = 0xE1A00000; // nop
		*(u32*)0x0203E34C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203E370 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203E38C, (u32)dsiSaveCreate);
		setBL(0x0203E39C, (u32)dsiSaveOpen);
		setBL(0x0203E3C8, (u32)dsiSaveWrite);
		setBL(0x0203E3E4, (u32)dsiSaveClose);
		setBL(0x0203E42C, (u32)dsiSaveOpen);
		setBL(0x0203E458, (u32)dsiSaveRead);
		setBL(0x0203E488, (u32)dsiSaveClose);
		setBL(0x0203E574, (u32)dsiSaveOpen);
		setBL(0x0203E5A0, (u32)dsiSaveWrite);
		setBL(0x0203E5BC, (u32)dsiSaveClose);
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
		//*(u32*)0x0200DC9C = 0xE1A00000; // nop
		*(u32*)0x02059E0C = 0xE1A00000; // nop
		*(u32*)0x02059E14 = 0xE1A00000; // nop
		setBL(0x02059E20, (u32)dsiSaveCreate);
		setBL(0x02059E3C, (u32)dsiSaveOpen);
		setBL(0x02059E68, (u32)dsiSaveSetLength);
		setBL(0x02059E84, (u32)dsiSaveWrite);
		setBL(0x02059E90, (u32)dsiSaveClose);
		*(u32*)0x02059EB0 = 0xE1A00000; // nop
		*(u32*)0x02059EB8 = 0xE1A00000; // nop
		setBL(0x02059F2C, (u32)dsiSaveOpen);
		setBL(0x02059F9C, (u32)dsiSaveRead);
		setBL(0x02059FA8, (u32)dsiSaveClose);
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
		//*(u32*)0x0200DD70 = 0xE1A00000; // nop
		*(u32*)0x02059EE0 = 0xE1A00000; // nop
		*(u32*)0x02059EE8 = 0xE1A00000; // nop
		setBL(0x02059EF4, (u32)dsiSaveCreate);
		setBL(0x02059F10, (u32)dsiSaveOpen);
		setBL(0x02059F3C, (u32)dsiSaveSetLength);
		setBL(0x02059F58, (u32)dsiSaveWrite);
		setBL(0x02059F64, (u32)dsiSaveClose);
		*(u32*)0x02059F84 = 0xE1A00000; // nop
		*(u32*)0x02059F8C = 0xE1A00000; // nop
		setBL(0x0205A000, (u32)dsiSaveOpen);
		setBL(0x0205A070, (u32)dsiSaveRead);
		setBL(0x0205A07C, (u32)dsiSaveClose);
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
		//*(u32*)0x0200DD70 = 0xE1A00000; // nop
		*(u32*)0x02059DF0 = 0xE1A00000; // nop
		*(u32*)0x02059DF8 = 0xE1A00000; // nop
		setBL(0x02059E04, (u32)dsiSaveCreate);
		setBL(0x02059E20, (u32)dsiSaveOpen);
		setBL(0x02059E4C, (u32)dsiSaveSetLength);
		setBL(0x02059E68, (u32)dsiSaveWrite);
		setBL(0x02059E74, (u32)dsiSaveClose);
		*(u32*)0x02059E88 = 0xE1A00000; // nop
		*(u32*)0x02059E90 = 0xE1A00000; // nop
		setBL(0x02059F04, (u32)dsiSaveOpen);
		setBL(0x02059F74, (u32)dsiSaveRead);
		setBL(0x02059F80, (u32)dsiSaveClose);
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

	// Army Defender (USA)
	// Army Defender (Europe)
	else if (strncmp(romTid, "KAY", 3) == 0) {
		*(u32*)0x0200523C = 0xE1A00000; // nop
		setBL(0x02020A28, (u32)dsiSaveCreate);
		setBL(0x02020A38, (u32)dsiSaveOpen);
		setBL(0x02020A8C, (u32)dsiSaveWrite);
		setBL(0x02020A94, (u32)dsiSaveClose);
		setBL(0x02020ADC, (u32)dsiSaveOpen);
		setBL(0x02020B08, (u32)dsiSaveGetLength);
		setBL(0x02020B18, (u32)dsiSaveRead);
		setBL(0x02020B20, (u32)dsiSaveClose);
		*(u32*)0x020426BC = 0xE1A00000; // nop
		tonccpy((u32*)0x02043360, dsiSaveGetResultCode, 0xC);
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
		*(u32*)0x0205CF4C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
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
		*(u32*)0x0204FDE4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02053548 = 0xE1A00000; // nop
	}

	// Aura-Aura Climber (USA)
	// Save code too advanced to patch, preventing support
	else if (strcmp(romTid, "KSRE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005164 = 0xE1A00000; // nop
		*(u32*)0x020104A0 = 0xE1A00000; // nop
		*(u32*)0x02010508 = 0xE1A00000; // nop
		*(u32*)0x02026760 = 0xE12FFF1E; // bx lr
		/* *(u32*)0x02026788 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x020267D4, (u32)dsiSaveOpen);
		setBL(0x020267E8, (u32)dsiSaveCreate);
		setBL(0x02026814, (u32)dsiSaveOpen);
		//*(u32*)0x02026834 = 0xE3A01B0B; // mov r1, #0x2C00
		setBL(0x0202683C, (u32)dsiSaveSetLength);
		setBL(0x0202684C, (u32)dsiSaveClose);
		setBL(0x02026870, (u32)dsiSaveWrite);
		setBL(0x0202687C, (u32)dsiSaveGetLength);
		*(u32*)0x02026880 = 0xE1A02000; // mov r2, r0
		*(u32*)0x02026884 = 0xE3A01000; // mov r1, #0
		//*(u32*)0x02026888 = 0xE3A03000; // mov r3, #0
		setBL(0x020268B8, (u32)dsiSaveSeek);
		setBL(0x020268D4, (u32)dsiSaveRead);
		setBL(0x02026BDC, (u32)dsiSaveSeek);
		setBL(0x02026C00, (u32)dsiSaveRead);
		setBL(0x02026CC0, (u32)dsiSaveSeek);
		setBL(0x02026CDC, (u32)dsiSaveRead);
		setBL(0x02026F6C, (u32)dsiSaveSeek);
		setBL(0x02026F84, (u32)dsiSaveWrite);
		setBL(0x020271E4, (u32)dsiSaveSeek);
		setBL(0x020271FC, (u32)dsiSaveWrite);
		setBL(0x0202723C, (u32)dsiSaveSeek);
		setBL(0x02027258, (u32)dsiSaveWrite);
		setBL(0x020273AC, (u32)dsiSaveSeek);
		setBL(0x020273C4, (u32)dsiSaveRead);
		setBL(0x020275A4, (u32)dsiSaveSeek);
		setBL(0x020275BC, (u32)dsiSaveWrite); */
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

	// Beauty Academy (Europe)
	else if (strcmp(romTid, "K8BP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02018B48 = 0xE1A00000; // nop
		*(u32*)0x0201C8C4 = 0xE1A00000; // nop
		*(u32*)0x0202187C = 0xE1A00000; // nop
		*(u32*)0x020236F0 = 0xE1A00000; // nop
		*(u32*)0x020236F4 = 0xE1A00000; // nop
		*(u32*)0x02023700 = 0xE1A00000; // nop
		*(u32*)0x02023860 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020238BC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02024DC0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02027C00 = 0xE1A00000; // nop
		setBL(0x02092D44, (u32)dsiSaveCreate);
		setBL(0x02092D54, (u32)dsiSaveOpen);
		setBL(0x02092D64, (u32)dsiSaveGetResultCode);
		setBL(0x02092DA0, (u32)dsiSaveSetLength);
		setBL(0x02092DB0, (u32)dsiSaveWrite);
		setBL(0x02092DB8, (u32)dsiSaveClose);
		setBL(0x02092DF4, (u32)dsiSaveOpen);
		setBL(0x02092E04, (u32)dsiSaveGetResultCode);
		setBL(0x02092E1C, (u32)dsiSaveGetLength);
		setBL(0x02092E2C, (u32)dsiSaveRead);
		setBL(0x02092E34, (u32)dsiSaveClose);
		setBL(0x02092E6C, (u32)dsiSaveOpen);
		setBL(0x02092E7C, (u32)dsiSaveGetResultCode);
		setBL(0x02092E94, (u32)dsiSaveClose);
	}

	// Bejeweled Twist (USA)
	else if (strcmp(romTid, "KBEE") == 0) {
		const u32 dsiSaveCreateT = 0x02095E90;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x02095EA0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02095EB0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02095EC0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02095ED0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveGetResultCodeT = 0x02095794;
		*(u16*)dsiSaveGetResultCodeT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetResultCodeT + 4), dsiSaveGetResultCode, 0xC);

		const u32 dsiSaveSetLengthT = 0x02096254;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveWriteT = 0x02096444;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u32*)0x0200498C = 0xE1A00000; // nop
		doubleNopT(0x020368DE); // dsiSaveCreateDirAuto
		setBLThumb(0x020368E6, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x020368F0, dsiSaveOpenT);
		//setBLThumb(0x020368FA, dsiSaveGetResultCodeT);
		doubleNopT(0x02036904);
		doubleNopT(0x0203691C); // dsiSaveCreateDirAuto
		setBLThumb(0x02036924, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x0203692E, dsiSaveOpenT);
		//setBLThumb(0x02036938, dsiSaveGetResultCodeT);
		//setBLThumb(0x0203696E, dsiSaveSetLengthT);
		setBLThumb(0x02036978, dsiSaveSeekT);
		//setBLThumb(0x02036982, dsiSaveWriteT);
		setBLThumb(0x02036988, dsiSaveCloseT);
		setBLThumb(0x020369B6, dsiSaveOpenT);
		setBLThumb(0x020369D0, dsiSaveSeekT);
		setBLThumb(0x020369DA, dsiSaveReadT);
		setBLThumb(0x020369E0, dsiSaveCloseT);
		doubleNopT(0x020945FA);
		doubleNopT(0x02097682);
		doubleNopT(0x0209B390);
		doubleNopT(0x0209CA52);
		doubleNopT(0x0209CA56);
		doubleNopT(0x0209CA62);
		doubleNopT(0x0209CB46);
		patchHiHeapDSiWareThumb(0x0209CB84, 0x0209A2F0, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0209CC5C = 0x0210E1C0;
		*(u16*)0x0209D80E = 0x46C0; // nop
		*(u16*)0x0209D812 = 0xBD38; // POP {R3-R5,PC}
		*(u16*)0x0209D828 = 0x2001; // movs r0, #1
		*(u16*)0x0209D82A = 0x4770; // bx lr
		*(u16*)0x0209D834 = 0x2000; // movs r0, #0
		*(u16*)0x0209D836 = 0x4770; // bx lr
		doubleNopT(0x0209FBB2);
	}

	// Bejeweled Twist (Europe, Australia)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBEV") == 0 && extendedMemory2) {
		const u32 dsiSaveCreateT = 0x02094A78;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x02094A88;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02094A98;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02094AA8;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02094AB8;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveGetResultCodeT = 0x020943F0;
		*(u16*)dsiSaveGetResultCodeT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetResultCodeT + 4), dsiSaveGetResultCode, 0xC);

		const u32 dsiSaveSetLengthT = 0x02094E3C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveWriteT = 0x02094FF4;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		doubleNopT(0x0200910C);
		doubleNopT(0x020091F2);
		doubleNopT(0x0203601E); // dsiSaveCreateDirAuto
		setBLThumb(0x02036026, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x02036030, dsiSaveOpenT);
		//setBLThumb(0x0203603A, dsiSaveGetResultCodeT);
		doubleNopT(0x02036044);
		doubleNopT(0x0203605C); // dsiSaveCreateDirAuto
		setBLThumb(0x02036064, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x0203606E, dsiSaveOpenT);
		//setBLThumb(0x02036078, dsiSaveGetResultCodeT);
		//setBLThumb(0x020360AE, dsiSaveSetLengthT);
		setBLThumb(0x020360B8, dsiSaveSeekT);
		//setBLThumb(0x020360C2, dsiSaveWriteT);
		setBLThumb(0x020360C8, dsiSaveCloseT);
		setBLThumb(0x020360F6, dsiSaveOpenT);
		setBLThumb(0x02036110, dsiSaveSeekT);
		setBLThumb(0x0203611A, dsiSaveReadT);
		setBLThumb(0x02036120, dsiSaveCloseT);
		doubleNopT(0x0209324E);
		doubleNopT(0x02096246);
		doubleNopT(0x02099F74);
		doubleNopT(0x0209B63E);
		doubleNopT(0x0209B642);
		doubleNopT(0x0209B64E);
		doubleNopT(0x0209B732);
		patchHiHeapDSiWareThumb(0x0209B770, 0x02098EC0, 0x02700000); // movs r0, #0x2700000
		*(u32*)0x0209B848 = 0x0212B7E0;
		*(u16*)0x0209C366 = 0x46C0; // nop
		*(u16*)0x0209C36A = 0xBD38; // POP {R3-R5,PC}
		*(u16*)0x0209C384 = 0x2001; // movs r0, #1
		*(u16*)0x0209C386 = 0x4770; // bx lr
		*(u16*)0x0209C38C = 0x2000; // movs r0, #0
		*(u16*)0x0209C38E = 0x4770; // bx lr
		doubleNopT(0x0209E722);
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

	// Bird & Beans (USA)
	// Difficult to get working
	/*else if (strcmp(romTid, "KP6E") == 0) {
		doubleNopT(0x0200509E);
		doubleNopT(0x020050A2);
		doubleNopT(0x020050AC);
		doubleNopT(0x02005254);
		*(u16*)0x0205F52 = 0x2001; // movs r0, #1
		*(u16*)0x0205F54 = 0x46C0; // nop
		doubleNopT(0x02005F74);
		*(u16*)0x0205F90 = 0x2001; // movs r0, #1
		*(u16*)0x0205F92 = 0x46C0; // nop
		doubleNopT(0x0200DC4E);
		//doubleNopT(0x0200EDCE);
		//doubleNopT(0x0200EDDE);
		doubleNopT(0x02020A9E);
		doubleNopT(0x02022EFA);
		doubleNopT(0x0202640C);
		doubleNopT(0x02027B7A);
		doubleNopT(0x02027B7E);
		doubleNopT(0x02027B8A);
		doubleNopT(0x02027C6E);
		patchHiHeapDSiWareThumb(0x02027CAC, 0x020256D8, heapEnd); // movs r0, #0x23C0000
		*(u16*)0x02028A30 = 0xBD10; // POP {R4,PC}
		doubleNopT(0x02028D26);
		doubleNopT(0x02028D2A);
		doubleNopT(0x02028D2E);
		doubleNopT(0x0202AD82);
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
		*(u32*)0x02078A40 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02078A5C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02078A60 = 0xE12FFF1E; // bx lr
		*(u32*)0x02078A68 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02078A6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BE88 = 0xE1A00000; // nop
		*(u32*)0x0207D764 = 0xE3A00003; // mov r0, #3
	}

	// BlayzBloo: Batoru x Batoru (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBZJ") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0206B810 = 0xE1A00000; // nop
		*(u32*)0x0206F30C = 0xE1A00000; // nop
		*(u32*)0x020755EC = 0xE1A00000; // nop
		*(u32*)0x020774F4 = 0xE1A00000; // nop
		*(u32*)0x020774F8 = 0xE1A00000; // nop
		*(u32*)0x02077504 = 0xE1A00000; // nop
		*(u32*)0x02077664 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020776C0, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02078914 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02078930 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02078934 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207893C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02078940 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BD4C = 0xE1A00000; // nop
		*(u32*)0x0207D628 = 0xE3A00003; // mov r0, #3
	}

	// BlayzBloo: Baeteul x Baeteul (Korea)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBZK") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0206B718 = 0xE1A00000; // nop
		*(u32*)0x0206F214 = 0xE1A00000; // nop
		*(u32*)0x020754F4 = 0xE1A00000; // nop
		*(u32*)0x020773FC = 0xE1A00000; // nop
		*(u32*)0x02077400 = 0xE1A00000; // nop
		*(u32*)0x0207740C = 0xE1A00000; // nop
		*(u32*)0x0207756C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020775C8, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0207881C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02078838 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207883C = 0xE12FFF1E; // bx lr
		*(u32*)0x02078844 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02078848 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BC64 = 0xE1A00000; // nop
		*(u32*)0x0207D540 = 0xE3A00003; // mov r0, #3
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
	else if (strncmp(romTid, "KBB", 3) == 0) {
		*(u32*)0x02008988 = 0xE1A00000; // nop
		tonccpy((u32*)0x02009670, dsiSaveGetResultCode, 0xC);
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
			//*(u32*)0x02043528 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			//*(u32*)0x020437AC = 0xE3A00001; // mov r0, #1
			//*(u32*)0x020437B0 = 0xE12FFF1E; // bx lr
			*(u32*)0x020437B4 = 0xE1A00000; // nop
			*(u32*)0x020437D0 = 0xE1A00000; // nop
			*(u32*)0x020437D8 = 0xE1A00000; // nop
			*(u32*)0x020437F0 = 0xE1A00000; // nop
			*(u32*)0x02043814 = 0xE1A00000; // nop
			setBL(0x02043950, (u32)dsiSaveOpen);
			setBL(0x020439D0, (u32)dsiSaveCreate);
			setBL(0x02043A5C, (u32)dsiSaveWrite);
			setBL(0x02043A70, (u32)dsiSaveClose);
			setBL(0x02043AE8, (u32)dsiSaveClose);
			setBL(0x02046394, (u32)dsiSaveOpen);
			setBL(0x02046428, (u32)dsiSaveRead);
			setBL(0x0204649C, (u32)dsiSaveClose);
			*(u32*)0x02085158 = 0xE3A00000; // mov r0, #0
			setBL(0x0208523C, 0x020867FC);
			setBL(0x020852F0, 0x02086930);
			setBL(0x020853A8, 0x0208699C);
			setBL(0x02085624, 0x02086AA4);
			setBL(0x02085704, 0x02086B54);
			setBL(0x02085844, 0x02086BC0);
			setBL(0x02085974, 0x02086D6C);
			*(u32*)0x02085D40 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02085D70 = 0xE3A00000; // mov r0, #0
			setBL(0x020864E0, 0x02086CA8);
			*(u32*)0x0208DF10 = 0xE1A00000; // nop
			*(u32*)0x0208DF18 = 0xE3A00001; // mov r0, #1
			setB(0x0208EF04, 0x0208EFEC);
			*(u32*)0x0208EFEC = 0xE1A00000; // nop
			*(u32*)0x0208EFF0 = 0xE1A00000; // nop
			*(u32*)0x0208EFF8 = 0xE1A00000; // nop
			*(u32*)0x0208EFFC = 0xE1A00000; // nop
			*(u32*)0x0208F000 = 0xE1A00000; // nop
			*(u32*)0x0208F004 = 0xE1A00000; // nop
			setB(0x0208F828, 0x0208F8C8);
			*(u32*)0x0208FA34 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FA38 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FA90 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FA94 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FB94 = 0xE1A00000; // nop
			setB(0x02090938, 0x02090AE4);
			*(u32*)0x020911B8 = 0xE1A00000; // nop
			*(u32*)0x020911BC = 0xE1A00000; // nop
			*(u32*)0x020911C0 = 0xE1A00000; // nop
			*(u32*)0x020911C4 = 0xE1A00000; // nop
			*(u32*)0x020911C8 = 0xE1A00000; // nop
			*(u32*)0x020911CC = 0xE1A00000; // nop
			*(u32*)0x020911D0 = 0xE1A00000; // nop
			*(u32*)0x020911D4 = 0xE1A00000; // nop
			*(u32*)0x020911D8 = 0xE1A00000; // nop
			setB(0x02092560, 0x0209257C);
			setB(0x020927CC, 0x020927F4);
			*(u32*)0x020927F4 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
			*(u32*)0x020927F8 += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
			*(u32*)0x020927FC = 0xE3A00000; // mov r0, #0
			*(u32*)0x02092800 = 0xE5850030; // str r0, [r5,#0x30]
			*(u32*)0x02092804 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
			*(u32*)0x020AAFD0 = 0xE3A02C07; // mov r2, #0x700
			*(u32*)0x020AAFF0 = 0xE2840B01; // add r0, r4, #0x400
			*(u32*)0x020AAFF8 = 0xE1A00004; // mov r0, r4
			*(u32*)0x020AB000 = 0xE1A00000; // nop
			*(u32*)0x020AB004 = 0xE1A00000; // nop
			*(u32*)0x020AB008 = 0xE1A00000; // nop
			*(u32*)0x020AB00C = 0xE1A00000; // nop
			*(u32*)0x020AB010 = 0xE1A00000; // nop
			*(u32*)0x020AB024 = 0xE2841B01; // add r1, r4, #0x400
		} else if (ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x020435E8 = 0xE1A00000; // nop
			//*(u32*)0x020435F4 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			//*(u32*)0x02043878 = 0xE3A00001; // mov r0, #1
			//*(u32*)0x0204387C = 0xE12FFF1E; // bx lr
			*(u32*)0x02043880 = 0xE1A00000; // nop
			*(u32*)0x0204389C = 0xE1A00000; // nop
			*(u32*)0x020438A4 = 0xE1A00000; // nop
			*(u32*)0x020438BC = 0xE1A00000; // nop
			*(u32*)0x020438E0 = 0xE1A00000; // nop
			setBL(0x02043A1C, (u32)dsiSaveOpen);
			setBL(0x02043A9C, (u32)dsiSaveCreate);
			setBL(0x02043B28, (u32)dsiSaveWrite);
			setBL(0x02043B28, (u32)dsiSaveClose);
			setBL(0x02043BB4, (u32)dsiSaveClose);
			setBL(0x02046460, (u32)dsiSaveOpen);
			setBL(0x020464F4, (u32)dsiSaveRead);
			setBL(0x02046568, (u32)dsiSaveClose);
			*(u32*)0x02085254 = 0xE3A00000; // mov r0, #0
			setBL(0x02085338, 0x020868F8);
			setBL(0x020853EC, 0x02086A2C);
			setBL(0x020854A4, 0x02086A98);
			setBL(0x02085720, 0x02086BA0);
			setBL(0x02085800, 0x02086C50);
			setBL(0x02085940, 0x02086CBC);
			setBL(0x02085A70, 0x02086E68);
			*(u32*)0x02085E3C = 0xE3A00001; // mov r0, #1
			*(u32*)0x02085E6C = 0xE3A00000; // mov r0, #0
			setBL(0x020865DC, 0x02086DA4);
			*(u32*)0x0208E00C = 0xE1A00000; // nop
			*(u32*)0x0208E014 = 0xE3A00001; // mov r0, #1
			setB(0x0208F000, 0x0208F0E8);
			*(u32*)0x0208F0E8 = 0xE1A00000; // nop
			*(u32*)0x0208F0EC = 0xE1A00000; // nop
			*(u32*)0x0208F0F4 = 0xE1A00000; // nop
			*(u32*)0x0208F0F8 = 0xE1A00000; // nop
			*(u32*)0x0208F0FC = 0xE1A00000; // nop
			*(u32*)0x0208F100 = 0xE1A00000; // nop
			setB(0x0208F924, 0x0208F9C4);
			*(u32*)0x0208FB30 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FB34 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FB8C = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FB90 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FC90 = 0xE1A00000; // nop
			setB(0x02090A34, 0x02090BE0);
			*(u32*)0x020912B4 = 0xE1A00000; // nop
			*(u32*)0x020912B8 = 0xE1A00000; // nop
			*(u32*)0x020912BC = 0xE1A00000; // nop
			*(u32*)0x020912C0 = 0xE1A00000; // nop
			*(u32*)0x020912C4 = 0xE1A00000; // nop
			*(u32*)0x020912C8 = 0xE1A00000; // nop
			*(u32*)0x020912CC = 0xE1A00000; // nop
			*(u32*)0x020912D0 = 0xE1A00000; // nop
			*(u32*)0x020912D4 = 0xE1A00000; // nop
			setB(0x0209265C, 0x02092678);
			setB(0x020928C8, 0x020928F0);
			*(u32*)0x020928F0 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
			*(u32*)0x020928F4 += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
			*(u32*)0x020928F8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020928FC = 0xE5850030; // str r0, [r5,#0x30]
			*(u32*)0x02092900 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
			*(u32*)0x020AB0CC = 0xE3A02C07; // mov r2, #0x700
			*(u32*)0x020AB0EC = 0xE2840B01; // add r0, r4, #0x400
			*(u32*)0x020AB0F4 = 0xE1A00004; // mov r0, r4
			*(u32*)0x020AB0FC = 0xE1A00000; // nop
			*(u32*)0x020AB100 = 0xE1A00000; // nop
			*(u32*)0x020AB104 = 0xE1A00000; // nop
			*(u32*)0x020AB108 = 0xE1A00000; // nop
			*(u32*)0x020AB10C = 0xE1A00000; // nop
			*(u32*)0x020AB120 = 0xE2841B01; // add r1, r4, #0x400
		} else if (ndsHeader->gameCode[3] == 'J') {
			*(u32*)0x02043248 = 0xE1A00000; // nop
			//*(u32*)0x02043254 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			//*(u32*)0x020434D8 = 0xE3A00001; // mov r0, #1
			//*(u32*)0x020434DC = 0xE12FFF1E; // bx lr
			*(u32*)0x020434E0 = 0xE1A00000; // nop
			*(u32*)0x020434FC = 0xE1A00000; // nop
			*(u32*)0x02043504 = 0xE1A00000; // nop
			*(u32*)0x0204351C = 0xE1A00000; // nop
			*(u32*)0x02043540 = 0xE1A00000; // nop
			setBL(0x0204367C, (u32)dsiSaveOpen);
			setBL(0x020436FC, (u32)dsiSaveCreate);
			setBL(0x02043788, (u32)dsiSaveWrite);
			setBL(0x0204379C, (u32)dsiSaveClose);
			setBL(0x02043814, (u32)dsiSaveClose);
			setBL(0x020460C0, (u32)dsiSaveOpen);
			setBL(0x02046154, (u32)dsiSaveRead);
			setBL(0x020461C8, (u32)dsiSaveClose);
			*(u32*)0x0207EEFC = 0xE3A00000; // mov r0, #0
			setBL(0x0207EFE0, 0x020805A0);
			setBL(0x0207F094, 0x020806D4);
			setBL(0x0207F14C, 0x02080740);
			setBL(0x0207F3C8, 0x02080848);
			setBL(0x0207F4A8, 0x020808F8);
			setBL(0x0207F5E8, 0x02080964);
			setBL(0x0207F718, 0x02080B10);
			*(u32*)0x0207FAE4 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0207FB14 = 0xE3A00000; // mov r0, #0
			setBL(0x02080284, 0x02080A4C);
			*(u32*)0x02087CB4 = 0xE1A00000; // nop
			*(u32*)0x02087CBC = 0xE3A00001; // mov r0, #1
			setB(0x02088CA8, 0x02088D90);
			*(u32*)0x02088D90 = 0xE1A00000; // nop
			*(u32*)0x02088D94 = 0xE1A00000; // nop
			*(u32*)0x02088D9C = 0xE1A00000; // nop
			*(u32*)0x02088DA0 = 0xE1A00000; // nop
			*(u32*)0x02088DA4 = 0xE1A00000; // nop
			*(u32*)0x02088DA8 = 0xE1A00000; // nop
			setB(0x020895CC, 0x0208966C);
			*(u32*)0x020897D8 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020897DC = 0xE12FFF1E; // bx lr
			*(u32*)0x02089834 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02089838 = 0xE12FFF1E; // bx lr
			*(u32*)0x02089938 = 0xE1A00000; // nop
			setB(0x0208A6DC, 0x0208A888);
			*(u32*)0x0208AF5C = 0xE1A00000; // nop
			*(u32*)0x0208AF60 = 0xE1A00000; // nop
			*(u32*)0x0208AF64 = 0xE1A00000; // nop
			*(u32*)0x0208AF68 = 0xE1A00000; // nop
			*(u32*)0x0208AF6C = 0xE1A00000; // nop
			*(u32*)0x0208AF70 = 0xE1A00000; // nop
			*(u32*)0x0208AF74 = 0xE1A00000; // nop
			*(u32*)0x0208AF78 = 0xE1A00000; // nop
			*(u32*)0x0208AF7C = 0xE1A00000; // nop
			setB(0x0208C304, 0x0208C320);
			setB(0x0208C570, 0x0208C598);
			*(u32*)0x0208C598 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
			*(u32*)0x0208C59C += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
			*(u32*)0x0208C5A0 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0208C5A4 = 0xE5850030; // str r0, [r5,#0x30]
			*(u32*)0x0208C5A8 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
			*(u32*)0x020A4D74 = 0xE3A02C07; // mov r2, #0x700
			*(u32*)0x020A4D94 = 0xE2840B01; // add r0, r4, #0x400
			*(u32*)0x020A4D9C = 0xE1A00004; // mov r0, r4
			*(u32*)0x020A4DA4 = 0xE1A00000; // nop
			*(u32*)0x020A4DA8 = 0xE1A00000; // nop
			*(u32*)0x020A4DAC = 0xE1A00000; // nop
			*(u32*)0x020A4DB0 = 0xE1A00000; // nop
			*(u32*)0x020A4DB4 = 0xE1A00000; // nop
			*(u32*)0x020A4DC8 = 0xE2841B01; // add r1, r4, #0x400
		}
	}

	// Bookworm (USA)
	// Saving is difficult to implement, preventing support
	/*else if (strcmp(romTid, "KBKE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02017DE4 = 0x8C000;
		}
		setBL(0x0204E744, (u32)dsiSaveClose);
		setBL(0x0204EA68, (u32)dsiSaveSeek);
		setBL(0x0204EAA8, (u32)dsiSaveWrite);
		setBL(0x0204EAF0, (u32)dsiSaveSeek);
		setBL(0x0204EB18, (u32)dsiSaveWrite);
		*(u32*)0x0204EB48 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0204EB90, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0204EBA8, (u32)dsiSaveOpen);
		setBL(0x0204EBD8, (u32)dsiSaveSetLength);
		setBL(0x0204EC8C, (u32)dsiSaveWrite);
		*(u32*)0x0204ECC4 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0204ECD0, (u32)dsiSaveGetResultCode);
		setBL(0x0204ED04, (u32)dsiSaveOpen);
		setBL(0x0204ED30, (u32)dsiSaveGetLength);
		setBL(0x0204ED88, (u32)dsiSaveRead);
		*(u32*)0x02065B14 = 0xE1A00000; // nop
		*(u32*)0x020699BC = 0xE1A00000; // nop
		*(u32*)0x0206E584 = 0xE1A00000; // nop
		*(u32*)0x02070484 = 0xE1A00000; // nop
		*(u32*)0x02070488 = 0xE1A00000; // nop
		*(u32*)0x02070494 = 0xE1A00000; // nop
		*(u32*)0x020705F4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02070650, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		if (!extendedMemory2) {
			*(u32*)0x02070784 = 0x020DE060;
		}
		*(u32*)0x02071E6C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0207574C = 0xE1A00000; // nop
	}*/

	// Art Style: BOXLIFE (USA)
	else if (strcmp(romTid, "KAHE") == 0) {
		*(u32*)0x0202FBD0 = 0xE1A00000; // nop
		setBL(0x020353B4, (u32)dsiSaveOpen);
		//*(u32*)0x020355D8 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020355DC = 0xE12FFF1E; // bx lr
		setBL(0x02035608, (u32)dsiSaveOpen);
		setBL(0x02035658, (u32)dsiSaveRead);
		setBL(0x0203569C, (u32)dsiSaveClose);
		//*(u32*)0x020356C4 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020356C8 = 0xE12FFF1E; // bx lr
		setBL(0x020356E8, (u32)dsiSaveCreate);
		setBL(0x020356F8, (u32)dsiSaveGetResultCode);
		setBL(0x0203571C, (u32)dsiSaveOpen);
		setBL(0x02035738, (u32)dsiSaveSetLength);
		setBL(0x02035754, (u32)dsiSaveWrite);
		setBL(0x02035770, (u32)dsiSaveClose);
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
		setBL(0x02034FFC, (u32)dsiSaveOpen);
		//*(u32*)0x02035220 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02035224 = 0xE12FFF1E; // bx lr
		setBL(0x02035250, (u32)dsiSaveOpen);
		setBL(0x020352A0, (u32)dsiSaveRead);
		setBL(0x020352E4, (u32)dsiSaveClose);
		//*(u32*)0x0203530C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02035310 = 0xE12FFF1E; // bx lr
		setBL(0x02035330, (u32)dsiSaveCreate);
		setBL(0x02035340, (u32)dsiSaveGetResultCode);
		setBL(0x02035364, (u32)dsiSaveOpen);
		setBL(0x02035380, (u32)dsiSaveSetLength);
		setBL(0x0203539C, (u32)dsiSaveWrite);
		setBL(0x020353B8, (u32)dsiSaveClose);
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
		setBL(0x02034348, (u32)dsiSaveOpen);
		//*(u32*)0x0203456C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02034570 = 0xE12FFF1E; // bx lr
		setBL(0x0203459C, (u32)dsiSaveOpen);
		setBL(0x020345EC, (u32)dsiSaveRead);
		setBL(0x02034630, (u32)dsiSaveClose);
		//*(u32*)0x02034658 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203465C = 0xE12FFF1E; // bx lr
		setBL(0x0203467C, (u32)dsiSaveCreate);
		setBL(0x0203468C, (u32)dsiSaveGetResultCode);
		setBL(0x020346B0, (u32)dsiSaveOpen);
		setBL(0x020346CC, (u32)dsiSaveSetLength);
		setBL(0x020346E8, (u32)dsiSaveWrite);
		setBL(0x02034704, (u32)dsiSaveClose);
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
		u32* saveFuncOffsets[22] = {NULL};

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201B334 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201BEB8, dsiSaveGetResultCode, 0xC);
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
			*(u32*)0x0205A438 = 0xE1A00000;
			*(u32*)0x0205A478 = 0xE1A00000;
			*(u32*)0x0205A550 = 0xE1A00000;
			*(u32*)0x0205A558 = 0xE1A00000;
			*(u32*)0x0205A588 = 0xE1A00000;
			*(u32*)0x0205A600 = 0xE1A00000;
			*(u32*)0x0205A608 = 0xE1A00000;
			*(u32*)0x0205A740 = 0xE1A00000;
			*(u32*)0x0205A83C = 0xE1A00000;
			*(u32*)0x0205A844 = 0xE1A00000;
			*(u32*)0x0205A8F4 = 0xE1A00000;
			*(u32*)0x0205A8FC = 0xE1A00000;
			*(u32*)0x0205ABBC = 0xE1A00000;
			*(u32*)0x0205ACEC = 0xE1A00000;
			*(u32*)0x0205ACF4 = 0xE1A00000;
			*(u32*)0x0205AEB0 = 0xE1A00000;
			*(u32*)0x0205AED8 = 0xE1A00000;
			*(u32*)0x0205AEE0 = 0xE1A00000;
			saveFuncOffsets[0] = (u32*)0x0205B148;
			saveFuncOffsets[1] = (u32*)0x0205B160;
			saveFuncOffsets[2] = (u32*)0x0205B174;
			saveFuncOffsets[3] = (u32*)0x0205B18C;
			saveFuncOffsets[4] = (u32*)0x0205B1A0;
			saveFuncOffsets[5] = (u32*)0x0205B1B8;
			saveFuncOffsets[6] = (u32*)0x0205B1CC;
			saveFuncOffsets[7] = (u32*)0x0205B1DC;
			saveFuncOffsets[8] = (u32*)0x0205B24C;
			saveFuncOffsets[9] = (u32*)0x0205B264;
			saveFuncOffsets[10] = (u32*)0x0205B278;
			saveFuncOffsets[11] = (u32*)0x0205B290;
			saveFuncOffsets[12] = (u32*)0x0205B2A4;
			saveFuncOffsets[13] = (u32*)0x0205B2BC;
			saveFuncOffsets[14] = (u32*)0x0205B2D0;
			saveFuncOffsets[15] = (u32*)0x0205B2E0;
			saveFuncOffsets[16] = (u32*)0x0205B350;
			saveFuncOffsets[17] = (u32*)0x0205B384;
			saveFuncOffsets[18] = (u32*)0x0205B3A4;
			saveFuncOffsets[19] = (u32*)0x0205B3AC;
			saveFuncOffsets[20] = (u32*)0x0205B40C;
			saveFuncOffsets[21] = (u32*)0x0205B424;
		} else if (ndsHeader->gameCode[3] == 'P') {
			*(u32*)0x0205B920 = 0xE1A00000;
			*(u32*)0x0205B960 = 0xE1A00000;
			*(u32*)0x0205BA38 = 0xE1A00000;
			*(u32*)0x0205BA40 = 0xE1A00000;
			*(u32*)0x0205BB6C = 0xE1A00000;
			*(u32*)0x0205BBE4 = 0xE1A00000;
			*(u32*)0x0205BBEC = 0xE1A00000;
			*(u32*)0x0205BC78 = 0xE1A00000;
			*(u32*)0x0205BCE4 = 0xE1A00000;
			*(u32*)0x0205BCEC = 0xE1A00000;
			*(u32*)0x0205BD78 = 0xE1A00000;
			*(u32*)0x0205BE74 = 0xE1A00000;
			*(u32*)0x0205BE7C = 0xE1A00000;
			*(u32*)0x0205BF2C = 0xE1A00000;
			*(u32*)0x0205BF34 = 0xE1A00000;
			*(u32*)0x0205C16C = 0xE1A00000;
			*(u32*)0x0205C29C = 0xE1A00000;
			*(u32*)0x0205C2A4 = 0xE1A00000;
			*(u32*)0x0205C460 = 0xE1A00000;
			*(u32*)0x0205C488 = 0xE1A00000;
			*(u32*)0x0205C490 = 0xE1A00000;
			saveFuncOffsets[0] = (u32*)0x0205C6F8;
			saveFuncOffsets[1] = (u32*)0x0205C710;
			saveFuncOffsets[2] = (u32*)0x0205C724;
			saveFuncOffsets[3] = (u32*)0x0205C73C;
			saveFuncOffsets[4] = (u32*)0x0205C750;
			saveFuncOffsets[5] = (u32*)0x0205C768;
			saveFuncOffsets[6] = (u32*)0x0205C77C;
			saveFuncOffsets[7] = (u32*)0x0205C78C;
			saveFuncOffsets[8] = (u32*)0x0205C7FC;
			saveFuncOffsets[9] = (u32*)0x0205C814;
			saveFuncOffsets[10] = (u32*)0x0205C828;
			saveFuncOffsets[11] = (u32*)0x0205C840;
			saveFuncOffsets[12] = (u32*)0x0205C854;
			saveFuncOffsets[13] = (u32*)0x0205C86C;
			saveFuncOffsets[14] = (u32*)0x0205C880;
			saveFuncOffsets[15] = (u32*)0x0205C890;
			saveFuncOffsets[16] = (u32*)0x0205C900;
			saveFuncOffsets[17] = (u32*)0x0205C934;
			saveFuncOffsets[18] = (u32*)0x0205C954;
			saveFuncOffsets[19] = (u32*)0x0205C95C;
			saveFuncOffsets[20] = (u32*)0x0205C9BC;
			saveFuncOffsets[21] = (u32*)0x0205C9D4; 
		}

		setBL((u32)saveFuncOffsets[0], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[1], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[2], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[3], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[4], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[5], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[6], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[7], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[8], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[9], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[10], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[11], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[12], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[13], (u32)dsiSaveRead);
		setBL((u32)saveFuncOffsets[14], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[15], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[16], (u32)dsiSaveCreate);
		setBL((u32)saveFuncOffsets[17], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[18], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[19], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[20], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[21], (u32)dsiSaveClose);
	}

	// Cake Ninja (USA)
	else if (strcmp(romTid, "K2JE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008C4C = 0xE1A00000; // nop
		*(u32*)0x02008DE4 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Make BG static to cut down RAM usage
			setB(0x02008400, 0x02008664);
			*(u32*)0x0200DCC0 = 0xE1A00000; // nop
			*(u32*)0x0200DCC4 = 0xE1A00000; // nop
			*(u32*)0x0200DCC8 = 0xE1A00000; // nop
			*(u32*)0x0200DCCC = 0xE1A00000; // nop
		}
		setBL(0x0202CDF0, (u32)dsiSaveOpen);
		setBL(0x0202CE48, (u32)dsiSaveCreate);
		setBL(0x0202CE7C, (u32)dsiSaveOpen);
		setBL(0x0202CE90, (u32)dsiSaveSetLength);
		setBL(0x0202CEA0, (u32)dsiSaveGetLength);
		setBL(0x0202CEA8, (u32)dsiSaveClose);
		setBL(0x0202CEE0, (u32)dsiSaveSetLength);
		setBL(0x0202CEF0, (u32)dsiSaveGetLength);
		setBL(0x0202CEF8, (u32)dsiSaveClose);
		*(u32*)0x0202D028 = 0xE1A00000; // nop
		setBL(0x0202D100, (u32)dsiSaveOpen);
		setBL(0x0202D128, (u32)dsiSaveSeek);
		setBL(0x0202D13C, (u32)dsiSaveRead);
		setBL(0x0202D154, (u32)dsiSaveClose);
		setBL(0x0202D21C, (u32)dsiSaveOpen);
		setBL(0x0202D244, (u32)dsiSaveSeek);
		setBL(0x0202D258, (u32)dsiSaveWrite);
		setBL(0x0202D264, (u32)dsiSaveClose);
		*(u32*)0x02057938 = 0xE1A00000; // nop
		tonccpy((u32*)0x020584CC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B0FC = 0xE1A00000; // nop
		*(u32*)0x02060CAC = 0xE1A00000; // nop
		*(u32*)0x02062CB0 = 0xE1A00000; // nop
		*(u32*)0x02062CB4 = 0xE1A00000; // nop
		*(u32*)0x02062CC0 = 0xE1A00000; // nop
		*(u32*)0x02062E20 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062E7C, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x02064424 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02064440 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02064444 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206444C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064450 = 0xE12FFF1E; // bx lr
		*(u32*)0x020679A0 = 0xE1A00000; // nop
	}

	// Cake Ninja (Europe)
	else if (strcmp(romTid, "K2JP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008C4C = 0xE1A00000; // nop
		*(u32*)0x02008ED4 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Make BG static to cut down RAM usage
			setB(0x02008400, 0x02008664);
			*(u32*)0x0200DD4C = 0xE1A00000; // nop
			*(u32*)0x0200DD50 = 0xE1A00000; // nop
			*(u32*)0x0200DD54 = 0xE1A00000; // nop
			*(u32*)0x0200DD58 = 0xE1A00000; // nop
		}
		setBL(0x0202CEC8, (u32)dsiSaveOpen);
		setBL(0x0202CF20, (u32)dsiSaveCreate);
		setBL(0x0202CF54, (u32)dsiSaveOpen);
		setBL(0x0202CF68, (u32)dsiSaveSetLength);
		setBL(0x0202CF78, (u32)dsiSaveGetLength);
		setBL(0x0202CF80, (u32)dsiSaveClose);
		setBL(0x0202CFB8, (u32)dsiSaveSetLength);
		setBL(0x0202CFC8, (u32)dsiSaveGetLength);
		setBL(0x0202CFD0, (u32)dsiSaveClose);
		*(u32*)0x0202D100 = 0xE1A00000; // nop
		setBL(0x0202D1D8, (u32)dsiSaveOpen);
		setBL(0x0202D200, (u32)dsiSaveSeek);
		setBL(0x0202D214, (u32)dsiSaveRead);
		setBL(0x0202D22C, (u32)dsiSaveClose);
		setBL(0x0202D2F4, (u32)dsiSaveOpen);
		setBL(0x0202D31C, (u32)dsiSaveSeek);
		setBL(0x0202D330, (u32)dsiSaveWrite);
		setBL(0x0202D33C, (u32)dsiSaveClose);
		*(u32*)0x02057A10 = 0xE1A00000; // nop
		tonccpy((u32*)0x020585A4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B1D4 = 0xE1A00000; // nop
		*(u32*)0x02060D84 = 0xE1A00000; // nop
		*(u32*)0x02062D88 = 0xE1A00000; // nop
		*(u32*)0x02062D8C = 0xE1A00000; // nop
		*(u32*)0x02062D90 = 0xE1A00000; // nop
		*(u32*)0x02062EF8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02062F54, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x020644FC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02064518 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206451C = 0xE12FFF1E; // bx lr
		*(u32*)0x02064524 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064528 = 0xE12FFF1E; // bx lr
		*(u32*)0x02067A78 = 0xE1A00000; // nop
	}

	// Cake Ninja 2 (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K2NE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008880 = 0xE1A00000; // nop
		*(u32*)0x020089F4 = 0xE1A00000; // nop
		setBL(0x0204C918, (u32)dsiSaveOpen);
		setBL(0x0204C970, (u32)dsiSaveCreate);
		setBL(0x0204C9A4, (u32)dsiSaveOpen);
		setBL(0x0204C9B8, (u32)dsiSaveSetLength);
		setBL(0x0204C9C8, (u32)dsiSaveGetLength);
		setBL(0x0204C9D0, (u32)dsiSaveClose);
		setBL(0x0204CA08, (u32)dsiSaveSetLength);
		setBL(0x0204CA18, (u32)dsiSaveGetLength);
		setBL(0x0204CA20, (u32)dsiSaveClose);
		*(u32*)0x0204CB50 = 0xE1A00000; // nop
		setBL(0x0204CC28, (u32)dsiSaveOpen);
		setBL(0x0204CC50, (u32)dsiSaveSeek);
		setBL(0x0204CC64, (u32)dsiSaveRead);
		setBL(0x0204CC7C, (u32)dsiSaveClose);
		setBL(0x0204CD44, (u32)dsiSaveOpen);
		setBL(0x0204CD6C, (u32)dsiSaveSeek);
		setBL(0x0204CD80, (u32)dsiSaveWrite);
		setBL(0x0204CD8C, (u32)dsiSaveClose);
		*(u32*)0x020774AC = 0xE1A00000; // nop
		tonccpy((u32*)0x02078040, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0207AC70 = 0xE1A00000; // nop
		*(u32*)0x0208084C = 0xE1A00000; // nop
		*(u32*)0x02082850 = 0xE1A00000; // nop
		*(u32*)0x02082854 = 0xE1A00000; // nop
		*(u32*)0x02082860 = 0xE1A00000; // nop
		*(u32*)0x020829C0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02082A1C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02083FC4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02083FE0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02083FE4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02083FEC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02083FF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02087540 = 0xE1A00000; // nop
	}

	// Cake Ninja 2 (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K2NP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02008880 = 0xE1A00000; // nop
		*(u32*)0x02008A88 = 0xE1A00000; // nop
		setBL(0x0204C974, (u32)dsiSaveOpen);
		setBL(0x0204C9CC, (u32)dsiSaveCreate);
		setBL(0x0204CA00, (u32)dsiSaveOpen);
		setBL(0x0204CA14, (u32)dsiSaveSetLength);
		setBL(0x0204CA24, (u32)dsiSaveGetLength);
		setBL(0x0204CA2C, (u32)dsiSaveClose);
		setBL(0x0204CA64, (u32)dsiSaveSetLength);
		setBL(0x0204CA74, (u32)dsiSaveGetLength);
		setBL(0x0204CA7C, (u32)dsiSaveClose);
		*(u32*)0x0204CBAC = 0xE1A00000; // nop
		setBL(0x0204CC84, (u32)dsiSaveOpen);
		setBL(0x0204CCAC, (u32)dsiSaveSeek);
		setBL(0x0204CCC0, (u32)dsiSaveRead);
		setBL(0x0204CCD8, (u32)dsiSaveClose);
		setBL(0x0204CDA0, (u32)dsiSaveOpen);
		setBL(0x0204CDC8, (u32)dsiSaveSeek);
		setBL(0x0204CDDC, (u32)dsiSaveWrite);
		setBL(0x0204CDE8, (u32)dsiSaveClose);
		*(u32*)0x02077508 = 0xE1A00000; // nop
		tonccpy((u32*)0x0207809C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0207ACCC = 0xE1A00000; // nop
		*(u32*)0x020808A8 = 0xE1A00000; // nop
		*(u32*)0x020828AC = 0xE1A00000; // nop
		*(u32*)0x020828B0 = 0xE1A00000; // nop
		*(u32*)0x020828BC = 0xE1A00000; // nop
		*(u32*)0x02082A1C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02082A78, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02084020 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0208403C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02084040 = 0xE12FFF1E; // bx lr
		*(u32*)0x02084048 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208404C = 0xE12FFF1E; // bx lr
		*(u32*)0x0208759C = 0xE1A00000; // nop
	}

	// Cake Ninja: XMAS (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYNE") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200846C = 0xE1A00000; // nop
		*(u32*)0x02008604 = 0xE1A00000; // nop
		setBL(0x0202571C, (u32)dsiSaveOpen);
		setBL(0x02025774, (u32)dsiSaveCreate);
		setBL(0x020257A8, (u32)dsiSaveOpen);
		setBL(0x020257BC, (u32)dsiSaveSetLength);
		setBL(0x020257CC, (u32)dsiSaveGetLength);
		setBL(0x020257D4, (u32)dsiSaveClose);
		setBL(0x0202580C, (u32)dsiSaveSetLength);
		setBL(0x0202581C, (u32)dsiSaveGetLength);
		setBL(0x02025824, (u32)dsiSaveClose);
		*(u32*)0x02025954 = 0xE1A00000; // nop
		setBL(0x02025A2C, (u32)dsiSaveOpen);
		setBL(0x02025A54, (u32)dsiSaveSeek);
		setBL(0x02025A68, (u32)dsiSaveRead);
		setBL(0x02025A80, (u32)dsiSaveClose);
		setBL(0x02025B48, (u32)dsiSaveOpen);
		setBL(0x02025B70, (u32)dsiSaveSeek);
		setBL(0x02025B84, (u32)dsiSaveWrite);
		setBL(0x02025B90, (u32)dsiSaveClose);
		*(u32*)0x02050348 = 0xE1A00000; // nop
		tonccpy((u32*)0x02050EDC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02053B0C = 0xE1A00000; // nop
		*(u32*)0x020597E8 = 0xE1A00000; // nop
		*(u32*)0x0205B7EC = 0xE1A00000; // nop
		*(u32*)0x0205B7F0 = 0xE1A00000; // nop
		*(u32*)0x0205B7FC = 0xE1A00000; // nop
		*(u32*)0x0205B95C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0205B9B8, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0205CF60 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205CF7C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205CF80 = 0xE12FFF1E; // bx lr
		*(u32*)0x0205CF88 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205CF8C = 0xE12FFF1E; // bx lr
		*(u32*)0x020604DC = 0xE1A00000; // nop
	}

	// Cake Ninja: XMAS (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYNP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200846C = 0xE1A00000; // nop
		*(u32*)0x020086F4 = 0xE1A00000; // nop
		setBL(0x020257A8, (u32)dsiSaveOpen);
		setBL(0x02025800, (u32)dsiSaveCreate);
		setBL(0x02025834, (u32)dsiSaveOpen);
		setBL(0x02025848, (u32)dsiSaveSetLength);
		setBL(0x02025858, (u32)dsiSaveGetLength);
		setBL(0x02025860, (u32)dsiSaveClose);
		setBL(0x02025898, (u32)dsiSaveSetLength);
		setBL(0x020258A8, (u32)dsiSaveGetLength);
		setBL(0x020258B0, (u32)dsiSaveClose);
		*(u32*)0x020259E0 = 0xE1A00000; // nop
		setBL(0x02025AB8, (u32)dsiSaveOpen);
		setBL(0x02025AE0, (u32)dsiSaveSeek);
		setBL(0x02025AF4, (u32)dsiSaveRead);
		setBL(0x02025B0C, (u32)dsiSaveClose);
		setBL(0x02025BD4, (u32)dsiSaveOpen);
		setBL(0x02025BFC, (u32)dsiSaveSeek);
		setBL(0x02025C10, (u32)dsiSaveWrite);
		setBL(0x02025C1C, (u32)dsiSaveClose);
		*(u32*)0x020503D4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02050F68, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02053B98 = 0xE1A00000; // nop
		*(u32*)0x02059874 = 0xE1A00000; // nop
		*(u32*)0x0205B878 = 0xE1A00000; // nop
		*(u32*)0x0205B87C = 0xE1A00000; // nop
		*(u32*)0x0205B888 = 0xE1A00000; // nop
		*(u32*)0x0205B9E8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0205BA44, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0205CFEC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0205D008 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205D00C = 0xE12FFF1E; // bx lr
		*(u32*)0x0205D014 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205D018 = 0xE12FFF1E; // bx lr
		*(u32*)0x02060568 = 0xE1A00000; // nop
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
		} else if (ndsHeader->gameCode[3] == 'P') {
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
		tonccpy((u32*)0x0201831C, dsiSaveGetResultCode, 0xC);
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
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x02065CA0, (u32)dsiSaveOpen);
			setBL(0x02065CB4, (u32)dsiSaveCreate);
			setBL(0x02065CC4, (u32)dsiSaveOpen);
			*(u32*)0x02065CE4 = 0xE1A00000; // nop
			setBL(0x02065CF0, (u32)dsiSaveCreate);
			setBL(0x02065D00, (u32)dsiSaveOpen);
			setBL(0x02066208, (u32)dsiSaveSeek);
			setBL(0x0206621C, (u32)dsiSaveWrite);
			setBL(0x0206622C, (u32)dsiSaveSeek);
			setBL(0x0206623C, (u32)dsiSaveWrite);
			setBL(0x0206624C, (u32)dsiSaveSeek);
			setBL(0x0206625C, (u32)dsiSaveWrite);
			setBL(0x020662C0, (u32)dsiSaveSeek);
			setBL(0x020662D4, (u32)dsiSaveWrite);
			setBL(0x020662DC, (u32)dsiSaveClose);
			setBL(0x02066330, (u32)dsiSaveOpen);
			setBL(0x02066648, (u32)dsiSaveSeek);
			setBL(0x02066658, (u32)dsiSaveRead);
			setBL(0x02066684, (u32)dsiSaveClose);
			setBL(0x02066BE4, (u32)dsiSaveOpen);
			setBL(0x02066BF8, (u32)dsiSaveSeek);
			setBL(0x02066C08, (u32)dsiSaveWrite);
			setBL(0x02066C10, (u32)dsiSaveClose);
		} else if (ndsHeader->gameCode[3] == 'J') {
			setBL(0x02026FA8, (u32)dsiSaveOpen);
			setBL(0x02026FC0, (u32)dsiSaveCreate);
			setBL(0x02026FD0, (u32)dsiSaveOpen);
			*(u32*)0x02026FF0 = 0xE1A00000; // nop
			setBL(0x02026FFC, (u32)dsiSaveCreate);
			setBL(0x0202700C, (u32)dsiSaveOpen);
			setBL(0x02027518, (u32)dsiSaveSeek);
			setBL(0x0202752C, (u32)dsiSaveWrite);
			setBL(0x0202753C, (u32)dsiSaveSeek);
			setBL(0x0202754C, (u32)dsiSaveWrite);
			setBL(0x0202755C, (u32)dsiSaveSeek);
			setBL(0x0202756C, (u32)dsiSaveWrite);
			setBL(0x020275E0, (u32)dsiSaveSeek);
			setBL(0x020275F4, (u32)dsiSaveWrite);
			setBL(0x020275FC, (u32)dsiSaveClose);
			setBL(0x02027654, (u32)dsiSaveOpen);
			setBL(0x020276C4, (u32)dsiSaveSeek);
			setBL(0x020276D4, (u32)dsiSaveRead);
			setBL(0x02027704, (u32)dsiSaveClose);
			setBL(0x02027D28, (u32)dsiSaveOpen);
			setBL(0x02027D3C, (u32)dsiSaveSeek);
			setBL(0x02027D4C, (u32)dsiSaveWrite);
			setBL(0x02027D54, (u32)dsiSaveClose);
		}
	}

	// Castle Conqueror: Heroes (Europe, Australia)
	else if (strcmp(romTid, "KC5V") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		tonccpy((u32*)0x02018248, dsiSaveGetResultCode, 0xC);
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
		setBL(0x02066110, (u32)dsiSaveOpen);
		setBL(0x02066128, (u32)dsiSaveCreate);
		setBL(0x02066138, (u32)dsiSaveOpen);
		*(u32*)0x02066158 = 0xE1A00000; // nop
		setBL(0x02066164, (u32)dsiSaveCreate);
		setBL(0x02066174, (u32)dsiSaveOpen);
		setBL(0x02066680, (u32)dsiSaveSeek);
		setBL(0x02066694, (u32)dsiSaveWrite);
		setBL(0x020666A4, (u32)dsiSaveSeek);
		setBL(0x020666B4, (u32)dsiSaveWrite);
		setBL(0x020666C4, (u32)dsiSaveSeek);
		setBL(0x020666D4, (u32)dsiSaveWrite);
		setBL(0x02066748, (u32)dsiSaveSeek);
		setBL(0x0206675C, (u32)dsiSaveWrite);
		setBL(0x02066764, (u32)dsiSaveClose);
		setBL(0x020667BC, (u32)dsiSaveOpen);
		setBL(0x0206682C, (u32)dsiSaveSeek);
		setBL(0x0206683C, (u32)dsiSaveRead);
		setBL(0x0206686C, (u32)dsiSaveClose);
		setBL(0x02066E90, (u32)dsiSaveOpen);
		setBL(0x02066EA4, (u32)dsiSaveSeek);
		setBL(0x02066EB4, (u32)dsiSaveWrite);
		setBL(0x02066EBC, (u32)dsiSaveClose);
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
		patchHiHeapDSiWare(0x0201EE90, 0x02700000); // mov r0, #0x2700000
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
		setBL(0x02005994, (u32)dsiSaveCreate);
		setBL(0x020059D0, (u32)dsiSaveOpen);
		setBL(0x02005A28, (u32)dsiSaveWrite);
		setBL(0x02005A40, (u32)dsiSaveClose);
		setBL(0x02005ADC, (u32)dsiSaveOpen);
		setBL(0x02005A28, (u32)dsiSaveWrite);
		setBL(0x02005AB0, (u32)dsiSaveOpen);
		setBL(0x02005ADC, (u32)dsiSaveOpen);
		setBL(0x02005AF8, (u32)dsiSaveGetLength);
		setBL(0x02005B0C, (u32)dsiSaveClose);
		setBL(0x02005B2C, (u32)dsiSaveSeek);
		setBL(0x02005B3C, (u32)dsiSaveRead);
		setBL(0x02005B44, (u32)dsiSaveClose);
		*(u32*)0x02005B88 = 0xE1A00000; // nop
		setBL(0x02005BAC, (u32)dsiSaveOpen);
		setBL(0x02005BD8, (u32)dsiSaveOpen);
		setBL(0x02005BF4, (u32)dsiSaveGetLength);
		setBL(0x02005C08, (u32)dsiSaveClose);
		setBL(0x02005C28, (u32)dsiSaveSeek);
		setBL(0x02005C38, (u32)dsiSaveWrite);
		setBL(0x02005C40, (u32)dsiSaveClose);
		*(u32*)0x02005C54 = 0xE1A00000; // nop
		*(u32*)0x02005C60 = 0xE1A00000; // nop
		*(u32*)0x02005C64 = 0xE1A00000; // nop
		*(u32*)0x0200A12C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0207342C = 0xE1A00000; // nop
		tonccpy((u32*)0x02073FA4, dsiSaveGetResultCode, 0xC);
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
		*(u32*)0x0201B9E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0201BBA4, (u32)dsiSaveCreate);
		setBL(0x0201BBB4, (u32)dsiSaveOpen);
		setBL(0x0201BBD0, (u32)dsiSaveSeek);
		setBL(0x0201BBE0, (u32)dsiSaveWrite);
		setBL(0x0201BBE8, (u32)dsiSaveClose);
		setBL(0x0201BD10, (u32)dsiSaveOpenR);
		setBL(0x0201BD28, (u32)dsiSaveSeek);
		setBL(0x0201BD38, (u32)dsiSaveRead);
		setBL(0x0201BD40, (u32)dsiSaveClose);
		*(u32*)0x0201BD60 = 0xE1A00000; // nop

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
		*(u32*)0x020459F0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02045BAC, (u32)dsiSaveCreate);
		setBL(0x02045BBC, (u32)dsiSaveOpen);
		setBL(0x02045BD8, (u32)dsiSaveSeek);
		setBL(0x02045BE8, (u32)dsiSaveWrite);
		setBL(0x02045BF0, (u32)dsiSaveClose);
		setBL(0x02045D1C, (u32)dsiSaveOpenR);
		setBL(0x02045D34, (u32)dsiSaveSeek);
		setBL(0x02045D44, (u32)dsiSaveRead);
		setBL(0x02045D4C, (u32)dsiSaveClose);
		*(u32*)0x02045D6C = 0xE1A00000; // nop
	}

	// Chuukara! Dairoujou (Japan)
	else if (strcmp(romTid, "KQLJ") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200DDFC = 0xE1A00000; // nop
		*(u32*)0x020115C8 = 0xE1A00000; // nop
		*(u32*)0x0201BD1C = 0xE1A00000; // nop
		*(u32*)0x0201DB98 = 0xE1A00000; // nop
		*(u32*)0x0201DB9C = 0xE1A00000; // nop
		*(u32*)0x0201DBA8 = 0xE1A00000; // nop
		*(u32*)0x0201DD08 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201DD64, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201F008 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201F024 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F028 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F030 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201F034 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022518 = 0xE1A00000; // nop
		setBL(0x020446E4, (u32)dsiSaveOpen);
		setBL(0x02044710, (u32)dsiSaveRead);
		setBL(0x02044720, (u32)dsiSaveClose);
		setBL(0x0204473C, (u32)dsiSaveClose);
		*(u32*)0x02044790 = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x020447CC = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x020447D8, (u32)dsiSaveCreate);
		setBL(0x020447E8, (u32)dsiSaveOpen);
		setBL(0x02044814, (u32)dsiSaveSetLength);
		setBL(0x02044824, (u32)dsiSaveClose);
		setBL(0x02044848, (u32)dsiSaveWrite);
		setBL(0x02044858, (u32)dsiSaveClose);
		setBL(0x02044874, (u32)dsiSaveClose);
	}

	// Clash of Elementalists (USA)
	// Clash of Elementalists (Europe)
	// Requires more than 8MB of RAM
	/*else if (strcmp(romTid, "KVLE") == 0 || strcmp(romTid, "KVLP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200C038 = 0xE1A00000; // nop
		*(u32*)0x0200C160 = 0xE1A00000; // nop
		*(u32*)0x0200C174 = 0xE1A00000; // nop
		*(u32*)0x0200F3C4 = 0xE1A00000; // nop
		*(u32*)0x020152B0 = 0xE1A00000; // nop
		*(u32*)0x02017208 = 0xE1A00000; // nop
		*(u32*)0x0201720C = 0xE1A00000; // nop
		*(u32*)0x02017218 = 0xE1A00000; // nop
		*(u32*)0x02017378 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020173D4, extendedMemory2 ? 0x02F80000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F80000 (mirrors to 0x2780000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02017508 -= 0x30000;
		*(u32*)0x0201875C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02018778 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201877C = 0xE12FFF1E; // bx lr
		*(u32*)0x02018784 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02018788 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201BB20 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0202627C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02028B6C = 0xE1A00000; // nop
			*(u32*)0x02028B70 = 0xE1A00000; // nop
			*(u32*)0x02028B88 = 0xE1A00000; // nop
			*(u32*)0x02028B90 = 0xE1A00000; // nop
			*(u32*)0x02028BB0 = 0xE1A00000; // nop
			*(u32*)0x0202B8CC = 0xE1A00000; // nop
			*(u32*)0x0202B8E8 = 0xE1A00000; // nop
		} else {
			*(u32*)0x02028C58 = 0xE1A00000; // nop
			*(u32*)0x02028C5C = 0xE1A00000; // nop
			*(u32*)0x02028C74 = 0xE1A00000; // nop
			*(u32*)0x02028C7C = 0xE1A00000; // nop
			*(u32*)0x02028C9C = 0xE1A00000; // nop
			*(u32*)0x0202BAC8 = 0xE1A00000; // nop
			*(u32*)0x0202BAE4 = 0xE1A00000; // nop
		}
	}*/

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

	// Crash-Course Domo (USA)
	else if (strcmp(romTid, "KDCE") == 0) {
		const u32 dsiSaveCreateT = 0x02024B0C;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02024B1C;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02024B2C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02024B3C;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02024B4C;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02024B5C;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02024B6C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0200DF38 = 0x2001; // movs r0, #1
		*(u16*)0x0200DF3A = 0x4770; // bx lr
		//doubleNopT(0x0200DF8A); // dsiSaveGetArcSrc
		doubleNopT(0x0200E1E6);
		*(u16*)0x0200E228 = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0200E22A = 0x4770; // bx lr
		setBLThumb(0x0200E28E, dsiSaveCreateT);
		setBLThumb(0x0200E2A4, dsiSaveOpenT);
		setBLThumb(0x0200E2C0, dsiSaveSetLengthT);
		setBLThumb(0x0200E2D4, dsiSaveWriteT);
		setBLThumb(0x0200E2E6, dsiSaveCloseT);
		*(u16*)0x0200E30C = 0x4778; // bx pc
		tonccpy((u32*)0x0200E310, dsiSaveGetLength, 0xC);
		setBLThumb(0x0200E33C, dsiSaveOpenT);
		setBLThumb(0x0200E362, dsiSaveCloseT);
		setBLThumb(0x0200E374, dsiSaveReadT);
		setBLThumb(0x0200E37A, dsiSaveCloseT);
		setBLThumb(0x0200E38E, dsiSaveDeleteT);
		*(u16*)0x020153C4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		*(u16*)0x02015418 = 0x46C0; // nop
		doubleNopT(0x02023C72);
		doubleNopT(0x02025EC2);
		doubleNopT(0x02028B44);
		doubleNopT(0x0202A0FE);
		doubleNopT(0x0202A102);
		doubleNopT(0x0202A10E);
		doubleNopT(0x0202A1F2);
		patchHiHeapDSiWareThumb(0x0202A230, 0x02024B7C, heapEnd); // movs r0, #0x23C0000
		doubleNopT(0x0202B2B6);
		*(u16*)0x0202B2BA = 0x46C0; // nop
		*(u16*)0x0202B2BC = 0x46C0; // nop
		doubleNopT(0x0202B2BE);
		doubleNopT(0x0202D372);
	}

	// CuteWitch! runner (USA)
	// CuteWitch! runner (Europe)
	// Stage music doesn't play on retail consoles
	else if (strncmp(romTid, "K32", 3) == 0) {
		u32* saveFuncOffsets[22] = {NULL};

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201B8CC = 0xE1A00000; // nop
		tonccpy((u32*)0x0201C450, dsiSaveGetResultCode, 0xC);
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
			*(u32*)0x020620D8 = 0xE1A00000; // nop
			*(u32*)0x02062118 = 0xE1A00000; // nop
			*(u32*)0x020621FC = 0xE1A00000; // nop
			*(u32*)0x02062204 = 0xE1A00000; // nop
			*(u32*)0x02062318 = 0xE1A00000; // nop
			*(u32*)0x02062394 = 0xE1A00000; // nop
			*(u32*)0x0206239C = 0xE1A00000; // nop
			*(u32*)0x02062428 = 0xE1A00000; // nop
			*(u32*)0x0206249C = 0xE1A00000; // nop
			*(u32*)0x020624A4 = 0xE1A00000; // nop
			*(u32*)0x02062530 = 0xE1A00000; // nop
			*(u32*)0x02062638 = 0xE1A00000; // nop
			*(u32*)0x0206263C = 0xE1A00000; // nop
			*(u32*)0x02062640 = 0xE1A00000; // nop
			*(u32*)0x020626F4 = 0xE1A00000; // nop
			*(u32*)0x020626FC = 0xE1A00000; // nop
			*(u32*)0x02062928 = 0xE1A00000; // nop
			*(u32*)0x02062A40 = 0xE1A00000; // nop
			*(u32*)0x02062A48 = 0xE1A00000; // nop
			*(u32*)0x02062C0C = 0xE1A00000; // nop
			*(u32*)0x02062C34 = 0xE1A00000; // nop
			*(u32*)0x02062C38 = 0xE1A00000; // nop
			*(u32*)0x02062C3C = 0xE1A00000; // nop
			saveFuncOffsets[0] = (u32*)0x02062EA4;
			saveFuncOffsets[1] = (u32*)0x02062EBC;
			saveFuncOffsets[2] = (u32*)0x02062ED0;
			saveFuncOffsets[3] = (u32*)0x02062EE8;
			saveFuncOffsets[4] = (u32*)0x02062EFC;
			saveFuncOffsets[5] = (u32*)0x02062F14;
			saveFuncOffsets[6] = (u32*)0x02062F28;
			saveFuncOffsets[7] = (u32*)0x02062F38;
			saveFuncOffsets[8] = (u32*)0x02062FA8;
			saveFuncOffsets[9] = (u32*)0x02062FC0;
			saveFuncOffsets[10] = (u32*)0x02062FD4;
			saveFuncOffsets[11] = (u32*)0x02062FEC;
			saveFuncOffsets[12] = (u32*)0x02063000;
			saveFuncOffsets[13] = (u32*)0x02063018;
			saveFuncOffsets[14] = (u32*)0x0206302C;
			saveFuncOffsets[15] = (u32*)0x0206303C;
			saveFuncOffsets[16] = (u32*)0x020630AC;
			saveFuncOffsets[17] = (u32*)0x020630E0;
			saveFuncOffsets[18] = (u32*)0x02063100;
			saveFuncOffsets[19] = (u32*)0x02063108;
			saveFuncOffsets[20] = (u32*)0x02063168;
			saveFuncOffsets[21] = (u32*)0x02063180;
		} else if (ndsHeader->gameCode[3] == 'P') {
			*(u32*)0x02093B14 = 0xE1A00000; // nop
			*(u32*)0x02093B54 = 0xE1A00000; // nop
			*(u32*)0x02093C38 = 0xE1A00000; // nop
			*(u32*)0x02093C40 = 0xE1A00000; // nop
			*(u32*)0x02093D54 = 0xE1A00000; // nop
			*(u32*)0x02093DD0 = 0xE1A00000; // nop
			*(u32*)0x02093DD8 = 0xE1A00000; // nop
			*(u32*)0x02093E64 = 0xE1A00000; // nop
			*(u32*)0x02093ED8 = 0xE1A00000; // nop
			*(u32*)0x02093EE0 = 0xE1A00000; // nop
			*(u32*)0x02093F6C = 0xE1A00000; // nop
			*(u32*)0x02094074 = 0xE1A00000; // nop
			*(u32*)0x02094078 = 0xE1A00000; // nop
			*(u32*)0x0209407C = 0xE1A00000; // nop
			*(u32*)0x02094130 = 0xE1A00000; // nop
			*(u32*)0x02094138 = 0xE1A00000; // nop
			*(u32*)0x02094364 = 0xE1A00000; // nop
			*(u32*)0x0209447C = 0xE1A00000; // nop
			*(u32*)0x02094480 = 0xE1A00000; // nop
			*(u32*)0x02094484 = 0xE1A00000; // nop
			*(u32*)0x02094648 = 0xE1A00000; // nop
			*(u32*)0x02094670 = 0xE1A00000; // nop
			*(u32*)0x02094674 = 0xE1A00000; // nop
			*(u32*)0x02094678 = 0xE1A00000; // nop
			saveFuncOffsets[0] = (u32*)0x020948E0;
			saveFuncOffsets[1] = (u32*)0x020948F8;
			saveFuncOffsets[2] = (u32*)0x0209490C;
			saveFuncOffsets[3] = (u32*)0x02094924;
			saveFuncOffsets[4] = (u32*)0x02094938;
			saveFuncOffsets[5] = (u32*)0x02094950;
			saveFuncOffsets[6] = (u32*)0x02094964;
			saveFuncOffsets[7] = (u32*)0x02094974;
			saveFuncOffsets[8] = (u32*)0x020949E4;
			saveFuncOffsets[9] = (u32*)0x020949FC;
			saveFuncOffsets[10] = (u32*)0x02094A10;
			saveFuncOffsets[11] = (u32*)0x02094A28;
			saveFuncOffsets[12] = (u32*)0x02094A3C;
			saveFuncOffsets[13] = (u32*)0x02094A54;
			saveFuncOffsets[14] = (u32*)0x02094A68;
			saveFuncOffsets[15] = (u32*)0x02094A78;
			saveFuncOffsets[16] = (u32*)0x02094AE8;
			saveFuncOffsets[17] = (u32*)0x02094B1C;
			saveFuncOffsets[18] = (u32*)0x02094B3C;
			saveFuncOffsets[19] = (u32*)0x02094B44;
			saveFuncOffsets[20] = (u32*)0x02094BA4;
			saveFuncOffsets[21] = (u32*)0x02094BBC;
		}

		setBL((u32)saveFuncOffsets[0], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[1], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[2], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[3], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[4], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[5], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[6], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[7], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[8], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[9], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[10], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[11], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[12], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[13], (u32)dsiSaveRead);
		setBL((u32)saveFuncOffsets[14], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[15], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[16], (u32)dsiSaveCreate);
		setBL((u32)saveFuncOffsets[17], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[18], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[19], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[20], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[21], (u32)dsiSaveClose);
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
		*(u32*)0x0201EFC0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201EFDC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EFE0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EFE8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EFEC = 0xE12FFF1E; // bx lr
		*(u32*)0x02022418 = 0xE1A00000; // nop
		setBL(0x02044B3C, (u32)dsiSaveOpen);
		setBL(0x02044B68, (u32)dsiSaveRead);
		setBL(0x02044B78, (u32)dsiSaveClose);
		setBL(0x02044B94, (u32)dsiSaveClose);
		*(u32*)0x02044BE8 = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x02044C24 = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x02044C30, (u32)dsiSaveCreate);
		setBL(0x02044C40, (u32)dsiSaveOpen);
		setBL(0x02044C6C, (u32)dsiSaveSetLength);
		setBL(0x02044C7C, (u32)dsiSaveClose);
		setBL(0x02044CA0, (u32)dsiSaveWrite);
		setBL(0x02044CB0, (u32)dsiSaveClose);
		setBL(0x02044CCC, (u32)dsiSaveClose);
	}

	// Karakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF3J") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200DD9C = 0xE1A00000; // nop
		*(u32*)0x020114D4 = 0xE1A00000; // nop
		*(u32*)0x0201BBF8 = 0xE1A00000; // nop
		*(u32*)0x0201DA6C = 0xE1A00000; // nop
		*(u32*)0x0201DA70 = 0xE1A00000; // nop
		*(u32*)0x0201DA7C = 0xE1A00000; // nop
		*(u32*)0x0201DBDC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201DC38, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201EECC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201EEE8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EEEC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EEF4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EEF8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022324 = 0xE1A00000; // nop
		setBL(0x02044680, (u32)dsiSaveOpen);
		setBL(0x020446AC, (u32)dsiSaveRead);
		setBL(0x020446BC, (u32)dsiSaveClose);
		setBL(0x020446D8, (u32)dsiSaveClose);
		*(u32*)0x0204472C = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x02044768 = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x02044774, (u32)dsiSaveCreate);
		setBL(0x02044784, (u32)dsiSaveOpen);
		setBL(0x020447B0, (u32)dsiSaveSetLength);
		setBL(0x020447C0, (u32)dsiSaveClose);
		setBL(0x020447E4, (u32)dsiSaveWrite);
		setBL(0x020447F4, (u32)dsiSaveClose);
		setBL(0x02044810, (u32)dsiSaveClose);
	}

	// Dark Void Zero (USA)
	// Dark Void Zero (Europe, Australia)
	else if (strcmp(romTid, "KDVE") == 0 || strcmp(romTid, "KDVV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02018A3C = 0xE1A00000; // nop
		*(u32*)0x02018A4C = 0xE1A00000; // nop
		tonccpy((u32*)0x02043DDC, dsiSaveGetResultCode, 0xC);
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
		//*(u32*)0x0208AE4C = 0xE12FFF1E; // bx lr
		//*(u32*)0x0208B008 = 0xE12FFF1E; // bx lr
		*(u32*)0x0208AE6C = 0xE1A00000; // nop
		setBL(0x0208AE90, (u32)dsiSaveOpen);
		setBL(0x0208AEA4, (u32)dsiSaveCreate);
		setBL(0x0208AEC4, (u32)dsiSaveOpen);
		setBL(0x0208AEE8, (u32)dsiSaveWrite);
		setBL(0x0208AF08, (u32)dsiSaveWrite);
		setBL(0x0208AF34, (u32)dsiSaveWrite);
		setBL(0x0208AF50, (u32)dsiSaveWrite);
		setBL(0x0208AF6C, (u32)dsiSaveWrite);
		setBL(0x0208AF7C, (u32)dsiSaveWrite);
		setBL(0x0208AF8C, (u32)dsiSaveWrite);
		setBL(0x0208AF9C, (u32)dsiSaveWrite);
		setBL(0x0208AFAC, (u32)dsiSaveWrite);
		setBL(0x0208AFB4, (u32)dsiSaveClose);
		*(u32*)0x0208AFD0 = 0xE1A00000; // nop
		*(u32*)0x0208AFD8 = 0xE1A00000; // nop
		setBL(0x0208B04C, (u32)dsiSaveOpen);
		setBL(0x0208B0DC, (u32)dsiSaveRead);
		setBL(0x0208B134, (u32)dsiSaveClose);
		setBL(0x0208B174, (u32)dsiSaveDelete);
		setBL(0x0208B1BC, (u32)dsiSaveRead);
		setBL(0x0208B234, (u32)dsiSaveRead);
		setBL(0x0208B290, (u32)dsiSaveRead);
		setBL(0x0208B2F4, (u32)dsiSaveRead);
		setBL(0x0208B340, (u32)dsiSaveRead);
		setBL(0x0208B38C, (u32)dsiSaveRead);
		setBL(0x0208B3D8, (u32)dsiSaveRead);
		setBL(0x0208B424, (u32)dsiSaveRead);
		setBL(0x0208B488, (u32)dsiSaveClose);
		setBL(0x0208B4C8, (u32)dsiSaveDelete);
		setBL(0x0208B50C, (u32)dsiSaveClose);
	}

	// GO Series: Defense Wars (USA)
	// GO Series: Defence Wars (Europe)
	else if (strcmp(romTid, "KWTE") == 0 || strcmp(romTid, "KWTP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200722C = 0xE1A00000; // nop
		*(u32*)0x0200B350 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0200C5C0, (u32)dsiSaveCreate);
		setBL(0x0200C5FC, (u32)dsiSaveOpen);
		setBL(0x0200C634, (u32)dsiSaveSetLength);
		setBL(0x0200C644, (u32)dsiSaveWrite);
		setBL(0x0200C65C, (u32)dsiSaveClose);
		setBL(0x0200C6E4, (u32)dsiSaveOpen);
		setBL(0x0200C71C, (u32)dsiSaveSetLength);
		setBL(0x0200C72C, (u32)dsiSaveWrite);
		setBL(0x0200C744, (u32)dsiSaveClose);
		setBL(0x0200C7C4, (u32)dsiSaveOpen);
		setBL(0x0200C7FC, (u32)dsiSaveRead);
		setBL(0x0200C810, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveDelete);
		setBL(0x0200C8CC, (u32)dsiSaveGetInfo);
		*(u32*)0x0200C910 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0200C914 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x02044AEC, dsiSaveGetResultCode, 0xC);
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
		*(u32*)0x02005358 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		*(u32*)0x02005370 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		*(u32*)0x02005358 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		tonccpy((u32*)0x02011160, dsiSaveGetResultCode, 0xC);
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
		*(u32*)0x0203D228 = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
		//*(u32*)0x0203D488 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203D48C = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[3] == 'E') {
			//*(u32*)0x02044B00 = 0xE3A00000; // mov r0, #0
			setBL(0x020590C0, (u32)dsiSaveCreate);
			setBL(0x02059270, (u32)dsiSaveOpen);
			setBL(0x020593CC, (u32)dsiSaveClose);
			setBL(0x020594E8, (u32)dsiSaveSeek);
			setBL(0x020594F8, (u32)dsiSaveRead);
			setBL(0x02059674, (u32)dsiSaveSeek);
			setBL(0x02059684, (u32)dsiSaveWrite);
			setBL(0x020597FC, (u32)dsiSaveOpenR);
			setBL(0x020598A0, (u32)dsiSaveClose);
			*(u32*)0x02059920 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206F430 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0207347C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020736DC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0207401C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02074054 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		} else {
			//*(u32*)0x02044A9C = 0xE3A00000; // mov r0, #0
			setBL(0x02058FB0, (u32)dsiSaveCreate);
			setBL(0x02059160, (u32)dsiSaveOpen);
			setBL(0x020592BC, (u32)dsiSaveClose);
			setBL(0x020593D8, (u32)dsiSaveSeek);
			setBL(0x020593E8, (u32)dsiSaveRead);
			setBL(0x02059564, (u32)dsiSaveSeek);
			setBL(0x02059574, (u32)dsiSaveWrite);
			setBL(0x020596EC, (u32)dsiSaveOpenR);
			setBL(0x02059790, (u32)dsiSaveClose);
			*(u32*)0x02059810 = 0xE3A00000; // mov r0, #0
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
		tonccpy((u32*)0x020118A4, dsiSaveGetResultCode, 0xC);
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
		*(u32*)0x0205824C = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
		//*(u32*)0x020584B4 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020584B8 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0205F6F0 = 0xE3A00000; // mov r0, #0
		setBL(0x020736C4, (u32)dsiSaveCreate);
		setBL(0x02073874, (u32)dsiSaveOpen);
		setBL(0x020739D0, (u32)dsiSaveClose);
		setBL(0x02073AEC, (u32)dsiSaveSeek);
		setBL(0x02073AFC, (u32)dsiSaveRead);
		setBL(0x02073C78, (u32)dsiSaveSeek);
		setBL(0x02073C88, (u32)dsiSaveWrite);
		setBL(0x02073E00, (u32)dsiSaveOpenR);
		setBL(0x02073EA4, (u32)dsiSaveClose);
		*(u32*)0x02073F24 = 0xE3A00000; // mov r0, #0
	}

	// Dragon's Lair (USA)
	else if (strcmp(romTid, "KDLE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop (Skip Manual screen)
		//*(u32*)0x02012064 = 0xE1A00000; // nop
		//*(u32*)0x02012068 = 0xE1A00000; // nop
		/*for (int i = 0; i < 5; i++) {
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
		}*/
		*(u32*)0x0201B894 = 0xE1A00000; // nop
		setBL(0x0201B8E8, (u32)dsiSaveOpen);
		setBL(0x0201B900, (u32)dsiSaveRead);
		setBL(0x0201B928, (u32)dsiSaveClose);
		setBL(0x0201B98C, (u32)dsiSaveCreate);
		setBL(0x0201B9BC, (u32)dsiSaveOpen);
		setBL(0x0201B9EC, (u32)dsiSaveWrite);
		setBL(0x0201BA14, (u32)dsiSaveClose);
		setBL(0x0201BAB4, (u32)dsiSaveOpen);
		setBL(0x0201BAFC, (u32)dsiSaveSeek);
		setBL(0x0201BB2C, (u32)dsiSaveWrite);
		setBL(0x0201BB54, (u32)dsiSaveClose);
		setBL(0x0201BBAC, (u32)dsiSaveGetResultCode);
		setBL(0x0201BBE8, (u32)dsiSaveClose);
		setBL(0x0201BC00, (u32)dsiSaveClose);
		*(u32*)0x0201BC18 = 0xE1A00000; // nop
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
		*(u32*)0x020051E4 = 0xE1A00000; // nop (Skip Manual screen)
		//*(u32*)0x0201205C = 0xE1A00000; // nop
		//*(u32*)0x02012060 = 0xE1A00000; // nop
		/*for (int i = 0; i < 5; i++) {
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
		}*/
		*(u32*)0x0201B888 = 0xE1A00000; // nop
		setBL(0x0201B8DC, (u32)dsiSaveOpen);
		setBL(0x0201B8F4, (u32)dsiSaveRead);
		setBL(0x0201B91C, (u32)dsiSaveClose);
		setBL(0x0201B980, (u32)dsiSaveCreate);
		setBL(0x0201B9B0, (u32)dsiSaveOpen);
		setBL(0x0201B9E0, (u32)dsiSaveWrite);
		setBL(0x0201BA08, (u32)dsiSaveClose);
		setBL(0x0201BAA8, (u32)dsiSaveOpen);
		setBL(0x0201BAF0, (u32)dsiSaveSeek);
		setBL(0x0201BB20, (u32)dsiSaveWrite);
		setBL(0x0201BB48, (u32)dsiSaveClose);
		setBL(0x0201BBA0, (u32)dsiSaveGetResultCode);
		setBL(0x0201BBDC, (u32)dsiSaveClose);
		setBL(0x0201BBF4, (u32)dsiSaveClose);
		*(u32*)0x0201BC0C = 0xE1A00000; // nop
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
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		//*(u32*)0x020171CC = 0xE1A00000; // nop
		//*(u32*)0x020171D0 = 0xE1A00000; // nop
		*(u32*)0x0201FFEC = 0xE1A00000; // nop
		setBL(0x02020034, (u32)dsiSaveOpen);
		setBL(0x0202004C, (u32)dsiSaveRead);
		setBL(0x02020074, (u32)dsiSaveClose);
		*(u32*)0x020200D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020104 = 0xE1A00000; // nop
		setBL(0x02020110, (u32)dsiSaveCreate);
		setBL(0x02020140, (u32)dsiSaveOpen);
		setBL(0x02020170, (u32)dsiSaveWrite);
		setBL(0x02020198, (u32)dsiSaveClose);
		*(u32*)0x020201CC = 0xE1A00000; // nop
		*(u32*)0x020201D8 = 0xE1A00000; // nop
		*(u32*)0x02020238 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020264 = 0xE1A00000; // nop
		setBL(0x02020274, (u32)dsiSaveOpen);
		setBL(0x020202B0, (u32)dsiSaveSeek);
		setBL(0x020202E0, (u32)dsiSaveWrite);
		setBL(0x02020308, (u32)dsiSaveClose);
		*(u32*)0x0202033C = 0xE1A00000; // nop
		*(u32*)0x02020348 = 0xE1A00000; // nop
		setBL(0x02020374, (u32)dsiSaveGetResultCode);
		setBL(0x020203A4, (u32)dsiSaveClose);
		setBL(0x020203BC, (u32)dsiSaveClose);
		*(u32*)0x020203C8 = 0xE1A00000; // nop
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
		*(u32*)0x020051E0 = 0xE1A00000; // nop (Skip Manual screen)
		//*(u32*)0x020171E8 = 0xE1A00000; // nop
		//*(u32*)0x020171EC = 0xE1A00000; // nop
		*(u32*)0x02020004 = 0xE1A00000; // nop
		setBL(0x0202004C, (u32)dsiSaveOpen);
		setBL(0x02020064, (u32)dsiSaveRead);
		setBL(0x0202008C, (u32)dsiSaveClose);
		*(u32*)0x020200F0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202011C = 0xE1A00000; // nop
		setBL(0x02020128, (u32)dsiSaveCreate);
		setBL(0x02020158, (u32)dsiSaveOpen);
		setBL(0x02020188, (u32)dsiSaveWrite);
		setBL(0x020201B0, (u32)dsiSaveClose);
		*(u32*)0x020201E4 = 0xE1A00000; // nop
		*(u32*)0x020201F0 = 0xE1A00000; // nop
		*(u32*)0x02020250 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202027C = 0xE1A00000; // nop
		setBL(0x0202028C, (u32)dsiSaveOpen);
		setBL(0x020202C8, (u32)dsiSaveSeek);
		setBL(0x020202F8, (u32)dsiSaveWrite);
		setBL(0x02020320, (u32)dsiSaveClose);
		*(u32*)0x02020354 = 0xE1A00000; // nop
		*(u32*)0x02020360 = 0xE1A00000; // nop
		setBL(0x0202038C, (u32)dsiSaveGetResultCode);
		setBL(0x020203BC, (u32)dsiSaveClose);
		setBL(0x020203D4, (u32)dsiSaveClose);
		*(u32*)0x020203DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020410 = 0xE1A00000; // nop
		*(u32*)0x02020418 = 0xE1A00000; // nop
		setBL(0x02020424, (u32)dsiSaveCreate);
		*(u32*)0x02020438 = 0xE1A00000; // nop
		*(u32*)0x02020444 = 0xE1A00000; // nop
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

	// Dragon Quest Wars (USA)
	// DSi save function patching not needed
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDQE") == 0 && extendedMemory2) {
		*(u32*)0x0201F208 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020A82EC = 0xE1A00000; // nop
		*(u32*)0x020AC120 = 0xE1A00000; // nop
		*(u32*)0x020B3598 = 0xE1A00000; // nop
		*(u32*)0x020B66CC = 0xE1A00000; // nop
		*(u32*)0x020B66D0 = 0xE1A00000; // nop
		*(u32*)0x020B66DC = 0xE1A00000; // nop
		*(u32*)0x020B6820 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020B687C, 0x02700000); // mov r0, #0x2700000
		//*(u32*)0x020B69B0 = 0x022A83C0;
		*(u32*)0x020B7BF0 = 0xE1A00000; // nop
		*(u32*)0x020B7BF8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020B7C18 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7C1C = 0xE12FFF1E; // bx lr
		*(u32*)0x020B7C6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B7C70 = 0xE12FFF1E; // bx lr
		*(u32*)0x020BB5AC = 0xE1A00000; // nop
	}

	// Dragon Quest Wars (Europe, Australia)
	// DSi save function patching not needed
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDQV") == 0 && extendedMemory2) {
		*(u32*)0x0201F250 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020A8334 = 0xE1A00000; // nop
		*(u32*)0x020AC168 = 0xE1A00000; // nop
		*(u32*)0x020B35E0 = 0xE1A00000; // nop
		*(u32*)0x020B6714 = 0xE1A00000; // nop
		*(u32*)0x020B6718 = 0xE1A00000; // nop
		*(u32*)0x020B6724 = 0xE1A00000; // nop
		*(u32*)0x020B6868 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020B68C4, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020B7C38 = 0xE1A00000; // nop
		*(u32*)0x020B7C40 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020B7C60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7C64 = 0xE12FFF1E; // bx lr
		*(u32*)0x020B7CB4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B7CB8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020BB5F4 = 0xE1A00000; // nop
	}

	// Dragon Quest Wars (Japan)
	// DSi save function patching not needed
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDQJ") == 0 && extendedMemory2) {
		*(u32*)0x0201EF84 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020A7CAC = 0xE1A00000; // nop
		*(u32*)0x020ABAE0 = 0xE1A00000; // nop
		*(u32*)0x020B2F58 = 0xE1A00000; // nop
		*(u32*)0x020B608C = 0xE1A00000; // nop
		*(u32*)0x020B6090 = 0xE1A00000; // nop
		*(u32*)0x020B609C = 0xE1A00000; // nop
		*(u32*)0x020B61E0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020B623C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020B75B0 = 0xE1A00000; // nop
		*(u32*)0x020B75B8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020B75D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B75DC = 0xE12FFF1E; // bx lr
		*(u32*)0x020B762C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B7630 = 0xE12FFF1E; // bx lr
		*(u32*)0x020BAF6C = 0xE1A00000; // nop
	}

	// Dreamwalker (USA)
	else if (strcmp(romTid, "K9EE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02029610 = 0xE1A00000; // nop
		setBL(0x0202963C, (u32)dsiSaveOpen);
		setBL(0x02029654, (u32)dsiSaveRead);
		setBL(0x0202967C, (u32)dsiSaveClose);
		*(u32*)0x020296E0 = 0xE1A00000; // nop
		*(u32*)0x020296E8 = 0xE1A00000; // nop
		setBL(0x020296F4, (u32)dsiSaveCreate);
		setBL(0x02029724, (u32)dsiSaveOpen);
		setBL(0x02029754, (u32)dsiSaveWrite);
		setBL(0x0202977C, (u32)dsiSaveClose);
		*(u32*)0x020297B0 = 0xE1A00000; // nop
		*(u32*)0x020297B8 = 0xE1A00000; // nop
		*(u32*)0x0202981C = 0xE1A00000; // nop
		*(u32*)0x0202982C = 0xE1A00000; // nop
		setBL(0x0202983C, (u32)dsiSaveOpen);
		setBL(0x02029878, (u32)dsiSaveSeek);
		setBL(0x020298A8, (u32)dsiSaveWrite);
		setBL(0x020298D0, (u32)dsiSaveClose);
		*(u32*)0x02029904 = 0xE1A00000; // nop
		*(u32*)0x0202990C = 0xE1A00000; // nop
		setBL(0x02029938, (u32)dsiSaveGetResultCode);
		setBL(0x0202996C, (u32)dsiSaveClose);
		setBL(0x02029984, (u32)dsiSaveClose);
		*(u32*)0x02029990 = 0xE1A00000; // nop
		setBL(0x020299DC, (u32)dsiSaveSeek);
		setBL(0x020299F0, (u32)dsiSaveWrite);
		*(u32*)0x0207DB84 = 0xE1A00000; // nop
		*(u32*)0x02080F88 = 0xE1A00000; // nop
		*(u32*)0x020856D0 = 0xE1A00000; // nop
		*(u32*)0x02087490 = 0xE1A00000; // nop
		*(u32*)0x02087494 = 0xE1A00000; // nop
		*(u32*)0x020874A0 = 0xE1A00000; // nop
		*(u32*)0x02087600 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0208765C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02088988 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0208BFD8 = 0xE1A00000; // nop
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
		//*(u32*)0x02005534 = 0xE1A00000; // nop
		*(u32*)0x0200A3D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		//*(u32*)0x0200A898 = 0xE12FFF1E; // bx lr
		setBL(0x0200AC14, (u32)dsiSaveOpen);
		setBL(0x0200AC50, (u32)dsiSaveRead);
		setBL(0x0200AC70, (u32)dsiSaveClose);
		setBL(0x0200AD0C, (u32)dsiSaveCreate);
		setBL(0x0200AD4C, (u32)dsiSaveOpen);
		setBL(0x0200AD84, (u32)dsiSaveSetLength);
		setBL(0x0200ADA0, (u32)dsiSaveWrite);
		setBL(0x0200ADC4, (u32)dsiSaveClose);
		setBL(0x0200AE58, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B800 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02014AB0 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		*(u32*)0x02036398 = 0xE1A00000; // nop
		*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0204BFE8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0204CB6C, dsiSaveGetResultCode, 0xC);
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

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014BEC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// GO Series: Earth Saver (Europe)
	else if (strcmp(romTid, "KB8P") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x0200A310 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0200AB24, (u32)dsiSaveOpen);
		setBL(0x0200AB60, (u32)dsiSaveRead);
		setBL(0x0200AB80, (u32)dsiSaveClose);
		setBL(0x0200AC1C, (u32)dsiSaveCreate);
		setBL(0x0200AC5C, (u32)dsiSaveOpen);
		setBL(0x0200AC94, (u32)dsiSaveSetLength);
		setBL(0x0200ACB0, (u32)dsiSaveWrite);
		setBL(0x0200ACD4, (u32)dsiSaveClose);
		setBL(0x0200AD68, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B710 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020149B4 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		*(u32*)0x02036394 = 0xE1A00000; // nop
		*(u32*)0x02047D50 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204BEEC = 0xE1A00000; // nop
		tonccpy((u32*)0x0204CA70, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204F44C = 0xE1A00000; // nop
		*(u32*)0x02054344 = 0xE1A00000; // nop
		*(u32*)0x0205612C = 0xE1A00000; // nop
		*(u32*)0x02056130 = 0xE1A00000; // nop
		*(u32*)0x0205613C = 0xE1A00000; // nop
		*(u32*)0x0205629C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020562F8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0205757C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020579B0 = 0xE1A00000; // nop
		*(u32*)0x020579B4 = 0xE1A00000; // nop
		*(u32*)0x020579B8 = 0xE1A00000; // nop
		*(u32*)0x020579BC = 0xE1A00000; // nop
		*(u32*)0x0205AAFC = 0xE1A00000; // nop

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014AF0;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Famicom Wars DS: Ushinawareta Hikari (Japan)
	// DSi save function patching not needed 
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
		*(u32*)0x02022C28 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C2C = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C4C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C50 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C64 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C70 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02022C74 = 0xE12FFF1E; // bx lr
		*(u32*)0x02026A94 = 0xE1A00000; // nop
	}

	// Fashion Tycoon (USA)
	// Saving not supported due to some weirdness with the code going on
	else if (strcmp(romTid, "KU7E") == 0) {
		/* const u32 dsiSaveCreateT = 0x020370F4;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC); // Original function overwritten, no BL setting needed

		const u32 dsiSaveSetLengthT = 0x02037104;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02037114;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveOpenRT = 0x02037124;
		*(u16*)dsiSaveOpenRT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenRT + 4), dsiSaveOpenR, 0x10);

		const u32 dsiSaveCloseT = 0x02037138;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02037148;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02037300;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020372F0;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC); // Original function overwritten, no BL setting needed */

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u16*)0x020271CC = 0x2001; // movs r0, #1
		*(u16*)0x020271CE = 0x4770; // bx lr
		*(u16*)0x02027A40 = 0x2001; // movs r0, #1
		*(u16*)0x02027A42 = 0x4770; // bx lr
		/* setBLThumb(0x020271F2, dsiSaveOpenRT);
		setBLThumb(0x020271FC, 0x020274A6);
		setBLThumb(0x020274C2, dsiSaveOpenT);
		setBLThumb(0x020274D6, dsiSaveSeekT);
		setBLThumb(0x020274E2, dsiSaveReadT);
		doubleNopT(0x020274F4);
		setBLThumb(0x02027500, dsiSaveCloseT);
		setBLThumb(0x02027828, dsiSaveOpenT);
		setBLThumb(0x02027838, dsiSaveSetLengthT);
		doubleNopT(0x0202783E); // dsiSaveFlush
		setBLThumb(0x02027844, dsiSaveCloseT);
		setBLThumb(0x0202784E, dsiSaveCloseT);
		setBLThumb(0x02027A6E, dsiSaveOpenRT);
		setBLThumb(0x02027A78, 0x02027D3E);
		setBLThumb(0x02027DF8, dsiSaveSetLengthT);
		doubleNopT(0x02027DFE); // dsiSaveFlush
		setBLThumb(0x02027E04, dsiSaveCloseT);
		setBLThumb(0x02027E2C, dsiSaveSeekT);
		setBLThumb(0x02027E36, dsiSaveReadT);
		doubleNopT(0x02027E4C);
		setBLThumb(0x02027E5E, dsiSaveCloseT);
		setBLThumb(0x02028170, dsiSaveOpenT);
		setBLThumb(0x02028198, dsiSaveSetLengthT);
		doubleNopT(0x0202819E); // dsiSaveFlush
		setBLThumb(0x020281A4, dsiSaveCloseT);
		setBLThumb(0x020281C6, dsiSaveCloseT);
		setBLThumb(0x02028200, dsiSaveOpenT);
		setBLThumb(0x02028216, dsiSaveSeekT);
		setBLThumb(0x02028220, dsiSaveReadT); // dsiSaveReadAsync
		doubleNopT(0x02028230); // dsiSaveFlush
		setBLThumb(0x02028236, dsiSaveCloseT); */
		doubleNopT(0x0203633E);
		//*(u16*)0x0203728C = 0x2000; // movs r0, #0 (dsiSaveOpenDir)
		//*(u16*)0x0203728E = 0x4770; // bx lr
		//*(u16*)0x020372D8 = 0x2000; // movs r0, #0 (dsiSaveCloseDir)
		//*(u16*)0x020372DA = 0x4770; // bx lr
		doubleNopT(0x02038386);
		doubleNopT(0x0203AC10);
		doubleNopT(0x0203C232);
		doubleNopT(0x0203C236);
		doubleNopT(0x0203C242);
		doubleNopT(0x0203C326);
		patchHiHeapDSiWareThumb(0x0203C364, 0x0203A144, heapEnd); // movs r0, #0x23C0000
		*(u16*)0x0203D182 = 0x46C0; // nop
		*(u16*)0x0203D186 = 0xBD38; // POP {R3-R5,PC}
		doubleNopT(0x0203F052);
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
		tonccpy((u32*)0x0201234C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020152F0 = 0xE1A00000; // nop
		*(u32*)0x0201A220 = 0xE1A00000; // nop
		*(u32*)0x0201C040 = 0xE1A00000; // nop
		*(u32*)0x0201C044 = 0xE1A00000; // nop
		*(u32*)0x0201C050 = 0xE1A00000; // nop
		*(u32*)0x0201C1B0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C20C, extendedMemory2 ? 0x02700000 : heapEnd+0x400000); // mov r0, extendedMemory2 ? #0x2700000 : #0x27C0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x02020C60 = 0xE1A00000; // nop
		*(u32*)0x020381BC = 0xE1A00000; // nop
		setBL(0x02038250, (u32)dsiSaveGetInfo);
		setBL(0x02038294, (u32)dsiSaveOpen);
		setBL(0x020382B0, (u32)dsiSaveRead);
		setBL(0x020382BC, (u32)dsiSaveClose);
		setBL(0x020383F0, (u32)dsiSaveGetInfo);
		setBL(0x02038418, (u32)dsiSaveDelete);
		setBL(0x02038424, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02038454, (u32)dsiSaveOpen);
		setBL(0x02038470, (u32)dsiSaveWrite);
		setBL(0x02038478, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x0204B968 = 0xE1A00000; // nop
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0204B98C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Game & Watch: Ball (USA, Europe)
	// Softlocks after a miss or exiting gameplay
	// Save code seems confusing to patch, preventing support
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
		/* setBL(0x02033ABC, (u32)dsiSaveCreate);
		setBL(0x02033AF8, (u32)dsiSaveOpen);
		setBL(0x02033B38, (u32)dsiSaveClose);
		setBL(0x02033D60, (u32)dsiSaveOpen); */
		*(u32*)0x02035078 = 0xE12FFF1E; // bx lr
		/* *(u32*)0x0203E124 = 0xE1A00000; // nop
		*(u32*)0x0203E148 = 0xE1A00000; // nop
		*(u32*)0x0203E150 = 0xE1A00000; // nop
		*(u32*)0x0203E180 = 0xE1A00000; // nop
		*(u32*)0x0203E188 = 0xE1A00000; // nop
		*(u32*)0x0203E19C = 0xE1A00000; // nop
		setBL(0x0203E160, (u32)dsiSaveWrite);
		setBL(0x0203E178, (u32)dsiSaveWrite);
		setBL(0x0203E1E8, (u32)dsiSaveRead); */
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
		if (ndsHeader->gameCode[3] == 'E') {
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

	// Goooooal America (USA)
	// Crash cause unknown
	/*else if (strcmp(romTid, "K9AE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201CE24 = 0xE1A00000; // nop
		*(u32*)0x02021154 = 0xE1A00000; // nop
		*(u32*)0x02024C14 = 0xE1A00000; // nop
		*(u32*)0x020269DC = 0xE1A00000; // nop
		*(u32*)0x020269E0 = 0xE1A00000; // nop
		*(u32*)0x020269EC = 0xE1A00000; // nop
		*(u32*)0x02026B4C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02026BA8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02027F34 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202B42C = 0xE1A00000; // nop
	}*/

	// Go! Go! Kokopolo (USA)
	// Go! Go! Kokopolo (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "K3GE") == 0 || strcmp(romTid, "K3GP") == 0) && extendedMemory2) {
		const u32 readCodeCopy = 0x02013CF4;

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02012738 = 0xE1A00000; // nop
		*(u32*)0x02015C98 = 0xE1A00000; // nop
		*(u32*)0x0201A10C = 0xE1A00000; // nop
		*(u32*)0x0201C014 = 0xE1A00000; // nop
		*(u32*)0x0201C018 = 0xE1A00000; // nop
		*(u32*)0x0201C024 = 0xE1A00000; // nop
		*(u32*)0x0201C184 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C1E0, 0x02700000); // mov r0, #0x2700000
		//*(u32*)0x0201C314 -= 0x30000;
		*(u32*)0x0201D604 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02021190 = 0xE1A00000; // nop
		*(u32*)0x02022AD8 = 0xE1A00000; // nop
		*(u32*)0x02022ADC = 0xE1A00000; // nop
		*(u32*)0x02022B0C = 0xE1A00000; // nop
		if (ndsHeader->romversion == 0) {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x02042F40 = 0xE1A00000; // nop
				*(u32*)0x02042F5C = 0xE1A00000; // nop

				tonccpy((u32*)readCodeCopy, (u32*)0x020BCB48, 0x70);

				*(u32*)0x020431B0 = 0xE1A00000; // nop
				setBL(0x02043290, readCodeCopy);
				setBL(0x02043328, readCodeCopy);

				setBL(readCodeCopy+0x18, 0x02013CC8);
				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
				setBL(readCodeCopy+0x54, 0x020BCA0C);

				*(u32*)0x0208EB74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

				setBL(0x020BCCF4, (u32)dsiSaveCreate);
				setBL(0x020BCD04, (u32)dsiSaveOpen);
				setBL(0x020BCD18, (u32)dsiSaveSetLength);
				setBL(0x020BCD28, (u32)dsiSaveWrite);
				setBL(0x020BCD30, (u32)dsiSaveClose);

				*(u32*)0x020BE934 = 0xE3A00000; // mov r0, #0
			} else {
				*(u32*)0x02042F9C = 0xE1A00000; // nop
				*(u32*)0x02042FB8 = 0xE1A00000; // nop

				tonccpy((u32*)readCodeCopy, (u32*)0x020BCC6C, 0x70);

				*(u32*)0x0204320C = 0xE1A00000; // nop
				setBL(0x020432EC, readCodeCopy);
				setBL(0x02043384, readCodeCopy);

				setBL(readCodeCopy+0x18, 0x02013CC8);
				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
				setBL(readCodeCopy+0x54, 0x020BCB20);

				*(u32*)0x0208EC38 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

				setBL(0x020BCE18, (u32)dsiSaveCreate);
				setBL(0x020BCE28, (u32)dsiSaveOpen);
				setBL(0x020BCE3C, (u32)dsiSaveSetLength);
				setBL(0x020BCE4C, (u32)dsiSaveWrite);
				setBL(0x020BCE54, (u32)dsiSaveClose);

				*(u32*)0x020BEA68 = 0xE3A00000; // mov r0, #0
			}
		} else {
			*(u32*)0x02042F9C = 0xE1A00000; // nop
			*(u32*)0x02042FB8 = 0xE1A00000; // nop

			tonccpy((u32*)readCodeCopy, (u32*)0x020BCDA4, 0x70);

			*(u32*)0x0204320C = 0xE1A00000; // nop
			setBL(0x020432EC, readCodeCopy);
			setBL(0x02043384, readCodeCopy);

			setBL(readCodeCopy+0x18, 0x02013CC8);
			setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
			setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
			setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
			setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
			setBL(readCodeCopy+0x54, 0x020BCC58);

			*(u32*)0x0208EE9C = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

			setBL(0x020BCF50, (u32)dsiSaveCreate);
			setBL(0x020BCF60, (u32)dsiSaveOpen);
			setBL(0x020BCF74, (u32)dsiSaveSetLength);
			setBL(0x020BCF84, (u32)dsiSaveWrite);
			setBL(0x020BCF8C, (u32)dsiSaveClose);

			*(u32*)0x020BEBA0 = 0xE3A00000; // mov r0, #0
		}
	}

	// Go! Go! Kokopolo (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K3GJ") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02012768 = 0xE1A00000; // nop
		*(u32*)0x02015CC8 = 0xE1A00000; // nop
		*(u32*)0x0201A13C = 0xE1A00000; // nop
		*(u32*)0x0201C044 = 0xE1A00000; // nop
		*(u32*)0x0201C048 = 0xE1A00000; // nop
		*(u32*)0x0201C054 = 0xE1A00000; // nop
		*(u32*)0x0201C1B4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C210, 0x02700000); // mov r0, #0x2700000
		//*(u32*)0x0201C344 -= 0x30000;
		*(u32*)0x0201D634 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020211C0 = 0xE1A00000; // nop
		*(u32*)0x02022B08 = 0xE1A00000; // nop
		*(u32*)0x02022B0C = 0xE1A00000; // nop
		*(u32*)0x02022B3C = 0xE1A00000; // nop

		*(u32*)0x02042EFC = 0xE1A00000; // nop
		*(u32*)0x02042F18 = 0xE1A00000; // nop

		const u32 readCodeCopy = 0x02013D24;
		tonccpy((u32*)readCodeCopy, (u32*)0x020BCD90, 0x70);

		*(u32*)0x0204316C = 0xE1A00000; // nop
		setBL(0x0204324C, readCodeCopy);
		setBL(0x020432E4, readCodeCopy);

		setBL(readCodeCopy+0x18, 0x02013CF8);
		setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
		setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
		setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
		setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
		setBL(readCodeCopy+0x54, 0x020BCC44);

		*(u32*)0x0208EC74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

		setBL(0x020BCF3C, (u32)dsiSaveCreate);
		setBL(0x020BCF4C, (u32)dsiSaveOpen);
		setBL(0x020BCF60, (u32)dsiSaveSetLength);
		setBL(0x020BCF70, (u32)dsiSaveWrite);
		setBL(0x020BCF78, (u32)dsiSaveClose);

		*(u32*)0x020BEB8C = 0xE3A00000; // mov r0, #0
	}

	// Gold Fever (USA)
	// Requires more than 8MB of RAM
	/*else if (strcmp(romTid, "KG7E") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02013E80 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02014008 = 0xE1A00000; // nop
		*(u32*)0x020142E0 = 0xE1A00000; // nop
		*(u32*)0x02023AA4 = 0xE1A00000; // nop
		*(u32*)0x02031854 = 0xE1A00000; // nop
		*(u32*)0x02032698 = 0xE1A00000; // nop
		*(u32*)0x02032A04 = 0xE1A00000; // nop
		*(u32*)0x020392C0 = 0xE1A00000; // nop
		*(u32*)0x0203B2F8 = 0xE1A00000; // nop
		*(u32*)0x0203E7B4 = 0xE1A00000; // nop
		*(u32*)0x02041FD8 = 0xE1A00000; // nop
		*(u32*)0x02043E5C = 0xE1A00000; // nop
		*(u32*)0x02043E60 = 0xE1A00000; // nop
		*(u32*)0x02043E6C = 0xE1A00000; // nop
		*(u32*)0x02043FCC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02044028, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0204546C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02048314 = 0xE1A00000; // nop
	}*/

	// Hard-Hat Domo (USA)
	else if (strcmp(romTid, "KDHE") == 0) {
		const u32 dsiSaveCreateT = 0x020238C8;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x020238D8;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x020238E8;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x020238F8;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02023908;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02023918;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02023928;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0200D060 = 0x2001; // movs r0, #1
		*(u16*)0x0200D062 = 0x4770; // bx lr
		*(u16*)0x0200D350 = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0200D352 = 0x4770; // bx lr
		doubleNopT(0x0200D30E);
		setBLThumb(0x0200D3B6, dsiSaveCreateT);
		setBLThumb(0x0200D3CC, dsiSaveOpenT);
		setBLThumb(0x0200D3E8, dsiSaveSetLengthT);
		setBLThumb(0x0200D3FC, dsiSaveWriteT);
		setBLThumb(0x0200D40E, dsiSaveCloseT);
		*(u16*)0x0200D434 = 0x4778; // bx pc
		tonccpy((u32*)0x0200D438, dsiSaveGetLength, 0xC);
		setBLThumb(0x0200D464, dsiSaveOpenT);
		setBLThumb(0x0200D48A, dsiSaveCloseT);
		setBLThumb(0x0200D49C, dsiSaveReadT);
		setBLThumb(0x0200D4A2, dsiSaveCloseT);
		setBLThumb(0x0200D4B6, dsiSaveDeleteT);
		*(u16*)0x020140AC = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02024C7E);
		doubleNopT(0x02022A2E);
		doubleNopT(0x02027900);
		doubleNopT(0x02028EBA);
		doubleNopT(0x02028EBE);
		doubleNopT(0x02028ECA);
		doubleNopT(0x02028FAE);
		patchHiHeapDSiWareThumb(0x02028FEC, 0x02023938, heapEnd); // movs r0, #0x23C0000
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
		setBL(0x0204BF68, (u32)dsiSaveOpenR);
		setBL(0x0204BF88, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0204BFC0, (u32)dsiSaveOpen);
		setBL(0x0204BFD4, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204BFEC = 0xE1A00000; // nop
		setBL(0x0205B640, (u32)dsiSaveOpen);
		setBL(0x0205B658, (u32)dsiSaveSeek);
		setBL(0x0205B668, (u32)dsiSaveRead);
		setBL(0x0205B680, (u32)dsiSaveClose);
		setBL(0x0205B69C, (u32)dsiSaveOpen);
		setBL(0x0205B6B4, (u32)dsiSaveSeek);
		setBL(0x0205B6C4, (u32)dsiSaveWrite);
		setBL(0x0205B6CC, (u32)dsiSaveClose);
		setBL(0x0205B71C, (u32)dsiSaveOpen);
		setBL(0x0205B738, (u32)dsiSaveSeek);
		setBL(0x0205B748, (u32)dsiSaveRead);
		setBL(0x0205B750, (u32)dsiSaveClose);
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
		setBL(0x020067C4, (u32)dsiSaveOpen);
		*(u32*)0x020067DC = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x020067F4 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x02006810, (u32)dsiSaveClose);
		setBL(0x020070A0, (u32)dsiSaveOpen);
		setBL(0x0200710C, (u32)dsiSaveClose);
		setBL(0x0200761C, (u32)dsiSaveOpen);
		setBL(0x020076A8, (u32)dsiSaveClose);
		*(u32*)0x02019F94 = 0xE1A00000; // nop (Skip Manual screen, Part 1)
		for (int i = 0; i < 11; i++) { // Skip Manual screen, Part 2
			u32* offset = (u32*)0x0201A028;
			offset[i] = 0xE1A00000; // nop
		}
		setBL(0x020B2934, (u32)dsiSaveOpen);
		setBL(0x020B294C, (u32)dsiSaveCreate);
		setBL(0x020B297C, (u32)dsiSaveOpen);
		setBL(0x020B29C4, (u32)dsiSaveWrite);
		setBL(0x020B29E4, (u32)dsiSaveClose);
		setBL(0x020B29FC, (u32)dsiSaveClose);
		setBL(0x020B2A14, (u32)dsiSaveOpen);
		setBL(0x020B2A24, (u32)dsiSaveGetLength);
		setBL(0x020B2A64, (u32)dsiSaveWrite);
		setBL(0x020B2A9C, (u32)dsiSaveSeek);
		setBL(0x020B2AD0, (u32)dsiSaveWrite);
		setBL(0x020B2ADC, (u32)dsiSaveClose);
		setBL(0x020B2BD0, (u32)dsiSaveOpen);
		setBL(0x020B2BF4, (u32)dsiSaveGetLength);
		setBL(0x020B2C48, (u32)dsiSaveRead);
		setBL(0x020B2C80, (u32)dsiSaveSeek);
		setBL(0x020B2CC0, (u32)dsiSaveRead);
		setBL(0x020B2CCC, (u32)dsiSaveClose);
		setBL(0x020B2E00, (u32)dsiSaveDelete);
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
		*(u32*)0x0201D8EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0201DA28, (u32)dsiSaveOpen);
		setBL(0x0201DA50, (u32)dsiSaveRead);
		setBL(0x0201DA60, (u32)dsiSaveRead);
		setBL(0x0201DA68, (u32)dsiSaveClose);
		setBL(0x0201DD24, (u32)dsiSaveOpen);
		setBL(0x0201DE6C, (u32)dsiSaveWrite);
		setBL(0x0201DE74, (u32)dsiSaveClose);
		*(u32*)0x02024994 = 0xE1A00000; // nop
		setBL(0x020249A0, (u32)dsiSaveCreate);
		*(u32*)0x02024A4C = 0xE1A00000; // nop
		setBL(0x02024A58, (u32)dsiSaveCreate);
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
		*(u32*)0x0201D8C0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0201D9FC, (u32)dsiSaveOpen);
		setBL(0x0201DA24, (u32)dsiSaveRead);
		setBL(0x0201DA34, (u32)dsiSaveRead);
		setBL(0x0201DA3C, (u32)dsiSaveClose);
		setBL(0x0201DCF8, (u32)dsiSaveOpen);
		setBL(0x0201DE40, (u32)dsiSaveWrite);
		setBL(0x0201DE48, (u32)dsiSaveClose);
		*(u32*)0x020248BC = 0xE1A00000; // nop
		setBL(0x020248C8, (u32)dsiSaveCreate);
		*(u32*)0x02024974 = 0xE1A00000; // nop
		setBL(0x02024980, (u32)dsiSaveCreate);
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
	// Requires 8MB of RAM
	else if (strncmp(romTid, "KQ9", 3) == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x02012AAC = 0xE1A00000; // nop
		*(u32*)0x020166D4 = 0xE1A00000; // nop
		*(u32*)0x020185C8 = 0xE1A00000; // nop
		*(u32*)0x020185CC = 0xE1A00000; // nop
		*(u32*)0x020185D8 = 0xE1A00000; // nop
		*(u32*)0x02018738 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02018794, 0x027B0000); // mov r0, #0x27B0000
		*(u32*)0x020188C8 -= 0x30000;
		*(u32*)0x0201994C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02019968 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201996C = 0xE12FFF1E; // bx lr
		*(u32*)0x02019974 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02019978 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201D01C = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02082A58 = 0xE1A00000; // nop
			//*(u32*)0x0208CDC0 = 0xE3A00000; // mov r0, #0 (Skip .wave file loading)
			*(u32*)0x020A44F4 = 0xE1A00000; // nop
			*(u32*)0x020A44F8 = 0xE1A00000; // nop
			*(u32*)0x020A44FC = 0xE1A00000; // nop
			*(u32*)0x020A467C = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x02082A78 = 0xE1A00000; // nop
			//*(u32*)0x0208CDE0 = 0xE3A00000; // mov r0, #0 (Skip .wave file loading)
			*(u32*)0x020A4514 = 0xE1A00000; // nop
			*(u32*)0x020A4518 = 0xE1A00000; // nop
			*(u32*)0x020A451C = 0xE1A00000; // nop
			*(u32*)0x020A469C = 0xE1A00000; // nop
		} else {
			*(u32*)0x02082A14 = 0xE1A00000; // nop
			//*(u32*)0x0208CD7C = 0xE3A00000; // mov r0, #0 (Skip .wave file loading)
			*(u32*)0x020A44B0 = 0xE1A00000; // nop
			*(u32*)0x020A44B4 = 0xE1A00000; // nop
			*(u32*)0x020A44B8 = 0xE1A00000; // nop
			*(u32*)0x020A4638 = 0xE1A00000; // nop
		}
	}

	// Legends of Exidia (USA)
	// Requires more than 8MB of RAM
	/*else if (strcmp(romTid, "KLEE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050F4 = 0xE1A00000; // nop
		*(u32*)0x0201473C = 0xE1A00000; // nop
		*(u32*)0x02017E2C = 0xE1A00000; // nop
		*(u32*)0x0201C794 = 0xE1A00000; // nop
		*(u32*)0x0201E624 = 0xE1A00000; // nop
		*(u32*)0x0201E628 = 0xE1A00000; // nop
		*(u32*)0x0201E634 = 0xE1A00000; // nop
		*(u32*)0x0201E794 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201E7F0, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201FDA8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023794 = 0xE1A00000; // nop
		*(u32*)0x02026E94 = 0xE1A00000; // nop
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

	// Little Red Riding Hood's Zombie BBQ (USA)
	else if (strcmp(romTid, "KZBE") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x02005124 = 0xE1A00000; // nop
		*(u32*)0x02005194 = 0xE1A00000; // nop
		*(u32*)0x02005198 = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		*(u32*)0x0200BFE0 = 0xE1A00000; // nop
		*(u32*)0x0200FDB4 = 0xE1A00000; // nop
		*(u32*)0x020172A0 = 0xE1A00000; // nop
		*(u32*)0x02019168 = 0xE1A00000; // nop
		*(u32*)0x0201916C = 0xE1A00000; // nop
		*(u32*)0x02019178 = 0xE1A00000; // nop
		*(u32*)0x020192BC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019318, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201D3D4 = 0xE1A00000; // nop
		*(u32*)0x0201F65C = 0xE1A00000; // nop
		*(u32*)0x0202178C = 0xE1A00000; // nop
		*(u32*)0x02021790 = 0xE1A00000; // nop
		*(u32*)0x02021794 = 0xE1A00000; // nop
		*(u32*)0x02026224 = 0xE1A00000; // nop
		*(u32*)0x02026BFC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02026C18 = 0xE1A00000; // nop
		*(u32*)0x02026CB0 = 0xE1A00000; // nop
		*(u32*)0x02026CB4 = 0xE1A00000; // nop
		*(u32*)0x02026CB8 = 0xE1A00000; // nop
		setBL(0x0204A7D4, (u32)dsiSaveOpen);
		setBL(0x0204A7EC, (u32)dsiSaveGetLength);
		setBL(0x0204A7FC, (u32)dsiSaveSeek);
		setBL(0x0204A80C, (u32)dsiSaveWrite);
		setBL(0x0204A814, (u32)dsiSaveClose);
		setBL(0x0204A884, (u32)dsiSaveOpen);
		setBL(0x0204A89C, (u32)dsiSaveGetLength);
		setBL(0x0204A8AC, (u32)dsiSaveSeek);
		setBL(0x0204A8BC, (u32)dsiSaveRead);
		setBL(0x0204A8C4, (u32)dsiSaveClose);
		setBL(0x0204A93C, (u32)dsiSaveCreate);
		setBL(0x0204A968, (u32)dsiSaveOpen);
		setBL(0x0204A9A4, (u32)dsiSaveWrite);
		setBL(0x0204A9B4, (u32)dsiSaveClose);
	}

	// Little Red Riding Hood's Zombie BBQ (Europe)
	else if (strcmp(romTid, "KZBP") == 0 && extendedMemory2) {
		*(u32*)0x02017550 = 0xE1A00000; // nop
		*(u32*)0x0201B324 = 0xE1A00000; // nop
		*(u32*)0x02022810 = 0xE1A00000; // nop
		*(u32*)0x020246CC = 0xE1A00000; // nop
		*(u32*)0x020246D0 = 0xE1A00000; // nop
		*(u32*)0x020246DC = 0xE1A00000; // nop
		*(u32*)0x02024820 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202487C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02028790 = 0xE1A00000; // nop
		*(u32*)0x02031F5C = 0xE3A00001; // mov r0, #1
		setBL(0x02055AC8, (u32)dsiSaveOpen);
		setBL(0x02055AE0, (u32)dsiSaveGetLength);
		setBL(0x02055AF0, (u32)dsiSaveSeek);
		setBL(0x02055B00, (u32)dsiSaveWrite);
		setBL(0x02055B08, (u32)dsiSaveClose);
		setBL(0x02055B78, (u32)dsiSaveOpen);
		setBL(0x02055B90, (u32)dsiSaveGetLength);
		setBL(0x02055BA0, (u32)dsiSaveSeek);
		setBL(0x02055BB0, (u32)dsiSaveRead);
		setBL(0x02055BB8, (u32)dsiSaveClose);
		setBL(0x02055C30, (u32)dsiSaveCreate);
		setBL(0x02055C5C, (u32)dsiSaveOpen);
		setBL(0x02055C98, (u32)dsiSaveWrite);
		setBL(0x02055CA8, (u32)dsiSaveClose);
	}

	// Littlest Pet Shop (USA)
	// Littlest Pet Shop (Europe, Australia)
	else if (strcmp(romTid, "KLPE") == 0 || strcmp(romTid, "KLPV") == 0) {
		*(u32*)0x0200506C = 0xE1A00000; // nop
		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x0200509C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020055BC;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x02014D4C = 0xE1A00000; // nop
		tonccpy((u32*)0x020159F0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02018354 = 0xE1A00000; // nop
		*(u32*)0x0201E8A4 = 0xE1A00000; // nop
		*(u32*)0x020207B0 = 0xE1A00000; // nop
		*(u32*)0x020207B4 = 0xE1A00000; // nop
		*(u32*)0x020207C0 = 0xE1A00000; // nop
		*(u32*)0x02020904 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02020960, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02021C1C = 0xE1A00000; // nop
		*(u32*)0x02021C24 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02025168 = 0xE1A00000; // nop
		setBL(0x0205DE18, (u32)dsiSaveOpen);
		setBL(0x0205DE70, (u32)dsiSaveRead);
		setBL(0x0205DF24, (u32)dsiSaveCreate);
		setBL(0x0205DF34, (u32)dsiSaveOpen);
		setBL(0x0205DF7C, (u32)dsiSaveSetLength);
		setBL(0x0205DF8C, (u32)dsiSaveWrite);
		setBL(0x0205DF94, (u32)dsiSaveClose);
	}

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

	// Magical Diary: Secrets Sharing (USA)
	// Requires 8MB of RAM
	// Unable to save data
	/*else if (strcmp(romTid, "K73E") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201A17C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0201A22C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x0201A9BC, (u32)dsiSaveOpen);
		setBL(0x0201A9D0, (u32)dsiSaveSeek);
		setBL(0x0201A9E4, (u32)dsiSaveRead);
		setBL(0x0201AA18, (u32)dsiSaveClose);
		setBL(0x0201AA34, (u32)dsiSaveRead);
		setBL(0x0201AA70, (u32)dsiSaveClose);
		setBL(0x0201AACC, (u32)dsiSaveRead);
		setBL(0x0201AAFC, (u32)dsiSaveClose);
		setBL(0x0201AB40, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0201AB54, (u32)dsiSaveRead);
		setBL(0x0201AB88, (u32)dsiSaveClose);
		setBL(0x0201ABE0, (u32)dsiSaveClose);
		setBL(0x0201ABFC, (u32)dsiSaveClose);
		setBL(0x0201AC30, (u32)dsiSaveClose);
		setBL(0x0201AD50, (u32)dsiSaveOpen);
		setBL(0x0201AD60, (u32)dsiSaveRead);
		setBL(0x0201AD8C, (u32)dsiSaveClose);
		setBL(0x0201ADAC, (u32)dsiSaveSeek);
		setBL(0x0201ADC0, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201ADF0, (u32)dsiSaveClose);
		setBL(0x0201AE28, (u32)dsiSaveOpen);
		setBL(0x0201AE38, (u32)dsiSaveRead);
		setBL(0x0201AE64, (u32)dsiSaveClose);
		setBL(0x0201AE84, (u32)dsiSaveSeek);
		setBL(0x0201AE98, (u32)dsiSaveWrite);
		setBL(0x0201AEC0, (u32)dsiSaveClose);
		setBL(0x0201AEE0, (u32)dsiSaveClose);
		setBL(0x0201AF5C, (u32)dsiSaveOpen);
		setBL(0x0201AF70, (u32)dsiSaveCreate);
		*(u32*)0x0201AF84 = 0xE1A00000; // nop
		setBL(0x0201AF90, (u32)dsiSaveCreate);
		setBL(0x0201AFB4, (u32)dsiSaveClose);
		setBL(0x0201B058, (u32)dsiSaveOpen);
		setBL(0x0201B06C, (u32)dsiSaveCreate);
		setBL(0x0201B090, (u32)dsiSaveClose);
		setBL(0x0201B0E4, (u32)dsiSaveOpen);
		setBL(0x0201B0F4, (u32)dsiSaveRead);
		setBL(0x0201B118, (u32)dsiSaveClose);
		setBL(0x0201B12C, (u32)dsiSaveClose);
		*(u32*)0x020764FC = 0xE1A00000; // nop
		*(u32*)0x0207650C = 0xE1A00000; // nop
		*(u32*)0x0208AFFC = 0xE1A00000; // nop
		*(u32*)0x0208B124 = 0xE1A00000; // nop
		*(u32*)0x0208B138 = 0xE1A00000; // nop
		*(u32*)0x0208E6DC = 0xE1A00000; // nop
		*(u32*)0x02094B20 = 0xE1A00000; // nop
		*(u32*)0x02096CE0 = 0xE1A00000; // nop
		*(u32*)0x02096CE4 = 0xE1A00000; // nop
		*(u32*)0x02096CF0 = 0xE1A00000; // nop
		*(u32*)0x02096E50 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02096EAC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0209831C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02098338 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0209833C = 0xE12FFF1E; // bx lr
		*(u32*)0x02098344 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02098348 = 0xE12FFF1E; // bx lr
		*(u32*)0x0209B90C = 0xE1A00000; // nop
	}*/

	// Tomodachi Tsukurou!: Mahou no Koukan Nikki (Japan)
	// Requires 8MB of RAM
	// Unable to save data
	/*else if (strcmp(romTid, "K85J") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201A3A8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0201A420 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x0201AB94, (u32)dsiSaveOpen);
		setBL(0x0201ABA8, (u32)dsiSaveSeek);
		setBL(0x0201ABBC, (u32)dsiSaveRead);
		setBL(0x0201ABF0, (u32)dsiSaveClose);
		setBL(0x0201AC0C, (u32)dsiSaveRead);
		setBL(0x0201AC48, (u32)dsiSaveClose);
		setBL(0x0201ACA4, (u32)dsiSaveRead);
		setBL(0x0201ACD4, (u32)dsiSaveClose);
		setBL(0x0201AD18, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0201AD2C, (u32)dsiSaveRead);
		setBL(0x0201AD60, (u32)dsiSaveClose);
		setBL(0x0201ADB8, (u32)dsiSaveClose);
		setBL(0x0201ADD4, (u32)dsiSaveClose);
		setBL(0x0201AE08, (u32)dsiSaveClose);
		setBL(0x0201AF28, (u32)dsiSaveOpen);
		setBL(0x0201AF38, (u32)dsiSaveRead);
		setBL(0x0201AF64, (u32)dsiSaveClose);
		setBL(0x0201AF84, (u32)dsiSaveSeek);
		setBL(0x0201AF98, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201AFC8, (u32)dsiSaveClose);
		setBL(0x0201B000, (u32)dsiSaveOpen);
		setBL(0x0201B010, (u32)dsiSaveRead);
		setBL(0x0201B03C, (u32)dsiSaveClose);
		setBL(0x0201B05C, (u32)dsiSaveSeek);
		setBL(0x0201B070, (u32)dsiSaveWrite);
		setBL(0x0201B098, (u32)dsiSaveClose);
		setBL(0x0201B0B8, (u32)dsiSaveClose);
		setBL(0x0201B128, (u32)dsiSaveOpen);
		setBL(0x0201B13C, (u32)dsiSaveCreate);
		*(u32*)0x0201B150 = 0xE1A00000; // nop
		setBL(0x0201B15C, (u32)dsiSaveCreate);
		setBL(0x0201B178, (u32)dsiSaveClose);
		setBL(0x0201B1F0, (u32)dsiSaveOpen);
		setBL(0x0201B204, (u32)dsiSaveCreate);
		setBL(0x0201B21C, (u32)dsiSaveClose);
		setBL(0x0201B270, (u32)dsiSaveOpen);
		setBL(0x0201B280, (u32)dsiSaveRead);
		setBL(0x0201B2A0, (u32)dsiSaveClose);
		*(u32*)0x02078BE4 = 0xE1A00000; // nop
		*(u32*)0x02078BF4 = 0xE1A00000; // nop
		*(u32*)0x020A6620 = 0xE1A00000; // nop
		*(u32*)0x020A9D64 = 0xE1A00000; // nop
		*(u32*)0x020B01A8 = 0xE1A00000; // nop
		*(u32*)0x020B2214 = 0xE1A00000; // nop
		*(u32*)0x020B2218 = 0xE1A00000; // nop
		*(u32*)0x020B2224 = 0xE1A00000; // nop
		*(u32*)0x020B2384 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020B23E0, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020B37CC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020B37E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B37EC = 0xE12FFF1E; // bx lr
		*(u32*)0x020B37F4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B37F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020B6DAC = 0xE1A00000; // nop
	}*/

	// Magical Drop Yurutto (Japan)
	// Crashes at 0x0201AF4C(?)
	/*else if (strcmp(romTid, "KMAJ") == 0) {
		*(u32*)0x020159D4 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x02019350 = 0xE1A00000; // nop
		*(u32*)0x02019360 = 0xE1A00000; // nop
		*(u32*)0x0201936C = 0xE1A00000; // nop
		*(u32*)0x02019378 = 0xE1A00000; // nop
		*(u32*)0x0201943C = 0xE1A00000; // nop
		setBL(0x020197AC, (u32)dsiSaveCreate);
		setBL(0x02019800, (u32)dsiSaveDelete);
		setBL(0x02019860, (u32)dsiSaveOpen);
		setBL(0x0201988C, (u32)dsiSaveGetLength);
		setBL(0x020198A0, (u32)dsiSaveRead);
		setBL(0x020198BC, (u32)dsiSaveClose);
		setBL(0x020198D0, (u32)dsiSaveClose);
		setBL(0x02019964, (u32)dsiSaveOpen);
		setBL(0x02019998, (u32)dsiSaveWrite);
		setBL(0x020199B4, (u32)dsiSaveClose);
		setBL(0x020199C8, (u32)dsiSaveClose);
		*(u32*)0x020199FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201C160 = 0xE1A00000; // nop
		*(u32*)0x0201C85C = 0xE1A00000; // nop
		*(u32*)0x0201C860 = 0xE1A00000; // nop
		*(u32*)0x0201C870 = 0xE1A00000; // nop
		*(u32*)0x0201CED4 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x020343E8 = 0xE1A00000; // nop
		tonccpy((u32*)0x020350D0, dsiSaveGetResultCode, 0xC);
		tonccpy((u32*)0x02035C7C, dsiSaveGetInfo, 0xC);
		*(u32*)0x02037D30 = 0xE1A00000; // nop
		*(u32*)0x0203BBFC = 0xE1A00000; // nop
		*(u32*)0x0203D9F4 = 0xE1A00000; // nop
		*(u32*)0x0203D9F8 = 0xE1A00000; // nop
		*(u32*)0x0203DA04 = 0xE1A00000; // nop
		*(u32*)0x0203DB48 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0203DBA4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0203F0F0 = 0xE1A00000; // nop
		*(u32*)0x0203F0F8 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0203F468 = 0xE1A00000; // nop
		*(u32*)0x0203F46C = 0xE1A00000; // nop
		*(u32*)0x0203F470 = 0xE1A00000; // nop
		*(u32*)0x0203F474 = 0xE1A00000; // nop
		*(u32*)0x02041E00 = 0xE1A00000; // nop
	}*/

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
		setBL(0x0203F434, (u32)dsiSaveOpen);
		setBL(0x0203F46C, (u32)dsiSaveSetLength);
		setBL(0x0203F50C, (u32)dsiSaveWrite);
		setBL(0x0203F514, (u32)dsiSaveClose);
		setBL(0x0203F584, (u32)dsiSaveOpen);
		setBL(0x0203F5A8, (u32)dsiSaveGetLength);
		setBL(0x0203F5B8, (u32)dsiSaveRead);
		setBL(0x0203F5C8, (u32)dsiSaveRead);
		setBL(0x0203F5D0, (u32)dsiSaveClose);
		setBL(0x0203F920, (u32)dsiSaveCreate);
		*(u32*)0x0203FB18 = 0xE1A00000; // nop
		setBL(0x0203FB34, (u32)dsiSaveCreate);
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
		setBL(0x0203F514, (u32)dsiSaveOpen);
		setBL(0x0203F54C, (u32)dsiSaveSetLength);
		setBL(0x0203F5EC, (u32)dsiSaveWrite);
		setBL(0x0203F5F4, (u32)dsiSaveClose);
		setBL(0x0203F664, (u32)dsiSaveOpen);
		setBL(0x0203F688, (u32)dsiSaveGetLength);
		setBL(0x0203F698, (u32)dsiSaveRead);
		setBL(0x0203F6A8, (u32)dsiSaveRead);
		setBL(0x0203F6B0, (u32)dsiSaveClose);
		setBL(0x0203FA00, (u32)dsiSaveCreate);
		*(u32*)0x0203FBF8 = 0xE1A00000; // nop
		setBL(0x0203FC14, (u32)dsiSaveCreate);
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

	// Make Up & Style (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYLE") == 0 && extendedMemory2) {
		/*for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
			u32* addr = (u32*)0x02004000;
			if (addr[i] >= 0x022F4000 && addr[i] < 0x02308000) {
				addr[i] -= extendedMemory2 ? 0x100000 : 0x180000;
			}
		}*/

		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x020052B8 = 0xE1A00000; // nop
		*(u32*)0x020052BC = 0xE1A00000; // nop
		*(u32*)0x020052E0 = 0xE1A00000; // nop
		*(u32*)0x020052F4 = 0xE1A00000; // nop
		*(u32*)0x02005324 = 0xE1A00000; // nop
		*(u32*)0x02005348 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0200534C = 0xE1A00000; // nop
		/*if (!extendedMemory2 && expansionPakFound) {
			*(u32*)0x020080DC = 0x09000000;
			*(u32*)0x020094CC = *(u32*)0x020080DC;
			*(u32*)0x020099CC = *(u32*)0x020080DC;
			*(u32*)0x0200BA74 = *(u32*)0x020080DC;
			*(u32*)0x0200C2CC = *(u32*)0x020080DC;
			*(u32*)0x0201E650 = *(u32*)0x020080DC;
		}*/
		setBL(0x0202A34C, (u32)dsiSaveOpen);
		setBL(0x0202A360, (u32)dsiSaveCreate);
		setBL(0x0202A384, (u32)dsiSaveOpen);
		setBL(0x0202A398, (u32)dsiSaveSetLength);
		setBL(0x0202A3A4, (u32)dsiSaveClose);
		setBL(0x0202A3C4, (u32)dsiSaveSetLength);
		setBL(0x0202A3CC, (u32)dsiSaveClose);
		*(u32*)0x0202A4E8 = 0xE1A00000; // nop
		setBL(0x0202A5C4, (u32)dsiSaveOpen);
		setBL(0x0202A5EC, (u32)dsiSaveSeek);
		setBL(0x0202A600, (u32)dsiSaveRead);
		setBL(0x0202A60C, (u32)dsiSaveClose);
		setBL(0x0202A6D4, (u32)dsiSaveOpen);
		setBL(0x0202A6FC, (u32)dsiSaveSeek);
		setBL(0x0202A710, (u32)dsiSaveWrite);
		setBL(0x0202A71C, (u32)dsiSaveClose);
		*(u32*)0x020558D4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02056468, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02059040 = 0xE1A00000; // nop
		*(u32*)0x0205E7C0 = 0xE1A00000; // nop
		*(u32*)0x02060810 = 0xE1A00000; // nop
		*(u32*)0x02060814 = 0xE1A00000; // nop
		*(u32*)0x02060820 = 0xE1A00000; // nop
		*(u32*)0x02060980 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020609DC, /*extendedMemory2 ?*/ 0x02700000 /*: heapEnd*/); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		//*(u32*)0x02060B10 -= extendedMemory2 ? 0x100000 : 0x310000;
		*(u32*)0x02061F74 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0206237C = 0xE1A00000; // nop
		*(u32*)0x02062380 = 0xE1A00000; // nop
		*(u32*)0x02062384 = 0xE1A00000; // nop
		*(u32*)0x02062388 = 0xE1A00000; // nop
		*(u32*)0x020654D8 = 0xE1A00000; // nop
	}

	// Make Up & Style (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYLP") == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020052D0 = 0xE1A00000; // nop
		*(u32*)0x020052D4 = 0xE1A00000; // nop
		*(u32*)0x020052F8 = 0xE1A00000; // nop
		*(u32*)0x0200530C = 0xE1A00000; // nop
		*(u32*)0x0200533C = 0xE1A00000; // nop
		*(u32*)0x02005360 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x02005364 = 0xE1A00000; // nop
		setBL(0x0202A484, (u32)dsiSaveOpen);
		setBL(0x0202A498, (u32)dsiSaveCreate);
		setBL(0x0202A4BC, (u32)dsiSaveOpen);
		setBL(0x0202A4D0, (u32)dsiSaveSetLength);
		setBL(0x0202A4DC, (u32)dsiSaveClose);
		setBL(0x0202A4FC, (u32)dsiSaveSetLength);
		setBL(0x0202A504, (u32)dsiSaveClose);
		*(u32*)0x0202A620 = 0xE1A00000; // nop
		setBL(0x0202A6FC, (u32)dsiSaveOpen);
		setBL(0x0202A724, (u32)dsiSaveSeek);
		setBL(0x0202A738, (u32)dsiSaveRead);
		setBL(0x0202A744, (u32)dsiSaveClose);
		setBL(0x0202A80C, (u32)dsiSaveOpen);
		setBL(0x0202A834, (u32)dsiSaveSeek);
		setBL(0x0202A848, (u32)dsiSaveWrite);
		setBL(0x0202A854, (u32)dsiSaveClose);
		*(u32*)0x02055A0C = 0xE1A00000; // nop
		tonccpy((u32*)0x020565A0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02059200 = 0xE1A00000; // nop
		*(u32*)0x0205E99C = 0xE1A00000; // nop
		*(u32*)0x020609F4 = 0xE1A00000; // nop
		*(u32*)0x020609F8 = 0xE1A00000; // nop
		*(u32*)0x02060A04 = 0xE1A00000; // nop
		*(u32*)0x02060B64 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02060BC0, /*extendedMemory2 ?*/ 0x02700000 /*: heapEnd*/); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x02062158 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02062560 = 0xE1A00000; // nop
		*(u32*)0x02062564 = 0xE1A00000; // nop
		*(u32*)0x02062568 = 0xE1A00000; // nop
		*(u32*)0x0206256C = 0xE1A00000; // nop
		*(u32*)0x020656BC = 0xE1A00000; // nop
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

	// Metal Torrent (USA)
	// Saving not supported due to using more than one file
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K59E") == 0 && extendedMemory2) {
		*(u32*)0x020136D4 = 0xE1A00000; // nop
		*(u32*)0x02017CD4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02017CF4 = 0xE1A00000; // nop
		/* *(u32*)0x0201DBAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DBB0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DC58 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DC5C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DCA0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DCA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DD20 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DD24 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DD68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DD6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DDB8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DDBC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DE80 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DE84 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DF48 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DF4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DFB4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DFB8 = 0xE12FFF1E; // bx lr */
		/*setBL(0x0201E198, (u32)dsiSaveCreate);
		setBL(0x0201E374, (u32)dsiSaveOpen);
		setBL(0x0201E4F4, (u32)dsiSaveClose);
		setBL(0x0201E628, (u32)dsiSaveSeek);
		setBL(0x0201E638, (u32)dsiSaveRead);
		setBL(0x0201E7D0, (u32)dsiSaveSeek);
		setBL(0x0201E7E0, (u32)dsiSaveWrite);
		setBL(0x0201E970, (u32)dsiSaveOpenR);
		setBL(0x0201EA18, (u32)dsiSaveClose);*/
		/* *(u32*)0x0201E0EC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E2B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E494 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E58C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EA5C = 0xE3A00001; // mov r0, #1 */
		*(u32*)0x0201EA90 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02045F88 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x02046070 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020460B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020474D0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204BA18 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0204BA1C = 0xE12FFF1E; // bx lr
		*(u32*)0x0204C0B8 = 0xE1A00000; // nop
		*(u32*)0x0204C0BC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C0D4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C238 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C2C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C2F8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C3C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C3F8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D710 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D748 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D764 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D7A0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D7C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D7E4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D800 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0204DF68 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x0204DF6C = 0xE12FFF1E; // bx lr
		*(u32*)0x02051724 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020552DC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02055B94 = 0xE3A00000; // mov r0, #0
		//tonccpy((u32*)0x02057F8C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205A830 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020629C0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0206E8FC = 0xE3A07000; // mov r7, #0
		*(u32*)0x02089CB8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089DA4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089E60 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089F14 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A198 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A27C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A3BC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A4F4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A8B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A8F0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208ADAC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208B0BC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020929F4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020939B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02093A94 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02094228 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02094440 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020944B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02094590 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02095208 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020959D0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02096D84 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02096FEC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02097308 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0209C0E0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DD1A0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DDB00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDD60 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDF00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DE0A4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x021081E8 = 0xE3A00000; // mov r0, #0
	}

	// Metal Torrent (Europe, Australia)
	// Saving not supported due to using more than one file
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K59V") == 0 && extendedMemory2) {
		*(u32*)0x020136EC = 0xE1A00000; // nop
		*(u32*)0x02017CEC = 0xE12FFF1E; // bx lr
		*(u32*)0x02017D0C = 0xE1A00000; // nop
		*(u32*)0x0201EAA8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02045FA0 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x02046088 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020460D0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020474E8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204BA30 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0204BA34 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204C0D0 = 0xE1A00000; // nop
		*(u32*)0x0204C0D4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C0EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C250 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C2E0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C310 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C3E0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C410 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D728 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D760 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D77C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D7B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D7DC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D7FC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204D818 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205173C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02055BAC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205A848 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020629D8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0206E894 = 0xE3A07000; // mov r7, #0
		*(u32*)0x02089AD0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089BBC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089C78 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089D2C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02089FB0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A094 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A1D4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A30C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A6CC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208A708 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208ABC4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208AED4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0209280C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020937D0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020938AC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02094040 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02094258 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020942C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020943A8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02095020 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020957E8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02096B9C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02096E04 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02096E50 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0209BEF8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DCFB8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DD918 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDB78 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDD18 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDEBC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02108000 = 0xE3A00000; // mov r0, #0
	}

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0) {
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02014BDC;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x0201A38C;

		setBL(0x0200B048, (u32)dsiSaveCreate);
		setBL(0x0200B070, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200B080 = 0xE1A00000; // nop
		setBL(0x0200B090, (u32)dsiSaveCreate);
		setBL(0x0200B0E8, (u32)dsiSaveOpen);
		*(u32*)0x0200B100 = 0xE1A00000; // nop
		setBL(0x0200B114, (u32)dsiSaveOpen);
		setBL(0x0200B124, (u32)dsiSaveRead);
		setBL(0x0200B12C, (u32)dsiSaveClose);
		setBL(0x0200B388, (u32)dsiSaveCreate);
		setBL(0x0200B39C, (u32)dsiSaveOpen);
		setBL(0x0200B5A4, (u32)dsiSaveSetLength);
		setBL(0x0200B5B4, (u32)dsiSaveWrite);
		setBL(0x0200B5BC, (u32)dsiSaveClose);
		setBL(0x02014BDC, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		setBL(0x0201A38C, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
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

		setBL(0x0200B350, (u32)dsiSaveCreate);
		setBL(0x0200B378, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200B388 = 0xE1A00000; // nop
		setBL(0x0200B398, (u32)dsiSaveCreate);
		setBL(0x0200B3F0, (u32)dsiSaveOpen);
		*(u32*)0x0200B408 = 0xE1A00000; // nop
		setBL(0x0200B41C, (u32)dsiSaveOpen);
		setBL(0x0200B42C, (u32)dsiSaveRead);
		setBL(0x0200B434, (u32)dsiSaveClose);
		setBL(0x0200B690, (u32)dsiSaveCreate);
		setBL(0x0200B6A4, (u32)dsiSaveOpen);
		setBL(0x0200B8AC, (u32)dsiSaveSetLength);
		setBL(0x0200B8BC, (u32)dsiSaveWrite);
		setBL(0x0200B8C4, (u32)dsiSaveClose);
		setBL(0x0201528C, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		setBL(0x0201AA44, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
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
		setBL(0x0200B134, (u32)dsiSaveCreate);
		setBL(0x0200B158, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200B168 = 0xE1A00000; // nop
		setBL(0x0200B174, (u32)dsiSaveCreate);
		setBL(0x0200B1D4, (u32)dsiSaveOpen);
		*(u32*)0x0200B1EC = 0xE1A00000; // nop
		setBL(0x0200B1FC, (u32)dsiSaveOpen);
		setBL(0x0200B210, (u32)dsiSaveRead);
		setBL(0x0200B218, (u32)dsiSaveClose);
		setBL(0x0200B478, (u32)dsiSaveCreate);
		setBL(0x0200B488, (u32)dsiSaveOpen);
		setBL(0x0200B694, (u32)dsiSaveSetLength);
		setBL(0x0200B6A4, (u32)dsiSaveWrite);
		setBL(0x0200B6AC, (u32)dsiSaveClose);
		setBL(0x02014718, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		setBL(0x02019C54, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
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
	// Music doesn't play on retail consoles
	else if (strncmp(romTid, "KWY", 3) == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 1;
		ce9->rumbleForce[1] = 1;

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200545C = 0xE1A00000; // nop
		setBL(0x0200547C, (u32)dsiSaveCreate);
		setBL(0x020054A0, (u32)dsiSaveGetResultCode);
		*(u32*)0x020054B0 = 0xE1A00000; // nop
		setBL(0x020054BC, (u32)dsiSaveCreate);
		*(u32*)0x020054C8 = 0xE1A00000; // nop
		*(u32*)0x020054CC = 0xE1A00000; // nop
		*(u32*)0x020054D0 = 0xE1A00000; // nop
		*(u32*)0x020054D4 = 0xE1A00000; // nop
		*(u32*)0x020054DC = 0xE1A00000; // nop
		setBL(0x02005534, (u32)dsiSaveOpen);
		*(u32*)0x0200554C = 0xE1A00000; // nop
		setBL(0x0200555C, (u32)dsiSaveOpen);
		setBL(0x02005570, (u32)dsiSaveRead);
		setBL(0x02005578, (u32)dsiSaveClose);
		*(u32*)0x020057C0 = 0xE1A00000; // nop
		setBL(0x020057E4, (u32)dsiSaveCreate);
		setBL(0x020057F4, (u32)dsiSaveOpen);
		setBL(0x020059FC, (u32)dsiSaveSetLength);
		setBL(0x02005A0C, (u32)dsiSaveWrite);
		setBL(0x02005A14, (u32)dsiSaveClose);
		*(u32*)0x02005A20 = 0xE1A00000; // nop
		*(u32*)0x02005A24 = 0xE1A00000; // nop
		*(u32*)0x02005A28 = 0xE1A00000; // nop
		*(u32*)0x02005A2C = 0xE1A00000; // nop
		*(u32*)0x02005A38 = 0xE1A00000; // nop
		*(u32*)0x02005A3C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020072CC = 0xE3A00901; // mov r0, #0x4000 (Shrink sound heap from 1MB to 16KB: Disables music)
			tonccpy((u32*)0x02008528, ce9->patches->musicPlay, 0xC);
			tonccpy((u32*)0x02008570, ce9->patches->musicStopEffect, 0xC);
		}
		if (ndsHeader->gameCode[3] == 'J') {
			ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201D008;
			ce9->patches->rumble_arm9[1][3] = *(u32*)0x020275F8;
			if (!extendedMemory2) {
				// Replace title music
				*(u32*)0x02012930 = 0xE3A0000A; // mov r0, #0xA
				setBL(0x02012934, 0x02008528);
				*(u32*)0x02012938 = 0xE1A00000; // nop
				*(u32*)0x0201293C = 0xE1A00000; // nop
				*(u32*)0x02012940 = 0xE1A00000; // nop
				*(u32*)0x02012944 = 0xE1A00000; // nop
				*(u32*)0x02012964 = 0xE1A00000; // nop
				*(u32*)0x02012980 = 0xE1A00000; // nop
				*(u32*)0x02012DC4 = 0xE1A00000; // nop
				*(u32*)0x02012DE0 = 0xE1A00000; // nop
			}
			setBL(0x0201D008, (int)ce9->patches->rumble_arm9[0]); // Rumble when Luna gets shocked
			setBL(0x020275F8, (int)ce9->patches->rumble_arm9[1]); // Rumble when planet is destroyed
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
				// Replace title music
				*(u32*)0x020128E4 = 0xE3A0000A; // mov r0, #0xA
				setBL(0x020128E8, 0x02008528);
				*(u32*)0x020128EC = 0xE1A00000; // nop
				*(u32*)0x020128F0 = 0xE1A00000; // nop
				*(u32*)0x020128F4 = 0xE1A00000; // nop
				*(u32*)0x020128F8 = 0xE1A00000; // nop
				*(u32*)0x02012918 = 0xE1A00000; // nop
				*(u32*)0x02012934 = 0xE1A00000; // nop
				*(u32*)0x02012D7C = 0xE1A00000; // nop
				*(u32*)0x02012D98 = 0xE1A00000; // nop
			}
			setBL(0x0201CFB0, (int)ce9->patches->rumble_arm9[0]); // Rumble when Luna gets shocked
			setBL(0x0202750C, (int)ce9->patches->rumble_arm9[1]); // Rumble when planet is destroyed
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

	// Missy Mila Twisted Tales (Europe)
	else if (strcmp(romTid, "KM7P") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020151B8 = 0xE1A00000; // nop
		*(u32*)0x0201874C = 0xE1A00000; // nop
		*(u32*)0x0201BF48 = 0xE1A00000; // nop
		*(u32*)0x0201DD10 = 0xE1A00000; // nop
		*(u32*)0x0201DD14 = 0xE1A00000; // nop
		*(u32*)0x0201DD20 = 0xE1A00000; // nop
		*(u32*)0x0201DE80 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201DEDC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		if (!extendedMemory2) {
			*(u32*)0x0201E010 = 0x02095240;
		}
		*(u32*)0x0201F350 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02022540 = 0xE1A00000; // nop
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

	// Model Academy (Europe)
	else if (strcmp(romTid, "K8MP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02011A0C = 0xE1A00000; // nop
		*(u32*)0x02015788 = 0xE1A00000; // nop
		*(u32*)0x0201AB14 = 0xE1A00000; // nop
		*(u32*)0x0201C988 = 0xE1A00000; // nop
		*(u32*)0x0201C98C = 0xE1A00000; // nop
		*(u32*)0x0201C998 = 0xE1A00000; // nop
		*(u32*)0x0201CAF8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201CB54, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E058 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02020E78 = 0xE1A00000; // nop
		setBL(0x020B19E4, (u32)dsiSaveCreate);
		setBL(0x020B19F4, (u32)dsiSaveOpen);
		setBL(0x020B1A04, (u32)dsiSaveGetResultCode);
		setBL(0x020B1A40, (u32)dsiSaveSetLength);
		setBL(0x020B1A50, (u32)dsiSaveWrite);
		setBL(0x020B1A58, (u32)dsiSaveClose);
		setBL(0x020B1A94, (u32)dsiSaveOpen);
		setBL(0x020B1AA4, (u32)dsiSaveGetResultCode);
		setBL(0x020B1ABC, (u32)dsiSaveGetLength);
		setBL(0x020B1ACC, (u32)dsiSaveRead);
		setBL(0x020B1AD4, (u32)dsiSaveClose);
		setBL(0x020B1B0C, (u32)dsiSaveOpen);
		setBL(0x020B1B1C, (u32)dsiSaveGetResultCode);
		setBL(0x020B1B34, (u32)dsiSaveClose);
	}

	// Monster Buster Club (USA)
	else if (strcmp(romTid, "KXBE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		/* *(u32*)0x0207F058 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F05C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F138 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F13C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F4FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F63C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F640 = 0xE12FFF1E; // bx lr */
		*(u32*)0x0207F098 = 0xE1A00000; // nop
		*(u32*)0x0207F0B8 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x0207F17C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0207F224 = 0xE1A00000; // nop
		setBL(0x0207F244, (u32)dsiSaveCreate);
		setBL(0x0207F254, (u32)dsiSaveOpen);
		*(u32*)0x0207F278 = 0xE1A00000; // nop
		setBL(0x0207F298, (u32)dsiSaveSetLength);
		setBL(0x0207F2B4, (u32)dsiSaveWrite);
		setBL(0x0207F2C8, (u32)dsiSaveClose);
		*(u32*)0x0207F2F0 = 0xE1A00000; // nop
		setBL(0x0207F494, (u32)dsiSaveOpen);
		setBL(0x0207F4C4, (u32)dsiSaveRead);
		setBL(0x0207F4D8, (u32)dsiSaveClose);
		*(u32*)0x0207F530 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F560 = 0xE3A00001; // mov r0, #1
		setBL(0x0207F584, 0x0207F5C4);
		*(u32*)0x0207F5E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F654 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02094318 = 0xE1A00000; // nop
		tonccpy((u32*)0x02094EAC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02097954 = 0xE1A00000; // nop
		*(u32*)0x0209AB24 = 0xE1A00000; // nop
		*(u32*)0x0209C8E4 = 0xE1A00000; // nop
		*(u32*)0x0209C8E8 = 0xE1A00000; // nop
		*(u32*)0x0209C8F4 = 0xE1A00000; // nop
		*(u32*)0x0209CA54 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0209CAB0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0209DEA8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0209E2B0 = 0xE1A00000; // nop
		*(u32*)0x0209E2B4 = 0xE1A00000; // nop
		*(u32*)0x0209E2B8 = 0xE1A00000; // nop
		*(u32*)0x0209E2BC = 0xE1A00000; // nop
		*(u32*)0x020A0C08 = 0xE1A00000; // nop
	}

	// Monster Buster Club (Europe)
	else if (strcmp(romTid, "KXBP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		/* *(u32*)0x0207EF64 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207EF68 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F048 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F414 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F418 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F558 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F55C = 0xE12FFF1E; // bx lr */
		*(u32*)0x0207EFA4 = 0xE1A00000; // nop
		*(u32*)0x0207EFC4 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x0207F084 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0207F12C = 0xE1A00000; // nop
		setBL(0x0207F14C, (u32)dsiSaveCreate);
		setBL(0x0207F15C, (u32)dsiSaveOpen);
		*(u32*)0x0207F180 = 0xE1A00000; // nop
		setBL(0x0207F1A0, (u32)dsiSaveSetLength);
		setBL(0x0207F1BC, (u32)dsiSaveWrite);
		setBL(0x0207F1D0, (u32)dsiSaveClose);
		*(u32*)0x0207F1F8 = 0xE1A00000; // nop
		setBL(0x0207F3AC, (u32)dsiSaveOpen);
		setBL(0x0207F3E0, (u32)dsiSaveRead);
		setBL(0x0207F3F4, (u32)dsiSaveClose);
		*(u32*)0x0207F44C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F47C = 0xE3A00001; // mov r0, #1
		setBL(0x0207F4A0, 0x0207F4E0);
		*(u32*)0x0207F504 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F584 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0209424C = 0xE1A00000; // nop
		tonccpy((u32*)0x02094DE0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02097888 = 0xE1A00000; // nop
		*(u32*)0x0209AA58 = 0xE1A00000; // nop
		*(u32*)0x0209C818 = 0xE1A00000; // nop
		*(u32*)0x0209C81C = 0xE1A00000; // nop
		*(u32*)0x0209C828 = 0xE1A00000; // nop
		*(u32*)0x0209C988 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0209C9E4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0209DDDC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0209E1E4 = 0xE1A00000; // nop
		*(u32*)0x0209E1E8 = 0xE1A00000; // nop
		*(u32*)0x0209E1EC = 0xE1A00000; // nop
		*(u32*)0x0209E1F0 = 0xE1A00000; // nop
		*(u32*)0x020A0B3C = 0xE1A00000; // nop
	}

	// Mr. Brain (Japan)
	else if (strcmp(romTid, "KMBJ") == 0) {
		*(u32*)0x020054EC = 0xE1A00000; // nop
		*(u32*)0x02005504 = 0xE1A00000; // nop
		*(u32*)0x020057A0 = 0xE1A00000; // nop
		setBL(0x02005A40, (u32)dsiSaveOpen);
		setBL(0x02005A5C, (u32)dsiSaveGetLength);
		setBL(0x02005A78, (u32)dsiSaveRead);
		setBL(0x02005AA0, (u32)dsiSaveClose);
		setBL(0x02005AF8, (u32)dsiSaveCreate);
		setBL(0x02005B08, (u32)dsiSaveOpen);
		setBL(0x02005B34, (u32)dsiSaveSetLength);
		setBL(0x02005B54, (u32)dsiSaveWrite);
		setBL(0x02005B6C, (u32)dsiSaveClose);
		*(u32*)0x0200648C = 0xE1A00000; // nop
		*(u32*)0x0200949C = 0xE1A00000; // nop
		*(u32*)0x02025AB4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02026748, dsiSaveGetResultCode, 0xC);
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

	// Mr. Driller: Drill Till You Drop (USA)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x02032410, (u32)dsiSaveOpen);
		setBL(0x02032428, (u32)dsiSaveClose);
		*(u32*)0x02032450 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02032470 = 0xE1A00000; // nop
		setBL(0x020324A8, (u32)dsiSaveCreate);
		setBL(0x020324CC, (u32)dsiSaveGetResultCode);
		setBL(0x0203277C, (u32)dsiSaveOpen);
		setBL(0x02032790, (u32)dsiSaveSeek);
		setBL(0x020327A0, (u32)dsiSaveRead);
		setBL(0x020327C0, (u32)dsiSaveClose);
		setBL(0x02032858, (u32)dsiSaveOpen);
		setBL(0x02032888, (u32)dsiSaveWrite);
		setBL(0x02032948, (u32)dsiSaveClose);
		*(u32*)0x02034DA4 = 0xE1A00000; // nop
		*(u32*)0x02036DA4 = 0xE1A00000; // nop
		*(u32*)0x0203AAB4 = 0xE1A00000; // nop
		*(u32*)0x0203AB34 = 0xE1A00000; // nop
		*(u32*)0x0203B1F4 = 0xE1A00000; // nop
		*(u32*)0x0203BC4C = 0xE1A00000; // nop
		*(u32*)0x0203BCE8 = 0xE1A00000; // nop
		*(u32*)0x0203BD9C = 0xE1A00000; // nop
		*(u32*)0x0203BE50 = 0xE1A00000; // nop
		*(u32*)0x0203BEF0 = 0xE1A00000; // nop
		*(u32*)0x0203BF70 = 0xE1A00000; // nop
		*(u32*)0x0203BFEC = 0xE1A00000; // nop
		*(u32*)0x0203C070 = 0xE1A00000; // nop
		*(u32*)0x0203C110 = 0xE1A00000; // nop
		*(u32*)0x0203C1CC = 0xE1A00000; // nop
		*(u32*)0x0203C308 = 0xE1A00000; // nop
		*(u32*)0x0203C36C = 0xE1A00000; // nop
		*(u32*)0x0203C434 = 0xE1A00000; // nop
		*(u32*)0x0203C4A4 = 0xE1A00000; // nop
		*(u32*)0x0203C530 = 0xE1A00000; // nop
		*(u32*)0x0203C5A0 = 0xE1A00000; // nop
		*(u32*)0x0203C628 = 0xE1A00000; // nop
		*(u32*)0x0203C698 = 0xE1A00000; // nop
		*(u32*)0x0203C7AC = 0xE1A00000; // nop
		*(u32*)0x0203C814 = 0xE1A00000; // nop
		*(u32*)0x0203C894 = 0xE1A00000; // nop
		*(u32*)0x0203C8F8 = 0xE1A00000; // nop
		*(u32*)0x0203C9B0 = 0xE1A00000; // nop
		*(u32*)0x0203CA20 = 0xE1A00000; // nop
		*(u32*)0x0203E32C = 0xE1A00000; // nop
		*(u32*)0x020400D4 = 0xE1A00000; // nop
		*(u32*)0x020400D8 = 0xE1A00000; // nop
		*(u32*)0x020400E4 = 0xE1A00000; // nop
		*(u32*)0x02040244 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020402A0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020417B8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02044DDC = 0xE1A00000; // nop
	}

	// Mr. Driller: Drill Till You Drop (Europe, Australia)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x02032160, (u32)dsiSaveOpen);
		setBL(0x02032178, (u32)dsiSaveClose);
		*(u32*)0x020321A0 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc) 
		*(u32*)0x020321C0 = 0xE1A00000; // nop
		setBL(0x020321F8, (u32)dsiSaveCreate);
		setBL(0x0203221C, (u32)dsiSaveGetResultCode);
		setBL(0x020324CC, (u32)dsiSaveOpen);
		setBL(0x020324E0, (u32)dsiSaveSeek);
		setBL(0x020324F0, (u32)dsiSaveRead);
		setBL(0x02032510, (u32)dsiSaveClose);
		setBL(0x020325A8, (u32)dsiSaveOpen);
		setBL(0x020325D8, (u32)dsiSaveWrite);
		setBL(0x02032698, (u32)dsiSaveClose);
		*(u32*)0x02034AF4 = 0xE1A00000; // nop
		*(u32*)0x02036B10 = 0xE1A00000; // nop
		*(u32*)0x0203A820 = 0xE1A00000; // nop
		*(u32*)0x0203A8A0 = 0xE1A00000; // nop
		*(u32*)0x0203AF60 = 0xE1A00000; // nop
		*(u32*)0x0203B9B8 = 0xE1A00000; // nop
		*(u32*)0x0203BA54 = 0xE1A00000; // nop
		*(u32*)0x0203BB08 = 0xE1A00000; // nop
		*(u32*)0x0203BBBC = 0xE1A00000; // nop
		*(u32*)0x0203BC5C = 0xE1A00000; // nop
		*(u32*)0x0203BCDC = 0xE1A00000; // nop
		*(u32*)0x0203BD58 = 0xE1A00000; // nop
		*(u32*)0x0203BDDC = 0xE1A00000; // nop
		*(u32*)0x0203BE7C = 0xE1A00000; // nop
		*(u32*)0x0203BF38 = 0xE1A00000; // nop
		*(u32*)0x0203C074 = 0xE1A00000; // nop
		*(u32*)0x0203C0D8 = 0xE1A00000; // nop
		*(u32*)0x0203C1A0 = 0xE1A00000; // nop
		*(u32*)0x0203C210 = 0xE1A00000; // nop
		*(u32*)0x0203C29C = 0xE1A00000; // nop
		*(u32*)0x0203C30C = 0xE1A00000; // nop
		*(u32*)0x0203C394 = 0xE1A00000; // nop
		*(u32*)0x0203C404 = 0xE1A00000; // nop
		*(u32*)0x0203C518 = 0xE1A00000; // nop
		*(u32*)0x0203C580 = 0xE1A00000; // nop
		*(u32*)0x0203C600 = 0xE1A00000; // nop
		*(u32*)0x0203C664 = 0xE1A00000; // nop
		*(u32*)0x0203C71C = 0xE1A00000; // nop
		*(u32*)0x0203C78C = 0xE1A00000; // nop
		*(u32*)0x0203E098 = 0xE1A00000; // nop
		*(u32*)0x0203FE40 = 0xE1A00000; // nop
		*(u32*)0x0203FE44 = 0xE1A00000; // nop
		*(u32*)0x0203FE50 = 0xE1A00000; // nop
		*(u32*)0x0203FFB0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204000C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02041524 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02044B48 = 0xE1A00000; // nop
	}

	// Sakutto Hamareru Hori Hori Action: Mr. Driller (Japan)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRJ") == 0) {
		*(u32*)0x0201FE10 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202000C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202027C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x0203204C, (u32)dsiSaveOpen);
		setBL(0x02032064, (u32)dsiSaveClose);
		*(u32*)0x0203208C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x020320AC = 0xE1A00000; // nop
		setBL(0x020320E4, (u32)dsiSaveCreate);
		setBL(0x02032108, (u32)dsiSaveGetResultCode);
		setBL(0x020323B8, (u32)dsiSaveOpen);
		setBL(0x020323CC, (u32)dsiSaveSeek);
		setBL(0x020323DC, (u32)dsiSaveRead);
		setBL(0x020323FC, (u32)dsiSaveClose);
		setBL(0x02032494, (u32)dsiSaveOpen);
		setBL(0x020324C4, (u32)dsiSaveWrite);
		setBL(0x02032584, (u32)dsiSaveClose);
		*(u32*)0x020349DC = 0xE1A00000; // nop
		*(u32*)0x0203697C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02036980 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203A8C8 = 0xE1A00000; // nop
		*(u32*)0x0203A950 = 0xE1A00000; // nop
		*(u32*)0x0203AFAC = 0xE1A00000; // nop
		*(u32*)0x0203DA98 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203DAF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203DB74 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203F9DC = 0xE1A00000; // nop
		*(u32*)0x02041740 = 0xE1A00000; // nop
		*(u32*)0x02041744 = 0xE1A00000; // nop
		*(u32*)0x02041750 = 0xE1A00000; // nop
		*(u32*)0x02041894 = 0xE1A00000; // nop
		*(u32*)0x02041898 = 0xE1A00000; // nop
		*(u32*)0x0204189C = 0xE1A00000; // nop
		*(u32*)0x020418A0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020418FC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02042E48 = 0xE1A00000; // nop
		*(u32*)0x02042E50 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
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

	// Need for Speed: Nitro-X (USA)
	// Need for Speed: Nitro-X (Europe, Australia)
	// Requires 8MB of RAM
	else if (strncmp(romTid, "KNP", 3) == 0 && extendedMemory2) {
		/*const u32 dsiSaveCreateT = 0x0201D090;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveGetInfoT = 0x0201D0A0;
		*(u16*)dsiSaveGetInfoT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetInfoT + 4), dsiSaveGetInfo, 0xC);

		const u32 dsiSaveOpenT = 0x0201D0B0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x0201D0C0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x0201D0D0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x0201D0E0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x0201D0F0;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);*/

		*(u32*)0x0200498C = 0xE1A00000; // nop
		doubleNopT(0x0200511E);
		doubleNopT(0x02005124);
		doubleNopT(0x02005272);
		*(u32*)0x0201BA24 = 0xE1A00000; // nop
		//tonccpy((u32*)0x0201C5A8, dsiSaveGetResultCode, 0xC);
		//tonccpy((u32*)0x0201D178, dsiSaveSetLength, 0xC);
		*(u32*)0x0201F744 = 0xE1A00000; // nop
		*(u32*)0x0202CD44 = 0xE1A00000; // nop
		*(u32*)0x0202ECE0 = 0xE1A00000; // nop
		*(u32*)0x0202ECE4 = 0xE1A00000; // nop
		*(u32*)0x0202ECF0 = 0xE1A00000; // nop
		*(u32*)0x0202EE50 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202EEAC, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x02030430 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0203044C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02030450 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203046C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02030470 = 0xE12FFF1E; // bx lr
		*(u32*)0x02033DD4 = 0xE1A00000; // nop
		*(u16*)0x020EBFC4 = 0x4770; // bx lr
		/*setBLThumb(0x020EBFDC, dsiSaveOpenT);
		setBLThumb(0x020EBFEA, dsiSaveCloseT);
		setBLThumb(0x020EBFF6, dsiSaveGetInfoT);
		doubleNopT(0x020EC00A);
		setBLThumb(0x020EC01A, dsiSaveCreateT);
		setBLThumb(0x020EC02C, dsiSaveOpenT);
		setBLThumb(0x020EC03C, dsiSaveCloseT);
		setBLThumb(0x020EC05A, dsiSaveOpenT);
		setBLThumb(0x020EC07A, dsiSaveSeekT);
		setBLThumb(0x020EC090, dsiSaveWriteT);
		setBLThumb(0x020EC098, dsiSaveCloseT);*/
		*(u16*)0x020EC0C0 = 0x4770; // bx lr
		/*setBLThumb(0x020EC0D8, dsiSaveOpenT);
		setBLThumb(0x020EC0E6, dsiSaveCloseT);
		setBLThumb(0x020EC0F2, dsiSaveGetInfoT);
		doubleNopT(0x020EC106);
		setBLThumb(0x020EC118, dsiSaveCreateT);
		setBLThumb(0x020EC126, dsiSaveOpenT);
		setBLThumb(0x020EC136, dsiSaveCloseT);
		setBLThumb(0x020EC14C, dsiSaveOpenT);
		setBLThumb(0x020EC162, dsiSaveSeekT);
		setBLThumb(0x020EC176, dsiSaveReadT);
		setBLThumb(0x020EC17E, dsiSaveCloseT);*/
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

	// Nintendogs (China)
	// Requires more than 8MB of RAM?
	/*else if (strcmp(romTid, "KDOC") == 0 && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0202A4FC = 0xE1A00000; // nop
		*(u32*)0x0202A524 = 0xE1A00000; // nop
		setBL(0x0202A6EC, (u32)dsiSaveSeek);
		setBL(0x0202A6FC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x0202A788 = 0xE1A00000; // nop
		*(u32*)0x0202A7E0 = 0xE1A00000; // nop
		setBL(0x0202A8C8, (u32)dsiSaveSeek);
		setBL(0x0202A8D8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202A95C, (u32)dsiSaveSeek);
		setBL(0x0202A96C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x0202A9F8 = 0xE1A00000; // nop
		*(u32*)0x0202AA50 = 0xE1A00000; // nop
		setBL(0x0202AB28, (u32)dsiSaveSeek);
		setBL(0x0202AB38, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202BB94, (u32)dsiSaveOpen);
		setBL(0x0202BBA8, (u32)dsiSaveCreate);
		setBL(0x0202BBB8, (u32)dsiSaveGetResultCode);
		setBL(0x0202BBD4, (u32)dsiSaveOpen);
		setBL(0x0202BBFC, (u32)dsiSaveSetLength);
		setBL(0x0202BC04, (u32)dsiSaveClose);
		setBL(0x0202BC14, (u32)dsiSaveOpen);
		*(u32*)0x0202BD3C = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0202BD44, (u32)dsiSaveClose);
		setBL(0x0202BE14, (u32)dsiSaveSeek);
		setBL(0x0202BE24, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0202BE44, (u32)dsiSaveSeek);
		setBL(0x0202BE54, (u32)dsiSaveRead); // dsiSaveReadAsync
		*(u32*)0x0202BCC0 = 0xE1A00000; // nop
		*(u32*)0x0202BD60 = 0xE1A00000; // nop
		*(u32*)0x0202BF0C = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0202BF14, (u32)dsiSaveClose);
		*(u32*)0x0202C400 = 0xE1A00000; // nop
		*(u32*)0x0202C408 = 0xE1A00000; // nop
		*(u32*)0x020FD6AC = 0xE1A00000; // nop
		*(u32*)0x021024D8 = 0xE1A00000; // nop
		*(u32*)0x0210A34C = 0xE1A00000; // nop
		*(u32*)0x0210C1D0 = 0xE1A00000; // nop
		*(u32*)0x0210C1D4 = 0xE1A00000; // nop
		*(u32*)0x0210C1E0 = 0xE1A00000; // nop
		*(u32*)0x0210C324 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0210C380, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0210D590 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0210D594 = 0xE12FFF1E; // bx lr
		*(u32*)0x0210D59C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0210D5A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0210D824 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02112108 = 0xE1A00000; // nop
	}*/

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
		*(u32*)0x020ACE4C = 0xE3A02C07; // mov r2, #0x700
		*(u32*)0x020ACE6C = 0xE2840B01; // add r0, r4, #0x400
		*(u32*)0x020ACE74 = 0xE1A00004; // mov r0, r4
		*(u32*)0x020ACE7C = 0xE1A00000; // nop
		*(u32*)0x020ACE80 = 0xE1A00000; // nop
		*(u32*)0x020ACE84 = 0xE1A00000; // nop
		*(u32*)0x020ACE88 = 0xE1A00000; // nop
		*(u32*)0x020ACE8C = 0xE1A00000; // nop
		*(u32*)0x020ACEA0 = 0xE2841B01; // add r1, r4, #0x400
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

	// Orion's Odyssey (USA)
	// Due to our save implementation, save data is stored in both slots
	else if (strcmp(romTid, "K6TE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02011FAC = 0xE1A00000; // nop
		*(u32*)0x02015790 = 0xE1A00000; // nop
		*(u32*)0x02018EE0 = 0xE1A00000; // nop
		*(u32*)0x02018EE0 = 0xE1A00000; // nop
		*(u32*)0x0201AC7C = 0xE1A00000; // nop
		*(u32*)0x0201AC80 = 0xE1A00000; // nop
		*(u32*)0x0201AC8C = 0xE1A00000; // nop
		*(u32*)0x0201ADEC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201AE48, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201AF7C = 0x02113EA0;
		*(u32*)0x0201C1F4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201F084 = 0xE1A00000; // nop
		*(u32*)0x02020B68 = 0xE1A00000; // nop
		*(u32*)0x02020B80 = 0xE1A00000; // nop
		setBL(0x020284D0, (u32)dsiSaveDelete);
		setBL(0x020284DC, (u32)dsiSaveCreate);
		setBL(0x020284EC, (u32)dsiSaveOpen);
		setBL(0x0202851C, (u32)dsiSaveSetLength);
		setBL(0x02028538, (u32)dsiSaveDelete);
		setBL(0x02028544, (u32)dsiSaveCreate);
		setBL(0x02028554, (u32)dsiSaveOpen);
		setBL(0x0202857C, (u32)dsiSaveSetLength);
		setBL(0x0202858C, (u32)dsiSaveWrite);
		setBL(0x02028594, (u32)dsiSaveClose);
		*(u32*)0x02028C3C = 0xE1A00000; // nop (dsiSaveOpenDir)
		setBL(0x02028C50, (u32)dsiSaveOpen);
		setBL(0x02028C6C, (u32)dsiSaveOpen);
		setBL(0x02028CB0, (u32)dsiSaveGetLength);
		setBL(0x02028CD4, (u32)dsiSaveRead);
		setBL(0x02028E0C, (u32)dsiSaveClose);
		setBL(0x02028E54, (u32)dsiSaveGetLength);
		setBL(0x02028E78, (u32)dsiSaveRead);
		setBL(0x02028FB0, (u32)dsiSaveClose);
		*(u32*)0x02028FC8 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x02029030 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02029048 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x02029060 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0202907C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x020290DC = 0xE1A00000; // nop (dsiSaveCreateDir)
		setBL(0x020290FC, (u32)dsiSaveCreate);
		setBL(0x02029130, (u32)dsiSaveOpen);
		setBL(0x02029148, (u32)dsiSaveSetLength);
		setBL(0x02029158, (u32)dsiSaveWrite);
		setBL(0x02029160, (u32)dsiSaveClose);
		setBL(0x0202917C, (u32)dsiSaveSetLength);
		*(u32*)0x020296B0 = 0xE1A00000; // nop
	}

	// Paul's Monster Adventure (USA)
	else if (strcmp(romTid, "KP9E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02013828 = 0xE1A00000; // nop
		*(u32*)0x02016F48 = 0xE1A00000; // nop
		*(u32*)0x0201C8B8 = 0xE1A00000; // nop
		*(u32*)0x0201E6E0 = 0xE1A00000; // nop
		*(u32*)0x0201E6E4 = 0xE1A00000; // nop
		*(u32*)0x0201E6F0 = 0xE1A00000; // nop
		*(u32*)0x0201E850 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201E8AC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201FB30 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02022E50 = 0xE1A00000; // nop
		setBL(0x02047940, (u32)dsiSaveOpen);
		setBL(0x02047958, (u32)dsiSaveCreate);
		setBL(0x02047970, (u32)dsiSaveOpen);
		setBL(0x02047990, (u32)dsiSaveWrite);
		setBL(0x020479A0, (u32)dsiSaveClose);
		setBL(0x020479BC, (u32)dsiSaveClose);
		setBL(0x020479F8, (u32)dsiSaveOpen);
		setBL(0x02047A18, (u32)dsiSaveRead);
		setBL(0x02047A28, (u32)dsiSaveClose);
		setBL(0x02047A44, (u32)dsiSaveClose);
		setBL(0x02047AF4, (u32)dsiSaveCreate);
		setBL(0x02047B04, (u32)dsiSaveOpen);
		setBL(0x02047B30, (u32)dsiSaveClose);
		setBL(0x02047B5C, (u32)dsiSaveCreate);
		setBL(0x02047B6C, (u32)dsiSaveOpen);
		setBL(0x02047B98, (u32)dsiSaveClose);
	}

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
		//*(u32*)0x0203A20C = 0xE12FFF1E; // bx lr
		setBL(0x02048524, (u32)dsiSaveOpen);
		setBL(0x0204853C, (u32)dsiSaveCreate);
		setBL(0x02048554, (u32)dsiSaveOpen);
		setBL(0x02048574, (u32)dsiSaveWrite);
		setBL(0x02048584, (u32)dsiSaveClose);
		setBL(0x020485A0, (u32)dsiSaveClose);
		setBL(0x020485DC, (u32)dsiSaveOpen);
		setBL(0x020485FC, (u32)dsiSaveRead);
		setBL(0x0204860C, (u32)dsiSaveClose);
		setBL(0x02048628, (u32)dsiSaveClose);
		setBL(0x020486D8, (u32)dsiSaveCreate);
		setBL(0x020486E8, (u32)dsiSaveOpen);
		setBL(0x02048714, (u32)dsiSaveClose);
		setBL(0x02048740, (u32)dsiSaveCreate);
		setBL(0x02048750, (u32)dsiSaveOpen);
		setBL(0x0204877C, (u32)dsiSaveClose);
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
		setBL(0x0202EE44, (u32)dsiSaveOpen);
		setBL(0x0202EE5C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202EE74, (u32)dsiSaveOpen);
		setBL(0x0202EE94, (u32)dsiSaveWrite);
		setBL(0x0202EEA4, (u32)dsiSaveClose);
		setBL(0x0202EEB4, (u32)dsiSaveClose);
		setBL(0x0202EEF4, (u32)dsiSaveOpen);
		setBL(0x0202EF18, (u32)dsiSaveRead);
		setBL(0x0202EF28, (u32)dsiSaveClose);
		setBL(0x0202EF38, (u32)dsiSaveClose);
		setBL(0x0202F018, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202F028, (u32)dsiSaveOpen);
		setBL(0x0202F054, (u32)dsiSaveClose);
		setBL(0x0202F08C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202F09C, (u32)dsiSaveOpen);
		setBL(0x0202F0C8, (u32)dsiSaveClose);
		//*(u32*)0x0203A730 = 0xE3A00001; // mov r0, #1
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
		*(u32*)0x0202EE48 = 0xE1A00000; // nop
		*(u32*)0x0202EE50 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0202EE70 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202EE74 = 0xE12FFF1E; // bx lr
		*(u16*)0x0209F778 = 0x46C0; // nop
		*(u16*)0x0209F77A = 0x46C0; // nop
	}

	// GO Series: Picdun (USA)
	// GO Series: Picdun (Europe)
	else if (strcmp(romTid, "KPQE") == 0 || strcmp(romTid, "KPQP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020052A4 = 0xE1A00000; // nop
		*(u32*)0x0200A5C4 = 0xE1A00000; // nop
		//*(u32*)0x0200A66C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200A670 = 0xE12FFF1E; // bx lr
		setBL(0x0200B038, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0200B088, (u32)dsiSaveOpen);
		setBL(0x0200B11C, (u32)dsiSaveGetLength);
		setBL(0x0200B138, (u32)dsiSaveRead);
		setBL(0x0200B140, (u32)dsiSaveClose);
		setBL(0x0200B184, (u32)dsiSaveOpen);
		setBL(0x0200B20C, (u32)dsiSaveSeek);
		setBL(0x0200B288, (u32)dsiSaveClose);
		setBL(0x0200B2B0, (u32)dsiSaveWrite);
		setBL(0x0200B2BC, (u32)dsiSaveClose);
		*(u32*)0x020642CC = 0xE1A00000; // nop
		tonccpy((u32*)0x02064E50, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02067E34 = 0xE1A00000; // nop
		*(u32*)0x0206CF2C = 0xE1A00000; // nop
		*(u32*)0x0206ED54 = 0xE1A00000; // nop
		*(u32*)0x0206ED58 = 0xE1A00000; // nop
		*(u32*)0x0206ED64 = 0xE1A00000; // nop
		*(u32*)0x0206EEC4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206EF20, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x020701A4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02073558 = 0xE1A00000; // nop
	}

	// Danjo RPG: Picudan (Japan)
	else if (strcmp(romTid, "KPQJ") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x0200528C = 0xE1A00000; // nop
		*(u32*)0x020097D8 = 0xE1A00000; // nop
		setBL(0x02009F24, (u32)dsiSaveOpen);
		setBL(0x02009F8C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02009FD8, (u32)dsiSaveGetLength);
		setBL(0x02009FF4, (u32)dsiSaveRead);
		setBL(0x02009FFC, (u32)dsiSaveClose);
		setBL(0x0200A040, (u32)dsiSaveOpen);
		setBL(0x0200A0C0, (u32)dsiSaveGetLength);
		setBL(0x0200A0D0, (u32)dsiSaveSeek);
		setBL(0x0200A14C, (u32)dsiSaveClose);
		setBL(0x0200A174, (u32)dsiSaveWrite);
		setBL(0x0200A180, (u32)dsiSaveClose);
		*(u32*)0x02061C34 = 0xE1A00000; // nop
		tonccpy((u32*)0x020627B8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02065714 = 0xE1A00000; // nop
		*(u32*)0x0206A7F0 = 0xE1A00000; // nop
		*(u32*)0x0206C610 = 0xE1A00000; // nop
		*(u32*)0x0206C614 = 0xE1A00000; // nop
		*(u32*)0x0206C620 = 0xE1A00000; // nop
		*(u32*)0x0206C780 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0206C7DC, extendedMemory2 ? 0x02F00000 : heapEnd+0xC00000); // mov r0, extendedMemory2 ? #0x2F00000 (mirrors to 0x2700000 on debug DS units) : #0x2FC0000 (mirrors to 0x23C0000 on retail DS units)
		*(u32*)0x0206DA60 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02070E14 = 0xE1A00000; // nop
	}

	// Art Style: PiCTOBiTS (USA)
	// Art Style: PiCOPiCT (Europe, Australia)
	else if (strcmp(romTid, "KAPE") == 0 || strcmp(romTid, "KAPV") == 0) {
		*(u32*)0x0200518C = 0xE1A00000; // nop
		*(u32*)0x02005198 = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		setBL(0x02005828, (u32)dsiSaveOpen);
		setBL(0x020058E8, (u32)dsiSaveOpen);
		setBL(0x02005928, (u32)dsiSaveRead);
		setBL(0x0200595C, (u32)dsiSaveClose);
		//*(u32*)0x020059E4 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020059E8 = 0xE12FFF1E; // bx lr
		setBL(0x02005A18, (u32)dsiSaveCreate);
		setBL(0x02005A28, (u32)dsiSaveGetResultCode);
		setBL(0x02005A4C, (u32)dsiSaveOpen);
		setBL(0x02005A6C, (u32)dsiSaveSetLength);
		setBL(0x02005A88, (u32)dsiSaveWrite);
		setBL(0x02005AB4, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
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
		} else if (ndsHeader->gameCode[3] == 'V') {
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
	}

	// Art Style: PiCOPiCT (Japan)
	else if (strcmp(romTid, "KAPJ") == 0) {
		*(u32*)0x02005194 = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A4 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x020051B0 = 0xE1A00000; // nop
		setBL(0x020058B0, (u32)dsiSaveOpen);
		setBL(0x02005968, (u32)dsiSaveOpen);
		setBL(0x020059B0, (u32)dsiSaveRead);
		setBL(0x020059F4, (u32)dsiSaveClose);
		//*(u32*)0x02005A8C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02005A90 = 0xE12FFF1E; // bx lr
		setBL(0x02005ABC, (u32)dsiSaveCreate);
		setBL(0x02005ACC, (u32)dsiSaveGetResultCode);
		setBL(0x02005AE8, (u32)dsiSaveOpen);
		setBL(0x02005B04, (u32)dsiSaveSetLength);
		setBL(0x02005B20, (u32)dsiSaveWrite);
		setBL(0x02005B4C, (u32)dsiSaveClose);
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
	// Audio doesn't play on retail consoles
	// Requires 8MB of RAM for full usage
	else if (strcmp(romTid, "KHRE") == 0) {
		/*if (!extendedMemory2) {
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				u32* addr = (u32*)0x02004000;
				if (addr[i] >= 0x02294000 && addr[i] < 0x022B0000) {
					addr[i] -= 0x120000;
				}
			}
		}*/

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020050E8 = 0xE3A00901; // mov r0, #0x4000
			*(u32*)0x0202F034 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202F038 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202F098 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202F09C = 0xE12FFF1E; // bx lr
			*(u32*)0x0205837C = 0xE1A00000; // nop
			*(u32*)0x02058608 = 0xE1A00000; // nop
		}
		*(u32*)0x0200511C = 0xE1A00000; // nop
		for (int i = 0; i < 7; i++) {
			u32* offset = (u32*)0x02005194;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x020051B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200CF44 = 0xE3A00000; // mov r0, #0
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
		patchHiHeapDSiWare(0x0201F2AC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201F3E0 -= 0x30000;
		*(u32*)0x02020614 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023D6C = 0xE1A00000; // nop
		*(u32*)0x02025884 = 0xE1A00000; // nop
		*(u32*)0x02044080 = 0xE1A00000; // nop
		*(u32*)0x020440A0 = 0xE1A00000; // nop
		*(u32*)0x0204D7CC = 0xE1A00000; // nop
		*(u32*)0x0204E638 = 0xE1A00000; // nop
		*(u32*)0x0204E644 = 0xE1A00000; // nop
		*(u32*)0x0204E654 = 0xE1A00000; // nop
		*(u32*)0x0204E660 = 0xE1A00000; // nop
		*(u32*)0x0205C7A8 = 0xE1A00000; // nop
	}

	// Hair Salon: Pocket Stylist (Europe, Australia)
	// Audio doesn't play on retail consoles
	// Requires 8MB of RAM for full usage
	else if (strcmp(romTid, "KHRV") == 0) {
		/*if (!extendedMemory2) {
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				u32* addr = (u32*)0x02004000;
				if (addr[i] >= 0x02294000 && addr[i] < 0x022B0000) {
					addr[i] -= 0x120000;
				}
			}
		}*/

		*(u32*)0x0200498C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020050D0 = 0xE3A00901; // mov r0, #0x4000
			*(u32*)0x0202EE50 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202EE54 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202EEB4 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202EEB8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0205837C = 0xE1A00000; // nop
			*(u32*)0x02058708 = 0xE1A00000; // nop
		}
		*(u32*)0x02005104 = 0xE1A00000; // nop
		for (int i = 0; i < 7; i++) {
			u32* offset = (u32*)0x0200517C;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x02005198 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200CE6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200D064 = 0xE1A00000; // nop
		*(u32*)0x02013168 = 0xE1A00000; // nop
		*(u32*)0x020132AC = 0xE1A00000; // nop
		*(u32*)0x020132C0 = 0xE1A00000; // nop
		*(u32*)0x0201718C = 0xE1A00000; // nop
		*(u32*)0x0201D6A0 = 0xE1A00000; // nop
		*(u32*)0x0201F5FC = 0xE1A00000; // nop
		*(u32*)0x0201F600 = 0xE1A00000; // nop
		*(u32*)0x0201F60C = 0xE1A00000; // nop
		*(u32*)0x0201F750 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F7AC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0201F8E0 -= 0x30000;
		*(u32*)0x02020B34 = 0xE1A00000; // nop
		*(u32*)0x02020B3C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x020244A0 = 0xE1A00000; // nop
		*(u32*)0x02026080 = 0xE1A00000; // nop
		*(u32*)0x02044130 = 0xE1A00000; // nop
		*(u32*)0x02044150 = 0xE1A00000; // nop
		*(u32*)0x0204D7CC = 0xE1A00000; // nop
		*(u32*)0x020571C4 = 0xE1A00000; // nop
		*(u32*)0x020571D0 = 0xE1A00000; // nop
		*(u32*)0x020571E0 = 0xE1A00000; // nop
		*(u32*)0x020571EC = 0xE1A00000; // nop
		*(u32*)0x0205C898 = 0xE1A00000; // nop
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

	// Plants vs. Zombies (USA)
	else if (strcmp(romTid, "KZLE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		setBL(0x02099244, (u32)dsiSaveOpen);
		setBL(0x02099268, (u32)dsiSaveGetLength);
		setBL(0x0209927C, (u32)dsiSaveRead);
		setBL(0x020992AC, (u32)dsiSaveClose);
		setBL(0x02099324, (u32)dsiSaveOpen);
		setBL(0x02099350, (u32)dsiSaveSetLength);
		setBL(0x02099374, (u32)dsiSaveWrite);
		setBL(0x02099390, (u32)dsiSaveClose);
		*(u32*)0x020993C8 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x020993D4, (u32)dsiSaveCreate);
		*(u32*)0x020C2B84 = 0xE1A00000; // nop
		*(u32*)0x020C2B8C = 0xE1A00000; // nop
		*(u32*)0x020C2BE0 = 0xE1A00000; // nop
		*(u32*)0x020C2BF4 = 0xE1A00000; // nop
		*(u32*)0x020C2BFC = 0xE1A00000; // nop
		*(u32*)0x020F71F4 = 0xE1A00000; // nop
		*(u32*)0x020F71F8 = 0xE1A00000; // nop
		*(u32*)0x020F7204 = 0xE1A00000; // nop
		*(u32*)0x020F7280 = 0xE1A00000; // nop
		*(u32*)0x020F7294 = 0xE1A00000; // nop
		tonccpy((u32*)0x020F8B24, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020FBF88 = 0xE1A00000; // nop
		*(u32*)0x0210634C = 0xE1A00000; // nop
		*(u32*)0x02108710 = 0xE1A00000; // nop
		*(u32*)0x02108714 = 0xE1A00000; // nop
		*(u32*)0x02108720 = 0xE1A00000; // nop
		*(u32*)0x02108880 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x021088DC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x02108A10 = 0x0226BE80;
		*(u32*)0x02109BCC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0210DBA4 = 0xE1A00000; // nop
		*(u32*)0x0211547C = 0xE1A00000; // nop
	}

	// Plants vs. Zombies (Europe, Australia)
	else if (strcmp(romTid, "KZLV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		setBL(0x02099AB0, (u32)dsiSaveOpen);
		setBL(0x02099AD4, (u32)dsiSaveGetLength);
		setBL(0x02099AE8, (u32)dsiSaveRead);
		setBL(0x02099B18, (u32)dsiSaveClose);
		setBL(0x02099B90, (u32)dsiSaveOpen);
		setBL(0x02099BBC, (u32)dsiSaveSetLength);
		setBL(0x02099BE0, (u32)dsiSaveWrite);
		setBL(0x02099BFC, (u32)dsiSaveClose);
		*(u32*)0x02099C34 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x02099C40, (u32)dsiSaveCreate);
		*(u32*)0x020C3DE8 = 0xE1A00000; // nop
		*(u32*)0x020C3DF0 = 0xE1A00000; // nop
		*(u32*)0x020C3E44 = 0xE1A00000; // nop
		*(u32*)0x020C3E58 = 0xE1A00000; // nop
		*(u32*)0x020C3E60 = 0xE1A00000; // nop
		*(u32*)0x020F8E64 = 0xE1A00000; // nop
		*(u32*)0x020F8E68 = 0xE1A00000; // nop
		*(u32*)0x020F8E74 = 0xE1A00000; // nop
		*(u32*)0x020F8EF0 = 0xE1A00000; // nop
		*(u32*)0x020F8F04 = 0xE1A00000; // nop
		tonccpy((u32*)0x020FA794, dsiSaveGetResultCode, 0xC);
		*(u32*)0x021083A4 = 0xE1A00000; // nop
		*(u32*)0x0210A768 = 0xE1A00000; // nop
		*(u32*)0x0210A76C = 0xE1A00000; // nop
		*(u32*)0x0210A778 = 0xE1A00000; // nop
		*(u32*)0x0210A8D8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0210A934, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		*(u32*)0x0210AA68 = 0x0226F8E0;
		*(u32*)0x0210BC24 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0210FBFC = 0xE1A00000; // nop
		*(u32*)0x021174D4 = 0xE1A00000; // nop
	}

	// GO Series: Portable Shrine Wars (USA)
	// GO Series: Portable Shrine Wars (Europe)
	else if (strcmp(romTid, "KOQE") == 0 || strcmp(romTid, "KOQP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020073E0 = 0xE1A00000; // nop
		setBL(0x0200CEC4, (u32)dsiSaveCreate);
		setBL(0x0200CF00, (u32)dsiSaveOpen);
		setBL(0x0200CF38, (u32)dsiSaveSetLength);
		setBL(0x0200CF48, (u32)dsiSaveWrite);
		setBL(0x0200CF60, (u32)dsiSaveClose);
		setBL(0x0200CFE8, (u32)dsiSaveOpen);
		setBL(0x0200D020, (u32)dsiSaveSetLength);
		setBL(0x0200D030, (u32)dsiSaveWrite);
		setBL(0x0200D048, (u32)dsiSaveClose);
		setBL(0x0200D0C8, (u32)dsiSaveOpen);
		setBL(0x0200D100, (u32)dsiSaveRead);
		setBL(0x0200D114, (u32)dsiSaveClose);
		setBL(0x0200D164, (u32)dsiSaveDelete);
		setBL(0x0200D1D0, (u32)dsiSaveGetInfo);
		*(u32*)0x0200D214 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0200D218 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E004 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204E1B8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0204ED3C, dsiSaveGetResultCode, 0xC);
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
	// Art Style: KUBOS (Europe, Australia)
	// Art Style: nalaku (Japan)
	else if (strncmp(romTid, "KAK", 3) == 0) {
		setBL(0x020075A8, (u32)dsiSaveOpen);
		setBL(0x02007668, (u32)dsiSaveOpen);
		setBL(0x020076AC, (u32)dsiSaveRead);
		setBL(0x020076E0, (u32)dsiSaveClose);
		//*(u32*)0x02007768 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200776C = 0xE12FFF1E; // bx lr
		setBL(0x0200779C, (u32)dsiSaveCreate);
		setBL(0x020077AC, (u32)dsiSaveGetResultCode);
		setBL(0x020077CC, (u32)dsiSaveOpen);
		setBL(0x020077EC, (u32)dsiSaveSetLength);
		setBL(0x02007808, (u32)dsiSaveWrite);
		setBL(0x02007834, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E' || ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x0200B370 = 0xE1A00000; // nop
			*(u32*)0x0200B37C = 0xE1A00000; // nop
			*(u32*)0x0200B3AC = 0xE1A00000; // nop
			*(u32*)0x0200B3B0 = 0xE1A00000; // nop
			*(u32*)0x0200B3B8 = 0xE1A00000; // nop
		} else {
			*(u32*)0x0200A654 = 0xE1A00000; // nop
			*(u32*)0x0200A660 = 0xE1A00000; // nop
			*(u32*)0x0200A690 = 0xE1A00000; // nop
			*(u32*)0x0200A694 = 0xE1A00000; // nop
			*(u32*)0x0200A69C = 0xE1A00000; // nop
		}
		if (ndsHeader->gameCode[3] == 'E') {
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
		} else if (ndsHeader->gameCode[3] == 'V') {
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
		} else {
			*(u32*)0x0203A528 = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x0203A52C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x0203D93C = 0xE1A00000; // nop
			*(u32*)0x02044A94 = 0xE1A00000; // nop
			*(u32*)0x0204686C = 0xE1A00000; // nop
			*(u32*)0x02046870 = 0xE1A00000; // nop
			*(u32*)0x0204687C = 0xE1A00000; // nop
			*(u32*)0x020469C0 = 0xE1A00000; // nop
			*(u32*)0x020469C4 = 0xE1A00000; // nop
			*(u32*)0x020469C8 = 0xE1A00000; // nop
			*(u32*)0x020469CC = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02046A28, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x02047D8C = 0xE1A00000; // nop
			*(u32*)0x02047D94 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		}
	}

	// Pro-Putt Domo (USA)
	else if (strcmp(romTid, "KDPE") == 0) {
		const u32 dsiSaveCreateT = 0x020270FC;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x0202710C;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x0202711C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x0202712C;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x0202713C;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x0202714C;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x0202715C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x020106BC = 0x2001; // movs r0, #1
		*(u16*)0x020106BE = 0x4770; // bx lr
		doubleNopT(0x0201096A);
		*(u16*)0x020109AC = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x020109AE = 0x4770; // bx lr
		setBLThumb(0x02010A12, dsiSaveCreateT);
		setBLThumb(0x02010A28, dsiSaveOpenT);
		setBLThumb(0x02010A44, dsiSaveSetLengthT);
		setBLThumb(0x02010A58, dsiSaveWriteT);
		setBLThumb(0x02010A6A, dsiSaveCloseT);
		*(u16*)0x02010A90 = 0x4778; // bx pc
		tonccpy((u32*)0x02010A94, dsiSaveGetLength, 0xC);
		setBLThumb(0x02010AC0, dsiSaveOpenT);
		setBLThumb(0x02010AE6, dsiSaveCloseT);
		setBLThumb(0x02010AF8, dsiSaveReadT);
		setBLThumb(0x02010AFE, dsiSaveCloseT);
		setBLThumb(0x02010B12, dsiSaveDeleteT);
		*(u16*)0x020179F4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02026262);
		doubleNopT(0x020284B2);
		doubleNopT(0x0202B2D4);
		doubleNopT(0x0202C88E);
		doubleNopT(0x0202C892);
		doubleNopT(0x0202C89E);
		doubleNopT(0x0202C982);
		patchHiHeapDSiWareThumb(0x0202C9C0, 0x0202716C, heapEnd); // movs r0, #0x23C0000
		doubleNopT(0x0202DA1E);
		*(u16*)0x0202DA22 = 0x46C0; // nop
		*(u16*)0x0202DA24 = 0x46C0; // nop
		doubleNopT(0x0202DA26);
		doubleNopT(0x0202FADA);
	}

	// Puzzle League: Express (USA)
	// Some code seems to make save reading fail, preventing support
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
		/* setBL(0x02057A80, (u32)dsiSaveOpen);
		setBL(0x02057A98, (u32)dsiSaveRead);
		setBL(0x02057AB0, (u32)dsiSaveClose);
		setBL(0x02057B34, (u32)dsiSaveOpen);
		setBL(0x02057B4C, (u32)dsiSaveWrite);
		setBL(0x02057B64, (u32)dsiSaveClose);
		setBL(0x02057BB8, 0x02057C54);
		//*(u32*)0x02057BC4 = 0xE3A00000; // mov r0, #0 (dsiSaveOpenDir)
		//*(u32*)0x02057BFC = 0xE1A00000; // nop (dsiSaveCloseDir)
		//*(u32*)0x02057C08 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02057C38 = 0xE3A00001; // mov r0, #1 (dsiSaveCreateDirAuto)
		setBL(0x02057C84, (u32)dsiSaveOpen);
		setBL(0x02057C94, (u32)dsiSaveGetLength);
		setBL(0x02057CA8, (u32)dsiSaveSetLength);
		setBL(0x02057CC4, (u32)dsiSaveClose);
		setBL(0x02057CF8, (u32)dsiSaveCreate);
		setBL(0x02057D10, (u32)dsiSaveOpen);
		setBL(0x02057D24, (u32)dsiSaveSetLength);
		setBL(0x02057D40, (u32)dsiSaveClose); */
		*(u32*)0x02064DD0 = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020ACF54 = 0xE1A00000; // nop
		//tonccpy((u32*)0x020ADBF4, dsiSaveGetResultCode, 0xC);
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

	// Puzzle to Go: Baby Animals (Europe)
	else if (strcmp(romTid, "KBYP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x020052E4 = 0xE1A00000; // nop
		*(u32*)0x020139E0 = 0xE1A00000; // nop
		*(u32*)0x0201759C = 0xE1A00000; // nop
		*(u32*)0x0201CFF4 = 0xE1A00000; // nop
		*(u32*)0x0201ED88 = 0xE1A00000; // nop
		*(u32*)0x0201ED8C = 0xE1A00000; // nop
		*(u32*)0x0201ED98 = 0xE1A00000; // nop
		*(u32*)0x0201EEF8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201EF54, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02020304 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02023758 = 0xE1A00000; // nop
		*(u32*)0x02025444 = 0xE1A00000; // nop
		*(u32*)0x0202551C = 0xE1A00000; // nop

		// Skip Manual screen
		*(u32*)0x02032AFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02032B80 = 0xE1A00000; // nop
		*(u32*)0x02032B88 = 0xE1A00000; // nop
		*(u32*)0x02032B94 = 0xE1A00000; // nop

		*(u32*)0x02032C0C = 0xE1A00000; // nop
		*(u32*)0x02034D08 = 0xE1A00000; // nop
		setBL(0x02036570, (u32)dsiSaveCreate);
		setBL(0x02036580, (u32)dsiSaveOpen);
		setBL(0x02036590, (u32)dsiSaveGetResultCode);
		setBL(0x020365AC, (u32)dsiSaveSetLength);
		setBL(0x020365BC, (u32)dsiSaveWrite);
		setBL(0x020365C4, (u32)dsiSaveClose);
		setBL(0x020365FC, (u32)dsiSaveOpen);
		setBL(0x0203660C, (u32)dsiSaveGetResultCode);
		setBL(0x02036624, (u32)dsiSaveGetLength);
		setBL(0x02036634, (u32)dsiSaveRead);
		setBL(0x0203663C, (u32)dsiSaveClose);
		setBL(0x02036674, (u32)dsiSaveOpen);
		setBL(0x02036684, (u32)dsiSaveGetResultCode);
		setBL(0x0203669C, (u32)dsiSaveClose);
	}

	// Puzzle to Go: Diddl (Europe)
	// Puzzle to Go: Wildlife (Europe)
	else if (strcmp(romTid, "KPUP") == 0 || strcmp(romTid, "KPDP") == 0) {
		*(u32*)0x020138D4 = 0xE1A00000; // nop
		*(u32*)0x020175C8 = 0xE1A00000; // nop
		*(u32*)0x0201D2F8 = 0xE1A00000; // nop
		*(u32*)0x0201ED88 = 0xE1A00000; // nop
		*(u32*)0x0201ED88 = 0xE1A00000; // nop
		*(u32*)0x0201F0EC = 0xE1A00000; // nop
		*(u32*)0x0201F230 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F28C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02020694 = 0xE1A00000; // nop
		*(u32*)0x0202069C = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x02023B6C = 0xE1A00000; // nop
		setBL(0x0203EA60, (u32)dsiSaveCreate);
		setBL(0x0203EA7C, (u32)dsiSaveOpen);
		setBL(0x0203EA8C, (u32)dsiSaveGetResultCode);
		setBL(0x0203EAA8, (u32)dsiSaveSetLength);
		setBL(0x0203EAB8, (u32)dsiSaveWrite);
		setBL(0x0203EAC0, (u32)dsiSaveClose);
		setBL(0x0203EAF8, (u32)dsiSaveOpen);
		setBL(0x0203EB08, (u32)dsiSaveGetResultCode);
		setBL(0x0203EB20, (u32)dsiSaveGetLength);
		setBL(0x0203EB30, (u32)dsiSaveRead);
		setBL(0x0203EB38, (u32)dsiSaveClose);
		setBL(0x0203EB70, (u32)dsiSaveOpen);
		setBL(0x0203EB80, (u32)dsiSaveGetResultCode);
		setBL(0x0203EB98, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x0203ECC4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0203ED28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203ED4C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203ED80 = 0xE1A00000; // nop
		*(u32*)0x0203ED8C = 0xE1A00000; // nop
	}

	// Puzzle to Go: Planets and Universe (Europe)
	// Puzzle to Go: Sightseeing (Europe)
	else if (strcmp(romTid, "KBXP") == 0 || strcmp(romTid, "KB3P") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x020052FC = 0xE1A00000; // nop
		*(u32*)0x020139F8 = 0xE1A00000; // nop
		*(u32*)0x02017648 = 0xE1A00000; // nop
		*(u32*)0x0201D0D0 = 0xE1A00000; // nop
		*(u32*)0x0201EE6C = 0xE1A00000; // nop
		*(u32*)0x0201EE70 = 0xE1A00000; // nop
		*(u32*)0x0201EE7C = 0xE1A00000; // nop
		*(u32*)0x0201EFDC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F038, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x020203F8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202384C = 0xE1A00000; // nop
		*(u32*)0x02025538 = 0xE1A00000; // nop
		*(u32*)0x02025610 = 0xE1A00000; // nop

		// Skip Manual screen
		*(u32*)0x02032BF0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02032C74 = 0xE1A00000; // nop
		*(u32*)0x02032C7C = 0xE1A00000; // nop
		*(u32*)0x02032C88 = 0xE1A00000; // nop

		*(u32*)0x02032D00 = 0xE1A00000; // nop
		*(u32*)0x02034DFC = 0xE1A00000; // nop
		setBL(0x02036680, (u32)dsiSaveCreate);
		setBL(0x02036690, (u32)dsiSaveOpen);
		setBL(0x020366A0, (u32)dsiSaveGetResultCode);
		setBL(0x020366BC, (u32)dsiSaveSetLength);
		setBL(0x020366CC, (u32)dsiSaveWrite);
		setBL(0x020366D4, (u32)dsiSaveClose);
		setBL(0x0203670C, (u32)dsiSaveOpen);
		setBL(0x0203671C, (u32)dsiSaveGetResultCode);
		setBL(0x02036734, (u32)dsiSaveGetLength);
		setBL(0x02036744, (u32)dsiSaveRead);
		setBL(0x0203674C, (u32)dsiSaveClose);
		setBL(0x02036784, (u32)dsiSaveOpen);
		setBL(0x02036794, (u32)dsiSaveGetResultCode);
		setBL(0x020367AC, (u32)dsiSaveClose);
	}

	// Quick Fill Q (USA)
	// Quick Fill Q (Europe)
	// A bit hard/confusing to add save support
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
		//*(u32*)0x0203EA70 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02040240 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rabi Laby (USA)
	// Rabi Laby (Europe)
	else if (strcmp(romTid, "KLBE") == 0 || strcmp(romTid, "KLBP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		setBL(0x0201DEF8, (u32)dsiSaveOpen);
		setBL(0x0201DF1C, (u32)dsiSaveRead);
		setBL(0x0201DF2C, (u32)dsiSaveRead);
		setBL(0x0201DF34, (u32)dsiSaveClose);
		setBL(0x0201E1E4, (u32)dsiSaveOpen);
		setBL(0x0201E354, (u32)dsiSaveWrite);
		setBL(0x0201E35C, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0202663C = 0xE1A00000; // nop
			setBL(0x02026648, (u32)dsiSaveCreate);
			*(u32*)0x020266F8 = 0xE1A00000; // nop
			setBL(0x02026704, (u32)dsiSaveCreate);
		} else {
			*(u32*)0x020266CC = 0xE1A00000; // nop
			setBL(0x020266D8, (u32)dsiSaveCreate);
			*(u32*)0x02026788 = 0xE1A00000; // nop
			setBL(0x02026794, (u32)dsiSaveCreate);
		}
	}

	// Akushon Pazuru: Rabi x Rabi (Japan)
	else if (strcmp(romTid, "KLBJ") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005190 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		setBL(0x0201DD80, (u32)dsiSaveOpen);
		setBL(0x0201DDA4, (u32)dsiSaveRead);
		setBL(0x0201DDB4, (u32)dsiSaveRead);
		setBL(0x0201DDBC, (u32)dsiSaveClose);
		setBL(0x0201E1E4, (u32)dsiSaveOpen);
		setBL(0x0201E354, (u32)dsiSaveWrite);
		setBL(0x0201E35C, (u32)dsiSaveClose);
		setBL(0x0201E06C, (u32)dsiSaveOpen);
		setBL(0x0201E1DC, (u32)dsiSaveWrite);
		setBL(0x0201E1E4, (u32)dsiSaveClose);
		*(u32*)0x02026BD0 = 0xE1A00000; // nop
		setBL(0x02026BDC, (u32)dsiSaveCreate);
		*(u32*)0x02026CB4 = 0xE1A00000; // nop
		setBL(0x02026CC0, (u32)dsiSaveCreate);
	}

	// Rabi Laby 2 (USA)
	// Rabi Laby 2 (Europe)
	// Akushon Pazuru: Rabi x Rabi Episodo 2 (Japan)
	else if (strncmp(romTid, "KLV", 3) == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020051E8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		*(u32*)0x0203571C = 0xE1A00000; // nop
		*(u32*)0x02035720 = 0xE1A00000; // nop
		*(u32*)0x02035724 = 0xE1A00000; // nop
		// The non-save branches are patching out "crc.dat" R/Ws
		setBL(0x02035738, (u32)dsiSaveOpen);
		*(u32*)0x0203573C = 0xE1A00000; // nop
		*(u32*)0x02035740 = 0xE1A00000; // nop
		*(u32*)0x02035744 = 0xE1A00000; // nop
		*(u32*)0x02035748 = 0xE1A00000; // nop
		*(u32*)0x0203574C = 0xE1A00000; // nop
		*(u32*)0x02035750 = 0xE1A00000; // nop
		*(u32*)0x02035758 = 0x028DDFA5; // addeq sp, sp, #0x294
		*(u32*)0x0203575C = 0x08BD81F8; // ldmeqfd sp!, {r3-r8,pc}
		*(u32*)0x02035760 = 0xE1A00000; // nop
		*(u32*)0x02035764 = 0xE1A00000; // nop
		*(u32*)0x02035768 = 0xE1A00000; // nop
		*(u32*)0x0203576C = 0xE1A00000; // nop
		setBL(0x02035784, (u32)dsiSaveRead);
		*(u32*)0x02035788 = 0xE1A00000; // nop
		*(u32*)0x0203578C = 0xE1A00000; // nop
		*(u32*)0x02035790 = 0xE1A00000; // nop
		*(u32*)0x02035794 = 0xE1A00000; // nop
		setBL(0x0203579C, (u32)dsiSaveClose);
		*(u32*)0x020357A0 = 0xE1A00000; // nop
		*(u32*)0x020357A4 = 0xE1A00000; // nop
		*(u32*)0x02035810 = 0xE1A00000; // nop
		*(u32*)0x02035814 = 0xE1A00000; // nop
		*(u32*)0x02035818 = 0xE1A00000; // nop
		setBL(0x0203582C, (u32)dsiSaveOpen);
		*(u32*)0x020357DC = 0xE1A00000; // nop
		*(u32*)0x020357E0 = 0xE3A04000; // mov r4, #0
		*(u32*)0x02035840 = 0xE1A00000; // nop
		*(u32*)0x02035844 = 0xE1A00000; // nop
		*(u32*)0x02035848 = 0xE1A00000; // nop
		*(u32*)0x0203584C = 0xE1A00000; // nop
		*(u32*)0x02035850 = 0xE1A00000; // nop
		*(u32*)0x02035854 = 0xE1A00000; // nop
		*(u32*)0x02035858 = 0xE1A00000; // nop
		*(u32*)0x0203585C = 0xE1A00000; // nop
		setBL(0x020358A8, (u32)dsiSaveWrite);
		*(u32*)0x020358AC = 0xE1A00000; // nop
		*(u32*)0x020358B0 = 0xE1A00000; // nop
		*(u32*)0x020358B4 = 0xE1A00000; // nop
		*(u32*)0x020358B8 = 0xE1A00000; // nop
		setBL(0x020358C0, (u32)dsiSaveClose);
		*(u32*)0x020358C4 = 0xE1A00000; // nop
		*(u32*)0x020358C8 = 0xE1A00000; // nop
		setBL(0x020358F8, (u32)dsiSaveCreate);
		*(u32*)0x020358FC = 0xE1A00000; // nop
		*(u32*)0x02035900 = 0xE1A00000; // nop
		*(u32*)0x02035904 = 0xE1A00000; // nop
		*(u32*)0x02035BF8 = 0xE1A00000; // nop
		*(u32*)0x02035CA8 = 0xE1A00000; // nop
	}

	// Real Crimes: Jack the Ripper (USA)
	else if (strcmp(romTid, "KRCE") == 0) {
		const u32 dsiSaveCreateT = 0x020119A0;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x020119B0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020119C0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x020119D0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x020119E0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020119F0;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020103D8 = 0xE1A00000; // nop
		*(u32*)0x02013974 = 0xE1A00000; // nop
		*(u32*)0x02017B00 = 0xE1A00000; // nop
		*(u32*)0x02019974 = 0xE1A00000; // nop
		*(u32*)0x02019978 = 0xE1A00000; // nop
		*(u32*)0x02019984 = 0xE1A00000; // nop
		*(u32*)0x02019AE4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019B40, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201AFB8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201E390 = 0xE1A00000; // nop
		setBLThumb(0x02026358, dsiSaveOpenT);
		setBLThumb(0x02026370, dsiSaveSeekT);
		setBLThumb(0x02026382, dsiSaveReadT);
		setBLThumb(0x02026398, dsiSaveReadT);
		setBLThumb(0x020263C0, dsiSaveCloseT);
		doubleNopT(0x020263DA);
		setBLThumb(0x0202642E, dsiSaveOpenT);
		setBLThumb(0x0202644E, dsiSaveSeekT);
		setBLThumb(0x02026460, dsiSaveReadT);
		setBLThumb(0x02026476, dsiSaveReadT);
		setBLThumb(0x0202649E, dsiSaveCloseT);
		doubleNopT(0x020264B6);
		setBLThumb(0x020264F8, dsiSaveOpenT);
		setBLThumb(0x0202650C, dsiSaveCloseT);
		doubleNopT(0x0202651E);
		setBLThumb(0x02026558, dsiSaveOpenT);
		setBLThumb(0x0202656A, dsiSaveCloseT);
		doubleNopT(0x0202657C);
		*(u16*)0x020265B6 = 0x2001; // movs r0, #1 (dsiSaveOpenDir)
		*(u16*)0x020265B8 = 0x46C0; // nop
		doubleNopT(0x020265C0); // dsiSaveCloseDir
		setBLThumb(0x020265CC, dsiSaveOpenT);
		setBLThumb(0x020265D6, dsiSaveCloseT);
		setBLThumb(0x02026656, dsiSaveCreateT);
		setBLThumb(0x02026666, dsiSaveOpenT);
		setBLThumb(0x02026674, dsiSaveWriteT);
		setBLThumb(0x020266DA, dsiSaveCloseT);
		doubleNopT(0x020266EE);
		setBLThumb(0x02026754, dsiSaveSeekT);
		setBLThumb(0x0202675E, dsiSaveWriteT);
		setBLThumb(0x02026774, dsiSaveWriteT);
		setBLThumb(0x020267AA, dsiSaveSeekT);
		setBLThumb(0x020267B4, dsiSaveWriteT);
		setBLThumb(0x020267CA, dsiSaveWriteT);
		*(u16*)0x020471B4 = 0x2100; // movs r1, #0 (Skip Manual screen)
	}

	// Real Crimes: Jack the Ripper (Europe, Australia)
	else if (strcmp(romTid, "KRCV") == 0) {
		const u32 dsiSaveCreateT = 0x020119A4;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x020119B4;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020119C4;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x020119D4;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x020119E4;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020119F4;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020103DC = 0xE1A00000; // nop
		*(u32*)0x02013978 = 0xE1A00000; // nop
		*(u32*)0x02017B04 = 0xE1A00000; // nop
		*(u32*)0x02019978 = 0xE1A00000; // nop
		*(u32*)0x0201997C = 0xE1A00000; // nop
		*(u32*)0x02019988 = 0xE1A00000; // nop
		*(u32*)0x02019AE8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019B44, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201AFBC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201E394 = 0xE1A00000; // nop
		setBLThumb(0x02026338, dsiSaveOpenT);
		setBLThumb(0x0202634E, dsiSaveSeekT);
		setBLThumb(0x02026360, dsiSaveReadT);
		setBLThumb(0x02026376, dsiSaveReadT);
		setBLThumb(0x0202639E, dsiSaveCloseT);
		doubleNopT(0x020263B8);
		setBLThumb(0x0202640E, dsiSaveOpenT);
		setBLThumb(0x0202642C, dsiSaveSeekT);
		setBLThumb(0x0202643E, dsiSaveReadT);
		setBLThumb(0x02026454, dsiSaveReadT);
		setBLThumb(0x0202647C, dsiSaveCloseT);
		doubleNopT(0x02026494);
		setBLThumb(0x020264D8, dsiSaveOpenT);
		setBLThumb(0x020264EC, dsiSaveCloseT);
		doubleNopT(0x020264FE);
		setBLThumb(0x02026538, dsiSaveOpenT);
		setBLThumb(0x0202654A, dsiSaveCloseT);
		doubleNopT(0x0202655C);
		*(u16*)0x02026596 = 0x2001; // movs r0, #1 (dsiSaveOpenDir)
		*(u16*)0x02026598 = 0x46C0; // nop
		doubleNopT(0x020265A0); // dsiSaveCloseDir
		setBLThumb(0x020265AC, dsiSaveOpenT);
		setBLThumb(0x020265B6, dsiSaveCloseT);
		setBLThumb(0x02026636, dsiSaveCreateT);
		setBLThumb(0x02026646, dsiSaveOpenT);
		setBLThumb(0x020266B0, dsiSaveCloseT);
		doubleNopT(0x020266C4);
		setBLThumb(0x02026726, dsiSaveSeekT);
		setBLThumb(0x02026730, dsiSaveWriteT);
		setBLThumb(0x02026746, dsiSaveWriteT);
		setBLThumb(0x0202677C, dsiSaveSeekT);
		setBLThumb(0x02026786, dsiSaveWriteT);
		setBLThumb(0x0202679C, dsiSaveWriteT);
		*(u16*)0x020471E4 = 0x2100; // movs r1, #0 (Skip Manual screen)
	}

	// Redau Shirizu: Gunjin Shougi (Japan)
	// A sprite layer is missing (containing text font and others)
	else if (strcmp(romTid, "KLXJ") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005254 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200E0F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201025C = 0xE1A00000; // nop
		*(u32*)0x020134D4 = 0xE1A00000; // nop
		*(u32*)0x02014664 = 0xE1A00000; // nop
		*(u32*)0x02014700 = 0xE1A00000; // nop
		*(u32*)0x020147B4 = 0xE1A00000; // nop
		*(u32*)0x02014868 = 0xE1A00000; // nop
		*(u32*)0x02014908 = 0xE1A00000; // nop
		*(u32*)0x02014988 = 0xE1A00000; // nop
		*(u32*)0x02014A04 = 0xE1A00000; // nop
		*(u32*)0x02014A88 = 0xE1A00000; // nop
		*(u32*)0x02014B28 = 0xE1A00000; // nop
		*(u32*)0x02014BE4 = 0xE1A00000; // nop
		*(u32*)0x02014CA0 = 0xE1A00000; // nop
		*(u32*)0x02014D5C = 0xE1A00000; // nop
		*(u32*)0x02014E18 = 0xE1A00000; // nop
		*(u32*)0x02014ED4 = 0xE1A00000; // nop
		*(u32*)0x02014F90 = 0xE1A00000; // nop
		*(u32*)0x0201504C = 0xE1A00000; // nop
		*(u32*)0x020150F8 = 0xE1A00000; // nop
		*(u32*)0x0201518C = 0xE1A00000; // nop
		*(u32*)0x020152B0 = 0xE1A00000; // nop
		*(u32*)0x02015314 = 0xE1A00000; // nop
		*(u32*)0x020153DC = 0xE1A00000; // nop
		*(u32*)0x0201544C = 0xE1A00000; // nop
		*(u32*)0x020154D8 = 0xE1A00000; // nop
		*(u32*)0x02015548 = 0xE1A00000; // nop
		*(u32*)0x020155D0 = 0xE1A00000; // nop
		*(u32*)0x02015640 = 0xE1A00000; // nop
		*(u32*)0x02015754 = 0xE1A00000; // nop
		*(u32*)0x020157BC = 0xE1A00000; // nop
		*(u32*)0x0201583C = 0xE1A00000; // nop
		*(u32*)0x020158A0 = 0xE1A00000; // nop
		*(u32*)0x02015958 = 0xE1A00000; // nop
		*(u32*)0x020159C8 = 0xE1A00000; // nop
		*(u32*)0x02017414 = 0xE1A00000; // nop
		*(u32*)0x02019278 = 0xE1A00000; // nop
		*(u32*)0x0201927C = 0xE1A00000; // nop
		*(u32*)0x02019288 = 0xE1A00000; // nop
		*(u32*)0x020193E8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02019444, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201A89C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201DD18 = 0xE1A00000; // nop
		*(u32*)0x0202001C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020020 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D134 = 0xE1A00000; // nop
		*(u32*)0x0202E338 = 0xE1A00000; // nop
		*(u32*)0x0202E528 = 0xE1A00000; // nop
		*(u32*)0x0202E560 = 0xE1A00000; // nop
		setBL(0x0203342C, (u32)dsiSaveCreate);
		setBL(0x0203343C, (u32)dsiSaveOpen);
		setBL(0x02033468, (u32)dsiSaveWrite);
		setBL(0x02033478, (u32)dsiSaveClose);
		setBL(0x02033494, (u32)dsiSaveClose);
		setBL(0x02033500, (u32)dsiSaveOpen);
		setBL(0x02033510, (u32)dsiSaveGetLength);
		setBL(0x02033528, (u32)dsiSaveRead);
		setBL(0x0203356C, (u32)dsiSaveClose);
		setBL(0x02033588, (u32)dsiSaveClose);
	}

	// Remote Racers (USA)
	// Remote Racers (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KQRE") == 0 || strcmp(romTid, "KQRV") == 0) && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020197F0 = 0xE1A00000; // nop
		*(u32*)0x0201CDA0 = 0xE1A00000; // nop
		*(u32*)0x02021E98 = 0xE1A00000; // nop
		*(u32*)0x02023C50 = 0xE1A00000; // nop
		*(u32*)0x02023C54 = 0xE1A00000; // nop
		*(u32*)0x02023C60 = 0xE1A00000; // nop
		*(u32*)0x02023DC0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02023E1C, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x020250BC = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02028758 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0208DCBC = 0xE1A00000; // nop
			*(u32*)0x0208DCC4 = 0xE1A00000; // nop
			*(u32*)0x0208DDB8 = 0xE1A00000; // nop
			*(u32*)0x0208E3A0 = 0xE1A00000; // nop
			*(u32*)0x0208E56C = 0xE1A00000; // nop
			setBL(0x0209A074, (u32)dsiSaveClose);
			setBL(0x0209A0D0, (u32)dsiSaveClose);
			setBL(0x0209A178, (u32)dsiSaveOpen);
			setBL(0x0209A190, (u32)dsiSaveSeek);
			setBL(0x0209A1A4, (u32)dsiSaveRead);
			setBL(0x0209A244, (u32)dsiSaveCreate);
			setBL(0x0209A274, (u32)dsiSaveOpen);
			setBL(0x0209A2A4, (u32)dsiSaveSetLength);
			setBL(0x0209A2CC, (u32)dsiSaveSeek);
			setBL(0x0209A2E0, (u32)dsiSaveWrite);
			setBL(0x0209A390, (u32)dsiSaveCreate);
			setBL(0x0209A3C8, (u32)dsiSaveOpen);
			setBL(0x0209A400, (u32)dsiSaveSetLength);
			setBL(0x0209A41C, (u32)dsiSaveSeek);
			setBL(0x0209A430, (u32)dsiSaveWrite);
			setBL(0x0209A590, (u32)dsiSaveSeek);
			setBL(0x0209A5A0, (u32)dsiSaveWrite);
			setBL(0x0209A718, (u32)dsiSaveGetResultCode);
			*(u32*)0x0209A75C = 0xE3A00000; // mov r0, #0
		} else {
			*(u32*)0x0208DC98 = 0xE1A00000; // nop
			*(u32*)0x0208DCA0 = 0xE1A00000; // nop
			*(u32*)0x0208DD94 = 0xE1A00000; // nop
			*(u32*)0x0208E37C = 0xE1A00000; // nop
			*(u32*)0x0208E548 = 0xE1A00000; // nop
			setBL(0x0209A338, (u32)dsiSaveClose);
			setBL(0x0209A394, (u32)dsiSaveClose);
			setBL(0x0209A43C, (u32)dsiSaveOpen);
			setBL(0x0209A454, (u32)dsiSaveSeek);
			setBL(0x0209A468, (u32)dsiSaveRead);
			setBL(0x0209A508, (u32)dsiSaveCreate);
			setBL(0x0209A538, (u32)dsiSaveOpen);
			setBL(0x0209A568, (u32)dsiSaveSetLength);
			setBL(0x0209A590, (u32)dsiSaveSeek);
			setBL(0x0209A5A4, (u32)dsiSaveWrite);
			setBL(0x0209A654, (u32)dsiSaveCreate);
			setBL(0x0209A68C, (u32)dsiSaveOpen);
			setBL(0x0209A6C4, (u32)dsiSaveSetLength);
			setBL(0x0209A6E0, (u32)dsiSaveSeek);
			setBL(0x0209A6F4, (u32)dsiSaveWrite);
			setBL(0x0209A854, (u32)dsiSaveSeek);
			setBL(0x0209A864, (u32)dsiSaveWrite);
			setBL(0x0209A9DC, (u32)dsiSaveGetResultCode);
			*(u32*)0x0209AA20 = 0xE3A00000; // mov r0, #0
		}
	}

	// Renjuku Kanji: Shougaku 1 Nensei (Japan)
	// Renjuku Kanji: Shougaku 2 Nensei (Japan)
	// Renjuku Kanji: Shougaku 3 Nensei (Japan)
	// Renjuku Kanji: Shougaku 4 Nensei (Japan)
	// Renjuku Kanji: Shougaku 5 Nensei (Japan)
	// Renjuku Kanji: Shougaku 6 Nensei (Japan)
	// Renjuku Kanji: Chuugakusei (Japan)
	else if (strcmp(romTid, "KJZJ") == 0 || strcmp(romTid, "KJ2J") == 0 || strcmp(romTid, "KJ3J") == 0 || strcmp(romTid, "KJ4J") == 0
		   || strcmp(romTid, "KJ5J") == 0 || strcmp(romTid, "KJ6J") == 0 || strcmp(romTid, "KJ8J") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200E618 = 0xE1A00000; // nop
		*(u32*)0x02011B38 = 0xE1A00000; // nop
		*(u32*)0x02015A4C = 0xE1A00000; // nop
		*(u32*)0x02017840 = 0xE1A00000; // nop
		*(u32*)0x02017844 = 0xE1A00000; // nop
		*(u32*)0x02017850 = 0xE1A00000; // nop
		*(u32*)0x020179B0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02017A0C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02018F14 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201C548 = 0xE1A00000; // nop
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		if (strncmp(romTid, "KJZ", 3) == 0) {
			*(u32*)0x02048D68 = 0xE1A00000; // nop
			*(u32*)0x02048D7C = 0xE1A00000; // nop
			*(u32*)0x02049A64 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049C50 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020532B8 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
			setBL(0x02064ED0, (u32)dsiSaveOpen);
			setBL(0x02064EEC, (u32)dsiSaveSeek);
			setBL(0x02064F00, (u32)dsiSaveClose);
			setBL(0x02064F18, (u32)dsiSaveRead);
			setBL(0x02064F28, (u32)dsiSaveClose);
			setBL(0x02064F34, (u32)dsiSaveClose);
			setBL(0x02064F68, (u32)dsiSaveOpen);
			setBL(0x02064F80, (u32)dsiSaveSeek);
			setBL(0x02064F98, (u32)dsiSaveRead); // dsiSaveReadAsync
			setBL(0x02064FC8, (u32)dsiSaveOpen);
			setBL(0x02064FE0, (u32)dsiSaveSetLength);
			setBL(0x02064FF0, (u32)dsiSaveClose);
			setBL(0x02065004, (u32)dsiSaveSeek);
			setBL(0x02065018, (u32)dsiSaveClose);
			setBL(0x02065030, (u32)dsiSaveWrite);
			setBL(0x02065040, (u32)dsiSaveClose);
			setBL(0x0206504C, (u32)dsiSaveClose);
			setBL(0x02065080, (u32)dsiSaveOpen);
			setBL(0x02065094, (u32)dsiSaveSetLength);
			setBL(0x020650AC, (u32)dsiSaveSeek);
			setBL(0x020650C4, (u32)dsiSaveWrite); // dsiSaveWriteAsync
			*(u32*)0x02065150 = 0xE12FFF1E; // bx lr
		} else if (strncmp(romTid, "KJ2", 3) == 0 || strncmp(romTid, "KJ3", 3) == 0) {
			*(u32*)0x02048D50 = 0xE1A00000; // nop
			*(u32*)0x02048D64 = 0xE1A00000; // nop
			*(u32*)0x02049A4C = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049C38 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020532A0 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
			setBL(0x02064EB8, (u32)dsiSaveOpen);
			setBL(0x02064ED4, (u32)dsiSaveSeek);
			setBL(0x02064EE8, (u32)dsiSaveClose);
			setBL(0x02064F00, (u32)dsiSaveRead);
			setBL(0x02064F10, (u32)dsiSaveClose);
			setBL(0x02064F1C, (u32)dsiSaveClose);
			setBL(0x02064F50, (u32)dsiSaveOpen);
			setBL(0x02064F68, (u32)dsiSaveSeek);
			setBL(0x02064F80, (u32)dsiSaveRead); // dsiSaveReadAsync
			setBL(0x02064FB0, (u32)dsiSaveOpen);
			setBL(0x02064FC8, (u32)dsiSaveSetLength);
			setBL(0x02064FD8, (u32)dsiSaveClose);
			setBL(0x02064FEC, (u32)dsiSaveSeek);
			setBL(0x02065000, (u32)dsiSaveClose);
			setBL(0x02065018, (u32)dsiSaveWrite);
			setBL(0x02065028, (u32)dsiSaveClose);
			setBL(0x02065034, (u32)dsiSaveClose);
			setBL(0x02065068, (u32)dsiSaveOpen);
			setBL(0x0206507C, (u32)dsiSaveSetLength);
			setBL(0x02065094, (u32)dsiSaveSeek);
			setBL(0x020650AC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
			*(u32*)0x02065138 = 0xE12FFF1E; // bx lr
		} else if (strncmp(romTid, "KJ8", 3) != 0) {
			*(u32*)0x02048D50 = 0xE1A00000; // nop
			*(u32*)0x02048D64 = 0xE1A00000; // nop
			*(u32*)0x02049AE8 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049CD4 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205333C = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
			setBL(0x02064F54, (u32)dsiSaveOpen);
			setBL(0x02064F70, (u32)dsiSaveSeek);
			setBL(0x02064F84, (u32)dsiSaveClose);
			setBL(0x02064F9C, (u32)dsiSaveRead);
			setBL(0x02064FAC, (u32)dsiSaveClose);
			setBL(0x02064FB8, (u32)dsiSaveClose);
			setBL(0x02064FEC, (u32)dsiSaveOpen);
			setBL(0x02065004, (u32)dsiSaveSeek);
			setBL(0x0206501C, (u32)dsiSaveRead); // dsiSaveReadAsync
			setBL(0x0206504C, (u32)dsiSaveOpen);
			setBL(0x02065064, (u32)dsiSaveSetLength);
			setBL(0x02065074, (u32)dsiSaveClose);
			setBL(0x02065088, (u32)dsiSaveSeek);
			setBL(0x0206509C, (u32)dsiSaveClose);
			setBL(0x020650B4, (u32)dsiSaveWrite);
			setBL(0x020650C4, (u32)dsiSaveClose);
			setBL(0x020650D0, (u32)dsiSaveClose);
			setBL(0x02065104, (u32)dsiSaveOpen);
			setBL(0x02065118, (u32)dsiSaveSetLength);
			setBL(0x02065130, (u32)dsiSaveSeek);
			setBL(0x02065148, (u32)dsiSaveWrite); // dsiSaveWriteAsync
			*(u32*)0x020651D4 = 0xE12FFF1E; // bx lr
		} else {
			*(u32*)0x02048D54 = 0xE1A00000; // nop
			*(u32*)0x02048D68 = 0xE1A00000; // nop
			*(u32*)0x02049A68 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049C54 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020532BC = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
			setBL(0x02064ED4, (u32)dsiSaveOpen);
			setBL(0x02064EF0, (u32)dsiSaveSeek);
			setBL(0x02064F04, (u32)dsiSaveClose);
			setBL(0x02064F1C, (u32)dsiSaveRead);
			setBL(0x02064F2C, (u32)dsiSaveClose);
			setBL(0x02064F38, (u32)dsiSaveClose);
			setBL(0x02064F6C, (u32)dsiSaveOpen);
			setBL(0x02064F84, (u32)dsiSaveSeek);
			setBL(0x02064F9C, (u32)dsiSaveRead); // dsiSaveReadAsync
			setBL(0x02064FCC, (u32)dsiSaveOpen);
			setBL(0x02064FE4, (u32)dsiSaveSetLength);
			setBL(0x02064FF4, (u32)dsiSaveClose);
			setBL(0x02065008, (u32)dsiSaveSeek);
			setBL(0x0206501C, (u32)dsiSaveClose);
			setBL(0x02065034, (u32)dsiSaveWrite);
			setBL(0x02065044, (u32)dsiSaveClose);
			setBL(0x02065050, (u32)dsiSaveClose);
			setBL(0x02065084, (u32)dsiSaveOpen);
			setBL(0x02065098, (u32)dsiSaveSetLength);
			setBL(0x020650B0, (u32)dsiSaveSeek);
			setBL(0x020650C8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
			*(u32*)0x02065154 = 0xE12FFF1E; // bx lr
		}
	}

	// Robot Rescue (USA)
	else if (strcmp(romTid, "KRTE") == 0) {
		/* *(u32*)0x0200C2DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2E0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C39C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C570 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C574 = 0xE12FFF1E; // bx lr */
		setBL(0x0200C594, (u32)dsiSaveOpen);
		setBL(0x0200C5A8, (u32)dsiSaveClose);
		setBL(0x0200C5C8, (u32)dsiSaveCreate);
		setBL(0x0200C5E0, (u32)dsiSaveOpen);
		setBL(0x0200C5F8, (u32)dsiSaveClose);
		setBL(0x0200C600, (u32)dsiSaveDelete);
		setBL(0x0200C76C, (u32)dsiSaveOpen);
		setBL(0x0200C784, (u32)dsiSaveGetLength);
		setBL(0x0200C7A8, (u32)dsiSaveRead);
		setBL(0x0200C7B0, (u32)dsiSaveClose);
		*(u32*)0x0200C814 = 0xE1A00000; // nop
		*(u32*)0x0200C81C = 0xE1A00000; // nop
		setBL(0x0200C838, (u32)dsiSaveOpen);
		setBL(0x0200C84C, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveCreate);
		setBL(0x0200C878, (u32)dsiSaveOpen);
		*(u32*)0x0200C888 = 0xE1A00000; // nop
		setBL(0x0200C894, (u32)dsiSaveClose);
		setBL(0x0200C89C, (u32)dsiSaveDelete);
		setBL(0x0200C8B0, (u32)dsiSaveCreate);
		setBL(0x0200C8C0, (u32)dsiSaveOpen);
		*(u32*)0x0200C8D8 = 0xE1A00000; // nop
		*(u32*)0x0200C8EC = 0xE1A00000; // nop
		setBL(0x0200C904, (u32)dsiSaveSetLength);
		setBL(0x0200C914, (u32)dsiSaveWrite);
		setBL(0x0200C91C, (u32)dsiSaveClose);
		*(u32*)0x0200C924 = 0xE1A00000; // nop
		*(u32*)0x0200C938 = 0xE1A00000; // nop
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
		/* *(u32*)0x0200C2CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C388 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C38C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C550 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C554 = 0xE12FFF1E; // bx lr */
		setBL(0x0200C57C, (u32)dsiSaveOpen);
		setBL(0x0200C590, (u32)dsiSaveClose);
		setBL(0x0200C5B0, (u32)dsiSaveCreate);
		setBL(0x0200C5CC, (u32)dsiSaveOpen);
		setBL(0x0200C5E4, (u32)dsiSaveClose);
		setBL(0x0200C5EC, (u32)dsiSaveDelete);
		setBL(0x0200C788, (u32)dsiSaveOpen);
		setBL(0x0200C7A0, (u32)dsiSaveGetLength);
		setBL(0x0200C7C4, (u32)dsiSaveRead);
		setBL(0x0200C7CC, (u32)dsiSaveClose);
		setBL(0x0200C810, (u32)dsiSaveOpen);
		setBL(0x0200C824, (u32)dsiSaveClose);
		setBL(0x0200C838, (u32)dsiSaveCreate);
		setBL(0x0200C854, (u32)dsiSaveOpen);
		*(u32*)0x0200C864 = 0xE1A00000; // nop
		setBL(0x0200C870, (u32)dsiSaveClose);
		setBL(0x0200C878, (u32)dsiSaveDelete);
		setBL(0x0200C890, (u32)dsiSaveCreate);
		setBL(0x0200C8A0, (u32)dsiSaveOpen);
		setBL(0x0200C8CC, (u32)dsiSaveSetLength);
		setBL(0x0200C8DC, (u32)dsiSaveWrite);
		setBL(0x0200C8E4, (u32)dsiSaveClose);
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
		/* *(u32*)0x0200F460 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F464 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F51C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F520 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F6E4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F6E8 = 0xE12FFF1E; // bx lr */
		setBL(0x0200F710, (u32)dsiSaveOpen);
		setBL(0x0200F724, (u32)dsiSaveClose);
		setBL(0x0200F724, (u32)dsiSaveCreate);
		setBL(0x0200F760, (u32)dsiSaveOpen);
		setBL(0x0200F778, (u32)dsiSaveClose);
		setBL(0x0200F780, (u32)dsiSaveDelete);
		setBL(0x0200F8B0, (u32)dsiSaveOpen);
		setBL(0x0200F8C8, (u32)dsiSaveGetLength);
		setBL(0x0200F8EC, (u32)dsiSaveRead);
		setBL(0x0200F8F4, (u32)dsiSaveClose);
		setBL(0x0200F938, (u32)dsiSaveOpen);
		setBL(0x0200F94C, (u32)dsiSaveClose);
		setBL(0x0200F960, (u32)dsiSaveCreate);
		setBL(0x0200F97C, (u32)dsiSaveOpen);
		*(u32*)0x0200F98C = 0xE1A00000; // nop
		setBL(0x0200F998, (u32)dsiSaveClose);
		setBL(0x0200F9A0, (u32)dsiSaveDelete);
		setBL(0x0200F9B8, (u32)dsiSaveCreate);
		setBL(0x0200F9C8, (u32)dsiSaveOpen);
		setBL(0x0200F9F4, (u32)dsiSaveSetLength);
		setBL(0x0200FA04, (u32)dsiSaveWrite);
		setBL(0x0200FA0C, (u32)dsiSaveClose);
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
	// Robot Rescue 2 (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KRRE") == 0 || strcmp(romTid, "KRRP") == 0) && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200C0AC = 0xE1A00000; // nop
		*(u32*)0x0200C0B4 = 0xE1A00000; // nop
		/* *(u32*)0x0200C2F4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C3B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C578 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C57C = 0xE12FFF1E; // bx lr */
		setBL(0x0200C5A4, (u32)dsiSaveOpen);
		setBL(0x0200C5B8, (u32)dsiSaveClose);
		setBL(0x0200C5D8, (u32)dsiSaveCreate);
		setBL(0x0200C5F4, (u32)dsiSaveOpen);
		setBL(0x0200C60C, (u32)dsiSaveClose);
		setBL(0x0200C614, (u32)dsiSaveDelete);
		setBL(0x0200C7B0, (u32)dsiSaveOpen);
		setBL(0x0200C7C8, (u32)dsiSaveGetLength);
		setBL(0x0200C7EC, (u32)dsiSaveRead);
		setBL(0x0200C7F4, (u32)dsiSaveClose);
		setBL(0x0200C838, (u32)dsiSaveOpen);
		setBL(0x0200C84C, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveCreate);
		setBL(0x0200C87C, (u32)dsiSaveOpen);
		*(u32*)0x0200C88C = 0xE1A00000; // nop
		setBL(0x0200C898, (u32)dsiSaveClose);
		setBL(0x0200C8A0, (u32)dsiSaveDelete);
		setBL(0x0200C8B8, (u32)dsiSaveCreate);
		setBL(0x0200C8C8, (u32)dsiSaveOpen);
		setBL(0x0200C8F4, (u32)dsiSaveSetLength);
		setBL(0x0200C904, (u32)dsiSaveWrite);
		setBL(0x0200C90C, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
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
		} else if (ndsHeader->gameCode[3] == 'P') {
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
	}

	// Rock-n-Roll Domo (USA)
	else if (strcmp(romTid, "KD6E") == 0) {
		const u32 dsiSaveCreateT = 0x02025C20;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02025C30;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02025C40;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02025C50;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02025C60;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02025C70;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02025C80;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x02010164 = 0x2001; // movs r0, #1
		*(u16*)0x02010166 = 0x4770; // bx lr
		doubleNopT(0x0201041A);
		*(u16*)0x0201045C = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0201045E = 0x4770; // bx lr
		setBLThumb(0x020104C2, dsiSaveCreateT);
		setBLThumb(0x020104D8, dsiSaveOpenT);
		setBLThumb(0x020104F4, dsiSaveSetLengthT);
		setBLThumb(0x02010508, dsiSaveWriteT);
		setBLThumb(0x0201051A, dsiSaveCloseT);
		*(u16*)0x02010540 = 0x4778; // bx pc
		tonccpy((u32*)0x02010544, dsiSaveGetLength, 0xC);
		setBLThumb(0x02010570, dsiSaveOpenT);
		setBLThumb(0x02010596, dsiSaveCloseT);
		setBLThumb(0x020105A8, dsiSaveReadT);
		setBLThumb(0x020105AE, dsiSaveCloseT);
		setBLThumb(0x020105C2, dsiSaveDeleteT);
		*(u16*)0x02016514 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02024D86);
		doubleNopT(0x02026FD6);
		doubleNopT(0x02029C58);
		doubleNopT(0x0202B212);
		doubleNopT(0x0202B216);
		doubleNopT(0x0202B222);
		doubleNopT(0x0202B306);
		patchHiHeapDSiWareThumb(0x0202B344, 0x02025C90, heapEnd); // movs r0, #0x23C0000
		doubleNopT(0x0202C3A2);
		*(u16*)0x0202C3A6 = 0x46C0;
		*(u16*)0x0202C3A8 = 0x46C0;
		doubleNopT(0x0202C3AA);
		doubleNopT(0x0202E45E);
	}

	// Shantae: Risky's Revenge (USA)
	// Requires 8MB of RAM, crashes after first battle with 4MB of RAM, but can get past with a save file
	// BGM is disabled to stay within RAM limitations
	else if (strcmp(romTid, "KS3E") == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleForce[0] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02026F68;

		*(u32*)0x0200498C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Disable pre-load function
			/* *(u32*)0x0201FBA0 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FD3C = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FDA8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FE14 = 0xE12FFF1E; // bx lr */
			//*(u32*)0x020AB800 = 0xE1A00000; // nop
			*(u32*)0x020BCE44 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x0201FC20 = 0xE12FFF1E; // bx lr (Disable loading sdat file)
		tonccpy((u32*)0x0201FC40, ce9->patches->musicPlay, 0xC);
		tonccpy((u32*)0x0201FC78, ce9->patches->musicStopEffect, 0xC);
		setBL(0x02026F68, (int)ce9->patches->rumble_arm9[0]); // Rumble when hair is whipped
		setBL(0x0209201C, (u32)dsiSaveCreate);
		setBL(0x02092040, (u32)dsiSaveGetResultCode);
		*(u32*)0x02092050 = 0xE1A00000; // nop
		setBL(0x0209205C, (u32)dsiSaveCreate);
		*(u32*)0x02092078 = 0xE3A00000; // mov r0, #0
		setBL(0x0209291C, (u32)dsiSaveOpen);
		*(u32*)0x02092934 = 0xE1A00000; // nop
		setBL(0x02092944, (u32)dsiSaveOpen);
		setBL(0x02092958, (u32)dsiSaveRead);
		setBL(0x02092960, (u32)dsiSaveClose);
		*(u32*)0x02092BA8 = 0xE1A00000; // nop
		setBL(0x02092BCC, (u32)dsiSaveCreate);
		setBL(0x02092BDC, (u32)dsiSaveOpen);
		setBL(0x02092DE4, (u32)dsiSaveSetLength);
		setBL(0x02092DF4, (u32)dsiSaveWrite);
		setBL(0x02092DFC, (u32)dsiSaveClose);
		*(u32*)0x02092E08 = 0xE1A00000; // nop
		*(u32*)0x02092E0C = 0xE1A00000; // nop
		*(u32*)0x02092E10 = 0xE1A00000; // nop
		*(u32*)0x02092E14 = 0xE1A00000; // nop
		*(u32*)0x02092E20 = 0xE1A00000; // nop
		*(u32*)0x02092E24 = 0xE1A00000; // nop
		*(u32*)0x020DE420 = 0xE1A00000; // nop
		*(u32*)0x020DE548 = 0xE1A00000; // nop
		*(u32*)0x020DE55C = 0xE1A00000; // nop
		*(u32*)0x020E20C4 = 0xE1A00000; // nop
		*(u32*)0x020E616C = 0xE1A00000; // nop
		*(u32*)0x020E7F64 = 0xE1A00000; // nop
		*(u32*)0x020E7F68 = 0xE1A00000; // nop
		*(u32*)0x020E7F74 = 0xE1A00000; // nop
		*(u32*)0x020E80D4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020E8130, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23E0000
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
	// Requires 8MB of RAM, crashes after first battle with 4MB of RAM, but can get past with a save file
	// BGM is disabled to stay within RAM limitations
	else if (strcmp(romTid, "KS3P") == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleForce[0] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x020271E0;

		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Disable pre-load function
			/* *(u32*)0x0201FE18 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FFB4 = 0xE12FFF1E; // bx lr
			*(u32*)0x02020020 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202008C = 0xE12FFF1E; // bx lr */
			//*(u32*)0x020ABBF0 = 0xE1A00000; // nop
			*(u32*)0x020BD234 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x0201FE98 = 0xE12FFF1E; // bx lr (Disable loading sdat file)
		tonccpy((u32*)0x0201FEB8, ce9->patches->musicPlay, 0xC);
		tonccpy((u32*)0x0201FEF0, ce9->patches->musicStopEffect, 0xC);
		setBL(0x020271E0, (int)ce9->patches->rumble_arm9[0]); // Rumble when hair is whipped
		setBL(0x020922A0, (u32)dsiSaveCreate);
		setBL(0x020922C4, (u32)dsiSaveGetResultCode);
		*(u32*)0x020922D4 = 0xE1A00000; // nop
		setBL(0x020922E0, (u32)dsiSaveCreate);
		*(u32*)0x020922FC = 0xE3A00000; // mov r0, #0
		setBL(0x02092D4C, (u32)dsiSaveOpen);
		*(u32*)0x02092D64 = 0xE1A00000; // nop
		setBL(0x02092D74, (u32)dsiSaveOpen);
		setBL(0x02092D88, (u32)dsiSaveRead);
		setBL(0x02092D90, (u32)dsiSaveClose);
		*(u32*)0x02092FD8 = 0xE1A00000; // nop
		setBL(0x02092FFC, (u32)dsiSaveCreate);
		setBL(0x0209300C, (u32)dsiSaveOpen);
		setBL(0x02093214, (u32)dsiSaveSetLength);
		setBL(0x02093224, (u32)dsiSaveWrite);
		setBL(0x0209322C, (u32)dsiSaveClose);
		*(u32*)0x02093238 = 0xE1A00000; // nop
		*(u32*)0x0209323C = 0xE1A00000; // nop
		*(u32*)0x02093240 = 0xE1A00000; // nop
		*(u32*)0x02093244 = 0xE1A00000; // nop
		*(u32*)0x020DE810 = 0xE1A00000; // nop
		*(u32*)0x020DE938 = 0xE1A00000; // nop
		*(u32*)0x020DE94C = 0xE1A00000; // nop
		*(u32*)0x020E253C = 0xE1A00000; // nop
		*(u32*)0x020E6600 = 0xE1A00000; // nop
		*(u32*)0x020E8400 = 0xE1A00000; // nop
		*(u32*)0x020E8404 = 0xE1A00000; // nop
		*(u32*)0x020E8410 = 0xE1A00000; // nop
		*(u32*)0x020E8570 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020E85CC, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23E0000
		*(u32*)0x020E8700 = 0x02190100;
		*(u32*)0x020E97E4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020E9C18 = 0xE1A00000; // nop
		*(u32*)0x020E9C1C = 0xE1A00000; // nop
		*(u32*)0x020E9C20 = 0xE1A00000; // nop
		*(u32*)0x020E9C24 = 0xE1A00000; // nop
		*(u32*)0x020E9C30 = 0xE1A00000; // nop (Enable error exception screen)
		*(u32*)0x020ED088 = 0xE1A00000; // nop
	}

	// Smart Girl's Playhouse Mini (USA)
	else if (strcmp(romTid, "K2FE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x02005548 = 0xE1A00000; // nop
		*(u32*)0x0200E44C = 0xE1A00000; // nop
		*(u32*)0x020118E8 = 0xE1A00000; // nop
		*(u32*)0x02014C34 = 0xE1A00000; // nop
		*(u32*)0x020169D0 = 0xE1A00000; // nop
		*(u32*)0x020169D4 = 0xE1A00000; // nop
		*(u32*)0x020169E0 = 0xE1A00000; // nop
		*(u32*)0x02016B40 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02016B9C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02016CD0 = 0x02114D80;
		*(u32*)0x02017DE4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201AB7C = 0xE1A00000; // nop
		*(u32*)0x02026128 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202E6F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0202E8B8, (u32)dsiSaveCreate);
		setBL(0x0202E8C8, (u32)dsiSaveOpen);
		setBL(0x0202E8E4, (u32)dsiSaveSeek);
		setBL(0x0202E8F4, (u32)dsiSaveWrite);
		setBL(0x0202E8FC, (u32)dsiSaveClose);
		setBL(0x0202EA24, (u32)dsiSaveOpenR);
		setBL(0x0202EA3C, (u32)dsiSaveSeek);
		setBL(0x0202EA4C, (u32)dsiSaveRead);
		setBL(0x0202EA54, (u32)dsiSaveClose);
		*(u32*)0x0202EA74 = 0xE1A00000; // nop
	}

	// SnowBoard Xtreme (USA)
	// SnowBoard Xtreme (Europe)
	else if (strcmp(romTid, "KX5E") == 0 || strcmp(romTid, "KX5P") == 0) {
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
		*(u32*)0x02010AB8 = 0xE1A00000; // nop
		*(u32*)0x02010AEC = 0xE1A00000; // nop
		setBL(0x02011B70, (u32)dsiSaveCreate);
		*(u32*)0x02011B84 = 0xE1A00000; // nop
		*(u32*)0x02011B90 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011C10 = 0xE1A00000; // nop
		*(u32*)0x02011C14 = 0xE1A00000; // nop
		*(u32*)0x02011C18 = 0xE1A00000; // nop
		setBL(0x02011C24, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011C3C = 0xE1A00000; // nop
		*(u32*)0x02011C48 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x020313AC, (u32)dsiSaveOpen);
			setBL(0x020313C4, (u32)dsiSaveGetLength);
			setBL(0x020313D4, (u32)dsiSaveSeek);
			setBL(0x020313E4, (u32)dsiSaveWrite);
			setBL(0x020313EC, (u32)dsiSaveClose);
			setBL(0x0203145C, (u32)dsiSaveOpen);
			setBL(0x02031474, (u32)dsiSaveGetLength);
			setBL(0x02031488, (u32)dsiSaveSeek);
			setBL(0x02031498, (u32)dsiSaveRead);
			setBL(0x020314A0, (u32)dsiSaveClose);
			setBL(0x02031518, (u32)dsiSaveCreate);
			setBL(0x02031544, (u32)dsiSaveOpen);
			setBL(0x02031580, (u32)dsiSaveWrite);
			setBL(0x02031590, (u32)dsiSaveClose);
			*(u32*)0x0206377C = 0xE1A00000; // nop
			*(u32*)0x0206A098 = 0xE1A00000; // nop
			*(u32*)0x0206BF28 = 0xE1A00000; // nop
			*(u32*)0x0206BF2C = 0xE1A00000; // nop
			*(u32*)0x0206BF38 = 0xE1A00000; // nop
			*(u32*)0x0206C098 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x0206C0F4, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x0206C228 = 0x022546E0;
			*(u32*)0x0207000C = 0xE1A00000; // nop
			*(u32*)0x02072840 = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x020313FC, (u32)dsiSaveOpen);
			setBL(0x02031414, (u32)dsiSaveGetLength);
			setBL(0x02031424, (u32)dsiSaveSeek);
			setBL(0x02031434, (u32)dsiSaveWrite);
			setBL(0x0203143C, (u32)dsiSaveClose);
			setBL(0x020314AC, (u32)dsiSaveOpen);
			setBL(0x020314C4, (u32)dsiSaveGetLength);
			setBL(0x020314D8, (u32)dsiSaveSeek);
			setBL(0x020314E8, (u32)dsiSaveRead);
			setBL(0x020314F0, (u32)dsiSaveClose);
			setBL(0x02031568, (u32)dsiSaveCreate);
			setBL(0x02031594, (u32)dsiSaveOpen);
			setBL(0x020315D0, (u32)dsiSaveWrite);
			setBL(0x020315E0, (u32)dsiSaveClose);
			*(u32*)0x020637CC = 0xE1A00000; // nop
			*(u32*)0x0206A0E8 = 0xE1A00000; // nop
			*(u32*)0x0206BF78 = 0xE1A00000; // nop
			*(u32*)0x0206BF7C = 0xE1A00000; // nop
			*(u32*)0x0206BF88 = 0xE1A00000; // nop
			*(u32*)0x0206C0E8 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x0206C144, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x0206C278 = 0x022547C0;
			*(u32*)0x0207005C = 0xE1A00000; // nop
			*(u32*)0x02072890 = 0xE1A00000; // nop
		}
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
	else if (strcmp(romTid, "KA6E") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02005DD0 = 0xE1A00000; // nop
		//*(u32*)0x02016458 = 0xE1A00000; // nop
		//*(u32*)0x0201645C = 0xE1A00000; // nop
		*(u32*)0x0201F874 = 0xE1A00000; // nop
		setBL(0x0201F8BC, (u32)dsiSaveOpen);
		setBL(0x0201F8D4, (u32)dsiSaveRead);
		setBL(0x0201F8FC, (u32)dsiSaveClose);
		*(u32*)0x0201F960 = 0xE1A00000; // nop
		*(u32*)0x0201F978 = 0xE1A00000; // nop
		*(u32*)0x0201F984 = 0xE1A00000; // nop
		*(u32*)0x0201F98C = 0xE1A00000; // nop
		setBL(0x0201F998, (u32)dsiSaveCreate);
		setBL(0x0201F9C8, (u32)dsiSaveOpen);
		setBL(0x0201F9F8, (u32)dsiSaveWrite);
		setBL(0x0201FA20, (u32)dsiSaveClose);
		*(u32*)0x0201FA54 = 0xE1A00000; // nop
		*(u32*)0x0201FA60 = 0xE1A00000; // nop
		*(u32*)0x0201FAC0 = 0xE1A00000; // nop
		*(u32*)0x0201FAD8 = 0xE1A00000; // nop
		*(u32*)0x0201FAE4 = 0xE1A00000; // nop
		*(u32*)0x0201FAEC = 0xE1A00000; // nop
		setBL(0x0201FAFC, (u32)dsiSaveOpen);
		setBL(0x0201FB38, (u32)dsiSaveSeek);
		setBL(0x0201FB68, (u32)dsiSaveWrite);
		setBL(0x0201FB90, (u32)dsiSaveClose);
		*(u32*)0x0201FBC4 = 0xE1A00000; // nop
		*(u32*)0x0201FBD0 = 0xE1A00000; // nop
		setBL(0x0201FBFC, (u32)dsiSaveGetResultCode);
		setBL(0x0201FC2C, (u32)dsiSaveClose);
		setBL(0x0201FC44, (u32)dsiSaveClose);
		*(u32*)0x0201FC50 = 0xE1A00000; // nop
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
		//*(u32*)0x0203E9B4 = 0xE1A00000; // nop (Forgot what this does)
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
			//*(u32*)0x0203701C = 0xE3A00001; // mov r0, #1
			//*(u32*)0x02037020 = 0xE12FFF1E; // bx lr
			setBL(0x02037560, (u32)dsiSaveOpen);
			*(u32*)0x02037580 = 0xE1A00000; // nop
			setBL(0x020375B0, (u32)dsiSaveCreate);
			setBL(0x020375F0, (u32)dsiSaveOpen);
			setBL(0x0203762C, (u32)dsiSaveSetLength);
			setBL(0x0203767C, (u32)dsiSaveSeek);
			setBL(0x0203768C, (u32)dsiSaveWrite);
			setBL(0x020376C8, (u32)dsiSaveClose);
			setBL(0x020376D8, (u32)dsiSaveClose);
			setBL(0x02037714, (u32)dsiSaveOpen);
			setBL(0x02037754, (u32)dsiSaveSeek);
			setBL(0x02037794, (u32)dsiSaveRead);
			setBL(0x020377A0, (u32)dsiSaveClose);
			setBL(0x02037810, (u32)dsiSaveOpen);
			setBL(0x02037850, (u32)dsiSaveSeek);
			setBL(0x02037890, (u32)dsiSaveWrite);
			setBL(0x0203789C, (u32)dsiSaveGetLength);
			setBL(0x020378C4, (u32)dsiSaveWrite);
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
			//*(u32*)0x0203609C = 0xE3A00001; // mov r0, #1
			//*(u32*)0x020360A0 = 0xE12FFF1E; // bx lr
			setBL(0x020364A4, (u32)dsiSaveOpen);
			*(u32*)0x020364C0 = 0xE1A00000; // nop
			setBL(0x020364F0, (u32)dsiSaveCreate);
			setBL(0x02036530, (u32)dsiSaveOpen);
			setBL(0x0203656C, (u32)dsiSaveSetLength);
			setBL(0x020365B0, (u32)dsiSaveSeek);
			setBL(0x020365C0, (u32)dsiSaveWrite);
			setBL(0x020365FC, (u32)dsiSaveClose);
			setBL(0x02036614, (u32)dsiSaveClose);
			setBL(0x0203665C, (u32)dsiSaveOpen);
			setBL(0x02036694, (u32)dsiSaveSeek);
			setBL(0x020366C8, (u32)dsiSaveRead);
			setBL(0x020366D4, (u32)dsiSaveClose);
			setBL(0x02036724, (u32)dsiSaveOpen);
			setBL(0x0203675C, (u32)dsiSaveSeek);
			setBL(0x02036790, (u32)dsiSaveWrite);
			setBL(0x0203679C, (u32)dsiSaveGetLength);
			setBL(0x020367BC, (u32)dsiSaveClose);
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
		*(u32*)0x0200698C = 0xE1A00000; // nop
		//*(u32*)0x020360E8 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x020360EC = 0xE12FFF1E; // bx lr
		setBL(0x020375AC, (u32)dsiSaveOpen);
		*(u32*)0x020375CC = 0xE1A00000; // nop
		setBL(0x020375FC, (u32)dsiSaveCreate);
		setBL(0x0203763C, (u32)dsiSaveOpen);
		setBL(0x02037678, (u32)dsiSaveSetLength);
		setBL(0x020376C8, (u32)dsiSaveSeek);
		setBL(0x020376D8, (u32)dsiSaveWrite);
		setBL(0x02037714, (u32)dsiSaveClose);
		setBL(0x02037724, (u32)dsiSaveClose);
		setBL(0x02037760, (u32)dsiSaveOpen);
		setBL(0x020377A0, (u32)dsiSaveSeek);
		setBL(0x020377E0, (u32)dsiSaveRead);
		setBL(0x020377EC, (u32)dsiSaveClose);
		setBL(0x0203785C, (u32)dsiSaveOpen);
		setBL(0x0203789C, (u32)dsiSaveSeek);
		setBL(0x020378DC, (u32)dsiSaveWrite);
		setBL(0x020378E8, (u32)dsiSaveGetLength);
		setBL(0x02037910, (u32)dsiSaveWrite);
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

	// Tales to Enjoy!: Little Red Riding Hood (USA)
	// Tales to Enjoy!: Puss in Boots (USA)
	// Tales to Enjoy!: The Three Little Pigs (USA)
	// Tales to Enjoy!: The Ugly Duckling (USA)
	else if (strcmp(romTid, "KZUE") == 0 || strcmp(romTid, "KZVE") == 0 || strcmp(romTid, "KZ7E") == 0 || strcmp(romTid, "KZ8E") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02006F60 = 0xE1A00000; // nop
		*(u32*)0x0200A80C = 0xE1A00000; // nop
		*(u32*)0x0200E0E4 = 0xE1A00000; // nop
		*(u32*)0x0200FECC = 0xE1A00000; // nop
		*(u32*)0x0200FED0 = 0xE1A00000; // nop
		*(u32*)0x0200FEDC = 0xE1A00000; // nop
		*(u32*)0x0201003C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02010098, extendedMemory2 ? 0x02700000 : heapEnd); // mov r0, extendedMemory2 ? #0x2700000 : #0x23C0000
		if (!extendedMemory2) {
			*(u32*)0x020101CC -= 0x30000;
		}
		*(u32*)0x02011470 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02014A7C = 0xE1A00000; // nop
		setBL(0x0204D500, (u32)dsiSaveOpen);
		*(u32*)0x0204D540 = 0xE1A00000; // nop
		setBL(0x0204D560, (u32)dsiSaveGetLength);
		setBL(0x0204D570, (u32)dsiSaveRead);
		setBL(0x0204D578, (u32)dsiSaveClose);
		*(u32*)0x0204D59C = 0xE3A00000; // mov r0, #0
		setBL(0x0204D5C8, (u32)dsiSaveOpen);
		*(u32*)0x0204D5E0 = 0xE1A00000; // nop (dsiSaveGetArcSrc)
		*(u32*)0x0204D5F0 = 0xE3A00001; // mov r0, #1
		setBL(0x0204D60C, (u32)dsiSaveCreate);
		setBL(0x0204D618, (u32)dsiSaveClose);
		setBL(0x0204D62C, (u32)dsiSaveOpen);
		setBL(0x0204D63C, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204D660 = 0xE1A00000; // nop
		setBL(0x0204D684, (u32)dsiSaveSetLength);
		setBL(0x0204D694, (u32)dsiSaveWrite);
		setBL(0x0204D69C, (u32)dsiSaveClose);
	}

	// Tangrams (USA)
	else if (strcmp(romTid, "KYYE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02010A2C = 0xE1A00000; // nop
		*(u32*)0x02014720 = 0xE1A00000; // nop
		*(u32*)0x0201833C = 0xE1A00000; // nop
		*(u32*)0x0201A0D8 = 0xE1A00000; // nop
		*(u32*)0x0201A0DC = 0xE1A00000; // nop
		*(u32*)0x0201A0E8 = 0xE1A00000; // nop
		*(u32*)0x0201A248 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201A2A4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201E434 = 0xE1A00000; // nop
		*(u32*)0x0201FF54 = 0xE1A00000; // nop
		*(u32*)0x0201FF6C = 0xE1A00000; // nop
		*(u32*)0x020397C0 = 0xE1A00000; // nop
		*(u32*)0x02039854 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203986C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0203987C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x020398F8 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x02039910, (u32)dsiSaveCreate);
		setBL(0x02039944, (u32)dsiSaveOpen);
		setBL(0x02039968, (u32)dsiSaveSetLength);
		setBL(0x020399A0, (u32)dsiSaveWrite);
		setBL(0x020399A8, (u32)dsiSaveClose);
		*(u32*)0x02039A6C = 0xE1A00000; // nop (dsiSaveOpenDir)
		setBL(0x02039A80, (u32)dsiSaveOpen);
		setBL(0x02039AD4, (u32)dsiSaveGetLength);
		setBL(0x02039AF8, (u32)dsiSaveRead);
		*(u32*)0x02039B90 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		setBL(0x02039BC8, (u32)dsiSaveClose);
		setBL(0x02039C70, (u32)dsiSaveDelete);
		setBL(0x02039C7C, (u32)dsiSaveCreate);
		setBL(0x02039CB0, (u32)dsiSaveOpen);
		setBL(0x02039D04, (u32)dsiSaveSetLength);
		setBL(0x02039D3C, (u32)dsiSaveWrite);
		setBL(0x02039D44, (u32)dsiSaveClose);
	}

	// Tantei Jinguuji Saburou: Tsubaki no Yukue (Japan)
	else if (strcmp(romTid, "KJTJ") == 0) {
		setBL(0x02005F80, (u32)dsiSaveOpen);
		setBL(0x02005F9C, (u32)dsiSaveCreate);
		*(u32*)0x02005FE0 = 0xE3A00000; // mov r0, #0
		setBL(0x02006008, (u32)dsiSaveCreate);
		setBL(0x02006024, (u32)dsiSaveOpen);
		setBL(0x02006070, (u32)dsiSaveWrite);
		setBL(0x02006080, (u32)dsiSaveClose);
		setBL(0x020060DC, (u32)dsiSaveOpen);
		setBL(0x02006130, (u32)dsiSaveSeek);
		setBL(0x02006140, (u32)dsiSaveRead);
		setBL(0x02006150, (u32)dsiSaveClose);
		setBL(0x020061B4, (u32)dsiSaveOpen);
		setBL(0x02006208, (u32)dsiSaveRead);
		setBL(0x02006238, (u32)dsiSaveClose);
		setBL(0x02006254, (u32)dsiSaveSeek);
		setBL(0x02006264, (u32)dsiSaveWrite);
		setBL(0x02006294, (u32)dsiSaveSeek);
		setBL(0x020062A4, (u32)dsiSaveWrite);
		setBL(0x020062C0, (u32)dsiSaveClose);
		*(u32*)0x0202D3A4 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0202D3A8 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0202E118, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02031180 = 0xE1A00000; // nop
		*(u32*)0x020378E8 = 0xE1A00000; // nop
		*(u32*)0x02039618 = 0xE1A00000; // nop
		*(u32*)0x0203961C = 0xE1A00000; // nop
		*(u32*)0x02039628 = 0xE1A00000; // nop
		*(u32*)0x0203976C = 0xE1A00000; // nop
		*(u32*)0x02039770 = 0xE1A00000; // nop
		*(u32*)0x02039774 = 0xE1A00000; // nop
		*(u32*)0x02039778 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020397D4, heapEnd); // mov r0, #0x23C0000
	}

	// Tantei Jinguuji Saburou: Akenaiyoru ni (Japan)
	// Tantei Jinguuji Saburou: Kadannoitte (Japan)
	// Tantei Jinguuji Saburou: Rensa Suru Noroi (Japan)
	// Tantei Jinguuji Saburou: Nakiko no Shouzou (Japan)
	else if (strcmp(romTid, "KJAJ") == 0 || strcmp(romTid, "KJQJ") == 0 || strcmp(romTid, "KJLJ") == 0 || strcmp(romTid, "KJ7J") == 0) {
		setBL(0x02005FD0, (u32)dsiSaveOpen);
		setBL(0x02005FEC, (u32)dsiSaveCreate);
		*(u32*)0x02006030 = 0xE3A00000; // mov r0, #0
		setBL(0x02006058, (u32)dsiSaveCreate);
		setBL(0x02006074, (u32)dsiSaveOpen);
		setBL(0x020060C0, (u32)dsiSaveWrite);
		setBL(0x020060D0, (u32)dsiSaveClose);
		setBL(0x0200612C, (u32)dsiSaveOpen);
		setBL(0x02006180, (u32)dsiSaveSeek);
		setBL(0x02006190, (u32)dsiSaveRead);
		setBL(0x020061A0, (u32)dsiSaveClose);
		setBL(0x02006204, (u32)dsiSaveOpen);
		setBL(0x02006258, (u32)dsiSaveRead);
		setBL(0x02006288, (u32)dsiSaveClose);
		setBL(0x020062A4, (u32)dsiSaveSeek);
		setBL(0x020062B4, (u32)dsiSaveWrite);
		setBL(0x020062E4, (u32)dsiSaveSeek);
		setBL(0x020062F4, (u32)dsiSaveWrite);
		setBL(0x02006310, (u32)dsiSaveClose);
		if (strncmp(romTid, "KJA", 3) == 0) {
			*(u32*)0x0202D49C = 0xE1A00000; // nop
			tonccpy((u32*)0x0202E130, dsiSaveGetResultCode, 0xC);
			*(u32*)0x020311D8 = 0xE1A00000; // nop
			*(u32*)0x020362A8 = 0xE1A00000; // nop
			*(u32*)0x020380C0 = 0xE1A00000; // nop
			*(u32*)0x020380C4 = 0xE1A00000; // nop
			*(u32*)0x020380D0 = 0xE1A00000; // nop
			*(u32*)0x02038214 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02038270, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x0203996C = 0xE1A00000; // nop
			*(u32*)0x02039970 = 0xE1A00000; // nop
			*(u32*)0x02039974 = 0xE1A00000; // nop
			*(u32*)0x02039978 = 0xE1A00000; // nop
			*(u32*)0x0203CD80 = 0xE1A00000; // nop
		} else if (strncmp(romTid, "KJ7", 3) != 0) {
			*(u32*)0x0202D4B4 = 0xE1A00000; // nop
			tonccpy((u32*)0x0202E148, dsiSaveGetResultCode, 0xC);
			*(u32*)0x020311F0 = 0xE1A00000; // nop
			*(u32*)0x020362C0 = 0xE1A00000; // nop
			*(u32*)0x020380D8 = 0xE1A00000; // nop
			*(u32*)0x020380DC = 0xE1A00000; // nop
			*(u32*)0x020380E8 = 0xE1A00000; // nop
			*(u32*)0x0203822C = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02038288, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x02039984 = 0xE1A00000; // nop
			*(u32*)0x02039988 = 0xE1A00000; // nop
			*(u32*)0x0203998C = 0xE1A00000; // nop
			*(u32*)0x02039990 = 0xE1A00000; // nop
			*(u32*)0x0203CD98 = 0xE1A00000; // nop
		} else {
			*(u32*)0x0202D51C = 0xE1A00000; // nop
			tonccpy((u32*)0x0202E1B0, dsiSaveGetResultCode, 0xC);
			*(u32*)0x02031258 = 0xE1A00000; // nop
			*(u32*)0x02036328 = 0xE1A00000; // nop
			*(u32*)0x02038140 = 0xE1A00000; // nop
			*(u32*)0x02038144 = 0xE1A00000; // nop
			*(u32*)0x02038150 = 0xE1A00000; // nop
			*(u32*)0x02038294 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x020382F0, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x020399EC = 0xE1A00000; // nop
			*(u32*)0x020399F0 = 0xE1A00000; // nop
			*(u32*)0x020399F4 = 0xE1A00000; // nop
			*(u32*)0x020399F8 = 0xE1A00000; // nop
			*(u32*)0x0203CE00 = 0xE1A00000; // nop
		}
	}

	// Tetris Party Live (USA)
	// Tetris Party Live (Europe, Australia)
	else if (strcmp(romTid, "KTEE") == 0 || strcmp(romTid, "KTEV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x02005170 = 0xE1A00000; // nop
		*(u32*)0x02005180 = 0xE1A00000; // nop
		*(u32*)0x020052C0 = 0xE1A00000; // nop
		*(u32*)0x02014828 = 0xE1A00000; // nop
		*(u32*)0x02017C50 = 0xE1A00000; // nop
		*(u32*)0x0201C208 = 0xE1A00000; // nop
		*(u32*)0x0201E2C0 = 0xE1A00000; // nop
		*(u32*)0x0201E2C4 = 0xE1A00000; // nop
		*(u32*)0x0201E2D0 = 0xE1A00000; // nop
		*(u32*)0x0201E430 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201E48C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201F82C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201F848 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F84C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F854 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F858 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F878 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F87C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F88C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F890 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F89C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201F8A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020235FC = 0xE1A00000; // nop
		*(u32*)0x02054C30 = 0xE1A00000; // nop (Skip Manual screen)
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x0205A768, (u32)dsiSaveOpenR);
			setBL(0x0205A778, (u32)dsiSaveGetLength);
			setBL(0x0205A7B0, (u32)dsiSaveRead);
			setBL(0x0205A7CC, (u32)dsiSaveClose);
			//*(u32*)0x0205A83C = 0xE12FFF1E; // bx lr
			setBL(0x0205A864, (u32)dsiSaveCreate);
			setBL(0x0205A874, (u32)dsiSaveGetResultCode);
			setBL(0x0205A89C, (u32)dsiSaveOpen);
			setBL(0x0205A8C0, (u32)dsiSaveWrite);
			setBL(0x0205A8F0, (u32)dsiSaveClose);
			//*(u32*)0x0205A92C = 0xE12FFF1E; // bx lr
			setBL(0x0205A95C, (u32)dsiSaveOpen);
			setBL(0x0205A99C, (u32)dsiSaveSeek);
			setBL(0x0205A9DC, (u32)dsiSaveWrite);
			setBL(0x0205AA54, (u32)dsiSaveSeek);
			setBL(0x0205AA80, (u32)dsiSaveWrite);
			setBL(0x0205AABC, (u32)dsiSaveClose);
			*(u32*)0x0205AAEC = 0xE1A00000; // nop
			*(u32*)0x0205B330 = 0xE1A00000; // nop
			*(u32*)0x0205B39C = 0xE3A00000; // mov r0, #0
		} else {
			setBL(0x0205A754, (u32)dsiSaveOpenR);
			setBL(0x0205A764, (u32)dsiSaveGetLength);
			setBL(0x0205A79C, (u32)dsiSaveRead);
			setBL(0x0205A7B8, (u32)dsiSaveClose);
			//*(u32*)0x0205A828 = 0xE12FFF1E; // bx lr
			setBL(0x0205A850, (u32)dsiSaveCreate);
			setBL(0x0205A860, (u32)dsiSaveGetResultCode);
			setBL(0x0205A888, (u32)dsiSaveOpen);
			setBL(0x0205A8AC, (u32)dsiSaveWrite);
			setBL(0x0205A8DC, (u32)dsiSaveClose);
			//*(u32*)0x0205A918 = 0xE12FFF1E; // bx lr
			setBL(0x0205A948, (u32)dsiSaveOpen);
			setBL(0x0205A988, (u32)dsiSaveSeek);
			setBL(0x0205A9C8, (u32)dsiSaveWrite);
			setBL(0x0205AA40, (u32)dsiSaveSeek);
			setBL(0x0205AA6C, (u32)dsiSaveWrite);
			setBL(0x0205AAA8, (u32)dsiSaveClose);
			*(u32*)0x0205AAD8 = 0xE1A00000; // nop
			*(u32*)0x0205B31C = 0xE1A00000; // nop
			*(u32*)0x0205B388 = 0xE3A00000; // mov r0, #0
		}
		*(u32*)0x0207AD18 = 0xE1A00000; // nop
		*(u32*)0x0207AD20 = 0xE3A00001; // mov r0, #1
		setB(0x0207BCD0, 0x0207BDA0);
		*(u32*)0x0207BDA0 = 0xE1A00000; // nop
		*(u32*)0x0207BDA4 = 0xE1A00000; // nop
		*(u32*)0x0207BDAC = 0xE1A00000; // nop
		*(u32*)0x0207BDB0 = 0xE1A00000; // nop
		*(u32*)0x0207BDB4 = 0xE1A00000; // nop
		*(u32*)0x0207BDB8 = 0xE1A00000; // nop
		setB(0x0207C52C, 0x0207C5D0);
		*(u32*)0x0207C734 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207C738 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207C790 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207C794 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207C850 = 0xE1A00000; // nop
		*(u32*)0x0207C854 = 0xE1A00000; // nop
		setB(0x0207D4B8, 0x0207D65C);
		*(u32*)0x0207DCC8 = 0xE1A00000; // nop
		*(u32*)0x0207DCCC = 0xE1A00000; // nop
		*(u32*)0x0207DCD0 = 0xE1A00000; // nop
		*(u32*)0x0207DCD4 = 0xE3A01001; // mov r1, #1
		setB(0x0207F01C, 0x0207F038);
		setB(0x0207F27C, 0x0207F2A4);
		*(u32*)0x0207F2A4 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
		*(u32*)0x0207F2A8 += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
		*(u32*)0x0207F2AC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207F2B0 = 0xE5850030; // str r0, [r5,#0x30]
		*(u32*)0x0207F2B4 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
		*(u32*)0x0209A674 = 0xE3A02C07; // mov r2, #0x700
		*(u32*)0x0209A694 = 0xE2840B01; // add r0, r4, #0x400
		*(u32*)0x0209A69C = 0xE1A00004; // mov r0, r4
		*(u32*)0x0209A6A4 = 0xE1A00000; // nop
		*(u32*)0x0209A6A8 = 0xE1A00000; // nop
		*(u32*)0x0209A6AC = 0xE1A00000; // nop
		*(u32*)0x0209A6B0 = 0xE1A00000; // nop
		*(u32*)0x0209A6B4 = 0xE1A00000; // nop
		*(u32*)0x0209A6C8 = 0xE2841B01; // add r1, r4, #0x400
		*(u32*)0x020A3200 = 0xE3A00000; // mov r0, #0
		setBL(0x020A32DC, 0x020A4830);
		setBL(0x020A338C, 0x020A4968);
		setBL(0x020A3440, 0x020A49D4);
		setBL(0x020A36AC, 0x020A4ADC);
		setBL(0x020A3784, 0x020A4B88);
		setBL(0x020A38B8, 0x020A4BF4);
		setBL(0x020A39E4, 0x020A4D94);
		*(u32*)0x020A3D94 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020A3DC4 = 0xE3A00000; // mov r0, #0
		setBL(0x020A451C, 0x020A4CDC);
	}

	// Topoloco (USA)
	// Topoloco (Europe)
	// Requires 8MB of RAM
	/*else if (strncmp(romTid, "KT5", 3) == 0 && extendedMemory2) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02014F78 = 0xE1A00000; // nop
		*(u32*)0x02018808 = 0xE1A00000; // nop
		*(u32*)0x0201C4AC = 0xE1A00000; // nop
		*(u32*)0x0201E334 = 0xE1A00000; // nop
		*(u32*)0x0201E338 = 0xE1A00000; // nop
		*(u32*)0x0201E344 = 0xE1A00000; // nop
		*(u32*)0x0201E4A4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201E500, 0x02700000); // mov r0, #0x2700000
		*(u32*)0x0201F8A4 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x020227C0 = 0xE1A00000; // nop
		*(u32*)0x020240D4 = 0xE1A00000; // nop
		*(u32*)0x020241D8 = 0xE1A00000; // nop
		*(u32*)0x02024200 = 0xE1A00000; // nop
		*(u32*)0x020516A4 = 0xE1A00000; // nop
		setBL(0x02051B38, (u32)dsiSaveDelete);
		setBL(0x02051BB8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02051CE0, (u32)dsiSaveOpen);
		setBL(0x02051D64, (u32)dsiSaveGetLength);
		setBL(0x02051DB4, (u32)dsiSaveClose);
		setBL(0x02051DE0, (u32)dsiSaveSetLength);
		setBL(0x02051E00, (u32)dsiSaveClose);
		setBL(0x02051E80, (u32)dsiSaveRead);
		setBL(0x02051ECC, (u32)dsiSaveClose);
		setBL(0x02051EE4, (u32)dsiSaveRead);
		setBL(0x02051EF4, (u32)dsiSaveClose);
		setBL(0x02051F30, (u32)dsiSaveWrite);
		setBL(0x02051F74, (u32)dsiSaveClose);
		setBL(0x02051F8C, (u32)dsiSaveWrite);
		setBL(0x02051FA0, (u32)dsiSaveClose);
		setBL(0x02051FB8, (u32)dsiSaveClose);
	}*/

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
		patchHiHeapDSiWareThumbOld(0x02022684, 0x209C, 0x0480); // movs r0, #0x2700000
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

	// True Swing Golf Express (USA)
	// A Little Bit of... Nintendo Touch Golf (Europe, Australia)
	// Crashes on white screens when going to menu
	/*else if ((strcmp(romTid, "K72E") == 0 || strcmp(romTid, "K72V") == 0) && extendedMemory2) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02009A84 = 0xE12FFF1E; // bx lr
		setBL(0x02009AC0, (u32)dsiSaveOpen);
		setBL(0x02009AE0, (u32)dsiSaveGetLength);
		setBL(0x02009B48, (u32)dsiSaveClose);
		setBL(0x02009BEC, (u32)dsiSaveOpen);
		setBL(0x02009C00, (u32)dsiSaveCreate);
		setBL(0x02009C18, (u32)dsiSaveOpen);
		setBL(0x02009C2C, (u32)dsiSaveSetLength);
		setBL(0x02009C64, (u32)dsiSaveGetLength);
		setBL(0x02009CA0, (u32)dsiSaveSeek);
		setBL(0x02009CB8, (u32)dsiSaveWrite);
		setBL(0x02009D34, (u32)dsiSaveOpen);
		setBL(0x02009D4C, (u32)dsiSaveSeek);
		setBL(0x02009D5C, (u32)dsiSaveSeek);
		setBL(0x02009D6C, (u32)dsiSaveRead);
		setBL(0x02009D80, (u32)dsiSaveClose);
		setBL(0x02009E4C, (u32)dsiSaveOpen);
		setBL(0x02009E64, (u32)dsiSaveSeek);
		setBL(0x02009E78, (u32)dsiSaveSeek);
		setBL(0x02009E88, (u32)dsiSaveWrite);
		setBL(0x02009E9C, (u32)dsiSaveClose);
		setBL(0x02009F2C, (u32)dsiSaveOpen);
		setBL(0x02009F70, (u32)dsiSaveSeek);
		setBL(0x02009F80, (u32)dsiSaveSeek);
		setBL(0x02009F90, (u32)dsiSaveWrite);
		setBL(0x02009FC4, (u32)dsiSaveClose);
		setBL(0x0200A024, (u32)dsiSaveOpen);
		setBL(0x0200A03C, (u32)dsiSaveSeek);
		setBL(0x0200A04C, (u32)dsiSaveSeek);
		setBL(0x0200A05C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0200A0AC, (u32)dsiSaveClose);
		setBL(0x0200A134, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02063214 = 0xE1A00000; // nop
			*(u32*)0x02066E74 = 0xE1A00000; // nop
			*(u32*)0x02072B10 = 0xE1A00000; // nop
			*(u32*)0x020749D4 = 0xE1A00000; // nop
			*(u32*)0x020749D8 = 0xE1A00000; // nop
			*(u32*)0x020749E4 = 0xE1A00000; // nop
			*(u32*)0x02074B44 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02074BA0, 0x02F00000); // mov r0, #0x2F00000
			*(u32*)0x020760C8 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
			*(u32*)0x020760E4 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020760E8 = 0xE12FFF1E; // bx lr
			*(u32*)0x020760F0 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020760F4 = 0xE12FFF1E; // bx lr
			*(u32*)0x02079700 = 0xE1A00000; // nop
		} else {
			*(u32*)0x02062FA8 = 0xE1A00000; // nop
			*(u32*)0x02066C08 = 0xE1A00000; // nop
			*(u32*)0x020728A4 = 0xE1A00000; // nop
			*(u32*)0x02074768 = 0xE1A00000; // nop
			*(u32*)0x0207476C = 0xE1A00000; // nop
			*(u32*)0x02074778 = 0xE1A00000; // nop
			*(u32*)0x020748D8 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x02074934, 0x02F00000); // mov r0, #0x2F00000
			*(u32*)0x02075E5C = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
			*(u32*)0x02075E78 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02075E7C = 0xE12FFF1E; // bx lr
			*(u32*)0x02075E84 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02075E88 = 0xE12FFF1E; // bx lr
			*(u32*)0x02079494 = 0xE1A00000; // nop
		}
	}*/

	// Turn: The Lost Artifact (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KTIE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x020051F4 = 0xE1A00000; // nop
		*(u32*)0x020128A8 = 0xE1A00000; // nop
		*(u32*)0x02015B98 = 0xE1A00000; // nop
		*(u32*)0x0201A2BC = 0xE1A00000; // nop
		*(u32*)0x0201C164 = 0xE1A00000; // nop
		*(u32*)0x0201C168 = 0xE1A00000; // nop
		*(u32*)0x0201C174 = 0xE1A00000; // nop
		*(u32*)0x0201C2D4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201C330, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0201D754 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0201D770 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201D774 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201D77C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D780 = 0xE12FFF1E; // bx lr
		*(u32*)0x02020544 = 0xE1A00000; // nop
		*(u32*)0x0202BE5C = 0xE1A00000; // nop
		*(u32*)0x0202BE64 = 0xE1A00000; // nop
	}

	// Unou to Sanougaren Sasuru: Uranoura (Japan)
	// Unable to read saved data
	else if (strcmp(romTid, "K6PJ") == 0) {
		*(u32*)0x02006E84 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		//*(u32*)0x02007344 = 0xE1A00000; // nop (Skip directory browse)
		*(u32*)0x020092D4 = 0xE3A00000; // mov r0, #0 (Disable NFTR loading from TWLNAND)
		for (int i = 0; i < 11; i++) { // Skip Manual screen
			u32* offset = (u32*)0x0200A608;
			offset[i] = 0xE1A00000; // nop
		}
		/*setBL(0x02020B50, (u32)dsiSaveOpen);
		*(u32*)0x02020B88 = 0xE1A00000; // nop
		setBL(0x02020B94, (u32)dsiSaveGetLength);
		setBL(0x02020BB4, (u32)dsiSaveRead);
		*(u32*)0x02020BDC = 0xE1A00000; // nop
		setBL(0x02020BE4, (u32)dsiSaveClose);
		*(u32*)0x02020C40 = 0xE1A00000; // nop
		setBL(0x02020C50, (u32)dsiSaveOpen); // dsiSaveOpenDir
		*(u32*)0x02020C7C = 0xE1A00000; // nop
		*(u32*)0x02020C98 = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		*(u32*)0x02020D94 = 0xE1A00000; // nop
		setBL(0x02020D9C, (u32)dsiSaveClose); // dsiSaveCloseDir
		setBL(0x02020F58, (u32)dsiSaveCreate);
		*(u32*)0x02020F80 = 0xE1A00000; // nop
		setBL(0x02020F90, (u32)dsiSaveOpen);
		*(u32*)0x02020FBC = 0xE1A00000; // nop
		setBL(0x02020FE8, (u32)dsiSaveSetLength);
		setBL(0x02020FF8, (u32)dsiSaveWrite);
		*(u32*)0x02021020 = 0xE1A00000; // nop
		setBL(0x02021028, (u32)dsiSaveClose);*/
		*(u32*)0x02038BE0 = 0xE1A00000; // nop
		//tonccpy((u32*)0x02039874, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0203CC30 = 0xE1A00000; // nop
		*(u32*)0x02042AF8 = 0xE1A00000; // nop
		*(u32*)0x02044928 = 0xE1A00000; // nop
		*(u32*)0x0204492C = 0xE1A00000; // nop
		*(u32*)0x02044938 = 0xE1A00000; // nop
		*(u32*)0x02044A7C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02044AD8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02045D78 = 0xE1A00000; // nop
		*(u32*)0x02045D80 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
		*(u32*)0x0204940C = 0xE1A00000; // nop
	}

	// VT Tennis (USA)
	else if (strcmp(romTid, "KVTE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x0200509C = 0xE1A00000; // nop
		*(u32*)0x020058EC = 0xE1A00000; // nop
		*(u32*)0x0201AA9C = 0xE1A00000; // nop
		tonccpy((u32*)0x0201B634, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201E184 = 0xE1A00000; // nop
		*(u32*)0x020246CC = 0xE1A00000; // nop
		*(u32*)0x02026608 = 0xE1A00000; // nop
		*(u32*)0x0202660C = 0xE1A00000; // nop
		*(u32*)0x02026618 = 0xE1A00000; // nop
		*(u32*)0x02026778 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020267D4, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02027A48 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202AFD4 = 0xE1A00000; // nop
		*(u32*)0x0205C000 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		setBL(0x0209C1B0, (u32)dsiSaveCreate);
		setBL(0x0209C1C0, (u32)dsiSaveOpen);
		setBL(0x0209C1E8, (u32)dsiSaveSetLength);
		setBL(0x0209C1F8, (u32)dsiSaveWrite);
		setBL(0x0209C200, (u32)dsiSaveClose);
		*(u32*)0x0209C21C = 0xE1A00000; // nop
		setBL(0x0209C284, (u32)dsiSaveOpen);
		setBL(0x0209C2B0, (u32)dsiSaveRead);
		setBL(0x0209C2BC, (u32)dsiSaveClose);
	}

	// VT Tennis (Europe, Australia)
	else if (strcmp(romTid, "KVTV") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005084 = 0xE1A00000; // nop
		*(u32*)0x020057D0 = 0xE1A00000; // nop
		*(u32*)0x0201A168 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201AD00, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201D850 = 0xE1A00000; // nop
		*(u32*)0x02023D98 = 0xE1A00000; // nop
		*(u32*)0x02025CD4 = 0xE1A00000; // nop
		*(u32*)0x02025CD8 = 0xE1A00000; // nop
		*(u32*)0x02025CE4 = 0xE1A00000; // nop
		*(u32*)0x02025E44 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02025EA0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02027114 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x0202A6A0 = 0xE1A00000; // nop
		*(u32*)0x0205B314 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		setBL(0x0209B120, (u32)dsiSaveCreate);
		setBL(0x0209B130, (u32)dsiSaveOpen);
		setBL(0x0209B158, (u32)dsiSaveSetLength);
		setBL(0x0209B168, (u32)dsiSaveWrite);
		setBL(0x0209B170, (u32)dsiSaveClose);
		*(u32*)0x0209B18C = 0xE1A00000; // nop
		setBL(0x0209B1F4, (u32)dsiSaveOpen);
		setBL(0x0209B220, (u32)dsiSaveRead);
		setBL(0x0209B22C, (u32)dsiSaveClose);
	}

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

	// WarioWare: Touched! DL (USA, Australia)
	// The sound loading code has been reworked to instead load the SDAT file all at once, so sound is disabled in order for the game to boot within RAM limitations
	else if (strcmp(romTid, "Z2AT") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x02009294 = 0xE1A00000; // nop
		*(u32*)0x020092A8 = 0xE1A00000; // nop
		*(u32*)0x0200937C = 0xE3A00000; // mov r0, #0 (Skip loading SDAT file)
		*(u32*)0x02009380 = 0xE12FFF1E; // bx lr
		setBL(0x0200BCA4, (u32)dsiSaveOpen);
		setBL(0x0200BCB4, (u32)dsiSaveGetLength);
		setBL(0x0200BCC8, (u32)dsiSaveSetLength);
		setBL(0x0200BCD0, (u32)dsiSaveClose);
		setBL(0x0200BD0C, (u32)dsiSaveCreate);
		setBL(0x0200BD1C, (u32)dsiSaveOpen);
		setBL(0x0200BD30, (u32)dsiSaveSetLength);
		setBL(0x0200BD38, (u32)dsiSaveClose);
		setBL(0x0200BDE0, (u32)dsiSaveOpen);
		setBL(0x0200BE10, (u32)dsiSaveSeek);
		setBL(0x0200BE20, (u32)dsiSaveRead);
		setBL(0x0200BE28, (u32)dsiSaveClose);
		setBL(0x0200BE84, (u32)dsiSaveOpen);
		setBL(0x0200BEC8, (u32)dsiSaveOpen);
		setBL(0x0200BEFC, (u32)dsiSaveSeek);
		setBL(0x0200BF0C, (u32)dsiSaveWrite);
		setBL(0x0200BF14, (u32)dsiSaveClose);
		*(u32*)0x0200BF3C = 0xE1A00000; // nop
		setBL(0x0200BF7C, (u32)dsiSaveCreate);
		setBL(0x0200BF90, (u32)dsiSaveOpen);
		setBL(0x0200BFA4, (u32)dsiSaveSetLength);
		setBL(0x0200BFAC, (u32)dsiSaveClose);
		*(u32*)0x020689F0 = 0xE3A00000; // mov r0, #0 (Skip playing sound file)
		*(u32*)0x020689F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206AA90 = 0xE1A00000; // nop
		*(u32*)0x0206E500 = 0xE1A00000; // nop
		*(u32*)0x02073B70 = 0xE1A00000; // nop
		*(u32*)0x020758E4 = 0xE1A00000; // nop
		*(u32*)0x020758E8 = 0xE1A00000; // nop
		*(u32*)0x020758F4 = 0xE1A00000; // nop
		*(u32*)0x02075A54 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02075AB0, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0207A224 = 0xE1A00000; // nop
	}

	// WarioWare: Touched! DL (Europe)
	// The sound loading code has been reworked to instead load the SDAT file all at once, so sound is disabled in order for the game to boot within RAM limitations
	else if (strcmp(romTid, "Z2AP") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020092F4 = 0xE1A00000; // nop
		*(u32*)0x02009308 = 0xE1A00000; // nop
		*(u32*)0x020093DC = 0xE3A00000; // mov r0, #0 (Skip loading SDAT file)
		*(u32*)0x020093E0 = 0xE12FFF1E; // bx lr
		setBL(0x0200BD04, (u32)dsiSaveOpen);
		setBL(0x0200BD14, (u32)dsiSaveGetLength);
		setBL(0x0200BD28, (u32)dsiSaveSetLength);
		setBL(0x0200BD30, (u32)dsiSaveClose);
		setBL(0x0200BD6C, (u32)dsiSaveCreate);
		setBL(0x0200BD7C, (u32)dsiSaveOpen);
		setBL(0x0200BD90, (u32)dsiSaveSetLength);
		setBL(0x0200BD98, (u32)dsiSaveClose);
		setBL(0x0200BE40, (u32)dsiSaveOpen);
		setBL(0x0200BE70, (u32)dsiSaveSeek);
		setBL(0x0200BE80, (u32)dsiSaveRead);
		setBL(0x0200BE88, (u32)dsiSaveClose);
		setBL(0x0200BEE4, (u32)dsiSaveOpen);
		setBL(0x0200BF28, (u32)dsiSaveOpen);
		setBL(0x0200BF5C, (u32)dsiSaveSeek);
		setBL(0x0200BF6C, (u32)dsiSaveWrite);
		setBL(0x0200BF74, (u32)dsiSaveClose);
		*(u32*)0x0200BF9C = 0xE1A00000; // nop
		setBL(0x0200BFDC, (u32)dsiSaveCreate);
		setBL(0x0200BFF0, (u32)dsiSaveOpen);
		setBL(0x0200C004, (u32)dsiSaveSetLength);
		setBL(0x0200C00C, (u32)dsiSaveClose);
		*(u32*)0x0206A9A4 = 0xE3A00000; // mov r0, #0 (Skip playing sound file)
		*(u32*)0x0206A9A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206CA44 = 0xE1A00000; // nop
		*(u32*)0x020704B4 = 0xE1A00000; // nop
		*(u32*)0x02075B24 = 0xE1A00000; // nop
		*(u32*)0x02077898 = 0xE1A00000; // nop
		*(u32*)0x0207789C = 0xE1A00000; // nop
		*(u32*)0x020778A8 = 0xE1A00000; // nop
		*(u32*)0x02077A08 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02077A64, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0207C258 = 0xE1A00000; // nop
	}

	// Sawaru Made in Wario DL (Japan)
	// The sound loading code has been reworked to instead load the SDAT file all at once, so sound is disabled in order for the game to boot within RAM limitations
	else if (strcmp(romTid, "Z2AJ") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x02009290 = 0xE1A00000; // nop
		*(u32*)0x020092A4 = 0xE1A00000; // nop
		*(u32*)0x02009378 = 0xE3A00000; // mov r0, #0 (Skip loading SDAT file)
		*(u32*)0x0200937C = 0xE12FFF1E; // bx lr
		setBL(0x0200BCA0, (u32)dsiSaveOpen);
		setBL(0x0200BCB0, (u32)dsiSaveGetLength);
		setBL(0x0200BCC4, (u32)dsiSaveSetLength);
		setBL(0x0200BCCC, (u32)dsiSaveClose);
		setBL(0x0200BD08, (u32)dsiSaveCreate);
		setBL(0x0200BD18, (u32)dsiSaveOpen);
		setBL(0x0200BD2C, (u32)dsiSaveSetLength);
		setBL(0x0200BD34, (u32)dsiSaveClose);
		setBL(0x0200BDDC, (u32)dsiSaveOpen);
		setBL(0x0200BE0C, (u32)dsiSaveSeek);
		setBL(0x0200BE1C, (u32)dsiSaveRead);
		setBL(0x0200BE24, (u32)dsiSaveClose);
		setBL(0x0200BE80, (u32)dsiSaveOpen);
		setBL(0x0200BEC4, (u32)dsiSaveOpen);
		setBL(0x0200BEF8, (u32)dsiSaveSeek);
		setBL(0x0200BF08, (u32)dsiSaveWrite);
		setBL(0x0200BF10, (u32)dsiSaveClose);
		*(u32*)0x0200BF38 = 0xE1A00000; // nop
		setBL(0x0200BF78, (u32)dsiSaveCreate);
		setBL(0x0200BF8C, (u32)dsiSaveOpen);
		setBL(0x0200BFA0, (u32)dsiSaveSetLength);
		setBL(0x0200BFA8, (u32)dsiSaveClose);
		*(u32*)0x02068970 = 0xE3A00000; // mov r0, #0 (Skip playing sound file)
		*(u32*)0x02068974 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206AA10 = 0xE1A00000; // nop
		*(u32*)0x0206E480 = 0xE1A00000; // nop
		*(u32*)0x02073AF0 = 0xE1A00000; // nop
		*(u32*)0x02075864 = 0xE1A00000; // nop
		*(u32*)0x02075868 = 0xE1A00000; // nop
		*(u32*)0x02075874 = 0xE1A00000; // nop
		*(u32*)0x020759D4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02075A30, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0207A1A4 = 0xE1A00000; // nop
	}

	// White-Water Domo (USA)
	else if (strcmp(romTid, "KDWE") == 0) {
		const u32 dsiSaveCreateT = 0x02023258;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02023268;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02023278;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02023288;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02023298;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x020232A8;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020232B8;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0200C918 = 0x2001; // movs r0, #1
		*(u16*)0x0200C91A = 0x4770; // bx lr
		doubleNopT(0x0200CBC6);
		*(u16*)0x0200CC08 = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0200CC0A = 0x4770; // bx lr
		setBLThumb(0x0200CC6E, dsiSaveCreateT);
		setBLThumb(0x0200CC84, dsiSaveOpenT);
		setBLThumb(0x0200CCA0, dsiSaveSetLengthT);
		setBLThumb(0x0200CCB4, dsiSaveWriteT);
		setBLThumb(0x0200CCC6, dsiSaveCloseT);
		*(u16*)0x0200CCEC = 0x4778; // bx pc
		tonccpy((u32*)0x0200CCF0, dsiSaveGetLength, 0xC);
		setBLThumb(0x0200CD1C, dsiSaveOpenT);
		setBLThumb(0x0200CD42, dsiSaveCloseT);
		setBLThumb(0x0200CD54, dsiSaveReadT);
		setBLThumb(0x0200CD5A, dsiSaveCloseT);
		setBLThumb(0x0200CD6E, dsiSaveDeleteT);
		*(u16*)0x02013B10 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x020223BE);
		doubleNopT(0x0202460E);
		doubleNopT(0x020272C4);
		doubleNopT(0x0202887E);
		doubleNopT(0x02028882);
		doubleNopT(0x0202888E);
		doubleNopT(0x02028972);
		patchHiHeapDSiWareThumb(0x020289B0, 0x020232C8, heapEnd); // movs r0, #0x23C0000
		doubleNopT(0x02029A36);
		*(u16*)0x02029A3A = 0x46C0;
		*(u16*)0x02029A3C = 0x46C0;
		doubleNopT(0x02029A3E);
		doubleNopT(0x0202BAF2);
	}

	// Art Style: ZENGAGE (USA)
	// Art Style: NEMREM (Europe, Australia)
	else if (strcmp(romTid, "KASE") == 0 || strcmp(romTid, "KASV") == 0) {
		*(u32*)0x0200E000 = 0xE1A00000; // nop
		*(u32*)0x0200E080 = 0xE1A00000; // nop
		*(u32*)0x0200E1E8 = 0xE1A00000; // nop
		*(u32*)0x0200E290 = 0xE1A00000; // nop
		setBL(0x0200E984, (u32)dsiSaveOpen);
		setBL(0x0200EA8C, (u32)dsiSaveOpen);
		setBL(0x0200EAE4, (u32)dsiSaveRead);
		setBL(0x0200EB28, (u32)dsiSaveClose);
		//*(u32*)0x0200EBC8 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200EBCC = 0xE12FFF1E; // bx lr
		setBL(0x0200EBF4, (u32)dsiSaveCreate);
		setBL(0x0200EC04, (u32)dsiSaveGetResultCode);
		setBL(0x0200EC28, (u32)dsiSaveOpen);
		setBL(0x0200EC48, (u32)dsiSaveSetLength);
		setBL(0x0200EC68, (u32)dsiSaveWrite);
		setBL(0x0200EC84, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
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
		} else if (ndsHeader->gameCode[3] == 'V') {
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

	// Art Style: SOMNIUM (Japan)
	else if (strcmp(romTid, "KASJ") == 0) {
		*(u32*)0x0200E280 = 0xE1A00000; // nop
		*(u32*)0x0200E300 = 0xE1A00000; // nop
		*(u32*)0x0200E480 = 0xE1A00000; // nop
		*(u32*)0x0200E534 = 0xE1A00000; // nop
		setBL(0x0200EC4C, (u32)dsiSaveOpen);
		setBL(0x0200ED4C, (u32)dsiSaveOpen);
		setBL(0x0200EDA0, (u32)dsiSaveRead);
		setBL(0x0200EDF0, (u32)dsiSaveClose);
		//*(u32*)0x0200EE98 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200EE9C = 0xE12FFF1E; // bx lr
		setBL(0x0200EEC0, (u32)dsiSaveCreate);
		setBL(0x0200EED0, (u32)dsiSaveGetResultCode);
		setBL(0x0200EEEC, (u32)dsiSaveOpen);
		setBL(0x0200EF08, (u32)dsiSaveSetLength);
		setBL(0x0200EF28, (u32)dsiSaveWrite);
		setBL(0x0200EF44, (u32)dsiSaveClose);
		*(u32*)0x0201D190 = 0xE1A00000; // nop
		*(u32*)0x0201D194 = 0xE1A00000; // nop
		*(u32*)0x0201D1A4 = 0xE1A00000; // nop
		*(u32*)0x0201D554 = 0xE1A00000; // nop
		*(u32*)0x0201D560 = 0xE1A00000; // nop
		*(u32*)0x0201D588 = 0xE1A00000; // nop
		*(u32*)0x0201DAAC = 0xE1A00000; // nop
		*(u32*)0x0201DAC4 = 0xE1A00000; // nop
		*(u32*)0x0201DB70 = 0xE1A00000; // nop
		*(u32*)0x02035688 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0203568C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02038E24 = 0xE1A00000; // nop
		*(u32*)0x0203FA44 = 0xE1A00000; // nop
		*(u32*)0x02041830 = 0xE1A00000; // nop
		*(u32*)0x02041834 = 0xE1A00000; // nop
		*(u32*)0x02041840 = 0xE1A00000; // nop
		*(u32*)0x02041984 = 0xE1A00000; // nop
		*(u32*)0x02041988 = 0xE1A00000; // nop
		*(u32*)0x0204198C = 0xE1A00000; // nop
		*(u32*)0x02041990 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020419EC, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02042D50 = 0xE1A00000; // nop
		*(u32*)0x02042D58 = 0xE8BD8010; // LDMFD SP!, {R4,PC}
	}

	// Zombie Blaster (USA)
	else if (strcmp(romTid, "K7KE") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0201A710 = 0xE1A00000; // nop
		*(u32*)0x0201E2D0 = 0xE1A00000; // nop
		*(u32*)0x02024888 = 0xE1A00000; // nop
		*(u32*)0x0202682C = 0xE1A00000; // nop
		*(u32*)0x02026830 = 0xE1A00000; // nop
		*(u32*)0x0202683C = 0xE1A00000; // nop
		*(u32*)0x0202699C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020269F8, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x0202B29C = 0xE1A00000; // nop
		*(u32*)0x0202D5D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202D5DC = 0xE12FFF1E; // bx lr
		setBL(0x02065568, (u32)dsiSaveOpenR);
		setBL(0x02065584, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x020655C0, (u32)dsiSaveOpen);
		setBL(0x020655D4, (u32)dsiSaveGetResultCode);
		*(u32*)0x020655EC = 0xE1A00000; // nop
		setBL(0x02067068, (u32)dsiSaveOpen);
		setBL(0x02067084, (u32)dsiSaveWrite);
		setBL(0x02067090, (u32)dsiSaveClose);
		setBL(0x020670E4, (u32)dsiSaveOpen);
		setBL(0x020670F8, (u32)dsiSaveGetLength);
		setBL(0x0206710C, (u32)dsiSaveRead);
		setBL(0x02067118, (u32)dsiSaveClose);
	}

	// Zombie Skape (USA)
	// Zombie Skape (Europe)
	else if (strcmp(romTid, "KZYE") == 0 || strcmp(romTid, "KZYP") == 0) {
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
		*(u32*)0x02010AB8 = 0xE1A00000; // nop
		*(u32*)0x02010AEC = 0xE1A00000; // nop
		setBL(0x02011D58, (u32)dsiSaveCreate);
		*(u32*)0x02011D6C = 0xE1A00000; // nop
		*(u32*)0x02011D78 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011DF8 = 0xE1A00000; // nop
		*(u32*)0x02011DFC = 0xE1A00000; // nop
		*(u32*)0x02011E00 = 0xE1A00000; // nop
		setBL(0x02011E0C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011E24 = 0xE1A00000; // nop
		*(u32*)0x02011E30 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x02032118, (u32)dsiSaveOpen);
			setBL(0x02032130, (u32)dsiSaveGetLength);
			setBL(0x02032140, (u32)dsiSaveSeek);
			setBL(0x02032150, (u32)dsiSaveWrite);
			setBL(0x02032158, (u32)dsiSaveClose);
			setBL(0x020321C8, (u32)dsiSaveOpen);
			setBL(0x020321E0, (u32)dsiSaveGetLength);
			setBL(0x020321F4, (u32)dsiSaveSeek);
			setBL(0x02032204, (u32)dsiSaveRead);
			setBL(0x0203220C, (u32)dsiSaveClose);
			setBL(0x02032284, (u32)dsiSaveCreate);
			setBL(0x020322B0, (u32)dsiSaveOpen);
			setBL(0x020322EC, (u32)dsiSaveWrite);
			setBL(0x020322FC, (u32)dsiSaveClose);
			*(u32*)0x020641BC = 0xE1A00000; // nop
			*(u32*)0x0206AAD8 = 0xE1A00000; // nop
			*(u32*)0x0206C968 = 0xE1A00000; // nop
			*(u32*)0x0206C96C = 0xE1A00000; // nop
			*(u32*)0x0206C978 = 0xE1A00000; // nop
			*(u32*)0x0206CAD8 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x0206CB34, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x0206CC68 = 0x02255080;
			*(u32*)0x02070A4C = 0xE1A00000; // nop
			*(u32*)0x02073280 = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x02032164, (u32)dsiSaveOpen);
			setBL(0x0203217C, (u32)dsiSaveGetLength);
			setBL(0x0203218C, (u32)dsiSaveSeek);
			setBL(0x0203219C, (u32)dsiSaveWrite);
			setBL(0x020321A4, (u32)dsiSaveClose);
			setBL(0x02032214, (u32)dsiSaveOpen);
			setBL(0x0203222C, (u32)dsiSaveGetLength);
			setBL(0x02032240, (u32)dsiSaveSeek);
			setBL(0x02032250, (u32)dsiSaveRead);
			setBL(0x02032258, (u32)dsiSaveClose);
			setBL(0x020322D0, (u32)dsiSaveCreate);
			setBL(0x020322FC, (u32)dsiSaveOpen);
			setBL(0x02032338, (u32)dsiSaveWrite);
			setBL(0x02032348, (u32)dsiSaveClose);
			*(u32*)0x02064208 = 0xE1A00000; // nop
			*(u32*)0x0206AB24 = 0xE1A00000; // nop
			*(u32*)0x0206C9B4 = 0xE1A00000; // nop
			*(u32*)0x0206C9B8 = 0xE1A00000; // nop
			*(u32*)0x0206C9C4 = 0xE1A00000; // nop
			*(u32*)0x0206CB24 = 0xE1A00000; // nop
			patchHiHeapDSiWare(0x0206CB80, heapEnd); // mov r0, #0x23C0000
			*(u32*)0x0206CCB4 = 0x02255160;
			*(u32*)0x02070A98 = 0xE1A00000; // nop
			*(u32*)0x020732CC = 0xE1A00000; // nop
		}
	}

	// Zuma's Revenge! (USA)
	// Zuma's Revenge! (Europe, Australia)
	else if (strcmp(romTid, "KZTE") == 0 || strcmp(romTid, "KZTV") == 0) {
		*(u32*)0x02004838 = 0xE1A00000; // nop
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		setBL(0x02014134, (u32)dsiSaveOpen);
		setBL(0x02014158, (u32)dsiSaveGetLength);
		setBL(0x0201416C, (u32)dsiSaveRead);
		setBL(0x0201419C, (u32)dsiSaveClose);
		setBL(0x02014214, (u32)dsiSaveOpen);
		setBL(0x02014240, (u32)dsiSaveSetLength);
		setBL(0x02014264, (u32)dsiSaveWrite);
		setBL(0x02014280, (u32)dsiSaveClose);
		*(u32*)0x020142B8 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x020142C4, (u32)dsiSaveCreate);
		*(u32*)0x02015334 = 0xE1A00000; // nop
		tonccpy((u32*)0x02016E10, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02019CEC = 0xE1A00000; // nop
		*(u32*)0x0201E0B0 = 0xE1A00000; // nop
		*(u32*)0x0201FF90 = 0xE1A00000; // nop
		*(u32*)0x0201FF94 = 0xE1A00000; // nop
		*(u32*)0x0201FFA0 = 0xE1A00000; // nop
		*(u32*)0x02020100 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0202015C, heapEnd); // mov r0, #0x23C0000
		*(u32*)0x02020290 -= 0x30000;
		*(u32*)0x020213C0 = 0xE8BD8038; // LDMFD SP!, {R3-R5,PC}
		*(u32*)0x02024BBC = 0xE1A00000; // nop
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02081270 = 0xE1A00000; // nop
			*(u32*)0x02081278 = 0xE1A00000; // nop
			*(u32*)0x02081324 = 0xE1A00000; // nop
			*(u32*)0x02081338 = 0xE1A00000; // nop
			*(u32*)0x02081340 = 0xE1A00000; // nop
			*(u32*)0x02081418 = 0xE1A00000; // nop
			*(u32*)0x0208142C = 0xE1A00000; // nop
			*(u32*)0x02081434 = 0xE1A00000; // nop
			*(u32*)0x02081620 = 0xE1A00000; // nop
			*(u32*)0x02081628 = 0xE1A00000; // nop
			*(u32*)0x020816C8 = 0xE1A00000; // nop
			*(u32*)0x020816DC = 0xE1A00000; // nop
			*(u32*)0x020816E4 = 0xE1A00000; // nop
			*(u32*)0x02081770 = 0xE1A00000; // nop
			*(u32*)0x02081774 = 0xE1A00000; // nop
			*(u32*)0x02081790 = 0xE1A00000; // nop
			*(u32*)0x020817A4 = 0xE1A00000; // nop
			*(u32*)0x020817AC = 0xE1A00000; // nop
			*(u32*)0x0208180C = 0xE1A00000; // nop
			*(u32*)0x02081814 = 0xE1A00000; // nop
			*(u32*)0x02081850 = 0xE1A00000; // nop
			*(u32*)0x02081864 = 0xE1A00000; // nop
			*(u32*)0x0208186C = 0xE1A00000; // nop
			*(u32*)0x02081990 = 0xE1A00000; // nop
			*(u32*)0x02081998 = 0xE1A00000; // nop
			*(u32*)0x02081AF0 = 0xE1A00000; // nop
			*(u32*)0x02081B04 = 0xE1A00000; // nop
			*(u32*)0x02081B0C = 0xE1A00000; // nop
			*(u32*)0x02081B84 = 0xE1A00000; // nop
			*(u32*)0x02081B8C = 0xE1A00000; // nop
			*(u32*)0x02081D3C = 0xE1A00000; // nop
			*(u32*)0x02081D50 = 0xE1A00000; // nop
			*(u32*)0x02081D58 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020812D8 = 0xE1A00000; // nop
			*(u32*)0x020812E0 = 0xE1A00000; // nop
			*(u32*)0x0208138C = 0xE1A00000; // nop
			*(u32*)0x020813A0 = 0xE1A00000; // nop
			*(u32*)0x020813A8 = 0xE1A00000; // nop
			*(u32*)0x02081480 = 0xE1A00000; // nop
			*(u32*)0x02081494 = 0xE1A00000; // nop
			*(u32*)0x0208149C = 0xE1A00000; // nop
			*(u32*)0x02081688 = 0xE1A00000; // nop
			*(u32*)0x02081690 = 0xE1A00000; // nop
			*(u32*)0x02081730 = 0xE1A00000; // nop
			*(u32*)0x02081744 = 0xE1A00000; // nop
			*(u32*)0x0208174C = 0xE1A00000; // nop
			*(u32*)0x020817D8 = 0xE1A00000; // nop
			*(u32*)0x020817E0 = 0xE1A00000; // nop
			*(u32*)0x020817F8 = 0xE1A00000; // nop
			*(u32*)0x0208180C = 0xE1A00000; // nop
			*(u32*)0x02081814 = 0xE1A00000; // nop
			*(u32*)0x02081874 = 0xE1A00000; // nop
			*(u32*)0x0208187C = 0xE1A00000; // nop
			*(u32*)0x020818B8 = 0xE1A00000; // nop
			*(u32*)0x020818CC = 0xE1A00000; // nop
			*(u32*)0x020818D4 = 0xE1A00000; // nop
			*(u32*)0x020819F8 = 0xE1A00000; // nop
			*(u32*)0x02081A00 = 0xE1A00000; // nop
			*(u32*)0x02081B58 = 0xE1A00000; // nop
			*(u32*)0x02081B6C = 0xE1A00000; // nop
			*(u32*)0x02081B74 = 0xE1A00000; // nop
			*(u32*)0x02081BEC = 0xE1A00000; // nop
			*(u32*)0x02081BF4 = 0xE1A00000; // nop
			*(u32*)0x02081DA4 = 0xE1A00000; // nop
			*(u32*)0x02081DB8 = 0xE1A00000; // nop
			*(u32*)0x02081DC0 = 0xE1A00000; // nop
		}
	}
}

void patchBinary(cardengineArm9* ce9, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	if (ndsHeader->unitCode == 3) {
		patchDSiModeToDSMode(ce9, ndsHeader);
		return;
	}

	const char* romTid = getRomTid(ndsHeader);

	const u32* dsiSaveGetResultCode = ce9->patches->dsiSaveGetResultCode;
	const u32* dsiSaveCreate = ce9->patches->dsiSaveCreate;
	const u32* dsiSaveDelete = ce9->patches->dsiSaveDelete;
	const u32* dsiSaveGetInfo = ce9->patches->dsiSaveGetInfo;
	const u32* dsiSaveSetLength = ce9->patches->dsiSaveSetLength;
	const u32* dsiSaveOpen = ce9->patches->dsiSaveOpen;
	const u32* dsiSaveOpenR = ce9->patches->dsiSaveOpenR;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveGetLength = ce9->patches->dsiSaveGetLength;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;
	const u32* dsiSaveWrite = ce9->patches->dsiSaveWrite;

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
		*(u32*)0x0202D25C = 0xEB00007C; // bl 0x0202D454 (Skip Manual screen)
		//*(u32*)0x0202D2EC = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0202D314 = 0xE3A00000; // mov r0, #0
		setBL(0x0203A248, (u32)dsiSaveOpen);
		setBL(0x0203A26C, (u32)dsiSaveClose);
		*(u32*)0x0203A288 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203A2B8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203A2DC = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203A2FC, (u32)dsiSaveCreate);
		setBL(0x0203A30C, (u32)dsiSaveOpen);
		setBL(0x0203A338, (u32)dsiSaveWrite);
		setBL(0x0203A350, (u32)dsiSaveClose);
		setBL(0x0203A39C, (u32)dsiSaveOpen);
		setBL(0x0203A3C4, (u32)dsiSaveRead);
		setBL(0x0203A3F0, (u32)dsiSaveClose);
		setBL(0x0203A4E0, (u32)dsiSaveOpen);
		setBL(0x0203A508, (u32)dsiSaveWrite);
		setBL(0x0203A524, (u32)dsiSaveClose);
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
		*(u32*)0x0202D288 = 0xEB00007C; // bl 0x0202D480 (Skip Manual screen)
		setBL(0x0203A2D8, (u32)dsiSaveOpen);
		setBL(0x0203A2FC, (u32)dsiSaveClose);
		*(u32*)0x0203A318 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203A348 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203A36C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203A38C, (u32)dsiSaveCreate);
		setBL(0x0203A39C, (u32)dsiSaveOpen);
		setBL(0x0203A3C8, (u32)dsiSaveWrite);
		setBL(0x0203A3E0, (u32)dsiSaveClose);
		setBL(0x0203A42C, (u32)dsiSaveOpen);
		setBL(0x0203A454, (u32)dsiSaveRead);
		setBL(0x0203A480, (u32)dsiSaveClose);
		setBL(0x0203A570, (u32)dsiSaveOpen);
		setBL(0x0203A598, (u32)dsiSaveWrite);
		setBL(0x0203A5B4, (u32)dsiSaveClose);
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
		*(u32*)0x0202E2AC = 0xEB000071; // bl 0x0202E478 (Skip Manual screen)
		setBL(0x0203B108, (u32)dsiSaveOpen);
		setBL(0x0203B12C, (u32)dsiSaveClose);
		*(u32*)0x0203B148 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203B170 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203B194 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203B1B0, (u32)dsiSaveCreate);
		setBL(0x0203B1C0, (u32)dsiSaveOpen);
		setBL(0x0203B1EC, (u32)dsiSaveWrite);
		setBL(0x0203B208, (u32)dsiSaveClose);
		setBL(0x0203B250, (u32)dsiSaveOpen);
		setBL(0x0203B27C, (u32)dsiSaveRead);
		setBL(0x0203B2AC, (u32)dsiSaveClose);
		setBL(0x0203B398, (u32)dsiSaveOpen);
		setBL(0x0203B3C4, (u32)dsiSaveWrite);
		setBL(0x0203B3E0, (u32)dsiSaveClose);
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
