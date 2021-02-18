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

	// The commented code doesn't seem to work
	/*u32 dispCntBak = REG_DISPCNT;
	REG_DISPCNT = 0x20000;
	*(vu8*)0x4000240 = 0x80;
	*(vu16*)0x6800000 = 0x8000;*/

	u32 dspcnt = REG_DISPCNT;
	u16 bg0cnt = REG_BG0CNT;

	REG_DISPCNT = 0x10100;
	REG_BG0CNT = 1 << 8;

	BG_PALETTE[0] = 0x0000;
	BG_PALETTE[1] = 0xFFFF;
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

	REG_DISPCNT = dspcnt;
	REG_BG0CNT = bg0cnt;

	//REG_DISPCNT = dispCntBak;

	leaveCriticalSection(oldIME);
}