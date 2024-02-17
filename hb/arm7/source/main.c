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

void VcountHandler(void) {
	inputGetAndSend();
}

/*static void myFIFOValue32Handler(u32 value,void* data)
{
	nocashMessage("myFIFOValue32Handler");
 	nocashMessage("default");
	nocashMessage("fifoSendValue32");
	fifoSendValue32(FIFO_USER_02, *(u16*)value);
}*/

void dsi_resetSlot1() {

#define REG_SCFG_MC_MASK (REG_SCFG_MC & 0x0C)

	// Power off Slot.
	while(REG_SCFG_MC_MASK != 0x0C); 	// Wait until state<>3.
	if(REG_SCFG_MC_MASK != 0x08) return; 	// Exit if state<>2.

	REG_SCFG_MC = 0x0C;          		// Set state=3.
	while(REG_SCFG_MC_MASK != 0x00);	// Wait until state=0.

	swiWaitForVBlank();

	// Power On Slot.
	while(REG_SCFG_MC_MASK != 0x0C);	// Wait until state<>3.
	if(REG_SCFG_MC_MASK != 0x00) return;	// Exit if state<>0.

	REG_SCFG_MC = 0x04;		// Wait 1ms, then set state=1.
	while(REG_SCFG_MC_MASK != 0x04);

	REG_SCFG_MC = 0x08;		// Wait 10ms, then set state=2.
	while(REG_SCFG_MC_MASK != 0x08);

	REG_ROMCTRL = 0x20000000;	// Wait 27ms, then set REG_ROMCTRL=20000000h.

	while((REG_ROMCTRL & 0x8000000) != 0x8000000);
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	// Grab from DS header in GBA slot
	*(u16*)0x02FFFC36 = *(u16*)0x0800015E;	// Header CRC16
	*(u32*)0x02FFFC38 = *(u32*)0x0800000C;	// Game Code

	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	SetYtrigger(80);

	installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT );

	if (isDSiMode() && REG_SCFG_EXT == 0) {
		u32 wordBak = *(vu32*)0x037C0000;
		*(vu32*)0x037C0000 = 0x414C5253;
		if (*(vu32*)0x037C0000 == 0x414C5253 && *(vu32*)0x037C8000 != 0x414C5253) {
			*(u32*)0x02FFE1A0 = 0x080037C0;
		}
		*(vu32*)0x037C0000 = wordBak;
	}

    nocashMessage("init completed");

    nocashMessage("wait for FIFO");

	swiIntrWait(0,IRQ_FIFO_NOT_EMPTY);
	//
	if(fifoCheckValue32(FIFO_USER_04)) { dsi_resetSlot1(); }
	fifocheck();
    nocashMessage("fifoSendValue32(FIFO_USER_05, 1);");
	//
	fifoSendValue32(FIFO_USER_05, 1);


	//fifoSetValue32Handler(FIFO_USER_01,myFIFOValue32Handler,0);

	// Keep the ARM7 mostly idle
	while (1) {
		swiIntrWait(0, IRQ_FIFO_NOT_EMPTY);
	}
}
