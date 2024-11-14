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

#include <nds/arm7/codec.h>

//---------------------------------------------------------------------------------
static u8 readTSC(u8 reg) {
//---------------------------------------------------------------------------------

	while (REG_SPICNT & 0x80);

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = 1 | (reg << 1);

	while (REG_SPICNT & 0x80);

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH;
	REG_SPIDATA = 0;

	while (REG_SPICNT & 0x80);
	return REG_SPIDATA;
}

//---------------------------------------------------------------------------------
static void writeTSC(u8 reg, u8 value) {
//---------------------------------------------------------------------------------

	while (REG_SPICNT & 0x80);

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = reg << 1;

	while (REG_SPICNT & 0x80);

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH;
	REG_SPIDATA = value;
}

//---------------------------------------------------------------------------------
static void bankSwitchTSC(u8 bank) {
//---------------------------------------------------------------------------------

	static u8 curBank = 0x63;
	if (bank != curBank) {
		writeTSC(curBank == 0xFF ? 0x7F : 0x00, bank);
		curBank = bank;
	}
}

//---------------------------------------------------------------------------------
u8 cdcReadReg(u8 bank, u8 reg) {
//---------------------------------------------------------------------------------

	bankSwitchTSC(bank);
	return readTSC(reg);
}

//---------------------------------------------------------------------------------
void cdcWriteReg(u8 bank, u8 reg, u8 value) {
//---------------------------------------------------------------------------------

	bankSwitchTSC(bank);
	writeTSC(reg, value);
}
