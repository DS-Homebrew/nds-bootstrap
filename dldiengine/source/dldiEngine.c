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
#include "fat.h"
#include "i2c.h"

static bool initialized = false;
extern volatile IntFn* volatile irqHandler; // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq
extern vu32* volatile irqSig; // always NULL
extern vu32* volatile commandAddr;

void sendValue32(vu32 value32) {
	nocashMessage("sendValue32");
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
            sdmmc_controller_init();
            result = sdmmc_sdcard_init();
        }
		//FAT_InitFiles(false);
		//u32 myDebugFile = getBootFileCluster ("NDSBTSRP.LOG");
		//enableDebug(myDebugFile);
        break;

    case SDMMC_SD_IS_INSERTED:
        result = sdmmc_cardinserted();
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
		dbg_printf("msg SDMMC_SD_READ_SECTORS received\n");
//		siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
//		nocashMessage(buf);
        retval = sdmmc_sdcard_readsectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    case SDMMC_SD_WRITE_SECTORS:
		dbg_printf("msg SDMMC_SD_WRITE_SECTORS received\n");
//		siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
//		nocashMessage(buf);
        retval = sdmmc_sdcard_writesectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    }    

    sendValue32(retval);
}

void runSdMmcEngineCheck (void) {
	//dbg_printf("runSdMmcEngineCheck\n");

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
	
	int oldIME = enterCriticalSection();

	if(*commandAddr == (vu32)0x027FEE04)
	{
		dbg_printf("sdmmc value received\n");
		sdmmcCustomValueHandler(commandAddr[1]);
	} else if(*commandAddr == (vu32)0x027FEE05)
	{
		dbg_printf("sdmmc msg received\n");
		sdmmcCustomMsgHandler(commandAddr[1]);
	}

	leaveCriticalSection(oldIME);
}

// interruptDispatcher.s jump_intr:
static const u32 homebrewSig[5] = {
	0xE5921000, // ldr    r1, [r2]        @ user IRQ handler address
	0xE3510000, // cmp    r1, #0
	0x1A000001, // bne    got_handler
	0xE1A01000, // mov    r1, r0
	0xEAFFFFF6  // b    no_handler
};	

// interruptDispatcher.s jump_intr:
//patch
static const u32 homebrewSigPatched[5] = {
	0xE59F1008, // ldr    r1, =0x23FF00C   @ my custom handler
	0xE5012008, // str    r2, [r1,#-8]     @ irqhandler
	0xE501F004, // str    pc, [r1,#-4]     @ irqsig 
	0xEA000000, // b      got_handler
	0x0390000C  // DCD 	  0x0390000C       
};

static u32* restoreInterruptHandlerHomebrew (u32* addr, u32 size) {
	dbg_printf("restoreInterruptHandlerHomebrew\n");	
	u32* end = addr + size/sizeof(u32);
	
	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewSigPatched[0]) && 
			(addr[1] == homebrewSigPatched[1]) && 
			(addr[2] == homebrewSigPatched[2]) && 
			(addr[3] == homebrewSigPatched[3]) && 
			(addr[4] == homebrewSigPatched[4])) 
		{
			break;
		}
		addr++;
	}
	
	if (addr >= end) {
		dbg_printf("addr >= end");	
		return 0;
	}
	
	// patch the program
	addr -= 5;
	addr[0] = homebrewSig[0];
	addr[1] = homebrewSig[1];
	addr[2] = homebrewSig[2];
	addr[3] = homebrewSig[3];
	addr[4] = homebrewSig[4];
	
	dbg_printf("restoreSuccessfull\n");	
	
	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}

//---------------------------------------------------------------------------------
void SyncHandler(void) {
//---------------------------------------------------------------------------------
	nocashMessage("SyncHandler");
	runSdMmcEngineCheck();
}

//---------------------------------------------------------------------------------
void checkIRQ_IPC_SYNC() {
//---------------------------------------------------------------------------------
	if(!initialized) {	
		nocashMessage("!initialized");	
		u32* current=irqHandler+1;
		
		while(*current!=IRQ_IPC_SYNC && *current!=0) {
			current+=2;
		}
		if(current==IRQ_IPC_SYNC) {
			nocashMessage("IRQ_IPC_SYNC slot found");	
		} else {
			*((IntFn*)current-1)	= SyncHandler;
			*current				= IRQ_IPC_SYNC;
		
			nocashMessage("IRQ_IPC_SYNC setted");
		}				
	
		initialized = true;
	}	
}


void myIrqHandler(void) {
	//dbg_printf("myIrqHandler\n");	
	
	checkIRQ_IPC_SYNC();
	runSdMmcEngineCheck();
}

void myIrqEnable(u32 irq) {	
	dbg_printf("myIrqEnable\n");
	int oldIME = enterCriticalSection();	
	if (irq & IRQ_VBLANK)
		REG_DISPSTAT |= DISP_VBLANK_IRQ ;
	if (irq & IRQ_HBLANK)
		REG_DISPSTAT |= DISP_HBLANK_IRQ ;
	if (irq & IRQ_VCOUNT)
		REG_DISPSTAT |= DISP_YTRIGGER_IRQ;
		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;
	nocashMessage("IRQ_IPC_SYNC enabled");

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
}


