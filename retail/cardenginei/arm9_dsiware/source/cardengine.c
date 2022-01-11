/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "hex.h"
#include "igm_text.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK5;

//static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;

s8 mainScreen = 0;

static void SetBrightness(u8 screen, s8 bright) {
	u8 mode = 1;

	if (bright < 0) {
		mode = 2;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(vu16*)(0x0400006C + (0x1000 * screen)) = bright | (mode << 14);
}

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
	if (!IPC_SYNC_hooked) {
		u32* ipcSyncHandler = ce9->irqTable + 16;
		ce9->intr_ipc_orig_return = *ipcSyncHandler;
		*ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
		IPC_SYNC_hooked = true;
	}
}

/*static void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}*/

//---------------------------------------------------------------------------------
void myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerVBlank");
	#endif	

//#ifndef TWLSDK
	if (sharedAddr[4] == 0x554E454D) {
		while (sharedAddr[4] != 0x54495845);
	}
//#endif
}

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG		
	nocashMessage("myIrqHandlerIPC");
	#endif	

	switch (IPC_GetSync()) {
		/*case 0x0:
			if(mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
			break;*/
		case 0x7: {
			mainScreen++;
			if(mainScreen > 2)
				mainScreen = 0;

			if(mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
		}
			break;
		case 0x9: {
			*(u32*)(INGAME_MENU_LOCATION_DSIWARE + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
			volatile void (*inGameMenu)(s8*) = (volatile void*)INGAME_MENU_LOCATION_DSIWARE + IGM_TEXT_SIZE_ALIGNED + 0x10;
			(*inGameMenu)(&mainScreen);
		}
			break;
	}

	if (sharedAddr[4] == 0x57534352) {
		enterCriticalSection();
		if (ce9->consoleModel < 2) {
			// Make screens white
			SetBrightness(0, 31);
			SetBrightness(1, 31);
		}
		cacheFlush();
		while (1);
	}
}

u32 myIrqEnable(u32 irq) {	
	int oldIME = enterCriticalSection();

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	hookIPC_SYNC();

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;		
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
}
