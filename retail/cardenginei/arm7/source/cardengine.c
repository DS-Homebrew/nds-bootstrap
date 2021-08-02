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
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/input.h>
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

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern u32 ce7;

static const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char bootNdsPath[14] = {'s','d','m','c',':','/','b','o','o','t','.','n','d','s'};

extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

extern vu32* volatile cardStruct;
extern u32 fileCluster;
extern u32 saveCluster;
extern u32 srParamsCluster;
extern u32 ramDumpCluster;
extern u32 screenshotCluster;
extern module_params_t* moduleParams;
extern u32 valueBits;
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

bool dsiSD = false;
bool sdRead = true;

static bool initialized = false;
static bool driveInited = false;
//static bool initializedIRQ = false;
static bool haltIsRunning = false;
static bool calledViaIPC = false;
static bool ipcSyncHooked = false;
//static bool dmaLed = false;
static bool swapScreens = false;
static bool dmaSignal = false;
static bool saveInRam = false;

#ifdef TWLSDK
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_SDK5;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION_SDK5;
static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION_SDK5;
#else
#ifdef ALTERNATIVE
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_ALT;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION_ALT;
static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION_ALT;
#else
static aFile* romFile = (aFile*)ROM_FILE_LOCATION;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION;
static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION;
#endif
#endif
static aFile ramDumpFile;
static aFile srParamsFile;
static aFile screenshotFile;

static int saveTimer = 0;

static int languageTimer = 0;
static int swapTimer = 0;
static int returnTimer = 0;
static int softResetTimer = 0;
static int ramDumpTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;

//static bool ndmaUsed = false;

static int cardEgnineCommandMutex = 0;
static int saveMutex = 0;

bool returnToMenu = false;

#ifdef TWLSDK
static const tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
static PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER_SDK5-0x180);
#else
static const tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
static PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER-0x180);
#endif
static const char* romLocation = NULL;

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
			*(u8*)(0x02000838+i2) = *(u8*)(ce7+0xA800+i);		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			i2 += 2;
		}
	}
	while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
		*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
	}
}

static void readSrBackendId(void) {
	// Use SR backend ID
	*(u32*)(0x02000300) = 0x434E4C54;	// 'CNLT'
	*(u16*)(0x02000304) = 0x1801;
	*(u32*)(0x02000308) = *(u32*)(ce7+0xA900);
	*(u32*)(0x0200030C) = *(u32*)(ce7+0xA904);
	*(u32*)(0x02000310) = *(u32*)(ce7+0xA900);
	*(u32*)(0x02000314) = *(u32*)(ce7+0xA904);
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

	if ((valueBits & b_dsiSD) && !(valueBits & gameOnFlashcard) && !(valueBits & saveOnFlashcard)) {
		if (sdmmc_read16(REG_SDSTATUS0) != 0) {
			sdmmc_init();
			SD_Init();
		}
		dsiSD = true;
		sdRead = true;				// Switch to SD
		FAT_InitFiles(false, false, 0);
	}
	if (((valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM)) || (valueBits & saveOnFlashcard)) {
		sdRead = false;			// Switch to flashcard
		FAT_InitFiles(false, true, 0);
		sdRead = true;				// Switch to SD
	}

	ramDumpFile = getFileFromCluster(ramDumpCluster);
	srParamsFile = getFileFromCluster(srParamsCluster);
	screenshotFile = getFileFromCluster(screenshotCluster);

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
	aFile myDebugFile = getBootFileCluster("NDSBTSRP.LOG", 0);
	enableDebug(myDebugFile);
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
	
	sdRead = sdReadBak;

	driveInited = true;
}

static void initialize(void) {
	if (initialized) {
		return;
	}

	#ifndef TWLSDK
	if (isSdk5(moduleParams)) {
		sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;
		ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER_SDK5-0x180);
	}
	#endif

	//if (isSdk5(moduleParams)) {
	//	*(u16*)0x02fffc40 = 1;	// Change boot indicator to Slot-1 card
	//}
	romLocation = (char*)(((valueBits & dsiMode) || isSdk5(moduleParams)) ? ROM_SDK5_LOCATION : ROM_LOCATION);
	if (valueBits & extendedMemory) {
		ndsHeader = (tNDSHeader*)NDS_HEADER_4MB;
		personalData = (PERSONAL_DATA*)((u8*)NDS_HEADER_4MB-0x180);
		romLocation = (char*)(ROM_LOCATION_EXT);
	}

	/*if (ndsHeader->unitCode > 0 && (valueBits & dsiMode)) {
		igmText = (struct IgmText *)INGAME_MENU_LOCATION_TWLSDK;
	}*/

	toncset((u8*)0x06000000, 0, 0x40000);	// Clear bootloader

	initialized = true;
}

static void cardReadLED(bool on, bool dmaLed) {
	if (consoleModel < 2) {
		if (dmaRomRead_LED == -1) dmaRomRead_LED = romRead_LED;
		if (on) {
			switch(dmaLed ? dmaRomRead_LED : romRead_LED) {
				case 0:
				default:
					break;
				case 1:
					i2cWriteRegister(0x4A, 0x30, 0x13);    // Turn WiFi LED on
					break;
				case 2:
					i2cWriteRegister(0x4A, 0x63, 0xFF);    // Turn power LED purple
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
					i2cWriteRegister(0x4A, 0x63, 0x00);    // Revert power LED to normal
					break;
				case 3:
					i2cWriteRegister(0x4A, 0x31, 0x00);    // Turn Camera LED off
					break;
			}
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

void forceGameReboot(void) {
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)(0x02000000) = 0;
	sharedAddr[4] = 0x57534352;
	IPC_SendSync(0x8);
	if (consoleModel < 2) {
		if (*(u32*)(ce7+0xA900) == 0) {
			unlaunchSetFilename(false);
		}
		waitFrames(5);							// Wait for DSi screens to stabilize
	}
	u32 clearBuffer = 0;
	driveInitialize();
	sdRead = !(valueBits & gameOnFlashcard);
	fileWrite((char*)&clearBuffer, srParamsFile, 0, 0x4, !sdRead, -1);
	if (*(u32*)(ce7+0xA900) == 0) {
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
		if (*(u32*)(ce7+0xA900) == 0) {
			tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
		}
		waitFrames(1);
	} else {
		if (*(u32*)(ce7+0xA900) == 0) {
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

void saveScreenshot(void) {
	if (igmText->currentScreenshot >= 50) return;

	driveInitialize();
	sdRead = (valueBits & b_dsiSD);
	fileWrite((char*)DONOR_ROM_ARM7_SIZE_LOCATION, screenshotFile, 0x200+(igmText->currentScreenshot*0x18400), 0x18046, !sdRead, -1);

	igmText->currentScreenshot++;
}

static void log_arm9(void) {
	//driveInitialize();
	#ifdef DEBUG
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
	#endif
}

static void nandRead(void) {
	u32 flash = *(vu32*)(sharedAddr+2);
	u32 memory = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr+1);
	u32 marker = *(vu32*)(sharedAddr+3);

	#ifdef DEBUG
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

    if (tryLockMutex(&saveMutex)) {
		driveInitialize();
	    cardReadLED(true, true);    // When a file is loading, turn on LED for card read indicator
		fileRead(memory, *savFile, flash, len, !sdRead, -1);
    	cardReadLED(false, true);
  		unlockMutex(&saveMutex);
	}
}

static void nandWrite(void) {
	u32 flash = *(vu32*)(sharedAddr+2);
	u32 memory = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr+1);
	u32 marker = *(vu32*)(sharedAddr+3);

	#ifdef DEBUG
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

  	if (tryLockMutex(&saveMutex)) {
		driveInitialize();
		saveTimer = 1;			// When we're saving, power button does nothing, in order to prevent corruption.
	    cardReadLED(true, true);    // When a file is loading, turn on LED for card read indicator
		fileWrite(memory, *savFile, flash, len, !sdRead, -1);
    	cardReadLED(false, true);
  		unlockMutex(&saveMutex);
	}
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

static bool readOngoing = false;
/*//static int currentCmd=0, currentNdmaSlot=0;
static int timeTillDmaLedOff = 0;

static bool start_cardRead_arm9(void) {
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

	//driveInitialize();
	cardReadLED(true, true);    // When a file is loading, turn on LED for card read indicator
	#ifdef DEBUG
	nocashMessage("fileRead romFile");
	#endif
	if(!fileReadNonBLocking((char*)dst, romFile, src, len, 0))
    {
        readOngoing = true;
        return false;    
        //while(!resumeFileRead()){}
    } 
    else
    {
        readOngoing = false;
        cardReadLED(false, true);    // After loading is done, turn off LED for card read indicator
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
    if(resumeFileRead())
    {
        readOngoing = false;
        cardReadLED(false, true);    // After loading is done, turn off LED for card read indicator
        return true;    
    } 
    else
    {
        return false;    
    }
}*/

static inline void sdmmcHandler(void) {
	switch (sharedAddr[4]) {
		case 0x53445231:
		case 0x53444D31: {
			//#ifdef DEBUG		
			//dbg_printf("my_sdmmc_sdcard_readsector\n");
			//#endif
			bool isDma = sharedAddr[4]==0x53444D31;
			cardReadLED(true, isDma);
			sharedAddr[4] = my_sdmmc_sdcard_readsector(sharedAddr[0], (u8*)sharedAddr[1], sharedAddr[2], sharedAddr[3]);
			cardReadLED(false, isDma);
		}	break;
		case 0x53445244:
		case 0x53444D41: {
			//#ifdef DEBUG		
			//dbg_printf("my_sdmmc_sdcard_readsectors\n");
			//#endif
			bool isDma = sharedAddr[4]==0x53444D41;
			cardReadLED(true, isDma);
			sharedAddr[4] = my_sdmmc_sdcard_readsectors(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2], sharedAddr[3]);
			cardReadLED(false, isDma);
		}	break;
		/*case 0x53444348:
			sharedAddr[4] = my_sdmmc_sdcard_check_command(sharedAddr[0], sharedAddr[1]);
			//currentCmd = sharedAddr[0];
			//currentNdmaSlot = sharedAddr[1];
			break;
		case 0x53415244:
			cardReadLED(true, true);
			sharedAddr[4] = my_sdmmc_sdcard_readsectors_nonblocking(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2], sharedAddr[3]);
			//currentCmd = sharedAddr[4];
			//currentNdmaSlot = sharedAddr[3];
			timeTillDmaLedOff = 0;
			readOngoing = true;
			break;
		case 0x53445752:
			cardReadLED(true, true);
			sharedAddr[4] = my_sdmmc_sdcard_writesectors(sharedAddr[0], sharedAddr[1], (u8*)sharedAddr[2], sharedAddr[3]);
			cardReadLED(false, true);
			break;*/
	}
}

static void runCardEngineCheck(void) {
	//dbg_printf("runCardEngineCheck\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheck");
	#endif	

    /*if (IPC_GetSync() == 0x3) {
		IPC_SendSync(0x3);
		return;
	}*/

  	if (tryLockMutex(&cardEgnineCommandMutex)) {
        //if(!readOngoing)
        //{
    
    		//nocashMessage("runCardEngineCheck mutex ok");
    
  		/*if (sharedAddr[3] == (vu32)0x5245424F) {
  			i2cWriteRegister(0x4A, 0x70, 0x01);
  			i2cWriteRegister(0x4A, 0x11, 0x01);
  		}*/

			if (!haltIsRunning && !(valueBits & gameOnFlashcard)) {
				sdmmcHandler();
			}

    		if (sharedAddr[3] == (vu32)0x026FF800) {
				sdRead = (valueBits & b_dsiSD);
    			log_arm9();
    			sharedAddr[3] = 0;
                //IPC_SendSync(0x8);
    		}
    
            if (sharedAddr[3] == (vu32)0x025FFC01) {
				sdRead = !(valueBits & saveOnFlashcard);
                //dmaLed = (sharedAddr[3] == (vu32)0x025FFC01);
    			nandRead();
    			sharedAddr[3] = 0;
    		}

            if (sharedAddr[3] == (vu32)0x025FFC02) {
				sdRead = !(valueBits & saveOnFlashcard);
                //dmaLed = (sharedAddr[3] == (vu32)0x025FFC02);
    			nandWrite();
    			sharedAddr[3] = 0;
    		}

            /*if (sharedAddr[3] == (vu32)0x025FBC01) {
				sdRead = true;
                dmaLed = false;
    			slot2Read();
    			sharedAddr[3] = 0;
    			IPC_SendSync(0x8);
    		}*/
        //}
  		unlockMutex(&cardEgnineCommandMutex);
  	}
}

//---------------------------------------------------------------------------------
void myIrqHandlerFIFO(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerFIFO");
	#endif	
	
	calledViaIPC = true;
	
	runCardEngineCheck();
}

//---------------------------------------------------------------------------------
void myIrqHandlerHalt(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerHalt");
	#endif	

	haltIsRunning = true;

	sdmmcHandler();

	if (sharedAddr[4] == (vu32)0x025FFB0A) {	// Card read DMA
		if (!readOngoing) {
			ndmaCopyWordsAsynch(0, (u8*)sharedAddr[2], (u8*)(sharedAddr[0] > 0x03000000 ? 0 : sharedAddr[0]), sharedAddr[1]);
			readOngoing = true;
		} else if (!ndmaBusy(0)) {
			readOngoing = false;
			sharedAddr[4] = 0;
			if (valueBits & ipcEveryFrame) {
				dmaSignal = true;
			} else {
				IPC_SendSync(0x3);
			}
		}
	}

	/*if (readOngoing) {
		timeTillDmaLedOff++;
		if (timeTillDmaLedOff > 10) {
			readOngoing = false;
			cardReadLED(false, true);
		}
	}*/
}

//---------------------------------------------------------------------------------
void myIrqHandlerNdma0(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerNdma0");
	#endif	
	
	calledViaIPC = false;

	//i2cWriteRegister(0x4A, 0x70, 0x01);
	//i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot console
	//runCardEngineCheckResume();
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

	//*(vu32*)(0x027FFB30) = (vu32)isSdEjected();
	if (!(valueBits & ROMinRAM) && isSdEjected()) {
		tonccpy((u32*)0x02000300, sr_data_error, 0x020);
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into error screen if SD card is removed
	}

	if ((0 == (REG_KEYINPUT & igmHotkey) && 0 == (REG_EXTKEYINPUT & (((igmHotkey >> 10) & 3) | ((igmHotkey >> 6) & 0xC0))) && !(valueBits & extendedMemory) && (ndsHeader->unitCode == 0 || !(valueBits & dsiMode))) || returnToMenu) {
		inGameMenu();
	}

	if (0==(REG_KEYINPUT & (KEY_L | KEY_R | KEY_UP)) && !(REG_EXTKEYINPUT & KEY_A/*KEY_X*/)) {
		if (tryLockMutex(&saveMutex)) {
			if (swapTimer == 60){
				swapTimer = 0;
				//if (!(valueBits & ipcEveryFrame)) {
					IPC_SendSync(0x7);
				//}
				swapScreens = true;
			}
		}
		unlockMutex(&saveMutex);
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
	}

	if (sharedAddr[3] == (vu32)0x52534554) {
		REG_MASTER_VOLUME = 0;
		int oldIME = enterCriticalSection();
		driveInitialize();
		sdRead = !(valueBits & gameOnFlashcard);
		fileWrite((char*)(isSdk5(moduleParams) ? RESET_PARAM_SDK5 : RESET_PARAM), srParamsFile, 0, 0x10, !sdRead, -1);
		if (consoleModel < 2 && *(u32*)(ce7+0xA900) == 0) {
			unlaunchSetFilename(false);
		}
		if (*(u32*)(ce7+0xA900) == 0) {
			tonccpy((u32*)0x02000300, sr_data_srllastran, 0x020);
		} else {
			// Use different SR backend ID
			readSrBackendId();
		}
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot game
		leaveCriticalSection(oldIME);
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

	if (valueBits & powerCodeOnVBlank) {
		i2cIRQHandler();
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
	
	if (saveTimer > 0) {
		saveTimer++;
		if (saveTimer == 60) {
			//i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
			saveTimer = 0;
		}
	}

	if (ipcSyncHooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	} else if (valueBits & b_runCardEngineCheck) {
		calledViaIPC = false;
		runCardEngineCheck();
	}
	/*if (readOngoing) {
		if (my_sdmmc_sdcard_check_command(0x33C12, 0)) {
			sharedAddr[4] = 0;
			cardReadLED(false);
			readOngoing = false;
			sharedAddr[3] = 0;
		}
	}*/

	// Update main screen or swap screens
	/*if (valueBits & ipcEveryFrame) {
		if (dmaSignal) {
			IPC_SendSync(0x3);
			dmaSignal = false;
		} else {
			IPC_SendSync(swapScreens ? 0x7 : 0x0);
		}
	}
	swapScreens = false;*/
}

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
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot console
		leaveCriticalSection(oldIME);
		break;
	}
	case 2:
		writePowerManagement(PM_CONTROL_REG,PM_SYSTEM_PWR);
		break;
	}
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG		
	nocashMessage("myIrqEnable\n");
	#endif	

	initialize();

	REG_AUXIE &= ~(1UL << 8);
	if (!(valueBits & gameOnFlashcard) && !(valueBits & ROMinRAM)) {
		driveInitialize();
	}

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	//irq |= BIT(28);
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	//if (!(valueBits & powerCodeOnVBlank)) {
	//	REG_AUXIE |= IRQ_I2C;
	//}
	leaveCriticalSection(oldIME);
	ipcSyncHooked = true;
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
		//while (readOngoing) {}
		driveInitialize();
		sdRead = ((valueBits & saveOnFlashcard) ? false : true);
		if (saveInRam) {
			tonccpy(dst, (char*)0x02440000 + src, len);
		} else {
			fileRead(dst, *savFile, src, len, !sdRead, -1);
		}
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
		//while (readOngoing) {}
		driveInitialize();
		sdRead = ((valueBits & saveOnFlashcard) ? false : true);
		saveTimer = 1;
		//i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		if (saveInRam) {
			tonccpy((char*)0x02440000 + dst, src, len);
		}
		fileWrite(src, *savFile, dst, len, !sdRead, -1);
  		unlockMutex(&saveMutex);
	}
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

	if (!(valueBits & saveOnFlashcard) && isSdEjected()) {
		return false;
	}

  	if (tryLockMutex(&saveMutex)) {
		//while (readOngoing) {}
		driveInitialize();
		sdRead = ((valueBits & saveOnFlashcard) ? false : true);
		saveTimer = 1;
		//i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		if (saveInRam) {
			tonccpy((char*)0x02440000 + dst, src, len);
		}
		fileWrite(src, *savFile, dst, len, !sdRead, -1);
  		unlockMutex(&saveMutex);
	}
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
		tonccpy(dst, romLocation + src, len);
	} else {
		//while (readOngoing) {}
		driveInitialize();
		sdRead = ((valueBits & gameOnFlashcard) ? false : true);
		cardReadLED(true, false);    // When a file is loading, turn on LED for card read indicator
		//ndmaUsed = false;
		#ifdef DEBUG	
		nocashMessage("fileRead romFile");
		#endif	
		fileRead(dst, *romFile, src, len, !sdRead, 0);
		//ndmaUsed = true;
		cardReadLED(false, false);    // After loading is done, turn off LED for card read indicator
	}
	
	return true;
}
