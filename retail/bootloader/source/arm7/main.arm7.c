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

#define WARIOWARE_ENABLE	(*(vuint16 *)0x080000C6)

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
#include "unpatched_funcs.h"

#include "io_m3_common.h"
#include "io_g6_common.h"
#include "io_sc_common.h"

typedef signed int addr_t;
typedef unsigned char data_t;

//#define memcpy __builtin_memcpy

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

extern void arm7clearRAM(void);
extern void arm7code(u32* addr);

//extern u32 _start;
extern u32 storedFileCluster;
extern u32 romSize;
extern u32 initDisc;
extern u32 saveFileCluster;
extern u32 saveSize;
extern u32 apPatchFileCluster;
extern u32 apPatchSize;
extern u32 cheatFileCluster;
extern u32 cheatSize;
extern u32 patchOffsetCacheFileCluster;
extern u32 ramDumpCluster;
extern u32 srParamsFileCluster;
extern u32 screenshotCluster;
extern u32 musicCluster;
extern u32 musicsSize;
extern u32 pageFileCluster;
extern u32 manualCluster;
extern u32 sharedFontCluster;
extern u32 patchMpuSize;
extern u8 patchMpuRegion;
extern u8 language;
extern u8 region;
extern u8 donorSdkVer;
extern u8 soundFreq;

extern u32 _io_dldi_features;

extern u32* lastClusterCacheUsed;
extern u32 clusterCache;
extern u32 clusterCacheSize;

u32 arm9iromOffset = 0;
u32 arm9ibinarySize = 0;
u32 arm7iromOffset = 0;

bool ce9Alt = false;
static u32 ce9Location = 0;
static u32 overlaysSize = 0;
static u32 ioverlaysSize = 0;
u32 fatTableAddr = 0;

static u32 softResetParams[4] = {0};
bool srlFromPageFile = false;
u32 srlAddr = 0;
u8 baseUnitCode = 0;
u16 baseHeaderCRC = 0;
u16 baseSecureCRC = 0;
u32 baseRomSize = 0;
u32 baseChipID = 0;
bool pkmnHeader = false;

u16 s2FlashcardId = 0;

bool expansionPakFound = false;

u32 newArm7binarySize = 0;
u32 arm7mbk = 0;
u32 accessControl = 0;

void s2RamAccess(bool open) {
	if (_io_dldi_features & FEATURE_SLOT_NDS) return;

	if (open) {
		if (s2FlashcardId == 0x334D) {
			_M3_changeMode(M3_MODE_RAM);
		} else if (s2FlashcardId == 0x3647) {
			_G6_SelectOperation(G6_MODE_RAM);
		} else if (s2FlashcardId == 0x4353) {
			_SC_changeMode(SC_MODE_RAM);
		}
	} else {
		if (s2FlashcardId == 0x334D) {
			_M3_changeMode(M3_MODE_MEDIA);
		} else if (s2FlashcardId == 0x3647) {
			_G6_SelectOperation(G6_MODE_MEDIA);
		} else if (s2FlashcardId == 0x4353) {
			_SC_changeMode(SC_MODE_MEDIA);
		}
	}
}

void s2RamAccessInit(bool open) {
	if (_io_dldi_features & FEATURE_SLOT_GBA) return;

	if (open) {
		if (s2FlashcardId == 0x334D) {
			_M3_changeMode(M3_MODE_RAM);
		} else if (s2FlashcardId == 0x3647) {
			_G6_SelectOperation(G6_MODE_RAM);
		} else if (s2FlashcardId == 0x4353) {
			_SC_changeMode(SC_MODE_RAM);
		}
	} else {
		if (s2FlashcardId == 0x334D) {
			_M3_changeMode(M3_MODE_MEDIA);
		} else if (s2FlashcardId == 0x3647) {
			_G6_SelectOperation(G6_MODE_MEDIA);
		} else if (s2FlashcardId == 0x4353) {
			_SC_changeMode(SC_MODE_MEDIA);
		}
	}
}

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
	REG_SNDCAP0CNT = 0;
	REG_SNDCAP1CNT = 0;

	REG_SNDCAP0DAD = 0;
	REG_SNDCAP0LEN = 0;
	REG_SNDCAP1DAD = 0;
	REG_SNDCAP1LEN = 0;

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
	memset_addrs_arm7(0x02000620, IMAGES_LOCATION-0x1000);	// clear part of EWRAM - except before nds-bootstrap images
	toncset((u32*)0x02380000, 0, 0x38000);		// clear part of EWRAM - except before 0x023DA000, which has the arm9 code
	toncset((u32*)0x023C0000, 0, 0x20000);
	toncset((u32*)0x023F0000, 0, 0xD000);
	toncset((u32*)0x023FF000, 0, 0x1000);
	if (extendedMemory2) {
		toncset((u32*)0x02400000, 0, 0x3FC000);
		toncset((u32*)0x027FF000, 0, dsDebugRam ? 0x1000 : 0x801000);
	}

	REG_IE = 0;
	REG_IF = ~0;
	*(vu32*)0x0380FFFC = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)0x0380FFF8 = 0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
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
	module_params_t* moduleParams = (module_params_t*)malloc(sizeof(module_params_t));
	toncset(moduleParams, 0, sizeof(module_params_t));

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

	return (module_params_t*)(moduleParams - 7);
}

static module_params_t* getModuleParams(const tNDSHeader* ndsHeader) {
	nocashMessage("Looking for moduleparams...\n");

	u32* moduleParamsOffset = findModuleParamsOffset(ndsHeader);

	//module_params_t* moduleParams = (module_params_t*)((u32)moduleParamsOffset - 0x1C);
	return moduleParamsOffset ? (module_params_t*)(moduleParamsOffset - 7) : NULL;
}

/*static inline u32 getRomSizeNoArmBins(const tNDSHeader* ndsHeader) {
	return baseRomSize - ndsHeader->arm7romOffset - ndsHeader->arm7binarySize + overlaysSize;
}*/

// SDK 5
static bool ROMsupportsDsiMode(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode > 0);
}

static void loadBinary_ARM7(const tDSiHeader* dsiHeaderTemp, aFile* file) {
	nocashMessage("loadBinary_ARM7");

	//u32 ndsHeader[0x170 >> 2];
	//u32 dsiHeader[0x2F0 >> 2]; // SDK 5

	char baseTid[5] = {0};
	fileRead((char*)&baseTid, file, 0xC, 4);
	fileRead((char*)&baseUnitCode, file, 0x12, 1);

	// Read DSi header (including NDS header)
	//fileRead((char*)ndsHeader, file, 0, 0x170, 3);
	//fileRead((char*)dsiHeader, file, 0, 0x2F0, 2); // SDK 5
	if (srlFromPageFile) {
		srlAddr = 0xFFFFFFFF;
		aFile pageFile;
		getFileFromCluster(&pageFile, pageFileCluster);

		fileRead((char*)dsiHeaderTemp, &pageFile, 0x2BFE00, 0x160);
		fileRead(dsiHeaderTemp->ndshdr.arm9destination, &pageFile, 0x14000, dsiHeaderTemp->ndshdr.arm9binarySize);
		fileRead(dsiHeaderTemp->ndshdr.arm7destination, &pageFile, 0x2C0000, dsiHeaderTemp->ndshdr.arm7binarySize);
	} else {
		srlAddr = softResetParams[3];
		fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp));
		if (srlAddr > 0 && ((u32)dsiHeaderTemp->ndshdr.arm9destination < 0x02000000 || (u32)dsiHeaderTemp->ndshdr.arm9destination > 0x02004000)) {
			// Invalid SRL
			srlAddr = 0;
			fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp));
		}
		extern u32 donorFileTwlCluster;	// SDK5 (TWL)
		fileRead((char*)&arm7mbk, file, srlAddr+0x1A0, sizeof(u32));
		fileRead((char*)&accessControl, file, srlAddr+0x1B4, sizeof(u32));

		// Load binaries into memory
		fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize);
		if ((ndsHeader->unitCode > 0) ? (arm7mbk != 0x080037C0 || (arm7mbk == 0x080037C0 && donorFileTwlCluster == CLUSTER_FREE)) : (strncmp(baseTid, "AYI", 3) != 0 || ndsHeader->arm7binarySize != 0x25F70)) {
			fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize);
		}
	}

	if (
		strncmp(baseTid, "ADA", 3) == 0    // Diamond
		|| strncmp(baseTid, "APA", 3) == 0 // Pearl
		|| strncmp(baseTid, "CPU", 3) == 0 // Platinum
		|| strncmp(baseTid, "IPK", 3) == 0 // HG
		|| strncmp(baseTid, "IPG", 3) == 0 // SS
	) {
		// Fix Pokemon games needing header data.
		tNDSHeader* ndsHeaderPokemon = (tNDSHeader*)NDS_HEADER_POKEMON;
		fileRead((char*)ndsHeaderPokemon, file, 0, 0x160);
		pkmnHeader = true;
	}

	fileRead((char*)&baseHeaderCRC, file, 0x15E, sizeof(u16));
	fileRead((char*)&baseSecureCRC, file, 0x6C, sizeof(u16));
	fileRead((char*)&baseRomSize, file, 0x80, sizeof(u32));
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
		if (s2FlashcardId == 0x5A45) {
			romSizeLimit = 0xF80000;
		} else if (s2FlashcardId == 0x4353) {
			romSizeLimit = 0x1F7FE00;
		} else {
			romSizeLimit = 0x1F80000;
		}
		if (ndsHeader->unitCode > 0 && ndsHeader->gameCode[0] == 'K' && ((ndsHeader->gameCode[3] == 'K') ? korSharedFont : (ndsHeader->gameCode[3] == 'C') ? chnSharedFont : twlSharedFont)) {
			romSizeLimit -= 0x200000;
		}
	} else if (!extendedMemory2) {
		if (strncmp(romTid, "KD3", 3) == 0 // Jinia Supasonaru: Eiwa Rakubiki Jiten
		 || strncmp(romTid, "KD5", 3) == 0 // Jinia Supasonaru: Waei Rakubiki Jiten
		 || strncmp(romTid, "KD4", 3) == 0) { // Meikyou Kokugo: Rakubiki Jiten
			return false;
		} else if (// strncmp(romTid, "KLT", 3) == 0 // My Little Restaurant
				/* || */ strncmp(romTid, "KAU", 3) == 0) { // Nintendo Cowndown Calendar
			romLocation += 0x200000;
			romSizeLimit -= 0x200000;
		} else if (strncmp(romTid, "KQR", 3) == 0) { // Remote Racers
			romLocation += 0x280000;
			romSizeLimit -= 0x280000;
		}
		if (romSizeLimit > 0 && ndsHeader->unitCode > 0 && ndsHeader->gameCode[0] == 'K' && ((ndsHeader->gameCode[3] == 'K') ? korSharedFont : (ndsHeader->gameCode[3] == 'C') ? chnSharedFont : twlSharedFont)) {
			romSizeLimit -= 0x200000;
		}
	}
	if ((strncmp(romTid, "KD3", 3) == 0 || strncmp(romTid, "KD4", 3) == 0 || strncmp(romTid, "KD5", 3) == 0) && s2FlashcardId == 0x5A45) {
		return false;
	}
	if (extendedMemory2 && !dsDebugRam) {
		*(vu32*)(0x0DFFFE0C) = 0x4253444E;		// Check for 32MB of RAM
		isDevConsole = (*(vu32*)(0x0DFFFE0C) == 0x4253444E);

		romLocation = 0x0C800000;
		if (isSdk5(moduleParams)) {
			if (isDevConsole) {
				romLocation = 0x0D000000;
			}
			romSizeLimit = isDevConsole ? 0x1000000 : 0x7E0000;
		} else {
			romSizeLimit = isDevConsole ? 0x1800000 : 0x800000;
		}
	}

	if (romSizeLimit == 0) {
		return false;
	}

	bool res = false;
	if ((strncmp(romTid, "UBR", 3) == 0 && isDevConsole)
	// || (strncmp(romTid, "KXO", 3) == 0 && s2FlashcardId != 0x5A45) // 18th Gate
	 || (strncmp(romTid, "KQJ", 3) == 0 && s2FlashcardId != 0x5A45) // Aru Seishun no Monogatari: Kouenji Joshi Sakka
	 || (strncmp(romTid, "KXC", 3) == 0 && s2FlashcardId != 0x5A45) // Castle Conqueror: Heroes 2
	 || (strncmp(romTid, "KQ9", 3) == 0 && s2FlashcardId != 0x5A45) // The Legend of Zelda: Four Swords: Anniversary Edition
	 || (strncmp(romTid, "KEV", 3) == 0 && s2FlashcardId != 0x5A45) // Space Invaders Extreme Z
	 || (strncmp(romTid, "UOR", 3) != 0
	 && strncmp(romTid, "KYP", 3) != 0 // 1st Class Poker & BlackJack
	 && strncmp(romTid, "KXG", 3) != 0 // Abyss
	 && strncmp(romTid, "KTR", 3) != 0 // Clubhouse Games Express: Card Classics
	 && strncmp(romTid, "KTC", 3) != 0 && strncmp(romTid, "KTP", 3) != 0 // Clubhouse Games Express: Family Favorites
	 && strncmp(romTid, "KTD", 3) != 0 && strncmp(romTid, "KTB", 3) != 0 // Clubhouse Games Express: Strategy Pack
	 && strncmp(romTid, "KPP", 3) != 0
	 && strncmp(romTid, "KPF", 3) != 0)
	) {
		res = ((expansionPakFound || (extendedMemory2 && !dsDebugRam)) && (ndsHeader->unitCode == 3 ? (arm9iromOffset-0x8000)+ioverlaysSize : (baseRomSize-0x8000)+0x88) <= romSizeLimit);
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

u8 getRumblePakType(void) {
	// First, make sure we're on DS Phat/Lite, and if DLDI is Slot-1
	if (*(u16*)0x4004700 != 0 || (_io_dldi_features & FEATURE_SLOT_GBA) || s2FlashcardId != 0 || *(vu16*)0x08240000 == 1 || GBA_BUS[0] == 0xFFFE || GBA_BUS[1] == 0xFFFF) {
		return 0;
	}
	// Then, check for 0x96 to see if it's a GBA game or flashcart
	if (GBA_HEADER.is96h == 0x96) {
		WARIOWARE_ENABLE = 8;
		return 1;
	} else {
		for (int i = 0; i < 4000; i++) { // Run 4000 times to make sure it works
			for (int p = 0; p < 0x1000/2; p++) {
				if (GBA_BUS[1+(p*2)] == 0xFFFD) {
					return 2;
				}
			}
		}
	}
	return 0;
}

static bool supportsExceptionHandler(const char* romTid) {
	if (0 == (REG_KEYINPUT & KEY_B)) {
		// ExceptionHandler2 (red screen) blacklist
		return (strncmp(romTid, "ASM", 3) != 0	// SM64DS
		&& strncmp(romTid, "SMS", 3) != 0	// SMSW
		&& strncmp(romTid, "A2D", 3) != 0	// NSMB
		&& strncmp(romTid, "AMC", 3) != 0	// MKDS (ROM hacks may contain their own exception handler)
		&& strncmp(romTid, "ADM", 3) != 0);	// AC:WW
	}
	return true;
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
	arm7code(ndsHeader->arm7executeAddress);
}

static void loadOverlaysintoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile* file, u32 ROMinRAM) {
	// Load overlays into RAM
	if (overlaysSize < romSizeLimit)
	{
		u32 overlaysLocation = romLocation;
		if (ROMinRAM) {
			overlaysLocation += (ndsHeader->arm9binarySize-ndsHeader->arm9romOffset);
		}
		if ((_io_dldi_features & FEATURE_SLOT_GBA) && s2FlashcardId != 0) {
			const u32 buffer = 0x037F8000;
			const u16 bufferSize = 0x8000;
			s32 len = (s32)overlaysSize;
			u32 dst = 0;
			while (1) {
				u32 readLen = (len > bufferSize) ? bufferSize : len;

				fileRead((char*)buffer, file, (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize)+dst, readLen);
				tonccpy((char*)overlaysLocation+dst, (char*)buffer, readLen);

				len -= bufferSize;
				dst += bufferSize;

				if (len <= 0) {
					break;
				}
			}
			toncset((char*)buffer, 0, bufferSize);
		} else {
			fileRead((char*)overlaysLocation, file, ndsHeader->arm9romOffset + ndsHeader->arm9binarySize, overlaysSize);
		}

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
			fileRead(twlCfg+0x10, &romFile, 0x20E, 1); // EULA Version (0=None/CountryChanged, 1=v1)
			fileRead(twlCfg+0x9C, &romFile, 0x2F0, 1);  // Parental Controls Years of Age Rating (00h..14h)
		}*/

		if (softResetParams[0] == 0xFFFFFFFF && (accessControl & BIT(4)) && !ce9Alt) {
			fileRead((char*)0x02FFE230, &romFile, 0x230, 8);
		}

		// Set region flag
		if (useRomRegion || region == 0xFF) {
			u8 newRegion = 0;
			if (ndsHeader->gameCode[3] == 'J') {
				newRegion = 0;
			} else if (ndsHeader->gameCode[3] == 'E' || ndsHeader->gameCode[3] == 'T') {
				newRegion = 1;
			} else if (ndsHeader->gameCode[3] == 'P' || ndsHeader->gameCode[3] == 'V') {
				newRegion = 2;
			} else if (ndsHeader->gameCode[3] == 'U') {
				newRegion = 3;
			} else if (ndsHeader->gameCode[3] == 'C') {
				newRegion = 4;
			} else if (ndsHeader->gameCode[3] == 'K') {
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

    dbg_printf("chipID: ");
    dbg_hexa(baseChipID);
    dbg_printf("\n"); 

    // TODO
    // figure out what is 0x027ffc10, somehow related to cardId check
    //*((u32*)(isSdk5(moduleParams) ? 0x02fffc10 : 0x027ffc10)) = 1;

	if (pkmnHeader) {
		// Make the Pokemon game code ADAJ.
		const char gameCodePokemon[] = { 'A', 'D', 'A', 'J' };
		tNDSHeader* ndsHeaderPokemon = (tNDSHeader*)NDS_HEADER_POKEMON;
		tonccpy(ndsHeaderPokemon->gameCode, gameCodePokemon, 4);
	}

    // Set memory values expected by loaded NDS
    // from NitroHax, thanks to Chism
	*((u32*)(isSdk5(moduleParams) ? 0x02fff800 : 0x027ff800)) = baseChipID;		// CurrentCardID
	*((u32*)(isSdk5(moduleParams) ? 0x02fff804 : 0x027ff804)) = baseChipID;		// Command10CardID
	*((u16*)(isSdk5(moduleParams) ? 0x02fff808 : 0x027ff808)) = baseHeaderCRC;	// Header Checksum, CRC-16 of [000h-15Dh]
	*((u16*)(isSdk5(moduleParams) ? 0x02fff80a : 0x027ff80a)) = baseSecureCRC;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	*((u16*)(isSdk5(moduleParams) ? 0x02fff850 : 0x027ff850)) = 0x5835;

	// Copies of above
	*((u32*)(isSdk5(moduleParams) ? 0x02fffc00 : 0x027ffc00)) = baseChipID;		// CurrentCardID
	*((u32*)(isSdk5(moduleParams) ? 0x02fffc04 : 0x027ffc04)) = baseChipID;		// Command10CardID
	*((u16*)(isSdk5(moduleParams) ? 0x02fffc08 : 0x027ffc08)) = baseHeaderCRC;	// Header Checksum, CRC-16 of [000h-15Dh]
	*((u16*)(isSdk5(moduleParams) ? 0x02fffc0a : 0x027ffc0a)) = baseSecureCRC;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc10 : 0x027ffc10)) = 0x5835;

	if (softResetParams[0] != 0xFFFFFFFF) {
		u32* resetParamLoc = (u32*)(isSdk5(moduleParams) ? RESET_PARAM_SDK5 : RESET_PARAM);
		resetParamLoc[0] = softResetParams[0];
		resetParamLoc[1] = softResetParams[1];
		if (!srlFromPageFile) {
			resetParamLoc[2] = softResetParams[2];
		}
		resetParamLoc[3] = softResetParams[3];
	}

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 0x1;						// Boot Indicator (Booted from card for SDK5) -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr

	const char* romTid = getRomTid(ndsHeader);
	if (strncmp(romTid, "KPP", 3) == 0 	// Pop Island
	 || strncmp(romTid, "KPF", 3) == 0		// Pop Island: Paperfield
	 || strncmp(romTid, "KGK", 3) == 0		// Glory Days: Tactical Defense
	 || (srlAddr > 0) || srlFromPageFile)
	{
		*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 0x2;					// Boot Indicator (Cloneboot/Multiboot)
	}

	/*if (softResetParams[2] == 0x44414F4C) {
		*((u16*)(isSdk5(moduleParams) ? 0x02fffcfa : 0x027ffcfa)) = 0x1041;	// NoCash: channel ch1+7+13 (Doesn't seem to work)
	}*/
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

	s2FlashcardId = *(u16*)(0x020000C0);

	s2RamAccessInit(true);

	// Init card
	if (!FAT_InitFiles(initDisc)) {
		nocashMessage("!FAT_InitFiles");
		errorOutput();
		//return -1;
	}

	if (logging) {
		aFile logFile;
		getBootFileCluster(&logFile, "NDSBTSRP.LOG");
		enableDebug(&logFile);
	}

	*(vu32*)(0x02000000) = 0; // Clear debug RAM check flag

	aFile srParamsFile;
	getFileFromCluster(&srParamsFile, srParamsFileCluster);
	fileRead((char*)&softResetParams, &srParamsFile, 0, 0x10);
	srlFromPageFile = (softResetParams[2] == 0x44414F4C); // 'LOAD'
	bool softResetParamsFound = (softResetParams[0] != 0xFFFFFFFF || srlFromPageFile || softResetParams[3] != 0);
	if (softResetParamsFound) {
		u32 clearBuffer = 0xFFFFFFFF;
		fileWrite((char*)&clearBuffer, &srParamsFile, 0, 0x4);
		clearBuffer = 0;
		fileWrite((char*)&clearBuffer, &srParamsFile, 0x4, 0x4);
		fileWrite((char*)&clearBuffer, &srParamsFile, 0x8, 0x4);
		fileWrite((char*)&clearBuffer, &srParamsFile, 0xC, 0x4);
	}

	// BOOT.NDS file
	//aFile bootNdsFile = getBootFileCluster(bootName, 0);

	// ROM file
	aFile romFile;
	getFileFromCluster(&romFile, storedFileCluster);

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
	aFile savFile;
	getFileFromCluster(&savFile, saveFileCluster);
	
	int errorCode;

	tDSiHeader dsiHeaderTemp;

	// Load the NDS file
	nocashMessage("Loading the NDS file...\n");

	loadBinary_ARM7(&dsiHeaderTemp, &romFile);
	
	// File containing cached patch offsets
	aFile patchOffsetCacheFile;
	getFileFromCluster(&patchOffsetCacheFile, patchOffsetCacheFileCluster);
	fileRead((char*)&patchOffsetCache, &patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	patchOffsetCacheFilePrevCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
	u16 prevPatchOffsetCacheFileVersion = patchOffsetCache.ver;

	nocashMessage("Loading the header...\n");

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);

	if (dsiHeaderTemp.ndshdr.unitCode < 3 && dsiHeaderTemp.ndshdr.gameCode[0] != 'K' && dsiHeaderTemp.ndshdr.gameCode[0] != 'Z' &&  srlAddr == 0 && (softResetParams[0] == 0 || softResetParams[0] == 0xFFFFFFFF)) {
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

	baseChipID = getChipId(pkmnHeader ? (tNDSHeader*)NDS_HEADER_POKEMON : ndsHeader, moduleParams);

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
		fileRead((char*)&arm9iromOffset, &romFile, 0x1C0, sizeof(u32));
		fileRead((char*)&arm9ibinarySize, &romFile, 0x1CC, sizeof(u32));
		fileRead((char*)&arm7iromOffset, &romFile, 0x1D0, sizeof(u32));

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
		toncset((char*)0x0C4000C0, 0, 0xE);
		*(u16*)0x0C4000CE = 0x7FFF;
		toncset((char*)0x0C4000D0, 0, 0x130);
	}

	*(vu16*)0x08240000 = 1;
	expansionPakFound = ((*(vu16*)0x08240000 == 1) && (strcmp(romTid, "UBRP") != 0));

	if ((strcmp(romTid, "UBRP") == 0) && /*(_io_dldi_features & FEATURE_SLOT_NDS) &&*/ s2FlashcardId != 0 && s2FlashcardId != 0x5A45) {
		toncset((char*)0x08000000, 0xFF, 0xC0);
		toncset((u8*)0x080000B2, 0, 3);
		toncset((u8*)0x080000B5, 0x24, 3);
		*(u16*)0x080000BE = 0x7FFF;
		toncset((char*)0x080000C0, 0, 0xE);
		*(u16*)0x080000CE = 0x7FFF;
		toncset((char*)0x080000D0, 0, 0x130);
	}

	// If possible, set to load ROM into RAM
	ROMinRAM = isROMLoadableInRAM(ndsHeader, romTid, moduleParams);

	nocashMessage("Trying to patch the card...\n");

	extern void rsetPatchCache(void);
	rsetPatchCache();

	ce9Location = *(u32*)CARDENGINE_ARM9_LOCATION_BUFFERED;
	ce9Alt = (ce9Location == CARDENGINE_ARM9_LOCATION_DLDI_ALT || ce9Location == CARDENGINE_ARM9_LOCATION_DLDI_ALT2);
	tonccpy((u32*)ce9Location, (u32*)CARDENGINE_ARM9_LOCATION_BUFFERED, ce9Alt ? 0x2800 : 0x3800);
	toncset((u32*)0x023E0000, 0, 0x10000);

	tonccpy((u8*)CARDENGINE_ARM7_LOCATION, (u8*)CARDENGINE_ARM7_LOCATION_BUFFERED, 0x1400);
	toncset((u8*)CARDENGINE_ARM7_LOCATION_BUFFERED, 0, 0x1400);

	if (!dldiPatchBinary((data_t*)ce9Location, 0x3800, (data_t*)(extendedMemory2 ? 0x027BD000 : ((accessControl & BIT(4)) && !ce9Alt) ? 0x023FC000 : 0x023FD000))) {
		nocashMessage("ce9 DLDI patch failed");
		dbg_printf("ce9 DLDI patch failed\n");
		errorOutput();
	}

	aFile musicsFile;
	getFileFromCluster(&musicsFile, musicCluster);
	if (musicCluster != 0) {
		dbg_printf("Music pack found!\n");
	}

	bool wramUsed = false;
	u32 fatTableSize = 0;
	u32 fatTableSizeNoExp = (moduleParams->sdk_version < 0x2008000) ? 0x19C00 : 0x1A800;
	if (ce9Alt && moduleParams->sdk_version >= 0x2008000) {
		fatTableSizeNoExp = 0x598;
	}
	if (s2FlashcardId == 0x334D || s2FlashcardId == 0x3647 || s2FlashcardId == 0x4353) {
		fatTableAddr = (s2FlashcardId==0x4353 ? 0x09F7FE00 : 0x09F80000);
		fatTableSize = 0x80000;
	} else if (s2FlashcardId == 0x5A45) {
		fatTableAddr = 0x08F80000;
		fatTableSize = 0x80000;
	} else if (expansionPakFound) {
		fatTableAddr = 0x09780000;
		fatTableSize = 0x80000;
	} else if (extendedMemory2) {
		if (strncmp(romTid, "KQ9", 3) == 0 // The Legend of Zelda: Four Swords: Anniversary Edition
		 || strncmp(romTid, "KDM", 3) == 0 // Mario vs. Donkey Kong: Minis March Again!
		 || strncmp(romTid, "KEV", 3) == 0 // Space Invaders Extreme Z
		) {
			fatTableAddr = 0x02000000;
			fatTableSize = 0x4000;
		} else {
			fatTableAddr = 0x02700000;
			fatTableSize = 0x80000;
		}
	} else {
		fatTableAddr = (moduleParams->sdk_version < 0x2008000) ? 0x023E0000 : 0x023C0000;
		fatTableSize = fatTableSizeNoExp;

		if (moduleParams->sdk_version >= 0x2008000) {
			fatTableAddr = ce9Alt ? 0x023FF268 : CARDENGINE_ARM9_LOCATION_DLDI;

			lastClusterCacheUsed = (u32*)0x037F8000;
			clusterCache = 0x037F8000;
			clusterCacheSize = 0x16000;

			wramUsed = true;
		}
	}

	if (!wramUsed) {
		lastClusterCacheUsed = (u32*)fatTableAddr;
		clusterCache = fatTableAddr;
		clusterCacheSize = fatTableSize;
	}

	if (fatTableSize <= 0x20000) {
		buildFatTableCacheCompressed(&romFile);
	} else {
		buildFatTableCache(&romFile);
	}

	if (wramUsed) {
		buildFatTableCacheCompressed(&romFile);
		if (romFile.fatTableCached) {
			bool startMem = (!ce9Alt && ndsHeader->unitCode > 0 && (u32)ndsHeader->arm9destination >= 0x02004000 && ((accessControl & BIT(4)) || arm7mbk == 0x080037C0) && romFile.fatTableCacheSize <= 0x4000);

			//fatTableAddr -= (romFile.fatTableCacheSize/0x200)*0x200;
			if (!ce9Alt) {
				if (startMem) {
					fatTableAddr = 0x02000000;
				} else {
					fatTableAddr -= romFile.fatTableCacheSize;
				}
			}
			tonccpy((u32*)fatTableAddr, (u32*)0x037F8000, romFile.fatTableCacheSize);
			romFile.fatTableCache = (u32*)fatTableAddr;

			lastClusterCacheUsed = (u32*)0x037F8000;
			clusterCache = 0x037F8000;
			clusterCacheSize = (startMem ? 0x4000 : fatTableSizeNoExp)-romFile.fatTableCacheSize;

			if (!startMem || (startMem && romFile.fatTableCacheSize < 0x4000)) {
				buildFatTableCacheCompressed(&savFile);
				if (savFile.fatTableCached) {
					if (startMem || ce9Alt) {
						fatTableAddr += romFile.fatTableCacheSize;
					} else {
						fatTableAddr -= savFile.fatTableCacheSize;
					}
					tonccpy((u32*)fatTableAddr, (u32*)0x037F8000, savFile.fatTableCacheSize);
					savFile.fatTableCache = (u32*)fatTableAddr;
				}
				if (musicCluster != 0 && !ce9Alt) {
					lastClusterCacheUsed = (u32*)0x037F8000;
					clusterCache = 0x037F8000;
					clusterCacheSize = (startMem ? 0x4000 : fatTableSizeNoExp)-savFile.fatTableCacheSize;

					buildFatTableCache(&musicsFile);
					if (startMem) {
						fatTableAddr += savFile.fatTableCacheSize;
					} else {
						fatTableAddr -= musicsFile.fatTableCacheSize;
					}
					tonccpy((u32*)fatTableAddr, (u32*)0x037F8000, musicsFile.fatTableCacheSize);
					musicsFile.fatTableCache = (u32*)fatTableAddr;
				}
			}
			if (!startMem && !ce9Alt) {
				fatTableAddr -= (fatTableAddr % 512); // Align end of heap to 512 bytes
			}
		} else {
			lastClusterCacheUsed = (u32*)fatTableAddr;
			clusterCache = fatTableAddr;
			clusterCacheSize = fatTableSize;

			buildFatTableCacheCompressed(&romFile);
			buildFatTableCacheCompressed(&savFile);
		}
	} else if (fatTableSize <= 0x20000) {
		buildFatTableCacheCompressed(&savFile);
	} else {
		buildFatTableCache(&savFile);
	}

	patchBinary((cardengineArm9*)ce9Location, ndsHeader, moduleParams);
	errorCode = patchCardNds(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		(cardengineArm9*)ce9Location,
		ndsHeader,
		moduleParams,
		(  (moduleParams->sdk_version < 0x4000000 && ((u32)ndsHeader->arm9executeAddress - (u32)ndsHeader->arm9destination) >= 0x1000
		&& moduleParams->sdk_version != 0x2007533)
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

	toncset((u32*)0x0380C000, 0, 0x2000);

	errorCode = hookNdsRetailArm7(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		ndsHeader,
		moduleParams,
		cheatFileCluster,
		cheatSize,
		apPatchFileCluster,
		apPatchSize,
		language,
		getRumblePakType()
	);
	if (errorCode == ERR_NONE) {
		nocashMessage("Card hook successful");
	} else {
		nocashMessage("Card hook failed");
		errorOutput();
	}

	if (expansionPakFound || (extendedMemory2 && !dsDebugRam && strncmp(romTid, "UBRP", 4) != 0)) {
		loadOverlaysintoRAM(ndsHeader, moduleParams, &romFile, ROMinRAM);
	}

	aFile bootNds;
	getBootFileCluster(&bootNds, "BOOT.NDS");

	errorCode = hookNdsRetailArm9(
		(cardengineArm9*)ce9Location,
		ndsHeader,
		moduleParams,
		bootNds.firstCluster,
		romFile.firstCluster,
		savFile.firstCluster,
		saveSize,
		(u32)romFile.fatTableCache,
		(u32)savFile.fatTableCache,
		romFile.fatTableCompressed,
		savFile.fatTableCompressed,
		(u32)musicsFile.fatTableCache,
		ramDumpCluster,
		srParamsFileCluster,
		screenshotCluster,
		musicCluster,
		musicsSize,
		pageFileCluster,
		manualCluster,
		sharedFontCluster,
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

	patchOffsetCacheFileNewCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
	if (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheFileNewCrc != patchOffsetCacheFilePrevCrc) {
		fileWrite((char*)&patchOffsetCache, &patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	}

	if (srlAddr == 0 && apPatchFileCluster != 0 && !apPatchIsCheat && apPatchSize > 0 && apPatchSize <= 0x40000) {
		aFile apPatchFile;
		getFileFromCluster(&apPatchFile, apPatchFileCluster);
		dbg_printf("AP-fix found\n");
		if (esrbScreenPrepared) {
			while (!esrbImageLoaded) {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			}
		}
		fileRead((char*)IMAGES_LOCATION, &apPatchFile, 0, apPatchSize);
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

	if (ce9Alt) {
		unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;
		extern u32* copyBackCe9;
		extern u32 copyBackCe9Len;

		cardengineArm9* ce9 = (cardengineArm9*)ce9Location;
		//u32 codeBranch = (u32)patchOffsetCache.cardReadStartOffset; // Breaks games which relocate arm9 code
		//codeBranch += 0x30;
		const u32 codeBranch = 0x023FF200;

		tonccpy((u32*)0x02370000, ce9, 0x2800);
		tonccpy((u32*)codeBranch, copyBackCe9, copyBackCe9Len);
		for (int i = 0; i < copyBackCe9Len/4; i++) {
			u32* addr = (u32*)codeBranch;
			if (addr[i] == 0x77777777) {
				addr[i] = ce9Location;
				break;
			}
		}

		toncset(ce9, 0, 0x2800);

		u32 blFrom = (u32)ndsHeader->arm9executeAddress;
		for (int i = 0; i < 0x200/4; i++) {
			u32* addr = ndsHeader->arm9executeAddress;
			if (addr[i] == 0xE5810000) {
				unpatchedFuncs->exeCode = addr[i];
				break;
			}
			blFrom += 4;
		}
		unpatchedFuncs->exeCodeOffset = (u32*)blFrom;

		setBL(blFrom, codeBranch);
	}

	extern u32 iUncompressedSize;
	aFile pageFile;
	getFileFromCluster(&pageFile, pageFileCluster);

	fileWrite((char*)ndsHeader->arm9destination, &pageFile, 0x14000, iUncompressedSize);
	fileWrite((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, newArm7binarySize);
	fileWrite((char*)CHEAT_ENGINE_LOCATION_B4DS, &pageFile, 0x2FE000, 0x2000);
	fileWrite((char*)&iUncompressedSize, &pageFile, 0x3FFFF0, sizeof(u32));
	fileWrite((char*)&newArm7binarySize, &pageFile, 0x3FFFF4, sizeof(u32));

	clearScreen();

	REG_SCFG_EXT = 0x12A03000;

	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);

	nocashMessage("Starting the NDS file...");
    setMemoryAddress(ndsHeader, moduleParams, romFile);

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A))) {		// Dump RAM
		aFile ramDumpFile;
		getFileFromCluster(&ramDumpFile, ramDumpCluster);
		if (extendedMemory2) {
			fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x7E0000);
			fileWrite((char*)(isSdk5(moduleParams) ? 0x02FE0000 : 0x027E0000), &ramDumpFile, 0x7E0000, 0x20000);
		} else {
			fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x400000);
		}
	}

	startBinary_ARM7();

	return 0;
}
