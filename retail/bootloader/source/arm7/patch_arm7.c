#include <nds/system.h>
#include <nds/bios.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
#include "locations.h"
#include "tonccpy.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"

extern u32 gameOnFlashcard;
extern u32 saveOnFlashcard;
extern u32 dsiSD;
extern u32 forceSleepPatch;

u32 savePatchV1(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchV2(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchUniversal(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchInvertedThumb(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster);
u32 savePatchV5(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, u32 saveFileCluster); // SDK 5

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
	bool useGetPitchTableBranch = (patchOffsetCache.a7IsThumb && !isSdk5(moduleParams));
	u32* swiGetPitchTableOffset = patchOffsetCache.swiGetPitchTableOffset;
	if (!patchOffsetCache.a7Swi12Offset) {
		swi12Offset = a7_findSwi12Offset(ndsHeader);
		if (swi12Offset) {
			patchOffsetCache.a7Swi12Offset = swi12Offset;
		}
	}
	if (!patchOffsetCache.swiGetPitchTableChecked) {
		if (useGetPitchTableBranch) {
			swiGetPitchTableOffset = (u32*)findSwiGetPitchTableThumbBranchOffset(ndsHeader);
		} else {
			swiGetPitchTableOffset = findSwiGetPitchTableOffset(ndsHeader, moduleParams);
		}
		if (swiGetPitchTableOffset) {
			patchOffsetCache.swiGetPitchTableOffset = swiGetPitchTableOffset;
		}
		patchOffsetCache.swiGetPitchTableChecked = true;
	}

	if (!(REG_SCFG_ROM & BIT(9))) {
		// swi 0x12 call
		if (swi12Offset) {
			// Patch to call swi 0x02 instead of 0x12
			u32* swi12Patch = ce7->patches->swi02;
			tonccpy(swi12Offset, swi12Patch, 0x4);
		}

		// swi get pitch table
		if (swiGetPitchTableOffset) {
			// Patch
			if (useGetPitchTableBranch) {
				tonccpy(swiGetPitchTableOffset, ce7->patches->j_twlGetPitchTableThumb, 0x40);
			} else if (!patchOffsetCache.a7IsThumb) {
				u32* swiGetPitchTablePatch = (isSdk5(moduleParams) ? ce7->patches->getPitchTableStub : ce7->patches->j_twlGetPitchTable);
				tonccpy(swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
			}
		}
	}

    dbg_printf("swi12 location : ");
    dbg_hexa((u32)swi12Offset);
    dbg_printf("\n\n");
    dbg_printf(useGetPitchTableBranch ? "swiGetPitchTableBranch location : " : "swiGetPitchTable location : ");
    dbg_hexa((u32)swiGetPitchTableOffset);
    dbg_printf("\n\n");
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
	tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x30);
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
		tonccpy(cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);
	}
}

extern void rsetA7Cache(void);

u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 saveFileCluster
) {
	if ((ndsHeader->arm7binarySize == 0x23708 && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x2378C && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x237F0 && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x23CAC && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x2434C && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x2484C && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x249DC && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x249E8 && !dsiSD)
	 || ndsHeader->arm7binarySize == 0x24DA8
	 || ndsHeader->arm7binarySize == 0x24F50
	 || (ndsHeader->arm7binarySize == 0x25D04 && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x25D94 && !dsiSD)
	 || (ndsHeader->arm7binarySize == 0x25FFC && !dsiSD)
	 || ndsHeader->arm7binarySize == 0x27618
	 || ndsHeader->arm7binarySize == 0x2762C
	 || ndsHeader->arm7binarySize == 0x29CEC) {
		// Replace incompatible ARM7 binary
		aFile donorRomFile;
		if (ndsHeader->arm7binarySize == 0x23CAC) {
			extern u32 donorFileE2Cluster;	// Early SDK2
			donorRomFile = getFileFromCluster(donorFileE2Cluster);
		} else if (ndsHeader->arm7binarySize == 0x24DA8
				 || ndsHeader->arm7binarySize == 0x24F50) {
			extern u32 donorFile2Cluster;	// SDK2
			donorRomFile = getFileFromCluster(donorFile2Cluster);
		} else if (ndsHeader->arm7binarySize == 0x2434C
				 || ndsHeader->arm7binarySize == 0x2484C
				 || ndsHeader->arm7binarySize == 0x249DC
				 || ndsHeader->arm7binarySize == 0x249E8
				 || ndsHeader->arm7binarySize == 0x25D04
				 || ndsHeader->arm7binarySize == 0x25D94
				 || ndsHeader->arm7binarySize == 0x25FFC) {
			extern u32 donorFile3Cluster;	// SDK3-4
			donorRomFile = getFileFromCluster(donorFile3Cluster);
		} else {
			extern u32 donorFileCluster;	// SDK5
			donorRomFile = getFileFromCluster(donorFileCluster);
		}
		if (donorRomFile.firstCluster == CLUSTER_FREE) {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
		u32 arm7src = 0;
		u32 arm7size = 0;
		fileRead((char*)&arm7src, donorRomFile, 0x30, 0x4, -1);
		fileRead((char*)&arm7size, donorRomFile, 0x3C, 0x4, -1);
		fileRead(ndsHeader->arm7destination, donorRomFile, arm7src, arm7size, -1);
		ndsHeader->arm7binarySize = arm7size;
		ndsHeader->headerCRC16 = swiCRC16(0xFFFF, ndsHeader, 0x15E);	// Fix CRC
	}

	if (ndsHeader->arm7binarySize != patchOffsetCache.a7BinSize) {
		rsetA7Cache();
		patchOffsetCache.a7BinSize = ndsHeader->arm7binarySize;
		patchOffsetCacheChanged = true;
	}

	patchSleepMode(ndsHeader);

	//patchRamClear(ndsHeader, moduleParams);

	// Touch fix for SM64DS (U) v1.0
	if (ndsHeader->arm7binarySize == 0x24B64
	 && *(u32*)0x023825E4 == 0xE92D4030
	 && *(u32*)0x023825E8 == 0xE24DD004) {
		tonccpy((char*)0x023825E4, (char*)ARM7_FIX_BUFFERED_LOCATION, 0x140);
	}
	toncset((char*)ARM7_FIX_BUFFERED_LOCATION, 0, 0x140);

    const char* romTid = getRomTid(ndsHeader);

	if ((strncmp(romTid, "UOR", 3) == 0 && !saveOnFlashcard)
	|| (strncmp(romTid, "UXB", 3) == 0 && !saveOnFlashcard)
	|| (!ROMinRAM && !gameOnFlashcard)) {
		if (!patchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
			return 0;
		}

		patchCardCheckPullOut(ce7, ndsHeader, moduleParams);
	}

	u32 saveResult = 0;
    
    if (
        (strncmp(romTid, "ATK", 3) == 0) && dsiSD  // Kirby: Canvas Curse
    ) {
        saveResult = savePatchInvertedThumb(ce7, ndsHeader, moduleParams, saveFileCluster);    
	} else if (isSdk5(moduleParams)) {
		// SDK 5
		saveResult = savePatchV5(ce7, ndsHeader, saveFileCluster);
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
	} /*else if (strncmp(romTid, "AMH", 3) == 0) {
		extern u32 dsiSD;
		aFile* savFile = (aFile*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT);
		fileRead((char*)0x02440000, *savFile, 0, 0x40000, 0);
	}*/

	fixForDsiBios(ce7, ndsHeader, moduleParams);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
