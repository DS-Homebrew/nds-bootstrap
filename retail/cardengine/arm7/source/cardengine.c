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

#include "tonccpy.h"
#include "my_sdmmc.h"
#include "my_fat.h"
#include "locations.h"
#include "module_params.h"
#include "debug_file.h"
#include "cardengine.h"
#include "nds_header.h"

#include "sr_data_error.h"      // For showing an error screen
#include "sr_data_srloader.h"   // For rebooting into DSiMenu++
#include "sr_data_srllastran.h" // For rebooting the game
#include "sr_data_srllastran_twltouch.h" // SDK 5 --> For rebooting the game (TWL-mode touch screen)

static const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

extern vu32* volatile cardStruct;
extern u32 fileCluster;
extern u32 saveCluster;
extern module_params_t* moduleParams;
extern u32 gameOnFlashcard;
extern u32 language;
extern u32 gottenSCFGExt;
extern u32 dsiMode;
extern u32 ROMinRAM;
extern u32 consoleModel;
extern u32 romread_LED;
extern u32 gameSoftReset;
extern u32 preciseVolumeControl;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

bool sdRead = true;	// Unused, but added to prevent errors

static bool initialized = false;
//static bool initializedIRQ = false;
static bool calledViaIPC = false;
static bool dmaLed = false;

static aFile* romFile = (aFile*)ROM_FILE_LOCATION;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION;

static int saveTimer = 0;

static int softResetTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;

//static bool ndmaUsed = false;

static int cardEgnineCommandMutex = 0;
static int saveMutex = 0;

static const tNDSHeader* ndsHeader = NULL;
static const char* romLocation = NULL;

static void unlaunchSetHiyaBoot(void) {
	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) = (BIT(0) | BIT(1));		// Load the title at 2000838h
													// Use colors 2000814h
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
}

static bool isSdEjected(void) {
	if (*(vu32*)(0x400481C) & BIT(3)) {
		return true;
	}
	return false;
}

static void initialize(void) {
	if (initialized) {
		return;
	}
	
	if (sdmmc_read16(REG_SDSTATUS0) != 0) {
		sdmmc_init();
		SD_Init();
	}
	FAT_InitFiles(false, 0);
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
	/*if (strncmp(getRomTid(ndsHeader), "BO5", 3) == 0
        || strncmp(getRomTid(ndsHeader), "TBR", 3) == 0) {
		ndsHeader = (tNDSHeader*)(NDS_HEADER_4MB);
	}*/

	initialized = true;
}

static void cardReadLED(bool on) {
	if (consoleModel < 2) {
		if (dmaLed) {
			if (on) {
				switch(romread_LED) {
					case 0:
					default:
						break;
					case 1:
						i2cWriteRegister(0x4A, 0x63, 0xFF);    // Turn power LED purple
						break;
					case 2:
					case 3:
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
					case 3:
						i2cWriteRegister(0x4A, 0x30, 0x12);    // Turn WiFi LED off
						break;
				}
			}
		} else {
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
		initialize();
	    cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
		fileRead(memory, *savFile, flash, len, -1);
    	cardReadLED(false);
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
		initialize();
		if (saveTimer == 0) {
			i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		}
		saveTimer = 1;
	    cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
		fileWrite(memory, *savFile, flash, len, -1);
    	cardReadLED(false);
  		unlockMutex(&saveMutex);
	}    
}

static bool readOngoing = false;

static bool start_cardRead_arm9(void) {
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
        readOngoing = true;
        return false;    
        //while(!resumeFileRead()){}
    } 
    else
    {
        readOngoing = false;
        // Primary fix for Mario's Holiday
		// TODO: Apply fix outside of RAM cache
    	if (*(u32*)(0x0C9328AC) == 0x4B434148) {
    		*(u32*)(0x0C9328AC) = 0xA00;
    	}
        cardReadLED(false);    // After loading is done, turn off LED for card read indicator
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
        // Primary fix for Mario's Holiday
		// TODO: Apply fix outside of RAM cache
    	if (*(u32*)(0x0C9328AC) == 0x4B434148) {
    		*(u32*)(0x0C9328AC) = 0xA00;
    	}
        cardReadLED(false);    // After loading is done, turn off LED for card read indicator
        return true;    
    } 
    else
    {
        return false;    
    }
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

static void runCardEngineCheckResume(void) {
	//dbg_printf("runCardEngineCheckResume\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheckResume");
	#endif	

  	if (tryLockMutex(&cardEgnineCommandMutex)) {
  		initialize();
  
		if(readOngoing)
		{
			if(resume_cardRead_arm9()) {
				*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
                IPC_SendSync(0xEE28);
			} 
		}
  		unlockMutex(&cardEgnineCommandMutex);
  	}
}

static void runCardEngineCheck(void) {
	//dbg_printf("runCardEngineCheck\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheck");
	#endif	

  	if (tryLockMutex(&cardEgnineCommandMutex)) {
		//*(vu32*)(0x027FFB30) = (vu32)isSdEjected();
		if (isSdEjected()) {
			tonccpy((u32*)0x02000300, sr_data_error, 0x020);
			i2cWriteRegister(0x4A, 0x70, 0x01);
			i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into error screen if SD card is removed
		}
  		initialize();
  
        if(!readOngoing)
        { 
    
    		//nocashMessage("runCardEngineCheck mutex ok");
    
  		/*if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x5245424F) {
  			i2cWriteRegister(0x4A, 0x70, 0x01);
  			i2cWriteRegister(0x4A, 0x11, 0x01);
  		}*/
  
    		if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x026FF800) {
    			log_arm9();
    			*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
                IPC_SendSync(0xEE28);
    		}
    
    
          if ((*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFB08) || (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFB0A)) {
              dmaLed = (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFB0A);
              if(start_cardRead_arm9()) {
                    *(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
                    IPC_SendSync(0xEE28);
              } 
          }
          
            if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFC01) {
                dmaLed = (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFC01);
    			nandRead();
    			*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
    			IPC_SendSync(0xEE28);
    		}
            
            if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFC02) {
                dmaLed = (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFC02);
    			nandWrite();
    			*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
    			IPC_SendSync(0xEE28);
    		}
    
    		/*if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x020FF800) {
    			asyncCardRead_arm9();
    			*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
    		}*/
        } else {
            if(resume_cardRead_arm9()) {
                *(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
                IPC_SendSync(0xEE28);
            } 
        }
  		unlockMutex(&cardEgnineCommandMutex);
  	}
}

/*static void runCardEngineCheckAlt(void) {
	//dbg_printf("runCardEngineCheck\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheck");
	#endif	

  	if (lockMutex(&cardEgnineCommandMutex)) {
  		initialize();
  
      if(!readOngoing)
      { 
  
  		//nocashMessage("runCardEngineCheck mutex ok");
  
  		if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x026FF800) {
  			log_arm9();
  			*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
  		}
  
  
      		if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x025FFB08) {
      			if(start_cardRead_arm9()) {
                    *(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
                } else {
                    while(!resume_cardRead_arm9()) {} 
                    *(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
                } 			
      		}
  
  		//if (*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) == (vu32)0x020FF800) {
  		//	asyncCardRead_arm9();
  		//	*(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0;
  		//}
        } else {
            while(!resume_cardRead_arm9()) {} 
            *(vu32*)(CARDENGINE_SHARED_ADDRESS+0xC) = 0; 
        }
  		unlockMutex(&cardEgnineCommandMutex);
  	}
}*/

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
void myIrqHandlerTimer(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerTimer");
	#endif	
	
	calledViaIPC = false;

	runCardEngineCheckResume();
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

	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B))) {
		if (tryLockMutex(&saveMutex)) {
			if ((softResetTimer == 60 * 2) && (saveTimer == 0)) {
				if (consoleModel < 2) {
					unlaunchSetHiyaBoot();
				}
				tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
				i2cWriteRegister(0x4A, 0x70, 0x01);
				i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into TWiLight Menu++
			}
			unlockMutex(&saveMutex);
		}
		softResetTimer++;
	} else {
		softResetTimer = 0;
	}

	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT)) && !gameSoftReset && saveTimer == 0) {
		if (tryLockMutex(&saveMutex)) {
			if (consoleModel < 2) {
				unlaunchSetHiyaBoot();
			}
			//tonccpy((u32*)0x02000300, dsiMode ? sr_data_srllastran_twltouch : sr_data_srllastran, 0x020); // SDK 5
			tonccpy((u32*)0x02000300, sr_data_srllastran, 0x020);
			i2cWriteRegister(0x4A, 0x70, 0x01);
			i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot game
			unlockMutex(&saveMutex);
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

	if (consoleModel < 2) {
	if (preciseVolumeControl && romread_LED == 0) {
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

	const char* romTid = getRomTid(ndsHeader);
	if (strncmp(romTid, "UOR", 3) == 0
	|| strncmp(romTid, "UXB", 3) == 0
	|| (!ROMinRAM && !gameOnFlashcard)) {
		runCardEngineCheck();
	}
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

	if (isSdEjected()) {
		return false;
	}

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
	
	if (isSdEjected()) {
		return false;
	}

  	if (tryLockMutex(&saveMutex)) {
		initialize();
		if (saveTimer == 0) {
			i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		}
		saveTimer = 1;
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

	if (isSdEjected()) {
		return false;
	}

  	if (tryLockMutex(&saveMutex)) {
		initialize();
		if (saveTimer == 0) {
			i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
		}
		saveTimer = 1;
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
	//fileWrite(src, savFile, dst, len, -1);
	//i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
	return true;
}

bool eepromPageErase (u32 dst) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageErase\n");	
	#endif	
	
	if (isSdEjected()) {
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
bool cardInitialized = false;
u32 cardId(void) {
	#ifdef DEBUG	
	dbg_printf("\ncardId\n");
	#endif
    
    u32 cardid = getChipId(ndsHeader, moduleParams);
        
    //if (!cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0xE080FF80; // golden sun
    //if (!cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0x80FF80E0; // golden sun
    //if (cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0xFF000000; // golden sun
    //if (cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0x000000FF; // golden sun

    cardInitialized= true;
    
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
	
	if (ROMinRAM) {
		tonccpy(dst, romLocation + src, len);
	} else if (gameOnFlashcard) {
		return true;
	} else {
		initialize();
		cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
		//ndmaUsed = false;
		#ifdef DEBUG	
		nocashMessage("fileRead romFile");
		#endif	
		fileRead(dst, *romFile, src, len, 0);
		//ndmaUsed = true;
		cardReadLED(false);    // After loading is done, turn off LED for card read indicator
	}
	
	return true;
}
