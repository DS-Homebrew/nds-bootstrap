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

#include "tonccpy.h"
#include "my_fat.h"
#include "dldi_patcher.h"
#include "hook.h"
#include "common.h"
#include "locations.h"

void arm7clearRAM();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
extern unsigned long _start;
extern unsigned long storedFileCluster;
extern unsigned long initDisc;
extern unsigned long wantToPatchDLDI;
extern unsigned long argStart;
extern unsigned long argSize;
extern unsigned long dsiSD;

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

	argSrc = (u32*)(argStart + (int)&_start);

	argDst = (u32*)((ARM9_DST + ARM9_LEN + 3) & ~3);		// Word aligned

	copyLoop(argDst, argSrc, argSize);

	__system_argv->argvMagic = ARGV_MAGIC;
	__system_argv->commandLine = (char*)argDst;
	__system_argv->length = argSize;
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
	int i;
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

	//clear out ARM7 DMA channels and timers
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	u8 languageBak = *(u8*)((u32)NDS_HEADER - 0x11C);

	arm7clearRAM();								// clear exclusive IWRAM

	REG_IE = 0;
	REG_IF = ~0;
	(*(vu32*)(0x04000000-4)) = 0;  //IRQ_HANDLER ARM7 version
	(*(vu32*)(0x04000000-8)) = ~0; //VBLANK_INTR_WAIT_FLAGS, ARM7 version
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

	// Change language
	*(u8*)((u32)NDS_HEADER - 0x11C) = languageBak;
}


static void loadBinary_ARM7 (aFile file)
{
	//nocashMessage("loadBinary_ARM7");

	// read NDS header
	fileRead((char*)NDS_HEADER, file, 0, 0x170, 0);

	// Load binaries into memory
	fileRead(ndsHeader->arm9destination, file, ndsHeader->arm9romOffset, ndsHeader->arm9binarySize, 0);
	fileRead(ndsHeader->arm7destination, file, ndsHeader->arm7romOffset, ndsHeader->arm7binarySize, 0);
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

static const unsigned char dldiMagicString[] = "\xED\xA5\x8D\xBF Chishm";	// Normal DLDI file

int arm7_main (void) {
	//nocashMessage("bootloader");

	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	//nocashMessage("Getting ARM7 to clear RAM...\n");

	resetMemory_ARM7();

	// Init card
	if(!FAT_InitFiles(initDisc, 0))
	{
		//nocashMessage("!FAT_InitFiles");
		return -1;
	}

	aFile romFile = getFileFromCluster(storedFileCluster);

	const char* bootName = "BOOT.NDS";

	if ((romFile.firstCluster < CLUSTER_FIRST) || (romFile.firstCluster >= CLUSTER_EOF)) {
		romFile = getBootFileCluster(bootName, 0);
	}

	if (romFile.firstCluster == CLUSTER_FREE) {
		//nocashMessage("fileCluster == CLUSTER_FREE");
		return -1;
	}

	// Load the NDS file
	//nocashMessage("Load the NDS file");
	loadBinary_ARM7(romFile);

	// Patch with DLDI if desired
	//if (wantToPatchDLDI) {
		//nocashMessage("wantToPatchDLDI");
		dldiPatchBinary ((u8*)((u32*)NDS_HEADER)[0x0A], ((u32*)NDS_HEADER)[0x0B]);
	//}

	// Pass command line arguments to loaded program
	passArgs_ARM7();

	// Find the DLDI reserved space in the file
	u32 patchOffset = quickFind ((u8*)((u32*)NDS_HEADER)[0x0A], dldiMagicString, ((u32*)NDS_HEADER)[0x0B], sizeof(dldiMagicString));
	u32* wordCommandAddr = (u32 *) (((u32)((u32*)NDS_HEADER)[0x0A])+patchOffset+0x80);

	hookNds(ndsHeader, (u32*)SDENGINE_LOCATION, wordCommandAddr);

	u32 bootloaderSignature[4] = {0xEA000002, 0x00000000, 0x00000001, 0x00000000};

	// Find and inject bootloader
	u32* addr = (u32*)ndsHeader->arm9destination;
	for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
		if (addr[i]   == bootloaderSignature[0]	
		 && addr[i+1] == bootloaderSignature[1]
		 && addr[i+2] == bootloaderSignature[2]
		 && addr[i+3] == bootloaderSignature[3])
		{
			toncset(addr + i, 0, 0x9C98);
			tonccpy(addr + i, (char*)BOOT_INJECT_LOCATION, 0x8000);
			break;
		}
	}

	startBinary_ARM7();

	return 0;
}
