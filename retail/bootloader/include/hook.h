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

#ifndef HOOK_H
#define HOOK_H

//#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "my_fat.h"
#include "module_params.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"

/*-------------------------------------------------------------------------
Adds a hook in the game's ARM7 binary to our own code
-------------------------------------------------------------------------*/
int hookNdsRetailArm7(
	cardengineArm7* ce7,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 cheatFileCluster,
	u32 cheatSize,
	u32 apPatchFileCluster,
	u32 apPatchSize,
	s32 mainScreen,
	u32 language,
	u8 RumblePakType
);
int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 bootNdsCluster,
	u32 fileCluster,
	u32 saveCluster,
	u32 saveSize,
	u32 romFatTableCache,
	u32 savFatTableCache,
	bool romFatTableCompressed,
	bool savFatTableCompressed,
    u32 patchOffsetCacheFileCluster,
    u32 musicFatTableCache,
	u32 ramDumpCluster,
	u32 srParamsFileCluster,
	u32 screenshotCluster,
	u32 apFixOverlaysCluster,
	u32 musicCluster,
	u32 musicsSize,
	u32 pageFileCluster,
	u32 manualCluster,
	u32 sharedFontCluster,
	bool expansionPakFound,
	bool extendedMemory,
	bool ROMinRAM,
	bool dsDebugRam,
	u8 enableExceptionHandler,
	s32 mainScreen,
	u32 overlaysSize,
	u32 ioverlaysSize,
	u32 maxClusterCacheSize,
    u32 fatTableAddr
);

#endif // HOOK_H
