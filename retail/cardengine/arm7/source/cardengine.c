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

#include "locations.h"
#include "module_params.h"
#include "unpatched_funcs.h"
#include "cardengine.h"
#include "nds_header.h"
#include "tonccpy.h"

#define RUMBLE_PAK			(*(vuint16 *)0x08000000)
#define WARIOWARE_PAK		(*(vuint16 *)0x080000C4)

#define	REG_EXTKEYINPUT	(*(vuint16*)0x04000136)

extern u32 ce7;

//static const char *unlaunchAutoLoadID = "AutoLoadInfo";
//static char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

//#define memcpy __builtin_memcpy

extern void ndsCodeStart(u32* addr);

extern vu32* volatile cardStruct;
extern module_params_t* moduleParams;
extern u32 language;
extern u32* languageAddr;
extern u16 igmHotkey;
extern u8 RumblePakType;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

static bool initialized = false;
static bool bootloaderCleared = false;
static bool funcsUnpatched = false;

//static int saveReadTimeOut = 0;

//static int saveTimer = 0;

/*static int softResetTimer = 0;
static int volumeAdjustDelay = 0;
static bool volumeAdjustActivated = false;*/

//static bool ndmaUsed = false;

bool ipcEveryFrame = false;
static bool swapScreens = false;

int RumbleTimer = 0;
int RumbleForce = 1;

//static int cardEgnineCommandMutex = 0;
//static int saveMutex = 0;
static int swapTimer = 0;
static int languageTimer = 0;
static bool halfVolume = false;
static int soundBuffer = 0;
static bool customMusic = false;
bool returnToMenu = false;
bool isSdk5Set = false;

static const tNDSHeader* ndsHeader = NULL;
static PERSONAL_DATA* personalData = NULL;
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
	while (sharedAddr[3] != (vu32)0);
	//saveReadTimeOut = 0;
}

static void initialize(void) {
	if (initialized) {
		return;
	}

	isSdk5Set = isSdk5(moduleParams);

	ndsHeader = (tNDSHeader*)(isSdk5Set ? NDS_HEADER_SDK5 : NDS_HEADER);
	personalData = (PERSONAL_DATA*)(isSdk5Set ? (u8*)NDS_HEADER_SDK5-0x180 : (u8*)NDS_HEADER-0x180);

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

void Rumble(int Frames) {
	if ((RumblePakType == 0) || (RumbleForce == 0)) return;

	if (RumblePakType == 1) WARIOWARE_PAK = 8; 	
	else if (RumblePakType == 2) RUMBLE_PAK = 2;

	if (RumbleForce == 1) RumbleTimer = Frames + 1;
	if (RumbleForce == 2) RumbleTimer = Frames * 20;

	return;
}

void StopRumble() {
	if (RumblePakType == 1) WARIOWARE_PAK = 0;
	else if (RumblePakType == 2) RUMBLE_PAK = 0;
}

void DoRumble() {
	if (RumbleTimer) 
	{
		RumbleTimer--;
		if (RumblePakType == 1) WARIOWARE_PAK = ((RumbleTimer % 2) ? 8 : 0); 	
		if (RumblePakType == 2) RUMBLE_PAK = ((RumbleTimer % 2) ? 2 : 0); 	
	}

	if (RumbleTimer == 1)
	{
		StopRumble();
		RumbleTimer = 0;
	}
}

void rebootConsole(void) {
	if (*(vu16*)0x4004700 != 0) {
		u8 readCommand = readPowerManagement(0x10);
		readCommand |= BIT(0);
		writePowerManagement(0x10, readCommand); // Reboot console
	} else {
		writePowerManagement(PM_CONTROL_REG,PM_SYSTEM_PWR);	// Shut down console
	}
	sharedAddr[3] = 0;
}

void reset(void) {
	u32 resetParam = (isSdk5Set ? RESET_PARAM_SDK5 : RESET_PARAM);
	if (sharedAddr[0] == 0x57495344 || *(u32*)resetParam == 0xFFFFFFFF) {
		rebootConsole();
	}

	register int i;
	
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

	initialized = false;
	funcsUnpatched = false;
	languageTimer = 0;

	while (sharedAddr[0] != 0x544F4F42) { // 'BOOT'
		if (sharedAddr[1] == 0x48495344) {  // 'DSIH'
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
			sharedAddr[1] = 0;
		}
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}

	// Start ARM7
	ndsCodeStart(ndsHeader->arm7executeAddress);
}


//---------------------------------------------------------------------------------
void myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	if (*(u32*)((u32)CHEAT_ENGINE_LOCATION_B4DS+0x3E8) != 0xCF000000) {
		volatile void (*cheatEngine)() = (volatile void*)CHEAT_ENGINE_LOCATION_B4DS+4;
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

	if (!funcsUnpatched && *(int*)(isSdk5Set ? 0x02FFFC3C : 0x027FFC3C) >= 60) {
		unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

		if (unpatchedFuncs->compressed_static_end) {
			*unpatchedFuncs->compressedFlagOffset = unpatchedFuncs->compressed_static_end;
		}

		if (!isSdk5Set) {
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
		}

		if (unpatchedFuncs->mpuDataOffset2) {
			if (isSdk5Set) {
				unpatchedFuncs->mpuDataOffset2[0] = 0xE3A0004A; // mov r0, #0x4A
				unpatchedFuncs->mpuDataOffset2[2] = 0xE3A0004A; // mov r0, #0x4A
				unpatchedFuncs->mpuDataOffset2[4] = 0xE3A0000A; // mov r0, #0x0A
			} else {
				*unpatchedFuncs->mpuDataOffset2 = unpatchedFuncs->mpuInitRegionOldData2;
			}
		}

		funcsUnpatched = true;
	}

	if (0 == (REG_KEYINPUT & igmHotkey) && 0 == (REG_EXTKEYINPUT & (((igmHotkey >> 10) & 3) | ((igmHotkey >> 6) & 0xC0)))) {
		inGameMenu();
	}

	if (sharedAddr[2] == 0x5053554D) { // 'MUSP'
		customMusic = true;
		sharedAddr[2] = 0;
	}

	if (sharedAddr[2] == 0x5353554D) { // 'MUSS'
		SCHANNEL_CR(15) &= ~SCHANNEL_ENABLE;
		soundBuffer = 0;
		customMusic = false;
		sharedAddr[2] = 0;
	}

	if (customMusic) {
		if (sharedAddr[2] == 0x5953554D) {
			IPC_SendSync(0x5);
		}
		if (!(SCHANNEL_CR(15) & SCHANNEL_ENABLE) && sharedAddr[2] != 0x5953554D) {
			sharedAddr[2] = 0x5953554D; // 'MUSY'
			IPC_SendSync(0x5);

			SCHANNEL_SOURCE(15) = 0x027F0000+(soundBuffer*0x2000);
			SCHANNEL_REPEAT_POINT(15) = 0;
			SCHANNEL_LENGTH(15) = 0x2000/4;
			SCHANNEL_TIMER(15) = SOUND_FREQ(22050);
			SCHANNEL_CR(15) = SCHANNEL_ENABLE | SOUND_VOL(127) | SOUND_PAN(63) | SOUND_FORMAT_8BIT | SOUND_ONE_SHOT;

			soundBuffer++;
			if (soundBuffer == 2) soundBuffer = 0;
		}
	}

	if (sharedAddr[3] == 0x424D5552) { // 'RUMB'
		RumbleForce = sharedAddr[1];
		Rumble(sharedAddr[0]);
		sharedAddr[3] = 0;
	}

	if (RumbleForce == 2) {
		for (int i = 0; i < 20; i++) {
			DoRumble();
		}
	} else {
		DoRumble();
	}

	if (sharedAddr[3] == (vu32)0x52534554) {
		reset();
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
	}*/

	if (0 == (REG_KEYINPUT & (KEY_SELECT | KEY_UP))) {
		halfVolume = false;
		REG_MASTER_VOLUME = 127;
	}
	if (0 == (REG_KEYINPUT & (KEY_SELECT | KEY_DOWN))) {
		halfVolume = true;
	}
	
	if (halfVolume) {
		REG_MASTER_VOLUME = 63;
	}

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

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq;
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

	// Send a command to the ARM9 to read the save
	u32 commandSaveRead = 0x53415652;

	// Write the command
	sharedAddr[0] = src;
	sharedAddr[1] = len;
	sharedAddr[2] = (vu32)dst;
	sharedAddr[3] = commandSaveRead;

	waitForArm9();

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
	
	// Send a command to the ARM9 to write the save
	u32 commandSaveWrite = 0x53415657;

	// Write the command
	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = (vu32)src;
	sharedAddr[3] = commandSaveWrite;

	waitForArm9();

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

	// Send a command to the ARM9 to write the save
	u32 commandSaveWrite = 0x53415657;

	// Write the command
	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = (vu32)src;
	sharedAddr[3] = commandSaveWrite;

	waitForArm9();

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
