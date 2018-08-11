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
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>
#include <nds/memory.h> // tNDSHeader
#include <nds/arm7/i2c.h>
#include <nds/debug.h>

#include "fat_alt.h"
//#include "dldi_patcher.h"
#include "patch.h"
#include "find.h"
#include "hook.h"
#include "common.h"
#include "locations.h"

#include "cardengine_arm7_bin.h"
#include "cardengine_arm9_bin.h"

//#define memcpy __builtin_memcpy

extern void arm7clearRAM(void);

const char* bootName = "BOOT.NDS";

//extern unsigned long _start;
extern unsigned long storedFileCluster;
extern unsigned long initDisc;
//extern unsigned long wantToPatchDLDI;
//extern unsigned long argStart;
//extern unsigned long argSize;
//extern unsigned long dsiSD;
extern unsigned long saveFileCluster;
extern unsigned long saveSize;
extern unsigned long language;
extern unsigned long dsiMode; // SDK 5
extern unsigned long donorSdkVer;
extern unsigned long patchMpuRegion;
extern unsigned long patchMpuSize;
extern unsigned long consoleModel;
extern unsigned long loadingScreen;
extern unsigned long romread_LED;
//extern unsigned long gameSoftReset;
extern unsigned long asyncPrefetch;
//extern unsigned long logging;

bool dsiModeConfirmed = false; // SDK 5

u32 ROMinRAM = false;
u32 ROM_TID;
u32 ROM_HEADERCRC;
u32 ARM9_LEN;
u32 ARM7_LEN; // SDK 5
u32 fatSize;
u32 romSize;
u32 romSizeNoArm9;

static aFile* romFile = (aFile*)0x37D5000;
static aFile* savFile = (aFile*)0x37D5000 + 1;
static module_params_t* moduleParams = NULL;
static vu32* tempArm9StartAddress = (vu32*)TEMP_ARM9_START_ADDRESS_LOCATION;
static char* romLocation = (char*)ROM_LOCATION;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Used for debugging purposes
static void errorOutput(void) {
	if (loadingScreen > 0) {
		// Wait until the ARM9 is ready
		while (arm9_stateFlag != ARM9_READY);
		// Set the error code, then tell ARM9 to display it
		arm9_errorColor = true;
	}
	// Stop
	while (1);
}

static void debugOutput(void) {
	if (loadingScreen > 0) {
		// Wait until the ARM9 is ready
		while (arm9_stateFlag != ARM9_READY);
		// Set the error code, then tell ARM9 to display it
		arm9_screenMode = loadingScreen - 1;
		arm9_stateFlag = ARM9_DISPERR;
		// Wait for completion
		while (arm9_stateFlag != ARM9_READY);
	}
}

static void increaseLoadBarLength(void) {
	arm9_loadBarLength++;
	if (loadingScreen == 1) {
		debugOutput(); // Let the loading bar finish before ROM starts
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ 0x03

void boot_readFirmware(u32 address, u8* buffer, u32 size) {
	u32 index;

	// Read command
	while (REG_SPICNT & SPI_BUSY);
	REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
	REG_SPIDATA = FW_READ;
	while (REG_SPICNT & SPI_BUSY);

	// Set the address
	REG_SPIDATA = (address >> 16) & 0xFF;
	while (REG_SPICNT & SPI_BUSY);
	REG_SPIDATA = (address >> 8) & 0xFF;
	while (REG_SPICNT & SPI_BUSY);
	REG_SPIDATA = (address) & 0xFF;
	while (REG_SPICNT & SPI_BUSY);

	for (index = 0; index < size; index++) {
		REG_SPIDATA = 0;
		while (REG_SPICNT & SPI_BUSY);
		buffer[index] = REG_SPIDATA & 0xFF;
	}
	REG_SPICNT = 0;
}

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
void resetMemory_ARM7(void) {
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

void reloadFirmwareSettings(void) {
	u8 settings1, settings2;
	u32 settingsOffset = 0;

	// Get settings location
	boot_readFirmware((u32)0x00020, (u8*)&settingsOffset, 0x2);
	settingsOffset *= 8;

	// Reload DS Firmware settings
	boot_readFirmware(settingsOffset + 0x070, &settings1, 0x1);
	boot_readFirmware(settingsOffset + 0x170, &settings2, 0x1);

	if ((settings1 & 0x7F) == ((settings2 + 1) & 0x7F)) {
		boot_readFirmware(settingsOffset + 0x000, (u8*)((u32)ndsHeader - 0x180), 0x70);
	} else {
		boot_readFirmware(settingsOffset + 0x100, (u8*)((u32)ndsHeader - 0x180), 0x70);
	}
	if (language >= 0 && language < 6) {
		// Change language
		*(u8*)((u32)ndsHeader - 0x11C) = language;
	}
}

// The following 3 functions are not in devkitARM r47
//---------------------------------------------------------------------------------
u32 readTSCReg(u32 reg) {
//---------------------------------------------------------------------------------
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = ((reg << 1) | 1) & 0xFF;
 
	while (REG_SPICNT & 0x80);
 
	REG_SPIDATA = 0;
 
	while (REG_SPICNT & 0x80);

	REG_SPICNT = 0;

	return REG_SPIDATA;
}

//---------------------------------------------------------------------------------
void readTSCRegArray(u32 reg, void *buffer, int size) {
//---------------------------------------------------------------------------------
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = ((reg << 1) | 1) & 0xFF;

	char *buf = (char*)buffer;
	while (REG_SPICNT & 0x80);
	int count = 0;
	while (count < size) {
		REG_SPIDATA = 0;
 
		while (REG_SPICNT & 0x80);

		buf[count++] = REG_SPIDATA;
	}
	REG_SPICNT = 0;
}

//---------------------------------------------------------------------------------
u32 writeTSCReg(u32 reg, u32 value) {
//---------------------------------------------------------------------------------
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = (reg << 1) & 0xFF;
 
	while (REG_SPICNT & 0x80);
 
	REG_SPIDATA = value;
 
	while (REG_SPICNT & 0x80);

	REG_SPICNT = 0;

	return REG_SPIDATA;
}

//---------------------------------------------------------------------------------
void NDSTouchscreenMode(void) {
//---------------------------------------------------------------------------------
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
	readTSCReg(0);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x3A, 0);
	readTSCReg(0x51);
	writeTSCReg(0, 3);				// bank3
	readTSCReg(2);
	writeTSCReg(0, 0);				// bank0
	readTSCReg(0x3F);
	writeTSCReg(0, 1);				// bank1
	readTSCReg(0x28);
	readTSCReg(0x2A);
	readTSCReg(0x2E);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x52, 0x80);
	writeTSCReg(0x40, 0xC);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x24, 0xFF);
	writeTSCReg(0x25, 0xFF);
	writeTSCReg(0x26, 0x7F);
	writeTSCReg(0x27, 0x7F);
	writeTSCReg(0x28, 0x4A);
	writeTSCReg(0x29, 0x4A);
	writeTSCReg(0x2A, 0x10);
	writeTSCReg(0x2B, 0x10);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x51, 0);
	writeTSCReg(0, 3);				// bank3
	readTSCReg(2);
	writeTSCReg(2, 0x98);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x23, 0);
	writeTSCReg(0x1F, 0x14);
	writeTSCReg(0x20, 0x14);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x3F, 0);
	readTSCReg(0x0B);
	writeTSCReg(0x5, 0);
	writeTSCReg(0xB, 0x1);
	writeTSCReg(0xC, 0x2);
	writeTSCReg(0x12, 0x1);
	writeTSCReg(0x13, 0x2);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x2E, 0);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x3A, 0x60);
	writeTSCReg(0x01, 0x01);
	writeTSCReg(0x39, 0x66);
	writeTSCReg(0, 1);				// bank1
	readTSCReg(0x20);
	writeTSCReg(0x20, 0x10);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x04, 0);
	writeTSCReg(0x12, 0x81);
	writeTSCReg(0x13, 0x82);
	writeTSCReg(0x51, 0x82);
	writeTSCReg(0x51, 0x00);
	writeTSCReg(0x04, 0x03);
	writeTSCReg(0x05, 0xA1);
	writeTSCReg(0x06, 0x15);
	writeTSCReg(0x0B, 0x87);
	writeTSCReg(0x0C, 0x83);
	writeTSCReg(0x12, 0x87);
	writeTSCReg(0x13, 0x83);
	writeTSCReg(0, 3);				// bank3
	readTSCReg(0x10);
	writeTSCReg(0x10, 0x08);
	writeTSCReg(0, 4);				// bank4
	writeTSCReg(0x08, 0x7F);
	writeTSCReg(0x09, 0xE1);
	writeTSCReg(0xA, 0x80);
	writeTSCReg(0xB, 0x1F);
	writeTSCReg(0xC, 0x7F);
	writeTSCReg(0xD, 0xC1);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x41, 0x08);
	writeTSCReg(0x42, 0x08);
	writeTSCReg(0x3A, 0x00);
	writeTSCReg(0, 4);				// bank4
	writeTSCReg(0x08, 0x7F);
	writeTSCReg(0x09, 0xE1);
	writeTSCReg(0xA, 0x80);
	writeTSCReg(0xB, 0x1F);
	writeTSCReg(0xC, 0x7F);
	writeTSCReg(0xD, 0xC1);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x2F, 0x2B);
	writeTSCReg(0x30, 0x40);
	writeTSCReg(0x31, 0x40);
	writeTSCReg(0x32, 0x60);
	writeTSCReg(0, 0);				// bank0
	readTSCReg(0x74);
	writeTSCReg(0x74, 0x02);
	readTSCReg(0x74);
	writeTSCReg(0x74, 0x10);
	readTSCReg(0x74);
	writeTSCReg(0x74, 0x40);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x21, 0x20);
	writeTSCReg(0x22, 0xF0);
	writeTSCReg(0, 0);				// bank0
	readTSCReg(0x51);
	readTSCReg(0x3F);
	writeTSCReg(0x3F, 0xD4);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x23, 0x44);
	writeTSCReg(0x1F, 0xD4);
	writeTSCReg(0x28, 0x4E);
	writeTSCReg(0x29, 0x4E);
	writeTSCReg(0x24, 0x9E);
	writeTSCReg(0x24, 0x9E);
	writeTSCReg(0x20, 0xD4);
	writeTSCReg(0x2A, 0x14);
	writeTSCReg(0x2B, 0x14);
	writeTSCReg(0x26, 0xA7);
	writeTSCReg(0x27, 0xA7);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x40, 0);
	writeTSCReg(0x3A, 0x60);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x26, volLevel);
	writeTSCReg(0x27, volLevel);
	writeTSCReg(0x2E, 0x03);
	writeTSCReg(0, 3);				// bank3
	writeTSCReg(3, 0);
	writeTSCReg(0, 1);				// bank1
	writeTSCReg(0x21, 0x20);
	writeTSCReg(0x22, 0xF0);
	readTSCReg(0x22);
	writeTSCReg(0x22, 0xF0);
	writeTSCReg(0, 0);				// bank0
	writeTSCReg(0x52, 0x80);
	writeTSCReg(0x51, 0x00);
	writeTSCReg(0, 3);				// bank3
	readTSCReg(0x02);
	writeTSCReg(2, 0x98);
	writeTSCReg(0, 0xFF);			// bankFF
	writeTSCReg(5, 0);
	
	// Power management
	writePowerManagement(0x80, 0x00);
	writePowerManagement(0x00, 0x0D);
	//*(unsigned char*)0x40001C2 = 0x80, 0x00; // read PWR[0]   ;<-- also part of TSC !
	//*(unsigned char*)0x40001C2 = 0x00, 0x0D; // PWR[0]=0Dh    ;<-- also part of TSC !
}

module_params_t* buildModuleParams() {
	//u32* moduleParamsOffset = malloc(sizeof(module_params_t));
	u32* moduleParamsOffset = malloc(0x100);

	//memset(moduleParamsOffset, 0, sizeof(module_params_t));
	memset(moduleParamsOffset, 0, 0x100);

	module_params_t* moduleParams = (module_params_t*)(moduleParamsOffset - 7);

	moduleParams->compressed_static_end = 0;
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

module_params_t* getModuleParams(const void* arm9binary) {
	nocashMessage("Looking for moduleparams...\n");

	u32* moduleParamsOffset = findModuleParamsOffset((u32*)arm9binary, ARM9_LEN);

	//module_params_t* moduleParams = (module_params_t*)((u32)moduleParamsOffset - 0x1C);
	return moduleParamsOffset ? (module_params_t*)(moduleParamsOffset - 7) : NULL;
}

static inline void decompressBinary(const tNDSHeader* ndsHeader, void* arm9binary) {
	u32 ROM_TID = *(u32*)ndsHeader->gameCode;

	// Chrono Trigger (Japan)
	if (ROM_TID == 0x4a555159) {
		decompressLZ77Backwards((u8*)arm9binary, ARM9_LEN);
	}

	// Chrono Trigger (USA/Europe)
	if (ROM_TID == 0x45555159 || ROM_TID == 0x50555159) {
		decompressLZ77Backwards((u8*)arm9binary, ARM9_LEN);
	}
}

static inline void patchBinary(const tNDSHeader* ndsHeader) {
	u32 ROM_TID = *(u32*)ndsHeader->gameCode;

	// The World Ends With You (USA) (Europe)
	if (ROM_TID == 0x454C5741 || ROM_TID == 0x504C5741) {
		*(u32*)0x203E7B0 = 0;
	}

	// Subarashiki Kono Sekai - It's a Wonderful World (Japan)
	if (ROM_TID == 0x4A4C5741) {
		*(u32*)0x203F114 = 0;
	}

	// Miami Nights - Singles in the City (USA)
	if (ROM_TID == 0x45575641) {
		// Fix not enough memory error
		*(u32*)0x0204CCCC = 0xe1a00000; //nop
	}

	// Miami Nights - Singles in the City (Europe)
	if (ROM_TID == 0x50575641) {
		// Fix not enough memory error
		*(u32*)0x0204CDBC = 0xe1a00000; //nop
	}
	
	// 0735 - Castlevania - Portrait of Ruin (USA)
	if (ROM_TID == 0x45424341) {
		*(u32*)0x02007910 = 0xeb02508e;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025052;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0676 - Akumajou Dracula - Gallery of Labyrinth (Japan)
	if (ROM_TID == 0x4a424341) {
		*(u32*)0x02007910 = 0xeb0250b0;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025074;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0881 - Castlevania - Portrait of Ruin (Europe) (En,Fr,De,Es,It)
	if (ROM_TID == 0x50424341) {
		*(u32*)0x02007b00 = 0xeb025370;
		*(u32*)0x02007b08 = 0xea000004;
		*(u32*)0x02007bf0 = 0xeb025334;
		*(u32*)0x02007bf8 = 0xe59f1030;
		*(u32*)0x02007bfc = 0xe59f0028;
		*(u32*)0x02007c00 = 0xe0281097;
		*(u32*)0x02007c04 = 0xea000003;
	}

	// Chrono Trigger (Japan)
	if (ROM_TID == 0x4a555159) {
		*(u32*)0x0204e364 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e368 = 0xe12fff1e; //bx lr
		*(u32*)0x0204e6c4 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e6c8 = 0xe12fff1e; //bx lr
	}

	// Chrono Trigger (USA/Europe)
	if (ROM_TID == 0x45555159 || ROM_TID == 0x50555159) {
		*(u32*)0x0204e334 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e338 = 0xe12fff1e; //bx lr
		*(u32*)0x0204e694 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e698 = 0xe12fff1e; //bx lr
	}
	
	// Dementium II (USA/EUR)
	if (ROM_TID == 0x45454442 || ROM_TID == 0x50454442) {
		*(u32*)0x020e9120 = 0xe3a00002;
		*(u32*)0x020e9124 = 0xea000029;
	}
	
	// Dementium II: Tozasareta Byoutou (JPN)
	if (ROM_TID == 0x4a454442) {
		*(u32*)0x020d9f60 = 0xe3a00005;
		*(u32*)0x020d9f68 = 0xea000029;
	}

	// Grand Theft Auto - Chinatown Wars (USA) (En,Fr,De,Es,It)
	// Grand Theft Auto - Chinatown Wars (Europe) (En,Fr,De,Es,It)
	if (ROM_TID == 0x45584759 || ROM_TID == 0x50584759) {
		*(u16*)0x02037a34 = 0x46c0;
		*(u32*)0x0216ac0c = 0x0001fffb;
	}

	// WarioWare: DIY (USA)
	if (ROM_TID == 0x45524F55) {
		*(u32*)0x02003114 = 0xE12FFF1E; //mov r0, #0
	}
}

void loadBinary_ARM7(aFile file) {
	nocashMessage("loadBinary_ARM7");

	//u32 ndsHeader[0x170 >> 2];
	//u32 dsiHeader[0x2F0 >> 2]; // SDK 5
	tDSiHeader dsiHeaderTemp;

	// Read DSi header (including NDS header)
	//fileRead((char*)ndsHeader, file, 0, 0x170, 3);
	//fileRead((char*)dsiHeader, file, 0, 0x2F0, 2); // SDK 5
	fileRead((char*)&dsiHeaderTemp, file, 0, sizeof(dsiHeaderTemp), 3);

	// Read ARM9 info from NDS header
	u32 ARM9_SRC   = dsiHeaderTemp.ndshdr.arm9romOffset;
	char* ARM9_DST = (char*)dsiHeaderTemp.ndshdr.arm9destination;
	ARM9_LEN       = dsiHeaderTemp.ndshdr.arm9binarySize;

	// Read ARM7 info from NDS header
	u32 ARM7_SRC   = dsiHeaderTemp.ndshdr.arm7romOffset;
	char* ARM7_DST = (char*)dsiHeaderTemp.ndshdr.arm7destination;
	ARM7_LEN       = dsiHeaderTemp.ndshdr.arm7binarySize;

	ROM_TID       = *(u32*)dsiHeaderTemp.ndshdr.gameCode;
	fatSize       = dsiHeaderTemp.ndshdr.fatSize;
	romSize       = dsiHeaderTemp.ndshdr.romSize;
	romSizeNoArm9 = romSize - 0x4000 - ARM9_LEN;
	ROM_HEADERCRC = dsiHeaderTemp.ndshdr.headerCRC16;

	// Fix Pokemon games needing header data.
	//fileRead((char*)0x027FF000, file, 0, 0x170, 3);
	//memcpy((void*)0x027FF000, &dsiHeaderTemp.ndshdr, sizeof(dsiHeaderTemp.ndshdr));
	*(tNDSHeader*)0x027FF000 = dsiHeaderTemp.ndshdr;

	if ((*(u32*)0x27FF00C & 0x00FFFFFF) == 0x414441 // Diamond
	|| (*(u32*)0x27FF00C & 0x00FFFFFF) == 0x415041  // Pearl
	|| (*(u32*)0x27FF00C & 0x00FFFFFF) == 0x555043  // Platinum
	|| (*(u32*)0x27FF00C & 0x00FFFFFF) == 0x4B5049  // HG
	|| (*(u32*)0x27FF00C & 0x00FFFFFF) == 0x475049) // SS
	{
		// Make the Pokemon game code ADAJ.
		*(u32*)0x27FF00C = 0x4A414441;
	}

	// Load binaries into memory
	fileRead(ARM9_DST, file, ARM9_SRC, ARM9_LEN, 3);
	fileRead(ARM7_DST, file, ARM7_SRC, ARM7_LEN, 3);

	// SDK 5
	//dsiModeConfirmed = (dsiMode && (dsiHeaderTemp[0x10 >> 2] & BIT(16+1));
	dsiModeConfirmed = (dsiMode && (dsiHeaderTemp.ndshdr.deviceSize & BIT(16+1)));
	if (dsiModeConfirmed) {
		u32 ARM9i_SRC   = (u32)dsiHeaderTemp.arm9iromOffset;
		char* ARM9i_DST = (char*)dsiHeaderTemp.arm9idestination;
		u32 ARM9i_LEN   = dsiHeaderTemp.arm9ibinarySize;
		u32 ARM7i_SRC   = (u32)dsiHeaderTemp.arm7iromOffset;
		char* ARM7i_DST = (char*)dsiHeaderTemp.arm7idestination;
		u32 ARM7i_LEN   = dsiHeaderTemp.arm7ibinarySize;

		if (ARM9i_LEN) {
			fileRead(ARM9i_DST, file, ARM9i_SRC, ARM9i_LEN, 3);
		}
		if (ARM7i_LEN) {
			fileRead(ARM7i_DST, file, ARM7i_SRC, ARM7i_LEN, 3);
		}
	}
	
	decompressBinary(&dsiHeaderTemp.ndshdr, ARM9_DST);
	patchBinary(&dsiHeaderTemp.ndshdr);
	
	//moduleParams = findModuleParams(ndsHeader, donorSdkVer);
	moduleParams = getModuleParams(ARM9_DST);
	if (moduleParams) {
		//*(vu32*)0x2800008 = ((u32)moduleParamsOffset - 0x8);
		//*(vu32*)0x2800008 = (vu32)(moduleParamsOffset - 2);
		*(vu32*)0x2800008 = (vu32)((u32*)moduleParams + 5); // (u32*)moduleParams + 7 - 2

		*(vu32*)0x280000C = moduleParams->compressed_static_end; // from 'ensureArm9Decompressed'
		//ensureArm9Decompressed(ndsHeader, moduleParams);
		ensureArm9Decompressed(ARM9_DST, ARM9_LEN, moduleParams);
	} else {
		nocashMessage("No moduleparams?\n");
		*(vu32*)0x2800010 = 1;
		moduleParams = buildModuleParams();
	}

	sdk5 = (moduleParams->sdk_version > 0x5000000);
	if (sdk5) {
		ndsHeader            = (tNDSHeader*)NDS_HEADER_SDK5;
		tempArm9StartAddress = (vu32*)TEMP_ARM9_START_ADDRESS_SDK5_LOCATION;
		romLocation          = (char*)ROM_SDK5_LOCATION;
	}

	if ((sdk5 && consoleModel > 0 && romSizeNoArm9 <= 0x01000000)
	|| (!sdk5 && consoleModel > 0 && romSizeNoArm9 <= 0x017FC000)
	|| (!sdk5 && consoleModel == 0 && romSizeNoArm9 <= 0x007FC000))
	{
		// Set to load ROM into RAM
		ROMinRAM = true;
	}

	// First copy the header to its proper location, excluding
	// the ARM9 start address, so as not to start it
	
	// Store for later
	*tempArm9StartAddress = (vu32)dsiHeaderTemp.ndshdr.arm9executeAddress;
	
	dsiHeaderTemp.ndshdr.arm9executeAddress = 0;
	
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, (void*)ndsHeader, 0x170);
	if (dsiModeConfirmed) {
		//dmaCopyWords(3, &dsiHeaderTemp, ndsHeader, sizeof(dsiHeaderTemp));
		*(tDSiHeader*)ndsHeader = dsiHeaderTemp;
	} else {
		//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, ndsHeader, sizeof(dsiHeaderTemp.ndshdr));
		*ndsHeader = dsiHeaderTemp.ndshdr;
	}

	if (!dsiModeConfirmed) {
		// Switch to NTR mode BIOS (no effect with locked arm7 SCFG)
		nocashMessage("Switch to NTR mode BIOS");
		REG_SCFG_ROM = 0x703;
	}
}

u32 enableExceptionHandler = true;

void setArm9Stuff(const tNDSHeader* ndsHeader, aFile file) {
	u32 ROM_TID = *(u32*)ndsHeader->gameCode;

	// ExceptionHandler2 (red screen) blacklist
	if ((ROM_TID & 0x00FFFFFF) == 0x4D5341	// SM64DS
	|| (ROM_TID & 0x00FFFFFF) == 0x534D53	// SMSW
	|| (ROM_TID & 0x00FFFFFF) == 0x443241	// NSMB
	|| (ROM_TID & 0x00FFFFFF) == 0x4D4441)	// AC:WW
	{
		enableExceptionHandler = false;
	}

	if (ROMinRAM == true) {
		// Load ROM into RAM
		fileRead(romLocation, file, 0x4000 + ARM9_LEN, romSizeNoArm9, 0);

		// Primary fix for Mario's Holiday
		if (*(u32*)((romLocation - 0x4000 - ARM9_LEN) + 0x003128AC) == 0x4B434148){
			*(u32*)((romLocation - 0x4000 - ARM9_LEN) + 0x003128AC) = 0xA00;
		}
	}

	hookNdsRetailArm9((cardengineArm9*)CARDENGINE_ARM9_LOCATION);
}

/*-------------------------------------------------------------------------
startBinary_ARM7
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
void startBinary_ARM7(void) {
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


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void initMBK(void) {
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

int arm7_main(void) {
	nocashMessage("bootloader");

	initMBK();
    
    // Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	nocashMessage("Getting ARM7 to clear RAM...\n");
	debugOutput(); // 1 dot
	resetMemory_ARM7();

	// Init card
	if (!FAT_InitFiles(initDisc, 3)) {
		nocashMessage("!FAT_InitFiles");
		return -1;
	}

	*romFile = getFileFromCluster(storedFileCluster);

	// Invalid file cluster specified
	if ((romFile->firstCluster < CLUSTER_FIRST) || (romFile->firstCluster >= CLUSTER_EOF)) {
		*romFile = getBootFileCluster(bootName, 3);
	}

	if (romFile->firstCluster == CLUSTER_FREE) {
		nocashMessage("fileCluster == CLUSTER_FREE");
		return -1;
	}
    
    buildFatTableCache(romFile, 3);
    
    *savFile = getFileFromCluster(saveFileCluster);
    
    if (savFile->firstCluster != CLUSTER_FREE) {
        buildFatTableCache(savFile, 3);
	}

	int errorCode;

	if (REG_SCFG_EXT == 0 || consoleModel < 2) {
		NDSTouchscreenMode();
		*(u16*)0x4000500 = 0x807F;
	}

	// Load the NDS file
	nocashMessage("Loading the NDS file...\n");
	loadBinary_ARM7(*romFile);
	increaseLoadBarLength(); // 2 dots

	// Reload DS Firmware settings
	nocashMessage("Reloading DS Firmware settings...\n");
	reloadFirmwareSettings(); // After 'sdk5' is set
	increaseLoadBarLength(); // 3 dots

	nocashMessage("Trying to patch the card...\n");

	memcpy((u32*)CARDENGINE_ARM7_LOCATION, (u32*)cardengine_arm7_bin, cardengine_arm7_bin_size);
	increaseLoadBarLength(); // 4 dots

	memcpy((u32*)CARDENGINE_ARM9_LOCATION, (u32*)cardengine_arm9_bin, cardengine_arm9_bin_size);
	increaseLoadBarLength(); // 5 dots

	errorCode = patchCardNds(ndsHeader, (cardengineArm7*)CARDENGINE_ARM7_LOCATION, (cardengineArm9*)CARDENGINE_ARM9_LOCATION, moduleParams, saveFileCluster, saveSize, patchMpuRegion, patchMpuSize);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card patch successful");
	} else {
		nocashMessage("Card patch failed");
		errorOutput();
	}
	increaseLoadBarLength(); // 6 dots

	errorCode = hookNdsRetailArm7(ndsHeader, *romFile, (cardengineArm7*)CARDENGINE_ARM7_LOCATION);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}
	increaseLoadBarLength(); // 7 dots

	setArm9Stuff(ndsHeader, *romFile);
	if (ROMinRAM == false) {
		if (romread_LED == 1 || (romread_LED > 0 && asyncPrefetch == 1)) {
			// Turn WiFi LED off
			i2cWriteRegister(0x4A, 0x30, 0x12);
		}
	}
	increaseLoadBarLength(); // Final 8 dots

	fadeType = false;
	while (screenBrightness != 31);	// Wait for screen to fade out

    // Lock SCFG
    REG_SCFG_EXT &= ~(1UL << 31);

	nocashMessage("Starting the NDS file...");
	startBinary_ARM7();

	return 0;
}
