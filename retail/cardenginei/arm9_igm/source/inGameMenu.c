#include "inGameMenu.h"

#include <nds/bios.h>
#include <nds/ndstypes.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>

#include "igm_text.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#include "nds_header.h"
#include "tonccpy.h"

void DC_InvalidateRange(const void *base, u32 size);
void DC_FlushRange(const void *base, u32 size);

#ifndef B4DS
	#define MAX_BRIGHTNESS 5
#else
	#define MAX_BRIGHTNESS 4
#endif

static u8 bgBak[sizeof(igmText.font) * 4];
static u16 bgMapBak[0x300];
static u16 palBak[256];

// For RAM viewer, global so it's persistant
vu32 *address = (vu32*)0x02000000;
static bool arm7Ram = false;
static u8 arm7RamBak[0xC0];

u16 igmPal[6] = {
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
#else
#ifdef EXTMEM
cardengineArm9* volatile ce9 = (cardengineArm9*)CARDENGINE_ARM9_LOCATION_DLDI_EXTMEM;
#else
cardengineArm9* volatile ce9 = (cardengineArm9*)CARDENGINE_ARM9_LOCATION_DLDI_START;
#endif

static u16* vramBak = (u16*)INGAME_MENU_EXT_LOCATION_B4DS+(0x18200/sizeof(u16));
static u16* bmpBuffer = (u16*)INGAME_MENU_EXT_LOCATION_B4DS;
#endif

// Header for a 256x192 16 bit (RGBA 565) BMP
const static u8 bmpHeader[] = {
	0x42, 0x4D, 0x46, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00,
	0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xC0, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x01, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07,
	0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define KEYS sharedAddr[5]

bool swiDelayEnabled = true;
void mySwiDelay(int delay) {
	if (!swiDelayEnabled) {
		return;
	}

	swiDelay(delay);
}

void SetBrightness(u8 screen, s8 bright) {
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

void print(int x, int y, const unsigned char *str, FontPalette palette, bool main) {
	u16 *dst = (main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15)) + y * 0x20 + x;
	while(*str)
		*(dst++) = *(str++) | palette << 12;
}

void printCenter(int x, int y, const unsigned char *str, FontPalette palette, bool main) {
	u16 *dst = (main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15)) + y * 0x20 + x;
	const unsigned char *start = str;
	while(*str)
		str++;
	dst += (str - start) / 2;
	while(str != start)
		*(--dst) = *(--str) | palette << 12;
}

void printRight(int x, int y, const unsigned char *str, FontPalette palette, bool main) {
	u16 *dst = (main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15)) + y * 0x20 + x;
	const unsigned char *start = str;
	while(*str)
		str++;
	while(str != start)
		*(dst--) = *(--str) | palette << 12;
}

void printChar(int x, int y, unsigned char c, FontPalette palette, bool main) {
	(main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15))[y * 0x20 + x] = c | palette << 12;
}

void printDec(int x, int y, u32 val, int digits, FontPalette palette, bool main) {
	u16 *dst = (main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15)) + y * 0x20 + x;
	for(int i = digits - 1; i >= 0; i--) {
		*(dst + i) = ('0' + (val % 10)) | palette << 12;
		val /= 10;
	}
}

void printHex(int x, int y, u32 val, u8 bytes, FontPalette palette, bool main) {
	u16 *dst = (main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15)) + y * 0x20 + x;
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		*(dst + i) = ((val & 0xF) >= 0xA ? 'A' + (val & 0xF) - 0xA : '0' + (val & 0xF)) | palette << 12;
		val >>= 4;
	}
}

static void printTime(void) {
	while (!(sharedAddr[7] & 0x10000000)) { // Wait for time to be received
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	}
	#ifndef B4DS
	#define timeYpos 3
	#else
	#define timeYpos 2
	#endif

	u8 hours = (u8)sharedAddr[7];
	u8 minutes = (u8)sharedAddr[8];
	printDec(0x20 - 6, 0x18 - timeYpos, hours, 2, FONT_LIGHT_BLUE, false);
	printChar(0x20 - 4, 0x18 - timeYpos, ':', FONT_LIGHT_BLUE, false);
	printDec(0x20 - 3, 0x18 - timeYpos, minutes, 2, FONT_LIGHT_BLUE, false);
}

#ifndef B4DS
static void printBattery(void) {
	while ((u8)sharedAddr[6] == 0) { // Wait for battery level to be received
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	}
	u8 batteryLevel = (u8)sharedAddr[6];
	const char *bars = "\3\3";
	if (batteryLevel & BIT(7)) {
		bars = "\6\6";	// Charging
	} else {
		switch (batteryLevel) {
			default:
				break;
			case 0x1:
			case 0x3:
				bars = "\3\4";
				break;
			case 0x7:
				bars = "\3\5";
				break;
			case 0xB:
				bars = "\4\5";
				break;
			case 0xF:
				bars = "\5\5";
				break;
		}
	}
	print(0x20 - 4, 0x18 - 2, (const unsigned char *)bars, FONT_LIGHT_BLUE, false);
}
#endif

static void waitKeys(u16 keys) {
	// Prevent key repeat for 10 frames
	for(int i = 0; i < 10 && (KEYS & keys); i++) {
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	}

	u32 status = sharedAddr[6]; // Battery, brightness, volume
	u8 hours = (u8)sharedAddr[7];
	u8 minutes = (u8)sharedAddr[8];

	do {
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	} while(!(KEYS & keys) && sharedAddr[6] == status && (u8)sharedAddr[7] == hours && (u8)sharedAddr[8] == minutes);
}

void clearScreen(bool main) {
	toncset16(main ? BG_MAP_RAM(15) : BG_MAP_RAM_SUB(15), 0, 0x300);
}

#define VRAM_x(bank) ((u16*)(0x6800000 + (0x0020000 * (bank))))
#define VRAM_x_CR(bank) (((vu8*)0x04000240)[bank])

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
	for(int i = 0; i <= 6; i++) {
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

		clearScreen(false);

		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 9 + 5, '-', 20);
		printCenter(15, 10, igmText.selectBank, FONT_WHITE, false);
		printChar(15, 12, 'A' + vramBank, FONT_LIGHT_BLUE, false);
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 13 + 5, '-', 20);

		FontPalette color = igmText.currentScreenshot == 50 ? FONT_RED : FONT_LIME;
		if(igmText.rtl) {
			printDec(6, 14, igmText.currentScreenshot, 2, color, false);
			printChar(8, 14, '/', color, false);
			printDec(9, 14, 50, 2, color, false);
			printRight(23, 14, igmText.count, 0, false);
		} else {
			print(6, 14, igmText.count, 0, false);
			printDec(19, 14, igmText.currentScreenshot, 2, color, false);
			printChar(21, 14, '/', color, false);
			printDec(22, 14, 50, 2, color, false);

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

	#ifdef B4DS
	codeJumpWord = ce9->prepareScreenshot;
	(*codeJump)();
	#else
	sharedAddr[4] = 0x50505353;
	while (sharedAddr[4] == 0x50505353) {
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	}
	#endif

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

	#ifdef B4DS
	codeJumpWord = ce9->saveScreenshot;
	(*codeJump)();
	#else
	sharedAddr[4] = 0x544F4853;
	while (sharedAddr[4] == 0x544F4853) {
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	}
	DC_InvalidateRange(&igmText.currentScreenshot, 1);
	#endif
}

static void manual(void) {
	#ifdef B4DS
	codeJumpWord = ce9->prepareManual;
	(*codeJump)();
	#else
	sharedAddr[4] = 0x4E414D50; // PMAN
	do {
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	} while (sharedAddr[4] == 0x4E414D50);
	#endif

	while (1) {
		#ifdef B4DS
		codeJumpWord = ce9->readManual;
		(*codeJump1)(igmText.manualLine);

		print(0, 0, (unsigned char *)0x027FF200, FONT_WHITE, false);
		#else
		DC_InvalidateRange((unsigned char *)INGAME_MENU_EXT_LOCATION, 32 * 24);
		sharedAddr[0] = igmText.manualLine;
		sharedAddr[4] = 0x554E414D; // MANU
		do {
			while (REG_VCOUNT != 191) mySwiDelay(100);
			while (REG_VCOUNT == 191) mySwiDelay(100);
		} while (sharedAddr[4] == 0x554E414D);

		print(0, 0, (unsigned char *)INGAME_MENU_EXT_LOCATION, FONT_WHITE, false);
		#endif

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_B);
		if(KEYS & KEY_UP) {
			igmText.manualLine -= (KEYS & KEY_R) ? 100 : 1;
		} else if(KEYS & KEY_DOWN) {
			igmText.manualLine += (KEYS & KEY_R) ? 100 : 1;
		} else if(KEYS & KEY_LEFT) {
			igmText.manualLine -= (KEYS & KEY_R) ? 1000 : 23;
			
		} else if(KEYS & KEY_RIGHT) {
			igmText.manualLine += (KEYS & KEY_R) ? 1000 : 23;
		} else if(KEYS & KEY_B) {
			break;
		}

		if(igmText.manualLine < 0)
			igmText.manualLine = 0;
		else if(igmText.manualLine > igmText.manualMaxLine - 23)
			igmText.manualLine = igmText.manualMaxLine - 23 > 0 ? igmText.manualMaxLine - 23 : 0;
	}

	#ifdef B4DS
	codeJumpWord = ce9->restorePreManual;
	(*codeJump)();
	#else
	sharedAddr[4] = 0x4E414D52; // RMAN
	do {
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	} while (sharedAddr[4] == 0x4E414D52);
	#endif
}

static void drawCursor(u8 line) {
	u8 pos = igmText.rtl ? 0x1F : 0;
	// Clear other cursors
	for(int i = 0; i < 0x18; i++)
		BG_MAP_RAM_SUB(15)[i * 0x20 + pos] = 0;

	// Set cursor on the selected line
	BG_MAP_RAM_SUB(15)[line * 0x20 + pos] = igmText.rtl ? '<' : '>';
}

static void drawMainMenu(MenuItem *menuItems, int menuItemCount) {
	clearScreen(false);

	// Print labels
	for(int i = 0; i < menuItemCount; i++) {
		if(igmText.rtl)
			printRight(0x1D, i, igmText.menu[menuItems[i]], FONT_WHITE, false);
		else
			print(2, i, igmText.menu[menuItems[i]], FONT_WHITE, false);
	}

	// Print info
	print(1, 0x18 - 3, igmText.ndsBootstrap, FONT_LIGHT_GRAY, false);
	print(1, 0x18 - 2, igmText.version, FONT_LIGHT_GRAY, false);

	#ifndef B4DS
	// Print battery
	printChar(0x20 - 5, 0x18 - 2, '\2', FONT_LIGHT_BLUE, false);
	printChar(0x20 - 2, 0x18 - 2, '\7', FONT_LIGHT_BLUE, false);
	#endif
}

static void optionsMenu(s32 *mainScreen, u32 consoleModel) {
	OptionsItem optionsItems[8];
	int optionsItemCount = 0;
	optionsItems[optionsItemCount++] = OPTIONS_MAIN_SCREEN;
	if(consoleModel < 2) // Not on 3DS
		optionsItems[optionsItemCount++] = OPTIONS_BRIGHTNESS;
#ifndef B4DS
	optionsItems[optionsItemCount++] = OPTIONS_VOLUME;
	optionsItems[optionsItemCount++] = OPTIONS_CLOCK_SPEED;
	optionsItems[optionsItemCount++] = OPTIONS_VRAM_MODE;
#endif

	bool mainScreenChanged = false;

	u8 cursorPosition = 0;
	while(1) {
		clearScreen(false);

		// Print options
		for(int i = 0; i < optionsItemCount; i++) {
			const unsigned char *optionValue = NULL;
			int optionPercent = 0;
			bool isString = true;
			switch(optionsItems[i]) {
				case OPTIONS_MAIN_SCREEN:
					optionValue = igmText.optionsValues[*mainScreen];
					break;
				case OPTIONS_BRIGHTNESS:
					optionPercent = (u8)(sharedAddr[6] >> 8) * 100 / MAX_BRIGHTNESS;
					isString = false;
					break;
				case OPTIONS_VOLUME:
					optionPercent = (u8)(sharedAddr[6] >> 16) * 100 / 31;
					isString = false;
					break;
				case OPTIONS_CLOCK_SPEED:
					optionValue = igmText.optionsValues[3 + ((REG_SCFG_CLK == 0 ? *scfgClkBak : REG_SCFG_CLK) & 1)];
					break;
				case OPTIONS_VRAM_MODE:
					optionValue = igmText.optionsValues[5 + (((REG_SCFG_EXT == 0 ? *scfgExtBak : REG_SCFG_EXT) & BIT(13)) >> 13)];
					break;
			}

			int digits = optionPercent == 100 ? 3 : (optionPercent >= 10 ? 2 : 1);
			if(igmText.rtl) {
				printRight(0x1D, i, igmText.optionsLabels[optionsItems[i]], FONT_WHITE, false);
				if(isString) {
					print(0, i, optionValue, FONT_WHITE, false);
				} else {
					printDec(0, i, optionPercent, digits, FONT_WHITE, false);
					printChar(0 + digits, i, '%', FONT_WHITE, false);
				}
			} else {
				print(2, i, igmText.optionsLabels[optionsItems[i]], FONT_WHITE, false);
				if(isString) {
					printRight(0x1E, i, optionValue, FONT_WHITE, false);
				} else {
					printDec(0x1E - digits, i, optionPercent, digits, FONT_WHITE, false);
					printChar(0x1E, i, '%', FONT_WHITE, false);
				}
			}
		}
		drawCursor(cursorPosition);

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_B | KEY_SELECT);

		if (KEYS & KEY_UP) {
			if(cursorPosition > 0)
				cursorPosition--;
			else
				cursorPosition = optionsItemCount - 1;
		} else if (KEYS & KEY_DOWN) {
			if(cursorPosition < (optionsItemCount - 1))
				cursorPosition++;
			else
				cursorPosition = 0;
		} else if (KEYS & (KEY_LEFT | KEY_RIGHT)) {
			switch(optionsItems[cursorPosition]) {
				case OPTIONS_MAIN_SCREEN:
					(KEYS & KEY_LEFT) ? (*mainScreen)-- : (*mainScreen)++;
					if(*mainScreen > 2)
						*mainScreen = 0;
					else if(*mainScreen < 0)
						*mainScreen = 2;
					sharedAddr[4] = (*mainScreen == 0) ? 0x4E435049 : 0x59435049;
					mainScreenChanged = true;
					break;
				case OPTIONS_BRIGHTNESS:
				{
					u8 brightness = (u8)(sharedAddr[6] >> 8);
					if(KEYS & KEY_LEFT && brightness > 0)
						brightness--;
					else if (KEYS & KEY_RIGHT && brightness < MAX_BRIGHTNESS)
						brightness++;

					sharedAddr[0] = brightness;
					sharedAddr[4] = 0x4554494C; // LITE
					while(sharedAddr[4] == 0x4554494C) {
						while (REG_VCOUNT != 191) mySwiDelay(100);
						while (REG_VCOUNT == 191) mySwiDelay(100);
					}
					break;
				}
				case OPTIONS_VOLUME:
				{
					u8 volume = (u8)(sharedAddr[6] >> 16);
					if(KEYS & KEY_LEFT && volume > 0)
						volume--;
					else if (KEYS & KEY_RIGHT && volume < 31)
						volume++;

					sharedAddr[0] = volume;
					sharedAddr[4] = 0x554C4F56; // VOLU
					while(sharedAddr[4] == 0x554C4F56) {
						while (REG_VCOUNT != 191) mySwiDelay(100);
						while (REG_VCOUNT == 191) mySwiDelay(100);
					}
					break;
				}
				case OPTIONS_CLOCK_SPEED:
					REG_SCFG_CLK ^= 1;
					u32* waitSysCyclesLoc = (u32*)(waitSysCyclesLocPtr ? *waitSysCyclesLocPtr : 0);
					if (waitSysCyclesLoc) {
						if (waitSysCyclesLoc[0] == 0xE92D4008) {
							waitSysCyclesLoc[1] = (REG_SCFG_CLK & BIT(1)) ? 0xE1A00100 : 0xE1A00080;
						} else {
							u16* offsetThumb = (u16*)waitSysCyclesLoc;
							offsetThumb[1] = (REG_SCFG_CLK & BIT(1)) ? 0x0080 : 0x0040;
						}
					}
					break;
				case OPTIONS_VRAM_MODE:
					REG_SCFG_EXT ^= BIT(13);
					break;
				default:
					break;
			}
		} else if (KEYS & KEY_B) {
			if (mainScreenChanged) {
				#ifndef B4DS
				sharedAddr[0] = *mainScreen;
				sharedAddr[4] = 0x53435049; // IPCS
				while(sharedAddr[4] == 0x53435049) {
					while (REG_VCOUNT != 191) mySwiDelay(100);
					while (REG_VCOUNT == 191) mySwiDelay(100);
				}
				#else
				codeJumpWord = ce9->saveMainScreenSetting;
				(*codeJump)();
				#endif
			}
			return;
		}
	}
}

static void jumpToAddress(void) {
	clearScreen(false);

	u8 cursorPosition = 0;
	while(1) {
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 9 + 5, '-', 20);
		printCenter(15, 10, igmText.jumpAddress, FONT_WHITE, false);
		printHex(11, 12, (u32)address, 4, FONT_LIGHT_BLUE, false);
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
	clearScreen(false);
	(*changeMpu)();

	u8 *arm7RamBuffer = ((u8*)sharedAddr) - 0x74C;
	tonccpy(arm7RamBak, arm7RamBuffer, 0xC0);
	bool ramLoaded = false;
	u8 cursorPosition = 0, mode = 0;
	while(1) {
		u8 *ramPtr = arm7Ram ? arm7RamBuffer : (u8*)address;

		unsigned char armText[5] = {'A', 'R', 'M', arm7Ram ? '7' : '9', 0};
		printCenter(14, 0, igmText.ramViewer, FONT_WHITE, false);
		print(27, 0, armText, FONT_LIGHT_BLUE, false);
		printHex(0, 0, (u32)address >> 0x10, 2, FONT_LIGHT_BLUE, false);

		if (arm7Ram && !ramLoaded) {
			sharedAddr[0] = (vu32)arm7RamBuffer;
			sharedAddr[1] = (vu32)address;
			sharedAddr[4] = 0x524D4152; // RAMR
			while (sharedAddr[4] == 0x524D4152) {
				while (REG_VCOUNT != 191) mySwiDelay(100);
				while (REG_VCOUNT == 191) mySwiDelay(100);
			}
		}
		ramLoaded = true;

		for(int i = 0; i < 23; i++) {
			printHex(0, i + 1, (u32)(address + (i * 2)) & 0xFFFF, 2, FONT_LIGHT_BLUE, false);
			for(int j = 0; j < 4; j++)
				printHex(5 + (j * 2), i + 1, ramPtr[(i * 8) + j], 1, 1 + j % 2, false);
			for(int j = 0; j < 4; j++)
				printHex(14 + (j * 2), i + 1, ramPtr[4 + (i * 8) + j], 1, 1 + j % 2, false);
			for(int j = 0; j < 8; j++)
				printChar(23 + j, i + 1, ramPtr[i * 8 + j], FONT_WHITE, false);
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
					clearScreen(false);
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
				clearScreen(false);
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
						while (REG_VCOUNT != 191) mySwiDelay(100);
						while (REG_VCOUNT == 191) mySwiDelay(100);
					}
					ramLoaded = false;
				}
				mode = 1;
			}
		}
	}
	tonccpy(arm7RamBuffer, arm7RamBak, 0xC0);
	(*revertMpu)();
}

void inGameMenu(s32 *mainScreen, u32 consoleModel, s32 *exceptionRegisters) {
	// If we were given exception registers, then we're handling an exception
	bool exception = (exceptionRegisters != 0);

	#ifdef B4DS
	#ifndef EXTMEM
	static bool ce9Set = false;
	if (!ce9Set) {
		if (*(u32*)CARDENGINE_ARM9_LOCATION_DLDI_ALT == CARDENGINE_ARM9_LOCATION_DLDI_ALT) {
			ce9 = (cardengineArm9*)CARDENGINE_ARM9_LOCATION_DLDI_ALT;
		} else if (*(u32*)CARDENGINE_ARM9_LOCATION_DLDI_ALT2 == CARDENGINE_ARM9_LOCATION_DLDI_ALT2) {
			ce9 = (cardengineArm9*)CARDENGINE_ARM9_LOCATION_DLDI_ALT2;
		} else if (*(u32*)CARDENGINE_ARM9_LOCATION_DLDI == CARDENGINE_ARM9_LOCATION_DLDI) {
			ce9 = (cardengineArm9*)CARDENGINE_ARM9_LOCATION_DLDI;
		}
		ce9Set = true;
	}
	#endif
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
	REG_MOSAIC_SUB = 0; // Register is write only, can't back up
	REG_BLDCNT_SUB = 0; // Register is write only, can't back up
	REG_BLDALPHA_SUB = 0; // Register is write only, can't back up
	REG_BLDY_SUB = 0; // Register is write only, can't back up

	tonccpy(bgMapBak, BG_MAP_RAM_SUB(15), sizeof(bgMapBak));	// Backup BG_MAP_RAM
	clearScreen(false);

	tonccpy(palBak, BG_PALETTE_SUB, sizeof(palBak));	// Backup the palette
	toncset16(BG_PALETTE_SUB, 0, 256);
	for(int i = 0; i < sizeof(igmPal) / sizeof(igmPal[0]); i++) {
		BG_PALETTE_SUB[i * 0x10 + 1] = igmPal[i];
	}

	tonccpy(bgBak, BG_GFX_SUB, sizeof(igmText.font) * 4);	// Backup the original graphics
	for(int i = 0; i < sizeof(igmText.font); i++) {	// Load font from 1bpp to 4bpp
		u8 val = igmText.font[i];
		BG_GFX_SUB[i * 2]     = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
		val >>= 4;
		BG_GFX_SUB[i * 2 + 1] = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
	}

	// Let ARM7 know the menu loaded
	sharedAddr[5] = 0x59444552; // 'REDY'

	MenuItem menuItems[8];
	int menuItemCount = 0;
	if(!exception)
		menuItems[menuItemCount++] = MENU_EXIT;
	menuItems[menuItemCount++] = MENU_RESET;
	menuItems[menuItemCount++] = MENU_SCREENSHOT;
	if(igmText.manualMaxLine > 0 && !exception)
		menuItems[menuItemCount++] = MENU_MANUAL;
	menuItems[menuItemCount++] = MENU_RAM_DUMP;
	if(!exception)
		menuItems[menuItemCount++] = MENU_OPTIONS;
	menuItems[menuItemCount++] = MENU_RAM_VIEWER;
	menuItems[menuItemCount++] = MENU_QUIT;

	if(exception) {
		showException(exceptionRegisters);
	}

	// Wait for keys to be released
	drawMainMenu(menuItems, menuItemCount);
	drawCursor(0);
	do {
		printTime();
		#ifndef B4DS
		printBattery();
		#endif
		while (REG_VCOUNT != 191) mySwiDelay(100);
		while (REG_VCOUNT == 191) mySwiDelay(100);
	} while(KEYS & igmText.hotkey);

	u8 cursorPosition = 0;
	while (sharedAddr[4] == 0x554E454D) {
		drawMainMenu(menuItems, menuItemCount);
		drawCursor(cursorPosition);
		printTime();
#ifndef B4DS
		printBattery();
#endif

		if(exception)
			waitKeys(KEY_UP | KEY_DOWN | KEY_A);
		else
			waitKeys(KEY_UP | KEY_DOWN | KEY_A | KEY_B);

		if (KEYS & KEY_UP) {
			if (cursorPosition > 0)
				cursorPosition--;
			else
				cursorPosition = menuItemCount - 1;
		} else if (KEYS & KEY_DOWN) {
			if (cursorPosition < (menuItemCount - 1))
				cursorPosition++;
			else
				cursorPosition = 0;
		} else if (KEYS & KEY_A) {
			switch(menuItems[cursorPosition]) {
				case MENU_EXIT:
					do {
						while (REG_VCOUNT != 191) mySwiDelay(100);
						while (REG_VCOUNT == 191) mySwiDelay(100);
					} while(KEYS & KEY_A);
					sharedAddr[4] = 0x54495845; // EXIT
					break;
				case MENU_RESET:
					extern bool exceptionPrinted;
					exceptionPrinted = false;
					sharedAddr[3] = 0x52534554; // TESR
					sharedAddr[4] = 0x54455352; // RSET
					break;
				case MENU_SCREENSHOT:
					screenshot();
					break;
				case MENU_MANUAL:
					manual();
					break;
				case MENU_RAM_DUMP:
					#ifndef B4DS
					sharedAddr[4] = 0x444D4152; // RAMD
					while (sharedAddr[4] == 0x444D4152) {
						while (REG_VCOUNT != 191) mySwiDelay(100);
						while (REG_VCOUNT == 191) mySwiDelay(100);
					}
					#else
					sharedAddr[3] = 0x444D4152; // RAMD
					sharedAddr[4] = 0x54495845; // EXIT
					#endif
					break;
				case MENU_OPTIONS:
					optionsMenu(mainScreen, consoleModel);
					break;
				case MENU_RAM_VIEWER:
					ramViewer();
					break;
				case MENU_QUIT:
					sharedAddr[3] = 0x54495845; // EXIT
					sharedAddr[4] = 0x54495551; // QUIT
					break;
				default:
					break;
			}
		} else if (KEYS & KEY_B && !exception) {
			do {
				while (REG_VCOUNT != 191) mySwiDelay(100);
				while (REG_VCOUNT == 191) mySwiDelay(100);
			} while(KEYS & KEY_B);
			sharedAddr[4] = 0x54495845; // EXIT
		}
		#ifndef B4DS
		else if (KEYS & KEY_R && !exception) {
			do {
				while (REG_VCOUNT != 191) mySwiDelay(100);
				while (REG_VCOUNT == 191) mySwiDelay(100);
			} while(KEYS & KEY_R);
			sharedAddr[4] = 0x50455453; // STEP
		}
		#endif
	}

	tonccpy(BG_MAP_RAM_SUB(15), bgMapBak, sizeof(bgMapBak));	// Restore BG_MAP_RAM
	tonccpy(BG_PALETTE_SUB, palBak, sizeof(palBak));	// Restore the palette
	tonccpy(BG_GFX_SUB, bgBak, sizeof(igmText.font) * 4);	// Restore the original graphics

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
}