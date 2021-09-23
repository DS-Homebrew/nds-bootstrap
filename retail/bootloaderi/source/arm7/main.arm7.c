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

//#define memcpy __builtin_memcpy

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

extern void arm7clearRAM(void);
extern void arm7_reset(void);
extern void arm7_reset_sdk5(void);

extern bool moreMemory;

//extern u32 _start;
extern u32 storedFileCluster;
extern u32 initDisc;
extern u16 gameOnFlashcard;
extern u16 saveOnFlashcard;
extern u16 a9ScfgRom;
extern u8 dsiSD;
extern u32 saveFileCluster;
extern u32 gbaFileCluster;
extern u32 gbaSaveFileCluster;
extern u32 romSize;
extern u32 saveSize;
extern u32 gbaRomSize;
extern u32 gbaSaveSize;
extern u32 wideCheatFileCluster;
extern u32 wideCheatSize;
extern u32 apPatchFileCluster;
extern u32 apPatchSize;
extern u32 cheatFileCluster;
extern u32 cheatSize;
extern u32 patchOffsetCacheFileCluster;
extern u32 fatTableFileCluster;
extern u32 ramDumpCluster;
extern u32 srParamsFileCluster;
extern u32 screenshotCluster;
extern u8 patchMpuSize;
extern u8 patchMpuRegion;
extern u8 language;
extern u8 region;
extern u8 dsiMode; // SDK 5
extern u8 isDSiWare; // SDK 5
extern u8 donorSdkVer;
extern u8 extendedMemory;
extern u8 consoleModel;
extern u8 romRead_LED;
extern u8 dmaRomRead_LED;
extern u8 soundFreq;
extern u8 specialCard;

bool useTwlCfg = false;
int twlCfgLang = 0;

bool sdRead = true;

//bool gbaRomFound = false;

static u32 ce7Location = CARDENGINEI_ARM7_LOCATION;
static u32 ce9Location = CARDENGINEI_ARM9_LOCATION;
u32 overlaysSize = 0;
u32 ioverlaysSize = 0;

static u32 softResetParams[4] = {0};
u32 srlAddr = 0;

u32 newArm7binarySize = 0;
u32 newArm7ibinarySize = 0;

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

	toncset((u32*)0x02004000, 0, 0x33C000);	// clear part of EWRAM - except before nds-bootstrap images
	toncset((u32*)0x02380000, 0, 0x5A000);		// clear part of EWRAM - except before 0x023DA000, which has the arm9 code
	toncset((u32*)0x023DB000, 0, 0x25000);		// clear part of EWRAM
	toncset((u32*)0x02500000, 0, 0x100000);	// clear part of EWRAM - except before in-game menu data
	memset_addrs_arm7(0x02700000, BLOWFISH_LOCATION);		// clear part of EWRAM - except before ce7 and ce9 binaries
	toncset((u32*)0x027F8000, 0, 0x8000);	// clear part of EWRAM
	memset_addrs_arm7(0x02800000, 0x02E80000);
	memset_addrs_arm7(0x02F00000, 0x02FFE000);
	toncset((u32*)0x02FFF000, 0, 0x1000);		// clear part of EWRAM: header
	REG_IE = 0;
	REG_IF = ~0;
	REG_AUXIE = 0;
	REG_AUXIF = ~0;
	*(vu32*)(0x04000000 - 4) = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)(0x04000000 - 8) = ~0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  // Turn off power to stuff

	useTwlCfg = ((*(u8*)0x02000400 & 0x0F) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0));
	twlCfgLang = *(u8*)0x02000406;
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
	//u32* moduleParamsOffset = malloc(sizeof(module_params_t));
	u32* moduleParamsOffset = malloc(0x100);

	//toncset(moduleParamsOffset, 0, sizeof(module_params_t));
	toncset(moduleParamsOffset, 0, 0x100);

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

static inline u32 getRomSizeNoArmBins(const tNDSHeader* ndsHeader) {
	return (ndsHeader->romSize+0x88) - ndsHeader->arm7romOffset - ndsHeader->arm7binarySize + overlaysSize;
}

static inline u32 getIRomSizeNoArmBins(const tDSiHeader* dsiHeader) {
	return (u32)dsiHeader->arm9iromOffset - dsiHeader->ndshdr.arm7romOffset - dsiHeader->ndshdr.arm7binarySize + overlaysSize;
}

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
	srlAddr = softResetParams[3];
	fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp), !sdRead, 0);
	if (srlAddr > 0 && ((u32)dsiHeaderTemp->ndshdr.arm9destination < 0x02000000 || (u32)dsiHeaderTemp->ndshdr.arm9destination > 0x02004000)) {
		// Invalid SRL
		srlAddr = 0;
		fileRead((char*)dsiHeaderTemp, file, srlAddr, sizeof(*dsiHeaderTemp), !sdRead, 0);
	}

	char baseTid[5] = {0};
	fileRead((char*)&baseTid, file, 0xC, 4, !sdRead, 0);
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

    /*if (
		strncmp(romTid, "APDE", 4) == 0    // Pokemon Dash
	) {
		// read statically the 6D2C4 function data 
        fileRead(0x0218A960, file, 0x000CE000, 0x1000, 0);
        fileRead(0x020D6340, file, 0x000D2400, 0x200, 0);
        fileRead(0x020D6140, file, 0x000CFA00, 0x200, 0);
        fileRead(0x023B9F00, file, 0x000CFC00, 0x2800, 0);
	}*/

	/*isGSDD = (strncmp(romTid, "BO5", 3) == 0)			// Golden Sun: Dark Dawn
        || (strncmp(romTid, "TBR", 3) == 0)			    // Disney Pixar Brave 
        ;*/


	// Load binaries into memory
	fileRead(dsiHeaderTemp->ndshdr.arm9destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm9romOffset, dsiHeaderTemp->ndshdr.arm9binarySize, !sdRead, 0);
	if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION == 0) {
		fileRead(dsiHeaderTemp->ndshdr.arm7destination, file, srlAddr+dsiHeaderTemp->ndshdr.arm7romOffset, dsiHeaderTemp->ndshdr.arm7binarySize, !sdRead, 0);
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

static bool isROMLoadableInRAM(const tDSiHeader* dsiHeader, const tNDSHeader* ndsHeader, const char* romTid, const module_params_t* moduleParams) {
	/*dbg_printf("Console model: ");
	dbg_hexa(consoleModel);
	dbg_printf("\nromTid: ");
	dbg_printf(romTid);
	dbg_printf("\n");*/

	bool res = false;
	if ((strncmp(romTid, "UBR", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "UOR", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "KPP", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "KPF", 3) == 0 && consoleModel>0)
	|| (strncmp(romTid, "APD", 3) != 0
	 && strncmp(romTid, "A24", 3) != 0
	 && strncmp(romTid, "UBR", 3) != 0
	 && strncmp(romTid, "UOR", 3) != 0
	 && strncmp(romTid, "KPP", 3) != 0
	 && strncmp(romTid, "KPF", 3) != 0)
	) {
		u32 romSize = (ndsHeader->romSize-0x8000)+0x88;
		res = ((dsiModeConfirmed && consoleModel>0 && ((ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) ? getIRomSizeNoArmBins(dsiHeader)+ioverlaysSize : romSize) < 0x01000000)
			|| (!dsiModeConfirmed && isSdk5(moduleParams) && consoleModel>0 && getRomSizeNoArmBins(ndsHeader) < 0x01000000)
			|| (!dsiModeConfirmed && !isSdk5(moduleParams) && consoleModel>0 && getRomSizeNoArmBins(ndsHeader) < 0x01800000)
			|| (!dsiModeConfirmed && !isSdk5(moduleParams) && consoleModel==0 && getRomSizeNoArmBins(ndsHeader) < 0x00800000));

	  if (!res && extendedMemory && !dsiModeConfirmed) {
		res = ((consoleModel>0 && romSize < (extendedMemory==2 ? 0x01C80000 : 0x01C00000))
			|| (consoleModel==0 && romSize < (extendedMemory==2 ? 0x00C80000 : 0x00C00000)));
		extendedMemoryConfirmed = res;
		moreMemory = (romSize >= (consoleModel==0 ? 0x00C00000 : 0x01C00000));
	  }
	}
	if ((strncmp(romTid, "HND", 3) == 0)
	|| (strncmp(romTid, "HNE", 3) == 0))
	{
		extendedMemoryConfirmed = true;
		return extendedMemoryConfirmed;
	}
	return res;
}

static tNDSHeader* loadHeader(tDSiHeader* dsiHeaderTemp, const module_params_t* moduleParams, int dsiMode, bool isDSiWare) {
	tNDSHeader* ndsHeader = (tNDSHeader*)(isSdk5(moduleParams) ? NDS_HEADER_SDK5 : NDS_HEADER);
	/*if (isGSDD) {
		ndsHeader = (tNDSHeader*)(NDS_HEADER_4MB);
	}*/

	// Copy the header to its proper location
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, (char*)ndsHeader, 0x170);
	//dmaCopyWords(3, &dsiHeaderTemp.ndshdr, ndsHeader, sizeof(dsiHeaderTemp.ndshdr));
	*ndsHeader = dsiHeaderTemp->ndshdr;
	if (dsiMode > 0) {
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

	if (useTwlCfg && (language == 0xFF || language == -1)) {
		language = twlCfgLang;
	}

	if (language >= 0 && language <= 7) {
		// Change language
		personalData->language = language; //*(u8*)((u32)ndsHeader - 0x11C) = language;
		if (ROMsupportsDsiMode(ndsHeader)) {
			*(u8*)0x02000406 = language;
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

static void loadOverlaysintoRAM(const tNDSHeader* ndsHeader, const char* romTid, const module_params_t* moduleParams, aFile file) {
	// Load overlays into RAM
	if (consoleModel>0
	? overlaysSize<=((isSdk5(moduleParams)||dsiModeConfirmed ? 0x1000000 : 0x1800000))
	: overlaysSize<=((isSdk5(moduleParams)||dsiModeConfirmed ? 0x700000 : 0x800000)))
	{
		u32 overlaysLocation = (u32)((isSdk5(moduleParams) || dsiModeConfirmed) ? ROM_SDK5_LOCATION : ROM_LOCATION);
		if (extendedMemoryConfirmed) {
			overlaysLocation = (u32)ROM_LOCATION_EXT;
		} else if (consoleModel == 0 && isSdk5(moduleParams)) {
			overlaysLocation = (u32)CACHE_ADRESS_START;

			if (strncmp(romTid, "BKW", 3) == 0 || strncmp(romTid, "VKG", 3) == 0) {
				overlaysLocation = (u32)CACHE_ADRESS_START_low;
			}
		}
		fileRead((char*)overlaysLocation, file, 0x4000 + ndsHeader->arm9binarySize, overlaysSize, !sdRead, 0);

		if (!isSdk5(moduleParams) && *(u32*)((overlaysLocation-0x4000-ndsHeader->arm9binarySize)+0x003128AC) == 0x4B434148) {
			*(u32*)((overlaysLocation-0x4000-ndsHeader->arm9binarySize)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday
		}
	}
}

static void loadIOverlaysintoRAM(const tDSiHeader* dsiHeader, aFile file) {
	// Load overlays into RAM
	if (ioverlaysSize>0x700000) return;

	fileRead((char*)ROM_SDK5_LOCATION+getIRomSizeNoArmBins(dsiHeader), file, (u32)dsiHeader->arm9iromOffset+dsiHeader->arm9ibinarySize, ioverlaysSize, !sdRead, 0);
}

static void loadROMintoRAM(const tNDSHeader* ndsHeader, bool isDSBrowser, const module_params_t* moduleParams, aFile* romFile, aFile* savFile) {
	// Load ROM into RAM
	u32 romLocation = (u32)((isSdk5(moduleParams) || dsiModeConfirmed) ? ROM_SDK5_LOCATION : ROM_LOCATION);
	if (extendedMemoryConfirmed) {
		romLocation = (u32)ROM_LOCATION_EXT;
	}

	u32 romOffset = 0x8000;
	u32 romSize = (ndsHeader->romSize-0x8000)+0x88;
	u32 romSizeLimit = (consoleModel==0 ? 0x00C00000 : 0x01C00000);
	if (romSize >= romSizeLimit) {
		tonccpy((char*)IMAGES_LOCATION-0x40000, romFile->fatTableCache, 0x80000);
		romFile->fatTableCache = (u32*)((char*)IMAGES_LOCATION-0x40000);
		tonccpy((char*)ce7Location+0xFC00, savFile->fatTableCache, 0x28000);
		savFile->fatTableCache = (u32*)((char*)ce7Location+0xFC00);

		fileRead((char*)romLocation, *romFile, romOffset, romSizeLimit, !sdRead, 0);
		fileRead((char*)ROM_LOCATION_EXT_P2, *romFile, romOffset + romSizeLimit, romSize-romSizeLimit, !sdRead, 0);

		toncset((char*)IMAGES_LOCATION-0x40000, 0, 0x80000);
		romFile->fatTableCached = false;
		//savFile->fatTableCached = false;
	} else {
		if (extendedMemoryConfirmed) {
			// Move FAT tables to avoid being overwritten by the ROM
			tonccpy((char*)IMAGES_LOCATION-0x40000, romFile->fatTableCache, 0x80000);
			romFile->fatTableCache = (u32*)((char*)IMAGES_LOCATION-0x40000);
			tonccpy((char*)ce7Location+0xFC00, savFile->fatTableCache, 0x28000);
			savFile->fatTableCache = (u32*)((char*)ce7Location+0xFC00);
		} else if (!dsiModeConfirmed) {
			if (!isDSBrowser) {
				fileRead((char*)ndsHeader->arm9destination + 0x0A400000 + 0x4000, *romFile, romOffset, ndsHeader->arm9binarySize - 0x4000, !sdRead, 0); // ARM9 binary
				fileRead((char*)ndsHeader->arm7destination + 0x0A400000, *romFile, ndsHeader->arm7romOffset, ndsHeader->arm7binarySize, !sdRead, 0); // ARM7 binary
			}
			fileRead((char*)romLocation, *romFile, 0x4000 + ndsHeader->arm9binarySize, overlaysSize, !sdRead, 0); // Overlays
			if (!isSdk5(moduleParams) && *(u32*)((romLocation-0x4000-ndsHeader->arm9binarySize)+0x003128AC) == 0x4B434148) {
				*(u32*)((romLocation-0x4000-ndsHeader->arm9binarySize)+0x3128AC) = 0xA00;	// Primary fix for Mario's Holiday
			}
		}
		fileRead((char*)romLocation + ((dsiModeConfirmed || extendedMemoryConfirmed) ? 0 : overlaysSize), *romFile,
			(dsiModeConfirmed || extendedMemoryConfirmed) ? romOffset : (ndsHeader->arm7romOffset + ndsHeader->arm7binarySize),
			(dsiModeConfirmed || extendedMemoryConfirmed) ? romSize : getRomSizeNoArmBins(ndsHeader)-overlaysSize,
		!sdRead, 0);
		if (extendedMemoryConfirmed) {
			toncset((char*)IMAGES_LOCATION-0x40000, 0, 0x80000);
			romFile->fatTableCached = false;
		}
	}

	dbg_printf("ROM loaded into RAM\n");
	if (extendedMemoryConfirmed) {
		dbg_printf("Complete ");
		if (romSize >= romSizeLimit) {
			dbg_printf(consoleModel==0 ? "12.5MB" : "28.5MB");
		} else {
			dbg_printf(consoleModel==0 ? "12MB" : "28MB");
		}
		dbg_printf(" used\n");
	}
	dbg_printf("\n");
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

static void setMemoryAddress(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool isDSiWare) {
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

		if (gameOnFlashcard || !isDSiWare) {
			tonccpy((u32*)0x02FFC000, (u32*)DSI_HEADER_SDK5, 0x1000);	// Make a duplicate of DSi header
			tonccpy((u32*)0x02FFFA80, (u32*)NDS_HEADER_SDK5, 0x160);	// Make a duplicate of DS header
		}

		/* *(u32*)(0x02FFA680) = 0x02FD4D80;
		*(u32*)(0x02FFA684) = 0x00000000;
		*(u32*)(0x02FFA688) = 0x00001980;

		*(u32*)(0x02FFF00C) = 0x0000007F;
		*(u32*)(0x02FFF010) = 0x550E25B8;
		*(u32*)(0x02FFF014) = 0x02FF4000; */

		// Set region flag
		if (region == 0xFE || region == -2) {
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
		} else if (region == 0xFF || region == -1) {
			u8 newRegion = 0;
			u8 country = *(u8*)0x02000405;
			if (country == 0x01) {
				newRegion = 0;	// Japan
			} else if (country == 0xA0) {
				newRegion = 4;	// China
			} else if (country == 0x88) {
				newRegion = 5;	// Korea
			} else if (country == 0x41 || country == 0x5F) {
				newRegion = 3;	// Australia
			} else if ((country >= 0x08 && country <= 0x34) || country == 0x99 || country == 0xA8) {
				newRegion = 1;	// USA
			} else if (country >= 0x40 && country <= 0x70) {
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
		}
	}

	if (!gameOnFlashcard && isDSiWare) {
		*(u16*)(0x02FFFC40) = 3;						// Boot Indicator (NAND/SD)
		return;
	}

	u32 chipID = getChipId(ndsHeader, moduleParams);
    dbg_printf("chipID: ");
    dbg_hexa(chipID);
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

	/*if (gbaRomFound) {
		*(u16*)0x027FFC30 = *(u16*)0x0D0000BE;
		tonccpy((u8*)0x027FFC32, (u8*)0x0D0000B5, 3);
		*(u16*)0x027FFC36 = *(u16*)0x0D0000B0;
		*(u32*)0x027FFC38 = *(u32*)0x0D0000AC;
	}*/

	if (softResetParams[0] != 0xFFFFFFFF) {
		u32* resetParamLoc = (u32*)(isSdk5(moduleParams) ? RESET_PARAM_SDK5 : RESET_PARAM);
		resetParamLoc[0] = softResetParams[0];
		resetParamLoc[1] = softResetParams[1];
		resetParamLoc[2] = softResetParams[2];
		resetParamLoc[3] = softResetParams[3];
	}

	*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 1;						// Boot Indicator (Booted from card for SDK5) -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr

	const char* romTid = getRomTid(ndsHeader);
	if (!dsiModeConfirmed && 
		(strncmp(romTid, "KPP", 3) == 0 	// Pop Island
	  || strncmp(romTid, "KPF", 3) == 0)	// Pop Island: Paperfield
	)
	{
		*((u16*)(isSdk5(moduleParams) ? 0x02fffc40 : 0x027ffc40)) = 2;					// Boot Indicator (Cloneboot/Multiboot)
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
	if (dsiSD) {
		if (!FAT_InitFiles(true, false, 0)) {
			nocashMessage("!FAT_InitFiles");
			errorOutput();
			//return -1;
		}
	}

	if (gameOnFlashcard || saveOnFlashcard) {
		sdRead = false;
		// Init Slot-1 card
		if (!FAT_InitFiles(initDisc, true, 0)) {
			nocashMessage("!FAT_InitFiles");
			//return -1;
		}
		sdRead = dsiSD;
	}

	if (logging) {
		enableDebug(getBootFileCluster("NDSBTSRP.LOG", 0));
	}

	if (gameOnFlashcard) sdRead = false;

	aFile srParamsFile = getFileFromCluster(srParamsFileCluster);
	fileRead((char*)&softResetParams, srParamsFile, 0, 0x10, !sdRead, -1);
	bool softResetParamsFound = (softResetParams[0] != 0xFFFFFFFF);
	if (softResetParamsFound) {
		u32 clearBuffer = 0xFFFFFFFF;
		fileWrite((char*)&clearBuffer, srParamsFile, 0, 0x4, !sdRead, -1);
		clearBuffer = 0;
		fileWrite((char*)&clearBuffer, srParamsFile, 0x4, 0x4, !sdRead, -1);
		fileWrite((char*)&clearBuffer, srParamsFile, 0x8, 0x4, !sdRead, -1);
		fileWrite((char*)&clearBuffer, srParamsFile, 0xC, 0x4, !sdRead, -1);
	}

	// ROM file
	aFile* romFile = (aFile*)(dsiSD ? ROM_FILE_LOCATION : ROM_FILE_LOCATION_ALT);
	*romFile = getFileFromCluster(storedFileCluster);

	sdRead = (saveOnFlashcard ? false : dsiSD);

	// Sav file
	aFile* savFile = (aFile*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT);
	*savFile = getFileFromCluster(saveFileCluster);

	sdRead = (gameOnFlashcard ? false : dsiSD);

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

	if (gameOnFlashcard || !isDSiWare) {
		u32 currentFatTableVersion = 3;
		extern u32 currentClusterCacheSize;

		// FAT table file
		aFile fatTableFile = getFileFromCluster(fatTableFileCluster);
		if (cacheFatTable && fatTableFile.firstCluster != CLUSTER_FREE) {
			fileRead((char*)0x2670000, fatTableFile, 0, 0x400, !sdRead, -1);
		}
		u32 fatTableVersion = *(u32*)(0x2670100);
		bool fatTableEmpty = (*(u32*)(0x2670200) == 0);

		if (*(u32*)(0x2670040) != storedFileCluster
		|| *(u32*)(0x2670044) != romSize)
		{
			fatTableEmpty = true;
		}

		if (!gameOnFlashcard) {
			if (*(u32*)(0x2670048) != saveFileCluster
			|| *(u32*)(0x267004C) != saveSize)
			{
				fatTableEmpty = true;
			}
		}
		
		if (fatTableVersion != currentFatTableVersion) {
			fatTableEmpty = true;
		}

		if (fatTableEmpty) {
			if (!softResetParamsFound) {
				pleaseWaitOutput();
			}
			buildFatTableCache(romFile, !sdRead, 0);
		} else {
			tonccpy((char*)(dsiSD ? ROM_FILE_LOCATION : ROM_FILE_LOCATION_ALT), (char*)0x2670000, sizeof(aFile));
		}
		//if (gameOnFlashcard) {
			tonccpy((char*)ROM_FILE_LOCATION_MAINMEM, (char*)(dsiSD ? ROM_FILE_LOCATION : ROM_FILE_LOCATION_ALT), sizeof(aFile));
		//}

		sdRead = (saveOnFlashcard ? false : dsiSD);

		if (savFile->firstCluster != CLUSTER_FREE) {
			if (fatTableEmpty) {
				buildFatTableCache(savFile, !sdRead, 0);
			} else {
				tonccpy((char*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT), (char*)0x2670020, sizeof(aFile));
			}
			//if (saveOnFlashcard) {
				tonccpy((char*)SAV_FILE_LOCATION_MAINMEM, (char*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT), sizeof(aFile));
			//}
		}

		if (gameOnFlashcard) sdRead = false;

		if (fatTableEmpty) {
			tonccpy((char*)0x2670000, (char*)(dsiSD ? ROM_FILE_LOCATION : ROM_FILE_LOCATION_ALT), sizeof(aFile));
			tonccpy((char*)0x2670020, (char*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT), sizeof(aFile));
			*(u32*)(0x2670040) = storedFileCluster;
			*(u32*)(0x2670044) = romSize;
			*(u32*)(0x2670048) = saveFileCluster;
			*(u32*)(0x267004C) = saveSize;
			*(u32*)(0x2670050) = currentClusterCacheSize;
			*(u32*)(0x2670100) = currentFatTableVersion;
			if (cacheFatTable) {
				fileWrite((char*)0x2670000, fatTableFile, 0, 0x200, !sdRead, -1);
				fileWrite((char*)0x2700000, fatTableFile, 0x200, currentClusterCacheSize, !sdRead, -1);
			}
		} else {
			currentClusterCacheSize = *(u32*)(0x2670050);
			fileRead((char*)0x2700000, fatTableFile, 0x200, currentClusterCacheSize, !sdRead, -1);
		}

		toncset((u32*)0x02670000, 0, 0x400);
	}

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
	if (dsiModeConfirmed) {
		/*if (consoleModel == 0 && !isDSiWare && !gameOnFlashcard) {
			dbg_printf("Cannot use DSi mode on DSi SD\n");
			errorOutput();
		}*/
		if (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && !isDSiWare) {
			/*if (consoleModel > 0) {
				tonccpy((char*)0x0DF80000, (char*)0x02700000, 0x80000);	// Move FAT table cache to debug RAM
				romFile->fatTableCache = (u32)romFile->fatTableCache+0xB880000;
				savFile->fatTableCache = (u32)savFile->fatTableCache+0xB880000;
			} else {*/
				tonccpy((char*)0x02F00000, (char*)0x02700000, 0x80000);	// Move FAT table cache elsewhere
				romFile->fatTableCache = (u32)romFile->fatTableCache+0x800000;
				savFile->fatTableCache = (u32)savFile->fatTableCache+0x800000;
			//}
			tonccpy((char*)INGAME_MENU_LOCATION_TWLSDK, (char*)INGAME_MENU_LOCATION, 0xA000);
			toncset((char*)INGAME_MENU_LOCATION, 0, 0x8A000);
		}
	} else {
		toncset((u32*)0x02400000, 0, 0x20);
		toncset((u32*)0x02E80000, 0, 0x800);
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
	aFile patchOffsetCacheFile = getFileFromCluster(patchOffsetCacheFileCluster);
	if (srlAddr == 0) {
		fileRead((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), !sdRead, -1);
	}
	u16 prevPatchOffsetCacheFileVersion = patchOffsetCache.ver;

	nocashMessage("Loading the header...\n");

	bool foundModuleParams;
	module_params_t* moduleParams = loadModuleParams(&dsiHeaderTemp.ndshdr, &foundModuleParams);
    dbg_printf("sdk_version: ");
    dbg_hexa(moduleParams->sdk_version);
    dbg_printf("\n"); 

	ndsHeader = loadHeader(&dsiHeaderTemp, moduleParams, dsiModeConfirmed, isDSiWare);

	if (gameOnFlashcard || !isDSiWare || (isDSiWare && REG_SCFG_EXT == 0 && *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0 && *(u32*)0x02FFE1A0 == 0x00403000)) {
		ensureBinaryDecompressed(&dsiHeaderTemp.ndshdr, moduleParams);
	}
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

	if (!gameOnFlashcard && isDSiWare) {
		tonccpy((char*)0x02FFC000, (char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0x400);
		toncset((char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0, 0x400);

		if (*(u8*)0x02FFE1BF & BIT(0)) {
			*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
			DSiTouchscreenMode();
		} else {
			*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
			NDSTouchscreenMode();
			if (macroMode) {
				u32 temp = readPowerManagement(PM_CONTROL_REG) & (~(PM_BACKLIGHT_TOP & 0xFFFF));
				writePowerManagement(PM_CONTROL_REG, temp);
			}
		}
		*(u16*)0x4000500 = 0x807F;

		//if (*(u8*)0x02FFE1BF & BIT(2)) {
			//const char* newBannerPath = "sdmc:/";
			//toncset((char*)0x020925A0, 0, 0xE);
			//tonccpy((char*)0x020925A0, newBannerPath, 6);
			//toncset((char*)0x020A8D84, 0, 0xE);
			//tonccpy((char*)0x020A8D84, newBannerPath, 6);
			//const char* newBannerPath = "sdmc:/banner.sav";
			//toncset((char*)0x02E9A828, 0, 0x1C);
			//tonccpy((char*)0x02E9A834, newBannerPath, 16);
		//}

		newArm7binarySize = ndsHeader->arm7binarySize;
		newArm7ibinarySize = __DSiHeader->arm7ibinarySize;

		extern void rsetPatchCache(bool dsiWare);
		rsetPatchCache(true);

		if (REG_SCFG_EXT == 0) {
			if (*(u32*)0x02FFE1A0 == 0x00403000) {
				extern void patchCardIdThing_cont(const tNDSHeader* ndsHeader, bool usesThumb, bool searchAgainForThumb);
				patchCardIdThing_cont(ndsHeader, false, true);

				if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0) {
					// Replace incompatible ARM7 binary
					newArm7binarySize = *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION;
					newArm7ibinarySize = *(u32*)DONOR_ROM_ARM7I_SIZE_LOCATION;
					*(u32*)0x02FFE1A0 = *(u32*)DONOR_ROM_MBK6_LOCATION;
					tonccpy(ndsHeader->arm7destination, (u8*)DONOR_ROM_ARM7_LOCATION, newArm7binarySize);
				}

				if (*(u32*)0x02FFE1D4 >= 0x03000000 && *(u32*)0x02FFE1D4 < 0x03040000) {
					// Relocate device list
					*(u32*)0x02FFE1D4 += 0x7D0000;
				}
			}

			if (newArm7binarySize != patchOffsetCache.a7BinSize) {
				rsetA7Cache();
				patchOffsetCache.a7BinSize = newArm7binarySize;
				patchOffsetCacheChanged = true;
			}

			extern void patchScfgExt(const tNDSHeader* ndsHeader);
			patchScfgExt(ndsHeader);
		}

		toncset((u32*)0x02680000, 0, 0x100000);

		errorCode = hookNdsRetailArm7(
			(cardengineArm7*)NULL,
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
			gameOnFlashcard,
			saveOnFlashcard,
			language,
			dsiModeConfirmed,
			dsiSD,
			extendedMemoryConfirmed,
			0,
			consoleModel,
			romRead_LED,
			dmaRomRead_LED
		);
		if (errorCode == ERR_NONE) {
			nocashMessage("Card hook successful");
		} else {
			nocashMessage("Card hook failed");
			errorOutput();
		}

		if (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheChanged) {
			fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), !sdRead, -1);
		}
	} else {
		const char* romTid = getRomTid(ndsHeader);
		if (strncmp(romTid, "UBR", 3) == 0) {
			toncset((char*)0x02400000, 0xFF, 0xC0);
			toncset((u8*)0x024000B2, 0, 3);
			toncset((u8*)0x024000B5, 0x24, 3);
			*(u16*)0x024000BE = 0x7FFF;
			*(u16*)0x024000CE = 0x7FFF;
		} /*else // GBA file
		if (consoleModel > 0 && !dsiModeConfirmed && !isSdk5(moduleParams) && gbaRomSize <= 0x1000000) {
			aFile* gbaFile = (aFile*)(dsiSD ? GBA_FILE_LOCATION : GBA_FILE_LOCATION_ALT);
			*gbaFile = getFileFromCluster(gbaFileCluster);
			gbaRomFound = (gbaFile->firstCluster != CLUSTER_FREE);
			patchSlot2Addr(ndsHeader);
		}*/

		if (!gameOnFlashcard && REG_SCFG_EXT != 0 && !(REG_SCFG_MC & BIT(0))) {
			if (specialCard) {
				// Enable Slot-1 for games that use IR
				my_enableSlot1();
			} else {
				my_disableSlot1();
			}
		}

		if (!dsiModeConfirmed || !ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) || (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && !(*(u8*)0x02FFE1BF & BIT(0)))) {
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
			if ((u8)a9ScfgRom != 1 && cdcReadReg(CDC_SOUND, 0x22) != 0xF0) {
				*(u8*)0x02FFE1BF &= ~BIT(0);	// Set NTR touch mode (Disables camera)
			} else if (*(u8*)0x02FFE1BF & BIT(0)) {
				*(u16*)0x4004700 = (soundFreq ? 0xC00F : 0x800F);
				DSiTouchscreenMode();
				*(u16*)0x4000500 = 0x807F;
			}
		}

		if (!dsiModeConfirmed) {
			/*if (
				strncmp(romTid, "APD", 3) != 0				// Pokemon Dash
			 && strncmp(romTid, "A24", 3) != 0				// Pokemon Dash (Kiosk Demo)
			) {*/
				NTR_BIOS();
			//}
		}

		// If possible, set to load ROM into RAM
		u32 ROMinRAM = isROMLoadableInRAM(&dsiHeaderTemp, &dsiHeaderTemp.ndshdr, romTid, moduleParams);

		nocashMessage("Trying to patch the card...\n");

		if (!dsiSD) {
			ce7Location = CARDENGINEI_ARM7_LOCATION_ALT;
		}

		bool useSdk5ce7 = (!extendedMemoryConfirmed && isSdk5(moduleParams) &&
		   (!dsiSD || (ROMsupportsDsiMode(&dsiHeaderTemp.ndshdr) && dsiModeConfirmed))
		);

		if (useSdk5ce7) {
			ce7Location = CARDENGINEI_ARM7_SDK5_LOCATION;
		}

		if (isSdk5(moduleParams)) {
			tonccpy((char*)ROM_FILE_LOCATION_SDK5, (char*)(dsiSD ? ROM_FILE_LOCATION : ROM_FILE_LOCATION_ALT), sizeof(aFile));
			tonccpy((char*)SAV_FILE_LOCATION_SDK5, (char*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT), sizeof(aFile));
		}

		//rebootConsole = (fatTableEmpty && !useSdk5ce7 && !gameOnFlashcard && (REG_SCFG_EXT == 0));

		tonccpy((u32*)ce7Location, (u32*)(useSdk5ce7 ? CARDENGINEI_ARM7_SDK5_BUFFERED_LOCATION : CARDENGINEI_ARM7_BUFFERED_LOCATION), 0xB000);
		if (gameOnFlashcard || saveOnFlashcard) {
			if (!dldiPatchBinary((data_t*)ce7Location, 0xA800)) {
				dbg_printf("ce7 DLDI patch failed\n");
				errorOutput();
			}
		}
		tonccpy((char*)ce7Location-0x8400, (char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0x400);
		toncset((char*)CHEAT_ENGINE_BUFFERED_LOCATION, 0, 0x400);

		if (isSdk5(moduleParams)) {
			ce9Location = CARDENGINEI_ARM9_SDK5_LOCATION;
			if (ROMinRAM) {
				if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
					ce9Location = CARDENGINEI_ARM9_TWLSDK_LOCATION;
				} else if ((u32)ndsHeader->arm9destination == 0x02004000) {
					ce9Location = CARDENGINEI_ARM9_CACHED_LOCATION2;
				} else if (extendedMemoryConfirmed && (moreMemory || REG_SCFG_EXT == 0)) {
					ce9Location = CARDENGINEI_ARM9_CACHED_LOCATION_ROMINRAM;
				} else if (REG_SCFG_EXT != 0) {
					ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
				}
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION, 0x1C00);
				if (((u32)ndsHeader->arm9destination != 0x02004000 && extendedMemoryConfirmed && (moreMemory || REG_SCFG_EXT == 0)) || (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed)) {
					patchHiHeapPointer(moduleParams, ndsHeader, ROMinRAM);
				}
				if (ce9Location != CARDENGINEI_ARM9_LOCATION_DSI_WRAM) {
					relocate_ce9(CARDENGINEI_ARM9_LOCATION_DSI_WRAM,ce9Location,0x1C00);
				}
			} else if (gameOnFlashcard) {
				if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
					ce9Location = CARDENGINEI_ARM9_TWLSDK_LOCATION;
					patchHiHeapPointer(moduleParams, ndsHeader, ROMinRAM);
				} else {
					ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
				}
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_SDK5_DLDI_BUFFERED_LOCATION, 0x7000);
				if (!dldiPatchBinary((data_t*)ce9Location, 0x7000)) {
					dbg_printf("ce9 DLDI patch failed\n");
					errorOutput();
				}
			} else if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
				ce9Location = CARDENGINEI_ARM9_TWLSDK_LOCATION;
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION, 0x5000);
				patchHiHeapPointer(moduleParams, ndsHeader, ROMinRAM);
			} else {
				if (REG_SCFG_EXT != 0) {
					ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
				}
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION, 0x5000);
			}
		} else if (gameOnFlashcard && !ROMinRAM) {
			ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
			tonccpy((u32*)CARDENGINEI_ARM9_LOCATION_DSI_WRAM, (u32*)CARDENGINEI_ARM9_DLDI_BUFFERED_LOCATION, 0x7000);
			if (!dldiPatchBinary((data_t*)ce9Location, 0x7000)) {
				dbg_printf("ce9 DLDI patch failed\n");
				errorOutput();
			}
		} else if (extendedMemoryConfirmed) {
			if (moreMemory || REG_SCFG_EXT == 0) {
				ce9Location = (moduleParams->sdk_version >= 0x2008000) ? (u32)patchHiHeapPointer(moduleParams, ndsHeader, ROMinRAM) : CARDENGINEI_ARM9_CACHED_LOCATION_ROMINRAM;
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION, 0x1C00);
				relocate_ce9(CARDENGINEI_ARM9_LOCATION_DSI_WRAM,ce9Location,0x1C00);
			} else {
				ce9Location = CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
				tonccpy((u32*)ce9Location, (u32*)CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION, 0x1C00);
			}
		} else {
			ce9Location = (REG_SCFG_EXT != 0) ? CARDENGINEI_ARM9_LOCATION_DSI_WRAM : CARDENGINEI_ARM9_LOCATION;
			u16 size = (ROMinRAM ? 0x1C00 : 0x4000);
			tonccpy((u32*)ce9Location, (u32*)(ROMinRAM ? CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION : CARDENGINEI_ARM9_BUFFERED_LOCATION), size);
			if (ROMinRAM && ce9Location == CARDENGINEI_ARM9_LOCATION) {
				relocate_ce9(CARDENGINEI_ARM9_LOCATION_DSI_WRAM,ce9Location,0x1C00);
			}
		}

		toncset((u32*)CARDENGINEI_ARM7_BUFFERED_LOCATION, 0, 0x48000);

		if (*(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0 && ndsHeader->unitCode > 0 && dsiModeConfirmed) {
			*(u32*)0x02FFE1A0 = *(u32*)DONOR_ROM_MBK6_LOCATION;
		}

		patchBinary(ndsHeader);
		errorCode = patchCardNds(
			(cardengineArm7*)ce7Location,
			(cardengineArm9*)ce9Location,
			ndsHeader,
			moduleParams,
			(ROMsupportsDsiMode(ndsHeader)
			|| strncmp(romTid, "ASK", 3) == 0 // Lost in Blue
			|| strncmp(romTid, "AKD", 3) == 0 // Trauma Center: Under the Knife
			) ? 0 : 1,
			0,
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
			gameOnFlashcard,
			saveOnFlashcard,
			language,
			dsiModeConfirmed,
			dsiSD,
			extendedMemoryConfirmed,
			ROMinRAM,
			consoleModel,
			romRead_LED,
			dmaRomRead_LED
		);
		if (errorCode == ERR_NONE) {
			nocashMessage("Card hook successful");
		} else {
			nocashMessage("Card hook failed");
			errorOutput();
		}

		hookNdsRetailArm9(
			(cardengineArm9*)ce9Location,
			moduleParams,
			romFile->firstCluster,
			savFile->firstCluster,
			saveOnFlashcard,
			strncmp(romTid, "B3R", 3)==0 ? 0x8000 : 0x4000,
			extendedMemoryConfirmed,
			ROMinRAM,
			dsiModeConfirmed,
			supportsExceptionHandler(romTid),
			consoleModel
		);

		if (srlAddr == 0 && (prevPatchOffsetCacheFileVersion != patchOffsetCacheFileVersion || patchOffsetCacheChanged)) {
			fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents), !sdRead, -1);
		}

		/*if (gbaRomFound && !extendedMemoryConfirmed) {
			aFile* gbaFile = (aFile*)(dsiSD ? GBA_FILE_LOCATION : GBA_FILE_LOCATION_ALT);
			//fileRead((char*)0x0D000000, *gbaFile, 0, 0xC0, -1);
			//fileRead((char*)0x0D0000CE, *gbaFile, 0x1FFFE, 2, -1);
			fileRead((char*)0x0D000000, *gbaFile, 0, gbaRomSize, 0);
			aFile* gbaSavFile = (aFile*)(dsiSD ? GBA_SAV_FILE_LOCATION : GBA_SAV_FILE_LOCATION_ALT);
			*gbaSavFile = getFileFromCluster(gbaSaveFileCluster);
			if (gbaSavFile->firstCluster != CLUSTER_FREE && gbaSaveSize <= 0x10000) {
				fileRead((char*)0x02600000, *gbaSavFile, 0, gbaSaveSize, 0);
			}
			dbg_printf("GBA ROM loaded\n");
		}*/

		if (isSdk5(moduleParams)) {
			tonccpy((u32*)UNPATCHED_FUNCTION_LOCATION_SDK5, (u32*)UNPATCHED_FUNCTION_LOCATION, 0x40);
			toncset((u32*)UNPATCHED_FUNCTION_LOCATION, 0, 0x40);
		}

		if (ROMinRAM) {
			if (extendedMemoryConfirmed) {
				tonccpy((u32*)0x023FF000, (u32*)(isSdk5(moduleParams) ? 0x02FFF000 : 0x027FF000), 0x1000);
				ndsHeader = (tNDSHeader*)NDS_HEADER_4MB;
			}
			loadROMintoRAM(ndsHeader, (strncmp(romTid, "UBR", 3) == 0), moduleParams, romFile, savFile);
			if (ROMsupportsDsiMode(ndsHeader) && dsiModeConfirmed) {
				loadIOverlaysintoRAM(&dsiHeaderTemp, *romFile);
			}
		} else if (consoleModel > 0 || ((ROMsupportsDsiMode(ndsHeader) || strncmp(romTid, "UBR", 3) != 0) && !dsiModeConfirmed)) {
			loadOverlaysintoRAM(ndsHeader, romTid, moduleParams, *romFile);
		}
		if (!gameOnFlashcard && !ROMinRAM && (romRead_LED==1 || dmaRomRead_LED==1)) {
			// Turn WiFi LED off
			i2cWriteRegister(0x4A, 0x30, 0x12);
		}

		if (srlAddr == 0 && apPatchFileCluster != 0 && !apPatchIsCheat && apPatchSize > 0 && apPatchSize <= 0x30000) {
			aFile apPatchFile = getFileFromCluster(apPatchFileCluster);
			dbg_printf("AP-fix found\n");
			fileRead((char*)IMAGES_LOCATION, apPatchFile, 0, apPatchSize, !sdRead, 0);
			if (applyIpsPatch(ndsHeader, (u8*)IMAGES_LOCATION, (*(u8*)(IMAGES_LOCATION+apPatchSize-1) == 0xA9), (isSdk5(moduleParams) || dsiModeConfirmed), ROMinRAM)) {
				dbg_printf("AP-fix applied\n");
			} else {
				dbg_printf("Failed to apply AP-fix\n");
			}
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

	toncset((u32*)IMAGES_LOCATION, 0, 0x40000);	// Clear nds-bootstrap images and IPS patch
	clearScreen();

	i2cReadRegister(0x4A, 0x10);	// Clear accidential POWER button press

	if ((!ROMsupportsDsiMode(ndsHeader) && !dsiModeConfirmed)
	/*|| (ROMsupportsDsiMode(ndsHeader) && !gameOnFlashcard)*/) {
		REG_SCFG_EXT &= ~(1UL << 31); // Lock SCFG
	}

	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);

	nocashMessage("Starting the NDS file...");
    setMemoryAddress(ndsHeader, moduleParams, isDSiWare);

	if (dsiSD && (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_DOWN | KEY_A)))) {
		aFile ramDumpFile = getFileFromCluster(ramDumpCluster);

		sdRead = dsiSD;
		fileWrite((char*)0x0C000000, ramDumpFile, 0, (consoleModel==0 ? 0x01000000 : 0x02000000), !sdRead, -1);		// Dump RAM
		//fileWrite((char*)dsiHeaderTemp.arm9idestination, ramDumpFile, 0, dsiHeaderTemp.arm9ibinarySize, !sdRead, -1);	// Dump (decrypted?) arm9 binary
	}

	if (ROMsupportsDsiMode(ndsHeader) && isDSiWare && !(REG_SCFG_ROM & BIT(9))) {
		*(vu32*)0x400481C = 0;				// Reset SD IRQ stat register
		*(vu32*)0x4004820 = 0x8B7F0305;	// Set SD IRQ mask register (Data won't read without the correct bytes!)
	}

	startBinary_ARM7();

	return 0;
}
