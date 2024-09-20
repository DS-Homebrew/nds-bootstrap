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

#include <stdlib.h>
#include <string.h>
#include <nds/ndstypes.h>
#include <nds/debug.h>
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#define ARM9
#undef ARM7
#include <nds/memory.h>
#include <nds/arm9/video.h>
#include <nds/arm9/input.h>
#undef ARM9
#define ARM7
#include <nds/arm7/audio.h>
#include <nds/arm7/codec.h>

#define REG_GPIO_WIFI *(vu16*)0x4004C04

#include "blocks_codec.h"
#include "tonccpy.h"
#include "dmaTwl.h"
//#include "my_sdmmc.h"
#include "my_fat.h"
#include "dldi_patcher.h"
#include "patch.h"
#include "hook.h"
#include "common.h"
#include "locations.h"
#include "i2c.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
extern unsigned long _start;
extern unsigned long storedFileCluster;
extern unsigned long initDisc;
//extern unsigned long wantToPatchDLDI;
extern unsigned long argStart;
extern unsigned long argSize;
extern unsigned long dsiSD;
extern u32 language;
extern u32 dsiMode;
extern u32 boostVram;
extern u32 ramDiskCluster;
extern u32 ramDiskSize;
extern u32 cfgCluster;
extern u32 cfgSize;
extern u32 romFileType;
extern u32 romIsCompressed;
extern u32 patchOffsetCacheFileCluster;
extern u32 srParamsFileCluster;
extern u32 ndsPreloaded;
extern u32 soundFreq;

u8 TWL_HEAD[0x1000] = {0};
static u32 sdEngineLocation = 0;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ        0x03

static void boot_readFirmware (uint32 address, uint8 * buffer, uint32 size) {
  uint32 index;

  // Read command
  while (REG_SPICNT & SPI_BUSY);
  REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
  REG_SPIDATA = FW_READ;
  while (REG_SPICNT & SPI_BUSY);

  // Set the address
  REG_SPIDATA =  (address>>16) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address>>8) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);

  for (index = 0; index < size; index++) {
    REG_SPIDATA = 0;
    while (REG_SPICNT & SPI_BUSY);
    buffer[index] = REG_SPIDATA & 0xFF;
  }
  REG_SPICNT = 0;
}


static inline void copyLoop (u32* dest, const u32* src, u32 size) {
	size = (size +3) & ~3;
	do {
		*dest++ = *src++;
	} while (size -= 4);
}

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

/*-------------------------------------------------------------------------
passArgs_ARM7
Copies the command line arguments to the end of the ARM9 binary,
then sets a flag in memory for the loaded NDS to use
--------------------------------------------------------------------------*/
static void passArgs_ARM7 (void) {
	u32 ARM9_DST = *((u32*)(NDS_HEADER + 0x028));
	u32 ARM9_LEN = *((u32*)(NDS_HEADER + 0x02C));
	u32* argSrc;
	u32* argDst;

	if (!argStart || !argSize) return;

	if ( ARM9_DST == 0 && ARM9_LEN == 0) {
		ARM9_DST = *((u32*)(NDS_HEADER + 0x038));
		ARM9_LEN = *((u32*)(NDS_HEADER + 0x03C));
	}

	argSrc = (u32*)(argStart + (int)&_start);

	argDst = (u32*)((ARM9_DST + ARM9_LEN + 3) & ~3);		// Word aligned

	if (ARM9_LEN > 0x380000) {
		argDst = (u32*)0x02FFA000;
	} else
	if (dsiModeConfirmed && (*(u8*)(NDS_HEADER + 0x012) & BIT(1)))
	{
		u32 ARM9i_DST = *((u32*)(TWL_HEAD + 0x1C8));
		u32 ARM9i_LEN = *((u32*)(TWL_HEAD + 0x1CC));
		if (ARM9i_LEN)
		{
			u32* argDst2 = (u32*)((ARM9i_DST + ARM9i_LEN + 3) & ~3);		// Word aligned
			if (argDst2 > argDst)
				argDst = argDst2;
		}
	}

	copyLoop(argDst, argSrc, argSize);

	__system_argv->argvMagic = ARGV_MAGIC;
	__system_argv->commandLine = (char*)argDst;
	__system_argv->length = argSize;
}




static void initMBK(void) {
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
	// WRAM-A mapped to the 0x3000000 - 0x303FFFF area : 256k
	REG_MBK6=0x00403000; // same as dsi-enhanced and certain dsiware
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
}

void memset_addrs_arm7(u32 start, u32 end)
{
	// toncset((u32*)start, 0, ((int)end - (int)start));
	dma_twlFill32(0, 0, (u32*)start, ((int)end - (int)start));
}

/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
static void resetMemory_ARM7 (void)
{
	int i, reg;
	u8 settings1, settings2;
	u32 settingsOffset = 0;

	REG_IME = 0;

	for (i=0; i<16; i++) {
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

	//clear out ARM7 DMA channels and timers
	for (i=0; i<4; i++) {
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

	sdEngineLocation = (*(u32*)0x02FFE1A0 == 0x080037C0) ? SDENGINE_LOCATION_ALT : SDENGINE_LOCATION;

	memset_addrs_arm7(0x03000000, 0x03800000 + 0x10000);
	if (ndsPreloaded) {
		dma_twlFill32(0, 0, (u32*)0x02200000, 0x180000);	// clear most of EWRAM (except pre-loaded ARM9 binary)
	} else {
		dma_twlFill32(0, 0, (u32*)0x02004000, 0x37C000);	// clear most of EWRAM
	}
	dma_twlFill32(0, 0, (u32*)0x02380000, 0x70000);
	dma_twlFill32(0, 0, (u32*)0x023F1000, 0xF000);
	if (romIsCompressed) {
		dma_twlFill32(0, 0, (u32*)0x02D00000, 0x300000);	// clear other part of EWRAM
	} else {
		dma_twlFill32(0, 0, (u32*)0x02400000, 0xC00000);	// clear other part of EWRAM
	}

	REG_IE = 0;
	REG_IF = ~0;
	REG_AUXIE = 0;
	REG_AUXIF = ~0;
	*(vu32*)0x0380FFFC = 0;  // IRQ_HANDLER ARM7 version
	*(vu32*)0x0380FFF8 = 0; // VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  //turn off power to stuff

	// Get settings location
	boot_readFirmware((u32)0x00020, (u8*)&settingsOffset, 0x2);
	settingsOffset *= 8;

	// Reload DS Firmware settings
	boot_readFirmware(settingsOffset + 0x070, &settings1, 0x1);
	boot_readFirmware(settingsOffset + 0x170, &settings2, 0x1);

	if ((settings1 & 0x7F) == ((settings2+1) & 0x7F)) {
		boot_readFirmware(settingsOffset + 0x000, (u8*)(NDS_HEADER-0x180), 0x70);
	} else {
		boot_readFirmware(settingsOffset + 0x100, (u8*)(NDS_HEADER-0x180), 0x70);
	}

	if ((*(u8*)0x02000400 & 0x0F) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (language == 0xFF || language == -1)) {
		language = *(u8*)0x02000406;
	}

	if (language >= 0 && language <= 7) {
		// Change language
		*(u8*)((u32)NDS_HEADER - 0x11C) = language;
	}
}


static u8 dsiFlags = 0;

static void loadBinary_ARM7 (aFile file)
{
	nocashMessage("loadBinary_ARM7");

	// read NDS header
	fileRead((char*)NDS_HEADER, file, 0, 0x170);
	fileRead((char*)&dsiFlags, file, 0x1BF, 1);

	// Load binaries into memory
	if (ndsPreloaded) {
		if ((u32)ndsHeader->arm9destination < 0x02004000) {
			fileRead(ndsHeader->arm9destination, file, ndsHeader->arm9romOffset, ndsHeader->arm9binarySize >= 0x4000 ? 0x4000 : ndsHeader->arm9binarySize);
		}
		if ((u32)ndsHeader->arm9destination+ndsHeader->arm9binarySize >= 0x02200000) {
			fileRead((char*)0x02200000, file, ndsHeader->arm9romOffset + 0x200000 + ((u32)ndsHeader->arm9destination - 0x02000000), ndsHeader->arm9binarySize-0x200000);
		}
	} else {
		fileRead(ndsHeader->arm9destination, file, ndsHeader->arm9romOffset, ndsHeader->arm9binarySize);
	}
	fileRead(ndsHeader->arm7destination, file, ndsHeader->arm7romOffset, ndsHeader->arm7binarySize);

	if (dsiModeConfirmed && (*(u8*)(NDS_HEADER + 0x012) & BIT(1)))
	{
		// Read full TWL header
		fileRead((char*)TWL_HEAD, file, 0, 0x1000);

		u32 ARM9i_SRC = *(u32*)(TWL_HEAD+0x1C0);
		char* ARM9i_DST = (char*)*(u32*)(TWL_HEAD+0x1C8);
		u32 ARM9i_LEN = *(u32*)(TWL_HEAD+0x1CC);
		u32 ARM7i_SRC = *(u32*)(TWL_HEAD+0x1D0);
		char* ARM7i_DST = (char*)*(u32*)(TWL_HEAD+0x1D8);
		u32 ARM7i_LEN = *(u32*)(TWL_HEAD+0x1DC);

		if (ARM9i_LEN)
			fileRead(ARM9i_DST, file, ARM9i_SRC, ARM9i_LEN);
		if (ARM7i_LEN)
			fileRead(ARM7i_DST, file, ARM7i_SRC, ARM7i_LEN);
	}
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

	const bool noSgba = (strncmp((const char*)0x04FFFA00, "no$gba", 6) == 0);

	// Touchscreen
	if (noSgba) {
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
	}
	cdcWriteReg(CDC_SOUND, 0x26, volLevel);
	cdcWriteReg(CDC_SOUND, 0x27, volLevel);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x03);
	cdcWriteReg(CDC_TOUCHCNT, 0x03, 0x00);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcWriteReg(CDC_SOUND, 0x22, 0x70);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);

	if (noSgba) {
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
	}

	// Finish up!
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(0xFF, 0x05, 0x00); //writeTSC(0x00, 0xFF);

	// Power management
	writePowerManagement(PM_READ_REGISTER, 0x00); //*(unsigned char*)0x40001C2 = 0x80, 0x00; // read PWR[0]   ;<-- also part of TSC !
	writePowerManagement(PM_CONTROL_REG, 0x0D); //*(unsigned char*)0x40001C2 = 0x00, 0x0D; // PWR[0]=0Dh    ;<-- also part of TSC !
}

static void NTR_BIOS() {
	// Switch to NTR mode BIOS (no effect with locked ARM7 SCFG)
	REG_SCFG_CLK = 0x181;
	REG_SCFG_ROM = 0x703;
	if (REG_SCFG_ROM == 0x703) {
		nocashMessage("Switched to NTR mode BIOS");
	}
}

/*-------------------------------------------------------------------------
startBinary_ARM7
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
static void startBinary_ARM7 (void) {
	REG_IME=0;
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);

	// Get the ARM9 to boot
	arm9_stateFlag = ARM9_BOOTBIN;

	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM7
	VoidFn arm7code = *(VoidFn*)(0x2FFFE34);
	arm7code();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main function

static u32 quickFind (const unsigned char* data, const unsigned char* search, u32 dataLen, u32 searchLen) {
	const int* dataChunk = (const int*) data;
	int searchChunk = ((const int*)search)[0];
	u32 i;
	u32 dataChunkEnd = (u32)(dataLen / sizeof(int));

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

extern unsigned char dldiMagicLoaderString[0xC];

int arm7_main (void) {
	nocashMessage("bootloader");

	initMBK();

	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	nocashMessage("Getting ARM7 to clear RAM...\n");

	resetMemory_ARM7();

	// Init card
	if(!FAT_InitFiles(initDisc))
	{
		nocashMessage("!FAT_InitFiles");
		return -1;
	}

	u32 standaloneFileCluster = CLUSTER_FREE;

	u32 srParams = 0xFFFFFFFF;
	aFile srParamsFile = getFileFromCluster(srParamsFileCluster);
	fileRead((char*)&standaloneFileCluster, srParamsFile, 0, 4);
	if (standaloneFileCluster != 0xFFFFFFFF) {
		fileWrite((char*)&srParams, srParamsFile, 0, 4);
		srParams = 0;
		fileWrite((char*)&srParams, srParamsFile, 4, 4);
		fileWrite((char*)&srParams, srParamsFile, 8, 4);
		fileWrite((char*)&srParams, srParamsFile, 0xC, 4);
	}

	const bool ramDiskFound = (ramDiskSize > 0);

	aFile romFile = getFileFromCluster(storedFileCluster);
	if (!ramDiskFound && (standaloneFileCluster != 0xFFFFFFFF)) {
		romFile = getFileFromCluster(standaloneFileCluster);
	}

	//const char* bootName = "BOOT.NDS";

	if ((romFile.firstCluster < CLUSTER_FIRST) || (romFile.firstCluster >= CLUSTER_EOF)) {
		//romFile = getBootFileCluster(bootName);
		return -1;
	}

	if (romFile.firstCluster == CLUSTER_FREE) {
		nocashMessage("fileCluster == CLUSTER_FREE");
		return -1;
	}

	if (ramDiskFound) {
		void* ramDiskLocation = (void*)(dsiMode ? RAM_DISK_LOCATION_DSIMODE : RAM_DISK_LOCATION);
		arm9_ramDiskCluster = ramDiskCluster;
		if (ramDiskSize < (dsiMode ? 0x01001000 : 0x01C01000)) {
			aFile ramDiskFile = getFileFromCluster(ramDiskCluster);
			if (romFileType != -1) {
				tonccpy ((char*)ramDiskLocation, (char*)0x06010000, (romFileType == 1) ? RAM_DISK_SNESROM : RAM_DISK_MDROM);
				toncset((u32*)0x06010000, 0, 0x10000);
				if (romIsCompressed) {
					u8* lz77RomSrc = (u8*)RAM_DISK_LOCATION_LZ77ROM;
					u32 leng = (lz77RomSrc[1] | (lz77RomSrc[2] << 8) | (lz77RomSrc[3] << 16));
					if (romFileType == 1) {
						*(u32*)((u8*)ramDiskLocation+RAM_DISK_SNESROMSIZE) = leng;
					} else {
						*(u32*)((u8*)ramDiskLocation+RAM_DISK_MDROMSIZE) = leng;
					}
					toncset((u32*)RAM_DISK_LOCATION_LZ77ROM, 0, 0x400000);	// clear compressed ROM
				} else {
					if (romFileType == 1) {
						*(u32*)((u8*)ramDiskLocation+RAM_DISK_SNESROMSIZE) = ramDiskSize;
					} else {
						*(u32*)((u8*)ramDiskLocation+RAM_DISK_MDROMSIZE) = ramDiskSize;
					}
					fileRead((char*)((romFileType == 1) ? ramDiskLocation+RAM_DISK_SNESROM : ramDiskLocation+RAM_DISK_MDROM), ramDiskFile, 0, ramDiskSize);
				}
				if (romFileType == 1 && cfgSize > 0) {
					// Load snemul.cfg
					*(u32*)((u8*)ramDiskLocation+RAM_DISK_SNESCFGSIZE) = cfgSize;
					aFile cfgFile = getFileFromCluster(cfgCluster);
					fileRead((char*)RAM_DISK_SNESCFG, cfgFile, 0, cfgSize);
				}
			} else {
				//buildFatTableCache(&ramDiskFile, 0);
				fileRead((char*)ramDiskLocation, ramDiskFile, 0, ramDiskSize);
				//toncset((u32*)0x023A0000, 0, 0x40000);
			}
		}
	}

	bool isGbaR2 = false;
	u32 bannerOffset = 0;
	char gbaR2Text[0x20];
	fileRead((char*)&bannerOffset, romFile, 0x48, 4);
	fileRead(gbaR2Text, romFile, bannerOffset+0x240, 0x20);
	isGbaR2 = (gbaR2Text[0] == 'G' && gbaR2Text[2] == 'B' && gbaR2Text[4] == 'A' && gbaR2Text[6] == 'R' && gbaR2Text[8] == 'u' && gbaR2Text[0xA] == 'n' && gbaR2Text[0xC] == 'n' && gbaR2Text[0xE] == 'e' && gbaR2Text[0x10] == 'r');

	if ((!soundFreq && (REG_SNDEXTCNT & BIT(13))) || (soundFreq && !(REG_SNDEXTCNT & BIT(13)))) {
		REG_SNDEXTCNT &= ~SNDEXTCNT_ENABLE; // Disable sound output: Runs before sound frequency change

		// Reconfigure clock dividers, based on the TSC2117 datasheet.
		// - We disable PLL, as MCLK is always equal to the sample frequency
		//   times 256, which is an integer multiple.
		// - We disable ADC NADC/MADC dividers, to share the DAC clock.
		// This also prevents us from having to reconfigure the PLL multipliers
		// for 32kHz/47kHz.
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_PLL_PR, 0);
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_MDAC, CDC_CONTROL_CLOCK_ENABLE(2));
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_NDAC, CDC_CONTROL_CLOCK_ENABLE(1));
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_MADC, CDC_CONTROL_CLOCK_DISABLE);
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_NADC, CDC_CONTROL_CLOCK_DISABLE);
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_CLOCK_MUX, CDC_CONTROL_CLOCK_PLL_IN_MCLK | CDC_CONTROL_CLOCK_CODEC_IN_MCLK);

		/* cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_MADC, CDC_CONTROL_CLOCK_DISABLE);
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_NADC, CDC_CONTROL_CLOCK_DISABLE);

		if (soundFreq)
		{
			// Configure a PLL multiplier/divider of 15/2, and a NDAC/NADC divider of 5.
			cdcWriteReg(CDC_CONTROL, CDC_CONTROL_PLL_J, 15);
			cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_NDAC, CDC_CONTROL_CLOCK_ENABLE(5));
		}
		else
		{
			// Configure a PLL multiplier/divider of 21/2, and a NDAC/NADC divider of 7.
			cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_NDAC, CDC_CONTROL_CLOCK_ENABLE(7));
			cdcWriteReg(CDC_CONTROL, CDC_CONTROL_PLL_J, 21);
		} */

		REG_SNDEXTCNT = (REG_SNDEXTCNT & ~SNDEXTCNT_FREQ_47KHZ) | (soundFreq ? SNDEXTCNT_FREQ_47KHZ : SNDEXTCNT_FREQ_32KHZ) | SNDEXTCNT_ENABLE;
		// REG_SNDEXTCNT |= SNDEXTCNT_ENABLE; // Enable sound output
	}

	if ((ndsHeader->arm9romOffset==0x4000 && dsiFlags==0) || !dsiMode) {
		NDSTouchscreenMode();
		*(vu16*)0x4000500 = 0x807F;
	}

	if (dsiMode) {
		dsiModeConfirmed = true;
	} else {
		NTR_BIOS();
		REG_GPIO_WIFI |= BIT(8);	// Old NDS-Wifi mode

		i2cWriteRegister(0x4A, 0x12, 0x00);		// Press power-button for auto-reset
		i2cWriteRegister(0x4A, 0x70, 0x01);		// Bootflag = Warmboot/SkipHealthSafety
	}

	// Load the NDS file
	nocashMessage("Load the NDS file");
	loadBinary_ARM7(romFile);

	u32* a9exe = (u32*)ndsHeader->arm9executeAddress;
	bool recentLibnds =
		  (a9exe[0] == 0xE3A00301
		&& a9exe[1] == 0xE5800208
		&& a9exe[2] == 0xE3A00013
		&& a9exe[3] == 0xE129F000);

	// File containing cached patch offsets
	aFile patchOffsetCacheFile = getFileFromCluster(patchOffsetCacheFileCluster);
	fileRead((char*)&patchOffsetCache, patchOffsetCacheFile, 0, 4);
	if (patchOffsetCache.ver == patchOffsetCacheFileVersion
	 && patchOffsetCache.type == 2) {	// 0 = Regular, 1 = B4DS, 2 = HB
		fileRead((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	} else {
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.type = 2;
	}

	patchOffsetCacheFilePrevCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));

	dldiMagicLoaderString[0]--;

	// Patch with DLDI if desired
	if (!recentLibnds || (recentLibnds && !dsiModeConfirmed) || ramDiskFound) {
		nocashMessage("wantToPatchDLDI");
		dldiPatchBinary ((u8*)((u32*)NDS_HEADER)[0x0A], ((u32*)NDS_HEADER)[0x0B], (ramDiskCluster != 0));
		patchOffsetCache.dldiChecked = true;
	}

	// Pass command line arguments to loaded program
	passArgs_ARM7();

	patchBinary(ndsHeader);

	if (!isGbaR2 && !ramDiskFound && (!recentLibnds || !dsiModeConfirmed)) {
		// Find the DLDI reserved space in the file
		u32 patchOffset = patchOffsetCache.dldiOffset;
		if (!patchOffsetCache.dldiChecked) {
			patchOffset = quickFind ((u8*)((u32*)NDS_HEADER)[0x0A], dldiMagicLoaderString, ((u32*)NDS_HEADER)[0x0B], sizeof(dldiMagicLoaderString));
			if (patchOffset) {
				patchOffsetCache.dldiOffset = patchOffset;
			}
			patchOffsetCache.dldiChecked = true;
		}

		hookNds(ndsHeader, (u32*)sdEngineLocation);

		if (!patchOffsetCache.bootloaderChecked) {
			u32 bootloaderSignature[4] = {0xEA000002, 0x00000000, 0x00000001, 0x00000000};

			// Find and inject bootloader
			u32* addr = (u32*)ndsHeader->arm9destination;
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				if (addr[i]   == bootloaderSignature[0]
				 && addr[i+1] == bootloaderSignature[1]
				 && addr[i+2] == bootloaderSignature[2]
				 && addr[i+3] == bootloaderSignature[3])
				{
					patchOffsetCache.bootloaderOffset = addr + i;
					break;
				}
			}
			patchOffsetCache.bootloaderChecked = true;
		}
		if (patchOffsetCache.bootloaderOffset) {
			//toncset(patchOffsetCache.bootloaderOffset, 0, 0x9C98);
			tonccpy(patchOffsetCache.bootloaderOffset, (char*)0x06000000, 0x8000);
			//tonccpy((char*)BOOT_INJECT_LOCATION, (char*)0x06000000, 0x8000);
		}
	} else if (!isGbaR2 && (!recentLibnds || !dsiModeConfirmed)) {
		hookNds(ndsHeader, NULL); // Only patch SWI functions
	}
	toncset((char*)0x06000000, 0, 0x8000);

	patchOffsetCacheFileNewCrc = swiCRC16(0xFFFF, &patchOffsetCache, sizeof(patchOffsetCacheContents));
	if (patchOffsetCacheFileNewCrc != patchOffsetCacheFilePrevCrc) {
		fileWrite((char*)&patchOffsetCache, patchOffsetCacheFile, 0, sizeof(patchOffsetCacheContents));
	}

	if (dsiModeConfirmed) {
		tonccpy ((char*)NDS_HEADER_16MB, (char*)NDS_HEADER, 0x1000);	// Copy user data and header to last MB of main memory
		tonccpy ((char*)0x02FFE000, (char*)TWL_HEAD, 0x1000);
		if (recentLibnds) {
			REG_MBK6=0x00403000;
		} else {
			tonccpy ((char*)NDS_HEADER_8MB, (char*)NDS_HEADER, 0x1000);
		}
	}

	arm9_boostVram = boostVram;

	while (arm9_stateFlag != ARM9_READY);
	arm9_stateFlag = ARM9_SETSCFG;
	while (arm9_stateFlag != ARM9_READY);
	if (!dsiModeConfirmed && ramDiskSize == 0 && recentLibnds) {
		arm9_stateFlag = ARM9_LOCKSCFG;
		while (arm9_stateFlag != ARM9_READY);
	}

	/*sdmmc_init(true);
	*(vu16*)(SDMMC_BASE + REG_DATACTL32) &= 0xFFFDu;
	*(vu16*)(SDMMC_BASE + REG_DATACTL) &= 0xFFDDu;
	*(vu16*)(SDMMC_BASE + REG_SDBLKLEN32) = 0;*/

	REG_SCFG_EXT &= ~(1UL << 31); // Lock SCFG

	startBinary_ARM7();

	return 0;
}
