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

static u16 color1;
static u16 color2;
static u16 color3;
static u16 color4;
static u16 color5;
static u16 color6;
static u16 color7;
static u16 color8;

/*-------------------------------------------------------------------------
arm9_errorOutput
Displays an error code on screen.
Written by Chishm.
Modified by Robz8:
 * Replace dots with brand new loading screen (original image made by Uupo03)
--------------------------------------------------------------------------*/
void arm9_regularLoadingScreen(void) {
	if (!drawnStuff) {
		REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw white BG
		for (int i = 0; i < 256*192; i++) {
			VRAM_A[i] = 0x7FFF;
		}

		// Draw "Loading..." text
		// L: Part 1
		for (int y = 10; y <= 42; y++) {
			for (int k = 54; k <= 57; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// L: Part 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 58; k <= 68; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 77; k <= 87; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 77; k <= 87; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 3
		for (int y = 25; y <= 38; y++) {
			for (int k = 73; k <= 76; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// o: Part 4
		for (int y = 25; y <= 38; y++) {
			for (int k = 88; k <= 91; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 96; k <= 110; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 2
		for (int y = 25; y <= 42; y++) {
			for (int k = 111; k <= 114; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 3
		for (int y = 32; y <= 38; y++) {
			for (int k = 96; k <= 99; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 4
		for (int y = 28; y <= 31; y++) {
			for (int k = 100; k <= 110; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// a: Part 5
		for (int y = 39; y <= 42; y++) {
			for (int k = 100; k <= 110; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 123; k <= 133; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 123; k <= 133; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 3
		for (int y = 25; y <= 38; y++) {
			for (int k = 119; k <= 122; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// d: Part 4
		for (int y = 10; y <= 42; y++) {
			for (int k = 134; k <= 137; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// i: Part 1
		for (int y = 13; y <= 16; y++) {
			for (int k = 142; k <= 145; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// i: Part 2
		for (int y = 21; y <= 42; y++) {
			for (int k = 142; k <= 145; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// n: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 150; k <= 164; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// n: Part 2
		for (int y = 25; y <= 42; y++) {
			for (int k = 150; k <= 153; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// n: Part 3
		for (int y = 25; y <= 42; y++) {
			for (int k = 165; k <= 168; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 1
		for (int y = 21; y <= 24; y++) {
			for (int k = 177; k <= 187; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 2
		for (int y = 36; y <= 39; y++) {
			for (int k = 177; k <= 187; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 3
		for (int y = 43; y <= 46; y++) {
			for (int k = 173; k <= 187; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 4
		for (int y = 25; y <= 35; y++) {
			for (int k = 173; k <= 176; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// g: Part 5
		for (int y = 21; y <= 42; y++) {
			for (int k = 188; k <= 191; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// Dot 1
		for (int y = 39; y <= 42; y++) {
			for (int k = 196; k <= 199; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// Dot 2
		for (int y = 39; y <= 42; y++) {
			for (int k = 204; k <= 207; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// Draw "Loading..." text
		// Dot 3
		for (int y = 39; y <= 42; y++) {
			for (int k = 212; k <= 215; k++) {
				VRAM_A[y*256+k] = 0x5AD6;
			}
		}

		// End of Draw "Loading..." text

		// Draw loading bar top edge
		for (int y = 154; y <= 158; y++) {
			for (int k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar top edge (shade line)
		for (int k = 8; k <= 247; k++) {
			VRAM_A[159*256+k] = 0x5AD6;
		}

		// Draw loading bar bottom edge (shade line)
		for (int k = 8; k <= 247; k++) {
			VRAM_A[184*256+k] = 0x5AD6;
		}

		// Draw loading bar bottom edge
		for (int y = 185; y <= 189; y++) {
			for (int k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar left edge
		for (int y = 160; y <= 183; y++) {
			for (int k = 2; k <= 6; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar left edge (shade line)
		for (int y = 160; y <= 183; y++) {
			VRAM_A[y*256+7] = 0x5AD6;
		}

		// Draw loading bar right edge
		for (int y = 160; y <= 183; y++) {
			for (int k = 248; k <= 252; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar right edge (shade line)
		for (int y = 160; y <= 183; y++) {
			VRAM_A[y*256+253] = 0x5AD6;
		}
		
		drawnStuff = true;
	}

	// Draw loading bar
	for (int i = 0; i <= arm9_loadBarLength; i++) {
		for (int y = 160; y <= 183; y++) {
			for (int k = 30*i+8; k < 30*i+38; k++) {
				VRAM_A[y*256+k] = 0x6F7B;
			}
		}
	}
	
	arm9_animateLoadingCircle = true;
}

void arm9_loadingCircle(void) {
	switch (loadingCircleFrame) {
		case 0:
		default:
			color1 = 0x7BDE;
			color2 = 0x77BD;
			color3 = 0x739C;
			color4 = 0x6F7B;
			color5 = 0x6B5A;
			color6 = 0x6739;
			color7 = 0x5EF7;
			color8 = 0x56B5;
			break;
		case 1:
			color1 = 0x56B5;
			color2 = 0x7BDE;
			color3 = 0x77BD;
			color4 = 0x739C;
			color5 = 0x6F7B;
			color6 = 0x6B5A;
			color7 = 0x6739;
			color8 = 0x5EF7;
			break;
		case 2:
			color1 = 0x5EF7;
			color2 = 0x56B5;
			color3 = 0x7BDE;
			color4 = 0x77BD;
			color5 = 0x739C;
			color6 = 0x6F7B;
			color7 = 0x6B5A;
			color8 = 0x6739;
			break;
		case 3:
			color1 = 0x6739;
			color2 = 0x5EF7;
			color3 = 0x56B5;
			color4 = 0x7BDE;
			color5 = 0x77BD;
			color6 = 0x739C;
			color7 = 0x6F7B;
			color8 = 0x6B5A;
			break;
		case 4:
			color1 = 0x6B5A;
			color2 = 0x6739;
			color3 = 0x5EF7;
			color4 = 0x56B5;
			color5 = 0x7BDE;
			color6 = 0x77BD;
			color7 = 0x739C;
			color8 = 0x6F7B;
			break;
		case 5:
			color1 = 0x6F7B;
			color2 = 0x6B5A;
			color3 = 0x6739;
			color4 = 0x5EF7;
			color5 = 0x56B5;
			color6 = 0x7BDE;
			color7 = 0x77BD;
			color8 = 0x739C;
			break;
		case 6:
			color1 = 0x739C;
			color2 = 0x6F7B;
			color3 = 0x6B5A;
			color4 = 0x6739;
			color5 = 0x5EF7;
			color6 = 0x56B5;
			color7 = 0x7BDE;
			color8 = 0x77BD;
			break;
		case 7:
			color1 = 0x77BD;
			color2 = 0x739C;
			color3 = 0x6F7B;
			color4 = 0x6B5A;
			color5 = 0x6739;
			color6 = 0x5EF7;
			color7 = 0x56B5;
			color8 = 0x7BDE;
			break;
	}

	while (REG_VCOUNT!=191);

	// Draw loading circle
	if (loadingCircleTime == 3) {
		loadingCircleTime = 0;

		for (int y = 64; y <= 87; y++) {
			for (int k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = color1;
			}
			for (int k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = color2;
			}
			for (int k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = color3;
			}
		}	
		for (int y = 92; y <= 115; y++) {
			for (int k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = color8;
			}
			for (int k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
			for (int k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = color4;
			}
		}	
		for (int y = 120; y <= 143; y++) {
			for (int k = 88; k <= 111; k++) {
				VRAM_A[y*256+k] = color7;
			}
			for (int k = 116; k <= 139; k++) {
				VRAM_A[y*256+k] = color6;
			}
			for (int k = 144; k <= 167; k++) {
				VRAM_A[y*256+k] = color5;
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
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// E: Part 2
	for (int y = 12; y <= 15; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// E: Part 3
	for (int y = 26; y <= 29; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// E: Part 4
	for (int y = 41; y <= 44; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// Draw 2 r's.
	for (int i = 0; i < 2; i++) {
		// Draw "Error!" text
		// r: Part 1
		for (int y = 23; y <= 44; y++) {
			for (int k = 95; k <= 98; k++) {
				VRAM_A[y*256+k+i*19] = 0x5AD6;
			}
		}
		
		// Draw "Error!" text
		// r: Part 2
		for (int y = 26; y <= 29; y++) {
			for (int k = 99; k <= 102; k++) {
				VRAM_A[y*256+k+i*19] = 0x5AD6;
			}
		}
		
		// Draw "Error!" text
		// r: Part 3
		for (int y = 23; y <= 26; y++) {
			for (int k = 103; k <= 109; k++) {
				VRAM_A[y*256+k+i*19] = 0x5AD6;
			}
		}
	}
	
	// Draw "Error!" text
	// o: Part 1
	for (int y = 23; y <= 26; y++) {
		for (int k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// o: Part 2
	for (int y = 41; y <= 44; y++) {
		for (int k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// o: Part 3
	for (int y = 27; y <= 40; y++) {
		for (int k = 133; k <= 136; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// o: Part 4
	for (int y = 27; y <= 40; y++) {
		for (int k = 148; k <= 151; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 1
	for (int y = 23; y <= 44; y++) {
		for (int k = 156; k <= 159; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 2
	for (int y = 26; y <= 29; y++) {
		for (int k = 160; k <= 163; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 3
	for (int y = 23; y <= 26; y++) {
		for (int k = 164; k <= 170; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}
	
	// Draw "Error!" text
	// !: Part 1
	for (int y = 12; y <= 32; y++) {
		for (int k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// Draw "Error!" text
	// !: Part 2
	for (int y = 38; y <= 44; y++) {
		for (int k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = 0x5AD6;
		}
	}

	// End of Draw "Error!" text
	
	// Change dots of loading circle to form an X
	for (int y = 64; y <= 87; y++) {
		// 1st dot
		for (int k = 88; k <= 111; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
		// 3rd dot
		for (int k = 144; k <= 167; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
	}
	for (int y = 92; y <= 115; y++) {
		// 5th dot
		for (int k = 116; k <= 139; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
	}	
	for (int y = 120; y <= 143; y++) {
		// 7th dot
		for (int k = 88; k <= 111; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
		// 9th dot
		for (int k = 144; k <= 167; k++) {
			VRAM_A[y*256+k] = 0x001B;
		}
	}

	arm9_animateLoadingCircle = false;
	displayScreen = false;
}