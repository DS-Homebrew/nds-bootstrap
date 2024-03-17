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

//#include <stddef.h>
#include <nds/system.h>
#include "nds_header.h"
#include "patch.h"
#include "common.h"
#include "tonccpy.h"

u16 patchOffsetCacheFilePrevCrc = 0;
u16 patchOffsetCacheFileNewCrc = 0;

patchOffsetCacheContents patchOffsetCache;

void rsetPatchCache(const tNDSHeader* ndsHeader)
{
	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 2) {
		toncset(&patchOffsetCache, 0, sizeof(patchOffsetCacheContents));
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.type = 2;	// 0 = Regular, 1 = B4DS, 2 = Homebrew
	}
}

void patchBinary(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	// Moonshell Ver 2.10 for child Zwai: Direct Boot
	if (strcmp(romTid, "####") == 0 && ndsHeader->headerCRC16 == 0x5638) {
		// Bypass ARM9/7 binary check
        *(u32*)0x02000B90 = 0xE1A00000; // nop
        *(u32*)0x02000B94 += 0xE0000000; // beq -> b
        *(u32*)0x02000D54 = 0xE1A00000; // nop
        *(u32*)0x02000D58 = 0xE1A00000; // nop
	}
}
