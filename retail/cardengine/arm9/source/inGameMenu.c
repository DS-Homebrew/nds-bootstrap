#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/arm9/video.h>
#include <nds/arm9/background.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"
#include "tonccpy.h"

extern vu32* volatile sharedAddr;


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

const char *digits = "0123456789ABCDEF";
void printHex(int x, int y, u32 val, u8 bytes, int palette) {
	u16 *dst = BG_MAP_RAM(4) + y * 0x20 + x;
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		*(dst + i) = digits[val & 0xF] | palette << 12;
		val >>= 4;
	}
}

void ramViewer(void) {
	toncset16(BG_MAP_RAM(4), 0, 0x300);

	vu32 *address = sharedAddr;

	print(11, 0, "RAM Viewer", 0);
	printHex(0, 0, (u32)(address) >> 0x10, 2, 2);

	for(int i = 0; i < 23; i++) {
		printHex(0, i + 1, (u32)(address + (i * 2)) & 0xFFFF, 2, 2);
		printHex(5, i + 1, *(address + (i * 2)), 4, 1);
		printHex(14, i + 1, *(address + (i * 2) + 1), 4, 1);
		printN(23, i + 1, (char*)(address + (i * 2)), 8, 0);
	}
}

void inGameMenu(void) {
	int oldIME = enterCriticalSection();

	u32 dspcnt = REG_DISPCNT;
	u16 bg0cnt = REG_BG0CNT;
	//u16 bg1cnt = REG_BG1CNT;
	//u16 bg2cnt = REG_BG2CNT;
	//u16 bg3cnt = REG_BG3CNT;

	REG_DISPCNT = 0x10100;
	REG_BG0CNT = 4 << 8;
	//REG_BG1CNT = 0;
	//REG_BG2CNT = 0;
	//REG_BG3CNT = 0;

	tonccpy((u16*)0x026FFB00, BG_MAP_RAM(4), 0x300 * 2);	// Backup BG_MAP_RAM
	toncset16(BG_MAP_RAM(4), 0, 0x300);	// Clear BG_MAP_RAM
	tonccpy((u16*)0x026FFE00, BG_PALETTE, 256*sizeof(u16));	// Backup the palette

	*(u32*)BG_PALETTE          = 0xFFFF0000; // First palette black, second white
	*(u32*)(BG_PALETTE + 0x10) = 0xEB5A0000; // Black, light gray
	*(u32*)(BG_PALETTE + 0x20) = 0xF3550000; // Black, light blue

	BG_MAP_RAM(4)[0] = '>';
	tonccpy((u8*)INGAME_FONT_LOCATION-0x2000, BG_GFX, 0x2000);	// Backup the original graphics
	tonccpy(BG_GFX, (u8*)INGAME_FONT_LOCATION, 0x2000); // Load font

	tonccpy(BG_MAP_RAM(4)+2, (u16*)INGAME_TEXT_LOCATION, 16*sizeof(u16)); // Display text 1
	tonccpy(BG_MAP_RAM(4)+32+2, (u16*)INGAME_TEXT_LOCATION + 32, 16*sizeof(u16)); // Display text 2
	tonccpy(BG_MAP_RAM(4)+64+2, (u16*)INGAME_TEXT_LOCATION + 64, 16*sizeof(u16)); // Display text 3

	u8 prevPositionOnScreen = 0;
	while (IPC_GetSync() != 0xA) {
		int cursorPosition = IPC_GetSync();
		int cursorPositionOnScreen = 0x20 * IPC_GetSync();
		if(prevPositionOnScreen != cursorPositionOnScreen && cursorPosition >= 0 && cursorPosition < 4) {
			BG_MAP_RAM(4)[prevPositionOnScreen] = 0;
			BG_MAP_RAM(4)[cursorPositionOnScreen] = '>';
			prevPositionOnScreen = cursorPositionOnScreen;
			if(cursorPosition == 3) {
				ramViewer();
			}
		}
	}

	tonccpy(BG_MAP_RAM(4), (u16*)0x026FFB00, 0x300 * 2);	// Restore BG_MAP_RAM
	tonccpy(BG_PALETTE, (u16*)0x026FFE00, 256*sizeof(u16));	// Restore the palette
	tonccpy(BG_GFX, (u8*)INGAME_FONT_LOCATION-0x2000, 0x2000);	// Restore the original graphics

	REG_DISPCNT = dspcnt;
	REG_BG0CNT = bg0cnt;
	//REG_BG1CNT = bg1cnt;
	//REG_BG2CNT = bg2cnt;
	//REG_BG3CNT = bg3cnt;

	leaveCriticalSection(oldIME);
}