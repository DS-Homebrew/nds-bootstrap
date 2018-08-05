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

#include <nds/memory.h>
#include <nds/ndstypes.h>
#include "fat_alt.h"

/*-------------------------------------------------------------------------
Adds a hook in the game's ARM7 binary to our own code
-------------------------------------------------------------------------*/
int hookNdsRetailArm7(const tNDSHeader* ndsHeader, aFile file, u32* cardEngineLocationArm7);
void hookNdsRetailArm9(u32* cardEngineLocationArm9);

#endif // HOOK_H
