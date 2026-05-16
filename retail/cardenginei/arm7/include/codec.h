/*---------------------------------------------------------------------------------

	DSi "codec" Touchscreen/Sound Controller control for ARM7

	Copyright (C) 2017
		fincs

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
#ifndef TWLSDK
#ifndef ARM7_CODEC_INCLUDE
#define ARM7_CODEC_INCLUDE
//---------------------------------------------------------------------------------


#ifndef ARM7
#error DSi TSC is only available on the ARM7
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/arm7/serial.h>
#include <nds/memory.h>
#include <nds/system.h>
#include <nds/touch.h>

static inline bool cdcIsAvailable(void) {
	return true;
}

enum cdcBanks {
	CDC_CONTROL     = 0x00, // Chip control
	CDC_SOUND       = 0x01, // ADC/DAC control
	CDC_TOUCHCNT	= 0x03, // TSC control
	CDC_TOUCHDATA	= 0xFC, // TSC data buffer
};

// Direct register functions
u8   cdcReadReg(u8 bank, u8 reg);
void cdcReadRegArray(u8 bank, u8 reg, void* data, u8 size);
void cdcWriteReg(u8 bank, u8 reg, u8 value);
void cdcWriteRegMask(u8 bank, u8 reg, u8 mask, u8 value);
void cdcWriteRegArray(u8 bank, u8 reg, const void* data, u8 size);

// Touchscreen functions
void cdcTouchInit(void);
bool cdcTouchPenDown(void);
bool cdcTouchRead(touchPosition* pos);

#ifdef __cplusplus
}
#endif


//---------------------------------------------------------------------------------
#endif	// ARM7_CODEC_INCLUDE
#endif	// TWLSDK
//---------------------------------------------------------------------------------

