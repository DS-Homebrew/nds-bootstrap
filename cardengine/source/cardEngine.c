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
extern vu32* volatile commandAddr;
extern vu32* volatile cardStruct;
extern vu32* volatile cacheStruct;
extern u32 fileCluster;
vu32* volatile debugAddr = (vu32*)0x02100000;

void runCardEngineCheck (void) {
	//dbg_printf("runCardEngineCheck\n");
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
		//dbg_printf("card read received\n");

        u32 src = *(vu32*)(cardStruct+7);
        u32 dst = *(vu32*)(cardStruct+8);
        u32 len = *(vu32*)(cardStruct+9);
        
        
        
        dbg_printf("src : \n");
        dbg_hexa(src);   
        dbg_printf("dst : \n");
        dbg_hexa(dst);
        dbg_printf("len : \n");
        dbg_hexa(len);
        
        fileRead(0x02140000,fileCluster,src,len);
        
        //dbg_printf("read \n");
        
        *(vu32*)(0x2100000) = 0;
        
    }

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


