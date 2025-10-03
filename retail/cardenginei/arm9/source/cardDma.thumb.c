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
#include <nds/bios.h>
#include <nds/system.h>
#include <nds/dma.h>
#include <nds/interrupts.h>
// #include <nds/input.h>
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

#define ROMinRAM BIT(1)
#define pingIpc BIT(3)
#define isSdk5 BIT(5)
#define overlaysCached BIT(6)
#define cacheFlushFlag BIT(7)
#define cardReadFix BIT(8)
#define cacheDisabled BIT(9)
#define useSharedWram BIT(13)
#define waitForPreloadToFinish BIT(18)

//#ifdef DLDI
#include "my_fat.h"
#include "card.h"
//#endif

#define dmaReadLen 512

extern cardengineArm9* volatile ce9;

extern vu32* volatile sharedAddr;
extern tNDSHeader* ndsHeader;

extern aFile* romFile;
extern aFile* apFixOverlaysFile;

extern u32 cacheDescriptor[];
extern int cacheCounter[];
extern int accessCounter;

extern int romMapLines;
extern u32 romMap[4][3];

extern void callEndReadDmaThumb(void);
extern void disableIrqMask(u32 mask);

bool isDma = false;
bool dmaDirectRead = false;
#ifndef DLDI
static bool dmaCheckValid = true;
#endif
extern bool romPart;

void endCardReadDma() {
	#ifndef TWLSDK
	if (dmaDirectRead && (ndmaBusy(0) || ndmaBusy(1)))
	#else
	if (dmaDirectRead && ndmaBusy(0))
	#endif
	{
		IPC_SendSync(0x3);
		return;
	}

	isDma = false;
	if (ce9->patches->cardEndReadDmaRef) {
		VoidFn cardEndReadDmaRef = (VoidFn)ce9->patches->cardEndReadDmaRef;
		(*cardEndReadDmaRef)();
	} else if (ce9->thumbPatches->cardEndReadDmaRef) {
		callEndReadDmaThumb();
	}
	sharedAddr[5] = 0; // Clear ping flag
}

extern bool IPC_SYNC_hooked;
extern void hookIPC_SYNC(void);
extern void enableIPC_SYNC(void);

#ifndef DLDI
bool dmaReadOnArm7 = false;
#endif
bool dmaReadOnArm9 = false;

#ifndef DLDI
extern int allocateCacheSlot(void);
extern int getSlotForSector(u32 sector);
//extern int getSlotForSectorManual(int i, u32 sector);
extern vu8* getCacheAddress(int slot);
extern void updateDescriptor(int slot, u32 sector);
#endif

/* static inline bool isPreloadFinished(void) {
	return (sharedAddr[5] == 0x44454C50); // 'PLED'
} */

static inline bool checkArm7(void) {
	if (ce9->valueBits & pingIpc) {
		IPC_SendSync(0x4);
	}
	return (sharedAddr[3] == (vu32)0);
}

static u32 * dmaParams = NULL;
static int currentLen = 0;
// #ifndef DLDI
// static int currentSlot = 0;
// #endif

static void cardReadDmaNormal(u8* dst, u32 src, u32 len) {
	#ifndef DLDI
	const u32 commandRead=0x025FFB0A;
	//u32 page = (src / 512) * 512;

	accessCounter++;

	#ifdef ASYNCPF
	processAsyncCommand();
	#endif

    while (len > 0) {
		// Read via the main RAM cache
		u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;
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

			// Send a command to the ARM7 to fill the RAM cache
			slot = allocateCacheSlot();

			buffer = getCacheAddress(slot);

			//fileRead((char*)buffer, *romFile, sector, ce9->cacheBlockSize);

			/*u32 len2 = (src - sector) + len;
			u16 readLen = ce9->cacheBlockSize;
			if (len2 > ce9->cacheBlockSize*3 && slot+3 < ce9->cacheSlots) {
				readLen = ce9->cacheBlockSize*4;
			} else if (len2 > ce9->cacheBlockSize*2 && slot+2 < ce9->cacheSlots) {
				readLen = ce9->cacheBlockSize*3;
			} else if (len2 > ce9->cacheBlockSize && slot+1 < ce9->cacheSlots) {
				readLen = ce9->cacheBlockSize*2;
			}*/

			// Write the command
			sharedAddr[0] = (vu32)buffer;
			sharedAddr[1] = ce9->cacheBlockSize;
			sharedAddr[2] = ((ce9->valueBits & overlaysCached) && src >= ce9->overlaysSrcAlign && src < ce9->overlaysSrcAlign+ce9->overlaysSizeAlign) ? sector+0x80000000 : sector;
			sharedAddr[3] = commandRead;

			dmaReadOnArm7 = true;

			if (ce9->valueBits & pingIpc) {
				IPC_SendSync(0x4);
			}

			updateDescriptor(slot, sector);
			/*if (readLen >= ce9->cacheBlockSize*2) {
				updateDescriptor(slot+1, sector+ce9->cacheBlockSize);
			}
			if (readLen >= ce9->cacheBlockSize*3) {
				updateDescriptor(slot+2, sector+(ce9->cacheBlockSize*2));
			}
			if (readLen >= ce9->cacheBlockSize*4) {
				updateDescriptor(slot+3, sector+(ce9->cacheBlockSize*3));
			}*/
			// currentSlot = slot;
			return;
		}
		#ifdef ASYNCPF
		if(cacheCounter[slot] == 0x0FFFFFFF) {
			// prefetch successfull
			getAsyncSector();

			triggerAsyncPrefetch(nextSector);
		} else {
			int i;
			for(i=0; i<5; i++) {
				if(asyncQueue[i]==sector) {
					// prefetch successfull
					triggerAsyncPrefetch(nextSector);
					break;
				}
			}
		}
		#endif
		updateDescriptor(slot, sector);

		u32 len2 = len;
		if ((src - sector) + len2 > ce9->cacheBlockSize) {
			len2 = sector - src + ce9->cacheBlockSize;
		}

		/*if (len2 > 512) {
			len2 -= src % 4;
			len2 -= len2 % 32;
		}*/

		#ifndef DLDI
		if (!dmaCheckValid) {
			tonccpy(dst, (u8*)buffer+(src-sector), len2);

			len -= len2;
			if (len > 0) {
				src += len2;
				dst += len2;
				accessCounter++;
				//slot = getSlotForSectorManual(slot+1, sector);
			}
		} else
		#endif
		{
			// Copy via dma
			ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
			dmaReadOnArm9 = true;
			currentLen = len2;
			// currentSlot = slot;

			IPC_SendSync(0x3);
			return;
		}
    }
    //disableIrqMask(IRQ_DMA0 << dma);
    //resetRequestIrqMask(IRQ_DMA0 << dma);
    //disableDMA(dma);
    endCardReadDma();
	#else
	/* sysSetCardOwner(false);	// Give Slot-1 access to arm7

	// Write the command
	sharedAddr[0] = (vu32)dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	dmaReadOnArm7 = true; */

	if (len > 0) {
		currentLen = (len > dmaReadLen) ? dmaReadLen : len;

		fileRead((char*)dst, ((ce9->valueBits & overlaysCached) && src >= ce9->overlaysSrc && src < ndsHeader->arm7romOffset) ? apFixOverlaysFile : romFile, src, currentLen);

		dmaReadOnArm9 = true;
		IPC_SendSync(0x3);
	} else {
		endCardReadDma();
	}
	#endif
}

void continueCardReadDmaArm9() {
    if (!dmaReadOnArm9) {
		return;
	}

	if (ndmaBusy(0)) {
		IPC_SendSync(0x3);
		return;
	}
    dmaReadOnArm9 = false;

	#ifndef TWLSDK
    vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;
    //u32	dma = cardStruct[3]; // dma channel
	#endif

	#ifndef TWLSDK
	u32 src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);
	#else
	u32 src = dmaParams[3];
	u8* dst = (u8*)dmaParams[4];
	u32 len = dmaParams[5];
	#endif

    // Update cardi common
	#ifndef TWLSDK
	if (ce9->valueBits & isSdk5) {
	#endif
		dmaParams[3] = src + currentLen;
		dmaParams[4] = (vu32)(dst + currentLen);
		dmaParams[5] = len - currentLen;
	#ifndef TWLSDK
	} else {
		cardStruct[0] = src + currentLen;
		cardStruct[1] = (vu32)(dst + currentLen);
		cardStruct[2] = len - currentLen;
	}
	#endif

	#ifndef TWLSDK
	src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
	dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
	len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);
	#else
	src = dmaParams[3];
	dst = (u8*)dmaParams[4];
	len = dmaParams[5];
	#endif

	cardReadDmaNormal(dst, src, len);
}

#ifndef DLDI
void continueCardReadDmaArm7() {
    if (!dmaReadOnArm7 || !checkArm7()) {
		return;
	}
    dmaReadOnArm7 = false;

    vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);

	/* if ((ce9->valueBits & cacheDisabled) && (u32)dst >= 0x02000000 && (u32)dst < 0x03000000) {
		endCardReadDma();
	} else { */
		u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

		u32 len2 = len;
		if ((src - sector) + len2 > ce9->cacheBlockSize) {
			len2 = sector - src + ce9->cacheBlockSize;
		}

		/*if (len2 > 512) {
			len2 -= src % 4;
			len2 -= len2 % 32;
		}*/

		vu8* buffer = getCacheAddress(getSlotForSector(sector));

		currentLen = len2;
		#ifndef DLDI
		if (!dmaCheckValid) {
			tonccpy(dst, (u8*)buffer+(src-sector), len2);
			dmaReadOnArm9 = true;

			continueCardReadDmaArm9();
		} else
		#endif
		{
			ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
			dmaReadOnArm9 = true;

			IPC_SendSync(0x3);
		}
	// }
}
#endif

void cardSetDma(u32 * params) {
	isDma = true;
	dmaDirectRead = false;

	#ifdef TWLSDK
	dmaParams = params;

	u32 src = dmaParams[3];
	u8* dst = (u8*)dmaParams[4];
	u32 len = dmaParams[5];

	// Simulate ROM mirroring
	while (src >= ce9->romPaddingSize) {
		src -= ce9->romPaddingSize;
	}
	dmaParams[3] = src;
	#else
	vu32* cardStruct = (vu32*)ce9->cardStruct0;

	if (ce9->valueBits & isSdk5) {
		dmaParams = params;
	}
	u32 src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);

	// Simulate ROM mirroring
	while (src >= ce9->romPaddingSize) {
		src -= ce9->romPaddingSize;
	}
	if (ce9->valueBits & isSdk5) {
		dmaParams[3] = src;
	} else {
		cardStruct[0] = src;
	}
	#endif

	romPart = false;
	//int romPartNo = 0;
	if (!(ce9->valueBits & ROMinRAM)) {
		#ifndef TWLSDK
		for (int i = 0; i < 4; i++) {
			if (ce9->romPartSrc[i] == 0) {
				break;
			}
			romPart = (src >= ce9->romPartSrc[i] && src < ce9->romPartSrcEnd[i]);
			if (romPart) {
				// romPartNo = i;
				break;
			}
		}
		#else
		romPart = (ce9->romPartSrc[0] > 0 && src >= ce9->romPartSrc[0] && src < ce9->romPartSrcEnd[0]);
		#endif
		/* #ifndef DLDI
		#ifndef TWLSDK
		if (romPart && (ce9->valueBits & waitForPreloadToFinish)) {
			if (isPreloadFinished()) {
				sharedAddr[5] = 0;
				ce9->valueBits &= ~waitForPreloadToFinish;
			} else {
				romPart = false;
			}
		}
		#endif
		#endif */
	}
	if ((ce9->valueBits & ROMinRAM) || romPart) {
		dmaDirectRead = true;

		disableIrqMask(IRQ_CARD);
		disableIrqMask(IRQ_CARD_LINE);

		enableIPC_SYNC();

		// Copy via dma
		// ndmaCopyWordsAsynch(0, (u8*)ce9->romLocation/*[romPartNo]*/+src, dst, len);

		#ifdef TWLSDK
		u32 newSrc = ce9->romLocation/*[romPartNo]*/+src;
		if (src > *(u32*)0x02FFE1C0) {
			newSrc -= *(u32*)0x02FFE1CC;
		}
		ndmaCopyWordsAsynch(0, (u8*)newSrc, dst, len);
		#else
		u32 newSrc = 0;
		u32 newLen = 0;
		int i = 0;
		for (i = 0; i < ce9->romMapLines; i++) {
			if (src >= ce9->romMap[i][0] && (i == ce9->romMapLines-1 || src < ce9->romMap[i+1][0])) {
				break;
			}
		}
		if (ce9->valueBits & useSharedWram) {
			WRAM_CR = 0; // Set shared WRAM to ARM9
		}
		while (len > 0) {
			newSrc = (ce9->romMap[i][1]-ce9->romMap[i][0])+src;
			newLen = len;
			while (newSrc+newLen > ce9->romMap[i][2]) {
				newLen--;
			}
			while (ndmaBusy(i % 2)) { swiDelay(100); }
			ndmaCopyWordsAsynch(i % 2, (u8*)newSrc, dst, newLen);
			src += newLen;
			dst += newLen;
			len -= newLen;
			i++;
		}
		#endif

		IPC_SendSync(0x3);
		return;
	} else if (ce9->patches->sleepRef || ce9->thumbPatches->sleepRef) {
		cardRead(NULL, dst, src, len);
		endCardReadDma();
		return;
	}

	disableIrqMask(IRQ_CARD);
	disableIrqMask(IRQ_CARD_LINE);

	enableIPC_SYNC();

	/* static bool disabled = false;
	if ((REG_KEYINPUT & KEY_SELECT) == 0) {
		disabled = true;
	}

	if (disabled) {
		return;
	} */

	/* if ((ce9->valueBits & cacheDisabled) && (u32)dst >= 0x02000000 && (u32)dst < 0x03000000) {
		// Write the command
		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		dmaReadOnArm7 = true;

		// IPC_SendSync(0x4);
	} else { */
		cardReadDmaNormal(dst, src, len);
	// }
}

#ifndef DLDI
static bool inline asyncDataAvailable(u32 src) {
	for (int i = 0; i < 2; i++) {
		if (ce9->asyncDataSrc[i] == 0) {
			return false;
		}
		if (src >= ce9->asyncDataSrc[i] && src < ce9->asyncDataSrcEnd[i]) {
			return true;
		}
	}
	return false;
}

static bool inline romPartAvailable(u32 src) {
	#ifndef TWLSDK
	for (int i = 0; i < 4; i++) {
		if (ce9->romPartSrc[i] == 0) {
			return false;
		}
		if (src >= ce9->romPartSrc[i] && src < ce9->romPartSrcEnd[i]) {
			return true;
		}
	}
	return false;
	#else
	return (ce9->romPartSrc[0] > 0 && src >= ce9->romPartSrc[0] && src < ce9->romPartSrcEnd[0]);
	#endif
}
#endif

extern bool isNotTcm(u32 address, u32 len);

u32 cardReadDma(u32 dma0, u8* dst0, u32 src0, u32 len0) {
	#ifndef TWLSDK
	vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
	u32 dma = ((ce9->valueBits & isSdk5) ? dma0 : cardStruct[3]); // dma channel
	#else
	u32 src = src0;
	u8* dst = dst0;
	u32 len = len0;
	u32 dma = dma0;
	#endif

	#ifdef DLDI
	const bool
	#endif
	dmaCheckValid = 
	  (dma <= 3
	//&& func != NULL
	&& len > 0
	#ifndef TWLSDK
	&& !(((u32)dst) & ((ce9->valueBits & isSdk5) ? 31 : 3))
	#else
	&& !(((u32)dst) & 31)
	#endif
	&& isNotTcm((u32)dst, len)
	// check 512 bytes page alignement
	&& !(len & 511)
	&& !(src & 511));

	const bool cardEndReadDmaFound = (ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef);

	#ifndef DLDI
	const bool sleepFound = (ce9->patches->sleepRef || ce9->thumbPatches->sleepRef);
	bool forceDma = false;
	if (!dmaCheckValid && cardEndReadDmaFound && !(ce9->valueBits & ROMinRAM) && len > 0
	&& ((ce9->strmLoadFlag && !sleepFound) || asyncDataAvailable(src)) && !romPartAvailable(src)) {
		while (!forceDma && len > 0) {
			u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;
			int slot = getSlotForSector(sector);
			if (slot == -1) {
				forceDma = true;
			} else {
				u32 len2 = len;
				if ((src - sector) + len2 > ce9->cacheBlockSize) {
					len2 = sector - src + ce9->cacheBlockSize;
				}

				len -= len2;
				if (len > 0) {
					src += len2;
				}
			}
		}
	}
	if (!sleepFound) {
		ce9->strmLoadFlag = 0;
	}
	#endif

    if (dmaCheckValid
	#ifndef DLDI
	|| forceDma
	#endif
	) {
		isDma = true;
        if (cardEndReadDmaFound) {
			// new dma method
			#ifndef TWLSDK
			if (!(ce9->valueBits & isSdk5)) {
				IC_InvalidateRange(dst, len);
				DC_InvalidateRange(dst, len);
				cardSetDma(NULL);
			}
			#endif
			return true;
		}
    } /*else {
        dma=4;
        clearIcache();
    }*/

    return false;
}
