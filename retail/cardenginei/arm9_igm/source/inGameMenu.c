#include <nds/ndstypes.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>

#include "locations.h"
#include "nds_header.h"
#include "tonccpy.h"

#include "default_font_bin.h"

static char bgBak[0x2000];
static u16 bgMapBak[0x300];
static u16 palBak[256];

static u16 igmPal[256] = {0};
static const char *menuText[] =
{
	"Return to Game",
	"Reset Game",
	"Dump RAM",
	"Options...",
	//"Cheats...(TBA)",
	"RAM Viewer...",
	"Quit Game"
};
static const char *optionsText[] =
{
	"Main Screen",
	"Clock Speed",
	"VRAM Boost",

	"   Auto",
	" Bottom",
	"    Top",
	" 67 MHz",
	"133 MHz",
	"    Off",
	"     On"
};

static const char *ndsBootstrapText = "nds-bootstrap";
static const char *ramViewerText = "RAM Viewer";
static const char *jumpAddressText = "Jump to Address";

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;
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

static void print(int x, int y, const char *str, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(4) + y * 0x20 + x;
	while(*str)
		*(dst++) = *(str++) | palette << 12;
}

static void printN(int x, int y, const char *str, int len, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(4) + y * 0x20 + x;
	for(int i = 0; i < len; i++)
		*(dst++) = *(str++) | palette << 12;
}

static void printHex(int x, int y, u32 val, u8 bytes, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(4) + y * 0x20 + x;
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		*(dst + i) = ((val & 0xF) >= 0xA ? 'A' + (val & 0xF) - 0xA : '0' + (val & 0xF)) | palette << 12;
		val >>= 4;
	}
}

static void printBattery(void) {
	u32 batteryLevel = *(u8*)(INGAME_MENU_LOCATION+0x7FFF);
	const char* bars = "    ";
	if (batteryLevel & BIT(7)) {
		bars = " ]- ";	// Charging
	} else
	switch(batteryLevel) {
		default:
			break;
		case 0x1:
		case 0x3:
			bars = "   |";
			break;
		case 0x7:
			bars = "  ||";
			break;
		case 0xB:
			bars = " |||";
			break;
		case 0xF:
			bars = "||||";
			break;
	}
	print(0x20 - 6, 0x18 - 2, bars, 2);
}

static void waitKeys(u16 keys) {
	// Prevent key repeat for 10 frames
	for(int i = 0; i < 10 && KEYS; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	do {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	} while(!(KEYS & keys));
}

static void waitKeysBattery(u16 keys) {
	// Prevent key repeat for 10 frames
	for(int i = 0; i < 10 && KEYS; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
		printBattery();
	}

	do {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
		printBattery();
	} while(!(KEYS & keys));
}

static void clearScreen(void) {
	toncset16(BG_MAP_RAM_SUB(4), 0, 0x300);
}

static void drawCursor(u8 line) {
	// Clear other cursors
	for(int i = 0; i < 0x18; i++)
		BG_MAP_RAM_SUB(4)[i * 0x20] = 0;

	// Set cursor on the selected line
	BG_MAP_RAM_SUB(4)[line * 0x20] = '>';
}

static void drawMainMenu(void) {
	clearScreen();

	// Print labels
	for(int i = 0; i < 6; i++) {
		print(2, i, (char*)menuText[i], 0);
	}

	// Print info
	print(1, 0x18 - 3, (char*)ndsBootstrapText, 1);
	print(1, 0x18 - 2, (char*)INGAME_MENU_LOCATION, 1);
	print(0x20 - 7, 0x18 - 2, "(", 2);
	print(0x20 - 2, 0x18 - 2, ")", 2);
}

static void optionsMenu(s8* mainScreen) {
	clearScreen();

	// Print labels
	for(int i = 0; i < 3; i++) {
		print(2, i, (char*)optionsText[i], 0);
	}

	u8 cursorPosition = 0;
	while(1) {
		drawCursor(cursorPosition);

		// Main screen
		print(0x20 - 8, 0, (char*)optionsText[3 + (*mainScreen)] , 0);
		// Clock speed
		print(0x20 - 8, 1, (char*)optionsText[6 + (REG_SCFG_CLK & 1)], 0);
		// VRAM boost
		print(0x20 - 8, 2, (char*)optionsText[8 + ((REG_SCFG_EXT & BIT(13)) >> 13)], 0);

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_B);

		if (KEYS & KEY_UP) {
			if(cursorPosition > 0)
				cursorPosition--;
		} else if (KEYS & KEY_DOWN) {
			if(cursorPosition < 2)
				cursorPosition++;
		} else if (KEYS & (KEY_LEFT | KEY_RIGHT)) {
			switch(cursorPosition) {
				case 0:
					(KEYS & KEY_LEFT) ? *mainScreen-- : *mainScreen++;
					if(*mainScreen > 2)
						*mainScreen = 0;
					else if(mainScreen < 0)
						*mainScreen = 2;
					break;
				case 1:
					REG_SCFG_CLK ^= 1;
					break;
				case 2:
					REG_SCFG_EXT ^= BIT(13);
					break;
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
		toncset16(BG_MAP_RAM_SUB(4) + 0x20 * 9 + 6, '-', 19);
		print(8, 10, (char*)jumpAddressText, 0);
		printHex(11, 12, (u32)address, 4, 2);
		BG_MAP_RAM_SUB(4)[0x20 * 12 + 11 + 6 - cursorPosition] = (BG_MAP_RAM_SUB(4)[0x20 * 12 + 11 + 6 - cursorPosition] & ~(0xF << 12)) | 3 << 12;
		toncset16(BG_MAP_RAM_SUB(4) + 0x20 * 13 + 6, '-', 19);

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B);

		if(KEYS & KEY_UP) {
			address += 4 << (cursorPosition * 4);
		} else if(KEYS & KEY_DOWN) {
			address -= 4 << (cursorPosition * 4);
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

	u8 cursorPosition = 0, mode = 0;
	while(1) {
		print(11, 0, (char*)ramViewerText, 0);
		printHex(0, 0, (u32)(address) >> 0x10, 2, 2);

		for(int i = 0; i < 23; i++) {
			printHex(0, i + 1, (u32)(address + (i * 2)) & 0xFFFF, 2, 2);
			for(int j = 0; j < 4; j++)
				printHex(5 + (j * 2), i + 1, *(((u8*)address) + (i * 8) + j), 1, 1);
			for(int j = 0; j < 4; j++)
				printHex(14 + (j * 2), i + 1, *(((u8*)address) + 4 + (i * 8) + j), 1, 1);
			printN(23, i + 1, (char*)(address + (i * 2)), 8, 0);
		}

		// Change color of selected byte
		if(mode > 0) {
			// Hex
			u16 loc = 0x20 * (1 + (cursorPosition / 8)) + 5 + ((cursorPosition % 8) * 2) + (cursorPosition % 8 >= 4);
			BG_MAP_RAM_SUB(4)[loc] = (BG_MAP_RAM_SUB(4)[loc] & ~(0xF << 12)) | (2 + mode) << 12;
			BG_MAP_RAM_SUB(4)[loc + 1] = (BG_MAP_RAM_SUB(4)[loc + 1] & ~(0xF << 12)) | (2 + mode) << 12;

			// Text
			loc = 0x20 * (1 + (cursorPosition / 8)) + 23 + (cursorPosition % 8);
			BG_MAP_RAM_SUB(4)[loc] = (BG_MAP_RAM_SUB(4)[loc] & ~(0xF << 12)) | (2 + mode) << 12;
		}

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B | KEY_Y);

		if(mode == 0) {
			if (KEYS & KEY_UP) {
				address -= 2;
			} else if (KEYS & KEY_DOWN) {
				address += 2;
			} else if (KEYS & KEY_LEFT) {
				address -= 2 * 23;
			} else if (KEYS & KEY_RIGHT) {
				address += 2 * 23;
			} else if (KEYS & KEY_A) {
				mode = 1;
			} else if (KEYS & KEY_B) {
				return;
			} else if(KEYS & KEY_Y) {
				jumpToAddress();
				clearScreen();
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
				((u8 *)address)[cursorPosition]++;
			} else if (KEYS & KEY_DOWN) {
				((u8 *)address)[cursorPosition]--;
			} else if (KEYS & KEY_LEFT) {
				((u8 *)address)[cursorPosition] -= 0x10;
			} else if (KEYS & KEY_RIGHT) {
				((u8 *)address)[cursorPosition] += 0x10;
			} else if (KEYS & (KEY_A | KEY_B)) {
				mode = 1;
			}
		}
	}
}

void inGameMenu(s8* mainScreen) {
	int oldIME = enterCriticalSection();

	u16 exmemcnt = REG_EXMEMCNT;
	sysSetCardOwner(false);	// Give Slot-1 access to arm7

	u32 dispcnt = REG_DISPCNT_SUB;
	u16 bg0cnt = REG_BG0CNT_SUB;
	//u16 bg1cnt = REG_BG1CNT_SUB;
	//u16 bg2cnt = REG_BG2CNT_SUB;
	//u16 bg3cnt = REG_BG3CNT_SUB;

	u16 powercnt = REG_POWERCNT;

	u16 masterBright = *(vu16*)0x0400106C;

	REG_DISPCNT_SUB = 0x10100;
	REG_BG0CNT_SUB = 4 << 8;
	//REG_BG1CNT_SUB = 0;
	//REG_BG2CNT_SUB = 0;
	//REG_BG3CNT_SUB = 0;

	REG_BG0VOFS_SUB = 0;
	REG_BG0HOFS_SUB = 0;

	// If main screen is on auto, then force the bottom
	if(*mainScreen == 0)
		REG_POWERCNT |= POWER_SWAP_LCDS;

	SetBrightness(1, 0);

	tonccpy(&bgMapBak, BG_MAP_RAM_SUB(4), 0x300 * sizeof(u16));	// Backup BG_MAP_RAM
	clearScreen();

	igmPal[0x01] = 0xFFFF; // White
	igmPal[0x11] = 0xDEF7; // Light gray
	igmPal[0x21] = 0xF355; // Light blue
	igmPal[0x31] = 0x801B; // Red
	igmPal[0x41] = 0x8360; // Lime

	tonccpy(&palBak, BG_PALETTE_SUB, 256 * sizeof(u16));	// Backup the palette
	tonccpy(BG_PALETTE_SUB, &igmPal, 0x200);

	tonccpy(&bgBak, BG_GFX_SUB, 0x2000);	// Backup the original graphics
	tonccpy(BG_GFX_SUB, default_font_bin, 0x2000); // Load font

	// Wait a frame so the key check is ready
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	u8 cursorPosition = 0;
	while (sharedAddr[4] == 0x554E454D) {
		drawMainMenu();
		drawCursor(cursorPosition);

		waitKeysBattery(KEY_UP | KEY_DOWN | KEY_A | KEY_B);

		if (KEYS & KEY_UP) {
			if (cursorPosition > 0)
				cursorPosition--;
		} else if (KEYS & KEY_DOWN) {
			if (cursorPosition < 5)
				cursorPosition++;
		} else if (KEYS & KEY_A) {
			switch(cursorPosition) {
				case 0:
					do {
						while (REG_VCOUNT != 191);
						while (REG_VCOUNT == 191);
					} while(KEYS & KEY_A);
					sharedAddr[4] = 0x54495845; // EXIT
					break;
				case 1:
					sharedAddr[4] = 0x54455352; // RSET
					break;
				case 2:
					sharedAddr[4] = 0x444D4152; // RAMD
					while (sharedAddr[4] == 0x444D4152);
					break;
				case 3:
					optionsMenu(mainScreen);
					break;
				// To be added: Cheats...
				case 4:
					ramViewer();
					break;
				case 5:
					sharedAddr[4] = 0x54495551; // QUIT
					break;
				default:
					break;
			}
		} else if (KEYS & KEY_B) {
			do {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			} while(KEYS & KEY_B);
			sharedAddr[4] = 0x54495845; // EXIT
		}
	}

	tonccpy(BG_MAP_RAM_SUB(4), &bgMapBak, 0x300 * sizeof(u16));	// Restore BG_MAP_RAM
	tonccpy(BG_PALETTE_SUB, &palBak, 256 * sizeof(u16));	// Restore the palette
	tonccpy(BG_GFX_SUB, &bgBak, 0x2000);	// Restore the original graphics

	*(vu16*)0x0400106C = masterBright;

	REG_DISPCNT_SUB = dispcnt;
	REG_BG0CNT_SUB = bg0cnt;
	//REG_BG1CNT_SUB = bg1cnt;
	//REG_BG2CNT_SUB = bg2cnt;
	//REG_BG3CNT_SUB = bg3cnt;

	REG_POWERCNT = powercnt;

	REG_EXMEMCNT = exmemcnt;

	leaveCriticalSection(oldIME);
}