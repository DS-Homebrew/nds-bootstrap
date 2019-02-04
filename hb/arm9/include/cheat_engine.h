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

#ifndef CHEAT_ENGINE_H
#define CHEAT_ENGINE_H

#include <nds/ndstypes.h>
#define CHEAT_MAX_DATA_SIZE (64 * 1024)			// 64KiB

#ifdef __cplusplus
extern "C" {
#endif


void runCheatEngine (void* cheats, int cheatLength);


#ifdef __cplusplus
}
#endif

#endif // CHEAT_ENGINE_H
