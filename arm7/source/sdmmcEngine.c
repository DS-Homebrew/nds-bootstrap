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
#include <nds/arm7/sdmmc.h>
#include <nds/fifomessages.h>

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
        if (sdmmc_read16(REG_SDSTATUS0) == 0) {
            result = 1;
        } else {
            sdmmc_controller_init();
            result = sdmmc_sdcard_init();
        }
        break;
		
	case SDMMC_NAND_START:
		// disable nand start by default until no$gba handle it correctly
        if (sdmmc_read16(REG_SDSTATUS0) == 0) {
            result = 1;
        } else {
            sdmmc_controller_init();
            result = sdmmc_nand_init();
        }
        break;

    case SDMMC_SD_IS_INSERTED:
        result = sdmmc_cardinserted();
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
	
	struct mmcdevice deviceSD =*getMMCDevice(1);

    getDatamsg(commandAddr, bytes, (u8*)&msg);

    int oldIME = enterCriticalSection();
    switch (msg.type) {

    case SDMMC_SD_READ_SECTORS:
		nocashMessage("msg SDMMC_SD_READ_SECTORS received");
		siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
		nocashMessage(buf);
        retval = sdmmc_readsectors(&deviceSD, msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    case SDMMC_SD_WRITE_SECTORS:
		nocashMessage("msg SDMMC_SD_WRITE_SECTORS received");
		siprintf(buf, "%X-%X-%X", msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
		nocashMessage(buf);
        retval = sdmmc_writesectors(&deviceSD, msg.sdParams.startsector, msg.sdParams.numsectors, msg.sdParams.buffer);
        break;
    }

    leaveCriticalSection(oldIME);

    sendValue32(commandAddr, retval);
}

void runSdMmcEngineCheck (vu32* commandAddr)
{
	//nocashMessage("runSdMmcEngineCheck");
	if(*commandAddr == (u32)0x027FEE04)
	{
		nocashMessage("sdmmc value received");
		sdmmcCustomValueHandler(commandAddr, commandAddr[1]);
	} else if(*commandAddr == (u32)0x027FEE05)
	{
		nocashMessage("sdmmc msg received");
		sdmmcCustomMsgHandler(commandAddr, commandAddr[1]);
	}
}
