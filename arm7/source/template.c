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
// #include <nds/fifocommon.h>

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	inputGetAndSend();
}
/*
void ResetSlot() {
	unsigned int * ROMCTRL=(unsigned int*)0x40001A4; 
	unsigned int * SCFG_ROM=(unsigned int*)0x4004000;
	unsigned int * SCFG_CLK=(unsigned int*)0x4004004; 
	unsigned int * SCFG_EXT=(unsigned int*)0x4004008; 
	unsigned int * SCFG_MC=(unsigned int*)0x4004010; 

	// Wait for arm9.
	fifoWaitValue32(FIFO_USER_02);

	//Power Off Slot
	int backup =*SCFG_EXT;
	*SCFG_EXT=0xFFFFFFFF;	

	while(*SCFG_MC&0x0C !=  0x0C); 		// wait until state<>3
	if(*SCFG_MC&0x0C != 0x08) return; 		// exit if state<>2      
	
	*SCFG_MC = 0x0C;          		// set state=3 
	while(*SCFG_MC&0x0C !=  0x00);  // wait until state=0

	// Tells arm9 to continue after powering off slot. (so that card init does not occur too soon)
	fifoSendValue32(FIFO_USER_01, 1);

	// Power On Slot
	while(*SCFG_MC&0x0C !=  0x0C); // wait until state<>3
	if(*SCFG_MC&0x0C != 0x00) return; //  exit if state<>0
	
	*SCFG_MC = 0x04;    // wait 1ms, then set state=1
	while(*SCFG_MC&0x0C != 0x04);
	
	*SCFG_MC = 0x08;    // wait 10ms, then set state=2      
	while(*SCFG_MC&0x0C != 0x08);
	
	*ROMCTRL = 0x20000000; // wait 27ms, then set ROMCTRL=20000000h
	
	while(*ROMCTRL&0x8000000 != 0x8000000);

	*SCFG_EXT=backup;
}
*/

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	unsigned int * SCFG_CLK=(unsigned int*)0x4004004; 
	unsigned int * SCFG_EXT=(unsigned int*)0x4004008; 

	// SCFG_EXT
	// 0x92A00000 : NTR
	// 0x93FFFF07 : TWL
	// 0x93FF0F07 : max accessible in NTR mode
	if(*SCFG_EXT == 0x92A00000) {
		*SCFG_EXT |= 0x830F0100; // NAND ACCESS
		// SCFG_CLK
		// 0x0180 : NTR
		// 0x0187 : TWL
		// 
		*SCFG_CLK |= 1;
	}	

	irqInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	fifoInit();

	SetYtrigger(80);

	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);   

	//Card Reset. Enable if needed.
	//ResetSlot();

	// Keep the ARM7 mostly idle
	while (1) swiWaitForVBlank();
}

