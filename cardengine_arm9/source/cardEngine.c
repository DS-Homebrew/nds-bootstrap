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

#define ROM_LOCATION 0x0C804000

#define CACHE_ADRESS_START 0x0C920000
#define retail_CACHE_ADRESS_SIZE 0x6E0000
#define dev_CACHE_ADRESS_SIZE 0x16E0000
#define HGSS_CACHE_ADRESS_SIZE 0x1E0000
#define retail_CACHE_SLOTS 0x37
#define dev_CACHE_SLOTS 0xB7
#define HGSS_CACHE_SLOTS 0xF

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 cacheDescriptor [dev_CACHE_SLOTS] = {0xffffffff};
static u32 cacheCounter [dev_CACHE_SLOTS];
static u32 accessCounter = 0;

static u16 cacheSlots = retail_CACHE_SLOTS;
static u32 cacheReadSizeSubtract = 0;
static u32 asyncReadSizeSubtract = 0;

static u32 asyncSector = 0xFFFFFFFF;
static u32 asyncQueue [10];
static int aQHead = 0;
static int aQTail = 0;
static int aQSize = 0;
static char hexbuffer [9];

static u32 readNum = 0;
static bool alreadySetMpu = false;

static bool flagsSet = false;
static bool hgssFix = false;
extern u32 ROMinRAM;
extern u32 ROM_TID;
extern u32 ARM9_LEN;
extern u32 romSize;
extern u32 consoleModel;
extern u32 enableExceptionHandler;

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
	for(int i=0; i<cacheSlots; i++) {
		if(cacheCounter[i]<=lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int getSlotForSector(u32 sector) {
	for(int i=0; i<cacheSlots; i++) {
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

void addToAsyncQueue(sector) {
	#ifdef DEBUG
	nocashMessage("\narm9 addToAsyncQueue\n");	
	nocashMessage("\narm9 sector\n");	
	nocashMessage(tohex(sector));
	#endif
	
	asyncQueue[aQHead] = sector;
	aQHead++;
	aQSize++;
	if(aQHead>9) {
		aQHead=0;
	}
	if(aQSize>10) {
		aQSize=10;
		aQTail++;
		if(aQTail>9) aQTail=0;
	}
}

void triggerAsyncPrefetch(sector) {	
	#ifdef DEBUG
	nocashMessage("\narm9 triggerAsyncPrefetch\n");	
	nocashMessage("\narm9 sector\n");	
	nocashMessage(tohex(sector));
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	asyncReadSizeSubtract = 0;
	if(asyncSector == 0xFFFFFFFF) {
		if (romSize > 0) {
			if (sector > romSize) {
				sector = 0;
			} else if ((sector+_128KB_READ_SIZE) > romSize) {
				for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
					asyncReadSizeSubtract++;
					if (((sector+_128KB_READ_SIZE)-asyncReadSizeSubtract) == romSize) break;
				}
			}
		}
		int slot = getSlotForSector(sector);
		// read max CACHE_READ_SIZE via the main RAM cache
		// do it only if there is no async command ongoing
		if(slot==-1) {
			addToAsyncQueue(sector);
			// send a command to the arm7 to fill the RAM cache
			u32 commandRead = 0x020ff800;		

			slot = allocateCacheSlot();
			vu8* buffer = getCacheAddress(slot);

			if(needFlushDCCache) DC_FlushRange(buffer, _128KB_READ_SIZE);

			cacheDescriptor[slot] = sector;
			cacheCounter[slot] = 0x0FFFFFFF ; // async marker
			asyncSector = sector;		

			// write the command
			sharedAddr[0] = buffer;
			sharedAddr[1] = _128KB_READ_SIZE-asyncReadSizeSubtract;
			sharedAddr[2] = sector;
			sharedAddr[3] = commandRead;

			//IPC_SendSync(0xEE24);			


			// do it asynchronously
			//waitForArm7();
		}
	}
}

void processAsyncCommand() {
	#ifdef DEBUG
	nocashMessage("\narm9 processAsyncCommand\n");	
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	if(asyncSector != 0xFFFFFFFF) {
		int slot = getSlotForSector(asyncSector);
		if(slot!=-1 && cacheCounter[slot] == 0x0FFFFFFF) {
			if(sharedAddr[3] == (vu32)0) {
				updateDescriptor(slot, asyncSector);
				asyncSector = 0xFFFFFFFF;
			}			
		}	
	}
}

void getAsyncSector() {
	#ifdef DEBUG
	nocashMessage("\narm9 getAsyncSector\n");	
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	if(asyncSector != 0xFFFFFFFF) {
		int slot = getSlotForSector(asyncSector);
		if(slot!=-1 && cacheCounter[slot] == 0x0FFFFFFF) {
			waitForArm7();

			updateDescriptor(slot, asyncSector);
			asyncSector = 0xFFFFFFFF;
		}	
	}	
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
	
	if((ROM_TID & 0x00FFFFFF) != 0x4D5341	// SM64DS
	&& (ROM_TID & 0x00FFFFFF) != 0x534D53)	// SMSW
	{
		if(readNum >= 0x100){ //don't set too early or some games will crash
			*(vu32*)(*(vu32*)(0x2800000)) = *(vu32*)(0x2800004);
			*(vu32*)(*(vu32*)(0x2800008)) = *(vu32*)(0x280000C);
			alreadySetMpu = true;
		}else{
			readNum += 1;
		}
	}
	
	if(!flagsSet) {
		if ((ROM_TID & 0x00FFFFFF) == 0x4B5049 // Pokemon HeartGold
		|| (ROM_TID & 0x00FFFFFF) == 0x475049) // Pokemon SoulSilver
		{
			cacheSlots = HGSS_CACHE_SLOTS;	// Use smaller cache size to avoid timing issues
			hgssFix = true;
		}
		else if (consoleModel > 0) cacheSlots = dev_CACHE_SLOTS;

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
	

	if (ROMinRAM == false) {
		u32 sector = (src/_128KB_READ_SIZE)*_128KB_READ_SIZE;
		cacheReadSizeSubtract = 0;
		if ((romSize > 0) && ((sector+_128KB_READ_SIZE) > romSize)) {
			for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
				cacheReadSizeSubtract++;
				if (((sector+_128KB_READ_SIZE)-cacheReadSizeSubtract) == romSize) break;
			}
		}

		accessCounter++;

		if (!hgssFix) processAsyncCommand();

		if(page == src && len > _128KB_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
			if (!hgssFix) getAsyncSector();

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
					if (!hgssFix) getAsyncSector();

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

					updateDescriptor(slot, sector);	
		
					if (!hgssFix) triggerAsyncPrefetch(nextSector);
				} else {
					if (!hgssFix) {
						if(cacheCounter[slot] == 0x0FFFFFFF) {
							// prefetch successfull
							getAsyncSector();
							
							triggerAsyncPrefetch(nextSector);	
						} else {
							int i;
							for(i=0; i<10; i++) {
								if(asyncQueue[i]==sector) {
									// prefetch successfull
									triggerAsyncPrefetch(nextSector);	
									break;
								}
							}
						}
					}
					updateDescriptor(slot, sector);
				}

				u32 len2=len;
				if((src - sector) + len2 > _128KB_READ_SIZE){
					len2 = sector - src + _128KB_READ_SIZE;
				}

				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(readCachedRef == 0 || len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
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
					memcpy(dst,buffer+(src-sector),len2);

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
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					memcpy(cacheBuffer, buffer+(page-sector), 512);
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
	} else {
		while(len > 0) {
			u32 len2=len;
			if(len2 > 512) {
				len2 -= src%4;
				len2 -= len2 % 32;
			}

			if(readCachedRef == 0 || len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
				#ifdef DEBUG
				// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = dst;
				sharedAddr[1] = len2;
				sharedAddr[2] = (ROM_LOCATION-0x4000-ARM9_LEN)+src;
				sharedAddr[3] = commandRead;

				//IPC_SendSync(0xEE24);

				waitForArm7();
				// -------------------------------------
				#endif

				// copy directly
				memcpy(dst,(ROM_LOCATION-0x4000-ARM9_LEN)+src,len2);

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
				sharedAddr[2] = (ROM_LOCATION-0x4000-ARM9_LEN)+page;
				sharedAddr[3] = commandRead;

				//IPC_SendSync(0xEE24);

				waitForArm7();
				// -------------------------------------
				#endif

				// read via the 512b ram cache
				memcpy(cacheBuffer, (ROM_LOCATION-0x4000-ARM9_LEN)+page, 512);
				*cachePage = page;
				(*readCachedRef)(cacheStruct);
			}
			len = cardStruct[2];
			if(len>0) {
				src = cardStruct[0];
				dst = cardStruct[1];
				page = (src/512)*512;
			}
		}
	}
	return 0;
}




