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
#include <nds/bios.h>
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

#define ROMinRAM BIT(1)
#define isSdk5 BIT(5)
#define overlaysCached BIT(6)
#define cacheFlushFlag BIT(7)
#define cardReadFix BIT(8)
#define cacheDisabled BIT(9)

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
#ifndef TWLSDK
static bool dataSplit = false;
#endif

void endCardReadDma() {
	#ifndef TWLSDK
	if (dmaDirectRead && (ndmaBusy(0) || (dataSplit && ndmaBusy(1))))
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

extern u32 newOverlayOffset;
extern u32 newOverlaysSize;
#endif

static inline bool checkArm7(void) {
	// IPC_SendSync(0x4);
	return (sharedAddr[3] == (vu32)0);
}

static u32 * dmaParams = NULL;
static int currentLen = 0;
//static int currentSlot = 0;

void continueCardReadDmaArm9() {
    if(dmaReadOnArm9) {
		if (ndmaBusy(0)) {
			IPC_SendSync(0x3);
			return;
		}
        dmaReadOnArm9 = false;

		#ifndef TWLSDK
        vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;
        //u32	dma = cardStruct[3]; // dma channel
		#endif

		#ifndef DLDI
		u32 commandRead=0x025FFB0A;
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

		#ifndef DLDI
		u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

		#ifdef ASYNCPF
		processAsyncCommand();
		#endif

        if (len > 0) {
			accessCounter++;  

            // Read via the main RAM cache
        	//int slot = getSlotForSectorManual(currentSlot+1, sector);
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
				sharedAddr[2] = ((ce9->valueBits & overlaysCached) && src >= newOverlayOffset && src < newOverlayOffset+newOverlaysSize) ? sector+0x80000000 : sector;
				sharedAddr[3] = commandRead;

				dmaReadOnArm7 = true;

				// IPC_SendSync(0x4);

				updateDescriptor(slot, sector);
				/*if (readLen >= ce9->cacheBlockSize*2) {
					updateDescriptor(slot+1, sector+ce9->cacheBlockSize);
				}
				if (readLen >= ce9->cacheBlockSize*3) {
					updateDescriptor(slot+2, sector+(ce9->cacheBlockSize*2));
				}
				if (readLen >= ce9->cacheBlockSize*4) {
					updateDescriptor(slot+3, sector+(ce9->cacheBlockSize*3));
				}
				currentSlot = slot;*/
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

			// Copy via dma
			ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
			dmaReadOnArm9 = true;
			currentLen = len2;
			//currentSlot = slot;

			IPC_SendSync(0x3);
        } else {
          //disableIrqMask(IRQ_DMA0 << dma);
          //resetRequestIrqMask(IRQ_DMA0 << dma);
          //disableDMA(dma);
          endCardReadDma();
		}
		#else
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
}

#ifndef DLDI
void continueCardReadDmaArm7() {
    if(dmaReadOnArm7) {
        if(!checkArm7()) return;
        dmaReadOnArm7 = false;

        vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;

		u32 src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
		u8* dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
		u32 len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);

		if ((u32)dst >= 0x02000000 && (u32)dst < 0x03000000 && ((u32)dst % 4) == 0 && len > ce9->cacheBlockSize) {
			endCardReadDma();
		} else {
			u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;

			u32 len2 = len;
			if ((src - sector) + len2 > ce9->cacheBlockSize) {
				len2 = sector - src + ce9->cacheBlockSize;
			}

			/*if (len2 > 512) {
				len2 -= src % 4;
				len2 -= len2 % 32;
			}*/

			//vu8* buffer = getCacheAddress(currentSlot);
			vu8* buffer = getCacheAddress(getSlotForSector(sector));

			// TODO Copy via dma
			ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
			dmaReadOnArm9 = true;
			currentLen = len2;

			IPC_SendSync(0x3);
		}
	}
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

	#ifndef TWLSDK
	dataSplit = false;
	#endif
	bool romPart = false;
	//int romPartNo = 0;
	if (!(ce9->valueBits & ROMinRAM)) {
		/*for (int i = 0; i < 2; i++) {
			if (ce9->romPartSize[i] == 0) {
				break;
			}
			romPart = (src >= ce9->romPartSrc[i] && src < ce9->romPartSrc[i]+ce9->romPartSize[i]);
			if (romPart) {
				romPartNo = i;
				break;
			}
		}*/
		romPart = (ce9->romPartSize > 0 && src >= ce9->romPartSrc && src < ce9->romPartSrc+ce9->romPartSize);
	}
	if ((ce9->valueBits & ROMinRAM) || romPart) {
		dmaDirectRead = true;

		disableIrqMask(IRQ_CARD);
		disableIrqMask(IRQ_CARD_LINE);

		enableIPC_SYNC();

		// Copy via dma
		// ndmaCopyWordsAsynch(0, (u8*)ce9->romLocation/*[romPartNo]*/+src, dst, len);

		u32 len2 = 0;
		for (int i = 0; i < ce9->romMapLines; i++) {
			if (!(src >= ce9->romMap[i][0] && (i == ce9->romMapLines-1 || src < ce9->romMap[i+1][0])))
				continue;

			u32 newSrc = (ce9->romMap[i][1]-ce9->romMap[i][0])+src;
			if (newSrc+len > ce9->romMap[i][2]) {
				do {
					len--;
					len2++;
				} while (newSrc+len != ce9->romMap[i][2]);
				ndmaCopyWordsAsynch(1, (u8*)newSrc, dst, len);
				src += len;
				dst += len;
				#ifndef TWLSDK
				dataSplit = true;
				#endif
			} else {
				ndmaCopyWordsAsynch(0, (u8*)newSrc, dst, len2==0 ? len : len2);
				break;
			}
		}

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

	#ifndef DLDI
	if (newOverlayOffset == 0) {
		newOverlayOffset = (ce9->overlaysSrc/ce9->cacheBlockSize)*ce9->cacheBlockSize;
		for (u32 i = newOverlayOffset; i < ndsHeader->arm7romOffset; i+= ce9->cacheBlockSize) {
			newOverlaysSize += ce9->cacheBlockSize;
		}
	}

	const u32 commandRead=0x025FFB0A;
	u32 sector = (src/ce9->cacheBlockSize)*ce9->cacheBlockSize;
	//u32 page = (src / 512) * 512;

	accessCounter++;  

	#ifdef ASYNCPF
	processAsyncCommand();
	#endif

	if ((u32)dst >= 0x02000000 && (u32)dst < 0x03000000 && ((u32)dst % 4) == 0 && len > ce9->cacheBlockSize) {
		// Write the command
		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		dmaReadOnArm7 = true;

		// IPC_SendSync(0x4);
	} else {
		// Read via the main RAM cache
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
			sharedAddr[2] = ((ce9->valueBits & overlaysCached) && src >= newOverlayOffset && src < newOverlayOffset+newOverlaysSize) ? sector+0x80000000 : sector;
			sharedAddr[3] = commandRead;

			dmaReadOnArm7 = true;

			// IPC_SendSync(0x4);

			updateDescriptor(slot, sector);
			/*if (readLen >= ce9->cacheBlockSize*2) {
				updateDescriptor(slot+1, sector+ce9->cacheBlockSize);
			}
			if (readLen >= ce9->cacheBlockSize*3) {
				updateDescriptor(slot+2, sector+(ce9->cacheBlockSize*2));
			}
			if (readLen >= ce9->cacheBlockSize*4) {
				updateDescriptor(slot+3, sector+(ce9->cacheBlockSize*3));
			}
			currentSlot = slot;*/
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

		// Copy via dma
		ndmaCopyWordsAsynch(0, (u8*)buffer+(src-sector), dst, len2);
		dmaReadOnArm9 = true;
		currentLen = len2;
		//currentSlot = slot;

		//fixme: why is this needed to make the function work
		//there seems to be some timing issue
		swiDelay(1);

		IPC_SendSync(0x3);
	}
	#else
	/* sysSetCardOwner(false);	// Give Slot-1 access to arm7

	// Write the command
	sharedAddr[0] = (vu32)dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	dmaReadOnArm7 = true; */

	currentLen = (len > dmaReadLen) ? dmaReadLen : len;

	fileRead((char*)dst, ((ce9->valueBits & overlaysCached) && src >= ce9->overlaysSrc && src < ndsHeader->arm7romOffset) ? apFixOverlaysFile : romFile, src, currentLen);

	dmaReadOnArm9 = true;
	IPC_SendSync(0x3);
	#endif
}

extern bool isNotTcm(u32 address, u32 len);

u32 cardReadDma(u32 dma0, u8* dst0, u32 src0, u32 len0) {
	vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
	u32 dma = ((ce9->valueBits & isSdk5) ? dma0 : cardStruct[3]); // dma channel

    if(dma >= 0 
        && dma <= 3 
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
        && !(src & 511)
	) {
		isDma = true;
        if (ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef) {
			// new dma method
			#ifndef TWLSDK
			if (!(ce9->valueBits & isSdk5)) {
				cacheFlush();
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
