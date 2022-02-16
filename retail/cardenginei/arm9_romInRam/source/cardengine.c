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
#define a7HaltPatched BIT(9)
#define slowSoftReset BIT(10)

//extern void user_exception(void);

extern cardengineArm9* volatile ce9;

static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static bool flagsSet = false;
static bool flagsSetOnce = false;
static bool region0FixNeeded = false;
static bool igmReset = false;
static bool isDma = false;

s8 mainScreen = 0;

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

void sleepMs(int ms) {
	if (REG_IME == 0 || REG_IF == 0) {
		return;
	}

	if(ce9->patches->sleepRef) {
		volatile void (*sleepRef)(int ms) = (volatile void*)ce9->patches->sleepRef;
		(*sleepRef)(ms);
	} else if(ce9->thumbPatches->sleepRef) {
		extern void callSleepThumb(int ms);
		callSleepThumb(ms);
	}
}

static void waitForArm7(void) {
	IPC_SendSync(0x4);
	while (sharedAddr[3] != (vu32)0);
}

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
		if (!(ce9->valueBits & isSdk5)) {
			u32* vblankHandler = ce9->irqTable;
			ce9->intr_vblank_orig_return = *vblankHandler;
			*vblankHandler = ce9->patches->vblankHandlerRef;
		}
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

//Currently used for NSMBDS romhacks
//void __attribute__((target("arm"))) debug8mbMpuFix(){
//	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
//}

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

	u32 newSrc = (u32)(ce9->romLocation-0x8000)+src;
	if (ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode) && src > *(u32*)0x02FFE1C0) {
		newSrc -= *(u32*)0x02FFE1CC;
	}

	// Copy via dma
	if (!(ce9->valueBits & extendedMemory) /*ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode)*/) {
		ndmaCopyWords(0, (u8*)newSrc, dst, len2);
		endCardReadDma();
	} else /*if (ce9->valueBits & extendedMemory)*/ {
		int oldIME = enterCriticalSection();
		REG_SCFG_EXT += 0xC000;
		ndmaCopyWords(0, (u8*)newSrc, dst, len2);
		REG_SCFG_EXT -= 0xC000;
		leaveCriticalSection(oldIME);
		endCardReadDma();
	} /*else {
		u32 commandRead=0x026FFB0A;

		// Write the command
		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len2;
		sharedAddr[2] = (vu32)newSrc;
		sharedAddr[3] = commandRead;

		if (dst >= 0x03000000) {
			ndmaCopyWordsAsynch(0, (u8*)newSrc, dst, len2);
		}

		IPC_SendSync(0x4);
	}*/
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
		isDma = true;
        cacheFlush();
        if(ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef)
		{
			// new dma method
			if (!(ce9->valueBits & isSdk5)){
				cardSetDma(NULL);
			}

            return true;
		} /*else {
			isDma = false;
			dma=4;
            clearIcache();
		}*/
    } /*else {
        isDma = false;
        dma=4;
        clearIcache();
    }*/

    return false;
}

static int counter=0;
int cardReadPDash(u32* cacheStruct, u32 src, u8* dst, u32 len) {
	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

    cardStruct[0] = src;
    cardStruct[1] = (vu32)dst;
    cardStruct[2] = len;

    cardRead(cacheStruct, dst, src, len);

    counter++;
	return counter;
}

void __attribute__((target("arm"))) openDebugRam() {
	asm("LDR R0,=#0x8000035\n\tmcr p15, 0, r0, C6,C3,0");
}

// Revert region 0 patch
void __attribute__((target("arm"))) region0Fix() {
	asm("LDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0");
}

void __attribute__((target("arm"))) sdk5MpuFix() {
	asm("LDR R0,=#0x2000031\n\tmcr p15, 0, r0, C6,C1,0");
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		if (ce9->valueBits & isSdk5) {
			if (ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode)) {
				openDebugRam();
			} else {
				sdk5MpuFix();
			}
		}
		if (region0FixNeeded) {
			region0Fix();
		}
		if (ce9->valueBits & extendedMemory) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_4MB;
		}

		flagsSet = true;
	}
	
	enableIPC_SYNC();

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
	u32 lenExt = 0;

	if ((ce9->valueBits & extendedMemory) && (u32)dst >= 0x02400000 && (u32)dst < 0x02700000) {
		dst -= 0x400000;	// Do not overwrite ROM
	}

	u32 romEnd1st = (ce9->consoleModel==0 ? 0x0D000000 : 0x0E000000);
	u32 newSrc = (u32)(ce9->romLocation-0x8000)+src;
	if (ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode) && src > *(u32*)0x02FFE1C0) {
		newSrc -= *(u32*)0x02FFE1CC;
	}
	if (newSrc >= romEnd1st) {
		newSrc = (u32)ROM_LOCATION_EXT_P2-(ce9->consoleModel==0 ? 0x00C00000 : 0x01C00000);
		newSrc = (u32)(newSrc-0x8000)+src;
	} else if (newSrc+len > romEnd1st) {
		u32 oldLen = len;
		for (int i = 0; i < oldLen; i++) {
			len--;
			lenExt++;
			if (newSrc+len == romEnd1st) break;
		}
	}

	if (isDma && !(ce9->valueBits & extendedMemory)) {
		ndmaCopyWordsAsynch(0, (u8*)newSrc, dst, len);
		while (ndmaBusy(0)) {
			sleepMs(1);
		}
	} else {
		int oldIME = 0;
		if (ce9->valueBits & extendedMemory) {
			// Open extra memory
			oldIME = enterCriticalSection();
			REG_SCFG_EXT += 0xC000;
		}

		tonccpy(dst, (u8*)newSrc, len);
		if (lenExt > 0) {
			newSrc = (u32)ROM_LOCATION_EXT_P2-(ce9->consoleModel==0 ? 0x00C00000 : 0x01C00000);
			newSrc = (u32)(newSrc-0x8000)+src+len;
			tonccpy(dst+len, (u8*)newSrc, lenExt);
		}

		if (ce9->valueBits & extendedMemory) {
			// Close extra memory
			REG_SCFG_EXT -= 0xC000;
			leaveCriticalSection(oldIME);
		}
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

bool nandRead(void* memory,void* flash,u32 len,u32 dma) {
	sysSetCardOwner(false);	// Give Slot-1 access to arm7

    // Send a command to the ARM7 to read the nand save
	u32 commandNandRead = 0x025FFC01;

	// Write the command
	sharedAddr[0] = memory;
	sharedAddr[1] = len;
	sharedAddr[2] = flash;
	sharedAddr[3] = commandNandRead;

	waitForArm7();

	sysSetCardOwner(true);	// Give Slot-1 access back to arm9
	return true; 
}

bool nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	sysSetCardOwner(false);	// Give Slot-1 access to arm7

	// Send a command to the ARM7 to write the nand save
	u32 commandNandWrite = 0x025FFC02;

	// Write the command
	sharedAddr[0] = memory;
	sharedAddr[1] = len;
	sharedAddr[2] = flash;
	sharedAddr[3] = commandNandWrite;

	waitForArm7();

	sysSetCardOwner(true);	// Give Slot-1 access back to arm9
	return true; 
}

u32 cartRead(u32 dma, u32 src, u8* dst, u32 len, u32 type) {
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

void __attribute__((target("arm"))) resetMpu(void) {
	asm("LDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0");
}

void reset(u32 param, u32 tid2) {
	u32 resetParam = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	if (ndsHeader->unitCode == 0 || (*(u32*)0x02FFE234 != 0x00030004 && *(u32*)0x02FFE234 != 0x00030005)) {
		*(u32*)resetParam = param;
	}
	if (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
		if (param != *(u32*)0x02FFE230 && tid2 != *(u32*)0x02FFE234) {
			if (ce9->consoleModel < 2) {
				// Make screens white
				SetBrightness(0, 31);
				SetBrightness(1, 31);
				waitFrames(5);	// Wait for DSi screens to stabilize
			}
			enterCriticalSection();
			cacheFlush();
			sharedAddr[3] = 0x54495845;
			while (1);
		} else {
			sysSetCardOwner(false);	// Give Slot-1 access to arm7
			sharedAddr[3] = 0x52534554;
		}
	} else if ((ce9->valueBits & slowSoftReset) || *(u32*)(resetParam+0xC) > 0) {
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
		if (ce9->valueBits & dsiMode) {
			sysSetCardOwner(false);	// Give Slot-1 access to arm7
		}
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

		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = *(u32*)0x02FFE230;
			*(u32*)0x02FFC234 = *(u32*)0x02FFE234;
		}
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);

		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = 0;
			*(u32*)0x02FFC234 = 0;
		}
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

	if (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
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

	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	if (ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode)) {
		sysSetCardOwner(true);	// Give Slot-1 access back to arm9
	}

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
		/*case 0x3:
			endCardReadDma();
			break;*/
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
			if (!(ce9->valueBits & extendedMemory)) {
				if (ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode)) {
					*(u32*)(INGAME_MENU_LOCATION_TWLSDK + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
					volatile void (*inGameMenu)(s8*) = (volatile void*)INGAME_MENU_LOCATION_TWLSDK + IGM_TEXT_SIZE_ALIGNED + 0x10;
					(*inGameMenu)(&mainScreen);
				} else {
					*(u32*)(INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
					volatile void (*inGameMenu)(s8*) = (volatile void*)INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED + 0x10;
					(*inGameMenu)(&mainScreen);
				}
				if (sharedAddr[3] == 0x52534554) {
					igmReset = true;
					if (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
						reset(*(u32*)0x02FFE230, *(u32*)0x02FFE234);
					} else {
						reset(0, 0);
					}
				}
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

	if (!flagsSetOnce) {
		if (ce9->valueBits & isSdk5) {
			sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
			unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION_SDK5;
		}
		flagsSetOnce = true;
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
