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
	
	if(fifoCheckValue32(FIFO_MAXMOD)) {
		// special setting (when found special gamecode)
		volLevel = 0xAC;
	} else {
		// normal setting (for any other gamecodes)
		volLevel = 0xA7;
	}

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
	if(fifoCheckValue32(FIFO_USER_04)) {
		i2cWriteRegister(0x4A, 0x73, 0x01);		// Set to run compatibility check
	}

	NDSTouchscreenMode();
	*(u16*)(0x4000500) = 0x807F;

	SCFGFifoCheck();
	//
	fifoSendValue32(FIFO_USER_05, 1);

	fifoSetValue32Handler(FIFO_USER_01,myFIFOValue32Handler,0);

	// Keep the ARM7 mostly idle
	while (1) {
		swiIntrWait(0,IRQ_FIFO_NOT_EMPTY);
	}
}

