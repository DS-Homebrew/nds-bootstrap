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

#include "tonccpy.h"
#include "my_fat.h"

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

extern u32 _io_dldi_features;

extern vu32* volatile cardStruct0;

extern u32* lastClusterCacheUsed;
extern u32 clusterCacheSize;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static aFile romFile;
static aFile savFile;

static int cardReadCount = 0;

static inline void setDeviceOwner(void) {
	if (_io_dldi_features & 0x00000010) {
		sysSetCartOwner (BUS_OWNER_ARM9);
	} else {
		sysSetCardOwner (BUS_OWNER_ARM9);
	}
}

static bool initialized = false;
static bool mariosHolidayPrimaryFixApplied = false;

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
        u32* ipcSyncHandler = ce9->irqTable + 16;
        ce9->intr_ipc_orig_return = *ipcSyncHandler;
        *ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

static void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}

void myIrqHandlerIPC(void) {
	if (sharedAddr[3] == 0x53415652) {
		// Read save
		setDeviceOwner();

		u32 dst = *(vu32*)(sharedAddr+2);
		u32 src = *(vu32*)(sharedAddr);
		u32 len = *(vu32*)(sharedAddr+1);

		fileRead((char*)dst, savFile, src, len);

		sharedAddr[3] = 0;
	}
	if (sharedAddr[3] == 0x53415657) {
		// Write save
		setDeviceOwner();

		u32 src = *(vu32*)(sharedAddr+2);
		u32 dst = *(vu32*)(sharedAddr);
		u32 len = *(vu32*)(sharedAddr+1);

		fileWrite((char*)src, savFile, dst, len);

		sharedAddr[3] = 0;
	}
	if (sharedAddr[3] == 0x524F4D52) {
		// Read ROM (redirected from arm7)
		setDeviceOwner();

		u32 dst = *(vu32*)(sharedAddr+2);
		u32 src = *(vu32*)(sharedAddr);
		u32 len = *(vu32*)(sharedAddr+1);

		fileRead((char*)dst, romFile, src, len);

		sharedAddr[3] = 0;
	}
}

//Currently used for NSMBDS romhacks
void __attribute__((target("arm"))) debug8mbMpuFix(){
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}

static void initialize(void) {
	if (!initialized) {
		if (!FAT_InitFiles(true)) {
			//nocashMessage("!FAT_InitFiles");
			while (1);
		}

		if (ndsHeader->romSize > 0) {
			u32 shrinksize = 0;
			for (u32 i = 0; i <= (ndsHeader->romSize)/0x2000; i += 4) {
				shrinksize = i;
			}
			if (shrinksize > ce9->maxClusterCacheSize) {
				shrinksize = ce9->maxClusterCacheSize;
			}
			clusterCacheSize = shrinksize;
		}

		lastClusterCacheUsed = (u32*)ce9->fatTableAddr;

		romFile = getFileFromCluster(ce9->fileCluster);
		buildFatTableCache(&romFile);

		//clusterCacheSize = ce9->maxClusterCacheSize;
		savFile = getFileFromCluster(ce9->saveCluster);
		//buildFatTableCache(&savFile);

		if (isSdk5(ce9->moduleParams)) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		} else {
			debug8mbMpuFix();
		}

		initialized = true;
	}
}

static inline int cardReadNormal(vu32* volatile cardStruct, u8* dst, u32 src, u32 len) {
	/*nocashMessage("begin\n");

	dbg_hexa(dst);
	nocashMessage("\n");
	dbg_hexa(src);
	nocashMessage("\n");
	dbg_hexa(len);
	nocashMessage("\n");*/

	//nocashMessage("aaaaaaaaaa\n");
	fileRead((char*)dst, romFile, src, len);

	if (!isSdk5(ce9->moduleParams) && strncmp(getRomTid(ndsHeader), "ASMP", 4)==0 && !mariosHolidayPrimaryFixApplied) {
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

	setDeviceOwner();

	cardReadCount++;

	initialize();
	enableIPC_SYNC();

	vu32* cardStruct = (vu32*)(isSdk5(ce9->moduleParams) ? 0x027DFFC0 : ce9->cardStruct0);

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

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}

	return cardReadNormal(cardStruct, dst, src, len);
}

u32 nandRead(void* memory,void* flash,u32 len,u32 dma) {
	setDeviceOwner();
	fileRead(memory, savFile, (u32)flash, len);
    return 0; 
}

u32 nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	setDeviceOwner();
	fileWrite(memory, savFile, (u32)flash, len);
	return 0;
}


u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();	

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	setDeviceOwner();
	initialize();

	hookIPC_SYNC();

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
