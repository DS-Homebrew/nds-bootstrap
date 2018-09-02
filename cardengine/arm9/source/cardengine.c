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

#include <string.h>
#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"

#define _32KB_READ_SIZE  0x8000
#define _64KB_READ_SIZE  0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE   0x100000

extern void user_exception(void);

extern vu32* volatile cardStruct0;
//extern vu32* volatile cacheStruct;

extern module_params_t* moduleParams;
extern u32 ROMinRAM;
extern u32 dsiMode; // SDK 5
extern u32 enableExceptionHandler;
extern u32 consoleModel;
extern u32 asyncPrefetch;

extern u32 needFlushDCCache;

extern volatile int (*readCachedRef)(u32*); // This pointer is not at the end of the table but at the handler pointer corresponding to the current irq

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static u32 cacheDescriptor[dev_CACHE_SLOTS] = {0xFFFFFFFF};
static u32 cacheCounter[dev_CACHE_SLOTS];
static u32 accessCounter = 0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
static u32 romLocation = ROM_LOCATION;
static u32 cacheAddress = CACHE_ADRESS_START; // SDK 5
static u16 cacheSlots = retail_CACHE_SLOTS;

static u32 cacheReadSizeSubtract = 0;
/*static u32 asyncReadSizeSubtract = 0;

static u32 asyncSector = 0xFFFFFFFF;
static u32 asyncQueue[10];
static int aQHead = 0;
static int aQTail = 0;
static int aQSize = 0;*/

static u32 readNum = 0;
static bool alreadySetMpu = false;

static bool flagsSet = false;
static bool hgssFix = false;

static int allocateCacheSlot(void) {
	int slot = 0;
	u32 lowerCounter = accessCounter;
	for (int i = 0; i < cacheSlots; i++) {
		if (cacheCounter[i] <= lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if (!lowerCounter) {
				break;
			}
		}
	}
	return slot;
}

static int getSlotForSector(u32 sector) {
	for (int i = 0; i < cacheSlots; i++) {
		if (cacheDescriptor[i] == sector) {
			return i;
		}
	}
	return -1;
}

static vu8* getCacheAddress(int slot) {
	//return (vu32*)(cacheAddress + slot*_128KB_READ_SIZE);
	return (vu8*)(cacheAddress + slot*_128KB_READ_SIZE);
}

static void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

static void waitForArm7(void) {
	while (sharedAddr[3] != (vu32)0);
}

/*static void addToAsyncQueue(u32 sector) {
	#ifdef DEBUG
	nocashMessage("\narm9 addToAsyncQueue\n");	
	nocashMessage("\narm9 sector\n");	
	nocashMessage(tohex(sector));
	#endif
	
	asyncQueue[aQHead] = sector;
	aQHead++;
	aQSize++;
	if (aQHead > 9) {
		aQHead = 0;
	}
	if (aQSize > 10) {
		aQSize = 10;
		aQTail++;
		if(aQTail > 9) {
			aQTail = 0;
		}
	}
}

static void triggerAsyncPrefetch(u32 sector) {	
	#ifdef DEBUG
	nocashMessage("\narm9 triggerAsyncPrefetch\n");	
	nocashMessage("\narm9 sector\n");	
	nocashMessage(tohex(sector));
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	asyncReadSizeSubtract = 0;
	if (asyncSector == 0xFFFFFFFF) {
		if (ndsHeader->romSize > 0) {
			if (sector > ndsHeader->romSize) {
				sector = 0;
			} else if ((sector+_128KB_READ_SIZE) > ndsHeader->romSize) {
				for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
					asyncReadSizeSubtract++;
					if (((sector+_128KB_READ_SIZE)-asyncReadSizeSubtract) == ndsHeader->romSize) {
						break;
					}
				}
			}
		}
		int slot = getSlotForSector(sector);
		// read max CACHE_READ_SIZE via the main RAM cache
		// do it only if there is no async command ongoing
		if (slot == -1) {
			addToAsyncQueue(sector);
			// send a command to the arm7 to fill the RAM cache
			u32 commandRead = 0x020ff800;		

			slot = allocateCacheSlot();
			vu8* buffer = getCacheAddress(slot);

			if (needFlushDCCache) {
				DC_FlushRange((u8*)buffer, _128KB_READ_SIZE);
			}

			cacheDescriptor[slot] = sector;
			cacheCounter[slot] = 0x0FFFFFFF; // async marker
			asyncSector = sector;		

			// write the command
			sharedAddr[0] = (vu32)buffer;
			sharedAddr[1] = _128KB_READ_SIZE-asyncReadSizeSubtract;
			sharedAddr[2] = sector;
			sharedAddr[3] = commandRead;

			//IPC_SendSync(0xEE24);			


			// do it asynchronously
			//waitForArm7();
		}
	}
}

static void processAsyncCommand(void) {
	#ifdef DEBUG
	nocashMessage("\narm9 processAsyncCommand\n");	
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	if (asyncSector != 0xFFFFFFFF) {
		int slot = getSlotForSector(asyncSector);
		if (slot != -1 && cacheCounter[slot] == 0x0FFFFFFF) {
			if(sharedAddr[3] == (vu32)0) {
				updateDescriptor(slot, asyncSector);
				asyncSector = 0xFFFFFFFF;
			}			
		}	
	}
}

static void getAsyncSector(void) {
	#ifdef DEBUG
	nocashMessage("\narm9 getAsyncSector\n");	
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	if (asyncSector != 0xFFFFFFFF) {
		int slot = getSlotForSector(asyncSector);
		if (slot != -1 && cacheCounter[slot] == 0x0FFFFFFF) {
			waitForArm7();

			updateDescriptor(slot, asyncSector);
			asyncSector = 0xFFFFFFFF;
		}	
	}
}*/

static inline bool isHGSS(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);
	return (strncmp(romTid, "IPK", 3) == 0  // Pokemon HeartGold
		|| strncmp(romTid, "IPG", 3) == 0); // Pokemon SoulSilver
}

static inline int cardReadNormal(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	u32 commandRead;
	u32 sector = (src/_128KB_READ_SIZE)*_128KB_READ_SIZE;
	cacheReadSizeSubtract = 0;
	if ((ndsHeader->romSize > 0) && ((sector+_128KB_READ_SIZE) > ndsHeader->romSize)) {
		for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
			cacheReadSizeSubtract++;
			if (((sector+_128KB_READ_SIZE)-cacheReadSizeSubtract) == ndsHeader->romSize) break;
		}
	}

	accessCounter++;

	//bool pAC = ((isSdk5(moduleParams) && consoleModel > 0) || (!isSdk5(moduleParams) && !hgssFix));

	/*if (asyncPrefetch && pAC) {
		processAsyncCommand();
	}*/

	if (page == src && len > _128KB_READ_SIZE && (u32)dst < 0x02700000 && (u32)dst > 0x02000000 && (u32)dst % 4 == 0) {
		/*if (asyncPrefetch && pAC) {
			getAsyncSector();
		}*/

		// Read directly at ARM7 level
		commandRead = 0x025FFB08;

		cacheFlush();

		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		//IPC_SendSync(0xEE24);

		waitForArm7();

	} else {
		// Read via the main RAM cache
		while(len > 0) {
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			//u32 nextSector = sector+_128KB_READ_SIZE;	
			// Read max CACHE_READ_SIZE via the main RAM cache
			if (slot == -1) {
				/*if (asyncPrefetch && pAC) {
					getAsyncSector();
				}*/

				// Send a command to the ARM7 to fill the RAM cache
				commandRead = 0x025FFB08;

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				if (needFlushDCCache) {
					DC_FlushRange((u8*)buffer, _128KB_READ_SIZE);
				}

				// Write the command
				sharedAddr[0] = (vu32)buffer;
				sharedAddr[1] = _128KB_READ_SIZE - cacheReadSizeSubtract;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				//IPC_SendSync(0xEE24);

				waitForArm7();

				updateDescriptor(slot, sector);	
	
				/*if (asyncPrefetch && pAC) {
					triggerAsyncPrefetch(nextSector);
				}*/
			} else {
				/*if (asyncPrefetch && pAC) {
					if (cacheCounter[slot] == 0x0FFFFFFF) {
						// Prefetch successful
						getAsyncSector();
						
						triggerAsyncPrefetch(nextSector);	
					} else {
						for (int i = 0; i < 10; i++) {
							if (asyncQueue[i]==sector) {
								// Prefetch successful
								triggerAsyncPrefetch(nextSector);	
								break;
							}
						}
					}
				}*/
				updateDescriptor(slot, sector);
			}

			u32 len2 = len;
			if ((src - sector) + len2 > _128KB_READ_SIZE) {
				len2 = sector - src + _128KB_READ_SIZE;
			}

			if (len2 > 512) {
				len2 -= src % 4;
				len2 -= len2 % 32;
			}

			if (isSdk5(moduleParams) || readCachedRef == 0 || (len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0)) {
				#ifdef DEBUG
				// Send a log command for debug purpose
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

				// Copy directly
				memcpy(dst, (u8*)buffer+(src-sector), len2);

				// Update cardi common
				cardStruct[0] = src + len2;
				cardStruct[1] = (vu32)(dst + len2);
				cardStruct[2] = len - len2;
			} else {
				#ifdef DEBUG
				// Send a log command for debug purpose
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

				// Read via the 512b ram cache
				//copy8(buffer+(page-sector)+(src%512), dst, len2);
				//cardStruct[0] = src + len2;
				//cardStruct[1] = dst + len2;
				//cardStruct[2] = len - len2;
				//(*readCachedRef)(cacheStruct);
				memcpy(cacheBuffer, (u8*)buffer+(page-sector), 512);
				*cachePage = page;
				(*readCachedRef)(cacheStruct);
			}
			len = cardStruct[2];
			if (len > 0) {
				src = cardStruct[0];
				dst = (u8*)cardStruct[1];
				page = (src / 512) * 512;
				sector = (src / _128KB_READ_SIZE) * _128KB_READ_SIZE;
				cacheReadSizeSubtract = 0;
				if (ndsHeader->romSize > 0 && (sector+_128KB_READ_SIZE) > ndsHeader->romSize) {
					for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
						cacheReadSizeSubtract++;
						if ((sector+_128KB_READ_SIZE) - cacheReadSizeSubtract == ndsHeader->romSize) {
							break;
						}
					}
				}
				accessCounter++;
			}
		}
	}
	
	return 0;
}

static inline int cardReadRAM(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	//u32 commandRead;
	while (len > 0) {
		u32 len2 = len;
		if (len2 > 512) {
			len2 -= src % 4;
			len2 -= len2 % 32;
		}

		if (isSdk5(moduleParams) || readCachedRef == 0 || (len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0)) {
			#ifdef DEBUG
			// Send a log command for debug purpose
			// -------------------------------------
			commandRead = 0x026ff800;

			sharedAddr[0] = dst;
			sharedAddr[1] = len;
			sharedAddr[2] = ((isSdk5(moduleParams) ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src;
			sharedAddr[3] = commandRead;

			//IPC_SendSync(0xEE24);

			waitForArm7();
			// -------------------------------------
			#endif

			// Copy directly
			memcpy(dst, (u8*)(((isSdk5(moduleParams) ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src),len);

			// Update cardi common
			cardStruct[0] = src + len;
			cardStruct[1] = (vu32)(dst + len);
			cardStruct[2] = len - len;
		} else {
			#ifdef DEBUG
			// Send a log command for debug purpose
			// -------------------------------------
			commandRead = 0x026ff800;

			sharedAddr[0] = page;
			sharedAddr[1] = len2;
			sharedAddr[2] = (romLocation-0x4000-ndsHeader->arm9binarySize)+page;
			sharedAddr[3] = commandRead;

			//IPC_SendSync(0xEE24);

			waitForArm7();
			// -------------------------------------
			#endif

			// Read via the 512b ram cache
			memcpy(cacheBuffer, (u8*)((romLocation - 0x4000 - ndsHeader->arm9binarySize) + page), 512);
			*cachePage = page;
			(*readCachedRef)(cacheStruct);
		}
		len = cardStruct[2];
		if (len > 0) {
			src = cardStruct[0];
			dst = (u8*)cardStruct[1];
			page = (src / 512) * 512;
		}
	}

	return 0;
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		if (isSdk5(moduleParams)) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
			romLocation = ROM_SDK5_LOCATION;
			cacheAddress = retail_CACHE_ADRESS_START_SDK5;
			cacheSlots = retail_CACHE_SLOTS_SDK5;
		}

		if (isHGSS(ndsHeader)) {
			cacheSlots = HGSS_CACHE_SLOTS;	// Use smaller cache size to avoid timing issues
			hgssFix = true;
		} else if (consoleModel > 0) {
			if (isSdk5(moduleParams)) {
				// SDK 5
				cacheAddress = dev_CACHE_ADRESS_START_SDK5;
			}
			cacheSlots = (isSdk5(moduleParams) ? dev_CACHE_SLOTS_SDK5 : dev_CACHE_SLOTS);
		}

		ndsHeader->romSize += 0x1000;

		if (enableExceptionHandler) {
			exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
			setExceptionHandler(user_exception);
		}
		
		flagsSet = true;
	}

	vu32* volatile cardStruct = (isSdk5(moduleParams) ? (vu32* volatile)(CARDENGINE_ARM9_LOCATION + 0x7BC0) : cardStruct0);

	u32 src = (isSdk5(moduleParams) ? src0 : cardStruct[0]);
	if (isSdk5(moduleParams)) {
		cardStruct[0] = src;
	}

	u8* dst = (isSdk5(moduleParams) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = (isSdk5(moduleParams) ? len0 : cardStruct[2]);

	if (isSdk5(moduleParams)) {
		cardStruct[1] = (vu32)dst;
		cardStruct[2] = len;
	}

	u32 page = (src / 512) * 512;

	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;

	//u32 commandRead;

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

	// SDK 5 --> White screen
	/*if (!isSdk5(moduleParams) && *(vu32*)0x2800010 != 1) {
		if (readNum >= 0x100){ // Don't set too early or some games will crash
			*(vu32*)(*(vu32*)(0x2800000)) = *(vu32*)0x2800004;
			*(vu32*)(*(vu32*)(0x2800008)) = *(vu32*)0x280000C;
			alreadySetMpu = true;
		} else {
			readNum += 1;
		}
	}*/

	if (src == 0) {
		// If ROM read location is 0, do not proceed.
		return 0;
	}

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}

	return ROMinRAM ? cardReadRAM(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage) : cardReadNormal(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage);
}
