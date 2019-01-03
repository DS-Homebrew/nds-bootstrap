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
#include <stdio.h>
#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
//#include <nds/interrupts.h>
//#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"

#include "my_fat.h"

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
extern u32 fileCluster;
extern u32 ROMinRAM;
extern u32 dsiMode;
extern u32 enableExceptionHandler;
extern u32 consoleModel;
extern u32 asyncPrefetch;

extern u32 needFlushDCCache;

extern volatile int (*readCachedRef)(u32*); // This pointer is not at the end of the table but at the handler pointer corresponding to the current irq

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

/*static u32 cacheDescriptor[dev_CACHE_SLOTS_32KB] = {0xFFFFFFFF};
static u32 cacheCounter[dev_CACHE_SLOTS_32KB];*/
static u32 accessCounter = 0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
/*static u32 romLocation = ROM_LOCATION;
static u32 readSize = _128KB_READ_SIZE;
static u32 cacheAddress = CACHE_ADRESS_START;
static u16 cacheSlots = retail_CACHE_SLOTS;*/

/*static u32 asyncReadSizeSubtract = 0;

static u32 asyncSector = 0xFFFFFFFF;
static u32 asyncQueue[10];
static int aQHead = 0;
static int aQTail = 0;
static int aQSize = 0;*/

/*static u32 readNum = 0;
static bool alreadySetMpu = false;*/

static aFile* romFile = (aFile*)ROM_FILE_LOCATION;

static bool flagsSet = false;

/*static int allocateCacheSlot(void) {
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
	//return (vu32*)(cacheAddress + slot*readSize);
	return (vu8*)(cacheAddress + slot*readSize);
}

static void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

static void waitForArm7(void) {
	while (sharedAddr[3] != (vu32)0);
}*/

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
			} else if ((sector+readSize) > ndsHeader->romSize) {
				for (u32 i = 0; i < readSize; i++) {
					asyncReadSizeSubtract++;
					if (((sector+readSize)-asyncReadSizeSubtract) == ndsHeader->romSize) {
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
				DC_FlushRange((u8*)buffer, readSize);
			}

			cacheDescriptor[slot] = sector;
			cacheCounter[slot] = 0x0FFFFFFF; // async marker
			asyncSector = sector;		

			// write the command
			sharedAddr[0] = (vu32)buffer;
			sharedAddr[1] = readSize-asyncReadSizeSubtract;
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

/*static inline bool isGameLaggy(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);
	//return (strncmp(romTid, "ASM", 3) == 0  // Super Mario 64 DS (fixes sound crackles, breaks Mario's Holiday)
	return (strncmp(romTid, "AP2", 3) == 0   // Metroid Prime Pinball
		|| strncmp(romTid, "ADM", 3) == 0   // Animal Crossing: Wild World (fixes some sound crackles)
		|| strncmp(romTid, "APT", 3) == 0   // Pokemon Trozei (slightly boosts load speed)
		|| strncmp(romTid, "A2D", 3) == 0   // New Super Mario Bros. (fixes sound crackles)
		|| strncmp(romTid, "AC9", 3) == 0   // Spider-Man: Battle for New York
		|| strncmp(romTid, "YCT", 3) == 0   // Contra 4 (slightly boosts load speed)
		|| strncmp(romTid, "YT7", 3) == 0   // SEGA Superstars Tennis (fixes some sound issues)
		|| strncmp(romTid, "CS5", 3) == 0   // Spider-Man: Web of Shadows
		|| strncmp(romTid, "YGX", 3) == 0   // Grand Theft Auto: Chinatown Wars
		|| strncmp(romTid, "CS3", 3) == 0   // Sonic & SEGA All-Stars Racing
		|| strncmp(romTid, "VSO", 3) == 0   // Sonic Classic Collection
		|| strncmp(romTid, "IPK", 3) == 0   // Pokemon HeartGold
		|| strncmp(romTid, "IPG", 3) == 0   // Pokemon SoulSilver
		|| strncmp(romTid, "B6Z", 3) == 0   // MegaMan Zero Collection (slightly boosts load speed)
		|| strncmp(romTid, "IRB", 3) == 0   // Pokemon Black
		|| strncmp(romTid, "IRA", 3) == 0   // Pokemon White
		|| strncmp(romTid, "IRE", 3) == 0   // Pokemon Black 2
		|| strncmp(romTid, "IRD", 3) == 0); // Pokemon White 2
}*/

static inline int cardReadNormal(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	accessCounter++;

	/*nocashMessage("begin\n");

	dbg_hexa(dst);
	nocashMessage("\n");
	dbg_hexa(src);
	nocashMessage("\n");
	dbg_hexa(len);
	nocashMessage("\n");*/

	//nocashMessage("aaaaaaaaaa\n");
	fileRead((char*)dst, *romFile, src, len, 0);

	//nocashMessage("end\n");

	if(strncmp(getRomTid(ndsHeader), "CLJ", 3) == 0){
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}
	
	return 0;
}

/*static inline int cardReadRAM(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
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
			sharedAddr[2] = ((dsiMode || isSdk5(moduleParams)) ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src;
			sharedAddr[3] = commandRead;

			//IPC_SendSync(0xEE24);

			waitForArm7();
			// -------------------------------------
			#endif

			// Copy directly
			memcpy(dst, (u8*)((((dsiMode || isSdk5(moduleParams)) ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src),len);

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
			sharedAddr[2] = ((dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+page;
			sharedAddr[3] = commandRead;

			//IPC_SendSync(0xEE24);

			waitForArm7();
			// -------------------------------------
			#endif

			// Read via the 512b ram cache
			memcpy(cacheBuffer, (u8*)(((dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation) - 0x4000 - ndsHeader->arm9binarySize) + page), 512);
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
}*/

//Currently used for NSMBDS romhacks
void __attribute__((target("arm"))) debug8mbMpuFix(){
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		if (!FAT_InitFiles(true, 3)) {
			//nocashMessage("!FAT_InitFiles");
			return -1;
		}

		//romFile = getFileFromCluster(fileCluster);
		//buildFatTableCache(&romFile, 0);

		if (isSdk5(moduleParams)) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		}
		/*if (dsiMode || isSdk5(moduleParams)) {
			romLocation = ROM_SDK5_LOCATION;
			cacheAddress = retail_CACHE_ADRESS_START_SDK5;
			cacheSlots = retail_CACHE_SLOTS_SDK5;
		}

		if (isGameLaggy(ndsHeader)) {
			if (consoleModel > 0) {
				if (dsiMode || isSdk5(moduleParams)) {
					// SDK 5
					cacheAddress = dev_CACHE_ADRESS_START_SDK5;
				}
				cacheSlots = ((dsiMode || isSdk5(moduleParams)) ? dev_CACHE_SLOTS_32KB_SDK5 : dev_CACHE_SLOTS_32KB);
			} else {
				cacheSlots = ((dsiMode || isSdk5(moduleParams)) ? retail_CACHE_SLOTS_32KB_SDK5 : retail_CACHE_SLOTS_32KB);
			}
			readSize = _32KB_READ_SIZE;
		} else if (consoleModel > 0) {
			if (dsiMode || isSdk5(moduleParams)) {
				// SDK 5
				cacheAddress = dev_CACHE_ADRESS_START_SDK5;
			}
			cacheSlots = ((dsiMode || isSdk5(moduleParams)) ? dev_CACHE_SLOTS_SDK5 : dev_CACHE_SLOTS);
		}*/

		debug8mbMpuFix();

		//ndsHeader->romSize += 0x1000;

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

	#ifdef DEBUG
	u32 commandRead;

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

	//return ROMinRAM ? cardReadRAM(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage) : cardReadNormal(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage);
	return cardReadNormal(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage);
}
