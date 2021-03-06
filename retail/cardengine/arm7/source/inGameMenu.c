#ifdef UNUSED
#include <nds/ndstypes.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern vu32* volatile sharedAddr;

extern void rebootConsole(void);

void inGameMenu(void) {
	sharedAddr[4] = 0x554E454D; // 'MENU'
	IPC_SendSync(0x9);
	int volBak = REG_MASTER_VOLUME;
	REG_MASTER_VOLUME = 0;
	//int oldIME = enterCriticalSection();

	while (sharedAddr[4] == 0x554E454D) {
		sharedAddr[5] = ~REG_KEYINPUT & 0x3FF;
		sharedAddr[5] |= ((~REG_EXTKEYINPUT & 0x3) << 10) | ((~REG_EXTKEYINPUT & 0xC0) << 6);

		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	switch (sharedAddr[4]) {
		case 0x54495845: // EXIT
		default:
			break;
		case 0x54495551: // QUIT
			rebootConsole();
			break;
	}

	sharedAddr[4] = 0x54495845; // EXIT

	//leaveCriticalSection(oldIME);
	REG_MASTER_VOLUME = volBak;
}
#endif