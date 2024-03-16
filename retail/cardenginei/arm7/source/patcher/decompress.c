/*
	Copyright (C) 2008 somebody
	Copyright (C) 2009 yellow wood goblin
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "nds_header.h"
#include "module_params.h"
#include "unpatched_funcs.h"
#include "decompress.h"
//#include "debug_file.h"
#include "locations.h"
#include "ndma.h"

u32 iUncompressedSize = 0;
static u32 iFixedAddr = 0;
static u32 iFixedData = 0;

static u32 decompressBinary(u8 *aMainMemory, u32 aCodeLength, u32 aMemOffset, const u32 resetParam) {
	u8 *ADDR1 = NULL;
	u8 *ADDR1_END = NULL;
	u8 *ADDR2 = NULL;
	u8 *ADDR3 = NULL;

	u8 *pBuffer32 = (u8 *)(aMainMemory);
	u8 *pBuffer32End = (u8 *)(aMainMemory + aCodeLength);

	while (pBuffer32 < pBuffer32End) {
		if (0xDEC00621 == *(u32 *)pBuffer32 && 0x2106C0DE == *(u32 *)(pBuffer32 + 4)) {
			ADDR1 = (u8 *)(*(u32 *)(pBuffer32 - 8));
			iFixedAddr = (u32)(pBuffer32 - 8);
			iFixedData = *(u32 *)(pBuffer32 - 8);
			*(u32 *)(pBuffer32 - 8) = 0;
			break;
		}
		pBuffer32 += 4;
	}
	if (0 == ADDR1) {
		iFixedAddr = 0;
		return 0;
	}

	u32 A = *(u32 *)(ADDR1 + aMemOffset - 4);
	u32 B = *(u32 *)(ADDR1 + aMemOffset - 8);
	ADDR1_END = ADDR1 + A;
	ADDR2 = ADDR1 - (B >> 24);
	B &= ~0xff000000;
	ADDR3 = ADDR1 - B;
	u32 uncompressEnd = ((u32)ADDR1_END) - ((u32)aMainMemory);
	if (uncompressEnd >= ((*(u32*)(resetParam+8) == 0x44414F4) ? 0x2C0000 : 0x380000)) {
		while (ndmaBusy(1)); // Wait for ARM7 binary to finish copying
	}

	while (!(ADDR2 <= ADDR3)) {
		u32 marku8 = *(--ADDR2 + aMemOffset);
		//ADDR2-=1;
		int count = 8;
		while (true) {
			count--;
			if (count < 0)
				break;
			if (0 == (marku8 & 0x80)) {
				*(--ADDR1_END + aMemOffset) = *(--ADDR2 + aMemOffset);
			} else {
				int u8_r12 = *(--ADDR2 + aMemOffset);
				int u8_r7 = *(--ADDR2 + aMemOffset);
				u8_r7 |= (u8_r12 << 8);
				u8_r7 &= ~0xf000;
				u8_r7 += 2;
				u8_r12 += 0x20;
				do
				{
					u8 realu8 = *(ADDR1_END + aMemOffset + u8_r7);
					*(--ADDR1_END + aMemOffset) = realu8;
					u8_r12 -= 0x10;
				} while (u8_r12 >= 0);
			}
			marku8 <<= 1;
			if (ADDR2 <= ADDR3) {
				break;
			}
		}
	}
	return uncompressEnd;
}

void ensureBinaryDecompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams, const u32 resetParam) {
	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	if (moduleParams->compressed_static_end) {
		// Compressed
		//dbg_printf("arm9 is compressed\n");
		unpatchedFuncs->compressedFlagOffset = (u32*)((u32)moduleParams+0x14);
		unpatchedFuncs->compressed_static_end = moduleParams->compressed_static_end;
		//decompressLZ77Backwards((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
		iUncompressedSize = decompressBinary((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize, 0, resetParam);
		moduleParams->compressed_static_end = 0;
	} else {
		// Not compressed
		//dbg_printf("arm9 is not compressed\n");
		iUncompressedSize = ndsHeader->arm9binarySize;
	}
}
