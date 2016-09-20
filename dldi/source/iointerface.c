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

void sendValue32(u32 value32) {
	nocashMessage("sendValue32");
	*((vu32*)0x027FEE24) = (u32)0x027FEE04;
	*((vu32*)0x027FEE28) = value32;
	IPC_SendSync(0xEE24);
}

void sendMsg(int size, u8* msg) {
	nocashMessage("sendMsg");
	*((vu32*)0x027FEE24) = (u32)0x027FEE05;
	*((vu32*)0x027FEE28) = size;
	for(int i=0;i<size;i++)  {
		*((u8*)0x027FEE2C+i) = msg[i];
	}	
	IPC_SendSync(0xEE24);
}

void waitValue32() {
	nocashMessage("waitValue32");
	while(*((vu32*)0x027FEE24) != (u32)0x027FEE08);
}

u32 getValue32() {
	nocashMessage("getValue32");
	return *((vu32*)0x027FEE28);
}

//---------------------------------------------------------------------------------
bool sd_Startup() {
//---------------------------------------------------------------------------------
	nocashMessage("sdio_Startup");
	//if (!isSDAcessible()) return false;
	
	REG_SCFG_EXT &= 0xC000;

	sendValue32(SDMMC_HAVE_SD);

	waitValue32();

	int result = getValue32();

	if(result==0) return false;

	sendValue32(SDMMC_SD_START);

	waitValue32();

	result = getValue32();
	
	return result == 0;
}

//---------------------------------------------------------------------------------
bool sd_IsInserted() {
//---------------------------------------------------------------------------------
	/*if (!isSDAcessible()) return false;

	fifoSendValue32(FIFO_SDMMCDSI,SDMMC_SD_IS_INSERTED);

	fifoWaitValue32(FIFO_SDMMCDSI);

	int result = fifoGetValue32(FIFO_SDMMCDSI);*/

	return true;
}

//---------------------------------------------------------------------------------
bool sd_ReadSectors(sec_t sector, sec_t numSectors,void* buffer) {
//---------------------------------------------------------------------------------
	nocashMessage("sd_ReadSectors");
	//if (!isSDAcessible()) return false;
	FifoMessage msg;
	
	vu32* mybuffer = (vu32*)0x027F0E24;

	DC_FlushRange(buffer,numSectors * 512);	

	msg.type = SDMMC_SD_READ_SECTORS;
	msg.sdParams.startsector = sector;
	msg.sdParams.numsectors = numSectors;
	msg.sdParams.buffer = mybuffer;
	
	sendMsg(sizeof(msg), (u8*)&msg);

	waitValue32();

	int result = getValue32();
	
	swiFastCopy(mybuffer, buffer, numSectors*512);
	
	return result == 0;
	
	return false;
}

//---------------------------------------------------------------------------------
bool sd_WriteSectors(sec_t sector, sec_t numSectors,const void* buffer) {
//---------------------------------------------------------------------------------
	nocashMessage("sd_ReadSectors");
	//if (!isSDAcessible()) return false;
	FifoMessage msg;
	
	vu32* mybuffer = (vu32*)0x027F0E24;

	DC_FlushRange(buffer,numSectors * 512);		
	
	swiFastCopy(buffer, mybuffer, numSectors*512);

	msg.type = SDMMC_SD_WRITE_SECTORS;
	msg.sdParams.startsector = sector;
	msg.sdParams.numsectors = numSectors;
	msg.sdParams.buffer = mybuffer;
	
	sendMsg(sizeof(msg), (u8*)&msg);

	waitValue32();

	int result = getValue32();	
	
	return result == 0;
	
	return false;
}


//---------------------------------------------------------------------------------
bool sd_ClearStatus() {
//---------------------------------------------------------------------------------
	//if (!isSDAcessible()) return false;
	return true;
}

//---------------------------------------------------------------------------------
bool sd_Shutdown() {
//---------------------------------------------------------------------------------
	//if (!isSDAcessible()) return false;
	return true;
}


/*-----------------------------------------------------------------
startUp
Initialize the interface, geting it into an idle, ready state
returns true if successful, otherwise returns false
-----------------------------------------------------------------*/
bool startup(void) {	
	nocashMessage("startup");
	return sd_Startup();
}

/*-----------------------------------------------------------------
isInserted
Is a card inserted?
return true if a card is inserted and usable
-----------------------------------------------------------------*/
bool isInserted (void) {
	nocashMessage("isInserted");
	return sd_IsInserted();
}


/*-----------------------------------------------------------------
clearStatus
Reset the card, clearing any status errors
return true if the card is idle and ready
-----------------------------------------------------------------*/
bool clearStatus (void) {
	nocashMessage("clearStatus");
	return sd_ClearStatus();
}


/*-----------------------------------------------------------------
readSectors
Read "numSectors" 512-byte sized sectors from the card into "buffer", 
starting at "sector". 
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool readSectors (u32 sector, u32 numSectors, void* buffer) {
	nocashMessage("readSectors");
	return sd_ReadSectors(sector,numSectors,buffer);
}



/*-----------------------------------------------------------------
writeSectors
Write "numSectors" 512-byte sized sectors from "buffer" to the card, 
starting at "sector".
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool writeSectors (u32 sector, u32 numSectors, void* buffer) {
	nocashMessage("writeSectors");
	return sd_WriteSectors(sector,numSectors,buffer);
}

/*-----------------------------------------------------------------
shutdown
shutdown the card, performing any needed cleanup operations
Don't expect this function to be called before power off, 
it is merely for disabling the card.
return true if the card is no longer active
-----------------------------------------------------------------*/
bool shutdown(void) {
	nocashMessage("shutdown");
	return sd_Shutdown();
}