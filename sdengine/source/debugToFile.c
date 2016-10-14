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
#include "fat.h"

static bool _debug = false;
static u32 _debugFileCluster = 0;
static u32 _currentPos = 0;

void enableDebug(u32 debugFileCluster) {	
	_debug = true;
	_debugFileCluster = debugFileCluster;
}

u32 dbg_printf( char * message)
{
	nocashMessage(message);
	
	if(!_debug) return 0;	
	
	u32 ret = fileWrite (message, _debugFileCluster, _currentPos,  strlen(message));
	
	_currentPos+=strlen(message);
	
	return ret;
}
