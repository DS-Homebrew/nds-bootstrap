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

#include <string.h>
#include <stdio.h>
#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
#include <nds/interrupts.h>
//#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#include "tonccpy.h"
#include "my_fat.h"

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

extern u32 _io_dldi_features;

extern vu32* volatile cardStruct0;

extern u32* lastClusterCacheUsed;
extern u32 clusterCacheSize;

static int cardEngineCommandMutex = 0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static aFile romFile;
static aFile savFile;

static int cardReadCount = 0;

static bool flagsSet = false;

static bool vblankHooked = false;
static void hookVblank(void) {
	resetRequestIrqMask(IRQ_VBLANK);
	u32* vblankHandler = ce9->irqTable;
	ce9->intr_vblank_orig_return = *vblankHandler;
	*vblankHandler = ce9->patches->vblankHandlerRef;
    enableIrqMask(IRQ_VBLANK);
	vblankHooked = true;
}

void myIrqHandlerVBlank(void) {
  	if (tryLockMutex(&cardEngineCommandMutex)) {
		if (sharedAddr[3] == 0x53415652) {
			// Read save
			sysSetCardOwner (BUS_OWNER_ARM9);

			u32 dst = *(vu32*)(sharedAddr+2);
			u32 src = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);

			if (dst >= 0x02000000 && dst < 0x03000000) {
				fileRead((char*)dst, savFile, src, len);
			} else {
				fileRead((char*)0x023E0000, savFile, src, len);
			}

			sharedAddr[3] = 0;
		}
		if (sharedAddr[3] == 0x53415657) {
			// Write save
			sysSetCardOwner (BUS_OWNER_ARM9);

			u32 src = *(vu32*)(sharedAddr+2);
			u32 dst = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);

			if (src >= 0x02000000 && src < 0x03000000) {
				fileWrite((char*)src, savFile, dst, len);
			} else {
				fileWrite((char*)0x023E0000, savFile, dst, len);
			}

			sharedAddr[3] = 0;
		}
		if (sharedAddr[3] == 0x524F4D52) {
			// Read ROM (redirected from arm7)
			sysSetCardOwner (BUS_OWNER_ARM9);

			u32 dst = *(vu32*)(sharedAddr+2);
			u32 src = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);

			fileRead((char*)dst, romFile, src, len);

			sharedAddr[3] = 0;
		}
  		unlockMutex(&cardEngineCommandMutex);
	}
}

static inline int cardReadNormal(vu32* volatile cardStruct, u8* dst, u32 src, u32 len, u32 page) {
	/*nocashMessage("begin\n");

	dbg_hexa(dst);
	nocashMessage("\n");
	dbg_hexa(src);
	nocashMessage("\n");
	dbg_hexa(len);
	nocashMessage("\n");*/

	//nocashMessage("aaaaaaaaaa\n");
	fileRead((char*)dst, romFile, src, len);

	//nocashMessage("end\n");

	/*if(strncmp(getRomTid(ndsHeader), "CLJ", 3) == 0){
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}*/
	
	return 0;
}

//Currently used for NSMBDS romhacks
/*void __attribute__((target("arm"))) debug8mbMpuFix(){
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}*/

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");

	//sysSetCartOwner (BUS_OWNER_ARM9);

	cardReadCount++;

	if (!flagsSet) {
		if (_io_dldi_features & 0x00000010) {
			sysSetCartOwner (BUS_OWNER_ARM9);
		}

		if (!ce9->ROMinRAM) {
			if (!FAT_InitFiles(true)) {
				//nocashMessage("!FAT_InitFiles");
				return -1;
			}

			if (ndsHeader->romSize > 0) {
				u32 shrinksize = 0;
				for (u32 i = 0; i <= ndsHeader->romSize; i += 0x200) {
					shrinksize += 4;
				}
				if (shrinksize > 0x4000) {
					shrinksize = 0x4000;
				}
				clusterCacheSize = shrinksize;
			}

			if (ce9->fatTableAddr < 0x02400000) {
				lastClusterCacheUsed = (u32*)ce9->fatTableAddr;
			}

			romFile = getFileFromCluster(ce9->fileCluster);
			buildFatTableCache(&romFile);
		}

		clusterCacheSize = 0x4000;
		savFile = getFileFromCluster(ce9->saveCluster);
		buildFatTableCache(&savFile);

		if (isSdk5(ce9->moduleParams)) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		} else {
			hookVblank();
		}

		flagsSet = true;
	}
	
	if (isSdk5(ce9->moduleParams) && cardReadCount == 3 && !vblankHooked) {
		hookVblank();
	}

	vu32* volatile cardStruct = (isSdk5(ce9->moduleParams) ? (vu32* volatile)(0x027DFFC0) : ce9->cardStruct0);

	u32 src = (isSdk5(ce9->moduleParams) ? src0 : cardStruct[0]);
	if (isSdk5(ce9->moduleParams)) {
		cardStruct[0] = src;
	}

	u8* dst = (isSdk5(ce9->moduleParams) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = (isSdk5(ce9->moduleParams) ? len0 : cardStruct[2]);

	if (isSdk5(ce9->moduleParams)) {
		cardStruct[1] = (vu32)dst;
		cardStruct[2] = len;
	}

	u32 page = (src / 512) * 512;

	return cardReadNormal(cardStruct, dst, src, len, page);
}

u32 nandRead(void* memory,void* flash,u32 len,u32 dma) {
	fileRead(memory, savFile, flash, len);
    return 0; 
}

u32 nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	fileWrite(memory, savFile, flash, len);
	return 0;
}