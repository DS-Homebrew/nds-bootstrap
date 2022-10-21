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

#include "ndma.h"
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

#define preciseVolumeControl BIT(6)
#define hiyaCfwFound BIT(10)
#define wideCheatUsed BIT(12)
#define twlTouch BIT(15)
#define scfgLocked BIT(31)

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern u32 ce7;

static const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char bootNdsPath[14] = {'s','d','m','c',':','/','b','o','o','t','.','n','d','s'};
static char hiyaDSiPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

extern void ndsCodeStart(u32* addr);

extern u32 saveCluster;
extern u32 srParamsCluster;
extern u32 ramDumpCluster;
extern u32 screenshotCluster;
extern u32 pageFileCluster;
extern u32 manualCluster;
extern module_params_t* moduleParams;
extern u32 valueBits;
extern u32* languageAddr;
extern u8 language;
extern u8 consoleModel;
extern u16 igmHotkey;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION_DSIWARE;

static bool initialized = false;
static bool driveInited = false;
static bool bootloaderCleared = false;
bool ipcEveryFrame = false;
static bool swapScreens = false;

#ifdef CARDSAVE
static aFile savFile;
#endif
static aFile ramDumpFile;
static aFile srParamsFile;
static aFile screenshotFile;
static aFile pageFile;
static aFile manualFile;

static int sdRightsTimer = 0;
static int languageTimer = 0;
static int swapTimer = 0;
static int returnTimer = 0;
static int softResetTimer = 0;
static int ramDumpTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;

bool returnToMenu = false;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
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
			*(u8*)(0x02000838+i2) = *(u8*)(ce7+0x8C00+i);		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
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
	*(u32*)(0x02000310) = *(u32*)(ce7+0x8D00);
	*(u32*)(0x02000314) = *(u32*)(ce7+0x8D04);
	*(u32*)(0x02000318) = 0x17;
	*(u32*)(0x0200031C) = 0;
	*(u16*)(0x02000306) = swiCRC16(0xFFFF, (void*)0x02000308, 0x18);
}

// Alternative to swiWaitForVBlank()
static inline void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

static inline bool isSdEjected(void) {
	if (*(vu32*)(0x400481C) & BIT(3)) {
		return true;
	}
	return false;
}

static void driveInitialize(void) {
	if (driveInited) {
		return;
	}

	if (sdmmc_read16(REG_SDSTATUS0) != 0) {
		sdmmc_init();
		SD_Init();
	}
	FAT_InitFiles(false, 0);

	#ifdef CARDSAVE
	savFile = getFileFromCluster(saveCluster);
	#endif
	ramDumpFile = getFileFromCluster(ramDumpCluster);
	srParamsFile = getFileFromCluster(srParamsCluster);
	screenshotFile = getFileFromCluster(screenshotCluster);
	pageFile = getFileFromCluster(pageFileCluster);
	manualFile = getFileFromCluster(manualCluster);

	#ifdef DEBUG		
	aFile myDebugFile = getBootFileCluster("NDSBTSRP.LOG", 0);
	enableDebug(myDebugFile);
	dbg_printf("logging initialized\n");		
	dbg_printf("sdk version :");
	dbg_hexa(moduleParams->sdk_version);		
	dbg_printf("\n");
	#endif
	
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

#ifdef CARDSAVE
void bakSdData(void) {
	auxIeBak = REG_AUXIE;
	sdStatBak = *(vu32*)0x400481C;
	sdMaskBak = *(vu32*)0x4004820;

	REG_AUXIE &= ~(1UL << 8);
	*(vu32*)0x400481C = 0;
	*(vu32*)0x4004820 = 0;
}

void restoreSdBakData(void) {
	REG_AUXIE = auxIeBak;
	*(vu32*)0x400481C = sdStatBak;
	*(vu32*)0x4004820 = sdMaskBak;
}
#endif

void reset(void) {
	register int i, reg;

	REG_IME = 0;

	for (i = 0; i < 16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}

	REG_SOUNDCNT = 0;
	REG_SNDCAP0CNT = 0;
	REG_SNDCAP1CNT = 0;

	REG_SNDCAP0DAD = 0;
	REG_SNDCAP0LEN = 0;
	REG_SNDCAP1DAD = 0;
	REG_SNDCAP1LEN = 0;

	// Clear out ARM7 DMA channels and timers
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	REG_IE = 0;
	REG_IF = ~0;
	REG_AUXIE = 0;
	REG_AUXIF = ~0;
	*(vu32*)0x0380FFFC = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)0x0380FFF8 = 0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff

	initialized = false;
	sdRightsTimer = 0;
	languageTimer = 0;

	u8* deviceListAddr = (u8*)(*(u32*)0x02FFE1D4);
	toncset(deviceListAddr+2, 0, 1); // Clear SD access rights

	if (consoleModel > 0) {
		ndmaCopyWordsAsynch(0, (char*)ndsHeader->arm9destination+0xB000000, ndsHeader->arm9destination, *(u32*)0x0DFFE02C);
		ndmaCopyWordsAsynch(1, (char*)ndsHeader->arm7destination+0xB000000, ndsHeader->arm7destination, *(u32*)0x0DFFE03C);
		ndmaCopyWordsAsynch(2, (char*)(*(u32*)0x02FFE1C8)+0xB000000, (void*)(*(u32*)0x02FFE1C8), *(u32*)0x0DFFE1CC);
		ndmaCopyWordsAsynch(3, (char*)(*(u32*)0x02FFE1D8)+0xB000000, (void*)(*(u32*)0x02FFE1D8), *(u32*)0x0DFFE1DC);
		while (ndmaBusy(0) || ndmaBusy(1) || ndmaBusy(2) || ndmaBusy(3));
	} else {
		bakData();

		driveInitialize();

		u32 iUncompressedSize = 0;
		u32 iUncompressedSizei = 0;
		u32 newArm7binarySize = 0;
		u32 newArm7ibinarySize = 0;

		fileRead((char*)&iUncompressedSize, pageFile, 0x5FFFF0, sizeof(u32), 0);
		fileRead((char*)&newArm7binarySize, pageFile, 0x5FFFF4, sizeof(u32), 0);
		fileRead((char*)&iUncompressedSizei, pageFile, 0x5FFFF8, sizeof(u32), 0);
		fileRead((char*)&newArm7ibinarySize, pageFile, 0x5FFFFC, sizeof(u32), 0);
		fileRead((char*)ndsHeader->arm9destination, pageFile, 0, iUncompressedSize, 0);
		fileRead((char*)ndsHeader->arm7destination, pageFile, 0x2C0000, newArm7binarySize, 0);
		fileRead((char*)(*(u32*)0x02FFE1C8), pageFile, 0x300000, iUncompressedSizei, 0);
		fileRead((char*)(*(u32*)0x02FFE1D8), pageFile, 0x580000, newArm7ibinarySize, 0);

		restoreBakData();
	}
	toncset((char*)0x02FFFD80, 0, 0x80);
	toncset((char*)0x02FFFF80, 0, 0x80);

	sharedAddr[0] = 0x44414F4C; // 'LOAD'

	for (i = 0; i < 4; i++) {
		for(reg=0; reg<0x1c; reg+=4)*((vu32*)(0x04004104 + ((i*0x1c)+reg))) = 0;//Reset NDMA.
	}

	while (sharedAddr[0] != 0x544F4F42) { // 'BOOT'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}

	// Start ARM7
	ndsCodeStart(ndsHeader->arm7executeAddress);
}

void forceGameReboot(void) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)(0x02000000) = 0;
	sharedAddr[4] = 0x57534352;
	IPC_SendSync(0x8);
	if (consoleModel < 2) {
		(*(u32*)(ce7+0x8D00) == 0) ? unlaunchSetFilename(false) : unlaunchSetHiyaFilename();
		waitFrames(5);							// Wait for DSi screens to stabilize
	}
	u32 clearBuffer = 0;
	driveInitialize();
	fileWrite((char*)&clearBuffer, srParamsFile, 0, 0x4, -1);
	if (*(u32*)(ce7+0x8D00) == 0) {
		tonccpy((u32*)0x02000300, sr_data_srllastran, 0x020);
	} else {
		// Use different SR backend ID
		readSrBackendId();
	}
	i2cWriteRegister(0x4A, 0x70, 0x01);
	i2cWriteRegister(0x4A, 0x11, 0x01);		// Force-reboot game
}

static void initMBK_dsiMode(void) {
	// This function has no effect with ARM7 SCFG locked
	*(vu32*)REG_MBK1 = *(u32*)0x02FFE180;
	*(vu32*)REG_MBK2 = *(u32*)0x02FFE184;
	*(vu32*)REG_MBK3 = *(u32*)0x02FFE188;
	*(vu32*)REG_MBK4 = *(u32*)0x02FFE18C;
	*(vu32*)REG_MBK5 = *(u32*)0x02FFE190;
	REG_MBK6 = *(u32*)0x02FFE1A0;
	REG_MBK7 = *(u32*)0x02FFE1A4;
	REG_MBK8 = *(u32*)0x02FFE1A8;
	REG_MBK9 = *(u32*)0x02FFE1AC;
}

void returnToLoader(bool wait) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)(0x02000000) = BIT(0) | BIT(1) | BIT(2);
	sharedAddr[4] = 0x57534352;
	//IPC_SendSync(0x8);

	u32 twlCfgLoc = *(u32*)0x02FFFDFC;
	if (twlCfgLoc != 0x02000400) {
		tonccpy((u8*)0x02000400, (u8*)twlCfgLoc, 0x128);
	}

	if (((valueBits & twlTouch) && !(*(u8*)0x02FFE1BF & BIT(0))) || (valueBits & wideCheatUsed)) {
		if (consoleModel >= 2) {
			if (*(u32*)(ce7+0x8D00) == 0) {
				tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
			} else if (*(char*)(ce7+0x8D03) == 'H' || *(char*)(ce7+0x8D03) == 'K') {
				// Use different SR backend ID
				readSrBackendId();
			}
			//waitFrames(1);
		} else {
			if (*(u32*)(ce7+0x8D00) == 0) {
				unlaunchSetFilename(true);
			} else {
				// Use different SR backend ID
				readSrBackendId();
			}
			//waitFrames(wait ? 5 : 1);							// Wait for DSi screens to stabilize
		}
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);
	}

	register int i, reg;

	REG_IME = 0;

	for (i = 0; i < 16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}

	REG_SOUNDCNT = 0;
	REG_SNDCAP0CNT = 0;
	REG_SNDCAP1CNT = 0;

	REG_SNDCAP0DAD = 0;
	REG_SNDCAP0LEN = 0;
	REG_SNDCAP1DAD = 0;
	REG_SNDCAP1LEN = 0;

	// Clear out ARM7 DMA channels and timers
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	REG_IE = 0;
	REG_IF = ~0;
	REG_AUXIE = 0;
	REG_AUXIF = ~0;
	*(vu32*)0x0380FFFC = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)0x0380FFF8 = 0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff

	REG_AUXIE &= ~(1UL << 8);
	*(vu32*)0x400481C = 0;
	*(vu32*)0x4004820 = 0;

	driveInitialize();

	aFile file = getBootFileCluster("BOOT.NDS");
	if (file.firstCluster == CLUSTER_FREE) {
		// File not found, so reboot console instead
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);
	}

	fileRead((char*)__DSiHeader, file, 0, sizeof(tDSiHeader), 0);
	*ndsHeader = __DSiHeader->ndshdr;

	fileRead(__DSiHeader->ndshdr.arm9destination, file, (u32)__DSiHeader->ndshdr.arm9romOffset, __DSiHeader->ndshdr.arm9binarySize, 0);
	fileRead(__DSiHeader->ndshdr.arm7destination, file, (u32)__DSiHeader->ndshdr.arm7romOffset, __DSiHeader->ndshdr.arm7binarySize, 0);
	if (ndsHeader->unitCode > 0) {
		fileRead(__DSiHeader->arm9idestination, file, (u32)__DSiHeader->arm9iromOffset, __DSiHeader->arm9ibinarySize, 0);
		fileRead(__DSiHeader->arm7idestination, file, (u32)__DSiHeader->arm7iromOffset, __DSiHeader->arm7ibinarySize, 0);

		initMBK_dsiMode();
	}

	sharedAddr[0] = 0x44414F4C; // 'LOAD'

	for (i = 0; i < 4; i++) {
		for(reg=0; reg<0x1c; reg+=4)*((vu32*)(0x04004104 + ((i*0x1c)+reg))) = 0;//Reset NDMA.
	}

	while (sharedAddr[0] != 0x544F4F42) { // 'BOOT'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}

	// Start ARM7
	ndsCodeStart(ndsHeader->arm7executeAddress);
}

void dumpRam(void) {
	driveInitialize();
	sharedAddr[3] = 0x444D4152;
	// Dump RAM
	fileWrite((char*)0x0C000000, ramDumpFile, 0, (consoleModel==0 ? 0x01000000 : 0x02000000), -1);
	sharedAddr[3] = 0;
}

void prepareScreenshot(void) {
	driveInitialize();
	fileWrite((char*)INGAME_MENU_EXT_LOCATION, pageFile, 0x540000, 0x40000, -1);
}

void saveScreenshot(void) {
	if (igmText->currentScreenshot >= 50) return;

	driveInitialize();
	fileWrite((char*)INGAME_MENU_EXT_LOCATION, screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 0x18046, -1);

	// Skip until next blank slot
	char magic;
	do {
		igmText->currentScreenshot++;
		fileRead(&magic, screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 1, -1);
	} while(magic == 'B' && igmText->currentScreenshot < 50);

	fileRead((char*)INGAME_MENU_EXT_LOCATION, pageFile, 0x540000, 0x40000, -1);
}

void prepareManual(void) {
	driveInitialize();
	fileWrite((char*)INGAME_MENU_EXT_LOCATION, pageFile, 0x540000, 32 * 24, -1);
}

void readManual(int line) {
	static int currentManualLine = 0;
	static int currentManualOffset = 0;
	char buffer[32];

	// Seek for desired line
	bool firstLoop = true;
	while(currentManualLine != line) {
		if(line > currentManualLine) {
			fileRead(buffer, manualFile, currentManualOffset, 32, -1);

			for(int i = 0; i < 32; i++) {
				if(buffer[i] == '\n') {
					currentManualOffset += i + 1;
					currentManualLine++;
					break;
				} else if(i == 31) {
					currentManualOffset += i + 1;
					break;
				}
			}
		} else {
			currentManualOffset -= 32;
			fileRead(buffer, manualFile, currentManualOffset, 32, -1);
			int i = firstLoop ? 30 : 31;
			firstLoop = false;
			for(; i >= 0; i--) {
				if((buffer[i] == '\n') || currentManualOffset + i == -1) {
					currentManualOffset += i + 1;
					currentManualLine--;
					firstLoop = true;
					break;
				}
			}
		}
	}

	toncset((u8*)INGAME_MENU_EXT_LOCATION, ' ', 32 * 24);
	((vu8*)INGAME_MENU_EXT_LOCATION)[32 * 24] = '\0';

	// Read in 24 lines
	u32 tempManualOffset = currentManualOffset;
	bool fullLine = false;
	for(int line = 0; line < 24 && line < igmText->manualMaxLine; line++) {
		fileRead(buffer, manualFile, tempManualOffset, 32, -1);

		// Fix for exactly 32 char lines
		if(fullLine && buffer[0] == '\n')
			fileRead(buffer, manualFile, ++tempManualOffset, 32, -1);

		for(int i = 0; i <= 32; i++) {
			if(i == 32 || buffer[i] == '\n' || buffer[i] == '\0') {
				tempManualOffset += i;
				if(buffer[i] == '\n')
					tempManualOffset++;
				fullLine = i == 32;
				tonccpy((char*)INGAME_MENU_EXT_LOCATION + line * 32, buffer, i);
				break;
			}
		}
	}
}

void restorePreManual(void) {
	fileRead((char*)INGAME_MENU_EXT_LOCATION, pageFile, 0x540000, 32 * 24, -1);
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

	if (sdRightsTimer == 60*3) {
		u8* deviceListAddr = (u8*)(*(u32*)0x02FFE1D4);
		toncset(deviceListAddr+2, 0x06, 1); // Set SD access rights

		sdRightsTimer++;
	} else if (sdRightsTimer < 60*3) {
		sdRightsTimer++;
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
			if (!ipcEveryFrame) {
				IPC_SendSync(0x7);
			}
			swapScreens = true;
		}
		swapTimer++;
	}else{
		swapTimer = 0;
	}
	
	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B))) {
		if (returnTimer == 60 * 2) {
			IPC_SendSync(0x5);
		}
		returnTimer++;
	} else {
		returnTimer = 0;
	}

	if (sharedAddr[3] == (vu32)0x54495845) {
		returnToLoader(false);
	}

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A))) {
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

	if (sharedAddr[3] == (vu32)0x52534554) {
		reset();
	}

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT))) {
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

	// Update main screen or swap screens
	if (ipcEveryFrame) {
		IPC_SendSync(swapScreens ? 0x7 : 0x6);
	}
	swapScreens = false;
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

#ifdef CARDSAVE
//
// ARM7 Redirected functions
//

bool eepromProtect(void) {
	#ifdef DEBUG		
	dbg_printf("\narm7 eepromProtect\n");
	#endif	
	
	return true;
}

bool eepromRead(u32 src, void *dst, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromRead\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa(src);		
	dbg_printf("\ndst : \n");
	dbg_hexa((u32)dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	

	bakSdData();
	driveInitialize();
	fileRead(dst, savFile, src, len, -1);
	restoreSdBakData();

	return true;
}

bool eepromPageWrite(u32 dst, const void *src, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageWrite\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa((u32)src);		
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	
	
	bakSdData();
	driveInitialize();
	fileWrite(src, savFile, dst, len, -1);
	restoreSdBakData();

	return true;
}

bool eepromPageProg(u32 dst, const void *src, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageProg\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa((u32)src);		
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	

	bakSdData();
	driveInitialize();
	fileWrite(src, savFile, dst, len, -1);
	restoreSdBakData();

	return true;
}

bool eepromPageVerify(u32 dst, const void *src, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageVerify\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa((u32)src);		
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	

	//i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
	//fileWrite(src, savFile, dst, len, -1);
	//i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
	return true;
}

bool eepromPageErase (u32 dst) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageErase\n");	
	#endif	

	// TODO: this should be implemented?
	return true;
}

/*
TODO: return the correct ID

From gbatek 
Returns RAW unencrypted Chip ID (eg. C2h,0Fh,00h,00h), repeated every 4 bytes.
  1st byte - Manufacturer (eg. C2h=Macronix) (roughly based on JEDEC IDs)
  2nd byte - Chip size (00h..7Fh: (N+1)Mbytes, F0h..FFh: (100h-N)*256Mbytes?)
  3rd byte - Flags (see below)
  4th byte - Flags (see below)
The Flag Bits in 3th byte can be
  0   Maybe Infrared flag? (in case ROM does contain on-chip infrared stuff)
  1   Unknown (set in some 3DS carts)
  2-7 Zero
The Flag Bits in 4th byte can be
  0-2 Zero
  3   Seems to be NAND flag (0=ROM, 1=NAND) (observed in only ONE cartridge)
  4   3DS Flag (0=NDS/DSi, 1=3DS)
  5   Zero   ... set in ... DSi-exclusive games?
  6   DSi flag (0=NDS/3DS, 1=DSi)
  7   Cart Protocol Variant (0=older/smaller carts, 1=newer/bigger carts)

Existing/known ROM IDs are:
  C2h,07h,00h,00h NDS Macronix 8MB ROM  (eg. DS Vision)
  AEh,0Fh,00h,00h NDS Noname   16MB ROM (eg. Meine Tierarztpraxis)
  C2h,0Fh,00h,00h NDS Macronix 16MB ROM (eg. Metroid Demo)
  C2h,1Fh,00h,00h NDS Macronix 32MB ROM (eg. Over the Hedge)
  C2h,1Fh,00h,40h DSi Macronix 32MB ROM (eg. Art Academy, TWL-VAAV, SystemFlaw)
  80h,3Fh,01h,E0h ?            64MB ROM+Infrared (eg. Walk with Me, NTR-IMWP)
  AEh,3Fh,00h,E0h DSi Noname   64MB ROM (eg. de Blob 2, TWL-VD2V)
  C2h,3Fh,00h,00h NDS Macronix 64MB ROM (eg. Ultimate Spiderman)
  C2h,3Fh,00h,40h DSi Macronix 64MB ROM (eg. Crime Lab, NTR-VAOP)
  80h,7Fh,00h,80h NDS SanDisk  128MB ROM (DS Zelda, NTR-AZEP-0)
  80h,7Fh,01h,E0h ?            128MB ROM+Infrared? (P-letter Soul Silver, IPGE)
  C2h,7Fh,00h,80h NDS Macronix 128MB ROM (eg. Spirit Tracks, NTR-BKIP)
  C2h,7Fh,00h,C0h DSi Macronix 128MB ROM (eg. Cooking Coach/TWL-VCKE)
  ECh,7Fh,00h,88h NDS Samsung  128MB NAND (eg. Warioware D.I.Y.)
  ECh,7Fh,01h,88h NDS Samsung? 128MB NAND+What? (eg. Jam with the Band, UXBP)
  ECh,7Fh,00h,E8h DSi Samsung? 128MB NAND (eg. Face Training, USKV)
  80h,FFh,80h,E0h NDS          256MB ROM (Kingdom Hearts - Re-Coded, NTR-BK9P)
  C2h,FFh,01h,C0h DSi Macronix 256MB ROM+Infrared? (eg. P-Letter White)
  C2h,FFh,00h,80h NDS Macronix 256MB ROM (eg. Band Hero, NTR-BGHP)
  C2h,FEh,01h,C0h DSi Macronix 512MB ROM+Infrared? (eg. P-Letter White 2)
  C2h,FEh,00h,90h 3DS Macronix probably 512MB? ROM (eg. Sims 3)
  45h,FAh,00h,90h 3DS SunDisk? maybe... 1.5GB? ROM (eg. Starfox)
  C2h,F8h,00h,90h 3DS Macronix maybe... 2GB?   ROM (eg. Kid Icarus)
  C2h,7Fh,00h,90h 3DS Macronix 128MB ROM CTR-P-AENJ MMinna no Ennichi
  C2h,FFh,00h,90h 3DS Macronix 256MB ROM CTR-P-AFSJ Pro Yakyuu Famista 2011
  C2h,FEh,00h,90h 3DS Macronix 512MB ROM CTR-P-AFAJ Real 3D Bass FishingFishOn
  C2h,FAh,00h,90h 3DS Macronix 1GB ROM CTR-P-ASUJ Hana to Ikimono Rittai Zukan
  C2h,FAh,02h,90h 3DS Macronix 1GB ROM CTR-P-AGGW Luigis Mansion 2 ASiA CHT
  C2h,F8h,00h,90h 3DS Macronix 2GB ROM CTR-P-ACFJ Castlevania - Lords of Shadow
  C2h,F8h,02h,90h 3DS Macronix 2GB ROM CTR-P-AH4J Monster Hunter 4
  AEh,FAh,00h,90h 3DS          1GB ROM CTR-P-AGKJ Gyakuten Saiban 5
  AEh,FAh,00h,98h 3DS          1GB NAND CTR-P-EGDJ Tobidase Doubutsu no Mori
  45h,FAh,00h,90h 3DS          1GB ROM CTR-P-AFLJ Fantasy Life
  45h,F8h,00h,90h 3DS          2GB ROM CTR-P-AVHJ Senran Kagura Burst - Guren
  C2h,F0h,00h,90h 3DS Macronix 4GB ROM CTR-P-ABRJ Biohazard Revelations
  FFh,FFh,FFh,FFh None (no cartridge inserted)
*/
u32 cardId(void) {
	#ifdef DEBUG	
	dbg_printf("\ncardId\n");
	#endif
    
    u32 cardid = getChipId(ndsHeader, moduleParams);

    //if (!cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0xE080FF80; // golden sun
    //if (!cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0x80FF80E0; // golden sun
    //if (cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0xFF000000; // golden sun
    //if (cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0x000000FF; // golden sun

    #ifdef DEBUG
    dbg_hexa(cardid);
    #endif
    
	return cardid;
}

bool cardRead(u32 dma, u32 src, void *dst, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 cardRead\n");	
	
	dbg_printf("\ndma : \n");
	dbg_hexa(dma);		
	dbg_printf("\nsrc : \n");
	dbg_hexa(src);		
	dbg_printf("\ndst : \n");
	dbg_hexa((u32)dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	
	
	return false;
}
#endif
