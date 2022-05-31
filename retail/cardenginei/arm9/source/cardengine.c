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
#include <nds/dma.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "module_params.h"
#include "ndma.h"
#include "tonccpy.h"
#include "hex.h"
#include "igm_text.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"

#define saveOnFlashcard BIT(0)
#define extendedMemory BIT(1)
#define dsiMode BIT(3)
#define enableExceptionHandler BIT(4)
#define overlaysInRam BIT(6)
#define cacheFlushFlag BIT(7)
#define cardReadFix BIT(8)
#define cacheDisabled BIT(9)
#define slowSoftReset BIT(10)
#define dsiBios BIT(11)
#define asyncCardRead BIT(12)

//#ifdef DLDI
#include "my_fat.h"
#include "card.h"
//#endif

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

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
aFile* romFile = (aFile*)ROM_FILE_LOCATION_MAINMEM;
aFile* savFile = (aFile*)SAV_FILE_LOCATION_MAINMEM;
//static aFile* gbaFile = (aFile*)GBA_FILE_LOCATION_MAINMEM;
//static aFile* gbaSavFile = (aFile*)GBA_SAV_FILE_LOCATION_MAINMEM;
#ifdef DLDI
bool sdRead = false;
#else
//static u32 sdatAddr = 0;
//static u32 sdatSize = 0;
u32* cacheDescriptor = (u32*)0x02790000;
u32* cacheCounter = (u32*)0x027A0000;
u32 accessCounter = 0;
#endif
static bool flagsSet = false;
static bool driveInitialized = false;
static bool region0FixNeeded = false;
static bool igmReset = false;

extern bool isDma;

extern void continueCardReadDmaArm7();
extern void continueCardReadDmaArm9();

s8 mainScreen = 0;

void myIrqHandlerDMA(void);

static void SetBrightness(u8 screen, s8 bright) {
	u8 mode = 1;

	if (bright < 0) {
		mode = 2;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(vu16*)(0x0400006C + (0x1000 * screen)) = bright | (mode << 14);
}

// Alternative to swiWaitForVBlank()
static void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

/*static void waitMs(int count) {
	for (int i = 0; i < count; i++) {
		while ((REG_VCOUNT % 32) != 31);
		while ((REG_VCOUNT % 32) == 31);
	}
}*/

#ifndef DLDI
void sleepMs(int ms) {
	if (!(ce9->valueBits & asyncCardRead) || REG_IME == 0 || REG_IF == 0) {
		swiDelay(50);
		return;
	}

	if(ce9->patches->sleepRef) {
		volatile void (*sleepRef)(int ms) = (volatile void*)ce9->patches->sleepRef;
		(*sleepRef)(ms);
	} else if(ce9->thumbPatches->sleepRef) {
		extern void callSleepThumb(int ms);
		callSleepThumb(ms);
	} else {
		swiDelay(50);
	}
}

/*static void getSdatAddr(u32 sector, u32 buffer) {
	if ((!ce9->patches->sleepRef && !ce9->thumbPatches->sleepRef) || sdatSize != 0) return;

	for (u32 i = 0; i < ce9->cacheBlockSize; i+=4) {
		if (*(u32*)(buffer+i) == 0x54414453 && *(u32*)(buffer+i+8) <= 0x20000000) {
			sdatAddr = sector+i;
			sdatSize = *(u32*)(buffer+i+8);
			break;
		}
	}
}*/

int allocateCacheSlot(void) {
	int slot = 0;
	u32 lowerCounter = accessCounter;
	for (int i = 0; i < ce9->cacheSlots; i++) {
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

int getSlotForSector(u32 sector) {
	for (int i = 0; i < ce9->cacheSlots; i++) {
		if (cacheDescriptor[i] == sector) {
			return i;
		}
	}
	return -1;
}

/*int getSlotForSectorManual(int i, u32 sector) {
	if (i >= ce9->cacheSlots) {
		i -= ce9->cacheSlots;
	}
	if (cacheDescriptor[i] == sector) {
		return i;
	}
	return -1;
}*/

vu8* getCacheAddress(int slot) {
	//return (vu32*)(ce9->cacheAddress + slot*ce9->cacheBlockSize);
	return (vu8*)(ce9->cacheAddress + slot*ce9->cacheBlockSize);
}

void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
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


extern void setExceptionHandler2();

static inline void waitForArm7(void) {
	IPC_SendSync(0x4);
	while (sharedAddr[3] != (vu32)0);
}

#ifndef DLDI
static inline bool checkArm7(void) {
    IPC_SendSync(0x4);
	return (sharedAddr[3] == (vu32)0);
}
#endif

bool IPC_SYNC_hooked = false;
void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
        u32* vblankHandler = ce9->irqTable;
        u32* ipcSyncHandler = ce9->irqTable + 16;
		ce9->intr_vblank_orig_return = *vblankHandler;
		ce9->intr_ipc_orig_return = *ipcSyncHandler;
        *vblankHandler = ce9->patches->vblankHandlerRef;
        *ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}


//static void clearIcache (void) {
      // Seems to have no effect
      // disable interrupt
      /*int oldIME = enterCriticalSection();
      IC_InvalidateAll();
      // restore interrupt
      leaveCriticalSection(oldIME);*/
//}

static inline void cardReadNormal(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len) {
#ifdef DLDI
	while (sharedAddr[3]==0x444D4152);	// Wait during a RAM dump
	fileRead((char*)dst, *romFile, src, len, 0);
#else
	u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

	accessCounter++;

	#ifdef ASYNCPF
	processAsyncCommand();
	#endif

	//if (src >= sdatAddr && src < sdatAddr+sdatSize) {
	//	sleepMsEnabled = true;
	//}

	/*if (ce9->valueBits & cacheDisabled) {
		fileRead((char*)dst, *romFile, src, len, 0);
	} else {*/
		// Read via the main RAM cache
		//bool runSleep = true;
		while(len > 0) {
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			#ifdef ASYNCPF
			u32 nextSector = sector+ce9->cacheBlockSize;
			#endif
			// Read max CACHE_READ_SIZE via the main RAM cache
			if (slot == -1) {
				#ifdef ASYNCPF
				getAsyncSector();
				#endif

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				/*u32 len2 = (src - sector) + len;
				u16 readLen = ce9->cacheBlockSize;
				if (len2 > ce9->cacheBlockSize*3 && slot+3 < ce9->cacheSlots) {
					readLen = ce9->cacheBlockSize*4;
				} else if (len2 > ce9->cacheBlockSize*2 && slot+2 < ce9->cacheSlots) {
					readLen = ce9->cacheBlockSize*3;
				} else if (len2 > ce9->cacheBlockSize && slot+1 < ce9->cacheSlots) {
					readLen = ce9->cacheBlockSize*2;
				}*/

				fileRead((char*)buffer, *romFile, sector, ce9->cacheBlockSize, 0);
				/*updateDescriptor(slot, sector);
				if (readLen >= ce9->cacheBlockSize*2) {
					updateDescriptor(slot+1, sector+ce9->cacheBlockSize);
				}
				if (readLen >= ce9->cacheBlockSize*3) {
					updateDescriptor(slot+2, sector+(ce9->cacheBlockSize*2));
				}
				if (readLen >= ce9->cacheBlockSize*4) {
					updateDescriptor(slot+3, sector+(ce9->cacheBlockSize*3));
				}*/

				#ifdef ASYNCPF
				if (REG_IME != 0 && REG_IF != 0) {
					triggerAsyncPrefetch(nextSector);
				}
				#endif
				//runSleep = false;
			} else {
				#ifdef ASYNCPF
				if(cacheCounter[slot] == 0x0FFFFFFF) {
					// prefetch successfull
					getAsyncSector();

					triggerAsyncPrefetch(nextSector);
				} /*else {
					int i;
					for(i=0; i<5; i++) {
						if(asyncQueue[i]==sector) {
							// prefetch successfull
							triggerAsyncPrefetch(nextSector);
							break;
						}
					}
				}*/
				#endif
				//updateDescriptor(slot, sector);
			}
			updateDescriptor(slot, sector);

			//getSdatAddr(sector, (u32)buffer);

			u32 len2 = len;
			if ((src - sector) + len2 > ce9->cacheBlockSize) {
				len2 = sector - src + ce9->cacheBlockSize;
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
    		// -------------------------------------
    		#endif
    
    		// Copy directly
			/*if (isDma) {
				ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
				while (ndmaBusy(0)) {
					if (runSleep) {
						sleepMs(1);
					}
				}
			} else {*/
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
				sector = (src / ce9->cacheBlockSize) * ce9->cacheBlockSize;
				accessCounter++;
				//slot = getSlotForSectorManual(slot+1, sector);
			}
		}
	//}
#endif

	//sleepMsEnabled = false;

	if (ce9->valueBits & cacheFlushFlag) {
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}
}

static inline void cardReadRAM(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len) {
	while (len > 0) {
		#ifdef DEBUG
		// Send a log command for debug purpose
		// -------------------------------------
		commandRead = 0x026ff800;

		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = (ce9->romLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+src;
		sharedAddr[3] = commandRead;

		waitForArm7();
		// -------------------------------------
		#endif

		// Copy directly
		tonccpy(dst, (u8*)((ce9->romLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+src),len);

		// Update cardi common
		cardStruct[0] = src + len;
		cardStruct[1] = (vu32)(dst + len);	
		cardStruct[2] = len - len;

		len = cardStruct[2];
		if (len > 0) {
			src = cardStruct[0];
			dst = (u8*)cardStruct[1];
		}
	}
}

bool isNotTcm(u32 address, u32 len) {
    u32 base = (getDtcmBase()>>12) << 12;
    return    // test data not in ITCM
    address > 0x02000000
    // test data not in DTCM
    && (address < base || address> base+0x4000)
    && (address+len < base || address+len> base+0x4000);
}  

static int counter=0;
int cardReadPDash(u32* cacheStruct, u32 src, u8* dst, u32 len) {
	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

    cardStruct[0] = src;
    cardStruct[1] = (vu32)dst;
    cardStruct[2] = len;

    cardRead(cacheStruct);

    counter++;
	return counter;
}

//extern void region2Disable();

// Required for proper access to the extra DSi RAM
extern void debugRamMpuFix();

// Revert region 0 patch
extern void region0Fix();

void cardRead(u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		if (region0FixNeeded) {
			region0Fix();
		}
		debugRamMpuFix();
		if (!driveInitialized) {
			FAT_InitFiles(false, 0);
			driveInitialized = true;
		}
		if (ce9->valueBits & enableExceptionHandler) {
			setExceptionHandler2();
		}
		flagsSet = true;
	}

	enableIPC_SYNC();

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];

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

	if ((ce9->valueBits & cardReadFix) && src < 0x8000) {
		// Fix reads below 0x8000
		src = 0x8000 + (src & 0x1FF);
	}

	if ((ce9->valueBits & overlaysInRam) && src >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && src < ndsHeader->arm7romOffset) {
		cardReadRAM(cardStruct, cacheStruct, dst, src, len);
	} else {
		cardReadNormal(cardStruct, cacheStruct, dst, src, len);
	}
    isDma=false;
}

void cardPullOut(void) {
	/*if (*(vu32*)(0x027FFB30) != 0) {
		/*volatile int (*terminateForPullOutRef)(u32*) = *ce9->patches->terminateForPullOutRef;
        (*terminateForPullOutRef);
		sharedAddr[3] = 0x5245424F;
		waitForArm7();
	}*/
}

bool nandRead(void* memory,void* flash,u32 len,u32 dma) {
	if (ce9->valueBits & saveOnFlashcard) {
#ifdef DLDI
		fileRead(memory, *savFile, flash, len, -1);
#endif
		return true;
	}

    // Send a command to the ARM7 to read the nand save
	u32 commandNandRead = 0x025FFC01;

	// Write the command
	sharedAddr[0] = memory;
	sharedAddr[1] = len;
	sharedAddr[2] = flash;
	sharedAddr[3] = commandNandRead;

	waitForArm7();
    return true; 
}

bool nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	if (ce9->valueBits & saveOnFlashcard) {
#ifdef DLDI
		fileWrite(memory, *savFile, flash, len, -1);
#endif
		return true;
	}

	// Send a command to the ARM7 to write the nand save
	u32 commandNandWrite = 0x025FFC02;

	// Write the command
	sharedAddr[0] = memory;
	sharedAddr[1] = len;
	sharedAddr[2] = flash;
	sharedAddr[3] = commandNandWrite;

	waitForArm7();
    return true; 
}

u32 cartRead(u32 dma, u32 src, u8* dst, u32 len, u32 type) {
	/*if (src >= 0x02000000 ? (gbaSavFile->firstCluster == CLUSTER_FREE) : (gbaFile->firstCluster == CLUSTER_FREE)) {
		return false;
	}

	fileRead(dst, src >= 0x02000000 ? *gbaSavFile : *gbaFile, src, len, 0);
	return true;*/
	return 0;
}

void __attribute__((target("arm"))) resetMpu(void) {
	asm("LDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0");
}

void reset(u32 param) {
	*(u32*)RESET_PARAM = param;
	if ((ce9->valueBits & slowSoftReset) || *(u32*)(RESET_PARAM+0xC) > 0) {
		if (ce9->consoleModel < 2) {
			// Make screens white
			SetBrightness(0, 31);
			SetBrightness(1, 31);
			waitFrames(5);	// Wait for DSi screens to stabilize
		}
		enterCriticalSection();
		cacheFlush();
		sharedAddr[3] = 0x52534554;
		while (1);
	} else {
		sharedAddr[3] = 0x52534554;
	}

 	register int i, reg;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	cacheFlush();
	resetMpu();

	if (igmReset) {
		igmReset = false;
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);
	}

	// Clear out ARM9 DMA channels
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	for (i = 0; i < 4; i++) {
		for(reg=0; reg<0x1c; reg+=4)*((vu32*)(0x04004104 + ((i*0x1c)+reg))) = 0;//Reset NDMA.
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	flagsSet = false;
	IPC_SYNC_hooked = false;

	sharedAddr[0] = 0x544F4F42; // 'BOOT'
	sharedAddr[3] = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	VoidFn arm9code = (VoidFn)ndsHeader->arm9executeAddress;
	arm9code();
}

//---------------------------------------------------------------------------------
void myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

	if (sharedAddr[4] == 0x554E454D) {
		while (sharedAddr[4] != 0x54495845);
	}
}

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerIPC");
	#endif

	switch (IPC_GetSync()) {
#ifndef DLDI
		case 0x3:
		if(ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef) { // new dma method  
			continueCardReadDmaArm7();
			continueCardReadDmaArm9();
		}
			break;
		case 0x4:
			extern bool dmaOn;
			dmaOn = !dmaOn;
			break;
#endif
		case 0x6:
			if(mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
			break;
		case 0x7: {
			mainScreen++;
			if(mainScreen > 2)
				mainScreen = 0;

			if(mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
		}
			break;
		case 0x9: {
			*(u32*)(INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED + 0x10;
			(*inGameMenu)(&mainScreen, ce9->consoleModel);
			if (sharedAddr[3] == 0x52534554) {
				igmReset = true;
				reset(0);
			}
		}
			break;
	}

	if (sharedAddr[4] == (vu32)0x57534352){
		enterCriticalSection();
		if (ce9->consoleModel < 2) {
			// Make screens white
			SetBrightness(0, 31);
			SetBrightness(1, 31);
		}
		cacheFlush();
		while (1);
	}
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	if (ce9->valueBits & enableExceptionHandler) {
		setExceptionHandler2();
	}

	if (unpatchedFuncs->mpuDataOffset) {
		region0FixNeeded = unpatchedFuncs->mpuInitRegionOldData == 0x4000033;
	}

	hookIPC_SYNC();

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
