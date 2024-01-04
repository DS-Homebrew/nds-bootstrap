#include <nds/ndstypes.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/clock.h>
#include <nds/arm7/i2c.h>
#include <time.h>

#include "igm_text.h"
#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"
#include "tonccpy.h"

#define sleepMode BIT(17)

#define REG_EXTKEYINPUT (*(vuint16*)0x04000136)

extern u32 valueBits;
extern u8 consoleModel;

extern vu32* volatile sharedAddr;
extern bool ipcEveryFrame;
extern bool returnToMenu;

extern struct IgmText *igmText;

extern void reset(void);
extern void dumpRam(void);
extern void returnToLoader(bool reboot);
extern void prepareScreenshot(void);
extern void saveScreenshot(void);
extern void prepareManual(void);
extern void readManual(int line);
extern void restorePreManual(void);
extern void saveMainScreenSetting(void);
extern void loadInGameMenu(void);
extern void unloadInGameMenu(void);

extern u16 biosRead16(u32 addr);

void biosRead(void* dst, const void* src, u32 len)
{
	u16* _dst = (u16*)dst;
	
	for (u32 i = 0; i < len; i+=2)
	{
		_dst[i>>1] = biosRead16(((u32)src) + i);
	}
}

volatile int timeTillStatusRefresh = 7;

void inGameMenu(void) {
	returnToMenu = false;
	sharedAddr[4] = 0x554E454D; // 'MENU'
	u32 errorBak = sharedAddr[0];
	IPC_SendSync(0x9);
	REG_MASTER_VOLUME = 0;
	int oldIME = enterCriticalSection();

	loadInGameMenu();

	int timeOut = 0;
	while (sharedAddr[5] != 0x59444552) { // 'REDY'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);

		timeOut++;
		if (timeOut == 60*2) {
			returnToLoader(true);
			timeOut = 0;
		}
	}

	if (sharedAddr[4] == 0x554E454D) {
		bool exitMenu = false;
		while (!exitMenu) {
			sharedAddr[5] = ~REG_KEYINPUT & 0x3FF;
			sharedAddr[5] |= ((~REG_EXTKEYINPUT & 0x3) << 10) | ((~REG_EXTKEYINPUT & 0xC0) << 6);
			if ((REG_EXTKEYINPUT & BIT(7)) && (valueBits & sleepMode) && (consoleModel < 2)) {
				// Save current power state.
				int power = readPowerManagement(PM_CONTROL_REG);
				// Set sleep LED. (Does not work)
				writePowerManagement(PM_CONTROL_REG, PM_LED_CONTROL(1));

				// Power down till we get our interrupt.
				swiSleep();

				while (REG_EXTKEYINPUT & BIT(7)) {
					//100ms
					swiDelay(838000);
				}

				// Restore power state.
				writePowerManagement(PM_CONTROL_REG, power);
			}
			timeTillStatusRefresh++;
			if (timeTillStatusRefresh >= 8) {
				timeTillStatusRefresh = 0;

				u8 brightness = i2cReadRegister(I2C_PM, 0x41);
				u32 pmControl = readPowerManagement(PM_CONTROL_REG);
				if(brightness && !(pmControl & 0xC)) { // Turn on backlights if SELECT+volume used
					pmControl |= 0xC;
					writePowerManagement(PM_CONTROL_REG, pmControl);
				}

				sharedAddr[6] = i2cReadRegister(I2C_PM, I2CREGPM_BATTERY); // Battery
				sharedAddr[6] |= (brightness + ((pmControl & 0xC) != 0)) << 8; // Brightness
				sharedAddr[6] |= i2cReadRegister(I2C_PM, I2CREGPM_VOL) << 16; // Volume

				RTCtime dstime;
				rtcGetTimeAndDate((uint8 *)&dstime);
				sharedAddr[7] = dstime.hours;
				sharedAddr[8] = dstime.minutes;
				sharedAddr[7] += 0x10000000; // Set time receive flag
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
					unloadInGameMenu();
					#ifdef TWLSDK
					i2cWriteRegister(0x4A, 0x12, 0x01);
					#endif
					reset();
					break;
				case 0x54495551: // QUIT
					unloadInGameMenu();
					returnToLoader(false);
					exitMenu = true;
					break;
				case 0x59435049: // IPCY
					ipcEveryFrame = true;
					break;
				case 0x4E435049: // IPCN
					ipcEveryFrame = false;
					break;
				case 0x53435049: // IPCS
					saveMainScreenSetting();
					break;
				case 0x444D4152: // RAMD
					dumpRam();
					exitMenu = true;
					break;
				case 0x50455453: // STEP
					returnToMenu = true;
					exitMenu = true;
					break;
				case 0x50505353: // SSPP
					prepareScreenshot();
					break;
				case 0x544F4853: // SHOT
					saveScreenshot();
					break;
				case 0x4E414D50: // PMAN
					prepareManual();
					break;
				case 0x554E414D: // MANU
					readManual(sharedAddr[0]);
					break;
				case 0x4E414D52: // RMAN
					restorePreManual();
					break;
				case 0x524D4152: // RAMR
					u32* dst = (u32*)((u32)sharedAddr[0]);
					u32* src = (u32*)((u32)sharedAddr[1]);
					for (int i = 0; i < 0xC0/8; i++) {
						if ((u32)src >= 0x8000) {
							tonccpy(dst, src, 8);
						} else {
							biosRead(dst, src, 8);
						}
						dst++;
						dst++;
						src++;
						src++;
					}
					break;
				case 0x574D4152: // RAMW
					if (sharedAddr[1]+sharedAddr[2] >= 0x8000) {
						tonccpy((u8*)((u32)sharedAddr[1])+sharedAddr[2], (u8*)((u32)sharedAddr[0])+sharedAddr[2], 1);
					}
					break;
				case 0x4554494C: // LITE
					if(sharedAddr[0] == 0) {
						writePowerManagement(PM_CONTROL_REG, readPowerManagement(PM_CONTROL_REG) & ~0xC);
					} else {
						i2cWriteRegister(I2C_PM, 0x41, sharedAddr[0] - 1);
						writePowerManagement(PM_CONTROL_REG, readPowerManagement(PM_CONTROL_REG) | 0xC);
					}
					timeTillStatusRefresh = 7;
					break;
				case 0x554C4F56: // VOLU
					i2cWriteRegister(I2C_PM, I2CREGPM_VOL, sharedAddr[0]);
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

	sharedAddr[0] = errorBak;
	sharedAddr[4] = 0x54495845; // EXIT
	sharedAddr[7] -= 0x10000000; // Clear time receive flag
	timeTillStatusRefresh = 7;

	unloadInGameMenu();

	leaveCriticalSection(oldIME);
	REG_MASTER_VOLUME = 127;
}
