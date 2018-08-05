#include <string.h>
#include "patch.h"
#include "find.h"
#include "common.h"
#include "cardengine_arm9_bin.h"
#include "debug_file.h"

//#define memcpy __builtin_memcpy // memcpy

//extern bool sdk5;
extern u32 ROM_TID;

bool cardReadFound = false; // card_patcher_common.c

void decompressLZ77Backwards(u8* addr, u32 size) {
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

u32 patchCardNdsArm9(const tNDSHeader* ndsHeader, u32* cardEngineLocationArm9, const module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {
	u32* debug = (u32*)0x037C6000;
	debug[4] = (u32)ndsHeader->arm9destination;
	debug[8] = moduleParams->sdk_version;

	//bool sdk5 = (moduleParams->sdk_version > 0x5000000);

	bool usesThumb = false;
	int readType = 0;
	int sdk5ReadType = 0; // SDK 5

	// Card read
	// SDK 5
	//dbg_printf("Trying SDK 5 thumb...\n");
	u32* cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type0(ndsHeader, moduleParams);
	if (!cardReadEndOffset) {
		// SDK 5
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb5Type1(ndsHeader, moduleParams);
		if (cardReadEndOffset) {
			sdk5ReadType = 1;
		}
	}
	if (!cardReadEndOffset) {
		//dbg_printf("Trying thumb...\n");
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb(ndsHeader);
	}
	if (cardReadEndOffset) {
		usesThumb = true;
	} else {
		cardReadEndOffset = findCardReadEndOffsetType0(ndsHeader, moduleParams);
		if (!cardReadEndOffset) {
			//dbg_printf("Trying alt...\n");
			cardReadEndOffset = findCardReadEndOffsetType1(ndsHeader);
			if (cardReadEndOffset) {
				readType = 1;
				if (*(cardReadEndOffset - 1) == 0xFFFFFE00) {
					dbg_printf("Found thumb\n\n");
					--cardReadEndOffset;
					usesThumb = true;
				}
			}
		}
	}
	if (!cardReadEndOffset) {
		return 0;
	}
	debug[1] = (u32)cardReadEndOffset;
	u32* cardReadStartOffset;
	// SDK 5
	//dbg_printf("Trying SDK 5 thumb...\n");
	if (sdk5ReadType == 0) {
		cardReadStartOffset = (u32*)findCardReadStartOffsetThumb5Type0(moduleParams, (u16*)cardReadEndOffset);
	} else {
		cardReadStartOffset = (u32*)findCardReadStartOffsetThumb5Type1(moduleParams, (u16*)cardReadEndOffset);
	}
	if (!cardReadStartOffset) {
		//dbg_printf("Trying thumb...\n");
		cardReadStartOffset = (u32*)findCardReadStartOffsetThumb((u16*)cardReadEndOffset);
	}
	if (!cardReadStartOffset) {
		//dbg_printf("Trying SDK 5...\n");
		cardReadStartOffset = (u32*)findCardReadStartOffset5(moduleParams, cardReadEndOffset);
	}
	if (!cardReadStartOffset) {
		if (readType == 0) {
			cardReadStartOffset = findCardReadStartOffsetType0(cardReadEndOffset);
		} else {
			cardReadStartOffset = findCardReadStartOffsetType1(cardReadEndOffset);
		}
	}
	if (!cardReadStartOffset) {
		return 0;
	}
	cardReadFound = true;

	// Card read cached
	u32* cardReadCachedEndOffset = findCardReadCachedEndOffset(ndsHeader, moduleParams);
	u32* cardReadCachedStartOffset = findCardReadCachedStartOffset(moduleParams, cardReadCachedEndOffset);
	u32 needFlushCache = (patchMpuRegion == 1) ? 1 : 0;

	// Card pull out
	u32* cardPullOutOffset;
	if (usesThumb) {
		//dbg_printf("Trying SDK 5 thumb...\n");
		if (sdk5ReadType == 0) {
			cardPullOutOffset = (u32*)findCardPullOutOffsetThumb5Type0(ndsHeader, moduleParams);
		} else {
			cardPullOutOffset = (u32*)findCardPullOutOffsetThumb5Type1(ndsHeader, moduleParams);
		}
		if (!cardPullOutOffset) {
			//dbg_printf("Trying thumb...\n");
			cardPullOutOffset = (u32*)findCardPullOutOffsetThumb(ndsHeader);
		}
	} else {
		cardPullOutOffset = findCardPullOutOffset(ndsHeader, moduleParams);
	}

	// Force to power off
	//u32* forceToPowerOffOffset = findForceToPowerOffOffset(ndsHeader);

	// Find the card id
	u32* cardIdEndOffset;
	u32* cardIdStartOffset;
	if (usesThumb) {
		cardIdEndOffset = (u32*)findCardIdEndOffsetThumb(ndsHeader, moduleParams, (u16*)cardReadEndOffset);
		cardIdStartOffset = (u32*)findCardIdStartOffsetThumb(moduleParams, (u16*)cardIdEndOffset);
	} else {
		cardIdEndOffset = findCardIdEndOffset(ndsHeader, moduleParams, cardReadEndOffset);
		cardIdStartOffset = findCardIdStartOffset(moduleParams, cardIdEndOffset);
	}
	if (cardIdEndOffset) {
		debug[1] = (u32)cardIdEndOffset;
	}

	// Card read dma
	u32* cardReadDmaEndOffset = NULL;
	if (usesThumb) {
		//dbg_printf("Trying thumb alt...\n");
		cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
	}
	if (!cardReadDmaEndOffset) {
		cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader);
	}
	u32* cardReadDmaStartOffset;
	if (usesThumb) {
		cardReadDmaStartOffset = (u32*)findCardReadDmaStartOffsetThumb((u16*)cardReadDmaEndOffset);
	} else {
		cardReadDmaStartOffset = findCardReadDmaStartOffset(moduleParams, cardReadDmaEndOffset);
	}

	// Find the mpu init
	u32* mpuStartOffset = findMpuStartOffset(ndsHeader, patchMpuRegion);
	u32* mpuDataOffset = findMpuDataOffset(moduleParams, patchMpuRegion, mpuStartOffset);
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

		*(vu32*)(sdk5 ? 0x3000000 : 0x2800000) = (vu32)mpuDataOffset; // ndsHead + 0x200
		*(vu32*)(sdk5 ? 0x3000004 : 0x2800004) = (vu32)*mpuDataOffset;

		*mpuDataOffset = mpuInitRegionNewData;

		if (mpuAccessOffset) {
			if (mpuNewInstrAccess) {
				mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
			}
			if (mpuNewDataAccess) {
				mpuDataOffset[mpuAccessOffset + 1] = mpuNewDataAccess;
			}
		}
	}

	// Find the mpu cache init
	u32* mpuInitCacheOffset = findMpuInitCacheOffset(mpuStartOffset);
	if (mpuInitCacheOffset) {
		*mpuInitCacheOffset = 0xE3A00046;
	}

	// Patch out all further mpu reconfiguration
	dbg_printf("patchMpuSize: ");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n\n");
	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	while (mpuStartOffset && patchMpuSize) {
		u32 patchSize = ndsHeader->arm9binarySize;
		if (patchMpuSize > 1) {
			patchSize = patchMpuSize;
		}
		mpuStartOffset = findOffset(
			//(u32*)((u32)mpuStartOffset + 4), patchSize,
			mpuStartOffset + 1, patchSize,
			mpuInitRegionSignature, 1
		);
		if (mpuStartOffset) {
			dbg_printf("Mpu init: ");
			dbg_hexa((u32)mpuStartOffset);
			dbg_printf("\n\n");

			*mpuStartOffset = 0xE1A00000; // nop

			// Try to find it
			/*for (int i = 0; i < 0x100; i++) {
				mpuDataOffset += i;
				if ((*mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
					*mpuDataOffset = PAGE_32M | 0x02000000 | 1;
					break;
				}
				if (i == 100) {
					*mpuStartOffset = 0xE1A00000;
				}
			}*/
		}
	}

	// Arena low
	/*u32* arenaLowOffset = findArenaLowOffset(ndsHeader);
	if (arenaLowOffset) {
		debug[0] = (u32)arenaLowOffset;

		arenaLowOffset = (u32*)((u32)arenaLowOffset + 0x88);
		debug[10] = (u32)arenaLowOffset;
		debug[11] = *arenaLowOffset;

		u32* oldArenaLow = (u32*)*arenaLowOffset;

		// *arenaLowOffset += 0x800; // shrink heap by 8 kb
		// *(vu32*)(0x027FFDA0) = *arenaLowOffset;
		debug[12] = *arenaLowOffset;

		u32* arenaLow2Offset = findOffset(
			(u32*)ndsHeader->arm9destination, 0x00100000,//ndsHeader->arm9binarySize,
			oldArenaLow, 1
		);

		// *arenaLow2Offset += 0x800; // shrink heap by 8 kb

		debug[13] = (u32)arenaLow2Offset;
	}*/
	
	// Random patch
	if (moduleParams->sdk_version > 0x3000000
	&& (ROM_TID & 0x00FFFFFF) != 0x544B41		// Doctor Tendo
	&& (ROM_TID & 0x00FFFFFF) != 0x5A4341		// Cars
	&& (ROM_TID & 0x00FFFFFF) != 0x434241		// Harvest Moon DS
	&& (ROM_TID & 0x00FFFFFF) != 0x4C5741)		// TWEWY
	{
		u32* randomPatchOffset = findRandomPatchOffset(ndsHeader);
		if (randomPatchOffset) {
			//*(u32*)((u32)randomPatchOffset + 0xC) = 0x0;
			*(randomPatchOffset + 3) = 0x0;
		}
	}

	// SDK 5
	u32* randomPatchOffset5First = findRandomPatchOffset5First(ndsHeader, moduleParams);
	if (randomPatchOffset5First) {
		*randomPatchOffset5First = 0xE3A00000;
		//*(u32*)((u32)randomPatchOffset5First + 4) = 0xE12FFF1E;
		*(randomPatchOffset5First + 1) = 0xE12FFF1E;
	}

	// SDK 5
	u32* randomPatchOffset5Second = findRandomPatchOffset5Second(ndsHeader, moduleParams);
	if (randomPatchOffset5Second) {
		*randomPatchOffset5Second = 0xE3A00000;
		//*(u32*)((u32)randomPatchOffset2 + 4) = 0xE12FFF1E;
		*(randomPatchOffset5Second + 1) = 0xE12FFF1E;
	}

	debug[2] = (u32)cardEngineLocationArm9;

	cardEngineLocationArm9[3] = moduleParams->sdk_version;

	u32* patches = (u32*)cardEngineLocationArm9[usesThumb ? 1 : 0];

	u32* cardReadPatch    = (u32*)patches[0];
	u32* cardPullOutPatch = (u32*)patches[6];
	u32* cardIdPatch      = (u32*)patches[3];
	u32* cardDmaPatch     = (u32*)patches[4];

	debug[5] = (u32)patches;

	u32** card_struct = (u32**)(cardReadEndOffset - 1);
	//u32* cache_struct = (u32**)(cardIdStartOffset - 1);

	debug[6] = (u32)*card_struct;
	//debug[7] = (u32)*cache_struct;

	if (moduleParams->sdk_version > 0x3000000) {
		cardEngineLocationArm9[5] = (u32)(*card_struct + 7);
		*(u32*)patches[5] = (u32)(*card_struct + 7); // Cache management alternative
	} else {
		cardEngineLocationArm9[5] = (u32)(*card_struct + 6);
		*(u32*)patches[5] = (u32)(*card_struct + 6); // Cache management alternative
	}
	//cardEngineLocationArm9[6] = (u32)*cache_struct;

	*(u32*)patches[7] = (u32)cardPullOutOffset + 4;

	if ((ROM_TID & 0x00FFFFFF) != 0x443241	// New Super Mario Bros
	&& (ROM_TID & 0x00FFFFFF) != 0x4D4441)	// Animal Crossing: Wild World
	{
		*(u32*)patches[8] = (u32)cardReadCachedStartOffset;
	}

	//if (!usesThumb) { // Based on: cardengine_arm9/source/card_engine_header.s
		patches[10] = needFlushCache;
	//}

	//memcpy(oldArenaLow, cardReadPatch, 0xF0); //copyLoop(oldArenaLow, cardReadPatch, 0xF0);

	memcpy(cardReadStartOffset, cardReadPatch, usesThumb ? (sdk5 ? 0xB0 : 0xA0) : 0xF0); //copyLoop(cardReadStartOffset, cardReadPatch, usesThumb ? 0xA0 : 0xF0);

	memcpy(cardPullOutOffset, cardPullOutPatch, 0x4); //copyLoop(cardPullOutOffset, cardPullOutPatch, 0x4);

	/*if (forceToPowerOffOffset) {
		memcpy(forceToPowerOffOffset, cardPullOutPatch, 0x4); //copyLoop(forceToPowerOffOffset, cardPullOutPatch, 0x4);
	}*/

	if (cardIdStartOffset) {
		memcpy(cardIdStartOffset, cardIdPatch, usesThumb ? 0x4 : 0x8); //copyLoop(cardIdStartOffset, cardIdPatch, usesThumb ? 0x4 : 0x8);
	}

	if (cardReadDmaStartOffset) {
		memcpy(cardReadDmaStartOffset, cardDmaPatch, usesThumb ? 0x4 : 0x8); //copyLoop(cardReadDmaStartOffset, cardDmaPatch, usesThumb ? 0x4 : 0x8);
	}

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
