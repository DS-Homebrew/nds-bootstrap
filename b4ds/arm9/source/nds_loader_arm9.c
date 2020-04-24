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
#include <stdlib.h> // free
#include <string.h> // memcpy
#include <nds/interrupts.h>
#include <nds/arm9/video.h>
#include <nds/memory.h>
#include <nds/system.h>
#include <nds/bios.h>

#include <nds/arm9/dldi.h>
#include <nds/debug.h>

#include "tonccpy.h"
#include "hex.h"
#include "configuration.h"
#include "nds_loader_arm9.h"
#include "locations.h"
#include "load_crt0.h"
#include "cardengine_header_arm7.h"

//#define memcpy __builtin_memcpy

//#define LCDC_BANK_C (u16*)0x06840000

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

/*static inline u32 readAddr(const u8* mem, u32 offset) {
	return ((u32*)mem)[offset/sizeof(u32)];
}

static inline void writeAddr(u8* mem, u32 offset, u32 value) {
	((u32*)mem)[offset/sizeof(u32)] = value;
}*/

// See: arm7/source/main.c
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

static const data_t dldiMagicLoaderString[] = "\xEE\xA5\x8D\xBF Chishm"; // Different to a normal DLDI file

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

void runNds(loadCrt0* loader, u32 loaderSize, u32 cluster, u32 saveCluster, u32 donorE2Cluster, u32 donor2Cluster, u32 donorCluster, u32 apPatchCluster, u32 patchOffsetCacheCluster, u32 srParamsCluster, configuration* conf) {
	nocashMessage("runNds");

	irqDisable(IRQ_ALL);

	// Direct CPU access to VRAM bank D
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;

	// Set the parameters for the loader

	free(conf->ndsPath);
	free(conf->savPath);

	loader->storedFileCluster           = cluster;
	loader->romSize                     = conf->romSize;
	loader->initDisc                    = conf->initDisc;
	loader->wantToPatchDLDI             = conf->dldiPatchNds;
	loader->saveFileCluster             = saveCluster;
	loader->donorFileE2Cluster          = donorE2Cluster;
	loader->donorFile2Cluster           = donor2Cluster;
	loader->donorFileCluster            = donorCluster;
	loader->saveSize                    = conf->saveSize;
	loader->apPatchFileCluster          = apPatchCluster;
	loader->apPatchSize                 = conf->apPatchSize;
	loader->patchOffsetCacheFileCluster = patchOffsetCacheCluster;
	loader->srParamsFileCluster         = srParamsCluster;
	loader->language                    = conf->language;
	loader->donorSdkVer                 = conf->donorSdkVer;
	loader->patchMpuRegion              = conf->patchMpuRegion;
	loader->patchMpuSize                = conf->patchMpuSize;
	loader->ceCached                    = conf->ceCached; // SDK 1-4
	loader->boostVram                   = conf->boostVram;
	loader->forceSleepPatch             = conf->forceSleepPatch;
	loader->logging                     = conf->logging;

	free(conf);

	// Load the loader into the correct address
	tonccpy(lc0, loader, loaderSize); //vramcpy(LCDC_BANK_D, loader, loaderSize);

	// Patch the loader with a DLDI for the card
	if (!dldiPatchLoader ((data_t*)lc0, loaderSize, true)) {
		return;
	}

	nocashMessage("irqDisable(IRQ_ALL);");
	irqDisable(IRQ_ALL);

	// Give the VRAM to the ARM7
	nocashMessage("Give the VRAM to the ARM7");
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_ARM7_0x06020000;		
	
	// Reset into a passme loop
	nocashMessage("Reset into a passme loop");
	REG_EXMEMCNT |= ARM7_OWNS_ROM | ARM7_OWNS_CARD;
	
	*(vu32*)0x02FFFFFC = 0;

	// Return to passme loop
	*(vu32*)0x02FFFE04 = (u32)0xE59FF018; // ldr pc, 0x02FFFE24
	*(vu32*)0x02FFFE24 = (u32)0x02FFFE04;  // Set ARM9 Loop address --> resetARM9(0x02FFFE04);
	
	// Reset ARM7
	nocashMessage("resetARM7");
	resetARM7(0x06020000);	

	// swi soft reset
	nocashMessage("swiSoftReset");
	swiSoftReset();
}
