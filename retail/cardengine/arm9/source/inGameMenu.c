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

	REG_DISPCNT = 0x10100;
	REG_BG0CNT = 1 << 8;

	tonccpy((u16*)0x0277FF00, BG_PALETTE, 256*sizeof(u16));	// Backup the palette

	*(u32*)BG_PALETTE = 0xFFFF0000; // First palette black, second white
	BG_MAP_RAM(1)[0] = 1;
	u8 smile[] = {
		0x00,0x11,0x11,0x00,
		0x10,0x11,0x11,0x01,
		0x11,0x12,0x21,0x11,
		0x11,0x11,0x11,0x11,
		0x11,0x11,0x11,0x11,
		0x21,0x11,0x11,0x12,
		0x10,0x22,0x22,0x01,
		0x00,0x11,0x11,0x00,
	};
	tonccpy((u16*)0x02770000, BG_GFX + (sizeof(smile) / 2), sizeof(smile));	// Backup the original tile
	tonccpy(BG_GFX + (sizeof(smile) / 2), smile, sizeof(smile));

	u8 prevPosition = 0;
	while (IPC_GetSync() != 0xA) {
		int cursorPosition = 0x20 * IPC_GetSync();
		if(prevPosition != cursorPosition) {
			BG_MAP_RAM(1)[prevPosition] = 0;
			BG_MAP_RAM(1)[cursorPosition] = 1;
			prevPosition = cursorPosition;
		}
	}

	tonccpy(BG_PALETTE, (u16*)0x0277FF00, 256*sizeof(u16));	// Restore the palette
	tonccpy(BG_GFX + (sizeof(smile) / 2), (u16*)0x02770000, sizeof(smile));	// Restore the original tile

	REG_DISPCNT = dspcnt;
	REG_BG0CNT = bg0cnt;

	leaveCriticalSection(oldIME);
}