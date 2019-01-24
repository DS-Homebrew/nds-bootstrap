/*-------------------------------------------------------------------------
Loading bar w/ Black BG "Simple"
--------------------------------------------------------------------------*/

#define ARM9
#undef ARM7

#include <nds/system.h>
#include <nds/arm9/video.h>

#include "common.h"
#include "loading.h"

static bool drawnStuff = false;

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
Modified by RocketRobz.
Just Loading Bar (code originally written by RocketRobz, modified by FlameKat53, original image by Uupo03)
--------------------------------------------------------------------------*/
void arm9_simpleLoadingScreen(void) {
	if (!drawnStuff) {
		REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw Black BG
		for (int i = 0; i < 256*192; i++) {
			VRAM_A[i] = 0x0000;
		}

		// Draw loading bar top edge
		for (int y = 82; y <= 86; y++) {
			for (int k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar top edge (shade line)
		for (int k = 8; k <= 247; k++) {
			VRAM_A[87*256+k] = 0x5AD6;
		}

		// Draw loading bar bottom edge (shade line)
		for (int k = 8; k <= 247; k++) {
			VRAM_A[112*256+k] = 0x5AD6;
		}

		// Draw loading bar bottom edge
		for (int y = 113; y <= 117; y++) {
			for (int k = 8; k <= 247; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar left edge
		for (int y = 88; y <= 111; y++) {
			for (int k = 2; k <= 6; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar left edge (shade line)
		for (int y = 88; y <= 111; y++) {
			VRAM_A[y*256+7] = 0x5AD6;
		}

		// Draw loading bar right edge
		for (int y = 88; y <= 111; y++) {
			for (int k = 248; k <= 252; k++) {
				VRAM_A[y*256+k] = 0x5294;
			}
		}

		// Draw loading bar right edge (shade line)
		for (int y = 88; y <= 111; y++) {
			VRAM_A[y*256+253] = 0x5AD6;
		}
		
		drawnStuff = true;
	}

	// Draw loading bar
	for (int i = 0; i <= arm9_loadBarLength; i++) {
		for (int y = 88; y <= 111; y++) {
			for (int k = 30*i+8; k < 30*i+38; k++) {
				VRAM_A[y*256+k] = 0x6F7B;
			}
		}
	}
	
}

void arm9_errorText(void) {

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

	// End of Drawing "Error!" text

	displayScreen = false;
}