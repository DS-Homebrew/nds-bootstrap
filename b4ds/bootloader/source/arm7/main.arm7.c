/*-----------------------------------------------------------------
 boot.c
 
 BootLoader
 Loads a file into memory and runs it

 All resetMemory and startBinary functions are based 
 on the MultiNDS loader by Darkain.
 Original source available at:
 http://cvs.sourceforge.net/viewcvs.py/ndslib/ndslib/examples/loader/boot/main.cpp

 License:
	Copyright (C) 2005  Michael "Chishm" Chisholm

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

	If you use this code, please give due credit and email me about your
	project at chishm@hotmail.com
 
	Helpful information:
	This code runs from VRAM bank C on ARM7
------------------------------------------------------------------*/

#ifndef ARM7
# define ARM7
#endif
#include <string.h> // memcpy & memset
#include <stdlib.h> // malloc
#include <nds/ndstypes.h>
#include <nds/arm7/codec.h>
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>
#include <nds/memory.h> // tNDSHeader
#include <nds/arm7/i2c.h>
#include <nds/debug.h>

#include <nds/arm9/dldi.h>

#include "tonccpy.h"
#include "my_fat.h"
#include "nds_header.h"
#include "module_params.h"
#include "decompress.h"
//#include "dldi_patcher.h"
#include "ips.h"
#include "patch.h"
#include "find.h"
#include "cheat_patch.h"
#include "hook.h"
#include "common.h"
#include "locations.h"
#include "loading_screen.h"

#define STORED_FILE_CLUSTER_OFFSET 4 
#define INIT_DISC_OFFSET 8
#define WANT_TO_PATCH_DLDI_OFFSET 12
#define ARG_START_OFFSET 16
#define ARG_SIZE_OFFSET 20
#define HAVE_DSISD_OFFSET 28
#define DSIMODE_OFFSET 32

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

static void vramcpy (void* dst, const void* src, int len)
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

// Normal DLDI uses "\xED\xA5\x8D\xBF Chishm"
// Bootloader string is different to avoid being patched
static const data_t dldiMagicLoaderString[] = "\xEE\xA5\x8D\xBF Chishm";	// Different to a normal DLDI file

#define DEVICE_TYPE_DLDI 0x49444C44


//#define memcpy __builtin_memcpy

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

extern void arm7clearRAM(void);

//extern u32 _start;
extern u32 storedFileCluster;
extern u32 romSize;
extern u32 initDisc;
//extern u32 wantToPatchDLDI;
//extern u32 argStart;
//extern u32 argSize;
//extern u32 dsiSD;
extern u32 saveFileCluster;
extern u32 saveSize;
extern u32 apPatchFileCluster;
extern u32 apPatchSize;
extern u32 patchOffsetCacheFileCluster;
extern u32 language;
extern u32 dsiMode; // SDK 5
extern u32 donorSdkVer;
extern u32 patchMpuRegion;
extern u32 patchMpuSize;
extern u32 ceCached;
extern u32 boostVram;
//extern u32 forceSleepPatch;
//extern u32 logging;

static u32 ce9Location = 0;

static void initMBK(void) {
	// Give all DSi WRAM to ARM7 at boot
	// This function has no effect with ARM7 SCFG locked
	
	// ARM7 is master of WRAM-A, arm9 of WRAM-B & C
	REG_MBK9 = 0x3000000F;
	
	// WRAM-A fully mapped to ARM7
	*(vu32*)REG_MBK1 = 0x8185898D; // Same as DSiWare
	
	// WRAM-B fully mapped to ARM7 // inverted order
	*(vu32*)REG_MBK2 = 0x9195999D;
	*(vu32*)REG_MBK3 = 0x8185898D;
	
	// WRAM-C fully mapped to arm7 // inverted order
	*(vu32*)REG_MBK4 = 0x9195999D;
	*(vu32*)REG_MBK5 = 0x8185898D;
	
	// WRAM mapped to the 0x3700000 - 0x37FFFFF area 
	// WRAM-A mapped to the 0x37C0000 - 0x37FFFFF area : 256k
	REG_MBK6 = 0x080037C0; // same as DSiWare
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7 = 0x07C03740; // same as DSiWare
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8 = 0x07403700; // same as DSiWare
}

/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
static void resetMemory_ARM7(void) {
	register int i;
	
	REG_IME = 0;

	for (i = 0; i < 16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}

	REG_SOUNDCNT = 0;

	// Clear out ARM7 DMA channels and timers
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	arm7clearRAM();
	toncset((u32*)0x02000000, 0, 0x350000);
	toncset((u32*)0x02380000, 0, 0x5A000);
	toncset((u32*)0x023DB000, 0, 0x5000);
	toncset((u32*)0x023F0000, 0, 0xE000);
	toncset((u32*)0x023FF000, 0, 0x1000);
	if (extendedMemory) {
		toncset((u32*)0x02400000, 0, dsDebugRam ? 0x400000 : 0xC00000);
	}

	REG_IE = 0;
	REG_IF = ~0;
	*(vu32*)(0x04000000 - 4) = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)(0x04000000 - 8) = ~0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff
}

static module_params_t* buildModuleParams(u32 donorSdkVer) {
	//u32* moduleParamsOffset = malloc(sizeof(module_params_t));
	u32* moduleParamsOffset = malloc(0x100);

	//memset(moduleParamsOffset, 0, sizeof(module_params_t));
	memset(moduleParamsOffset, 0, 0x100);

	module_params_t* moduleParams = (module_params_t*)(moduleParamsOffset - 7);

	moduleParams->compressed_static_end = 0; // Avoid decompressing
	switch (donorSdkVer) {
		case 0:
		default:
			break;
		case 1:
			moduleParams->sdk_version = 0x1000500;
			break;
		case 2:
			moduleParams->sdk_version = 0x2001000;
			break;
		case 3:
			moduleParams->sdk_version = 0x3002001;
			break;
		case 4:
			moduleParams->sdk_version = 0x4002001;
			break;
		case 5:
			moduleParams->sdk_version = 0x5003001;
			break;
	}

	return moduleParams;
}

static module_params_t* getModuleParams(const tNDSHeader* ndsHeader) {
	nocashMessage("Looking for moduleparams...\n");

	u32* moduleParamsOffset = findModuleParamsOffset(ndsHeader);

	//module_params_t* moduleParams = (module_params_t*)((u32)moduleParamsOffset - 0x1C);
	return moduleParamsOffset ? (module_params_t*)(moduleParamsOffset - 7) : NULL;
}

/*static inline u32 getRomSizeNoArm9(const tNDSHeader* ndsHeader) {
	return ndsHeader->romSize - 0x4000 - ndsHeader->arm9binarySize;
}*/

// SDK 5
static bool ROMsupportsDsiMode(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode > 0);
}

static void loadBinary_ARM7(const tDSiHeader* dsiHeaderTemp, aFile file, bool dsiMode, bool* dsiModeConfirmedPtr) {
	nocashMessage("loadBinary_ARM7");

	//u32 ndsHeader[0x170 >> 2];
	//u32 dsiHeader[0x2F0 >> 2]; // SDK 5

	// Read DSi header (including NDS header)
	//fileRead((char*)ndsHeader, file, 0, 0x170, 3);
	//fileRead((char*)dsiHeader, file, 0, 0x2F0, 2); // SDK 5
	fileRead((char*)dsiHeaderTemp, file, 0, sizeof(*dsiHeaderTemp));

	// Fix Pokemon games needing header data.
	//fileRead((char*)0x027FF000, file, 0, 0x170, 3);
	//memcpy((char*)0x027FF000, &dsiHeaderTemp.ndshdr, sizeof(dsiHeaderTemp.ndshdr));
	tNDSHeader* ndsHeaderPokemon = (tNDSHeader*)NDS_HEADER_POKEMON;
	*ndsHeaderPokemon = dsiHeaderTemp->ndshdr;

	const char* romTid = getRomTid(&dsiHeaderTemp->ndshdr);
	if (
		strncmp(romTid, "ADA", 3) == 0    // Diamond
		|| strncmp(romTid, "APA", 3) == 0 // Pearl
		|| strncmp(romTid, "CPU", 3) == 0 // Platinum
		|| strncmp(romTid, "IPK", 3) == 0 // HG
		|| strncmp(romTid, "IPG", 3) == 0 // SS
	) {
		// Make the Pokemon game code ADAJ.
		const char gameCodePokemon[] = { 'A', 'D', 'A', 'J' };
		memcpy(ndsHeaderPokemon->gameCode, gameCodePokemon, 4);
	}

	// Load binaries into memory
	fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize);
	fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize);
}

static module_params_t* loadModuleParams(const tNDSHeader* ndsHeader, bool* foundPtr) {
	module_params_t* moduleParams = getModuleParams(ndsHeader);
	*foundPtr = (bool)moduleParams;
	if (*foundPtr) {
		// Found module params
	} else {
		nocashMessage("No moduleparams?\n");
		moduleParams = buildModuleParams(donorSdkVer);
	}
	return moduleParams;
}

static vu32* storeArm9StartAddress(tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	vu32* arm9StartAddress = (vu32*)(isSdk5(moduleParams) ? ARM9_START_ADDRESS_SDK5_LOCATION : ARM9_START_ADDRESS_LOCATION);

	// Store for later
	*arm9StartAddress = (vu32)ndsHeader->arm9executeAddress;
	
	// Exclude the ARM9 start address, so as not to start it
	ndsHeader->arm9executeAddress = NULL; // 0
	
	return arm9StartAddress;
}

static tNDSHeader* loadHeader(tDSiHeader* dsiHeaderTemp, const module_params_t* moduleParams, bool dsiMode) {
	tNDSHeader* ndsHeader = (tNDSHeader*)(isSdk5(moduleParams) ? NDS_HEADER_SDK5 : NDS_HEADER);

	// Copy the header to its proper location
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, (char*)ndsHeader, 0x170);
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, ndsHeader, sizeof(dsiHeaderTemp.ndshdr));
	*ndsHeader = dsiHeaderTemp->ndshdr;
	if (dsiMode) {
		//dmaCopyWords(3, &dsiHeaderTemp, ndsHeader, sizeof(dsiHeaderTemp));
		//*(tDSiHeader*)ndsHeader = *dsiHeaderTemp;
		tDSiHeader* dsiHeader = (tDSiHeader*)((u32)ndsHeader - (u32)__NDSHeader + (u32)__DSiHeader); // __DSiHeader
		*dsiHeader = *dsiHeaderTemp;
	}

	return ndsHeader;
}

static void my_readUserSettings(const tNDSHeader* ndsHeader) {
	PERSONAL_DATA slot1;
	PERSONAL_DATA slot2;

	short slot1count, slot2count; //u8
	short slot1CRC, slot2CRC;

	u32 userSettingsBase;

	// Get settings location
	readFirmware(0x20, &userSettingsBase, 2);

	u32 slot1Address = userSettingsBase * 8;
	u32 slot2Address = userSettingsBase * 8 + 0x100;

	// Reload DS Firmware settings
	readFirmware(slot1Address, &slot1, sizeof(PERSONAL_DATA)); //readFirmware(slot1Address, personalData, 0x70);
	readFirmware(slot2Address, &slot2, sizeof(PERSONAL_DATA)); //readFirmware(slot2Address, personalData, 0x70);
	readFirmware(slot1Address + 0x70, &slot1count, 2); //readFirmware(slot1Address + 0x70, &slot1count, 1);
	readFirmware(slot2Address + 0x70, &slot2count, 2); //readFirmware(slot1Address + 0x70, &slot2count, 1);
	readFirmware(slot1Address + 0x72, &slot1CRC, 2);
	readFirmware(slot2Address + 0x72, &slot2CRC, 2);

	// Default to slot 1 user settings
	void *currentSettings = &slot1;

	short calc1CRC = swiCRC16(0xFFFF, &slot1, sizeof(PERSONAL_DATA));
	short calc2CRC = swiCRC16(0xFFFF, &slot2, sizeof(PERSONAL_DATA));

	// Bail out if neither slot is valid
	if (calc1CRC != slot1CRC && calc2CRC != slot2CRC) {
		return;
	}

	// If both slots are valid pick the most recent
	if (calc1CRC == slot1CRC && calc2CRC == slot2CRC) { 
		currentSettings = (slot2count == ((slot1count + 1) & 0x7f) ? &slot2 : &slot1); //if ((slot1count & 0x7F) == ((slot2count + 1) & 0x7F)) {
	} else {
		if (calc2CRC == slot2CRC) {
			currentSettings = &slot2;
		}
	}

	PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u32)__NDSHeader - (u32)ndsHeader + (u32)PersonalData); //(u8*)((u32)ndsHeader - 0x180)
	
	memcpy(PersonalData, currentSettings, sizeof(PERSONAL_DATA));
	
	if (language >= 0 && language < 6) {
		// Change language
		personalData->language = language; //*(u8*)((u32)ndsHeader - 0x11C) = language;
	}
}

static bool supportsExceptionHandler(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	// ExceptionHandler2 (red screen) blacklist
	return (strncmp(romTid, "ASM", 3) != 0	// SM64DS
	&& strncmp(romTid, "SMS", 3) != 0	// SMSW
	&& strncmp(romTid, "A2D", 3) != 0	// NSMB
	&& strncmp(romTid, "ADM", 3) != 0);	// AC:WW
}

/*-------------------------------------------------------------------------
startBinary_ARM7
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
static void startBinary_ARM7(const vu32* tempArm9StartAddress) {
	REG_IME = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Copy NDS ARM9 start address into the header, starting ARM9
	ndsHeader->arm9executeAddress = (void*)*tempArm9StartAddress;

	// Get the ARM9 to boot
	arm9_stateFlag = ARM9_BOOTBIN;

	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM7
	VoidFn arm7code = (VoidFn)ndsHeader->arm7executeAddress;
	arm7code();
}

static void setMemoryAddress(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32 chipID = getChipId(ndsHeader, moduleParams);
    dbg_printf("chipID: ");
    dbg_hexa(chipID);
    dbg_printf("\n"); 

    // TODO
    // figure out what is 0x027ffc10, somehow related to cardId check
    //*((u32*)(isSdk5(moduleParams) ? 0x02fffc10 : 0x027ffc10)) = 1;

    // Set memory values expected by loaded NDS
    // from NitroHax, thanks to Chism
	*((u32*)(isSdk5(moduleParams) ? 0x02fff800 : 0x027ff800)) = chipID;					// CurrentCardID
	*((u32*)(isSdk5(moduleParams) ? 0x02fff804 : 0x027ff804)) = chipID;					// Command10CardID
	*((u16*)(isSdk5(moduleParams) ? 0x02fff808 : 0x027ff808)) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
	*((u16*)(isSdk5(moduleParams) ? 0x02fff80a : 0x027ff80a)) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	// Copies of above
	*((u32*)(isSdk5(moduleParams) ? 0x02fffc00 : 0x027ffc00)) = chipID;					// CurrentCardID
	*((u32*)(isSdk5(moduleParams) ? 0x02fffc04 : 0x027ffc04)) = chipID;					// Command10CardID
	*((u16*)(isSdk5(moduleParams) ? 0x02fffc08 : 0x027ffc08)) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
	*((u16*)(isSdk5(moduleParams) ? 0x02fffc0a : 0x027ffc0a)) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 0x1;						// Boot Indicator (Booted from card for SDK5) -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr
}

int arm7_main(void) {
	nocashMessage("bootloader");

	initMBK();
	
	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	nocashMessage("Getting ARM7 to clear RAM...\n");
	resetMemory_ARM7();

	// Init card
	if (!FAT_InitFiles(initDisc)) {
		nocashMessage("!FAT_InitFiles");
		errorOutput();
		//return -1;
	}

	// ROM file
	aFile* romFile = malloc(32);
	*romFile = getFileFromCluster(storedFileCluster);

	const char* bootName = "BOOT.NDS";

	// Invalid file cluster specified
	if ((romFile->firstCluster < CLUSTER_FIRST) || (romFile->firstCluster >= CLUSTER_EOF)) {
		*romFile = getBootFileCluster(bootName);
	}

	if (romFile->firstCluster == CLUSTER_FREE) {
		nocashMessage("fileCluster == CLUSTER_FREE");
		errorOutput();
		//return -1;
	}

	nocashMessage("status1");

	// Sav file
	aFile* savFile = malloc(32);
	*savFile = getFileFromCluster(saveFileCluster);
	
	// File containing cached patch offsets
	aFile patchOffsetCacheFile = getFileFromCluster(patchOffsetCacheFileCluster);
	fileRead((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	u16 prevPatchOffsetCacheFileVersion = patchOffsetCache.ver;

	int errorCode;

	tDSiHeader dsiHeaderTemp;

	// Load the NDS file
	nocashMessage("Loading the NDS file...\n");

	loadBinary_ARM7(&dsiHeaderTemp, *romFile, dsiMode, &dsiModeConfirmed);
	
	nocashMessage("Loading the header...\n");

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);

	ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams, foundModuleParams);
	if (decrypt_arm9(&dsiHeaderTemp.ndshdr)) {
		nocashMessage("Secure area decrypted successfully");
		dbg_printf("Secure area decrypted successfully");
	} else {
		nocashMessage("Secure area already decrypted");
		dbg_printf("Secure area already decrypted");
	}
	dbg_printf("\n");

	vu32* arm9StartAddress = storeArm9StartAddress(&dsiHeaderTemp.ndshdr, moduleParams);
	ndsHeader = loadHeader(&dsiHeaderTemp, moduleParams, dsiModeConfirmed);

	my_readUserSettings(ndsHeader); // Header has to be loaded first

	nocashMessage("Trying to patch the card...\n");

	//tonccpy((u32*)CARDENGINE_ARM7_LOCATION, (u32*)CARDENGINE_ARM7_LOCATION_BUFFERED, 0x1000);

	extern u32 _dldi_start;
	u32 dldiStart = (u32)(_dldi_start+0x40);
	u32 dldiEnd = (u32)(_dldi_start+0x44);
	u16 dldiSize = 0;
	for (u32 i = dldiStart; i <= dldiEnd; i += 4) {
		dldiSize += 4;
	}

	if (dldiSize >= 0x2000 && dldiSize < 0x3000) {
		ce9Location = CARDENGINE_ARM9_LOCATION_DLDI_12KB;
		tonccpy((u32*)CARDENGINE_ARM9_LOCATION_DLDI_12KB, (u32*)CARDENGINE_ARM9_LOCATION_BUFFERED1, 0x5000);
	} else if (dldiSize < 0x2000) {
		ce9Location = CARDENGINE_ARM9_LOCATION_DLDI_8KB;
		tonccpy((u32*)CARDENGINE_ARM9_LOCATION_DLDI_8KB, (u32*)CARDENGINE_ARM9_LOCATION_BUFFERED2, 0x4000);
	}
	toncset((u32*)0x023E0000, 0, 0x10000);

	if (!dldiPatchBinary((data_t*)ce9Location, 0x5000)) {
		nocashMessage("ce9 DLDI patch failed");
		dbg_printf("ce9 DLDI patch failed");
		dbg_printf("\n");
		errorOutput();
	}

	patchBinary(ndsHeader);
	errorCode = patchCardNds(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		(cardengineArm9*)ce9Location,
		ndsHeader,
		moduleParams,
		patchMpuRegion,
		patchMpuSize,
		saveFileCluster,
		saveSize
	);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card patch successful");
	} else {
		nocashMessage("Card patch failed");
		errorOutput();
	}

	errorCode = hookNdsRetailArm7(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		ndsHeader,
		moduleParams,
		language
	);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}

	u32 fatTableAddr = (((isSdk5(moduleParams) && ROMsupportsDsiMode(ndsHeader)) || (ndsHeader->romSize > 0x8000000) || !ceCached) ? 0x02380000 : (u32)patchHeapPointer(moduleParams, ndsHeader, romSize, saveSize));
	u32 fatTableSize = ((ndsHeader->romSize > 0x8000000) ? 0x8000 : 0x4000);
	if (moduleParams->sdk_version <= 0x2007FFF) {
		fatTableAddr = 0x023E8000;
	} else if (!ceCached) {
		fatTableAddr = 0x023D8000;
	}
	if (extendedMemory) {
		fatTableAddr = 0x02700000;
		fatTableSize = 0x40000;
	}

	errorCode = hookNdsRetailArm9(
		(cardengineArm9*)ce9Location,
		moduleParams,
		romFile->firstCluster,
		savFile->firstCluster,
		fatTableSize,
		fatTableAddr
	);
	/*if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}*/

	if (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheChanged) {
		fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	}

	aFile apPatchFile = getFileFromCluster(apPatchFileCluster);
	if (apPatchFile.firstCluster != CLUSTER_FREE && apPatchSize <= 0x30000) {
		fileRead((char*)IMAGES_LOCATION, apPatchFile, 0, apPatchSize);
		applyIpsPatch(ndsHeader, (u8*)IMAGES_LOCATION);
	}

	arm9_boostVram = boostVram;

	REG_SCFG_EXT &= ~(1UL << 31); // Lock SCFG

	toncset((u32*)IMAGES_LOCATION, 0, 0x30000);	// clear nds-bootstrap images and IPS patch
	clearScreen();

	while (arm9_stateFlag != ARM9_READY);
	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);

	nocashMessage("Starting the NDS file...");
    setMemoryAddress(ndsHeader, moduleParams);
	startBinary_ARM7(arm9StartAddress);

	return 0;
}
