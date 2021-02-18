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

void inGameMenu(void) {
	int oldIME = enterCriticalSection();

	u32 dspcnt = REG_DISPCNT;
	u16 bg0cnt = REG_BG0CNT;
	//u16 bg1cnt = REG_BG1CNT;
	//u16 bg2cnt = REG_BG2CNT;
	//u16 bg3cnt = REG_BG3CNT;

	REG_DISPCNT = 0x10100;
	REG_BG0CNT = 1 << 8;
	//REG_BG1CNT = 0;
	//REG_BG2CNT = 0;
	//REG_BG3CNT = 0;

	tonccpy((u16*)0x0277FE00, BG_PALETTE, 256*sizeof(u16));	// Backup the palette

	*(u32*)BG_PALETTE = 0xFFFF0000; // First palette black, second white
	BG_MAP_RAM(1)[0] = 1;
	/*u8 smile[] = {
		0x00,0x11,0x11,0x00,
		0x10,0x11,0x11,0x01,
		0x11,0x12,0x21,0x11,
		0x11,0x11,0x11,0x11,
		0x11,0x11,0x11,0x11,
		0x21,0x11,0x11,0x12,
		0x10,0x22,0x22,0x01,
		0x00,0x11,0x11,0x00,
	};*/
	tonccpy((u8*)INGAME_FONT_LOCATION-0x2000, BG_GFX, 0x2000);	// Backup the original graphics
	tonccpy(BG_GFX, (u8*)INGAME_FONT_LOCATION, 0x2000); // Load font
	//tonccpy(BG_GFX + (sizeof(smile) / 2), smile, sizeof(smile));
	tonccpy(BG_MAP_RAM(1)+2, (u8*)INGAME_TEXT_LOCATION, 16); // Load text 1
	tonccpy(BG_MAP_RAM(1)+32+2, (u8*)INGAME_TEXT_LOCATION+32, 16); // Load text 2
	tonccpy(BG_MAP_RAM(1)+64+2, (u8*)INGAME_TEXT_LOCATION+64, 16); // Load text 3

	u8 prevPosition = 0;
	while (IPC_GetSync() != 0xA) {
		int cursorPosition = 0x20 * IPC_GetSync();
		if(prevPosition != cursorPosition) {
			BG_MAP_RAM(1)[prevPosition] = 0;
			BG_MAP_RAM(1)[cursorPosition] = 1;
			prevPosition = cursorPosition;
		}
	}

	tonccpy(BG_PALETTE, (u16*)0x0277FE00, 256*sizeof(u16));	// Restore the palette
	tonccpy(BG_GFX, (u8*)INGAME_FONT_LOCATION-0x2000, 0x2000);	// Restore the original graphics

	REG_DISPCNT = dspcnt;
	REG_BG0CNT = bg0cnt;
	//REG_BG1CNT = bg1cnt;
	//REG_BG2CNT = bg2cnt;
	//REG_BG3CNT = bg3cnt;

	leaveCriticalSection(oldIME);
}