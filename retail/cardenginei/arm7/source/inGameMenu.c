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

volatile int timeTilBatteryLevelRefresh = 7;

static u32 romWordBak[6] = {0};

void inGameMenu(void) {
	romWordBak[0] = sharedAddr[-1];
	romWordBak[1] = sharedAddr[0];
	romWordBak[2] = sharedAddr[1];
	romWordBak[3] = sharedAddr[2];
	romWordBak[4] = sharedAddr[3];
	romWordBak[5] = sharedAddr[4];

	sharedAddr[4] = 0x554E454D; // 'MENU'
	IPC_SendSync(0x9);
	REG_MASTER_VOLUME = 0;
	int oldIME = enterCriticalSection();

	int timeOut = 0;
	while (sharedAddr[-1] != 0x59444552) { // 'REDY'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);

		timeOut++;
		if (timeOut == 60*2) {
			returnToLoader();
			timeOut = 0;
		}
	}

	while (sharedAddr[4] == 0x554E454D) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);

		sharedAddr[5] = ~REG_KEYINPUT & 0x3FF;
		sharedAddr[5] |= (~REG_EXTKEYINPUT & 0x3) << 10;
		timeTilBatteryLevelRefresh++;
		if (timeTilBatteryLevelRefresh == 8) {
			*(u8*)(INGAME_MENU_LOCATION+0x7FFF) = i2cReadRegister(I2C_PM, I2CREGPM_BATTERY);
			timeTilBatteryLevelRefresh = 0;
		}
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
	sharedAddr[-1] = 0;
	timeTilBatteryLevelRefresh = 7;

	leaveCriticalSection(oldIME);
	sharedAddr[-1] = romWordBak[0];
	sharedAddr[0] = romWordBak[1];
	sharedAddr[1] = romWordBak[2];
	sharedAddr[2] = romWordBak[3];
	sharedAddr[3] = romWordBak[4];
	sharedAddr[4] = romWordBak[5];
	REG_MASTER_VOLUME = 127;
}