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

static bool initialized = false;
extern volatile IntFn* volatile irqHandler; // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq
extern vu32* volatile irqSig; // always NULL
extern vu32* volatile commandAddr;
extern vu32* volatile cardStruct;
extern vu32* volatile cacheStruct;
extern u32 fileCluster;
vu32* volatile debugAddr = (vu32*)0x02100000;

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
		FAT_InitFiles(false);
		u32 myDebugFile = getBootFileCluster ("NDSBTSRP.LOG");
		enableDebug(myDebugFile);
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

void runCardEngineCheck (void) {
	//dbg_printf("runSdMmcEngineCheck\n");
	int oldIME = enterCriticalSection();
	
	if(!initialized) {
		if (sdmmc_read16(REG_SDSTATUS0) != 0) {
			sdmmc_controller_init();
			sdmmc_sdcard_init();
		}
		FAT_InitFiles(false);
		u32 myDebugFile = getBootFileCluster ("NDSBTSRP.LOG");
		enableDebug(myDebugFile);
		dbg_printf("logging initialized\n");
		dbg_hexa(0x02100000);
		initialized=true;
	}

	if(*(vu32*)(0x02100000) == (vu32)0x027FEE04)
    {
        dbg_printf("card read received\n");
	}
	/*
	if(*debugAddr == (vu32)0x027FEE04)
	{
		dbg_printf("card read executed\n");
	}
	if(*commandAddr == (vu32)0x027FEE04)
	{
		dbg_printf("sdmmc value received\n");
		sdmmcCustomValueHandler(commandAddr[1]);
	} else if(*commandAddr == (vu32)0x027FEE05)
	{
		dbg_printf("sdmmc msg received\n");
		sdmmcCustomMsgHandler(commandAddr[1]);
	}*/

	leaveCriticalSection(oldIME);
}

//---------------------------------------------------------------------------------
void myIrqHandlerFIFO(void) {
//---------------------------------------------------------------------------------
	nocashMessage("myIrqHandlerFIFO");
	
	runCardEngineCheck();
}


void myIrqHandlerVBlank(void) {
	nocashMessage("myIrqHandlerVBlank");
	
	runCardEngineCheck();
}


