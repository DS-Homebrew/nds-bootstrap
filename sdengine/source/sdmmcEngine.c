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
#include "my_sdmmc.h"
#include "sdmmcEngine.h"
#include "i2c.h"

extern int tryLockMutex(int* addr);
extern int lockMutex(int* addr);
extern int unlockMutex(int* addr);

static bool initialized = false;
extern volatile IntFn* volatile irqHandler; // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq
extern vu32* volatile irqSig; // always NULL
extern vu32* volatile commandAddr;

static int cardEgnineCommandMutex = 0;

void sendValue32(vu32 value32) {
	//nocashMessage("sendValue32");
	commandAddr[0] = (u32)0x027FEE08;
	commandAddr[1] = value32;
}

void getDatamsg(int size, vu8* msg) {
	for(int i=0;i<size;i++)  {
		msg[i]=*((vu8*)commandAddr+8+i);
	}	
}

//---------------------------------------------------------------------------------
void sdmmcCustomValueHandler(u32 value) {
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
void sdmmcCustomMsgHandler(int bytes) {
//---------------------------------------------------------------------------------
    FifoMessage msg;
    int retval = 0;
	//char buf[64];

    getDatamsg(bytes, (u8*)&msg);
    
    switch (msg.type) {

    case SDMMC_SD_READ_SECTORS:
        retval = my_sdmmc_sdcard_readsectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer, 0);
        break;
    case SDMMC_SD_WRITE_SECTORS:
        retval = my_sdmmc_sdcard_writesectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer, -1);
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

//---------------------------------------------------------------------------------
void SyncHandler(void) {
//---------------------------------------------------------------------------------
	//nocashMessage("SyncHandler");
	runSdMmcEngineCheck();
}

//---------------------------------------------------------------------------------
void checkIRQ_IPC_SYNC() {
//---------------------------------------------------------------------------------
	if(!initialized) {	
		//nocashMessage("!initialized");	
		u32* current=irqHandler+1;
		
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
	
	if (REG_SCFG_EXT == 0) {
		// Control volume with the - and + buttons.
		u8 volLevel;
		u8 i2cVolLevel = i2cReadRegister(0x4A, 0x40);
		switch(i2cVolLevel) {
			case 0x00:
			default:
				volLevel = 0;
				break;
			case 0x01:
				volLevel = 0;
				break;
			case 0x02:
				volLevel = 1;
				break;
			case 0x03:
				volLevel = 1;
				break;
			case 0x04:
				volLevel = 3;
				break;
			case 0x05:
				volLevel = 3;
				break;
			case 0x06:
				volLevel = 6;
				break;
			case 0x07:
				volLevel = 6;
				break;
			case 0x08:
				volLevel = 10;
				break;
			case 0x09:
				volLevel = 10;
				break;
			case 0x0A:
				volLevel = 15;
				break;
			case 0x0B:
				volLevel = 15;
				break;
			case 0x0C:
				volLevel = 21;
				break;
			case 0x0D:
				volLevel = 21;
				break;
			case 0x0E:
				volLevel = 28;
				break;
			case 0x0F:
				volLevel = 28;
				break;
			case 0x10:
				volLevel = 36;
				break;
			case 0x11:
				volLevel = 36;
				break;
			case 0x12:
				volLevel = 45;
				break;
			case 0x13:
				volLevel = 45;
				break;
			case 0x14:
				volLevel = 55;
				break;
			case 0x15:
				volLevel = 55;
				break;
			case 0x16:
				volLevel = 66;
				break;
			case 0x17:
				volLevel = 66;
				break;
			case 0x18:
				volLevel = 78;
				break;
			case 0x19:
				volLevel = 78;
				break;
			case 0x1A:
				volLevel = 91;
				break;
			case 0x1B:
				volLevel = 91;
				break;
			case 0x1C:
				volLevel = 105;
				break;
			case 0x1D:
				volLevel = 105;
				break;
			case 0x1E:
				volLevel = 120;
				break;
			case 0x1F:
				volLevel = 120;
				break;
		}
		REG_MASTER_VOLUME = volLevel;
	}

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


