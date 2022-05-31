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
#include <nds/arm9/video.h>
#include <nds/system.h>
#include <nds/dma.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "ndma.h"
#include "tonccpy.h"
#include "hex.h"
#include "igm_text.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;

static bool igmReset = false;

s8 mainScreen = 0;

static void SetBrightness(u8 screen, s8 bright) {
	u8 mode = 1;

	if (bright < 0) {
		mode = 2;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(vu16*)(0x0400006C + (0x1000 * screen)) = bright | (mode << 14);
}

// Alternative to swiWaitForVBlank()
/*static void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}*/

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
	if (!IPC_SYNC_hooked) {
		u32* ipcSyncHandler = ce9->irqTable + 16;
		ce9->intr_ipc_orig_return = *ipcSyncHandler;
		*ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
		IPC_SYNC_hooked = true;
	}
}

/*static void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}*/

void initMBKARM9_dsiMode(void) {
	*(vu32*)REG_MBK1 = *(u32*)0x02FFE180;
	*(vu32*)REG_MBK2 = *(u32*)0x02FFE184;
	*(vu32*)REG_MBK3 = *(u32*)0x02FFE188;
	*(vu32*)REG_MBK4 = *(u32*)0x02FFE18C;
	*(vu32*)REG_MBK5 = *(u32*)0x02FFE190;
	REG_MBK6 = *(u32*)0x02FFE194;
	REG_MBK7 = *(u32*)0x02FFE198;
	REG_MBK8 = *(u32*)0x02FFE19C;
	REG_MBK9 = *(u32*)0x02FFE1AC;
}

void __attribute__((target("arm"))) resetMpu(void) {
	asm("LDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0");
}

void reset(u32 tid1, u32 tid2) {
	if (tid1 != *(u32*)0x02FFE230 && tid2 != *(u32*)0x02FFE234) {
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

	register int i, reg;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	cacheFlush();
	resetMpu();

	if (igmReset) {
		igmReset = false;

		if (ce9->intr_vblank_orig_return) {
			*(u32*)0x02FFC230 = *(u32*)0x02FFE230;
			*(u32*)0x02FFC234 = *(u32*)0x02FFE234;
		}
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);

		if (ce9->intr_vblank_orig_return) {
			*(u32*)0x02FFC230 = 0;
			*(u32*)0x02FFC234 = 0;
		}
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

	IPC_SYNC_hooked = false;

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

	sharedAddr[0] = 0x544F4F42; // 'BOOT'
	sharedAddr[3] = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	VoidFn arm9code = (VoidFn)ndsHeader->arm9executeAddress;
	arm9code();
}

//---------------------------------------------------------------------------------
void myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

//#ifndef TWLSDK
	if (sharedAddr[4] == 0x554E454D) {
		while (sharedAddr[4] != 0x54495845);
	}
//#endif
}

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerIPC");
	#endif	

	switch (IPC_GetSync()) {
		case 0x3:
			igmReset = true;
			sharedAddr[3] = 0x54495845;
			reset(0, 0);
			break;
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
			*(u32*)(INGAME_MENU_LOCATION_DSIWARE + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION_DSIWARE + IGM_TEXT_SIZE_ALIGNED + 0x10;
			(*inGameMenu)(&mainScreen, ce9->consoleModel);
			if (sharedAddr[3] == 0x52534554 || sharedAddr[3] == 0x54495845) {
				igmReset = true;
				if (sharedAddr[3] == 0x52534554) {
					reset(*(u32*)0x02FFE230, *(u32*)0x02FFE234);
				} else {
					reset(0, 0);
				}
			}
		}
			break;
	}

	if (sharedAddr[4] == 0x57534352) {
		enterCriticalSection();
		if (ce9->consoleModel < 2) {
			// Make screens white
			SetBrightness(0, 31);
			SetBrightness(1, 31);
		}
		cacheFlush();
		while (1);
	}
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	hookIPC_SYNC();

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
