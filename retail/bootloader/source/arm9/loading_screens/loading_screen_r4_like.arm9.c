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

static u16 bgColor;

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
		baseColor = arm9_darkTheme ? 0x2D6B : 0x5294;

		color[0] = arm9_darkTheme ? 0x0C63 : 0x739C;
		color[1] = arm9_darkTheme ? 0x1084 : 0x6F7B;
		color[2] = arm9_darkTheme ? 0x14A5 : 0x6B5A;
		color[3] = arm9_darkTheme ? 0x18C6 : 0x6739;
		color[4] = arm9_darkTheme ? 0x1CE7 : 0x6318;
		color[5] = arm9_darkTheme ? 0x2108 : 0x5EF7;
		color[6] = arm9_darkTheme ? 0x2529 : 0x5AD6;
		color[7] = arm9_darkTheme ? 0x294A : 0x56B5;

		bgColor = arm9_darkTheme ? 0x0842 : 0x7fff;

			errorColor = 0x001B;

		if (!arm9_swapLcds) {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
		} else {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);
		}
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		u16* bgLocation = (u16*)0x02700000;

		// Draw Loading Screen Image
		for (int i = 0; i < 256*192; i++) {
			//VRAM_A[i] = bgColor;
			VRAM_A[i] = *(bgLocation+i);
		}

		drawnStuff = true;
	}

	// Draw loading bar
	drawRectangleGradient(16, 91, 30*(arm9_loadBarLength+1), 10, 16, 16, 16);

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

		drawRectangle (1, 89, 4, 4, dot[6]);
		drawRectangle (6, 89, 4, 4, dot[5]);
		drawRectangle (11, 89, 4, 4, dot[4]);

		drawRectangle (1, 94, 4, 4, dot[7]);
		drawRectangle (6, 94, 4, 4, baseColor);
		drawRectangle (11, 94, 4, 4, dot[3]);

		drawRectangle (1, 99, 4, 4, dot[0]);
		drawRectangle (6, 99, 4, 4, dot[1]);
		drawRectangle (11, 99, 4, 4, dot[2]);

		loadingCircleFrame++;
		if (loadingCircleFrame == 8) loadingCircleFrame = 0;
	} else {
		loadingCircleTime++;
	}
}

void arm9_errorText3(void) {
	// Cover "Loading..." text
	for (int i = 0; i < 256*48; i++) {
		VRAM_A[i] = 0x7FFF;
	}

	// Draw "Error!" text
	// E: Part 1
	for (int y = 12; y <= 44; y++) {
		for (int k = 76; k <= 79; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// E: Part 2
	for (int y = 12; y <= 15; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// E: Part 3
	for (int y = 26; y <= 29; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// E: Part 4
	for (int y = 41; y <= 44; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// Draw 2 r's.
	for (int i = 0; i < 2; i++) {
		// Draw "Error!" text
		// r: Part 1
		for (int y = 23; y <= 44; y++) {
			for (int k = 95; k <= 98; k++) {
				VRAM_A[y*256+k+i*19] = baseColor;
			}
		}

		// Draw "Error!" text
		// r: Part 2
		for (int y = 26; y <= 29; y++) {
			for (int k = 99; k <= 102; k++) {
				VRAM_A[y*256+k+i*19] = baseColor;
			}
		}

		// Draw "Error!" text
		// r: Part 3
		for (int y = 23; y <= 26; y++) {
			for (int k = 103; k <= 109; k++) {
				VRAM_A[y*256+k+i*19] = baseColor;
			}
		}
	}

	// Draw "Error!" text
	// o: Part 1
	for (int y = 23; y <= 26; y++) {
		for (int k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// o: Part 2
	for (int y = 41; y <= 44; y++) {
		for (int k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// o: Part 3
	for (int y = 27; y <= 40; y++) {
		for (int k = 133; k <= 136; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// o: Part 4
	for (int y = 27; y <= 40; y++) {
		for (int k = 148; k <= 151; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 1
	for (int y = 23; y <= 44; y++) {
		for (int k = 156; k <= 159; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 2
	for (int y = 26; y <= 29; y++) {
		for (int k = 160; k <= 163; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 3
	for (int y = 23; y <= 26; y++) {
		for (int k = 164; k <= 170; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// !: Part 1
	for (int y = 12; y <= 32; y++) {
		for (int k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// Draw "Error!" text
	// !: Part 2
	for (int y = 38; y <= 44; y++) {
		for (int k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = baseColor;
		}
	}

	// End of Draw "Error!" text

	// Change dots of loading circle to form an X
	drawRectangle (1, 89, 4, 4, errorColor);
	drawRectangle (11, 89, 4, 4, errorColor);
	drawRectangle (6, 94, 4, 4, errorColor);
	drawRectangle (1, 99, 4, 4, errorColor);
	drawRectangle (11, 99, 4, 4, errorColor);

	arm9_animateLoadingCircle = false;
	displayScreen = false;
}
