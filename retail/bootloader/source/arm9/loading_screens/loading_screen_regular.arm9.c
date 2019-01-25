/*-------------------------------------------------------------------------
Loading regular
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

static u16 dot1;
static u16 dot2;
static u16 dot3;
static u16 dot4;
static u16 dot5;
static u16 dot6;
static u16 dot7;
static u16 dot8;

static u16 baseColor;

static u16 color1;
static u16 color2;
static u16 color3;
static u16 color4;
static u16 color5;
static u16 color6;
static u16 color7;
static u16 color8;

static bgColor;

static errorColor;
/*-------------------------------------------------------------------------
arm9_errorOutput
Displays an error code on screen.
Written by Chishm.
Modified by RocketRobz:
 * Replace dots with brand new loading screen (original image made by Uupo03)
--------------------------------------------------------------------------*/
void arm9_regularLoadingScreen(void) {
	if (!drawnStuff) {
		baseColor = darkTheme ? 0x2D6B : 0x5294;

		color1 = darkTheme ? 0x294A : 0x56B5;
		color2 = darkTheme ? 0x2529 : 0x5AD6;
		color3 = darkTheme ? 0x2108 : 0x5EF7;
		color4 = darkTheme ? 0x1CE7 : 0x6318;
		color5 = darkTheme ? 0x18C6 : 0x6739;
		color6 = darkTheme ? 0x14A5 : 0x6B5A;
		color7 = darkTheme ? 0x1084 : 0x6F7B;
		color8 = darkTheme ? 0x0C63 : 0x739C;

		bgColor = darkTheme ? 0x0842 : 0x7fff;

		//if (!darkTheme) {
			errorColor = 0x001B;
		//} else {
		//	errorColor = 0x001B;
		//}

		if (!swapLcds) {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
		} else {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);	
		}
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw white/dark BG
		for (int i = 0; i < 256*192; i++) {
			VRAM_A[i] = bgColor;
		}

		// Draw "Loading..." text
		// L: Part 1
		for (int y = 10; y <= 42; y++) {
			for (int k = 54; k <= 57; k++) {
				VRAM_A[y*256+k] = baseColor;	
			}
		}

		// Draw "Loading..." text
		// L: Part 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 58; k <= 68; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// o: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 77; k <= 87; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// o: Part 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 77; k <= 87; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// o: Part 3
		for (int y = 25; y <= 38; y++) {
			for (int k = 73; k <= 76; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// o: Part 4
		for (int y = 25; y <= 38; y++) {
			for (int k = 88; k <= 91; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// a: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 96; k <= 110; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// a: Part 2
		for (int y = 25; y <= 42; y++) {
			for (int k = 111; k <= 114; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// a: Part 3
		for (int y = 32; y <= 38; y++) {
			for (int k = 96; k <= 99; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// a: Part 4
		for (int y = 28; y <= 31; y++) {
			for (int k = 100; k <= 110; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// a: Part 5
		for (int y = 39; y <= 42; y++) {
			for (int k = 100; k <= 110; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// d: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 123; k <= 133; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// d: Part 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 123; k <= 133; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// d: Part 3
		for (int y = 25; y <= 38; y++) {
			for (int k = 119; k <= 122; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// d: Part 4
		for (int y = 10; y <= 42; y++) {
			for (int k = 134; k <= 137; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// i: Part 1
		for (int y = 13; y <= 16; y++) {
			for (int k = 142; k <= 145; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// i: Part 2
		for (int y = 21; y <= 42; y++) {
			for (int k = 142; k <= 145; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// n: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 150; k <= 164; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// n: Part 2
		for (int y = 25; y <= 42; y++) {
			for (int k = 150; k <= 153; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// n: Part 3
		for (int y = 25; y <= 42; y++) {
			for (int k = 165; k <= 168; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// g: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 177; k <= 187; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// g: Part 2
		for (int y = 36; y <= 39; y++) {
			for (int k = 177; k <= 187; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// g: Part 3
		for (int y = 43; y <= 46; y++) {
			for (int k = 173; k <= 187; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// g: Part 4
		for (int y = 25; y <= 35; y++) {
			for (int k = 173; k <= 176; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// g: Part 5
		for (int y = 21; y <= 42; y++) {
			for (int k = 188; k <= 191; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// Dot 1
		for (int y = 39; y <= 42; y++) {
			for (int k = 196; k <= 199; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// Dot 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 204; k <= 207; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw "Loading..." text
		// Dot 3
		for (int y = 39; y <= 42; y++) {
			for (int k = 212; k <= 215; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// End of Draw "Loading..." text

		// Draw loading bar top edge
		for (int y = 154; y <= 159; y++) {
			for (int k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw loading bar bottom edge
		for (int y = 184; y <= 189; y++) {
			for (int k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw loading bar left edge
		for (int y = 160; y <= 183; y++) {
			for (int k = 2; k <= 7; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}

		// Draw loading bar right edge
		for (int y = 160; y <= 183; y++) {
			for (int k = 247; k <= 252; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
		}
		
		drawnStuff = true;
	}

	// Draw loading bar
	for (int i = 0; i <= arm9_loadBarLength; i++) {
		for (int y = 160; y <= 183; y++) {
			for (int k = 30*i+8; k < 30*i+38; k++) {
				VRAM_A[y*256+k] = color6;
			}
		}
	}
	
	arm9_animateLoadingCircle = true;
}

void arm9_loadingCircle(void) {
	switch (loadingCircleFrame) {
		case 0:
			default:
			dot1 = color1;
			dot2 = color2;
			dot3 = color3;
			dot4 = color4;
			dot5 = color5;
			dot6 = color6;
			dot7 = color7;
			dot8 = color8;
			break;
		case 1:
			dot1 = color8;
			dot2 = color1;
			dot3 = color2;
			dot4 = color3;
			dot5 = color4;
			dot6 = color5;
			dot7 = color6;
			dot8 = color7;
			break;
		case 2:
			dot1 = color7;
			dot2 = color8;
			dot3 = color1;
			dot4 = color2;
			dot5 = color3;
			dot6 = color4;
			dot7 = color5;
			dot8 = color6;
			break;
		case 3:
			dot1 = color6;
			dot2 = color7;
			dot3 = color8;
			dot4 = color1;
			dot5 = color2;
			dot6 = color3;
			dot7 = color4;
			dot8 = color5;
			break;
		case 4:
			dot1 = color5;
			dot2 = color6;
			dot3 = color7;
			dot4 = color8;
			dot5 = color1;
			dot6 = color2;
			dot7 = color3;
			dot8 = color4;
			break;
		case 5:
			dot1 = color4;
			dot2 = color5;
			dot3 = color6;
			dot4 = color7;
			dot5 = color8;
			dot6 = color1;
			dot7 = color2;
			dot8 = color3;
			break;
		case 6:
			dot1 = color3;
			dot2 = color4;
			dot3 = color5;
			dot4 = color6;
			dot5 = color7;
			dot6 = color8;
			dot7 = color1;
			dot8 = color2;
			break;
		case 7:
			dot1 = color2;
			dot2 = color3;
			dot3 = color4;
			dot4 = color5;
			dot5 = color6;
			dot6 = color7;
			dot7 = color8;
			dot8 = color1;
			break;
	}

	while (REG_VCOUNT!=191);

	// Draw loading circle
	if (loadingCircleTime == 3) {
		loadingCircleTime = 0;

		for (int y = 64; y <= 87; y++) {
			for (int k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = dot1;
			}
			for (int k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = dot2;
			}
			for (int k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = dot3;
			}
		}	
		for (int y = 92; y <= 115; y++) {
			for (int k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = dot8;
			}
			for (int k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = baseColor;
			}
			for (int k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = dot4;
			}
		}	
		for (int y = 120; y <= 143; y++) {
			for (int k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = dot7;
			}
			for (int k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = dot6;
			}
			for (int k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = dot5;
			}
		}

		loadingCircleFrame++;
		if (loadingCircleFrame == 8) loadingCircleFrame = 0;
	} else {
		loadingCircleTime++;
	}
}

void arm9_errorText(void) {
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
	for (int y = 64; y <= 87; y++) {
		// 1st dot
		for (int k = 88; k <= 111; k++) {
			VRAM_A[y*256+k] = errorColor;
		}
		// 3rd dot
		for (int k = 144; k <= 167; k++) {
			VRAM_A[y*256+k] = errorColor;
		}
	}
	for (int y = 92; y <= 115; y++) {
		// 5th dot
		for (int k = 116; k <= 139; k++) {
			VRAM_A[y*256+k] = errorColor;
		}
	}	
	for (int y = 120; y <= 143; y++) {
		// 7th dot
		for (int k = 88; k <= 111; k++) {
			VRAM_A[y*256+k] = errorColor;
		}
		// 9th dot
		for (int k = 144; k <= 167; k++) {
			VRAM_A[y*256+k] = errorColor;
		}
	}

	arm9_animateLoadingCircle = false;
	displayScreen = false;
}
