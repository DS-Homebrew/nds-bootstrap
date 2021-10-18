#include <nds/system.h>
#include <nds/bios.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
#include "value_bits.h"
#include "locations.h"
#include "tonccpy.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"

extern u32 _io_dldi_features;

extern u32 newArm7binarySize;
extern u32 arm7mbk;

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
	if ((_io_dldi_features & 0x00000010) || forceSleepPatch) {
		if (sleepPatchOffset) {
			// Patch
			*((u16*)sleepPatchOffset + 2) = 0;
			*((u16*)sleepPatchOffset + 3) = 0;
		}
	}
}

static void patchRamClear(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return;
	}

	u32* ramClearOffset = patchOffsetCache.ramClearOffset;
	if (!patchOffsetCache.ramClearOffset && !patchOffsetCache.ramClearChecked) {
		ramClearOffset = findRamClearOffset(ndsHeader);
		if (ramClearOffset) {
			patchOffsetCache.ramClearOffset = ramClearOffset;
		}
	}
	if (ramClearOffset) {
		*(ramClearOffset) = 0x023FC000;
		*(ramClearOffset + 1) = 0x023FC000;
		dbg_printf("RAM clear location : ");
		dbg_hexa((u32)ramClearOffset);
		dbg_printf("\n\n");
	}
	patchOffsetCache.ramClearChecked = true;
}

static void patchPostBoot(const tNDSHeader* ndsHeader) {
	if (ndsHeader->unitCode == 0 || arm7mbk != 0x080037C0) {
		return;
	}

	u32* postBootOffset = patchOffsetCache.postBootOffset;
	if (!patchOffsetCache.postBootOffset) {
		postBootOffset = findPostBootOffset(ndsHeader);
		if (postBootOffset) {
			patchOffsetCache.postBootOffset = postBootOffset;
		}
	}
	if (postBootOffset) {
		*postBootOffset = 0xE12FFF1E;	// bx lr
		dbg_printf("Post boot location : ");
		dbg_hexa((u32)postBootOffset);
		dbg_printf("\n\n");
	}
}

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
	bool usesThumb = (*(u16*)cardIrqEnableOffset == 0xB510);
	if (usesThumb) {
		u16* cardIrqEnablePatch = (u16*)ce7->patches->thumb_card_irq_enable_arm7;
		tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x20);
	} else {
		u32* cardIrqEnablePatch = ce7->patches->card_irq_enable_arm7;
		tonccpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x30);
	}

    dbg_printf("cardIrqEnable location : ");
    dbg_hexa((u32)cardIrqEnableOffset);
    dbg_printf("\n\n");
	return true;
}

/*static void patchCardCheckPullOut(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card check pull out
	u32* cardCheckPullOutOffset = findCardCheckPullOutOffset(ndsHeader, moduleParams);
	if (cardCheckPullOutOffset) {
		u32* cardCheckPullOutPatch = ce7->patches->card_pull_out_arm9;
		tonccpy(cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);
	}
}*/

extern void rsetA7Cache(void);

u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 saveFileCluster
) {
	newArm7binarySize = ndsHeader->arm7binarySize;

	if (ndsHeader->arm7binarySize == 0x22B40
	 || ndsHeader->arm7binarySize == 0x22BCC
	 || ndsHeader->arm7binarySize == 0x2352C
	 || ndsHeader->arm7binarySize == 0x235DC
	 || ndsHeader->arm7binarySize == 0x23708
	 || ndsHeader->arm7binarySize == 0x2378C
	 || ndsHeader->arm7binarySize == 0x237F0
	 || ndsHeader->arm7binarySize == 0x23CAC
	 || ndsHeader->arm7binarySize == 0x2434C
	 || ndsHeader->arm7binarySize == 0x245C4
	 || ndsHeader->arm7binarySize == 0x2484C
	 || ndsHeader->arm7binarySize == 0x249DC
	 || ndsHeader->arm7binarySize == 0x249E8
	 || ndsHeader->arm7binarySize == 0x24DA8
	 || ndsHeader->arm7binarySize == 0x24F50
	 || ndsHeader->arm7binarySize == 0x25D00
	 || ndsHeader->arm7binarySize == 0x25D04
	 || ndsHeader->arm7binarySize == 0x25D94
	 || ndsHeader->arm7binarySize == 0x25FFC
	 || arm7mbk == 0x080037C0) {
		// Replace incompatible ARM7 binary
		aFile donorRomFile;
		const char* romTid = getRomTid(ndsHeader);
		if (memcmp(romTid, "B7N", 3) == 0) { // Ben 10: Triple Pack
			// Donor ROM not needed, so read ARM7 binary from an SRL file within the ROM
			extern u32 storedFileCluster;
			donorRomFile = getFileFromCluster(storedFileCluster);

			u32 fatAddr = 0;
			u32 srlAddr = 0;
			u32 arm7src = 0;
			u32 arm7size = 0;
			fileRead((char*)&fatAddr, donorRomFile, 0x48, 0x4);
			fileRead((char*)&srlAddr, donorRomFile, fatAddr+0x80, 0x4);
			fileRead((char*)&arm7src, donorRomFile, srlAddr+0x30, 0x4);
			fileRead((char*)&arm7size, donorRomFile, srlAddr+0x3C, 0x4);
			fileRead(ndsHeader->arm7destination, donorRomFile, srlAddr+arm7src, arm7size);
			newArm7binarySize = arm7size;
		} else {
		if (ndsHeader->arm7binarySize == 0x2352C
		 || ndsHeader->arm7binarySize == 0x235DC
		 || ndsHeader->arm7binarySize == 0x23CAC) {
			extern u32 donorFileE2Cluster;	// Early SDK2
			donorRomFile = getFileFromCluster(donorFileE2Cluster);
		} else if (ndsHeader->arm7binarySize == 0x245C4
				 || ndsHeader->arm7binarySize == 0x24DA8
				 || ndsHeader->arm7binarySize == 0x24F50) {
			extern u32 donorFile2Cluster;	// SDK2
			donorRomFile = getFileFromCluster(donorFile2Cluster);
		} else if (ndsHeader->arm7binarySize == 0x2434C
				 || ndsHeader->arm7binarySize == 0x25D00
				 || ndsHeader->arm7binarySize == 0x25D04
				 || ndsHeader->arm7binarySize == 0x25D94
				 || ndsHeader->arm7binarySize == 0x25FFC) {
			extern u32 donorFile3Cluster;	// SDK3
			donorRomFile = getFileFromCluster(donorFile3Cluster);
		} else if (ndsHeader->arm7binarySize == 0x2484C
				 || ndsHeader->arm7binarySize == 0x249DC
				 || ndsHeader->arm7binarySize == 0x249E8) {
			extern u32 donorFile4Cluster;	// SDK4
			donorRomFile = getFileFromCluster(donorFile4Cluster);
		} else if (ndsHeader->arm7binarySize == 0x22B40
				 || ndsHeader->arm7binarySize == 0x22BCC
				 || arm7mbk == 0x080037C0) {
			extern u32 donorFileTwlCluster;	// SDK5 (TWL)
			donorRomFile = getFileFromCluster(donorFileTwlCluster);
		} else {
			extern u32 donorFileCluster;	// SDK5 (NTR)
			donorRomFile = getFileFromCluster(donorFileCluster);
		}
		if (donorRomFile.firstCluster == CLUSTER_FREE && ndsHeader->gameCode[0] != 'D') {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
		u32 arm7src = 0;
		u32 arm7size = 0;
		fileRead((char*)&arm7src, donorRomFile, 0x30, 0x4);
		fileRead((char*)&arm7size, donorRomFile, 0x3C, 0x4);
		fileRead(ndsHeader->arm7destination, donorRomFile, arm7src, arm7size);
		newArm7binarySize = arm7size;
		}
	}

	if (newArm7binarySize != patchOffsetCache.a7BinSize) {
		rsetA7Cache();
		patchOffsetCache.a7BinSize = newArm7binarySize;
		patchOffsetCacheChanged = true;
	}

	patchPostBoot(ndsHeader);

	patchSleepMode(ndsHeader);

	patchRamClear(ndsHeader, moduleParams);

	//const char* romTid = getRomTid(ndsHeader);

	if (!patchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
		return 0;
	}

	//patchCardCheckPullOut(ce7, ndsHeader, moduleParams);

	u32 saveResult = 0;

    /*if (
        strncmp(romTid, "ATK", 3) == 0  // Kirby: Canvas Curse
    ) {
        saveResult = savePatchInvertedThumb(ce7, ndsHeader, moduleParams, saveFileCluster);    
	} else*/ if (isSdk5(moduleParams)) {
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
	}

	/*if (REG_SCFG_ROM != 0x703) {
		fixForDsiBios(ce7, ndsHeader, moduleParams);
	}*/

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
