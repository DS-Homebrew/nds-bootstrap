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
#include "cardEngine.h"

#define READ_SIZE_ARM7 0x8000
#define MARKER_ADDRESS_1 0x03740000
#define MARKER_ADDRESS_2 0x03746004
#define BUFFER_ADDRESS 0x03740000
#define REG_MBK_B	(*(vu8*)0x4004047)

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq
static u32 currentSector = 0;


u32 cardId (void) {
	//nocashMessage("\narm9 cardId\n");

	return	1;
}

void cardRead (u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");	
	
	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;
	u32 commandRead;
	u32 src = cardStruct[0];
	u8* dst = (u8*) (cardStruct[1]);
	u32 len = cardStruct[2];
	
	u32 page = (src/512)*512;
	
	u32 sector = (src/READ_SIZE_ARM7)*READ_SIZE_ARM7;
	
	/*// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;	
	
	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;
	
	IPC_SendSync(0xEE24);
	
	while(sharedAddr[3] != (vu32)0);
	// -------------------------------------*/

	
	if(page == src && len > READ_SIZE_ARM7 && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
		// read directly at arm7 level
		commandRead = 0x025FFB08;
		
		cacheFlush();
		
		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;
		
		IPC_SendSync(0xEE24);
		
		while(sharedAddr[3] != (vu32)0);
		
	} else {
		// read via the WRAM cache
		while(len > 0) {
			// read max 32k via the WRAM cache
			if(!currentSector || sector != currentSector) {
				// send a command to the arm7 to fill the WRAM cache
				commandRead = 0x025FFB08;
				
				DC_FlushRange((vu32*)BUFFER_ADDRESS, READ_SIZE_ARM7);
				//cacheFlush();
				
				// transfer the WRAM-B cache to the arm7
				REG_MBK_B=(vu8)0x81;					
				
				/*while(*((vu32*)MARKER_ADDRESS_1) == (vu32)0xDEADBABE) {
					DC_FlushRange((vu32*)MARKER_ADDRESS_1, 4);
				}
				
				while(*((vu32*)MARKER_ADDRESS_2) == (vu32)0xDEADBABE) {
					DC_FlushRange((vu32*)MARKER_ADDRESS_2, 4);
				}*/
				
				// write the command
				sharedAddr[0] = BUFFER_ADDRESS;
				sharedAddr[1] = READ_SIZE_ARM7;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;
				
				IPC_SendSync(0xEE24);	

				while(sharedAddr[3] != (vu32)0);	
				
				// transfer back the WRAM-B cache to the arm9
				REG_MBK_B=(vu8)0x80;
				
				/*DC_FlushRange((vu32*)MARKER_ADDRESS_1, 4);
				while(*((vu32*)MARKER_ADDRESS_1) != (vu32)0xDEADBABE) {
					DC_FlushRange((vu32*)MARKER_ADDRESS_1, 4);
				}
				
				DC_FlushRange((vu32*)MARKER_ADDRESS_2, 4);
				while(*((vu32*)MARKER_ADDRESS_2) != (vu32)0xDEADBABE) {
					DC_FlushRange((vu32*)MARKER_ADDRESS_2, 4);
				}*/
				
				currentSector = sector;
			}		
		
			u32 len2=len;
			if((src - currentSector) + len2 > READ_SIZE_ARM7){
				len2 = len2 - ((src - currentSector) + len2 - READ_SIZE_ARM7);
			}
			
			if((len2>512) && ((len2 % 32) == 0) && ((u32)dst)%4 == 0) {
				/*// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;	
				
				sharedAddr[0] = dst;
				sharedAddr[1] = len2;
				sharedAddr[2] = BUFFER_ADDRESS+src-sector;
				sharedAddr[3] = commandRead;
				
				IPC_SendSync(0xEE24);
				
				while(sharedAddr[3] != (vu32)0);
				// -------------------------------------*/
			
				// copy directly
				fastCopy32(BUFFER_ADDRESS+(src-currentSector),dst,len2);	
				
				// update cardi common
				cardStruct[0] = src + len2;
				cardStruct[1] = dst + len2;
				cardStruct[2] = len - len2;
			} else {			
				bool remainToRead = true;
				u32 src2 = cardStruct[0];
				while (remainToRead && (src2-currentSector < READ_SIZE_ARM7) ) {
					u32 page2 = (src2/512)*512;
					
					/*// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;	
					
					sharedAddr[0] = page2;
					sharedAddr[1] = len2;
					sharedAddr[2] = 0x03740000+page2-sector;
					sharedAddr[3] = commandRead;
					
					IPC_SendSync(0xEE24);
					
					while(sharedAddr[3] != (vu32)0);
					// -------------------------------------*/
						
					// read via the 512b ram cache
					fastCopy32(BUFFER_ADDRESS+(page2-currentSector), cacheBuffer, 512);
					*cachePage = page2;
					remainToRead = (*readCachedRef)(cacheStruct);
					src2 = cardStruct[0];
				}
			}
			if(len == len2) {
				len =0;
				/*// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;	
				
				sharedAddr[0] = dst;
				sharedAddr[1] = len;
				sharedAddr[2] = src;
				sharedAddr[3] = commandRead;
				
				IPC_SendSync(0xEE24);
				
				while(sharedAddr[3] != (vu32)0);
				// -------------------------------------*/
			
			} else {
				// bigger than 32k unaligned command
				len = cardStruct[2];
				src = cardStruct[0];
				dst = cardStruct[1];
				page = (src/512)*512;
				sector = (src/READ_SIZE_ARM7)*READ_SIZE_ARM7;
			}			
		}
	}	
}




