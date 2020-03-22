#include <string.h> // memcpy
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
#include "locations.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"

//#define memcpy __builtin_memcpy

extern u32 gameOnFlashcard;
extern u32 saveOnFlashcard;
extern u32 forceSleepPatch;

u32 savePatchV1(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchV2(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchUniversal(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchInvertedThumb(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchV5(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster); // SDK 5

u32 generateA7Instr(int arg1, int arg2) {
	return (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

const u16* generateA7InstrThumb(int arg1, int arg2) {
	static u16 instrs[2];

	// 23 bit offset
	u32 offset = (u32)(arg2 - arg1 - 4);
	//dbg_printf("generateA7InstrThumb offset\n");
	//dbg_hexa(offset);
	
	// 1st instruction contains the upper 11 bit of the offset
	instrs[0] = ((offset >> 12) & 0x7FF) | 0xF000;

	// 2nd instruction contains the lower 11 bit of the offset
	instrs[1] = ((offset >> 1) & 0x7FF) | 0xF800;

	return instrs;
}

static void fixForDsiBios(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32* swi12Offset = patchOffsetCache.a7Swi12Offset;
	u32* swiGetPitchTableOffset = patchOffsetCache.swiGetPitchTableOffset;
	if (!patchOffsetCache.a7Swi12Offset) {
		swi12Offset = a7_findSwi12Offset(ndsHeader);
		if (swi12Offset) {
			patchOffsetCache.a7Swi12Offset = swi12Offset;
		}
	}
	if (!patchOffsetCache.swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findSwiGetPitchTableOffset(ndsHeader, moduleParams);
		if (swiGetPitchTableOffset) {
			patchOffsetCache.swiGetPitchTableOffset = swiGetPitchTableOffset;
		}
	}

	if (!(REG_SCFG_ROM & BIT(9))) {
		// swi 0x12 call
		if (swi12Offset) {
			// Patch to call swi 0x02 instead of 0x12
			u32* swi12Patch = ce7->patches->swi02;
			memcpy(swi12Offset, swi12Patch, 0x4);
		}

		// swi get pitch table
		if (swiGetPitchTableOffset) {
			// Patch
			u32* swiGetPitchTablePatch = (isSdk5(moduleParams) ? ce7->patches->getPitchTableStub : ce7->patches->j_twlGetPitchTable);
			memcpy(swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
		}
	}
}

static void patchSleepMode(const tNDSHeader* ndsHeader) {
	// Sleep
	u32* sleepPatchOffset = patchOffsetCache.sleepPatchOffset;
	if (!patchOffsetCache.sleepPatchOffset) {
		sleepPatchOffset = findSleepPatchOffset(ndsHeader);
		if (!sleepPatchOffset) {
			dbg_printf("Trying thumb...\n");
			sleepPatchOffset = (u32*)findSleepPatchOffsetThumb(ndsHeader);
		}
		patchOffsetCache.sleepPatchOffset = sleepPatchOffset;
	}
	if (REG_SCFG_EXT == 0 || forceSleepPatch || REG_SCFG_MC == 0x11) {
		if (sleepPatchOffset) {
			// Patch
			*((u16*)sleepPatchOffset + 2) = 0;
			*((u16*)sleepPatchOffset + 3) = 0;
		}
	}
}

/*static void patchRamClear(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	u32* ramClearOffset = findRamClearOffset(ndsHeader);
	
	if (ramClearOffset) {
		*(ramClearOffset + 1) = 0x02FFD000;
	}
}*/

static bool patchCardIrqEnable(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card irq enable
	u32* cardIrqEnableOffset = patchOffsetCache.a7CardIrqEnableOffset;
	if (!patchOffsetCache.a7CardIrqEnableOffset) {
		cardIrqEnableOffset = findCardIrqEnableOffset(ndsHeader, moduleParams);
		if (cardIrqEnableOffset) {
			patchOffsetCache.a7CardIrqEnableOffset = cardIrqEnableOffset;
		}
	}
	if (!cardIrqEnableOffset) {
		return false;
	}
	u32* cardIrqEnablePatch = ce7->patches->card_irq_enable_arm7;
	memcpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x30);
	return true;
}

static void patchCardCheckPullOut(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card check pull out
	u32* cardCheckPullOutOffset = patchOffsetCache.cardCheckPullOutOffset;
	if (!patchOffsetCache.cardCheckPullOutChecked) {
		cardCheckPullOutOffset = findCardCheckPullOutOffset(ndsHeader, moduleParams);
		if (cardCheckPullOutOffset) {
			patchOffsetCache.cardCheckPullOutOffset = cardCheckPullOutOffset;
		}
		patchOffsetCache.cardCheckPullOutChecked = true;
	}
	if (cardCheckPullOutOffset) {
		u32* cardCheckPullOutPatch = ce7->patches->card_pull_out_arm9;
		memcpy(cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);
	}
}

u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 saveFileCluster
) {
	fixForDsiBios(ce7, ndsHeader, moduleParams);

	patchSleepMode(ndsHeader);

	//patchRamClear(ndsHeader, moduleParams);

    const char* romTid = getRomTid(ndsHeader);

	if ((strncmp(romTid, "UOR", 3) == 0 && !saveOnFlashcard)
	|| (strncmp(romTid, "UXB", 3) == 0 && !saveOnFlashcard)
	|| ROMinRAM || !gameOnFlashcard) {
		if (!patchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
			return 0;
		}

		patchCardCheckPullOut(ce7, ndsHeader, moduleParams);
	}

	u32 saveResult = 0;
    
    if (
        strncmp(romTid, "ATK", 3) == 0  // Kirby: Canvas Curse
    ) {
        saveResult = savePatchInvertedThumb(ce7, ndsHeader, moduleParams, saveFileCluster);    
	} else if (isSdk5(moduleParams)) {
		// SDK 5
		saveResult = savePatchV5(ce7, ndsHeader, moduleParams, saveFileCluster);
	} else {
		if (patchOffsetCache.savePatchType == 0) {
			saveResult = savePatchV1(ce7, ndsHeader, moduleParams, saveFileCluster);
			if (!saveResult) {
				patchOffsetCache.savePatchType = 1;
			}
		}
		if (!saveResult && patchOffsetCache.savePatchType == 1) {
			saveResult = savePatchV2(ce7, ndsHeader, moduleParams, saveFileCluster);
			if (!saveResult) {
				patchOffsetCache.savePatchType = 2;
			}
		}
		if (!saveResult && patchOffsetCache.savePatchType == 2) {
			saveResult = savePatchUniversal(ce7, ndsHeader, moduleParams, saveFileCluster);
		}
	}
	if (!saveResult) {
		patchOffsetCache.savePatchType = 0;
	} else if (strncmp(romTid, "AMH", 3) == 0) {
		extern u32 dsiSD;
		aFile* savFile = (aFile*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT);
		fileRead((char*)0x02400000, *savFile, 0, 0x80000, 0);
	}

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
