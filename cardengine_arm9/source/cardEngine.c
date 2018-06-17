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

#define _32KB_READ_SIZE 0x8000
#define _64KB_READ_SIZE 0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE 0x100000

#define CACHE_ADRESS_START 0x0C420000
#define CACHE_ADRESS_SIZE 0x1E0000
#define CACHE_SLOTS 0xF

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr = (vu32*)0x026FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 cacheDescriptor [CACHE_SLOTS] = {0xffffffff};
static u32 cacheCounter [CACHE_SLOTS];
static u32 accessCounter = 0;

static u32 cacheReadSizeSubtract = 0;

static u32 readNum = 0;
static bool alreadySetMpu = false;

static bool flagsSet = false;
extern u32 romSize;
extern u32 consoleModel;
extern u32 enableExceptionHandler;

static char hexbuffer [9];

char* tohex(u32 n)
{
    unsigned size = 9;
    char *buffer = hexbuffer;
    unsigned index = size - 2;

	for (int i=0; i<size; i++) {
		buffer[i] = '0';
	}
	
    while (n > 0)
    {
        unsigned mod = n % 16;

        if (mod >= 10)
            buffer[index--] = (mod - 10) + 'A';
        else
            buffer[index--] = mod + '0';

        n /= 16;
    }
    buffer[size - 1] = '\0';
    return buffer;
}

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
	u32 lowerCounter = accessCounter;
	for(int i=0; i<CACHE_SLOTS; i++) {
		if(cacheCounter[i]<=lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int getSlotForSector(u32 sector) {
	for(int i=0; i<CACHE_SLOTS; i++) {
		if(cacheDescriptor[i]==sector) {
			return i;
		}
	}
	return -1;
}


vu8* getCacheAddress(int slot) {
	return (vu32*)(CACHE_ADRESS_START+slot*_128KB_READ_SIZE);
}

void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

void waitForArm7() {
	while(sharedAddr[3] != (vu32)0);
}

int cardRead (u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");
	
	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;
	u32 commandRead;
	u32 src = cardStruct[0];
	if(src==0) {
		return 0;	// If ROM read location is 0, do not proceed.
	}
	u8* dst = (u8*) (cardStruct[1]);
	u32 len = cardStruct[2];

	u32 page = (src/512)*512;
	
	if(readNum >= 0x100){ //don't set too early or some games will crash
		*(vu32*)(*(vu32*)(0x2800000)) = *(vu32*)(0x2800004);
		*(vu32*)(*(vu32*)(0x2800008)) = *(vu32*)(0x280000C);
		alreadySetMpu = true;
	}else{
		readNum += 1;
	}
	
	if(!flagsSet) {
		romSize += 0x1000;
		
		if (enableExceptionHandler) {
			setExceptionHandler2();
		}
		flagsSet = true;
	}

	#ifdef DEBUG
	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	//IPC_SendSync(0xEE24);

	waitForArm7();
	// -------------------------------------*/
	#endif
	
	
	u32 sector = (src/_128KB_READ_SIZE)*_128KB_READ_SIZE;
	cacheReadSizeSubtract = 0;
	if ((romSize > 0) && ((sector+_128KB_READ_SIZE) > romSize)) {
		for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
			cacheReadSizeSubtract++;
			if (((sector+_128KB_READ_SIZE)-cacheReadSizeSubtract) == romSize) break;
		}
	}

	accessCounter++;

	if(page == src && len > _128KB_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
		// read directly at arm7 level
		commandRead = 0x025FFB08;

		cacheFlush();

		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		//IPC_SendSync(0xEE24);

		waitForArm7();

	} else {
		// read via the main RAM cache
		while(len > 0) {
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			u32 nextSector = sector+_128KB_READ_SIZE;	
			// read max CACHE_READ_SIZE via the main RAM cache
			if(slot==-1) {
				// send a command to the arm7 to fill the RAM cache
				commandRead = 0x025FFB08;

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				if(needFlushDCCache) DC_FlushRange(buffer, _128KB_READ_SIZE);

				// write the command
				sharedAddr[0] = buffer;
				sharedAddr[1] = _128KB_READ_SIZE-cacheReadSizeSubtract;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				//IPC_SendSync(0xEE24);

				waitForArm7();
			}

			updateDescriptor(slot, sector);

			u32 len2=len;
			if((src - sector) + len2 > _128KB_READ_SIZE){
				len2 = sector - src + _128KB_READ_SIZE;
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

				//IPC_SendSync(0xEE24);

				waitForArm7();
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

				//IPC_SendSync(0xEE24);

				waitForArm7();
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
				sector = (src/_128KB_READ_SIZE)*_128KB_READ_SIZE;
				cacheReadSizeSubtract = 0;
				if ((romSize > 0) && ((sector+_128KB_READ_SIZE) > romSize)) {
					for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
						cacheReadSizeSubtract++;
						if (((sector+_128KB_READ_SIZE)-cacheReadSizeSubtract) == romSize) break;
					}
				}
				accessCounter++;
			}
		}
	}
	return 0;
}




