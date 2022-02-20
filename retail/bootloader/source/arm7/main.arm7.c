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
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>
#include <nds/memory.h> // tNDSHeader
#include <nds/arm7/i2c.h>
#include <nds/debug.h>
#include <nds/ipc.h>

#include <nds/arm9/dldi.h>

#define REG_GPIO_WIFI *(vu16*)0x4004C04

#include "tonccpy.h"
#include "my_fat.h"
#include "debug_file.h"
#include "nds_header.h"
#include "module_params.h"
#include "decompress.h"
#include "dldi_patcher.h"
#include "ips.h"
#include "patch.h"
#include "find.h"
#include "cheat_patch.h"
#include "hook.h"
#include "common.h"
#include "locations.h"
#include "value_bits.h"
#include "loading_screen.h"

typedef signed int addr_t;
typedef unsigned char data_t;

//#define memcpy __builtin_memcpy

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

extern void arm7clearRAM(void);

//extern u32 _start;
extern u32 storedFileCluster;
extern u32 romSize;
extern u32 initDisc;
extern u32 saveFileCluster;
extern u32 saveSize;
extern u32 pageFileCluster;
extern u32 apPatchFileCluster;
extern u32 apPatchSize;
extern u32 cheatFileCluster;
extern u32 cheatSize;
extern u32 patchOffsetCacheFileCluster;
extern u32 ramDumpCluster;
extern u32 srParamsFileCluster;
extern u32 patchMpuSize;
extern u8 patchMpuRegion;
extern u8 language;
extern u8 region;
extern u8 donorSdkVer;
extern u8 soundFreq;

extern u32 _io_dldi_features;

u32 arm9iromOffset = 0;
u32 arm9ibinarySize = 0;
u32 arm7iromOffset = 0;

static u32 ce9Location = 0;
static u32 overlaysSize = 0;
static u32 ioverlaysSize = 0;

static u32 softResetParams[4] = {0};
u32 srlAddr = 0;

u16 s2FlashcardId = 0;

bool expansionPakFound = false;

u32 newArm7binarySize = 0;
u32 arm7mbk = 0;

static void initMBK(void) {
	// arm7 is master of WRAM-A, arm9 of WRAM-B & C
	REG_MBK9=0x3000000F;

	// WRAM-A fully mapped to arm7
	*((vu32*)REG_MBK1)=0x8D898581; // same as dsiware

	// WRAM-B fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK2)=0x8C888480;
	*((vu32*)REG_MBK3)=0x9C989490;

	// WRAM-C fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK4)=0x8C888480;
	*((vu32*)REG_MBK5)=0x9C989490;

	// WRAM mapped to the 0x3700000 - 0x37FFFFF area 
	// WRAM-A mapped to the 0x3000000 - 0x303FFFF area : 256k
	REG_MBK6=0x00403000;
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
}

void memset_addrs_arm7(u32 start, u32 end)
{
	toncset((u32*)start, 0, ((int)end - (int)start));
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

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	memset_addrs_arm7(0x03800000 - 0x8000, 0x03800000 + 0x10000);
	memset_addrs_arm7(0x02004000, IMAGES_LOCATION);	// clear part of EWRAM - except before nds-bootstrap images
	toncset((u32*)0x02380000, 0, 0x60000);		// clear part of EWRAM - except before 0x023DA000, which has the arm9 code
	toncset((u32*)0x023F0000, 0, 0xB000);
	toncset((u32*)0x023FF000, 0, 0x1000);
	if (extendedMemory2) {
		toncset((u32*)0x02400000, 0, 0x3FC000);
		toncset((u32*)0x027FF000, 0, dsDebugRam ? 0x1000 : 0x801000);
	}

	REG_IE = 0;
	REG_IF = ~0;
	*(vu32*)(0x04000000 - 4) = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)(0x04000000 - 8) = ~0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff
}

static void NDSTouchscreenMode(void) {
	u8 volLevel;
	
	// 0xAC: special setting (when found special gamecode)
	// 0xA7: normal setting (for any other gamecodes)
	volLevel = volumeFix ? 0xAC : 0xA7;

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
	cdcWriteReg(CDC_SOUND, 0x25, 0x9E);
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
	cdcWriteReg(CDC_SOUND, 0x22, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	
	// Set remaining values
	cdcWriteReg(CDC_CONTROL, 0x03, 0x44);
	cdcWriteReg(CDC_CONTROL, 0x0D, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x0E, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x0F, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x10, 0x08);
	cdcWriteReg(CDC_CONTROL, 0x14, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x15, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x16, 0x04);
	cdcWriteReg(CDC_CONTROL, 0x1A, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x1E, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x24, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x33, 0x34);
	cdcWriteReg(CDC_CONTROL, 0x34, 0x32);
	cdcWriteReg(CDC_CONTROL, 0x35, 0x12);
	cdcWriteReg(CDC_CONTROL, 0x36, 0x03);
	cdcWriteReg(CDC_CONTROL, 0x37, 0x02);
	cdcWriteReg(CDC_CONTROL, 0x38, 0x03);
	cdcWriteReg(CDC_CONTROL, 0x3C, 0x19);
	cdcWriteReg(CDC_CONTROL, 0x3D, 0x05);
	cdcWriteReg(CDC_CONTROL, 0x44, 0x0F);
	cdcWriteReg(CDC_CONTROL, 0x45, 0x38);
	cdcWriteReg(CDC_CONTROL, 0x49, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x4A, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x4B, 0xEE);
	cdcWriteReg(CDC_CONTROL, 0x4C, 0x10);
	cdcWriteReg(CDC_CONTROL, 0x4D, 0xD8);
	cdcWriteReg(CDC_CONTROL, 0x4E, 0x7E);
	cdcWriteReg(CDC_CONTROL, 0x4F, 0xE3);
	cdcWriteReg(CDC_CONTROL, 0x58, 0x7F);
	cdcWriteReg(CDC_CONTROL, 0x74, 0xD2);
	cdcWriteReg(CDC_CONTROL, 0x75, 0x2C);
	cdcWriteReg(CDC_SOUND, 0x22, 0x70);
	cdcWriteReg(CDC_SOUND, 0x2C, 0x20);

	// Finish up!
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(0xFF, 0x05, 0x00); //writeTSC(0x00, 0xFF);

	// Power management
	writePowerManagement(PM_READ_REGISTER, 0x00); //*(unsigned char*)0x40001C2 = 0x80, 0x00; // read PWR[0]   ;<-- also part of TSC !
	writePowerManagement(PM_CONTROL_REG, 0x0D); //*(unsigned char*)0x40001C2 = 0x00, 0x0D; // PWR[0]=0Dh    ;<-- also part of TSC !
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

/*static inline u32 getRomSizeNoArmBins(const tNDSHeader* ndsHeader) {
	return ndsHeader->romSize - ndsHeader->arm7romOffset - ndsHeader->arm7binarySize + overlaysSize;
}*/

// SDK 5
static bool ROMsupportsDsiMode(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode > 0);
}

static void loadBinary_ARM7(const tDSiHeader* dsiHeaderTemp, aFile file) {
	nocashMessage("loadBinary_ARM7");

	//u32 ndsHeader[0x170 >> 2];
	//u32 dsiHeader[0x2F0 >> 2]; // SDK 5

	// Read DSi header (including NDS header)
	//fileRead((char*)ndsHeader, file, 0, 0x170, 3);
	//fileRead((char*)dsiHeader, file, 0, 0x2F0, 2); // SDK 5
	srlAddr = softResetParams[3];
	fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp));
	if (srlAddr > 0 && ((u32)dsiHeaderTemp->ndshdr.arm9destination < 0x02000000 || (u32)dsiHeaderTemp->ndshdr.arm9destination > 0x02004000)) {
		// Invalid SRL
		srlAddr = 0;
		fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp));
	}

	fileRead(&arm7mbk, file, srlAddr+0x1A0, sizeof(u32));

	char baseTid[5] = {0};
	fileRead((char*)&baseTid, file, 0xC, 4);
	if (
		strncmp(baseTid, "ADA", 3) == 0    // Diamond
		|| strncmp(baseTid, "APA", 3) == 0 // Pearl
		|| strncmp(baseTid, "CPU", 3) == 0 // Platinum
		|| strncmp(baseTid, "IPK", 3) == 0 // HG
		|| strncmp(baseTid, "IPG", 3) == 0 // SS
	) {
		// Fix Pokemon games needing header data.
		tNDSHeader* ndsHeaderPokemon = (tNDSHeader*)NDS_HEADER_POKEMON;
		*ndsHeaderPokemon = dsiHeaderTemp->ndshdr;

		// Make the Pokemon game code ADAJ.
		const char gameCodePokemon[] = { 'A', 'D', 'A', 'J' };
		tonccpy(ndsHeaderPokemon->gameCode, gameCodePokemon, 4);
	}

	extern u32 donorFileTwlCluster;	// SDK5 (TWL)

	// Load binaries into memory
	fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize);
	if (arm7mbk != 0x080037C0 || (arm7mbk == 0x080037C0 && donorFileTwlCluster == 0)) {
		fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize);
	}
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

u32 romLocation = 0x09000000;
u32 romSizeLimit = 0x780000;
u32 ROMinRAM = 0;

static bool isROMLoadableInRAM(const tNDSHeader* ndsHeader, const char* romTid, const module_params_t* moduleParams) {
	bool isDevConsole = false;
	if (s2FlashcardId == 0x334D || s2FlashcardId == 0x3647 || s2FlashcardId == 0x4353 || s2FlashcardId == 0x5A45) {
		romLocation = 0x08000000;
		romSizeLimit = (s2FlashcardId == 0x5A45) ? 0xF80000 : 0x1F80000;
	}
	if (extendedMemory2 && !dsDebugRam) {
		*(vu32*)(0x0DFFFE0C) = 0x4253444E;		// Check for 32MB of RAM
		isDevConsole = (*(vu32*)(0x0DFFFE0C) == 0x4253444E);

		romLocation = 0x0C800000;
		if (isSdk5(moduleParams)) {
			if (isDevConsole) {
				romLocation = 0x0D000000;
			}
			romSizeLimit = isDevConsole ? 0x1000000 : 0x700000;
		} else {
			romSizeLimit = isDevConsole ? 0x1800000 : 0x800000;
		}
	}

	bool res = false;
	if ((strncmp(romTid, "UBR", 3) == 0 && isDevConsole)
	 || (/*strncmp(romTid, "AMC", 3) != 0
	 && strncmp(romTid, "A8T", 3) != 0
	 &&*/ strncmp(romTid, "UOR", 3) != 0
	 && strncmp(romTid, "KPP", 3) != 0
	 && strncmp(romTid, "KPF", 3) != 0)
	) {
		res = ((expansionPakFound || (extendedMemory2 && !dsDebugRam)) && (ndsHeader->unitCode == 3 ? (arm9iromOffset-0x8000)+ioverlaysSize : (ndsHeader->romSize-0x8000)+0x88) < romSizeLimit);
		if (res) {
			dbg_printf(expansionPakFound ? "ROM is loadable into Slot-2 RAM\n" : "ROM is loadable into RAM\n");
		}
	  }
	return res;
}

static tNDSHeader* loadHeader(tDSiHeader* dsiHeaderTemp, const module_params_t* moduleParams) {
	tNDSHeader* ndsHeader = (tNDSHeader*)(isSdk5(moduleParams) ? NDS_HEADER_SDK5 : NDS_HEADER);

	// Copy the header to its proper location
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, (char*)ndsHeader, 0x170);
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, ndsHeader, sizeof(dsiHeaderTemp.ndshdr));
	*ndsHeader = dsiHeaderTemp->ndshdr;

	return ndsHeader;
}

static void my_readUserSettings(tNDSHeader* ndsHeader) {
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

	tonccpy(PersonalData, currentSettings, sizeof(PERSONAL_DATA));

	if (language >= 0 && language <= 7) {
		// Change language
		personalData->language = language; //*(u8*)((u32)ndsHeader - 0x11C) = language;
		/*if (ROMsupportsDsiMode(ndsHeader) && ndsHeader->arm9destination >= 0x02000800) {
			*(u8*)0x02000406 = language;
		}*/
	}

	if (personalData->language != 6 && ndsHeader->reserved1[8] == 0x80) {
		ndsHeader->reserved1[8] = 0;	// Patch iQue game to be region-free
		ndsHeader->headerCRC16 = swiCRC16(0xFFFF, ndsHeader, 0x15E);	// Fix CRC
	}
}

static bool supportsExceptionHandler(const char* romTid) {
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
static void startBinary_ARM7(void) {
	// Get the ARM9 to boot
	arm9_stateFlag = ARM9_BOOTBIN;

	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM7
	VoidFn arm7code = (VoidFn)ndsHeader->arm7executeAddress;
	arm7code();
}

static void loadOverlaysintoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile file, u32 ROMinRAM) {
	// Load overlays into RAM
	if (overlaysSize < romSizeLimit)
	{
		u32 overlaysLocation = romLocation;
		if (ROMinRAM) {
			overlaysLocation += (ndsHeader->arm9binarySize-ndsHeader->arm9romOffset);
		}
		fileRead((char*)overlaysLocation, file, ndsHeader->arm9romOffset + ndsHeader->arm9binarySize, overlaysSize);

		if (!isSdk5(moduleParams) && *(u32*)((overlaysLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+0x003128AC) == 0x4B434148) {
			*(u32*)((overlaysLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday
		}
	}
}

static void setMemoryAddress(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile romFile) {
	if (ROMsupportsDsiMode(ndsHeader)) {
		/*if (ndsHeader->arm9destination >= 0x02000800) {
			// Construct TWLCFG
			u8* twlCfg = (u8*)0x02000400;
			u8* personalData = (u8*)0x02FFFC80;
			tonccpy(twlCfg+0x6, personalData+0x64, 1); // Selected Language (eg. 1=English)
			tonccpy(twlCfg+0x7, personalData+0x66, 1); // RTC Year (last date change) (max 63h=2099)
			tonccpy(twlCfg+0x8, personalData+0x68, 4); // RTC Offset (difference in seconds on change)
			tonccpy(twlCfg+0x1A, personalData+0x52, 1); // Alarm Hour   (0..17h)
			tonccpy(twlCfg+0x1B, personalData+0x53, 1); // Alarm Minute (0..3Bh)
			tonccpy(twlCfg+0x1E, personalData+0x56, 1); // Alarm Enable (0=Off, 1=On)
			toncset(twlCfg+0x24, 0x03, 1); // Unknown (02h or 03h)
			tonccpy(twlCfg+0x30, personalData+0x58, 0xC); // TSC calib
			toncset32(twlCfg+0x3C, 0x0201209C, 1);
			tonccpy(twlCfg+0x44, personalData+0x02, 1); // Favorite color (also Sysmenu Cursor Color)
			tonccpy(twlCfg+0x46, personalData+0x03, 2); // Birthday (month, day)
			tonccpy(twlCfg+0x48, personalData+0x06, 0x16); // Nickname (UCS-2), max 10 chars+EOL
			tonccpy(twlCfg+0x5E, personalData+0x1C, 0x36); // Message (UCS-2), max 26 chars+EOL
			readFirmware(0x1FD, twlCfg+0x1E0, 1); // WlFirm Type (1=DWM-W015, 2=W024, 3=W028)
			if (twlCfg[0x1E0] == 2 || twlCfg[0x1E0] == 3) {
				toncset32(twlCfg+0x1E4, 0x520000, 1); // WlFirm RAM vars
				toncset32(twlCfg+0x1E8, 0x520000, 1); // WlFirm RAM base
				toncset32(twlCfg+0x1EC, 0x020000, 1); // WlFirm RAM size
			} else {
				toncset32(twlCfg+0x1E4, 0x500400, 1); // WlFirm RAM vars
				toncset32(twlCfg+0x1E8, 0x500000, 1); // WlFirm RAM base
				toncset32(twlCfg+0x1EC, 0x02E000, 1); // WlFirm RAM size
			}
			*(u16*)(twlCfg+0x1E2) = swiCRC16(0xFFFF, twlCfg+0x1E4, 0xC); // WlFirm CRC16

			u32 configFlags = 0x0100000F;
			configFlags |= BIT(3);

			toncset32(twlCfg, configFlags, 1); // Config Flags
			fileRead(twlCfg+0x10, romFile, 0x20E, 1); // EULA Version (0=None/CountryChanged, 1=v1)
			fileRead(twlCfg+0x9C, romFile, 0x2F0, 1);  // Parental Controls Years of Age Rating (00h..14h)
		}*/

		// Set region flag
		if (useRomRegion || region == 0xFF) {
			u8 newRegion = 0;
			if (strncmp(getRomTid(ndsHeader)+3, "J", 1) == 0) {
				newRegion = 0;
			} else if (strncmp(getRomTid(ndsHeader)+3, "E", 1) == 0 || strncmp(getRomTid(ndsHeader)+3, "T", 1) == 0) {
				newRegion = 1;
			} else if (strncmp(getRomTid(ndsHeader)+3, "P", 1) == 0 || strncmp(getRomTid(ndsHeader)+3, "V", 1) == 0) {
				newRegion = 2;
			} else if (strncmp(getRomTid(ndsHeader)+3, "U", 1) == 0) {
				newRegion = 3;
			} else if (strncmp(getRomTid(ndsHeader)+3, "C", 1) == 0) {
				newRegion = 4;
			} else if (strncmp(getRomTid(ndsHeader)+3, "K", 1) == 0) {
				newRegion = 5;
			}
			toncset((u8*)0x02FFFD70, newRegion, 1);
		} else {
			toncset((u8*)0x02FFFD70, region, 1);
		}
		// Set bitmask for supported languages
		u8 curRegion = *(u8*)0x02FFFD70;
		if (curRegion == 1) {
			*(u32*)(0x02FFFD68) = 0x26;
		} else if (curRegion == 2 || curRegion == 3) {
			*(u32*)(0x02FFFD68) = 0x3E;
		} else if (curRegion == 4) {
			*(u32*)(0x02FFFD68) = 0x40; //CHN
		} else if (curRegion == 5) {
			*(u32*)(0x02FFFD68) = 0x80; //KOR
		} else if (curRegion == 0) {
			*(u32*)(0x02FFFD68) = 0x01; //JAP
		}
	}

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

	*((u16*)(isSdk5(moduleParams) ? 0x02fff850 : 0x027ff850)) = 0x5835;

	// Copies of above
	*((u32*)(isSdk5(moduleParams) ? 0x02fffc00 : 0x027ffc00)) = chipID;					// CurrentCardID
	*((u32*)(isSdk5(moduleParams) ? 0x02fffc04 : 0x027ffc04)) = chipID;					// Command10CardID
	*((u16*)(isSdk5(moduleParams) ? 0x02fffc08 : 0x027ffc08)) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
	*((u16*)(isSdk5(moduleParams) ? 0x02fffc0a : 0x027ffc0a)) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc10 : 0x027ffc10)) = 0x5835;

	if (softResetParams[0] != 0xFFFFFFFF) {
		u32* resetParamLoc = (u32*)(isSdk5(moduleParams) ? RESET_PARAM_SDK5 : RESET_PARAM);
		resetParamLoc[0] = softResetParams[0];
		resetParamLoc[1] = softResetParams[1];
		resetParamLoc[2] = softResetParams[2];
		resetParamLoc[3] = softResetParams[3];
	}

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 0x1;						// Boot Indicator (Booted from card for SDK5) -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr

	const char* romTid = getRomTid(ndsHeader);
	if (strncmp(romTid, "KPP", 3) == 0 	// Pop Island
	 || strncmp(romTid, "KPF", 3) == 0		// Pop Island: Paperfield
	 || strncmp(romTid, "KGK", 3) == 0)	// Glory Days: Tactical Defense
	{
		*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 0x2;					// Boot Indicator (Cloneboot/Multiboot)
	}
}

int arm7_main(void) {
	nocashMessage("bootloader");

	initMBK();
	
	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	nocashMessage("Getting ARM7 to clear RAM...\n");
	resetMemory_ARM7();

	arm9_macroMode = macroMode;

	// Init card
	if (!FAT_InitFiles(initDisc)) {
		nocashMessage("!FAT_InitFiles");
		errorOutput();
		//return -1;
	}

	if (logging) {
		enableDebug(getBootFileCluster("NDSBTSRP.LOG"));
	}

	if (_io_dldi_features & FEATURE_SLOT_NDS) {
		s2FlashcardId = *(u16*)(0x020000C0);
	}

	aFile srParamsFile = getFileFromCluster(srParamsFileCluster);
	fileRead((char*)&softResetParams, srParamsFile, 0, 0x10);
	bool softResetParamsFound = (softResetParams[0] != 0xFFFFFFFF);
	if (softResetParamsFound) {
		u32 clearBuffer = 0xFFFFFFFF;
		fileWrite((char*)&clearBuffer, srParamsFile, 0, 0x4);
		clearBuffer = 0;
		fileWrite((char*)&clearBuffer, srParamsFile, 0x4, 0x4);
		fileWrite((char*)&clearBuffer, srParamsFile, 0x8, 0x4);
		fileWrite((char*)&clearBuffer, srParamsFile, 0xC, 0x4);
	}

	// BOOT.NDS file
	//aFile bootNdsFile = getBootFileCluster(bootName, 0);

	// ROM file
	aFile romFile = getFileFromCluster(storedFileCluster);

	// Invalid file cluster specified
	/*if ((romFile.firstCluster < CLUSTER_FIRST) || (romFile.firstCluster >= CLUSTER_EOF)) {
		romFile = getBootFileCluster(bootName, 0);
	}

	if (romFile.firstCluster == CLUSTER_FREE) {
		nocashMessage("fileCluster == CLUSTER_FREE");
		errorOutput();
		//return -1;
	}*/

	//nocashMessage("status1");

	// Sav file
	aFile savFile = getFileFromCluster(saveFileCluster);
	
	int errorCode;

	tDSiHeader dsiHeaderTemp;

	// Load the NDS file
	nocashMessage("Loading the NDS file...\n");

	loadBinary_ARM7(&dsiHeaderTemp, romFile);
	
	// File containing cached patch offsets
	aFile patchOffsetCacheFile = getFileFromCluster(patchOffsetCacheFileCluster);
	if (srlAddr == 0) {
		fileRead((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	}
	u16 prevPatchOffsetCacheFileVersion = patchOffsetCache.ver;

	nocashMessage("Loading the header...\n");

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);

	if (dsiHeaderTemp.ndshdr.unitCode < 3 && dsiHeaderTemp.ndshdr.gameCode[0] != 'K' && dsiHeaderTemp.ndshdr.gameCode[0] != 'Z' && (softResetParams[0] == 0 || softResetParams[0] == 0xFFFFFFFF)) {
		esrbOutput();
	}

	ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams, foundModuleParams);
	if (decrypt_arm9(&dsiHeaderTemp.ndshdr)) {
		nocashMessage("Secure area decrypted successfully");
		dbg_printf("Secure area decrypted successfully");
	} else {
		nocashMessage("Secure area already decrypted");
		dbg_printf("Secure area already decrypted");
	}
	dbg_printf("\n");

	ndsHeader = loadHeader(&dsiHeaderTemp, moduleParams);

	my_readUserSettings(ndsHeader); // Header has to be loaded first

	if (cdcReadReg(CDC_SOUND, 0x22) == 0xF0) {
		// Switch touch mode to NTR
		*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
		NDSTouchscreenMode();
		*(u16*)0x4000500 = 0x807F;
	}

	REG_GPIO_WIFI |= BIT(8);	// Old NDS-Wifi mode

	// Switch to NTR mode BIOS
	REG_SCFG_ROM = 0x703;

	// Calculate overlay pack size
	for (u32 i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i < ndsHeader->arm7romOffset; i++) {
		overlaysSize++;
	}
	if (ndsHeader->unitCode == 3) {
		fileRead(&arm9iromOffset, romFile, 0x1C0, sizeof(u32));
		fileRead(&arm9ibinarySize, romFile, 0x1CC, sizeof(u32));
		fileRead(&arm7iromOffset, romFile, 0x1D0, sizeof(u32));

		// Calculate i-overlay pack size
		for (u32 i = arm9iromOffset+arm9ibinarySize; i < arm7iromOffset; i++) {
			ioverlaysSize++;
		}
	}

	const char* romTid = getRomTid(ndsHeader);

	if ((strcmp(romTid, "UBRP") == 0) && extendedMemory2 && !dsDebugRam) {
		toncset((char*)0x0C400000, 0xFF, 0xC0);
		toncset((u8*)0x0C4000B2, 0, 3);
		toncset((u8*)0x0C4000B5, 0x24, 3);
		*(u16*)0x0C4000BE = 0x7FFF;
		*(u16*)0x0C4000CE = 0x7FFF;
	}

	*(vu32*)(0x08240000) = 1;
	expansionPakFound = ((*(vu32*)(0x08240000) == 1) && (strcmp(romTid, "UBRP") != 0));

	// If possible, set to load ROM into RAM
	ROMinRAM = isROMLoadableInRAM(ndsHeader, romTid, moduleParams);

	nocashMessage("Trying to patch the card...\n");

	ce9Location = extendedMemory2 ? CARDENGINE_ARM9_LOCATION_DLDI_EXTMEM : CARDENGINE_ARM9_LOCATION_DLDI;
	tonccpy((u32*)ce9Location, (u32*)CARDENGINE_ARM9_LOCATION_BUFFERED, 0x7000);
	toncset((u32*)0x023E0000, 0, 0x10000);

	if (!dldiPatchBinary((data_t*)ce9Location, 0x7000)) {
		nocashMessage("ce9 DLDI patch failed");
		dbg_printf("ce9 DLDI patch failed");
		dbg_printf("\n");
		errorOutput();
	}

	patchBinary((cardengineArm9*)ce9Location, ndsHeader, moduleParams);
	errorCode = patchCardNds(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		(cardengineArm9*)ce9Location,
		ndsHeader,
		moduleParams,
		(ROMsupportsDsiMode(ndsHeader)
		|| strncmp(romTid, "AMU", 3) == 0 // Big Mutha Truckers
		|| strncmp(romTid, "ASK", 3) == 0 // Lost in Blue
		|| strncmp(romTid, "AKD", 3) == 0 // Trauma Center: Under the Knife
		) ? 0 : 1,
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
		cheatFileCluster,
		cheatSize,
		apPatchFileCluster,
		apPatchSize,
		language
	);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}

	u32 fatTableAddr = 0;
	u32 fatTableSize = 0;
	if (s2FlashcardId == 0x334D || s2FlashcardId == 0x3647 || s2FlashcardId == 0x4353) {
		fatTableAddr = 0x09F80000;
		fatTableSize = (s2FlashcardId==0x4353 ? 0x7FFFC : 0x80000);
	} else if (s2FlashcardId == 0x5A45) {
		fatTableAddr = 0x08F80000;
		fatTableSize = 0x80000;
	} else if (expansionPakFound) {
		fatTableAddr = 0x09780000;
		fatTableSize = 0x80000;
	} else if (extendedMemory2) {
		fatTableAddr = 0x02700000;
		fatTableSize = 0x80000;
	} else {
		fatTableAddr = (moduleParams->sdk_version < 0x2008000) ? 0x023E0000 : 0x023C0000;
		fatTableSize = 0x19000;
	}

	if (expansionPakFound || (extendedMemory2 && !dsDebugRam && strncmp(romTid, "UBRP", 4) != 0)) {
		loadOverlaysintoRAM(ndsHeader, moduleParams, romFile, ROMinRAM);
	}

	errorCode = hookNdsRetailArm9(
		(cardengineArm9*)ce9Location,
		moduleParams,
		romFile.firstCluster,
		savFile.firstCluster,
		ramDumpCluster,
		srParamsFileCluster,
		pageFileCluster,
		expansionPakFound,
		extendedMemory2,
		ROMinRAM,
		dsDebugRam,
		supportsExceptionHandler(romTid),
		overlaysSize,
		ioverlaysSize,
		fatTableSize,
		fatTableAddr
	);
	/*if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}*/

	if (srlAddr == 0 && (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheChanged)) {
		fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	}

	if (srlAddr == 0 && apPatchFileCluster != 0 && !apPatchIsCheat && apPatchSize > 0 && apPatchSize <= 0x40000) {
		aFile apPatchFile = getFileFromCluster(apPatchFileCluster);
		dbg_printf("AP-fix found\n");
		if (esrbScreenPrepared) {
			while (!esrbImageLoaded) {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			}
		}
		fileRead((char*)IMAGES_LOCATION, apPatchFile, 0, apPatchSize);
		if (applyIpsPatch(ndsHeader, (u8*)IMAGES_LOCATION, (*(u8*)(IMAGES_LOCATION+apPatchSize-1) == 0xA9), ROMinRAM)) {
			dbg_printf("AP-fix applied\n");
		} else {
			dbg_printf("Failed to apply AP-fix\n");
		}
	}

	arm9_boostVram = boostVram;

	if (esrbScreenPrepared) {
		while (!esrbImageLoaded) {
			while (REG_VCOUNT != 191);
			while (REG_VCOUNT == 191);
		}
	}

	toncset16((u32*)IMAGES_LOCATION, 0, (256*192)*3);	// Clear nds-bootstrap images and IPS patch
	clearScreen();

	REG_SCFG_EXT = 0x12A03000;

	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);

	nocashMessage("Starting the NDS file...");
    setMemoryAddress(ndsHeader, moduleParams, romFile);

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A))) {		// Dump RAM
		aFile ramDumpFile = getFileFromCluster(ramDumpCluster);
		if (extendedMemory2) {
			fileWrite((char*)0x02000000, ramDumpFile, 0, 0x7E0000);
			fileWrite((char*)(isSdk5(moduleParams) ? 0x02FE0000 : 0x027E0000), ramDumpFile, 0x7E0000, 0x20000);
		} else {
			fileWrite((char*)0x02000000, ramDumpFile, 0, 0x400000);
		}
	}

	startBinary_ARM7();

	return 0;
}
