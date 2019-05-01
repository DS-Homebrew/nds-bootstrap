/*-------------------------------------------------------------------------
Loading regular
--------------------------------------------------------------------------*/

#define ARM9
#undef ARM7

#include <nds/system.h>
#include <nds/arm9/video.h>

#include "common.h"
#include "loading.h"

extern void drawRectangle (int x, int y, int sizeX, int sizeY, u16 color);

void arm9_errorText(void) {
	VRAM_A_CR = VRAM_ENABLE;

	// Draw white BG
	for (int i = 0; i < 256*192; i++) {
		VRAM_A[i] = 0x7FFF;
	}

	// Draw "Error!" text
	// E: Part 1
	for (int y = 12; y <= 44; y++) {
		for (int k = 76; k <= 79; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// E: Part 2
	for (int y = 12; y <= 15; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// E: Part 3
	for (int y = 26; y <= 29; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// E: Part 4
	for (int y = 41; y <= 44; y++) {
		for (int k = 80; k <= 90; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// Draw 2 r's.
	for (int i = 0; i < 2; i++) {
		// Draw "Error!" text
		// r: Part 1
		for (int y = 23; y <= 44; y++) {
			for (int k = 95; k <= 98; k++) {
				VRAM_A[y*256+k+i*19] = 0x5294;
			}
		}

		// Draw "Error!" text
		// r: Part 2
		for (int y = 26; y <= 29; y++) {
			for (int k = 99; k <= 102; k++) {
				VRAM_A[y*256+k+i*19] = 0x5294;
			}
		}

		// Draw "Error!" text
		// r: Part 3
		for (int y = 23; y <= 26; y++) {
			for (int k = 103; k <= 109; k++) {
				VRAM_A[y*256+k+i*19] = 0x5294;
			}
		}
	}

	// Draw "Error!" text
	// o: Part 1
	for (int y = 23; y <= 26; y++) {
		for (int k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// o: Part 2
	for (int y = 41; y <= 44; y++) {
		for (int k = 137; k <= 147; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// o: Part 3
	for (int y = 27; y <= 40; y++) {
		for (int k = 133; k <= 136; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// o: Part 4
	for (int y = 27; y <= 40; y++) {
		for (int k = 148; k <= 151; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 1
	for (int y = 23; y <= 44; y++) {
		for (int k = 156; k <= 159; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 2
	for (int y = 26; y <= 29; y++) {
		for (int k = 160; k <= 163; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// 3rd r: Part 3
	for (int y = 23; y <= 26; y++) {
		for (int k = 164; k <= 170; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// !: Part 1
	for (int y = 12; y <= 32; y++) {
		for (int k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// Draw "Error!" text
	// !: Part 2
	for (int y = 38; y <= 44; y++) {
		for (int k = 175; k <= 178; k++) {
			VRAM_A[y*256+k] = 0x5294;
		}
	}

	// End of Draw "Error!" text

	// Draw X
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

	REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
	REG_DISPCNT = MODE_FB0;
}
