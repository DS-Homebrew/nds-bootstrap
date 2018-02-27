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

#define BASE_DELAY (100)

void twlEnableSlot1() {
	int oldIME = enterCriticalSection();

	while((REG_SCFG_MC & 0x0c) == 0x0c) swiDelay(1 * BASE_DELAY);

	if(!(REG_SCFG_MC & 0x0c)) {

		REG_SCFG_MC = (REG_SCFG_MC & ~0x0c) | 4;
		swiDelay(10 * BASE_DELAY);
		REG_SCFG_MC = (REG_SCFG_MC & ~0x0c) | 8;
		swiDelay(10 * BASE_DELAY);
	}
	leaveCriticalSection(oldIME);
}

void twlDisableSlot1() {
	int oldIME = enterCriticalSection();

	while((REG_SCFG_MC & 0x0c) == 0x0c) swiDelay(1 * BASE_DELAY);

	if((REG_SCFG_MC & 0x0c) == 8) {

		REG_SCFG_MC = (REG_SCFG_MC & ~0x0c) | 0x0c;
		while((REG_SCFG_MC & 0x0c) != 0) swiDelay(1 * BASE_DELAY);
	}

	leaveCriticalSection(oldIME);
}

static vu32 * wordCommandAddr;

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	inputGetAndSend();
}


void myFIFOValue32Handler(u32 value,void* data)
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

static char hexbuffer [9];

char* tohex(u32 n)
{
    unsigned size = 9;
    char *buffer = hexbuffer;
    unsigned index = size - 2;

	for (int i=0; i<size; i++) {
		buffer[i] = '0';
	}

    while (n > 0)
    {
        unsigned mod = n % 16;

        if (mod >= 10)
            buffer[index--] = (mod - 10) + 'A';
        else
            buffer[index--] = mod + '0';

        n /= 16;
    }
    buffer[size - 1] = '\0';
    return buffer;
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	// Switch to NTR Mode
	//REG_SCFG_ROM = 0x703;

	// read User Settings from firmware
	readUserSettings();
	irqInit();

	// Start the RTC tracking IRQ
	initClockIRQ();
	fifoInit();

	SetYtrigger(80);

	installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT);

	i2cWriteRegister(0x4A, 0x12, 0x00);		// Press power-button for auto-reset
	i2cWriteRegister(0x4A, 0x70, 0x01);		// Bootflag = Warmboot/SkipHealthSafety

	swiIntrWait(0,IRQ_FIFO_NOT_EMPTY);
	//
	int romread_LED = fifoGetValue32(FIFO_USER_05);
	if(romread_LED == 1) {
		i2cWriteRegister(0x4A, 0x72, 0x01);		// Set to use WiFi LED as card read indicator
		i2cWriteRegister(0x4A, 0x30, 0x12);    // Turn WiFi LED off
	} else if(romread_LED == 2) {
		i2cWriteRegister(0x4A, 0x72, 0x02);		// Set to use power LED (turn to purple) as card read indicator
	} else if(romread_LED == 3) {
		i2cWriteRegister(0x4A, 0x72, 0x03);		// Set to use Camera LED as card read indicator
	}

	if(fifoCheckValue32(FIFO_USER_04)) {
		i2cWriteRegister(0x4A, 0x73, 0x01);		// Set to run compatibility check
	}

	SCFGFifoCheck();
	//
	fifoSendValue32(FIFO_USER_05, 1);

	fifoSetValue32Handler(FIFO_USER_01,myFIFOValue32Handler,0);

	// Keep the ARM7 mostly idle
	while (1) {
		swiIntrWait(0,IRQ_FIFO_NOT_EMPTY);
	}
}

