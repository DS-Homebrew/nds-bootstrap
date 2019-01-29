/*
 main.arm9.c

 By Michael Chisholm (Chishm)

 All resetMemory and startBinary functions are based
 on the MultiNDS loader by Darkain.
 Original source available at:
 http://cvs.sourceforge.net/viewcvs.py/ndslib/ndslib/examples/loader/boot/main.cpp

 License:
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

#define ARM9
#undef ARM7

#include <stddef.h> // NULL
#include <nds/memory.h>
#include <nds/arm9/video.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/ipc.h>
#include <nds/system.h>

#include "locations.h"
#include "common.h"
#include "loading.h"

extern void arm9_clearCache(void);

tNDSHeader* ndsHeader = NULL;
bool dsiModeConfirmed = false;
volatile bool arm9_boostVram = false;
volatile int arm9_stateFlag = ARM9_BOOT;
volatile u32 arm9_BLANK_RAM = 0;
volatile int arm9_screenMode = 0; // 0 = Regular, 1 = Pong, 2 = Tic-Tac-Toe
volatile int screenBrightness = 25;
volatile bool fadeType = true;

volatile bool arm9_darkTheme = false;
volatile bool arm9_swapLcds = false;
volatile bool arm9_errorColor = false;
volatile int arm9_loadBarLength = 0;
volatile bool arm9_animateLoadingCircle = false;
bool displayScreen = false;

void initMBKARM9(void) {
	// Default DSiWare settings

	// WRAM-B fully mapped to arm7 // inverted order
	*(vu32*)REG_MBK2 = 0x9195999D;
	*(vu32*)REG_MBK3 = 0x8185898D;
	
	// WRAM-C fully mapped to arm7 // inverted order
	*(vu32*)REG_MBK4 = 0x9195999D;
	*(vu32*)REG_MBK5 = 0x8185898D;
		
	// WRAM-A not mapped (reserved to arm7)
	REG_MBK6 = 0x00000000;
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7 = 0x07C03740; // Same as DSiWare
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8 = 0x07403700; // Same as DSiWare
}

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

void drawRectangle (int x, int y, int sizeX, int sizeY, u16 color) {
	for (int iy = y; iy <= y+sizeY-1; iy++) {
		for (int ix = x; ix <= x+sizeX-1; ix++) {
			VRAM_A[iy*256+ix] = color;	
		}
	}
}

/*-------------------------------------------------------------------------
arm9_main

Clears the ARM9's DMA channels and resets video memory
Jumps to the ARM9 NDS binary in sync with the display and ARM7
Written by Darkain.
Modified by Chishm:
 * Changed MultiNDS specific stuff
--------------------------------------------------------------------------*/
void arm9_main(void) {
 	register int i;
  
	// Set shared ram to ARM7
	WRAM_CR = 0x03;
	REG_EXMEMCNT = 0xE880;

	initMBKARM9();

	arm9_stateFlag = ARM9_START;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	arm9_clearCache();

	for (i = 0; i < 16*1024; i += 4) { // First 16KB
		*(vu32*)(i + 0x00000000) = 0x00000000; // Clear ITCM
		*(vu32*)(i + 0x00800000) = 0x00000000; // Clear DTCM
	}

	for (i = 16*1024; i < 32*1024; i += 4) { // Second 16KB
		*(vu32*)(i + 0x00000000) = 0x00000000; // Clear ITCM
	}

	arm9_stateFlag = ARM9_MEMCLR;

	*(vu32*)0x00803FFC = 0;  // IRQ_HANDLER ARM9 version
	*(vu32*)0x00803FF8 = ~0; // VBLANK_INTR_WAIT_FLAGS ARM9 version

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

	VRAM_A_CR = 0x80;
	VRAM_B_CR = 0x80;
	// Don't mess with the VRAM used for execution
	//VRAM_C_CR = 0;
	//VRAM_D_CR = 0x80;
	VRAM_E_CR = 0x80;
	VRAM_F_CR = 0x80;
	VRAM_G_CR = 0x80;
	VRAM_H_CR = 0x80;
	VRAM_I_CR = 0x80;
	BG_PALETTE[0] = 0xFFFF;
	dmaFill((u16*)&arm9_BLANK_RAM, BG_PALETTE+1, (2*1024)-2);
	dmaFill((u16*)&arm9_BLANK_RAM, OAM, 2*1024);
	dmaFill((u16*)&arm9_BLANK_RAM, (u16*)0x04000000, 0x56);  // Clear main display registers
	dmaFill((u16*)&arm9_BLANK_RAM, (u16*)0x04001000, 0x56);  // Clear sub display registers
	dmaFill((u16*)&arm9_BLANK_RAM, VRAM_A, 256*1024);		// Banks A, B
	dmaFill((u16*)&arm9_BLANK_RAM, VRAM_E, 272*1024);		// Banks E, F, G, H, I

	REG_DISPSTAT = 0;

	VRAM_A_CR = 0;
	VRAM_B_CR = 0;
	// Don't mess with the ARM7's VRAM
	//VRAM_C_CR = 0;
	//VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = 0;
	VRAM_I_CR = 0;
	REG_POWERCNT = 0x820F;

	*(u16*)0x0400006C |= BIT(14);
	*(u16*)0x0400006C &= BIT(15);
	SetBrightness(0, 31);
	SetBrightness(1, 31);

	// Return to passme loop
	//*(vu32*)0x02FFFE04 = (u32)0xE59FF018; // ldr pc, 0x02FFFE24
	//*(vu32*)0x02FFFE24 = (u32)0x02FFFE04; // Set ARM9 Loop address

	//asm volatile(
	//	"\tbx %0\n"
	//	: : "r" (0x02FFFE04)
	//);

	REG_SCFG_EXT = 0x8300C000;
	//REG_SCFG_EXT |= BIT(16);	// Access to New DMA Controller
	if (arm9_boostVram) {
		REG_SCFG_EXT |= BIT(13);	// Extended VRAM Access
	}

	screenBrightness = 25;
	fadeType = true;

	// Set ARM9 state to ready and wait for it to change again
	arm9_stateFlag = ARM9_READY;
	while (arm9_stateFlag != ARM9_BOOTBIN) {
		// SDK 5
		*(u32*)0x23FFC40 = 01;
		*(u32*)0x2FFFC40 = 01;

		if (arm9_stateFlag == ARM9_DISPERR) {
			displayScreen = true;
			if (arm9_stateFlag == ARM9_DISPERR) {
				arm9_stateFlag = ARM9_READY;
			}
		}
		if (displayScreen) {
			if (fadeType) {
				screenBrightness--;
				if (screenBrightness < 0) screenBrightness = 0;
			} else {
				screenBrightness++;
				if (screenBrightness > 25) screenBrightness = 25;
			}
			SetBrightness(0, screenBrightness);
			SetBrightness(1, screenBrightness);

			switch (arm9_screenMode) {
				case 0:
				default:
					arm9_regularLoadingScreen();
					if (arm9_errorColor) {
						arm9_errorText();
					}
					if (arm9_animateLoadingCircle) {
						arm9_loadingCircle();
					}
					break;

				case 1:
					arm9_pong();
					break;
					
				case 2:
					arm9_ttt();
					break;
				case 3:
					arm9_simpleLoadingScreen();
					if (arm9_errorColor) {
						arm9_errorText2();
					}
					break;
				case 4:
					arm9_R4LikeLoadingScreen();
					if (arm9_errorColor) {
						arm9_errorText4();
					}
					if (arm9_rotateLoadingCircle) {
						arm9_loadingCircle();
					}
					break;		
			}
		}
	}

	if (dsiModeConfirmed) {
		REG_SCFG_EXT = 0x8307F100;
	} else {
		// lock SCFG
		REG_SCFG_EXT &= ~(1UL << 31);
	}

	REG_IME = 0;
	REG_EXMEMCNT = 0xE880;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	VoidFn arm9code = (VoidFn)ndsHeader->arm9executeAddress;
	arm9code();
	
	while (1);
}
