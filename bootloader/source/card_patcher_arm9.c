#include "card_patcher.h"
#include "card_finder.h"
#include "debugToFile.h"

extern u32 ROM_TID;

bool cardReadFound = false; // card_patcher_common.c

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

	bool usesThumb = false;
	int readType = 0;

	// Card read
	u32* cardReadEndOffset = findCardReadEndOffset0(ndsHeader, moduleParams);
	if (!cardReadEndOffset) {
		cardReadEndOffset = findCardReadEndOffset1(ndsHeader);
		if (cardReadEndOffset) {
			readType = 1;
		}
	}
	if (!cardReadEndOffset) {
		cardReadEndOffset = (u32*)findCardReadEndOffsetThumb(ndsHeader, moduleParams);
		if (cardReadEndOffset) {
			usesThumb = true;
		}
	}
	if (!cardReadEndOffset) {
		return 0;
	}
	debug[1] = (u32)cardReadEndOffset;
	cardReadFound = true;
	u32* cardReadStartOffset;
	if (usesThumb) {
		cardReadStartOffset = (u32*)findCardReadStartOffsetThumb((u16*)cardReadEndOffset);
	} else {
		if (readType == 0) {
			cardReadStartOffset = findCardReadStartOffset0(cardReadEndOffset);
		} else {
			cardReadStartOffset = findCardReadStartOffset1(cardReadEndOffset);
		}
	}
	if (!cardReadStartOffset) {
		return 0;
	}

	// Card read cached
	u32* cardReadCachedEndOffset = findCardReadCachedEndOffset(ndsHeader, moduleParams);
	u32* cardReadCachedStartOffset = findCardReadCachedStartOffset(moduleParams, cardReadCachedEndOffset);
	u32 needFlushCache = (patchMpuRegion == 1) ? 1 : 0;

	// Card pull out
	u32* cardPullOutOffset;
	if (usesThumb) {
		cardPullOutOffset = (u32*)findCardPullOutOffsetThumb(ndsHeader, moduleParams);
	} else {
		cardPullOutOffset = findCardPullOutOffset(ndsHeader, moduleParams);
	}

	// Force to power off
	//u32 forceToPowerOffOffset = (u32)findForceToPowerOffOffset(ndsHeader);

	// Find the card id
	u32* cardIdEndOffset;
	u32* cardIdStartOffset;
	if (usesThumb) {
		cardIdEndOffset = (u32*)findCardIdEndOffsetThumb(ndsHeader, (u16*)cardReadEndOffset);
		cardIdStartOffset = (u32*)findCardIdStartOffsetThumb((u16*)cardIdEndOffset);
	} else {
		cardIdEndOffset = findCardIdEndOffset(ndsHeader, cardReadEndOffset);
		cardIdStartOffset = findCardIdStartOffset(cardIdEndOffset);
	}
	if (cardIdEndOffset) {
		debug[1] = (u32)cardIdEndOffset;
	}

	// Card read dma
	u32* cardReadDmaEndOffset;
	u32* cardReadDmaStartOffset;
	if (usesThumb) {
		cardReadDmaEndOffset = (u32*)findCardReadDmaEndOffsetThumb(ndsHeader);
		cardReadDmaStartOffset = (u32*)findCardReadDmaStartOffsetThumb((u16*)cardReadDmaEndOffset);
	} else {
		cardReadDmaEndOffset = findCardReadDmaEndOffset(ndsHeader);
		cardReadDmaStartOffset = findCardReadDmaStartOffset(cardReadDmaEndOffset);
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

		*(vu32*)(0x2800000) = (vu32)mpuDataOffset;
		*(vu32*)(0x2800004) = (vu32)*mpuDataOffset;

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
	dbg_printf("patchMpuSize :\t");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n");
	u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);
	while (mpuStartOffset && patchMpuSize) {
		u32 patchSize = ndsHeader->arm9binarySize;
		if (patchMpuSize > 1) {
			patchSize = patchMpuSize;
		}
		mpuStartOffset = findOffset(
			(u32*)((u32)mpuStartOffset + 4), patchSize,
			mpuInitRegionSignature, 1,
			1
		);
		if (mpuStartOffset) {
			dbg_printf("Mpu init :\t");
			dbg_hexa((u32)mpuStartOffset);
			dbg_printf("\n");

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
			oldArenaLow, 1,
			1
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
			*(u32*)((u32)randomPatchOffset + 0xC) = 0x0;
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

	u32* card_struct = cardReadEndOffset - 1;
	//u32* cache_struct = cardIdStartOffset - 1;

	debug[6] = *card_struct;
	//debug[7] = *cache_struct;

	cardEngineLocation[5] = (u32)((u32*)*card_struct + 6);
	if (moduleParams->sdk_version > 0x3000000) {
		cardEngineLocation[5] = (u32)((u32*)*card_struct + 7);
	}
	//cardEngineLocation[6] = *cache_struct;

	// Cache management alternative
	*(u32*)patches[5] = (u32)((u32*)*card_struct + 6);
	if (moduleParams->sdk_version > 0x3000000) {
		*(u32*)patches[5] = (u32)((u32*)*card_struct + 7);
	}

	*(u32*)patches[7] = (u32)cardPullOutOffset + 4;

	if ((ROM_TID & 0x00FFFFFF) != 0x443241	// New Super Mario Bros
	&& (ROM_TID & 0x00FFFFFF) != 0x4D4441)	// Animal Crosing: Wild World
	{
		*(u32*)patches[8] = (u32)cardReadCachedStartOffset;
	}

	patches[10] = needFlushCache;

	//copyLoop(oldArenaLow, cardReadPatch, 0xF0);

	copyLoop(cardReadStartOffset, cardReadPatch, usesThumb ? 0xA0 : 0xF0);

	copyLoop(cardPullOutOffset, cardPullOutPatch, 0x4);

	/*if (forceToPowerOffOffset) {
		copyLoop((u32*)forceToPowerOffOffset, cardPullOutPatch, 0x4);
	}*/

	if (cardIdStartOffset) {
		copyLoop(cardIdStartOffset, cardIdPatch, usesThumb ? 0x4 : 0x8);
	}

	if (cardReadDmaStartOffset) {
		copyLoop(cardReadDmaStartOffset, cardDmaPatch, usesThumb ? 0x4 : 0x8);
	}

	dbg_printf("ERR_NONE");
	return ERR_NONE;
}
