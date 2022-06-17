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
#include <nds/timers.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
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
#define ROMinRAM BIT(2)
#define dsiMode BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define overlaysInRam BIT(6)
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

//extern vu32* volatile cacheStruct;

extern cardengineArm9* volatile ce9;

static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION_SDK5;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
static aFile* romFile = (aFile*)ROM_FILE_LOCATION_SDK5;
#ifdef TWLSDK
static aFile* savFile = (aFile*)SAV_FILE_LOCATION_SDK5;
#endif
#ifdef DLDI
bool sdRead = false;
#else
//static u32 sdatAddr = 0;
//static u32 sdatSize = 0;
#ifdef TWLSDK
u32 cacheDescriptor[dev_CACHE_SLOTS_16KB_SDK5];
u32 cacheCounter[dev_CACHE_SLOTS_16KB_SDK5];
#else
u32* cacheDescriptor = (u32*)0x02790000;
u32* cacheCounter = (u32*)0x027A0000;
#endif
u32 accessCounter = 0;

#if ASYNCPF
static u32 asyncSector = 0;
/*static u32 asyncQueue[5];
static int aQHead = 0;
static int aQTail = 0;
static int aQSize = 0;*/
#endif
#endif

static bool flagsSet = false;
static bool driveInitialized = false;
#ifndef TWLSDK
static bool region0FixNeeded = false;
#endif
static bool igmReset = false;

extern bool isDma;

extern void continueCardReadDmaArm7();
extern void continueCardReadDmaArm9();

s8 mainScreen = 0;

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

#ifdef TWLSDK
static void waitMs(int count) {
	for (int i = 0; i < count; i++) {
		while ((REG_VCOUNT % 32) != 31);
		while ((REG_VCOUNT % 32) == 31);
	}
}
#endif

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

/*#if ASYNCPF
void addToAsyncQueue(sector) {
	asyncQueue[aQHead] = sector;
	aQHead++;
	aQSize++;
	if(aQHead>4) {
		aQHead=0;
	}
	if(aQSize>5) {
		aQSize=5;
		aQTail++;
		if(aQTail>4) aQTail=0;
	}
}

u32 popFromAsyncQueueHead() {	
	if(aQSize>0) {
	
		aQHead--;
		if(aQHead == -1) aQHead = 4;
		aQSize--;
		
		return asyncQueue[aQHead];
	} else return 0;
}
#endif*/

static inline bool checkArm7(void) {
    IPC_SendSync(0x4);
	return (sharedAddr[3] == (vu32)0);
}
#endif

bool IPC_SYNC_hooked = false;
void hookIPC_SYNC(void) {
	if (!IPC_SYNC_hooked) {
		u32* ipcSyncHandler = ce9->irqTable + 16;
		ce9->intr_ipc_orig_return = *ipcSyncHandler;
		*ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
		IPC_SYNC_hooked = true;
	}
}

void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}


#ifndef DLDI
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

#ifdef TWLSDK
void resetSlots(void) {
	for (int i = 0; i < ce9->cacheSlots; i++) {
		cacheDescriptor[i] = 0;
		cacheCounter[i] = 0;
	}
	accessCounter = 0;
}
#endif

vu8* getCacheAddress(int slot) {
	//return (vu32*)(ce9->cacheAddress + slot*ce9->cacheBlockSize);
	return (vu8*)(ce9->cacheAddress + slot*ce9->cacheBlockSize);
}

void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}
#endif

void user_exception(void);
extern u32 exceptionAddr;

extern void setExceptionHandler2();

//#ifdef TWLSDK
static void waitForArm7(bool ipc) {
	if (!ipc) {
		IPC_SendSync(0x4);
	}
	while (sharedAddr[3] != (vu32)0) {
		if (ipc) {
			IPC_SendSync(0x4);
			sleepMs(1);
		}
	}
}
//#endif

#ifndef DLDI
#ifdef ASYNCPF
void triggerAsyncPrefetch(sector) {	
	if(asyncSector == 0) {
		int slot = getSlotForSector(sector);
		// read max 32k via the WRAM cache
		// do it only if there is no async command ongoing
		if(slot==-1) {
			//addToAsyncQueue(sector);
			// send a command to the arm7 to fill the main RAM cache
			u32 commandRead = (isDma ? 0x020FF80A : 0x020FF808);

			slot = allocateCacheSlot();

			vu8* buffer = getCacheAddress(slot);

			cacheDescriptor[slot] = sector;
			cacheCounter[slot] = 0x0FFFFFFF; // async marker
			asyncSector = sector;		

			// write the command
			sharedAddr[0] = buffer;
			sharedAddr[1] = ce9->cacheBlockSize;
			sharedAddr[2] = sector;
			sharedAddr[3] = commandRead;

			IPC_SendSync(0x4);

			// do it asynchronously
			/*waitForArm7();*/
		}	
	}	
}

void processAsyncCommand() {
	if(asyncSector != 0) {
		int slot = getSlotForSector(asyncSector);
		if(slot!=-1 && cacheCounter[slot] == 0x0FFFFFFF) {
			// get back the data from arm7
			if(sharedAddr[3] == (vu32)0) {
				updateDescriptor(slot, asyncSector);
				asyncSector = 0;
			}			
		}	
	}
}

void getAsyncSector() {
	if(asyncSector != 0) {
		int slot = getSlotForSector(asyncSector);
		if(slot!=-1 && cacheCounter[slot] == 0x0FFFFFFF) {
			// get back the data from arm7
			waitForArm7(true);

			updateDescriptor(slot, asyncSector);
			asyncSector = 0;
		}	
	}	
}
#endif
#endif

//static void clearIcache (void) {
      // Seems to have no effect
      // disable interrupt
      /*int oldIME = enterCriticalSection();
      IC_InvalidateAll();
      // restore interrupt
      leaveCriticalSection(oldIME);*/
//}

static inline void cardReadNormal(u8* dst, u32 src, u32 len) {
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

			len -= len2;
			if (len > 0) {
				src = src + len2;
				dst = (u8*)(dst + len2);
				sector = (src / ce9->cacheBlockSize) * ce9->cacheBlockSize;
				accessCounter++;
				//slot = getSlotForSectorManual(slot+1, sector);
			}
		}
	//}
#endif

	//sleepMsEnabled = false;
}

static inline void cardReadRAM(u8* dst, u32 src, u32 len) {
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
}

#ifdef TWLSDK
extern void openDebugRam();
#else
// Revert region 0 patch
extern void region0Fix();

extern void mpuFix();
#endif

bool isNotTcm(u32 address, u32 len) {
    u32 base = (getDtcmBase()>>12) << 12;
    return    // test data not in ITCM
    address > 0x02000000
    // test data not in DTCM
    && (address < base || address> base+0x4000)
    && (address+len < base || address+len> base+0x4000);
}  

void cardRead(u32 dma, u8* dst, u32 src, u32 len) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		#ifdef TWLSDK
		openDebugRam();
		#else
		mpuFix();
		if (region0FixNeeded) {
			region0Fix();
		}
		#endif
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

	if ((ce9->valueBits & overlaysInRam) && src >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && src < ndsHeader->arm7romOffset) {
		cardReadRAM(dst, src, len);
	} else {
		cardReadNormal(dst, src, len);
	}
    isDma=false;
}

bool nandRead(void* memory,void* flash,u32 len,u32 dma) {
#ifdef TWLSDK
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

	waitForArm7(false);
#endif
    return true; 
}

bool nandWrite(void* memory,void* flash,u32 len,u32 dma) {
#ifdef TWLSDK
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

	waitForArm7(false);
#endif
    return true; 
}

#ifdef TWLSDK
void initMBKARM9_dsiMode(void) {
	*(vu32*)REG_MBK1 = *(u32*)0x02FFE180;
	*(vu32*)REG_MBK2 = *(u32*)0x02FFE184;
	*(vu32*)REG_MBK3 = *(u32*)0x02FFE188;
	*(vu32*)REG_MBK4 = *(u32*)0x02FFE18C;
	*(vu32*)REG_MBK5 = *(u32*)0x02FFE190;
	REG_MBK6 = *(u32*)0x02FFE194;
	REG_MBK7 = *(u32*)0x02FFE198;
	REG_MBK8 = *(u32*)0x02FFE19C;
	REG_MBK9 = *(u32*)0x02FFE1AC;
}
#endif

extern void resetMpu();

void reset(u32 param, u32 tid2) {
#ifndef TWLSDK
	*(u32*)RESET_PARAM_SDK5 = param;
	if ((ce9->valueBits & slowSoftReset) || *(u32*)(RESET_PARAM_SDK5+0xC) > 0) {
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
#else
	#ifdef DLDI
	sysSetCardOwner(false);	// Give Slot-1 access to arm7
	#endif
	if (param == 0xFFFFFFFF || *(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
		if (param == 0xFFFFFFFF || (param != *(u32*)0x02FFE230 && tid2 != *(u32*)0x02FFE234)) {
			/*if (ce9->consoleModel < 2) {
				// Make screens white
				SetBrightness(0, 31);
				SetBrightness(1, 31);
				waitFrames(5);	// Wait for DSi screens to stabilize
			}
			enterCriticalSection();
			cacheFlush();*/
			sharedAddr[3] = 0x54495845;
			//while (1);
		} else {
			sharedAddr[3] = 0x52534554;
		}
	} else {
		*(u32*)RESET_PARAM_SDK5 = param;
		sharedAddr[3] = 0x52534554;
	}
#endif

	register int i, reg;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	cacheFlush();
	resetMpu();

	if (igmReset) {
		igmReset = false;
#ifdef TWLSDK
		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = *(u32*)0x02FFE230;
			*(u32*)0x02FFC234 = *(u32*)0x02FFE234;
		}
#endif
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);
#ifdef TWLSDK
		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = 0;
			*(u32*)0x02FFC234 = 0;
		}
#endif
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

	flagsSet = false;
	IPC_SYNC_hooked = false;

#ifdef TWLSDK
	if (param == 0xFFFFFFFF || *(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
		REG_DISPSTAT = 0;
		REG_DISPCNT = 0;
		REG_DISPCNT_SUB = 0;

		toncset((u16*)0x04000000, 0, 0x56);
		toncset((u16*)0x04001000, 0, 0x56);

		VRAM_A_CR = 0x80;
		VRAM_B_CR = 0x80;
		VRAM_C_CR = 0x80;
		VRAM_D_CR = 0x80;
		VRAM_E_CR = 0x80;
		VRAM_F_CR = 0x80;
		VRAM_G_CR = 0x80;
		VRAM_H_CR = 0x80;
		VRAM_I_CR = 0x80;

		toncset16(BG_PALETTE, 0, 256); // Clear palettes
		toncset16(BG_PALETTE_SUB, 0, 256);
		toncset(VRAM, 0, 0xC0000); // Clear VRAM

		VRAM_A_CR = 0;
		VRAM_B_CR = 0;
		VRAM_C_CR = 0;
		VRAM_D_CR = 0;
		VRAM_E_CR = 0;
		VRAM_F_CR = 0;
		VRAM_G_CR = 0;
		VRAM_H_CR = 0;
		VRAM_I_CR = 0;
	}

	#ifndef DLDI
	if (ce9->consoleModel == 0) {
		resetSlots();
	}
	#endif

	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	if (ndsHeader->unitCode > 0 && sharedAddr[3] == 0x54495845) {
		initMBKARM9_dsiMode();
		REG_SCFG_EXT = 0x8307F100;
		REG_SCFG_CLK = 0x87;
		REG_SCFG_RST = 1;
	}

	#ifdef DLDI
	sysSetCardOwner(true);	// Give Slot-1 access back to arm9
	#endif
#else
	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
#endif

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

//#ifndef TWLSDK
	if (sharedAddr[4] == 0x554E454D) {
		while (sharedAddr[4] != 0x54495845);
	}
//#endif
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
		case 0x5:
			igmReset = true;
			sharedAddr[3] = 0x54495845;
			if (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) {
				reset(0, 0);
			} else {
				reset(0xFFFFFFFF, 0);
			}
			break;
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
#ifdef TWLSDK
			*(u32*)(INGAME_MENU_LOCATION_TWLSDK + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION_TWLSDK + IGM_TEXT_SIZE_ALIGNED + 0x10;
#else
			*(u32*)(INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED + 0x10;
#endif
			(*inGameMenu)(&mainScreen, ce9->consoleModel);
#ifdef TWLSDK
			if (sharedAddr[3] == 0x54495845) {
				igmReset = true;
				if (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) {
					reset(0, 0);
				} else {
					reset(0xFFFFFFFF, 0);
				}
			} else
#endif
			if (sharedAddr[3] == 0x52534554) {
				igmReset = true;
#ifdef TWLSDK
				if (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
					reset(*(u32*)0x02FFE230, *(u32*)0x02FFE234);
				} else {
#endif
					reset(0, 0);
#ifdef TWLSDK
				}
#endif
			}
		}
			break;
	}

	if (sharedAddr[4] == 0x57534352) {
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

	#ifndef TWLSDK
	if (unpatchedFuncs->mpuDataOffset) {
		region0FixNeeded = unpatchedFuncs->mpuInitRegionOldData == 0x4000033;
	}
	#endif

	hookIPC_SYNC();

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
