#include <nds/ndstypes.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>

#include "igm_text.h"
#include "locations.h"
#include "nds_header.h"
#include "tonccpy.h"

#include "font_bin.h"

// Needs to be font_bin_size * 4, but needs to be statically defined
#define FONT_SIZE 0x5C00

extern struct IgmText igmText;

extern u32 scfgExtBak;
extern u16 scfgClkBak;
extern vu32* volatile sharedAddr;

static char bgBak[FONT_SIZE];
static u16 bgMapBak[0x300];
static u16 palBak[256];

static u16 igmPal[] = {
	0xFFFF, // White
	0xDEF7, // Light gray
	0xCE73, // Darker gray
	0xF355, // Light blue
	0x801B, // Red
	0x8360, // Lime
};

#ifndef B4DS
static u16* vramBak = (u16*)INGAME_MENU_EXT_LOCATION+(0x18200/sizeof(u16));
static u16* bmpBuffer = (u16*)INGAME_MENU_EXT_LOCATION;

// Header for a 256x192 16 bit (RGBA 565) BMP
const static u8 bmpHeader[] = {
	0x42, 0x4D, 0x46, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00,
	0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xC0, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x01, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07,
	0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif

#define KEYS sharedAddr[5]

static void SetBrightness(u8 screen, s8 bright) {
	u8 mode = 1;

	if (bright < 0) {
		mode = 2;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(vu16*)(0x0400006C + (0x1000 * screen)) = bright | (mode << 14);
}

// For RAM viewer, global so it's persistant
static vu32 *address = (vu32*)0x02000000;
static bool arm7Ram = false;

static void print(int x, int y, const u16 *str, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	while(*str)
		*(dst++) = *(str++) | palette << 12;
}

static void printCenter(int x, int y, const u16 *str, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	const u16 *start = str;
	while(*str)
		str++;
	dst += (str - start) / 2;
	while(str != start)
		*(--dst) = *(--str) | palette << 12;
}

static void printRight(int x, int y, const u16 *str, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	const u16 *start = str;
	while(*str)
		str++;
	while(str != start)
		*(dst--) = *(--str) | palette << 12;
}

static void printChar(int x, int y, u16 c, int palette) {
	BG_MAP_RAM_SUB(15)[y * 0x20 + x] = c | palette << 12;
}

static void printDec(int x, int y, u32 val, int digits, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	for(int i = digits - 1; i >= 0; i--) {
		*(dst + i) = ('0' + (val % 10)) | palette << 12;
		val /= 10;
	}
}

static void printHex(int x, int y, u32 val, u8 bytes, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		*(dst + i) = ((val & 0xF) >= 0xA ? 'A' + (val & 0xF) - 0xA : '0' + (val & 0xF)) | palette << 12;
		val >>= 4;
	}
}

#ifndef B4DS
static void printBattery(void) {
	u8 batteryLevel = (u8)sharedAddr[6];
	const u16 *bars = u"\3\3";
	if (batteryLevel & BIT(7)) {
		bars = u"\6\6";	// Charging
	} else {
		switch (batteryLevel) {
			default:
				break;
			case 0x1:
			case 0x3:
				bars = u"\3\4";
				break;
			case 0x7:
				bars = u"\3\5";
				break;
			case 0xB:
				bars = u"\4\5";
				break;
			case 0xF:
				bars = u"\5\5";
				break;
		}
	}
	print(0x20 - 4, 0x18 - 2, bars, 3);
}
#endif

static void waitKeys(u16 keys) {
	// Prevent key repeat for 10 frames
	for(int i = 0; i < 10 && (KEYS & keys); i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	do {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	} while(!(KEYS & keys));
}

#ifndef B4DS
static void waitKeysBattery(u16 keys) {
	// Prevent key repeat for 10 frames
	for(int i = 0; i < 10 && (KEYS & keys); i++) {
		printBattery();
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	do {
		printBattery();
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	} while(!(KEYS & keys));
}
#endif

static void clearScreen(void) {
	toncset16(BG_MAP_RAM_SUB(15), 0, 0x300);
}

#define VRAM_x(bank) ((u16*)(0x6800000 + (0x0020000 * (bank))))
#define VRAM_x_CR(bank) (((vu8*)0x04000240)[bank])

#ifndef B4DS
static void screenshot(void) {
	// Try to find the safest bank to capture to
	u8 vramBank = 2;
	if((VRAM_D_CR & 1) == 0) {
		vramBank = 3;
	} else if((VRAM_C_CR & 1) == 0) {
		vramBank = 2;
	} else if((VRAM_B_CR & 7) == 0) {
		vramBank = 1;
	} else if((VRAM_A_CR & 7) == 0) {
		vramBank = 0;
	}

	// Use capture mode B if no banks are mapped for main engine
	u8 captureMode = DCAP_MODE_B;
	for(int i = 0; i < 4; i++) {
		if(VRAM_x_CR(i) & 1)
			captureMode = DCAP_MODE_A;
	}

	u8 vramCr = VRAM_x_CR(vramBank);

	// Select bank
	u8 cursorPosition = vramBank;
	while(1) {
		// Configure VRAM
		VRAM_x_CR(vramBank) = vramCr; // LCD
		vramCr = VRAM_x_CR(cursorPosition);
		VRAM_x_CR(cursorPosition) = VRAM_ENABLE; // LCD
		vramBank = cursorPosition;

		clearScreen();

		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 9 + 5, '-', 20);
		printCenter(15, 10, igmText.selectBank, 0);
		printChar(15, 12, 'A' + vramBank, 3);
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 13 + 5, '-', 20);

		u8 color = igmText.currentScreenshot == 50 ? 4 : 5;
		if(igmText.rtl) {
			printDec(6, 14, igmText.currentScreenshot, 2, color);
			printChar(8, 14, '/', color);
			printDec(9, 14, 50, 2, color);
			printRight(23, 14, igmText.count, 0);
		} else {
			print(6, 14, igmText.count, 0);
			printDec(19, 14, igmText.currentScreenshot, 2, color);
			printChar(21, 14, '/', color);
			printDec(22, 14, 50, 2, color);

		}

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B);

		if (KEYS & (KEY_UP | KEY_LEFT)) {
			if(vramBank > 0)
				cursorPosition--;
		} else if (KEYS & (KEY_DOWN | KEY_RIGHT)) {
			if(vramBank < 3)
				cursorPosition++;
		} else if(KEYS & KEY_A && igmText.currentScreenshot < 50) {
			break;
		} else if(KEYS & KEY_B) {
			VRAM_x_CR(vramBank) = vramCr;
			return;
		}
	}

	// Backup VRAM bank
	tonccpy(vramBak, VRAM_x(vramBank), 0x18000);

	REG_DISPCAPCNT = DCAP_BANK(vramBank) | DCAP_SIZE(DCAP_SIZE_256x192) | DCAP_MODE(captureMode) | DCAP_ENABLE;
	while(REG_DISPCAPCNT & DCAP_ENABLE);

	tonccpy(bmpBuffer, bmpHeader, sizeof(bmpHeader));

	// ABGR 1555 -> RGB 565
	for (int i = 0; i < 256 * 192; i++) {
		u16 val = VRAM_x(vramBank)[i];
		VRAM_x(vramBank)[i] = ((val >> 10) & 31) | ((val & (31 << 5)) << 1) | ((val & 31) << 11);
	}

	// Write image data, upside down as that's how BMPs want it
	u16 *bmp = bmpBuffer + sizeof(bmpHeader) / sizeof(u16);
	for(int i = 191; i >= 0; i--) {
		tonccpy(bmp, VRAM_x(vramBank) + (i * 256), 256 * sizeof(u16));
		bmp += 256;
	}

	// Restore VRAM bank
	tonccpy(VRAM_x(vramBank), vramBak, 0x18000);
	VRAM_x_CR(vramBank) = vramCr;

	sharedAddr[4] = 0x544F4853;
	while (sharedAddr[4] == 0x544F4853) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}
#endif

static void drawCursor(u8 line) {
	u8 pos = igmText.rtl ? 0x1F : 0;
	// Clear other cursors
	for(int i = 0; i < 0x18; i++)
		BG_MAP_RAM_SUB(15)[i * 0x20 + pos] = 0;

	// Set cursor on the selected line
	BG_MAP_RAM_SUB(15)[line * 0x20 + pos] = igmText.rtl ? '<' : '>';
}

static void drawMainMenu(void) {
	clearScreen();

	// Print labels
	#ifndef B4DS
	for(int i = 0; i < 7; i++) {
	#else
	for(int i = 0; i < 4; i++) {
	#endif
		if(igmText.rtl)
			printRight(0x1D, i, igmText.menu[i], 0);
		else
			print(2, i, igmText.menu[i], 0);
	}

	// Print info
	print(1, 0x18 - 3, igmText.ndsBootstrap, 1);
	print(1, 0x18 - 2, igmText.version, 1);
	#ifndef B4DS
	printChar(0x20 - 5, 0x18 - 2, '\2', 3);
	printChar(0x20 - 2, 0x18 - 2, '\7', 3);
	#endif
}

static void optionsMenu(s8 *mainScreen) {
	u8 cursorPosition = 0;
	while(1) {
		clearScreen();

		// Print labels
		for(int i = 0; i < 3; i++) {
			if(igmText.rtl)
				printRight(0x1D, i, igmText.options[i], 0);
			else
				print(2, i, igmText.options[i], 0);
		}
		drawCursor(cursorPosition);

		#ifndef B4DS
		if(igmText.rtl) {
			// Main screen
			print(1, 0, igmText.options[3 + (*mainScreen)] , 0);
			// Clock speed
			print(1, 1, igmText.options[6 + ((REG_SCFG_CLK==0 ? scfgClkBak : REG_SCFG_CLK) & 1)], 0);
			// VRAM mode
			print(1, 2, igmText.options[8 + (((REG_SCFG_EXT==0 ? scfgExtBak : REG_SCFG_EXT) & BIT(13)) >> 13)], 0);
		} else {
			// Main screen
			printRight(0x1E, 0, igmText.options[3 + (*mainScreen)] , 0);
			// Clock speed
			printRight(0x1E, 1, igmText.options[6 + ((REG_SCFG_CLK==0 ? scfgClkBak : REG_SCFG_CLK) & 1)], 0);
			// VRAM mode
			printRight(0x1E, 2, igmText.options[8 + (((REG_SCFG_EXT==0 ? scfgExtBak : REG_SCFG_EXT) & BIT(13)) >> 13)], 0);
		}

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_B);
		#else
		if(igmText.rtl) {
			// Main screen
			print(1, 0, igmText.options[3 + (*mainScreen)] , 0);
		} else {
			// Main screen
			printRight(0x1E, 0, igmText.options[3 + (*mainScreen)] , 0);
		}

		waitKeys(KEY_LEFT | KEY_RIGHT | KEY_B);
		#endif

		#ifndef B4DS
		if (KEYS & KEY_UP) {
			if(cursorPosition > 0)
				cursorPosition--;
		} else if (KEYS & KEY_DOWN) {
			if(cursorPosition < 2)
				cursorPosition++;
		} else
		#endif
		if (KEYS & (KEY_LEFT | KEY_RIGHT)) {
			switch(cursorPosition) {
				case 0:
					(KEYS & KEY_LEFT) ? (*mainScreen)-- : (*mainScreen)++;
					if(*mainScreen > 2)
						*mainScreen = 0;
					else if(*mainScreen < 0)
						*mainScreen = 2;
					break;
				#ifndef B4DS
				case 1:
					REG_SCFG_CLK ^= 1;
					break;
				case 2:
					REG_SCFG_EXT ^= BIT(13);
					break;
				#endif
				default:
					break;
			}
		} else if (KEYS & KEY_B) {
			return;
		}
	}
}

static void jumpToAddress(void) {
	clearScreen();

	u8 cursorPosition = 0;
	while(1) {
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 9 + 5, '-', 20);
		printCenter(15, 10, igmText.jumpAddress, 0);
		printHex(11, 12, (u32)address, 4, 3);
		BG_MAP_RAM_SUB(15)[0x20 * 12 + 11 + 6 - cursorPosition] = (BG_MAP_RAM_SUB(15)[0x20 * 12 + 11 + 6 - cursorPosition] & ~(0xF << 12)) | 4 << 12;
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 13 + 5, '-', 20);

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B);

		if(KEYS & KEY_UP) {
			address = (vu32*)(((u32)address & ~(0xF0 << cursorPosition * 4)) | (((u32)address + (0x10 << (cursorPosition * 4))) & (0xF0 << cursorPosition * 4)));
		} else if(KEYS & KEY_DOWN) {
			address = (vu32*)(((u32)address & ~(0xF0 << cursorPosition * 4)) | (((u32)address - (0x10 << (cursorPosition * 4))) & (0xF0 << cursorPosition * 4)));
		} else if(KEYS & KEY_LEFT) {
			if(cursorPosition < 6)
				cursorPosition++;
		} else if(KEYS & KEY_RIGHT) {
			if(cursorPosition > 0)
				cursorPosition--;
		} else if(KEYS & (KEY_A | KEY_B)) {
			return;
		}
	}
}

static void ramViewer(void) {
	clearScreen();

	u8 *arm7RamBuffer = ((u8*)sharedAddr) - 0x94C;
	bool ramLoaded = false;
	u8 cursorPosition = 0, mode = 0;
	while(1) {
		u8 *ramPtr = arm7Ram ? arm7RamBuffer : (u8*)address;

		u16 armText[5] = {'A', 'R', 'M', arm7Ram ? '7' : '9', 0};
		printCenter(14, 0, igmText.ramViewer, 0);
		print(27, 0, armText, 3);
		printHex(0, 0, (u32)address >> 0x10, 2, 3);

		if (arm7Ram && !ramLoaded) {
			sharedAddr[0] = (vu32)arm7RamBuffer;
			sharedAddr[1] = (vu32)address;
			sharedAddr[4] = 0x524D4152; // RAMR
			while (sharedAddr[4] == 0x524D4152) {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			}
		}
		ramLoaded = true;

		for(int i = 0; i < 23; i++) {
			printHex(0, i + 1, (u32)(address + (i * 2)) & 0xFFFF, 2, 3);
			for(int j = 0; j < 4; j++)
				printHex(5 + (j * 2), i + 1, ramPtr[(i * 8) + j], 1, 1 + j % 2);
			for(int j = 0; j < 4; j++)
				printHex(14 + (j * 2), i + 1, ramPtr[4 + (i * 8) + j], 1, 1 + j % 2);
			for(int j = 0; j < 8; j++)
				printChar(23 + j, i + 1, ramPtr[i * 8 + j], 0);
		}

		// Change color of selected byte
		if(mode > 0) {
			// Hex
			u16 loc = 0x20 * (1 + (cursorPosition / 8)) + 5 + ((cursorPosition % 8) * 2) + (cursorPosition % 8 >= 4);
			BG_MAP_RAM_SUB(15)[loc] = (BG_MAP_RAM_SUB(15)[loc] & ~(0xF << 12)) | (3 + mode) << 12;
			BG_MAP_RAM_SUB(15)[loc + 1] = (BG_MAP_RAM_SUB(15)[loc + 1] & ~(0xF << 12)) | (3 + mode) << 12;

			// Text
			loc = 0x20 * (1 + (cursorPosition / 8)) + 23 + (cursorPosition % 8);
			BG_MAP_RAM_SUB(15)[loc] = (BG_MAP_RAM_SUB(15)[loc] & ~(0xF << 12)) | (3 + mode) << 12;
		}

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B | KEY_Y | KEY_SELECT);

		if(mode == 0) {
			if(KEYS & KEY_R && KEYS & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
				if (KEYS & KEY_UP) {
					address -= 0x400;
					ramLoaded = false;
				} else if (KEYS & KEY_DOWN) {
					address += 0x400;
					ramLoaded = false;
				} else if (KEYS & KEY_LEFT) {
					address -= 0x4000;
					ramLoaded = false;
				} else if (KEYS & KEY_RIGHT) {
					address += 0x4000;
					ramLoaded = false;
				}
			} else {
				if (KEYS & KEY_UP) {
					address -= 2;
					ramLoaded = false;
				} else if (KEYS & KEY_DOWN) {
					address += 2;
					ramLoaded = false;
				} else if (KEYS & KEY_LEFT) {
					address -= 2 * 23;
					ramLoaded = false;
				} else if (KEYS & KEY_RIGHT) {
					address += 2 * 23;
					ramLoaded = false;
				} else if (KEYS & KEY_A) {
					mode = 1;
				} else if (KEYS & KEY_B) {
					return;
				} else if(KEYS & KEY_Y) {
					jumpToAddress();
					clearScreen();
					ramLoaded = false;
				}else if (KEYS & KEY_SELECT) {
					arm7Ram = !arm7Ram;
					ramLoaded = false;
				}
			}
		} else if(mode == 1) {
			if (KEYS & KEY_UP) {
				if(cursorPosition >= 8)
					cursorPosition -= 8;
				else
					address -= 2;
			} else if (KEYS & KEY_DOWN) {
				if(cursorPosition < 8 * 22)
					cursorPosition += 8;
				else
					address += 2;
			} else if (KEYS & KEY_LEFT) {
				if(cursorPosition > 0)
					cursorPosition--;
			} else if (KEYS & KEY_RIGHT) {
				if(cursorPosition < 8 * 23 - 1)
					cursorPosition++;
			} else if (KEYS & KEY_A) {
				mode = 2;
			} else if (KEYS & KEY_B) {
				mode = 0;
			} else if(KEYS & KEY_Y) {
				jumpToAddress();
				clearScreen();
			}
		} else if(mode == 2) {
			if (KEYS & KEY_UP) {
				ramPtr[cursorPosition]++;
			} else if (KEYS & KEY_DOWN) {
				ramPtr[cursorPosition]--;
			} else if (KEYS & KEY_LEFT) {
				ramPtr[cursorPosition] -= 0x10;
			} else if (KEYS & KEY_RIGHT) {
				ramPtr[cursorPosition] += 0x10;
			} else if (KEYS & (KEY_A | KEY_B)) {
				if(arm7Ram) {
					sharedAddr[0] = (vu32)arm7RamBuffer;
					sharedAddr[1] = (vu32)address;
					sharedAddr[2] = cursorPosition;
					sharedAddr[4] = 0x574D4152; // RAMW
					while (sharedAddr[4] == 0x574D4152) {
						while (REG_VCOUNT != 191);
						while (REG_VCOUNT == 191);
					}
					ramLoaded = false;
				}
				mode = 1;
			}
		}
	}
}

void inGameMenu(s8* mainScreen) {
	#ifndef B4DS
	int oldIME = enterCriticalSection();

	u16 exmemcnt = REG_EXMEMCNT;
	sysSetCardOwner(false);	// Give Slot-1 access to arm7
	#endif

	u32 dispcnt = REG_DISPCNT_SUB;
	u16 bg0cnt = REG_BG0CNT_SUB;
	u16 bg1cnt = REG_BG1CNT_SUB;
	u16 bg2cnt = REG_BG2CNT_SUB;
	u16 bg3cnt = REG_BG3CNT_SUB;

	u8 vramCCr = VRAM_C_CR;
	u8 vramHCr = VRAM_H_CR;

	u16 powercnt = REG_POWERCNT;

	u16 masterBright = *(vu16*)0x0400106C;

	REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG3_ACTIVE;
	REG_BG0CNT_SUB = 0;
	REG_BG1CNT_SUB = 0;
	REG_BG2CNT_SUB = 0;
	REG_BG3CNT_SUB = (u16)(BG_MAP_BASE(15) | BG_TILE_BASE(0) | BgSize_T_256x256);

	if(VRAM_C_CR & 4) // If VRAM C is mapped to sub bg, unmap it
		VRAM_C_CR = VRAM_C_LCD;
	VRAM_H_CR = VRAM_ENABLE | VRAM_H_SUB_BG;

	REG_BG3VOFS_SUB = 0;
	REG_BG3HOFS_SUB = 0;

	// If main screen is on auto, then force the bottom
	REG_POWERCNT |= POWER_SWAP_LCDS;

	SetBrightness(1, 0);
	REG_BLDY_SUB = 0; // Register is write only, can't back up

	tonccpy(bgMapBak, BG_MAP_RAM_SUB(15), sizeof(bgMapBak));	// Backup BG_MAP_RAM
	clearScreen();

	tonccpy(palBak, BG_PALETTE_SUB, sizeof(palBak));	// Backup the palette
	toncset16(BG_PALETTE_SUB, 0, 256);
	for(int i = 0; i < sizeof(igmPal) / sizeof(igmPal[0]); i++) {
		BG_PALETTE_SUB[i * 0x10 + 1] = igmPal[i];
	}

	tonccpy(bgBak, BG_GFX_SUB, FONT_SIZE);	// Backup the original graphics
	for(int i = 0; i < font_bin_size; i++) {	// Load font from 1bpp to 4bpp
		u8 val = font_bin[i];
		BG_GFX_SUB[i * 2]     = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
		val >>= 4;
		BG_GFX_SUB[i * 2 + 1] = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
	}

	// Let ARM7 know the menu loaded
	sharedAddr[5] = 0x59444552; // 'REDY'

	// Wait for keys to be released
	drawMainMenu();
	drawCursor(0);
	do {
		#ifndef B4DS
		printBattery();
		#endif
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	} while(KEYS & igmText.hotkey);

	u8 cursorPosition = 0;
	while (sharedAddr[4] == 0x554E454D) {
		drawMainMenu();
		drawCursor(cursorPosition);

		#ifndef B4DS
		waitKeysBattery(KEY_UP | KEY_DOWN | KEY_A | KEY_B | KEY_R);
		#else
		waitKeys(KEY_UP | KEY_DOWN | KEY_A | KEY_B);
		#endif

		if (KEYS & KEY_UP) {
			if (cursorPosition > 0)
				cursorPosition--;
		} else if (KEYS & KEY_DOWN) {
			#ifndef B4DS
			if (cursorPosition < 6)
			#else
			if (cursorPosition < 3)
			#endif
				cursorPosition++;
		} else if (KEYS & KEY_A) {
			#ifndef B4DS
			switch(cursorPosition) {
				case 0:
					do {
						while (REG_VCOUNT != 191);
						while (REG_VCOUNT == 191);
					} while(KEYS & KEY_A);
					sharedAddr[4] = 0x54495845; // EXIT
					break;
				case 1:
					sharedAddr[3] = 0x52534554; // RSET
					sharedAddr[4] = 0x54455352; // RSET
					break;
				case 2:
					screenshot();
					break;
				case 3:
					sharedAddr[4] = 0x444D4152; // RAMD
					while (sharedAddr[4] == 0x444D4152);
					break;
				case 4:
					optionsMenu(mainScreen);
					break;
				// To be added: Cheats...
				case 5:
					ramViewer();
					break;
				case 6:
					sharedAddr[4] = 0x54495551; // QUIT
					break;
				default:
					break;
			}
			#else
			switch(cursorPosition) {
				case 0:
					do {
						while (REG_VCOUNT != 191);
						while (REG_VCOUNT == 191);
					} while(KEYS & KEY_A);
					sharedAddr[4] = 0x54495845; // EXIT
					break;
				case 1:
					optionsMenu(mainScreen);
					break;
				// To be added: Cheats...
				case 2:
					ramViewer();
					break;
				case 3:
					sharedAddr[3] = 0x52534554; // RSET
					sharedAddr[4] = 0x54495551; // QUIT
					break;
				default:
					break;
			}
			#endif
		} else if (KEYS & KEY_B) {
			do {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			} while(KEYS & KEY_B);
			sharedAddr[4] = 0x54495845; // EXIT
		}
		#ifndef B4DS
		else if (KEYS & KEY_R) {
			do {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			} while(KEYS & KEY_R);
			sharedAddr[4] = 0x50455453; // STEP
		}
		#endif
	}

	tonccpy(BG_MAP_RAM_SUB(15), bgMapBak, sizeof(bgMapBak));	// Restore BG_MAP_RAM
	tonccpy(BG_PALETTE_SUB, palBak, sizeof(palBak));	// Restore the palette
	tonccpy(BG_GFX_SUB, bgBak, FONT_SIZE);	// Restore the original graphics

	*(vu16*)0x0400106C = masterBright;

	REG_DISPCNT_SUB = dispcnt;
	REG_BG0CNT_SUB = bg0cnt;
	REG_BG1CNT_SUB = bg1cnt;
	REG_BG2CNT_SUB = bg2cnt;
	REG_BG3CNT_SUB = bg3cnt;

	VRAM_C_CR = vramCCr;
	VRAM_H_CR = vramHCr;

	REG_POWERCNT = powercnt;

	if(*mainScreen == 1)
		REG_POWERCNT &= ~POWER_SWAP_LCDS;
	else if(*mainScreen == 2)
		REG_POWERCNT |= POWER_SWAP_LCDS;

	#ifndef B4DS
	REG_EXMEMCNT = exmemcnt;

	leaveCriticalSection(oldIME);
	#endif
}