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

#define READ_SIZE_NDMA7 0x40000

#define CACHE_ADRESS_START 0x02800000
#define CACHE_ADRESS_SIZE 0x7F8000
#define REG_MBK_CACHE_SIZE	0x1F

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 cacheDescriptor [REG_MBK_CACHE_SIZE];
static u32 cacheCounter [REG_MBK_CACHE_SIZE];
static u32 accessCounter = 0;

void user_exception(void);

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	exceptionStack = (u32)0x23EFFFC ;
	EXCEPTION_VECTOR = enterException ;
	*exceptionC = user_exception;
}

int allocateCacheSlot() {
	int slot = 0;
	int lowerCounter = accessCounter;
	for(int i=0; i<REG_MBK_CACHE_SIZE; i++) {
		if(cacheCounter[i]<=lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int getSlotForSector(u32 sector) {
	for(int i=0; i<REG_MBK_CACHE_SIZE; i++) {
		if(cacheDescriptor[i]==sector) {
			return i;
		}
	}
	return -1;
}


vu8* getCacheAddress(int slot) {
	return (vu32*)(CACHE_ADRESS_START+slot*READ_SIZE_NDMA7);
}

void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

int cardRead (u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");
	
	REG_SCFG_EXT = 0x83008000;
	*(u32*)(0x2FFFFFC) = &cacheDescriptor;
	REG_SCFG_EXT = 0x83000000;
	
	setExceptionHandler2();
	
	accessCounter++;

	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;
	u32 commandRead;
	u32 src = cardStruct[0];
	u8* dst = (u8*) (cardStruct[1]);
	u32 len = cardStruct[2];

	u32 page = (src/512)*512;

	u32 sector = (src/READ_SIZE_NDMA7)*READ_SIZE_NDMA7;

	#ifdef DEBUG
	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	IPC_SendSync(0xEE24);

	while(sharedAddr[3] != (vu32)0);
	// -------------------------------------*/
	#endif


	REG_SCFG_EXT = 0x83008000;

	if(page == src && len > READ_SIZE_NDMA7 && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
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
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			// read max 32k via the WRAM cache
			if(slot==-1) {
				// send a command to the arm7 to fill the WRAM cache
				commandRead = 0x025FFB08;

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				if(needFlushDCCache) DC_FlushRange(buffer, READ_SIZE_NDMA7);

				// transfer the WRAM-B cache to the arm7
				//transfertToArm7(slot);

				// write the command
				sharedAddr[0] = buffer;
				sharedAddr[1] = READ_SIZE_NDMA7;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				while(sharedAddr[3] != (vu32)0);

				// transfer back the WRAM-B cache to the arm9
				//transfertToArm9(slot);
			}

			updateDescriptor(slot, sector);

			u32 len2=len;
			if((src - sector) + len2 > READ_SIZE_NDMA7){
			    len2 = sector - src + READ_SIZE_NDMA7;
			}

			if(len2 > 512) {
				len2 -= src%4;
				len2 -= len2 % 32;
			}

			if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
				#ifdef DEBUG
				// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = dst;
				sharedAddr[1] = len2;
				sharedAddr[2] = buffer+src-sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				while(sharedAddr[3] != (vu32)0);
				// -------------------------------------*/
				#endif

				// copy directly
				fastCopy32(buffer+(src-sector),dst,len2);

				// update cardi common
				cardStruct[0] = src + len2;
				cardStruct[1] = dst + len2;
				cardStruct[2] = len - len2;
			} else {
				#ifdef DEBUG
				// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = page;
				sharedAddr[1] = len2;
				sharedAddr[2] = buffer+page-sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				while(sharedAddr[3] != (vu32)0);
				// -------------------------------------*/
				#endif

				// read via the 512b ram cache
				fastCopy32(buffer+(page-sector), cacheBuffer, 512);
				*cachePage = page;
				(*readCachedRef)(cacheStruct);
			}
			len = cardStruct[2];
			if(len>0) {
				src = cardStruct[0];
				dst = cardStruct[1];
				page = (src/512)*512;
				sector = (src/READ_SIZE_NDMA7)*READ_SIZE_NDMA7;
				accessCounter++;
			}
		}
	}
	REG_SCFG_EXT = 0x83000000;
	return 0;
}




