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
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/ipc.h>
#include <nds/system.h>

#include "locations.h"
#include "common.h"
#include "loading.h"

extern void arm9_clearCache(void);
extern void arm9code(u32* addr);

u32* arm9executeAddress = NULL;
bool arm9_boostVram = false;
bool extendedMemory = false;
bool dsDebugRam = false;
volatile bool esrbScreenPrepared = false;
volatile bool esrbOnlineNoticeFound = false;
volatile bool esrbOnlineNoticeDisplayed = false;
volatile bool topScreenFadedIn = false;
volatile bool bottomScreenFadedIn = false;
volatile bool esrbImageLoaded = false;
volatile int arm9_stateFlag = ARM9_BOOT;

void initMBKARM9(void) {
	// Default DSiWare settings

	// WRAM-B fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK2)=0x8C888480;
	*((vu32*)REG_MBK3)=0x9C989490;

	// WRAM-C fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK4)=0x8C888480;
	*((vu32*)REG_MBK5)=0x9C989490;

	// WRAM-A not mapped (reserved to arm7)
	REG_MBK6=0x00000000;
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
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
	*(vu16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

void fadeIn(u8 screens) {
	for (int i = 25; i >= 0; i--) {
		if (screens & BIT(0)) {
			SetBrightness(0, i);
		}
		if (screens & BIT(1)) {
			SetBrightness(1, i);
		}
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

void fadeOut(u8 screens) {
	for (int i = 0; i <= 25; i++) {
		if (screens & BIT(0)) {
			SetBrightness(0, i);
		}
		if (screens & BIT(1)) {
			SetBrightness(1, i);
		}
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

void waitFrames(int frames) {
	for (int i = 0; i < frames; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

void memset_addrs_arm9(u32 start, u32 end)
{
	dmaFill9(0, (u32*)start, ((int)end - (int)start));
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

	*(vu32*)(0x02000000) = 0x314D454D;
	*(vu32*)(0x02400000) = 0x324D454D;
	if ((*(vu32*)(0x02000000) == 0x314D454D) && (*(vu32*)(0x02400000) == 0x324D454D)) {
		extendedMemory = true;
		dsDebugRam = ((vu8)((vu32)REG_SCFG_EXT+3) == 0);
	}

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
	//VRAM_C_CR = 0x80;
	//VRAM_D_CR = 0;
	VRAM_E_CR = 0x80;
	VRAM_F_CR = 0x80;
	VRAM_G_CR = 0x80;
	VRAM_H_CR = 0x80;
	VRAM_I_CR = 0x80;
	dmaFill9(0, BG_PALETTE, 2*1024);
	dmaFill9(0, OAM, 2*1024);
	for (vu16 *p = (vu16*)&REG_DISPCNT; p <= (vu16*)&REG_MASTER_BRIGHT; p++)
	{
		// Skip VCOUNT. Writing to it was setting it to 0 causing a frame to be
		// misrendered. This can also have side effects on 3DS, even though the
		// official TWL_FIRM can recover from it.
		if (p != (vu16*)&REG_VCOUNT)
			*p = 0;
	}
	for (vu16 *p = (vu16*)&REG_DISPCNT_SUB; p <= (vu16*)&REG_MASTER_BRIGHT_SUB; p++)
	{
		*p = 0;
	}
	SetBrightness(0, 31);
	SetBrightness(1, 31);
	dmaFill9(0, VRAM_A, 0x20000*3);		// Banks A, B, C
	dmaFill9(0, VRAM_D, 272*1024);		// Banks D (excluded), E, F, G, H, I

	memset_addrs_arm9(0x02000620, 0x02084000);	// clear part of EWRAM
	memset_addrs_arm9(0x02280000, IMAGES_LOCATION);	// clear part of EWRAM - except before nds-bootstrap images
	if (extendedMemory) {
		dmaFill9(0, (u32*)0x02400000, 0x3FC000);
		dmaFill9(0, (u32*)0x027FF000, dsDebugRam ? 0x1000 : 0x801000);
	}

	REG_DISPSTAT = 0;
	GFX_STATUS = 0;

	//VRAM_A_CR = 0;
	VRAM_B_CR = 0;
	// Don't mess with the ARM7's VRAM
	//VRAM_C_CR = 0;
	//VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = VRAM_ENABLE | VRAM_H_SUB_BG;
	VRAM_I_CR = VRAM_ENABLE | VRAM_I_SUB_BG_0x06208000;

	REG_DISPCNT = MODE_FB0;
	REG_DISPCNT_SUB = MODE_3_2D | DISPLAY_BG3_ACTIVE;
	REG_BG3CNT_SUB = (u16)BgSize_B8_256x256;

	REG_BG3PA_SUB = 0x0100;
	REG_BG3PD_SUB = 0x0100;

	REG_POWERCNT = BIT(0) | BIT(9) | BIT(15); // POWER_LCD | POWER_2D_B | POWER_SWAP_LCDS

	esrbOnlineNoticeFound = (*(u32*)IMAGES_LOCATION == 0x494C4E4F); // 'ONLI'

	// Return to passme loop
	//*(vu32*)0x02FFFE04 = (u32)0xE59FF018; // ldr pc, 0x02FFFE24
	//*(vu32*)0x02FFFE24 = (u32)0x02FFFE04; // Set ARM9 Loop address

	//asm volatile(
	//	"\tbx %0\n"
	//	: : "r" (0x02FFFE04)
	//);

	// Set ARM9 state to ready and wait for it to change again
	arm9_stateFlag = ARM9_READY;
	while (arm9_stateFlag != ARM9_BOOTBIN) {
		if (arm9_stateFlag == ARM9_SCRNCLR) {
			if (topScreenFadedIn || bottomScreenFadedIn) {
				u8 screens = 0;
				if (topScreenFadedIn) {
					screens |= BIT(0);
				}
				if (bottomScreenFadedIn) {
					screens |= BIT(1);
				}
				fadeOut(screens);
				dmaFill9(0, BG_PALETTE, 2*1024);
				dmaFill9(0, VRAM_A, 0x18000);		// Bank A
				dmaFill9(0, BG_GFX_SUB, 0xC000);		// Bank H-I
			}
			REG_DISPSTAT = 0;
			VRAM_A_CR = 0;
			VRAM_H_CR = 0;
			VRAM_I_CR = 0;
			REG_DISPCNT = 0;
			REG_DISPCNT_SUB = 0;
			REG_BG3CNT_SUB = 0;
			REG_BG3PA_SUB = 0;
			REG_BG3PD_SUB = 0;
			REG_POWERCNT = 0x820F;
			SetBrightness(0, 0);
			SetBrightness(1, 0);
			arm9_stateFlag = ARM9_READY;
		}
		if (arm9_stateFlag == ARM9_DISPESRB) { // Display ESRB rating and descriptor(s) for current title
			if (*(u32*)IMAGES_LOCATION != 0) {
				esrbScreenPrepared = true;
				if (bottomScreenFadedIn && esrbOnlineNoticeFound) {
					fadeOut(BIT(1));
					bottomScreenFadedIn = false;
				}
				arm9_esrbScreen();
				esrbImageLoaded = true;
				u8 screens = BIT(0);
				if (!bottomScreenFadedIn && esrbOnlineNoticeFound) {
					screens |= BIT(1);
					bottomScreenFadedIn = true;
				}
				if (!topScreenFadedIn) {
					fadeIn(screens);
					topScreenFadedIn = true;
					waitFrames(60*3); // Wait a minimum of 3 seconds
				}
				esrbOnlineNoticeDisplayed = esrbOnlineNoticeFound;
			}
			arm9_stateFlag = ARM9_READY;
		}
		if (arm9_stateFlag == ARM9_DISPSCRN) { // Display nds-bootstrap: Please wait
			arm9_pleaseWaitText();
			if (!bottomScreenFadedIn) {
				fadeIn(BIT(1));
				bottomScreenFadedIn = true;
			}
			arm9_stateFlag = ARM9_READY;
		}
		if (arm9_stateFlag == ARM9_DISPERR) { // Display nds-bootstrap: An error has occurred
			if (esrbOnlineNoticeDisplayed) {
				fadeOut(BIT(1));
				bottomScreenFadedIn = false;
			}
			arm9_errorText();
			if (!bottomScreenFadedIn) {
				fadeIn(BIT(1));
				bottomScreenFadedIn = true;
			}
			arm9_stateFlag = ARM9_READY;
		}
		if (arm9_stateFlag == ARM9_SETSCFG) {
			if (arm9_boostVram) {
				REG_SCFG_EXT |= BIT(13);	// Extended VRAM Access
			}
			// lock SCFG
			REG_SCFG_EXT &= ~(1UL << 31);

			arm9_stateFlag = ARM9_READY;
		}
	}

	// Clear out ARM9 DMA channel 3
	DMA_CR(3) = 0;
	DMA_SRC(3) = 0;
	DMA_DEST(3) = 0;

	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	arm9code(arm9executeAddress);
}
