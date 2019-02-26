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
//#include <nds/interrupts.h>
#include <nds/dma.h>
#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define _32KB_READ_SIZE  0x8000
#define _64KB_READ_SIZE  0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE   0x100000

//extern void user_exception(void);

//extern vu32* volatile cacheStruct;

extern cardengineArm9* volatile ce9;

extern u32 ROMinRAM;
extern u32 dsiMode;
extern u32 enableExceptionHandler;
extern u32 consoleModel;

extern u32 needFlushDCCache;

extern volatile int (*readCachedRef)(u32*); // This pointer is not at the end of the table but at the handler pointer corresponding to the current irq

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static u32 cacheDescriptor[dev_CACHE_SLOTS_32KB_SDK5] = {0xFFFFFFFF};
static u32 cacheCounter[dev_CACHE_SLOTS_32KB_SDK5];
static u32 accessCounter = 0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
static u32 readSize = _32KB_READ_SIZE;
static u32 cacheAddress = retail_CACHE_ADRESS_START_SDK5;
static u16 cacheSlots = retail_CACHE_SLOTS_32KB_SDK5;

static bool flagsSet = false;
static bool isDma = false;
static u8 dma = 0;

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
	//return (vu32*)(cacheAddress + slot*readSize);
	return (vu8*)(cacheAddress + slot*readSize);
}

static void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

static void yield() {
    if(ce9->patches->yieldRef) {
        volatile void (*yieldRef)(void) = ce9->patches->yieldRef;
        (*yieldRef)();
    } else if(ce9->thumbPatches->yieldRef) {
        callYieldThumb();
    }    
}

static void sleep(u32 ms) {
    if(ce9->patches->sleepRef) {
        volatile void (*sleepRef)(u32) = ce9->patches->sleepRef;
        (*sleepRef)(ms);
    } else if(ce9->thumbPatches->sleepRef) {
        callSleepThumb(ms);
    }    
}

static void waitForArm7(void) {
    IPC_SendSync(0xEE24);
    int count = 0;
	while (sharedAddr[3] != (vu32)0) {
        count++;
        yield();
        sleep(5);
        if(count==20000000){
            IPC_SendSync(0xEE24);
            count=0;
        }
    }
}

/*static inline bool isGameLaggy(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);
	//return (strncmp(romTid, "ASM", 3) == 0  // Super Mario 64 DS (fixes sound crackles, breaks Mario's Holiday)
	return (strncmp(romTid, "AP2", 3) == 0   // Metroid Prime Pinball
		|| strncmp(romTid, "ADM", 3) == 0   // Animal Crossing: Wild World (fixes some sound crackles)
		|| strncmp(romTid, "APT", 3) == 0   // Pokemon Trozei (slightly boosts load speed)
		|| strncmp(romTid, "A2D", 3) == 0   // New Super Mario Bros. (fixes sound crackles)
		|| strncmp(romTid, "ARZ", 3) == 0   // MegaMan ZX (slightly boosts load speed)
		|| strncmp(romTid, "AC9", 3) == 0   // Spider-Man: Battle for New York
		|| strncmp(romTid, "YZX", 3) == 0   // MegaMan ZX Advent (slightly boosts load speed)
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

static inline int cardReadNormal(u8* dst, u32 src, u32 len) {
	u32 commandRead;
	u32 sector = (src/readSize)*readSize;

	accessCounter++;

	/*if (page == src && len > readSize && (u32)dst < 0x02700000 && (u32)dst > 0x02000000 && (u32)dst % 4 == 0) {
		// Read directly at ARM7 level
		commandRead = 0x025FFB08;

		//cacheFlush();

		//REG_IME = 0;

		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		waitForArm7();

		//REG_IME = 1;

	} else {*/
		// Read via the main RAM cache
		while(len > 0) {
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			// Read max CACHE_READ_SIZE via the main RAM cache
			if (slot == -1) {
				// Send a command to the ARM7 to fill the RAM cache
				commandRead = 0x025FFB08;

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				//REG_IME = 0;

				// Write the command
				sharedAddr[0] = (vu32)buffer;
				sharedAddr[1] = readSize;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				waitForArm7();

				//REG_IME = 1;
			}

			updateDescriptor(slot, sector);	

			u32 len2 = len;
			if ((src - sector) + len2 > readSize) {
				len2 = sector - src + readSize;
			}

			if (len2 > 512) {
				len2 -= src % 4;
				len2 -= len2 % 32;
			}

			#ifdef DEBUG
			// Send a log command for debug purpose
			// -------------------------------------
			commandRead = 0x026ff800;

			sharedAddr[0] = dst;
			sharedAddr[1] = len2;
			sharedAddr[2] = buffer+src-sector;
			sharedAddr[3] = commandRead;

			waitForArm7();
			// -------------------------------------*/
			#endif

            if (isDma) {
                // Copy via dma
  				dmaCopyWordsAsynch(dma, (u8*)buffer+(src-sector), dst, len2);
                while (dmaBusy(dma)) {
                    yield();
                    sleep(2);
                }        
  
            }  else {
    			// Copy directly
    			memcpy(dst, (u8*)buffer+(src-sector), len2);
            }
            
			len = len - len2;
			if (len > 0) {
				src = src + len2;
				dst = (u8*)(dst + len2);
				sector = (src / readSize) * readSize;
				accessCounter++;
			}
		}
	//}
	
	return 0;
}

static inline int cardReadRAM(u8* dst, u32 src, u32 len) {
	//u32 commandRead;
	while (len > 0) {
		u32 len2 = len;
		if (len2 > 512) {
			len2 -= src % 4;
			len2 -= len2 % 32;
		}

		#ifdef DEBUG
		// Send a log command for debug purpose
		// -------------------------------------
		commandRead = 0x026ff800;

		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = ((dev_CACHE_ADRESS_START_SDK5-0x4000-ndsHeader->arm9binarySize)+src);
		sharedAddr[3] = commandRead;

		waitForArm7();
		// -------------------------------------
		#endif

		// Copy directly
		memcpy(dst, (u8*)((dev_CACHE_ADRESS_START_SDK5-0x4000-ndsHeader->arm9binarySize)+src), len);

		len = len - len2;
		if (len > 0) {
			src = src + len2;
			dst = (u8*)(dst + len2);
		}
	}

	return 0;
}

u32 cardReadDma() {
	vu32* volatile cardStruct = ce9->cardStruct0;
    
	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];
    dma = cardStruct[3]; // dma channel
	void* func = (void*)cardStruct[4]; // function to call back once read done
	void* arg  = (void*)cardStruct[5]; // arguments of the function above
    
    if(dma >= 0 
        && dma <= 3 
        //&& func != NULL
        && len > 0
        && !(((int)dst) & 31)
        // test data not in ITCM
        && dst > 0x02000000
        // test data not in DTCM
        && (dst < 0x27E0000 || dst > 0x27E4000) 
        // check 512 bytes page alignement 
        && !(((int)len) & 511)
        && !(((int)src) & 511)
        ) {
        isDma = true;
        
        /*if (len < THRESHOLD_CACHE_FLUSH) {
            int oldIME = enterCriticalSection();
            u32     dst2 = dst;
            u32     mod = (dst2 & (CACHE_LINE_SIZE - 1));
            if (mod)
            {
                dst2 -= mod;
                DC_StoreRange((void *)(dst2), CACHE_LINE_SIZE);
                DC_StoreRange((void *)(dst2 + len), CACHE_LINE_SIZE);
                len += CACHE_LINE_SIZE;
            }
            IC_InvalidateRange((void *)dst, len);
            DC_InvalidateRange((void *)dst2, len);
            DC_WaitWriteBufferEmpty();
            leaveCriticalSection(oldIME);   
        } else {*/ 
            // Note : cacheFlush disable / reenable irq
            cacheFlush();
        //}
    } else { 
        isDma = false;
        dma=0;
    }
    
    return 0;    
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		//if (isGameLaggy(ndsHeader)) {
			if (consoleModel > 0) {
				// SDK 5
				cacheAddress = dev_CACHE_ADRESS_START_SDK5;
				cacheSlots = dev_CACHE_SLOTS_32KB_SDK5;
			}
			//readSize = _32KB_READ_SIZE;
		/*} else if (consoleModel > 0) {
			// SDK 5
			cacheAddress = dev_CACHE_ADRESS_START_SDK5;
			cacheSlots = dev_CACHE_SLOTS_SDK5;
		}*/

		if (enableExceptionHandler) {
			//exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
			//setExceptionHandler(user_exception);
		}
		
		flagsSet = true;
	}

	u32 src = src0;
	u8* dst = dst0;
	u32 len = len0;

	#ifdef DEBUG
	u32 commandRead;

	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	waitForArm7();
	// -------------------------------------
	#endif

	if (src == 0) {
		// If ROM read location is 0, do not proceed.
		return 0;
	}

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}

	return ROMinRAM ? cardReadRAM(dst, src, len) : cardReadNormal(dst, src, len);
}
