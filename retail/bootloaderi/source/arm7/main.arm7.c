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
#include <nds/card.h>
#include <nds/system.h>
#include <nds/bios.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>
#include <nds/memory.h> // tNDSHeader
#include <nds/arm7/i2c.h>
#include <nds/debug.h>
#include <nds/ipc.h>

#define BASE_DELAY (100)

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
#include "hook.h"
#include "common.h"
#include "locations.h"
#include "value_bits.h"
#include "loading_screen.h"

#define cacheBlockSize 0x4000

//#define memcpy __builtin_memcpy

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

extern void arm7clearRAM(void);
extern void arm7code(u32* addr);

extern bool moreMemory;

//extern u32 _start;
extern u32 storedFileCluster;
extern u32 initDisc;
extern u16 bootstrapOnFlashcard;
extern u8 gameOnFlashcard;
extern u8 saveOnFlashcard;
extern u16 a9ScfgRom;
extern u8 dsiSD;
extern u32 saveFileCluster;
extern u32 gbaFileCluster;
extern u32 gbaSaveFileCluster;
extern u32 romSize;
extern u32 saveSize;
extern u32 gbaRomSize;
extern u32 gbaSaveSize;
extern u32 dataToPreloadAddr[2];
extern u32 dataToPreloadSize[2];
extern u32 wideCheatFileCluster;
extern u32 wideCheatSize;
extern u32 apPatchFileCluster;
extern u32 apPatchSize;
extern u32 cheatFileCluster;
extern u32 cheatSize;
extern u32 patchOffsetCacheFileCluster;
extern u32 ramDumpCluster;
extern u32 srParamsFileCluster;
extern u32 screenshotCluster;
extern u32 apFixOverlaysCluster;
extern u32 pageFileCluster;
extern u32 manualCluster;
extern u32 sharedFontCluster;
extern u8 patchMpuSize;
extern u8 patchMpuRegion;
extern u8 language;
extern u8 region;
extern u8 dsiMode; // SDK 5
extern u8 donorSdkVer;
extern u8 extendedMemory;
extern u8 consoleModel;
extern u8 romRead_LED;
extern u8 dmaRomRead_LED;
extern u8 soundFreq;

bool useTwlCfg = false;
u8 twlCfgCountry = 0;
int twlCfgLang = 0;
int sharedFontRegion = 0;
u8 wifiLedState = 0;

//bool gbaRomFound = false;

static u32 ce7Location = CARDENGINEI_ARM7_LOCATION;
static u32 ce9Location = CARDENGINEI_ARM9_LOCATION;
u32 overlaysSize = 0;
u32 ioverlaysSize = 0;
bool overlayPatch = false;
bool overlaysInRam = false;

static u32 softResetParams[4] = {0};
u32 srlAddr = 0;
u16 baseHeaderCRC = 0;
u16 baseSecureCRC = 0;
u32 baseRomSize = 0;
u32 baseChipID = 0;
bool pkmnHeader = false;

u32 newArm7binarySize = 0;
u32 newArm7ibinarySize = 0;
u32 oldArm7mbk = 0;

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

static void initMBK_dsiMode(void) {
	// This function has no effect with ARM7 SCFG locked
	*(vu32*)REG_MBK1 = *(u32*)0x02FFE180;
	*(vu32*)REG_MBK2 = *(u32*)0x02FFE184;
	*(vu32*)REG_MBK3 = *(u32*)0x02FFE188;
	*(vu32*)REG_MBK4 = *(u32*)0x02FFE18C;
	*(vu32*)REG_MBK5 = *(u32*)0x02FFE190;
	REG_MBK6 = *(u32*)0x02FFE1A0;
	REG_MBK7 = *(u32*)0x02FFE1A4;
	REG_MBK8 = *(u32*)0x02FFE1A8;
	REG_MBK9 = *(u32*)0x02FFE1AC;
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
	int i, reg;

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
		for(reg=0; reg<0x1c; reg+=4)*((vu32*)(0x04004104 + ((i*0x1c)+reg))) = 0;//Reset NDMA.
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	//if(dsiMode) {
		memset_addrs_arm7(0x03000000, 0x0380FFC0);
		memset_addrs_arm7(0x0380FFD0, 0x03800000 + 0x10000);
	/*} else {
		memset_addrs_arm7(0x03800000 - 0x8000, 0x03800000 + 0x10000);
	}*/

	memset_addrs_arm7(0x02004000, IMAGES_LOCATION);	// clear part of EWRAM - except before nds-bootstrap images
	toncset((u32*)0x02380000, 0, 0x3F000);		// clear part of EWRAM - except before 0x023C0000, which has the arm9 code
	toncset((u32*)0x023C0000, 0, 0x40000);		// clear part of EWRAM
	memset_addrs_arm7(0x02700000, BLOWFISH_LOCATION);		// clear part of EWRAM - except before ce7 and ce9 binaries
	toncset((u32*)0x027F8000, 0, 0x8000);	// clear part of EWRAM
	memset_addrs_arm7(0x02800000, 0x02E80000);
	memset_addrs_arm7(0x02F80000, 0x02FFD7BC); // Leave eMMC CID intact
	memset_addrs_arm7(0x02FFD7CC, 0x02FFE000);
	toncset((u32*)0x02FFF000, 0, 0xD60);		// clear part of EWRAM
	toncset32((u32*)0x02FFFDFC, 0, 1);		// clear TWLCFG address
	toncset((u32*)0x02FFFE00, 0, 0x200);		// clear part of EWRAM: header
	REG_IE = 0;
	REG_IF = ~0;
	REG_AUXIE = 0;
	REG_AUXIF = ~0;
	*(vu32*)0x0380FFFC = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)0x0380FFF8 = 0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff

	useTwlCfg = ((*(u8*)0x02000400 != 0) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (*(u8*)0x02000448 != 0));
	twlCfgCountry = *(u8*)0x02000405;
	twlCfgLang = *(u8*)0x02000406;
	if (useTwlCfg) {
		/*if (twlCfgCountry == 0x01 || (twlCfgCountry >= 0x08 && twlCfgCountry <= 0x34) || twlCfgCountry == 0x99 || twlCfgCountry == 0xA8 || (twlCfgCountry >= 0x40 && twlCfgCountry <= 0x70) || twlCfgCountry == 0x41 || twlCfgCountry == 0x5F) {
			sharedFontRegion = 0;	// Japan/USA/Europe/Australia
		} else*/ if (twlCfgCountry == 0xA0) {
			sharedFontRegion = 1;	// China
		} else if (twlCfgCountry == 0x88) {
			sharedFontRegion = 2;	// Korea
		} else {
			sharedFontRegion = -1;
		}
		tonccpy((u8*)0x02FFD400, (u8*)0x02000400, 0x128); // Duplicate TWLCFG, in case it gets overwritten
	} else {
		sharedFontRegion = -1;
	}
}

void my_enableSlot1() {
	while((REG_SCFG_MC & 0x0c) == 0x0c) swiDelay(1 * BASE_DELAY);

	if(!(REG_SCFG_MC & 0x0c)) {

		REG_SCFG_MC = (REG_SCFG_MC & ~0x0c) | 4;
		swiDelay(10 * BASE_DELAY);
		REG_SCFG_MC = (REG_SCFG_MC & ~0x0c) | 8;
		swiDelay(10 * BASE_DELAY);
	}
	// IR enable
	REG_AUXSPICNT = CARD_CR1_ENABLE|CARD_CR1_IRQ;
	REG_ROMCTRL = 0x20000000;
}

void my_disableSlot1() {
	while((REG_SCFG_MC & 0x0c) == 0x0c) swiDelay(1 * BASE_DELAY);

	if((REG_SCFG_MC & 0x0c) == 8) {

		REG_SCFG_MC = (REG_SCFG_MC & ~0x0c) | 0x0c;
		while((REG_SCFG_MC & 0x0c) != 0) swiDelay(1 * BASE_DELAY);
	}
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

static void DSiTouchscreenMode(void) {
	// Touchscreen
	cdcWriteReg(0, 0x01, 0x01);
	cdcWriteReg(0, 0x39, 0x66);
	cdcWriteReg(1, 0x20, 0x16);
	cdcWriteReg(0, 0x04, 0x00);
	cdcWriteReg(0, 0x12, 0x81);
	cdcWriteReg(0, 0x13, 0x82);
	cdcWriteReg(0, 0x51, 0x82);
	cdcWriteReg(0, 0x51, 0x00);
	cdcWriteReg(0, 0x04, 0x03);
	cdcWriteReg(0, 0x05, 0xA1);
	cdcWriteReg(0, 0x06, 0x15);
	cdcWriteReg(0, 0x0B, 0x87);
	cdcWriteReg(0, 0x0C, 0x83);
	cdcWriteReg(0, 0x12, 0x87);
	cdcWriteReg(0, 0x13, 0x83);
	cdcWriteReg(3, 0x10, 0x88);
	cdcWriteReg(4, 0x08, 0x7F);
	cdcWriteReg(4, 0x09, 0xE1);
	cdcWriteReg(4, 0x0A, 0x80);
	cdcWriteReg(4, 0x0B, 0x1F);
	cdcWriteReg(4, 0x0C, 0x7F);
	cdcWriteReg(4, 0x0D, 0xC1);
	cdcWriteReg(0, 0x41, 0x08);
	cdcWriteReg(0, 0x42, 0x08);
	cdcWriteReg(0, 0x3A, 0x00);
	cdcWriteReg(4, 0x08, 0x7F);
	cdcWriteReg(4, 0x09, 0xE1);
	cdcWriteReg(4, 0x0A, 0x80);
	cdcWriteReg(4, 0x0B, 0x1F);
	cdcWriteReg(4, 0x0C, 0x7F);
	cdcWriteReg(4, 0x0D, 0xC1);
	cdcWriteReg(1, 0x2F, 0x2B);
	cdcWriteReg(1, 0x30, 0x40);
	cdcWriteReg(1, 0x31, 0x40);
	cdcWriteReg(1, 0x32, 0x60);
	cdcWriteReg(0, 0x74, 0x82);
	cdcWriteReg(0, 0x74, 0x92);
	cdcWriteReg(0, 0x74, 0xD2);
	cdcWriteReg(1, 0x21, 0x20);
	cdcWriteReg(1, 0x22, 0xF0);
	cdcWriteReg(0, 0x3F, 0xD4);
	cdcWriteReg(1, 0x23, 0x44);
	cdcWriteReg(1, 0x1F, 0xD4);
	cdcWriteReg(1, 0x28, 0x4E);
	cdcWriteReg(1, 0x29, 0x4E);
	cdcWriteReg(1, 0x24, 0x9E);
	cdcWriteReg(1, 0x25, 0x9E);
	cdcWriteReg(1, 0x20, 0xD4);
	cdcWriteReg(1, 0x2A, 0x14);
	cdcWriteReg(1, 0x2B, 0x14);
	cdcWriteReg(1, 0x26, 0xA7);
	cdcWriteReg(1, 0x27, 0xA7);
	cdcWriteReg(0, 0x40, 0x00);
	cdcWriteReg(0, 0x3A, 0x60);

	// Finish up!
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x00);
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
	return (baseRomSize+0x88) - ndsHeader->arm7romOffset - ndsHeader->arm7binarySize + overlaysSize;
}

static inline u32 getIRomSizeNoArmBins(const tDSiHeader* dsiHeader) {
	return (u32)dsiHeader->arm9iromOffset - dsiHeader->ndshdr.arm7romOffset - dsiHeader->ndshdr.arm7binarySize + overlaysSize;
}*/

// SDK 5
static bool ROMsupportsDsiMode(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode > 0);
}

// SDK 5
/*static bool ROMisDsiEnhanced(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode == 0x02);
}

// SDK 5
static bool ROMisDsiExclusive(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode == 0x03);
}*/

static void loadBinary_ARM7(const tDSiHeader* dsiHeaderTemp, aFile file) {
	nocashMessage("loadBinary_ARM7");

	//u32 ndsHeader[0x170 >> 2];
	//u32 dsiHeader[0x2F0 >> 2]; // SDK 5

	// Read DSi header (including NDS header)
	//fileRead((char*)ndsHeader, file, 0, 0x170, 3);
	//fileRead((char*)dsiHeader, file, 0, 0x2F0, 2); // SDK 5
	bool separateSrl = (softResetParams[2] == 0x44414F4C); // 'LOAD'
	if (separateSrl) {
		srlAddr = 0xFFFFFFFF;
		aFile pageFile = getFileFromCluster(pageFileCluster, bootstrapOnFlashcard);

		fileRead((char*)dsiHeaderTemp, pageFile, 0x2BFE00, 0x160, 0);
		fileRead(dsiHeaderTemp->ndshdr.arm9destination, pageFile, 0, dsiHeaderTemp->ndshdr.arm9binarySize, 0);
		fileRead(dsiHeaderTemp->ndshdr.arm7destination, pageFile, 0x2C0000, dsiHeaderTemp->ndshdr.arm7binarySize, 0);
	} else {
		srlAddr = softResetParams[3];
		fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp), 0);
		if (srlAddr > 0 && ((u32)dsiHeaderTemp->ndshdr.arm9destination < 0x02000000 || (u32)dsiHeaderTemp->ndshdr.arm9destination > 0x02004000)) {
			// Invalid SRL
			srlAddr = 0;
			fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp), 0);
		}

		// Load binaries into memory
		fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize, 0);
		if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION == 0) {
			fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize, 0);
		}
	}

	char baseTid[5] = {0};
	fileRead((char*)&baseTid, file, 0xC, 4, 0);
	if (
		strncmp(baseTid, "ADA", 3) == 0    // Diamond
		|| strncmp(baseTid, "APA", 3) == 0 // Pearl
		|| strncmp(baseTid, "CPU", 3) == 0 // Platinum
		|| strncmp(baseTid, "IPK", 3) == 0 // HG
		|| strncmp(baseTid, "IPG", 3) == 0 // SS
	) {
		// Fix Pokemon games needing header data.
		tNDSHeader* ndsHeaderPokemon = (tNDSHeader*)NDS_HEADER_POKEMON;
		fileRead((char*)ndsHeaderPokemon, file, 0, 0x160, 0);
		pkmnHeader = true;
	}

	fileRead((char*)&baseHeaderCRC, file, 0x15E, sizeof(u16), 0);
	fileRead((char*)&baseSecureCRC, file, 0x6C, sizeof(u16), 0);
	fileRead((char*)&baseRomSize, file, 0x80, sizeof(u32), 0);

	/*isGSDD = (strncmp(romTid, "BO5", 3) == 0)			// Golden Sun: Dark Dawn
        || (strncmp(romTid, "TBR", 3) == 0)			    // Disney Pixar Brave 
        ;*/
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

static bool isROMLoadableInRAM(const tDSiHeader* dsiHeader, const tNDSHeader* ndsHeader, const char* romTid, const module_params_t* moduleParams, const bool usesCloneboot) {
	/*dbg_printf("Console model: ");
	dbg_hexa(consoleModel);
	dbg_printf("\nromTid: ");
	dbg_printf(romTid);
	dbg_printf("\n");*/

	bool res = false;
	if ((strncmp(romTid, "UBR", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "KPP", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "KPF", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "UBR", 3) != 0
	 && strncmp(romTid, "KPP", 3) != 0
	 && strncmp(romTid, "KPF", 3) != 0)
	) {
		u32 romSize = (baseRomSize-0x8000)+0x88;
		if (!usesCloneboot) {
			romSize = (baseRomSize - ndsHeader->arm9binarySize);
			romSize -= ndsHeader->arm9romOffset;
		}
		res = ((dsiModeConfirmed && consoleModel>0 && ROMsupportsDsiMode(ndsHeader) && (usesCloneboot ? ((u32)dsiHeader->arm9iromOffset-0x8000) : ((u32)dsiHeader->arm9iromOffset-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize))+ioverlaysSize <= 0x00F80000)
			|| (isSdk5(moduleParams) && consoleModel>0 && romSize <= 0x01000000)
			|| (!dsiModeConfirmed && !isSdk5(moduleParams) && consoleModel>0 && romSize <= 0x01800000)
			|| (!dsiModeConfirmed && isSdk5(moduleParams) && consoleModel==0 && romSize <= 0x007E0000)
			|| (!dsiModeConfirmed && !isSdk5(moduleParams) && consoleModel==0 && romSize <= 0x00800000));

	  if (!res && extendedMemory && !dsiModeConfirmed
	  && strncmp(romTid, "UOR", 3) != 0
	  && strncmp(romTid, "UXB", 3) != 0
	  ) {
	    u32 romSizeLimit = (consoleModel==0 ? (extendedMemory==2 ? 0x00C76000 : 0x00BF6000) : (extendedMemory==2 ? 0x01C76000 : 0x01BF6000));
		res = (romSize <= romSizeLimit);
		extendedMemoryConfirmed = res;
		moreMemory = (dsiWramAccess && romSize > (consoleModel==0 ? 0x00BFE000 : 0x01BFE000));
	  }
	}
	/*if ((strncmp(romTid, "HND", 3) == 0)
	|| (strncmp(romTid, "HNE", 3) == 0))
	{
		extendedMemoryConfirmed = true;
		return extendedMemoryConfirmed;
	}*/
	return res;
}

static tNDSHeader* loadHeader(tDSiHeader* dsiHeaderTemp, const module_params_t* moduleParams, bool dsiModeConfirmed) {
	tNDSHeader* ndsHeader = (tNDSHeader*)(isSdk5(moduleParams) ? NDS_HEADER_SDK5 : NDS_HEADER);
	/*if (isGSDD) {
		ndsHeader = (tNDSHeader*)(NDS_HEADER_4MB);
	}*/

	// Copy the header to its proper location
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, (char*)ndsHeader, 0x170);
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, ndsHeader, sizeof(dsiHeaderTemp.ndshdr));
	*ndsHeader = dsiHeaderTemp->ndshdr;
	if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
		//dmaCopyWords(3, &dsiHeaderTemp, ndsHeader, sizeof(dsiHeaderTemp));
		//*(tDSiHeader*)ndsHeader = *dsiHeaderTemp;
		tDSiHeader* dsiHeader = (tDSiHeader*)(isSdk5(moduleParams) ? DSI_HEADER_SDK5 : DSI_HEADER); // __DSiHeader
		*dsiHeader = *dsiHeaderTemp;

		//*(u32*)(dsiHeader+0x1B8) |= BIT(18);	// SD access

		//toncset((char*)dsiHeader+0x220, 0, 0x10);	// Clear modcrypt bytes

		/*if (!isDSiWare) {
			// Clear out Digest offsets/lengths
			toncset((char*)dsiHeader+0x1E0, 0, 0x28);
		}*/
	}

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

	if (useTwlCfg && language == 0xFF) {
		language = twlCfgLang;
	}

	if (language >= 0 && language <= 7) {
		// Change language
		personalData->language = language; //*(u8*)((u32)ndsHeader - 0x11C) = language;
		if (ROMsupportsDsiMode(ndsHeader) && (u32)ndsHeader->arm9destination >= 0x02000800) {
			*(u8*)0x02000406 = language;
			*(u8*)0x02FFD406 = language;
		}
	}

	if (personalData->language != 6 && ndsHeader->reserved1[8] == 0x80) {
		ndsHeader->reserved1[8] = 0;	// Patch iQue game to be region-free
		ndsHeader->headerCRC16 = swiCRC16(0xFFFF, ndsHeader, 0x15E);	// Fix CRC
	}
}

static void NTR_BIOS() {
	// Switch to NTR mode BIOS (no effect with locked ARM7 SCFG)
	REG_SCFG_ROM = 0x703;
	if (REG_SCFG_ROM == 0x703) {
		dbg_printf("Switched to NTR mode BIOS\n");
	}
}

static void loadROMPartIntoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile file) {
	if (dataToPreloadSize[0] == 0 || (dataToPreloadSize[0]/*+dataToPreloadSize[1]*/) >= (consoleModel > 0 ? (isSdk5(moduleParams) || dsiModeConfirmed ? 0xF00000 : 0x1700000) : (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed ? 0x400000 : 0x700000))) {
		return;
	}

	u32 dataLocation = (u32)((consoleModel > 0 && (isSdk5(moduleParams) || dsiModeConfirmed)) ? ROM_SDK5_LOCATION : ROM_LOCATION);
	fileRead((char*)dataLocation, file, dataToPreloadAddr[0], dataToPreloadSize[0], 0);
	/*if (dataToPreloadSize[1] > 0) {
		fileRead((char*)dataLocation+dataToPreloadSize[0], file, dataToPreloadAddr[1], dataToPreloadSize[1], 0);
	}*/
	dbg_printf("Part of ROM pre-loaded into RAM\n");
}

static void loadOverlaysintoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile file) {
	if (!overlayPatch) {
		return;
	}

	// Load overlays into RAM
	if (overlaysSize <= 0x700000) {
		u32 overlaysLocation = CACHE_ADRESS_START;
		u32 alignedOverlaysOffset = ((ndsHeader->arm9romOffset + ndsHeader->arm9binarySize)/cacheBlockSize)*cacheBlockSize;
		u32 newOverlaysSize = 0;
		for (u32 i = alignedOverlaysOffset; i < ndsHeader->arm7romOffset; i+= cacheBlockSize) {
			newOverlaysSize += cacheBlockSize;
		}

		fileRead((char*)overlaysLocation, file, alignedOverlaysOffset, newOverlaysSize, 0);

		if (!isSdk5(moduleParams) && *(u32*)((overlaysLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+0x003128AC) == 0x4B434148) {
			*(u32*)((overlaysLocation-ndsHeader->arm9romOffset-ndsHeader->arm9binarySize)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday (before Rev 11)
		}

		overlaysInRam = true;
	}
}

static void loadIOverlaysintoRAM(const tDSiHeader* dsiHeader, aFile file, const bool usesCloneboot) {
	// Load overlays into RAM
	if (ioverlaysSize>0x700000) return;

	u32 romOffset = usesCloneboot ? 0x8000 : (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
	fileRead((char*)ROM_SDK5_LOCATION+((u32)dsiHeader->arm9iromOffset-romOffset), file, (u32)dsiHeader->arm9iromOffset+dsiHeader->arm9ibinarySize, ioverlaysSize, 0);
}

static void loadROMintoRAM(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, aFile* romFile, aFile* savFile, const bool usesCloneboot) {
	// Load ROM into RAM
	u32 romLocation = (u32)((consoleModel > 0 && (isSdk5(moduleParams) || dsiModeConfirmed)) ? ROM_SDK5_LOCATION : ROM_LOCATION);
	if (extendedMemoryConfirmed) {
		romLocation = (u32)((moduleParams->sdk_version < 0x2008000) ? ROM_LOCATION_EXT_SDK2 : ROM_LOCATION_EXT);
	}

	u32 romOffset = 0x8000;
	u32 romSizeEdit = (baseRomSize-0x8000)+0x88;
	if (!usesCloneboot) {
		romOffset = ndsHeader->arm9romOffset + ndsHeader->arm9binarySize;
		romSizeEdit = (baseRomSize - ndsHeader->arm9binarySize);
		romSizeEdit -= ndsHeader->arm9romOffset;
	}
	u32 romSizeLimit = (consoleModel==0 ? 0x00C00000 : 0x01C00000);
	if (extendedMemoryConfirmed) {
		romSizeLimit -= 0x2000;
		u32 romSizeLimitTilA7 = ((isSdk5(moduleParams) && ndsHeader->unitCode > 0) ? 0x00BFE000 : 0x003FE000);
		u32 romIncr = ((moduleParams->sdk_version < 0x2008000) ? 0x42000 : 0x20000);
		if (isSdk5(moduleParams) && ndsHeader->unitCode == 0) {
			romIncr -= 0x1000;
		}
		if (esrbScreenPrepared) {
			while (!esrbImageLoaded) {
				while (REG_VCOUNT != 191);
				while (REG_VCOUNT == 191);
			}
		}
		tonccpy((char*)IMAGES_LOCATION, romFile->fatTableCache, romFile->fatTableCacheSize > 0x48000 ? 0x48000 : romFile->fatTableCacheSize);
		//tonccpy((char*)ce7Location+0x11C00, savFile->fatTableCache, 0x2E000);
		romFile->fatTableCache = (u32*)((char*)IMAGES_LOCATION);
		//savFile->fatTableCache = (u32*)((char*)ce7Location+0x11C00);

		if (romSizeEdit <= romSizeLimitTilA7) {
			fileRead((char*)romLocation, *romFile, romOffset, romSizeEdit, 0);
		} else {
			fileRead((char*)romLocation, *romFile, romOffset, romSizeLimitTilA7, 0);
			if (isSdk5(moduleParams) && ndsHeader->unitCode == 0 && consoleModel > 0) {
				fileRead((char*)romLocation+romSizeLimitTilA7+romIncr, *romFile, romOffset + romSizeLimitTilA7, 0x800000, 0);
				fileRead((char*)romLocation+romSizeLimitTilA7+romIncr+0x801000, *romFile, romOffset + romSizeLimitTilA7 + 0x800000, romSizeEdit-romSizeLimitTilA7-0x800000, 0);
			} else {
				fileRead((char*)romLocation+romSizeLimitTilA7+romIncr, *romFile, romOffset + romSizeLimitTilA7, romSizeEdit-romSizeLimitTilA7, 0);
			}
			if (romSizeEdit > romSizeLimit) {
				fileRead((char*)ROM_LOCATION_EXT_P2, *romFile, romOffset + romSizeLimit, romSizeEdit-romSizeLimit, 0);
			}
		}
		if (!isSdk5(moduleParams) && *(u32*)((romLocation-romOffset)+0x003128AC) == 0x4B434148) {
			*(u32*)((romLocation-romOffset)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday (before Rev 11)
		}
		romFile->fatTableCached = false;
		savFile->fatTableCached = false;
	} else {
		fileRead((char*)romLocation, *romFile, romOffset, romSizeEdit, 0);
		if (!isSdk5(moduleParams) && *(u32*)((romLocation-romOffset)+0x003128AC) == 0x4B434148) {
			*(u32*)((romLocation-romOffset)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday (before Rev 11)
		}
		toncset((char*)IMAGES_LOCATION, 0, 0x8000);
		toncset((char*)IMAGES_LOCATION+0x8000, 0xFF, 0x8000);
		u32 offsetEnd = (romSize > (romSizeLimit-0x400000)+0x8000) ? (romSizeLimit-0x400000)+0x8000 : romSize;
		for (u32 offset = baseRomSize+(usesCloneboot ? 0x88 : 0); offset < offsetEnd; offset += 0x8000) { // Load more data beyond ROM size in header (Some ROM hacks may need this)
			toncset((char*)IMAGES_LOCATION+0x10000, 0, 0x8000); // Clear leftover data
			fileRead((char*)IMAGES_LOCATION+0x10000, *romFile, offset, 0x8000, 0);
			if ((memcmp((char*)IMAGES_LOCATION+0x10000, (char*)IMAGES_LOCATION, 0x8000) != 0) && (memcmp((char*)IMAGES_LOCATION+0x10000, (char*)IMAGES_LOCATION+0x8000, 0x8000) != 0)) {
				// Data is found
				tonccpy((char*)(romLocation-romOffset)+offset, (char*)IMAGES_LOCATION+0x10000, 0x8000);
			} else {
				// Padding detected
				break;
			}
		}
	}
	overlaysInRam = true;

	dbg_printf("ROM pre-loaded into RAM\n");
	if (extendedMemoryConfirmed) {
		dbg_printf("Complete ");
		if (romSizeEdit >= romSizeLimit) {
			dbg_printf(consoleModel==0 ? "12.5MB" : "28.5MB");
		} else {
			dbg_printf(consoleModel==0 ? "12MB" : "28MB");
		}
		dbg_printf(" used\n");
	}
	dbg_printf("\n");
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

static void setMemoryAddress(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (ROMsupportsDsiMode(ndsHeader)) {
		if (isDSiWare && !(REG_SCFG_ROM & BIT(9))) {
			u32* deviceListAddr = (u32*)(*(u32*)0x02FFE1D4);

			dbg_printf("Device list address: ");
			dbg_hexa((u32)deviceListAddr);
			dbg_printf("\n");

			tonccpy(deviceListAddr, (u32*)0x02EFF000, 0x400);
			toncset((u32*)0x02EFF000, 0, 0x400);
			/*const char *ndsPath = "sdmc:/DSIWARE.NDS";
			const char *pubPath = "sdmc:/DSIWARE.PUB";
			tonccpy((u8*)deviceListAddr+0x1B8, pubPath, 18);
			tonccpy((u8*)deviceListAddr+0x3C0, ndsPath, 18);*/
		}

		tonccpy((u32*)0x02FFC000, (u32*)DSI_HEADER_SDK5, 0x1000);	// Make a duplicate of DSi header

		if (gameOnFlashcard || !isDSiWare) {
			tonccpy((u32*)0x02FFFA80, (u32*)NDS_HEADER_SDK5, 0x160);	// Make a duplicate of DS header
		}

		/* *(u32*)(0x02FFA680) = 0x02FD4D80;
		*(u32*)(0x02FFA684) = 0x00000000;
		*(u32*)(0x02FFA688) = 0x00001980;

		*(u32*)(0x02FFF00C) = 0x0000007F;
		*(u32*)(0x02FFF010) = 0x550E25B8;
		*(u32*)(0x02FFF014) = 0x02FF4000; */

		if (!useTwlCfg) {
			// Reconstruct TWLCFG
			u8* twlCfg = (u8*)0x02000400;
			if ((u32)ndsHeader->arm9destination < 0x02000800) {
				twlCfg = (u8*)0x02FFD400;
			}
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

			dbg_printf("TWLCFG reconstructed\n");
		}

		u8* twlCfg = (u8*)0x02000400;
		if ((u32)ndsHeader->arm9destination < 0x02000800) {
			twlCfg = (u8*)0x02FFD400;
			*(u32*)0x02FFFDFC = 0x02FFD400;
		}
		u32 configFlags = useTwlCfg ? (*(u32*)((u32)twlCfg)) : 0x0100000F;
		if (consoleModel < 2) {
			if (wifiLedState == 0 || wifiLedState == 0x12) {
				configFlags &= ~BIT(3); // Clear WiFi Enable flag
			} else {
				configFlags |= BIT(3);
			}
		}

		toncset32(twlCfg, configFlags, 1); // Config Flags
		tonccpy(twlCfg+0x10, (u8*)0x02FFE20E, 1); // EULA Version (0=None/CountryChanged, 1=v1)
		tonccpy(twlCfg+0x9C, (u8*)0x02FFE2F0, 1); // Parental Controls Years of Age Rating (00h..14h)

		if (consoleModel == 0 && memcmp(ndsHeader->gameCode, "DD3", 3) == 0) {
			// Relocate TWLCFG for Hidden Photo
			*(u32*)0x02FFFDFC = 0x02FFD400;
		} else if (*(u32*)0x02FFFDFC != 0x02FFD400) {
			toncset((u32*)0x02FFD400, 0, 0x128);
		}

		// Set region flag
		if (useRomRegion && ndsHeader->gameCode[3] != 'A' && ndsHeader->gameCode[3] != 'O') {
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
		} else if (region == 0xFF && twlCfgCountry != 0) {
			u8 newRegion = 0;
			if (twlCfgCountry == 0x01) {
				newRegion = 0;	// Japan
			} else if (twlCfgCountry == 0xA0) {
				newRegion = 4;	// China
			} else if (twlCfgCountry == 0x88) {
				newRegion = 5;	// Korea
			} else if (twlCfgCountry == 0x41 || twlCfgCountry == 0x5F) {
				newRegion = 3;	// Australia
			} else if ((twlCfgCountry >= 0x08 && twlCfgCountry <= 0x34) || twlCfgCountry == 0x99 || twlCfgCountry == 0xA8) {
				newRegion = 1;	// USA
			} else if (twlCfgCountry >= 0x40 && twlCfgCountry <= 0x70) {
				newRegion = 2;	// Europe
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

		if (dsiModeConfirmed) {
			i2cWriteRegister(I2C_PM, I2CREGPM_MMCPWR, 1);		// Have IRQ check for power button press
			i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);		// SDK 5 --> Bootflag = Warmboot/SkipHealthSafety
		} else {
			toncset((u32*)0x02FFFD60, 0, 0xA0);
		}
	} else if (isSdk5(moduleParams)) {
		toncset((u32*)0x02FFFD60, 0, 0xA0);
	}

	if (!gameOnFlashcard && isDSiWare) {
		*(u16*)(0x02FFFC40) = 3;						// Boot Indicator (NAND/SD)
		return;
	}

    dbg_printf("chipID: ");
    dbg_hexa(baseChipID);
    dbg_printf("\n"); 

    // TODO
    // figure out what is 0x027ffc10, somehow related to cardId check
    //*((u32*)(isSdk5(moduleParams) ? 0x02fffc10 : 0x027ffc10)) = 1;

	/*if (isGSDD) {
		// Set memory values expected by loaded NDS
		// from NitroHax, thanks to Chism
		*((u32*)0x023ff800) = chipID;					// CurrentCardID
		*((u32*)0x023ff804) = chipID;					// Command10CardID
		*((u32*)0x023ffc00) = chipID;					// 3rd chip ID
		*((u32*)0x023ffc04) = chipID;					// 4th chip ID
		*((u16*)0x023ff808) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
		*((u16*)0x023ff80a) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]
		*((u16*)0x023ffc08) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
		*((u16*)0x023ffc0a) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]
		*((u16*)0x023ffc40) = 0x1;						// Booted from card -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr
		return;
	}*/

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
		if (softResetParams[2] != 0x44414F4C) {
			resetParamLoc[2] = softResetParams[2];
		}
		resetParamLoc[3] = softResetParams[3];
	}

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 1;						// Boot Indicator (Booted from card for SDK5) -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr

	const char* romTid = getRomTid(ndsHeader);
	if ((!dsiModeConfirmed && 
		(strncmp(romTid, "KPP", 3) == 0 	// Pop Island
	  || strncmp(romTid, "KPF", 3) == 0)	// Pop Island: Paperfield
	) || (srlAddr > 0) || (softResetParams[2] == 0x44414F4C))
	{
		*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 2;					// Boot Indicator (Cloneboot/Multiboot)
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

	wifiLedState = i2cReadRegister(0x4A, 0x30);

	// Init card
	if (dsiSD) {
		if (!FAT_InitFiles(true, false, 0)) {
			nocashMessage("!FAT_InitFiles");
			errorOutput();
			//return -1;
		}
	}

	if (gameOnFlashcard || saveOnFlashcard) {
		// Init Slot-1 card
		if (!FAT_InitFiles(initDisc, true, 0)) {
			nocashMessage("!FAT_InitFiles");
			errorOutput();
			//return -1;
		}
	}

	if (logging) {
		enableDebug(getBootFileCluster("NDSBTSRP.LOG", bootstrapOnFlashcard));
	}

	bool dsiEnhancedMbk = (*(u32*)0x02FFE1A0 == 0x00403000 && REG_SCFG_EXT == 0);

	aFile srParamsFile = getFileFromCluster(srParamsFileCluster, gameOnFlashcard);
	fileRead((char*)&softResetParams, srParamsFile, 0, 0x10, -1);
	bool softResetParamsFound = (softResetParams[0] != 0xFFFFFFFF);
	if (softResetParamsFound) {
		u32 clearBuffer = 0xFFFFFFFF;
		fileWrite((char*)&clearBuffer, srParamsFile, 0, 0x4, -1);
		clearBuffer = 0;
		fileWrite((char*)&clearBuffer, srParamsFile, 0x4, 0x4, -1);
		fileWrite((char*)&clearBuffer, srParamsFile, 0x8, 0x4, -1);
		fileWrite((char*)&clearBuffer, srParamsFile, 0xC, 0x4, -1);
	}

	// ROM file
	aFile* romFile = (aFile*)(dsiEnhancedMbk ? ROM_FILE_LOCATION_ALT : ROM_FILE_LOCATION);
	*romFile = getFileFromCluster(storedFileCluster, gameOnFlashcard);

	fileRead((char*)&oldArm7mbk, *romFile, 0x1A0, sizeof(u32), -1);

	// Sav file
	aFile* savFile = (aFile*)(dsiEnhancedMbk ? SAV_FILE_LOCATION_ALT : SAV_FILE_LOCATION);
	*savFile = getFileFromCluster(saveFileCluster, saveOnFlashcard);

	/*const char* bootName = "BOOT.NDS";

	// Invalid file cluster specified
	if ((romFile->firstCluster < CLUSTER_FIRST) || (romFile->firstCluster >= CLUSTER_EOF)) {
		*romFile = getBootFileCluster(bootName, 0);
	}

	if (romFile->firstCluster == CLUSTER_FREE) {
		nocashMessage("fileCluster == CLUSTER_FREE");
		errorOutput();
		//return -1;
	}*/

	int errorCode;

	tDSiHeader dsiHeaderTemp;

	// Load the NDS file
	nocashMessage("Loading the NDS file...\n");

	//bool dsiModeConfirmed;
	loadBinary_ARM7(&dsiHeaderTemp, *romFile);
	if (isDSiWare)
	{
		dsiModeConfirmed = true;
	} else if (dsiMode == 2) {
		dsiModeConfirmed = dsiMode;
	} else {
		dsiModeConfirmed = dsiMode && ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr);
	}
	if (gameOnFlashcard || !isDSiWare) {
		if (dsiModeConfirmed && ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && consoleModel == 0) {
			extern u32 clusterCacheSize;
			clusterCacheSize = 0x20000;

			buildFatTableCacheCompressed(romFile);
			buildFatTableCacheCompressed(savFile);
		} else {
			buildFatTableCache(romFile);
			buildFatTableCache(savFile);
		}
	}

	if (dsiModeConfirmed) {
		/*if (consoleModel == 0 && !isDSiWare && !gameOnFlashcard) {
			dbg_printf("Cannot use DSi mode on DSi SD\n");
			errorOutput();
		}*/
		extern u32* lastClusterCacheUsed;
		extern u32 clusterCache;
		if ((u32)dsiHeaderTemp.arm7idestination > 0x02E80000) dsiHeaderTemp.arm7idestination = (u32*)0x02E80000;
		if (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && (gameOnFlashcard || !isDSiWare)) {
			if (consoleModel > 0) {
				tonccpy((char*)0x0DF80000, (char*)0x02700000, 0x80000);	// Move FAT table cache to debug RAM
				romFile->fatTableCache = (u32*)((u32)romFile->fatTableCache+0xB880000);
				savFile->fatTableCache = (u32*)((u32)savFile->fatTableCache+0xB880000);
				lastClusterCacheUsed = (u32*)((u32)lastClusterCacheUsed+0xB880000);
				clusterCache += 0xB880000;
				tonccpy((char*)INGAME_MENU_LOCATION_DSIWARE, (char*)INGAME_MENU_LOCATION, 0x9C00);
			} else {
				u32 add = 0x826000; // 0x02F26000
				if ((u8)a9ScfgRom != 1) {
					add -= 0x6000; // 0x02F20000
				}
				tonccpy((char*)0x02700000+add, (char*)0x02700000, 0x20000);	// Move FAT table cache elsewhere
				romFile->fatTableCache = (u32*)((u32)romFile->fatTableCache+add);
				savFile->fatTableCache = (u32*)((u32)savFile->fatTableCache+add);
				lastClusterCacheUsed = (u32*)((u32)lastClusterCacheUsed+add);
				clusterCache += add;
				tonccpy((char*)INGAME_MENU_LOCATION_TWLSDK, (char*)INGAME_MENU_LOCATION, 0x9C00);
			}
			toncset((char*)INGAME_MENU_LOCATION, 0, 0x89C00);
		}
	}
	if (!ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) || !dsiModeConfirmed) {
		toncset((u32*)0x02400000, 0, 0x20);
		toncset((u32*)0x02500000, 0, 0x100000);	// clear part of EWRAM - except before in-game menu data
		toncset((u32*)0x02E80000, 0, 0x800);
		memset_addrs_arm7(0x02F00000, 0x02F80000);
		memset_addrs_arm7(0x02FFE000, 0x02FFF000); // clear DSi header
	} /*else if (!gameOnFlashcard) {
		*(u32*)0x03708000 = 0x54455354;
		if (*(u32*)0x03700000 != 0x54455354) {	// If DSi WRAM isn't mirrored by 32KB...
			tonccpy((char*)0x03700000, (char*)0x02700000, 0x80000);	// Copy FAT table cache to DSi WRAM
			toncset((char*)0x02700000, 0, 0x80000);
			romFile->fatTableCache = (u32)romFile->fatTableCache+0x01000000;
			savFile->fatTableCache = (u32)savFile->fatTableCache+0x01000000;
		}
	}*/

	// File containing cached patch offsets
	aFile patchOffsetCacheFile = getFileFromCluster(patchOffsetCacheFileCluster, gameOnFlashcard);
	fileRead((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), -1);
	patchOffsetCacheFilePrevCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
	u16 prevPatchOffsetCacheFileVersion = patchOffsetCache.ver;

	nocashMessage("Loading the header...\n");

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);
	ltd_module_params_t* ltdModuleParams = NULL;
	if (ndsHeader->unitCode > 0) {
		extern u32* findLtdModuleParamsOffset(const tNDSHeader* ndsHeader);
		ltdModuleParams = (ltd_module_params_t*)(findLtdModuleParamsOffset(&dsiHeaderTemp.ndshdr) - 4);
	}
    dbg_printf("sdk_version: ");
    dbg_hexa(moduleParams->sdk_version);
    dbg_printf("\n"); 

	ndsHeader = loadHeader(&dsiHeaderTemp, moduleParams, dsiModeConfirmed);

	if (!isDSiWare && srlAddr == 0 && (softResetParams[0] == 0 || softResetParams[0] == 0xFFFFFFFF)) {
		esrbOutput();
	}

	//if (gameOnFlashcard || !isDSiWare || !dsiWramAccess) {
		ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams, ltdModuleParams, true);
	//}
	if (decrypt_arm9(&dsiHeaderTemp)) {
		nocashMessage("Secure area decrypted successfully");
	} else {
		nocashMessage("Secure area already decrypted");
	}
	dbg_printf("\n");

	// Calculate overlay pack size
	for (u32 i = ndsHeader->arm9romOffset+ndsHeader->arm9binarySize; i < ndsHeader->arm7romOffset; i++) {
		overlaysSize++;
	}
	if (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && dsiModeConfirmed) {
		// Calculate i-overlay pack size
		for (u32 i = (u32)dsiHeaderTemp.arm9iromOffset+dsiHeaderTemp.arm9ibinarySize; i < (u32)dsiHeaderTemp.arm7iromOffset; i++) {
			ioverlaysSize++;
		}
	}
    /*dbg_printf("overlaysSize: ");
    dbg_hexa(overlaysSize);
    dbg_printf("\n");*/

	my_readUserSettings(ndsHeader); // Header has to be loaded first

	baseChipID = getChipId(pkmnHeader ? (tNDSHeader*)NDS_HEADER_POKEMON : ndsHeader, moduleParams);

	const char* romTid = getRomTid(ndsHeader);

	if (!gameOnFlashcard && isDSiWare) {
		extern void patchSharedFontPath(const cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const ltd_module_params_t* ltdModuleParams);

		bool twlTouch = (cdcReadReg(CDC_SOUND, 0x22) == 0xF0);

		if (twlTouch && *(u8*)0x02FFE1BF & BIT(0)) {
			*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
			DSiTouchscreenMode();
		} else {
			*(u8*)0x02FFE1BF &= ~BIT(0);	// Set NTR touch mode (Disables camera)
			*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
			NDSTouchscreenMode();
			if (macroMode) {
				u32 temp = readPowerManagement(PM_CONTROL_REG) & (~(PM_BACKLIGHT_TOP & 0xFFFF));
				writePowerManagement(PM_CONTROL_REG, temp);
			}
		}
		*(u16*)0x4000500 = 0x807F;

	 //if (*(u32*)0x02FFE1D8 <= 0x02E80000) {
		memset_addrs_arm7(0x02F00000, 0x02F80000);
	  if (memcmp(romTid, "KRV", 3) == 0 // Hip Hop King: Rytmik Edition
	   || memcmp(romTid, "KYR", 3) == 0 // Rytmik Retrobits
	   || memcmp(romTid, "KRQ", 3) == 0 // Rytmik Rock Edition
	   || memcmp(romTid, "KYH", 3) == 0 // Rytmik World Music
	  ) {
		// Skip loading in-game menu and cheat engine
		toncset((char*)INGAME_MENU_LOCATION, 0, 0xA000);
		toncset((u32*)CARDENGINEI_ARM7_BUFFERED_LOCATION, 0, 0x8000);
		toncset((char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0, 0x400);

		patchSharedFontPath(NULL, ndsHeader, moduleParams, ltdModuleParams);

		newArm7binarySize = ndsHeader->arm7binarySize;
		newArm7ibinarySize = __DSiHeader->arm7ibinarySize;

		if (REG_SCFG_EXT == 0) {
			if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0) {
				// Replace incompatible ARM7 binary
				newArm7binarySize = *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION;
				newArm7ibinarySize = *(u32*)DONOR_ROM_ARM7I_SIZE_LOCATION;
				*(u32*)0x02FFE1A0 = *(u32*)DONOR_ROM_MBK6_LOCATION;
				*(u32*)0x02FFE1D4 = *(u32*)DONOR_ROM_DEVICE_LIST_LOCATION;
				tonccpy(ndsHeader->arm7destination, (u8*)DONOR_ROM_ARM7_LOCATION, newArm7binarySize);
			}

			if (newArm7binarySize != patchOffsetCache.a7BinSize) {
				extern void rsetA7Cache(void);
				rsetA7Cache();
				patchOffsetCache.a7BinSize = newArm7binarySize;
			}

			if (dsiEnhancedMbk && oldArm7mbk == 0x080037C0) {
				extern void patchPostBoot(const tNDSHeader* ndsHeader);
				patchPostBoot(ndsHeader);
			}
		} else if (newArm7binarySize != patchOffsetCache.a7BinSize) {
			extern void rsetA7Cache(void);
			rsetA7Cache();
			patchOffsetCache.a7BinSize = newArm7binarySize;
		}

		extern void patchScfgExt(const tNDSHeader* ndsHeader);
		patchScfgExt(ndsHeader);

		toncset((u32*)0x02680000, 0, 0x100000);
		toncset((u32*)UNPATCHED_FUNCTION_LOCATION, 0, 0x40);

		patchOffsetCacheFileNewCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
		if (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheFileNewCrc != patchOffsetCacheFilePrevCrc) {
			fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), -1);
		}
	  } else {
		ce9Location = CARDENGINEI_ARM9_DSIWARE_LOCATION;
		ce7Location = CARDENGINEI_ARM7_DSIWARE_LOCATION;

		tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_BUFFERED_LOCATION, 0x6000);
		tonccpy((char*)INGAME_MENU_LOCATION_DSIWARE, (char*)INGAME_MENU_LOCATION, 0xA000);
		toncset((char*)INGAME_MENU_LOCATION, 0, 0xA000);

		tonccpy((u32*)ce7Location, (u32*)CARDENGINEI_ARM7_BUFFERED_LOCATION, 0x9000);
		tonccpy((char*)ce7Location-0x8400, (char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0x400);
		toncset((u32*)CARDENGINEI_ARM7_BUFFERED_LOCATION, 0, 0x8000);
		toncset((char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0, 0x400);

		//ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams, false);

		patchSharedFontPath((cardengineArm9*)ce9Location, ndsHeader, moduleParams, ltdModuleParams);
		dsiWarePatch((cardengineArm9*)ce9Location, ndsHeader);

		if (*(u8*)0x02FFE1BF & BIT(2)) {
			bannerSavPatch(ndsHeader);
		}

		newArm7binarySize = ndsHeader->arm7binarySize;
		newArm7ibinarySize = __DSiHeader->arm7ibinarySize;

		extern void rsetPatchCache(bool dsiWare);
		rsetPatchCache(true);

		patchHiHeapPointer(moduleParams, ndsHeader, false);

		extern bool a9PatchCardIrqEnable(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
		a9PatchCardIrqEnable((cardengineArm9*)ce9Location, ndsHeader, moduleParams);

		extern void patchResetTwl(cardengineArm9* ce9, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
		patchResetTwl((cardengineArm9*)ce9Location, ndsHeader, moduleParams);

		if (REG_SCFG_EXT == 0) {
			if (!dsiWramAccess) {
				extern void patchA9Mbk(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool standAlone);
				patchA9Mbk(ndsHeader, moduleParams, true);
			}

			if (!dsiEnhancedMbk && oldArm7mbk == 0x00403000) {
				extern void patchGbaSlotInit_cont(const tNDSHeader* ndsHeader, bool usesThumb, bool searchAgainForThumb);
				patchGbaSlotInit_cont(ndsHeader, false, true);
			}

			if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0) {
				// Replace incompatible ARM7 binary
				newArm7binarySize = *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION;
				newArm7ibinarySize = *(u32*)DONOR_ROM_ARM7I_SIZE_LOCATION;
				*(u32*)0x02FFE1A0 = *(u32*)DONOR_ROM_MBK6_LOCATION;
				*(u32*)0x02FFE1D4 = *(u32*)DONOR_ROM_DEVICE_LIST_LOCATION;
				tonccpy(ndsHeader->arm7destination, (u8*)DONOR_ROM_ARM7_LOCATION, newArm7binarySize);
			}

			if (newArm7binarySize != patchOffsetCache.a7BinSize) {
				extern void rsetA7Cache(void);
				rsetA7Cache();
				patchOffsetCache.a7BinSize = newArm7binarySize;
			}

			if (dsiEnhancedMbk && oldArm7mbk == 0x080037C0) {
				extern void patchPostBoot(const tNDSHeader* ndsHeader);
				patchPostBoot(ndsHeader);
			}
		} else if (newArm7binarySize != patchOffsetCache.a7BinSize) {
			extern void rsetA7Cache(void);
			rsetA7Cache();
			patchOffsetCache.a7BinSize = newArm7binarySize;
		}

		extern void patchScfgExt(const tNDSHeader* ndsHeader);
		patchScfgExt(ndsHeader);

		extern bool a7PatchCardIrqEnable(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
		if (!a7PatchCardIrqEnable((cardengineArm7*)ce7Location, ndsHeader, moduleParams)) {
			dbg_printf("ERR_LOAD_OTHR");
			errorOutput();
		}

		/*extern u32 savePatchV5(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, u32 saveFileCluster); // SDK 5
		savePatchV5((cardengineArm7*)ce7Location, ndsHeader, saveFileCluster);*/

		toncset((u32*)0x02680000, 0, 0x100000);

		errorCode = hookNdsRetailArm7(
			(cardengineArm7*)ce7Location,
			ndsHeader,
			moduleParams,
			romFile->firstCluster,
			srParamsFileCluster,
			ramDumpCluster,
			screenshotCluster,
			wideCheatFileCluster,
			wideCheatSize,
			cheatFileCluster,
			cheatSize,
			apPatchFileCluster,
			apPatchSize,
			pageFileCluster,
			manualCluster,
			bootstrapOnFlashcard,
			gameOnFlashcard,
			saveOnFlashcard,
			language,
			dsiModeConfirmed,
			dsiSD,
			extendedMemoryConfirmed,
			0,
			consoleModel,
			romRead_LED,
			dmaRomRead_LED,
			twlTouch,
			false
		);
		if (errorCode == ERR_NONE) {
			nocashMessage("Card hook successful");
		} else {
			nocashMessage("Card hook failed");
			errorOutput();
		}

		hookNdsRetailArm9Mini(
			(cardengineArm9*)ce9Location,
			ndsHeader,
			consoleModel
		);

		toncset((u32*)UNPATCHED_FUNCTION_LOCATION, 0, 0x40);

		patchOffsetCacheFileNewCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
		if (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheFileNewCrc != patchOffsetCacheFilePrevCrc) {
			fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), -1);
		}

		extern u32 iUncompressedSize;
		extern u32 iUncompressedSizei;
		if (consoleModel > 0) {
			tonccpy((char*)ndsHeader->arm9destination+0xB000000, ndsHeader->arm9destination, iUncompressedSize);
			tonccpy((char*)ndsHeader->arm7destination+0xB000000, ndsHeader->arm7destination, newArm7binarySize);
			tonccpy((char*)(*(u32*)0x02FFE1C8)+0xB000000, (u32*)*(u32*)0x02FFE1C8, (iUncompressedSizei > 0 ? iUncompressedSizei : *(u32*)0x02FFE1CC));
			tonccpy((char*)(*(u32*)0x02FFE1D8)+0xB000000, (u32*)*(u32*)0x02FFE1D8, newArm7ibinarySize);
			*(u32*)0x0DFFE02C = iUncompressedSize;
			*(u32*)0x0DFFE03C = newArm7binarySize;
			*(u32*)0x0DFFE1CC = (iUncompressedSizei > 0 ? iUncompressedSizei : *(u32*)0x02FFE1CC);
			*(u32*)0x0DFFE1DC = newArm7ibinarySize;
		} else {
			aFile pageFile = getFileFromCluster(pageFileCluster, bootstrapOnFlashcard);

			fileWrite((char*)ndsHeader->arm9destination, pageFile, 0, iUncompressedSize, -1);
			fileWrite((char*)ndsHeader->arm7destination, pageFile, 0x2C0000, newArm7binarySize, -1);
			fileWrite((char*)&iUncompressedSize, pageFile, 0x5FFFF0, sizeof(u32), -1);
			fileWrite((char*)&newArm7binarySize, pageFile, 0x5FFFF4, sizeof(u32), -1);
			fileWrite((char*)(*(u32*)0x02FFE1C8), pageFile, 0x300000, (iUncompressedSizei > 0 ? iUncompressedSizei : *(u32*)0x02FFE1CC), -1);
			fileWrite((char*)(*(u32*)0x02FFE1D8), pageFile, 0x580000, newArm7ibinarySize, -1);
			fileWrite((char*)(iUncompressedSizei > 0 ? &iUncompressedSizei : (u32*)0x02FFE1CC), pageFile, 0x5FFFF8, sizeof(u32), -1);
			fileWrite((char*)&newArm7ibinarySize, pageFile, 0x5FFFFC, sizeof(u32), -1);
		}
	  }
	 //}
	} else {
		if (strncmp(romTid, "UBR", 3) == 0) {
			toncset((char*)0x0C400000, 0xFF, 0xC0);
			toncset((u8*)0x0C4000B2, 0, 3);
			toncset((u8*)0x0C4000B5, 0x24, 3);
			*(u16*)0x0C4000BE = 0x7FFF;
			toncset((char*)0x0C4000C0, 0, 0xE);
			*(u16*)0x0C4000CE = 0x7FFF;
			toncset((char*)0x0C4000D0, 0, 0x130);
		} /*else // GBA file
		if (!dsiModeConfirmed && !isSdk5(moduleParams)) {
			aFile* gbaFile = (aFile*)(dsiEnhancedMbk ? GBA_FILE_LOCATION_ALT : GBA_FILE_LOCATION);
			*gbaFile = getFileFromCluster(gbaFileCluster);
			aFile* gbaSavFile = (aFile*)(dsiEnhancedMbk ? GBA_SAV_FILE_LOCATION_ALT : GBA_SAV_FILE_LOCATION);
			*gbaSavFile = getFileFromCluster(gbaSaveFileCluster);
			gbaRomFound = (gbaFile->firstCluster != CLUSTER_FREE);
			//patchSlot2Addr(ndsHeader);

			tonccpy((char*)GBA_FILE_LOCATION_MAINMEM, gbaFile, sizeof(aFile));
			tonccpy((char*)GBA_SAV_FILE_LOCATION_MAINMEM, gbaSavFile, sizeof(aFile));

			if (gbaRomFound) {
				fileRead((char*)0x027FFC30, *gbaFile, 0xBE, 2, -1);
				fileRead((char*)0x027FFC32, *gbaFile, 0xB5, 3, -1);
				fileRead((char*)0x027FFC36, *gbaFile, 0xB0, 2, -1);
				fileRead((char*)0x027FFC38, *gbaFile, 0xAC, 4, -1);
			}
		}*/

		if (!gameOnFlashcard && REG_SCFG_EXT != 0 && !(REG_SCFG_MC & BIT(0))) {
			if (specialCard) {
				// Enable Slot-1 for games that use IR
				my_enableSlot1();
			} else {
				my_disableSlot1();
			}
		}

		bool twlTouch = (cdcReadReg(CDC_SOUND, 0x22) == 0xF0);

		if (!twlTouch || !dsiModeConfirmed || !ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) || (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && !(*(u8*)0x02FFE1BF & BIT(0)))) {
			*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
			NDSTouchscreenMode();
			*(u16*)0x4000500 = 0x807F;
			if (!dsiModeConfirmed || !ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr)) {
				REG_GPIO_WIFI |= BIT(8);	// Old NDS-Wifi mode
			}
			if (macroMode) {
				u32 temp = readPowerManagement(PM_CONTROL_REG) & (~(PM_BACKLIGHT_TOP & 0xFFFF));
				writePowerManagement(PM_CONTROL_REG, temp);
			}
		}

		if (dsiModeConfirmed && ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr)) {
			if (!twlTouch) {
				*(u8*)0x02FFE1BF &= ~BIT(0);	// Set NTR touch mode (Disables camera)
			} else/* if (!(*(u8*)0x02FFE1BF & BIT(0))) {
				*(u8*)0x02FFE1BF |= BIT(0);	// Set TWL touch mode
				*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
				DSiTouchscreenMode();
				*(u16*)0x4000500 = 0x807F;
			} else*/ if (*(u8*)0x02FFE1BF & BIT(0)) {
				*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
				DSiTouchscreenMode();
				*(u16*)0x4000500 = 0x807F;
			}
		}

		if (!dsiModeConfirmed) {
			NTR_BIOS();
		}

		u32 clonebootFlag = 0;
		fileRead((char*)&clonebootFlag, *romFile, baseRomSize, sizeof(u32), -1);
		bool usesCloneboot = (clonebootFlag == 0x16361);
		if (usesCloneboot) {
			dbg_printf("Cloneboot detected\n");
		}

		// If possible, set to load ROM into RAM
		u32 ROMinRAM = isROMLoadableInRAM(&dsiHeaderTemp, &dsiHeaderTemp.ndshdr, romTid, moduleParams, usesCloneboot);

		nocashMessage("Trying to patch the card...\n");

		u16 ce9size = 0;
		u32 ce7Size = 0x11C00;
		if (dsiEnhancedMbk) {
			ce7Location = CARDENGINEI_ARM7_LOCATION_ALT;
		}

		bool useSdk5ce7 = (isSdk5(moduleParams) && ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && dsiModeConfirmed);

		if (useSdk5ce7) {
			ce7Location = CARDENGINEI_ARM7_SDK5_LOCATION;
			ce7Size = 0xFC00;
		}

		if (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && dsiModeConfirmed) {
			tonccpy((char*)ROM_FILE_LOCATION_TWLSDK, romFile, sizeof(aFile));
			tonccpy((char*)SAV_FILE_LOCATION_TWLSDK, savFile, sizeof(aFile));
		} else {
			tonccpy((char*)ROM_FILE_LOCATION_MAINMEM, romFile, sizeof(aFile));
			tonccpy((char*)SAV_FILE_LOCATION_MAINMEM, savFile, sizeof(aFile));
		}

		bool useApPatch = (srlAddr == 0 && apPatchFileCluster != 0 && !apPatchIsCheat && apPatchSize > 0 && apPatchSize <= 0x40000);

		if (useApPatch) {
			aFile apPatchFile = getFileFromCluster(apPatchFileCluster, gameOnFlashcard);
			dbg_printf("AP-fix found\n");
			fileRead((char*)IPS_LOCATION, apPatchFile, 0, apPatchSize, 0);
			if (*(u8*)(IPS_LOCATION+apPatchSize-1) != 0xA9) {
				overlayPatch = ipsHasOverlayPatch(ndsHeader, (u8*)IPS_LOCATION);
			}
		}

		if (strncmp(romTid, "ASM", 3) == 0) {
			overlayPatch = true; // Allow overlay patching for SM64DS ROM hacks (ex. Mario's Holiday)
		}

		extern void rsetPatchCache(bool dsiWare);
		rsetPatchCache(false);

		tonccpy((u32*)ce7Location, (u32*)CARDENGINEI_ARM7_BUFFERED_LOCATION, ce7Size);
		if (gameOnFlashcard || saveOnFlashcard) {
			if (!dldiPatchBinary((data_t*)ce7Location, ce7Size-0x400)) {
				dbg_printf("ce7 DLDI patch failed\n");
				errorOutput();
			}
		}
		tonccpy((char*)ce7Location-0x8400, (char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0x400);
		toncset((char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0, 0x400);

		if (extendedMemoryConfirmed) {
			ce9size = 0x2000;
			if (moreMemory || !dsiWramAccess) {
				ce9Location = (moduleParams->sdk_version >= 0x2008000) ? CARDENGINEI_ARM9_CACHED_LOCATION2_ROMINRAM : CARDENGINEI_ARM9_CACHED_LOCATION1_ROMINRAM;
				/*if ((u32)ndsHeader->arm9destination == 0x02004000 && moduleParams->sdk_version < 0x2008000) {
					ce9Location = CARDENGINEI_ARM9_CACHED_LOCATION1;
				}*/
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION, ce9size);
				relocate_ce9(CARDENGINEI_ARM9_LOCATION_DSI_WRAM,ce9Location,ce9size);
			} else {
				ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION, ce9size);
			}
		} else if (isSdk5(moduleParams) && ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
			ce9Location = CARDENGINEI_ARM9_TWLSDK_LOCATION;
			if (gameOnFlashcard) {
				ce9size = 0x7000;
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_SDK5_DLDI_BUFFERED_LOCATION, ce9size);
				if (!dldiPatchBinary((data_t*)ce9Location, ce9size)) {
					dbg_printf("ce9 DLDI patch failed\n");
					errorOutput();
				}
			} else {
				ce9size = 0x5000;
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION, ce9size);
			}
			patchHiHeapPointer(moduleParams, ndsHeader, ROMinRAM);
		} else if (gameOnFlashcard) {
			ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
			ce9size = 0x7000;
			tonccpy((u32*)CARDENGINEI_ARM9_LOCATION_DSI_WRAM, (u32*)CARDENGINEI_ARM9_DLDI_BUFFERED_LOCATION, ce9size);
			if (!dldiPatchBinary((data_t*)ce9Location, ce9size)) {
				dbg_printf("ce9 DLDI patch failed\n");
				errorOutput();
			}
		} else {
			ce9Location = dsiWramAccess ? CARDENGINEI_ARM9_LOCATION_DSI_WRAM : CARDENGINEI_ARM9_LOCATION;
			ce9size = 0x5000;
			tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_BUFFERED_LOCATION, ce9size);
		}

		if (extendedMemoryConfirmed && (moreMemory || !dsiWramAccess)) {
			memset_addrs_arm7(CARDENGINEI_ARM7_BUFFERED_LOCATION, CARDENGINEI_ARM9_CACHED_LOCATION1_ROMINRAM);
			memset_addrs_arm7(CARDENGINEI_ARM9_CACHED_LOCATION1_ROMINRAM+0x2000, CARDENGINEI_ARM9_SDK5_DLDI_BUFFERED_LOCATION+0x7000);
		} else {
			toncset((u32*)CARDENGINEI_ARM7_BUFFERED_LOCATION, 0, 0x48000);
		}

		if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0 && ndsHeader->unitCode > 0 && dsiModeConfirmed) {
			*(u32*)0x02FFE1A0 = *(u32*)DONOR_ROM_MBK6_LOCATION;
			*(u32*)0x02FFE1D4 = *(u32*)DONOR_ROM_DEVICE_LIST_LOCATION;
		}

		patchBinary((cardengineArm9*)ce9Location, ndsHeader, moduleParams);
		errorCode = patchCardNds(
			(cardengineArm7*)ce7Location,
			(cardengineArm9*)ce9Location,
			ndsHeader,
			moduleParams,
			ltdModuleParams,
			(  (moduleParams->sdk_version < 0x4000000 && ((u32)ndsHeader->arm9executeAddress - (u32)ndsHeader->arm9destination) >= 0x1000
			&& moduleParams->sdk_version != 0x2007533)
			|| strncmp(romTid, "AKD", 3) == 0 // Trauma Center: Under the Knife
			) ? 0 : 1,
			usesCloneboot,
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

		errorCode = hookNdsRetailArm7(
			(cardengineArm7*)ce7Location,
			ndsHeader,
			moduleParams,
			romFile->firstCluster,
			srParamsFileCluster,
			ramDumpCluster,
			screenshotCluster,
			wideCheatFileCluster,
			wideCheatSize,
			cheatFileCluster,
			cheatSize,
			apPatchFileCluster,
			apPatchSize,
			pageFileCluster,
			manualCluster,
			bootstrapOnFlashcard,
			gameOnFlashcard,
			saveOnFlashcard,
			language,
			dsiModeConfirmed,
			dsiSD,
			extendedMemoryConfirmed,
			ROMinRAM,
			consoleModel,
			romRead_LED,
			dmaRomRead_LED,
			twlTouch,
			usesCloneboot
		);
		if (errorCode == ERR_NONE) {
			nocashMessage("Card hook successful");
		} else {
			nocashMessage("Card hook failed");
			errorOutput();
		}

		hookNdsRetailArm9(
			(cardengineArm9*)ce9Location,
			ndsHeader,
			moduleParams,
			romFile->firstCluster,
			savFile->firstCluster,
			saveSize,
			saveOnFlashcard,
			cacheBlockSize,
			extendedMemoryConfirmed,
			ROMinRAM,
			dsiModeConfirmed,
			supportsExceptionHandler(romTid),
			consoleModel,
			usesCloneboot
		);

		patchOffsetCacheFileNewCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
		if (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheFileNewCrc != patchOffsetCacheFilePrevCrc) {
			fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), -1);
		}

		if (isSdk5(moduleParams)) {
			tonccpy((u32*)UNPATCHED_FUNCTION_LOCATION_SDK5, (u32*)UNPATCHED_FUNCTION_LOCATION, 0x40);
			toncset((u32*)UNPATCHED_FUNCTION_LOCATION, 0, 0x40);
		}

		if (ROMinRAM) {
			loadROMintoRAM(ndsHeader, moduleParams, romFile, savFile, usesCloneboot);
			if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
				loadIOverlaysintoRAM(&dsiHeaderTemp, *romFile, usesCloneboot);
			}
		} else if ((ROMsupportsDsiMode(ndsHeader) && !isDSiWare) || strncmp(romTid, "UBR", 3) != 0) {
			loadOverlaysintoRAM(ndsHeader, moduleParams, *romFile);
		}

		if (useApPatch) {
			if (applyIpsPatch(ndsHeader, (u8*)IPS_LOCATION, (*(u8*)(IPS_LOCATION+apPatchSize-1) == 0xA9), isSdk5(moduleParams), ROMinRAM, usesCloneboot)) {
				dbg_printf("AP-fix applied\n");
			} else {
				dbg_printf("Failed to apply AP-fix\n");
			}
			toncset((u32*)IPS_LOCATION, 0, apPatchSize);	// Clear IPS patch
		}

		if (!ROMinRAM && overlayPatch) {
			aFile* apFixOverlaysFile = (aFile*)((ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) ? OVL_FILE_LOCATION_TWLSDK : OVL_FILE_LOCATION_MAINMEM);
			*apFixOverlaysFile = getFileFromCluster(apFixOverlaysCluster, gameOnFlashcard);
			buildFatTableCacheCompressed(apFixOverlaysFile);

			u32 alignedOverlaysOffset = ((ndsHeader->arm9romOffset + ndsHeader->arm9binarySize)/cacheBlockSize)*cacheBlockSize;
			u32 newOverlaysSize = 0;
			for (u32 i = alignedOverlaysOffset; i < ndsHeader->arm7romOffset; i+= cacheBlockSize) {
				newOverlaysSize += cacheBlockSize;
			}

			fileWrite((char*)CACHE_ADRESS_START, *apFixOverlaysFile, alignedOverlaysOffset, newOverlaysSize, -1);	// Write AP-fixed overlays to a file
			toncset((char*)CACHE_ADRESS_START, 0, newOverlaysSize);

			dbg_printf("Overlays cached to a file\n");
		}

		if (!ROMinRAM && ((ROMsupportsDsiMode(ndsHeader) && !isDSiWare) || strncmp(romTid, "UBR", 3) != 0)) {
			loadROMPartIntoRAM(ndsHeader, moduleParams, *romFile);
		}

		if (gameOnFlashcard && isDSiWare) {
			aFile* sharedFontFile = (aFile*)FONT_FILE_LOCATION_TWLSDK;
			*sharedFontFile = getFileFromCluster(sharedFontCluster, true);
			buildFatTableCacheCompressed(sharedFontFile);
		}

		extern u32 iUncompressedSize;
		if (strncmp(romTid, "UBR", 3) == 0) {
			// Do nothing
		} else if (extendedMemoryConfirmed || dsiModeConfirmed) {
			extern u32 iUncompressedSizei;
			aFile pageFile = getFileFromCluster(pageFileCluster, bootstrapOnFlashcard);

			fileWrite((char*)ndsHeader->arm9destination, pageFile, 0, iUncompressedSize, -1);
			fileWrite((char*)ndsHeader->arm7destination, pageFile, 0x2C0000, newArm7binarySize, -1);
			fileWrite((char*)&iUncompressedSize, pageFile, 0x5FFFF0, sizeof(u32), -1);
			fileWrite((char*)&newArm7binarySize, pageFile, 0x5FFFF4, sizeof(u32), -1);
			if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
				fileWrite((char*)(*(u32*)0x02FFE1C8), pageFile, 0x300000, iUncompressedSizei, -1);
				fileWrite((char*)(*(u32*)0x02FFE1D8), pageFile, 0x580000, newArm7ibinarySize, -1);
				fileWrite((char*)&iUncompressedSizei, pageFile, 0x5FFFF8, sizeof(u32), -1);
				fileWrite((char*)&newArm7ibinarySize, pageFile, 0x5FFFFC, sizeof(u32), -1);
			}
		} else {
			*(u32*)ARM9_DEC_SIZE_LOCATION = iUncompressedSize;
			tonccpy((char*)ndsHeader->arm9destination+0x400000, ndsHeader->arm9destination, iUncompressedSize);
			tonccpy((char*)DONOR_ROM_ARM7_LOCATION, ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		}
		if (!gameOnFlashcard && !ROMinRAM && (romRead_LED==1 || dmaRomRead_LED==1)) {
			// Turn WiFi LED off
			i2cWriteRegister(0x4A, 0x30, 0x12);
		}
	}

	arm9_boostVram = boostVram;
	arm9_isSdk5 = isSdk5(moduleParams);

    /*if (isGSDD) {
	   *(vu32*)REG_MBK1 = 0x8185898C; // WRAM-A slot 0 mapped to ARM9
	}*/

	if (isSdk5(moduleParams) && ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
		initMBK_dsiMode();
		REG_SCFG_EXT = 0x93FFFB06;
		REG_SCFG_CLK = 0x187;
	}

	if (esrbScreenPrepared) {
		while (!esrbImageLoaded) {
			while (REG_VCOUNT != 191);
			while (REG_VCOUNT == 191);
		}
	}

	toncset16((u32*)IMAGES_LOCATION, 0, (256*192)*3);	// Clear nds-bootstrap images
	clearScreen();

	i2cReadRegister(0x4A, 0x10);	// Clear accidential POWER button press

	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);

	nocashMessage("Starting the NDS file...");
    setMemoryAddress(ndsHeader, moduleParams);

	if (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A))) {
		aFile ramDumpFile = getFileFromCluster(ramDumpCluster, bootstrapOnFlashcard);

		fileWrite((char*)0x0C000000, ramDumpFile, 0, (consoleModel==0 ? 0x01000000 : 0x02000000), -1);		// Dump RAM
		//fileWrite((char*)dsiHeaderTemp.arm9idestination, ramDumpFile, 0, dsiHeaderTemp.arm9ibinarySize, -1);	// Dump (decrypted?) arm9 binary
	}

	if (ROMsupportsDsiMode(ndsHeader) && isDSiWare && !gameOnFlashcard && !(REG_SCFG_ROM & BIT(9))) {
		*(vu32*)0x400481C = 0;				// Reset SD IRQ stat register
		*(vu32*)0x4004820 = 0x8B7F0305;	// Set SD IRQ mask register (Data won't read without the correct bytes!)
	} /*else if (!isDSiWare) {
		*(vu32*)0x400481C = 0;				// Reset SD IRQ stat register
		if (!gameOnFlashcard) {
			*(vu32*)0x4004820 = BIT(3);	// Set SD IRQ mask register
		} else {
			*(vu32*)0x4004820 = 0;	// Clear SD IRQ mask register
		}
	}*/
			//*(vu32*)0x4004820 = (BIT(0) | BIT(2) | BIT(3) | BIT(24) | BIT(25) | BIT(29) | BIT(30));	// Set SD IRQ mask register
			//*(vu32*)0x4004820 = 0x8B7F0305;	// Set SD IRQ mask register

	if (!dsiModeConfirmed /*|| (ROMsupportsDsiMode(ndsHeader) && !isDSiWare)*/) {
		REG_SCFG_EXT &= ~(1UL << 31); // Lock SCFG
	}

	startBinary_ARM7();

	return 0;
}
