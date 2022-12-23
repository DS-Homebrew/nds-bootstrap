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

extern u16 a9ScfgRom;

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

void setB(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEA000000;
}

void setBL(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

u32* getOffsetFromBL(u32* blOffset) {
	if (*blOffset < 0xEB000000 && *blOffset >= 0xEC000000) {
		return NULL;
	}
	u32 opCode = (*blOffset) - 0xEB000000;

	if (opCode >= 0x00800000 && opCode <= 0x00FFFFFF) {
		u32 offset = (u32)blOffset + 8;
		for (u32 i = opCode; i <= 0x00FFFFFF; i++) {
			offset -= 4;
		}

		return (u32*)offset;
	}
	return (u32*)((u32)blOffset + (opCode*4) + 8);
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

void setBLThumb(int arg1, int arg2) {
	u16 instrs[2];

	// 23 bit offset
	u32 offset = (u32)(arg2 - arg1 - 4);
	//dbg_printf("generateA7InstrThumb offset\n");
	//dbg_hexa(offset);

	// 1st instruction contains the upper 11 bit of the offset
	instrs[0] = ((offset >> 12) & 0x7FF) | 0xF000;

	// 2nd instruction contains the lower 11 bit of the offset
	instrs[1] = ((offset >> 1) & 0x7FF) | 0xF800;

	*(u16*)arg1 = instrs[0];
	*(u16*)(arg1 + 2) = instrs[1];
}

u16* getOffsetFromBLThumb(u16* blOffset) {
	s16 codeOffset = blOffset[1];

	return (u16*)((u32)blOffset + (codeOffset*2) + 4);
}

static bool patchWramClear(const tNDSHeader* ndsHeader) {
	if (ndsHeader->unitCode == 0) {
		u32* offset = patchOffsetCache.wramEndAddrOffset;
		if (!patchOffsetCache.wramEndAddrOffset) {
			offset = findWramEndAddrOffset(ndsHeader);
			if (offset) {
				patchOffsetCache.wramEndAddrOffset = offset;
			}
		}
		if (offset) {
			*offset = CARDENGINE_ARM7_LOCATION;
			dbg_printf("WRAM end addr location : ");
			dbg_hexa((u32)offset);
			dbg_printf("\n\n");
		} else {
			return false;
		}
	}
	if (newArm7binarySize == 0xCAB4) {
		return true;
	}
	u32* offset = patchOffsetCache.wramClearOffset;
	if (!patchOffsetCache.wramClearOffset) {
		offset = findWramClearOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.wramClearOffset = offset;
		}
	}
	if (offset) {
		bool notWithinSubroutine = (*(u16*)offset == 0x2008 || *((u16*)offset + 1) == 0xE3A0);
		bool usesThumb = (notWithinSubroutine ? (*(u16*)offset == 0x2008) : (*((u16*)offset + 1) != 0xE92D));
		if (notWithinSubroutine) {
			if (usesThumb) {
				*((u16*)offset + 20) = 0x2200;	// movs r2, #0
			} else {
				offset[13] = 0xE3A02000;	// mov r2, #0
			}
		} else {
			if (usesThumb) {
				if (ndsHeader->unitCode == 0) {
					*((u16*)offset + 21) = 0x2200;	// movs r2, #0
				} else {
					*((u16*)offset + 11) = 0x2100;	// movs r1, #0
					*((u16*)offset + 55) = 0x2100;	// movs r1, #0
				}
			} else {
				if (ndsHeader->unitCode == 0) {
					offset[*offset==0xE92D4038 ? 15 : (offset[1]==0xE24DD008 ? 18 : 17)] = 0xE3A02000;	// mov r2, #0
				} else if (offset[1]==0xE24DD008) {
					offset[10] = 0xE3A01000;	// mov r1, #0
					offset[60] = 0xE3A01000;	// mov r1, #0
				} else if (*offset==0xE92D43FE) {
					offset[12] = 0xE3A01000;	// mov r1, #0
					offset[61] = 0xE3A01000;	// mov r1, #0
				} else {
					offset[*offset==0xE92D40F8 ? 10 : 9]  = 0xE3A01000;	// mov r1, #0
					offset[*offset==0xE92D40F8 ? 44 : 43] = 0xE3A01000;	// mov r1, #0
				}
			}
		}
		dbg_printf("WRAM clear location : ");
		dbg_hexa((u32)offset);
		dbg_printf("\n\n");
	} else {
		return false;
	}
	return true;
}

u32 vAddrOfRelocSrc = 0;
u32 relocDestAtSharedMem = 0;
u32 newSwiGetPitchTableAddr = 0;

static u16 swi12Patch[2] =
{
	0xDF02, // SWI  0x02
	0x4770, // BX LR
};

static u16 swiGetPitchTablePatch[8] =
{
	0x4902, // LDR  R1, =0x46A
	0x1A40, // SUBS R0, R0, R1
	0xDF1B, // SWI  0x1B
	0x0400, // LSLS R0, R0, #0x10
	0x0C00, // LSRS R0, R0, #0x10
	0x4770, // BX LR
	0x046A,
	0
};

static void fixForDSiBios(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32* swi12Offset = patchOffsetCache.a7Swi12Offset;
	u32* swiGetPitchTableOffset = patchOffsetCache.swiGetPitchTableOffset;
	if (!patchOffsetCache.a7Swi12Offset) {
		swi12Offset = a7_findSwi12Offset(ndsHeader);
		if (swi12Offset) {
			patchOffsetCache.a7Swi12Offset = swi12Offset;
		}
	}
	if (!patchOffsetCache.swiGetPitchTableChecked) {
		if (patchOffsetCache.a7IsThumb && !isSdk5(moduleParams)) {
			swiGetPitchTableOffset = (u32*)findSwiGetPitchTableThumbOffset(ndsHeader);
		} else {
			swiGetPitchTableOffset = findSwiGetPitchTableOffset(ndsHeader, moduleParams);
		}
		if (swiGetPitchTableOffset) {
			patchOffsetCache.swiGetPitchTableOffset = swiGetPitchTableOffset;
		}
		patchOffsetCache.swiGetPitchTableChecked = true;
	}

	if (a9ScfgRom == 1 && REG_SCFG_ROM != 0x703) {
		// swi 0x12 call
		if (swi12Offset) {
			// Patch to call swi 0x02 instead of 0x12
			tonccpy(swi12Offset, swi12Patch, 0x4);
		}

		// swi get pitch table
		if (swiGetPitchTableOffset) {
			// Patch
			if (isSdk5(moduleParams)) {
				u16* offset = (u16*)swiGetPitchTableOffset;
				for (int i = 0; i < 6; i++) {
					offset[i] = 0x46C0; // nop
				}
			} else if (patchOffsetCache.a7IsThumb) {
				tonccpy((u16*)newSwiGetPitchTableAddr, swiGetPitchTablePatch, 0x10);
				u32 srcAddr = (u32)swiGetPitchTableOffset - vAddrOfRelocSrc + 0x37F8000;
				u32 dstAddr = (u32)newSwiGetPitchTableAddr - vAddrOfRelocSrc + 0x37F8000;
				const u16* swiGetPitchTableBranch = generateA7InstrThumb(srcAddr, dstAddr);
				tonccpy(swiGetPitchTableOffset, swiGetPitchTableBranch, 0x4);

				dbg_printf("swiGetPitchTable new location : ");
				dbg_hexa(newSwiGetPitchTableAddr);
				dbg_printf("\n\n");
			} else {
				tonccpy(swiGetPitchTableOffset, ce7->patches->j_twlGetPitchTable, 0xC);
			}
		}
	}

	dbg_printf("swi12 location : ");
	dbg_hexa((u32)swi12Offset);
	dbg_printf("\n\n");
	dbg_printf("swiGetPitchTable location : ");
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
	if ((_io_dldi_features & 0x00000010) || forceSleepPatch) {
		if (sleepPatchOffset) {
			// Patch
			*((u16*)sleepPatchOffset + 2) = 0;
			*((u16*)sleepPatchOffset + 3) = 0;

			dbg_printf("Sleep location : ");
			dbg_hexa((u32)sleepPatchOffset);
			dbg_printf("\n\n");
		}
	}
}

static void patchRamClear(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000 || ndsHeader->unitCode == 0) {
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
		*(ramClearOffset) = 0x02FFF000;
		*(ramClearOffset + 1) = 0x02FFF000;
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
		bool usesThumb = (*(u16*)postBootOffset == 0xB5F8);
		if (usesThumb) {
			*(u16*)postBootOffset = 0x4770;	// bx lr
		} else {
			*postBootOffset = 0xE12FFF1E;	// bx lr
		}
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

static void operaRamPatch(void) {
	// Opera RAM patch (ARM7)
	*(u32*)0x0238C7BC = 0xC400000;
	*(u32*)0x0238C7C0 = 0xC4000CE;

	//*(u32*)0x0238C950 = 0xC400000;
}

extern void rsetA7Cache(void);

u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 saveFileCluster
) {
	newArm7binarySize = ndsHeader->arm7binarySize;

	if ((ndsHeader->unitCode > 0) ? (arm7mbk == 0x080037C0) : (ndsHeader->arm7binarySize == 0x25F70)) {
		// Replace incompatible ARM7 binary
		extern u32 donorFileTwlCluster;	// SDK5 (TWL)
		aFile donorRomFile;
		getFileFromCluster(&donorRomFile, donorFileTwlCluster);
		if (donorRomFile.firstCluster == CLUSTER_FREE && ndsHeader->gameCode[0] != 'D') {
			dbg_printf("ERR_LOAD_OTHR\n\n");
			return ERR_LOAD_OTHR;
		}
		u32 arm7dst = 0;
		fileRead((char*)&arm7dst, &donorRomFile, 0x38, 0x4);
		if (arm7dst == 0x02380000) {
			// Donor found within a ROM file
			u32 arm7src = 0;
			u32 arm7size = 0;
			fileRead((char*)&arm7src, &donorRomFile, 0x30, 0x4);
			fileRead((char*)&arm7size, &donorRomFile, 0x3C, 0x4);
			fileRead(ndsHeader->arm7destination, &donorRomFile, arm7src, arm7size);
			newArm7binarySize = arm7size;
		} else {
			// Standalone donor found
			extern u32 donorFileTwlSize;
			fileRead(ndsHeader->arm7destination, &donorRomFile, 0, donorFileTwlSize);
			newArm7binarySize = donorFileTwlSize;
		}
	}

	if (newArm7binarySize != patchOffsetCache.a7BinSize) {
		rsetA7Cache();
		patchOffsetCache.a7BinSize = newArm7binarySize;
	}

	if (!patchWramClear(ndsHeader)) {
		dbg_printf("ERR_LOAD_OTHR");
		return ERR_LOAD_OTHR;
	}

	patchPostBoot(ndsHeader);

	patchSleepMode(ndsHeader);

	patchRamClear(ndsHeader, moduleParams);

	const char* romTid = getRomTid(ndsHeader);

	if (!patchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
		dbg_printf("ERR_LOAD_OTHR");
		return ERR_LOAD_OTHR;
	}

	//patchCardCheckPullOut(ce7, ndsHeader, moduleParams);

	const u32 cardId = getChipId(ndsHeader, moduleParams);
	u32* cardIdPatch = (u32*)ce7->patches->arm7Functions->cardId;
	u32* cardIdPatchThumb = (u32*)ce7->patches->arm7FunctionsThumb->cardId;
	cardIdPatch[2] = cardId;
	cardIdPatchThumb[1] = cardId;

	if (a7GetReloc(ndsHeader, moduleParams)) {
		u32 saveResult = 0;

		if (newArm7binarySize==0x2352C || newArm7binarySize==0x235DC || newArm7binarySize==0x23CAC || newArm7binarySize==0x245C4) {
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
		}
	}

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch();
	}

	fixForDSiBios(ce7, ndsHeader, moduleParams);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
