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

extern void arm7code(u32* addr);

//extern u32 _start;
extern u16 a9ScfgRom;
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
extern u32 apFixOverlaysCluster;
extern u32 musicCluster;
extern u32 musicsSize;
extern u32 pageFileCluster;
extern u32 manualCluster;
extern u32 sharedFontCluster;
extern u32 patchMpuSize;
extern u8 patchMpuRegion;
extern u8 language;
extern s8 region;
extern u8 donorSdkVer;
extern u8 soundFreq;

extern u32 _io_dldi_features;

extern u32* lastClusterCacheUsed;
extern u32 clusterCache;
extern u32 clusterCacheSize;

bool ce9Alt = false;
bool ce9AltLargeTable = false;
static u32 ce9Location = 0;
static u32 overlaysSize = 0;
static u32 ioverlaysSize = 0;
u32 fatTableAddr = 0;

static aFile srParamsFile;
static u32 softResetParams[4] = {0};
bool srlFromPageFile = false;
u32 srlAddr = 0;
u8 baseUnitCode = 0;
u16 baseHeaderCRC = 0;
u16 baseSecureCRC = 0;
u32 baseRomSize = 0;
u32 romPaddingSize = 0;
u32 arm9iromOffset = 0;
u32 arm9ibinarySize = 0;
u32 baseChipID = 0;
bool pkmnHeader = false;

u16 s2FlashcardId = 0;

bool expansionPakFound = false;
bool sharedFontInMep = false;

u8 arm7newUnitCode = 0;
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

	REG_RCNT = 0;

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	memset_addrs_arm7(0x03800000 - 0x8000, 0x03800000 + 0x10000);
	memset_addrs_arm7(0x02000620, 0x02084000);	// clear part of EWRAM
	memset_addrs_arm7(0x02280000, IMAGES_LOCATION-0x1000);	// clear part of EWRAM - except before nds-bootstrap images
	toncset((u32*)0x02380000, 0, 0x38000);		// clear part of EWRAM - except before 0x023DA000, which has the arm9 code
	toncset((u32*)0x023C0000, 0, 0x20000);
	toncset((u32*)0x023F0000, 0, 0xD000);
	toncset((u32*)0x023FE000, 0, 0x400);
	toncset((u32*)0x023FF000, 0, 0x1000);
	if (extendedMemory) {
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
	// 0xAC: special setting (when found special gamecode)
	// 0xA7: normal setting (for any other gamecodes)
	const u8 volLevel = volumeFix ? 0xAC : 0xA7;

	// Touchscreen
	cdcWriteReg(CDC_SOUND, 0x26, volLevel);
	cdcWriteReg(CDC_SOUND, 0x27, volLevel);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x03);
	cdcWriteReg(CDC_TOUCHCNT, 0x03, 0x00);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcWriteReg(CDC_SOUND, 0x22, 0x70);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	
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
	u32* moduleParamsOffset = patchOffsetCache.moduleParamsOffset;
	if (!patchOffsetCache.moduleParamsOffset) {
		// nocashMessage("Looking for moduleparams...\n");
		moduleParamsOffset = findModuleParamsOffset(ndsHeader);
		if (moduleParamsOffset) {
			patchOffsetCache.moduleParamsOffset = moduleParamsOffset;
		}
	}

	if (moduleParamsOffset) {
		dbg_printf("Module params offset: ");
		dbg_hexa((u32)moduleParamsOffset);
		dbg_printf("\n");
	}

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
	// nocashMessage("loadBinary_ARM7");

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
		extern u32 donorFileCluster;	// SDK5
		fileRead((char*)&arm7mbk, file, srlAddr+0x1A0, sizeof(u32));
		fileRead((char*)&accessControl, file, srlAddr+0x1B4, sizeof(u32));

		// Load binaries into memory
		fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize);
		if ((ndsHeader->unitCode > 0) ? (arm7mbk != 0x080037C0 || (arm7mbk == 0x080037C0 && donorFileCluster == CLUSTER_FREE)) : (strncmp(baseTid, "AYI", 3) != 0 || ndsHeader->arm7binarySize != 0x25F70)) {
			fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize);
		}
	}

	dbg_printf("Header CRC is ");
	u16 currentHeaderCRC = swiCRC16(0xFFFF, (void*)&dsiHeaderTemp->ndshdr, 0x15E);
	if (currentHeaderCRC != dsiHeaderTemp->ndshdr.headerCRC16) {
		dbg_printf("in");
	}
	dbg_printf("valid!\n");

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

	u8 baseDeviceSize = 0;
	fileRead((char*)&baseDeviceSize, file, 0x14, sizeof(u8));

	romPaddingSize = 0x20000 << baseDeviceSize;
	if (baseRomSize == 0 ? (romSize > romPaddingSize) : (baseRomSize > romPaddingSize)) {
		dbg_printf("ROM size is larger than device size!\n");
		while (baseRomSize == 0 ? (romSize > romPaddingSize) : (baseRomSize > romPaddingSize)) {
			baseDeviceSize++;
			romPaddingSize = 0x20000 << baseDeviceSize;
		}
	}

	arm9iromOffset = (u32)dsiHeaderTemp->arm9iromOffset;
	arm9ibinarySize = dsiHeaderTemp->arm9ibinarySize;
}

static module_params_t* loadModuleParams(const tNDSHeader* ndsHeader, bool* foundPtr) {
	module_params_t* moduleParams = getModuleParams(ndsHeader);
	*foundPtr = (bool)moduleParams;
	if (*foundPtr) {
		// Found module params
	} else {
		// nocashMessage("No moduleparams?\n");
		moduleParams = buildModuleParams(donorSdkVer);
	}
	return moduleParams;
}

u32 romLocation = 0x09000000;
s32 romSizeLimit = 0x780000;
u32 ROMinRAM = 0;

static bool isROMLoadableInRAM(const tDSiHeader* dsiHeader, const tNDSHeader* ndsHeader, const char* romTid, const module_params_t* moduleParams, const bool usesCloneboot) {
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
		if (strncmp(romTid, "UBR", 3) == 0) {
			romSizeLimit = 0x800000;
			if (s2FlashcardId == 0x5A45) {
				romLocation += 0x200;
				romSizeLimit -= 0x200;
			} else {
				romLocation += 0x800000;
			}
		}
	} else if (!extendedMemory) {
		s32 romSizeLimitChange = 0;
		if (strncmp(romTid, "KD3", 3) == 0 // Jinia Supasonaru: Eiwa Rakubiki Jiten
		 || strncmp(romTid, "KD5", 3) == 0 // Jinia Supasonaru: Waei Rakubiki Jiten
		 || strncmp(romTid, "KD4", 3) == 0) { // Meikyou Kokugo: Rakubiki Jiten
			return false;
		} else if (strncmp(romTid, "KBJ", 3) == 0) { // 21 Blackjack
			romSizeLimitChange = 0x410000;
		} else if (strncmp(romTid, "K5I", 3) == 0) { // 5 in 1 Solitaire
			romSizeLimitChange = 0x1F0000;
		} /* else if (strncmp(romTid, "KAT", 3) == 0) { // AiRace: Tunnel
			romSizeLimitChange = 0x80000;
		} */ else if (strncmp(romTid, "KCT", 3) == 0 // Chess Challenge!
				 || strncmp(romTid, "KWK", 3) == 0 // Mega Words
				 || strncmp(romTid, "KSC", 3) == 0 // Sudoku Challenge!
				 || strncmp(romTid, "KWS", 3) == 0 // Word Searcher
				 || strncmp(romTid, "KWR", 3) == 0 // Word Searcher II
				 || strncmp(romTid, "KW6", 3) == 0 // Word Searcher III
				 || strncmp(romTid, "KW8", 3) == 0) { // Word Searcher IV
			romSizeLimitChange = 0x77C000;
		} /* else if (strncmp(romTid, "KGU", 3) == 0) { // Flipnote Studio
			romSizeLimitChange = 0x140000;
		} */ else if (strncmp(romTid, "KUP", 3) == 0) { // Match Up!
			romSizeLimitChange = 0x380000;
		} else if (strncmp(romTid, "KQR", 3) == 0) { // Remote Racers
			romSizeLimitChange = 0x280000;
		}
		if (romSizeLimitChange) {
			romLocation += romSizeLimitChange;
			romSizeLimit -= romSizeLimitChange;
		}
	}
	if (sharedFontInMep) {
		romSizeLimit -= 0x200000;
	}
	if ((strncmp(romTid, "KD3", 3) == 0 || strncmp(romTid, "KD4", 3) == 0 || strncmp(romTid, "KD5", 3) == 0) && s2FlashcardId == 0x5A45) {
		return false;
	}
	if ((strncmp(romTid, "KVL", 3) == 0) // Clash of Elementalists
	&& ((s2FlashcardId != 0x334D && s2FlashcardId != 0x3647 && s2FlashcardId != 0x4353) || (s2FlashcardId == 0x5A45 && baseRomSize > 0x800000))) {
		return false;
	}
	if (extendedMemory && !dsDebugRam) {
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

	if (romSizeLimit <= 0 || (strncmp(romTid, "UBR", 3) == 0 && extendedMemory && !dsDebugRam && !isDevConsole)) {
		return false;
	}

	bool res = false;
	if ((strncmp(romTid, "UBR", 3) == 0 && isDevConsole)
	// || (strncmp(romTid, "KXO", 3) == 0 && s2FlashcardId != 0x5A45) // 18th Gate
	 || (strncmp(romTid, "KQJ", 3) == 0 && s2FlashcardId != 0x5A45) // Aru Seishun no Monogatari: Kouenji Joshi Sakka
	 || (strncmp(romTid, "KXC", 3) == 0 && s2FlashcardId != 0x5A45) // Castle Conqueror: Heroes 2
	 || (strncmp(romTid, "KQ9", 3) == 0 && s2FlashcardId != 0x5A45) // The Legend of Zelda: Four Swords: Anniversary Edition
	 || (strncmp(romTid, "KEV", 3) == 0 && s2FlashcardId != 0x5A45) // Space Invaders Extreme Z
	 || (strncmp(romTid, "K97", 3) == 0 && s2FlashcardId != 0x5A45) // Sutanoberuzu: Kono Hareta Sora no Shita de
	 || (strncmp(romTid, "K98", 3) == 0 && s2FlashcardId != 0x5A45) // Sutanoberuzu: Shirogane no Torikago
	 || (strncmp(romTid, "UOR", 3) != 0
	 && strncmp(romTid, "KPP", 3) != 0 // Pop Island
	 && strncmp(romTid, "KPF", 3) != 0) // Pop Island: Paperfield
	) {
		u32 romSize = baseRomSize;
		if (usesCloneboot) {
			romSize -= 0x8000;
			romSize += 0x88;
		} else if (ndsHeader->arm9overlaySource == 0 || ndsHeader->arm9overlaySize == 0) {
			romSize -= (ndsHeader->arm7romOffset + ndsHeader->arm7binarySize);
		} else if (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) {
			romSize -= (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
		} else {
			romSize -= ndsHeader->arm9overlaySource;
		}
		res = ((expansionPakFound || (extendedMemory && !dsDebugRam)) && (ndsHeader->unitCode == 3 ? (usesCloneboot ? ((u32)dsiHeader->arm9iromOffset-0x8000) : ((u32)dsiHeader->arm9iromOffset-ndsHeader->arm9overlaySource))+ioverlaysSize : romSize) <= romSizeLimit);
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
	if (*(vu16*)0x4004700 != 0 || (_io_dldi_features & FEATURE_SLOT_GBA) || s2FlashcardId != 0 || *(vu16*)0x08240000 == 1 || GBA_BUS[0] == 0xFFFE || GBA_BUS[1] == 0xFFFF) {
		return 0;
	}
	// Then, check for 0x96 to see if it's a GBA game or flashcart
	if (GBA_HEADER.is96h == 0x96) {
		WARIOWARE_ENABLE = 8;
		return 1;
	} else {
		// Check for DS Phat or Lite Rumble Pak
		for (int i = 0; i < 0xFFF; i++) {
			if (GBA_BUS[i] != 0xFFFD && GBA_BUS[i] != 0xFDFF) {
				return 2;
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

static void loadOverlaysintoFile(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile* file) {
	// Load overlays into RAM
	if (overlaysSize <= 0x700000)
	{
		aFile apFixOverlaysFile;
		getFileFromCluster(&apFixOverlaysFile, apFixOverlaysCluster);

		const u32 buffer = 0x037F8000;
		const u16 bufferSize = 0x8000;
		const u32 overlaysOffset = (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) ? (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize) : ndsHeader->arm9overlaySource;
		s32 len = (s32)overlaysSize;
		u32 dst = 0;
		while (1) {
			u32 readLen = (len > bufferSize) ? bufferSize : len;

			fileRead((char*)buffer, file, overlaysOffset+dst, readLen);
			fileWrite((char*)buffer, &apFixOverlaysFile, overlaysOffset+dst, readLen);

			len -= bufferSize;
			dst += bufferSize;

			if (len <= 0) {
				break;
			}
		}
		toncset((char*)buffer, 0, bufferSize);

		if (!isSdk5(moduleParams)) {
			u32 word = 0;
			fileRead((char*)&word, &apFixOverlaysFile, 0x3128AC, sizeof(u32));
			if (word == 0x4B434148) {
				word = 0xA00;	// Primary fix for Mario's Holiday
				fileWrite((char*)&word, &apFixOverlaysFile, 0x3128AC, sizeof(u32));
			}
		}
	} else {
		apFixOverlaysCluster = 0;
	}
}

static void fileReadWithBuffer(const u32 memDst, aFile* file, const u32 src, u32 readLen) {
	const u32 buffer = 0x037F8000;
	const u16 bufferSize = 0x8000;
	s32 len = readLen;
	u32 dst = 0;
	while (1) {
		u32 readLenBuf = (len > bufferSize) ? bufferSize : len;

		fileRead((char*)buffer, file, src+dst, readLenBuf);
		tonccpy((char*)memDst+dst, (char*)buffer, readLenBuf);

		len -= bufferSize;
		dst += bufferSize;

		if (len <= 0) {
			break;
		}
	}
	toncset((char*)buffer, 0, bufferSize);
}

static void loadIOverlaysintoRAM(const tDSiHeader* dsiHeader, aFile* file, const bool usesCloneboot) {
	// Load overlays into RAM
	if (ioverlaysSize>0x700000) return;

	const u32 romOffset = usesCloneboot ? 0x8000 : ndsHeader->arm9overlaySource;
	const u32 overlayOffset = (u32)dsiHeader->arm9iromOffset+dsiHeader->arm9ibinarySize;
	const u32 romLocationEdit = romLocation+((u32)dsiHeader->arm9iromOffset-romOffset);

	if ((_io_dldi_features & FEATURE_SLOT_GBA) && s2FlashcardId != 0) {
		fileReadWithBuffer(romLocationEdit, file, overlayOffset, ioverlaysSize);
	} else {
		fileRead((char*)romLocationEdit, file, overlayOffset, ioverlaysSize);
	}
}

static void loadROMintoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile* file, const bool usesCloneboot) {
	u32 romOffset = 0;
	s32 romSizeEdit = baseRomSize;
	if (usesCloneboot) {
		romOffset = 0x8000;
		romSizeEdit -= 0x8000;
		romSizeEdit += 0x88;
	} else if (ndsHeader->arm9overlaySource == 0 || ndsHeader->arm9overlaySize == 0) {
		romOffset = (ndsHeader->arm7romOffset + ndsHeader->arm7binarySize);
	} else if (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) {
		romOffset = (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
	} else {
		romOffset = ndsHeader->arm9overlaySource;
	}
	if (!usesCloneboot) {
		romSizeEdit -= romOffset;
	}

	if ((_io_dldi_features & FEATURE_SLOT_GBA) && s2FlashcardId != 0) {
		fileReadWithBuffer(romLocation, file, romOffset, romSizeEdit);
	} else {
		fileRead((char*)romLocation, file, romOffset, romSizeEdit);
	}

	if (!isSdk5(moduleParams) && *(u32*)((romLocation-romOffset)+0x003128AC) == 0x4B434148) {
		*(u32*)((romLocation-romOffset)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday
	}

	dbg_printf("ROM pre-loaded into RAM at ");
	dbg_hexa(romLocation);
	dbg_printf("\n\n");
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
		if ((useRomRegion || region == -1) && ndsHeader->gameCode[3] != 'A' && ndsHeader->gameCode[3] != 'O') {
			// Determine region by TID
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
		} else if (region == -1) {
			// Determine region by language
			PERSONAL_DATA* personalData = (PERSONAL_DATA*)((u32)__NDSHeader - (u32)ndsHeader + (u32)PersonalData); //(u8*)((u32)ndsHeader - 0x180)
			u8 newRegion = 0;
			if (personalData->language == 0) {
				newRegion = 0;	// Japan
			} else if (personalData->language == 6) {
				newRegion = 4;	// China
			} else if (personalData->language == 7) {
				newRegion = 5;	// Korea
			} else if (personalData->language == 1) {
				newRegion = 1;	// USA
			} else if (personalData->language >= 2 && personalData->language <= 5) {
				newRegion = 2;	// Europe
			}
			toncset((u8*)0x02FFFD70, newRegion, 1);
		} else {
			toncset((u8*)0x02FFFD70, region, 1);
		}
		// Set bitmask for supported languages
		u8 curRegion = *(u8*)0x02FFFD70;
		if (curRegion == 1) {
			*(u32*)(0x02FFFD68) = 0x26; // USA
		} else if (curRegion == 2) {
			*(u32*)(0x02FFFD68) = 0x3E; // EUR
		} else if (curRegion == 3) {
			*(u32*)(0x02FFFD68) = 0x02; // AUS
		} else if (curRegion == 4) {
			*(u32*)(0x02FFFD68) = 0x40; // CHN
		} else if (curRegion == 5) {
			*(u32*)(0x02FFFD68) = 0x80; // KOR
		} else if (curRegion == 0) {
			*(u32*)(0x02FFFD68) = 0x01; // JAP
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
	 || (strncmp(romTid, "KPF", 3) == 0 && (ndsHeader->fatSize == 0 || !extendedMemory))		// Pop Island: Paperfield
	 || (strncmp(romTid, "KGK", 3) == 0 && (ndsHeader->fatSize == 0 || !extendedMemory))		// Glory Days: Tactical Defense
	 || (strcmp(romTid, "NTRJ") == 0 && (ndsHeader->headerCRC16 == 0x53E2 || ndsHeader->headerCRC16 == 0x681E || ndsHeader->headerCRC16 == 0xCD01)) || srlFromPageFile)
	{
		*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 0x2;					// Boot Indicator (Cloneboot/Multiboot)
	}

	if (memcmp(romTid, "HND", 3) == 0 || memcmp(romTid, "HNE", 3) == 0) {
		*((u16*)(isSdk5(moduleParams) ? 0x02fffcfa : 0x027ffcfa)) = 0x1041;	// NoCash: channel ch1+7+13
		if (a9ScfgRom == 1 && REG_SCFG_ROM != 0x703) {
			*(u32*)0x03FFFFC8 = 0x7884;	// Fix sound pitch table for downloaded SDK5 SRL
		}
	}
}

int arm7_main(void) {
	// nocashMessage("bootloader");

	initMBK();
	
	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	// nocashMessage("Getting ARM7 to clear RAM...\n");
	resetMemory_ARM7();

	arm9_macroMode = macroMode;

	s2FlashcardId = *(u16*)(0x020000C0);

	s2RamAccessInit(true);

	// Init card
	if (!FAT_InitFiles(initDisc)) {
		// nocashMessage("!FAT_InitFiles");
		errorOutput();
		//return -1;
	}

	if (logging) {
		aFile logFile;
		getBootFileCluster(&logFile, "NDSBTSRP.LOG");
		enableDebug(&logFile);
	}

	*(vu32*)(0x02000000) = 0; // Clear debug RAM check flag

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
		// nocashMessage("fileCluster == CLUSTER_FREE");
		errorOutput();
		//return -1;
	}*/

	// Sav file
	aFile savFile;
	getFileFromCluster(&savFile, saveFileCluster);
	
	int errorCode;

	tDSiHeader dsiHeaderTemp;

	// Load the NDS file
	dbg_printf("Loading the NDS file...\n");

	loadBinary_ARM7(&dsiHeaderTemp, &romFile);
	if (dsiHeaderTemp.ndshdr.arm9binarySize == 0) {
		dbg_printf("ARM9 binary is empty!");
		errorOutput();
	}
	if (dsiHeaderTemp.ndshdr.arm7binarySize == 0) {
		dbg_printf("ARM7 binary is empty!");
		errorOutput();
	}

	// File containing cached patch offsets
	aFile patchOffsetCacheFile;
	getFileFromCluster(&patchOffsetCacheFile, patchOffsetCacheFileCluster);
	fileRead((char*)&patchOffsetCache, &patchOffsetCacheFile, 0, 4);
	if (patchOffsetCache.ver == patchOffsetCacheFileVersion
	 && patchOffsetCache.type == 1) {	// 0 = Regular, 1 = B4DS, 2 = HB
		fileRead((char*)&patchOffsetCache, &patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	} else {
		if (baseUnitCode > 0 || srlAddr == 0) pleaseWaitOutput();
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.type = 1;
	}

	s32 mainScreen = 0;
	fileRead((char*)&mainScreen, &patchOffsetCacheFile, 0x1FC, sizeof(u32));

	patchOffsetCacheFilePrevCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);
    dbg_printf("sdk_version: ");
    dbg_hexa(moduleParams->sdk_version);
    dbg_printf("\n"); 

	ndsHeader = loadHeader(&dsiHeaderTemp, moduleParams);
	const char* romTid = getRomTid(ndsHeader);

	if (!(accessControl & BIT(4)) && srlAddr == 0 && memcmp(romTid, "UBR", 3) != 0 && memcmp(romTid, "HND", 3) != 0 && memcmp(romTid, "HNE", 3) != 0 && srlAddr == 0 && (softResetParams[0] == 0 || softResetParams[0] == 0xFFFFFFFF)) {
		esrbOutput();
	}

	ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams, foundModuleParams);
	if (decrypt_arm9(&dsiHeaderTemp.ndshdr)) {
		dbg_printf("Secure area decrypted successfully");
	} else {
		dbg_printf("Secure area already decrypted");
	}
	dbg_printf("\n");

	my_readUserSettings(ndsHeader); // Header has to be loaded first

	baseChipID = getChipId(pkmnHeader ? (tNDSHeader*)NDS_HEADER_POKEMON : ndsHeader, moduleParams);

	if (cdcReadReg(CDC_SOUND, 0x22) == 0xF0) {
		// Switch touch mode to NTR
		*(vu16*)0x4004700 &= ~BIT(15); // Disable sound output: Runs before sound frequency change
		*(vu16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
		*(vu16*)0x4004700 |= BIT(15); // Enable sound output
		NDSTouchscreenMode();
		*(vu16*)0x4000500 = 0x807F;
	}

	REG_GPIO_WIFI |= BIT(8);	// Old NDS-Wifi mode

	// Switch to NTR mode BIOS
	REG_SCFG_ROM = 0x703;

	u32 clonebootFlag = 0;
	fileRead((char*)&clonebootFlag, &romFile, ((romSize-0x88) <= baseRomSize) ? (romSize-0x88) : baseRomSize, sizeof(u32));
	bool usesCloneboot = (clonebootFlag == 0x16361);
	if (usesCloneboot) {
		dbg_printf("Cloneboot detected\n");
	}

	// Calculate overlay pack size
	if (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) {
		for (u32 i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i < ndsHeader->arm7romOffset; i++) {
			overlaysSize++;
		}
	} else {
		for (u32 i = ndsHeader->arm9overlaySource; i < ndsHeader->arm7romOffset; i++) {
			overlaysSize++;
		}
	}
	if (ndsHeader->unitCode == 3) {
		// Calculate i-overlay pack size
		for (u32 i = (u32)dsiHeaderTemp.arm9iromOffset+dsiHeaderTemp.arm9ibinarySize; i < (u32)dsiHeaderTemp.arm7iromOffset; i++) {
			ioverlaysSize++;
		}
	}

	const bool dsBrowser = (strcmp(romTid, "UBRP") == 0);

	if (dsBrowser && extendedMemory && !dsDebugRam) {
		toncset((char*)0x0C400000, 0xFF, 0xC0);
		toncset((u8*)0x0C4000B2, 0, 3);
		toncset((u8*)0x0C4000B5, 0x24, 3);
		*(u16*)0x0C4000BE = 0x7FFF;
		toncset((char*)0x0C4000C0, 0, 0xE);
		*(u16*)0x0C4000CE = 0x7FFF;
		toncset((char*)0x0C4000D0, 0, 0x130);
	}

	*(vu16*)0x08240000 = 1;
	expansionPakFound = ((*(vu16*)0x08240000 == 1) && (s2FlashcardId != 0 || !dsBrowser));

	if (dsBrowser && s2FlashcardId != 0 && s2FlashcardId != 0x5A45) {
		toncset((char*)0x08000000, 0xFF, 0xC0);
		toncset((u8*)0x080000B2, 0, 3);
		toncset((u8*)0x080000B5, 0x24, 3);
		*(u16*)0x080000BE = 0x7FFF;
		toncset((char*)0x080000C0, 0, 0xE);
		*(u16*)0x080000CE = 0x7FFF;
		toncset((char*)0x080000D0, 0, 0x130);
	}

	// dbg_printf("Trying to patch the card...\n");

	ce9Location = *(u32*)CARDENGINE_ARM9_LOCATION_BUFFERED;
	ce9Alt = (ce9Location == CARDENGINE_ARM9_LOCATION_DLDI_ALT || ce9Location == CARDENGINE_ARM9_LOCATION_DLDI_ALT2);
	// const bool ce9NotInHeap = (ce9Alt || ce9Location == CARDENGINE_ARM9_LOCATION_DLDI_START);
	tonccpy((u32*)ce9Location, (u32*)CARDENGINE_ARM9_LOCATION_BUFFERED, ce9Alt ? 0x2800 : 0x3800);
	toncset((u32*)0x023E0000, 0, 0x10000);

	tonccpy((u8*)CARDENGINE_ARM7_LOCATION, (u8*)CARDENGINE_ARM7_LOCATION_BUFFERED, 0x1000);
	toncset((u8*)CARDENGINE_ARM7_LOCATION_BUFFERED, 0, 0x1000);

	if (!dldiPatchBinary((data_t*)ce9Location, 0x3800, (data_t*)(extendedMemory ? 0x027FC000 : ((accessControl & BIT(4)) && !ce9Alt) ? 0x023FC000 : 0x023FD000))) {
		dbg_printf("ce9 DLDI patch failed\n");
		errorOutput();
	}

	aFile musicsFile;
	getFileFromCluster(&musicsFile, musicCluster);
	if (((accessControl & BIT(4)) || arm7mbk == 0x080037C0) && musicCluster != 0) {
		if (strncmp(romTid, "DMF", 3) == 0) {
			dbg_printf("Photo/video found!\n");
		} else {
			dbg_printf("Music pack found!\n");
		}
	}

	const bool laterSdk = ((moduleParams->sdk_version >= 0x2008000 && moduleParams->sdk_version != 0x2012774) || moduleParams->sdk_version == 0x20029A8);
	bool wramUsed = false;
	u32 fatTableSize = 0;
	u32 fatTableSizeNoExp = !laterSdk ? 0x19C00 : 0x1A400;
	if (s2FlashcardId == 0x334D || s2FlashcardId == 0x3647 || s2FlashcardId == 0x4353) {
		fatTableAddr = (s2FlashcardId==0x4353 ? 0x09F7FE00 : 0x09F80000);
		fatTableSize = 0x80000;
	} else if (s2FlashcardId == 0x5A45 && !dsBrowser) {
		fatTableAddr = 0x08F80000;
		fatTableSize = 0x80000;
	} else if (expansionPakFound && !dsBrowser) {
		fatTableAddr = 0x09780000;
		fatTableSize = 0x80000;
	} else if (extendedMemory) {
		if (ndsHeader->unitCode > 0 && (u32)ndsHeader->arm9destination >= 0x02004000 && ((accessControl & BIT(4)) || arm7mbk == 0x080037C0)) {
			fatTableAddr = 0x02000000;
			fatTableSize = 0x4000;
		} else {
			fatTableAddr = 0x02700000;
			fatTableSize = 0x80000;
		}
	} else {
		if (laterSdk) {
			fatTableAddr = ce9Alt ? 0x023FF268 : 0x023FF200;
			fatTableSizeNoExp = ce9Alt ? 0x598 : 0x600;

			lastClusterCacheUsed = (u32*)0x037F8000;
			clusterCache = 0x037F8000;
			clusterCacheSize = 0x16780;

			wramUsed = true;
		} else {
			fatTableAddr = 0x023E0000;
		}
		fatTableSize = fatTableSizeNoExp;
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
			// const bool startMem = (!ce9NotInHeap && ndsHeader->unitCode > 0 && (u32)ndsHeader->arm9destination >= 0x02004000 && ((accessControl & BIT(4)) || arm7mbk == 0x080037C0) && romFile.fatTableCacheSize <= 0x4000);

			// if (ce9NotInHeap) {
				ce9AltLargeTable = ((romFile.fatTableCacheSize > fatTableSizeNoExp) && laterSdk);
				if (ce9AltLargeTable) {
					dbg_printf("\n");
					dbg_printf("Cluster cache is above 0x");
					dbg_printf(ce9Alt ? "598" : "600");
					dbg_printf(" bytes!\n");
					dbg_printf("Consider backing up and restoring the SD card contents to defragment it");
					if (*(u16*)0x4004700 == 0) {
						dbg_printf(",\nor insert an Expansion Pak\n");
					} else {
						dbg_printf("\n");
					}
					if (
						strncmp(romTid, "KII", 3) == 0 // 101 Pinball World
					||	strncmp(romTid, "KAT", 3) == 0 // AiRace: Tunnel
					||	strncmp(romTid, "K2Z", 3) == 0 // G.G Series: Altered Weapon
					||	strncmp(romTid, "KSR", 3) == 0 // Aura-Aura Climber
					||	strcmp(romTid, "KBEV") == 0 // Bejeweled Twist (Europe, Australia) (DSiWare)
					||	strncmp(romTid, "K9G", 3) == 0 // Big Bass Arcade
					||	strncmp(romTid, "KUG", 3) == 0 // G.G Series: Drift Circuit 2
					||	strncmp(romTid, "KEI", 3) == 0 // Electroplankton: Beatnes
					||	strncmp(romTid, "KEA", 3) == 0 // Electroplankton: Trapy
					||	strncmp(romTid, "KFO", 3) == 0 // Frenzic
					||	strncmp(romTid, "K5M", 3) == 0 // G.G Series: The Last Knight
					||	strncmp(romTid, "KPT", 3) == 0 // Link 'n' Launch
					||	strncmp(romTid, "CLJ", 3) == 0 // Mario & Luigi: Bowser's Inside Story
					||	strncmp(romTid, "KNP", 3) == 0 // Need for Speed: Nitro-X
					||	strncmp(romTid, "K9K", 3) == 0 // Nintendoji
					||	strncmp(romTid, "K6T", 3) == 0 // Orion's Odyssey
					||	strncmp(romTid, "KPS", 3) == 0 // Phantasy Star 0 Mini
					||	strncmp(romTid, "KHR", 3) == 0 // Picture Perfect: Pocket Stylist
					||	strncmp(romTid, "KS3", 3) == 0 // Shantae: Risky's Revenge
					||	strncmp(romTid, "VSO", 3) == 0 // Sonic Classic Collection
					||	strncmp(romTid, "KZU", 3) == 0 // Tales to Enjoy!: Little Red Riding Hood
					||	strncmp(romTid, "KZV", 3) == 0 // Tales to Enjoy!: Puss in Boots
					||	strncmp(romTid, "KZ7", 3) == 0 // Tales to Enjoy!: The Three Little Pigs
					||	strncmp(romTid, "KZ8", 3) == 0 // Tales to Enjoy!: The Ugly Duckling
					||	strncmp(romTid, "KZ2", 3) == 0 // G.G Series: Z-One 2
					) {
						// Game's heap cannot be shrunk, so display error
						errorOutput();
					}
					dbg_printf("\n");

					fatTableAddr = 0x023E0000;
					fatTableSizeNoExp = 0x20000;
					if (((u32)ndsHeader->arm9destination < 0x02004000) && ((accessControl & BIT(4)) || arm7mbk == 0x080037C0)) {
						fatTableAddr = CARDENGINE_ARM9_LOCATION_DLDI;
						fatTableSizeNoExp = 0x1A400;
					}

					const u32 cheatSizeTotal = cheatSize+(apPatchIsCheat ? apPatchSize : 0);
					if (fatTableAddr != CARDENGINE_ARM9_LOCATION_DLDI && cheatSizeTotal > 4) {
						fatTableAddr -= 0x2000;
						fatTableSizeNoExp -= 0x2000;
					}

					fatTableAddr -= romFile.fatTableCacheSize;
				}
			/* } else {
				if (startMem) {
					fatTableAddr = 0x02000000;
				} else {
					fatTableAddr -= romFile.fatTableCacheSize;
				}
			} */
			tonccpy((u32*)fatTableAddr, (u32*)0x037F8000, romFile.fatTableCacheSize);
			romFile.fatTableCache = (u32*)fatTableAddr;

			lastClusterCacheUsed = (u32*)0x037F8000;
			clusterCache = 0x037F8000;
			clusterCacheSize = (/* startMem ? 0x4000 : */ fatTableSizeNoExp)-romFile.fatTableCacheSize;

			// if (!startMem || (startMem && romFile.fatTableCacheSize < 0x4000)) {
				buildFatTableCacheCompressed(&savFile);
				if (savFile.fatTableCached) {
					// if (startMem || (ce9NotInHeap && !ce9AltLargeTable)) {
					if (!ce9AltLargeTable) {
						fatTableAddr += romFile.fatTableCacheSize;
					} else {
						fatTableAddr -= savFile.fatTableCacheSize;
					}
					tonccpy((u32*)fatTableAddr, (u32*)0x037F8000, savFile.fatTableCacheSize);
					savFile.fatTableCache = (u32*)fatTableAddr;
				}
				if (musicCluster != 0) {
					lastClusterCacheUsed = (u32*)0x037F8000;
					clusterCache = 0x037F8000;
					clusterCacheSize = (/* startMem ? 0x4000 : */ fatTableSizeNoExp)-savFile.fatTableCacheSize;

					buildFatTableCacheCompressed(&musicsFile);
					// if (startMem || (ce9NotInHeap && !ce9AltLargeTable)) {
					if (!ce9AltLargeTable) {
						fatTableAddr += savFile.fatTableCacheSize;
					} else {
						fatTableAddr -= musicsFile.fatTableCacheSize;
					}
					tonccpy((u32*)fatTableAddr, (u32*)0x037F8000, musicsFile.fatTableCacheSize);
					musicsFile.fatTableCache = (u32*)fatTableAddr;
				}
			// }
			if (/* (!startMem && !ce9NotInHeap) || */ ce9AltLargeTable) {
				fatTableAddr -= (fatTableAddr % 512); // Align end of heap to 512 bytes
			}
		} else {
			fatTableAddr = 0x023C0000;
			lastClusterCacheUsed = (u32*)fatTableAddr;
			clusterCache = fatTableAddr;
			clusterCacheSize = fatTableSize;

			buildFatTableCacheCompressed(&romFile);
			buildFatTableCacheCompressed(&savFile);
			if (musicCluster != 0) {
				buildFatTableCacheCompressed(&musicsFile);
			}
		}
	} else if (fatTableSize <= 0x20000) {
		buildFatTableCacheCompressed(&savFile);
		if (musicCluster != 0) {
			buildFatTableCacheCompressed(&musicsFile);
		}
	} else {
		buildFatTableCache(&savFile);
		if (musicCluster != 0) {
			buildFatTableCache(&musicsFile);
		}
	}

	ROMinRAM = isROMLoadableInRAM(&dsiHeaderTemp, &dsiHeaderTemp.ndshdr, romTid, moduleParams, usesCloneboot); // If possible, set to load ROM into RAM
	errorCode = patchCardNds(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		(cardengineArm9*)ce9Location,
		ndsHeader,
		moduleParams,
		1,
		usesCloneboot,
		saveFileCluster,
		saveSize
	);
	if (errorCode == ERR_NONE) {
		dbg_printf("Card patch successful\n\n");
	} else {
		dbg_printf("Card patch failed");
		errorOutput();
	}
	patchBinary((cardengineArm9*)ce9Location, ndsHeader, moduleParams);
	patchCardNdsArm9Cont((cardengineArm9*)ce9Location, ndsHeader, moduleParams);

	toncset((u32*)0x0380C000, 0, 0x2780);

	errorCode = hookNdsRetailArm7(
		(cardengineArm7*)CARDENGINE_ARM7_LOCATION,
		ndsHeader,
		moduleParams,
		cheatFileCluster,
		cheatSize,
		apPatchFileCluster,
		apPatchSize,
		mainScreen,
		language,
		getRumblePakType()
	);
	if (errorCode == ERR_NONE) {
		// dbg_printf("Card hook 7 successful\n\n");
	} else {
		// dbg_printf("Card hook 7 failed");
		errorOutput();
	}

	if (ROMinRAM) {
		loadROMintoRAM(ndsHeader, moduleParams, &romFile, usesCloneboot);
		if (ndsHeader->unitCode == 3) {
			loadIOverlaysintoRAM(&dsiHeaderTemp, &romFile, usesCloneboot);
		}
	} else {
		loadOverlaysintoFile(ndsHeader, moduleParams, &romFile);
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
		musicsFile.fatTableCompressed,
		patchOffsetCacheFileCluster,
		(u32)musicsFile.fatTableCache,
		ramDumpCluster,
		srParamsFileCluster,
		screenshotCluster,
		apFixOverlaysCluster,
		musicCluster,
		musicsSize,
		pageFileCluster,
		manualCluster,
		sharedFontCluster,
		expansionPakFound,
		extendedMemory,
		ROMinRAM,
		dsDebugRam,
		supportsExceptionHandler(romTid),
		mainScreen,
		usesCloneboot,
		overlaysSize,
		ioverlaysSize,
		arm9iromOffset,
		arm9ibinarySize,
		fatTableSize,
		fatTableAddr
	);
	/* if (errorCode == ERR_NONE) {
		dbg_printf("Card hook 9 successful\n\n");
	} else {
		dbg_printf("Card hook 9 failed");
		errorOutput();
	} */

	patchOffsetCacheFileNewCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
	if (patchOffsetCacheFileNewCrc != patchOffsetCacheFilePrevCrc) {
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
		if (applyIpsPatch(ndsHeader, (u8*)IMAGES_LOCATION, (*(u8*)(IMAGES_LOCATION+apPatchSize-1) == 0xA9), ROMinRAM, usesCloneboot)) {
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

	/* fileWrite((char*)ndsHeader->arm9destination, &pageFile, 0x14000, iUncompressedSize);
	fileWrite((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, newArm7binarySize);
	fileWrite((char*)CHEAT_ENGINE_LOCATION_B4DS, &pageFile, 0x2FE000, 0x2000); */
	fileWrite((char*)&iUncompressedSize, &pageFile, 0x3FFFF0, sizeof(u32));
	fileWrite((char*)&newArm7binarySize, &pageFile, 0x3FFFF4, sizeof(u32));

	clearScreen();

	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);

	// dbg_printf("Starting the NDS file...");
    setMemoryAddress(ndsHeader, moduleParams, romFile);

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A))) {		// Dump RAM
		aFile ramDumpFile;
		getFileFromCluster(&ramDumpFile, ramDumpCluster);
		if (extendedMemory) {
			fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x7E0000);
			fileWrite((char*)(isSdk5(moduleParams) ? 0x02FE0000 : 0x027E0000), &ramDumpFile, 0x7E0000, 0x20000);
		} else {
			fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x400000);
		}
	}

	REG_SCFG_EXT = 0x12A03000;

	startBinary_ARM7();

	return 0;
}
