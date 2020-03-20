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
#include <nds/arm9/video.h>
#include <nds/system.h>
#include <nds/dma.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
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

#define ICACHE_SIZE      0x2000      
#define DCACHE_SIZE      0x1000      
#define CACHE_LINE_SIZE  32

#define THRESHOLD_CACHE_FLUSH 0x500

#define END_FLAG   0
#define BUSY_FLAG   4  

//extern void user_exception(void);

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static u32 romLocation = ROM_LOCATION;
#ifdef DLDI
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_MAINMEM;
static aFile* savFile = (aFile*)SAV_FILE_LOCATION_MAINMEM;

bool sdRead = false;
#else
/*static u32 cacheDescriptor[dev_CACHE_SLOTS_32KB] = {0xFFFFFFFF};
static u32 cacheCounter[dev_CACHE_SLOTS_32KB];*/
static u32* cacheDescriptor = (u32*)0x02710000;
static u32* cacheCounter = (u32*)0x02712000;
static u32 accessCounter = 0;

static u32 cacheAddress = CACHE_ADRESS_START;
static u16 cacheSlots = retail_CACHE_SLOTS_16KB;
#endif
static u32 overlaysSize = 0;

static bool flagsSet = false;
static bool loadOverlaysFromRam = true;
static bool isDma = false;
static bool dmaLed = false;
static bool dmaReadOnArm7 = false;
static bool dmaReadOnArm9 = false;

void myIrqHandlerDMA(void);

void SetBrightness(u8 screen, s8 bright) {
	u16 mode = 1 << 14;

	if (bright < 0) {
		mode = 2 << 14;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(u16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

// Alternative to swiWaitForVBlank()
static void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

#ifndef DLDI
static int allocateCacheSlot(void) {
	int slot = 0;
	u32 lowerCounter = accessCounter;
	for (int i = 0; i < cacheSlots; i++) {
		if (*(cacheCounter+i) <= lowerCounter) {
			lowerCounter = *(cacheCounter+i);
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
		if (*(cacheDescriptor+i) == sector) {
			return i;
		}
	}
	return -1;
}

static vu8* getCacheAddress(int slot) {
	//return (vu32*)(cacheAddress + slot*ce9->cacheBlockSize);
	return (vu8*)(cacheAddress + slot*ce9->cacheBlockSize);
}

static void updateDescriptor(int slot, u32 sector) {
	*(cacheDescriptor+slot) = sector;
	*(cacheCounter+slot) = accessCounter;
}
#endif

/*static void sleep(u32 ms) {
    if(ce9->patches->sleepRef) {
        volatile void (*sleepRef)(u32) = ce9->patches->sleepRef;
        (*sleepRef)(ms);
    } else if(ce9->thumbPatches->sleepRef) {
        callSleepThumb(ms);
    }    
}*/


static void waitForArm7(void) {
    IPC_SendSync(0x4);
    int count = 0;
    /*if (ce9->patches->sleepRef || ce9->thumbPatches->sleepRef) {
        while (sharedAddr[3] != (vu32)0) {
           if(count==0) {
                sleep(1);
                IPC_SendSync(0x4);
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

#ifndef DLDI
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

/*static inline 
void ndmaCopyWordsAsynchTimer1(uint8 ndmaSlot, const void* src, void* dest, uint32 size) {
	*(u32*)(0x4004104+(ndmaSlot*0x1C)) = src;
	*(u32*)(0x4004108+(ndmaSlot*0x1C)) = dest;

    *(u32*)(0x400410C+(ndmaSlot*0x1C)) = size/4;
	
	*(u32*)(0x4004110+(ndmaSlot*0x1C)) = 0x80;
	
    *(u32*)(0x4004114+(ndmaSlot*0x1C)) = 0x1;
	
	*(u32*)(0x400411C+(ndmaSlot*0x1C)) = 0x81070000;
} */

static inline 
bool ndmaBusy(uint8 ndmaSlot) {
	return	*(u32*)(0x400411C+(ndmaSlot*0x1C)) & BIT(31) == 0x80000000;
}

static inline bool checkArm7(void) {
    IPC_SendSync(0x4);
	return (sharedAddr[3] == (vu32)0);
}
#endif

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
        u32* ipcSyncHandler = ce9->irqTable + 16;
        ce9->intr_ipc_orig_return = *ipcSyncHandler;
        *ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

static void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}


static void clearIcache (void) {
      // Seems to have no effect
      // disable interrupt
      /*int oldIME = enterCriticalSection();
      IC_InvalidateAll();
      // restore interrupt
      leaveCriticalSection(oldIME);*/
}

void endCardReadDma() {
    if(ce9->patches->cardEndReadDmaRef) {
        volatile void (*cardEndReadDmaRef)() = ce9->patches->cardEndReadDmaRef;
        (*cardEndReadDmaRef)();
    } else if(ce9->thumbPatches->cardEndReadDmaRef) {
        callEndReadDmaThumb();
    }    
}

#ifndef DLDI
static int currentLen=0;

void continueCardReadDmaArm9() {
    if(dmaReadOnArm9) {
        vu32* volatile cardStruct = ce9->cardStruct0;
        u32	dma = cardStruct[3]; // dma channel

        if(ndmaBusy(0)) return;

        dmaReadOnArm9 = false;
        sharedAddr[3] = 0;

        u32 commandRead=0x025FFB0A;
        u32 commandPool=0x025AAB08;

        u32 src = cardStruct[0];
        u8* dst = (u8*)(cardStruct[1]);
        u32 len = cardStruct[2];

        // Update cardi common
  		cardStruct[0] = src + currentLen;
  		cardStruct[1] = (vu32)(dst + currentLen);
  		cardStruct[2] = len - currentLen;

        src = cardStruct[0];
        dst = (u8*)(cardStruct[1]);
        len = cardStruct[2]; 

        u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

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
        		sharedAddr[1] = ce9->cacheBlockSize;
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
        		if ((src - sector) + len2 > ce9->cacheBlockSize) {
        			len2 = sector - src + ce9->cacheBlockSize;
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

        u32 src = cardStruct[0];
        u8* dst = (u8*)(cardStruct[1]);
        u32 len = cardStruct[2];
        u32	dma = cardStruct[3]; // dma channel

		u32 page = (src / 512) * 512;

		/*if (page == src && len > ce9->cacheBlockSize && (u32)dst < 0x02700000 && (u32)dst > 0x02000000 && (u32)dst % 4 == 0) {
			sharedAddr[3] = 0;
			endCardReadDma();
		} else {*/
			u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

			u32 len2 = len;
			if ((src - sector) + len2 > ce9->cacheBlockSize) {
				len2 = sector - src + ce9->cacheBlockSize;
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
		//}
    }
}
#endif

void cardSetDma(void) {
	vu32* volatile cardStruct = ce9->cardStruct0;

    disableIrqMask(IRQ_CARD);
    disableIrqMask(IRQ_CARD_LINE);

	enableIPC_SYNC();

	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];
    u32 dma = cardStruct[3]; // dma channel     

	#ifdef DLDI
	while (sharedAddr[3]==0x52414D44);	// Wait during a RAM dump
	fileRead((char*)dst, *romFile, src, len, 0);
	endCardReadDma();
	#else
	u32 commandRead=0x025FFB0A;
	u32 commandPool=0x025AAB08;
	u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;
	u32 page = (src / 512) * 512;

	if (ce9->ROMinRAM) {
  		u32 len2 = len;
  		if (len2 > 512) {
  			len2 -= src % 4;
  			len2 -= len2 % 32;
  		}

        /*ndmaCopyWordsAsynch(0, (u8*)((romLocation-0x4000-ndsHeader->arm9binarySize)+src), dst, len2);
        while (ndmaBusy(0));
		endCardReadDma();*/

  		// Copy via dma
        ndmaCopyWordsAsynch(0, (u8*)((romLocation-0x4000-ndsHeader->arm9binarySize)+src), dst, len2);
        dmaReadOnArm9 = true;
        currentLen = len2;

        sharedAddr[3] = commandPool;
        IPC_SendSync(0x3);        

		return;
	}

	accessCounter++;  

	/*if (page == src && len > ce9->cacheBlockSize && (u32)dst < 0x02700000 && (u32)dst > 0x02000000 && (u32)dst % 4 == 0) {
		// Read directly at ARM7 level
		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		// do not wait for arm7 and return immediately
		checkArm7();

		dmaReadOnArm7 = true;
	} else {*/
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
			sharedAddr[1] = ce9->cacheBlockSize;
			sharedAddr[2] = sector;
			sharedAddr[3] = commandRead;

			// do not wait for arm7 and return immediately
			checkArm7();

			dmaReadOnArm7 = true;

			updateDescriptor(slot, sector);
		} else {
			updateDescriptor(slot, sector);	

			u32 len2 = len;
			if ((src - sector) + len2 > ce9->cacheBlockSize) {
				len2 = sector - src + ce9->cacheBlockSize;
			}

			if (len2 > 512) {
				len2 -= src % 4;
				len2 -= len2 % 32;
			}

			// Copy via dma
			ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
			dmaReadOnArm9 = true;
			currentLen = len2;

			sharedAddr[3] = commandPool;
			IPC_SendSync(0x3);
		}
	//}
	#endif
}

static inline int cardReadNormal(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page) {
#ifdef DLDI
	while (sharedAddr[3]==0x52414D44);	// Wait during a RAM dump
	fileRead((char*)dst, *romFile, src, len, 0);
#else
	u32 commandRead;
	u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

	accessCounter++;

	/*if (page == src && len > ce9->cacheBlockSize && (u32)dst < 0x02700000 && (u32)dst > 0x02000000 && (u32)dst % 4 == 0) {
		// Read directly at ARM7 level
		commandRead = (dmaLed ? 0x025FFB0A : 0x025FFB08);

		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		waitForArm7();

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
				sharedAddr[1] = ce9->cacheBlockSize;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				//REG_IME = 0;
				waitForArm7();
				//REG_IME = 1;
			}

			updateDescriptor(slot, sector);	
	
			u32 len2 = len;
			if ((src - sector) + len2 > ce9->cacheBlockSize) {
				len2 = sector - src + ce9->cacheBlockSize;
			}

            /*if (isDma) {
                // Copy via dma
  				dmaCopyWordsAsynchIrq(dma, (u8*)buffer+(src-sector), dst, len2);
                while (dmaBusy(dma)) {
                    sleep(1);
                }        
            } else {*/
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
    
    			// Copy directly
    			tonccpy(dst, (u8*)buffer+(src-sector), len2);
            //}
    		// Update cardi common
    		cardStruct[0] = src + len2;
    		cardStruct[1] = (vu32)(dst + len2);
    		cardStruct[2] = len - len2;

			len = cardStruct[2];
			if (len > 0) {
				src = cardStruct[0];
				dst = (u8*)cardStruct[1];
				page = (src / 512) * 512;
				sector = (src / ce9->cacheBlockSize) * ce9->cacheBlockSize;
				accessCounter++;
			}
		}
	//}
#endif

	if(strncmp(getRomTid(ndsHeader), "CLJ", 3) == 0){
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}

	return 0;
}

static inline int cardReadRAM(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page) {
	//u32 commandRead;
	while (len > 0) {
		/*if (isDma) {
            // Copy via dma
  			dmaCopyWordsAsynchIrq(dma, (u8*)(((ce9->dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src), dst, len);
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
			sharedAddr[2] = (romLocation-0x4000-ndsHeader->arm9binarySize)+src;
			sharedAddr[3] = commandRead;

			waitForArm7();
			// -------------------------------------
			#endif

			// Copy directly
			tonccpy(dst, (u8*)((romLocation-0x4000-ndsHeader->arm9binarySize)+src),len);
		//}
		// Update cardi common
		cardStruct[0] = src + len;
		cardStruct[1] = (vu32)(dst + len);	
		cardStruct[2] = len - len;

		len = cardStruct[2];
		if (len > 0) {
			src = cardStruct[0];
			dst = (u8*)cardStruct[1];
			page = (src / 512) * 512;
		}
	}

	return 0;
}

//Currently used for NSMBDS romhacks
void __attribute__((target("arm"))) debug8mbMpuFix(){
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}

bool isNotTcm(u32 address, u32 len) {
    u32 base = (getDtcmBase()>>12) << 12;
    return    // test data not in ITCM
    address > 0x02000000
    // test data not in DTCM
    && (address < base || address> base+0x4000)
    && (address+len < base || address+len> base+0x4000);     
}  

u32 cardReadDma() {
	vu32* volatile cardStruct = ce9->cardStruct0;
    
	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];
    u32 dma = cardStruct[3]; // dma channel

    if(dma >= 0 
        && dma <= 3 
        //&& func != NULL
        && len > 0
        && !(((int)dst) & 3)
        && isNotTcm(dst, len)
        // check 512 bytes page alignement 
        && !(((int)len) & 511)
        && !(((int)src) & 511)
	) {
		dmaLed = true;

        if(ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef)
		{
			isDma = true;
			// new dma method

            cacheFlush();

            cardSetDma();

            return true;
		} else {
			isDma = false;
			dma=4;
            clearIcache();
		}
    } else {
		dmaLed = false;
        isDma = false;
        dma=4;
        clearIcache();
    }

    return 0;
}

static int counter=0;
int cardReadPDash(u32* cacheStruct, u32 src, u8* dst, u32 len) {
#ifndef DLDI
    dmaLed = true;
#endif

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

    cardStruct[0] = src;
    cardStruct[1] = (vu32)dst;
    cardStruct[2] = len;

    cardRead(cacheStruct, dst, src, len);

    counter++;
	return counter;
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		#ifdef DLDI
		if (!FAT_InitFiles(false, 0)) {
			//nocashMessage("!FAT_InitFiles");
			//return -1;
		}

		const char* romTid = getRomTid(ndsHeader);
		if (strncmp(romTid, "UBR", 3) == 0) {
			loadOverlaysFromRam = false;
		} else {
			if (ce9->dsiMode) {
				romLocation = ROM_SDK5_LOCATION;
				if (ce9->consoleModel == 0) {
					romLocation = retail_CACHE_ADRESS_START_SDK5;
				}
			}

			for (int i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i <= ndsHeader->arm7romOffset; i++) {
				overlaysSize = i;
			}

			if (ce9->consoleModel>0 ? overlaysSize<=0x1800000 : overlaysSize<=0x800000) {} else {
				loadOverlaysFromRam = false;
			}

			debug8mbMpuFix();
		}
		#else
		const char* romTid = getRomTid(ndsHeader);
		if (!ce9->ROMinRAM && strncmp(romTid, "UBR", 3) == 0) {
			loadOverlaysFromRam = false;
			cacheAddress = CACHE_ADRESS_START_low;
			cacheSlots = retail_CACHE_ADRESS_SIZE_low/ce9->cacheBlockSize;
		} else {
			if (ce9->dsiMode) {
				romLocation = ROM_SDK5_LOCATION;
				cacheAddress = dev_CACHE_ADRESS_START_SDK5;
				/*if (ce9->consoleModel == 0) {
					romLocation = retail_CACHE_ADRESS_START_SDK5;
					cacheAddress = retail_CACHE_ADRESS_START_SDK5;
				}*/
			}

			if (ce9->consoleModel > 0) {
				cacheSlots = (ce9->dsiMode ? dev_CACHE_ADRESS_SIZE_SDK5 : dev_CACHE_ADRESS_SIZE)/ce9->cacheBlockSize;
			} else {
				//cacheSlots = (ce9->dsiMode ? retail_CACHE_SLOTS_16KB_low : retail_CACHE_SLOTS_16KB);
				cacheSlots = retail_CACHE_ADRESS_SIZE/ce9->cacheBlockSize;
			}

			if (!ce9->ROMinRAM) {
				for (int i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i <= ndsHeader->arm7romOffset; i++) {
					overlaysSize = i;
				}

				if (ce9->consoleModel>0 ? overlaysSize<=0x1800000 : overlaysSize<=0x800000) {
					for (int i = 0; i < overlaysSize; i += ce9->cacheBlockSize) {
						cacheAddress += ce9->cacheBlockSize;
						cacheSlots--;
					}
				} else {
					loadOverlaysFromRam = false;
				}
			}

			debug8mbMpuFix();
		}

		//ndsHeader->romSize += 0x1000;

		if (ce9->enableExceptionHandler && ce9==CARDENGINE_ARM9_LOCATION) {
			//exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
			//setExceptionHandler(user_exception);
		}
		#endif
		
		flagsSet = true;
	}
	
	enableIPC_SYNC();

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];

	u32 page = (src / 512) * 512;

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
	// -------------------------------------*/
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
		return cardReadRAM(cardStruct, cacheStruct, dst, src, len, page);
	}

	#ifdef DLDI
	int ret = cardReadNormal(cardStruct, cacheStruct, dst, src, len, page);
	#else
	int ret = ce9->ROMinRAM ? cardReadRAM(cardStruct, cacheStruct, dst, src, len, page) : cardReadNormal(cardStruct, cacheStruct, dst, src, len, page);
	#endif

    isDma=false;

	return ret; 
}

void cardPullOut(void) {
	/*if (!ce9->ROMinRAM && *(vu32*)(0x027FFB30) != 0) {
		/*volatile int (*terminateForPullOutRef)(u32*) = *ce9->patches->terminateForPullOutRef;
        (*terminateForPullOutRef);
		sharedAddr[3] = 0x5245424F;
		waitForArm7();
	}*/
}

u32 nandRead(void* memory,void* flash,u32 len,u32 dma) {
	if (ce9->saveOnFlashcard) {
#ifdef DLDI
		fileRead(memory, *savFile, flash, len, -1);
#endif
		return 0;
	}

    // Send a command to the ARM7 to read the nand save
	u32 commandNandRead = 0x025FFC01;

	// Write the command
	sharedAddr[0] = memory;
	sharedAddr[1] = len;
	sharedAddr[2] = flash;
	sharedAddr[3] = commandNandRead;

	waitForArm7();
    return 0; 
}

u32 nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	if (ce9->saveOnFlashcard) {
#ifdef DLDI
		fileWrite(memory, *savFile, flash, len, -1);
#endif
		return 0;
	}

	// Send a command to the ARM7 to write the nand save
	u32 commandNandWrite = 0x025FFC02;

	// Write the command
	sharedAddr[0] = memory;
	sharedAddr[1] = len;
	sharedAddr[2] = flash;
	sharedAddr[3] = commandNandWrite;

	waitForArm7();
    return 0; 
}

u32 slot2Read(u8* dst, u32 src, u32 len, u32 dma) {
	// Send a command to the ARM7 to read the GBA ROM
	/*u32 commandRead = 0x025FBC01;

	// Write the command
	sharedAddr[0] = (vu32)dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	waitForArm7();*/
    return 0; 
}

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerIPC");
	#endif	

#ifndef DLDI
	if (sharedAddr[4] == 0x025AAB08) {
		if(ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef) { // new dma method  
			continueCardReadDmaArm7();
			continueCardReadDmaArm9();
		}
		sharedAddr[4] = 0;
	}
#endif

	if (sharedAddr[4] == 0x57534352) {
		enterCriticalSection();
		// Make screens white
		SetBrightness(0, 31);
		SetBrightness(1, 31);

		while (1);
	}
}

void reset(u32 param) {
	if (ce9->consoleModel < 2) {
		// Make screens white
		SetBrightness(0, 31);
		SetBrightness(1, 31);
		waitFrames(5);	// Wait for DSi screens to stabilize
	}
	enterCriticalSection();
	*(u32*)RESET_PARAM = param;
	sharedAddr[3] = 0x52534554;
	while (1);
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	hookIPC_SYNC();

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
