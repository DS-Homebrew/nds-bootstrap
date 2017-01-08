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
static aFile _debugFileCluster;
static u32 _currentPos = 0;
static char hexbuffer [9];

void enableDebug(aFile debugFileCluster) {	
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

char* tohex(u32 n)
{
    unsigned size = 9;
    char *buffer = hexbuffer;
    unsigned index = size - 2;

	for (int i=0; i<size; i++) {
		buffer[i] = '0';
	}
	
    while (n > 0)
    {
        unsigned mod = n % 16;

        if (mod >= 10)
            buffer[index--] = (mod - 10) + 'A';
        else
            buffer[index--] = mod + '0';

        n /= 16;
    }
    buffer[size - 1] = '\0';
    return buffer;
}

u32 dbg_hexa(u32 n) {
	return dbg_printf(tohex(n));
}
