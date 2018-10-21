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

#include "my_sdmmc.h"
#include "my_fat.h"
#include "locations.h"
#include "module_params.h"
#include "debug_file.h"
#include "cardengine.h"

#include "sr_data_error.h"      // For showing an error screen
#include "sr_data_srloader.h"   // For rebooting into DSiMenu++
#include "sr_data_srllastran.h" // For rebooting the game
#include "sr_data_srllastran_twltouch.h" // SDK 5 --> For rebooting the game (TWL-mode touch screen)

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

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static bool initialized = false;
//static bool initializedIRQ = false;
static bool calledViaIPC = false;

static aFile* romFile = (aFile*)ROM_FILE_LOCATION;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION;

static int cardReadTimeOut = 0;
static int saveTimer = 0;

static int softResetTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;

//static bool ndmaUsed = false;

static int cardEgnineCommandMutex = 0;

static const tNDSHeader* ndsHeader = NULL;
static const char* romLocation = NULL;

static void initialize(void) {
	if (initialized) {
		return;
	}
	
	if (sdmmc_read16(REG_SDSTATUS0) != 0) {
		sdmmc_init();
		SD_Init();
	}
	FAT_InitFiles(false, 3);
	//romFile = getFileFromCluster(fileCluster);
	//buildFatTableCache(&romFile, 3);
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
	aFile myDebugFile = getBootFileCluster("NDSBTSRP.LOG", 3);
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
	romLocation = (char*)((dsiMode || isSdk5(moduleParams)) ? ROM_SDK5_LOCATION : ROM_LOCATION);

	initialized = true;
}

static void cardReadLED(bool on) {
	if (consoleModel < 2) {
		if (on) {
			switch(romread_LED) {
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
			switch(romread_LED) {
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

static void log_arm9(void) {
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

static void cardRead_arm9(void) {
	u32 src = *(vu32*)(sharedAddr + 2);
	u32 dst = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr + 1);
	#ifdef DEBUG
	u32 marker = *(vu32*)(sharedAddr + 3);

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

	cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
	#ifdef DEBUG
	nocashMessage("fileRead romFile");
	#endif
	if(!fileReadNonBLocking((char*)dst, romFile, src, len, 0))
    {
        while(!resumeFileRead()){}
    }

	// Primary fix for Mario's Holiday
	if (*(u32*)(0x0C9328AC) == 0x4B434148) {
		*(u32*)(0x0C9328AC) = 0xA00;
	}

	cardReadLED(false);    // After loading is done, turn off LED for card read indicator

	#ifdef DEBUG
	dbg_printf("\nread \n");
	if (is_aligned(dst, 4) || is_aligned(len, 4)) {
		dbg_printf("\n aligned read : \n");
	} else {
		dbg_printf("\n misaligned read : \n");
	}
	#endif
}

/*static void asyncCardRead_arm9(void) {
	u32 src = *(vu32*)(sharedAddr + 2);
	u32 dst = *(vu32*)(sharedAddr);
	u32 len = *(vu32*)(sharedAddr + 1);
	#ifdef DEBUG
	u32 marker = *(vu32*)(sharedAddr + 3);

	dbg_printf("\nasync card read received\n");

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

	asyncCardReadLED(true);    // When a file is loading, turn on LED for async card read indicator
	#ifdef DEBUG
	nocashMessage("fileRead romFile");
	#endif
	fileRead((char*)dst, *romFile, src, len, 0);
	asyncCardReadLED(false);    // After loading is done, turn off LED for async card read indicator

	#ifdef DEBUG
	dbg_printf("\nread \n");
	if (is_aligned(dst, 4) || is_aligned(len, 4)) {
		dbg_printf("\n aligned read : \n");
	} else {
		dbg_printf("\n misaligned read : \n");
	}
	#endif
}*/

static void runCardEngineCheck(void) {
	//dbg_printf("runCardEngineCheck\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheck");
	#endif	

	if (cardReadTimeOut == 30 && tryLockMutex(&cardEgnineCommandMutex)) {
		initialize();

		//nocashMessage("runCardEngineCheck mutex ok");

		if (*(vu32*)(0x027FFB14) == (vu32)0x026FF800) {
			log_arm9();
			*(vu32*)(0x027FFB14) = 0;
		}

		if (*(vu32*)(0x027FFB14) == (vu32)0x025FFB08) {
			cardRead_arm9();
			*(vu32*)(0x027FFB14) = 0;
		}

		/*if (*(vu32*)(0x027FFB14) == (vu32)0x020FF800) {
			asyncCardRead_arm9();
			*(vu32*)(0x027FFB14) = 0;
		}*/
		unlockMutex(&cardEgnineCommandMutex);
	}
}

static void runCardEngineCheckHalt(void) {
	//dbg_printf("runCardEngineCheckHalt\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheckHalt");
	#endif	

	// lockMutex should be possible to be used here instead of tryLockMutex since the execution of irq is not blocked
	// to be checked
	if (lockMutex(&cardEgnineCommandMutex)) {
		initialize();

		if (soundFix) {
			cardReadTimeOut = 0;
		}

		//nocashMessage("runCardEngineCheck mutex ok");
		if (*(vu32*)(0x027FFB14) == (vu32)0x026FF800) {
			log_arm9();
			*(vu32*)(0x027FFB14) = 0;
		}

		if (*(vu32*)(0x027FFB14) == (vu32)0x025FFB08) {
			cardRead_arm9();
			*(vu32*)(0x027FFB14) = 0;
		}

		/*if (*(vu32*)(0x027FFB14) == (vu32)0x020FF800) {
			asyncCardRead_arm9();
			*(vu32*)(0x027FFB14) = 0;
		}*/
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
void mySwiHalt(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("mySwiHalt");
	#endif	
	
	calledViaIPC = false;

	runCardEngineCheckHalt();
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

	if (!ROMinRAM) {
		runCardEngineCheck();
	}

	if (soundFix) {
		if (*(vu32*)(0x027FFB14) != 0 && cardReadTimeOut != 30) {
			cardReadTimeOut++;
		}
	} else {
		cardReadTimeOut = 30;
	}

	if (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B)) {
		softResetTimer = 0;
	} else { 
		if (softResetTimer == 60 * 2) {
			if (saveTimer == 0) {
				memcpy((u32*)0x02000300, sr_data_srloader, 0x020);
				i2cWriteRegister(0x4A, 0x70, 0x01);
				i2cWriteRegister(0x4A, 0x11, 0x01);	// Reboot into DSiMenu++
			}
		}
		softResetTimer++;
	}

	if (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT)) {
	} else if (!gameSoftReset) {
		if (saveTimer == 0) {
			memcpy((u32*)0x02000300, dsiMode ? sr_data_srllastran_twltouch : sr_data_srllastran, 0x020); // SDK 5
			i2cWriteRegister(0x4A, 0x70, 0x01);
			i2cWriteRegister(0x4A, 0x11, 0x01);	// Reboot game
		}
	}

	if (gottenSCFGExt == 0) {
		// Control volume with the - and + buttons.
		u8 volLevel;
		u8 i2cVolLevel = i2cReadRegister(0x4A, 0x40);
		if (consoleModel >= 2) {
			switch(i2cVolLevel) {
				case 0x00:
				case 0x01:
				default:
					volLevel = 0;
					break;
				case 0x02:
				case 0x03:
					volLevel = 1;
					break;
				case 0x04:
				case 0x05:
					volLevel = 2;
					break;
				case 0x06:
				case 0x07:
					volLevel = 3;
					break;
				case 0x08:
				case 0x09:
					volLevel = 4;
					break;
				case 0x0A:
				case 0x0B:
					volLevel = 5;
					break;
				case 0x0C:
				case 0x0D:
					volLevel = 6;
					break;
				case 0x0E:
				case 0x0F:
					volLevel = 7;
					break;
				case 0x10:
				case 0x11:
					volLevel = 8;
					break;
				case 0x12:
				case 0x13:
					volLevel = 9;
					break;
				case 0x14:
				case 0x15:
					volLevel = 10;
					break;
				case 0x16:
				case 0x17:
					volLevel = 11;
					break;
				case 0x18:
				case 0x19:
					volLevel = 12;
					break;
				case 0x1A:
				case 0x1B:
					volLevel = 13;
					break;
				case 0x1C:
				case 0x1D:
					volLevel = 14;
					break;
				case 0x1E:
				case 0x1F:
					volLevel = 15;
					break;
			}
		} else {
			switch(i2cVolLevel) {
				case 0x00:
				case 0x01:
				default:
					volLevel = 0;
					break;
				case 0x02:
				case 0x03:
					volLevel = 1;
					break;
				case 0x04:
					volLevel = 2;
					break;
				case 0x05:
					volLevel = 3;
					break;
				case 0x06:
					volLevel = 4;
					break;
				case 0x07:
					volLevel = 6;
					break;
				case 0x08:
					volLevel = 8;
					break;
				case 0x09:
					volLevel = 10;
					break;
				case 0x0A:
					volLevel = 12;
					break;
				case 0x0B:
					volLevel = 15;
					break;
				case 0x0C:
					volLevel = 17;
					break;
				case 0x0D:
					volLevel = 21;
					break;
				case 0x0E:
					volLevel = 24;
					break;
				case 0x0F:
					volLevel = 28;
					break;
				case 0x10:
					volLevel = 32;
					break;
				case 0x11:
					volLevel = 36;
					break;
				case 0x12:
					volLevel = 40;
					break;
				case 0x13:
					volLevel = 45;
					break;
				case 0x14:
					volLevel = 50;
					break;
				case 0x15:
					volLevel = 55;
					break;
				case 0x16:
					volLevel = 60;
					break;
				case 0x17:
					volLevel = 66;
					break;
				case 0x18:
					volLevel = 71;
					break;
				case 0x19:
					volLevel = 78;
					break;
				case 0x1A:
					volLevel = 85;
					break;
				case 0x1B:
					volLevel = 91;
					break;
				case 0x1C:
					volLevel = 100;
					break;
				case 0x1D:
					volLevel = 113;
					break;
				case 0x1E:
					volLevel = 120;
					break;
				case 0x1F:
					volLevel = 127;
					break;
			}
		}
		REG_MASTER_VOLUME = volLevel;
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
	}

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

	initialize();
	fileRead(dst, *savFile, src, len, -1);
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
	
	initialize();
	if (saveTimer == 0) {
		i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
	}
	saveTimer = 1;
	fileWrite(src, *savFile, dst, len, -1);

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

	initialize();
	if (saveTimer == 0) {
		i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
	}
	saveTimer = 1;
	fileWrite(src, *savFile, dst, len, -1);

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
	
	if (ROMinRAM) {
		memcpy(dst, romLocation + src, len);
	} else {
		initialize();
		cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
		//ndmaUsed = false;
		#ifdef DEBUG	
		nocashMessage("fileRead romFile");
		#endif	
		fileRead(dst, *romFile, src, len, 2);
		//ndmaUsed = true;
		cardReadLED(false);    // After loading is done, turn off LED for card read indicator
	}
	
	return true;
}
