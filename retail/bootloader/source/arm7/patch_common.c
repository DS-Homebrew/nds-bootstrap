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

u16 patchOffsetCacheFileVersion = 22;	// Change when new functions are being patched, some offsets removed
										// the offset order changed, and/or the function signatures changed

patchOffsetCacheContents patchOffsetCache;

bool patchOffsetCacheChanged = false;

void patchDSiModeToDSMode(const tNDSHeader* ndsHeader) {
	extern bool expansionPakFound;
	//extern u32 generateA7Instr(int arg1, int arg2);
	const char* romTid = getRomTid(ndsHeader);

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
		*(u32*)0x0200F1A4 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0200F1C8 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0200F1D0 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0200F1A4 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0200EF70 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0200EF78 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x02019BBC = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x02019BE0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019BE8 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201E6CC = 0xE1A00000; // nop
	}

	// Patch DSiWare to run in DS mode

	// GO Series: 10 Second Run (USA)
	// Crashes on the title screen
	else if (strcmp(romTid, "KJUE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02005888 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x02019414 = 0xE1A00000; // nop
		*(u32*)0x0201942C = 0xE1A00000; // nop
		*(u32*)0x02030A88 = 0xE1A00000; // nop
		*(u32*)0x02034224 = 0xE1A00000; // nop
		*(u32*)0x02037F24 = 0xE1A00000; // nop
		*(u32*)0x02039CCC = 0xE1A00000; // nop
		*(u32*)0x02039CD0 = 0xE1A00000; // nop
		*(u32*)0x02039CDC = 0xE1A00000; // nop
		*(u32*)0x02039E3C = 0xE1A00000; // nop
		*(u32*)0x02039E98 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02039EBC = 0xE3500001; // cmp r0, #1
		*(u32*)0x02039EC4 = 0x13A00627; // movne r0, #0x2700000
		//*(u32*)0x0203B77C = 0xE12FFF1E; // bx lr
		/* *(u32*)0x0203B7D4 = 0xE1A00000; // nop
		*(u32*)0x0203B7D8 = 0xE1A00000; // nop
		*(u32*)0x0203B7DC = 0xE1A00000; // nop
		*(u32*)0x0203B7E0 = 0xE1A00000; // nop */
		*(u32*)0x0203E7D0 = 0xE1A00000; // nop
	}

	// Ace Mathician (USA)
	else if (strcmp(romTid, "KQKE") == 0) {
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
		*(u32*)0x020383AC = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020383D0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020383D8 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x02039B74 = 0xE1A00000; // nop
		*(u32*)0x02039B78 = 0xE1A00000; // nop
		*(u32*)0x02039B7C = 0xE1A00000; // nop
		*(u32*)0x02039B80 = 0xE1A00000; // nop
		*(u32*)0x0203C5DC = 0xE1A00000; // nop
	}

	// Ace Mathician (Europe, Australia)
	else if (strcmp(romTid, "KQKV") == 0) {
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
		*(u32*)0x020383BC = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020383E0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020383E8 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x02039B84 = 0xE1A00000; // nop
		*(u32*)0x02039B88 = 0xE1A00000; // nop
		*(u32*)0x02039B8C = 0xE1A00000; // nop
		*(u32*)0x02039B90 = 0xE1A00000; // nop
		*(u32*)0x0203C5EC = 0xE1A00000; // nop
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
		*(u32*)0x020587F4 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x02058818 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02058820 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0205D21C = 0xE1A00000; // nop
	}*/

	// G.G. Series: All Breaker (USA)
	// G.G. Series: All Breaker (Japan)
	else if ((strcmp(romTid, "K27E") == 0 || strcmp(romTid, "K27J") == 0) && extendedMemory2) {
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200D71C = 0xE1A00000; // nop
		*(u32*)0x0204E880 = 0xE1A00000; // nop
		*(u32*)0x02052814 = 0xE1A00000; // nop
		*(u32*)0x02058BE8 = 0xE1A00000; // nop
		*(u32*)0x0205AA84 = 0xE1A00000; // nop
		*(u32*)0x0205AA88 = 0xE1A00000; // nop
		*(u32*)0x0205AA94 = 0xE1A00000; // nop
		*(u32*)0x0205ABF4 = 0xE1A00000; // nop
		*(u32*)0x0205AC50 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0205AC74 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0205AC7C = 0x13A00627; // movne r0, #0x2700000
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

	// Antipole (USA)
	// Does not boot due to lack of memory
	/*else if (strcmp(romTid, "KJHE") == 0) {
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
	// Audio doesn't play
	// Pressing A to exit options will cause an error
	else if (strcmp(romTid, "KAAE") == 0) {
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		*(u32*)0x0203BB4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0203BC18 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x02064C04 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02064C28 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02064C30 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x02066054 = 0xE12FFF1E; // bx lr
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
	else if (strcmp(romTid, "KABE") == 0 && extendedMemory2) {
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200D83C = 0xE1A00000; // nop
		*(u32*)0x0204F59C = 0xE1A00000; // nop
		*(u32*)0x02053530 = 0xE1A00000; // nop
		*(u32*)0x02059980 = 0xE1A00000; // nop
		*(u32*)0x0205B81C = 0xE1A00000; // nop
		*(u32*)0x0205B820 = 0xE1A00000; // nop
		*(u32*)0x0205B82C = 0xE1A00000; // nop
		*(u32*)0x0205B98C = 0xE1A00000; // nop
		*(u32*)0x0205B9E8 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0205BA0C = 0xE3500001; // cmp r0, #1
		*(u32*)0x0205BA14 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020607FC = 0xE1A00000; // nop
	}

	// G.G. Series: Assault Buster (Japan)
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
		*(u32*)0x0204E890 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0204E8B4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0204E8BC = 0x13A00627; // movne r0, #0x2700000
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
		/*if (extendedMemory2) {
			*(u32*)0x0204B448 = 0xE3A0079F; // mov r0, #0x27C0000
		} else {
			*(u32*)0x0204B448 = 0xE3A0078F; // mov r0, #0x23C0000
		}*/
		*(u32*)0x0204B448 = 0xE59F0094; // ldr r0,=0x02??0000
		*(u32*)0x0204B46C = 0xE3500001; // cmp r0, #1
		*(u32*)0x0204B474 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0204B488 = 0xE3A01000; // mov r1, #0
		if (extendedMemory2) {
			*(u32*)0x0204B4E4 = 0x02700000;
		} else {
			//*(u32*)0x0204B4E4 = 0x023E0000;
			*(u32*)0x0204B4E4 = expansionPakFound ? CARDENGINE_ARM9_LOCATION_DLDI : 0x023C0000;
		}
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
		/*if (extendedMemory2) {
			*(u32*)0x0204B4C8 = 0xE3A0079F; // mov r0, #0x27C0000
		} else {
			*(u32*)0x0204B4C8 = 0xE3A0078F; // mov r0, #0x23C0000
		}*/
		*(u32*)0x0204B4C8 = 0xE59F0094; // ldr r0,=0x02??0000
		*(u32*)0x0204B4EC = 0xE3500001; // cmp r0, #1
		*(u32*)0x0204B4F4 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0204B508 = 0xE3A01000; // mov r1, #0
		if (extendedMemory2) {
			*(u32*)0x0204B564 = 0x02700000;
		} else {
			//*(u32*)0x0204B564 = 0x023E0000;
			*(u32*)0x0204B564 = expansionPakFound ? CARDENGINE_ARM9_LOCATION_DLDI : 0x023C0000;
		}
	}

	// Big Bass Arcade (USA)
	// Locks up on the first shown logos
	/*else if (strcmp(romTid, "K9GE") == 0) {
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
		//*(u32*)0x020777EC = 0xE3A0078F; // mov r0, #0x23C0000
		//*(u32*)0x02077810 = 0xE3500001; // cmp r0, #1
		//*(u32*)0x02077818 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0207BE88 = 0xE1A00000; // nop
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
		*(u32*)0x0204F0C8 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0204F0EC = 0xE3500001; // cmp r0, #1
		*(u32*)0x0204F0F4 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200DDB4 = 0xE1A00000; // nop
		*(u32*)0x02011580 = 0xE1A00000; // nop
		*(u32*)0x0201BCD4 = 0xE1A00000; // nop
		*(u32*)0x0201DB50 = 0xE1A00000; // nop
		*(u32*)0x0201DB54 = 0xE1A00000; // nop
		*(u32*)0x0201DB60 = 0xE1A00000; // nop
		*(u32*)0x0201DCC0 = 0xE1A00000; // nop
		*(u32*)0x0201DD1C = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201DD40 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201DD48 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201EFDC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EFE8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022418 = 0xE1A00000; // nop
	}

	// GO Series: Defense Wars (USA)
	else if (strcmp(romTid, "KWTE") == 0) {
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
		*(u32*)0x020557A0 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020557C4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020557CC = 0x13A00627; // movne r0, #0x2700000
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
		if (extendedMemory2) {
			*(u32*)0x02038A18 = 0xE3A00627; // mov r0, #0x2700000
		} else {
			*(u32*)0x02038A18 = 0xE3A0079F; // mov r0, #0x27C0000 (mirrors to 0x23C0000 on retail units)
		}
		*(u32*)0x02038A3C = 0xE3500001; // cmp r0, #1
		*(u32*)0x02038A44 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0203A068 = 0xE12FFF1E; // bx lr (Not needed on NO$GBA)
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
		if (extendedMemory2) {
			*(u32*)0x02038A0C = 0xE3A00627; // mov r0, #0x2700000
		} else {
			*(u32*)0x02038A0C = 0xE3A0079F; // mov r0, #0x27C0000 (mirrors to 0x23C0000 on retail units)
		}
		*(u32*)0x02038A30 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02038A38 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0203A05C = 0xE12FFF1E; // bx lr (Not needed on NO$GBA)
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
		if (extendedMemory2) {
			*(u32*)0x0203CA8C = 0xE3A00627; // mov r0, #0x2700000
		} else {
			*(u32*)0x0203CA8C = 0xE3A0079F; // mov r0, #0x27C0000 (mirrors to 0x23C0000 on retail units)
		}
		*(u32*)0x0203CAB0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0203CAB8 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0203E0D4 = 0xE12FFF1E; // bx lr (Not needed on NO$GBA)
		*(u32*)0x02041040 = 0xE1A00000; // nop
	}

	// Dragon's Lair II: Time Warp (Europe, Australia)
	// Crashes on company logos (Cause unknown)
	else if (strcmp(romTid, "KLYV") == 0) {
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
		if (extendedMemory2) {
			*(u32*)0x0203CBB4 = 0xE3A00627; // mov r0, #0x2700000
		} else {
			*(u32*)0x0203CBB4 = 0xE3A0079F; // mov r0, #0x27C0000 (mirrors to 0x23C0000 on retail units)
		}
		*(u32*)0x0203CBD8 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0203CBE0 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0203E1FC = 0xE12FFF1E; // bx lr (Not needed on NO$GBA)
		*(u32*)0x02041168 = 0xE1A00000; // nop
	}

	// GO Series: Earth Saver (USA)
	// Extra fixes required for it to boot on real hardware
	else if (strcmp(romTid, "KB8E") == 0) {
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x02005534 = 0xE1A00000; // nop
		*(u32*)0x0200A898 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200B690 = 0xE1A00000; // nop
		*(u32*)0x0200B694 = 0xE1A00000; // nop
		*(u32*)0x0200B6A0 = 0xE1A00000; // nop
		*(u32*)0x0200B6B8 = 0xE1A00000; // nop
		*(u32*)0x0200B6D0 = 0xE1A00000; // nop
		*(u32*)0x0200B6E0 = 0xE1A00000; // nop
		*(u32*)0x02036398 = 0xE1A00000; // nop
		*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0204BFE8 = 0xE1A00000; // nop
		*(u32*)0x0204F548 = 0xE1A00000; // nop
		*(u32*)0x02054440 = 0xE1A00000; // nop
		*(u32*)0x02056228 = 0xE1A00000; // nop
		*(u32*)0x0205622C = 0xE1A00000; // nop
		*(u32*)0x02056238 = 0xE1A00000; // nop
		*(u32*)0x020563F4 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02056418 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02056420 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02015BC4 = 0xE1A00000; // nop
		*(u32*)0x020197B8 = 0xE1A00000; // nop
		*(u32*)0x0201F670 = 0xE1A00000; // nop
		*(u32*)0x020216B8 = 0xE1A00000; // nop
		*(u32*)0x020216BC = 0xE1A00000; // nop
		*(u32*)0x020216C8 = 0xE1A00000; // nop
		*(u32*)0x02021828 = 0xE1A00000; // nop
		*(u32*)0x02021884 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020218A8 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020218B0 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020219B8 = 0x02257500;
		*(u32*)0x02026A94 = 0xE1A00000; // nop
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
		*(u32*)0x0201985C = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019880 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019888 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x020196B0 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020196D4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020196DC = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201B554 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201B578 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201B580 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201B3A8 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201B3CC = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201B3D4 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x02019AD8 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019AFC = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019B04 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201992C = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019950 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019958 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x02019CA8 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019CCC = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019CD4 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x02019AFC = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019B20 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019B28 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201985C = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019880 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019888 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x020196B0 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020196D4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020196DC = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201B5E0 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201B604 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201B60C = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201B434 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201B458 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201B460 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x0201985C = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019880 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02019888 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x020196B0 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020196D4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020196DC = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201E07C = 0xE1A00000; // nop
		*(u32*)0x0202CE74 = 0xE12FFF1E; // bx lr
	}

	// Glory Days: Tactical Defense (USA)
	else if (strcmp(romTid, "KGKE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x0200B488 = 0xE1A00000; // nop
		*(u32*)0x02017128 = 0xE1A00000; // nop
		*(u32*)0x02018F94 = 0xE1A00000; // nop
		*(u32*)0x02018F98 = 0xE1A00000; // nop
		*(u32*)0x02018FA4 = 0xE1A00000; // nop
		*(u32*)0x02019104 = 0xE1A00000; // nop
		*(u32*)0x02019160 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019184 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201918C = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201D83C = 0xE1A00000; // nop
		for (int i = 0; i < 12; i++) {
			u32* offset = (u32*)0x0206710C;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x020671B4 = 0xE1A00000; // nop
		for (int i = 0; i < 10; i++) {
			u32* offset = (u32*)0x02075514;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Glory Days: Tactical Defense (Europe)
	else if (strcmp(romTid, "KGKP") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		//*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x0200B488 = 0xE1A00000; // nop
		*(u32*)0x02017128 = 0xE1A00000; // nop
		*(u32*)0x02018F94 = 0xE1A00000; // nop
		*(u32*)0x02018F98 = 0xE1A00000; // nop
		*(u32*)0x02018FA4 = 0xE1A00000; // nop
		*(u32*)0x02019104 = 0xE1A00000; // nop
		*(u32*)0x02019160 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02019184 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201918C = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201D83C = 0xE1A00000; // nop
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

	// The Legend of Zelda: Four Swords: Anniversary Edition (USA)
	// Does not boot (Lack of memory?)
	/*else if (strcmp(romTid, "KQ9E") == 0) {
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x020050F8 = 0xE1A00000; // nop
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x02012AAC = 0xE1A00000; // nop
		*(u32*)0x020166D4 = 0xE1A00000; // nop
		*(u32*)0x020185C8 = 0xE1A00000; // nop
		*(u32*)0x020185CC = 0xE1A00000; // nop
		*(u32*)0x020185D8 = 0xE1A00000; // nop
		*(u32*)0x02018738 = 0xE1A00000; // nop
		*(u32*)0x02018738 = 0xE1A00000; // nop
		*(u32*)0x0201D01C = 0xE1A00000; // nop
		*(u32*)0x0205663C = 0xE12FFF1E; // bx lr
		*(u32*)0x02056738 = 0xE12FFF1E; // bx lr
		*(u32*)0x02082A3C = 0xE1A00000; // nop
		*(u32*)0x02082A58 = 0xE1A00000; // nop
		*(u32*)0x020A467C = 0xE1A00000; // nop
	}*/

	// Mario Calculator (USA)
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
		*(u32*)0x0206F968 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0206F98C = 0xE3500001; // cmp r0, #1
		*(u32*)0x0206F994 = 0x13A00627; // movne r0, #0x2700000
	}*/

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0) {
		*(u32*)0x0204D3C4 = 0xE1A00000; // nop
		*(u32*)0x02051124 = 0xE1A00000; // nop
		*(u32*)0x020566E8 = 0xE1A00000; // nop
		*(u32*)0x020585BC = 0xE1A00000; // nop
		*(u32*)0x020585C0 = 0xE1A00000; // nop
		*(u32*)0x020585CC = 0xE1A00000; // nop
		*(u32*)0x02058710 = 0xE1A00000; // nop
		*(u32*)0x0205876C = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02058790 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02058798 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0205C93C = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Europe, Australia)
	else if (strcmp(romTid, "KMGV") == 0) {
		*(u32*)0x0204D504 = 0xE1A00000; // nop
		*(u32*)0x02050F30 = 0xE1A00000; // nop
		*(u32*)0x020564F4 = 0xE1A00000; // nop
		*(u32*)0x020583E4 = 0xE1A00000; // nop
		*(u32*)0x020583E8 = 0xE1A00000; // nop
		*(u32*)0x020583F4 = 0xE1A00000; // nop
		*(u32*)0x02058538 = 0xE1A00000; // nop
		*(u32*)0x02058594 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x020585B8 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020585C0 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0205C88C = 0xE1A00000; // nop
	}

	// Mighty Milky Way (USA)
	// Mighty Milky Way (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KWYE") == 0 || strcmp(romTid, "KWYP") == 0) && extendedMemory2) {
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x0200545C = 0xE1A00000; // nop
		*(u32*)0x020054B0 = 0xE1A00000; // nop
		*(u32*)0x020054C8 = 0xE1A00000; // nop
		*(u32*)0x020054CC = 0xE1A00000; // nop
		*(u32*)0x020054D0 = 0xE1A00000; // nop
		*(u32*)0x020054D4 = 0xE1A00000; // nop
		*(u32*)0x020054DC = 0xE1A00000; // nop
		*(u32*)0x0200554C = 0xE1A00000; // nop
		*(u32*)0x020057C0 = 0xE1A00000; // nop
		*(u32*)0x02005A20 = 0xE1A00000; // nop
		*(u32*)0x02005A28 = 0xE1A00000; // nop
		*(u32*)0x02005A2C = 0xE1A00000; // nop
		*(u32*)0x02005A20 = 0xE1A00000; // nop
		*(u32*)0x02005A38 = 0xE1A00000; // nop
		*(u32*)0x02064E34 = 0xE1A00000; // nop
		*(u32*)0x0206CCE0 = 0xE1A00000; // nop
		*(u32*)0x0206EB6C = 0xE1A00000; // nop
		*(u32*)0x0206EB70 = 0xE1A00000; // nop
		*(u32*)0x0206EB7C = 0xE1A00000; // nop
		*(u32*)0x0206ECDC = 0xE1A00000; // nop
		*(u32*)0x0206ED38 = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0206ED5C = 0xE3500001; // cmp r0, #1
		*(u32*)0x0206ED64 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x02070384 = 0xE1A00000; // nop
		*(u32*)0x02070388 = 0xE1A00000; // nop
		*(u32*)0x0207038C = 0xE1A00000; // nop
		*(u32*)0x02070390 = 0xE1A00000; // nop
		*(u32*)0x0207388C = 0xE1A00000; // nop
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
		*(u32*)0x02011D54 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02011D78 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02011D80 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x02011FF8 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201201C = 0xE3500001; // cmp r0, #1
		*(u32*)0x02012024 = 0x13A00627; // movne r0, #0x2700000
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
		*(u32*)0x02011D88 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02011DAC = 0xE3500001; // cmp r0, #1
		*(u32*)0x02011DB4 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x02015DD8 = 0xE1A00000; // nop
	}

	// Nintendoji (Japan)
	// Does not boot
	/*else if (strcmp(romTid, "K9KJ") == 0) {
		*(u32*)0x0200499C = 0xE1A00000; // nop
		*(u32*)0x02005538 = 0xE1A00000; // nop
		*(u32*)0x0200554C = 0xE1A00000; // nop
		*(u32*)0x02018C08 = 0xE1A00000; // nop
		*(u32*)0x0201A9F8 = 0xE1A00000; // nop
		*(u32*)0x0201A9FC = 0xE1A00000; // nop
		*(u32*)0x0201AA08 = 0xE1A00000; // nop
		*(u32*)0x0201AB68 = 0xE1A00000; // nop
		*(u32*)0x0201ABC4 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x0201ABD0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0201ABF0 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0201E7B0 = 0xE1A00000; // nop
		*(u32*)0x0209EEB8 = 0xE1A00000; // nop
	}*/

	// Petit Computer (USA)
	// Does not boot (black screens, seems to rely on code from DSi binaries)
	/*else if (strcmp(romTid, "KNAE") == 0) {
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
		*(u32*)0x02092C40 = 0xE3A0078F; // mov r0, #0x23C0000
		*(u32*)0x02092C64 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02092C6C = 0x13A00627; // movne r0, #0x2700000
	}*/

	// Shantae: Risky's Revenge (USA)
	// Crashes after selecting a file due to memory limitations
	/*else if (strcmp(romTid, "KS3E") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x02092050 = 0xE1A00000; // nop
		*(u32*)0x02092078 = 0xE1A00000; // nop
		*(u32*)0x02092BA8 = 0xE1A00000; // nop
		*(u32*)0x02092E08 = 0xE1A00000; // nop
		*(u32*)0x02092E14 = 0xE1A00000; // nop
		*(u32*)0x02092E20 = 0xE1A00000; // nop
		*(u32*)0x020DDB84 = 0xE1A00000; // nop
		*(u32*)0x020DE420 = 0xE1A00000; // nop
		*(u32*)0x020DE548 = 0xE1A00000; // nop
		*(u32*)0x020DE55C = 0xE1A00000; // nop
		*(u32*)0x020E20C4 = 0xE1A00000; // nop
		*(u32*)0x020E616C = 0xE1A00000; // nop
		*(u32*)0x020E7F64 = 0xE1A00000; // nop
		*(u32*)0x020E7F68 = 0xE1A00000; // nop
		*(u32*)0x020E7F74 = 0xE1A00000; // nop
		*(u32*)0x020E80D4 = 0xE1A00000; // nop
		*(u32*)0x020E8130 = 0xE3A0079F; // mov r0, #0x27C0000
		*(u32*)0x020E8154 = 0xE3500001; // cmp r0, #1
		*(u32*)0x020E815C = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020E8264 = 0x02186C60;
		*(u32*)0x020E977C = 0xE1A00000; // nop
		*(u32*)0x020E9780 = 0xE1A00000; // nop
		*(u32*)0x020E9784 = 0xE1A00000; // nop
		*(u32*)0x020E9788 = 0xE1A00000; // nop
		*(u32*)0x020E9794 = 0xE1A00000; // nop (Activates a message when memory runs out)
		*(u32*)0x020ECBEC = 0xE1A00000; // nop
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
		if (extendedMemory2) {
			*(u32*)0x0203C2D4 = 0xE3A00627; // mov r0, #0x2700000
		} else {
			*(u32*)0x0203C2D4 = 0xE3A0079F; // mov r0, #0x27C0000 (mirrors to 0x23C0000 on retail units)
		}
		*(u32*)0x0203C2F8 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0203C300 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0203D91C = 0xE12FFF1E; // bx lr (Not needed on NO$GBA)
		*(u32*)0x0203DDC8 = 0xE1A00000; // nop
		*(u32*)0x0203DDCC = 0xE1A00000; // nop
		*(u32*)0x0203DDD0 = 0xE1A00000; // nop
		*(u32*)0x0203DDD4 = 0xE1A00000; // nop
		*(u32*)0x0203E9B4 = 0xE1A00000; // nop
		*(u32*)0x02040888 = 0xE1A00000; // nop
	}

	// Spotto! (USA)
	// Does not boot: Issue unknown
	/*else if (strcmp(romTid, "KSPE") == 0) {
		*(u32*)0x0200498C = 0xE1A00000; // nop
		*(u32*)0x02022AB4 = 0xE1A00000; // nop
		*(u32*)0x02026038 = 0xE1A00000; // nop
		*(u32*)0x0202C280 = 0xE1A00000; // nop
		*(u32*)0x0202E0E0 = 0xE1A00000; // nop
		*(u32*)0x0202E0E4 = 0xE1A00000; // nop
		*(u32*)0x0202E0F0 = 0xE1A00000; // nop
		*(u32*)0x0202E250 = 0xE1A00000; // nop
		*(u32*)0x0202E2AC = 0xE3A00627; // mov r0, #0x2700000
		*(u32*)0x0202E2D0 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0202E2D8 = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x02031EB4 = 0xE1A00000; // nop
		*(u32*)0x0204CA50 = 0xE1A00000; // nop
		*(u32*)0x0204CA74 = 0xE1A00000; // nop
		*(u32*)0x020558C4 = 0xE1A00000; // nop
		*(u32*)0x020558F4 = 0xE1A00000; // nop
		*(u32*)0x020558FC = 0xE1A00000; // nop
	}*/
}

void patchBinary(const tNDSHeader* ndsHeader) {
	if (ndsHeader->unitCode == 3) {
		patchDSiModeToDSMode(ndsHeader);
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
	else if (strncmp(romTid, "ADM", 3) == 0 || strncmp(romTid, "A62", 3) == 0) {
		int instancesPatched = 0;
		u32 addrOffset = (u32)ndsHeader->arm9destination;
		while (instancesPatched < 3) {
			if(*(u32*)addrOffset >= 0x023FF000 && *(u32*)addrOffset < 0x023FF020) { 
				*(u32*)addrOffset -= 0x3000;
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
}

static bool rsetA7CacheDone = false;

void rsetA7Cache(void)
{
	if (rsetA7CacheDone) return;

	patchOffsetCache.a7BinSize = 0;
	patchOffsetCache.a7IsThumb = 0;
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
	extern u32 srlAddr;

	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 1) {
		if (srlAddr == 0) pleaseWaitOutput();
		u32* moduleParamsOffset = patchOffsetCache.moduleParamsOffset;
		toncset(&patchOffsetCache, 0, sizeof(patchOffsetCacheContents));
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.type = 1;	// 0 = Regular, 1 = B4DS
		patchOffsetCache.moduleParamsOffset = moduleParamsOffset;
		rsetA7CacheDone = true;
	}

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
