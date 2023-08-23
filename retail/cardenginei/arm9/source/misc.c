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
#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/dma.h>
#include <nds/arm9/video.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define extendedMemory BIT(1)
#define eSdk2 BIT(2)
#define dsiMode BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define overlaysInRam BIT(6)
#define slowSoftReset BIT(10)
#define softResetMb BIT(13)
#define cloneboot BIT(14)

#include "my_fat.h"

extern cardengineArm9* volatile ce9;

extern vu32* volatile sharedAddr;

extern tNDSHeader* ndsHeader;
extern aFile* romFile;
extern aFile* savFile;
extern aFile* apFixOverlaysFile;
extern u32* cacheAddressTable;

extern bool flagsSet;
extern bool igmReset;

extern u32 getDtcmBase(void);
extern void ndsCodeStart(u32* addr);
void resetSlots(void);
void initMBKARM9_dsiMode(void);

void SetBrightness(u8 screen, s8 bright) {
	u16 mode = 1 << 14;

	if (bright < 0) {
		mode = 2 << 14;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(u16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

// Alternative to swiWaitForVBlank()
void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

/*void sleepMs(int ms) {
	if (REG_IME == 0 || REG_IF == 0) {
		return;
	}

	if(ce9->patches->sleepRef) {
		volatile void (*sleepRef)(int ms) = (volatile void*)ce9->patches->sleepRef;
		(*sleepRef)(ms);
	} else if(ce9->thumbPatches->sleepRef) {
		extern void callSleepThumb(int ms);
		callSleepThumb(ms);
	}
}

static void waitForArm7(void) {
	IPC_SendSync(0x4);
	while (sharedAddr[3] != (vu32)0);
}*/

bool IPC_SYNC_hooked = false;
void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
		#ifndef TWLSDK
		if (!(ce9->valueBits & isSdk5)) {
			u32* vblankHandler = ce9->irqTable;
			ce9->intr_vblank_orig_return = *vblankHandler;
			*vblankHandler = (u32)ce9->patches->vblankHandlerRef;
		}
		#endif
		u32* ipcSyncHandler = ce9->irqTable + 16;
		ce9->intr_ipc_orig_return = *ipcSyncHandler;
		*ipcSyncHandler = (u32)ce9->patches->ipcSyncHandlerRef;
		IPC_SYNC_hooked = true;
    }
}

void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}

#ifndef TWLSDK
void initialize(void) {
	static bool initialized = false;

	if (!initialized) {
		if (ce9->valueBits & isSdk5) {
			sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
			if (ndsHeader->unitCode > 0) {
				romFile = (aFile*)ROM_FILE_LOCATION_MAINMEM5;
				savFile = (aFile*)SAV_FILE_LOCATION_MAINMEM5;
				apFixOverlaysFile = (aFile*)OVL_FILE_LOCATION_MAINMEM5;
				#ifndef DLDI
				cacheAddressTable = (u32*)CACHE_ADDRESS_TABLE_LOCATION_TWLSDK;
				#endif
			}
		}
		initialized = true;
	}
}
#endif


//static void clearIcache (void) {
      // Seems to have no effect
      // disable interrupt
      /*int oldIME = enterCriticalSection();
      IC_InvalidateAll();
      // restore interrupt
      leaveCriticalSection(oldIME);*/
//}

extern void resetMpu(void);

void reset(u32 param, u32 tid2) {
#ifndef TWLSDK
	u32 resetParams = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	*(u32*)resetParams = param;
	if (ce9->valueBits & slowSoftReset) {
		if (ce9->consoleModel < 2) {
			// Make screens white
			SetBrightness(0, 31);
			SetBrightness(1, 31);
			waitFrames(5);	// Wait for DSi screens to stabilize
		}
		enterCriticalSection();
		if (!igmReset && (ce9->valueBits & softResetMb)) {
			*(u32*)resetParams = 0;
			*(u32*)(resetParams+8) = 0x44414F4C; // 'LOAD'
		}
		cacheFlush();
		sharedAddr[3] = 0x52534554;
		while (1);
	} else {
		if (*(u32*)(resetParams+0xC) > 0) {
			sharedAddr[1] = ce9->valueBits;
		}
		if (!igmReset && (ce9->valueBits & softResetMb)) {
			*(u32*)resetParams = 0;
			*(u32*)(resetParams+8) = 0x44414F4C; // 'LOAD'
		}
		sharedAddr[3] = 0x52534554;
	}
#else
	#ifdef DLDI
	sysSetCardOwner(false);	// Give Slot-1 access to arm7
	#endif
	if (param == 0xFFFFFFFF || *(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
		if (param == 0xFFFFFFFF || (param != *(u32*)0x02FFE230 && tid2 != *(u32*)0x02FFE234)) {
			/*if (ce9->consoleModel < 2) {
				// Make screens white
				SetBrightness(0, 31);
				SetBrightness(1, 31);
				waitFrames(5);	// Wait for DSi screens to stabilize
			}
			enterCriticalSection();
			cacheFlush();*/
			sharedAddr[3] = 0x54495845;
			//while (1);
		} else {
			sharedAddr[3] = 0x52534554;
		}
	} else {
		*(u32*)RESET_PARAM_SDK5 = param;
		sharedAddr[3] = 0x52534554;
	}
#endif

 	register int i, reg;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	cacheFlush();
	resetMpu();

	if (igmReset) {
		igmReset = false;
#ifdef TWLSDK
		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = *(u32*)0x02FFE230;
			*(u32*)0x02FFC234 = *(u32*)0x02FFE234;
		}
#endif
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);
#ifdef TWLSDK
		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = 0;
			*(u32*)0x02FFC234 = 0;
		}
#endif
	}

	// Clear out ARM9 DMA channels
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	for (i = 0; i < 4; i++) {
		for(reg=0; reg<0x1c; reg+=4)*((vu32*)(0x04004104 + ((i*0x1c)+reg))) = 0;//Reset NDMA.
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	flagsSet = false;
	IPC_SYNC_hooked = false;

#ifdef TWLSDK
	if (param == 0xFFFFFFFF || *(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
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
	}

	#ifndef DLDI
	if (ce9->consoleModel == 0) {
		resetSlots();
	}
	#endif

	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	if (ndsHeader->unitCode > 0 && sharedAddr[3] == 0x54495845) {
		initMBKARM9_dsiMode();
		REG_SCFG_EXT = 0x8307F100;
		REG_SCFG_CLK = 0x87;
		REG_SCFG_RST = 1;
	}

	#ifdef DLDI
	sysSetCardOwner(true);	// Give Slot-1 access back to arm9
	#endif
#else
	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	if (*(u32*)(RESET_PARAM+0xC) > 0) {
		u32 newIrqTable = sharedAddr[2];
		ce9->valueBits = sharedAddr[1];
		ce9->irqTable = (u32*)newIrqTable;
		ce9->cardStruct0 = sharedAddr[4];
		sharedAddr[4] = 0;
	}
#endif

	sharedAddr[0] = 0x544F4F42; // 'BOOT'
	sharedAddr[3] = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	ndsCodeStart(ndsHeader->arm9executeAddress);
}
