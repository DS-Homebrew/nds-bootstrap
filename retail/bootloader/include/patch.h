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

#ifndef PATCH_H
#define PATCH_H

//#include <stddef.h>
#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "module_params.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"

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

//extern bool cardReadFound; // patch_arm9.c

u32 generateA7Instr(int arg1, int arg2);
const u16* generateA7InstrThumb(int arg1, int arg2);
void patchBinary(const tNDSHeader* ndsHeader);
u32 patchCardNdsArm9(
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 patchMpuRegion,
	u32 patchMpuSize
);
u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 saveFileCluster
);
u32 patchCardNds(
	cardengineArm7* ce7,
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 patchMpuRegion,
	u32 patchMpuSize,
	u32 ROMinRAM,
	u32 saveFileCluster,
	u32 saveSize
);
u32* patchHeapPointer(
    const module_params_t* moduleParams,
    const tNDSHeader* ndsHeader, 
    bool usesThumb
);
void relocate_ce9(
    u32 default_location, 
    u32 current_location, 
    u32 size
);  

#endif // PATCH_H
