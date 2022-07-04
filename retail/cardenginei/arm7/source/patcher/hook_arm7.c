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
//#include "debug_file.h"
#include "nds_header.h"
#include "cardengine_header_arm7.h"
#include "common.h"
#include "patch.h"
#include "find.h"
#include "hook.h"

static const int MAX_HANDLER_LEN = 50;

static const u32 handlerStartSig[3] = {
	0xe92d4000, 	// push {lr}
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe28cce21		// add  ip, ip, #0x210
};

static const u32 handlerEndSig[1] = {
	0xe12fff10		// bx   r0
};

static u32* findIrqHandlerOffset(const u32* start, size_t size) {
	// Find the start of the handler
	u32* addr = findOffset(
		start, size,
		handlerStartSig, 3
	);
	if (!addr) {
		return NULL;
	}

	return addr;
}

static u32* findIrqHandlerWordsOffset(u32* handlerOffset, const u32* start, size_t size) {
	// Find the end of the handler
	u32* addr = findOffset(
		handlerOffset, MAX_HANDLER_LEN*sizeof(u32),
		handlerEndSig, 1
	);
	if (!addr) {
		return NULL;
	}

	return addr+1;
}

int hookNdsRetailArm7(
	cardengineArm7* ce7,
	const tNDSHeader* ndsHeader
) {
	//dbg_printf("hookNdsRetailArm7\n");

	u32* handlerLocation = findIrqHandlerOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);

	//const char* romTid = getRomTid(ndsHeader);

	if (!handlerLocation) {
	/*	if (strncmp(romTid, "YGX", 3) == 0) {
			ce7->valueBits |= b_powerCodeOnVBlank;
		} else {
			// Patch
			memcpy(handlerLocation, ce7->patches->j_irqHandler, 0xC);
		}
	} else {*/
		//dbg_printf("ERR_HOOK\n");
		return ERR_HOOK;
	}

	u32* wordsLocation = findIrqHandlerWordsOffset(handlerLocation, (u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);

	u32* hookLocation = NULL;
	// Now find the IRQ vector table
	if (wordsLocation[1] >= 0x037F0000 && wordsLocation[1] < 0x03800000) {
		// Use relative and absolute addresses to find the location of the table in RAM
		u32 tableAddr = wordsLocation[0];
		u32 returnAddr = wordsLocation[1];
		u32* actualReturnAddr = wordsLocation + 2;
		hookLocation = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);
	}

	/* *(ce7->irqTable_offset) = wordsLocation[0];
	if (wordsLocation[1] >= 0x037F0000 && wordsLocation[1] < 0x03800000) {
		*(ce7->irqTable_offset + 1) = wordsLocation[1];
	} else {
		*(ce7->irqTable_offset + 1) = wordsLocation[3];
	} */

   	/*dbg_printf("hookLocation arm7: ");
	dbg_hexa((u32)hookLocation);
	dbg_printf("\n\n");*/

	u32* vblankHandler = hookLocation;
	u32* ipcSyncHandler = hookLocation + 16;

	ce7->intr_vblank_orig_return  = *vblankHandler;
	ce7->intr_fifo_orig_return    = *ipcSyncHandler;

	*vblankHandler = ce7->patches->vblankHandler;
	*ipcSyncHandler = ce7->patches->fifoHandler;

	//dbg_printf("ERR_NONE\n");
	return ERR_NONE;
}
