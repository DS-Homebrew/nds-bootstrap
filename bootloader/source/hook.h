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

#include <nds/memory.h>
#include <nds/ndstypes.h>
#include "fat.h"

/*-------------------------------------------------------------------------
arm7_hookGame
Adds a hook in the game's ARM7 binary to our own code
-------------------------------------------------------------------------*/
int hookNdsRetail (const tNDSHeader* ndsHeader, aFile file, const u32* cheatData, u32* cheatEngineLocation, u32* cardEngineLocation);
void hookNdsRetail_ROMinRAM (u32* cardEngineLocation9, u32 ROMinRAM, u32 cleanRomSize);
