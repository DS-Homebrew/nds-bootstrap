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
#include <nds/fifomessages.h>
#include <nds/fifocommon.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>
#include "tonccpy.h"
#include "my_sdmmc.h"
#include "sdmmcEngine.h"
//#include "i2c.h"

//#include "sr_data_error.h"      // For showing an error screen
//#include "sr_data_srloader.h"   // For rebooting into DSiMenu++
//#include "sr_data_srllastran.h" // For rebooting the game

//static const char *unlaunchAutoLoadID = "AutoLoadInfo";
//static char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};

extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

static bool initialized = false;
extern volatile IntFn* volatile irqHandler; // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq
extern vu32* volatile irqSig; // always NULL
extern vu32* volatile commandAddr;

static int cardEgnineCommandMutex = 0;

/*static int softResetTimer = 0;

static void unlaunchSetHiyaBoot(void) {
	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) |= BIT(0);		// Load the title at 2000838h
	*(u32*)(0x02000810) |= BIT(1);		// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < 14; i++) {
		*(u8*)(0x02000838+i2) = hiyaNdsPath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
		*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
	}
}*/

static void sendValue32(vu32 value32) {
	//nocashMessage("sendValue32");
	commandAddr[0] = (u32)0x027FEE08;
	commandAddr[1] = value32;
}

static inline void getDatamsg(int size, u8* msg) {
	tonccpy(msg, (u8*)commandAddr+8, size);
}

//---------------------------------------------------------------------------------
static void sdmmcCustomValueHandler(u32 value) {
//---------------------------------------------------------------------------------
    int result = 0;

    switch(value) {

    case SDMMC_HAVE_SD:
        result = sdmmc_read16(REG_SDSTATUS0);
        break;

    case SDMMC_SD_START:
        if (sdmmc_read16(REG_SDSTATUS0) == 0) {
            result = 1;
        } else {
            sdmmc_init();
            result = SD_Init();
        }
        break;

    case SDMMC_SD_IS_INSERTED:
        result = 1;
        break;

    case SDMMC_SD_STOP:
        break;
	}

    sendValue32(result);
}

//---------------------------------------------------------------------------------
static void sdmmcCustomMsgHandler(int bytes) {
//---------------------------------------------------------------------------------
    FifoMessage msg;
    int retval = 0;
	//char buf[64];

    getDatamsg(bytes, (u8*)&msg);

    switch (msg.type) {

    case SDMMC_SD_READ_SECTORS:
        retval = my_sdmmc_sdcard_readsectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    case SDMMC_SD_WRITE_SECTORS:
        retval = my_sdmmc_sdcard_writesectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    }

    sendValue32(retval);
}

void runSdMmcEngineCheck (void) {

  	if (tryLockMutex(&cardEgnineCommandMutex)) {
		if(*commandAddr == (vu32)0x027FEE04)
		{
			sdmmcCustomValueHandler(commandAddr[1]);
		} else if(*commandAddr == (vu32)0x027FEE05)
		{
			sdmmcCustomMsgHandler(commandAddr[1]);
		}
  		unlockMutex(&cardEgnineCommandMutex);
  	}
}

/*void runSdMmcEngineCheck2(void) {

  	if (lockMutex(&cardEgnineCommandMutex)) {
		if(*commandAddr == (vu32)0x027FEE04)
		{
			sdmmcCustomValueHandler(commandAddr[1]);
		} else if(*commandAddr == (vu32)0x027FEE05)
		{
			sdmmcCustomMsgHandler(commandAddr[1]);
		}
  		unlockMutex(&cardEgnineCommandMutex);
  	}
}*/

//---------------------------------------------------------------------------------
static void SyncHandler(void) {
//---------------------------------------------------------------------------------
	//nocashMessage("SyncHandler");
	runSdMmcEngineCheck();
}

//---------------------------------------------------------------------------------
static void checkIRQ_IPC_SYNC() {
//---------------------------------------------------------------------------------
	if(!initialized) {
		//nocashMessage("!initialized");
		u32* current=(u32*)irqHandler+1;

		while(*current!=IRQ_IPC_SYNC && *current!=0) {
			current+=2;
		}
		/*if(current==IRQ_IPC_SYNC) {
			nocashMessage("IRQ_IPC_SYNC slot found");
		} else {
			nocashMessage("empty irqtable slot found");
		}*/

		*((IntFn*)current-1)	= SyncHandler;
		*current				= IRQ_IPC_SYNC;

		//nocashMessage("IRQ_IPC_SYNC setted");

		initialized = true;
	}
}


void myIrqHandler(void) {

	/*if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_B))) {
		if (softResetTimer == 60 * 2) {
			//if (consoleModel < 2) {
				unlaunchSetHiyaBoot();
			//}
			tonccpy((u32*)0x02000300, sr_data_srloader, 0x020);
			i2cWriteRegister(0x4A, 0x70, 0x01);
			i2cWriteRegister(0x4A, 0x11, 0x01);		// Reboot into TWiLight Menu++
		}
		softResetTimer++;
	} else {
		softResetTimer = 0;
	}

	if ( 0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT))) {
		//if (consoleModel < 2) {
			unlaunchSetHiyaBoot();
		//}
		tonccpy((u32*)0x02000300, sr_data_srllastran, 0x020);
		i2cWriteRegister(0x4A, 0x70, 0x01);
		i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot game
	}*/

	checkIRQ_IPC_SYNC();
	runSdMmcEngineCheck();
}

void myIrqEnable(u32 irq) {
	int oldIME = enterCriticalSection();
	if (irq & IRQ_VBLANK)
		REG_DISPSTAT |= DISP_VBLANK_IRQ ;
	if (irq & IRQ_HBLANK)
		REG_DISPSTAT |= DISP_HBLANK_IRQ ;
	if (irq & IRQ_VCOUNT)
		REG_DISPSTAT |= DISP_YTRIGGER_IRQ;

	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;
	//nocashMessage("IRQ_IPC_SYNC enabled");

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
}
