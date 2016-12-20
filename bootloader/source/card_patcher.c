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
u32 moduleParamsSignature[2]   = {0xDEC00621, 0x2106C0DE};

// sdk < 4 version
u32 a9cardReadSignature1[2]    = {0x04100010, 0x040001A4};
u32 cardReadStartSignature1[1] = {0xE92D4FF0};

// sdk > 4 version
u32 a9cardReadSignature4[2]    = {0x040001A4, 0x04100010};
u32 cardReadStartSignature4[1] = {0xE92D4070};

u32 a9cardIdSignature[2]      = {0x040001A4,0x04100010};
u32 cardIdStartSignature[1]   = {0xE92D4000};
u32 a9instructionBHI[1]       = {0x8A000001};
u32 cardPullOutSignature[4]   = {0xE92D4000,0xE24DD004,0xE201003F,0xE3500011};
u32 a9cardSendSignature[7]    = {0xE92D40F0,0xE24DD004,0xE1A07000,0xE1A06001,0xE1A01007,0xE3A0000E,0xE3A02000};
u32 cardCheckPullOutSignature[4]   = {0xE92D4018,0xE24DD004,0xE59F204C,0xE1D210B0};

// infinite loop
//u32 cardInitPullOutStartSignature[4] = {0xE92D40F0,0xE24DD004,0xE1A07000,0xE1A06001};

//u32 cardInitPullOutStartSignature[3] = {0xE92D4000,0xE3A0C301,0xE28CCE21,0xE51C1008};

u32 cardInitPullOutStartSignature[4] = {0xE59FC02C,0xE1DC30B0,0xE3A01000,0xE1CC10B0};
    
//
// Look in @data for @find and return the position of it.
//
u32 getOffsetA9(u32* addr, size_t size, u32* find, size_t sizeofFind, int direction)
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

module_params_t* findModuleParams(const tNDSHeader* ndsHeader)
{
	nocashMessage("Looking for moduleparams\n");
	uint32_t moduleparams = getOffsetA9((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize, (u32*)moduleParamsSignature, 2, 1);
	if(!moduleparams)
	{
		nocashMessage("No moduleparams?\n");
        return 0;
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
		nocashMessage("This rom is not compressed\n");
		return; //not compressed
	}
	nocashMessage("This rom is compressed ;)\n");
	decompressLZ77Backwards((uint8_t*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
	moduleParams->compressed_static_end = 0;
}


u32 patchCardNds (const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams) {	
	nocashMessage("patchCardNds");
	
	u32* debug = (u32*)0x037D0000;
	debug[4] = ndsHeader->arm9destination;
	debug[8] = moduleParams->sdk_version;
	
	u32* a9cardReadSignature = a9cardReadSignature1;
	u32* cardReadStartSignature = cardReadStartSignature1;
	if(moduleParams->sdk_version > 0x4000000) {
		a9cardReadSignature = a9cardReadSignature4;
		cardReadStartSignature = cardReadStartSignature4;
	} 

	// Find the card read
    u32 cardReadEndOffset =  
        getOffsetA9((u32*)ndsHeader->arm9destination, 0x00400000,//ndsHeader->arm9binarySize,
              (u32*)a9cardReadSignature, 2, 1);
    if (!cardReadEndOffset) {
        nocashMessage("Card read end not found\n");
        return 0;
    }	
	debug[1] = cardReadEndOffset;	
    u32 cardReadStartOffset =   
        getOffsetA9((u32*)cardReadEndOffset, -0xF9,
              (u32*)cardReadStartSignature, 1, -1);
    if (!cardReadStartOffset) {
        nocashMessage("Card read start not found\n");
        return 0;
    }
	debug[0] = cardReadStartOffset;
    nocashMessage("Card read found\n");	
	
	u32 cardPullOutOffset =   
        getOffsetA9((u32*)ndsHeader->arm9destination, 0x00400000,//, ndsHeader->arm9binarySize,
              (u32*)cardCheckPullOutSignature, 4, 1);
    if (!cardPullOutOffset) {
        nocashMessage("Card pull out not found\n");
        //return 0;
    }
	debug[0] = cardPullOutOffset;
    nocashMessage("Card pull out found\n");
	
	u32 cardInitPullOutOffset =   
        getOffsetA9((u32*)ndsHeader->arm7destination, 0x00400000,//, ndsHeader->arm9binarySize,
              (u32*)cardInitPullOutStartSignature, 4, 1);
    if (!cardInitPullOutOffset) {
        nocashMessage("Card init pull out not found\n");
        return 0;
    }
	debug[0] = cardInitPullOutOffset;
    nocashMessage("Card init pull out found\n");
	
	/*u32 cardSendOffset =   
        getOffsetA9((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)a9cardSendSignature, 4, 1);
    if (!cardSendOffset) {
        nocashMessage("Card send not found\n");
        return 0;
    }
	debug[0] = cardSendOffset;
    nocashMessage("Card send found\n");	
	
	// Find the card id
    u32 cardIdEndOffset =  
        getOffsetA9((u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
              (u32*)a9cardIdSignature, 2, 1);
    if (!cardIdEndOffset) {
        nocashMessage("Card id end not found\n");
        return 0;
    }	
	debug[1] = cardIdEndOffset;	
    u32 cardIdStartOffset =   
        getOffsetA9((u32*)cardIdEndOffset, -0x100,
              (u32*)cardIdStartSignature, 1, -1);
    if (!cardIdStartOffset) {
        nocashMessage("Card id start not found\n");
        return 0;
    }
	debug[0] = cardIdStartOffset;
    nocashMessage("Card id found\n");	*/	
	


	debug[2] = cardEngineLocation;
	
	u32* patches =  (u32*) cardEngineLocation[0];
	
	cardEngineLocation[3] = moduleParams->sdk_version;
	
	u32* cardReadPatch = (u32*) patches[0];
	
	u32* cardPullOutPatch = (u32*) patches[1];
	
	u32* cardInitPullOutPatch = (u32*) patches[2];
	
	debug[5] = patches;
	
	u32* card_struct = ((u32*)cardReadEndOffset) - 1;
	//u32* cache_struct = ((u32*)cardIdStartOffset) - 1;
	
	debug[6] = *card_struct;
	//debug[7] = *cache_struct;
	
	cardEngineLocation[5] = *card_struct;
	//cardEngineLocation[6] = *cache_struct;
	
	*((u32*)patches[5]) = ((u32*)*card_struct)+6;	
	if(moduleParams->sdk_version > 0x3000000) {
		*((u32*)patches[5]) = ((u32*)*card_struct)+7;	
	}
	
	copyLoop ((u32*)cardReadStartOffset, cardReadPatch, 0xF0);	
	
	if(cardPullOutOffset>0)
		copyLoop ((u32*)cardPullOutOffset, cardPullOutPatch, 0x4);	
		
	copyLoop ((u32*)cardInitPullOutOffset, cardInitPullOutPatch, 0x30);	
	
	nocashMessage("ERR_NONE");
	return 0;
}


