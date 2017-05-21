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

#include <nds/ndstypes.h>
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

#include "fat.h"
#include "dldi_patcher.h"
#include "card.h"
#include "card_patcher.h"
#include "cardengine_arm7_bin.h"
#include "cardengine_arm9_bin.h"
#include "boot.h"
#include "hook.h"
#include "common.h"

void arm7clearRAM();
int sdmmc_sdcard_readsectors(u32 sector_no, u32 numsectors, void *out);
int sdmmc_sdcard_init();
void sdmmc_controller_init();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
#define TEMP_MEM 0x02FFE000
#define NDS_HEAD 0x02FFFE00
#define TEMP_ARM9_START_ADDRESS (*(vu32*)0x02FFFFF4)

#define CHEAT_ENGINE_LOCATION	0x027FE000
#define CHEAT_DATA_LOCATION  	0x06010000
#define ENGINE_LOCATION_ARM7  	0x03780000
#define ENGINE_LOCATION_ARM9  	0x03700000

const char* bootName = "BOOT.NDS";

extern unsigned long _start;
extern unsigned long storedFileCluster;
extern unsigned long initDisc;
extern unsigned long wantToPatchDLDI;
extern unsigned long argStart;
extern unsigned long argSize;
extern unsigned long dsiSD;
extern unsigned long saveFileCluster;
extern unsigned long donorFileCluster;
extern unsigned long patchMpuRegion;
extern unsigned long patchMpuSize;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ        0x03

void boot_readFirmware (uint32 address, uint8 * buffer, uint32 size) {
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

//#define resetCpu() __asm volatile("\tswi 0x000000\n");

/*-------------------------------------------------------------------------
passArgs_ARM7
Copies the command line arguments to the end of the ARM9 binary, 
then sets a flag in memory for the loaded NDS to use
--------------------------------------------------------------------------*/
void passArgs_ARM7 (void) {
	u32 ARM9_DST = *((u32*)(NDS_HEAD + 0x028));
	u32 ARM9_LEN = *((u32*)(NDS_HEAD + 0x02C));
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
void resetMemory_ARM7 (void)
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
	
	arm7clearRAM();

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
		boot_readFirmware(settingsOffset + 0x000, (u8*)0x02FFFC80, 0x70);
	} else {
		boot_readFirmware(settingsOffset + 0x100, (u8*)0x02FFFC80, 0x70);
	}
}


void loadBinary_ARM7 (aFile file)
{
	u32 ndsHeader[0x170>>2];
	
	nocashMessage("loadBinary_ARM7");

	// read NDS header
	fileRead ((char*)ndsHeader, file, 0, 0x170);
	// read ARM9 info from NDS header
	u32 ARM9_SRC = ndsHeader[0x020>>2];
	char* ARM9_DST = (char*)ndsHeader[0x028>>2];
	u32 ARM9_LEN = ndsHeader[0x02C>>2];
	// read ARM7 info from NDS header
	u32 ARM7_SRC = ndsHeader[0x030>>2];
	char* ARM7_DST = (char*)ndsHeader[0x038>>2];
	u32 ARM7_LEN = ndsHeader[0x03C>>2];
	
	// Load binaries into memory
	fileRead(ARM9_DST, file, ARM9_SRC, ARM9_LEN);
	fileRead(ARM7_DST, file, ARM7_SRC, ARM7_LEN);

	// first copy the header to its proper location, excluding
	// the ARM9 start address, so as not to start it
	TEMP_ARM9_START_ADDRESS = ndsHeader[0x024>>2];		// Store for later
	ndsHeader[0x024>>2] = 0;
	dmaCopyWords(3, (void*)ndsHeader, (void*)NDS_HEAD, 0x170);
}

/*-------------------------------------------------------------------------
startBinary_ARM7
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
void startBinary_ARM7 (void) {	
	REG_IME=0;
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	// copy NDS ARM9 start address into the header, starting ARM9
	*((vu32*)0x02FFFE24) = TEMP_ARM9_START_ADDRESS;
	ARM9_START_FLAG = 1;
	// Start ARM7
	VoidFn arm7code = *(VoidFn*)(0x2FFFE34);
	arm7code();
}

int sdmmc_sd_readsectors(u32 sector_no, u32 numsectors, void *out);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main function
bool sdmmc_inserted() {
	return true;
}

bool sdmmc_startup() {
	sdmmc_controller_init();
	return sdmmc_sdcard_init() == 0;
}

bool sdmmc_readsectors(u32 sector_no, u32 numsectors, void *out) {
	return sdmmc_sdcard_readsectors(sector_no, numsectors, out) == 0;
}

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

void initMBK() {
	// give all DSI WRAM to arm7 at boot
	
	// arm7 is master of WRAM-A, arm9 of WRAM-B & C
	REG_MBK9=0x3000000F;
	
	// WRAM-A fully mapped to arm7
	*((vu32*)REG_MBK1)=0x8185898D;
	
	// WRAM-B fully mapped to arm7
	*((vu32*)REG_MBK2)=0x8D898581;
	*((vu32*)REG_MBK3)=0x9D999591;
	
	// WRAM-C fully mapped to arm7
	*((vu32*)REG_MBK4)=0x8D898581;
	*((vu32*)REG_MBK5)=0x9D999591;
	
	// WRAM mapped to the 0x3700000 - 0x37AFFFF area 
	// WRAM-A mapped to the 0x3780000 - 0x37BFFFF area : 256k
	REG_MBK6=0x07C03780;
	// WRAM-B mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK7=0x07403700;
	// WRAM-C mapped to the 0x3740000 - 0x377FFFF area : 256k
	REG_MBK8=0x07803740;
}

static const unsigned char dldiMagicString[] = "\xED\xA5\x8D\xBF Chishm";	// Normal DLDI file

int main (void) {
	nocashMessage("bootloader");
	initMBK();
	
	if (dsiSD) {
		_io_dldi.fn_readSectors = sdmmc_readsectors;
		_io_dldi.fn_isInserted = sdmmc_inserted;
		_io_dldi.fn_startup = sdmmc_startup;
	}
	
	// Init card
	if(!FAT_InitFiles(initDisc))
	{
		nocashMessage("!FAT_InitFiles");
		return -1;
	}
	
	aFile file = getFileFromCluster (storedFileCluster);
	aFile donorFile = getFileFromCluster (donorFileCluster);
	
	if ((file.firstCluster < CLUSTER_FIRST) || (file.firstCluster >= CLUSTER_EOF)) 	/* Invalid file cluster specified */
	{
		file = getBootFileCluster(bootName);
	}
	if (file.firstCluster == CLUSTER_FREE)
	{
		nocashMessage("fileCluster == CLUSTER_FREE");
		return -1;
	}
	
	// ARM9 clears its memory part 2
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)resetMemory2_ARM9, resetMemory2_ARM9_size);
	(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function
	// Wait until the ARM9 has completed its task
	nocashMessage("Wait until the ARM9 has completed its task");
	while ((*(vu32*)0x02FFFE24) == (u32)TEMP_MEM);

	// Get ARM7 to clear RAM
	nocashMessage("Get ARM7 to clear RAM");
	resetMemory_ARM7();	
	
	// ARM9 enters a wait loop
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)startBinary_ARM9, startBinary_ARM9_size);
	(*(vu32*)0x02FFFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function

	// Load the NDS file
	nocashMessage("Load the NDS file");
	loadBinary_ARM7(file);
	
	//wantToPatchDLDI = wantToPatchDLDI && ((u32*)NDS_HEAD)[0x084] > 0x200;
	
	nocashMessage("try to patch dldi");
	wantToPatchDLDI = dldiPatchBinary ((u8*)((u32*)NDS_HEAD)[0x0A], ((u32*)NDS_HEAD)[0x0B]);
	if (wantToPatchDLDI) {		
		nocashMessage("dldi patch successful");
		// Find the DLDI reserved space in the file
		u32 patchOffset = quickFind ((u8*)((u32*)NDS_HEAD)[0x0A], dldiMagicString, ((u32*)NDS_HEAD)[0x0B], sizeof(dldiMagicString));
		u32* wordCommandAddr = (u32 *) (((u32)((u32*)NDS_HEAD)[0x0A])+patchOffset+0x80);
		
		int error = hookNdsHomebrew(NDS_HEAD, (const u32*)CHEAT_DATA_LOCATION, (u32*)CHEAT_ENGINE_LOCATION, (u32*)ENGINE_LOCATION_ARM7, wordCommandAddr);
		if(error == ERR_NONE) {
			nocashMessage("dldi hook Sucessfull");
		} else {
			nocashMessage("error during dldi hook");
		}
	} else {	
		nocashMessage("dldi Patch Unsuccessful try to patch card");
		copyLoop (ENGINE_LOCATION_ARM7, (u32*)cardengine_arm7_bin, cardengine_arm7_bin_size);	
		copyLoop (ENGINE_LOCATION_ARM9, (u32*)cardengine_arm9_bin, cardengine_arm9_bin_size);			

		module_params_t* params = findModuleParams(NDS_HEAD);
		if(params)
		{
			ensureArm9Decompressed(NDS_HEAD, params);
		}

		patchCardNds(NDS_HEAD,ENGINE_LOCATION_ARM7,ENGINE_LOCATION_ARM9,params,saveFileCluster, patchMpuRegion, patchMpuSize, donorFile);
		
		int error = hookNdsRetail(NDS_HEAD, file, (const u32*)CHEAT_DATA_LOCATION, (u32*)CHEAT_ENGINE_LOCATION, (u32*)ENGINE_LOCATION_ARM7);
			if(error == ERR_NONE) {
			nocashMessage("card hook Sucessfull");
		} else {
			nocashMessage("error during card hook");
		}
	}
 
	

	// Pass command line arguments to loaded program
	//passArgs_ARM7();
	
	nocashMessage("Start the NDS file");
	startBinary_ARM7();

	return 0;
}

