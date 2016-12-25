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

#ifndef CARD_PATCHER_H
#define CARD_PATCHER_H

#include <nds/memory.h>
#include <nds/ndstypes.h>

typedef struct 
{
	u32 auto_load_list_offset;
	u32 auto_load_list_end;
	u32 auto_load_start;
	u32 static_bss_start;
	u32 static_bss_end;
	u32 compressed_static_end;
	u32 sdk_version;
	u32 nitro_code_be;
	u32 nitro_code_le;
} module_params_t;


module_params_t* findModuleParams(const tNDSHeader* ndsHeader);
void ensureArm9Decompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
/*-------------------------------------------------------------------------
arm7_hookGame
Adds a hook in the game's ARM7 binary to our own code
-------------------------------------------------------------------------*/
u32 patchCardNds (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams);

#endif // CARD_PATCHER_H