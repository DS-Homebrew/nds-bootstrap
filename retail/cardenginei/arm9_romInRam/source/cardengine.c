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
#include <nds/bios.h>
#include <nds/system.h>
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
#include "fat.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"

#define eSdk2 BIT(2)
#define dsiMode BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define slowSoftReset BIT(10)
#define softResetMb BIT(13)
#define cloneboot BIT(14)

extern cardengineArm9* volatile ce9;

extern u32 getDtcmBase(void);
extern void ndsCodeStart(u32* addr);
extern void disableIrqMask(u32 mask);
extern void callEndReadDmaThumb(void);

static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
aFile* savFile = (aFile*)SAV_FILE_LOCATION_TWLSDK;

int romMapLines = 3; // 4 for SDK5 NTR ROMs on 3DS
u32 romMap[4][3] =
{	// 0: ROM part start, 1: ROM part start in RAM, 2: ROM part end in RAM
	{0x00008000, 0x0C3E2000, 0x0C7E0000},
	{0x00406000, 0x0C800000, 0x0D000000},
	{0x00C06000, 0x03700000, 0x03778000},
	{0x01C06000, 0x03700000, 0x03778000}
};

/*u32 romMapSdk5Ntr[4][3] =
{
	{0x00008000, 0x0C3E2000, 0x0C7E0000},
	{0x00406000, 0x0C7FF000, 0x0CFFF000},
	{0x00C06000, 0x0D000000, 0x0E000000},
	{0x01C06000, 0x03700000, 0x03778000}
};*/

bool flagsSet = false;
bool flagsSetOnce = false;
bool region0FixNeeded = false;
bool igmReset = false;
bool isDma = false;
bool dmaOn = false;

void myIrqHandlerDMA(void);

extern void SetBrightness(u8 screen, s8 bright);

// Alternative to swiWaitForVBlank()
extern void waitFrames(int count);

/*void sleepMs(int ms) {
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
}*/

static inline void runArm7Cmd(u32 cmd) {
	sharedAddr[3] = cmd;
	IPC_SendSync(0x4);
	while (sharedAddr[3] == cmd);
}

extern bool IPC_SYNC_hooked;
extern void hookIPC_SYNC(void);
extern void enableIPC_SYNC(void);


//Currently used for NSMBDS romhacks
//void __attribute__((target("arm"))) debug8mbMpuFix(){
//	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
//}

void setupRomMap(void) {
	if (flagsSetOnce) {
		return;
	}
	if (ce9->valueBits & isSdk5) {
		sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;
		ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION_SDK5;
	}
	if (ce9->consoleModel > 0) {
		romMap[1][2] = 0x0E000000;
		romMap[2][0] += 0x1000000;
	}
	if (ce9->valueBits & isSdk5) {
		if (ndsHeader->unitCode > 0) {
			/*if (ce9->valueBits & dsiMode) {
			romMap[0][1] = 0x0D000000;
			romMap[0][2] = 0x0DF80000;
			} else {*/
				romMap[0][2] = 0x0CFE0000;
				romMap[1][0] = 0x00C06000;
				if (ce9->consoleModel > 0) {
					romMap[1][1] = 0x0D000000;
					romMap[2][0] += 0x1000000;
				} else {
					romMap[1][1] = 0x03700000;
					romMap[1][2] = 0x03780000;
				}
			//}
		} else {
			romMap[1][2] = 0x0CFFF000;
			romMap[2][2] = 0x0D000000;
			if (ce9->consoleModel > 0) {
				romMap[2][1] = 0x0D000000;
				romMap[2][2] = 0x0E000000;
				romMapLines = 4;
			}
		}
	} else if (ce9->valueBits & eSdk2) {
		romMap[0][1] -= 0x22000;
		romMap[0][2] -= 0x22000;
	}
	if (!(ce9->valueBits & cloneboot)) {
		for (int i = 0; i < 4; i++) {
			romMap[i][0] -= 0x8000;
			romMap[i][0] += (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
		}
	}
	flagsSetOnce = true;
}

void endCardReadDma() {
	if (dmaOn && ndmaBusy(0)) {
		IPC_SendSync(0x3);
		return;
	}

	if (ce9->patches->cardEndReadDmaRef) {
		VoidFn cardEndReadDmaRef = (VoidFn)ce9->patches->cardEndReadDmaRef;
		(*cardEndReadDmaRef)();
	} else if (ce9->thumbPatches->cardEndReadDmaRef) {
		callEndReadDmaThumb();
	}
}

void cardSetDma(u32 * params) {
	setupRomMap();

	vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? params[3] : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? (u8*)(params[4]) : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? params[5] : cardStruct[2]);

	if (!dmaOn) {
		cardRead(0, dst, src, len);
		endCardReadDma();
		return;
	}

	disableIrqMask(IRQ_CARD);
	disableIrqMask(IRQ_CARD_LINE);

	enableIPC_SYNC();

	/*if ((ce9->valueBits & extendedMemory) && (u32)dst >= 0x02400000 && (u32)dst < 0x02700000) {
		dst -= 0x400000;	// Do not overwrite ROM
	}*/

	u32 len2 = 0;
	for (int i = 0; i < romMapLines; i++) {
		if (!(src >= romMap[i][0] && (i == romMapLines-1 || src < romMap[i+1][0])))
			continue;

		u32 newSrc = (romMap[i][1]-romMap[i][0])+src;
		if (newSrc+len > romMap[i][2]) {
			do {
				len--;
				len2++;
			} while (newSrc+len != romMap[i][2]);
			tonccpy(dst, (u8*)newSrc, len);
			src += len;
			dst += len;
		} else {
			ndmaCopyWordsAsynch(0, (u8*)newSrc, dst, len2==0 ? len : len2);
			break;
		}
	}
	IPC_SendSync(0x3);
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
	vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
	u32 dma = ((ce9->valueBits & isSdk5) ? dma0 : cardStruct[3]); // dma channel

	if (dma >= 0 
         && dma <= 3 
         //&& func != NULL
         && len > 0
         && !(((int)dst) & 3)
         && isNotTcm((u32)dst, len)
         // check 512 bytes page alignement 
         && !(((int)len) & 511)
         && !(((int)src) & 511)
	) {
		isDma = true;
		if (ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef) {
			// new dma method
			if (!(ce9->valueBits & isSdk5)) {
				cacheFlush();
				cardSetDma(NULL);
			}
			return true;
		} /* else {
			isDma = false;
			dma = 4;
			clearIcache();
		} */
	} /* else {
		isDma = false;
		dma=4;
		clearIcache();
	} */

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

/*void __attribute__((target("arm"))) openDebugRam() {
	asm("LDR R0,=#0x8000035\n\tmcr p15, 0, r0, C6,C3,0");
}*/

// Revert region 0 patch
void __attribute__((target("arm"))) region0Fix() {
	asm("LDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0");
}

/*void __attribute__((target("arm"))) sdk5MpuFix() {
	asm("LDR R0,=#0x2000031\n\tmcr p15, 0, r0, C6,C1,0");
}*/

void cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		if (region0FixNeeded) {
			region0Fix();
		}
		flagsSet = true;
	}
	
	setupRomMap();

	enableIPC_SYNC();

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);

	/*if ((ce9->valueBits & extendedMemory) && (u32)dst >= 0x02400000 && (u32)dst < 0x02700000) {
		dst -= 0x400000;	// Do not overwrite ROM
	}*/

	u32 len2 = 0;
	for (int i = 0; i < romMapLines; i++) {
		if (!(src >= romMap[i][0] && (i == romMapLines-1 || src < romMap[i+1][0])))
			continue;

		u32 newSrc = (romMap[i][1]-romMap[i][0])+src;
		if (newSrc+len > romMap[i][2]) {
			do {
				len--;
				len2++;
			} while (newSrc+len != romMap[i][2]);
			tonccpy(dst, (u8*)newSrc, len);
			src += len;
			dst += len;
		} else {
			tonccpy(dst, (u8*)newSrc, len2==0 ? len : len2);
			break;
		}
	}
	isDma = false;
}

/*void cardPullOut(void) {
	if (*(vu32*)(0x027FFB30) != 0) {
		// volatile int (*terminateForPullOutRef)(u32*) = *ce9->patches->terminateForPullOutRef;
        // (*terminateForPullOutRef);
		// sharedAddr[3] = 0x5245424F;
		// runArm7Cmd();
	}
}*/

u32 cartRead(u32 dma, u32 src, u8* dst, u32 len, u32 type) {
	// Send a command to the ARM7 to read the GBA ROM
	/*u32 commandRead = 0x025FBC01;

	// Write the command
	sharedAddr[0] = (vu32)dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	runArm7Cmd(commandRead);*/
	return 0;
}

extern void reset(u32 param);

void inGameMenu(s32* exRegisters) {
	int oldIME = enterCriticalSection();

	while (sharedAddr[5] == 0x4C4D4749) { // 'IGML'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}

	*(u32*)(INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
	volatile void (*inGameMenu)(s32*, u32, s32*) = (volatile void*)INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED + 0x10;
	(*inGameMenu)(&ce9->mainScreen, ce9->consoleModel, exRegisters);

	while (sharedAddr[5] != 0x4C4D4749) { // 'IGML'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}
	while (sharedAddr[5] == 0x4C4D4749) { // 'IGML'
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}

	if (sharedAddr[3] == 0x52534554) {
		igmReset = true;
		reset(0);
	}

	leaveCriticalSection(oldIME);
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
		case 0x3:
			endCardReadDma();
			break;
		case 0x4:
			dmaOn = !dmaOn;
			break;
		/*case 0x5:
			igmReset = true;
			sharedAddr[3] = 0x54495845;
			reset(0xFFFFFFFF);
			break;*/
		case 0x6:
			if(ce9->mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(ce9->mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
			break;
		/* case 0x7: {
			ce9->mainScreen++;
			if(ce9->mainScreen > 2)
				ce9->mainScreen = 0;

			if(ce9->mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(ce9->mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
		}
			break; */
		case 0x9:
			inGameMenu((s32*)0);
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

	setupRomMap();

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
