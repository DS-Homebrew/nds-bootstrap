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

#include <nds.h> 
#include <nds/fifomessages.h>
#include "sdmmc.h"
#include "debugToFile.h"
#include "cardEngine.h"
#include "fat.h"
#include "i2c.h"

#include "sr_data_twloader.h"	// For showing an error screen

static bool initialized = false;
static bool initializedIRQ = false;
static bool calledViaIPC = false;
extern vu32* volatile cardStruct;
extern vu32* volatile cacheStruct;
extern u32 fileCluster;
extern u32 saveCluster;
extern u32 sdk_version;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
static aFile romFile;
static aFile savFile;

static bool timeoutRun = true;
static int timeoutTimer = 0;

void initLogging() {
	if(!initialized) {
		if (sdmmc_read16(REG_SDSTATUS0) != 0) {
			sdmmc_controller_init();
			sdmmc_sdcard_init();
		}
		FAT_InitFiles(false);
		romFile = getFileFromCluster(fileCluster);
		if(saveCluster>0)
			savFile = getFileFromCluster(saveCluster);
		else
			savFile.firstCluster = CLUSTER_FREE;
		buildFatTableCache(romFile);
		#ifdef DEBUG		
		aFile myDebugFile = getBootFileCluster ("NDSBTSRP.LOG");
		enableDebug(myDebugFile);
		dbg_printf("logging initialized\n");		
		dbg_printf("sdk version :");
		dbg_hexa(sdk_version);		
		dbg_printf("\n");	
		dbg_printf("rom file :");
		dbg_hexa(fileCluster);	
		dbg_printf("\n");	
		dbg_printf("save file :");
		dbg_hexa(saveCluster);	
		dbg_printf("\n");
		#endif			
		initialized=true;
	}
	
}

void cardReadLED (bool on) {
	u8 setting = i2cReadRegister(0x4A, 0x72);
	
	if(on) {
		switch(setting) {
			case 0x00:
			default:
				break;
			case 0x01:
				i2cWriteRegister(0x4A, 0x30, 0x13);    // Turn WiFi LED on
				break;
			case 0x02:
				i2cWriteRegister(0x4A, 0x63, 0xFF);    // Turn power LED purple
				break;
			case 0x03:
				i2cWriteRegister(0x4A, 0x31, 0x01);    // Turn Camera LED on
				break;
		}
	} else {
		switch(setting) {
			case 0x00:
			default:
				break;
			case 0x01:
				i2cWriteRegister(0x4A, 0x30, 0x12);    // Turn WiFi LED off
				break;
			case 0x02:
				i2cWriteRegister(0x4A, 0x63, 0x00);    // Revert power LED to normal
				break;
			case 0x03:
				i2cWriteRegister(0x4A, 0x31, 0x00);    // Turn Camera LED off
				break;
		}
	}
}

void runCardEngineCheck (void) {
	//dbg_printf("runCardEngineCheck\n");
	#ifdef DEBUG		
	nocashMessage("runCardEngineCheck");
	#endif	
	
	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_A | KEY_B | KEY_X | KEY_Y))) {
		memcpy((u32*)0x02000000,sr_data_twloader,0x560);
		i2cWriteRegister(0x4a,0x70,0x01);
		i2cWriteRegister(0x4a,0x11,0x01);
	}	
	
	if (timeoutRun) {
		u8 setting = i2cReadRegister(0x4A, 0x73);
		if (setting == 0x01) {
			timeoutTimer += 1;
			if (timeoutTimer == 60*2) {
				memcpy((u32*)0x02000000,sr_data_twloader,0x560);
				i2cWriteRegister(0x4a,0x70,0x01);
				i2cWriteRegister(0x4a,0x11,0x01);	// If on white screen for a while, the game is incompatible, so show an error screen
			}
		} else {
			timeoutRun = false;
		}
	}

	if(tryLockMutex()) {	
		initLogging();
		
		//nocashMessage("runCardEngineCheck mutex ok");
		
		if(*(vu32*)(0x027FFB14) == (vu32)0x026ff800)
		{			
			#ifdef DEBUG		
			u32 src = *(vu32*)(sharedAddr+2);
			u32 dst = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);
			u32 marker = *(vu32*)(sharedAddr+3);			

			dbg_printf("\ncard read received\n");			
				
			if(calledViaIPC) {
				dbg_printf("\ntriggered via IPC\n");
			}
			dbg_printf("\nstr : \n");
			dbg_hexa(cardStruct);		
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
			
			*(vu32*)(0x027FFB14) = 0;	
		}
		
		if(*(vu32*)(0x027FFB14) == (vu32)0x025FFB08)
		{
			u32 src = *(vu32*)(sharedAddr+2);
			u32 dst = *(vu32*)(sharedAddr);
			u32 len = *(vu32*)(sharedAddr+1);
			u32 marker = *(vu32*)(sharedAddr+3);
		
			#ifdef DEBUG		
			dbg_printf("\ncard read received v2\n");
			
			if(calledViaIPC) {
				dbg_printf("\ntriggered via IPC\n");
			}
			
			dbg_printf("\nstr : \n");
			dbg_hexa(cardStruct);		
			dbg_printf("\nsrc : \n");
			dbg_hexa(src);		
			dbg_printf("\ndst : \n");
			dbg_hexa(dst);
			dbg_printf("\nlen : \n");
			dbg_hexa(len);
			dbg_printf("\nmarker : \n");
			dbg_hexa(marker);			
			#endif		
			
			timeoutRun = false;	// If card read received, do not show error screen

			cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
			fileRead(dst,romFile,src,len);
			cardReadLED(false);    // After loading is done, turn off LED for card read indicator
			
			#ifdef DEBUG		
			dbg_printf("\nread \n");			
			if(is_aligned(dst,4) || is_aligned(len,4)) {
				dbg_printf("\n aligned read : \n");
			} else {
				dbg_printf("\n misaligned read : \n");
			}			
			#endif	
	
			*(vu32*)(0x027FFB14) = 0;		
		}
		unlockMutex();
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


void myIrqHandlerVBlank(void) {
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	
	
	calledViaIPC = false;
	
	runCardEngineCheck();
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

void irqIPCSYNCEnable() {	
	if(!initializedIRQ) {
		int oldIME = enterCriticalSection();	
		initLogging();	
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
}

// ARM7 Redirected function

bool eepromProtect (void) {
	#ifdef DEBUG		
	dbg_printf("\narm7 eepromProtect\n");
	#endif	
	
	return true;
}

bool eepromRead (u32 src, void *dst, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromRead\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa(src);		
	dbg_printf("\ndst : \n");
	dbg_hexa((u32)dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	
	
	fileRead(dst,savFile,src,len);
	
	return true;
}

bool eepromPageWrite (u32 dst, const void *src, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageWrite\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa((u32)src);		
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	

	i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
	fileWrite(src,savFile,dst,len);
	i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
	
	return true;
}

bool eepromPageProg (u32 dst, const void *src, u32 len) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageProg\n");	
	
	dbg_printf("\nsrc : \n");
	dbg_hexa((u32)src);		
	dbg_printf("\ndst : \n");
	dbg_hexa(dst);
	dbg_printf("\nlen : \n");
	dbg_hexa(len);
	#endif	

	i2cWriteRegister(0x4A, 0x12, 0x01);		// When we're saving, power button does nothing, in order to prevent corruption.
	fileWrite(src,savFile,dst,len);
	i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
	
	return true;
}

bool eepromPageVerify (u32 dst, const void *src, u32 len) {
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
	//fileWrite(src,savFile,dst,len);
	//i2cWriteRegister(0x4A, 0x12, 0x00);		// If saved, power button works again.
	return true;
}

bool eepromPageErase (u32 dst) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageErase\n");	
	#endif	
	
	return true;
}

u32 cardId (void) {
	#ifdef DEBUG	
	dbg_printf("\cardId\n");
	#endif	

	return	1;
}

bool cardRead (u32 dma,  u32 src, void *dst, u32 len) {
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
	
	timeoutRun = false;	// Do not show error screen

	cardReadLED(true);    // When a file is loading, turn on LED for card read indicator
	fileRead(dst,romFile,src,len);
	cardReadLED(false);    // After loading is done, turn off LED for card read indicator
	
	return true;
}




