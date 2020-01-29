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
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "hex.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#ifdef DLDI
#include "my_fat.h"
#include "card_dldionly.h"
#endif

#define _16KB_READ_SIZE  0x4000
#define _32KB_READ_SIZE  0x8000
#define _64KB_READ_SIZE  0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE   0x100000

//extern vu32* volatile cacheStruct;

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;

static u32 romLocation = retail_CACHE_ADRESS_START_SDK5;
#ifdef DLDI
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_MAINMEM;
//static aFile* savFile = (aFile*)SAV_FILE_LOCATION_MAINMEM;

bool sdRead = false;
#else
static u32 cacheDescriptor[dev_CACHE_SLOTS_16KB_SDK5] = {0xFFFFFFFF};
static u32 cacheCounter[dev_CACHE_SLOTS_16KB_SDK5];
static u32 accessCounter = 0;

static u32 readSize = _16KB_READ_SIZE;
static u32 cacheAddress = retail_CACHE_ADRESS_START_SDK5;
static u16 cacheSlots = retail_CACHE_SLOTS_16KB_SDK5;
#endif
static u32 overlaysSize = 0;

static bool flagsSet = false;
static bool loadOverlaysFromRam = true;
static bool isDma = false;
static bool dmaLed = false;
static u8 dma = 4;

#ifndef DLDI
static inline 
bool ndmaBusy(uint8 ndmaSlot) {
	return	*(u32*)(0x400411C+(ndmaSlot*0x1C)) & BIT(31) == 0x80000000;
}

static inline 
/*! \fn void ndmaCopyWordsAsynch(uint8 channel, const void* src, void* dest, uint32 size)
\brief copies from source to destination on one of the 4 available channels in half words.  
This function returns immediately after starting the transfer.
\param channel the dma channel to use (0 - 3).  
\param src the source to copy from
\param dest the destination to copy to
\param size the size in bytes of the data to copy.  Will be truncated to the nearest word (4 bytes)
*/
void ndmaCopyWordsAsynch(uint8 ndmaSlot, const void* src, void* dest, uint32 size) {
	*(u32*)(0x4004104+(ndmaSlot*0x1C)) = src;
	*(u32*)(0x4004108+(ndmaSlot*0x1C)) = dest;
	
	*(u32*)(0x4004110+(ndmaSlot*0x1C)) = size/4;	
	
    *(u32*)(0x4004114+(ndmaSlot*0x1C)) = 0x1;
	
	*(u32*)(0x400411C+(ndmaSlot*0x1C)) = 0x90070000;
}

static inline bool checkArm7(void) {
    IPC_SendSync(0x4);
	return (sharedAddr[3] == (vu32)0);
}

static bool IPC_SYNC_hooked = false;
static inline void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
        u32* ipcSyncHandler = ce9->irqTable + 16;
        ce9->intr_ipc_orig_return   = *ipcSyncHandler;
        *ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

static inline void enableIPCSYNC(void) {
    // enable IPC_SYNC
    REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;  
    enableIrqMask(IRQ_IPC_SYNC);
}


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
#endif

#ifndef DLDI
static void waitForArm7(void) {
    IPC_SendSync(0x4);
    int count = 0;
    /*if (ce9->patches->sleepRef || ce9->thumbPatches->sleepRef) {
        while (sharedAddr[3] != (vu32)0) {
           if(count==0) {
                sleep(1);
                IPC_SendSync(0xEE24);
                count=1000;
            }
            count--;
        }
    } else {*/
        while (sharedAddr[3] != (vu32)0) {
           if(count==20000000) {
                IPC_SendSync(0x4);
                count=0;
            }
            count++;
        }
    //}
}

void endCardReadDma() {
    if(ce9->patches->cardEndReadDmaRef) {
        volatile void (*cardEndReadDmaRef)() = ce9->patches->cardEndReadDmaRef;
        (*cardEndReadDmaRef)();
    } else if(ce9->thumbPatches->cardEndReadDmaRef) {
        callEndReadDmaThumb();
    }    
}

static int currentLen=0;
static bool dmaReadOnArm7 = false;
static bool dmaReadOnArm9 = false;
static u32 * dmaParams = NULL;

void continueCardReadDmaArm9() {
    if(dmaReadOnArm9) {             
        if(ndmaBusy(0)) return;

        dmaReadOnArm9 = false;
        sharedAddr[3] = 0;        

        u32 commandRead=0x025FFB0A;
        u32 commandPool=0x025AAB08;

        u32 src = dmaParams[3];
    	u8* dst = (u8*)dmaParams[4];
    	u32 len = dmaParams[5];           

        // Update dma params
  		dmaParams[3] = src + currentLen;
  		dmaParams[4] = (vu32)(dst + currentLen);
  		dmaParams[5] = len - currentLen;

        src = dmaParams[3];
        dst = (u8*)(dmaParams[4]);
        len = dmaParams[5]; 

        u32 sector = (src/readSize)*readSize;

        if (len > 0) {
			accessCounter++;  

            // Read via the main RAM cache
        	int slot = getSlotForSector(sector);
        	vu8* buffer = getCacheAddress(slot);
        	// Read max CACHE_READ_SIZE via the main RAM cache
        	if (slot == -1) {
        		// Send a command to the ARM7 to fill the RAM cache

        		slot = allocateCacheSlot();
        
        		buffer = getCacheAddress(slot);


        		// Write the command
        		sharedAddr[0] = (vu32)buffer;
        		sharedAddr[1] = readSize;
        		sharedAddr[2] = sector;
        		sharedAddr[3] = commandRead;

                // do not wait for arm7 and return immediately
        		checkArm7();
                
                dmaReadOnArm7 = true;
                
                updateDescriptor(slot, sector);	
                return;

        	} else {
        		updateDescriptor(slot, sector);	
        
        		u32 len2 = len;
        		if ((src - sector) + len2 > readSize) {
        			len2 = sector - src + readSize;
        		}
        
        		if (len2 > 512) {
        			len2 -= src % 4;
        			len2 -= len2 % 32;
        		}
        
        		// Copy via dma
                ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
                dmaReadOnArm9 = true;
                currentLen= len2;
 
                sharedAddr[3] = commandPool;               
                IPC_SendSync(0x3);        
            }  
        }
        if (len==0) {
          //disableIrqMask(IRQ_DMA0 << dma);
          //resetRequestIrqMask(IRQ_DMA0 << dma);
          //disableDMA(dma); 
          endCardReadDma();
       } 
    }
}

void continueCardReadDmaArm7() {
        
    if(dmaReadOnArm7) {
        if(!checkArm7()) return;
        
        dmaReadOnArm7 = false;
        
        vu32* volatile cardStruct = ce9->cardStruct0;
        u32 commandPool=0x025AAB08;
        
        u32 src = dmaParams[3];
    	u8* dst = (u8*)dmaParams[4];
    	u32 len = dmaParams[5];   
        
        u32 sector = (src/readSize)*readSize;
        
        u32 len2 = len;
		if ((src - sector) + len2 > readSize) {
			len2 = sector - src + readSize;
		}

		if (len2 > 512) {
			len2 -= src % 4;
			len2 -= len2 % 32;
		}
        
        int slot = getSlotForSector(sector);
        vu8* buffer = getCacheAddress(slot);

        // TODO Copy via dma
        ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
        dmaReadOnArm9 = true;
        currentLen= len2;
        
        sharedAddr[3] = commandPool;
        IPC_SendSync(0x3);
    }
}

void cardSetDma (u32 * params) {

    disableIrqMask(IRQ_CARD);
    disableIrqMask(IRQ_CARD_LINE );

    // reset IPC_SYNC IRQs    
    resetRequestIrqMask(IRQ_IPC_SYNC);

    int oldIME = enterCriticalSection();

    hookIPC_SYNC();

    leaveCriticalSection(oldIME); 

    enableIPCSYNC();

    dmaParams = params;
    u32 src = dmaParams[3];
	u8* dst = (u8*)dmaParams[4];
	u32 len = dmaParams[5];    

    u32 commandRead=0x025FFB0A;
    u32 commandPool=0x025AAB08;
	u32 sector = (src/readSize)*readSize;

	if (ce9->ROMinRAM) {
  		u32 len2 = len;
  		if (len2 > 512) {
  			len2 -= src % 4;
  			len2 -= len2 % 32;
  		}

  		// Copy via dma
        ndmaCopyWordsAsynch(0, (u8*)((romLocation-0x4000-ndsHeader->arm9binarySize)+src), dst, len2);
        dmaReadOnArm9 = true;
        currentLen = len2;

        sharedAddr[3] = commandPool;
        IPC_SendSync(0x3);        

		return;
	}

	accessCounter++;  
  
  	// Read via the main RAM cache
  	int slot = getSlotForSector(sector);
  	vu8* buffer = getCacheAddress(slot);
  	// Read max CACHE_READ_SIZE via the main RAM cache
  	if (slot == -1) {    
  		// Send a command to the ARM7 to fill the RAM cache
  
  		slot = allocateCacheSlot();
  
  		buffer = getCacheAddress(slot);
      
  		// Write the command
  		sharedAddr[0] = (vu32)buffer;
  		sharedAddr[1] = readSize;
  		sharedAddr[2] = sector;
  		sharedAddr[3] = commandRead;
  
          // do not wait for arm7 and return immediately
  		checkArm7();
          
        dmaReadOnArm7 = true;
        
        updateDescriptor(slot, sector);
  	} else {
  		updateDescriptor(slot, sector);	
  
  		u32 len2 = len;
  		if ((src - sector) + len2 > readSize) {
  			len2 = sector - src + readSize;
  		}
  
  		if (len2 > 512) {
  			len2 -= src % 4;
  			len2 -= len2 % 32;
  		}
  
  		// Copy via dma
        ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
        dmaReadOnArm9 = true;
        currentLen= len2;
          
        sharedAddr[3] = commandPool;
        IPC_SendSync(0x3);        
      }
}

#else

void cardSetDma(void) {
    return false;
}


#endif

static void clearIcache (void) {
      // Seems to have no effect
      // disable interrupt
      /*int oldIME = enterCriticalSection();
      IC_InvalidateAll();
      // restore interrupt
      leaveCriticalSection(oldIME);*/
}

static inline int cardReadNormal(u8* dst, u32 src, u32 len) {
#ifdef DLDI
	while (sharedAddr[3]==0x52414D44);	// Wait during a RAM dump
	fileRead((char*)dst, *romFile, src, len, 0);
#else
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
				commandRead = (dmaLed ? 0x025FFB0A : 0x025FFB08);

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				// Write the command
				sharedAddr[0] = (vu32)buffer;
				sharedAddr[1] = readSize;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				waitForArm7();
			}

			updateDescriptor(slot, sector);	

			u32 len2 = len;
			if ((src - sector) + len2 > readSize) {
				len2 = sector - src + readSize;
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

            /*if (isDma) {
                // Copy via dma
  				dmaCopyWordsAsynch(dma, (u8*)buffer+(src-sector), dst, len2);
                while (dmaBusy(dma)) {
                    sleep(1);
                }
            } else {*/
    			// Copy directly
    			tonccpy(dst, (u8*)buffer+(src-sector), len2);
            //}

			len = len - len2;
			if (len > 0) {
				src = src + len2;
				dst = (u8*)(dst + len2);
				sector = (src / readSize) * readSize;
				accessCounter++;
			}
		}
	//}
#endif
	
	return 0;
}

static inline int cardReadRAM(u8* dst, u32 src, u32 len) {
	while (len > 0) {
		/*if (isDma) {
            // Copy via dma
  			dmaCopyWordsAsynch(dma, (u8*)((romLocation-0x4000-ndsHeader->arm9binarySize)+src), dst, len);
            while (dmaBusy(dma)) {
                sleep(1);
            }        
		} else {*/
			#ifdef DEBUG
			// Send a log command for debug purpose
			// -------------------------------------
			commandRead = 0x026ff800;

			sharedAddr[0] = dst;
			sharedAddr[1] = len;
			sharedAddr[2] = ((romLocation-0x4000-ndsHeader->arm9binarySize)+src);
			sharedAddr[3] = commandRead;

			waitForArm7();
			// -------------------------------------
			#endif

			// Copy directly
			tonccpy(dst, (u8*)((romLocation-0x4000-ndsHeader->arm9binarySize)+src), len);
		//}

		len = len - len;
		if (len > 0) {
			src = src + len;
			dst = (u8*)(dst + len);
		}
	}
	return 0;
}

u32 cardReadDma(u32 dma0, void *dst, u32 src, u32 len) {
    return 0;    
}

int cardRead(u32* cacheStruct, u8* dst, u32 src, u32 len) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		const char* romTid = getRomTid(ndsHeader);
		#ifdef DLDI
		if (!FAT_InitFiles(false, 0)) {
			//nocashMessage("!FAT_InitFiles");
			//return -1;
		}

		if ((strncmp(romTid, "BKW", 3) == 0)
		|| (strncmp(romTid, "VKG", 3) == 0)) {
			romLocation = CACHE_ADRESS_START_low;
		}

		for (int i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i <= ndsHeader->arm7romOffset; i++) {
			overlaysSize = i;
		}

		if (ce9->consoleModel>0 ? overlaysSize<=0x1000000 : overlaysSize<=0x700000) {} else {
			loadOverlaysFromRam = false;
		}
		#else
		if (ce9->consoleModel > 0) {
			romLocation = ROM_SDK5_LOCATION;
			cacheAddress = dev_CACHE_ADRESS_START_SDK5;
			cacheSlots = dev_CACHE_SLOTS_16KB_SDK5;
		} else if ((strncmp(romTid, "BKW", 3) == 0)
				|| (strncmp(romTid, "VKG", 3) == 0)) {
			romLocation = CACHE_ADRESS_START_low;
			cacheAddress = CACHE_ADRESS_START_low;
			cacheSlots = CACHE_SLOTS_16KB_low;
		}

		if (!ce9->ROMinRAM) {
			for (int i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i <= ndsHeader->arm7romOffset; i++) {
				overlaysSize = i;
			}

			if (ce9->consoleModel>0 ? overlaysSize<=0x1000000 : overlaysSize<=0x700000) {
				for (int i = 0; i < overlaysSize; i += readSize) {
					cacheAddress += readSize;
					cacheSlots--;
				}
			} else {
				loadOverlaysFromRam = false;
			}
		}

		if (ce9->enableExceptionHandler) {
			//exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
			//setExceptionHandler(user_exception);
		}
		#endif
		flagsSet = true;
	}

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

	if (loadOverlaysFromRam && src >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && src < ndsHeader->arm7romOffset) {
		return cardReadRAM(dst, src, len);
	}

	#ifdef DLDI
	return cardReadNormal(dst, src, len);
	#else
	return ce9->ROMinRAM ? cardReadRAM(dst, src, len) : cardReadNormal(dst, src, len);
	#endif
}

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerIPC");
	#endif	

#ifndef DLDI
    if(ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef) { // new dma method  
        continueCardReadDmaArm7();
        continueCardReadDmaArm9();
    }
#endif
}
