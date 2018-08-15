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
#include "module_params.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"
#include "patch.h"
#include "common.h"
#include "debug_file.h"

extern bool logging;

void patchBinary(const tNDSHeader* ndsHeader) {
	u32 ROM_TID = *(u32*)ndsHeader->gameCode;

	// The World Ends With You (USA) (Europe)
	if (ROM_TID == 0x454C5741 || ROM_TID == 0x504C5741) {
		*(u32*)0x203E7B0 = 0;
	}

	// Subarashiki Kono Sekai - It's a Wonderful World (Japan)
	if (ROM_TID == 0x4A4C5741) {
		*(u32*)0x203F114 = 0;
	}

	// Miami Nights - Singles in the City (USA)
	if (ROM_TID == 0x45575641) {
		// Fix not enough memory error
		*(u32*)0x0204CCCC = 0xe1a00000; //nop
	}

	// Miami Nights - Singles in the City (Europe)
	if (ROM_TID == 0x50575641) {
		// Fix not enough memory error
		*(u32*)0x0204CDBC = 0xe1a00000; //nop
	}
	
	// 0735 - Castlevania - Portrait of Ruin (USA)
	if (ROM_TID == 0x45424341) {
		*(u32*)0x02007910 = 0xeb02508e;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025052;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0676 - Akumajou Dracula - Gallery of Labyrinth (Japan)
	if (ROM_TID == 0x4a424341) {
		*(u32*)0x02007910 = 0xeb0250b0;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025074;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0881 - Castlevania - Portrait of Ruin (Europe) (En,Fr,De,Es,It)
	if (ROM_TID == 0x50424341) {
		*(u32*)0x02007b00 = 0xeb025370;
		*(u32*)0x02007b08 = 0xea000004;
		*(u32*)0x02007bf0 = 0xeb025334;
		*(u32*)0x02007bf8 = 0xe59f1030;
		*(u32*)0x02007bfc = 0xe59f0028;
		*(u32*)0x02007c00 = 0xe0281097;
		*(u32*)0x02007c04 = 0xea000003;
	}

	// Chrono Trigger (Japan)
	if (ROM_TID == 0x4a555159) {
		*(u32*)0x0204e364 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e368 = 0xe12fff1e; //bx lr
		*(u32*)0x0204e6c4 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e6c8 = 0xe12fff1e; //bx lr
	}

	// Chrono Trigger (USA/Europe)
	if (ROM_TID == 0x45555159 || ROM_TID == 0x50555159) {
		*(u32*)0x0204e334 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e338 = 0xe12fff1e; //bx lr
		*(u32*)0x0204e694 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e698 = 0xe12fff1e; //bx lr
	}
	
	// Dementium II (USA/EUR)
	if (ROM_TID == 0x45454442 || ROM_TID == 0x50454442) {
		*(u32*)0x020e9120 = 0xe3a00002;
		*(u32*)0x020e9124 = 0xea000029;
	}
	
	// Dementium II: Tozasareta Byoutou (JPN)
	if (ROM_TID == 0x4a454442) {
		*(u32*)0x020d9f60 = 0xe3a00005;
		*(u32*)0x020d9f68 = 0xea000029;
	}

	// Grand Theft Auto - Chinatown Wars (USA) (En,Fr,De,Es,It)
	// Grand Theft Auto - Chinatown Wars (Europe) (En,Fr,De,Es,It)
	if (ROM_TID == 0x45584759 || ROM_TID == 0x50584759) {
		*(u16*)0x02037a34 = 0x46c0;
		*(u32*)0x0216ac0c = 0x0001fffb;
	}

	// WarioWare: DIY (USA)
	if (ROM_TID == 0x45524F55) {
		*(u32*)0x02003114 = 0xE12FFF1E; //mov r0, #0
	}
}

u32 patchCardNds(
	const tNDSHeader* ndsHeader,
	cardengineArm7* ce7,
	cardengineArm9* ce9,
	const module_params_t* moduleParams, 
	u32 saveFileCluster,
	u32 saveSize,
	u32 patchMpuRegion,
	u32 patchMpuSize) {
	if (logging) {
		enableDebug(getBootFileCluster("NDSBTSRP.LOG", 3));
	}

	dbg_printf("patchCardNds\n\n");

	bool sdk5 = isSdk5(moduleParams);
	if (sdk5) {
		dbg_printf("[SDK 5]\n\n");
	}

	patchCardNdsArm9(ndsHeader, ce9, moduleParams, patchMpuRegion, patchMpuSize);
	
	if (cardReadFound || ndsHeader->fatSize == 0) {
		patchCardNdsArm7(ndsHeader, ce7, moduleParams, saveFileCluster, saveSize);

		dbg_printf("ERR_NONE");
		return ERR_NONE;
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}
