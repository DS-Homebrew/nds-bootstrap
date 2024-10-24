/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2010
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h> // free
#include <string.h> // memcpy
#include <nds/interrupts.h>
#include <nds/arm9/video.h>
#include <nds/arm9/dldi.h>
#include <nds/memory.h>
#include <nds/system.h>
#include <nds/bios.h>

//#include <nds/arm9/dldi.h>
#include <nds/debug.h>

#include "myDSiMode.h"
#include "lzss.h"
#include "tonccpy.h"
#include "hex.h"
#include "configuration.h"
#include "nds_loader_arm9.h"
#include "locations.h"
#include "load_crt0.h"
#include "cardengine_header_arm7.h"

//#define memcpy __builtin_memcpy

//#define LCDC_BANK_C (u16*)0x06840000

extern u8* lz77ImageBuffer;
void* loaderBin = (void*)0x02034000;

loadCrt0* lc0 = (loadCrt0*)LOAD_CRT0_LOCATION;

typedef signed int addr_t;  // s32
typedef unsigned char data_t; // u8

#define FIX_ALL  0x01
#define FIX_GLUE 0x02
#define FIX_GOT  0x04
#define FIX_BSS  0x08

enum DldiOffsets {
	DO_magicString      = 0x00, // "\xED\xA5\x8D\xBF Chishm"
	DO_magicToken       = 0x00, // 0xBF8DA5ED
	DO_magicShortString = 0x04, // " Chishm"
	DO_version          = 0x0C,
	DO_driverSize       = 0x0D,
	DO_fixSections      = 0x0E,
	DO_allocatedSpace   = 0x0F,

	DO_friendlyName = 0x10,

	DO_text_start = 0x40, // Data start
	DO_data_end   = 0x44, // Data end
	DO_glue_start = 0x48, // Interworking glue start	-- Needs address fixing
	DO_glue_end   = 0x4C, // Interworking glue end
	DO_got_start  = 0x50, // GOT start					-- Needs address fixing
	DO_got_end    = 0x54, // GOT end
	DO_bss_start  = 0x58, // bss start					-- Needs setting to zero
	DO_bss_end    = 0x5C, // bss end

	// IO_INTERFACE data
	DO_ioType       = 0x60,
	DO_features     = 0x64,
	DO_startup      = 0x68,	
	DO_isInserted   = 0x6C,	
	DO_readSectors  = 0x70,	
	DO_writeSectors = 0x74,
	DO_clearStatus  = 0x78,
	DO_shutdown     = 0x7C,
	DO_code         = 0x80
};


static inline addr_t readAddr(data_t *mem, addr_t offset) {
	return ((addr_t*)mem)[offset/sizeof(addr_t)];
}

static inline void writeAddr(data_t *mem, addr_t offset, addr_t value) {
	((addr_t*)mem)[offset/sizeof(addr_t)] = value;
}

static u32 quickFind (const unsigned char* data, const unsigned char* search, u32 dataSize, u32 searchSize) {
	const int* dataChunk = (const int*) data;
	int searchChunk = ((const int*)search)[0];
	u32 i;
	u32 dataLen = (u32)(dataSize / sizeof(int));

	for ( i = 0; i < dataLen; i++) {
		if (dataChunk[i] == searchChunk) {
			if ((i*sizeof(int) + searchSize) > dataSize) {
				return -1;
			}
			if (memcmp (&data[i*sizeof(int)], search, searchSize) == 0) {
				return i*sizeof(int);
			}
		}
	}

	return -1;
}

// Normal DLDI uses "\xED\xA5\x8D\xBF Chishm"
// Bootloader string is different to avoid being patched
static const data_t dldiMagicLoaderString[] = "\xEE\xA5\x8D\xBF Chishm";	// Different to a normal DLDI file

#define DEVICE_TYPE_DLDI 0x49444C44

static bool dldiPatchLoader (data_t *binData, u32 binSize, bool clearBSS)
{
	addr_t memOffset;			// Offset of DLDI after the file is loaded into memory
	addr_t patchOffset;			// Position of patch destination in the file
	addr_t relocationOffset;	// Value added to all offsets within the patch to fix it properly
	addr_t ddmemOffset;			// Original offset used in the DLDI file
	addr_t ddmemStart;			// Start of range that offsets can be in the DLDI file
	addr_t ddmemEnd;			// End of range that offsets can be in the DLDI file
	addr_t ddmemSize;			// Size of range that offsets can be in the DLDI file

	addr_t addrIter;

	data_t *pDH;
	data_t *pAH;

	size_t dldiFileSize = 0;
	
	// Find the DLDI reserved space in the file
	patchOffset = quickFind (binData, dldiMagicLoaderString, binSize, sizeof(dldiMagicLoaderString));

	if (patchOffset < 0) {
		// does not have a DLDI section
		return false;
	}

	pDH = (data_t*)(io_dldi_data);
	
	pAH = &(binData[patchOffset]);

	if (*((u32*)(pDH + DO_ioType)) == DEVICE_TYPE_DLDI) {
		// No DLDI patch
		return false;
	}

	if (pDH[DO_driverSize] > pAH[DO_allocatedSpace]) {
		// Not enough space for patch
		return false;
	}
	
	dldiFileSize = 1 << pDH[DO_driverSize];

	memOffset = readAddr (pAH, DO_text_start);
	if (memOffset == 0) {
			memOffset = readAddr (pAH, DO_startup) - DO_code;
	}
	ddmemOffset = readAddr (pDH, DO_text_start);
	relocationOffset = memOffset - ddmemOffset;

	ddmemStart = readAddr (pDH, DO_text_start);
	ddmemSize = (1 << pDH[DO_driverSize]);
	ddmemEnd = ddmemStart + ddmemSize;

	// Remember how much space is actually reserved
	pDH[DO_allocatedSpace] = pAH[DO_allocatedSpace];
	// Copy the DLDI patch into the application
	tonccpy (pAH, pDH, dldiFileSize);

	// Fix the section pointers in the header
	writeAddr (pAH, DO_text_start, readAddr (pAH, DO_text_start) + relocationOffset);
	writeAddr (pAH, DO_data_end, readAddr (pAH, DO_data_end) + relocationOffset);
	writeAddr (pAH, DO_glue_start, readAddr (pAH, DO_glue_start) + relocationOffset);
	writeAddr (pAH, DO_glue_end, readAddr (pAH, DO_glue_end) + relocationOffset);
	writeAddr (pAH, DO_got_start, readAddr (pAH, DO_got_start) + relocationOffset);
	writeAddr (pAH, DO_got_end, readAddr (pAH, DO_got_end) + relocationOffset);
	writeAddr (pAH, DO_bss_start, readAddr (pAH, DO_bss_start) + relocationOffset);
	writeAddr (pAH, DO_bss_end, readAddr (pAH, DO_bss_end) + relocationOffset);
	// Fix the function pointers in the header
	writeAddr (pAH, DO_startup, readAddr (pAH, DO_startup) + relocationOffset);
	writeAddr (pAH, DO_isInserted, readAddr (pAH, DO_isInserted) + relocationOffset);
	writeAddr (pAH, DO_readSectors, readAddr (pAH, DO_readSectors) + relocationOffset);
	writeAddr (pAH, DO_writeSectors, readAddr (pAH, DO_writeSectors) + relocationOffset);
	writeAddr (pAH, DO_clearStatus, readAddr (pAH, DO_clearStatus) + relocationOffset);
	writeAddr (pAH, DO_shutdown, readAddr (pAH, DO_shutdown) + relocationOffset);

	if (pDH[DO_fixSections] & FIX_ALL) { 
		// Search through and fix pointers within the data section of the file
		for (addrIter = (readAddr(pDH, DO_text_start) - ddmemStart); addrIter < (readAddr(pDH, DO_data_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}

	if (pDH[DO_fixSections] & FIX_GLUE) { 
		// Search through and fix pointers within the glue section of the file
		for (addrIter = (readAddr(pDH, DO_glue_start) - ddmemStart); addrIter < (readAddr(pDH, DO_glue_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}

	if (pDH[DO_fixSections] & FIX_GOT) { 
		// Search through and fix pointers within the Global Offset Table section of the file
		for (addrIter = (readAddr(pDH, DO_got_start) - ddmemStart); addrIter < (readAddr(pDH, DO_got_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}

	if (clearBSS && (pDH[DO_fixSections] & FIX_BSS)) { 
		// Initialise the BSS to 0, only if the disc is being re-inited
		toncset (&pAH[readAddr(pDH, DO_bss_start) - ddmemStart] , 0, readAddr(pDH, DO_bss_end) - readAddr(pDH, DO_bss_start));
	}

	return true;
}

int runNds(u32 cluster, u32 saveCluster, u32 donorTwlCluster, /* u32 gbaCluster, u32 gbaSavCluster, */ u32 wideCheatCluster, u32 apPatchCluster, u32 cheatCluster, u32 patchOffsetCacheCluster, u32 ramDumpCluster, u32 srParamsCluster, u32 screenshotCluster, u32 apFixOverlaysCluster, u32 musicCluster, u32 pageFileCluster, u32 manualCluster, u32 sharedFontCluster, configuration* conf) {
	nocashMessage("runNds");

	// Load bootloader binary
	char bootloaderPath[32];
	sprintf(bootloaderPath, "nitro:/load%s%i.lz77", (dsiFeatures() && !conf->b4dsMode) ? "i" : "", conf->loaderType);
	FILE* bootloaderBin = fopen(bootloaderPath, "rb");
	if (bootloaderBin) {
		fread(lz77ImageBuffer, 1, 0x30000, bootloaderBin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)loaderBin);
		fclose(bootloaderBin);
	} else {
		return 2;
	}

	loadCrt0* loader = (loadCrt0*)loaderBin;

	loader->storedFileCluster = cluster;
	loader->initDisc          = conf->initDisc;
	loader->bootstrapOnFlashcard = conf->bootstrapOnFlashcard;
	loader->gameOnFlashcard   = conf->gameOnFlashcard;
	loader->saveOnFlashcard   = conf->saveOnFlashcard;
	loader->a9ScfgRom         = REG_SCFG_ROM;
	loader->dsiSD             = conf->sdFound;
	loader->valueBits         = conf->valueBits;

	loader->saveFileCluster             = saveCluster;
	loader->donorFileCluster            = donorTwlCluster;
	loader->donorFileSize               = conf->donorFileSize;
	loader->donorFileOffset             = conf->donorFileOffset;
	// loader->gbaFileCluster              = gbaCluster;
	// loader->gbaSaveFileCluster          = gbaSavCluster;
	loader->romSize                     = conf->romSize;
	loader->saveSize                    = conf->saveSize;
	// loader->gbaRomSize                  = conf->gbaRomSize;
	// loader->gbaSaveSize                 = conf->gbaSaveSize;
	loader->dataToPreloadAddr           = conf->dataToPreloadAddr;
	loader->dataToPreloadSize           = conf->dataToPreloadSize;
	loader->dataToPreloadFrame          = conf->dataToPreloadFrame;
	loader->wideCheatFileCluster        = wideCheatCluster;
	loader->wideCheatSize               = conf->wideCheatSize;
	loader->apPatchFileCluster          = apPatchCluster;
	loader->apPatchSize                 = conf->apPatchSize;
	loader->cheatFileCluster            = cheatCluster;
	loader->cheatSize                   = conf->cheatSize;
	loader->patchOffsetCacheFileCluster = patchOffsetCacheCluster;
	loader->ramDumpCluster              = ramDumpCluster;
	loader->srParamsFileCluster         = srParamsCluster;
	loader->screenshotCluster           = screenshotCluster;
	loader->apFixOverlaysCluster        = apFixOverlaysCluster;
	loader->musicCluster                = musicCluster;
	loader->musicsSize                  = conf->musicsSize;
	loader->pageFileCluster             = pageFileCluster;
	loader->manualCluster               = manualCluster;
	loader->sharedFontCluster           = sharedFontCluster;
	loader->language                    = conf->language;
	loader->region                      = conf->region;
	loader->dsiMode                     = conf->dsiMode; // SDK 5
	loader->valueBits2                  = conf->valueBits2;
	loader->donorSdkVer                 = conf->donorSdkVer;
	loader->patchMpuRegion              = conf->patchMpuRegion;
	loader->patchMpuSize                = conf->patchMpuSize;
	loader->consoleModel                = conf->consoleModel;
	loader->romRead_LED                 = conf->romRead_LED;
	loader->dmaRomRead_LED              = conf->dmaRomRead_LED;
	loader->valueBits3                  = conf->valueBits3;

	nocashMessage("irqDisable(IRQ_ALL);");
	irqDisable(IRQ_ALL);

	// Direct CPU access to VRAM banks C & D
	VRAM_C_CR = VRAM_ENABLE | VRAM_C_LCD;
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;

	// Set the parameters for the loader

	// Load the loader into the correct address
	tonccpy(lc0, loader, 0x40000); //vramcpy(LCDC_BANK_D, loader, loaderSize);

	if(conf->gameOnFlashcard || conf->saveOnFlashcard) {
		// Patch the loader with a DLDI for the card
		if (!dldiPatchLoader ((data_t*)lc0, 0x40000, conf->initDisc)) {
			return 3;
		}
	}

	// Give the VRAM to the ARM7
	nocashMessage("Give the VRAM to the ARM7");
	VRAM_C_CR = VRAM_ENABLE | VRAM_C_ARM7_0x06000000;
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_ARM7_0x06020000;

	if (dsiFeatures() && conf->b4dsMode) {
		REG_SCFG_EXT = conf->b4dsMode == 2 ? 0x8300C000 : 0x83000000;
	}

	// Reset into a passme loop
	nocashMessage("Reset into a passme loop");
	REG_EXMEMCNT |= ARM7_OWNS_ROM | ARM7_OWNS_CARD;
	
	*(vu32*)0x02FFFFFC = 0;

	// Return to passme loop
	*(vu32*)0x02FFFE04 = (u32)0xE59FF018; // ldr pc, 0x02FFFE24
	*(vu32*)0x02FFFE24 = (u32)0x02FFFE04;  // Set ARM9 Loop address --> resetARM9(0x02FFFE04);
	
	// Reset ARM7
	nocashMessage("resetARM7");
	resetARM7(0x06000000);	

	// swi soft reset
	nocashMessage("swiSoftReset");
	swiSoftReset();

	return true;
}
