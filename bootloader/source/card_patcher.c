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

#include "card_patcher.h"
#include "common.h"
#include "cardengine_bin.h"

// Subroutine function signatures arm7
u32 relocateStartSignature[1]  = {0x027FFFFA};
u32 a7cardReadSignature[2]     = {0x04100010,0x040001A4};
u32 a7something1Signature[2]   = {0xE350000C,0x908FF100};
u32 a7something2Signature[2]   = {0x0000A040,0x040001A0};

// Subroutine function signatures arm9
u32 compressionSignature[2]   = {0xDEC00621, 0x2106C0DE};
u32 a9cardReadSignature[2]    = {0x04100010, 0x040001A4};
u32 cardReadStartSignature[1] = {0xE92D4FF0};
u32 a9cardIdSignature[2]      = {0x040001A4,0x04100010};
u16 cardIdStartSignature[1]   = {0xE92D};
u32 a9instructionBHI[1]       = {0x8A000001};

//
// Look in @data for @find and return the position of it.
//
u32 getOffsetA9(u32* addr, size_t size, u32* find, size_t sizeofFind, int direction)
{
	u32* end = addr + size/sizeof(u32);
	u32* debug = (u32*)0x037D0000;
	debug[3] = end;
	
    u32 result = 0;
	bool found = false;
	
	do {
		for(int i=0;i<sizeofFind;i++) {
			if (addr[i] != find[i]) 
			{
				break;
			} else if(i==sizeofFind-1) {
				found = true;				
			}
		}	
		if(!found) addr+=direction;
	} while (addr != end && !found);
	
	if (addr == end) {
		return NULL;
	}
	
	return addr;
}


u32 patchCardNds (const tNDSHeader* ndsHeader) {	
	nocashMessage("patchCardNds");
	
	u32* debug = (u32*)0x037D0000;
	debug[4] = ndsHeader->arm9destination;

	// Find the card read
    u32 cardReadEndOffset =  
        getOffsetA9((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)a9cardReadSignature, 2, 1);
    if (!cardReadEndOffset) {
        nocashMessage("Card read end not found\n");
        return 0;
    }	
	debug[1] = cardReadEndOffset;	
    u32 cardReadStartOffset =   
        getOffsetA9((u32*)cardReadEndOffset, -0xF9,
              (u32*)cardReadStartSignature, 1, -1);
    if (!cardReadStartOffset) {
        nocashMessage("Card read start not found\n");
        return 0;
    }
	debug[0] = cardReadStartOffset;
    nocashMessage("Card read found\n");	

	debug[2] = cardengine_bin;
	
	u32* myCardEngine = (u32*)cardengine_bin;
	
	u32* patches =  (u32*)*myCardEngine;
	
	debug[5] = patches;
	
	copyLoop ((u32*)cardReadStartOffset, patches, 0xF0);	
	
	nocashMessage("ERR_NONE");
	return cardReadStartOffset;
}


