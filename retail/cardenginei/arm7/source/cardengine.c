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
#include "unpatched_funcs.h"
#include "debug_file.h"
#include "cardengine.h"
#include "nds_header.h"
#include "igm_text.h"

#ifndef TWLSDK
// Patcher
#include "common.h"
#include "decompress.h"
#include "patch.h"
#include "find.h"
#include "hook.h"
#endif

// TWL soft-reset
#include "sr_data_error.h"      // For showing an error screen
#include "sr_data_srloader.h"   // For rebooting into TWiLight Menu++ or the game

#define gameOnFlashcard BIT(0)
#define saveOnFlashcard BIT(1)
#define ROMinRAM BIT(3)
#define dsiMode BIT(4)
#define b_dsiSD BIT(5)
#define preciseVolumeControl BIT(6)
#define powerCodeOnVBlank BIT(7)
#define b_runCardEngineCheck BIT(8)
#define igmAccessible BIT(9)
#define hiyaCfwFound BIT(10)
#define slowSoftReset BIT(11)
#define wideCheatUsed BIT(12)
#define isSdk5 BIT(13)
//#define asyncCardRead BIT(14)
#define twlTouch BIT(15)
#define cloneboot BIT(16)
#define sleepMode BIT(17)
#define dsiBios BIT(18)
#define bootstrapOnFlashcard BIT(19)
#define ndmaDisabled BIT(20)
#define isDlp BIT(21)
#define i2cBricked BIT(30)
#define scfgLocked BIT(31)

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)
#define	REG_WIFIIRQ	(*(vuint16*)0x04808012)

extern u32 ce7;

static const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char bootNdsPath[14] = {'s','d','m','c',':','/','b','o','o','t','.','n','d','s'};
static char hiyaDSiPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

extern void ndsCodeStart(u32* addr);
extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

extern vu32* volatile cardStruct;
extern u32 cheatEngineAddr;
extern u32 fileCluster;
extern u32 saveCluster;
extern u32 saveSize;
extern u32 patchOffsetCacheFileCluster;
extern u32 srParamsCluster;
extern u32 ramDumpCluster;
extern u32 screenshotCluster;
extern u32 pageFileCluster;
extern u32 manualCluster;
extern module_params_t* moduleParams;
extern u32 valueBits;
extern s32 mainScreen;
extern u32* languageAddr;
extern u8 language;
extern u8 consoleModel;
extern u8 romRead_LED;
extern u8 dmaRomRead_LED;
extern u16 igmHotkey;

#ifdef TWLSDK
vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;
#else
vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;
#endif

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION;

static bool initialized = false;
static bool driveInited = false;
#ifdef TWLSDK
static bool sixInHeader = false;
#endif
static bool bootloaderCleared = false;
static bool funcsUnpatched = false;
//static bool initializedIRQ = false;
//static bool calledViaIPC = false;
//static bool ipcSyncHooked = false;
bool ipcEveryFrame = false;
//static bool dmaLed = false;
static bool powerLedChecked = false;
static bool powerLedIsPurple = false;
static bool swapScreens = false;
static bool dmaSignal = false;
static bool wifiIrq = false;
static int wifiIrqTimer = 0;
//static bool saveInRam = false;

#ifdef TWLSDK
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_TWLSDK;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION_TWLSDK;
//static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION_TWLSDK;
static aFile* apFixOverlaysFile = (aFile*)OVL_FILE_LOCATION_TWLSDK;
#else
#ifdef ALTERNATIVE
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_ALT;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION_ALT;
//static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION_ALT;
static aFile* apFixOverlaysFile = (aFile*)OVL_FILE_LOCATION_ALT;
#else
static aFile* romFile = (aFile*)ROM_FILE_LOCATION;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION;
//static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION;
static aFile* apFixOverlaysFile = (aFile*)OVL_FILE_LOCATION;
#endif
#endif
static aFile patchOffsetCacheFile;
static aFile ramDumpFile;
static aFile srParamsFile;
static aFile screenshotFile;
static aFile pageFile;
static aFile manualFile;

static int saveTimer = 0;

static int languageTimer = 0;
// static int swapTimer = 0;
static int returnTimer = 0;
static int softResetTimer = 0;
// static int ramDumpTimer = 0;
static int noI2CVolLevel = 127; // Volume workaround for bricked I2C chips
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;

//static bool ndmaUsed = false;

// static int cardEgnineCommandMutex = 0;
static int saveMutex = 0;

bool returnToMenu = false;

#ifdef TWLSDK
static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
static PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER_SDK5-0x180);
#else
static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
static PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER-0x180);
#endif

extern u32 romLocation;

extern u32 romMapLines;
// 0: ROM part start, 1: ROM part start in RAM, 2: ROM part end in RAM
extern u32 romMap[5][3];

u32 currentSrlAddr = 0;

void i2cIRQHandler(void);

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
			#ifdef TWLSDK
			*(u8*)(0x02000838+i2) = *(u8*)(ce7+0x8400+i);		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			#else
			*(u8*)(0x02000838+i2) = *(u8*)(ce7+0x11800+i);	// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			#endif
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
	#ifdef TWLSDK
	*(u32*)(0x02000310) = *(u32*)(ce7+0x8500);
	*(u32*)(0x02000314) = *(u32*)(ce7+0x8504);
	*(u32*)(0x02000318) = /* *(u32*)(ce7+0x8504) == 0x00030000 ? 0x13 : */ 0x17;
	#else
	*(u32*)(0x02000310) = *(u32*)(ce7+0x11900);
	*(u32*)(0x02000314) = *(u32*)(ce7+0x11904);
	*(u32*)(0x02000318) = /* *(u32*)(ce7+0x11904) == 0x00030000 ? 0x13 : */ 0x17;
	#endif
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
	return (*(vu32*)(0x400481C) & BIT(3));
}

static void driveInitialize(void) {
	if (driveInited) {
		return;
	}

	if (valueBits & b_dsiSD) {
		if (sdmmc_read16(REG_SDSTATUS0) != 0) {
			sdmmc_init();
			SD_Init();
		}
		FAT_InitFiles(false, false);
	}
	if (((valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM)) || (valueBits & saveOnFlashcard)) {
		FAT_InitFiles(false, true);
	}

	getFileFromCluster(&patchOffsetCacheFile, patchOffsetCacheFileCluster, (valueBits & gameOnFlashcard));
	getFileFromCluster(&ramDumpFile, ramDumpCluster, (valueBits & bootstrapOnFlashcard));
	getFileFromCluster(&srParamsFile, srParamsCluster, (valueBits & gameOnFlashcard));
	getFileFromCluster(&screenshotFile, screenshotCluster, (valueBits & bootstrapOnFlashcard));
	getFileFromCluster(&pageFile, pageFileCluster, (valueBits & bootstrapOnFlashcard));
	getFileFromCluster(&manualFile, manualCluster, (valueBits & bootstrapOnFlashcard));

	//romFile = getFileFromCluster(fileCluster);
	//buildFatTableCache(&romFile, 0);
	#ifdef DEBUG	
	if (romFile->fatTableCached) {
		nocashMessage("fat table cached");
	} else {
		nocashMessage("fat table not cached"); 
	}
	#endif
	
	/*if (saveCluster > 0) {
		savFile = getFileFromCluster(saveCluster);
	} else {
		savFile.firstCluster = CLUSTER_FREE;
	}*/

	#ifdef DEBUG		
	aFile myDebugFile;
	getBootFileCluster(&myDebugFile, "NDSBTSRP.LOG", 0, !(valueBits & b_dsiSD));
	enableDebug(&myDebugFile);
	dbg_printf("logging initialized\n");		
	dbg_printf("sdk version :");
	dbg_hexa(moduleParams->sdk_version);		
	dbg_printf("\n");	
	dbg_printf("rom file :");
	dbg_hexa(fileCluster);	
	dbg_printf("\n");	
	dbg_printf("save file :");
	dbg_hexa(saveCluster);	
	dbg_printf("\n");
	#endif

	if (valueBits & ndmaDisabled) {
		sdmmc_lock_ndma_slot();
	}

	sdmmc_set_ndma_slot(0);
	driveInited = true;
}

static void initialize(void) {
	if (initialized) {
		return;
	}

	#ifdef TWLSDK
	if (*(u8*)(DSI_HEADER_SDK5+0x234) == 6) {
		*(u8*)(DSI_HEADER_SDK5+0x234) = 0;
		sixInHeader = true;
	}
	#else
	if (valueBits & isSdk5) {
		sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;
		ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER_SDK5-0x180);
	} else {
		sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;
		ndsHeader = (tNDSHeader*)NDS_HEADER;
		personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER-0x180);
	}
	#endif

	if (language >= 0 && language <= 7) {
		// Change language
		personalData->language = language;
	}

	if (!bootloaderCleared) {
		toncset((u8*)0x06000000, 0, 0x40000);	// Clear bootloader
		if (mainScreen) {
			swapScreens = (mainScreen == 2);
			ipcEveryFrame = true;
		}
		bootloaderCleared = true;
	}

	initialized = true;
}

#ifdef TWLSDK
/*u32 auxIeBak = 0;
u32 sdStatBak = 0;
u32 sdMaskBak = 0;

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
}*/
#else
static module_params_t* getModuleParams(const tNDSHeader* ndsHeader) {
	//nocashMessage("Looking for moduleparams...\n");

	u32* moduleParamsOffset = findModuleParamsOffset(ndsHeader);

	//module_params_t* moduleParams = (module_params_t*)((u32)moduleParamsOffset - 0x1C);
	return moduleParamsOffset ? (module_params_t*)(moduleParamsOffset - 7) : NULL;
}
#endif

static void cardReadRAM(u8* dst, u32 src, u32 len/*, int romPartNo*/) {
	// Copy directly
	#ifdef TWLSDK
	u32 newSrc = romLocation/*[romPartNo]*/+src;
	if (src > *(u32*)0x02FFE1C0) {
		newSrc -= *(u32*)0x02FFE1CC;
	}
	tonccpy(dst, (u8*)newSrc, len);
	#else
	// tonccpy(dst, (u8*)romLocation/*[romPartNo]*/+src, len);
	u32 len2 = 0;
	for (int i = 0; i < romMapLines; i++) {
		if (!(src >= romMap[i][0] && (i == romMapLines-1 || src < romMap[i+1][0])))
			continue;

		u32 newSrc = (romMap[i][1]-romMap[i][0])+src;
		if (newSrc+len > romMap[i][2]) {
			do {
				len--;
				len2++;
			} while (newSrc+len != romMap[i][2]);
			tonccpy(dst, (u8*)newSrc, len);
			src += len;
			dst += len;
		} else {
			tonccpy(dst, (u8*)newSrc, len2==0 ? len : len2);
			break;
		}
	}
	#endif
}

void reset(void) {
	register int i, reg;

#ifndef TWLSDK
	u32 resetParam = ((valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	if (((valueBits & isDlp) && *(u32*)(NDS_HEADER_SDK5+0xC) == 0) || (valueBits & slowSoftReset) || (*(u32*)(resetParam+0xC) > 0 && (*(u32*)CARDENGINEI_ARM9_LOCATION == 0 || (valueBits & isSdk5)))) {
		REG_MASTER_VOLUME = 0;
		int oldIME = enterCriticalSection();
		//driveInitialize();
		if (*(u32*)(resetParam+8) == 0x44414F4C) { // 'LOAD'
			fileWrite((char*)ndsHeader, &pageFile, 0x2BFE00, 0x160);
			fileWrite((char*)ndsHeader->arm9destination, &pageFile, 0x14000, ndsHeader->arm9binarySize);
			fileWrite((char*)0x022C0000, &pageFile, 0x2C0000, ndsHeader->arm7binarySize);
		}
		fileWrite((char*)resetParam, &srParamsFile, 0, 0x10);
		toncset((u32*)0x02000000, 0, 0x400);
		*(u32*)0x02000000 = BIT(3);
		*(u32*)0x02000004 = 0x54455352; // 'RSET'
		if (consoleModel < 2) {
			(*(u32*)(ce7+0x11900) == 0 && (valueBits & b_dsiSD)) ? unlaunchSetFilename(false) : unlaunchSetHiyaFilename();
		}
		if (*(u32*)(ce7+0x11900) == 0 && (valueBits & b_dsiSD)) {
			tonccpy((u32*)0x02000300, sr_data_srloader, 0x20);
		} else {
			// Use different SR backend ID
			readSrBackendId();
		}
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot game
		leaveCriticalSection(oldIME);
		while (1);
	}

	if (!(valueBits & isDlp)) {
#endif

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
	*(vu32*)0x0380FFFC = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)0x0380FFF8 = 0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff

	funcsUnpatched = false;

#ifndef TWLSDK
	}
#endif

	initialized = false;
	//ipcSyncHooked = false;
	languageTimer = 0;

	#ifndef TWLSDK
	if ((valueBits & isDlp) || currentSrlAddr != *(u32*)(resetParam+0xC) || *(u32*)(resetParam+8) == 0x44414F4C) {
		currentSrlAddr = *(u32*)(resetParam+0xC);
		if (valueBits & isDlp) {
			ndmaCopyWordsAsynch(1, (u32*)0x022C0000, ndsHeader->arm7destination, ndsHeader->arm7binarySize);
			*(u16*)0x02fffc40 = 2; // Boot Indicator (Cloneboot/Multiboot)
		} else {
			if (*(u32*)(resetParam+8) == 0x44414F4C) {
				ndmaCopyWordsAsynch(1, (u32*)0x022C0000, ndsHeader->arm7destination, ndsHeader->arm7binarySize);
				*((u16*)(/*isSdk5(moduleParams) ? 0x02fffc40 :*/ 0x027ffc40)) = 2; // Boot Indicator (Cloneboot/Multiboot)
				// tonccpy((u32*)0x027FFC40, (u32*)0x02344820, 0x40); // Multiboot info?
			} else if (valueBits & ROMinRAM) {
				cardReadRAM((u8*)ndsHeader, currentSrlAddr, 0x160);
				cardReadRAM((u8*)ndsHeader->arm9destination, currentSrlAddr+ndsHeader->arm9romOffset, ndsHeader->arm9binarySize);
				cardReadRAM((u8*)ndsHeader->arm7destination, currentSrlAddr+ndsHeader->arm7romOffset, ndsHeader->arm7binarySize);
			} else {
				fileRead((char*)ndsHeader, romFile, currentSrlAddr, 0x160);
				fileRead((char*)ndsHeader->arm9destination, romFile, currentSrlAddr+ndsHeader->arm9romOffset, ndsHeader->arm9binarySize);
				fileRead((char*)ndsHeader->arm7destination, romFile, currentSrlAddr+ndsHeader->arm7romOffset, ndsHeader->arm7binarySize);
			}
		}

		moduleParams = getModuleParams(ndsHeader);
		/*dbg_printf("sdk_version: ");
		dbg_hexa(moduleParams->sdk_version);
		dbg_printf("\n");*/ 
		if (moduleParams->sdk_version > 0x5000000) {
			valueBits |= isSdk5;
		} else {
			valueBits &= ~isSdk5;
		}

		ensureBinaryDecompressed(ndsHeader, moduleParams, (valueBits & isDlp) ? 0x44414F4 : resetParam);

		patchCardNdsArm9(
			(cardengineArm9*)((valueBits & isDlp) ? CARDENGINEI_ARM9_LOCATION_DLP : CARDENGINEI_ARM9_LOCATION),
			ndsHeader,
			moduleParams,
			1
		);
		while (ndmaBusy(1));
		patchCardNdsArm7(
			(cardengineArm7*)ce7,
			ndsHeader,
			moduleParams
		);

		hookNdsRetailArm7(
			(cardengineArm7*)ce7,
			ndsHeader
		);
		hookNdsRetailArm9(ndsHeader);

		extern u32 iUncompressedSize;

		fileWrite((char*)ndsHeader->arm9destination, &pageFile, 0x14000, iUncompressedSize);
		fileWrite((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, ndsHeader->arm7binarySize);
		fileWrite((char*)&iUncompressedSize, &pageFile, 0x5FFFF0, sizeof(u32));
		fileWrite((char*)&ndsHeader->arm7binarySize, &pageFile, 0x5FFFF4, sizeof(u32));
		/* } else {
			*(u32*)ARM9_DEC_SIZE_LOCATION = iUncompressedSize;
			ndmaCopyWordsAsynch(0, ndsHeader->arm9destination, (char*)ndsHeader->arm9destination+0x400000, *(u32*)ARM9_DEC_SIZE_LOCATION);
			ndmaCopyWordsAsynch(1, ndsHeader->arm7destination, (char*)DONOR_ROM_ARM7_LOCATION, ndsHeader->arm7binarySize);
			while (ndmaBusy(0) || ndmaBusy(1));
		} */
		if (valueBits & isDlp) {
			toncset((u32*)0x022C0000, 0, ndsHeader->arm7binarySize);
			if (!(valueBits & isSdk5)) {
				tonccpy((u8*)0x027FF000, (u8*)0x02FFF000, 0x1000);
			}
		} else {
			*(u32*)(resetParam+8) = 0;
		}
		valueBits &= ~isDlp;
	} else {
		//driveInitialize();

		u32 iUncompressedSize = 0;
		u32 newArm7binarySize = 0;
		fileRead((char*)&iUncompressedSize, &pageFile, 0x5FFFF0, sizeof(u32));
		fileRead((char*)&newArm7binarySize, &pageFile, 0x5FFFF4, sizeof(u32));
		fileRead((char*)ndsHeader->arm9destination, &pageFile, 0x14000, iUncompressedSize);
		fileRead((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, newArm7binarySize);
	} /* else {
		ndmaCopyWordsAsynch(0, (char*)ndsHeader->arm9destination+0x400000, ndsHeader->arm9destination, *(u32*)ARM9_DEC_SIZE_LOCATION);
		ndmaCopyWordsAsynch(1, (char*)DONOR_ROM_ARM7_LOCATION, ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		while (ndmaBusy(0) || ndmaBusy(1));
	} */
	#else
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();

	//driveInitialize();

	u32 iUncompressedSize = 0;
	u32 iUncompressedSizei = 0;
	u32 newArm7binarySize = 0;
	u32 newArm7ibinarySize = 0;

	fileRead((char*)&iUncompressedSize, &pageFile, 0x5FFFF0, sizeof(u32));
	fileRead((char*)&newArm7binarySize, &pageFile, 0x5FFFF4, sizeof(u32));
	fileRead((char*)&iUncompressedSizei, &pageFile, 0x5FFFF8, sizeof(u32));
	fileRead((char*)&newArm7ibinarySize, &pageFile, 0x5FFFFC, sizeof(u32));
	fileRead((char*)ndsHeader->arm9destination, &pageFile, 0x14000, iUncompressedSize);
	fileRead((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, newArm7binarySize);
	fileRead((char*)(*(u32*)0x02FFE1C8), &pageFile, 0x300000, iUncompressedSizei);
	fileRead((char*)(*(u32*)0x02FFE1D8), &pageFile, 0x580000, newArm7ibinarySize);

	if (sixInHeader) {
		*(u8*)(DSI_HEADER_SDK5+0x234) = 6;
	}
	//if (doBak) restoreSdBakData();
	#endif
	toncset((char*)((valueBits & isSdk5) ? 0x02FFFD80 : 0x027FFD80), 0, 0x80);
	toncset((char*)((valueBits & isSdk5) ? 0x02FFFF80 : 0x027FFF80), 0, 0x80);

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

static void cardReadLED(const bool on, const bool dmaLed) {
	if (!(valueBits & i2cBricked) && consoleModel < 2) { /* Proceed below */ } else { return; }

	/* static bool ledIsOn = false;
	static bool dmaLedIsOn = false;
	if (dmaLed ? dmaLedIsOn == on : ledIsOn == on) {
		return;
	}
	dmaLed
		? (dmaLedIsOn = on)
		: (ledIsOn = on); */

	if (dmaRomRead_LED == -1) dmaRomRead_LED = romRead_LED;
	if (!powerLedChecked && (romRead_LED || dmaRomRead_LED)) {
		const u8 byte = i2cReadRegister(0x4A, 0x63);
		powerLedIsPurple = (byte == 0xFF);
		powerLedChecked = true;
	}
	if (on) {
		switch(dmaLed ? dmaRomRead_LED : romRead_LED) {
			case 0:
			default:
				break;
			case 1:
				i2cWriteRegister(0x4A, 0x30, 0x13);    // Turn WiFi LED on
				break;
			case 2:
				i2cWriteRegister(0x4A, 0x63, powerLedIsPurple ? 0x00 : 0xFF);    // Turn power LED purple
				break;
			case 3:
				i2cWriteRegister(0x4A, 0x31, 0x01);    // Turn Camera LED on
				break;
		}
	} else {
		switch(dmaLed ? dmaRomRead_LED : romRead_LED) {
			case 0:
			default:
				break;
			case 1:
				i2cWriteRegister(0x4A, 0x30, 0x12);    // Turn WiFi LED off
				break;
			case 2:
				i2cWriteRegister(0x4A, 0x63, powerLedIsPurple ? 0xFF : 0x00);    // Revert power LED to normal
				break;
			case 3:
				i2cWriteRegister(0x4A, 0x31, 0x00);    // Turn Camera LED off
				break;
		}
	}
}

/*static void asyncCardReadLED(bool on) {
	if (consoleModel < 2) {
		if (on) {
			switch(romread_LED) {
				case 0:
				default:
					break;
				case 1:
					i2cWriteRegister(0x4A, 0x63, 0xFF);    // Turn power LED purple
					break;
				case 2:
					i2cWriteRegister(0x4A, 0x30, 0x13);    // Turn WiFi LED on
					break;
			}
		} else {
			switch(romread_LED) {
				case 0:
				default:
					break;
				case 1:
					i2cWriteRegister(0x4A, 0x63, 0x00);    // Revert power LED to normal
					break;
				case 2:
					i2cWriteRegister(0x4A, 0x30, 0x12);    // Turn WiFi LED off
					break;
			}
		}
	}
}*/

extern void inGameMenu(void);

static inline void rebootConsole(void) {
	if (valueBits & i2cBricked) {
		u8 readCommand = readPowerManagement(0x10);
		readCommand |= BIT(0);
		writePowerManagement(0x10, readCommand); // Reboot console
		return;
	}
	i2cWriteRegister(0x4A, 0x70, 0x01);
	i2cWriteRegister(0x4A, 0x11, 0x01);
}

void forceGameReboot(void) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)0x02000000 = BIT(3);
	*(u32*)0x02000004 = 0x54455352; // 'RSET'
	sharedAddr[4] = 0x57534352;
	IPC_SendSync(0x8);
	if (consoleModel < 2) {
		if (valueBits & b_dsiSD) {
			#ifdef TWLSDK
			(*(u32*)(ce7+0x8500) == 0) ? unlaunchSetFilename(false) : unlaunchSetHiyaFilename();
			#else
			(*(u32*)(ce7+0x11900) == 0) ? unlaunchSetFilename(false) : unlaunchSetHiyaFilename();
			#endif
		}
		waitFrames(5);							// Wait for DSi screens to stabilize
	}
	u32 clearBuffer = 0;
	#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
	#endif
	//driveInitialize();
	fileWrite((char*)&clearBuffer, &srParamsFile, 0, 0x4);
  	#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
	if (*(u32*)(ce7+0x8500) == 0 && (valueBits & b_dsiSD))
	#else
	if (*(u32*)(ce7+0x11900) == 0 && (valueBits & b_dsiSD))
	#endif
	{
		tonccpy((u32*)0x02000300, sr_data_srloader, 0x20);
	} else {
		// Use different SR backend ID
		readSrBackendId();
	}
	rebootConsole();		// Force-reboot game
}

#ifdef TWLSDK
/* static void initMBK_dsiMode(void) {
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
} */

extern bool dldiPatchBinary (unsigned char *binData, u32 binSize);
#endif

void returnToLoader(bool reboot) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)0x02000000 = BIT(0) | BIT(1) | BIT(2);
	*(u32*)0x02000004 = 0x54455352; // 'RSET'
	sharedAddr[4] = 0x57534352;
#ifdef TWLSDK
	u32 twlCfgLoc = *(u32*)0x02FFFDFC;
	if (twlCfgLoc != 0x02000400) {
		tonccpy((u8*)0x02000400, (u8*)twlCfgLoc, 0x128);
	}

	if (reboot || !(valueBits & dsiBios) || ((valueBits & twlTouch) && !(*(u8*)0x02FFE1BF & BIT(0))) || ((valueBits & b_dsiSD) && (valueBits & wideCheatUsed))) {
		if (consoleModel >= 2) {
			if (*(u32*)(ce7+0x8500) == 0) {
				tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
			} else if (*(char*)(ce7+0x8503) == 'H' || *(char*)(ce7+0x8503) == 'K') {
				// Use different SR backend ID
				readSrBackendId();
			}
			waitFrames(1);
		} else {
			if (*(u32*)(ce7+0x8500) == 0) {
				unlaunchSetFilename(true);
			} else {
				// Use different SR backend ID
				readSrBackendId();
			}
			waitFrames(5);							// Wait for DSi screens to stabilize
		}
		rebootConsole();
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

	//driveInitialize();

	aFile file;
	getBootFileCluster(&file, "BOOT.NDS", !(valueBits & b_dsiSD));
	if (file.firstCluster == CLUSTER_FREE) {
		// File not found, so reboot console instead
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);
	}

	fileRead((char*)__DSiHeader, &file, 0, sizeof(tDSiHeader));
	*ndsHeader = __DSiHeader->ndshdr;

	fileRead(__DSiHeader->ndshdr.arm9destination, &file, (u32)__DSiHeader->ndshdr.arm9romOffset, __DSiHeader->ndshdr.arm9binarySize);
	fileRead(__DSiHeader->ndshdr.arm7destination, &file, (u32)__DSiHeader->ndshdr.arm7romOffset, __DSiHeader->ndshdr.arm7binarySize);
	if (ndsHeader->unitCode > 0) {
		fileRead(__DSiHeader->arm9idestination, &file, (u32)__DSiHeader->arm9iromOffset, __DSiHeader->arm9ibinarySize);
		fileRead(__DSiHeader->arm7idestination, &file, (u32)__DSiHeader->arm7iromOffset, __DSiHeader->arm7ibinarySize);

		// Disabled due to ce7 code taking place in DSi WRAM
		// initMBK_dsiMode();
	}

	if (!(valueBits & b_dsiSD)) {
		dldiPatchBinary(ndsHeader->arm9destination, ndsHeader->arm9binarySize);
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
#else
	IPC_SendSync(0x8);
	if (consoleModel >= 2) {
		if (*(u32*)(ce7+0x11900) == 0 && (valueBits & b_dsiSD))
		{
			tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
		}
		else if (*(char*)(ce7+0x11903) == 'H' || *(char*)(ce7+0x11903) == 'K')
		{
			// Use different SR backend ID
			readSrBackendId();
		}
		waitFrames(1);
	} else {
		if (*(u32*)(ce7+0x11900) == 0 && (valueBits & b_dsiSD))
		{
			unlaunchSetFilename(true);
		} else {
			// Use different SR backend ID
			readSrBackendId();
		}
		waitFrames(5);							// Wait for DSi screens to stabilize
	}

	rebootConsole();		// Reboot into TWiLight Menu++
#endif
}

void dumpRam(void) {
	#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
	#endif
	//driveInitialize();
	sharedAddr[3] = 0x444D4152;
	// Dump RAM
	if (valueBits & dsiMode) {
		// Dump full RAM
		fileWrite((char*)0x0C000000, &ramDumpFile, 0, (consoleModel==0 ? 0x01000000 : 0x02000000));
	} else if (valueBits & isSdk5) {
		// Dump RAM used in DS mode (SDK5)
		fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x3E0000);
		fileWrite((char*)(ndsHeader->unitCode==2 ? 0x02FE0000 : 0x027E0000), &ramDumpFile, 0x3E0000, 0x1F000);
		fileWrite((char*)0x02FFF000, &ramDumpFile, 0x3FF000, 0x1000);
	} else if (moduleParams->sdk_version >= 0x2008000) {
		// Dump RAM used in DS mode (SDK2.1+)
		fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x3E0000);
		fileWrite((char*)0x027E0000, &ramDumpFile, 0x3E0000, 0x20000);
	} else {
		// Dump RAM used in DS mode (SDK2.0)
		fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x3C0000);
		fileWrite((char*)0x027C0000, &ramDumpFile, 0x3C0000, 0x40000);
	}
	sharedAddr[3] = 0;
  	#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
	#endif
}

void prepareScreenshot(void) {
#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
#endif
		//driveInitialize();
		fileWrite((char*)INGAME_MENU_EXT_LOCATION, &pageFile, 0x540000, 0x40000);
#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
#endif
}

void saveScreenshot(void) {
	if (igmText->currentScreenshot >= 50) return;

#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
#endif
	//driveInitialize();
	fileWrite((char*)INGAME_MENU_EXT_LOCATION, &screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 0x18046);

	// Skip until next blank slot
	char magic;
	do {
		igmText->currentScreenshot++;
		fileRead(&magic, &screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 1);
	} while(magic == 'B' && igmText->currentScreenshot < 50);

		fileRead((char*)INGAME_MENU_EXT_LOCATION, &pageFile, 0x540000, 0x40000);
#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
#endif
}

void prepareManual(void) {
#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
#endif
		//driveInitialize();
		fileWrite((char*)INGAME_MENU_EXT_LOCATION, &pageFile, 0x540000, 32 * 24);
#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
#endif
}

void readManual(int line) {
	static int currentManualLine = 0;
	static int currentManualOffset = 0;
	char buffer[32];

	// Seek for desired line
	bool firstLoop = true;
	while(currentManualLine != line) {
		if(line > currentManualLine) {
			fileRead(buffer, &manualFile, currentManualOffset, 32);

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
			fileRead(buffer, &manualFile, currentManualOffset, 32);
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
		fileRead(buffer, &manualFile, tempManualOffset, 32);

		// Fix for exactly 32 char lines
		if(fullLine && buffer[0] == '\n')
			fileRead(buffer, &manualFile, ++tempManualOffset, 32);

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
#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
#endif
		//driveInitialize();
		fileRead((char*)INGAME_MENU_EXT_LOCATION, &pageFile, 0x540000, 32 * 24);
#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
#endif
}

void saveMainScreenSetting(void) {
	fileWrite((char*)sharedAddr, &patchOffsetCacheFile, 0x1FC, sizeof(u32));
}

void loadInGameMenu(void) {
	const u32 igmLocation = INGAME_MENU_LOCATION;

	sharedAddr[5] = 0x4C4D4749; // 'IGML'
	fileWrite((char*)igmLocation, &pageFile, 0xA000, 0xA000);	// Backup part of game RAM to page file
	fileRead((char*)igmLocation, &pageFile, 0, 0xA000);	// Read in-game menu
	sharedAddr[5] = 0;
}

void unloadInGameMenu(void) {
	while (REG_VCOUNT != 191) swiDelay(100);
	while (REG_VCOUNT == 191) swiDelay(100);

	const u32 igmLocation = INGAME_MENU_LOCATION;

	sharedAddr[5] = 0x4C4D4749; // 'IGML'
	fileWrite((char*)igmLocation, &pageFile, 0, 0xA000);	// Store in-game menu
	fileRead((char*)igmLocation, &pageFile, 0xA000, 0xA000);	// Restore part of game RAM from page file
	sharedAddr[5] = 0;
}

#ifdef DEBUG
static void log_arm9(void) {
	//driveInitialize();
	u32 src = *(vu32*)(sharedAddr+2);
	u32 dst = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr+1);
	u32 marker = *(vu32*)(sharedAddr+3);

	dbg_printf("\ncard read received\n");

	if (calledViaIPC) {
		dbg_printf("\ntriggered via IPC\n");
	}
	dbg_printf("\nstr : \n");
	dbg_hexa((u32)cardStruct);
	dbg_printf("\nsrc : \n");
	dbg_hexa(src);
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	dbg_printf("\nmarker : \n");
	dbg_hexa(marker);

	dbg_printf("\nlog only \n");
}
#endif

static void nandRead(void) {
	u32 flash = *(vu32*)(sharedAddr+2);
	u32 memory = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr+1);
	#ifdef DEBUG
	u32 marker = *(vu32*)(sharedAddr+3);

	dbg_printf("\nnand read received\n");

	if (calledViaIPC) {
		dbg_printf("\ntriggered via IPC\n");
	}
	dbg_printf("\nflash : \n");
	dbg_hexa(flash);
	dbg_printf("\nmemory : \n");
	dbg_hexa(memory);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	dbg_printf("\nmarker : \n");
	dbg_hexa(marker);
	#endif

	//driveInitialize();
	//cardReadLED(true, true);    // When a file is loading, turn on LED for card read indicator
	sdmmc_set_ndma_slot(4);
	fileRead((char *)memory, savFile, flash, len);
	sdmmc_set_ndma_slot(0);
	//cardReadLED(false, true);
}

static void nandWrite(void) {
	u32 flash = *(vu32*)(sharedAddr+2);
	u32 memory = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr+1);
	#ifdef DEBUG
	u32 marker = *(vu32*)(sharedAddr+3);

	dbg_printf("\nnand write received\n");

	if (calledViaIPC) {
		dbg_printf("\ntriggered via IPC\n");
	}
	dbg_printf("\nflash : \n");
	dbg_hexa(flash);
	dbg_printf("\nmemory : \n");
	dbg_hexa(memory);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	dbg_printf("\nmarker : \n");
	dbg_hexa(marker);
	#endif

	//driveInitialize();
	saveTimer = 1;			// When we're saving, power button does nothing, in order to prevent corruption.
	//cardReadLED(true, true);    // When a file is loading, turn on LED for card read indicator
	sdmmc_set_ndma_slot(4);
	fileWrite((char *)memory, savFile, flash, len);
	sdmmc_set_ndma_slot(0);
	//cardReadLED(false, true);
}

/*static void slot2Read(void) {
	u32 src = *(vu32*)(sharedAddr+2);
	u32 dst = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr+1);
	#ifdef DEBUG
	u32 marker = *(vu32*)(sharedAddr+3);

	dbg_printf("\nslot2 read received\n");

	if (calledViaIPC) {
		dbg_printf("\ntriggered via IPC\n");
	}
	dbg_printf("\nsrc : \n");
	dbg_hexa(src);
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	dbg_printf("\nmarker : \n");
	dbg_hexa(marker);
	#endif

	cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
	fileRead((char*)dst, *gbaFile, src, len, -1);
	cardReadLED(false);
}*/

//static bool readOngoing = false;
static bool sdReadOngoing = false;
static bool ongoingIsDma = false;
//static int currentCmd=0, currentNdmaSlot=0;
//static int timeTillDmaLedOff = 0;

#ifndef TWLSDK
/* static bool start_cardRead_arm9(void) {
	u32 src = sharedAddr[2];
	u32 dst = sharedAddr[0];
	u32 len = sharedAddr[1];
	#ifdef DEBUG
	u32 marker = sharedAddr[3];

	dbg_printf("\ncard read received v2\n");

	if (calledViaIPC) {
		dbg_printf("\ntriggered via IPC\n");
	}

	dbg_printf("\nstr : \n");
	dbg_hexa((u32)cardStruct);
	dbg_printf("\nsrc : \n");
	dbg_hexa(src);
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	dbg_printf("\nmarker : \n");
	dbg_hexa(marker);	
	#endif

	//bool isDma = sharedAddr[3] == (0x0000000A & 0x0000000F);
	bool isDma = true;

	//driveInitialize();
	cardReadLED(true, isDma);    // When a file is loading, turn on LED for card read indicator
	#ifdef DEBUG
	nocashMessage("fileRead romFile");
	#endif
	if ((dst % 4) == 0) {
		if(!fileReadNonBLocking((char*)dst, romFile, src, len))
		{
			readOngoing = true;
			return false;
			//while(!resumeFileRead()){}
		} 
		else
		{
			readOngoing = false;
			cardReadLED(false, isDma);    // After loading is done, turn off LED for card read indicator
			return true;
		}
	} else {
		fileRead((char*)dst, romFile, src, len);
		cardReadLED(false, isDma);    // After loading is done, turn off LED for card read indicator
		return true;
	}

	#ifdef DEBUG
	dbg_printf("\nread \n");
	if (is_aligned(dst, 4) || is_aligned(len, 4)) {
		dbg_printf("\n aligned read : \n");
	} else {
		dbg_printf("\n misaligned read : \n");
	}
	#endif
}

static bool resume_cardRead_arm9(void) {
	//bool isDma = sharedAddr[3] == (0x0000000A & 0x0000000F);
	bool isDma = true;
    if(resumeFileRead())
    {
        readOngoing = false;
        cardReadLED(false, isDma);    // After loading is done, turn off LED for card read indicator
        return true;    
    } 
    else
    {
        return false;    
    }
} */

/* static bool gsddFix(void) {
	if (sharedAddr[4] != 0x44445347) {
        return false;
    }

	const u32 gsddOverlayChecksumOffset = *(u32*)0x02FFF004;
	// const u32 gsddOverlayFuncOffset = *(u32*)0x02FFF008;
	const u32 oldChecksum = 0x2FBB82E1;
	const u32 newChecksum = *(u32*)0x02FFF17C;

	if (*(u32*)gsddOverlayChecksumOffset == oldChecksum) {
		*(u32*)gsddOverlayChecksumOffset = newChecksum;
		// *(u32*)gsddOverlayFuncOffset = 0xE1A00000; // nop (Start the game past name setting)
	}

	sharedAddr[4] = 0;
	return true;
} */
#endif

static inline void sdmmcHandler(void) { // Unused
	if (sdReadOngoing) {
		if (my_sdmmc_sdcard_check_command(0x33C12)) {
			sharedAddr[4] = 0;
			cardReadLED(false, ongoingIsDma);
			sdReadOngoing = false;
		}
		return;
	}

	switch (sharedAddr[4]) {
		case 0x53445231:
		case 0x53444D31: {
		
			//#ifdef DEBUG		
			//dbg_printf("my_sdmmc_sdcard_readsector\n");
			//#endif
			// bool isDma = sharedAddr[4]==0x53444D31;
			// cardReadLED(true, isDma);
			ongoingIsDma = (sharedAddr[4] == 0x53444D31);
			cardReadLED(true, ongoingIsDma);
			sharedAddr[4] = my_sdmmc_sdcard_readsector(sharedAddr[0], (u8*)sharedAddr[1], sharedAddr[2], sharedAddr[3]);
			// cardReadLED(false, isDma);
		}	break;
		case 0x53445244:
		case 0x53444D41: {
		
			//#ifdef DEBUG		
			//dbg_printf("my_sdmmc_sdcard_readsectors\n");
			//#endif
			//bool isDma = sharedAddr[4]==0x53444D41;
			ongoingIsDma = (sharedAddr[4] == 0x53444D41);
			cardReadLED(true, ongoingIsDma);
			if ((sharedAddr[2] % 4) != 0 || (valueBits & ndmaDisabled)) {
				sharedAddr[4] = my_sdmmc_sdcard_readsectors(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2]);
				cardReadLED(false, ongoingIsDma);
			} else {
				my_sdmmc_sdcard_readsectors_nonblocking(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2]);
				sdReadOngoing = true;
			}
		}	break;
		/*case 0x53444348:
			sharedAddr[4] = my_sdmmc_sdcard_check_command(sharedAddr[0], sharedAddr[1]);
			//currentCmd = sharedAddr[0];
			//currentNdmaSlot = sharedAddr[1];
			break;
		case 0x53415244:
			cardReadLED(true, true);
			sharedAddr[4] = my_sdmmc_sdcard_readsectors_nonblocking(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2]);
			//currentCmd = sharedAddr[4];
			//currentNdmaSlot = sharedAddr[3];
			timeTillDmaLedOff = 0;
			readOngoing = true;
			break;*/
		/* case 0x53445752:
			cardReadLED(true, true);
			sharedAddr[4] = my_sdmmc_sdcard_writesectors(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2]);
			cardReadLED(false, true);
			break; */
	}
}

void runCardEngineCheck(void) {
	// if (!(valueBits & b_runCardEngineCheck)) return;

	//dbg_printf("runCardEngineCheck\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheck");
	#endif	

  	// if (lockMutex(&cardEgnineCommandMutex)) {
        //if(!readOngoing)
        //{
    
    		//nocashMessage("runCardEngineCheck mutex ok");
    
  		/*if (sharedAddr[3] == (vu32)0x5245424F) {
  			i2cWriteRegister(0x4A, 0x70, 0x01);
  			i2cWriteRegister(0x4A, 0x11, 0x01);
  		}*/

			if (!(valueBits & gameOnFlashcard)) {
				// #ifndef TWLSDK
				if (/* sharedAddr[3] == (vu32)0x020FF808 || sharedAddr[3] == (vu32)0x020FF80A || */ sharedAddr[3] == (vu32)0x025FFB08 || sharedAddr[3] == (vu32)0x025FFB0A) {	// ARM9 Card Read
					//if (!readOngoing ? start_cardRead_arm9() : resume_cardRead_arm9()) {
						bool useApFixOverlays = false;
						u32 src = sharedAddr[2];
						u32 dst = sharedAddr[0];
						u32 len = sharedAddr[1];
						if (src >= 0x80000000) {
							src -= 0x80000000;
							useApFixOverlays = true;
						}

						const bool isDma = (sharedAddr[3] == (vu32)0x025FFB0A);

						// readOngoing = true;
						cardReadLED(true, isDma);    // When a file is loading, turn on LED for card read indicator
						fileRead((char*)dst, useApFixOverlays ? apFixOverlaysFile : romFile, src, len);
						cardReadLED(false, isDma);    // After loading is done, turn off LED for card read indicator
						// readOngoing = false;
						sharedAddr[3] = 0;
					if (isDma) {
						IPC_SendSync(0x3);
					}
					//}
				} /*else if (sharedAddr[3] == (vu32)0x026FFB0A) {	// Card read DMA (Card data cache)
					ndmaCopyWords(0, (u8*)sharedAddr[2], (u8*)(sharedAddr[0] >= 0x03000000 ? 0 : sharedAddr[0]), sharedAddr[1]);
					sharedAddr[3] = 0;
					IPC_SendSync(0x3);
				}*/
				// #endif
			}

			#ifdef DEBUG
    		if (sharedAddr[3] == (vu32)0x026FF800) {
    			log_arm9();
    			sharedAddr[3] = 0;
                //IPC_SendSync(0x8);
    		} else
			#endif

			if (sharedAddr[3] == (vu32)0x025FFC01) {
				//dmaLed = (sharedAddr[3] == (vu32)0x025FFC01);
				nandRead();
    			sharedAddr[3] = 0;
			} else if (sharedAddr[3] == (vu32)0x025FFC02) {
				//dmaLed = (sharedAddr[3] == (vu32)0x025FFC02);
				nandWrite();
    			sharedAddr[3] = 0;
			}

            /*if (sharedAddr[3] == (vu32)0x025FBC01) {
                dmaLed = false;
    			slot2Read();
    			sharedAddr[3] = 0;
    			IPC_SendSync(0x8);
    		}*/
        //}
  		// unlockMutex(&cardEgnineCommandMutex);
  	// }
}

//---------------------------------------------------------------------------------
void myIrqHandlerFIFO(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerFIFO");
	#endif

	// calledViaIPC = true;

    if (IPC_GetSync() == 0x3) {
		/* #ifndef TWLSDK
		if (gsddFix()) {
			return;
		}
		#endif */
		swiDelay(100);
		IPC_SendSync(0x3);
		return;
	}

	// runCardEngineCheck();
	/* if (!(valueBits & gameOnFlashcard)) {
		sdmmcHandler();
		if (sdReadOngoing) {
			sharedAddr[5] = 0x474E4950; // 'PING'
		}
	} */
}


void myIrqHandlerVBlank(void) {
  while (1) {
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	if (valueBits & i2cBricked) {
		REG_MASTER_VOLUME = noI2CVolLevel;
	}

	#ifdef DEBUG
	nocashMessage("cheat_engine_start\n");
	#endif

	if (*(u32*)cheatEngineAddr == 0x3E4 && *(u32*)(cheatEngineAddr+0x3E8) != 0xCF000000) {
		volatile void (*cheatEngine)() = (volatile void*)cheatEngineAddr+4;
		(*cheatEngine)();
	}

	if (language >= 0 && language <= 7 && languageTimer < 60*3) {
		// Change language
		personalData->language = language;
		#ifndef TWLSDK
		if (languageAddr > 0) {
			// Extra measure for specific games
			*languageAddr = language;
		}
		#endif
		languageTimer++;
	}

	if (!funcsUnpatched && *(int*)((valueBits & isSdk5) ? 0x02FFFC3C : 0x027FFC3C) >= 60) {
		unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)((valueBits & isSdk5) ? UNPATCHED_FUNCTION_LOCATION_SDK5 : UNPATCHED_FUNCTION_LOCATION);

		if (unpatchedFuncs->compressed_static_end) {
			*unpatchedFuncs->compressedFlagOffset = unpatchedFuncs->compressed_static_end;
		}

		#ifdef TWLSDK
		if (unpatchedFuncs->ltd_compressed_static_end) {
			*unpatchedFuncs->iCompressedFlagOffset = unpatchedFuncs->ltd_compressed_static_end;
		}

		if (unpatchedFuncs->mpuInitOffset2) {
			*unpatchedFuncs->mpuInitOffset2 = 0xEE060F12;
		}
		if (unpatchedFuncs->mpuDataOffset2) {
			*unpatchedFuncs->mpuDataOffset2 = unpatchedFuncs->mpuInitRegionOldData2;
		}
		#else
		if (!(valueBits & isSdk5)) {
			if (unpatchedFuncs->mpuDataOffset) {
				*unpatchedFuncs->mpuDataOffset = unpatchedFuncs->mpuInitRegionOldData;

				if (unpatchedFuncs->mpuAccessOffset) {
					if (unpatchedFuncs->mpuOldInstrAccess) {
						unpatchedFuncs->mpuDataOffset[unpatchedFuncs->mpuAccessOffset] = unpatchedFuncs->mpuOldInstrAccess;
					}
					if (unpatchedFuncs->mpuOldDataAccess) {
						unpatchedFuncs->mpuDataOffset[unpatchedFuncs->mpuAccessOffset + 1] = unpatchedFuncs->mpuOldDataAccess;
					}
				}
			}

			if ((u32)unpatchedFuncs->mpuDataOffsetAlt >= (u32)ndsHeader->arm9destination && (u32)unpatchedFuncs->mpuDataOffsetAlt < (u32)ndsHeader->arm9destination+0x4000) {
				*unpatchedFuncs->mpuDataOffsetAlt = unpatchedFuncs->mpuInitRegionOldDataAlt;
			}

			if (unpatchedFuncs->mpuInitOffset2) {
				*unpatchedFuncs->mpuInitOffset2 = 0xEE060F12;
			}
			if (unpatchedFuncs->mpuDataOffset2) {
				*unpatchedFuncs->mpuDataOffset2 = unpatchedFuncs->mpuInitRegionOldData2;
			}
		}
		#endif

		funcsUnpatched = true;
	}

#ifndef TWLSDK
	if (!(valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM) && isSdEjected()) {
		tonccpy((u32*)0x02000300, sr_data_error, 0x020);
		rebootConsole();		// Reboot into error screen if SD card is removed
	}
#endif

/* #ifndef TWLSDK
	if (valueBits & isDlp) {
		if (!(REG_EXTKEYINPUT & KEY_A) && *(u32*)(NDS_HEADER_SDK5+0xC) != 0 && !wifiIrq) {
			IPC_SendSync(0x5);
			reset();
		}
	}
#endif */

	if ((0 == (REG_KEYINPUT & igmHotkey) && 0 == (REG_EXTKEYINPUT & (((igmHotkey >> 10) & 3) | ((igmHotkey >> 6) & 0xC0))) && (valueBits & igmAccessible) && !wifiIrq) /* || returnToMenu */ || sharedAddr[5] == 0x4C4D4749 /* IGML */) {
#ifdef TWLSDK
		igmText = (struct IgmText *)INGAME_MENU_LOCATION;
		i2cWriteRegister(0x4A, 0x12, 0x00);
#endif
		inGameMenu();
#ifdef TWLSDK
		i2cWriteRegister(0x4A, 0x12, 0x01);
#endif
	}

/*KEY_X*/
	/* if (0==(REG_KEYINPUT & (KEY_L | KEY_R | KEY_UP)) && !(REG_EXTKEYINPUT & KEY_A)) {
		if (tryLockMutex(&saveMutex)) {
			if (swapTimer == 60){
				swapTimer = 0;
				if (!ipcEveryFrame) {
					IPC_SendSync(0x7);
				}
				swapScreens = true;
			}
		}
		unlockMutex(&saveMutex);
		swapTimer++;
	} else {
		swapTimer = 0;
	} */

#ifdef TWLSDK
	if (sharedAddr[3] == (vu32)0x54495845) {
		returnToLoader(false);
	}
#endif

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B))) {
		if (returnTimer == 60 * 2) {
#ifdef TWLSDK
			IPC_SendSync(0x5);
#else
			returnToLoader(false);
#endif
		}
		returnTimer++;
	} else {
		returnTimer = 0;
	}

	/* if ((valueBits & b_dsiSD) && (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A)))) {
		if (tryLockMutex(&cardEgnineCommandMutex)) {
			if (ramDumpTimer == 60 * 2) {
				REG_MASTER_VOLUME = 0;
				int oldIME = enterCriticalSection();
				dumpRam();
				leaveCriticalSection(oldIME);
				REG_MASTER_VOLUME = 127;
			}
			unlockMutex(&cardEgnineCommandMutex);
		}
		ramDumpTimer++;
	} else {
		ramDumpTimer = 0;
	} */

	if (sharedAddr[3] == (vu32)0x52534554) {
		reset();
	}

	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT))) {
		if (tryLockMutex(&saveMutex)) {
			if ((softResetTimer == 60 * 2) && (saveTimer == 0)) {
				REG_MASTER_VOLUME = 0;
				int oldIME = enterCriticalSection();
				forceGameReboot();
				leaveCriticalSection(oldIME);
			}
			unlockMutex(&saveMutex);
		}
		softResetTimer++;
	} else {
		softResetTimer = 0;
	}

	#ifndef TWLSDK
	if (valueBits & powerCodeOnVBlank) {
		i2cIRQHandler();
	}
	#endif

	if ((valueBits & preciseVolumeControl) || (valueBits & i2cBricked)) {
		// Precise volume adjustment (for DSi)
		if (volumeAdjustActivated) {
			volumeAdjustDelay++;
			if (volumeAdjustDelay == 30) {
				volumeAdjustDelay = 0;
				volumeAdjustActivated = false;
			}
		} else if (0==(REG_KEYINPUT & KEY_SELECT)) {
			if (valueBits & i2cBricked) {
				const int oldVolLevel = noI2CVolLevel;
				if (0==(REG_KEYINPUT & KEY_UP)) {
					noI2CVolLevel = 127;
				} else if (0==(REG_KEYINPUT & KEY_DOWN)) {
					noI2CVolLevel = 0;
				}
				volumeAdjustActivated = (noI2CVolLevel != oldVolLevel);
			} else {
				const u8 i2cVolLevel = i2cReadRegister(0x4A, 0x40);
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
	}
	
	if (saveTimer > 0) {
		saveTimer++;
		if (saveTimer == 60) {
			//i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
			saveTimer = 0;
		}
	}

	if (REG_IE & IRQ_NETWORK) {
		REG_IE &= ~IRQ_NETWORK; // DSi RTC fix
	}

	bool wifiIrqCheck = (REG_WIFIIRQ != 0);
	if (wifiIrq != wifiIrqCheck) {
		// Turn off card read DMA if WiFi is used, and back on when not in use
		if (wifiIrq) {
			wifiIrqTimer++;
			if (wifiIrqTimer == 30) {
				// IPC_SendSync(0x4);
				wifiIrq = wifiIrqCheck;
			}
		} else {
			// IPC_SendSync(0x4);
			wifiIrq = wifiIrqCheck;
		}
	} else {
		wifiIrqTimer = 0;
	}

	// calledViaIPC = false;
	// runCardEngineCheck();

	// Update main screen or swap screens
	if (ipcEveryFrame) {
		if (dmaSignal) {
			IPC_SendSync(0x3);
			dmaSignal = false;
		} else {
			IPC_SendSync(swapScreens ? 0x7 : 0x6);
		}
	}
	swapScreens = false;

	if (sharedAddr[0] == 0x524F5245) { // 'EROR'
		REG_MASTER_VOLUME = 0;
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	} else {
		break;
	}
  }
}

#ifndef TWLSDK
void i2cIRQHandler(void) {
	int cause = (i2cReadRegister(I2C_PM, I2CREGPM_PWRIF) & 0x3) | (i2cReadRegister(I2C_GPIO, 0x02)<<2);

	switch (cause & 3) {
	case 1: {
		if (saveTimer != 0) return;

		REG_MASTER_VOLUME = 0;
		int oldIME = enterCriticalSection();
		if (consoleModel < 2) {
			//unlaunchSetFilename(true);
			sharedAddr[4] = 0x57534352;
			IPC_SendSync(0x8);
			waitFrames(5);							// Wait for DSi screens to stabilize
		}
		rebootConsole();			// Reboot console
		leaveCriticalSection(oldIME);
		break;
	}
	case 2:
		writePowerManagement(PM_CONTROL_REG,PM_SYSTEM_PWR);
		break;
	}
}
#endif

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG		
	nocashMessage("myIrqEnable\n");
	#endif	

	initialize();

	//if (!(valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM)) {
		REG_AUXIE &= ~(1UL << 8);
	//}

	#ifdef TWLSDK
	//bool doBak = ((valueBits & gameOnFlashcard) && (valueBits & b_dsiSD));
	//if (doBak) bakSdData();
	#endif
	driveInitialize();
  	#ifdef TWLSDK
	//if (doBak) restoreSdBakData();
	#endif

	/*if (!(valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM) && ndsHeader->unitCode > 0 && (valueBits & dsiMode)) {
		extern u32* dsiIrqTable;
		extern u32* dsiIrqRet;
		extern u32* extraIrqTable_offset;
		extern u32* extraIrqRet_offset;

		dsiIrqTable[8] = extraIrqTable_offset[8];
		dsiIrqRet[8] = extraIrqRet_offset[8];
	}*/

	/*const char* romTid = getRomTid(ndsHeader);

	if ((strncmp(romTid, "UOR", 3) == 0)
	|| (strncmp(romTid, "UXB", 3) == 0)
	|| (strncmp(romTid, "USK", 3) == 0)
	|| (!(valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM))) {
		// Proceed below "else" code
	} else {
		u32 irq_before = REG_IE;		
		REG_IE |= irq;
		leaveCriticalSection(oldIME);
		return irq_before;
	}*/

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;
	irq |= IRQ_IPC_SYNC;
	//irq |= BIT(28);
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	//if (!(valueBits & powerCodeOnVBlank)) {
	//	REG_AUXIE |= IRQ_I2C;
	//}
	//if (!(valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM)) {	
	//	REG_AUXIE |= IRQ_SDMMC;
	//}
	leaveCriticalSection(oldIME);
	//ipcSyncHooked = true;
	return irq_before;
}

/*static void irqIPCSYNCEnable(void) {	
	if (!initializedIRQ) {
		int oldIME = enterCriticalSection();
		initialize();	
		#ifdef DEBUG		
		dbg_printf("\nirqIPCSYNCEnable\n");	
		#endif	
		REG_IE |= IRQ_IPC_SYNC;
		REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;
		#ifdef DEBUG		
		dbg_printf("IRQ_IPC_SYNC enabled\n");
		#endif	
		leaveCriticalSection(oldIME);
		initializedIRQ = true;
	}
}*/

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

	if (!(valueBits & saveOnFlashcard) && isSdEjected()) {
		return false;
	}

	if (tryLockMutex(&saveMutex)) {
		// while (readOngoing) { swiDelay(100); }
		#ifdef TWLSDK
		//bool doBak = ((valueBits & gameOnFlashcard) && !(valueBits & saveOnFlashcard));
		//if (doBak) bakSdData();
		#endif
		//driveInitialize();
		/*if (saveInRam) {
			tonccpy(dst, (char*)0x02440000 + src, len);
		} else {*/
			sdmmc_set_ndma_slot(4);
			if ((u32)(src % saveSize)+len > saveSize) {
				u32 len2 = len;
				u32 len3 = 0;
				while ((u32)(src % saveSize)+len2 > saveSize) {
					len2--;
					len3++;
				}
				fileRead(dst, savFile, (src % saveSize), len2);
				fileRead(dst+len2, savFile, ((src+len2) % saveSize), len3);
			} else {
				fileRead(dst, savFile, (src % saveSize), len);
			}
			sdmmc_set_ndma_slot(0);
		//}
		#ifdef TWLSDK
		//if (doBak) restoreSdBakData();
		#endif
  		unlockMutex(&saveMutex);
	}
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

	if (!(valueBits & saveOnFlashcard) && isSdEjected()) {
		return false;
	}

	if (tryLockMutex(&saveMutex)) {
		// while (readOngoing) { swiDelay(100); }
		#ifdef TWLSDK
		//bool doBak = ((valueBits & gameOnFlashcard) && !(valueBits & saveOnFlashcard));
		//if (doBak) bakSdData();
		#endif
		//driveInitialize();
		saveTimer = 1;
		//i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		/*if (saveInRam) {
			tonccpy((char*)0x02440000 + dst, src, len);
		}*/
		sdmmc_set_ndma_slot(4);
		if ((dst % saveSize)+len > saveSize) {
			u32 len2 = len;
			u32 len3 = 0;
			while ((u32)(dst % saveSize)+len2 > saveSize) {
				len2--;
				len3++;
			}
			fileWrite(src, savFile, (dst % saveSize), len2);
			fileWrite(src+len2, savFile, ((dst+len2) % saveSize), len3);
		} else {
			fileWrite(src, savFile, (dst % saveSize), len);
		}
		sdmmc_set_ndma_slot(0);
		#ifdef TWLSDK
		//if (doBak) restoreSdBakData();
		#endif
  		unlockMutex(&saveMutex);
	}
	return true;
}

bool eepromPageProg(u32 dst, const void *src, u32 len) {
	#ifdef DEBUG
	dbg_printf("\narm7 eepromPageProg\n");
	#endif

	return eepromPageWrite(dst, src, len);
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

	if (!(valueBits & saveOnFlashcard) && isSdEjected()) {
		return false;
	}

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

	if (valueBits & ROMinRAM) {
		cardReadRAM(dst, src, len);
	} else {
		// while (readOngoing) { swiDelay(100); }
		//driveInitialize();
		cardReadLED(true, false);    // When a file is loading, turn on LED for card read indicator
		//ndmaUsed = false;
		#ifdef DEBUG	
		nocashMessage("fileRead romFile");
		#endif	
		fileRead(dst, romFile, src, len);
		//ndmaUsed = true;
		cardReadLED(false, false);    // After loading is done, turn off LED for card read indicator
	}

	return true;
}
