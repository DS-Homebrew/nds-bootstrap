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
#include <nds/arm7/i2c.h>
#include <nds/debug.h>

//#include "my_fat.h"
#include "unpatched_funcs.h"
#include "debug_file.h"
#include "nds_header.h"
#include "cardengine_header_arm7.h"
#include "cheat_engine.h"
#include "value_bits.h"
#include "common.h"
#include "patch.h"
#include "find.h"
#include "hook.h"
#include "tonccpy.h"

#define b_gameOnFlashcard BIT(0)
#define b_saveOnFlashcard BIT(1)
#define b_eSdk2 BIT(2)
#define b_ROMinRAM BIT(3)
#define b_dsiMode BIT(4)
#define b_dsiSD BIT(5)
#define b_preciseVolumeControl BIT(6)
#define b_powerCodeOnVBlank BIT(7)
#define b_runCardEngineCheck BIT(8)
#define b_igmAccessible BIT(9)
#define b_hiyaCfwFound BIT(10)
#define b_slowSoftReset BIT(11)
#define b_wideCheatUsed BIT(12)
#define b_isSdk5 BIT(13)
#define b_asyncCardRead BIT(14)
#define b_twlTouch BIT(15)
#define b_cloneboot BIT(16)
#define b_sleepMode BIT(17)
#define b_dsiBios BIT(18)
#define b_bootstrapOnFlashcard BIT(19)
#define b_ndmaDisabled BIT(20)
#define b_isDlp BIT(21)
#define b_i2cBricked BIT(30)
#define b_scfgLocked BIT(31)

extern u32 newArm7binarySize;
extern u32 newArm7ibinarySize;
bool igmAccessible = true;

static const int MAX_HANDLER_LEN = 50;

static const u32 handlerStartSig[3] = {
	0xe92d4000, 	// push {lr}
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe28cce21		// add  ip, ip, #0x210
};

static const u32 handlerEndSig[1] = {
	0xe12fff10		// bx   r0
};

static const u32 irqListEndTwlSig[1] = {0x12345678};

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

static u32* findIrqListOffset(const u32* start, size_t size) {
	// Find the start of the handler
	u32* addr = findOffset(
		start, size,
		irqListEndTwlSig, 1
	);
	if (!addr) {
		return NULL;
	}

	addr -= 1;
	while (*addr == 0 || *addr == 0xFFFFFFFF) {
		addr -= 1;
	}
	addr -= 0x7C/sizeof(u32);

	return addr;
}

int hookNdsRetailArm7(
	cardengineArm7* ce7,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 fileCluster,
    u32 patchOffsetCacheFileCluster,
	u32 srParamsFileCluster,
	u32 ramDumpCluster,
	u32 screenshotCluster,
	u32 wideCheatFileCluster,
	u32 wideCheatSize,
	u32 cheatFileCluster,
	u32 cheatSize,
	u32 apPatchFileCluster,
	u32 apPatchSize,
	u32 pageFileCluster,
	u32 manualCluster,
    u16 bootstrapOnFlashcard,
    u8 gameOnFlashcard,
    u8 saveOnFlashcard,
	s32 mainScreen,
	u8 language,
	u8 dsiMode, // SDK 5
	u8 dsiSD,
	u8 ROMinRAM,
	u8 consoleModel,
	u8 romRead_LED,
	u8 dmaRomRead_LED,
	bool ndmaDisabled,
	bool twlTouch,
	bool usesCloneboot,
	u32 romPartLocation
) {
	dbg_printf("hookNdsRetailArm7\n");

	bool ce7NotFound = (ce7 == NULL);

	/*if (newArm7binarySize < 0x1000) {
		return ERR_NONE;
	}*/

	u32* handlerLocation = patchOffsetCache.a7IrqHandlerOffset;
	if (!handlerLocation && !ce7NotFound) {
		handlerLocation = findIrqHandlerOffset((u32*)ndsHeader->arm7destination, newArm7binarySize);
		if (!handlerLocation && ndsHeader->unitCode == 0x03) {
			handlerLocation = findIrqHandlerOffset((u32*)__DSiHeader->arm7idestination, newArm7ibinarySize);
		}
		if (handlerLocation) {
			patchOffsetCache.a7IrqHandlerOffset = handlerLocation;
		}
	}

	const char* romTid = getRomTid(ndsHeader);

	if (!handlerLocation && !ce7NotFound) {
	/*	if (strncmp(romTid, "YGX", 3) == 0) {
			ce7->valueBits |= b_powerCodeOnVBlank;
		} else {
			// Patch
			memcpy(handlerLocation, ce7->patches->j_irqHandler, 0xC);
		}
	} else {*/
		dbg_printf("ERR_HOOK\n");
		return ERR_HOOK;
	}

	/*bool handlerPatched = false;
	if (!gameOnFlashcard && !ROMinRAM && handlerLocation && ce7->patches->fifoHandler) {
		tonccpy(handlerLocation, ce7->patches->j_irqHandler, 0xC);
		handlerPatched = true;
	}*/

	u32* wordsLocation = patchOffsetCache.a7IrqHandlerWordsOffset;
	if (!wordsLocation && !ce7NotFound) {
		wordsLocation = findIrqHandlerWordsOffset(handlerLocation, (u32*)ndsHeader->arm7destination, newArm7binarySize);
		if (wordsLocation) {
			patchOffsetCache.a7IrqHandlerWordsOffset = wordsLocation;
		}
	}

	u32* hookLocation = patchOffsetCache.a7IrqHookOffset;
	if (!hookLocation) {
		// Now find the IRQ vector table
		if (ndsHeader->unitCode > 0) {
			switch (newArm7binarySize) {	// SDK 5
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
					hookLocation = (u32*)0x23A5340;		// DSi-Exclusive/DSiWare games
					break;

				case 0x000257DC:
					hookLocation = (u32*)0x23A54B8;		// DSi-Exclusive/DSiWare games
					break;

				case 0x00025860:
					hookLocation = (u32*)0x23A5538;		// DSi-Exclusive/DSiWare games
					break;

				case 0x000268DC:
					hookLocation = (u32*)0x23A5FFC;		// DSi-Exclusive/DSiWare games
					break;

				case 0x00026DF4:
					hookLocation = (u32*)0x23A6AD4;		// DSi-Exclusive/DSiWare games
					break;

				case 0x00027FB4:
					hookLocation = (u32*)0x23A7664;		// DSi-Exclusive/DSiWare games
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
			if (!hookLocation && ndsHeader->unitCode == 3) {
				switch (newArm7ibinarySize) {
					case 0x6AFD4:
						hookLocation = (u32*)0x2F67360;
						break;
					case 0x6B038:
						hookLocation = (u32*)0x2F67348;
						break;
					case 0x6EAD0:
						hookLocation = (u32*)0x2F6772C;
						break;
					case 0x6EB54:
						hookLocation = (u32*)0x2F67734;
						break;
					case 0x7250C:
						hookLocation = (u32*)0x2EE5E10;
						break;
					case 0x7603C:
						hookLocation = (u32*)0x2EE61FC;
						break;
				}
			}
			if (!hookLocation) {
				hookLocation = findIrqListOffset((u32*)ndsHeader->arm7destination, newArm7binarySize);
			}
			if (!hookLocation && ndsHeader->unitCode == 3) {
				dbg_printf("ERR_HOOK\n");
				return ERR_HOOK;
			}
		} else if (wordsLocation[1] >= 0x037F0000 && wordsLocation[1] < 0x03800000) {
			// Use relative and absolute addresses to find the location of the table in RAM
			u32 tableAddr = wordsLocation[0];
			u32 returnAddr = wordsLocation[1];
			u32* actualReturnAddr = wordsLocation + 2;
			hookLocation = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);
		}
	}

	/*if (handlerPatched) {
		*(ce7->irqTable_offset) = wordsLocation[0];
		if (wordsLocation[1] >= 0x037F0000 && wordsLocation[1] < 0x03800000) {
			*(ce7->irqTable_offset + 1) = wordsLocation[1];
		} else {
			*(ce7->irqTable_offset + 1) = wordsLocation[3];
			*(ce7->irqTable_offset + 2) = wordsLocation[1];
			*(ce7->irqTable_offset + 3) = wordsLocation[2];
		}
	}*/

   	dbg_printf("hookLocation arm7: ");
	dbg_hexa((u32)hookLocation);
	dbg_printf("\n\n");
	patchOffsetCache.a7IrqHookOffset = hookLocation;

	u32* vblankHandler = hookLocation;
	u32* ipcSyncHandler = hookLocation + 16;

	extern u32 cheatEngineOffset;

	if (!ce7NotFound) {
	/*	u32 intr_vblank_orig_return = *(u32*)0x2FFC004;
		intr_vblank_orig_return += 0x2FFC008;

		*(u32*)intr_vblank_orig_return = *vblankHandler;
		*vblankHandler = 0x2FFC008;
	} else {*/
		extern u32 iUncompressedSize;
		extern u32 dataToPreloadAddr;
		extern u32 dataToPreloadSize;
		extern u32 dataToPreloadFrame;
		extern bool dataToPreloadFound(const tNDSHeader* ndsHeader);
		const bool laterSdk = ((moduleParams->sdk_version >= 0x2008000 && moduleParams->sdk_version != 0x2012774) || moduleParams->sdk_version == 0x20029A8);

		ce7->intr_vblank_orig_return  = *vblankHandler;
		ce7->intr_fifo_orig_return    = *ipcSyncHandler;
		ce7->cheatEngineAddr          = cheatEngineOffset;
		ce7->fileCluster              = fileCluster;
		ce7->patchOffsetCacheFileCluster = patchOffsetCacheFileCluster;
		ce7->srParamsCluster          = srParamsFileCluster;
		ce7->ramDumpCluster           = ramDumpCluster;
		ce7->screenshotCluster        = screenshotCluster;
		ce7->pageFileCluster          = pageFileCluster;
		ce7->manualCluster            = manualCluster;
		if (gameOnFlashcard) {
			ce7->valueBits |= b_gameOnFlashcard;
		}
		if (saveOnFlashcard) {
			ce7->valueBits |= b_saveOnFlashcard;
		}
		if (!laterSdk) {
			ce7->valueBits |= b_eSdk2;
		}
		if (ROMinRAM) {
			ce7->valueBits |= b_ROMinRAM;
		}
		if (dsiMode) {
			ce7->valueBits |= b_dsiMode; // SDK 5
		}
		if (dsiSD) {
			ce7->valueBits |= b_dsiSD;
		}
		if (consoleModel < 2 && preciseVolumeControl) {
			ce7->valueBits |= b_preciseVolumeControl;
		}
		if (hiyaCfwFound) {
			ce7->valueBits |= b_hiyaCfwFound;
		}
		if (strncmp(romTid, "UBR", 3) == 0 || iUncompressedSize > 0x26C000) {
			ce7->valueBits |= b_slowSoftReset;
		}
		if (igmAccessible) {
			ce7->valueBits |= b_igmAccessible;
		}
		if (isSdk5(moduleParams)) {
			ce7->valueBits |= b_isSdk5;
		}
		if (asyncCardRead) {
			ce7->valueBits |= b_asyncCardRead;
		}
		if (twlTouch) {
			ce7->valueBits |= b_twlTouch;
		}
		if (usesCloneboot) {
			ce7->valueBits |= b_cloneboot;
		}
		if (sleepMode) {
			ce7->valueBits |= b_sleepMode;
		}
		if (!(REG_SCFG_ROM & BIT(9))) {
			ce7->valueBits |= b_dsiBios;
		}
		if (bootstrapOnFlashcard) {
			ce7->valueBits |= b_bootstrapOnFlashcard;
		}
		if (ndmaDisabled) {
			ce7->valueBits |= b_ndmaDisabled;
		}
		if (strncmp(romTid, "HND", 3) == 0) {
			ce7->valueBits |= b_isDlp;
		}
		extern bool i2cBricked;
		if (i2cBricked) {
			ce7->valueBits |= b_i2cBricked;
		}
		if (REG_SCFG_EXT == 0) {
			ce7->valueBits |= b_scfgLocked;
		}
		ce7->mainScreen               = mainScreen;
		ce7->language                 = language;
		if (strcmp(romTid, "AKYP") == 0) { // Etrian Odyssey (EUR)
			ce7->languageAddr = (u32*)0x020DC5DC;
		}
		ce7->consoleModel             = consoleModel;
		ce7->romRead_LED              = romRead_LED;
		ce7->dmaRomRead_LED           = dmaRomRead_LED;
		ce7->scfgRomBak               = REG_SCFG_ROM;

		extern u32 getRomLocation(const tNDSHeader* ndsHeader, const bool isSdk5);
		u32 romLocation = getRomLocation(ndsHeader, (ce7->valueBits & b_isSdk5));
		ce7->romLocation = romLocation;

		u32 romOffset = 0;
		if (usesCloneboot) {
			romOffset = 0x4000;
		} else if (ndsHeader->arm9overlaySource == 0 || ndsHeader->arm9overlaySize == 0) {
			romOffset = (ndsHeader->arm7romOffset + ndsHeader->arm7binarySize);
		} else {
			romOffset = ndsHeader->arm9overlaySource;
		}
		ce7->romLocation -= romOffset;
		if (dataToPreloadFound(ndsHeader) && dataToPreloadFrame) {
			ce7->romPartLocation = romPartLocation;
			ce7->romPartSrc = dataToPreloadAddr;
			ce7->romPartSize = dataToPreloadSize;
			ce7->romPartFrame = dataToPreloadFrame;
		}

		// 0: ROM part start, 1: ROM part start in RAM, 2: ROM part end in RAM
		extern u32 romMapLines;
		extern u32 romMap[5][3];

		ce7->romMapLines = romMapLines;
		for (int i = 0; i < 5; i++) {
			for (int i2 = 0; i2 < 3; i2++) {
				ce7->romMap[i][i2] = romMap[i][i2];
			}
		}

		*vblankHandler = ce7->patches->vblankHandler;
		if (ce7->patches->fifoHandler) {
		*ipcSyncHandler = ce7->patches->fifoHandler;
		/*if ((strncmp(romTid, "UOR", 3) == 0)
		 || (strncmp(romTid, "UXB", 3) == 0)
		 || (strncmp(romTid, "USK", 3) == 0)
		|| (!gameOnFlashcard && !ROMinRAM)) {
			if (!ROMinRAM) {
				ce7->valueBits |= b_runCardEngineCheck;
			}
		}*/
		}

		/*extern bool setDmaPatched;

		if (!setDmaPatched
		// && strncmp(romTid, "ALK", 3) != 0
		 && strncmp(romTid, "VDE", 3) != 0) {
			ce7->valueBits |= b_ipcEveryFrame;
		}*/
	}

	extern u32 cheatSizeTotal;
	extern char cheatEngineBuffer[0x400];
	u16 cheatSizeLimit = (ce7NotFound ? 0x1C00 : 0x8000);
	if (!ce7NotFound) {
		if (cheatEngineOffset == CHEAT_ENGINE_DSIWARE_LOCATION) {
			cheatSizeLimit -= 0x1800;
		} else if (cheatEngineOffset == CHEAT_ENGINE_DSIWARE_LOCATION3) {
			cheatSizeLimit -= 0x1000;
		}
	}
	char* cheatDataOffset = (char*)cheatEngineOffset+0x3E8;
	/*if (ce7NotFound) {
		cheatEngineOffset = 0x2FFC000;
		cheatDataOffset = (char*)cheatEngineOffset+0x3E8;
		*(u32*)((u32)cheatDataOffset) = 0x22FFFCE4;
		cheatDataOffset += 4;
		*(u32*)((u32)cheatDataOffset) = language;
		cheatDataOffset += 4;

		unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

		if (unpatchedFuncs->compressed_static_end) {
			*(u32*)((u32)cheatDataOffset) = (u32)unpatchedFuncs->compressedFlagOffset;
			cheatDataOffset += 4;
			*(u32*)((u32)cheatDataOffset) = unpatchedFuncs->compressed_static_end;
			cheatDataOffset += 4;
			cheatSizeLimit -= 8;
		}
		if (unpatchedFuncs->ltd_compressed_static_end) {
			*(u32*)((u32)cheatDataOffset) = (u32)unpatchedFuncs->iCompressedFlagOffset;
			cheatDataOffset += 4;
			*(u32*)((u32)cheatDataOffset) = unpatchedFuncs->ltd_compressed_static_end;
			cheatDataOffset += 4;
			cheatSizeLimit -= 8;
		}
		*(cheatDataOffset + 3) = 0xCF;
	}
	if (!gameOnFlashcard && isDSiWare) {
		cheatSizeLimit -= 0x10;
	}*/

	if (cheatSizeTotal > 4 && cheatSizeTotal <= cheatSizeLimit) {
		aFile wideCheatFile;
		getFileFromCluster(&wideCheatFile, wideCheatFileCluster, gameOnFlashcard);
		aFile cheatFile;
		getFileFromCluster(&cheatFile, cheatFileCluster, gameOnFlashcard);
		aFile apPatchFile;
		getFileFromCluster(&apPatchFile, apPatchFileCluster, gameOnFlashcard);

		tonccpy((u8*)cheatEngineOffset, cheatEngineBuffer, 0x400);

		if (ndsHeader->unitCode < 3 && apPatchFile.firstCluster != CLUSTER_FREE && apPatchIsCheat) {
			fileRead(cheatDataOffset, &apPatchFile, 0, apPatchSize);
			cheatDataOffset += apPatchSize;
			*(cheatDataOffset + 3) = 0xCF;
			dbg_printf("AP-fix found and applied\n");
		}
		if (wideCheatFile.firstCluster != CLUSTER_FREE) {
			fileRead(cheatDataOffset, &wideCheatFile, 0, wideCheatSize);
			cheatDataOffset += wideCheatSize;
			*(cheatDataOffset + 3) = 0xCF;
			ce7->valueBits |= b_wideCheatUsed;
			dbg_printf("Wide cheat found and applied\n");
		}
		if (cheatFile.firstCluster != CLUSTER_FREE) {
			fileRead(cheatDataOffset, &cheatFile, 0, cheatSize);
			dbg_printf("Cheats found and applied\n");
		}
	}

	dbg_printf("ERR_NONE\n");
	return ERR_NONE;
}
