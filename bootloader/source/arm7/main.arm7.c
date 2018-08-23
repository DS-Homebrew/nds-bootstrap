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

#include "my_fat.h"
#include "nds_header.h"
#include "module_params.h"
#include "decompress.h"
//#include "dldi_patcher.h"
#include "patch.h"
#include "find.h"
#include "cheat_patch.h"
#include "hook.h"
#include "common.h"
#include "locations.h"
#include "loading_screen.h"

#include "cardengine_arm7_bin.h"
#include "cardengine_arm9_bin.h"

//#define memcpy __builtin_memcpy

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

extern void arm7clearRAM(void);

//extern u32 _start;
extern u32 storedFileCluster;
extern u32 initDisc;
//extern u32 wantToPatchDLDI;
//extern u32 argStart;
//extern u32 argSize;
//extern u32 dsiSD;
extern u32 saveFileCluster;
extern u32 saveSize;
extern u32 language;
extern u32 dsiMode; // SDK 5
extern u32 donorSdkVer;
extern u32 patchMpuRegion;
extern u32 patchMpuSize;
extern u32 consoleModel;
//extern u32 loadingScreen;
extern u32 romread_LED;
extern u32 gameSoftReset;
extern u32 asyncPrefetch;
extern u32 soundFix;
//extern u32 logging;

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
	REG_IME = 0;

	for (int i = 0; i < 16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}

	REG_SOUNDCNT = 0;

	// Clear out ARM7 DMA channels and timers
	for (int i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	arm7clearRAM();

	REG_IE = 0;
	REG_IF = ~0;
	*(vu32*)(0x04000000 - 4) = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)(0x04000000 - 8) = ~0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff
}

static void NDSTouchscreenMode(void) {
	//unsigned char * *(unsigned char*)0x40001C0=		(unsigned char*)0x40001C0;
	//unsigned char * *(unsigned char*)0x40001C0byte2=(unsigned char*)0x40001C1;
	//unsigned char * *(unsigned char*)0x40001C2=	(unsigned char*)0x40001C2;
	//unsigned char * I2C_DATA=	(unsigned char*)0x4004500;
	//unsigned char * I2C_CNT=	(unsigned char*)0x4004501;

	u8 volLevel;
	
	//if (fifoCheckValue32(FIFO_MAXMOD)) {
	//	// special setting (when found special gamecode)
	//	volLevel = 0xAC;
	//} else {
		// normal setting (for any other gamecodes)
		volLevel = 0xA7;
	//}

	if (REG_SCFG_EXT == 0) {
		volLevel += 0x13;
	}

	// Touchscreen
	cdcReadReg (0x63, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x00);
	cdcReadReg (CDC_CONTROL, 0x51);
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcReadReg (CDC_CONTROL, 0x3F);
	cdcReadReg (CDC_SOUND, 0x28);
	cdcReadReg (CDC_SOUND, 0x2A);
	cdcReadReg (CDC_SOUND, 0x2E);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x40, 0x0C);
	cdcWriteReg(CDC_SOUND, 0x24, 0xFF);
	cdcWriteReg(CDC_SOUND, 0x25, 0xFF);
	cdcWriteReg(CDC_SOUND, 0x26, 0x7F);
	cdcWriteReg(CDC_SOUND, 0x27, 0x7F);
	cdcWriteReg(CDC_SOUND, 0x28, 0x4A);
	cdcWriteReg(CDC_SOUND, 0x29, 0x4A);
	cdcWriteReg(CDC_SOUND, 0x2A, 0x10);
	cdcWriteReg(CDC_SOUND, 0x2B, 0x10);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(CDC_SOUND, 0x23, 0x00);
	cdcWriteReg(CDC_SOUND, 0x1F, 0x14);
	cdcWriteReg(CDC_SOUND, 0x20, 0x14);
	cdcWriteReg(CDC_CONTROL, 0x3F, 0x00);
	cdcReadReg (CDC_CONTROL, 0x0B);
	cdcWriteReg(CDC_CONTROL, 0x05, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x0B, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x0C, 0x02);
	cdcWriteReg(CDC_CONTROL, 0x12, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x13, 0x02);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x60);
	cdcWriteReg(CDC_CONTROL, 0x01, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x39, 0x66);
	cdcReadReg (CDC_SOUND, 0x20);
	cdcWriteReg(CDC_SOUND, 0x20, 0x10);
	cdcWriteReg(CDC_CONTROL, 0x04, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x12, 0x81);
	cdcWriteReg(CDC_CONTROL, 0x13, 0x82);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x82);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x04, 0x03);
	cdcWriteReg(CDC_CONTROL, 0x05, 0xA1);
	cdcWriteReg(CDC_CONTROL, 0x06, 0x15);
	cdcWriteReg(CDC_CONTROL, 0x0B, 0x87);
	cdcWriteReg(CDC_CONTROL, 0x0C, 0x83);
	cdcWriteReg(CDC_CONTROL, 0x12, 0x87);
	cdcWriteReg(CDC_CONTROL, 0x13, 0x83);
	cdcReadReg (CDC_TOUCHCNT, 0x10);
	cdcWriteReg(CDC_TOUCHCNT, 0x10, 0x08);
	cdcWriteReg(0x04, 0x08, 0x7F);
	cdcWriteReg(0x04, 0x09, 0xE1);
	cdcWriteReg(0x04, 0x0A, 0x80);
	cdcWriteReg(0x04, 0x0B, 0x1F);
	cdcWriteReg(0x04, 0x0C, 0x7F);
	cdcWriteReg(0x04, 0x0D, 0xC1);
	cdcWriteReg(CDC_CONTROL, 0x41, 0x08);
	cdcWriteReg(CDC_CONTROL, 0x42, 0x08);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x00);
	cdcWriteReg(0x04, 0x08, 0x7F);
	cdcWriteReg(0x04, 0x09, 0xE1);
	cdcWriteReg(0x04, 0x0A, 0x80);
	cdcWriteReg(0x04, 0x0B, 0x1F);
	cdcWriteReg(0x04, 0x0C, 0x7F);
	cdcWriteReg(0x04, 0x0D, 0xC1);
	cdcWriteReg(CDC_SOUND, 0x2F, 0x2B);
	cdcWriteReg(CDC_SOUND, 0x30, 0x40);
	cdcWriteReg(CDC_SOUND, 0x31, 0x40);
	cdcWriteReg(CDC_SOUND, 0x32, 0x60);
	cdcReadReg (CDC_CONTROL, 0x74);
	cdcWriteReg(CDC_CONTROL, 0x74, 0x02);
	cdcReadReg (CDC_CONTROL, 0x74);
	cdcWriteReg(CDC_CONTROL, 0x74, 0x10);
	cdcReadReg (CDC_CONTROL, 0x74);
	cdcWriteReg(CDC_CONTROL, 0x74, 0x40);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcReadReg (CDC_CONTROL, 0x51);
	cdcReadReg (CDC_CONTROL, 0x3F);
	cdcWriteReg(CDC_CONTROL, 0x3F, 0xD4);
	cdcWriteReg(CDC_SOUND, 0x23, 0x44);
	cdcWriteReg(CDC_SOUND, 0x1F, 0xD4);
	cdcWriteReg(CDC_SOUND, 0x28, 0x4E);
	cdcWriteReg(CDC_SOUND, 0x29, 0x4E);
	cdcWriteReg(CDC_SOUND, 0x24, 0x9E);
	cdcWriteReg(CDC_SOUND, 0x24, 0x9E);
	cdcWriteReg(CDC_SOUND, 0x20, 0xD4);
	cdcWriteReg(CDC_SOUND, 0x2A, 0x14);
	cdcWriteReg(CDC_SOUND, 0x2B, 0x14);
	cdcWriteReg(CDC_SOUND, 0x26, 0xA7);
	cdcWriteReg(CDC_SOUND, 0x27, 0xA7);
	cdcWriteReg(CDC_CONTROL, 0x40, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x60);
	cdcWriteReg(CDC_SOUND, 0x26, volLevel);
	cdcWriteReg(CDC_SOUND, 0x27, volLevel);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x03);
	cdcWriteReg(CDC_TOUCHCNT, 0x03, 0x00);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcReadReg (CDC_SOUND, 0x22);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(0xFF, 0x05, 0x00); //writeTSC(0x00, 0xFF);
	
	// Power management
	writePowerManagement(0x80, 0x00);
	writePowerManagement(0x00, 0x0D);
	//*(unsigned char*)0x40001C2 = 0x80, 0x00; // read PWR[0]   ;<-- also part of TSC !
	//*(unsigned char*)0x40001C2 = 0x00, 0x0D; // PWR[0]=0Dh    ;<-- also part of TSC !
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

static inline u32 getRomSizeNoArm9(const tNDSHeader* ndsHeader) {
	return ndsHeader->romSize - 0x4000 - ndsHeader->arm9binarySize;
}

// SDK 5
static bool ROMsupportsDsiMode(const tNDSHeader* ndsHeader) {
	//dsiModeConfirmed = (dsiMode && (dsiHeaderTemp[0x10 >> 2] & BIT(16+1));
	//dsiModeConfirmed = (dsiMode && (dsiHeaderTemp->ndshdr.deviceSize & BIT(16+1)));
	return (ndsHeader->unitCode > 0);
}

static void loadBinary_ARM7(const tDSiHeader* dsiHeaderTemp, aFile file, bool dsiMode, bool* dsiModeConfirmedPtr) {
	nocashMessage("loadBinary_ARM7");

	//u32 ndsHeader[0x170 >> 2];
	//u32 dsiHeader[0x2F0 >> 2]; // SDK 5

	// Read DSi header (including NDS header)
	//fileRead((char*)ndsHeader, file, 0, 0x170, 3);
	//fileRead((char*)dsiHeader, file, 0, 0x2F0, 2); // SDK 5
	fileRead((char*)dsiHeaderTemp, file, 0, sizeof(*dsiHeaderTemp), 3);

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
	fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize, 3);
	fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize, 3);

	// SDK 5
	*dsiModeConfirmedPtr = dsiMode && ROMsupportsDsiMode(&dsiHeaderTemp->ndshdr);
	if (*dsiModeConfirmedPtr) {
		fileRead(dsiHeaderTemp->arm9idestination, file, (u32)dsiHeaderTemp->arm9iromOffset, dsiHeaderTemp->arm9ibinarySize, 3);
		fileRead(dsiHeaderTemp->arm7idestination, file, (u32)dsiHeaderTemp->arm7iromOffset, dsiHeaderTemp->arm7ibinarySize, 3);
	}
}

static module_params_t* loadModuleParams(const tNDSHeader* ndsHeader, bool* foundPtr) {
	module_params_t* moduleParams = getModuleParams(ndsHeader);
	*foundPtr = (bool)moduleParams;
	if (*foundPtr) {
		// Found module params
		//*(vu32*)0x2800008 = ((u32)moduleParamsOffset - 0x8);
		//*(vu32*)0x2800008 = (vu32)(moduleParamsOffset - 2);
		*(vu32*)0x2800008 = (vu32)((u32*)moduleParams + 5); // (u32*)moduleParams + 7 - 2
	} else {
		nocashMessage("No moduleparams?\n");
		moduleParams = buildModuleParams(donorSdkVer);
		*(vu32*)0x2800010 = 1;
	}
	return moduleParams;
}

static bool isROMLoadableInRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 consoleModel) {
	return ((isSdk5(moduleParams) && consoleModel > 0 && getRomSizeNoArm9(ndsHeader) <= 0x01000000)
		|| (!isSdk5(moduleParams) && consoleModel > 0 && getRomSizeNoArm9(ndsHeader) <= 0x017FC000)
		|| (!isSdk5(moduleParams) && consoleModel == 0 && getRomSizeNoArm9(ndsHeader) <= 0x007FC000));
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
	if (dsiMode) {
		//dmaCopyWords(3, &dsiHeaderTemp, ndsHeader, sizeof(dsiHeaderTemp));
		*(tDSiHeader*)ndsHeader = *dsiHeaderTemp;
	} else {
		//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, ndsHeader, sizeof(dsiHeaderTemp.ndshdr));
		*ndsHeader = dsiHeaderTemp->ndshdr;
	}

	return ndsHeader;
}

static void reloadFirmwareSettings(const tNDSHeader* ndsHeader) {
	u8 settings1, settings2;
	u32 settingsOffset = 0;

	// Get settings location
	readFirmware((u32)0x00020, (u8*)&settingsOffset, 0x2);
	settingsOffset *= 8;

	// Reload DS Firmware settings
	readFirmware(settingsOffset + 0x070, &settings1, 0x1);
	readFirmware(settingsOffset + 0x170, &settings2, 0x1);

	if ((settings1 & 0x7F) == ((settings2 + 1) & 0x7F)) {
		readFirmware(settingsOffset + 0x000, (u8*)((u32)ndsHeader - 0x180), 0x70);
	} else {
		readFirmware(settingsOffset + 0x100, (u8*)((u32)ndsHeader - 0x180), 0x70);
	}
	if (language >= 0 && language < 6) {
		// Change language
		*(u8*)((u32)ndsHeader - 0x11C) = language;
	}
}

static void NTR_BIOS() {
	// Switch to NTR mode BIOS (no effect with locked ARM7 SCFG)
	nocashMessage("Switch to NTR mode BIOS");
	REG_SCFG_ROM = 0x703;
}

static void loadROMintoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile file) {
	char* romLocation = (char*)(isSdk5(moduleParams) ? ROM_SDK5_LOCATION : ROM_LOCATION);

	// Load ROM into RAM
	fileRead(romLocation, file, 0x4000 + ndsHeader->arm9binarySize, getRomSizeNoArm9(ndsHeader), 0);
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

int arm7_main(void) {
	nocashMessage("bootloader");

	initMBK();
	
	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	nocashMessage("Getting ARM7 to clear RAM...\n");
	debugOutput();

	//
	// 1 dot
	//

	resetMemory_ARM7();

	// Init card
	if (!FAT_InitFiles(initDisc, 3)) {
		nocashMessage("!FAT_InitFiles");
		return -1;
	}

	// ROM file
	aFile* romFile = (aFile*)0x37D5000;
	*romFile = getFileFromCluster(storedFileCluster);

	const char* bootName = "BOOT.NDS";

	// Invalid file cluster specified
	if ((romFile->firstCluster < CLUSTER_FIRST) || (romFile->firstCluster >= CLUSTER_EOF)) {
		*romFile = getBootFileCluster(bootName, 3);
	}

	if (romFile->firstCluster == CLUSTER_FREE) {
		nocashMessage("fileCluster == CLUSTER_FREE");
		return -1;
	}
	
	buildFatTableCache(romFile, 3);

	// Sav file
	aFile* savFile = romFile + 1; //aFile* savFile = (aFile*)0x37D5000 + 1;
	*savFile = getFileFromCluster(saveFileCluster);
	
	if (savFile->firstCluster != CLUSTER_FREE) {
		buildFatTableCache(savFile, 3);
	}

	int errorCode;

	if (REG_SCFG_EXT == 0) {
		NDSTouchscreenMode();
		*(u16*)0x4000500 = 0x807F;
	}

	tDSiHeader dsiHeaderTemp;

	// Load the NDS file
	nocashMessage("Loading the NDS file...\n");

	//bool dsiModeConfirmed;
	loadBinary_ARM7(&dsiHeaderTemp, *romFile, dsiMode, &dsiModeConfirmed);
	
	increaseLoadBarLength();

	//
	// 2 dots
	//

	nocashMessage("Loading the header...\n");

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);

	ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams, foundModuleParams);

	// If possible, set to load ROM into RAM
	u32 ROMinRAM = isROMLoadableInRAM(&dsiHeaderTemp.ndshdr, moduleParams, consoleModel);

	vu32* arm9StartAddress = storeArm9StartAddress(&dsiHeaderTemp.ndshdr, moduleParams);
	ndsHeader = loadHeader(&dsiHeaderTemp, moduleParams, dsiModeConfirmed);

	reloadFirmwareSettings(ndsHeader); // Header has to be loaded first

	if (!dsiModeConfirmed) {
		NTR_BIOS();
	}

	increaseLoadBarLength();

	//
	// 3 dots
	//

	nocashMessage("Trying to patch the card...\n");

	memcpy((u32*)CARDENGINE_ARM7_LOCATION, (u32*)cardengine_arm7_bin, cardengine_arm7_bin_size);
	increaseLoadBarLength();

	//
	// 4 dots
	//

	memcpy((u32*)CARDENGINE_ARM9_LOCATION, (u32*)cardengine_arm9_bin, cardengine_arm9_bin_size);
	increaseLoadBarLength();

	//
	// 5 dots
	//
	patchBinary(ndsHeader);
	errorCode = patchCardNds(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		(cardengineArm9*)CARDENGINE_ARM9_LOCATION,
		ndsHeader,
		moduleParams,
		patchMpuRegion,
		patchMpuSize,
		ROMinRAM,
		saveFileCluster,
		saveSize
	);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card patch successful");
	} else {
		nocashMessage("Card patch failed");
		errorOutput();
	}
	increaseLoadBarLength();

	//
	// 6 dots
	//

	cheatPatch((cardengineArm7*)CARDENGINE_ARM7_LOCATION, ndsHeader);
	errorCode = hookNdsRetailArm7(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		ndsHeader,
		moduleParams,
		romFile->firstCluster,
		language,
		dsiModeConfirmed,
		ROMinRAM,
		consoleModel,
		romread_LED,
		gameSoftReset,
		soundFix
	);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}
	increaseLoadBarLength();

	//
	// 7 dots
	//

	hookNdsRetailArm9(
		(cardengineArm9*)CARDENGINE_ARM9_LOCATION,
		moduleParams,
		ROMinRAM,
		dsiModeConfirmed,
		supportsExceptionHandler(ndsHeader),
		consoleModel,
		asyncPrefetch
	);
	if (ROMinRAM) {
		loadROMintoRAM(ndsHeader, moduleParams, *romFile);
	} else {
		if (romread_LED == 1 || (romread_LED > 0 && asyncPrefetch)) {
			// Turn WiFi LED off
			i2cWriteRegister(0x4A, 0x30, 0x12);
		}
	}
	increaseLoadBarLength();

	//
	// Final 8 dots
	//

	fadeOut();

	REG_SCFG_EXT &= ~(1UL << 31); // Lock SCFG

	nocashMessage("Starting the NDS file...");
	startBinary_ARM7(arm9StartAddress);

	return 0;
}
