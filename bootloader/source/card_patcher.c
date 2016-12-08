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
u32 a9cardReadSignature[2]    = {0x10001004, 0xA4010004};
u32 a9cardReadSignatureHE[2]    = {0x04100010, 0x040001A4};
u16 cardReadStartSignature[1] = {0xE92D};
u32 a9cardIdSignature[2]      = {0x040001A4,0x04100010};
u16 cardIdStartSignature[1]   = {0xE92D};
u32 a9instructionBHI[1]       = {0x8A000001};

//
// Look in @data for @find and return the position of it.
//
u32 getOffsetA9(u8* data, int size, void* find, u32 sizeofFind, int specifier)
{
    s32 result = 0;

    // Go backwards
    if (specifier == -1) {
        // Simply scan through @data
        u8* comparison = (data-sizeofFind);
        result = -sizeofFind;
        s32 offset = 0;
        // @result, @offset, exist due to a reasoning in the original patcher
        // I can't entirely explain; this is required for proper offsets
        for (result = -sizeofFind; result >= -size; result--)
        {
            // If @find is found, break and return the decremented offset
            if (!memcmp(comparison + offset, find, sizeofFind))
                return result;
            offset--;
        }
    }

    // Go forwards
    if (specifier == 1) {
        // Simply scan through @data
        for (result = 0; result < size - sizeofFind; result++)
        {
            // If @find is found, break and return the incremented offset
            if (!memcmp((void*)(data + result), find, sizeofFind))
                return result;
        }
    }

    // If scanning is finished and end was not reached, return no offset.
    return 0;
}


u32 patchCardNds (const tNDSHeader* ndsHeader) {	
	nocashMessage("patchCardNds");

	// Find the card read
    u32 cardReadEndOffset =  // should result in 5AA64
        getOffsetA9((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (void*)a9cardReadSignature, 8, 1);
    if (!cardReadEndOffset) {
        nocashMessage("Card read end not found\n");
        return 0;;
    }
    u8 *vAddrCardReadEnd = ((u32*)ndsHeader->arm9destination)[cardReadEndOffset];
    u32 cardReadStartOffset =   // should result in FFFFFF0A (-F6?)
        getOffsetA9((void*)(vAddrCardReadEnd - 0x2000000), 0x200,
              (void*)cardReadStartSignature, 2, -1);
    if (!cardReadStartOffset) {
        nocashMessage("Card read start not found\n");
        return 0;;
    }
    u32 vAddrOfCardRead = cardReadEndOffset + cardReadStartOffset +
        (u32*)ndsHeader->arm9destination - 2;
    nocashMessage("Card read found\n");
	
	nocashMessage("ERR_NONE");
	return vAddrOfCardRead;
}


