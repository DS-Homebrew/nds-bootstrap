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
#include <nds/ndstypes.h>
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>

#include "fat.h"
#include "i2c.h"
//#include "dldi_patcher.h"
#include "card.h"
#include "card_patcher.h"
#include "cardengine_arm7_bin.h"
#include "cardengine_arm9_bin.h"
#include "hook.h"
#include "common.h"

#include "databwlist.h"

void arm7clearRAM();
int sdmmc_sdcard_readsectors(u32 sector_no, u32 numsectors, void *out);
int sdmmc_sdcard_init();
void sdmmc_controller_init();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
#define TEMP_MEM 0x02FFE000
#define NDS_HEAD 0x027FFE00
#define TEMP_ARM9_START_ADDRESS (*(vu32*)0x027FFFF4)

#define CHEAT_ENGINE_LOCATION	0x027FE000
#define CHEAT_DATA_LOCATION  	0x06010000
#define ENGINE_LOCATION_ARM7  	0x037C0000
#define ENGINE_LOCATION_ARM9  	0x02400000

const char* bootName = "BOOT.NDS";

extern unsigned long _start;
extern unsigned long storedFileCluster;
extern unsigned long initDisc;
extern unsigned long wantToPatchDLDI;
extern unsigned long argStart;
extern unsigned long argSize;
extern unsigned long dsiSD;
extern unsigned long saveFileCluster;
extern unsigned long saveSize;
extern unsigned long donorSdkVer;
extern unsigned long patchMpuRegion;
extern unsigned long patchMpuSize;
extern unsigned long consoleModel;
extern unsigned long ntrTouch;
extern unsigned long loadingScreen;
extern unsigned long romread_LED;
extern unsigned long gameSoftReset;

u32 setDataBWlist[7] = {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
u32 setDataBWlist_1[3] = {0x00000000, 0x00000000, 0x00000000};
u32 setDataBWlist_2[3] = {0x00000000, 0x00000000, 0x00000000};
u32 setDataBWlist_3[3] = {0x00000000, 0x00000000, 0x00000000};
u32 setDataBWlist_4[3] = {0x00000000, 0x00000000, 0x00000000};
int dataAmount = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Used for debugging purposes
static void errorOutput (void) {
	if(loadingScreen > 0) {
		// Wait until the ARM9 is ready
		while (arm9_stateFlag != ARM9_READY);
		// Set the error code, then tell ARM9 to display it
		arm9_errorColor = true;
	}
	// Stop
	while(1);
}

static void debugOutput (void) {
	if(loadingScreen > 0) {
		// Wait until the ARM9 is ready
		while (arm9_stateFlag != ARM9_READY);
		// Set the error code, then tell ARM9 to display it
		arm9_screenMode = loadingScreen-1;
		arm9_stateFlag = ARM9_DISPERR;
		// Wait for completion
		while (arm9_stateFlag != ARM9_READY);
	}
}

static void increaseLoadBarLength (void) {
	arm9_loadBarLength++;
	if(loadingScreen == 1) debugOutput();	// Let the loading bar finish before ROM starts
}

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
		boot_readFirmware(settingsOffset + 0x000, (u8*)0x027FFC80, 0x70);
	} else {
		boot_readFirmware(settingsOffset + 0x100, (u8*)0x027FFC80, 0x70);
	}
}

// The following 3 functions are not in devkitARM r47
//---------------------------------------------------------------------------------
u32 readTSCReg(u32 reg) {
//---------------------------------------------------------------------------------
 
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = ((reg<<1) | 1) & 0xFF;
 
	while(REG_SPICNT & 0x80);
 
	REG_SPIDATA = 0;
 
	while(REG_SPICNT & 0x80);

	REG_SPICNT = 0;

	return REG_SPIDATA;
}

//---------------------------------------------------------------------------------
void readTSCRegArray(u32 reg, void *buffer, int size) {
//---------------------------------------------------------------------------------
 
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = ((reg<<1) | 1) & 0xFF;

	char *buf = (char*)buffer;
	while(REG_SPICNT & 0x80);
	int count = 0;
	while(count<size) {
		REG_SPIDATA = 0;
 
		while(REG_SPICNT & 0x80);


		buf[count++] = REG_SPIDATA;
		
	}
	REG_SPICNT = 0;

}


//---------------------------------------------------------------------------------
u32 writeTSCReg(u32 reg, u32 value) {
//---------------------------------------------------------------------------------
 
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = ((reg<<1)) & 0xFF;
 
	while(REG_SPICNT & 0x80);
 
	REG_SPIDATA = value;
 
	while(REG_SPICNT & 0x80);

	REG_SPICNT = 0;

	return REG_SPIDATA;
}


//---------------------------------------------------------------------------------
void NDSTouchscreenMode() {
//---------------------------------------------------------------------------------
	//unsigned char * *(unsigned char*)0x40001C0=		(unsigned char*)0x40001C0;
	//unsigned char * *(unsigned char*)0x40001C0byte2=(unsigned char*)0x40001C1;
	//unsigned char * *(unsigned char*)0x40001C2=	(unsigned char*)0x40001C2;
	//unsigned char * I2C_DATA=	(unsigned char*)0x4004500;
	//unsigned char * I2C_CNT=	(unsigned char*)0x4004501;


	u8 volLevel;
	
	//if(fifoCheckValue32(FIFO_MAXMOD)) {
	//	// special setting (when found special gamecode)
	//	volLevel = 0xAC;
	//} else {
		// normal setting (for any other gamecodes)
		volLevel = 0xA7;
	//}

	volLevel += 0x13;

	// Touchscr
	readTSCReg(0);
	writeTSCReg(0,0);
	writeTSCReg(0x3a,0);
	readTSCReg(0x51);
	writeTSCReg(3,0);
	readTSCReg(2);
	writeTSCReg(0,0);
	readTSCReg(0x3f);
	writeTSCReg(0,1);
	readTSCReg(0x38);
	readTSCReg(0x2a);
	readTSCReg(0x2E);
	writeTSCReg(0,0);
	writeTSCReg(0x52,0x80);
	writeTSCReg(0x40,0xC);
	writeTSCReg(0,1);
	writeTSCReg(0x24,0xff);
	writeTSCReg(0x25,0xff);
	writeTSCReg(0x26,0x7f);
	writeTSCReg(0x27,0x7f);
	writeTSCReg(0x28,0x4a);
	writeTSCReg(0x29,0x4a);
	writeTSCReg(0x2a,0x10);
	writeTSCReg(0x2b,0x10);
	writeTSCReg(0,0);
	writeTSCReg(0x51,0);
	writeTSCReg(0,3);
	readTSCReg(2);
	writeTSCReg(2,0x98);
	writeTSCReg(0,1);
	writeTSCReg(0x23,0);
	writeTSCReg(0x1f,0x14);
	writeTSCReg(0x20,0x14);
	writeTSCReg(0,0);
	writeTSCReg(0x3f,0);
	readTSCReg(0x0b);
	writeTSCReg(0x5,0);
	writeTSCReg(0xb,0x1);
	writeTSCReg(0xc,0x2);
	writeTSCReg(0x12,0x1);
	writeTSCReg(0x13,0x2);
	writeTSCReg(0,1);
  writeTSCReg(0x2E,0x00);
  writeTSCReg(0,0);
  writeTSCReg(0x3A,0x60);
  writeTSCReg(0x01,01);
  writeTSCReg(0x9,0x66);
  writeTSCReg(0,1);
  readTSCReg(0x20);
  writeTSCReg(0x20,0x10);
  writeTSCReg(0,0);
  writeTSCReg( 04,00);
  writeTSCReg( 0x12,0x81);
  writeTSCReg( 0x13,0x82);
  writeTSCReg( 0x51,0x82);
  writeTSCReg( 0x51,0x00);
  writeTSCReg( 0x04,0x03);
  writeTSCReg( 0x05,0xA1);
  writeTSCReg( 0x06,0x15);
  writeTSCReg( 0x0B,0x87);
  writeTSCReg( 0x0C,0x83);
  writeTSCReg( 0x12,0x87);
  writeTSCReg( 0x13,0x83);
  writeTSCReg(0,3);
  readTSCReg(0x10);
  writeTSCReg(0x10,0x08);
  writeTSCReg(0,4);
  writeTSCReg(0x08,0x7F);
  writeTSCReg(0x09,0xE1);
  writeTSCReg(0xa,0x80);
  writeTSCReg(0xb,0x1F);
  writeTSCReg(0xc,0x7F);
  writeTSCReg(0xd,0xC1);
  writeTSCReg(0,0);
  writeTSCReg( 0x41, 0x08);
  writeTSCReg( 0x42, 0x08);
  writeTSCReg( 0x3A, 0x00);
  writeTSCReg(0,4);
  writeTSCReg(0x08,0x7F);
  writeTSCReg(0x09,0xE1);
  writeTSCReg(0xa,0x80);
  writeTSCReg(0xb,0x1F);
  writeTSCReg(0xc,0x7F);
  writeTSCReg(0xd,0xC1);
  writeTSCReg(0,1);
  writeTSCReg(0x2F, 0x2B);
  writeTSCReg(0x30, 0x40);
  writeTSCReg(0x31, 0x40);
  writeTSCReg(0x32, 0x60);
  writeTSCReg(0,0);
  readTSCReg( 0x74);
  writeTSCReg( 0x74, 0x02);
  readTSCReg( 0x74);
  writeTSCReg( 0x74, 0x10);
  readTSCReg( 0x74);
  writeTSCReg( 0x74, 0x40);
  writeTSCReg(0,1);
  writeTSCReg( 0x21, 0x20);
  writeTSCReg( 0x22, 0xF0);
  writeTSCReg(0,0);
  readTSCReg( 0x51);
  readTSCReg( 0x3f);
  writeTSCReg( 0x3f, 0xd4);
  writeTSCReg(0,1);
  writeTSCReg(0x23,0x44);
  writeTSCReg(0x1F,0xD4);
  writeTSCReg(0x28,0x4e);
  writeTSCReg(0x29,0x4e);
  writeTSCReg(0x24,0x9e);
  writeTSCReg(0x24,0x9e);
  writeTSCReg(0x20,0xD4);
  writeTSCReg(0x2a,0x14);
  writeTSCReg(0x2b,0x14);
  writeTSCReg(0x26,volLevel);
  writeTSCReg(0x27,volLevel);
  writeTSCReg(0,0);
  writeTSCReg(0x40,0);
  writeTSCReg(0x3a,0x60);
  writeTSCReg(0,1);
  writeTSCReg(0x26,volLevel);
  writeTSCReg(0x27,volLevel);
  writeTSCReg(0x2e,0x03);
  writeTSCReg(0,3);
  writeTSCReg(3,0);
  writeTSCReg(0,1);
  writeTSCReg(0x21,0x20);
  writeTSCReg(0x22,0xF0);
  readTSCReg(0x22);
  writeTSCReg(0x22,0xF0);
  writeTSCReg(0,0);
  writeTSCReg(0x52,0x80);
  writeTSCReg(0x51,0x00);
  writeTSCReg(0,3);
  readTSCReg(0x02);
  writeTSCReg(2,0x98);
  writeTSCReg(0,0xff);
  writeTSCReg(5,0);
	
	
	
	
	
	
	
	// Powerman
	writePowerManagement(0x00,0x0D);
	//*(unsigned char*)0x40001C2 = 0x80, 0x00;		// read PWR[0]   ;<-- also part of TSC !
	//*(unsigned char*)0x40001C2 = 0x00, 0x0D;		// PWR[0]=0Dh    ;<-- also part of TSC !
	

}


u32 ROM_LOCATION = 0x0C800000;
u32 ROM_TID;
u32 ROM_HEADERCRC;
u32 ARM9_LEN;
u32 fatSize;
u32 romSize;

void loadBinary_ARM7 (aFile file)
{
	u32 ndsHeader[0x170>>2];

	nocashMessage("loadBinary_ARM7");

	// read NDS header
	fileRead ((char*)ndsHeader, file, 0, 0x170);
	// read ARM9 info from NDS header
	u32 ARM9_SRC = ndsHeader[0x020>>2];
	char* ARM9_DST = (char*)ndsHeader[0x028>>2];
	ARM9_LEN = ndsHeader[0x02C>>2];
	// read ARM7 info from NDS header
	u32 ARM7_SRC = ndsHeader[0x030>>2];
	char* ARM7_DST = (char*)ndsHeader[0x038>>2];
	u32 ARM7_LEN = ndsHeader[0x03C>>2];

	ROM_TID = ndsHeader[0x00C>>2];
	fatSize = ndsHeader[0x04C>>2];
	romSize = ndsHeader[0x080>>2];
	ROM_HEADERCRC = ndsHeader[0x15C>>2];

	//Fix Pokemon games needing header data.
	fileRead ((char*)0x027FF000, file, 0, 0x170);

	if((*(u32*)(0x27FF00C) & 0x00FFFFFF) == 0x414441	// Diamond
	|| (*(u32*)(0x27FF00C) & 0x00FFFFFF) == 0x415041	// Pearl
	|| (*(u32*)(0x27FF00C) & 0x00FFFFFF) == 0x555043	// Platinum
	|| (*(u32*)(0x27FF00C) & 0x00FFFFFF) == 0x4B5049	// HG
	|| (*(u32*)(0x27FF00C) & 0x00FFFFFF) == 0x475049)	// SS
	{
		*(u32*)(0x27FF00C) = 0x4A414441;//Make the Pokemon game code ADAJ.
	}
	
	// Load binaries into memory
	fileRead(ARM9_DST, file, ARM9_SRC, ARM9_LEN);
	fileRead(ARM7_DST, file, ARM7_SRC, ARM7_LEN);
	
	// The World Ends With You (USA) (Europe)
	if(ROM_TID == 0x454C5741 || ROM_TID == 0x504C5741){
		*(u32*)(0x203E7B0) = 0;
	}

	// Subarashiki Kono Sekai - It's a Wonderful World (Japan)
	if(ROM_TID == 0x4A4C5741){
		*(u32*)(0x203F114) = 0;
	}

	// Miami Nights - Singles in the City (USA)
	if(ROM_TID == 0x45575641){
		//fixes not enough memory error
		*(u32*)(0x0204cccc) = 0xe1a00000; //nop
	}

	// Miami Nights - Singles in the City (Europe)
	if(ROM_TID == 0x50575641){
		//fixes not enough memory error
		*(u32*)(0x0204cdbc) = 0xe1a00000; //nop
	}

	// "Chrono Trigger (Japan)"
	if(ROM_TID == 0x4a555159){
		*(u32*)(0x0204e364) = 0xe3a00000; //mov r0, #0
		*(u32*)(0x0204e368) = 0xe12fff1e; //bx lr
		*(u32*)(0x0204e6c4) = 0xe3a00000; //mov r0, #0
		*(u32*)(0x0204e6c8) = 0xe12fff1e; //bx lr
	}

	// "Chrono Trigger (USA/Europe)"
	if(ROM_TID == 0x45555159 || ROM_TID == 0x50555159){
		*(u32*)(0x0204e334) = 0xe3a00000; //mov r0, #0
		*(u32*)(0x0204e338) = 0xe12fff1e; //bx lr
		*(u32*)(0x0204e694) = 0xe3a00000; //mov r0, #0
		*(u32*)(0x0204e698) = 0xe12fff1e; //bx lr
	}

	// "Grand Theft Auto - Chinatown Wars (USA) (En,Fr,De,Es,It)"
	// "Grand Theft Auto - Chinatown Wars (Europe) (En,Fr,De,Es,It)"
	/*if(ROM_TID == 0x45584759 || ROM_TID == 0x50584759){
		*(u16*)(-0x02037a34) = 0x46c0;
		*(u32*)(0x0216ac0c) = 0x0001fffb;
	}*/

	// first copy the header to its proper location, excluding
	// the ARM9 start address, so as not to start it
	TEMP_ARM9_START_ADDRESS = ndsHeader[0x024>>2];		// Store for later
	ndsHeader[0x024>>2] = 0;
	dmaCopyWords(3, (void*)ndsHeader, (void*)NDS_HEAD, 0x170);

	// Switch to NTR mode BIOS (no effect with locked arm7 SCFG)
	nocashMessage("Switch to NTR mode BIOS");
	REG_SCFG_ROM = 0x703;
}

u32 enableExceptionHandler = true;
u32 dsiWramUsed = false;

void loadRomIntoRam(aFile file) {
	u32 ROMinRAM = 0;
	u32 cleanRomSize = romSize;
	u32 ramRomSizeLimit = 0x00800000;
	if (consoleModel > 0) ramRomSizeLimit = 0x01800000;

	// ExceptionHandler2 (red screen) blacklist
	if((ROM_TID & 0x00FFFFFF) == 0x4D5341	// SM64DS
	|| (ROM_TID & 0x00FFFFFF) == 0x534D53	// SMSW
	|| (ROM_TID & 0x00FFFFFF) == 0x443241	// NSMB
	|| (ROM_TID & 0x00FFFFFF) == 0x4D4441)	// AC:WW
	{
		enableExceptionHandler = false;
	}

	if((romSize & 0x0000000F) == 0x1
	|| (romSize & 0x0000000F) == 0x3
	|| (romSize & 0x0000000F) == 0x5
	|| (romSize & 0x0000000F) == 0x7
	|| (romSize & 0x0000000F) == 0x9
	|| (romSize & 0x0000000F) == 0xB
	|| (romSize & 0x0000000F) == 0xD
	|| (romSize & 0x0000000F) == 0xF)
	{
		romSize--;	// If ROM size is at an odd number, subtract 1 from it.
	}
	romSize -= 0x4000;
	romSize -= ARM9_LEN;

	// If ROM size is 0x01800000 (0x00800000 for retail DSi) or below, then load the ROM into RAM.
	if((fatSize > 0) && (romSize > 0) && (romSize <= ramRomSizeLimit) && (ROM_TID != 0x45475241)
	&& (ROM_TID != 0x45525243) && (ROM_TID != 0x45425243)
	&& (romSize != (0x012C7066-0x4000-ARM9_LEN))
	&& !dsiWramUsed) {
		ROMinRAM = 1;
		fileRead(ROM_LOCATION, file, 0x4000+ARM9_LEN, romSize);
	} else if (consoleModel > 0) {
		if((ROM_TID == 0x4A575A41) && (ROM_HEADERCRC == 0x539FCF56)) {		// Sawaru - Made in Wario (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AZWJ0[i];
		} else if((ROM_TID == 0x4A575A41) && (ROM_HEADERCRC == 0xE37BCF56)) {	// Sawaru - Made in Wario (J) (v02)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AZWJ2[i];
		} else if((ROM_TID == 0x45575A41) && (ROM_HEADERCRC == 0x7356CF56)) {	// WarioWare: Touched (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AZWE0[i];
		} else if((ROM_TID == 0x50575A41) && (ROM_HEADERCRC == 0x8E8FCF56)) {	// WarioWare: Touched (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AZWP0[i];
		} else if((ROM_TID == 0x43575A41) && (ROM_HEADERCRC == 0xE10BCF56)) {	// Momo Waliou Zhizao (C)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AZWC0[i];
		} else if((ROM_TID == 0x4B575A41) && (ROM_HEADERCRC == 0xB5C6CF56)) {	// Manjyeora! Made in Wario (KS)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AZWK0[i];
		} else if((ROM_TID == 0x45533241) && (ROM_HEADERCRC == 0xA860CF56)) {	// Dragon Ball Z: Supersonic Warriors 2 (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_A2SE0[i];
		} else if((ROM_TID == 0x50424441) && (ROM_HEADERCRC == 0x53C2CF56)) {	// Dragon Ball Z: Supersonic Warriors 2 (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_ADBP0[i];
		} else if((ROM_TID == 0x45484241) && (ROM_HEADERCRC == 0x3AFCCF56)) {	// Resident Evil: Deadly Silence (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_ABHE0[i];
		} else if((ROM_TID == 0x4A464641) && (ROM_HEADERCRC == 0xBE5ACF56)) {	// Final Fantasy III (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AFFJ0[i];
		} else if((ROM_TID == 0x45464641) && (ROM_HEADERCRC == 0x70A6CF56)) {	// Final Fantasy III (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AFFE0[i];
		} else if((ROM_TID == 0x50464641) && (ROM_HEADERCRC == 0x1AE7CF56)) {	// Final Fantasy III (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AFFP0[i];
		/* } else if((ROM_TID == 0x50514D41) && (ROM_HEADERCRC == 0x9703CF56)) {	// Mario Vs Donkey Kong 2: March of the Minis (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_AMQP0[i];
		} else if((ROM_TID == 0x45424341) && (ROM_HEADERCRC == 0xF10BCF56)) {	// Castlevania: Portrait of Ruin (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_ACBE0[i];
		} else if((ROM_TID == 0x45414441) && (ROM_HEADERCRC == 0xCA37CF56)		// Pokemon Diamond (U)
				|| (ROM_TID == 0x45415041) && (ROM_HEADERCRC == 0xA80CCF56)) {	// Pokemon Pearl (U)
			for(int i = 0; i < 7; i++) {
				setDataBWlist[i] = dataWhitelist_ADAE0_0[i];
				if (i < 3) setDataBWlist_1[i] = dataWhitelist_ADAE0_1[i];
				if (i < 3) setDataBWlist_2[i] = dataWhitelist_ADAE0_2[i];
				if (i < 3) setDataBWlist_3[i] = dataWhitelist_ADAE0_3[i];
			}
			dataAmount = 3; */
		} else if((ROM_TID == 0x4A554343) && (ROM_HEADERCRC == 0x61DCCF56)) {	// Tomodachi Collection (J)
			for(int i = 0; i < 7; i++) {
				setDataBWlist[i] = dataWhitelist_CCUJ0_0[i];
				if (i != 3) setDataBWlist_1[i] = dataWhitelist_CCUJ0_1[i];
			}
			dataAmount = 1;
		} else if((ROM_TID == 0x4A554343) && (ROM_HEADERCRC == 0x9B26CF56)) {	// Tomodachi Collection (J) (Rev 1)
			for(int i = 0; i < 7; i++) {
				setDataBWlist[i] = dataWhitelist_CCUJ1_0[i];
				if (i < 3) setDataBWlist_1[i] = dataWhitelist_CCUJ1_1[i];
			}
			dataAmount = 1;
		/* } else if((ROM_TID == 0x45344659) && (ROM_HEADERCRC == 0x2635CF56)) {	// Final Fantasy IV (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_YF4E0[i];
		} else if((ROM_TID == 0x50344659) && (ROM_HEADERCRC == 0xDB3BCF56)) {	// Final Fantasy IV (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_YF4P0[i];
		} else if((ROM_TID == 0x45555159) && (ROM_HEADERCRC == 0xC2DFCF56)) {	// Chrono Trigger (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataWhitelist_YQUE0[i]; */
		} else if((ROM_TID == 0x45525241) && (ROM_HEADERCRC == 0xBE09CF56)) {	// Ridge Racer DS (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_ARRE0[i];
		} else if((ROM_TID == 0x45324441) && (ROM_HEADERCRC == 0x8AE1CF56)		// Nintendogs: Chihuahua & Friends (U)
				|| (ROM_TID == 0x45334441) && (ROM_HEADERCRC == 0x9D25CF56)		// Nintendogs: Lab & Friends (U)
				|| (ROM_TID == 0x45354441) && (ROM_HEADERCRC == 0x0451CF56)		// Nintendogs: Best Friends (U)
				|| (ROM_TID == 0x45474441) && (ROM_HEADERCRC == 0x164BCF56)) {	// Nintendogs: Dachshund & Friends (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AD2E0[i];
		} else if((ROM_TID == 0x45594741) && (ROM_HEADERCRC == 0x9AD6CF56)) {	// Phoenix Wright: Ace Attorney (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AGYE0[i];
		} else if((ROM_TID == 0x50594741) && (ROM_HEADERCRC == 0x0744CF56)) {	// Phoenix Wright: Ace Attorney (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AGYP0[i];
		} else if((ROM_TID == 0x454D5241) && (ROM_HEADERCRC == 0x089ECF56)) {	// Mario & Luigi: Partners in Time (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_ARME0[i];
		} else if((ROM_TID == 0x504D5241) && (ROM_HEADERCRC == 0xD0BCCF56)) {	// Mario & Luigi: Partners in Time (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_ARMP0[i];
		} else if((ROM_TID == 0x4A334241) && (ROM_HEADERCRC == 0x0C22CF56)) {	// Mario Basketball: 3 on 3 (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AB3J0[i];
		} else if((ROM_TID == 0x45334241) && (ROM_HEADERCRC == 0xE6D9CF56)) {	// Mario Hoops 3 on 3 (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AB3E0[i];
		} else if((ROM_TID == 0x50334241) && (ROM_HEADERCRC == 0xB642CF56)) {	// Mario Slam Basketball (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[0] = dataBlacklist_AB3P0[i];
		} else if((ROM_TID == 0x4A485041) && (ROM_HEADERCRC == 0x4B0ECF56)
				|| (ROM_TID == 0x4A485041) && (ROM_HEADERCRC == 0x23C2CF56)) {	// Pokemon Fushigi no Dungeon: Ao no Kyuujotai (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_APHJ0[i];
		} else if((ROM_TID == 0x4A485041) && (ROM_HEADERCRC == 0x498ECF56)
				|| (ROM_TID == 0x4A485041) && (ROM_HEADERCRC == 0x398BCF56)) {	// Pokemon Fushigi no Dungeon: Ao no Kyuujotai (J) (Rev 1)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_APHJ1[i];
		} else if((ROM_TID == 0x45485041) && (ROM_HEADERCRC == 0xD376CF56)) {	// Pokemon Mystery Dungeon: Blue Rescue Team (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_APHE0[i];
		} else if((ROM_TID == 0x50485041) && (ROM_HEADERCRC == 0xF167CF56)) {	// Pokemon Mystery Dungeon: Blue Rescue Team (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_APHP0[i];
		} else if((ROM_TID == 0x4B485041) && (ROM_HEADERCRC == 0xA6C2CF56)) {	// Pokemon Bulgasaui Dungeon: Parang Gujodae (KS)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_APHK0[i];
		} else if((ROM_TID == 0x45475241) && (ROM_HEADERCRC == 0x5461CF56)) {	// Pokemon Ranger (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_ARGE0[i];
		/* } else if((ROM_TID == 0x4A575941) && (ROM_HEADERCRC == 0x404FCF56)) {	// Yoshi's Island DS (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AYWJ0[i];
		} else if((ROM_TID == 0x45575941) && (ROM_HEADERCRC == 0xA300CF56)
				|| (ROM_TID == 0x45575941) && (ROM_HEADERCRC == 0xFA95CF56)) {	// Yoshi's Island DS (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AYWE0[i];
		*/ } else if((ROM_TID == 0x4A4E4441) && (ROM_HEADERCRC == 0x462DCF56)) {	// Digimon Story (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_ADNJ0[i];
		} else if((ROM_TID == 0x454E4441) && (ROM_HEADERCRC == 0xAC46CF56)) {	// Digimon World DS (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_ADNE0[i];
		} else if((ROM_TID == 0x4A574B41) && (ROM_HEADERCRC == 0x5CE4CF56)) {	// Hoshi no Kirby: Sanjou! Dorotche Dan (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AKWJ0[i];
		} else if((ROM_TID == 0x45574B41) && (ROM_HEADERCRC == 0xC8C3CF56)) {	// Kirby Squeak Squad (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AKWE0[i];
		} else if((ROM_TID == 0x50574B41) && (ROM_HEADERCRC == 0x706CCF56)) {	// Kirby Mouse Attack (E)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_AKWP0[i];
		} else if((ROM_TID == 0x4A593341) && (ROM_HEADERCRC == 0x77E4CF56)) {	// Sonic Rush Adventure (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_A3YJ0[i];
		} else if((ROM_TID == 0x45593341) && (ROM_HEADERCRC == 0x7A5ACF56)	// Sonic Rush Adventure (U)
				|| (ROM_TID == 0x50593341) && (ROM_HEADERCRC == 0xB96BCF56)	// Sonic Rush Adventure (E)
				|| (ROM_TID == 0x50593341) && (ROM_HEADERCRC == 0xD1B2CF56)) {	// Sonic Rush Adventure (E) (v01)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_A3YE0[i];
		} else if((ROM_TID == 0x4B593341) && (ROM_HEADERCRC == 0x3DF8CF56)) {	// Sonic Rush Adventure (KS)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_A3YK0[i];
		} else if((ROM_TID == 0x454F4359) && (ROM_HEADERCRC == 0x7591CF56)) {	// Call of Duty 4: Modern Warfare (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_YCOE0[i];
		} else if((ROM_TID == 0x45594659) && (ROM_HEADERCRC == 0xB0CECF56)	// Pokemon Mystery Dungeon: Explorers of Darkness (U)
				|| (ROM_TID == 0x45544659) && (ROM_HEADERCRC == 0xA0BDCF56)) {	// Pokemon Mystery Dungeon: Explorers of Time (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_YFYE0[i];
		} else if((ROM_TID == 0x4A574B59) && (ROM_HEADERCRC == 0xF999CF56)) {	// Hoshi no Kirby: Ultra Super Deluxe (J)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_YKWJ0[i];
		} else if((ROM_TID == 0x45574B59) && (ROM_HEADERCRC == 0x317DCF56)) {	// Kirby Super Star Ultra (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_YKWE0[i];
		} else if((ROM_TID == 0x4B574B59) && (ROM_HEADERCRC == 0xE2E5CF56)) {	// Kirby Ultra Super Deluxe (KS)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_YKWK0[i];
		} else if((ROM_TID == 0x45525243) && (ROM_HEADERCRC == 0xAB01CF56)) {	// Megaman Star Force 3: Red Joker (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_CRRE0[i];
		} else if((ROM_TID == 0x45425243) && (ROM_HEADERCRC == 0xE8CFCF56)) {	// Megaman Star Force 3: Black Ace (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_CRBE0[i];
		} else if((ROM_TID == 0x454A4C43) && (ROM_HEADERCRC == 0xCE77CF56)
				|| (ROM_TID == 0x454A4C43) && (ROM_HEADERCRC == 0x8F73CF56)) {	// Mario & Luigi: Bowser's Inside Story (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_CLJE0[i];
		} else if((ROM_TID == 0x45494B42) && (ROM_HEADERCRC == 0xE25BCF56)) {	// The Legend of Zelda: Spirit Tracks (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_BKIE0[i];
		} else if((ROM_TID == 0x45494B42) && (ROM_HEADERCRC == 0x4471CF56)) {	// The Legend of Zelda: Spirit Tracks (U) (XPA's AP-patch)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_BKIE0_patch[i];
		} else if((ROM_TID == 0x455A3642) && (ROM_HEADERCRC == 0x0026CF56)) {	// MegaMan Zero Collection (U)
			for(int i = 0; i < 7; i++)
				setDataBWlist[i] = dataBlacklist_B6ZE0[i];
		}
		if(setDataBWlist[0] == 0 && setDataBWlist[1] == 0 && setDataBWlist[2] == 0){
		} else {
			ROMinRAM = 2;
			if(setDataBWlist[3] == true) {
				fileRead(ROM_LOCATION, file, setDataBWlist[0], setDataBWlist[2]);
				if(dataAmount >= 1) {
					fileRead(ROM_LOCATION+setDataBWlist[2], file, setDataBWlist_1[0], setDataBWlist_1[2]);
				}
				if(dataAmount >= 2) {
					fileRead(ROM_LOCATION+setDataBWlist[2]+setDataBWlist_1[2], file, setDataBWlist_2[0], setDataBWlist_2[2]);
				}
				if(dataAmount >= 3) {
					fileRead(ROM_LOCATION+setDataBWlist[2]+setDataBWlist_1[2]+setDataBWlist_2[2], file, setDataBWlist_3[0], setDataBWlist_3[2]);
				}
				if(dataAmount == 4) {
					fileRead(ROM_LOCATION+setDataBWlist[2]+setDataBWlist_1[2]+setDataBWlist_2[2]+setDataBWlist_3[2], file, setDataBWlist_4[0], setDataBWlist_4[2]);
				}
			} else {
				setDataBWlist[0] -= 0x4000;
				setDataBWlist[0] -= ARM9_LEN;
				fileRead(ROM_LOCATION, file, 0x4000+ARM9_LEN, setDataBWlist[0]);
				u32 lastRomSize = 0;
				for(u32 i = setDataBWlist[1]; i < romSize; i++) {
					lastRomSize++;
				}
				fileRead(ROM_LOCATION+setDataBWlist[0], file, setDataBWlist[1], lastRomSize);
			}
		}
	}

	hookNdsRetail9((u32*)ENGINE_LOCATION_ARM9, ROMinRAM, cleanRomSize);
}

u32 numberToActivateRunViaHalt = 30;

void setNumberToActivateRunViaHalt (void) {
	// Set number of reads to activate running cardEngine7 via halt SWI branch
	if ((ROM_TID & 0x00FFFFFF) == 0x545041			// Pokemon Trozei
	|| (ROM_TID & 0x00FFFFFF) == 0x334241			// Mario Hoops 3 on 3
	|| (ROM_TID & 0x00FFFFFF) == 0x474C59			// LEGO Star Wars: The Complete Saga
	|| (ROM_TID & 0x00FFFFFF) == 0x574B59)			// Kirby Super Star Ultra
	{
		numberToActivateRunViaHalt = 10;
	}
	else if ((ROM_TID & 0x00FFFFFF) == 0x474B59)	// Kingdom Hearts: 358-2 Days
	{
		numberToActivateRunViaHalt = 20;
	}
	else if ((ROM_TID & 0x00FFFFFF) == 0x484241	// Resident Evil: Deadly Silence
			|| (ROM_TID & 0x00FFFFFF) == 0x413641	// MegaMan Star Force: Pegasus
			|| (ROM_TID & 0x00FFFFFF) == 0x423641	// MegaMan Star Force: Leo
			|| (ROM_TID & 0x00FFFFFF) == 0x433641	// MegaMan Star Force: Dragon
			|| (ROM_TID & 0x00FFFFFF) == 0x583642)	// Rockman EXE: Operate Star Force
	{
		numberToActivateRunViaHalt = 24;
	}
	else if ((ROM_TID & 0x00FFFFFF) == 0x4D4441)	// Animal Crossing: Wild World
	{
		numberToActivateRunViaHalt = 40;
	}
	else if ((ROM_TID & 0x00FFFFFF) == 0x484D41	// Metroid Prime Hunters
			|| (ROM_TID & 0x00FFFFFF) == 0x514D41)	// Mario Vs Donkey Kong 2: March of the Minis
	{
		numberToActivateRunViaHalt = 55;
	}
	
	if (consoleModel > 0) {
		if ((ROM_TID & 0x00FFFFFF) == 0x574B41		// Kirby Squeak Squad
		|| (ROM_TID & 0x00FFFFFF) == 0x593341)		// Sonic Rush Adventure
		{
			numberToActivateRunViaHalt = 2;
		}
	} else {
		if ((ROM_TID & 0x00FFFFFF) == 0x593341)	// Sonic Rush Adventure
		{
			numberToActivateRunViaHalt = 20;
		}
	}
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
	*((vu32*)0x027FFE24) = TEMP_ARM9_START_ADDRESS;
	// Get the ARM9 to boot
	arm9_stateFlag = ARM9_BOOTBIN;

	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	// Start ARM7
	VoidFn arm7code = *(VoidFn*)(0x27FFE34);
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
	// this function have no effect with ARM7 SCFG locked
	
	// arm7 is master of WRAM-A, arm9 of WRAM-B & C
	REG_MBK9=0x3000000F;
	
	// WRAM-A fully mapped to arm7
	*((vu32*)REG_MBK1)=0x8185898D; // same as dsiware
	
	// WRAM-B fully mapped to arm7 // inverted order
	*((vu32*)REG_MBK2)=0x9195999D;
	*((vu32*)REG_MBK3)=0x8185898D;
	
	// WRAM-C fully mapped to arm7 // inverted order
	*((vu32*)REG_MBK4)=0x9195999D;
	*((vu32*)REG_MBK5)=0x8185898D;
	
	// WRAM mapped to the 0x3700000 - 0x37FFFFF area 
	// WRAM-A mapped to the 0x37C0000 - 0x37FFFFF area : 256k
	REG_MBK6=0x080037C0; // same as dsiware
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
}

static const unsigned char dldiMagicString[] = "\xED\xA5\x8D\xBF Chishm";	// Normal DLDI file

void arm7_main (void) {
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

	if ((file.firstCluster < CLUSTER_FIRST) || (file.firstCluster >= CLUSTER_EOF)) 	/* Invalid file cluster specified */
	{
		file = getBootFileCluster(bootName);
	}
	if (file.firstCluster == CLUSTER_FREE)
	{
		nocashMessage("fileCluster == CLUSTER_FREE");
		return -1;
	}

	int errorCode;

	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	// Get ARM7 to clear RAM
	nocashMessage("Get ARM7 to clear RAM");
	debugOutput();	// 1 dot
	resetMemory_ARM7();

	if (REG_SCFG_EXT == 0 || ntrTouch) {
		NDSTouchscreenMode();
		*(u16*)(0x4000500) = 0x807F;
	}

	// Load the NDS file
	nocashMessage("Load the NDS file");
	loadBinary_ARM7(file);
	increaseLoadBarLength();	// 2 dots

	nocashMessage("try to patch card");
	copyLoop (ENGINE_LOCATION_ARM7, (u32*)cardengine_arm7_bin, cardengine_arm7_bin_size);
	increaseLoadBarLength();	// 3 dots
	copyLoop (ENGINE_LOCATION_ARM9, (u32*)cardengine_arm9_bin, cardengine_arm9_bin_size);
	increaseLoadBarLength();	// 4 dots

	module_params_t* params = findModuleParams(NDS_HEAD, donorSdkVer);
	if(params)
	{
		ensureArm9Decompressed(NDS_HEAD, params);
	}
	increaseLoadBarLength();	// 5 dots

	errorCode = patchCardNds(NDS_HEAD, ENGINE_LOCATION_ARM7, ENGINE_LOCATION_ARM9, params, saveFileCluster, saveSize, patchMpuRegion, patchMpuSize);
	if(errorCode == ERR_NONE) {
		nocashMessage("patch card Sucessfull");
	} else {
		nocashMessage("game uses thumb");
		errorOutput();
	}
	increaseLoadBarLength();	// 6 dots

	setNumberToActivateRunViaHalt();
	errorCode = hookNdsRetail(NDS_HEAD, file, (const u32*)CHEAT_DATA_LOCATION, (u32*)CHEAT_ENGINE_LOCATION, (u32*)ENGINE_LOCATION_ARM7);
	if(errorCode == ERR_NONE) {
		nocashMessage("card hook Sucessfull");
	} else {
		nocashMessage("error during card hook");
		errorOutput();
	}
	increaseLoadBarLength();	// 7 dots
 


	loadRomIntoRam(file);

	if(romread_LED == 1 || romread_LED == 2) {
		i2cWriteRegister(0x4A, 0x30, 0x12);    // Turn WiFi LED off
	}

	nocashMessage("Start the NDS file");
	increaseLoadBarLength();	// and finally, 8 dots
	startBinary_ARM7();

	return 0;
}

