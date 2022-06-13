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
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "igm_text.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"

#define EXCEPTION_VECTOR_SDK1	(*(VoidFn *)(0x27FFD9C))

#define FEATURE_SLOT_GBA			0x00000010
#define FEATURE_SLOT_NDS			0x00000020

#define expansionPakFound BIT(0)
#define extendedMemory BIT(1)
#define ROMinRAM BIT(2)
#define dsDebugRam BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define overlaysInRam BIT(6)
#define cacheFlushFlag BIT(7)
#define cardReadFix BIT(8)
#define softResetMb BIT(13)

#include "tonccpy.h"
#include "card.h"
#include "my_fat.h"

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

extern vu32* volatile cardStruct0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
static u32 arm9iromOffset = 0;
static u32 arm9ibinarySize = 0;

static aFile romFile;
static aFile savFile;
static aFile ramDumpFile;
static aFile srParamsFile;
static aFile pageFile;
// static aFile manualFile;

static int cardReadCount = 0;

static inline u32 getRomSizeNoArm9Bin(const tNDSHeader* ndsHeader) {
	return ndsHeader->romSize - ndsHeader->arm7romOffset + ce9->overlaysSize;
}

static inline void setDeviceOwner(void) {
	if ((ce9->valueBits & expansionPakFound) || (__myio_dldi.features & FEATURE_SLOT_GBA)) {
		sysSetCartOwner (BUS_OWNER_ARM9);
	}
	if (__myio_dldi.features & FEATURE_SLOT_NDS) {
		sysSetCardOwner (BUS_OWNER_ARM9);
	}
}

static bool initialized = false;
static bool region0FixNeeded = false;
static bool igmReset = false;
static bool mpuSet = false;
static bool mariosHolidayPrimaryFixApplied = false;
static bool isDSiWare = false;

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

void user_exception(void);

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	if (EXCEPTION_VECTOR_SDK1 == enterException && *exceptionC == user_exception) return;

	exceptionStack = (u32)EXCEPTION_STACK_LOCATION_B4DS;
	EXCEPTION_VECTOR_SDK1 = enterException;
	*exceptionC = user_exception;
}

extern void region0Fix(); // Revert region 0 patch
extern void sdk5MpuFix();
extern void resetMpu();

void reset(u32 param) {
	setDeviceOwner();
	u32 resetParams = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	u32 iUncompressedSize = 0;
	fileRead((char*)&iUncompressedSize, pageFile, 0x3FFFF0, sizeof(u32));

	*(u32*)resetParams = param;
	if (isDSiWare || iUncompressedSize > 0x280000 || param == 0xFFFFFFFF || *(u32*)(resetParams+0xC) > 0) {
		enterCriticalSection();
		if (isDSiWare || iUncompressedSize > 0x280000) {
			sharedAddr[0] = 0x57495344; // 'DSIW'
		} else if (param != 0xFFFFFFFF && !igmReset && (ce9->valueBits & softResetMb)) {
			*(u32*)resetParams = 0;
			*(u32*)(resetParams+8) = 0x44414F4C; // 'LOAD'
			fileWrite((char*)ndsHeader, pageFile, 0x2BFE00, 0x160);
			fileWrite((char*)ndsHeader->arm9destination, pageFile, 0x14000, ndsHeader->arm9binarySize);
			fileWrite((char*)0x022C0000, pageFile, 0x2C0000, ndsHeader->arm7binarySize);
		}
		fileWrite((char*)resetParams, srParamsFile, 0, 0x10);
		sharedAddr[3] = 0x52534554;
		while (1);
	}
	sharedAddr[3] = 0x52534554;

	//EXCEPTION_VECTOR_SDK1 = 0;

	//volatile u32 arm9_BLANK_RAM = 0;
 	register int i;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	//toncset((u32*)0x01FF8000, 0, 0x8000);

	cacheFlush();
	resetMpu();

	if (igmReset) {
		igmReset = false;
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);
	}

	// Clear out ARM9 DMA channels
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	mpuSet = false;
	IPC_SYNC_hooked = false;
	mariosHolidayPrimaryFixApplied = false;

	toncset((char*)((ce9->valueBits & isSdk5) ? 0x02FFFD80 : 0x027FFD80), 0, 0x80);
	toncset((char*)((ce9->valueBits & isSdk5) ? 0x02FFFF80 : 0x027FFF80), 0, 0x80);

	/*if (isDSiWare) {
		REG_DISPSTAT = 0;
		REG_DISPCNT = 0;
		REG_DISPCNT_SUB = 0;

		toncset((u16*)0x04000000, 0, 0x56);
		toncset((u16*)0x04001000, 0, 0x56);

		VRAM_A_CR = 0x80;
		VRAM_B_CR = 0x80;
		VRAM_C_CR = 0x80;
		VRAM_D_CR = 0x80;
		VRAM_E_CR = 0x80;
		VRAM_F_CR = 0x80;
		VRAM_G_CR = 0x80;
		VRAM_H_CR = 0x80;
		VRAM_I_CR = 0x80;

		toncset16(BG_PALETTE, 0, 256); // Clear palettes
		toncset16(BG_PALETTE_SUB, 0, 256);
		toncset(VRAM, 0, 0xC0000); // Clear VRAM

		VRAM_A_CR = 0;
		VRAM_B_CR = 0;
		VRAM_C_CR = 0;
		VRAM_D_CR = 0;
		VRAM_E_CR = 0;
		VRAM_F_CR = 0;
		VRAM_G_CR = 0;
		VRAM_H_CR = 0;
		VRAM_I_CR = 0;
	}*/

	/*mpuFullRam();

	*(u32*)(0x02000000) = BIT(0) | BIT(1);
	if (param == 0xFFFFFFFF) {
		*(u32*)(0x02000000) |= BIT(2);
	}
	toncset((u32*)0x02000004, 0, 0x3D9000 - 4);
	toncset((u32*)0x023E0000, 0, 0x1E000);

	ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;

	WRAM_CR = 0; // Set shared ram to ARM9

	aFile bootNds = getBootFileCluster("BOOT.NDS");
	fileRead((char*)ndsHeader, bootNds, 0, 0x170);
	fileRead(ndsHeader->arm9destination, bootNds, ndsHeader->arm9romOffset, ndsHeader->arm9binarySize);
	fileRead(ndsHeader->arm7destination, bootNds, ndsHeader->arm7romOffset, ndsHeader->arm7binarySize);

	WRAM_CR = 0x03; // Set shared ram to ARM7

	if (!dldiPatchBinary(ndsHeader->arm9destination, ndsHeader->arm9binarySize)) {
		while (1);
	}*/

	u32 newArm7binarySize = 0;
	fileRead((char*)&newArm7binarySize, pageFile, 0x3FFFF4, sizeof(u32));
	fileRead((char*)ndsHeader->arm9destination, pageFile, 0x14000, iUncompressedSize);
	fileRead((char*)ndsHeader->arm7destination, pageFile, 0x2C0000, newArm7binarySize);

	sharedAddr[0] = 0x544F4F42; // 'BOOT'
	sharedAddr[3] = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	VoidFn arm9code = (VoidFn)ndsHeader->arm9executeAddress;
	arm9code();
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
		case 0x6:
			if(mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
			break;
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
		case 0x9: {
			int oldIME = enterCriticalSection();
			setDeviceOwner();

			fileWrite((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0xA000, 0xA000);	// Backup part of game RAM to page file
			fileRead((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0, 0xA000);	// Read in-game menu

			*(u32*)(INGAME_MENU_LOCATION_B4DS + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION_B4DS + IGM_TEXT_SIZE_ALIGNED + 0x10;
			(*inGameMenu)(&mainScreen, 0);

			fileWrite((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0, 0xA000);	// Store in-game menu
			fileRead((char*)INGAME_MENU_LOCATION_B4DS, pageFile, 0xA000, 0xA000);	// Restore part of game RAM from page file

			if (sharedAddr[3] == 0x52534554) {
				igmReset = true;
				reset(0);
			} else if (sharedAddr[3] == 0x444D4152) { // RAMD
				#ifdef EXTMEM
				fileWrite((char*)0x02000000, ramDumpFile, 0, 0x7E0000);
				fileWrite((char*)((ce9->valueBits & isSdk5) ? 0x02FE0000 : 0x027E0000), ramDumpFile, 0x7E0000, 0x20000);
				#else
				fileWrite((char*)0x02000000, ramDumpFile, 0, 0x400000);
				#endif
				sharedAddr[3] = 0;
			}

			leaveCriticalSection(oldIME);
		}
			break;
	}
}

static void initialize(void) {
	if (!initialized) {
		if (!FAT_InitFiles(true)) {
			//nocashMessage("!FAT_InitFiles");
			while (1);
		}

		romFile = getFileFromCluster(ce9->fileCluster);
		savFile = getFileFromCluster(ce9->saveCluster);

		if (ce9->romFatTableCache != 0) {
			romFile.fatTableCache = (u32*)ce9->romFatTableCache;
			romFile.fatTableCached = true;
		}
		if (ce9->savFatTableCache != 0) {
			savFile.fatTableCache = (u32*)ce9->savFatTableCache;
			savFile.fatTableCached = true;
		}

		ramDumpFile = getFileFromCluster(ce9->ramDumpCluster);
		srParamsFile = getFileFromCluster(ce9->srParamsCluster);
		pageFile = getFileFromCluster(ce9->pageFileCluster);
		// manualFile = getFileFromCluster(ce9->manualCluster);

		bool cloneboot = (ce9->valueBits & isSdk5) ? *(u16*)0x02FFFC40 == 2 : *(u16*)0x027FFC40 == 2;

		if (ce9->valueBits & isSdk5) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		}

		if (ndsHeader->unitCode > 0) {
			u32 buffer = 0;
			fileRead((char*)&buffer, romFile, 0x234, sizeof(u32));
			isDSiWare = (buffer == 0x00030004 || buffer == 0x00030005);
		}

		if ((ce9->valueBits & ROMinRAM) && !cloneboot) {
			fileRead((char*)ce9->romLocation, romFile, 0x8000, ndsHeader->arm9binarySize-ndsHeader->arm9romOffset);
			fileRead((char*)ce9->romLocation+(ndsHeader->arm9binarySize-ndsHeader->arm9romOffset)+ce9->overlaysSize, romFile, (u32)ndsHeader->arm7romOffset, getRomSizeNoArm9Bin(ndsHeader)+0x88);
			if (ndsHeader->unitCode == 3) {
				fileRead((char*)&arm9iromOffset, romFile, 0x1C0, sizeof(u32));
				fileRead((char*)&arm9ibinarySize, romFile, 0x1CC, sizeof(u32));

				fileRead((char*)ce9->romLocation+(arm9iromOffset-0x8000), romFile, arm9iromOffset+arm9ibinarySize, ce9->ioverlaysSize);
			}
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

static int counter=0;
int cardReadPDash(u32* cacheStruct, u32 src, u8* dst, u32 len) {
	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

    cardStruct[0] = src;
    cardStruct[1] = (vu32)dst;
    cardStruct[2] = len;

    cardRead(cacheStruct, dst, src, len);

    counter++;
	return counter;
}

void cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	if (!mpuSet) {
		if ((ce9->valueBits & isSdk5) && ndsHeader->unitCode > 0 && ndsHeader->unitCode < 3) {
			sdk5MpuFix();
		}
		if (region0FixNeeded) {
			region0Fix();
		}
		if (ce9->valueBits & enableExceptionHandler) {
			setExceptionHandler2();
		}
		mpuSet = true;
	}

	u16 exmemcnt = REG_EXMEMCNT;

	setDeviceOwner();
	initialize();

	cardReadCount++;

	enableIPC_SYNC();

	vu32* cardStruct = (vu32*)(ce9->cardStruct0);

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);

	if ((ce9->valueBits & cardReadFix) && src < 0x8000) {
		// Fix reads below 0x8000
		src = 0x8000 + (src & 0x1FF);
	}

	if (ce9->valueBits & ROMinRAM) {
		u32 newSrc = (u32)(ce9->romLocation-0x8000)+src;
		if (ndsHeader->unitCode == 3 && src >= arm9iromOffset) {
			newSrc = (u32)(ce9->romLocation-0x8000-arm9ibinarySize)+src;
		}
		tonccpy(dst, (u8*)newSrc, len);
	} else if ((ce9->valueBits & overlaysInRam) && src >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && src < ndsHeader->arm7romOffset) {
		tonccpy(dst, (u8*)((ce9->romLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+src),len);
	} else {
		cardReadNormal(dst, src, len);
	}

	REG_EXMEMCNT = exmemcnt;
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


/*void reset(u32 param) {
	setDeviceOwner();
	u32 resetParams = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	if (ce9->valueBits & softResetMb) {
		*(u32*)resetParams = 0;
		*(u32*)(resetParams+8) = 0x44414F4C; // 'LOAD'
		fileWrite(ndsHeader, pageFile, 0x2BFE00, 0x160);
		fileWrite((char*)ndsHeader->arm9destination, pageFile, 0x14000, ndsHeader->arm9binarySize);
		fileWrite((char*)0x022C0000, pageFile, 0x2C0000, ndsHeader->arm7binarySize);
	} else {
		*(u32*)resetParams = param;
	}
	fileWrite((char*)resetParams, srParamsFile, 0, 0x10);
	sharedAddr[3] = 0x52534554;
	while (1);
}*/

void rumble(u32 arg) {
	sharedAddr[0] = ce9->rumbleFrames[0];
	sharedAddr[1] = ce9->rumbleForce[0];
	sharedAddr[3] = 0x424D5552; // 'RUMB'
}

void rumble2(u32 arg) {
	sharedAddr[0] = ce9->rumbleFrames[1];
	sharedAddr[1] = ce9->rumbleForce[1];
	sharedAddr[3] = 0x424D5552; // 'RUMB'
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();
	u16 exmemcnt = REG_EXMEMCNT;

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	setDeviceOwner();
	initialize();

	if (ce9->valueBits & enableExceptionHandler) {
		setExceptionHandler2();
	}

	if (unpatchedFuncs->mpuDataOffset) {
		region0FixNeeded = unpatchedFuncs->mpuInitRegionOldData == 0x4000033;
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
