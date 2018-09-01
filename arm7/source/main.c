/*---------------------------------------------------------------------------------

default ARM7 core

Copyright (C) 2005 - 2010
	Michael Noland (joat)
	Jason Rogers (dovoto)
	Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
	must not claim that you wrote the original software. If you use
	this software in a product, an acknowledgment in the product
	documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
	must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
	distribution.

---------------------------------------------------------------------------------*/
#include <nds.h>

#include <nds/ndstypes.h>

#include "fifocheck.h"
#include "sdmmcEngine.h"

static u32 * wordCommandAddr;

//---------------------------------------------------------------------------------
void SyncHandler(void) {
//---------------------------------------------------------------------------------
	runSdMmcEngineCheck(myMemUncached(wordCommandAddr));
}

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	runSdMmcEngineCheck(myMemUncached(wordCommandAddr));
	inputGetAndSend();
}

static void myFIFOValue32Handler(u32 value,void* data)
{
  nocashMessage("myFIFOValue32Handler");

  nocashMessage("default");
  nocashMessage("fifoSendValue32");
  fifoSendValue32(FIFO_USER_02,*((unsigned int*)value));	

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

static const unsigned char dldiMagicString[] = "\xED\xA5\x8D\xBF Chishm";	// Normal DLDI file

void dsi_resetSlot1() {
		
	// Power off Slot
	while(REG_SCFG_MC&0x0C !=  0x0C); 		// wait until state<>3
	if(REG_SCFG_MC&0x0C != 0x08) return; 		// exit if state<>2      
	
	REG_SCFG_MC = 0x0C;          		// set state=3 
	while(REG_SCFG_MC&0x0C !=  0x00);  // wait until state=0

	swiWaitForVBlank();

	// Power On Slot
	while(REG_SCFG_MC&0x0C !=  0x0C); // wait until state<>3
	if(REG_SCFG_MC&0x0C != 0x00) return; //  exit if state<>0
	
	REG_SCFG_MC = 0x04;    // wait 1ms, then set state=1
	while(REG_SCFG_MC&0x0C != 0x04);
	
	REG_SCFG_MC = 0x08;    // wait 10ms, then set state=2      
	while(REG_SCFG_MC&0x0C != 0x08);
	
	REG_ROMCTRL = 0x20000000; // wait 27ms, then set REG_ROMCTRL=20000000h
	
	while(REG_ROMCTRL&0x8000000 != 0x8000000);
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	nocashMessage("main arm7");
    
    //REG_SCFG_CLK = 0x0181;
	//REG_SCFG_EXT = 0x93A40000; // NAND/SD Access
    
    __NDSHeader->unitCode = 0;
    	
	// Find the DLDI reserved space in the file
	u32 patchOffset = quickFind (__DSiHeader->ndshdr.arm9destination, dldiMagicString, __DSiHeader->ndshdr.arm9binarySize, sizeof(dldiMagicString));
	if(patchOffset == -1) {
		nocashMessage("dldi not found");
	}
	wordCommandAddr = (u32 *) (((u32)__DSiHeader->ndshdr.arm9destination)+patchOffset+0x80);
    
    nocashMessage("dldi found");
	
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();


	// Start the RTC tracking IRQ
	initClockIRQ();
	
	SetYtrigger(80);

	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_IPC_SYNC, SyncHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_IPC_SYNC);
  
	REG_IPC_SYNC|=IPC_SYNC_IRQ_ENABLE; 

	i2cWriteRegister(0x4A, 0x12, 0x00);		// Press power-button for auto-reset
	i2cWriteRegister(0x4A, 0x70, 0x01);		// Bootflag = Warmboot/SkipHealthSafety
    
    nocashMessage("init completed");
    
    nocashMessage("wait for FIFO");

	swiIntrWait(0,IRQ_FIFO_NOT_EMPTY);
	//
	if(fifoCheckValue32(FIFO_USER_04)) { dsi_resetSlot1(); }
    nocashMessage("fifoSendValue32(FIFO_USER_05, 1);");
	//
	fifoSendValue32(FIFO_USER_05, 1);


	fifoSetValue32Handler(FIFO_USER_01,myFIFOValue32Handler,0);   

	// Keep the ARM7 mostly idle
	while (1) { swiIntrWait(0,IRQ_FIFO_NOT_EMPTY); fifocheck(); }
}

