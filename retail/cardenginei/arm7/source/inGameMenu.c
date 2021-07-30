#include <nds/ndstypes.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/i2c.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"

#define REG_EXTKEYINPUT (*(vuint16*)0x04000136)

extern vu32* volatile sharedAddr;
extern bool returnToMenu;

extern void forceGameReboot(void);
extern void dumpRam(void);
extern void returnToLoader(void);
extern void saveScreenshot(void);

volatile int timeTilBatteryLevelRefresh = 7;

void inGameMenu(void) {
	returnToMenu = false;
	sharedAddr[4] = 0x554E454D; // 'MENU'
	IPC_SendSync(0x9);
	REG_MASTER_VOLUME = 0;
	int oldIME = enterCriticalSection();

	int timeOut = 0;
	while (sharedAddr[5] != 0x59444552) { // 'REDY'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);

		timeOut++;
		if (timeOut == 60*2) {
			returnToLoader();
			timeOut = 0;
		}
	}

	if (sharedAddr[4] == 0x554E454D) {
		while (1) {
			sharedAddr[5] = ~REG_KEYINPUT & 0x3FF;
			sharedAddr[5] |= ((~REG_EXTKEYINPUT & 0x3) << 10) | ((~REG_EXTKEYINPUT & 0xC0) << 6);
			timeTilBatteryLevelRefresh++;
			if (timeTilBatteryLevelRefresh == 8) {
				*(u8*)(INGAME_MENU_LOCATION+0x9FFF) = i2cReadRegister(I2C_PM, I2CREGPM_BATTERY);
				timeTilBatteryLevelRefresh = 0;
			}

			while (REG_VCOUNT != 191);
			while (REG_VCOUNT == 191);

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
				case 0x544F4853: // SHOT
					saveScreenshot();
					sharedAddr[4] = 0x554E454D;
					break;
				case 0x50455453: // STEP
					returnToMenu = true;
					break;
			}
			if (sharedAddr[4] != 0x554E454D) {
				break;
			}
		}
	}

	sharedAddr[4] = 0x54495845; // EXIT
	timeTilBatteryLevelRefresh = 7;

	leaveCriticalSection(oldIME);
	REG_MASTER_VOLUME = 127;
}
