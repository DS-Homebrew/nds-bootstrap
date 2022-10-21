#ifndef IN_GAME_MENU_H
#define IN_GAME_MENU_H

#include <nds/ndstypes.h>

typedef enum {
	MENU_EXIT = 0,
	MENU_RESET = 1,
	MENU_SCREENSHOT = 2,
	MENU_MANUAL = 3,
	MENU_RAM_DUMP = 4,
	MENU_OPTIONS = 5,
	MENU_RAM_VIEWER = 6,
	MENU_QUIT = 7

} MenuItem;

typedef enum {
	OPTIONS_MAIN_SCREEN,
	OPTIONS_BRIGHTNESS,
	OPTIONS_VOLUME,
	OPTIONS_CLOCK_SPEED,
	OPTIONS_VRAM_MODE
} OptionsItem;

typedef enum {
	FONT_WHITE = 0,
	FONT_LIGHT_GRAY = 1,
	FONT_DARKER_GRAY = 2,
	FONT_LIGHT_BLUE = 3,
	FONT_RED = 4,
	FONT_LIME = 5
} FontPalette;

extern struct IgmText igmText;

extern u32* waitSysCyclesLoc;
extern u32 scfgExtBak;
extern u16 scfgClkBak;
extern vu32* volatile sharedAddr;

extern u16 igmPal[6];

void SetBrightness(u8 screen, s8 bright);

void print(int x, int y, const unsigned char *str, FontPalette palette, bool main);
void printCenter(int x, int y, const unsigned char *str, FontPalette palette, bool main);
void printRight(int x, int y, const unsigned char *str, FontPalette palette, bool main);
void printChar(int x, int y, unsigned char c, FontPalette palette, bool main);
void printDec(int x, int y, u32 val, int digits, FontPalette palette, bool main);
void printHex(int x, int y, u32 val, u8 bytes, FontPalette palette, bool main);

void clearScreen(bool main);

void showException(s32 *expReg);

#endif // IN_GAME_MENU_H
