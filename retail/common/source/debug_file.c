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

#include <string.h>
#include <nds/debug.h>
#include "my_fat.h"
#include "hex.h"

static bool _debug = false;
static aFile _debugFileCluster;
static u32 _currentPos = 0;

void enableDebug(const aFile* debugFileCluster) {
	_debug = true;
	_debugFileCluster = *debugFileCluster;
}

u32 dbg_printf(const char* message) {
	if(!_debug) {
        return 0;
    }

	#ifndef B4DS
	nocashMessage(message);
	#endif

	u32 ret = fileWrite(message, &_debugFileCluster, _currentPos, strlen(message));

	_currentPos += strlen(message);

	return ret;
}

u32 dbg_hexa(u32 n) {
	return dbg_printf(tohex(n));
}
