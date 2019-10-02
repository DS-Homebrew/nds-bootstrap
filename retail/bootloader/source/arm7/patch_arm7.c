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
	// swi 0x12 call
	u32* swi12Offset = findSwi12Offset(ndsHeader);
	if (swi12Offset) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = ce7->patches->swi02;
		memcpy(swi12Offset, swi12Patch, 0x4);
	}

	// swi get pitch table
	u32* swiGetPitchTableOffset = findSwiGetPitchTableOffset(ndsHeader, moduleParams);
	if (swiGetPitchTableOffset) {
		// Patch
		u32* swiGetPitchTablePatch = (isSdk5(moduleParams) ? ce7->patches->getPitchTableStub : ce7->patches->j_twlGetPitchTable);
		memcpy(swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
	}
}

static void patchSwiHalt(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	bool usesThumb = false;

	// swi halt
	u32* swiHaltOffset = findSwiHaltOffset(ndsHeader, moduleParams);
	if (!swiHaltOffset) {
		dbg_printf("Trying thumb...\n");
		swiHaltOffset = (u32*)findSwiHaltOffsetThumb(ndsHeader);
		if (swiHaltOffset) {
			usesThumb = true;
		}
	}
	if (swiHaltOffset) {
		// Patch
		u32* swiHaltPatch = (usesThumb ? ce7->patches->jThumb_newSwiHalt : ce7->patches->j_newSwiHalt); // SDK 5
		if (usesThumb) {
			/*
            // Find the relocation signature
            u32 relocationStart = getOffset((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
                relocateStartSignature, 1, 1);
            if (!relocationStart) {
                dbg_printf("Relocation start not found\n");
        		return 0;
            }
        
        	// Validate the relocation signature
            u32 vAddrOfRelocSrc = relocationStart + 0x8;
        
            dbg_hexa((u32)swiHaltOffset);
			const u16* patchSwiHalt = generateA7InstrThumb(swiHaltOffset - vAddrOfRelocSrc + 0x37F8000, ce7->patches->arm7FunctionsThumb->swiHalt);
			((u16*)swiHaltOffset)[0] = patchSwiHalt[0];
            ((u16*)swiHaltOffset)[1] = patchSwiHalt[1];*/
		} else {
			memcpy(swiHaltOffset, swiHaltPatch, 0xC);
		}
	}
}

static void patchSleep(const tNDSHeader* ndsHeader) {
	bool usesThumb = false;

	// Sleep
	u32* sleepPatchOffset = findSleepPatchOffset(ndsHeader);
	if (!sleepPatchOffset) {
		dbg_printf("Trying thumb...\n");
		sleepPatchOffset = (u32*)findSleepPatchOffsetThumb(ndsHeader);
		usesThumb = true;
	}
	if (sleepPatchOffset) {
		// Patch
		if (usesThumb) {
			*((u16*)sleepPatchOffset + 2) = 0;
			*((u16*)sleepPatchOffset + 3) = 0;
		} else {
			*(sleepPatchOffset + 2) = 0;
		}
	}
}

static bool patchCardIrqEnable(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card irq enable
	u32* cardIrqEnableOffset = findCardIrqEnableOffset(ndsHeader, moduleParams);
	if (!cardIrqEnableOffset) {
		return false;
	}
	u32* cardIrqEnablePatch = ce7->patches->card_irq_enable_arm7;
	memcpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x30);
	return true;
}

static void patchCardCheckPullOut(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card check pull out
	u32* cardCheckPullOutOffset = findCardCheckPullOutOffset(ndsHeader, moduleParams);
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
	/*if (REG_SCFG_ROM != 0x703) {
		fixForDsiBios(ce7, ndsHeader, moduleParams);
	}
	if (!ROMinRAM) {
		patchSwiHalt(ce7, ndsHeader, moduleParams);
	}*/

	if (forceSleepPatch) {
		patchSleep(ndsHeader);
	}

    const char* romTid = getRomTid(ndsHeader);

	/*if (!patchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
		return 0;
	}

	patchCardCheckPullOut(ce7, ndsHeader, moduleParams);*/

	u32 saveResult = 0;

	if (
        strncmp(romTid, "ATK", 3) == 0  // Kirby: Canvas Curse
    ) {
        saveResult = savePatchInvertedThumb(ce7, ndsHeader, moduleParams, saveFileCluster);    
	} else if (isSdk5(moduleParams)) {
		// SDK 5
		saveResult = savePatchV5(ce7, ndsHeader, moduleParams, saveFileCluster);
	} else {
		saveResult = savePatchV1(ce7, ndsHeader, moduleParams, saveFileCluster);
		if (!saveResult) {
			saveResult = savePatchV2(ce7, ndsHeader, moduleParams, saveFileCluster);
		}
		if (!saveResult) {
			saveResult = savePatchUniversal(ce7, ndsHeader, moduleParams, saveFileCluster);
		}
	}

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
