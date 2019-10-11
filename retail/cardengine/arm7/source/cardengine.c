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

#include "locations.h"
#include "module_params.h"
#include "cardengine.h"
#include "nds_header.h"
#include "tonccpy.h"

//static const char *unlaunchAutoLoadID = "AutoLoadInfo";
//static char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

//#define memcpy __builtin_memcpy

extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

extern vu32* volatile cardStruct;
extern module_params_t* moduleParams;
extern u32 language;
extern u32 dsiMode;
extern u32 ROMinRAM;
extern u32 consoleModel;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static bool initialized = false;

static int saveReadTimeOut = 0;

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

static void waitForArm9(void) {
    IPC_SendSync(0x4);
    int count = 0;
	while (sharedAddr[3] != (vu32)0) {
		if(count==20000000) {
			IPC_SendSync(0x4);
			count=0;
		}
		count++;
	}
	saveReadTimeOut = 0;
}

static void __attribute__((target("thumb"))) initialize(void) {
	if (initialized) {
		return;
	}

	toncset((u32*)0x023DA000, 0, 0x1000);	// Clear arm9 side of bootloader

	ndsHeader = (tNDSHeader*)(isSdk5(moduleParams) ? NDS_HEADER_SDK5 : NDS_HEADER);

	initialized = true;
}


//---------------------------------------------------------------------------------
void __attribute__((target("thumb"))) myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	initialize();

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

	if (sharedAddr[3] != 0) {
		saveReadTimeOut++;
		if (saveReadTimeOut > 60) {
			sharedAddr[3] = 0;		// Cancel save read/write, if arm9 does nothing
		}
	}

	/*#ifdef DEBUG
	nocashMessage("cheat_engine_start\n");
	#endif	
	
	cheat_engine_start();*/
}

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

  	if (lockMutex(&saveMutex)) {
		// Send a command to the ARM9 to read the save
		u32 commandSaveRead = 0x53415652;

		// Write the command
		sharedAddr[0] = src;
		sharedAddr[1] = len;
		sharedAddr[2] = (vu32)dst;
		sharedAddr[3] = commandSaveRead;

		waitForArm9();

		if ((u32)dst < 0x02000000 && (u32)dst >= 0x03000000) {
			// Transfer from main RAM to WRAM
			tonccpy((char*)dst, (char*)0x023E0000, len);
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
	
  	if (lockMutex(&saveMutex)) {
		if ((u32)src < 0x02000000 && (u32)src >= 0x03000000) {
			// Transfer from WRAM to main RAM
			tonccpy((char*)0x023E0000, (char*)src, len);
		}

		// Send a command to the ARM9 to write the save
		u32 commandSaveWrite = 0x53415657;

		// Write the command
		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = (vu32)src;
		sharedAddr[3] = commandSaveWrite;

		waitForArm9();

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

  	if (lockMutex(&saveMutex)) {
		if ((u32)src < 0x02000000 && (u32)src >= 0x03000000) {
			// Transfer from WRAM to main RAM
			tonccpy((char*)0x023E0000, (char*)src, len);
		}

		// Send a command to the ARM9 to write the save
		u32 commandSaveWrite = 0x53415657;

		// Write the command
		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = (vu32)src;
		sharedAddr[3] = commandSaveWrite;

		waitForArm9();

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

  	/*if (lockMutex(&saveMutex)) {
		// Send a command to the ARM9 to write the save
		u32 commandSaveWrite = 0x53415657;

		// Write the command
		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandSaveWrite;

		waitForArm9();

  		unlockMutex(&saveMutex);
	}*/
	return true;
}

bool eepromPageErase (u32 dst) {
	#ifdef DEBUG	
	dbg_printf("\narm7 eepromPageErase\n");	
	#endif	
	
	// TODO: this should be implemented?
	return true;
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
	
	// Send a command to the ARM9 to read the ROM
	u32 commandRomRead = 0x524F4D52;

	// Write the command
	sharedAddr[0] = src;
	sharedAddr[1] = len;
	sharedAddr[2] = (vu32)dst;
	sharedAddr[3] = commandRomRead;

	waitForArm9();

	return true;
}
