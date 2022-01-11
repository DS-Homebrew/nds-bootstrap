/*
	NitroHax -- Cheat tool for the Nintendo DS
	Copyright (C) 2008  Michael "Chishm" Chisholm

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <nds/ndstypes.h>
#include <nds/fifomessages.h>
#include <nds/dma.h>
#include <nds/ipc.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/i2c.h>
#include <nds/memory.h> // tNDSHeader
#include <nds/debug.h>

#include "tonccpy.h"
#include "my_sdmmc.h"
#include "my_fat.h"
#include "locations.h"
#include "module_params.h"
#include "debug_file.h"
#include "cardengine.h"
#include "nds_header.h"
#include "igm_text.h"

#include "sr_data_error.h"      // For showing an error screen
#include "sr_data_srloader.h"   // For rebooting into TWiLight Menu++
#include "sr_data_srllastran.h" // For rebooting the game

#define gameOnFlashcard BIT(0)
#define saveOnFlashcard BIT(1)
#define extendedMemory BIT(2)
#define ROMinRAM BIT(3)
#define dsiMode BIT(4)
#define b_dsiSD BIT(5)
#define preciseVolumeControl BIT(6)
#define powerCodeOnVBlank BIT(7)
#define b_runCardEngineCheck BIT(8)
#define ipcEveryFrame BIT(9)
#define hiyaCfwFound BIT(10)
#define slowSoftReset BIT(11)
#define scfgLocked BIT(31)

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern u32 ce7;

static const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char bootNdsPath[14] = {'s','d','m','c',':','/','b','o','o','t','.','n','d','s'};
static char hiyaDSiPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

extern u32 srParamsCluster;
extern u32 ramDumpCluster;
extern u32 screenshotCluster;
extern u32 pageFileCluster;
extern module_params_t* moduleParams;
extern u32 valueBits;
extern u32* languageAddr;
extern u8 language;
extern u8 consoleModel;
extern u16 igmHotkey;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION_DSIWARE;

bool dsiSD = false;
bool sdRead = true;

static bool initialized = false;
static bool driveInited = false;
static bool bootloaderCleared = false;
static bool swapScreens = false;

static aFile ramDumpFile;
static aFile srParamsFile;
static aFile screenshotFile;
static aFile pageFile;

static int languageTimer = 0;
static int swapTimer = 0;
static int returnTimer = 0;
static int softResetTimer = 0;
static int ramDumpTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;

bool returnToMenu = false;

//static const tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
static PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER_SDK5-0x180);

static void unlaunchSetFilename(bool boot) {
	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) = (BIT(0) | BIT(1));		// Load the title at 2000838h
													// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	if (boot) {
		for (int i = 0; i < (int)sizeof(bootNdsPath); i++) {
			*(u8*)(0x02000838+i2) = bootNdsPath[i];				// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			i2 += 2;
		}
	} else {
		for (int i = 0; i < 256; i++) {
			*(u8*)(0x02000838+i2) = *(u8*)(ce7+0x7C00+i);		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			i2 += 2;
		}
	}
	*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
}

static void unlaunchSetHiyaFilename(void) {
	if (!(valueBits & hiyaCfwFound)) return;

	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) = (BIT(0) | BIT(1));		// Load the title at 2000838h
													// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < (int)sizeof(hiyaDSiPath); i++) {
		*(u8*)(0x02000838+i2) = hiyaDSiPath[i];				// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
}

static void readSrBackendId(void) {
	// Use SR backend ID
	*(u32*)(0x02000300) = 0x434E4C54;	// 'CNLT'
	*(u16*)(0x02000304) = 0x1801;
	*(u32*)(0x02000308) = 0;
	*(u32*)(0x0200030C) = 0;
	*(u32*)(0x02000310) = *(u32*)(ce7+0x7D00);
	*(u32*)(0x02000314) = *(u32*)(ce7+0x7D04);
	*(u32*)(0x02000318) = 0x17;
	*(u32*)(0x0200031C) = 0;
	*(u16*)(0x02000306) = swiCRC16(0xFFFF, (void*)0x02000308, 0x18);
}

// Alternative to swiWaitForVBlank()
static void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

static bool isSdEjected(void) {
	if (*(vu32*)(0x400481C) & BIT(3)) {
		return true;
	}
	return false;
}

static void driveInitialize(void) {
	if (driveInited) {
		return;
	}

	bool sdReadBak = sdRead;

	if (valueBits & b_dsiSD) {
		if (sdmmc_read16(REG_SDSTATUS0) != 0) {
			sdmmc_init();
			SD_Init();
		}
		dsiSD = true;
		sdRead = true;				// Switch to SD
		FAT_InitFiles(false, false, 0);
	}

	ramDumpFile = getFileFromCluster(ramDumpCluster);
	srParamsFile = getFileFromCluster(srParamsCluster);
	screenshotFile = getFileFromCluster(screenshotCluster);
	pageFile = getFileFromCluster(pageFileCluster);

	#ifdef DEBUG		
	aFile myDebugFile = getBootFileCluster("NDSBTSRP.LOG", 0);
	enableDebug(myDebugFile);
	dbg_printf("logging initialized\n");		
	dbg_printf("sdk version :");
	dbg_hexa(moduleParams->sdk_version);		
	dbg_printf("\n");
	#endif
	
	sdRead = sdReadBak;

	driveInited = true;
}

static void initialize(void) {
	if (initialized) {
		return;
	}

	if (language >= 0 && language <= 7) {
		// Change language
		personalData->language = language;
	}

	if (!bootloaderCleared) {
		toncset((u8*)0x06000000, 0, 0x40000);	// Clear bootloader
		bootloaderCleared = true;
	}

	initialized = true;
}

extern void inGameMenu(void);

u32 auxIeBak = 0;
u32 sdStatBak = 0;
u32 sdMaskBak = 0;

void bakData(void) {
	auxIeBak = REG_AUXIE;
	sdStatBak = *(vu32*)0x400481C;
	sdMaskBak = *(vu32*)0x4004820;

	i2cWriteRegister(0x4A, 0x12, 0x00);
	REG_AUXIE &= ~(1UL << 8);
	*(vu32*)0x400481C = 0;
	*(vu32*)0x4004820 = 0;
}

void restoreBakData(void) {
	REG_AUXIE = auxIeBak;
	*(vu32*)0x400481C = sdStatBak;
	*(vu32*)0x4004820 = sdMaskBak;
	i2cWriteRegister(0x4A, 0x12, 0x01);
}

void forceGameReboot(void) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)(0x02000000) = 0;
	sharedAddr[4] = 0x57534352;
	IPC_SendSync(0x8);
	if (consoleModel < 2) {
		if (valueBits & b_dsiSD) {
			(*(u32*)(ce7+0x7D00) == 0) ? unlaunchSetFilename(false) : unlaunchSetHiyaFilename();
		}
		waitFrames(5);							// Wait for DSi screens to stabilize
	}
	u32 clearBuffer = 0;
	driveInitialize();
	sdRead = !(valueBits & gameOnFlashcard);
	fileWrite((char*)&clearBuffer, srParamsFile, 0, 0x4, !sdRead, -1);
	if (*(u32*)(ce7+0x7D00) == 0 && (valueBits & b_dsiSD)) {
		tonccpy((u32*)0x02000300, sr_data_srllastran, 0x020);
	} else {
		// Use different SR backend ID
		readSrBackendId();
	}
	i2cWriteRegister(0x4A, 0x70, 0x01);
	i2cWriteRegister(0x4A, 0x11, 0x01);		// Force-reboot game
}

void returnToLoader(void) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)(0x02000000) = BIT(0) | BIT(1) | BIT(2);
	sharedAddr[4] = 0x57534352;
	IPC_SendSync(0x8);
	if (consoleModel >= 2) {
		if (*(u32*)(ce7+0x7D00) == 0 && (valueBits & b_dsiSD)) {
			tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
		} else if (*(char*)(ce7+0x7D03) == 'H' || *(char*)(ce7+0x7D03) == 'K') {
			// Use different SR backend ID
			readSrBackendId();
		}
		waitFrames(1);
	} else {
		if (*(u32*)(ce7+0x7D00) == 0 && (valueBits & b_dsiSD)) {
			unlaunchSetFilename(true);
		} else {
			// Use different SR backend ID
			readSrBackendId();
		}
		waitFrames(5);							// Wait for DSi screens to stabilize
	}
	i2cWriteRegister(0x4A, 0x70, 0x01);
	i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into TWiLight Menu++
}

void dumpRam(void) {
	driveInitialize();
	sdRead = (valueBits & b_dsiSD);
	sharedAddr[3] = 0x444D4152;
	// Dump RAM
	if (valueBits & dsiMode) {
		// Dump full RAM
		fileWrite((char*)0x0C000000, ramDumpFile, 0, (consoleModel==0 ? 0x01000000 : 0x02000000), !sdRead, -1);
	} else {
		// Dump RAM used in DS mode
		fileWrite((char*)0x02000000, ramDumpFile, 0, 0x3E0000, !sdRead, -1);
		fileWrite((char*)((moduleParams->sdk_version > 0x5000000) ? 0x02FE0000 : 0x027E0000), ramDumpFile, 0x3E0000, 0x20000, !sdRead, -1);
	}
	sharedAddr[3] = 0;
}

void prepareScreenshot(void) {
	driveInitialize();
	sdRead = (valueBits & b_dsiSD);
	fileWrite((char*)INGAME_MENU_EXT_LOCATION, pageFile, 0x400000, 0x40000, !sdRead, -1);
}

void saveScreenshot(void) {
	if (igmText->currentScreenshot >= 50) return;

	driveInitialize();
	sdRead = (valueBits & b_dsiSD);
	fileWrite((char*)INGAME_MENU_EXT_LOCATION, screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 0x18046, !sdRead, -1);

	// Skip until next blank slot
	char magic;
	do {
		igmText->currentScreenshot++;
		fileRead(&magic, screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 1, !sdRead, -1);
	} while(magic == 'B' && igmText->currentScreenshot < 50);

	fileRead((char*)INGAME_MENU_EXT_LOCATION, pageFile, 0x400000, 0x40000, !sdRead, -1);
}

void myIrqHandlerVBlank(void) {
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	#ifdef DEBUG
	nocashMessage("cheat_engine_start\n");
	#endif

	if (*(u32*)((u32)ce7-(0x8400+0x3E8)) != 0xCF000000) {
		volatile void (*cheatEngine)() = (volatile void*)ce7-0x83FC;
		(*cheatEngine)();
	}

	if (language >= 0 && language <= 7 && languageTimer < 60*3) {
		// Change language
		personalData->language = language;
		if (languageAddr > 0) {
			// Extra measure for specific games
			*languageAddr = language;
		}
		languageTimer++;
	}

	if (isSdEjected()) {
		tonccpy((u32*)0x02000300, sr_data_error, 0x020);
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into error screen if SD card is removed
	}

	if ((0 == (REG_KEYINPUT & igmHotkey) && 0 == (REG_EXTKEYINPUT & (((igmHotkey >> 10) & 3) | ((igmHotkey >> 6) & 0xC0)))) || returnToMenu) {
		bakData();
		inGameMenu();
		restoreBakData();
	}

	if (0==(REG_KEYINPUT & (KEY_L | KEY_R | KEY_UP)) && !(REG_EXTKEYINPUT & KEY_A/*KEY_X*/)) {
		if (swapTimer == 60){
			swapTimer = 0;
			//if (!(valueBits & ipcEveryFrame)) {
				IPC_SendSync(0x7);
			//}
			swapScreens = true;
		}
		swapTimer++;
	}else{
		swapTimer = 0;
	}
	
	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B))) {
		if (returnTimer == 60 * 2) {
			returnToLoader();
		}
		returnTimer++;
	} else {
		returnTimer = 0;
	}

	if ((valueBits & b_dsiSD) && (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A)))) {
		if (ramDumpTimer == 60 * 2) {
			REG_MASTER_VOLUME = 0;
			int oldIME = enterCriticalSection();
			bakData();
			dumpRam();
			restoreBakData();
			leaveCriticalSection(oldIME);
			REG_MASTER_VOLUME = 127;
		}
		ramDumpTimer++;
	} else {
		ramDumpTimer = 0;
	}

	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT))) {
		if (softResetTimer == 60 * 2) {
			REG_MASTER_VOLUME = 0;
			int oldIME = enterCriticalSection();
			bakData();
			forceGameReboot();
			restoreBakData();
			leaveCriticalSection(oldIME);
		}
		softResetTimer++;
	} else {
		softResetTimer = 0;
	}

	if (valueBits & preciseVolumeControl) {
		// Precise volume adjustment (for DSi)
		if (volumeAdjustActivated) {
			volumeAdjustDelay++;
			if (volumeAdjustDelay == 30) {
				volumeAdjustDelay = 0;
				volumeAdjustActivated = false;
			}
		} else if (0==(REG_KEYINPUT & KEY_SELECT)) {
			u8 i2cVolLevel = i2cReadRegister(0x4A, 0x40);
			u8 i2cNewVolLevel = i2cVolLevel;
			if (0==(REG_KEYINPUT & KEY_UP)) {
				i2cNewVolLevel++;
			} else if (0==(REG_KEYINPUT & KEY_DOWN)) {
				i2cNewVolLevel--;
			}
			if (i2cNewVolLevel == 0xFF) {
				i2cNewVolLevel = 0;
			} else if (i2cNewVolLevel > 0x1F) {
				i2cNewVolLevel = 0x1F;
			}
			if (i2cNewVolLevel != i2cVolLevel) {
				i2cWriteRegister(0x4A, 0x40, i2cNewVolLevel);
				volumeAdjustActivated = true;
			}
		}
	}

	/*if (REG_IE & IRQ_NETWORK) {
		REG_IE &= ~IRQ_NETWORK; // DSi RTC fix (Not needed for DSiWare)
	}*/
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG		
	nocashMessage("myIrqEnable\n");
	#endif	

	initialize();

	u32 irq_before = REG_IE;		
	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
