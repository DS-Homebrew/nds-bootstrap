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

void sendValue32(u32 value32) {
	nocashMessage("sendValue32");
	*((vu32*)0x027FEE24) = (u32)0x027FEE08;
	*((vu32*)0x027FEE28) = value32;
}

void getDatamsg(int size, u8* msg) {
	for(int i=0;i<size;i++)  {
		msg[i]=*((u8*)0x027FEE2C+i);
	}	
}

//---------------------------------------------------------------------------------
void sdmmcCustomValueHandler(u32 value) {
//---------------------------------------------------------------------------------
    int result = 0;

    int oldIME = enterCriticalSection();

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
        break;

    case SDMMC_SD_IS_INSERTED:
        result = sdmmc_cardinserted();
        break;

    case SDMMC_SD_STOP:
        break;
	}
    leaveCriticalSection(oldIME);

    sendValue32(result);
}

//---------------------------------------------------------------------------------
void sdmmcCustomMsgHandler(int bytes) {
//---------------------------------------------------------------------------------
    FifoMessage msg;
    int retval = 0;
	//char buf[64];

    getDatamsg(bytes, (u8*)&msg);

    int oldIME = enterCriticalSection();
    switch (msg.type) {

    case SDMMC_SD_READ_SECTORS:
		nocashMessage("msg SDMMC_SD_READ_SECTORS received");
//		siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
//		nocashMessage(buf);
        retval = sdmmc_sdcard_readsectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    case SDMMC_SD_WRITE_SECTORS:
		nocashMessage("msg SDMMC_SD_WRITE_SECTORS received");
//		siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
//		nocashMessage(buf);
        retval = sdmmc_sdcard_writesectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    }

    leaveCriticalSection(oldIME);

    sendValue32(retval);
}

static bool initialized = false;
extern IntFn* irqHandler; // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq
extern u32* irqSig; // always NULL

void runSdMmcEngineCheck (void) {
	nocashMessage("runSdMmcEngineCheck");
	//nocashMessage("runSdMmcEngineCheck");
	if(*((vu32*)0x027FEE24) == (u32)0x027FEE04)
	{
		nocashMessage("sdmmc value received");
		sdmmcCustomValueHandler(*((vu32*)0x027FEE28));
	} else if(*((vu32*)0x027FEE24) == (u32)0x027FEE05)
	{
		nocashMessage("sdmmc msg received");
		sdmmcCustomMsgHandler(*((vu32*)0x027FEE28));
	}
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
	0xEA000001, // b      got_handler
	0x023FF00C  // DCD 	  0x23FF00C       
};

static u32* restoreInterruptHandlerHomebrew (u32* addr, u32 size) {
	nocashMessage("restoreInterruptHandlerHomebrew");	
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
		nocashMessage("addr >= end");	
		return 0;
	}
	
	// patch the program
	addr -= 5;
	addr[0] = homebrewSig[0];
	addr[1] = homebrewSig[1];
	addr[2] = homebrewSig[2];
	addr[3] = homebrewSig[3];
	addr[4] = homebrewSig[4];
	
	nocashMessage("restoreSuccessfull");	
	
	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}

void myIrqHandler(void) {
	nocashMessage("myIrqHandler");	
	
	u32 irq = *((u32*)irqHandler-4);
		
	nocashMessage("interrupt handler found");
	IntFn handler = *irqHandler;
	if(handler>0) handler();

	if(!initialized) {	
		u32* current=irqHandler+4;
		
		while(*current!=IRQ_IPC_SYNC || !*current) {
			current+=8;
		}
		
		*((IntFn*)current-4)	= handler;
		*current				= IRQ_IPC_SYNC;
	
		nocashMessage("IRQ_IPC_SYNC setted");
		
		REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;	
		REG_IE       |= IRQ_IPC_SYNC;
		
		nocashMessage("IRQ_IPC_SYNC enabled");	
		// restore the irq Handler for better compatibility
		restoreInterruptHandlerHomebrew(irqSig-8,24);
	}	
	runSdMmcEngineCheck();
}


