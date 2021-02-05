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
extern void returnToLoader(void);

static int cursorPosition = 0;

void inGameMenu(void) {
	sharedAddr[4] = 0x554E454D;	// 'MENU'
	REG_MASTER_VOLUME = 0;
	int oldIME = enterCriticalSection();
	bool exit = false;
	while (0 == (REG_KEYINPUT & KEY_DOWN) || 0 == (REG_KEYINPUT & KEY_B));	// Prevent going straight to the next option
	while (1) {
		if (0 == (REG_KEYINPUT & KEY_UP)) {
			cursorPosition--;
			if (cursorPosition < 0) cursorPosition = 0;
			while (0 == (REG_KEYINPUT & KEY_UP));
		} else if (0 == (REG_KEYINPUT & KEY_DOWN)) {
			cursorPosition++;
			if (cursorPosition > 2) cursorPosition = 2;
			while (0 == (REG_KEYINPUT & KEY_DOWN));
		}
		if (0 == (REG_KEYINPUT & KEY_A)) {
			switch (cursorPosition) {
				case 0:
				default:
					break;
				case 1:
					forceGameReboot();
					break;
				case 2:
					returnToLoader();
					break;
			}
			exit = true;
		}
		if (0 == (REG_KEYINPUT & KEY_B)) {
			cursorPosition = 0;
			exit = true;
		}
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
		if (exit) {
			sharedAddr[4] = 0x54495845;	// 'EXIT'
			break;
		}
	}
	leaveCriticalSection(oldIME);
	REG_MASTER_VOLUME = 127;
}