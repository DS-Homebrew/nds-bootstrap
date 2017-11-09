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
#include <nds/memory.h>
#include <nds/arm9/video.h>
#include <nds/arm9/input.h>
#include <nds/interrupts.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <nds/system.h>
#include <nds/ipc.h>

#include "common.h"

volatile int arm9_stateFlag = ARM9_BOOT;
volatile u32 arm9_BLANK_RAM = 0;
volatile bool arm9_errorColor = false;
volatile bool arm9_extRAM = false;
volatile u32 arm9_SCFG_EXT = 0;
volatile int arm9_loadBarLength = 0;
volatile bool arm9_animateLoadingCircle = false;

static int loadingCircleFrame = 0;
static bool drawnStuff = false;

static u16 colour1;
static u16 colour2;
static u16 colour3;
static u16 colour4;
static u16 colour5;
static u16 colour6;
static u16 colour7;
static u16 colour8;

static int i, y, k;

/*-------------------------------------------------------------------------
External functions
--------------------------------------------------------------------------*/
extern void arm9_clearCache (void);

/*-------------------------------------------------------------------------
arm9_errorOutput
Displays an error code on screen.
Written by Chishm.
Modified by Robz8:
 * Replace dots with brand new loading screen (original image made by Uupo03)
--------------------------------------------------------------------------*/
static void arm9_errorOutput (void) {
	if(!drawnStuff) {
		REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw white BG
		for (i = 0; i < 256*192; i++) {
			VRAM_A[i] = 0x7FFF;
		}

		// Draw "Loading..." text
		// L: Part 1
		for (y = 10; y <= 42; y++) {
			for (k = 54; k <= 57; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// L: Part 2
		for (y = 39; y <= 42; y++) {
			for (k = 58; k <= 68; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 1
		for (y = 21; y <= 24; y++) {
			for (k = 77; k <= 87; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 2
		for (y = 39; y <= 42; y++) {
			for (k = 77; k <= 87; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 3
		for (y = 25; y <= 38; y++) {
			for (k = 73; k <= 76; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 4
		for (y = 25; y <= 38; y++) {
			for (k = 88; k <= 91; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 1
		for (y = 21; y <= 24; y++) {
			for (k = 96; k <= 110; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 2
		for (y = 25; y <= 42; y++) {
			for (k = 111; k <= 114; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 3
		for (y = 32; y <= 38; y++) {
			for (k = 96; k <= 99; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 4
		for (y = 28; y <= 31; y++) {
			for (k = 100; k <= 110; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 5
		for (y = 39; y <= 42; y++) {
			for (k = 100; k <= 110; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 1
		for (y = 21; y <= 24; y++) {
			for (k = 123; k <= 133; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 2
		for (y = 39; y <= 42; y++) {
			for (k = 123; k <= 133; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 3
		for (y = 25; y <= 38; y++) {
			for (k = 119; k <= 122; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 4
		for (y = 10; y <= 42; y++) {
			for (k = 134; k <= 137; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// i: Part 1
		for (y = 13; y <= 16; y++) {
			for (k = 142; k <= 145; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// i: Part 2
		for (y = 21; y <= 42; y++) {
			for (k = 142; k <= 145; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// n: Part 1
		for (y = 21; y <= 24; y++) {
			for (k = 150; k <= 164; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// n: Part 2
		for (y = 25; y <= 42; y++) {
			for (k = 150; k <= 153; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// n: Part 3
		for (y = 25; y <= 42; y++) {
			for (k = 165; k <= 168; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 1
		for (y = 21; y <= 24; y++) {
			for (k = 177; k <= 187; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 2
		for (y = 36; y <= 39; y++) {
			for (k = 177; k <= 187; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 3
		for (y = 43; y <= 46; y++) {
			for (k = 173; k <= 187; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 4
		for (y = 25; y <= 35; y++) {
			for (k = 173; k <= 176; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 5
		for (y = 21; y <= 42; y++) {
			for (k = 188; k <= 191; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// Dot 1
		for (y = 39; y <= 42; y++) {
			for (k = 196; k <= 199; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// Dot 2
		for (y = 39; y <= 42; y++) {
			for (k = 204; k <= 207; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// Dot 3
		for (y = 39; y <= 42; y++) {
			for (k = 212; k <= 215; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// End of Draw "Loading..." text

		// Draw loading bar top edge
		for (y = 154; y <= 158; y++) {
			for (k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar top edge (shade line)
		for (k = 8; k <= 247; k++) {
			VRAM_A[159*256+k] = 0x5AD6;
		}

		// Draw loading bar bottom edge (shade line)
		for (k = 8; k <= 247; k++) {
			VRAM_A[184*256+k] = 0x5AD6;
		}

		// Draw loading bar bottom edge
		for (y = 185; y <= 189; y++) {
			for (k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar left edge
		for (y = 160; y <= 183; y++) {
			for (k = 2; k <= 6; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar left edge (shade line)
		for (y = 160; y <= 183; y++) {
			VRAM_A[y*256+7] = 0x5AD6;
		}

		// Draw loading bar right edge
		for (y = 160; y <= 183; y++) {
			for (k = 248; k <= 252; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar right edge (shade line)
		for (y = 160; y <= 183; y++) {
			VRAM_A[y*256+253] = 0x5AD6;
		}
		
		drawnStuff = true;
	}

	// Draw loading bar
	for (i = 0; i <= arm9_loadBarLength; i++) {
		for (y = 160; y <= 183; y++) {
			for (k = 30*i+8; k < 30*i+38; k++) {
				VRAM_A[y*256+k] = 0x6F7B;
			}
		}
	}
	
	arm9_animateLoadingCircle = true;
}

static void arm9_loadingCircle (void) {
	switch(loadingCircleFrame) {
		case 0:
		default:
			colour1 = 0x7BDE;
			colour2 = 0x77BD;
			colour3 = 0x739C;
			colour4 = 0x6F7B;
			colour5 = 0x6B5A;
			colour6 = 0x6739;
			colour7 = 0x5EF7;
			colour8 = 0x56B5;
			break;
		case 1:
			colour1 = 0x56B5;
			colour2 = 0x7BDE;
			colour3 = 0x77BD;
			colour4 = 0x739C;
			colour5 = 0x6F7B;
			colour6 = 0x6B5A;
			colour7 = 0x6739;
			colour8 = 0x5EF7;
			break;
		case 2:
			colour1 = 0x5EF7;
			colour2 = 0x56B5;
			colour3 = 0x7BDE;
			colour4 = 0x77BD;
			colour5 = 0x739C;
			colour6 = 0x6F7B;
			colour7 = 0x6B5A;
			colour8 = 0x6739;
			break;
		case 3:
			colour1 = 0x6739;
			colour2 = 0x5EF7;
			colour3 = 0x56B5;
			colour4 = 0x7BDE;
			colour5 = 0x77BD;
			colour6 = 0x739C;
			colour7 = 0x6F7B;
			colour8 = 0x6B5A;
			break;
		case 4:
			colour1 = 0x6B5A;
			colour2 = 0x6739;
			colour3 = 0x5EF7;
			colour4 = 0x56B5;
			colour5 = 0x7BDE;
			colour6 = 0x77BD;
			colour7 = 0x739C;
			colour8 = 0x6F7B;
			break;
		case 5:
			colour1 = 0x6F7B;
			colour2 = 0x6B5A;
			colour3 = 0x6739;
			colour4 = 0x5EF7;
			colour5 = 0x56B5;
			colour6 = 0x7BDE;
			colour7 = 0x77BD;
			colour8 = 0x739C;
			break;
		case 6:
			colour1 = 0x739C;
			colour2 = 0x6F7B;
			colour3 = 0x6B5A;
			colour4 = 0x6739;
			colour5 = 0x5EF7;
			colour6 = 0x56B5;
			colour7 = 0x7BDE;
			colour8 = 0x77BD;
			break;
		case 7:
			colour1 = 0x77BD;
			colour2 = 0x739C;
			colour3 = 0x6F7B;
			colour4 = 0x6B5A;
			colour5 = 0x6739;
			colour6 = 0x5EF7;
			colour7 = 0x56B5;
			colour8 = 0x7BDE;
			break;
	}

	// Draw loading circle (7 times, for slow animation)
	for (i = 0; i < 7; i++) {
		for (y = 64; y <= 87; y++) {
			for (k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = colour1;
			}
			for (k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = colour2;
			}
			for (k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = colour3;
			}
		}	
		for (y = 92; y <= 115; y++) {
			for (k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = colour8;
			}
			for (k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
			for (k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = colour4;
			}
		}	
		for (y = 120; y <= 143; y++) {
			for (k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = colour7;
			}
			for (k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = colour6;
			}
			for (k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = colour5;
			}
		}
	}
	
	loadingCircleFrame++;
	if(loadingCircleFrame==8) loadingCircleFrame = 0;
}

static void arm9_errorText (void) {
	// Cover "Loading..." text
	for (i = 0; i < 256*48; i++) {
		VRAM_A[i] = 0x7FFF;
	}

	// Draw "Error!" text
	// E: Part 1
	for (y = 12; y <= 44; y++) {
		for (k = 76; k <= 79; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// E: Part 2
	for (y = 12; y <= 15; y++) {
		for (k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// E: Part 3
	for (y = 26; y <= 29; y++) {
		for (k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// E: Part 4
	for (y = 41; y <= 44; y++) {
		for (k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// Draw 2 r's.
	for (i = 0; i < 2; i++) {
		// Draw "Error!" text
		// r: Part 1
		for (y = 23; y <= 44; y++) {
			for (k = 95; k <= 98; k++) {
				VRAM_A[y*256+k+i*19] = 0x5AD6;
			}
		}
		
		// Draw "Error!" text
		// r: Part 2
		for (y = 26; y <= 29; y++) {
			for (k = 99; k <= 102; k++) {
				VRAM_A[y*256+k+i*19] = 0x5AD6;
			}
		}
		
		// Draw "Error!" text
		// r: Part 3
		for (y = 23; y <= 26; y++) {
			for (k = 103; k <= 109; k++) {
				VRAM_A[y*256+k+i*19] = 0x5AD6;
			}
		}
	}
	
	// Draw "Error!" text
	// o: Part 1
	for (y = 23; y <= 26; y++) {
		for (k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// o: Part 2
	for (y = 41; y <= 44; y++) {
		for (k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// o: Part 3
	for (y = 27; y <= 40; y++) {
		for (k = 133; k <= 136; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// o: Part 4
	for (y = 27; y <= 40; y++) {
		for (k = 148; k <= 151; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 1
	for (y = 23; y <= 44; y++) {
		for (k = 156; k <= 159; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 2
	for (y = 26; y <= 29; y++) {
		for (k = 160; k <= 163; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 3
	for (y = 23; y <= 26; y++) {
		for (k = 164; k <= 170; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// !: Part 1
	for (y = 12; y <= 32; y++) {
		for (k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// !: Part 2
	for (y = 38; y <= 44; y++) {
		for (k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// End of Draw "Error!" text
	
	// Change dots of loading circle to form an X
	for (y = 64; y <= 87; y++) {
		// 1st dot
		for (k = 88; k <= 111; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
		// 3rd dot
		for (k = 144; k <= 167; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
	}
	for (y = 92; y <= 115; y++) {
		// 5th dot
		for (k = 116; k <= 139; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
	}	
	for (y = 120; y <= 143; y++) {
		// 7th dot
		for (k = 88; k <= 111; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
		// 9th dot
		for (k = 144; k <= 167; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
	}

	arm9_animateLoadingCircle = false;
}

/*-------------------------------------------------------------------------
arm9_main

Clears the ARM9's DMA channels and resets video memory
Jumps to the ARM9 NDS binary in sync with the display and ARM7
Written by Darkain.
Modified by Chishm:
 * Changed MultiNDS specific stuff
--------------------------------------------------------------------------*/
void arm9_main (void) 
{
 	register int i;
  
	//set shared ram to ARM7
	WRAM_CR = 0x03;
	REG_EXMEMCNT = 0xE880;

	arm9_stateFlag = ARM9_START;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	arm9_clearCache();

	for (i=0; i<16*1024; i+=4) {  //first 16KB
		(*(vu32*)(i+0x00000000)) = 0x00000000;      //clear ITCM
		(*(vu32*)(i+0x00800000)) = 0x00000000;      //clear DTCM
	}

	for (i=16*1024; i<32*1024; i+=4) {  //second 16KB
		(*(vu32*)(i+0x00000000)) = 0x00000000;      //clear ITCM
	}

	arm9_stateFlag = ARM9_MEMCLR;

	(*(vu32*)0x00803FFC) = 0;   //IRQ_HANDLER ARM9 version
	(*(vu32*)0x00803FF8) = ~0;  //VBLANK_INTR_WAIT_FLAGS ARM9 version

	//clear out ARM9 DMA channels
	for (i=0; i<4; i++) {
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
//	VRAM_C_CR = 0;
//	VRAM_D_CR = 0x80;
	VRAM_E_CR = 0x80;
	VRAM_F_CR = 0x80;
	VRAM_G_CR = 0x80;
	VRAM_H_CR = 0x80;
	VRAM_I_CR = 0x80;
	BG_PALETTE[0] = 0xFFFF;
	dmaFill((void*)&arm9_BLANK_RAM, BG_PALETTE+1, (2*1024)-2);
	dmaFill((void*)&arm9_BLANK_RAM, OAM,     2*1024);
	dmaFill((void*)&arm9_BLANK_RAM, (void*)0x04000000, 0x56);  //clear main display registers
	dmaFill((void*)&arm9_BLANK_RAM, (void*)0x04001000, 0x56);  //clear sub  display registers
	dmaFill((void*)&arm9_BLANK_RAM, VRAM_A,  256*1024);		// Banks A, B
	dmaFill((void*)&arm9_BLANK_RAM, VRAM_E,  272*1024);		// Banks E, F, G, H, I

	REG_DISPSTAT = 0;

	VRAM_A_CR = 0;
	VRAM_B_CR = 0;
// Don't mess with the ARM7's VRAM
//	VRAM_C_CR = 0;
//	VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = 0;
	VRAM_I_CR = 0;
	REG_POWERCNT  = 0x820F;

	// Return to passme loop
	//*((vu32*)0x02FFFE04) = (u32)0xE59FF018;		// ldr pc, 0x02FFFE24
	//*((vu32*)0x02FFFE24) = (u32)0x02FFFE04;		// Set ARM9 Loop address

	//asm volatile(
	//	"\tbx %0\n"
	//	: : "r" (0x02FFFE04)
	//);

	// set ARM9 state to ready and wait for it to change again
	arm9_stateFlag = ARM9_READY;
	while ( arm9_stateFlag != ARM9_BOOTBIN ) {
		if(arm9_extRAM) {
			REG_SCFG_EXT = 0x83008000;
		} else {
			REG_SCFG_EXT = 0x83000000;
		}
		arm9_SCFG_EXT = REG_SCFG_EXT;
		if (arm9_stateFlag == ARM9_DISPERR) {
			arm9_errorOutput();
			if(arm9_errorColor) arm9_errorText();
			if ( arm9_stateFlag == ARM9_DISPERR) {
				arm9_stateFlag = ARM9_READY;
			}
		}
		if(arm9_animateLoadingCircle) arm9_loadingCircle();
	}

	//REG_IME=0;
	//REG_EXMEMCNT = 0xE880;
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	VoidFn arm9code = *(VoidFn*)(0x2FFFE24);
	arm9code();
	while(1);
}

