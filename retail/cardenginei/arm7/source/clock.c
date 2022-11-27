/*---------------------------------------------------------------------------------

	Copyright (C) 2005
		Michael Noland (Joat)
		Jason Rogers (Dovoto)
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

#include <nds/arm7/clock.h>
#include <nds/ndstypes.h>



// Delay (in swiDelay units) for each bit transfer
#define RTC_DELAY 48

// Pin defines on RTC_CR
#define CS_0    (1<<6)
#define CS_1    ((1<<6) | (1<<2))
#define SCK_0   (1<<5)
#define SCK_1   ((1<<5) | (1<<1))
#define SIO_0   (1<<4)
#define SIO_1   ((1<<4) | (1<<0))
#define SIO_out (1<<4)
#define SIO_in  (1)

//---------------------------------------------------------------------------------
void BCDToInteger(uint8 * data, uint32 length) {
//---------------------------------------------------------------------------------
	u32 i;
	for (i = 0; i < length; i++) {
		data[i] = (data[i] & 0xF) + ((data[i] & 0xF0)>>4)*10;
	}
}

//---------------------------------------------------------------------------------
void rtcTransaction(uint8 * command, uint32 commandLength, uint8 * result, uint32 resultLength) {
//---------------------------------------------------------------------------------
	uint32 bit;
	uint8 data;

	// Raise CS
	RTC_CR8 = CS_0 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);
	RTC_CR8 = CS_1 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);

	// Write command byte (high bit first)
		data = *command++;

		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data>>7);
			swiDelay(RTC_DELAY);

			RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data>>7);
			swiDelay(RTC_DELAY);

			data = data << 1;
		}
	// Write parameter bytes (low bit first)
	for ( ; commandLength > 1; commandLength--) {
		data = *command++;

		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data & 1);
			swiDelay(RTC_DELAY);

			RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data & 1);
			swiDelay(RTC_DELAY);

			data = data >> 1;
		}
	}

	// Read result bytes (low bit first)
	for ( ; resultLength > 0; resultLength--) {
		data = 0;

		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0;
			swiDelay(RTC_DELAY);

			RTC_CR8 = CS_1 | SCK_1;
			swiDelay(RTC_DELAY);

			if (RTC_CR8 & SIO_in) data |= (1 << bit);
		}
		*result++ = data;
	}

	// Finish up by dropping CS low
	RTC_CR8 = CS_0 | SCK_1;
	swiDelay(RTC_DELAY);
}


//---------------------------------------------------------------------------------
void rtcGetTimeAndDate(uint8 * time) {
//---------------------------------------------------------------------------------
	uint8 command, status;

	command = READ_TIME_AND_DATE;
	rtcTransaction(&command, 1, time, 7);

	command = READ_STATUS_REG1;
	rtcTransaction(&command, 1, &status, 1);

	if ( status & STATUS_24HRS ) {
		time[4] &= 0x3f;
	} else {

	}
	BCDToInteger(time,7);
}
