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
	u32 language,
	u32 dsiMode, // SDK5
	u32 ROMinRAM,
	u32 consoleModel,
	u32 romread_LED,
	u32 gameSoftReset
);
int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 dsiMode, // SDK5
	u32 enableExceptionHandler,
	u32 consoleModel
);

#endif // HOOK_H
