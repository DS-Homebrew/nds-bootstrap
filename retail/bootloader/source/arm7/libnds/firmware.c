/*---------------------------------------------------------------------------------

  Copyright (C) 2014
	Dave Murphy (WinterMute)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1.  The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.
  2.  Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.
  3.  This notice may not be removed or altered from any source
      distribution.

---------------------------------------------------------------------------------*/
#include <nds/arm7/serial.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <string.h>

static u8 readwriteSPI(u8 data) {
	REG_SPIDATA = data;
	SerialWaitBusy();
	return REG_SPIDATA;
}

//---------------------------------------------------------------------------------
void readFirmware(u32 address, void * destination, u32 size) {
//---------------------------------------------------------------------------------
	int oldIME=enterCriticalSection();
	u8 *buffer = destination;

	// Read command
	REG_SPICNT = SPI_ENABLE | SPI_BYTE_MODE | SPI_CONTINUOUS | SPI_DEVICE_FIRMWARE;
	readwriteSPI(FIRMWARE_READ);

	// Set the address
	readwriteSPI((address>>16) & 0xFF);
	readwriteSPI((address>> 8) & 0xFF);
	readwriteSPI((address) & 0xFF);

	u32 i;

	// Read the data
	for(i=0;i<size;i++) {
		buffer[i] = readwriteSPI(0);
	}

	REG_SPICNT = 0;
	leaveCriticalSection(oldIME);
}
