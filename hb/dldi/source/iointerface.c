/*
	iointerface.c template

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. All derivative works must be clearly marked as such. Derivatives of this file
	 must have the author of the derivative indicated within the source.
  2. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define MAX_READ 53
#define BYTES_PER_READ 512

#ifndef NULL
 #define NULL 0
#endif

#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/disc_io.h>
#include <nds/debug.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/dma.h>
#include <nds/ipc.h>
#include <nds/arm9/dldi.h>
#include "my_sdmmc.h"
#include "memcpy.h"
#include "locations.h"

extern char ioType[4];
extern vu32* _start;
static vu32* word_command;
static vu32* word_params;
static vu32* words_msg;

 // Use the dldi remaining space as temporary buffer : 28k usually available
extern vu32* tmp_buf_addr;
extern vu8 allocated_space;

void sendValue32(u32 value32) {
	//nocashMessage("sendValue32");
	*((vu32*)myMemUncached(word_params)) = value32;
	*((vu32*)myMemUncached(word_command)) = (vu32)0x027FEE04;
	IPC_SendSync(0xEE24);
}

void sendMsg(int size, u8* msg) {
	//nocashMessage("sendMsg");
	*((vu32*)myMemUncached(word_params)) = size;
	for(int i=0;i<size;i++)  {
		*((u8*)myMemUncached(words_msg)+i) = msg[i];
	}
	*((vu32*)myMemUncached(word_command)) = (vu32)0x027FEE05;
	IPC_SendSync(0xEE24);
}

void waitValue32() {
	//nocashMessage("waitValue32");
    //dbg_hexa(word_command);
    //dbg_hexa(myMemUncached(word_command));
	while(*((vu32*)myMemUncached(word_command)) != (vu32)0x027FEE08);
}

u32 getValue32() {
	//nocashMessage("getValue32");
	return *((vu32*)myMemUncached(word_params));
}

/*void goodOldCopy32(u32* src, u32* dst, int size) {
	for(int i = 0 ; i<size/4; i++) {
		dst[i]=src[i];
	}
}*/

void extendedMemory(bool yes) {
	if (yes) {
		REG_SCFG_EXT = 0x8300C000;
	} else {
		REG_SCFG_EXT = 0x83000000;
	}
}

void __custom_mpu_setup();
void __custom_mpu_restore();

//---------------------------------------------------------------------------------
bool sd_Startup() {
//---------------------------------------------------------------------------------
	//nocashMessage("sdio_Startup");
	//if (!isSDAcessible()) return false;

	//REG_SCFG_EXT &= 0xC000;

	//__custom_mpu_setup();
    
    word_command = _start-6;
    word_params  = _start-5;
    words_msg    = _start-4;

	sendValue32(SDMMC_HAVE_SD);

	waitValue32();

	int result = getValue32();

	if(result==0) return false;

	sendValue32(SDMMC_SD_START);

	waitValue32();

	result = getValue32();

	//__custom_mpu_restore();

	return result == 0;
}

//---------------------------------------------------------------------------------
bool sd_ReadSectors(sec_t sector, sec_t numSectors,void* buffer) {
//---------------------------------------------------------------------------------
	//nocashMessage("sd_ReadSectors");
	//if (!isSDAcessible()) return false;
	FifoMessage msg;
	int result = 0;
	sec_t startsector, readsectors;

	//__custom_mpu_setup();

	int max_reads = ((1 << allocated_space) / 512) - 11;

	for(int numreads =0; numreads<numSectors; numreads+=max_reads) {
		startsector = sector+numreads;
		if(numSectors - numreads < max_reads) readsectors = numSectors - numreads ;
		else readsectors = max_reads;

		vu32* mybuffer = myMemUncached(tmp_buf_addr);

		msg.type = SDMMC_SD_READ_SECTORS;
		msg.sdParams.startsector = startsector;
		msg.sdParams.numsectors = readsectors;
		msg.sdParams.buffer = (u32*)mybuffer;

		sendMsg(sizeof(msg), (u8*)&msg);

		waitValue32();

		result = getValue32();

		memcpy((u32*)mybuffer, buffer+numreads*512, readsectors*512);
	}

	//__custom_mpu_restore();

	return result == 0;
}

//---------------------------------------------------------------------------------
bool sd_WriteSectors(sec_t sector, sec_t numSectors,const void* buffer) {
//---------------------------------------------------------------------------------
	//nocashMessage("sd_ReadSectors");
	//if (!isSDAcessible()) return false;
	FifoMessage msg;
	int result = 0;
	sec_t startsector, readsectors;

	//__custom_mpu_setup();

	int max_reads = ((1 << allocated_space) / 512) - 11;

	for(int numreads =0; numreads<numSectors; numreads+=max_reads) {
		startsector = sector+numreads;
		if(numSectors - numreads < max_reads) readsectors = numSectors - numreads ;
		else readsectors = max_reads;

		vu32* mybuffer = myMemUncached(tmp_buf_addr);

		memcpy(buffer+numreads*512, (u32*)mybuffer, readsectors*512);

		msg.type = SDMMC_SD_WRITE_SECTORS;
		msg.sdParams.startsector = startsector;
		msg.sdParams.numsectors = readsectors;
		msg.sdParams.buffer = (u32*)mybuffer;

		sendMsg(sizeof(msg), (u8*)&msg);

		waitValue32();

		result = getValue32();
	}

	//__custom_mpu_restore();

	return result == 0;
}

bool isArm7 = false;
bool ramDisk = false;


//---------------------------------------------------------------------------------
bool ramd_ReadSectors(u32 sector, u32 numSectors, void* buffer) {
//---------------------------------------------------------------------------------
	if (buffer >= (void*)0x02C00000 && buffer < (void*)0x03000000) {
		buffer -= 0xC00000;		// Move out of RAM disk location
	} else if (buffer >= (void*)0x02800000 && buffer < (void*)0x02C00000) {
		buffer -= 0x800000;		// Move out of RAM disk location
	} else if (buffer >= (void*)0x02400000 && buffer < (void*)0x02800000) {
		buffer -= 0x400000;		// Move out of RAM disk location
	}

	if (!isArm7) extendedMemory(true);		// Enable extended memory mode to access RAM drive
	memcpy((void*)RAM_DISK_LOCATION+(sector << 9), buffer, numSectors << 9);
	if (!isArm7) extendedMemory(false);	// Disable extended memory mode

	return true;
}

//---------------------------------------------------------------------------------
bool ramd_WriteSectors(u32 sector, u32 numSectors, void* buffer) {
//---------------------------------------------------------------------------------
	if (buffer >= (void*)0x02C00000 && buffer < (void*)0x03000000) {
		buffer -= 0xC00000;		// Move out of RAM disk location
	} else if (buffer >= (void*)0x02800000 && buffer < (void*)0x02C00000) {
		buffer -= 0x800000;		// Move out of RAM disk location
	} else if (buffer >= (void*)0x02400000 && buffer < (void*)0x02800000) {
		buffer -= 0x400000;		// Move out of RAM disk location
	}

	if (!isArm7) extendedMemory(true);		// Enable extended memory mode to access RAM drive
	memcpy(buffer, (void*)RAM_DISK_LOCATION+(sector << 9), numSectors << 9);
	if (!isArm7) extendedMemory(false);	// Disable extended memory mode

	return true;
}


/*-----------------------------------------------------------------
startUp
Initialize the interface, geting it into an idle, ready state
returns true if successful, otherwise returns false
-----------------------------------------------------------------*/
bool startup(void) {
	//nocashMessage("startup");
	isArm7 = sdmmc_read16(REG_SDSTATUS0)!=0;

	ramDisk = (ioType[0] == 'R' && ioType[1] == 'A' && ioType[2] == 'M' && ioType[3] == 'D');

	if (ramDisk) {
		return true;
	} else if (isArm7) {
		sdmmc_init();
		return SD_Init()==0;
	} else {
		return sd_Startup();
	}
}

/*-----------------------------------------------------------------
isInserted
Is a card inserted?
return true if a card is inserted and usable
-----------------------------------------------------------------*/
bool isInserted (void) {
	//nocashMessage("isInserted");
	return true;
}


/*-----------------------------------------------------------------
clearStatus
Reset the card, clearing any status errors
return true if the card is idle and ready
-----------------------------------------------------------------*/
bool clearStatus (void) {
	//nocashMessage("clearStatus");
	return true;
}


/*-----------------------------------------------------------------
readSectors
Read "numSectors" 512-byte sized sectors from the card into "buffer",
starting at "sector".
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool readSectors (u32 sector, u32 numSectors, void* buffer) {
	//nocashMessage("readSectors");
	if (ramDisk) {
		return ramd_ReadSectors(sector,numSectors,buffer);
	} else if (isArm7) {
		return my_sdmmc_sdcard_readsectors(sector,numSectors,buffer,0)==0;
	} else {
		return sd_ReadSectors(sector,numSectors,buffer);
	}
}



/*-----------------------------------------------------------------
writeSectors
Write "numSectors" 512-byte sized sectors from "buffer" to the card,
starting at "sector".
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool writeSectors (u32 sector, u32 numSectors, void* buffer) {
	//nocashMessage("writeSectors");
	if (ramDisk) {
		return ramd_WriteSectors(sector,numSectors,buffer);
	} else if (isArm7) {
		return my_sdmmc_sdcard_writesectors(sector,numSectors,buffer,-1)==0;
	} else {
		return sd_WriteSectors(sector,numSectors,buffer);
	}
}

/*-----------------------------------------------------------------
shutdown
shutdown the card, performing any needed cleanup operations
Don't expect this function to be called before power off,
it is merely for disabling the card.
return true if the card is no longer active
-----------------------------------------------------------------*/
bool shutdown(void) {
	//nocashMessage("shutdown");
	return true;
}
