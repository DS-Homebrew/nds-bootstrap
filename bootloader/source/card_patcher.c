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

#include <nds/system.h>
#include "card_patcher.h"
#include "common.h"
#include "cardengine_arm9_bin.h"
#include "cardengine_arm7_bin.h"
#include "debugToFile.h"

extern u32 ROM_TID;

// Subroutine function signatures arm7
u32 relocateStartSignature[1]  = {0x027FFFFA};
u32 nextFunctiontSignature[1]  = {0xE92D4000};
u32 a7cardReadSignature[2]     = {0x04100010,0x040001A4};
u32 a7something1Signature[2]   = {0xE350000C,0x908FF100};
u32 a7something2Signature[2]   = {0x0000A040,0x040001A0};

u32 a7JumpTableSignature[4] = {0xE5950024,0xE3500000,0x13A00001,0x03A00000};
u32 a7JumpTableSignatureV3_1[3] = {0xE92D4FF0,0xE24DD004,0xE59F91F8};
u32 a7JumpTableSignatureV3_2[3] = {0xE92D4FF0,0xE24DD004,0xE59F91D4};
u32 a7JumpTableSignatureV4_1[3] = {0xE92D41F0,0xE59F4224,0xE3A05000}; 
u32 a7JumpTableSignatureV4_2[3] = {0xE92D41F0,0xE59F4200,0xE3A05000};

u32 a7JumpTableSignatureUniversal[3] = {0xE592000C,0xE5921010,0xE5922014};
u32 a7JumpTableSignatureUniversal_pt2[3] = {0xE5920010,0xE592100C,0xE5922014};
u32 a7JumpTableSignatureUniversal_pt3[2] = {0xE5920010,0xE5921014};
u32 a7JumpTableSignatureUniversal_2[3] = {0xE593000C,0xE5931010,0xE5932014};
u32 a7JumpTableSignatureUniversal_2_pt2[3] = {0xE5930010,0xE593100C,0xE5932014};
u32 a7JumpTableSignatureUniversal_2_pt3[2] = {0xE5930010,0xE5931014};
u32 a7JumpTableSignatureUniversalThumb[2] = {0x68D06822,0x69526911};
u32 a7JumpTableSignatureUniversalThumb_pt2[2] = {0x69106822,0x695268D1};
u32 a7JumpTableSignatureUniversalThumb_pt3[1] = {0x69496908};
u32 a7JumpTableSignatureUniversalThumbAlt[1] = {0x691168D0};


u32 j_HaltSignature1[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00007BAF};
u32 j_HaltSignature1Alt1[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B7A3};
u32 j_HaltSignature1Alt2[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B837};
u32 j_HaltSignature1Alt3[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000B92B};
u32 j_HaltSignature1Alt4[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x0000BAEB};
u32 j_HaltSignature1Alt5[3] = {0xE59FC000, 0xE12FFF1C, 0x03803B93};
u32 j_HaltSignature1Alt6[3] = {0xE59FC000, 0xE12FFF1C, 0x03803DAF};
u32 j_HaltSignature1Alt7[3] = {0xE59FC000, 0xE12FFF1C, 0x03803DB3};
u32 j_HaltSignature1Alt8[3] = {0xE59FC000, 0xE12FFF1C, 0x03803ECB};
u32 j_HaltSignature1Alt9[3] = {0xE59FC000, 0xE12FFF1C, 0x03803F13};
u32 j_HaltSignature1Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03804133};
u32 j_HaltSignature3[3] = {0xE59FC000, 0xE12FFF1C, 0x03800F7F};
u32 j_HaltSignature3Alt1[3] = {0xE59FC000, 0xE12FFF1C, 0x038010F3};
u32 j_HaltSignature3Alt2[3] = {0xE59FC000, 0xE12FFF1C, 0x038011BF};
u32 j_HaltSignature3Alt3[3] = {0xE59FC000, 0xE12FFF1C, 0x03803597};
u32 j_HaltSignature3Alt4[3] = {0xE59FC000, 0xE12FFF1C, 0x038040C3};
u32 j_HaltSignature3Alt5[3] = {0xE59FC000, 0xE12FFF1C, 0x038042AB};
u32 j_HaltSignature3Alt6[3] = {0xE59FC000, 0xE12FFF1C, 0x038042AF};
u32 j_HaltSignature3Alt7[3] = {0xE59FC000, 0xE12FFF1C, 0x0380433F};
u32 j_HaltSignature3Alt8[3] = {0xE59FC000, 0xE12FFF1C, 0x038043E3};
u32 j_HaltSignature3Alt9[3] = {0xE59FC000, 0xE12FFF1C, 0x03804503};
u32 j_HaltSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038045BF};
u32 j_HaltSignature3Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x0380538B};
u32 j_HaltSignature4[3] = {0xE59FC000, 0xE12FFF1C, 0x0380064B};
u32 j_HaltSignature4Alt1[3] = {0xE59FC000, 0xE12FFF1C, 0x038008C3};
u32 j_HaltSignature4Alt2[3] = {0xE59FC000, 0xE12FFF1C, 0x038008CF};
u32 j_HaltSignature4Alt3[3] = {0xE59FC000, 0xE12FFF1C, 0x0380356F};
u32 j_HaltSignature4Alt4[3] = {0xE59FC000, 0xE12FFF1C, 0x038036BF};
u32 j_HaltSignature4Alt5[3] = {0xE59FC000, 0xE12FFF1C, 0x038037D3};
u32 j_HaltSignature4Alt6[3] = {0xE59FC000, 0xE12FFF1C, 0x03803E7F};
u32 j_HaltSignature4Alt7[3] = {0xE59FC000, 0xE12FFF1C, 0x03803EBF};
u32 j_HaltSignature4Alt8[3] = {0xE59FC000, 0xE12FFF1C, 0x03804503};

u32 swi12Signature[1] = {0x4770DF12};	// LZ77UnCompReadByCallbackWrite16bit
u32 j_GetPitchTableSignature1[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004721};
u32 j_GetPitchTableSignature1Alt1[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BB9};
u32 j_GetPitchTableSignature1Alt2[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BC9};
u32 j_GetPitchTableSignature1Alt3[4] = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BE5};
u32 j_GetPitchTableSignature1Alt4[3] = {0xE59FC000, 0xE12FFF1C, 0x03803BE9};
u32 j_GetPitchTableSignature1Alt5[3] = {0xE59FC000, 0xE12FFF1C, 0x03803E05};
u32 j_GetPitchTableSignature1Alt6[3] = {0xE59FC000, 0xE12FFF1C, 0x03803E09};
u32 j_GetPitchTableSignature1Alt7[3] = {0xE59FC000, 0xE12FFF1C, 0x03803F21};
u32 j_GetPitchTableSignature1Alt8[3] = {0xE59FC000, 0xE12FFF1C, 0x03804189};
u32 j_GetPitchTableSignature3[3] = {0xE59FC000, 0xE12FFF1C, 0x03800FD5};
u32 j_GetPitchTableSignature3Alt1[3] = {0xE59FC000, 0xE12FFF1C, 0x03801149};
u32 j_GetPitchTableSignature3Alt2[3] = {0xE59FC000, 0xE12FFF1C, 0x03801215};
u32 j_GetPitchTableSignature3Alt3[3] = {0xE59FC000, 0xE12FFF1C, 0x03804119};
u32 j_GetPitchTableSignature3Alt4[3] = {0xE59FC000, 0xE12FFF1C, 0x03804301};
u32 j_GetPitchTableSignature3Alt5[3] = {0xE59FC000, 0xE12FFF1C, 0x03804305};
u32 j_GetPitchTableSignature3Alt6[3] = {0xE59FC000, 0xE12FFF1C, 0x03804395};
u32 j_GetPitchTableSignature3Alt7[3] = {0xE59FC000, 0xE12FFF1C, 0x03804439};
u32 j_GetPitchTableSignature3Alt8[3] = {0xE59FC000, 0xE12FFF1C, 0x03804559};
u32 j_GetPitchTableSignature3Alt9[3] = {0xE59FC000, 0xE12FFF1C, 0x03804615};
u32 j_GetPitchTableSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038053E1};
u32 j_GetPitchTableSignature4[3] = {0xE59FC000, 0xE12FFF1C, 0x038006A1};
u32 j_GetPitchTableSignature4Alt1[3] = {0xE59FC000, 0xE12FFF1C, 0x03800919};
u32 j_GetPitchTableSignature4Alt2[3] = {0xE59FC000, 0xE12FFF1C, 0x03800925};
u32 j_GetPitchTableSignature4Alt3[3] = {0xE59FC000, 0xE12FFF1C, 0x038035C5};
u32 j_GetPitchTableSignature4Alt4[3] = {0xE59FC000, 0xE12FFF1C, 0x038035ED};
u32 j_GetPitchTableSignature4Alt5[3] = {0xE59FC000, 0xE12FFF1C, 0x03803715};
u32 j_GetPitchTableSignature4Alt6[3] = {0xE59FC000, 0xE12FFF1C, 0x03803829};
u32 j_GetPitchTableSignature4Alt7[3] = {0xE59FC000, 0xE12FFF1C, 0x03803ED5};
u32 j_GetPitchTableSignature4Alt8[3] = {0xE59FC000, 0xE12FFF1C, 0x03803F15};
u32 swiGetPitchTableSignature5[4] = {0x781A4B06, 0xD3030791, 0xD20106D1, 0x1A404904};

// Subroutine function signatures arm9
u32 moduleParamsSignature[2]   = {0xDEC00621, 0x2106C0DE};

// sdk < 4 version
u32 a9cardReadSignature1[2]    = {0x04100010, 0x040001A4};
u32 a9cardReadSignatureThumb[2]    = {0x040001A4, 0x00000200};
u32 a9cardReadSignatureThumbAlt1[2]    = {0xFFFFFE00, 0x040001A4};
u32 cardReadStartSignature1[1] = {0xE92D4FF0};
u32 cardReadStartSignatureThumb[1] = {0xB082B5F8};
u32 cardReadStartSignatureThumbAlt1[1] = {0xB083B5F0};

// sdk > 4 version
u32 a9cardReadSignature4[2]    = {0x040001A4, 0x04100010};
u32 cardReadStartSignature4[1] = {0xE92D4070};

u32 a9cardIdSignature[2]      = {0x040001A4,0x04100010};
u32 a9cardIdSignatureThumb[3]    = {0xF8FFFFFF, 0x040001A4, 0x04100010};
u32 cardIdStartSignature[1]   = {0xE92D4000};
u32 cardIdStartSignatureAlt[1]   = {0xE92D4008};
u32 cardIdStartSignatureAlt2[1]   = {0xE92D4010};
u32 cardIdStartSignatureThumb[1]   = {0xB081B500};
u32 cardIdStartSignatureThumbAlt[1]   = {0x202EB508};
u32 cardIdStartSignatureThumbAlt2[1]   = {0x20B8B508};
u32 cardIdStartSignatureThumbAlt3[1]   = {0x24B8B510};
  
//u32 a9instructionBHI[1]       = {0x8A000001};
u32 cardPullOutSignature1[4]   = {0xE92D4000,0xE24DD004,0xE201003F,0xE3500011};
u32 cardPullOutSignature4[4]   = {0xE92D4008,0xE201003F,0xE3500011,0x1A00000D};
u32 cardPullOutSignatureThumb[2]   = {0x203FB508,0x28114008};
//u32 a9cardSendSignature[7]    = {0xE92D40F0,0xE24DD004,0xE1A07000,0xE1A06001,0xE1A01007,0xE3A0000E,0xE3A02000};
u32 cardCheckPullOutSignature1[4]   = {0xE92D4018,0xE24DD004,0xE59F204C,0xE1D210B0};
u32 cardCheckPullOutSignature3[4]   = {0xE92D4000,0xE24DD004,0xE59F002C,0xE1D000B0};

u32 cardReadDmaStartSignature[1]   = {0xE92D4FF8};
u32 cardReadDmaStartSignatureAlt[1]   = {0xE92D47F0};
u32 cardReadDmaStartSignatureAlt2[1]   = {0xE92D4FF0};
u32 cardReadDmaStartSignatureThumb1[1]   = {0xB083B5F0};
u32 cardReadDmaStartSignatureThumb3[1]   = {0xB084B5F8};
u32 cardReadDmaEndSignature[2]   = {0x01FF8000,0x000001FF};     
u32 cardReadDmaEndSignatureThumbAlt[2]   = {0x01FF8000,0x02000000};     

u32 aRandomPatch[4] = {0xE3500000, 0x1597002C, 0x10406004,0x03E06000};
u32 sleepPatch[2] = {0x0A000001, 0xE3A00601}; 
u32 sleepPatchThumb[1] = {0x4831D002}; 
u32 sleepPatchThumbAlt[1] = {0x0440D002}; 


     
// irqEnable
u32 irqEnableStartSignature1[4] = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0};
u32 irqEnableStartSignature4[4] = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020};

//u32 arenaLowSignature[4] = {0xE1A00100,0xE2800627,0xE2800AFF,0xE5801DA0};  

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

bool cardReadFound = false;

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

void generateA7InstrThumb(u16* instrs, int arg1, int arg2) {
    // 23 bit offset
    u32 offset = (u32)(arg2 - arg1 - 4);
    //dbg_printf("generateA7InstrThumb offset\n");
    //dbg_hexa(offset);
    // 1st instruction contains the upper 11 bit of the offset
    instrs[0] = (( offset >> 12) & 0x7FF) | 0xF000;

    // 2nd instruction contains the lower 11 bit of the offset
    instrs[1] = (( offset >> 1) & 0x7FF) | 0xF800;
}


module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer)
{
	dbg_printf("Looking for moduleparams\n");
	uint32_t moduleparams = getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize, (u32*)moduleParamsSignature, 2, 1);
	*(vu32*)(0x2800008) = (getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize, (u32*)moduleParamsSignature, 2, 1) - 0x8);
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
	*(vu32*)(0x280000C) = moduleParams->compressed_static_end;
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

	u32* debug = (u32*)0x037C6000;
	debug[4] = ndsHeader->arm9destination;
	debug[8] = moduleParams->sdk_version;

	u32* a9cardReadSignature = a9cardReadSignature1;
	u32* cardReadStartSignature = cardReadStartSignature1;
	u32* cardPullOutSignature = cardPullOutSignature1;
	u32* mpuInitRegion2Data = mpuInitRegion2Data1;
	u32* mpuInitRegion1Data = mpuInitRegion1Data1;
	if(moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		mpuInitRegion2Data = mpuInitRegion2Data3;
	} else if(moduleParams->sdk_version > 0x4000000) {
		a9cardReadSignature = a9cardReadSignature4;
		cardReadStartSignature = cardReadStartSignature4;
		cardPullOutSignature = cardPullOutSignature4;
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

	bool usesThumb = false;

	// Find the card read
	u32 cardReadEndOffset = 0;
	if (ROM_TID == 0x45524F55) {
		// Start at 0x3800 for "WarioWare: DIY (USA)"
		cardReadEndOffset =  
		getOffset((u32*)ndsHeader->arm9destination+0x3800, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)a9cardReadSignature, 2, 1);
	} else {
		cardReadEndOffset =  
		getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)a9cardReadSignature, 2, 1);
	}
	if (!cardReadEndOffset) {
		dbg_printf("Card read end not found. Trying thumb\n");
		usesThumb = true;
		cardReadEndOffset =  
			getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
				(u32*)a9cardReadSignatureThumb, 2, 1);
	}
	if (!cardReadEndOffset) {
		dbg_printf("Thumb card read end not found\n");
		cardReadEndOffset =  
		getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
			(u32*)a9cardReadSignatureThumbAlt1, 2, 1);
	}
	if (!cardReadEndOffset) {
		dbg_printf("Thumb card read end alt 1 not found\n");
		return 0;
	}
	debug[1] = cardReadEndOffset;
    u32 cardReadStartOffset = 0;
	cardReadStartOffset =   
		getOffset((u32*)cardReadEndOffset, -0x109,
			  (u32*)cardReadStartSignature, 1, -1);
	if (!cardReadStartOffset) {
		dbg_printf("Card read start not found. Trying thumb\n");
		cardReadStartOffset =   
			getOffset((u32*)cardReadEndOffset, -0xC0,
				(u32*)cardReadStartSignatureThumb, 1, -1);
		if (!usesThumb) {
			cardReadEndOffset -= 0x4;
			usesThumb = true;
		}
	}
	if (!cardReadStartOffset) {
		dbg_printf("Thumb card read start not found\n");
		cardReadStartOffset =   
			getOffset((u32*)cardReadEndOffset, -0xC0,
				(u32*)cardReadStartSignatureThumbAlt1, 1, -1);
	}
	if (!cardReadStartOffset) {
		dbg_printf("Thumb card read start alt 1 not found\n");
		return 0;
	}
	cardReadFound = true;
	dbg_printf("Arm9 Card read:\t");
	dbg_hexa(cardReadStartOffset);
	dbg_printf("\n");

	u32 cardPullOutOffset = 0;
	if (usesThumb) {
		cardPullOutOffset = 
			getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//, ndsHeader->arm9binarySize,
				(u32*)cardPullOutSignatureThumb, 2, 1);
		if (!cardPullOutOffset) {
			dbg_printf("Thumb card pull out handler not found\n");
			//return 0;
		} else {
			dbg_printf("Thumb card pull out handler:\t");
			dbg_hexa(cardPullOutOffset);
			dbg_printf("\n");
		}
	} else {
		cardPullOutOffset = 
			getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//, ndsHeader->arm9binarySize,
				(u32*)cardPullOutSignature, 4, 1);
		if (!cardPullOutOffset) {
			dbg_printf("Card pull out handler not found\n");
			//return 0;
		} else {
			dbg_printf("Card pull out handler:\t");
			dbg_hexa(cardPullOutOffset);
			dbg_printf("\n");
		}
	}

	u32 cardReadDmaOffset = 0;
	u32 cardReadDmaEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
              (u32*)cardReadDmaEndSignature, 2, 1);
    if (!cardReadDmaEndOffset) {
        dbg_printf("Card read dma end not found\n");
	}
    if (!cardReadDmaEndOffset && usesThumb) {
        dbg_printf("Trying thumb alt\n");
		cardReadDmaEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, 0x00300000,//ndsHeader->arm9binarySize,
              (u32*)cardReadDmaEndSignatureThumbAlt, 2, 1);
	}
    if (!cardReadDmaEndOffset && usesThumb) {
        dbg_printf("Thumb card read dma end alt not found\n");
	}
	if (cardReadDmaEndOffset>0) {
		dbg_printf("Card read dma end :\t");
		dbg_hexa(cardReadDmaEndOffset);
		dbg_printf("\n");
		if (usesThumb) {
			//dbg_printf("Card read dma start not found\n");
			cardReadDmaOffset =   
				getOffset((u32*)cardReadDmaEndOffset, -0x100,
					  (u32*)cardReadDmaStartSignatureThumb1, 1, -1);
			if (!cardReadDmaOffset) {
				dbg_printf("Thumb card read dma start 1 not found\n");
				cardReadDmaOffset =   
				getOffset((u32*)cardReadDmaEndOffset, -0x200,
					  (u32*)cardReadDmaStartSignatureThumb3, 1, -1);
			}
			if (!cardReadDmaOffset) {
				dbg_printf("Thumb card read dma start 3 not found\n");
			}
		} else {
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
	}    

	// Find the card id
	u32 cardIdStartOffset = 0;
    u32 cardIdEndOffset =  
        getOffset((u32*)cardReadEndOffset+0x10, ndsHeader->arm9binarySize,
              (u32*)a9cardIdSignature, 2, 1);
			  
	if(!cardIdEndOffset && !usesThumb){
		cardIdEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)a9cardIdSignature, 2, 1);
	}
	if(!cardIdEndOffset && usesThumb){
		cardIdEndOffset =  
        getOffset((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)a9cardIdSignatureThumb, 3, 1);
	}
    if (!cardIdEndOffset) {
        dbg_printf("Card id end not found\n");
    } else {
		debug[1] = cardIdEndOffset;
		cardIdStartOffset =   
			getOffset((u32*)cardIdEndOffset, -0x100,
				  (u32*)cardIdStartSignature, 1, -1);
		if (usesThumb) {
			if (!cardIdStartOffset) {
			cardIdStartOffset =   
				getOffset((u32*)cardIdEndOffset, -0x40,
					  (u32*)cardIdStartSignatureThumb, 1, -1);
			}
			if (!cardIdStartOffset) {
			cardIdStartOffset =   
				getOffset((u32*)cardIdEndOffset, -0x40,
					  (u32*)cardIdStartSignatureThumbAlt, 1, -1);
			}
			if (!cardIdStartOffset) {
			cardIdStartOffset =   
				getOffset((u32*)cardIdEndOffset, -0x40,
					  (u32*)cardIdStartSignatureThumbAlt2, 1, -1);
			}
			if (!cardIdStartOffset) {
			cardIdStartOffset =   
				getOffset((u32*)cardIdEndOffset, -0x40,
					  (u32*)cardIdStartSignatureThumbAlt3, 1, -1);
			}
		} else {
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
		
		*(vu32*)(0x2800000) = mpuDataOffset;
		*(vu32*)(0x2800004) = *mpuDataOffset;
		
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

	u32* patches = 0;
	if (usesThumb) {
		patches = (u32*) cardEngineLocation[1];
	} else {
		patches = (u32*) cardEngineLocation[0];
	}

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

	patches[10] = needFlushCache;

	//copyLoop (oldArenaLow, cardReadPatch, 0xF0);

	if (usesThumb) {
		copyLoop ((u32*)cardReadStartOffset, cardReadPatch, 0xA0);
	} else {
		copyLoop ((u32*)cardReadStartOffset, cardReadPatch, 0xF0);
	}

	copyLoop ((u32*)(cardPullOutOffset), cardPullOutPatch, 0x4);

	if (cardIdStartOffset) {
		if (usesThumb) {
			copyLoop ((u32*)cardIdStartOffset, cardIdPatch, 0x4);
		} else {
			copyLoop ((u32*)cardIdStartOffset, cardIdPatch, 0x8);
		}
	}

	if (cardReadDmaOffset) {
		dbg_printf("Card read dma :\t");
		dbg_hexa(cardReadDmaOffset);
		dbg_printf("\n");

		if (usesThumb) {
			copyLoop ((u32*)cardReadDmaOffset, cardDmaPatch, 0x4);
		} else {
			copyLoop ((u32*)cardReadDmaOffset, cardDmaPatch, 0x8);
		}
	}

	dbg_printf("ERR_NONE");
	return 0;
}

u32 savePatchUniversal (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {

    dbg_printf("\nArm7 (patch vAll)\n");
    
    bool usesThumb = false;
    int thumbType = 0;

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
        dbg_printf("Error in relocation checking method 1\n");
        
        // found the beginning of the next function
       u32 nextFunction = getOffset(relocationStart, ndsHeader->arm7binarySize,
          nextFunctiontSignature, 1, 1);
    
       	// Validate the relocation signature
        forwardedRelocStartAddr = nextFunction  - 0x14;
        
    	// Validate the relocation signature
        vAddrOfRelocSrc =
            *(u32*)(nextFunction - 0xC);
        
        // sanity checks
        relocationCheck1 =
            *(u32*)(forwardedRelocStartAddr + 0xC);
        relocationCheck2 =
            *(u32*)(forwardedRelocStartAddr + 0x10);
        if ( vAddrOfRelocSrc != relocationCheck1
          || vAddrOfRelocSrc != relocationCheck2) {
            dbg_printf("Error in relocation checking method 2\n");
    		return 0;
        }
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
    /*u32 cardReadEndAddr =
        getOffset((u32*)ndsHeader->arm7destination, 0x00400000, 
		a7cardReadSignature, 2, 1);
    if (!cardReadEndAddr) {
        dbg_printf("[Error!] Card read addr not found\n"); return 0;
    }

	dbg_printf("cardReadEndAddr: ");
	dbg_hexa(cardReadEndAddr);
	dbg_printf("\n");
    */
    
    u32 JumpTableFunc = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
        a7JumpTableSignatureUniversal, 3, 1);

	u32 EepromReadJump = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
        a7JumpTableSignatureUniversal, 3, 1);
		
	u32 EepromWriteJump = getOffset((u32*)EepromReadJump+4, ndsHeader->arm7binarySize,
        a7JumpTableSignatureUniversal_pt2, 3, 1);
		
	u32 EepromProgJump = getOffset((u32*)EepromWriteJump+4, ndsHeader->arm7binarySize,
        a7JumpTableSignatureUniversal_pt2, 3, 1);
	
	u32 EepromVerifyJump = getOffset((u32*)EepromProgJump+4, ndsHeader->arm7binarySize,
        a7JumpTableSignatureUniversal_pt2, 3, 1);
		
	u32 EepromEraseJump = getOffset((u32*)EepromVerifyJump+4, ndsHeader->arm7binarySize,
        a7JumpTableSignatureUniversal_pt3, 2, 1);
	
	if(!JumpTableFunc){
		JumpTableFunc = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2, 3, 1);

		EepromReadJump = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2, 3, 1);
			
		EepromWriteJump = getOffset((u32*)EepromReadJump+4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2_pt2, 3, 1);
			
		EepromProgJump = getOffset((u32*)EepromWriteJump+4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2_pt2, 3, 1);
		
		EepromVerifyJump = getOffset((u32*)EepromProgJump+4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2_pt2, 3, 1);
			
		EepromEraseJump = getOffset((u32*)EepromVerifyJump+4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2_pt3, 2, 1);
		if(!JumpTableFunc){
            usesThumb = true;
    		JumpTableFunc = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumb, 2, 1);
                
            dbg_printf("usesThumb");
            dbg_printf("JumpTableFunc");
	        dbg_hexa(JumpTableFunc);
    
    		EepromReadJump = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumb, 2, 1);
    			
    		EepromWriteJump = getOffset((u32*)EepromReadJump+2, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumb_pt2, 2, 1);
    			
    		EepromProgJump = getOffset((u32*)EepromWriteJump+2, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumb_pt2, 2, 1);
    		
    		EepromVerifyJump = getOffset((u32*)EepromProgJump+2, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumb_pt2, 2, 1);
    			
    		EepromEraseJump = getOffset((u32*)EepromVerifyJump+2, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumb_pt3, 1, 1);
    		
    	}	
    
		if(!JumpTableFunc){
			thumbType = 1;
    		JumpTableFunc = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
    			a7JumpTableSignatureUniversalThumbAlt, 1, 1);
                
            dbg_printf("usesThumb");
            dbg_printf("JumpTableFunc");
	        dbg_hexa(JumpTableFunc);
    	}
	}

    if(!JumpTableFunc){
		return 0;
	}    
		
	dbg_printf("JumpTableFunc: ");
	dbg_hexa(JumpTableFunc);
	dbg_printf("\n");



	u32* patches =  (u32*) cardEngineLocation[0];
	u32* arm7Function =  (u32*) patches[9];
    u32* arm7FunctionThumb =  (u32*) patches[14];
	u32 srcAddr;
    
    if (usesThumb) {

        u16 instrs [2];

		if (thumbType == 1) {
			u16* eepromRead = (u16*) (JumpTableFunc + 0x6);
			dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");
			srcAddr = JumpTableFunc + 0x6  - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[5]);
			eepromRead[0]=instrs[0];
			eepromRead[1]=instrs[1];
		
			u16* eepromPageWrite = (u16*) (JumpTableFunc + 0x14);
			dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");
			srcAddr = JumpTableFunc + 0x14 - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[3]);
			eepromPageWrite[0]=instrs[0];
			eepromPageWrite[1]=instrs[1];

			u16* eepromPageProg = (u16*) (JumpTableFunc + 0x22);
			dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");
			srcAddr = JumpTableFunc + 0x22 - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[4]);
			eepromPageProg[0]=instrs[0];
			eepromPageProg[1]=instrs[1];

			u16* eepromPageVerify = (u16*) (JumpTableFunc + 0x30);
			dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");
			srcAddr =  JumpTableFunc + 0x30 - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[2]);
			eepromPageVerify[0]=instrs[0];
			eepromPageVerify[1]=instrs[1];


			u16* eepromPageErase = (u16*) (JumpTableFunc + 0x3C);
			dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");
			srcAddr = JumpTableFunc + 0x3C - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[1]);        
			eepromPageErase[0]=instrs[0];
			eepromPageErase[1]=instrs[1];
		} else {
			u16* eepromRead = (u16*) (EepromReadJump + 0x8);
			dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");
			srcAddr = EepromReadJump + 0xC  - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[5]);
			eepromRead[0]=instrs[0];
			eepromRead[1]=instrs[1];
		
			u16* eepromPageWrite = (u16*) (EepromWriteJump + 0x8);
			dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");
			srcAddr = EepromWriteJump + 0xC - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[3]);
			eepromPageWrite[0]=instrs[0];
			eepromPageWrite[1]=instrs[1];

			u16* eepromPageProg = (u16*) (EepromProgJump + 0x8);
			dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");
			srcAddr = EepromProgJump + 0xC - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[4]);
			eepromPageProg[0]=instrs[0];
			eepromPageProg[1]=instrs[1];

			u16* eepromPageVerify = (u16*) (EepromVerifyJump + 0x8);
			dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");
			srcAddr =  EepromVerifyJump + 0xC - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[2]);
			eepromPageVerify[0]=instrs[0];
			eepromPageVerify[1]=instrs[1];


			u16* eepromPageErase = (u16*) (EepromEraseJump + 0x4);
			dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");
			srcAddr = EepromEraseJump + 0x8 - vAddrOfRelocSrc + 0x37F8000 ;
			generateA7InstrThumb(instrs, srcAddr,
				arm7FunctionThumb[1]);        
			eepromPageErase[0]=instrs[0];
			eepromPageErase[1]=instrs[1];
		}

	} else {

    	u32* eepromRead = (u32*) (EepromReadJump + 0xC);
    	dbg_printf("Eeprom read:\t");
    	dbg_hexa((u32)eepromRead);
    	dbg_printf("\n");
    	srcAddr = EepromReadJump + 0xC  - vAddrOfRelocSrc + relocDestAtSharedMem ;
    	u32 patchRead = generateA7Instr(srcAddr,
    		arm7Function[5]);
    	*eepromRead=patchRead;
    
    	u32* eepromPageWrite = (u32*) (EepromWriteJump + 0xC);
    	dbg_printf("Eeprom page write:\t");
    	dbg_hexa((u32)eepromPageWrite);
    	dbg_printf("\n");
    	srcAddr = EepromWriteJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem ;
    	u32 patchWrite = generateA7Instr(srcAddr,
    		arm7Function[3]);
    	*eepromPageWrite=patchWrite;
    
    	u32* eepromPageProg = (u32*) (EepromProgJump + 0xC);
    	dbg_printf("Eeprom page prog:\t");
    	dbg_hexa((u32)eepromPageProg);
    	dbg_printf("\n");
    	srcAddr = EepromProgJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem ;
    	u32 patchProg = generateA7Instr(srcAddr,
    		arm7Function[4]);
    	*eepromPageProg=patchProg;
    
    	u32* eepromPageVerify = (u32*) (EepromVerifyJump + 0xC);
    	dbg_printf("Eeprom verify:\t");
    	dbg_hexa((u32)eepromPageVerify);
    	dbg_printf("\n");
    	srcAddr =  EepromVerifyJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem ;
    	u32 patchVerify = generateA7Instr(srcAddr,
    		arm7Function[2]);
    	*eepromPageVerify=patchVerify;
    
    
    	u32* eepromPageErase = (u32*) (EepromEraseJump + 0x8);
    	dbg_printf("Eeprom page erase:\t");
    	dbg_hexa((u32)eepromPageErase);
    	dbg_printf("\n");
    	srcAddr = EepromEraseJump + 0x8 - vAddrOfRelocSrc + relocDestAtSharedMem ;
    	u32 patchErase = generateA7Instr(srcAddr,
    		arm7Function[1]);
    	*eepromPageErase=patchErase;
    }

	arm7Function[8] = saveFileCluster;
	arm7Function[9] = saveSize;

	return 1;
}

u32 savePatchV2 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {

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
        dbg_printf("Error in relocation checking method 1\n");
        
        // found the beginning of the next function
       u32 nextFunction = getOffset(relocationStart, ndsHeader->arm7binarySize,
          nextFunctiontSignature, 1, 1);
    
       	// Validate the relocation signature
        forwardedRelocStartAddr = nextFunction  - 0x14;
        
    	// Validate the relocation signature
        vAddrOfRelocSrc =
            *(u32*)(nextFunction - 0xC);
        
        // sanity checks
        relocationCheck1 =
            *(u32*)(forwardedRelocStartAddr + 0xC);
        relocationCheck2 =
            *(u32*)(forwardedRelocStartAddr + 0x10);
        if ( vAddrOfRelocSrc != relocationCheck1
          || vAddrOfRelocSrc != relocationCheck2) {
            dbg_printf("Error in relocation checking method 2\n");
    		return 0;
        }
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
		arm7Function[9] = saveSize;
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
		arm7Function[9] = saveSize;
	}    

	return 1;
}


u32 savePatchV1 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {

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
        dbg_printf("Error in relocation checking method 1\n");
        
        // found the beginning of the next function
       u32 nextFunction = getOffset(relocationStart, ndsHeader->arm7binarySize,
          nextFunctiontSignature, 1, 1);
    
       	// Validate the relocation signature
        forwardedRelocStartAddr = nextFunction  - 0x14;
        
    	// Validate the relocation signature
        vAddrOfRelocSrc =
            *(u32*)(nextFunction - 0xC);
        
        // sanity checks
        relocationCheck1 =
            *(u32*)(forwardedRelocStartAddr + 0xC);
        relocationCheck2 =
            *(u32*)(forwardedRelocStartAddr + 0x10);
        if ( vAddrOfRelocSrc != relocationCheck1
          || vAddrOfRelocSrc != relocationCheck2) {
            dbg_printf("Error in relocation checking method 2\n");
    		return 0;
        }
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
	arm7Function[9] = saveSize;

	dbg_printf("Arm7 patched!\n");

    return 1;
}

void patchSwiHalt (const tNDSHeader* ndsHeader, u32* cardEngineLocation) {
	u32* patches =  (u32*) cardEngineLocation[0];
	u32 swiHaltOffset =   
		getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
			  (u32*)j_HaltSignature1, 4, 1);
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt1, 4, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 1 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt2, 4, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 2 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt3, 4, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 3 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt4, 4, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 4 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt5, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 5 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt6, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 6 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt7, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 7 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt8, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 8 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt9, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 9 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature1Alt10, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK2 call alt 10 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt1, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 1 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt2, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 2 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt3, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 3 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt4, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 4 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt5, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 5 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt6, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 6 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt7, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 7 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt8, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 8 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt9, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 9 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt10, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 10 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature3Alt11, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK3 call alt 11 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt1, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 1 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt2, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 2 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt3, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 3 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt4, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 4 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt5, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 5 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt6, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 6 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt7, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 7 not found\n");
		swiHaltOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00002000,//, ndsHeader->arm7binarySize,
				  (u32*)j_HaltSignature4Alt8, 3, 1);
	}
	if (!swiHaltOffset) {
		dbg_printf("swiHalt SDK4 call alt 8 not found\n");
	}
	if (swiHaltOffset>0) {
		dbg_printf("swiHalt call found\n");
		u32* swiHaltPatch = (u32*) patches[11];
		copyLoop ((u32*)swiHaltOffset, swiHaltPatch, 0xC);
	}
}

void fixForDsiBios (const tNDSHeader* ndsHeader, u32* cardEngineLocation) {
	u32* patches =  (u32*) cardEngineLocation[0];
	u32 swi12Offset =   
		getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
			  (u32*)swi12Signature, 1, 1);
	if (!swi12Offset) {
		dbg_printf("swi 0x12 call not found\n");
	} else {
		// Patch to call swi 0x02 instead of 0x12
		dbg_printf("swi 0x12 call found\n");
		u32* swi12Patch = (u32*) patches[10];
		copyLoop ((u32*)swi12Offset, swi12Patch, 0x4);
	}

	bool sdk5 = false;
	u32 swiGetPitchTableOffset =   
		getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
			  (u32*)j_GetPitchTableSignature1, 4, 1);
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt1, 4, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 1 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt2, 4, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 2 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt3, 4, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 3 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt4, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 4 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt5, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 5 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt6, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 6 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt7, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 7 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature1Alt8, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK2 call alt 8 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt1, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 1 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt2, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 2 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt3, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 3 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt4, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 4 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt5, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 5 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt6, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 6 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt7, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 7 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt8, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 8 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt9, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 9 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature3Alt10, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK3 call alt 10 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt1, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 1 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt2, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 2 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt3, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 3 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt4, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 4 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt5, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 5 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt6, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 6 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt7, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 7 not found\n");
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)j_GetPitchTableSignature4Alt8, 3, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable SDK4 call alt 8 not found\n");
		sdk5 = true;
		swiGetPitchTableOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00010000,//, ndsHeader->arm7binarySize,
				  (u32*)swiGetPitchTableSignature5, 4, 1);
	}
	if (!swiGetPitchTableOffset) {
		dbg_printf("swiGetPitchTable call SDK5 not found\n");
	}
	if (swiGetPitchTableOffset>0) {
		dbg_printf("swiGetPitchTable call found\n");
		if (sdk5) {
			u32* swiGetPitchTablePatch = (u32*) patches[13];
			copyLoop ((u32*)swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
		} else {
			u32* swiGetPitchTablePatch = (u32*) patches[12];
			copyLoop ((u32*)swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
		}
	}
}

u32 patchCardNdsArm7 (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {
	u32* debug = (u32*)0x037C6000;

	if(REG_SCFG_ROM != 0x703) {
		fixForDsiBios(ndsHeader, cardEngineLocation);
	}
	patchSwiHalt(ndsHeader, cardEngineLocation);

	u32* irqEnableStartSignature = irqEnableStartSignature1;
	u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
	if(moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	if(moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}
	
	bool usesThumb = false;

	u32 sleepPatchOffset =   
        getOffset((u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
              (u32*)sleepPatch, 2, 1);
    if (!sleepPatchOffset) {
        dbg_printf("sleep patch not found. Trying thumb\n");
        //return 0;
		usesThumb = true;
    } else {
		dbg_printf("sleep patch found\n");
		*(u32*)(sleepPatchOffset+8) = 0;
	}
	if (usesThumb) {
		sleepPatchOffset =   
        getOffset((u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
              (u32*)sleepPatchThumb, 1, 1);
		if (!sleepPatchOffset) {
			dbg_printf("Thumb sleep patch not found. Trying alt\n");
			//return 0;
			sleepPatchOffset =   
			getOffset((u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
				  (u32*)sleepPatchThumbAlt, 1, 1);
			if (!sleepPatchOffset) {
				dbg_printf("Thumb sleep patch alt not found\n");
				//return 0;
			} else {
				dbg_printf("Thumb sleep patch alt found\n");
				*(u32*)(sleepPatchOffset+4) = 0;
			}
		} else {
			dbg_printf("Thumb sleep patch found\n");
			*(u32*)(sleepPatchOffset+4) = 0;
		}
	}

	u32 cardCheckPullOutOffset =   
        getOffset((u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
              (u32*)cardCheckPullOutSignature, 4, 1);
    if (!cardCheckPullOutOffset) {
        dbg_printf("Card check pull out not found\n");
        //return 0;
    } else {
		debug[0] = cardCheckPullOutOffset;
		dbg_printf("Card check pull out found\n");
	}

	u32 cardIrqEnableOffset =   
        getOffset((u32*)ndsHeader->arm7destination, 0x00020000,//, ndsHeader->arm9binarySize,
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

	u32 saveResult = savePatchV1(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	if(!saveResult) saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	if(!saveResult) saveResult = savePatchUniversal(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	//if ((saveResult == 1) && (saveSize > 0) && (saveSize <= 0x00100000)) {
	//	aFile saveFile = getFileFromCluster (saveFileCluster);
	//	fileRead(0x0C5E0000, saveFile, 0, saveSize, 3);
	//}

	dbg_printf("ERR_NONE");
	return 0;
}

u32 patchCardNds (const tNDSHeader* ndsHeader, u32* cardEngineLocationArm7, u32* cardEngineLocationArm9, module_params_t* moduleParams, 
		u32 saveFileCluster, u32 saveSize, u32 patchMpuRegion, u32 patchMpuSize) {

	//Debug stuff.

	/*aFile myDebugFile = getBootFileCluster ("NDSBTSR2.LOG");
	enableDebug(myDebugFile);*/

	dbg_printf("patchCardNds");

	patchCardNdsArm9(ndsHeader, cardEngineLocationArm9, moduleParams, patchMpuRegion, patchMpuSize);
	if (cardReadFound || ndsHeader->fatSize == 0) {
		patchCardNdsArm7(ndsHeader, cardEngineLocationArm7, moduleParams, saveFileCluster, saveSize);

		dbg_printf("ERR_NONE");
		return ERR_NONE;
	} else {
		dbg_printf("ERR_LOAD_OTHR");
		return ERR_LOAD_OTHR;
	}
}
