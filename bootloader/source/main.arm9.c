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
volatile int arm9_screenMode = 0;	// 0 = Regular, 1 = Pong, 2 = Tic-Tac-Toe
volatile bool arm9_extRAM = false;
volatile u32 arm9_SCFG_EXT = 0;
volatile int arm9_loadBarLength = 0;
volatile bool arm9_animateLoadingCircle = false;

static bool displayScreen = false;

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

static int e, i, y, k;

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
static void arm9_regularLoadingScreen (void) {
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
	displayScreen = false;
}

static int leftpaddle_yPos = 76;
static int rightpaddle_yPos = 76;
static int ball_xPos = 16;
static int ball_yPos = 72;
static int ball_ySpeed = 2;
static int ball_moveUD = false;	// false = up, true = down
static int ball_moveLR = true;	// false = left, true = right

static u16 pong_color = 0x0000;

static void arm9_pong (void) {
	if(!drawnStuff) {
		REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw white BG
		for (i = 0; i < 256*192; i++) {
			VRAM_A[i] = 0x7FFF;
		}

		drawnStuff = true;
	}
	
	if(arm9_errorColor) pong_color = 0x001B;

	while(REG_VCOUNT!=191);	// fix speed
	
	rightpaddle_yPos = ball_yPos-16;
	if(rightpaddle_yPos <= 0) rightpaddle_yPos = 0;
	if(rightpaddle_yPos >= 156) rightpaddle_yPos = 156;

	// Draw ball (back)
	for (y = ball_yPos-16; y <= ball_yPos+24; y++) {
		for (k = ball_xPos-2; k <= ball_xPos+10; k++) {
			VRAM_A[y*256+k] = 0x7FFF;
		}
	}

	// Draw left paddle (back)
	for (y = leftpaddle_yPos-4; y <= leftpaddle_yPos+36; y++) {
		for (k = 8; k <= 16; k++) {
			VRAM_A[y*256+k] = 0x7FFF;
		}
	}

	// Draw right paddle (back)
	for (y = rightpaddle_yPos-16; y <= rightpaddle_yPos+48; y++) {
		for (k = 240; k <= 248; k++) {
			VRAM_A[y*256+k] = 0x7FFF;
		}
	}

	// Draw ball
	for (y = ball_yPos; y <= ball_yPos+8; y++) {
		for (k = ball_xPos; k <= ball_xPos+8; k++) {
			VRAM_A[y*256+k] = pong_color;
		}
	}

	// Draw left paddle
	for (y = leftpaddle_yPos; y <= leftpaddle_yPos+32; y++) {
		for (k = 8; k <= 16; k++) {
			VRAM_A[y*256+k] = pong_color;
		}
	}

	// Draw right paddle
	for (y = rightpaddle_yPos; y <= rightpaddle_yPos+32; y++) {
		for (k = 240; k <= 248; k++) {
			VRAM_A[y*256+k] = pong_color;
		}
	}

	if(ball_moveUD==false) {
		ball_yPos -= ball_ySpeed;
		if(ball_yPos <= 0) ball_moveUD = true;
	} else if(ball_moveUD==true) {
		ball_yPos += ball_ySpeed;
		if(ball_yPos >= 184) ball_moveUD = false;
	}

	if(ball_moveLR==false) {
		ball_xPos -= 2;
		if((ball_yPos > leftpaddle_yPos-8) && (ball_yPos < leftpaddle_yPos+32)) {
			if(ball_xPos == 16) {
				ball_moveLR = true;
				if(ball_ySpeed != 6) {
					ball_ySpeed++;	// Increase Y speed of ball
				}
			}
		} else {
			if(ball_xPos <= 0) {
				// Restart minigame
				for (i = 0; i < 256*192; i++) {
					VRAM_A[i] = 0x7FFF;
				}
				leftpaddle_yPos = 76;
				rightpaddle_yPos = 76;
				ball_xPos = 16;
				ball_yPos = 64;
				ball_ySpeed = 2;
				ball_moveUD = false;	// false = up, true = down
				ball_moveLR = true;	// false = left, true = right
			}
		}
	} else if(ball_moveLR==true) {
		ball_xPos += 2;
		if(ball_xPos == 236) {
			ball_moveLR = false;
			if(ball_ySpeed != 6) {
				ball_ySpeed++;	// Increase Y speed of ball
			}
		}
	}

	// Control left paddle
	if(REG_KEYINPUT & (KEY_UP)) {} else {
		leftpaddle_yPos -= 4;
		if(leftpaddle_yPos <= 0) leftpaddle_yPos = 0;
	}
	if(REG_KEYINPUT & (KEY_DOWN)) {} else {
		leftpaddle_yPos += 4;
		if(leftpaddle_yPos >= 156) leftpaddle_yPos = 156;
	}

	// Control right paddle
	/*if(REG_KEYINPUT & (KEY_X)) {} else {
		rightpaddle_yPos--;
		if(rightpaddle_yPos <= 0) rightpaddle_yPos = 0;
	}
	if(REG_KEYINPUT & (KEY_B)) {} else {
		rightpaddle_yPos++;
		if(rightpaddle_yPos >= 156) rightpaddle_yPos = 156;
	}*/

}

static int ttt_selected[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};	// 0 = Blank, 1 = N, 2 = T
static int ttt_highlighted = 0;

static u16 ttt_rectColor[9] = {0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF};

static u16 ttt_selColor = 0x03A0;

//static bool ttt_keypressed = false;

static int ttt_drawXpos = 0;
static int ttt_drawYpos = 0;

static void ttt_drawN (void) {
	// 1st h line
	for (y = ttt_drawYpos+0; y <= ttt_drawYpos+39; y++) {
		for (k = ttt_drawXpos+0; k <= ttt_drawXpos+7; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
	// 2nd h line
	for (y = ttt_drawYpos+8; y <= ttt_drawYpos+15; y++) {
		for (k = ttt_drawXpos+8; k <= ttt_drawXpos+15; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
	// 3rd h line
	for (y = ttt_drawYpos+16; y <= ttt_drawYpos+23; y++) {
		for (k = ttt_drawXpos+16; k <= ttt_drawXpos+23; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
	// 4th h line
	for (y = ttt_drawYpos+24; y <= ttt_drawYpos+31; y++) {
		for (k = ttt_drawXpos+24; k <= ttt_drawXpos+31; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
	// 5th h line
	for (y = ttt_drawYpos+0; y <= ttt_drawYpos+39; y++) {
		for (k = ttt_drawXpos+32; k <= ttt_drawXpos+39; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
}

static void ttt_drawT (void) {
	// Top
	for (y = ttt_drawYpos+0; y <= ttt_drawYpos+7; y++) {
		for (k = ttt_drawXpos+0; k <= ttt_drawXpos+39; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
	// Bottom
	for (y = ttt_drawYpos+8; y <= ttt_drawYpos+39; y++) {
		for (k = ttt_drawXpos+16; k <= ttt_drawXpos+23; k++) {
			VRAM_A[y*256+k] = 0x0000;
		}
	}
}

static void arm9_ttt (void) {
	if(!drawnStuff) {
		REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw white BG
		for (i = 0; i < 256*192; i++) {
			VRAM_A[i] = 0x7FFF;
		}

		// Draw top of v line 1
		for (y = 56; y <= 59; y++) {
			for (k = 0; k <= 256; k++) {
				VRAM_A[y*256+k] = 0x4631;
			}
		}

		// Draw v line 1
		for (y = 60; y <= 67; y++) {
			for (k = 0; k <= 256; k++) {
				VRAM_A[y*256+k] = 0x0000;
			}
		}

		// Draw v line 2
		for (y = 124; y <= 131; y++) {
			for (k = 0; k <= 256; k++) {
				VRAM_A[y*256+k] = 0x0000;
			}
		}

		// Draw bottom of v line 2
		for (y = 132; y <= 135; y++) {
			for (k = 0; k <= 256; k++) {
				VRAM_A[y*256+k] = 0x4631;
			}
		}

		// Draw h line 1
		for (y = 0; y <= 191; y++) {
			for (k = 80; k <= 87; k++) {
				VRAM_A[y*256+k] = 0x0000;
			}
		}

		// Draw h line 2
		for (y = 0; y <= 191; y++) {
			for (k = 168; k <= 175; k++) {
				VRAM_A[y*256+k] = 0x0000;
			}
		}

		drawnStuff = true;
	}
	
	if(arm9_errorColor) ttt_selColor = 0x001B;
	
	// Draw highlighter, vertical line 1
	for (e = 0; e <= 2; e++) {
		// Vertical parts
		for (y = 0; y <= 7; y++) {
			for (k = 0; k <= 79; k++) {
				VRAM_A[y*256+k+e*88] = ttt_rectColor[e];
			}
		}
		for (y = 48; y <= 55; y++) {
			for (k = 0; k <= 79; k++) {
				VRAM_A[y*256+k+e*88] = ttt_rectColor[e];
			}
		}
		// Horizontal parts
		for (i = 0; i <= 72; i += 72) {
			for (y = 8; y <= 47; y++) {
				for (k = 0; k <= 7; k++) {
					VRAM_A[y*256+k+i+e*88] = ttt_rectColor[e];
				}
			}
		}
	}

	// Draw highlighter, vertical line 2
	for (e = 0; e <= 2; e++) {
		// Vertical parts
		for (y = 68; y <= 75; y++) {
			for (k = 0; k <= 79; k++) {
				VRAM_A[y*256+k+e*88] = ttt_rectColor[e+3];
			}
		}
		for (y = 116; y <= 123; y++) {
			for (k = 0; k <= 79; k++) {
				VRAM_A[y*256+k+e*88] = ttt_rectColor[e+3];
			}
		}
		// Horizontal parts
		for (i = 0; i <= 72; i += 72) {
			for (y = 76; y <= 115; y++) {
				for (k = 0; k <= 7; k++) {
					VRAM_A[y*256+k+i+e*88] = ttt_rectColor[e+3];
				}
			}
		}
	}

	// Draw highlighter, vertical line 3
	for (e = 0; e <= 2; e++) {
		// Vertical parts
		for (y = 136; y <= 143; y++) {
			for (k = 0; k <= 79; k++) {
				VRAM_A[y*256+k+e*88] = ttt_rectColor[e+6];
			}
		}
		for (y = 184; y <= 191; y++) {
			for (k = 0; k <= 79; k++) {
				VRAM_A[y*256+k+e*88] = ttt_rectColor[e+6];
			}
		}
		// Horizontal parts
		for (i = 0; i <= 72; i += 72) {
			for (y = 144; y <= 183; y++) {
				for (k = 0; k <= 7; k++) {
					VRAM_A[y*256+k+i+e*88] = ttt_rectColor[e+6];
				}
			}
		}
	}

	while(REG_VCOUNT!=191);	// fix speed

	// Control highlighter
	if(REG_KEYINPUT & (KEY_UP)) {} else {
		ttt_highlighted -= 3;
		if(ttt_highlighted < 0) ttt_highlighted = 0;
		//ttt_keypressed = true;
	}
	if(REG_KEYINPUT & (KEY_DOWN)) {} else {
		ttt_highlighted += 3;
		if(ttt_highlighted > 8) ttt_highlighted = 8;
		//ttt_keypressed = true;
	}
	if(REG_KEYINPUT & (KEY_LEFT)) {} else {
		ttt_highlighted--;
		if(ttt_highlighted < 0) ttt_highlighted = 0;
		//ttt_keypressed = true;
	}
	if(REG_KEYINPUT & (KEY_RIGHT)) {} else {
		ttt_highlighted++;
		if(ttt_highlighted > 8) ttt_highlighted = 8;
		//ttt_keypressed = true;
	}

	if(REG_KEYINPUT & (KEY_L)) {} else {
		if(ttt_selected[ttt_highlighted] == 0){
			ttt_selected[ttt_highlighted] = 1;	// N
			// Set X position
			switch(ttt_highlighted) {
				case 0:
				case 3:
				case 6:
				default:
					ttt_drawXpos = 20;
					break;
				case 1:
				case 4:
				case 7:
					ttt_drawXpos = 108;
					break;
				case 2:
				case 5:
				case 8:
					ttt_drawXpos = 196;
					break;
			}
			// Set Y position
			switch(ttt_highlighted) {
				case 0:
				case 1:
				case 2:
				default:
					ttt_drawYpos = 8;
					break;
				case 3:
				case 4:
				case 5:
					ttt_drawYpos = 76;
					break;
				case 6:
				case 7:
				case 8:
					ttt_drawYpos = 144;
					break;
			}
			ttt_drawN();
		}
	}
	if(REG_KEYINPUT & (KEY_R)) {} else {
		if(ttt_selected[ttt_highlighted] == 0){
			ttt_selected[ttt_highlighted] = 2;	// T
			// Set X position
			switch(ttt_highlighted) {
				case 0:
				case 3:
				case 6:
				default:
					ttt_drawXpos = 20;
					break;
				case 1:
				case 4:
				case 7:
					ttt_drawXpos = 108;
					break;
				case 2:
				case 5:
				case 8:
					ttt_drawXpos = 196;
					break;
			}
			// Set Y position
			switch(ttt_highlighted) {
				case 0:
				case 1:
				case 2:
				default:
					ttt_drawYpos = 8;
					break;
				case 3:
				case 4:
				case 5:
					ttt_drawYpos = 76;
					break;
				case 6:
				case 7:
				case 8:
					ttt_drawYpos = 144;
					break;
			}
			ttt_drawT();
		}
	}

	if(REG_KEYINPUT & (KEY_START)) {} else {
		// Clear all marks
		drawnStuff = false;
		for(i = 0; i < 9; i++) ttt_selected[i] = 0;
	}

	if(ttt_highlighted == 0) {
		ttt_rectColor[0] = ttt_selColor;
	} else {
		ttt_rectColor[0] = 0x7FFF;
	}
	if(ttt_highlighted == 1) {
		ttt_rectColor[1] = ttt_selColor;
	} else {
		ttt_rectColor[1] = 0x7FFF;
	}
	if(ttt_highlighted == 2) {
		ttt_rectColor[2] = ttt_selColor;
	} else {
		ttt_rectColor[2] = 0x7FFF;
	}
	if(ttt_highlighted == 3) {
		ttt_rectColor[3] = ttt_selColor;
	} else {
		ttt_rectColor[3] = 0x7FFF;
	}
	if(ttt_highlighted == 4) {
		ttt_rectColor[4] = ttt_selColor;
	} else {
		ttt_rectColor[4] = 0x7FFF;
	}
	if(ttt_highlighted == 5) {
		ttt_rectColor[5] = ttt_selColor;
	} else {
		ttt_rectColor[5] = 0x7FFF;
	}
	if(ttt_highlighted == 6) {
		ttt_rectColor[6] = ttt_selColor;
	} else {
		ttt_rectColor[6] = 0x7FFF;
	}
	if(ttt_highlighted == 7) {
		ttt_rectColor[7] = ttt_selColor;
	} else {
		ttt_rectColor[7] = 0x7FFF;
	}
	if(ttt_highlighted == 8) {
		ttt_rectColor[8] = ttt_selColor;
	} else {
		ttt_rectColor[8] = 0x7FFF;
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
			REG_SCFG_EXT = 0x8300C000;
		} else {
			REG_SCFG_EXT = 0x83008000;
		}
		*(u32*)(0x23ffc40) = 01;
		*(u32*)(0x2fffc40) = 01;
		arm9_SCFG_EXT = REG_SCFG_EXT;
		if (arm9_stateFlag == ARM9_DISPERR) {
			displayScreen = true;
			if ( arm9_stateFlag == ARM9_DISPERR) {
				arm9_stateFlag = ARM9_READY;
			}
		}
		if(displayScreen) {
			if(arm9_screenMode == 2) {
				arm9_ttt();
			} else if(arm9_screenMode == 1) {
				arm9_pong();
			} else {
				arm9_regularLoadingScreen();
				if(arm9_errorColor) arm9_errorText();
				if(arm9_animateLoadingCircle) arm9_loadingCircle();
			}
		}
	}

	REG_IME=0;
	REG_EXMEMCNT = 0xE880;
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	VoidFn arm9code = *(VoidFn*)(0x2FFFE24);
	arm9code();
	*(u32*)(0x4004008) = 0x83008000;
	while(1);
}

