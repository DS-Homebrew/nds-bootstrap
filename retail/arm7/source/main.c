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

//#include <stdio.h>
#include <stdlib.h> // NULL
#include <string.h>
#include <nds/ndstypes.h>
#include <nds/arm7/input.h>
#include <nds/fifocommon.h>
#include <nds/system.h>
#include <nds/arm7/aes.h>
#include <nds/arm7/clock.h>
#include <nds/arm7/i2c.h>
#include <nds/debug.h>

#include "hex.h"
#include "fifocheck.h"

//static vu32* wordCommandAddr;

void my_installSystemFIFO(void);
void my_sdmmc_get_cid(int devicenumber, u32 *cid);

void VcountHandler(void) {
	inputGetAndSend();
}

void set_ctr(u32* ctr){
	for (int i = 0; i < 4; i++) REG_AES_IV[i] = ctr[3-i];
}

// 10 11  22 23 24 25
void aes(void* in, void* out, void* iv, u32 method){ //this is sort of a bodged together dsi aes function adapted from this 3ds function
	REG_AES_CNT = ( AES_CNT_MODE(method) |           //https://github.com/TiniVi/AHPCFW/blob/master/source/aes.c#L42
					AES_WRFIFO_FLUSH |				 //as long as the output changes when keyslot values change, it's good enough.
					AES_RDFIFO_FLUSH | 
					AES_CNT_KEY_APPLY | 
					AES_CNT_KEYSLOT(3) |
					AES_CNT_DMA_WRITE_SIZE(2) |
					AES_CNT_DMA_READ_SIZE(1)
					);
					
    if (iv != NULL) set_ctr((u32*)iv);
	REG_AES_BLKCNT = (1 << 16);
	REG_AES_CNT |= 0x80000000;
	
	for (int j = 0; j < 0x10; j+=4) REG_AES_WRFIFO = *((u32*)(in+j));
	while(((REG_AES_CNT >> 0x5) & 0x1F) < 0x4); //wait for every word to get processed
	for (int j = 0; j < 0x10; j+=4) *((u32*)(out+j)) = REG_AES_RDFIFO;
	//REG_AES_CNT &= ~0x80000000;
	//if (method & (AES_CTR_DECRYPT | AES_CTR_ENCRYPT)) add_ctr((u8*)iv);
}

void myFIFOValue32Handler(u32 value, void* userdata) {
	nocashMessage("myFIFOValue32Handler");
 	nocashMessage("default");
	nocashMessage("fifoSendValue32");
	fifoSendValue32(FIFO_USER_02, *(u16*)value);
}

int main(void) {
	*(u16*)0x02FFFC30 = *(vu16*)0x4004700; // SNDEXCNT (Used for checking for regular DS or DSi/3DS in DS mode)

	// Grab from DS header in GBA slot
	*(u16*)0x02FFFC36 = *(u16*)0x0800015E;	// Header CRC16
	*(u32*)0x02FFFC38 = *(u32*)0x0800000C;	// Game Code

	*(u32*)0x02FFFDF0 = REG_SCFG_EXT;

	*(vu32*)0x400481C = 0;				// Clear SD IRQ stat register
	*(vu32*)0x4004820 = 0x8B7F0305;
	*(u8*)0x02FFFDF4 = (*(vu32*)0x4004820 != 0) ? 1 : 0; // SD/MMC access is enabled in SCFG
	*(vu32*)0x4004820 = 0;				// Clear SD IRQ mask register

	// read User Settings from firmware
	readUserSettings();
	irqInit();

	// Start the RTC tracking IRQ
	initClockIRQ();
	fifoInit();

	SetYtrigger(80);

	my_installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);

	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);

	if (isDSiMode()) {
		i2cWriteRegister(I2C_PM, I2CREGPM_MMCPWR, 0);		// Press power button for auto-reset
		//i2cWriteRegister(I2C_PM, I2CREGPM_MMCPWR, 1);		// Have IRQ check for power button press
		//i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);		// SDK 5 --> Bootflag = Warmboot/SkipHealthSafety
	}

	if (isDSiMode() && REG_SCFG_EXT == 0) {
		u32 wordBak = *(vu32*)0x037C0000;
		*(vu32*)0x037C0000 = 0x414C5253;
		if (*(vu32*)0x037C0000 == 0x414C5253 && *(vu32*)0x037C8000 != 0x414C5253) {
			*(u32*)0x02FFE1A0 = 0x080037C0;
		}
		*(vu32*)0x037C0000 = wordBak;
	}

	if (isDSiMode()) {
		u8 *out=(u8*)0x02074000;
		memset(out, 0, 17);

		// Save whether this is a dev unit or not. For 3DS NAND reading...
		// This does not imply 32 MBs of RAM!
		out[16] = (*((uint16_t*)0x04004024)) & 0x13; // Is this a dev unit?

		/*for (int i = 0; i < 8; i++) {
			*(u8*)(0x2FFFD00+i) = *(u8*)(0x4004D07-i);	// Get ConsoleID
		}*/

		// For getting ConsoleID without reading from 0x4004D00...

		u8 base[16]={0};
		u8 in[16]={0};
		u8 iv[16]={0};
		u8 *scratch=(u8*)0x02074200; 
		u8 *key3=(u8*)0x40044D0;
		
		aes(in, base, iv, 2);

		//write consecutive 0-255 values to any byte in key3 until we get the same aes output as "base" above - this reveals the hidden byte. this way we can uncover all 16 bytes of the key3 normalkey pretty easily.
		//greets to Martin Korth for this trick https://problemkaputt.de/gbatek.htm#dsiaesioports (Reading Write-Only Values)
		for(int i=0;i<16;i++){  
			for(int j=0;j<256;j++){
				*(key3+i)=j & 0xFF;
				aes(in, scratch, iv, 2);
				if(!memcmp(scratch, base, 16)){
					out[i]=j;
					//hit++;
					break;
				}
			}
		}
	}

	swiIntrWait(0, IRQ_FIFO_NOT_EMPTY);

	SCFGFifoCheck();

	fifoSendValue32(FIFO_USER_05, 1);

	fifoSetValue32Handler(FIFO_USER_01, myFIFOValue32Handler, NULL);

	// Keep the ARM7 mostly idle
	while (1) {
		if (*(u32*)(0x2FFFD0C) == 0x454D4D43) {
			my_sdmmc_get_cid(true, (u32*)0x2FFD7BC);	// Get eMMC CID
			*(u32*)(0x2FFFD0C) = 0;
		}
		swiIntrWait(0, IRQ_FIFO_NOT_EMPTY);
	}
	
	return 0;
}
