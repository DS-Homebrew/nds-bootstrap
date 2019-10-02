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

#include <string.h> // memcpy
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

#include "my_fat.h"
#include "locations.h"
#include "module_params.h"
#include "debug_file.h"
#include "cardengine.h"

#include "sr_data_error.h"      // For showing an error screen
#include "sr_data_srloader.h"   // For rebooting into DSiMenu++
#include "sr_data_srllastran.h" // For rebooting the game
#include "sr_data_srllastran_twltouch.h" // SDK 5 --> For rebooting the game (TWL-mode touch screen)

//static const char *unlaunchAutoLoadID = "AutoLoadInfo";
//static char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

//#define memcpy __builtin_memcpy

extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

extern vu32* volatile cardStruct;
extern u32 fileCluster;
extern u32 saveCluster;
extern module_params_t* moduleParams;
extern u32 language;
extern u32 gottenSCFGExt;
extern u32 dsiMode;
extern u32 ROMinRAM;
extern u32 consoleModel;
extern u32 romread_LED;
extern u32 gameSoftReset;
extern u32 soundFix;

static bool initialized = false;
//static bool initializedIRQ = false;
static bool calledViaIPC = false;

static aFile* romFile = (aFile*)ROM_FILE_LOCATION;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION;

//static int saveTimer = 0;

/*static int softResetTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;*/

//static bool ndmaUsed = false;

//static int cardEgnineCommandMutex = 0;
static int saveMutex = 0;

static const tNDSHeader* ndsHeader = NULL;
//static const char* romLocation = NULL;

/*static void unlaunchSetHiyaBoot(void) {
	memcpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) |= BIT(0);		// Load the title at 2000838h
	*(u32*)(0x02000810) |= BIT(1);		// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	memset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < 14; i++) {
		*(u8*)(0x02000838+i2) = hiyaNdsPath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
		*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
	}
}*/

static void initialize(void) {
	if (initialized) {
		return;
	}
	
	/*if (sdmmc_read16(REG_SDSTATUS0) != 0) {
		sdmmc_init();
		SD_Init();
	}*/
	FAT_InitFiles(true, 0);
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

	ndsHeader = (tNDSHeader*)(isSdk5(moduleParams) ? NDS_HEADER_SDK5 : NDS_HEADER);
	//romLocation = (char*)((dsiMode || isSdk5(moduleParams)) ? ROM_SDK5_LOCATION : ROM_LOCATION);

	initialized = true;
}


//---------------------------------------------------------------------------------
void myIrqHandlerFIFO(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerFIFO");
	#endif	
	
	calledViaIPC = true;
}

//---------------------------------------------------------------------------------
void mySwiHalt(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("mySwiHalt");
	#endif	
	
	calledViaIPC = false;
}


void myIrqHandlerVBlank(void) {
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	calledViaIPC = false;

	if (language >= 0 && language < 6) {
		// Change language
		*(u8*)((u32)ndsHeader - 0x11C) = language;
	}

	/*if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B))) {
		if ((softResetTimer == 60 * 2) && (saveTimer == 0)) {
			if (consoleModel < 2) {
				unlaunchSetHiyaBoot();
			}
			memcpy((u32*)0x02000300, sr_data_srloader, 0x20);
			i2cWriteRegister(0x4A, 0x70, 0x01);
			i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into TWiLight Menu++/DSiMenu++/SRLoader
		}
		softResetTimer++;
	} else {
		softResetTimer = 0;
	}

	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT)) && !gameSoftReset && saveTimer == 0) {
		if (consoleModel < 2) {
			unlaunchSetHiyaBoot();
		}
		//memcpy((u32*)0x02000300, dsiMode ? sr_data_srllastran_twltouch : sr_data_srllastran, 0x020); // SDK 5
		memcpy((u32*)0x02000300, sr_data_srllastran, 0x020);
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot game
	}

	if (consoleModel < 2 && romread_LED == 0) {
		// Precise volume adjustment (for DSi)
		if (volumeAdjustActivated) {
			volumeAdjustDelay++;
			if (volumeAdjustDelay == 30) {
				volumeAdjustDelay = 0;
				volumeAdjustActivated = false;
			}
		} else {
			u8 i2cVolLevel = i2cReadRegister(0x4A, 0x40);
			u8 i2cNewVolLevel = i2cVolLevel;
			if (REG_KEYINPUT & (KEY_SELECT | KEY_UP)) {} else {
				i2cNewVolLevel++;
			}
			if (REG_KEYINPUT & (KEY_SELECT | KEY_DOWN)) {} else {
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
			i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
			saveTimer = 0;
		}
	}*/

	#ifdef DEBUG
	nocashMessage("cheat_engine_start\n");
	#endif	
	
	cheat_engine_start();
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();	
	
	#ifdef DEBUG		
	nocashMessage("myIrqEnable\n");
	#endif	
	
	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
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

  	if (tryLockMutex(&saveMutex)) {
		initialize();
		fileRead(dst, *savFile, src, len, -1);
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
	
  	if (tryLockMutex(&saveMutex)) {
		initialize();
		/*if (saveTimer == 0) {
			i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		}
		saveTimer = 1;*/
		fileWrite(src, *savFile, dst, len, -1);
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

  	if (tryLockMutex(&saveMutex)) {
		initialize();
		/*if (saveTimer == 0) {
			i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		}
		saveTimer = 1;*/
		fileWrite(src, *savFile, dst, len, -1);
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
	//fileWrite(src, *savFile, dst, len, -1);
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

u32 cardId(void) {
	#ifdef DEBUG	
	dbg_printf("\ncardId\n");
	#endif	

	return 1;
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
	
	/*if (ROMinRAM) {
		memcpy(dst, romLocation + src, len);
	} else {*/
		initialize();
		//cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
		//ndmaUsed = false;
		#ifdef DEBUG	
		nocashMessage("fileRead romFile");
		#endif	
		fileRead(dst, *romFile, src, len, 2);
		//ndmaUsed = true;
		//cardReadLED(false);    // After loading is done, turn off LED for card read indicator
	//}
	
	return true;
}
