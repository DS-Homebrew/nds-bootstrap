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

#include <string.h> // memcpy
#include <stdio.h>
#include <nds/system.h>
#include <nds/debug.h>

//#include "my_fat.h"
#include "cardengine_header_arm7.h"
#include "cheat_engine.h"
#include "common.h"
#include "patch.h"
#include "find.h"
#include "hook.h"

// SDK 5
/*extern u32 setDataMobicliplist[3];
extern u32 setDataBWlist[7];
extern u32 setDataBWlist_1[3];
extern u32 setDataBWlist_2[3];
extern u32 setDataBWlist_3[3];
extern u32 setDataBWlist_4[3];*/

static const int MAX_HANDLER_LEN = 50;

static const u32 handlerStartSig[5] = {
	0xe92d4000, 	// push {lr}
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe28cce21,		// add  ip, ip, #0x210
	0xe51c1008,		// ldr	r1, [ip, #-8]
	0xe3510000		// cmp	r1, #0
};

static const u32 handlerEndSig[4] = {
	0xe59f1008, 	// ldr  r1, [pc, #8]	(IRQ Vector table address)
	0xe7910100,		// ldr  r0, [r1, r0, lsl #2]
	0xe59fe004,		// ldr  lr, [pc, #4]	(IRQ return address)
	0xe12fff10		// bx   r0
};

static u32* hookInterruptHandler(const u32* start, size_t size) {
	// Find the start of the handler
	u32* addr = findOffset(
		start, size,
		handlerStartSig, 5
	);
	if (!addr) {
		return NULL;
	}

	// Find the end of the handler
	addr = findOffset(
		addr, MAX_HANDLER_LEN*sizeof(u32),
		handlerEndSig, 4
	);
	if (!addr) {
		return NULL;
	}

	// Now find the IRQ vector table
	// Make addr point to the vector table address pointer within the IRQ handler
	addr += sizeof(handlerEndSig)/sizeof(handlerEndSig[0]);

	// Use relative and absolute addresses to find the location of the table in RAM
	u32 tableAddr = addr[0];
	u32 returnAddr = addr[1];
	u32* actualReturnAddr = addr + 2;
	u32* actualTableAddr = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);

	// The first entry in the table is for the Vblank handler, which is what we want
	return actualTableAddr;
	// 2     LCD V-Counter Match
}

int hookNdsRetailArm7(
	cardengineArm7* ce7,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 cheatFileCluster,
	u32 cheatSize,
    u32 gameOnFlashcard,
	u32 language,
	u32 dsiMode, // SDK 5
	u32 ROMinRAM,
	u32 consoleModel,
	u32 romread_LED,
	u32 gameSoftReset,
	u32 preciseVolumeControl
) {
	dbg_printf("hookNdsRetailArm7\n");

	u32* hookLocation = patchOffsetCache.a7IrqHandlerOffset;
	if (!hookLocation) {
		hookLocation = hookInterruptHandler((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
	}

	// SDK 5
	bool sdk5 = isSdk5(moduleParams);
	if (!hookLocation && sdk5) {
		switch (ndsHeader->arm7binarySize) {
			case 0x00022B40:
				hookLocation = (u32*)0x238DED8;
				break;

			case 0x00022BCC:
				hookLocation = (u32*)0x238DF60;
				break;

			case 0x00025664:
				hookLocation = (u32*)0x23A5330;		// DSi-Exclusive cart games
				break;

			case 0x000257DC:
				hookLocation = (u32*)0x23A54B8;		// DSi-Exclusive cart games
				break;

			case 0x00025860:
				hookLocation = (u32*)0x23A5538;		// DSi-Exclusive cart games
				break;

			case 0x00026DF4:
				hookLocation = (u32*)0x23A6AD4;		// DSi-Exclusive cart games
				break;

			case 0x00028F84:
				hookLocation = (u32*)0x2391918;
				break;

			case 0x0002909C:
				hookLocation = (u32*)0x2391A30;
				break;

			case 0x0002914C:
			case 0x00029164:
				hookLocation = (u32*)0x2391ADC;
				break;

			case 0x00029EE8:
				hookLocation = (u32*)0x2391F70;
				break;

			case 0x0002A2EC:
				hookLocation = (u32*)0x23921BC;
				break;

			case 0x0002A318:
				hookLocation = (u32*)0x23921D8;
				break;

			case 0x0002AF18:
				hookLocation = (u32*)0x239227C;
				break;

			case 0x0002C5B4:
				hookLocation = (u32*)0x2392E74;
				break;
		}
	}

	if (!hookLocation) {
		dbg_printf("ERR_HOOK\n");
		return ERR_HOOK;
	}
    
   	dbg_printf("hookLocation: ");
	dbg_hexa((u32)hookLocation);
	dbg_printf("\n\n");
	patchOffsetCache.a7IrqHandlerOffset = hookLocation;

	u32* vblankHandler = hookLocation;
	u32* timer0Handler = hookLocation + 3;
	u32* timer1Handler = hookLocation + 4;
	//u32* timer2Handler = hookLocation + 5;
	//u32* timer3Handler = hookLocation + 6;
	u32* ipcSyncHandler = hookLocation + 16;

	ce7->intr_vblank_orig_return = *vblankHandler;
	ce7->intr_timer0_orig_return = *timer0Handler;
	ce7->intr_timer1_orig_return = *timer1Handler;
	//ce7->intr_timer2_orig_return = *timer2Handler;
	//ce7->intr_timer3_orig_return = *timer3Handler;
	ce7->intr_fifo_orig_return   = *ipcSyncHandler;
	ce7->moduleParams            = moduleParams;
	ce7->fileCluster             = fileCluster;
	ce7->gameOnFlashcard         = gameOnFlashcard;
	ce7->language                = language;
	ce7->gottenSCFGExt           = REG_SCFG_EXT; // Pass unlocked SCFG before locking it
	ce7->dsiMode                 = dsiMode; // SDK 5
	ce7->ROMinRAM                = ROMinRAM;
	ce7->consoleModel            = consoleModel;
	ce7->romread_LED             = romread_LED;
	ce7->gameSoftReset           = gameSoftReset;
	ce7->preciseVolumeControl    = preciseVolumeControl;

	*vblankHandler = ce7->patches->vblankHandler;
	if (!ROMinRAM && !gameOnFlashcard) {
		*timer0Handler = ce7->patches->timer0Handler;
		*timer1Handler = ce7->patches->timer1Handler;
		//*timer2Handler = ce7->patches->timer2Handler;
		//*timer3Handler = ce7->patches->timer3Handler;
		*ipcSyncHandler = ce7->patches->fifoHandler;
	}

	aFile cheatFile = getFileFromCluster(cheatFileCluster);
	if (cheatFile.firstCluster != CLUSTER_FREE && cheatSize <= 0x8000) {
		fileRead(ce7->cheat_data_offset, cheatFile, 0, cheatSize, 0);
	}

	dbg_printf("ERR_NONE\n");
	return ERR_NONE;
}
