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

#include <stddef.h>
#include <nds/system.h>
#include "card_patcher.h"
#include "card_finder.h"
#include "common.h"
#include "cardengine_arm9_bin.h"
#include "cardengine_arm7_bin.h"
#include "debugToFile.h"

extern u32 ROMinRAM;
extern u32 ROM_TID;

u32 savePatchV1(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize);
u32 savePatchV2(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize);
u32 savePatchUniversal(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize);

u32 generateA7Instr(int arg1, int arg2) {
	return (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

void generateA7InstrThumb(u16* instrs, int arg1, int arg2) {
	// 23 bit offset
	u32 offset = (u32)(arg2 - arg1 - 4);
	//dbg_printf("generateA7InstrThumb offset\n");
	//dbg_hexa(offset);
	
	// 1st instruction contains the upper 11 bit of the offset
	instrs[0] = ((offset >> 12) & 0x7FF) | 0xF000;

	// 2nd instruction contains the lower 11 bit of the offset
	instrs[1] = ((offset >> 1) & 0x7FF) | 0xF800;
}

void decompressLZ77Backwards(u8* addr, size_t size) {
	u32 len = *(u32*)(addr + size - 4) + size;

	//byte[] Result = new byte[len];
	//Array.Copy(Data, Result, Data.Length);

	u32 end = *(u32*)(addr + size - 8) & 0xFFFFFF;

	u8* result = addr;

	int Offs = (int)(size - (*(u32*)(addr + size - 8) >> 24));
	int dstoffs = (int)len;
	while (true) {
		u8 header = result[--Offs];
		for (int i = 0; i < 8; i++) {
			if ((header & 0x80) == 0) {
				result[--dstoffs] = result[--Offs];
			} else {
				u8 a = result[--Offs];
				u8 b = result[--Offs];
				int offs = (((a & 0xF) << 8) | b) + 2;//+ 1;
				int length = (a >> 4) + 2;
				do {
					result[dstoffs - 1] = result[dstoffs + offs];
					dstoffs--;
					length--;
				} while (length >= 0);
			}

			if (Offs <= size - end) {
				return;
			}

			header <<= 1;
		}
	}
}

void ensureArm9Decompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	*(vu32*)(0x280000C) = moduleParams->compressed_static_end;
	if (!moduleParams->compressed_static_end) {
		dbg_printf("This rom is not compressed\n");
		return; // Not compressed
	}
	dbg_printf("This rom is compressed;)\n");
	decompressLZ77Backwards((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
	moduleParams->compressed_static_end = 0;
}

u32 patchCardNdsArm9(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	u32* debug = (u32*)0x037C6000;
	debug[4] = (u32)ndsHeader->arm9destination;
	debug[8] = moduleParams->sdk_version;

	// Card read
	u32 cardReadEndOffset = getCardReadEndOffset(ndsHeader, moduleParams);
	if (!cardReadEndOffset) {
		return 0;
	}
	debug[1] = cardReadEndOffset;
	u32 cardReadStartOffset = getCardReadStartOffset(ndsHeader, cardReadEndOffset);
	if (!cardReadStartOffset) {
		return 0;
	}

	// Card read cached
	u32 cardReadCachedEndOffset = getCardReadCachedEndOffset(ndsHeader, moduleParams);
	u32 cardReadCachedStartOffset = getCardReadCachedStartOffset(ndsHeader, moduleParams, cardReadCachedEndOffset);
	u32 needFlushCache = (patchMpuRegion == 1) ? 1 : 0;

	// Card pull out
	u32 cardPullOutOffset = getCardPullOutOffset(ndsHeader, moduleParams);

	// Force to power off
	//u32 forceToPowerOffOffset = getForceToPowerOffOffset(ndsHeader);

	// Find the card id
	u32 cardIdEndOffset = getCardIdEndOffset(ndsHeader, cardReadEndOffset);
	if (cardIdEndOffset) {
		debug[1] = cardIdEndOffset;
	}
	u32 cardIdStartOffset = getCardIdStartOffset(ndsHeader, cardIdEndOffset);

	// Card read dma
	u32 cardReadDmaEndOffset = getCardReadDmaEndOffset(ndsHeader);
	u32 cardReadDmaStartOffset = getCardReadDmaStartOffset(ndsHeader, cardReadDmaEndOffset);

	// Find the mpu unit
	u32 mpuStartOffset = getMpuStartOffset(ndsHeader, patchMpuRegion);
	u32 mpuDataOffset = getMpuDataOffset(ndsHeader, moduleParams, mpuStartOffset, patchMpuRegion);
	if (mpuDataOffset) {
		// Change the region 1 configuration

		u32 mpuInitRegionNewData = PAGE_32M | 0x02000000 | 1;
		u32 mpuNewDataAccess     = 0;
		u32 mpuNewInstrAccess    = 0;
		int mpuAccessOffset      = 0;
		switch (patchMpuRegion) {
			case 0:
				mpuInitRegionNewData = PAGE_128M | 0x00000000 | 1;
				break;
			case 2:
				mpuNewDataAccess  = 0x15111111;
				mpuNewInstrAccess = 0x5111111;
				mpuAccessOffset   = 6;
				break;
			case 3:
				mpuInitRegionNewData = PAGE_8M | 0x03000000 | 1;
				mpuNewInstrAccess    = 0x5111111;
				mpuAccessOffset      = 5;
				break;
		}

		*(vu32*)(0x2800000) = (vu32)(mpuDataOffset);
		*(vu32*)(0x2800004) = (vu32)*(u32*)mpuDataOffset;

		*(u32*)mpuDataOffset = mpuInitRegionNewData;

		if (mpuAccessOffset) {
			if (mpuNewInstrAccess) {
				((u32*)mpuDataOffset)[mpuAccessOffset] = mpuNewInstrAccess;
			}
			if (mpuNewDataAccess) {
				((u32*)mpuDataOffset)[mpuAccessOffset + 1] = mpuNewDataAccess;
			}
		}
	}

	// Find the mpu cache init
	u32 mpuInitCacheOffset = getMpuInitCacheOffset(ndsHeader, mpuStartOffset);
	if (mpuInitCacheOffset) {
		*(u32*)mpuInitCacheOffset = 0xE3A00046;
	}

	// Patch out all further mpu reconfiguration
	dbg_printf("patchMpuSize :\t");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n");
	u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	while (mpuStartOffset && patchMpuSize) {
		u32 patchSize = ndsHeader->arm9binarySize;
		if (patchMpuSize > 1) {
			patchSize = patchMpuSize;
		}
		mpuStartOffset = getOffset(
			(u32*)(mpuStartOffset + 4), patchSize,
			mpuInitRegionSignature, 1,
			1
		);
		if (mpuStartOffset) {
			dbg_printf("Mpu init :\t");
			dbg_hexa(mpuStartOffset);
			dbg_printf("\n");

			*(u32*)mpuStartOffset = 0xE1A00000; // nop

			// Try to find it
			/*for (int i = 0; i < 0x100; i++) {
				mpuDataOffset += i;
				if ((*mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
					*mpuDataOffset = PAGE_32M | 0x02000000 | 1;
					break;
				}
				if (i == 100) {
					*(u32*)mpuStartOffset = 0xE1A00000;
				}
			}*/
		}
	}

	// Arena low
	/*u32 arenaLowOffset = getArenaLowOffset(ndsHeader);
	if (arenaLowOffset) {
		debug[0] = arenaLowOffset;

		arenaLowOffset += 0x88;
		debug[10] = arenaLowOffset;
		debug[11] = *(u32*)arenaLowOffset;

		u32* oldArenaLow = (u32*)*(u32*)arenaLowOffset;

		// *(u32*)arenaLowOffset += 0x800; // shrink heap by 8 kb
		// *(vu32*)(0x027FFDA0) = *(u32*)arenaLowOffset;
		debug[12] = *(u32*)arenaLowOffset;

		u32 arenaLow2Offset = getOffset(
			(u32*)ndsHeader->arm9destination, 0x00100000,//ndsHeader->arm9binarySize,
			oldArenaLow, 1,
			1
		);

		// *(u32*)arenaLow2Offset += 0x800; // shrink heap by 8 kb

		debug[13] = arenaLow2Offset;
	}*/
	
	// Random patch
	if (moduleParams->sdk_version > 0x3000000
	&& (ROM_TID & 0x00FFFFFF) != 0x544B41		// Doctor Tendo
	&& (ROM_TID & 0x00FFFFFF) != 0x5A4341		// Cars
	&& (ROM_TID & 0x00FFFFFF) != 0x434241		// Harvest Moon DS
	&& (ROM_TID & 0x00FFFFFF) != 0x4C5741)		// TWEWY
	{
		u32 randomPatchOffset = getRandomPatchOffset(ndsHeader);
		if (randomPatchOffset) {
			*(u32*)(randomPatchOffset + 0xC) = 0x0;
		}
	}

	debug[2] = (u32)cardEngineLocation;

	cardEngineLocation[3] = moduleParams->sdk_version;

	u32* patches = (u32*)cardEngineLocation[usesThumb ? 1 : 0];

	u32* cardReadPatch    = (u32*)patches[0];
	u32* cardPullOutPatch = (u32*)patches[6];
	u32* cardIdPatch      = (u32*)patches[3];
	u32* cardDmaPatch     = (u32*)patches[4];

	debug[5] = (u32)patches;

	u32* card_struct = (u32*)cardReadEndOffset - 1;
	//u32* cache_struct = (u32*)cardIdStartOffset - 1;

	debug[6] = *card_struct;
	//debug[7] = *cache_struct;

	cardEngineLocation[5] = (u32)(((u32*)*card_struct) + 6);
	if (moduleParams->sdk_version > 0x3000000) {
		cardEngineLocation[5] = (u32)(((u32*)*card_struct) + 7);
	}
	//cardEngineLocation[6] = *cache_struct;

	// Cache management alternative
	*(u32*)patches[5] = (u32)(((u32*)*card_struct) + 6);
	if (moduleParams->sdk_version > 0x3000000) {
		*(u32*)patches[5] = (u32)(((u32*)*card_struct) + 7);
	}

	*(u32*)patches[7] = cardPullOutOffset + 4;

	if ((ROM_TID & 0x00FFFFFF) != 0x443241	// New Super Mario Bros
	&& (ROM_TID & 0x00FFFFFF) != 0x4D4441)	// Animal Crosing: Wild World
	{
		*(u32*)patches[8] = cardReadCachedStartOffset;
	}

	patches[10] = needFlushCache;

	//copyLoop(oldArenaLow, cardReadPatch, 0xF0);

	copyLoop((u32*)cardReadStartOffset, cardReadPatch, usesThumb ? 0xA0 : 0xF0);

	copyLoop((u32*)cardPullOutOffset, cardPullOutPatch, 0x4);

	/*if (forceToPowerOffOffset) {
		copyLoop((u32*)forceToPowerOffOffset, cardPullOutPatch, 0x4);
	}*/

	if (cardIdStartOffset) {
		copyLoop((u32*)cardIdStartOffset, cardIdPatch, usesThumb ? 0x4 : 0x8);
	}

	if (cardReadDmaStartOffset) {
		copyLoop((u32*)cardReadDmaStartOffset, cardDmaPatch, usesThumb ? 0x4 : 0x8);
	}

	dbg_printf("ERR_NONE");
	return ERR_NONE;
}

void patchSwiHalt(const tNDSHeader* ndsHeader, u32* cardEngineLocation) {
	// swi halt
	u32 swiHaltOffset = getSwiHaltOffset(ndsHeader);
	if (swiHaltOffset) {
		// Patch
		u32* patches = (u32*)cardEngineLocation[0];
		u32* swiHaltPatch = (u32*)patches[11];
		copyLoop((u32*)swiHaltOffset, swiHaltPatch, 0xC);
	}
}

void fixForDsiBios(const tNDSHeader* ndsHeader, u32* cardEngineLocation) {
	u32* patches = (u32*)cardEngineLocation[0];

	// swi 0x12 call
	u32 swi12Offset = getSwi12Offset(ndsHeader);
	if (swi12Offset) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = (u32*)patches[10];
		copyLoop((u32*)swi12Offset, swi12Patch, 0x4);
	}

	sdk5 = false;

	// swi get pitch table
	u32 swiGetPitchTableOffset = getSwiGetPitchTableOffset(ndsHeader);
	if (swiGetPitchTableOffset) {
		// Patch
		u32* swiGetPitchTablePatch = (u32*)patches[sdk5 ? 13 : 12];
		copyLoop((u32*)swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
	}
}

u32 patchCardNdsArm7(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {
	u32* debug = (u32*)0x037C6000;

	if (REG_SCFG_ROM != 0x703) {
		fixForDsiBios(ndsHeader, cardEngineLocation);
	}
	if (ROMinRAM == false) {
		patchSwiHalt(ndsHeader, cardEngineLocation);
	}
	
	usesThumb = false;

	// Sleep patch
	u32 sleepPatchOffset = getSleepPatchOffset(ndsHeader);
	if (sleepPatchOffset) {
		// Patch
		if (usesThumb) {
			*(u16*)(sleepPatchOffset + 4) = 0;
			*(u16*)(sleepPatchOffset + 6) = 0;
		} else {
			*(u32*)(sleepPatchOffset + 8) = 0;
		}
	}

	// Card check pull out
	u32 cardCheckPullOutOffset = getCardCheckPullOutOffset(ndsHeader, moduleParams);
	if (cardCheckPullOutOffset) {
		debug[0] = cardCheckPullOutOffset;
	}

	// Card irq enable
	u32 cardIrqEnableOffset = getCardIrqEnableOffset(ndsHeader, moduleParams);
	if (!cardIrqEnableOffset) {
		return 0;
	}
	debug[0] = cardIrqEnableOffset;

	cardEngineLocation[3] = moduleParams->sdk_version;

	u32* patches = (u32*)cardEngineLocation[0];

	u32* cardIrqEnablePatch    = (u32*)patches[2];
	u32* cardCheckPullOutPatch = (u32*)patches[1];

	if (cardCheckPullOutOffset) {
		copyLoop((u32*)cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);
	}

	copyLoop((u32*)cardIrqEnableOffset, cardIrqEnablePatch, 0x30);

	u32 saveResult = savePatchV1(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	if (!saveResult) {
		saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	}
	if (!saveResult) {
		saveResult = savePatchUniversal(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	}
	if (saveResult == 1 && ROMinRAM == false && saveSize > 0 && saveSize <= 0x00100000) {
		aFile saveFile = getFileFromCluster(saveFileCluster);
		fileRead((char*)0x0C820000, saveFile, 0, saveSize, 3);
	}

	dbg_printf("ERR_NONE");
	return ERR_NONE;
}

u32 patchCardNds(
	const tNDSHeader* ndsHeader,
	u32* cardEngineLocationArm7,
	u32* cardEngineLocationArm9,
	module_params_t* moduleParams, 
	u32 saveFileCluster,
	u32 saveSize,
	u32 patchMpuRegion,
	u32 patchMpuSize) {
	// Debug stuff
	/*aFile myDebugFile = getBootFileCluster ("NDSBTSR2.LOG");
	enableDebug(myDebugFile);*/

	dbg_printf("patchCardNds");

	patchCardNdsArm9(ndsHeader, cardEngineLocationArm9, moduleParams, patchMpuRegion, patchMpuSize);
	
	if (cardReadFound || ndsHeader->fatSize == 0) {
		patchCardNdsArm7(ndsHeader, cardEngineLocationArm7, moduleParams, saveFileCluster, saveSize);

		dbg_printf("ERR_NONE");
		return ERR_NONE;
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}
