/*-------------------------------------------------------------------------
Loading R4-Like
--------------------------------------------------------------------------*/

#define ARM9
#undef ARM7

#include <nds/system.h>
#include <nds/arm9/video.h>

#include "common.h"
#include "loading.h"

static bool drawnStuff = false;

static int loadingCircleTime = 3;
static int loadingCircleFrame = 0;

static u16 dot[8];

static u16 baseColor;

static u16 color[8];

static u16 errorColor;

extern void drawRectangle (int x, int y, int sizeX, int sizeY, u16 color);
extern void drawRectangleGradient (int x, int y, int sizeX, int sizeY, int R, int G, int B);
/*-------------------------------------------------------------------------
arm9_errorOutput
Displays an error code on screen.
Written by Chishm.
Modified by RocketRobz:
 * Replace dots with brand new loading screen (original image made by Uupo03)
--------------------------------------------------------------------------*/
void arm9_flashcardlikeLoadingScreen(void) {
	if (!drawnStuff) {
		baseColor = 0x2D6B;

		color[0] = 0x0C63;
		color[1] = 0x1084;
		color[2] = 0x14A5;
		color[3] = 0x18C6;
		color[4] = 0x1CE7;
		color[5] = 0x2108;
		color[6] = 0x2529;
		color[7] = 0x294A;

		errorColor = 0x001B;

		if (!arm9_swapLcds) {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
		} else {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);
		}
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		u16* bgLocation = (u16*)0x02D00000;

		// Draw Loading Screen Image
		dmaCopy(bgLocation, VRAM_A, 0x20000);

		drawnStuff = true;
	}

	// Draw loading bar
	drawRectangleGradient(16, arm9_loadingBarYpos+2, 28*(arm9_loadBarLength+1), 10, 16, 16, 16);

	arm9_animateLoadingCircle = true;
}

void arm9_loadingCircle2(void) {
	switch (loadingCircleFrame) {
		case 0:
			default:
			dot[0] = color[0];
			dot[1] = color[1];
			dot[2] = color[2];
			dot[3] = color[3];
			dot[4] = color[4];
			dot[5] = color[5];
			dot[6] = color[6];
			dot[7] = color[7];
			break;
		case 1:
			dot[0] = color[7];
			dot[1] = color[0];
			dot[2] = color[1];
			dot[3] = color[2];
			dot[4] = color[3];
			dot[5] = color[4];
			dot[6] = color[5];
			dot[7] = color[6];
			break;
		case 2:
			dot[0] = color[6];
			dot[1] = color[7];
			dot[2] = color[0];
			dot[3] = color[1];
			dot[4] = color[2];
			dot[5] = color[3];
			dot[6] = color[4];
			dot[7] = color[5];
			break;
		case 3:
			dot[0] = color[5];
			dot[1] = color[6];
			dot[2] = color[7];
			dot[3] = color[0];
			dot[4] = color[1];
			dot[5] = color[2];
			dot[6] = color[3];
			dot[7] = color[4];
			break;
		case 4:
			dot[0] = color[4];
			dot[1] = color[5];
			dot[2] = color[6];
			dot[3] = color[7];
			dot[4] = color[0];
			dot[5] = color[1];
			dot[6] = color[2];
			dot[7] = color[3];
			break;
		case 5:
			dot[0] = color[3];
			dot[1] = color[4];
			dot[2] = color[5];
			dot[3] = color[6];
			dot[4] = color[7];
			dot[5] = color[0];
			dot[6] = color[1];
			dot[7] = color[2];
			break;
		case 6:
			dot[0] = color[2];
			dot[1] = color[3];
			dot[2] = color[4];
			dot[3] = color[5];
			dot[4] = color[6];
			dot[5] = color[7];
			dot[6] = color[0];
			dot[7] = color[1];
			break;
		case 7:
			dot[0] = color[1];
			dot[1] = color[2];
			dot[2] = color[3];
			dot[3] = color[4];
			dot[4] = color[5];
			dot[5] = color[6];
			dot[6] = color[7];
			dot[7] = color[0];
			break;
	}

	while (REG_VCOUNT!=191);

	// Draw loading circle
	if (loadingCircleTime == 3) {
		loadingCircleTime = 0;

		drawRectangle (1, arm9_loadingBarYpos, 4, 4, dot[6]);
		drawRectangle (6, arm9_loadingBarYpos, 4, 4, dot[5]);
		drawRectangle (11, arm9_loadingBarYpos, 4, 4, dot[4]);

		drawRectangle (1, arm9_loadingBarYpos+5, 4, 4, dot[7]);
		drawRectangle (6, arm9_loadingBarYpos+5, 4, 4, baseColor);
		drawRectangle (11, arm9_loadingBarYpos+5, 4, 4, dot[3]);

		drawRectangle (1, arm9_loadingBarYpos+10, 4, 4, dot[0]);
		drawRectangle (6, arm9_loadingBarYpos+10, 4, 4, dot[1]);
		drawRectangle (11, arm9_loadingBarYpos+10, 4, 4, dot[2]);

		loadingCircleFrame++;
		if (loadingCircleFrame == 8) loadingCircleFrame = 0;
	} else {
		loadingCircleTime++;
	}
}

void arm9_errorText3(void) {
	// Change dots of loading circle to form an X
	drawRectangle (1, 89, 4, 4, errorColor);
	drawRectangle (11, 89, 4, 4, errorColor);
	drawRectangle (6, 94, 4, 4, errorColor);
	drawRectangle (1, 99, 4, 4, errorColor);
	drawRectangle (11, 99, 4, 4, errorColor);

	// Make loading bar red
	drawRectangleGradient(16, arm9_loadingBarYpos+2, 28*(arm9_loadBarLength+1), 10, 0, 16, 0);

	arm9_animateLoadingCircle = false;
	displayScreen = false;
}
