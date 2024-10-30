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

#define expansionPakFound BIT(0)
#define ROMinRAM BIT(2)
#define isSdk5 BIT(5)
#define cacheFlushFlag BIT(7)
#define cardReadFix BIT(8)

//#ifdef DLDI
#include "my_fat.h"
#include "card.h"
//#endif

#define dmaReadLen 512

#ifndef GSDD

extern cardengineArm9* volatile ce9;

extern bool cardReadInProgress;

extern void setDeviceOwner(void);

extern void cardReadNormal(u8* dst, u32 src, u32 len);
extern void cardReadRAM(u8* dst, u32 src, u32 len);

extern void callEndReadDmaThumb(void);
extern void disableIrqMask(u32 mask);

void endCardReadDma() {
	if (ce9->patches->cardEndReadDmaRef) {
		VoidFn cardEndReadDmaRef = (VoidFn)ce9->patches->cardEndReadDmaRef;
		(*cardEndReadDmaRef)();
	} else if (ce9->thumbPatches->cardEndReadDmaRef) {
		callEndReadDmaThumb();
	}    
}

extern void enableIPC_SYNC(void);

bool dmaReadOnArm9 = false;

static u32 * dmaParams = NULL;
static int currentLen = 0;
//static int currentSlot = 0;

void continueCardReadDmaArm9() {
    if (dmaReadOnArm9) {
        dmaReadOnArm9 = false;

        vu32* volatile cardStruct = (vu32*)ce9->cardStruct0;
        //u32	dma = cardStruct[3]; // dma channel

		u32 src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
		u8* dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
		u32 len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);

        // Update cardi common
		if (ce9->valueBits & isSdk5) {
			dmaParams[3] = src + currentLen;
			dmaParams[4] = (vu32)(dst + currentLen);
			dmaParams[5] = len - currentLen;
		} else {
			cardStruct[0] = src + currentLen;
			cardStruct[1] = (vu32)(dst + currentLen);
			cardStruct[2] = len - currentLen;
		}

		src = ((ce9->valueBits & isSdk5) ? dmaParams[3] : cardStruct[0]);
		dst = ((ce9->valueBits & isSdk5) ? (u8*)(dmaParams[4]) : (u8*)(cardStruct[1]));
		len = ((ce9->valueBits & isSdk5) ? dmaParams[5] : cardStruct[2]);

		if (len > 0) {
			const u16 exmemcnt = REG_EXMEMCNT;
			setDeviceOwner();

			currentLen = (len > dmaReadLen) ? dmaReadLen : len;

			if ((ce9->valueBits & ROMinRAM) || (ce9->romPartSize > 0 && src >= ce9->romPartSrc && src < ce9->romPartSrc+ce9->romPartSize)) {
				cardReadRAM(dst, src, currentLen);
			} else {
				cardReadNormal(dst, src, currentLen);
			}

			dmaReadOnArm9 = true;
			REG_EXMEMCNT = exmemcnt;
			IPC_SendSync(0x3);
		} else {
			cardReadInProgress = false;
			endCardReadDma();
		}
    }
}

void cardSetDma(u32 * params) {
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

	disableIrqMask(IRQ_CARD);
	disableIrqMask(IRQ_CARD_LINE);

	enableIPC_SYNC();

	const u16 exmemcnt = REG_EXMEMCNT;
	cardReadInProgress = true;

	if (__myio_dldi.features & FEATURE_SLOT_GBA) {
		REG_IE &= ~IRQ_CART;
	}

	setDeviceOwner();

	currentLen = (len > dmaReadLen) ? dmaReadLen : len;

	if ((ce9->valueBits & ROMinRAM) || (ce9->romPartSize > 0 && src >= ce9->romPartSrc && src < ce9->romPartSrc+ce9->romPartSize)) {
		cardReadRAM(dst, src, currentLen);
	} else {
		cardReadNormal(dst, src, currentLen);
	}

	dmaReadOnArm9 = true;
	REG_EXMEMCNT = exmemcnt;
	IPC_SendSync(0x3);
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
        && !(((u32)dst) & ((ce9->valueBits & isSdk5) ? 31 : 3))
        && isNotTcm((u32)dst, len)
        // check 512 bytes page alignement 
        && !(len & 511)
        && !(src & 511)
	) {
        if (ce9->patches->cardEndReadDmaRef || ce9->thumbPatches->cardEndReadDmaRef) {
			// new dma method
			if (!(ce9->valueBits & isSdk5)) {
				cacheFlush();
				cardSetDma(NULL);
			}
			return true;
		}
    } /*else {
        dma=4;
        clearIcache();
    }*/

    return false;
}
#endif
