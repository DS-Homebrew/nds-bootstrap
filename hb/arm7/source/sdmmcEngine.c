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

#include <nds/debug.h>
#include <nds/fifomessages.h>
#include <nds/fifocommon.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>
#include "sdmmc.h"
#include "debugToFile.h"
#include "sdmmcEngine.h"
#include "fat.h"
#include "i2c.h"

void sendValue32(vu32* commandAddr, u32 value32) {
	nocashMessage("sendValue32");
	commandAddr[1] = value32;
	commandAddr[0] = (u32)0x027FEE08;
}

void getDatamsg(vu32* commandAddr, int size, u8* msg) {
	for(int i=0;i<size;i++)  {
		msg[i]=*((u8*)commandAddr+8+i);
	}
}

//---------------------------------------------------------------------------------
void sdmmcCustomValueHandler(vu32* commandAddr, u32 value) {
//---------------------------------------------------------------------------------
    int result = 0;

    int oldIME = enterCriticalSection();

    switch(value) {

    case SDMMC_HAVE_SD:
        result = sdmmc_read16(REG_SDSTATUS0);
        break;

    case SDMMC_SD_START:
        nocashMessage("SDMMC_SD_START");
        /*if (sdmmc_read16(REG_SDSTATUS0) == 0) {
            result = 1;
        } else { */

            sdmmc_init();
            result = SD_Init();
        //}
        break;

    case SDMMC_SD_IS_INSERTED:
        result = 1;
        break;

    case SDMMC_SD_STOP:
        break;
	}
    leaveCriticalSection(oldIME);

    sendValue32(commandAddr, result);
}

//---------------------------------------------------------------------------------
void sdmmcCustomMsgHandler(vu32* commandAddr, int bytes) {
//---------------------------------------------------------------------------------
    FifoMessage msg;
    int retval = 0;
	char buf[64];

    getDatamsg(commandAddr, bytes, (u8*)&msg);

    int oldIME = enterCriticalSection();
    switch (msg.type) {

    case SDMMC_SD_READ_SECTORS:
		nocashMessage("msg SDMMC_SD_READ_SECTORS received");
		//siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
		nocashMessage(buf);
        retval = my_sdmmc_sdcard_readsectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer, -1);
        break;
    case SDMMC_SD_WRITE_SECTORS:
		nocashMessage("msg SDMMC_SD_WRITE_SECTORS received");
		//siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
		nocashMessage(buf);
        retval = my_sdmmc_sdcard_writesectors(msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer, -1);
        break;
    }

    leaveCriticalSection(oldIME);

    sendValue32(commandAddr, retval);
}

void runSdMmcEngineCheck (vu32* commandAddr)
{
	int oldIME = enterCriticalSection();
	nocashMessage("runSdMmcEngineCheck");
	if(*((vu32*)myMemUncached(commandAddr)) == (u32)0x027FEE04)
	{
		nocashMessage("sdmmc value received");
		sdmmcCustomValueHandler((vu32*)myMemUncached(commandAddr), ((vu32*)myMemUncached(commandAddr))[1]);
	} else if(*((vu32*)myMemUncached(commandAddr)) == (u32)0x027FEE05)
	{
		nocashMessage("sdmmc msg received");
		sdmmcCustomMsgHandler((vu32*)myMemUncached(commandAddr), ((vu32*)myMemUncached(commandAddr))[1]);
	}
    leaveCriticalSection(oldIME);
}
