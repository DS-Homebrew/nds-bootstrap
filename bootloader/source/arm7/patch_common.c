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
#include "patch.h"
#include "common.h"
#include "debug_file.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"

extern bool logging;

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

	bool sdk5 = (moduleParams->sdk_version > 0x5000000);
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
