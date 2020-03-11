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
#include "debug_file.h"
#include "nds_header.h"
#include "cardengine_header_arm7.h"
#include "cheat_engine.h"
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
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 srParamsFileCluster,
	u32 ramDumpCluster,
	u32 wideCheatFileCluster,
	u32 wideCheatSize,
	u32 cheatFileCluster,
	u32 cheatSize,
    u32 gameOnFlashcard,
    u32 saveOnFlashcard,
	u32 language,
	u32 dsiMode, // SDK 5
	u32 dsiSD,
	u32 ROMinRAM,
	u32 consoleModel,
	u32 romRead_LED,
	u32 dmaRomRead_LED,
	u32 preciseVolumeControl
) {
	dbg_printf("hookNdsRetailArm7\n");

	if (ndsHeader->arm7binarySize < 0x1000) {
		return ERR_NONE;
	}

	u32* handlerLocation = patchOffsetCache.a7IrqHandlerOffset;
	if (!handlerLocation) {
		handlerLocation = findIrqHandlerOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		if (handlerLocation) {
			patchOffsetCache.a7IrqHandlerOffset = handlerLocation;
		}
	}

	if (!handlerLocation) {
		// Patch
		//memcpy(handlerLocation, ce7->patches->j_irqHandler, 0xC);
	//} else {
		dbg_printf("ERR_HOOK\n");
		return ERR_HOOK;
	}

	u32* wordsLocation = patchOffsetCache.a7IrqHandlerWordsOffset;
	if (!wordsLocation) {
		wordsLocation = findIrqHandlerWordsOffset(handlerLocation, (u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		if (wordsLocation) {
			patchOffsetCache.a7IrqHandlerWordsOffset = wordsLocation;
		}
	}

	u32* hookLocation = patchOffsetCache.a7IrqHookOffset;
	if (!hookLocation) {
		// Now find the IRQ vector table
		if (wordsLocation[1] >= 0x037F0000 && wordsLocation[1] < 0x03800000) {
			// Use relative and absolute addresses to find the location of the table in RAM
			u32 tableAddr = wordsLocation[0];
			u32 returnAddr = wordsLocation[1];
			u32* actualReturnAddr = wordsLocation + 2;
			hookLocation = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);
		} else switch (ndsHeader->arm7binarySize) {	// SDK 5
			case 0x0001D5A8:
				hookLocation = (u32*)0x239D280;		// DS WiFi Settings
				break;

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

			case 0x0002B184:
				hookLocation = (u32*)0x23924CC;
				break;

			case 0x0002B24C:
				hookLocation = (u32*)0x2392578;
				break;

			case 0x0002C5B4:
				hookLocation = (u32*)0x2392E74;
				break;
		}
	}

	*(ce7->extraIrqTable_offset - 2) = wordsLocation[0];
	if (wordsLocation[1] >= 0x037F0000 && wordsLocation[1] < 0x03800000) {
		*(ce7->extraIrqTable_offset - 1) = wordsLocation[1];
	} else {
		*(ce7->extraIrqTable_offset - 1) = wordsLocation[3];
	}

   	dbg_printf("hookLocation arm7: ");
	dbg_hexa((u32)hookLocation);
	dbg_printf("\n\n");
	patchOffsetCache.a7IrqHookOffset = hookLocation;

	u32* vblankHandler = hookLocation;
	u32* ipcSyncHandler = hookLocation + 16;
	//u32* networkHandler = hookLocation + 24;

	ce7->intr_vblank_orig_return  = *vblankHandler;
	ce7->intr_fifo_orig_return    = *ipcSyncHandler;
	//ce7->intr_network_orig_return = *networkHandler;
	ce7->moduleParams             = moduleParams;
	ce7->fileCluster              = fileCluster;
	ce7->srParamsCluster          = srParamsFileCluster;
	ce7->ramDumpCluster           = ramDumpCluster;
	ce7->gameOnFlashcard          = gameOnFlashcard;
	ce7->saveOnFlashcard          = saveOnFlashcard;
	ce7->language                 = language;
	ce7->dsiMode                  = dsiMode; // SDK 5
	ce7->dsiSD                    = dsiSD;
	ce7->ROMinRAM                 = ROMinRAM;
	ce7->consoleModel             = consoleModel;
	ce7->romRead_LED              = romRead_LED;
	ce7->dmaRomRead_LED           = dmaRomRead_LED;
	ce7->preciseVolumeControl     = preciseVolumeControl;

	const char* romTid = getRomTid(ndsHeader);
	*vblankHandler = ce7->patches->vblankHandler;
	*ipcSyncHandler = ce7->patches->fifoHandler;
	/*if ((strncmp(romTid, "UOR", 3) == 0 && !saveOnFlashcard)
	|| (strncmp(romTid, "UXB", 3) == 0 && !saveOnFlashcard)
	|| (!ROMinRAM && !gameOnFlashcard)) {
		//*networkHandler = ce7->patches->networkHandler;
	}*/

	aFile wideCheatFile = getFileFromCluster(wideCheatFileCluster);
	aFile cheatFile = getFileFromCluster(cheatFileCluster);
	if (wideCheatSize+cheatSize <= 0x8000) {
		char* cheatDataOffset = (char*)ce7->cheat_data_offset;
		if (wideCheatFile.firstCluster != CLUSTER_FREE) {
			fileRead(cheatDataOffset, wideCheatFile, 0, wideCheatSize, 0);
			cheatDataOffset += wideCheatSize;
			*(cheatDataOffset + 3) = 0xCF;
		}
		if (cheatFile.firstCluster != CLUSTER_FREE) {
			fileRead(cheatDataOffset, cheatFile, 0, cheatSize, 0);
		}
	}

	dbg_printf("ERR_NONE\n");
	return ERR_NONE;
}
