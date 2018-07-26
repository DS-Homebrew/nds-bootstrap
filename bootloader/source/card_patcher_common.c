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
#include "card_patcher.h"
#include "common.h"
#include "debugToFile.h"

extern bool cardReadFound; // card_patcher_arm9.c

#define LOG

u32 patchCardNds(
	const tNDSHeader* ndsHeader,
	u32* cardEngineLocationArm7,
	u32* cardEngineLocationArm9,
	const module_params_t* moduleParams, 
	u32 saveFileCluster,
	u32 saveSize,
	u32 patchMpuRegion,
	u32 patchMpuSize) {
	#ifdef LOG
	aFile myDebugFile = getBootFileCluster("NDSBTSRP2.LOG", 3);
	enableDebug(myDebugFile);
	#endif

	dbg_printf("patchCardNds\n\n");

	patchCardNdsArm9(ndsHeader, cardEngineLocationArm9, moduleParams, patchMpuRegion, patchMpuSize);
	
	if (cardReadFound || ndsHeader->fatSize == 0) {
		patchCardNdsArm7(ndsHeader, cardEngineLocationArm7, moduleParams, saveFileCluster, saveSize);

		dbg_printf("ERR_NONE");
		return ERR_NONE;
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}
