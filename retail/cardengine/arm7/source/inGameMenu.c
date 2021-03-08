#include <nds/ndstypes.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/i2c.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern vu32* volatile sharedAddr;

extern void forceGameReboot(void);
extern void dumpRam(void);
extern void returnToLoader(void);

void inGameMenu(void) {
	sharedAddr[4] = 0x554E454D; // 'MENU'
	IPC_SendSync(0x9);
	REG_MASTER_VOLUME = 0;
	int oldIME = enterCriticalSection();

	while (sharedAddr[4] == 0x554E454D) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);

		sharedAddr[5] = ~REG_KEYINPUT & 0x3FF;
		sharedAddr[5] |= (~REG_EXTKEYINPUT & 0x3) << 10;
	}

	switch (sharedAddr[4]) {
		case 0x54495845: // EXIT
		default:
			break;
		case 0x54455352: // RSET
			forceGameReboot();
			break;
		case 0x54495551: // QUIT
			returnToLoader();
			break;
		case 0x444D4152: // RAMD
			dumpRam();
			break;
	}

	sharedAddr[4] = 0x54495845; // EXIT

	leaveCriticalSection(oldIME);
	REG_MASTER_VOLUME = 127;
}