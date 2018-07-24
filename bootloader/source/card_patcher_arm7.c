#include <nds/system.h>
#include "card_patcher.h"
#include "card_finder.h"
#include "debugToFile.h"

extern u32 ROMinRAM;

u32 savePatchV1(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize);
u32 savePatchV2(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize);
u32 savePatchUniversal(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize);

u32 generateA7Instr(int arg1, int arg2) {
	return (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

u16* generateA7InstrThumb(int arg1, int arg2) {
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

void fixForDsiBios(const tNDSHeader* ndsHeader, u32* cardEngineLocation) {
	u32* patches = (u32*)cardEngineLocation[0];

	// swi 0x12 call
	u32* swi12Offset = findSwi12Offset(ndsHeader);
	if (swi12Offset) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = (u32*)patches[10];
		copyLoop(swi12Offset, swi12Patch, 0x4);
	}

	//sdk5 = false;

	// swi get pitch table
	u32* swiGetPitchTableOffset = findSwiGetPitchTableOffset(ndsHeader);
	if (swiGetPitchTableOffset) {
		// Patch
		//u32* swiGetPitchTablePatch = (u32*)patches[sdk5 ? 13 : 12];
		u32* swiGetPitchTablePatch = (u32*)patches[12];
		copyLoop(swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
	}
}

void patchSwiHalt(const tNDSHeader* ndsHeader, u32* cardEngineLocation) {
	// swi halt
	u32* swiHaltOffset = findSwiHaltOffset(ndsHeader);
	if (swiHaltOffset) {
		// Patch
		u32* patches = (u32*)cardEngineLocation[0];
		u32* swiHaltPatch = (u32*)patches[11];
		copyLoop(swiHaltOffset, swiHaltPatch, 0xC);
	}
}

u32 patchCardNdsArm7(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {
	u32* debug = (u32*)0x037C6000;

	if (REG_SCFG_ROM != 0x703) {
		fixForDsiBios(ndsHeader, cardEngineLocation);
	}
	if (ROMinRAM == false) {
		patchSwiHalt(ndsHeader, cardEngineLocation);
	}
	
	bool usesThumb = false;

	// Sleep patch
	u32* sleepPatchOffset = findSleepPatchOffset(ndsHeader);
	if (!sleepPatchOffset) {
		sleepPatchOffset = (u32*)findSleepPatchOffsetThumb(ndsHeader);
		usesThumb = true;
	}
	if (sleepPatchOffset) {
		// Patch
		if (usesThumb) {
			*(u16*)((u32)sleepPatchOffset + 4) = 0;
			*(u16*)((u32)sleepPatchOffset + 6) = 0;
		} else {
			*(u32*)((u32)sleepPatchOffset + 8) = 0;
		}
	}

	// Card check pull out
	u32* cardCheckPullOutOffset = findCardCheckPullOutOffset(ndsHeader, moduleParams);
	if (cardCheckPullOutOffset) {
		debug[0] = (u32)cardCheckPullOutOffset;
	}

	// Card irq enable
	u32* cardIrqEnableOffset = findCardIrqEnableOffset(ndsHeader, moduleParams);
	if (!cardIrqEnableOffset) {
		return 0;
	}
	debug[0] = (u32)cardIrqEnableOffset;

	cardEngineLocation[3] = moduleParams->sdk_version;

	u32* patches = (u32*)cardEngineLocation[0];

	u32* cardIrqEnablePatch    = (u32*)patches[2];
	u32* cardCheckPullOutPatch = (u32*)patches[1];

	if (cardCheckPullOutOffset) {
		copyLoop(cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);
	}

	copyLoop(cardIrqEnableOffset, cardIrqEnablePatch, 0x30);

	u32 saveResult = savePatchV1(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	if (!saveResult) {
		saveResult = savePatchV2(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	}
	if (!saveResult) {
		saveResult = savePatchUniversal(ndsHeader, cardEngineLocation, moduleParams, saveFileCluster, saveSize);
	}
	if (saveResult == 1 && ROMinRAM == false && saveSize > 0 && saveSize <= 0x00100000) {
		aFile saveFile = getFileFromCluster(saveFileCluster);
		fileRead((char*)0x0C820000, saveFile, 0, saveSize, 3);
	}

	dbg_printf("ERR_NONE");
	return ERR_NONE;
}
