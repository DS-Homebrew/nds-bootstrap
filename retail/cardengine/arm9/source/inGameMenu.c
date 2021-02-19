#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/input.h>
#include <nds/system.h>
#include <nds/arm9/video.h>
#include <nds/arm9/background.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"
#include "tonccpy.h"

extern vu32* volatile sharedAddr;

#define KEYS sharedAddr[5]

void print(int x, int y, const char *str, int palette) {
	u16 *dst = BG_MAP_RAM(4) + y * 0x20 + x;
	while(*str)
		*(dst++) = *(str++) | palette << 12;
}

void printN(int x, int y, const char *str, int len, int palette) {
	u16 *dst = BG_MAP_RAM(4) + y * 0x20 + x;
	for(int i = 0; i < len; i++)
		*(dst++) = *(str++) | palette << 12;
}

void printHex(int x, int y, u32 val, u8 bytes, int palette) {
	u16 *dst = BG_MAP_RAM(4) + y * 0x20 + x;
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		*(dst + i) = ((val & 0xF) >= 0xA ? 'A' + (val & 0xF) - 0xA : '0' + (val & 0xF)) | palette << 12;
		val >>= 4;
	}
}

void drawMainMenu(void) {
	// Clear screen
	toncset16(BG_MAP_RAM(4), 0, 0x300);

	// Print labels
	for(int i = 0; i < 6; i++) {
		print(2, i, (char*)INGAME_TEXT_LOCATION + i * 0x10, 0);
	}

	// Print info
	print(0x20 - 14, 0x18 - 3, "nds-bootstrap", 1);
	print(0x20 - 8, 0x18 - 2, "nightly", 1); // TODO: Get version number / commit hash or so
}

void ramViewer(void) {
	toncset16(BG_MAP_RAM(4), 0, 0x300);

	vu32 *address = sharedAddr;

	print(11, 0, "RAM Viewer", 0);

	while(1) {
		printHex(0, 0, (u32)(address) >> 0x10, 2, 2);

		for(int i = 0; i < 23; i++) {
			printHex(0, i + 1, (u32)(address + (i * 2)) & 0xFFFF, 2, 2);
			printHex(5, i + 1, *(address + (i * 2)), 4, 1);
			printHex(14, i + 1, *(address + (i * 2) + 1), 4, 1);
			printN(23, i + 1, (char*)(address + (i * 2)), 8, 0);
		}

		// Prevent key repeat
		for(int i = 0; i < 10 && KEYS; i++) {
			while (REG_VCOUNT != 191);
			while (REG_VCOUNT == 191);
		}

		do {
			while (REG_VCOUNT != 191);
			while (REG_VCOUNT == 191);
		} while(!(KEYS & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_B)));

		if (KEYS & KEY_UP) {
			address -= 2;
		} else if (KEYS & KEY_DOWN) {
			address += 2;
		} else if (KEYS & KEY_LEFT) {
			address -= 2 * 23;
		} else if (KEYS & KEY_RIGHT) {
			address += 2 * 23;
		} else if (KEYS & KEY_B) {
			return;
		}
	}
}

void inGameMenu(void) {
	int oldIME = enterCriticalSection();

	u32 dispcnt = REG_DISPCNT;
	u16 bg0cnt = REG_BG0CNT;
	//u16 bg1cnt = REG_BG1CNT;
	//u16 bg2cnt = REG_BG2CNT;
	//u16 bg3cnt = REG_BG3CNT;

	u16 powercnt = REG_POWERCNT;

	REG_DISPCNT = 0x10100;
	REG_BG0CNT = 4 << 8;
	//REG_BG1CNT = 0;
	//REG_BG2CNT = 0;
	//REG_BG3CNT = 0;

	REG_POWERCNT &= ~POWER_SWAP_LCDS;

	tonccpy((u16*)0x026FFB00, BG_MAP_RAM(4), 0x300 * 2);	// Backup BG_MAP_RAM
	toncset16(BG_MAP_RAM(4), 0, 0x300);	// Clear BG_MAP_RAM
	tonccpy((u16*)0x026FFE00, BG_PALETTE, 256*sizeof(u16));	// Backup the palette

	*(u32*)BG_PALETTE          = 0xFFFF0000; // First palette black, second white
	*(u32*)(BG_PALETTE + 0x10) = 0xEB5A0000; // Black, light gray
	*(u32*)(BG_PALETTE + 0x20) = 0xF3550000; // Black, light blue

	BG_MAP_RAM(4)[0] = '>';
	tonccpy((u8*)INGAME_FONT_LOCATION-0x2000, BG_GFX, 0x2000);	// Backup the original graphics
	tonccpy(BG_GFX, (u8*)INGAME_FONT_LOCATION, 0x2000); // Load font

	drawMainMenu();

	// Wait a frame so the key check is ready
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	u8 cursorPosition = 0, prevPosition = 0;
	while (sharedAddr[4] == 0x554E454D) {
		// Draw cursor
		BG_MAP_RAM(4)[prevPosition * 0x20] = 0;
		BG_MAP_RAM(4)[cursorPosition * 0x20] = '>';
		prevPosition = cursorPosition;

		// Prevent key repeat
		while(KEYS & (KEY_UP | KEY_DOWN | KEY_A | KEY_B));

		do {
			while (REG_VCOUNT != 191);
			while (REG_VCOUNT == 191);
		} while(!(KEYS & (KEY_UP | KEY_DOWN | KEY_A | KEY_B)));

		if (KEYS & KEY_UP) {
			if (cursorPosition > 0)
				cursorPosition--;
		} else if (KEYS & KEY_DOWN) {
			if (cursorPosition < 5)
				cursorPosition++;
		} else if (KEYS & KEY_A) {
			switch(cursorPosition) {
				case 0:
					sharedAddr[4] = 0x54495845; // EXIT
					break;
				case 1:
					sharedAddr[4] = 0x54455352; // RSET
					break;
				case 4:
					ramViewer();
					drawMainMenu();
					break;
				case 5:
					sharedAddr[4] = 0x54495551; // QUIT
					break;
				default:
					break;
			}
		} else if (KEYS & KEY_B) {
			sharedAddr[4] = 0x54495845; // EXIT
		}
	}

	tonccpy(BG_MAP_RAM(4), (u16*)0x026FFB00, 0x300 * 2);	// Restore BG_MAP_RAM
	tonccpy(BG_PALETTE, (u16*)0x026FFE00, 256*sizeof(u16));	// Restore the palette
	tonccpy(BG_GFX, (u8*)INGAME_FONT_LOCATION-0x2000, 0x2000);	// Restore the original graphics

	REG_DISPCNT = dispcnt;
	REG_BG0CNT = bg0cnt;
	//REG_BG1CNT = bg1cnt;
	//REG_BG2CNT = bg2cnt;
	//REG_BG3CNT = bg3cnt;

	REG_POWERCNT = powercnt;

	leaveCriticalSection(oldIME);
}