#include <nds/ndstypes.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/clock.h>
#include <time.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"
#include "tonccpy.h"

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern vu32* volatile sharedAddr;
extern bool ipcEveryFrame;
extern bool returnToMenu;

extern void rebootConsole(void);
extern void reset(void);

volatile int timeTillStatusRefresh = 7;

void inGameMenu(void) {
	sharedAddr[4] = 0x554E454D; // 'MENU'
	IPC_SendSync(0x9);
	int volBak = REG_MASTER_VOLUME;
	REG_MASTER_VOLUME = 0;
	int oldIME = enterCriticalSection();

	int timeOut = 0;
	while (sharedAddr[5] != 0x59444552) { // 'REDY'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);

		timeOut++;
		if (timeOut == 60*2) {
			rebootConsole();
			timeOut = 0;
		}
	}

	if (sharedAddr[4] == 0x554E454D) {
		bool exitMenu = false;
		while (!exitMenu) {
			sharedAddr[5] = ~REG_KEYINPUT & 0x3FF;
			sharedAddr[5] |= ((~REG_EXTKEYINPUT & 0x3) << 10) | ((~REG_EXTKEYINPUT & 0xC0) << 6);
			timeTillStatusRefresh++;
			if (timeTillStatusRefresh >= 8) {
				timeTillStatusRefresh = 0;

				u32 pmBacklight = readPowerManagement(PM_BACKLIGHT_LEVEL);
				if(pmBacklight & 0xF0) { // DS Lite
					sharedAddr[6] = ((pmBacklight & 3) + ((readPowerManagement(PM_CONTROL_REG) & 0xC) != 0)) << 8; // Brightness
				} else { // DS Phat
					sharedAddr[6] = (((readPowerManagement(PM_CONTROL_REG) & 0xC) != 0) ? 5 : 0) << 8;
				}

				RTCtime dstime;
				rtcGetTimeAndDate((uint8 *)&dstime);
				sharedAddr[7] = dstime.hours;
				sharedAddr[8] = dstime.minutes;
				sharedAddr[7] += 0x10000000; // Set time recieve flag
			}

			while (REG_VCOUNT != 191) swiDelay(100);
			while (REG_VCOUNT == 191) swiDelay(100);

			switch (sharedAddr[4]) {
				case 0x54495845: // EXIT
					exitMenu = true;
					break;
				case 0x54455352: // RSET
					exitMenu = true;
					timeTillStatusRefresh = 7;
					for (int i = 0; i < 2; i++) {
						while (REG_VCOUNT != 191) swiDelay(100);
						while (REG_VCOUNT == 191) swiDelay(100);
					}
					reset();
					break;
				case 0x54495551: // QUIT
					rebootConsole();
					exitMenu = true;
					break;
				case 0x59435049: // IPCY
					ipcEveryFrame = true;
					break;
				case 0x4E435049: // IPCN
					ipcEveryFrame = false;
					break;
				case 0x524D4152: // RAMR
					tonccpy((u32*)((u32)sharedAddr[0]), (u32*)((u32)sharedAddr[1]), 0xC0);
					break;
				case 0x574D4152: // RAMW
					tonccpy((u8*)((u32)sharedAddr[1])+sharedAddr[2], (u8*)((u32)sharedAddr[0])+sharedAddr[2], 1);
					break;
				case 0x4554494C: // LITE
					if(sharedAddr[0] == 0) {
						writePowerManagement(PM_CONTROL_REG, readPowerManagement(PM_CONTROL_REG) & ~0xC);
					} else {
						u32 pmBacklight = readPowerManagement(PM_BACKLIGHT_LEVEL);
						if(pmBacklight & 0xF0) // DS Lite
							writePowerManagement(PM_BACKLIGHT_LEVEL, (pmBacklight & ~3) | (sharedAddr[0] - 1));
						writePowerManagement(PM_CONTROL_REG, readPowerManagement(PM_CONTROL_REG) | 0xC);
					}
					timeTillStatusRefresh = 7;
					break;
				default:
					break;
			}

			if (!exitMenu) {
				sharedAddr[4] = 0x554E454D; // MENU
			}
		}
	}

	sharedAddr[4] = 0x54495845; // EXIT

	leaveCriticalSection(oldIME);
	REG_MASTER_VOLUME = volBak;
}
