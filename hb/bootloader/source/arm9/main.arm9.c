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

extern void arm9_clearCache(void);

tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
bool dsiModeConfirmed = false;
bool arm9_boostVram = false;
volatile bool ram32MB = false;
volatile int arm9_stateFlag = ARM9_BOOT;
volatile u32 arm9_BLANK_RAM = 0;
volatile u32 arm9_ramDiskCluster = 0;

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

	*(vu32*)(0x0DFFFE0C) = 0x54455354;		// Check for 32MB of RAM
	ram32MB = (*(vu32*)(0x0DFFFE0C) == 0x54455354);

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

	vu16 *mainregs = (vu16*)0x04000000;
	vu16 *subregs = (vu16*)0x04001000;
	
	for (register int i=0; i<43; i++) {
		mainregs[i] = 0;
		subregs[i] = 0;
	}
	
	u16 mode = 1 << 14;

	*(vu16*)(0x0400006C + (0x1000 * 0)) = 0 + mode;
	*(vu16*)(0x0400006C + (0x1000 * 1)) = 0 + mode;

	VRAM_A_CR = 0x80;
	VRAM_B_CR = 0x80;
	//VRAM_C_CR = 0;
	// Don't mess with the VRAM used for execution
	//VRAM_D_CR = 0;
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
	dmaFill((u16*)&arm9_BLANK_RAM, VRAM_A, 0x20000*3);		// Banks A, B, C
	dmaFill((u16*)&arm9_BLANK_RAM, VRAM_D, 272*1024);		// Banks D (excluded), E, F, G, H, I

	REG_DISPSTAT = 0;
	GFX_STATUS = 0;

	VRAM_A_CR = 0;
	VRAM_B_CR = 0;
	//VRAM_C_CR = 0;
	// Don't mess with the ARM7's VRAM
	//VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = 0;
	VRAM_I_CR = 0;
	REG_POWERCNT = 0x820F;

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
		if (arm9_stateFlag == ARM9_SETSCFG) {
			if (dsiModeConfirmed) {
				REG_SCFG_EXT = 0x8307F100;
			} else {
				REG_SCFG_EXT = 0x83000000;
				if (arm9_boostVram) {
					REG_SCFG_EXT |= BIT(13);	// Extended VRAM Access
				}
			}
			arm9_stateFlag = ARM9_READY;
		}
		if (arm9_stateFlag == ARM9_LOCKSCFG) {
			REG_SCFG_EXT &= ~(1UL << 31); // Lock SCFG
			arm9_stateFlag = ARM9_READY;
		}
	}

	REG_IME = 0;
	REG_EXMEMCNT = 0xE880;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	VoidFn arm9code = *(VoidFn*)(0x2FFFE24);
	arm9code();
	
	while (1);
}
