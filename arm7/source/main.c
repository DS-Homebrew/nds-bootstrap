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

#include <maxmod7.h>

#include "fifocheck.h"
#include "sdmmcEngine.h"

//---------------------------------------------------------------------------------
void SyncHandler(void) {
//---------------------------------------------------------------------------------
	runSdMmcEngineCheck();
}

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	runSdMmcEngineCheck();
	inputGetAndSend();
}

static void myFIFOValue32Handler(u32 value,void* data)
{
  nocashMessage("myFIFOValue32Handler");

  nocashMessage("default");
  nocashMessage("fifoSendValue32");
  fifoSendValue32(FIFO_USER_02,*((unsigned int*)value));	

}

//---------------------------------------------------------------------------------
void initMBK() {
//---------------------------------------------------------------------------------
	// DS browser setting
	/*REG_MBK_1=0x8185838D;
	REG_MBK_2=0x8185838D;
	REG_MBK_3=0x9195999D;
	REG_MBK_4=0x0105098D;
	REG_MBK_5=0x9195999D;
	REG_MBK_6=0x080037C0;
	REG_MBK_7=0x07C03780;
	REG_MBK_8=0x07803700;
	REG_MBK_9=0xFCF8FF0F;*/

	// DS compat setting
	/*REG_MBK_1=0x8185898D;
	REG_MBK_2=0x81858991;
	REG_MBK_3=0x91959991;
	REG_MBK_4=0x81858991;
	REG_MBK_5=0x91959991;
	REG_MBK_6=0x09403900;
	REG_MBK_7=0x09803940;
	REG_MBK_8=0x09C03980;
	REG_MBK_9=0xFCFFFF0F;*/
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	
	initMBK();
	
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	mmInstall(FIFO_MAXMOD);

	SetYtrigger(80);

	installSoundFIFO();
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

