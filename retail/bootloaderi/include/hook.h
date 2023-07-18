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
	u32 fileCluster,
    u32 patchOffsetCacheFileCluster,
	u32 srParamsFileCluster,
	u32 ramDumpCluster,
	u32 screenshotCluster,
	u32 wideCheatFileCluster,
	u32 wideCheatSize,
	u32 cheatFileCluster,
	u32 cheatSize,
	u32 apPatchFileCluster,
	u32 apPatchSize,
	u32 pageFileCluster,
	u32 manualCluster,
    u16 bootstrapOnFlashcard,
    u8 gameOnFlashcard,
    u8 saveOnFlashcard,
	s32 mainScreen,
	u8 language,
	u8 dsiMode, // SDK5
	u8 dsiSD,
	u8 extendedMemory,
	u8 ROMinRAM,
	u8 consoleModel,
	u8 romRead_LED,
	u8 dmaRomRead_LED,
	bool ndmaDisabled,
	bool twlTouch,
	bool usesCloneboot
);
int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 saveCluster,
	u32 saveSize,
	u8 saveOnFlashcard,
	u32 cacheBlockSize,
	u8 extendedMemory,
	u8 ROMinRAM,
	u8 dsiMode, // SDK5
	u8 enableExceptionHandler,
	s32 mainScreen,
	u8 consoleModel,
	bool usesCloneboot
);
int hookNdsRetailArm9Mini(cardengineArm9* ce9, const tNDSHeader* ndsHeader, s32 mainScreen, u8 consoleModel);

#endif // HOOK_H
