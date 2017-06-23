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
#include "fat.h"

#define PAGE_4K		(0b01011 << 1)
#define PAGE_8K		(0b01100 << 1)
#define PAGE_16K	(0b01101 << 1)
#define PAGE_32K	(0b01110 << 1)
#define PAGE_64K	(0b01111 << 1)
#define PAGE_128K	(0b10000 << 1)
#define PAGE_256K	(0b10001 << 1)
#define PAGE_512K	(0b10010 << 1)
#define PAGE_1M		(0b10011 << 1)
#define PAGE_2M		(0b10100 << 1)
#define PAGE_4M		(0b10101 << 1)
#define PAGE_8M		(0b10110 << 1)
#define PAGE_16M	(0b10111 << 1)
#define PAGE_32M	(0b11000 << 1)
#define PAGE_64M	(0b11001 << 1)
#define PAGE_128M	(0b11010 << 1)
#define PAGE_256M	(0b11011 << 1)
#define PAGE_512M	(0b11100 << 1)
#define PAGE_1G		(0b11101 << 1)
#define PAGE_2G		(0b11110 << 1)
#define PAGE_4G		(0b11111 << 1)

#define NDS_HEAD ((u32*)0x02FFFE00)

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


module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer);
void ensureArm9Decompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
/*-------------------------------------------------------------------------
arm7_hookGame
Adds a hook in the game's ARM7 binary to our own code
-------------------------------------------------------------------------*/
u32 patchCardNds (const tNDSHeader* ndsHeader, u32* cardEngineLocationArm7, u32* cardEngineLocationArm9, module_params_t* moduleParams, u32 saveFileCluster, u32 patchMpuRegion, u32 patchMpuSize, aFile donorFile);

#endif // CARD_PATCHER_H