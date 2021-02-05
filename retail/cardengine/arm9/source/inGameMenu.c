#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/arm9/video.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"

extern vu32* volatile sharedAddr;

void inGameMenu(void) {
	int oldIME = enterCriticalSection();

	// The commented code doesn't seem to work
	/*u32 dispCntBak = REG_DISPCNT;
	REG_DISPCNT = 0x20000;
	*(vu8*)0x4000240 = 0x80;
	*(vu16*)0x6800000 = 0x8000;*/

	while (1) {
		if (sharedAddr[4] == 0x54495845
		|| sharedAddr[4] == 0x57534352) break;
	}

	//REG_DISPCNT = dispCntBak;

	leaveCriticalSection(oldIME);
}