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
#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"

#define FEATURE_SLOT_GBA			0x00000010
#define FEATURE_SLOT_NDS			0x00000020

#define expansionPakFound BIT(0)
#define extendedMemory BIT(1)
#define ROMinRAM BIT(2)
#define dsDebugRam BIT(3)
#define isSdk5 BIT(5)
#define overlaysInRam BIT(6)
#define cacheFlushFlag BIT(7)

#include "tonccpy.h"
#include "my_fat.h"

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

extern u32 _io_dldi_features;

extern vu32* volatile cardStruct0;

extern u32* lastClusterCacheUsed;
extern u32 clusterCache;
extern u32 clusterCacheSize;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static aFile romFile;
static aFile savFile;
static aFile srParamsFile;
//static aFile pageFile;

static int cardReadCount = 0;

static inline u32 getRomSizeNoArm9Bin(const tNDSHeader* ndsHeader) {
	return ndsHeader->romSize - ndsHeader->arm7romOffset + ce9->overlaysSize;
}

static inline void setDeviceOwner(void) {
	if ((ce9->valueBits & expansionPakFound) || (_io_dldi_features & FEATURE_SLOT_GBA)) {
		sysSetCartOwner (BUS_OWNER_ARM9);
	}
	if (_io_dldi_features & FEATURE_SLOT_NDS) {
		sysSetCardOwner (BUS_OWNER_ARM9);
	}
}

static bool initialized = false;
static bool mariosHolidayPrimaryFixApplied = false;

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
        //u32* vblankHandler = ce9->irqTable;
        u32* ipcSyncHandler = ce9->irqTable + 16;
        //ce9->intr_vblank_orig_return = *vblankHandler;
        ce9->intr_ipc_orig_return = *ipcSyncHandler;
        //*vblankHandler = ce9->patches->vblankHandlerRef;
        *ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

static void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}

s8 mainScreen = 0;

//---------------------------------------------------------------------------------
/*void myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	if (sharedAddr[4] == 0x554E454D) {
		while (sharedAddr[4] != 0x54495845);
	}
}*/

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
	switch (sharedAddr[3]) {
		case 0x53415652: {
			u16 exmemcnt = REG_EXMEMCNT;
			// Read save
			setDeviceOwner();

			u32 dst = *(vu32*)(sharedAddr+2);
			u32 src = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);

			fileRead((char*)dst, savFile, src, len);

			sharedAddr[3] = 0;
			REG_EXMEMCNT  = exmemcnt;
		} break;
		case 0x53415657: {
			u16 exmemcnt = REG_EXMEMCNT;
			// Write save
			setDeviceOwner();

			u32 src = *(vu32*)(sharedAddr+2);
			u32 dst = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);

			fileWrite((char*)src, savFile, dst, len);

			sharedAddr[3] = 0;
			REG_EXMEMCNT  = exmemcnt;
		} break;
		case 0x524F4D52: {
			u16 exmemcnt = REG_EXMEMCNT;
			// Read ROM (redirected from arm7)
			setDeviceOwner();

			u32 dst = *(vu32*)(sharedAddr+2);
			u32 src = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);

			fileRead((char*)dst, romFile, src, len);

			sharedAddr[3] = 0;
			REG_EXMEMCNT  = exmemcnt;
		} break;
	}

	switch (IPC_GetSync()) {
		case 0x7: {
			mainScreen++;
			if(mainScreen > 2)
				mainScreen = 0;

			if(mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
		}
			break;
		/*case 0x9: {
			int oldIME = enterCriticalSection();
			setDeviceOwner();

			fileWrite((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0xA000, 0xA000, -1);	// Backup part of game RAM to page file
			fileRead((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0, 0xA000, -1);	// Read in-game menu

			*(u32*)(INGAME_MENU_LOCATION_B4DS+0x400) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*) = (volatile void*)INGAME_MENU_LOCATION_B4DS+0x40C;
			(*inGameMenu)(&mainScreen);

			fileWrite((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0, 0xA000, -1);	// Store in-game menu
			fileRead((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0xA000, 0xA000, -1);	// Restore part of game RAM from page file

			leaveCriticalSection(oldIME);
		}
			break;*/
	}
}

static void initialize(void) {
	if (!initialized) {
		if (!FAT_InitFiles(true)) {
			//nocashMessage("!FAT_InitFiles");
			while (1);
		}

		lastClusterCacheUsed = (u32*)ce9->fatTableAddr;
		clusterCache = ce9->fatTableAddr;
		clusterCacheSize = ce9->maxClusterCacheSize;

		romFile = getFileFromCluster(ce9->fileCluster);
		savFile = getFileFromCluster(ce9->saveCluster);

		srParamsFile = getFileFromCluster(ce9->srParamsCluster);
		//pageFile = getFileFromCluster(ce9->pageFileCluster);

		buildFatTableCache(&romFile);
		buildFatTableCache(&savFile);

		if (ce9->valueBits & isSdk5) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		}

		if (ce9->valueBits & ROMinRAM) {
			fileRead((char*)ce9->romLocation, romFile, 0x8000, ndsHeader->arm9binarySize-0x4000);
			fileRead((char*)ce9->romLocation+(ndsHeader->arm9binarySize-0x4000)+ce9->overlaysSize, romFile, (u32)ndsHeader->arm7romOffset, getRomSizeNoArm9Bin(ndsHeader));
		}

		initialized = true;
	}
}

static inline void cardReadNormal(u8* dst, u32 src, u32 len) {
	/*nocashMessage("begin\n");

	dbg_hexa(dst);
	nocashMessage("\n");
	dbg_hexa(src);
	nocashMessage("\n");
	dbg_hexa(len);
	nocashMessage("\n");*/

	//nocashMessage("aaaaaaaaaa\n");
	fileRead((char*)dst, romFile, src, len);

	if (!(ce9->valueBits & isSdk5) && strncmp(getRomTid(ndsHeader), "ASMP", 4)==0 && !mariosHolidayPrimaryFixApplied) {
		for (u32 i = 0; i < len; i += 4) {
			if (*(u32*)(dst+i) == 0x4B434148) {
				*(u32*)(dst+i) = 0xA00;
				mariosHolidayPrimaryFixApplied = true;
				break;
			}
		}
		if (cardReadCount > 10) mariosHolidayPrimaryFixApplied = true;
	}

	//nocashMessage("end\n");

	if (ce9->valueBits & cacheFlushFlag) {
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	u16 exmemcnt = REG_EXMEMCNT;

	setDeviceOwner();
	initialize();

	cardReadCount++;

	enableIPC_SYNC();

	vu32* cardStruct = (vu32*)(ce9->cardStruct0);

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}

	if (ce9->valueBits & ROMinRAM) {
		u32 newSrc = (u32)(ce9->romLocation-0x8000)+src;
		tonccpy(dst, (u8*)newSrc, len);
	} else if ((ce9->valueBits & overlaysInRam) && src >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && src < ndsHeader->arm7romOffset) {
		tonccpy(dst, (u8*)((ce9->romLocation-0x4000-ndsHeader->arm9binarySize)+src),len);
	} else {
		cardReadNormal(dst, src, len);
	}

	REG_EXMEMCNT = exmemcnt;
	return 0;
}

bool nandRead(void* memory,void* flash,u32 len,u32 dma) {
	setDeviceOwner();
	fileRead(memory, savFile, (u32)flash, len);
    return true; 
}

bool nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	setDeviceOwner();
	fileWrite(memory, savFile, (u32)flash, len);
	return true;
}


void reset(u32 param) {
	setDeviceOwner();
	*(u32*)((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM) = param;
	fileWrite((char*)((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM), srParamsFile, 0, 0x10);
	sharedAddr[3] = 0x52534554;
	while (1);
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();
	u16 exmemcnt = REG_EXMEMCNT;

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	setDeviceOwner();
	initialize();

	if (unpatchedFuncs->compressed_static_end) {
		module_params_t* moduleParams = unpatchedFuncs->moduleParams;
		moduleParams->compressed_static_end = unpatchedFuncs->compressed_static_end;
	}

	if (unpatchedFuncs->mpuDataOffset) {
		*unpatchedFuncs->mpuDataOffset = unpatchedFuncs->mpuInitRegionOldData;

		if (unpatchedFuncs->mpuOldInstrAccess) {
			unpatchedFuncs->mpuDataOffset[unpatchedFuncs->mpuAccessOffset] = unpatchedFuncs->mpuOldInstrAccess;
		}
		if (unpatchedFuncs->mpuOldDataAccess) {
			unpatchedFuncs->mpuDataOffset[unpatchedFuncs->mpuAccessOffset + 1] = unpatchedFuncs->mpuOldDataAccess;
		}
	}

	if (unpatchedFuncs->mpuInitCacheOffset) {
		*unpatchedFuncs->mpuInitCacheOffset = unpatchedFuncs->mpuInitCacheOld;
	}

	if (unpatchedFuncs->mpuDataOffset2) {
		*unpatchedFuncs->mpuDataOffset2 = unpatchedFuncs->mpuOldDataAccess2;
	}

	hookIPC_SYNC();

	REG_EXMEMCNT = exmemcnt;

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
