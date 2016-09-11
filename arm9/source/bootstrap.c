/*-----------------------------------------------------------------

 Copyright (C) 2010  Dave "WinterMute" Murphy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include <nds.h>
#include <fat.h>

#include <stdio.h>

// #include <nds/fifocommon.h>

#include "nds_loader_arm9.h"

// Disabled by default
/*
void WaitForCart() {

	unsigned int * SCFG_MC=(unsigned int*)0x4004010;
	
	// Hold in loop until cartridge detected. (this is skipped if there's one already inserted at boot)
	printf("No cartridge detected!\nPlease insert a cartridge to\ncontinue.");
	do {
		swiWaitForVBlank();
	} while (*SCFG_MC == 0x11);
}

// Waits for a preset amount of time then waits for arm7 to send fifo for FIFO_USER_01
// This means it has powered off slot and has continued the card reset.
// This ensures arm9 doesn't attempt to init card too soon when it's not ready.
void WaitForSlot() {
	
	// Tell Arm7 it's ready to start card reset.
	fifoSendValue32(FIFO_USER_02, 1);
	
	// Waits for arm7 to power off slot before continuing
	fifoWaitValue32(FIFO_USER_01);		
	// Wait for half a second to make sure Power On sequence on arm7 is complete.
	for (int i = 0; i < 30; i++) {
		swiWaitForVBlank();
	}
}
*/

int main( int argc, char **argv) {
	
	//unsigned int * SCFG_MC=(unsigned int*)0x4004010;
	
	consoleDemoInit();
	
	// Cart Init Stuff. Enable if cart reset needed.
	// if(*SCFG_MC == 0x11) { WaitForCart(); }
	
	// WaitForSlot();
	
	if (fatInitDefault()) {
		runNdsFile("/Boot.nds", 0, NULL);
	} else {
		printf("SD init failed!\nLauncher not patched!");
	}
	while(1) swiWaitForVBlank();
	
	// Alternate exit loop. Stops program if no cart inserted. Enable if making a Stage3 BootStrap for a flashcart.
	// Can also be used for other purposes requiring cart reset. Disable original exit loop before using this.
	/*
	if (*SCFG_MC == 0x11) {
		printf("Cartridge Not Inserted!\nReboot and try again!");
		} else {
			if (fatInitDefault()) {
				runNdsFile("Boot.nds", 0, NULL);
				} else {
					printf("SD init failed!\nLauncher not patched!");
				}
			}
	
	while(1) swiWaitForVBlank();
	*/
}

