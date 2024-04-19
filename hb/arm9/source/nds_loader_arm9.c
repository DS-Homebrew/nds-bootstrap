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
#include <string.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>

#include <unistd.h>
#include <fat.h>

#include "lzss.h"
#include "tonccpy.h"
#include "locations.h"

#ifndef _NO_BOOTSTUB_
#include "bootstub_bin.h"
#endif

#include "nds_loader_arm9.h"
#define LCDC_BANK_C (u16*)0x06840000
#define LCDC_BANK_D (u16*)0x06860000
#define STORED_FILE_CLUSTER (*(((u32*)LCDC_BANK_D) + 1))
#define INIT_DISC (*(((u32*)LCDC_BANK_D) + 2))
#define WANT_TO_PATCH_DLDI (*(((u32*)LCDC_BANK_D) + 3))


/*
	b	startUp
	
storedFileCluster:
	.word	0x0FFFFFFF		@ default BOOT.NDS
initDisc:
	.word	0x00000001		@ init the disc by default
wantToPatchDLDI:
	.word	0x00000001		@ by default patch the DLDI section of the loaded NDS
@ Used for passing arguments to the loaded app
argStart:
	.word	_end - _start
argSize:
	.word	0x00000000
dldiOffset:
	.word	_dldi_start - _start
dsiSD:
	.word	0
*/

#define STORED_FILE_CLUSTER_OFFSET 4 
#define INIT_DISC_OFFSET 8
#define WANT_TO_PATCH_DLDI_OFFSET 12
#define ARG_START_OFFSET 16
#define ARG_SIZE_OFFSET 20
#define HAVE_DSISD_OFFSET 28
#define LANGUAGE_OFFSET 32
#define DSIMODE_OFFSET 36
#define BOOSTVRAM_OFFSET 40
#define RAM_DISK_CLUSTER_OFFSET 44
#define RAM_DISK_SIZE_OFFSET 48
#define CFG_CLUSTER_OFFSET 52
#define CFG_SIZE_OFFSET 56
#define ROM_FILE_TYPE_OFFSET 60
#define ROM_IS_COMPRESSED_OFFSET 64
#define PATCHCACHE_FILE_OFFSET 68
#define SOFTRESET_FILE_OFFSET 72


typedef signed int addr_t;
typedef unsigned char data_t;

#define FIX_ALL	0x01
#define FIX_GLUE	0x02
#define FIX_GOT	0x04
#define FIX_BSS	0x08

enum DldiOffsets {
	DO_magicString = 0x00,			// "\xED\xA5\x8D\xBF Chishm"
	DO_magicToken = 0x00,			// 0xBF8DA5ED
	DO_magicShortString = 0x04,		// " Chishm"
	DO_version = 0x0C,
	DO_driverSize = 0x0D,
	DO_fixSections = 0x0E,
	DO_allocatedSpace = 0x0F,

	DO_friendlyName = 0x10,

	DO_text_start = 0x40,			// Data start
	DO_data_end = 0x44,				// Data end
	DO_glue_start = 0x48,			// Interworking glue start	-- Needs address fixing
	DO_glue_end = 0x4C,				// Interworking glue end
	DO_got_start = 0x50,			// GOT start					-- Needs address fixing
	DO_got_end = 0x54,				// GOT end
	DO_bss_start = 0x58,			// bss start					-- Needs setting to zero
	DO_bss_end = 0x5C,				// bss end

	// IO_INTERFACE data
	DO_ioType = 0x60,
	DO_features = 0x64,
	DO_startup = 0x68,	
	DO_isInserted = 0x6C,	
	DO_readSectors = 0x70,	
	DO_writeSectors = 0x74,
	DO_clearStatus = 0x78,
	DO_shutdown = 0x7C,
	DO_code = 0x80
};

static addr_t readAddr (data_t *mem, addr_t offset) {
	return ((addr_t*)mem)[offset/sizeof(addr_t)];
}

static void writeAddr (data_t *mem, addr_t offset, addr_t value) {
	((addr_t*)mem)[offset/sizeof(addr_t)] = value;
}

/*static void vramcpy (void* dst, const void* src, int len)
{
	u16* dst16 = (u16*)dst;
	u16* src16 = (u16*)src;
	
	//dmaCopy(src, dst, len);

	for ( ; len > 0; len -= 2) {
		*dst16++ = *src16++;
	}
}	

static addr_t quickFind (const data_t* data, const data_t* search, size_t dataLen, size_t searchLen) {
	const int* dataChunk = (const int*) data;
	int searchChunk = ((const int*)search)[0];
	addr_t i;
	addr_t dataChunkEnd = (addr_t)(dataLen / sizeof(int));

	for ( i = 0; i < dataChunkEnd; i++) {
		if (dataChunk[i] == searchChunk) {
			if ((i*sizeof(int) + searchLen) > dataLen) {
				return -1;
			}
			if (memcmp (&data[i*sizeof(int)], search, searchLen) == 0) {
				return i*sizeof(int);
			}
		}
	}

	return -1;
}

//static const data_t dldiMagicString[] = "\xED\xA5\x8D\xBF Chishm";	// Normal DLDI file
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
	
	nocashMessage("dldiPatchLoader");
	
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
		nocashMessage("No DLDI patch");
		return false;
	}

	if (pDH[DO_driverSize] > pAH[DO_allocatedSpace]) {
		// Not enough space for patch	
		nocashMessage("Not enough space for patch");
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
	vramcpy (pAH, pDH, dldiFileSize);

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
		memset (&pAH[readAddr(pDH, DO_bss_start) - ddmemStart] , 0, readAddr(pDH, DO_bss_end) - readAddr(pDH, DO_bss_start));
	}

	return true;
}*/

char load_bin[0x10000];
char loadInject_bin[0x8000];
char imgTemplateBuffer[0xEA00];

int runNds (const void* loader, u32 loaderSize, u32 cluster, u32 ramDiskCluster, u32 ramDiskSize, u32 srParamsCluster, u32 patchOffsetCacheCluster, u32 cfgCluster, u32 cfgSize, int romToRamDisk, bool romIsCompressed, bool initDisc, bool dldiPatchNds, int argc, const char** argv, int language, int dsiMode, bool boostVram, int consoleModel, u32 srTid1, u32 srTid2)
{
	char* argStart;
	u16* argData;
	u16 argTempVal = 0;
	int argSize;
	const char* argChar;
	
	nocashMessage("runNds");

	irqDisable(IRQ_ALL);

	// Direct CPU access to VRAM bank D
	VRAM_C_CR = VRAM_ENABLE | VRAM_C_LCD;
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;
	// Load the loader/patcher into the correct address
	tonccpy (LCDC_BANK_C, loadInject_bin, 0x8000);
	tonccpy (LCDC_BANK_D, loader, loaderSize);

	if (romToRamDisk != -1) {
		tonccpy ((u8*)LCDC_BANK_C+0x10000, imgTemplateBuffer, sizeof(imgTemplateBuffer));
	}

	// Set the parameters for the loader
	// STORED_FILE_CLUSTER = cluster;
	writeAddr ((data_t*) LCDC_BANK_D, STORED_FILE_CLUSTER_OFFSET, cluster);
	// INIT_DISC = initDisc;
	writeAddr ((data_t*) LCDC_BANK_D, INIT_DISC_OFFSET, initDisc);

	/*if(argv[0][0]=='s' && argv[0][1]=='d') {
		dldiPatchNds = false;
		writeAddr ((data_t*) LCDC_BANK_D, HAVE_DSISD_OFFSET, 1);
	}*/

	// WANT_TO_PATCH_DLDI = dldiPatchNds;
	writeAddr ((data_t*) LCDC_BANK_D, WANT_TO_PATCH_DLDI_OFFSET, dldiPatchNds);
	// Give arguments to loader
	argStart = (char*)LCDC_BANK_D + readAddr((data_t*)LCDC_BANK_D, ARG_START_OFFSET);
	argStart = (char*)(((int)argStart + 3) & ~3);	// Align to word
	argData = (u16*)argStart;
	argSize = 0;
	
	for (; argc > 0 && *argv; ++argv, --argc) 
	{
		for (argChar = *argv; *argChar != 0; ++argChar, ++argSize) 
		{
			if (argSize & 1) 
			{
				argTempVal |= (*argChar) << 8;
				*argData = argTempVal;
				++argData;
			} 
			else 
			{
				argTempVal = *argChar;
			}
		}
		if (argSize & 1)
		{
			*argData = argTempVal;
			++argData;
		}
		argTempVal = 0;
		++argSize;
	}
	*argData = argTempVal;
	
	writeAddr ((data_t*) LCDC_BANK_C, 0x24, consoleModel);
	writeAddr ((data_t*) LCDC_BANK_C, 0x28, srParamsCluster);
	writeAddr ((data_t*) LCDC_BANK_C, 0x2C, srTid1);
	writeAddr ((data_t*) LCDC_BANK_C, 0x30, srTid2);

	writeAddr ((data_t*) LCDC_BANK_D, ARG_START_OFFSET, (addr_t)argStart - (addr_t)LCDC_BANK_D);
	writeAddr ((data_t*) LCDC_BANK_D, ARG_SIZE_OFFSET, argSize);
	writeAddr ((data_t*) LCDC_BANK_D, LANGUAGE_OFFSET, language);
	writeAddr ((data_t*) LCDC_BANK_D, DSIMODE_OFFSET, dsiMode);
	writeAddr ((data_t*) LCDC_BANK_D, BOOSTVRAM_OFFSET, boostVram);
	writeAddr ((data_t*) LCDC_BANK_D, RAM_DISK_CLUSTER_OFFSET, ramDiskCluster);
	writeAddr ((data_t*) LCDC_BANK_D, RAM_DISK_SIZE_OFFSET, ramDiskSize);
	writeAddr ((data_t*) LCDC_BANK_D, CFG_CLUSTER_OFFSET, cfgCluster);
	writeAddr ((data_t*) LCDC_BANK_D, CFG_SIZE_OFFSET, cfgSize);
	writeAddr ((data_t*) LCDC_BANK_D, ROM_FILE_TYPE_OFFSET, romToRamDisk);
	writeAddr ((data_t*) LCDC_BANK_D, ROM_IS_COMPRESSED_OFFSET, romIsCompressed);
	writeAddr ((data_t*) LCDC_BANK_D, PATCHCACHE_FILE_OFFSET, patchOffsetCacheCluster);
	writeAddr ((data_t*) LCDC_BANK_D, SOFTRESET_FILE_OFFSET, srParamsCluster);

		
	/*if(dldiPatchNds) {
		// Patch the loader with a DLDI for the card
		nocashMessage("dldiPatchNds");
		if (!dldiPatchLoader ((data_t*)LCDC_BANK_D, loaderSize, initDisc)) {
			nocashMessage("return 3");
			return 3;
		}
	}*/
	
	nocashMessage("irqDisable(IRQ_ALL);");

	irqDisable(IRQ_ALL);

	nocashMessage("Give the VRAM to the ARM7");
	// Give the VRAM to the ARM7
	VRAM_C_CR = VRAM_ENABLE | VRAM_C_ARM7_0x06000000;
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_ARM7_0x06020000;
	
	nocashMessage("Reset into a passme loop");
	// Reset into a passme loop
	REG_EXMEMCNT |= ARM7_OWNS_ROM | ARM7_OWNS_CARD;
	
	*((vu32*)0x02FFFFFC) = 0;
	*((vu32*)0x02FFFE04) = (u32)0xE59FF018;
	*((vu32*)0x02FFFE24) = (u32)0x02FFFE04;
	
	nocashMessage("resetARM7");

	resetARM7(0x06020000);
	
	nocashMessage("swiSoftReset");

	swiSoftReset(); 
	return true;
}

int runNdsFile (const char* filename, const char* ramDiskFilename, const char* cfgFilename, u32 ramDiskSize, const char* srParamsFilename, const char* patchOffsetCacheFilename, u32 cfgSize, int romToRamDisk, bool romIsCompressed, int argc, const char** argv, int language, int dsiMode, bool boostVram, int consoleModel, u32 srTid1, u32 srTid2) {
	struct stat st;
	struct stat stRam;
	struct stat stCfg;
	struct stat stPatchCache;
	struct stat stSr;
	u32 clusterRam = 0;
	u32 clusterCfg = 0;
	u32 clusterPatchCache = 0;
	u32 clusterSr = 0;
	char filePath[PATH_MAX];
	int pathLen;
	const char* args[1];

	FILE *ramDiskTemplate;
	if (romToRamDisk == 4) {
		ramDiskTemplate = fopen("nitro:/imgTemplate_PCE.bin", "rb");
		if (ramDiskTemplate) fread(imgTemplateBuffer, 1, sizeof(imgTemplateBuffer), ramDiskTemplate);
		fclose(ramDiskTemplate);
	} else if (romToRamDisk == 3) {
		ramDiskTemplate = fopen("nitro:/imgTemplate_GG.bin", "rb");
		if (ramDiskTemplate) fread(imgTemplateBuffer, 1, sizeof(imgTemplateBuffer), ramDiskTemplate);
		fclose(ramDiskTemplate);
	} else if (romToRamDisk == 2) {
		ramDiskTemplate = fopen("nitro:/imgTemplate_SMS.bin", "rb");
		if (ramDiskTemplate) fread(imgTemplateBuffer, 1, sizeof(imgTemplateBuffer), ramDiskTemplate);
		fclose(ramDiskTemplate);
	} else if (romToRamDisk == 1) {
		ramDiskTemplate = fopen("nitro:/imgTemplate_SNES.bin", "rb");
		if (ramDiskTemplate) fread(imgTemplateBuffer, 1, sizeof(imgTemplateBuffer), ramDiskTemplate);
		fclose(ramDiskTemplate);
	} else if (romToRamDisk == 0) {
		ramDiskTemplate = fopen("nitro:/imgTemplate_SegaMD.bin", "rb");
		if (ramDiskTemplate) fread(imgTemplateBuffer, 1, sizeof(imgTemplateBuffer), ramDiskTemplate);
		fclose(ramDiskTemplate);
	}

	if (romIsCompressed) {
		ramDiskTemplate = fopen(ramDiskFilename, "rb");
		if (ramDiskTemplate) {
			fread((void*)RAM_DISK_LOCATION_LZ77ROM, 1, ramDiskSize, ramDiskTemplate);
			fclose(ramDiskTemplate);
			if (romToRamDisk == 1) {
				LZ77_Decompress((u8*)RAM_DISK_LOCATION_LZ77ROM, (u8*)RAM_DISK_LOCATION+RAM_DISK_SNESROM);
			} else if (romToRamDisk == 0 || romToRamDisk == 2 || romToRamDisk == 3 || romToRamDisk == 4) {
				LZ77_Decompress((u8*)RAM_DISK_LOCATION_LZ77ROM, (u8*)RAM_DISK_LOCATION+RAM_DISK_MDROM);
			}
		}
	}

	ramDiskTemplate = fopen("nitro:/load.bin", "rb");
	if (ramDiskTemplate) {
		fread((void*)load_bin, 1, 0x10000, ramDiskTemplate);
		fclose(ramDiskTemplate);
	}

	ramDiskTemplate = fopen("nitro:/loadInject.bin", "rb");
	if (ramDiskTemplate) {
		fread((void*)loadInject_bin, 1, 0x8000, ramDiskTemplate);
		fclose(ramDiskTemplate);
	}

	if (stat (filename, &st) < 0) {
		return 1;
	}
	
	if (ramDiskSize > 0) {
		if (stat(ramDiskFilename, &stRam) >= 0) {
			clusterRam = stRam.st_ino;
		}
	}

	if (stat(cfgFilename, &stCfg) >= 0) {
		clusterCfg = stCfg.st_ino;
	}

	if (stat(patchOffsetCacheFilename, &stPatchCache) >= 0) {
		clusterPatchCache = stPatchCache.st_ino;
	}

	if (stat(srParamsFilename, &stSr) >= 0) {
		clusterSr = stSr.st_ino;
	}

	if (argc <= 0 || !argv) {
		// Construct a command line if we weren't supplied with one
		if (!getcwd (filePath, PATH_MAX)) {
			return 2;
		}
		pathLen = strlen (filePath);
		strcpy (filePath + pathLen, filename);
		args[0] = filePath;
		argv = args;
	}

	//bool havedsiSD = false;

	//if(argv[0][0]=='s' && argv[0][1]=='d') havedsiSD = true;
	
	//installBootStub(havedsiSD);

	return runNds (load_bin, 0x10000, st.st_ino, clusterRam, ramDiskSize, clusterSr, clusterPatchCache, clusterCfg, cfgSize, romToRamDisk, romIsCompressed, true, true, argc, argv, language, dsiMode, boostVram, consoleModel, srTid1, srTid2);
}

/*
	b	startUp
	
storedFileCluster:
	.word	0x0FFFFFFF		@ default BOOT.NDS
initDisc:
	.word	0x00000001		@ init the disc by default
wantToPatchDLDI:
	.word	0x00000001		@ by default patch the DLDI section of the loaded NDS
@ Used for passing arguments to the loaded app
argStart:
	.word	_end - _start
argSize:
	.word	0x00000000
dldiOffset:
	.word	_dldi_start - _start
dsiSD:
	.word	0
*/
/*bool installBootStub(bool havedsiSD) {
#ifndef _NO_BOOTSTUB_
	nocashMessage("installBootStub");
	extern char *fake_heap_end;
	struct __bootstub *bootcode = (struct __bootstub *)fake_heap_end;

	memcpy(fake_heap_end,bootstub_bin,bootstub_bin_size);
	memcpy(fake_heap_end+bootstub_bin_size,load_bin,load_bin_size);
	bool ret = false;

	if( havedsiSD) {
		ret = true;
		u32 *bootcode = (u32*)(fake_heap_end+bootstub_bin_size);
		bootcode[3] = 0; // don't dldi patch
		bootcode[7] = 1; // use internal dsi SD code
	} else {
		ret = dldiPatchLoader((data_t*)(fake_heap_end+bootstub_bin_size), load_bin_size,false);
	}
	bootcode->arm9reboot = (VoidFn)(((u32)bootcode->arm9reboot)+fake_heap_end); 
	bootcode->arm7reboot = (VoidFn)(((u32)bootcode->arm7reboot)+fake_heap_end); 
	bootcode->bootsize = load_bin_size;
	
	DC_FlushAll();

	return ret;
#else
	return true;
#endif

}*/

