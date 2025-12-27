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

#ifndef COMMON_H
#define COMMON_H

//#include <stdlib.h>
#include <nds/dma.h>
#include <nds/memory.h> // tNDSHeader

#define DMA_FILL(n)    (*(vuint32*)(0x040000E0+(n*4)))

/*#define resetCpu() \
		__asm volatile("swi 0x000000")*/

// ERROR_CODES
//enum {
#define ERR_NONE 0x00
#define ERR_STS_CLR_MEM 0x01
#define ERR_STS_LOAD_BIN 0x02
#define ERR_STS_HOOK_BIN 0x03
#define ERR_STS_START 0x04
// initCard error codes:
#define ERR_LOAD_NORM 0x11
#define ERR_LOAD_OTHR 0x12
#define ERR_SEC_NORM 0x13
#define ERR_SEC_OTHR 0x14
#define ERR_LOGO_CRC 0x15
#define ERR_HEAD_CRC 0x16
// hookARM7Binary error codes:
#define ERR_NOCHEAT 0x21
#define ERR_HOOK 0x22
//};

//ARM9_STATE
//enum {
#define ARM9_BOOT 0
#define ARM9_START 1
#define ARM9_MEMCLR 2
#define ARM9_READY 3
#define ARM9_BOOTBIN 4
#define ARM9_SCRNCLR 5
#define ARM9_DISPSCRN 6
#define ARM9_DISPERR 7
#define ARM9_INITMBK 8
#define ARM9_SETSCFG 9
#define ARM9_DISPESRB 10
#define ARM9_WRAMONARM7 11
#define ARM9_WRAMONARM9 12
//};

extern u32* arm9executeAddress;
extern bool arm9_supportsDSiMode;
extern bool dsiModeConfirmed;
extern bool arm9_boostVram;
extern u32 arm9_SCFG_EXT;
extern u16 arm9_SCFG_CLK;
extern volatile bool esrbScreenPrepared;
extern volatile bool esrbImageLoaded;
extern volatile int arm9_stateFlag;
extern volatile int arm9_screenMode;
extern volatile int screenBrightness;
extern volatile bool fadeType;

static inline void dmaFill(const void* src, void* dest, u32 size) {
	DMA_SRC(3)  = (u32)src;
	DMA_DEST(3) = (u32)dest;
	DMA_CR(3)   = DMA_COPY_WORDS | DMA_SRC_FIX | (size>>2);
	while (DMA_CR(3) & DMA_BUSY);
}

static inline void dmaFill9(u32 value, void* dest, u32 size) {
	DMA_FILL(3) = (u32)value;
	DMA_SRC(3)  = (u32)&DMA_FILL(3);
	DMA_DEST(3) = (u32)dest;
	DMA_CR(3)   = DMA_SRC_FIX | DMA_COPY_WORDS | (size>>2);
	while (DMA_CR(3) & DMA_BUSY);
}

/*static inline void copyLoop(u32* dest, const u32* src, size_t size) {
	do {
		*dest++ = *src++;
	} while (size -= 4);
}*/

/*static inline void copyLoop(u32* dest, const u32* src, u32 size) {
	size = (size +3) & ~3; // Bigger nearest multiple of 4
	do {
		*dest++ = *src++;
	} while (size -= 4);
}*/

#endif // COMMON_H
