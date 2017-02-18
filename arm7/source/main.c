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

// #include <maxmod7.h>
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

u32 dsi_powerOffSlot1() {
	// Power Off Slot
	while(REG_SCFG_MC&0x0C !=  0x0C); // wait until state<>3
	if(REG_SCFG_MC&0x0C != 0x08) return 1; // exit if state<>2      
	
	REG_SCFG_MC = 0x0C; // set state=3 
	while(REG_SCFG_MC&0x0C !=  0x00); // wait until state=0
}

u32 dsi_powerOnSlot1() {
	// Power On Slot
	while(REG_SCFG_MC&0x0C !=  0x0C); // wait until state<>3
	if(REG_SCFG_MC&0x0C != 0x00) return 1; //  exit if state<>0
	
	REG_SCFG_MC = 0x04; // set state=1
	while(REG_SCFG_MC&0x0C != 0x04); // wait until state=1
	
	REG_SCFG_MC = 0x08; // set state=2      
	while(REG_SCFG_MC&0x0C != 0x08); // wait until state=2
	
	REG_ROMCTRL = 0x20000000; // set ROMCTRL=20000000h
	
	while(REG_ROMCTRL&0x8000000 != 0x8000000); // wait until ROMCTRL.bit31=1
	
	return 0;
}

//---------------------------------------------------------------------------------
u32 dsi_resetSlot1() {
//---------------------------------------------------------------------------------	
	dsi_powerOffSlot1();
	for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }
	dsi_powerOnSlot1();	
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	
	initMBK();
	
	// Find the DLDI reserved space in the file
	u32 patchOffset = quickFind (__NDSHeader->arm9destination, dldiMagicString, __NDSHeader->arm9binarySize, sizeof(dldiMagicString));
	wordCommandAddr = (u32 *) (((u32)__NDSHeader->arm9destination)+patchOffset+0x80);
	
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	// mmInstall(FIFO_MAXMOD);

	SetYtrigger(80);

	// installSoundFIFO();
	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_IPC_SYNC, SyncHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK | IRQ_IPC_SYNC);
  
	REG_IPC_SYNC|=IPC_SYNC_IRQ_ENABLE; 

	fifoWaitValue32(FIFO_USER_03);
	if(fifoCheckValue32(FIFO_USER_04)) { dsi_resetSlot1(); }
	fifoSendValue32(FIFO_USER_05, 1);

	fifoSetValue32Handler(FIFO_USER_01,myFIFOValue32Handler,0);

	// Keep the ARM7 mostly idle
	while (1) { swiWaitForVBlank(); fifocheck(); }
}

