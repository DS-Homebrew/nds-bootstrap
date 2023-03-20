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

#include "tonccpy.h"
//#include "my_sdmmc.h"
#include "boot.h"
#include "my_fat.h"
#include "dldi_patcher.h"
#include "hook.h"
#include "common.h"
#include "locations.h"
#include "i2c.h"

#include "sr_data_srloader.h"   // For rebooting the game

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
extern u32 consoleModel;
extern u32 srParamsFileCluster;
extern u32 srTid1;
extern u32 srTid2;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static const char *unlaunchAutoLoadID = "AutoLoadInfo";
static const char *resetgameSrldrPath = "sdmc:/_nds/TWiLightMenu/main.srldr";

static void unlaunchSetFilename(void) {
	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) = (BIT(0) | BIT(1));		// Load the title at 2000838h
													// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < strlen(resetgameSrldrPath); i++) {
		*(u8*)(0x02000838+i2) = resetgameSrldrPath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
}

static void readSrBackendId(void) {
	// Use SR backend ID
	*(u32*)(0x02000300) = 0x434E4C54;	// 'CNLT'
	*(u16*)(0x02000304) = 0x1801;
	*(u32*)(0x02000308) = 0;
	*(u32*)(0x0200030C) = 0;
	*(u32*)(0x02000310) = srTid1;
	*(u32*)(0x02000314) = srTid2;
	*(u32*)(0x02000318) = /* srTid2 == 0x00030000 ? 0x13 : */ 0x17;
	*(u32*)(0x0200031C) = 0;
	*(u16*)(0x02000306) = swiCRC16(0xFFFF, (void*)0x02000308, 0x18);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ        0x03

/*static void boot_readFirmware (uint32 address, uint8 * buffer, uint32 size) {
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
}*/

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

/*-------------------------------------------------------------------------
passArgs_ARM7
Copies the command line arguments to the end of the ARM9 binary,
then sets a flag in memory for the loaded NDS to use
--------------------------------------------------------------------------*/
/*static void passArgs_ARM7 (void) {
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
}*/




/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
/*static void resetMemory_ARM7 (void)
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


void loadBinary_ARM7 (aFile file)
{
	u32 ndsHeader[0x170>>2];

	// read NDS header
	fileRead ((char*)ndsHeader, file, 0, 0x170, 0);
	// read ARM9 info from NDS header
	u32 ARM9_SRC = ndsHeader[0x020>>2];
	char* ARM9_DST = (char*)ndsHeader[0x028>>2];
	u32 ARM9_LEN = ndsHeader[0x02C>>2];
	// read ARM7 info from NDS header
	u32 ARM7_SRC = ndsHeader[0x030>>2];
	char* ARM7_DST = (char*)ndsHeader[0x038>>2];
	u32 ARM7_LEN = ndsHeader[0x03C>>2];

	// Load binaries into memory
	fileRead(ARM9_DST, file, ARM9_SRC, ARM9_LEN, 0);
	fileRead(ARM7_DST, file, ARM7_SRC, ARM7_LEN, 0);

	// first copy the header to its proper location, excluding
	// the ARM9 start address, so as not to start it
	TEMP_ARM9_START_ADDRESS = ndsHeader[0x024>>2];		// Store for later
	ndsHeader[0x024>>2] = 0;
	dmaCopyWords(3, (void*)ndsHeader, (void*)NDS_HEADER, 0x170);
}*/

/*-------------------------------------------------------------------------
startBinary_ARM7
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
/*void startBinary_ARM7 (void) {
	REG_IME=0;
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	// copy NDS ARM9 start address into the header, starting ARM9
	*((vu32*)0x02FFFE24) = TEMP_ARM9_START_ADDRESS;
	ARM9_START_FLAG = 1;
	// Start ARM7
	VoidFn arm7code = *(VoidFn*)(0x2FFFE34);
	arm7code();
}*/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main function

/*static u32 quickFind (const unsigned char* data, const unsigned char* search, u32 dataLen, u32 searchLen) {
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

extern unsigned char dldiMagicString[12];

void mpu_reset();
void mpu_reset_end();*/

int main (void) {
	//nocashMessage("bootloader");

	extern void *_io_dldi;
	//const char* bootName = "BOOT.NDS";

	if(memcmp(_io_dldi, "RAMD", 4) == 0)
	{
		return -1;
	}

	// ARM9 clears its memory part 2
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	tonccpy((void*)TEMP_MEM, (void*)resetMemory2_ARM9, resetMemory2_ARM9_size);
	(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function

	// Init card
	if(!FAT_InitFiles(true))
	{
		return -1;
	}
	aFile file = getFileFromCluster(storedFileCluster);
	if ((file.firstCluster < CLUSTER_FIRST) || (file.firstCluster >= CLUSTER_EOF)) 	/* Invalid file cluster specified */
	{
		//file = getBootFileCluster(bootName);
		return -1;
	}
	if (file.firstCluster == CLUSTER_FREE)
	{
		return -1;
	}

	u8 tidCrc[6] = {0};
	fileRead((char*)tidCrc, file, 0xC, 4);
	fileRead((char*)tidCrc+4, file, 0x15E, 2);

	aFile srParamsFile = getFileFromCluster(srParamsFileCluster);
	fileWrite((char*)&storedFileCluster, srParamsFile, 0, 4);	// Write file cluster to soft-reset params file for nds-bootstrap to read after rebooting the console
	fileWrite((char*)tidCrc, srParamsFile, 4, 6);

	if (srTid1 != 0) {
		readSrBackendId();
	} else if (consoleModel >= 2) {
		tonccpy((u32*)0x02000300, sr_data_srloader, 0x20);
	} else {
		unlaunchSetFilename();
	}
	toncset((u32*)0x02000000, 0, 0x400);
	*(u32*)(0x02000000) = BIT(3);

	i2cWriteRegister(0x4A, 0x70, 0x01);
	i2cWriteRegister(0x4A, 0x11, 0x01);			// Reboot game

	while (1);
/*
	// ARM9 clears its memory part 2
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)resetMemory2_ARM9, resetMemory2_ARM9_size);
	(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function
	// Wait until the ARM9 has completed its task
	while ((*(vu32*)0x02FFFE24) == (u32)TEMP_MEM);

	// ARM9 sets up mpu
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)mpu_reset, mpu_reset_end - mpu_reset);
	(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function
	// Wait until the ARM9 has completed its task
	while ((*(vu32*)0x02FFFE24) == (u32)TEMP_MEM);

	// Get ARM7 to clear RAM
	resetMemory_ARM7();

	// ARM9 enters a wait loop
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)startBinary_ARM9, startBinary_ARM9_size);
	(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function

	// Load the NDS file
	loadBinary_ARM7(file);

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

	tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
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
*/
	//if (!dsiMode && ramDiskSize == 0) {
	/*	u32* a9exe = (u32*)ndsHeader->arm9executeAddress;
		bool recentLibnds =
			  (a9exe[0] == 0xE3A00301
			&& a9exe[1] == 0xE5800208
			&& a9exe[2] == 0xE3A00013
			&& a9exe[3] == 0xE129F000);
		if (recentLibnds) {
			copyLoop((void*)TEMP_MEM, (void*)lockSCFG_ARM9, lockSCFG_ARM9_size);
			(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function
			while ((*(vu32*)0x02FFFE24) == (u32)TEMP_MEM);
		}*/
	//}

	/*sdmmc_init(true);
	*(vu16*)(SDMMC_BASE + REG_DATACTL32) &= 0xFFFDu;
	*(vu16*)(SDMMC_BASE + REG_DATACTL) &= 0xFFDDu;
	*(vu16*)(SDMMC_BASE + REG_SDBLKLEN32) = 0;*/

	/*startBinary_ARM7();

	return 0;*/
}
