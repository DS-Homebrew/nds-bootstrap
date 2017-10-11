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
#include "cardengine_arm9_bin.h"
#include "cardengine_arm7_bin.h"
#include "debugToFile.h"

// Subroutine function signatures arm7
u32 relocateStartSignature[1]  = {0x027FFFFA};
u32 a7cardReadSignature[2]     = {0x04100010,0x040001A4};
u32 a7something1Signature[2]   = {0xE350000C,0x908FF100};
u32 a7something2Signature[2]   = {0x0000A040,0x040001A0};

u32 a7JumpTableSignature[4] = {0xE5950024,0xE3500000,0x13A00001,0x03A00000}; 

// Subroutine function signatures arm9
u32 moduleParamsSignature[2]   = {0xDEC00621, 0x2106C0DE};

// sdk < 4 version
u32 a9cardReadSignature1[2]    = {0x04100010, 0x040001A4};
u32 cardReadStartSignature1[1] = {0xE92D4FF0};

// sdk > 4 version
u32 a9cardReadSignature4[2]    = {0x040001A4, 0x04100010};
u32 cardReadStartSignature4[1] = {0xE92D4070};

u32 a9cardIdSignature[2]      = {0x040001A4,0x04100010};  
u32 cardIdStartSignature[1]   = {0xE92D4000};
u32 cardIdStartSignatureAlt[1]   = {0xE92D4008};   
u32 cardIdStartSignatureAlt2[1]   = {0xE92D4010};
  
u32 a9instructionBHI[1]       = {0x8A000001};
u32 cardPullOutSignature1[4]   = {0xE92D4000,0xE24DD004,0xE201003F,0xE3500011};
u32 cardPullOutSignature4[4]   = {0xE92D4008,0xE201003F,0xE3500011,0x1A00000D};
u32 a9cardSendSignature[7]    = {0xE92D40F0,0xE24DD004,0xE1A07000,0xE1A06001,0xE1A01007,0xE3A0000E,0xE3A02000};
u32 cardCheckPullOutSignature1[4]   = {0xE92D4018,0xE24DD004,0xE59F204C,0xE1D210B0};
u32 cardCheckPullOutSignature3[4]   = {0xE92D4000,0xE24DD004,0xE59F002C,0xE1D000B0};
u32 cardReadCachedStartSignature1[2]   = {0xE92D4030,0xE24DD004};
u32 cardReadCachedEndSignature1[4]   = {0xE5950020,0xE3500000,0x13A00001,0x03A00000};

u32 cardReadCachedEndSignature3[4]   = {0xE5950024,0xE3500000,0x13A00001,0x03A00000};

u32 cardReadCachedStartSignature4[2]   = {0xE92D4038,0xE59F407C};
u32 cardReadCachedEndSignature4[4]   = {0xE5940024,0xE3500000,0x13A00001,0x03A00000};
   
u32 cardReadDmaStartSignature[1]   = {0xE92D4FF8};
u32 cardReadDmaStartSignatureAlt[1]   = {0xE92D47F0};
u32 cardReadDmaStartSignatureAlt2[1]   = {0xE92D4FF0};
u32 cardReadDmaEndSignature[2]   = {0x01FF8000,0x000001FF};     

u32 aRandomPatch[4] = {0xE3500000, 0x1597002C, 0x10406004,0x03E06000};
 

     
// irqEnable
u32 irqEnableStartSignature1[4] = {0xE59FC028,0xE1DC30B0,0xE3A01000,0xE1CC10B0};
u32 irqEnableStartSignature4[4] = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020};

u32 arenaLowSignature[4] = {0xE1A00100,0xE2800627,0xE2800AFF,0xE5801DA0};  

u32 mpuInitRegion0Signature[1] = {0xEE060F10};
u32 mpuInitRegion0Data[1] = {0x4000033};

u32 mpuInitRegion1Signature[1] = {0xEE060F11};
u32 mpuInitRegion1Data1[1] = {0x200002D};
// sdk >= 4 version
u32 mpuInitRegion1Data4[1] = {0x200002D};

u32 mpuInitRegion1DataAlt[1] = {0x200002B};

u32 mpuInitRegion2Signature[1] = {0xEE060F12};
// sdk < 3 version
u32 mpuInitRegion2Data1[1] = {0x27C0023};
// sdk >= 3 version
u32 mpuInitRegion2Data3[1] = {0x27E0021};

u32 mpuInitRegion3Signature[1] = {0xEE060F13};
u32 mpuInitRegion3Data[1] = {0x8000035};

u32 mpuInitCache[1] = {0xE3A00042};

//
// Look in @data for @find and return the position of it.
//
u32 getOffset(u32* addr, size_t size, u32* find, size_t sizeofFind, int direction)
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

u32 generateA7Instr(int arg1, int arg2) {
    return (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer)
{
	dbg_printf("Looking for moduleparams\n");
	uint32_t moduleparams = getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize, (u32*)moduleParamsSignature, 2, 1);
	if(!moduleparams)
	{
		dbg_printf("No moduleparams?\n");
		moduleparams = malloc(0x100);
		memset(moduleparams,0,0x100);
		((module_params_t*)(moduleparams - 0x1C))->compressed_static_end = 0;
		switch(donorSdkVer) {
			case 0:
			default:
				break;
			case 1:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x1000500;
				break;
			case 2:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x2001000;
				break;
			case 3:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x3002001;
				break;
			case 4:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x4002001;
				break;
			case 5:
				((module_params_t*)(moduleparams - 0x1C))->sdk_version = 0x5003001;
				break;
		}
	}
	return (module_params_t*)(moduleparams - 0x1C);
}

void decompressLZ77Backwards(uint8_t* addr, size_t size)
{
	uint32_t leng = *((uint32_t*)(addr + size - 4)) + size;
	//byte[] Result = new byte[leng];
	//Array.Copy(Data, Result, Data.Length);
	uint32_t end = (*((uint32_t*)(addr + size - 8))) & 0xFFFFFF;
	uint8_t* result = addr;
	int Offs = (int)(size - ((*((uint32_t*)(addr + size - 8))) >> 24));
	int dstoffs = (int)leng;
	while (true)
	{
		uint8_t header = result[--Offs];
		for (int i = 0; i < 8; i++)
		{
			if ((header & 0x80) == 0) result[--dstoffs] = result[--Offs];
			else
			{
				uint8_t a = result[--Offs];
				uint8_t b = result[--Offs];
				int offs = (((a & 0xF) << 8) | b) + 2;//+ 1;
				int length = (a >> 4) + 2;
				do
				{
					result[dstoffs - 1] = result[dstoffs + offs];
					dstoffs--;
					length--;
				}
				while (length >= 0);
			}
			if (Offs <= size - end)
				return;
			header <<= 1;
		}
	}
}

void ensureArm9Decompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams)
{
	if(!moduleParams->compressed_static_end)
	{
		dbg_printf("This rom is not compressed\n");
		return; //not compressed
	}
	dbg_printf("This rom is compressed ;)\n");
	decompressLZ77Backwards((uint8_t*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
	moduleParams->compressed_static_end = 0;
}

u32 patchCardNdsArm9 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 patchMpuRegion, u32 patchMpuSize) {

	u32* debug = (u32*)0x03784000;
	debug[4] = ndsHeader->arm9destination;
	debug[8] = moduleParams->sdk_version;

	u32* a9cardReadSignature = a9cardReadSignature1;
	u32* cardReadStartSignature = cardReadStartSignature1;
	u32* cardPullOutSignature = cardPullOutSignature1;
	u32* cardReadCachedStartSignature = cardReadCachedStartSignature1;
	u32* cardReadCachedEndSignature = cardReadCachedEndSignature1;
	u32* mpuInitRegion2Data = mpuInitRegion2Data1;
	u32* mpuInitRegion1Data = mpuInitRegion1Data1;
	if(moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardReadCachedEndSignature = cardReadCachedEndSignature3;
		mpuInitRegion2Data = mpuInitRegion2Data3;
	} else if(moduleParams->sdk_version > 0x4000000) {
		a9cardReadSignature = a9cardReadSignature4;
		cardReadStartSignature = cardReadStartSignature4;
		cardPullOutSignature = cardPullOutSignature4;
		cardReadCachedStartSignature = cardReadCachedStartSignature4;
		cardReadCachedEndSignature = cardReadCachedEndSignature4;
		mpuInitRegion1Data = mpuInitRegion1Data4;
	} 

	u32* mpuInitRegionSignature = mpuInitRegion1Signature;
	u32* mpuInitRegionData = mpuInitRegion1Data;
	u32 mpuInitRegionNewData = PAGE_32M  | 0x02000000 | 1;
	u32 needFlushCache = 0;
	int mpuAccessOffset = 0;
	u32 mpuNewDataAccess = 0;
	u32 mpuNewInstrAccess = 0;

	switch(patchMpuRegion) {
		case 0 :
			mpuInitRegionSignature = mpuInitRegion0Signature;
			mpuInitRegionData = mpuInitRegion0Data;
			mpuInitRegionNewData = PAGE_128M  | 0x00000000 | 1;
			break;
		case 1 :
			mpuInitRegionSignature = mpuInitRegion1Signature;
			mpuInitRegionData = mpuInitRegion1Data;
			needFlushCache = 1;
			break;
		case 2 :
			mpuInitRegionSignature = mpuInitRegion2Signature;
			mpuInitRegionData = mpuInitRegion2Data;
			mpuNewDataAccess = 0x15111111;
			mpuNewInstrAccess = 0x5111111;
			mpuAccessOffset = 6;
			break;
		case 3 :
			mpuInitRegionSignature = mpuInitRegion3Signature;
			mpuInitRegionData = mpuInitRegion3Data;
			mpuInitRegionNewData = PAGE_8M  | 0x03000000 | 1;
			mpuNewInstrAccess = 0x5111111;
			mpuAccessOffset = 5;
			break;
	}

	// Find the card read
    u32 cardReadEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
              (u32*)a9cardReadSignature, 2, 1);
    if (!cardReadEndOffset) {
        dbg_printf("Card read end not found\n");
        return 0;
    }
	debug[1] = cardReadEndOffset;
    u32 cardReadStartOffset =   
        getOffset((u32*)cardReadEndOffset, -0xF9,
              (u32*)cardReadStartSignature, 1, -1);
    if (!cardReadStartOffset) {
        dbg_printf("Card read start not found\n");
        return 0;
    }
	dbg_printf("Arm9 Card read:\t");
	dbg_hexa(cardReadStartOffset);
	dbg_printf("\n");

	u32 cardPullOutOffset =   
        getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//, ndsHeader->arm9binarySize,
              (u32*)cardPullOutSignature, 4, 1);
    if (!cardPullOutOffset) {
        dbg_printf("Card pull out handler not found\n");
        return 0;
    }
	dbg_printf("Card pull out handler:\t");
	dbg_hexa(cardPullOutOffset);
	dbg_printf("\n");


    u32 cardReadCachedEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
              (u32*)cardReadCachedEndSignature, 4, 1);
    if (!cardReadCachedEndOffset) {
        dbg_printf("Card read cached end not found\n");
        return 0;
    }
    u32 cardReadCachedOffset =   
        getOffset((u32*)cardReadCachedEndOffset, -0xFF,
              (u32*)cardReadCachedStartSignature, 2, -1);
    if (!cardReadStartOffset) {
        dbg_printf("Card read cached start not found\n");
        return 0;
    }
	dbg_printf("Card read cached :\t");
	dbg_hexa(cardReadCachedOffset);
	dbg_printf("\n");

	u32 cardReadDmaOffset = 0;
	u32 cardReadDmaEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
              (u32*)cardReadDmaEndSignature, 2, 1);
    if (!cardReadDmaEndOffset) {
        dbg_printf("Card read dma end not found\n");
    } else {
		dbg_printf("Card read dma end :\t");
		dbg_hexa(cardReadDmaEndOffset);
		dbg_printf("\n");
		cardReadDmaOffset =   
			getOffset((u32*)cardReadDmaEndOffset, -0x200,
				  (u32*)cardReadDmaStartSignature, 1, -1);
		if (!cardReadDmaOffset) {
			dbg_printf("Card read dma start not found\n");
			cardReadDmaOffset =   
				getOffset((u32*)cardReadDmaEndOffset, -0x200,
					  (u32*)cardReadDmaStartSignatureAlt, 1, -1);
			if (!cardReadDmaOffset) {
				dbg_printf("Card read dma start alt not found\n");
			}
		}
		if (!cardReadDmaOffset) {
			//dbg_printf("Card read dma start not found\n");
			cardReadDmaOffset =   
				getOffset((u32*)cardReadDmaEndOffset, -0x200,
					  (u32*)cardReadDmaStartSignatureAlt2, 1, -1);
			if (!cardReadDmaOffset) {
				dbg_printf("Card read dma start alt2 not found\n");
			}
		}
	}    

	// Find the card id
	u32 cardIdStartOffset = 0;
    u32 cardIdEndOffset =  
        getOffset((u32*)cardReadEndOffset+0x10, ndsHeader->arm9binarySize,
              (u32*)a9cardIdSignature, 2, 1);
			  
	if(!cardIdEndOffset){
		cardIdEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)a9cardIdSignature, 2, 1);
	}
    if (!cardIdEndOffset) {
        dbg_printf("Card id end not found\n");
    } else {
		debug[1] = cardIdEndOffset;
		cardIdStartOffset =   
			getOffset((u32*)cardIdEndOffset, -0x100,
				  (u32*)cardIdStartSignature, 1, -1);
		if (!cardIdStartOffset) {
		cardIdStartOffset =   
			getOffset((u32*)cardIdEndOffset, -0x100,
				  (u32*)cardIdStartSignatureAlt, 1, -1);
		}
		if (!cardIdStartOffset) {
		cardIdStartOffset =   
			getOffset((u32*)cardIdEndOffset, -0x100,
				  (u32*)cardIdStartSignatureAlt2, 1, -1);
		}
		if (!cardIdStartOffset) {
			dbg_printf("Card id start not found\n");
		} else {
			dbg_printf("Card id :\t");
			dbg_hexa(cardIdStartOffset);
			dbg_printf("\n");
		}
	}

	// Find the mpu init
	u32* mpuDataOffset = 0;
    u32 mpuStartOffset =  
        getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)mpuInitRegionSignature, 1, 1);
    if (!mpuStartOffset) {
        dbg_printf("Mpu init not found\n");
    } else {
		mpuDataOffset =   
			getOffset((u32*)mpuStartOffset, 0x100,
				  (u32*)mpuInitRegionData, 1, 1);
		if (!mpuDataOffset) {
			dbg_printf("Mpu data not found\n");
		} else {
			dbg_printf("Mpu data :\t");
			dbg_hexa((u32)mpuDataOffset);
			dbg_printf("\n");
		}
	}

	if(!mpuDataOffset) {
		// try to found it
		for (int i = 0; i<0x100; i++) {
			mpuDataOffset = (u32*)(mpuStartOffset+i);
			if(((*mpuDataOffset) & 0xFFFFFF00) == 0x02000000) break;
		}
	}

	if(mpuDataOffset) {
		// change the region 1 configuration
		*mpuDataOffset = mpuInitRegionNewData;

		if(mpuAccessOffset) {
			if(mpuNewInstrAccess) {
				mpuDataOffset[mpuAccessOffset] = mpuNewInstrAccess;
			}
			if(mpuNewDataAccess) {
				mpuDataOffset[mpuAccessOffset+1] = mpuNewDataAccess;
			}
		}
	}

	// Find the mpu cache init
    /*u32* mpuCacheOffset =  
        getOffset((u32*)mpuStartOffset, 0x100,
              (u32*)mpuInitCache, 1, 1);
    if (!mpuCacheOffset) {
        dbg_printf("Mpu init cache not found\n");
    } else {
		*mpuCacheOffset = 0xE3A00046;
	}	*/

	dbg_printf("patchMpuSize :\t");
	dbg_hexa(patchMpuSize);
	dbg_printf("\n");

	// patch out all further mpu reconfiguration
	while(mpuStartOffset && patchMpuSize) {
		u32 patchSize = ndsHeader->arm9binarySize;
		if(patchMpuSize>1) {
			patchSize = patchMpuSize;
		}
		mpuStartOffset = getOffset(mpuStartOffset+4, patchSize,
              (u32*)mpuInitRegionSignature, 1, 1);
		if(mpuStartOffset) {
			dbg_printf("Mpu init :\t");
			dbg_hexa(mpuStartOffset);
			dbg_printf("\n");

			*((u32*)mpuStartOffset) = 0xE1A00000 ; // nop

			/*// try to found it
			for (int i = 0; i<0x100; i++) {
				mpuDataOffset = (u32*)(mpuStartOffset+i);
				if(((*mpuDataOffset) & 0xFFFFFF00) == 0x02000000) {
					*mpuDataOffset = PAGE_32M  | 0x02000000 | 1;
					break;
				}
				if(i == 100) {
					*((u32*)mpuStartOffset) = 0xE1A00000 ;
				}
			}*/
		}
	}

	/*u32 arenaLoOffset =   
        getOffsetA9((u32*)ndsHeader->arm9destination, 0x00300000,//, ndsHeader->arm9binarySize,
              (u32*)arenaLowSignature, 4, 1);
    if (!arenaLoOffset) {
        nocashMessage("Arenow low not found\n");
    } else {
		debug[0] = arenaLoOffset;
		nocashMessage("Arenow low found\n");

		arenaLoOffset += 0x88;
		debug[10] = arenaLoOffset;
		debug[11] = *((u32*)arenaLoOffset);

		u32* oldArenaLow = (u32*) *((u32*)arenaLoOffset);

		// *((u32*)arenaLoOffset) = *((u32*)arenaLoOffset) + 0x800; // shrink heap by 8 kb
		// *(vu32*)(0x027FFDA0) = *((u32*)arenaLoOffset);
		debug[12] = *((u32*)arenaLoOffset);

		u32 arenaLo2Offset =   
			getOffsetA9((u32*)ndsHeader->arm9destination, 0x00100000,//, ndsHeader->arm9binarySize,
				  oldArenaLow, 1, 1);

		// *((u32*)arenaLo2Offset) = *((u32*)arenaLo2Offset) + 0x800; // shrink heap by 8 kb

		debug[13] = arenaLo2Offset;
	}*/
	
	if(moduleParams->sdk_version > 0x3000000
	&& (*(u32*)(0x27FF00C) & 0x00FFFFFF) != 0x544B41	// Doctor Tendo
	&& (*(u32*)(0x27FF00C) & 0x00FFFFFF) != 0x5A4341	// Cars
	&& (*(u32*)(0x27FF00C) & 0x00FFFFFF) != 0x434241	// Harvest Moon DS
	&& (*(u32*)(0x27FF00C) & 0x00FFFFFF) != 0x4C5741)	// TWEWY
	{
		u32 randomPatchOffset =  
				getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
					  (u32*)aRandomPatch, 4, 1);
			if(randomPatchOffset){
				*(u32*)(randomPatchOffset+0xC) = 0x0;
			}
				if (!randomPatchOffset) {
					//dbg_printf("Random patch not found\n"); Don't bother logging it.
				}
	}

	debug[2] = cardEngineLocation;

	u32* patches =  (u32*) cardEngineLocation[0];

	cardEngineLocation[3] = moduleParams->sdk_version;

	u32* cardReadPatch = (u32*) patches[0];

	u32* cardPullOutPatch = patches[6];

	u32* cardIdPatch = patches[3];

	u32* cardDmaPatch = patches[4];

	debug[5] = patches;

	u32* card_struct = ((u32*)cardReadEndOffset) - 1;
	//u32* cache_struct = ((u32*)cardIdStartOffset) - 1;

	debug[6] = *card_struct;
	//debug[7] = *cache_struct;

	cardEngineLocation[5] = ((u32*)*card_struct)+6;
	if(moduleParams->sdk_version > 0x3000000) {
		cardEngineLocation[5] = ((u32*)*card_struct)+7;
	}
	//cardEngineLocation[6] = *cache_struct;

	// cache management alternative
	*((u32*)patches[5]) = ((u32*)*card_struct)+6;
	if(moduleParams->sdk_version > 0x3000000) {
		*((u32*)patches[5]) = ((u32*)*card_struct)+7;
	}

	*((u32*)patches[7]) = cardPullOutOffset+4;
	if(cardReadCachedOffset==0x020777F0){
		*((u32*)patches[8]) = cardReadCachedOffset-0x87E0; //NSMBDS fix.
	}else if(cardReadCachedOffset==0x021240E8){
		*((u32*)patches[8]) = 0x0211E1A8; //ACWW fix.
		cardIdStartOffset = 0x02123FF0; //ACWW fix part2.
	}else{
		*((u32*)patches[8]) = cardReadCachedOffset;
	}
	patches[10] = needFlushCache;

	//copyLoop (oldArenaLow, cardReadPatch, 0xF0);

	copyLoop ((u32*)cardReadStartOffset, cardReadPatch, 0xF0);

	copyLoop ((u32*)(cardPullOutOffset), cardPullOutPatch, 0x5C);

	if (cardIdStartOffset) {
		copyLoop ((u32*)cardIdStartOffset, cardIdPatch, 0x8);
	}

	if (cardReadDmaOffset) {
		dbg_printf("Card read dma :\t");
		dbg_hexa(cardReadDmaOffset);
		dbg_printf("\n");

		copyLoop ((u32*)cardReadDmaOffset, cardDmaPatch, 0x8);
	}

	dbg_printf("ERR_NONE");
	return 0;
}

u32 savePatchV2 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster) {

    dbg_printf("\nArm7 (patch v2.0)\n");

	// Find the relocation signature
    u32 relocationStart = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
        relocateStartSignature, 1, 1);
    if (!relocationStart) {
        dbg_printf("Relocation start not found\n");
		return 0;
    }

	// Validate the relocation signature
    u32 forwardedRelocStartAddr = relocationStart + 4;
    if (!*(u32*)forwardedRelocStartAddr)
        forwardedRelocStartAddr += 4;
    u32 vAddrOfRelocSrc =
        *(u32*)(forwardedRelocStartAddr + 8);
    // sanity checks
    u32 relocationCheck1 =
        *(u32*)(forwardedRelocStartAddr + 0xC);
    u32 relocationCheck2 =
        *(u32*)(forwardedRelocStartAddr + 0x10);
    if ( vAddrOfRelocSrc != relocationCheck1
      || vAddrOfRelocSrc != relocationCheck2) {
        dbg_printf("Error in relocation checking\n");
		return 0;
    }


    // Get the remaining details regarding relocation
    u32 valueAtRelocStart =
        *(u32*)forwardedRelocStartAddr;
    u32 relocDestAtSharedMem =
        *(u32*)valueAtRelocStart;
    if (relocDestAtSharedMem != 0x37F8000) { // shared memory in RAM
        // Try again
        vAddrOfRelocSrc +=
            *(u32*)(valueAtRelocStart + 4);
        relocDestAtSharedMem =
            *(u32*)(valueAtRelocStart + 0xC);
        if (relocDestAtSharedMem != 0x37F8000) {
            dbg_printf("Error in finding shared memory relocation area\n");
			return 0;
        }
    }

    dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");
	dbg_printf("Relocation dst: ");
	dbg_hexa(relocDestAtSharedMem);
	dbg_printf("\n");

    // Find the card read
    u32 cardReadEndAddr =
        getOffset((u32*)ndsHeader->arm7destination, 0x00400000, 
		a7cardReadSignature, 2, 1);
    if (!cardReadEndAddr) {
        dbg_printf("[Error!] Card read addr not found\n"); return 0;
    }

	dbg_printf("cardReadEndAddr: ");
	dbg_hexa(cardReadEndAddr);
	dbg_printf("\n");

	// nonsense variable names below
    u32 cardstructAddr = *(u32*)(cardReadEndAddr - 4);

	dbg_printf("cardstructAddr: ");
	dbg_hexa(cardstructAddr);
	dbg_printf("\n");

    u32 readCacheEnd =
         getOffset(cardReadEndAddr,
             0x18000 - cardReadEndAddr, &cardstructAddr, 1, 1);
			 
	dbg_printf("readCacheEnd: ");
	dbg_hexa(readCacheEnd);
	dbg_printf("\n");

    if (!readCacheEnd)
    {
        dbg_printf("[Error!] ___ addr not found\n"); return 0;
    }
    u32 JumpTableFunc = readCacheEnd + 4;

	dbg_printf("JumpTableFunc: ");
	dbg_hexa(JumpTableFunc);
	dbg_printf("\n");

	//
    // Here is where the differences in the retry begin
    //

	u32 returned_A0_with_MKDS =
        getOffset(JumpTableFunc, 0x100,
            (void*)a7something1Signature, 2, 1);
    if (!returned_A0_with_MKDS) {
        dbg_printf("[Error!]...\n");
        return 0;
    }

	dbg_printf("returned_A0_with_MKDS: ");
	dbg_hexa(returned_A0_with_MKDS);
	dbg_printf("\n");

    u32 addrOfSomething_85C0 =
        getOffset((u32*)ndsHeader->arm7destination, 0x18000,
            (void*)a7something2Signature, 2, 1);
    if ( !addrOfSomething_85C0 )
    {
        dbg_printf("[Error!] ...\n");
        return 0;
    }

	dbg_printf("addrOfSomething_85C0: ");
	dbg_hexa(addrOfSomething_85C0);
	dbg_printf("\n");

    u32 anotherLocinA7WRAM = *(u32*)(addrOfSomething_85C0 - 4);

	dbg_printf("anotherLocinA7WRAM: ");
	dbg_hexa(anotherLocinA7WRAM);
	dbg_printf("\n");

    u32 amal_8CBC = returned_A0_with_MKDS;

	dbg_printf("amal_8CBC: ");
	dbg_hexa((u32)amal_8CBC);
	dbg_printf("\n");

    // no, no idea what this is yet
    // and no idea how to cleanly fix this warning yet.
    // but it should be (in MKDS), 0x7F54
    u8* aFinalLocation =
        (u8*)(JumpTableFunc
        + 4 * (*(u32*)(JumpTableFunc + 0x38) & 0xFFFFFF)
        + 0x48
        + 4 * (*(u32*)((
                4 * (*(u32*)(JumpTableFunc + 0x38) & 0xFFFFFF) + 0x48
                ) + JumpTableFunc) | 0xFF000000
              )
        + 8);

	dbg_printf("aFinalLocation: ");
	dbg_hexa((u32)aFinalLocation);
	dbg_printf("\n");

	u32* patches =  (u32*) cardEngineLocation[0];
	u32* arm7Function =  (u32*) patches[9];
	u32 srcAddr;

	u32* eepromProtect = (u32*) (JumpTableFunc + 0xE0);
	u32* cardRead = (u32*) (JumpTableFunc + 0x108);
	if((((*eepromProtect) & 0xFF000000) == 0xEB000000) 
		&& (((*cardRead) & 0xFF000000) == 0xEB000000)) {
		dbg_printf("Eeprom protect:\t");
		dbg_hexa((u32)eepromProtect);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xE0 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchProtect = generateA7Instr(srcAddr,
			arm7Function[0] );
		*eepromProtect=patchProtect; 

		u32* cardId = (u32*) (JumpTableFunc + 0xE8);
		dbg_printf("Card id:\t");
		dbg_hexa((u32)cardId);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xE8 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchCardId = generateA7Instr(srcAddr,
			arm7Function[7]);
		*cardId=patchCardId;

		dbg_printf("Card  read:\t");
		dbg_hexa((u32)cardRead);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x108 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchCardRead = generateA7Instr(srcAddr,
			arm7Function[6]);
		*cardRead=patchCardRead;

		u32* eepromRead = (u32*) (JumpTableFunc + 0x120);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x120  - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchRead = generateA7Instr(srcAddr,
			arm7Function[5]);
		*eepromRead=patchRead;

		u32* eepromPageWrite = (u32*) (JumpTableFunc + 0x138);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x138 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchWrite = generateA7Instr(srcAddr,
			arm7Function[3]);
		*eepromPageWrite=patchWrite;

		u32* eepromPageProg = (u32*) (JumpTableFunc + 0x150);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x150 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchProg = generateA7Instr(srcAddr,
			arm7Function[4]);
		*eepromPageProg=patchProg;

		u32* eepromPageVerify = (u32*) (JumpTableFunc + 0x168);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x168 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchVerify = generateA7Instr(srcAddr,
			arm7Function[2]);
		*eepromPageVerify=patchVerify;


		u32* eepromPageErase = (u32*) (JumpTableFunc + 0x178);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x178 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchErase = generateA7Instr(srcAddr,
			arm7Function[1]);
		*eepromPageErase=patchErase; 

		arm7Function[8] = saveFileCluster;
	} else {
		dbg_printf("[Warning] Eeprom protect not found \n");
		cardRead = (u32*) (JumpTableFunc + 0x100);

		if(((*cardRead) & 0xFF000000) != 0xEB000000) {
			dbg_printf("[Error] CardRead not found:\n");
			dbg_hexa((u32)cardRead);
			dbg_printf("\n");
			return 0;
		}

		u32* cardId = (u32*) (JumpTableFunc + 0xE0);
		dbg_printf("Card id:\t");
		dbg_hexa((u32)cardId);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xE0 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchCardId = generateA7Instr(srcAddr,
			arm7Function[7]);
		*cardId=patchCardId;

		dbg_printf("Card  read:\t");
		dbg_hexa((u32)cardRead);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x100 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchCardRead = generateA7Instr(srcAddr,
			arm7Function[6]);
		*cardRead=patchCardRead;

		u32* eepromRead = (u32*) (JumpTableFunc + 0x118);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x118  - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchRead = generateA7Instr(srcAddr,
			arm7Function[5]);
		*eepromRead=patchRead;

		u32* eepromPageWrite = (u32*) (JumpTableFunc + 0x130);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x130 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchWrite = generateA7Instr(srcAddr,
			arm7Function[3]);
		*eepromPageWrite=patchWrite;

		u32* eepromPageProg = (u32*) (JumpTableFunc + 0x148);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x148 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchProg = generateA7Instr(srcAddr,
			arm7Function[4]);
		*eepromPageProg=patchProg;

		u32* eepromPageVerify = (u32*) (JumpTableFunc + 0x160);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x160 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchVerify = generateA7Instr(srcAddr,
			arm7Function[2]);
		*eepromPageVerify=patchVerify;


		u32* eepromPageErase = (u32*) (JumpTableFunc + 0x170);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x170 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchErase = generateA7Instr(srcAddr,
			arm7Function[1]);
		*eepromPageErase=patchErase; 

		arm7Function[8] = saveFileCluster;
	}    

	return 1;
}


u32 savePatchV1 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster ) {

    dbg_printf("\nArm7 (patch v1.0)\n");

	// Find the relocation signature
    u32 relocationStart = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
        relocateStartSignature, 1, 1);
    if (!relocationStart) {
        dbg_printf("Relocation start not found\n");
		return 0;
    }

	// Validate the relocation signature
    u32 forwardedRelocStartAddr = relocationStart + 4;
    if (!*(u32*)forwardedRelocStartAddr)
        forwardedRelocStartAddr += 4;
    u32 vAddrOfRelocSrc =
        *(u32*)(forwardedRelocStartAddr + 8);
    // sanity checks
    u32 relocationCheck1 =
        *(u32*)(forwardedRelocStartAddr + 0xC);
    u32 relocationCheck2 =
        *(u32*)(forwardedRelocStartAddr + 0x10);
    if ( vAddrOfRelocSrc != relocationCheck1
      || vAddrOfRelocSrc != relocationCheck2) {
        dbg_printf("Error in relocation checking\n");
		return 0;
    }


    // Get the remaining details regarding relocation
    u32 valueAtRelocStart =
        *(u32*)forwardedRelocStartAddr;
    u32 relocDestAtSharedMem =
        *(u32*)valueAtRelocStart;
    if (relocDestAtSharedMem != 0x37F8000) { // shared memory in RAM
        // Try again
        vAddrOfRelocSrc +=
            *(u32*)(valueAtRelocStart + 4);
        relocDestAtSharedMem =
            *(u32*)(valueAtRelocStart + 0xC);
        if (relocDestAtSharedMem != 0x37F8000) {
            dbg_printf("Error in finding shared memory relocation area\n");
			return 0;
        }
    }

    dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");
	dbg_printf("Relocation dst: ");
	dbg_hexa(relocDestAtSharedMem);
	dbg_printf("\n");

    // Find the card read
    u32 cardReadEndAddr =
        getOffset((u32*)ndsHeader->arm7destination, 0x00400000, 
		a7cardReadSignature, 2, 1);
    if (!cardReadEndAddr) {
        dbg_printf("[Error!] Card read addr not found\n"); return 0;
    }

	dbg_printf("cardReadEndAddr: ");
	dbg_hexa(cardReadEndAddr);
	dbg_printf("\n");

	// nonsense variable names below
    u32 cardstructAddr = *(u32*)(cardReadEndAddr - 4);

	dbg_printf("cardstructAddr: ");
	dbg_hexa(cardstructAddr);
	dbg_printf("\n");

    u32 readCacheEnd =
         getOffset(cardReadEndAddr,
             0x18000 - cardReadEndAddr, &cardstructAddr, 1, 1);
			 
	dbg_printf("readCacheEnd: ");
	dbg_hexa(readCacheEnd);
	dbg_printf("\n");

    if (!readCacheEnd)
    {
        dbg_printf("[Error!] ___ addr not found\n"); return 0;
    }
    u32 JumpTableFunc = readCacheEnd + 4;

	dbg_printf("JumpTableFunc: ");
	dbg_hexa(JumpTableFunc);
	dbg_printf("\n");

	//
    // Here is where the differences in the retry begin
    //

    u32 specificWramAddr = *(u32*)(JumpTableFunc + 0x10);
    // if out of specific ram range...
    if (specificWramAddr < 0x37F8000 || specificWramAddr > 0x380FFFF) {
		dbg_printf("Retry the search\n");
        JumpTableFunc =
            getOffset(JumpTableFunc,
              0x18000 - JumpTableFunc, &cardstructAddr, 1, 1) + 4;
		dbg_printf("JumpTableFunc: ");
		dbg_hexa(JumpTableFunc);
		dbg_printf("\n");	  
        specificWramAddr = *(u32*)(JumpTableFunc + 0x10);
		if (specificWramAddr < 0x37F8000 || specificWramAddr > 0x380FFFF) {
			return 0;
		}
    }

	dbg_printf("specificWramAddr: ");
	dbg_hexa(specificWramAddr);
	dbg_printf("\n");

    u32 someAddr_799C = getOffset((u32*)ndsHeader->arm7destination, 0x18000, a7something2Signature,
        2, 1);
    if (!someAddr_799C) {
        dbg_printf("[Error!] ___ someOffset not found\n"); return 0;
    }

	dbg_printf("someAddr_799C: ");
	dbg_hexa(someAddr_799C);
	dbg_printf("\n");

	u32* patches =  (u32*) cardEngineLocation[0];
	u32* arm7Function =  (u32*) patches[9];

	u32* eepromPageErase = (u32*) (JumpTableFunc + 0x10);
    dbg_printf("Eeprom page erase:\t");
	dbg_hexa((u32)eepromPageErase);
	dbg_printf("\n");
	*eepromPageErase=arm7Function[1];

	u32* eepromPageVerify = (u32*) (JumpTableFunc + 0x2C);
	dbg_printf("Eeprom verify:\t");
	dbg_hexa((u32)eepromPageVerify);
	dbg_printf("\n");
	*eepromPageVerify=arm7Function[2];

	u32* eepromPageWrite = (u32*) (JumpTableFunc + 0x48);
	dbg_printf("Eeprom page write:\t");
	dbg_hexa((u32)eepromPageWrite);
	dbg_printf("\n");
	*eepromPageWrite=arm7Function[3];

	u32* eepromPageProg = (u32*) (JumpTableFunc + 0x64);
	dbg_printf("Eeprom page prog:\t");
	dbg_hexa((u32)eepromPageProg);
	dbg_printf("\n");
	*eepromPageProg=arm7Function[4];

	u32* eepromRead = (u32*) (JumpTableFunc + 0x80);
	dbg_printf("Eeprom read:\t");
	dbg_hexa((u32)eepromRead);
	dbg_printf("\n");
	*eepromRead=arm7Function[5];

	u32* cardRead = (u32*) (JumpTableFunc + 0xA0);
	dbg_printf("Card  read:\t");
	dbg_hexa((u32)cardRead);
	dbg_printf("\n");
	*cardRead=arm7Function[6];

	// different patch for card id
	u32* cardId = (u32*) (JumpTableFunc + 0xAC);
	dbg_printf("Card id:\t");
	dbg_hexa((u32)cardId);
	dbg_printf("\n");
	u32 srcAddr = JumpTableFunc + 0xAC - vAddrOfRelocSrc + relocDestAtSharedMem ;
	u32 patchCardID = generateA7Instr(srcAddr,
        arm7Function[7]);
	*cardId=patchCardID; 

	u32 anotherWramAddr = *(u32*)(JumpTableFunc + 0xD0);
    if (anotherWramAddr > 0x37F7FFF && anotherWramAddr < 0x3810000) {
        u32* current = (u32*)(JumpTableFunc + 0xD0);
        dbg_printf("???:\t\t\t");
		dbg_hexa((u32)current);
		dbg_printf("\n");

		*current=arm7Function[0];
    }

	arm7Function[8] = saveFileCluster;

	dbg_printf("Arm7 patched!\n");

    return 1;
}

void swapBinary_ARM7(aFile donorfile)
{
	u32 ndsHeader[0x170>>2];

	nocashMessage("loadBinary_ARM7");

	// read NDS header
	fileRead ((char*)ndsHeader, donorfile, 0, 0x170);
	// read ARM7 info from NDS header
	u32 ARM7_SRC = ndsHeader[0x030>>2];
	char* ARM7_DST = (char*)ndsHeader[0x038>>2];
	u32 ARM7_LEN = ndsHeader[0x03C>>2];

	fileRead(ARM7_DST, donorfile, ARM7_SRC, ARM7_LEN);

	NDS_HEAD[0x030>>2] = ARM7_SRC;
	NDS_HEAD[0x038>>2] = ARM7_DST;
	NDS_HEAD[0x03C>>2] = ARM7_LEN;
}

u32 patchCardNdsArm7 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, aFile donorFile, u32 useArm7Donor) {
	u32* debug = (u32*)0x03784000;

	u32* irqEnableStartSignature = irqEnableStartSignature1;
	u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
	if(moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	if(moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32 cardCheckPullOutOffset =   
        getOffset((u32*)ndsHeader->arm7destination, 0x00400000,//, ndsHeader->arm9binarySize,
              (u32*)cardCheckPullOutSignature, 4, 1);
    if (!cardCheckPullOutOffset) {
        dbg_printf("Card check pull out not found\n");
        //return 0;
    } else {
		debug[0] = cardCheckPullOutOffset;
		dbg_printf("Card check pull out found\n");
	}

	u32 cardIrqEnableOffset =   
        getOffset((u32*)ndsHeader->arm7destination, 0x00400000,//, ndsHeader->arm9binarySize,
              (u32*)irqEnableStartSignature, 4, 1);
    if (!cardIrqEnableOffset) {
        dbg_printf("irq enable not found\n");
        return 0;
    }
	debug[0] = cardIrqEnableOffset;
    dbg_printf("irq enable found\n");


	cardEngineLocation[3] = moduleParams->sdk_version;

	u32* patches =  (u32*) cardEngineLocation[0];
	u32* cardIrqEnablePatch = (u32*) patches[2];
	u32* cardCheckPullOutPatch = (u32*) patches[1];

	if(cardCheckPullOutOffset>0)
		copyLoop ((u32*)cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);

	copyLoop ((u32*)cardIrqEnableOffset, cardIrqEnablePatch, 0x30);

	u32 saveResult = 0;
	if (useArm7Donor == 2) {
		if ((donorFile.firstCluster >= CLUSTER_FIRST) && (donorFile.firstCluster < CLUSTER_EOF)) {			
			dbg_printf("swap the arm7 binary");	
			swapBinary_ARM7(donorFile);
			// apply the arm7 binary swap and the save patch, assume save v2 nds file
			saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster);
		} else {
			dbg_printf("no arm7 binary specified for swapping");	
		}
	} else if (useArm7Donor == 1) {
		saveResult = savePatchV1(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster);
		if(!saveResult) saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster);
		if(!saveResult) {
			if ((donorFile.firstCluster >= CLUSTER_FIRST) && (donorFile.firstCluster < CLUSTER_EOF)) {			
				dbg_printf("swap the arm7 binary");	
				swapBinary_ARM7(donorFile);
				// apply the arm7 binary swap and the save patch again, assume save v2 nds file
				saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster);
			} else {
				dbg_printf("no arm7 binary specified for swapping");	
			}
		}
	} else {
		saveResult = savePatchV1(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster);
		if(!saveResult) saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster);
	}

	dbg_printf("ERR_NONE");
	return 0;
}

u32 patchCardNds (const tNDSHeader* ndsHeader, u32* cardEngineLocationArm7, u32* cardEngineLocationArm9, module_params_t* moduleParams, 
		u32 saveFileCluster, u32 patchMpuRegion, u32 patchMpuSize, aFile donorFile, u32 useArm7Donor) {

	//Debug stuff.

	/*aFile myDebugFile = getBootFileCluster ("NDSBTSR2.LOG");
	enableDebug(myDebugFile);*/

	dbg_printf("patchCardNds");

	patchCardNdsArm9(ndsHeader, cardEngineLocationArm9, moduleParams, patchMpuRegion, patchMpuSize);
	patchCardNdsArm7(ndsHeader, cardEngineLocationArm7, moduleParams, saveFileCluster, donorFile, useArm7Donor);

	dbg_printf("ERR_NONE");
	return 0;
}
