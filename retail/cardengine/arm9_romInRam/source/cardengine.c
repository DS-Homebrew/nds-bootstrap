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
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "hex.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define saveOnFlashcard BIT(0)
#define extendedMemory BIT(1)
#define ROMinRAM BIT(2)
#define dsiMode BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define overlaysInRam BIT(6)

//extern void user_exception(void);

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static bool flagsSet = false;
static bool isDma = false;
static bool dmaLed = false;

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

static int readCount = 0;
/*static bool sleepMsEnabled = false;

static void sleepMs(int ms) {
	extern void callSleepThumb(int ms);

	if (readCount >= 100) {
		sleepMsEnabled = true;
	}

	if (!sleepMsEnabled) return;

    if(ce9->patches->sleepRef) {
        volatile void (*sleepRef)(int ms) = (volatile void*)ce9->patches->sleepRef;
        (*sleepRef)(ms);
    } else if(ce9->thumbPatches->sleepRef) {
        callSleepThumb(ms);
    }
}*/

static void waitForArm7(void) {
	IPC_SendSync(0x4);
	while (sharedAddr[3] != (vu32)0);
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

static u32 dmaParams[8] = {0};

void endCardReadDma() {
    if(ce9->patches->cardEndReadDmaRef) {
        volatile void (*cardEndReadDmaRef)() = ce9->patches->cardEndReadDmaRef;
        (*cardEndReadDmaRef)();
    } else if(ce9->thumbPatches->cardEndReadDmaRef) {
        callEndReadDmaThumb();
    }    
}

void cardSetDma(u32 * params) {
	vu32* volatile cardStruct = ce9->cardStruct0;

    disableIrqMask(IRQ_CARD);
    disableIrqMask(IRQ_CARD_LINE);

	enableIPC_SYNC();

	u32 src = ((ce9->valueBits & isSdk5) ? params[3] : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? params[4] : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? params[5] : cardStruct[2]);

  	u32 len2 = len;
  	if (len2 > 512) {
  		len2 -= src % 4;
  		len2 -= len2 % 32;
  	}

	u32 newSrc = (u32)(ce9->romLocation-0x4000-ndsHeader->arm9binarySize)+src;
	if (src > ndsHeader->arm7romOffset) {
		newSrc -= ndsHeader->arm7binarySize;
	}

	// Copy via dma
	int oldIME = REG_IME;
	if (ce9->valueBits & extendedMemory) {
		REG_IME = 0;
		REG_SCFG_EXT += 0xC000;
	}
	ndmaCopyWordsAsynch(0, (u8*)newSrc, dst, len2);
	while (ndmaBusy(0));
	if (ce9->valueBits & extendedMemory) {
		REG_SCFG_EXT -= 0xC000;
		REG_IME = oldIME;
	}
	endCardReadDma();
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

u32 cardReadDma(u32 dma0, u8* dst0, u32 src0, u32 len0) {
	vu32* volatile cardStruct = ce9->cardStruct0;
    
	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
    u32 dma = ((ce9->valueBits & isSdk5) ? dma0 : cardStruct[3]); // dma channel

	dmaParams[3] = src;
	dmaParams[4] = (u32)dst;
	dmaParams[5] = len;

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

            cardSetDma(dmaParams);

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
		if (ce9->valueBits & isSdk5) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		} else {
			debug8mbMpuFix();
		}
		if (ce9->valueBits & extendedMemory) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_4MB;
		}

		//if (ce9->enableExceptionHandler && ce9==CARDENGINE_ARM9_LOCATION) {
			//exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
			//setExceptionHandler(user_exception);
		//}

		flagsSet = true;
	}
	
	enableIPC_SYNC();

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
	u32 len2 = 0;

	readCount++;

	if (src == 0) {
		// If ROM read location is 0, do not proceed.
		return 0;
	}

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}

	if ((ce9->valueBits & extendedMemory) && dst >= 0x02400000 && dst < 0x02700000) {
		dst -= 0x400000;	// Do not overwrite ROM
	}
	
	u32 romEnd1st = (ce9->consoleModel==0 ? 0x0D000000 : 0x0E000000);
	u32 newSrc = (u32)(ce9->romLocation-0x4000-ndsHeader->arm9binarySize)+src;
	if (src > ndsHeader->arm7romOffset) {
		newSrc -= ndsHeader->arm7binarySize;
	}
	if (newSrc >= romEnd1st) {
		newSrc = (u32)ROM_LOCATION_EXT_P2-(ce9->consoleModel==0 ? 0x00C00000 : 0x01C00000);
		newSrc = (u32)(newSrc-0x4000-ndsHeader->arm9binarySize)+src;
		if (src > ndsHeader->arm7romOffset) {
			newSrc -= ndsHeader->arm7binarySize;
		}
	} else if (newSrc+len > romEnd1st) {
		u32 oldLen = len;
		for (int i = 0; i < oldLen; i++) {
			len--;
			len2++;
			if (newSrc+len == romEnd1st) break;
		}
	}

	int oldIME = REG_IME;
	if (ce9->valueBits & extendedMemory) {
		// Open extra memory
		if (!(ce9->valueBits & isSdk5)) REG_IME = 0;
		REG_SCFG_EXT += 0xC000;
	}

	tonccpy(dst, (u8*)newSrc, len);
	if (len2 > 0) {
		newSrc = (u32)ROM_LOCATION_EXT_P2-(ce9->consoleModel==0 ? 0x00C00000 : 0x01C00000);
		newSrc = (u32)(newSrc-0x4000-ndsHeader->arm9binarySize)+src+len;
		if (src > ndsHeader->arm7romOffset) {
			newSrc -= ndsHeader->arm7binarySize;
		}
		tonccpy(dst+len, (u8*)newSrc, len2);
	}

	if (ce9->valueBits & extendedMemory) {
		// Close extra memory
		REG_SCFG_EXT -= 0xC000;
		if (!(ce9->valueBits & isSdk5)) REG_IME = oldIME;
	}

    isDma=false;

	return 0; 
}

/*void cardPullOut(void) {
	if (*(vu32*)(0x027FFB30) != 0) {
		/*volatile int (*terminateForPullOutRef)(u32*) = *ce9->patches->terminateForPullOutRef;
        (*terminateForPullOutRef);
		sharedAddr[3] = 0x5245424F;
		waitForArm7();
	}
}*/

u32 nandRead(void* memory,void* flash,u32 len,u32 dma) {
	if (ce9->valueBits & saveOnFlashcard) {
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
	if (ce9->valueBits & saveOnFlashcard) {
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
	
	if (IPC_GetSync() == 0x7){
		lcdSwap();
	}
	
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
