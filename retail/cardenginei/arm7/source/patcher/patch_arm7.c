#include <nds/system.h>
#include <nds/bios.h>
#include "nds_header.h"
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "common.h"
//#include "value_bits.h"
#include "locations.h"
#include "aeabi.h"
#include "cardengine_header_arm7.h"
//#include "debug_file.h"

#define gameOnFlashcard BIT(0)
#define ROMinRAM BIT(3)
#define dsiMode BIT(4)

extern u32 valueBits;
extern u16 scfgRomBak;
//extern u8 dsiSD;

extern bool sdRead;

u32 savePatchV1(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32 savePatchV2(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32 savePatchUniversal(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32 savePatchInvertedThumb(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32 savePatchV5(const cardengineArm7* ce7, const tNDSHeader* ndsHeader); // SDK 5

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

u16* getOffsetFromBLThumb(u16* blOffset) {
	s16 codeOffset = blOffset[1];

	return (u16*)((u32)blOffset + (codeOffset*2) + 4);
}

u32 vAddrOfRelocSrc = 0;
u32 relocDestAtSharedMem = 0;
bool a7IsThumb = false;

static void fixForDifferentBios(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (scfgRomBak & BIT(9)) {
		return;
	}

	u32* swi12Offset = a7_findSwi12Offset(ndsHeader);
	bool useGetPitchTableBranch = a7IsThumb;
	u32* swiGetPitchTableOffset = NULL;
	if (useGetPitchTableBranch) {
		swiGetPitchTableOffset = (u32*)findSwiGetPitchTableThumbBranchOffset(ndsHeader);
	} else {
		swiGetPitchTableOffset = findSwiGetPitchTableOffset(ndsHeader, moduleParams);
	}

	// swi 0x12 call
	if (swi12Offset) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = ce7->patches->swi02;
		__aeabi_memcpy(swi12Offset, swi12Patch, 0x4);
	}

	// swi get pitch table
	if (swiGetPitchTableOffset) {
		// Patch
		if (useGetPitchTableBranch) {
			__aeabi_memcpy(swiGetPitchTableOffset, ce7->patches->j_twlGetPitchTableThumb, 0x40);
		} else if (isSdk5(moduleParams)) {
			__aeabi_memset(swiGetPitchTableOffset, 6*sizeof(u16), 0x46C046C0);
		} else if (!a7IsThumb) {
			u32* swiGetPitchTablePatch = ce7->patches->j_twlGetPitchTable;
			__aeabi_memcpy(swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
		}
	}

	/*dbg_printf("swi12 location : ");
	dbg_hexa((u32)swi12Offset);
	dbg_printf("\n\n");
	dbg_printf(useGetPitchTableBranch ? "swiGetPitchTableBranch location : " : "swiGetPitchTable location : ");
	dbg_hexa((u32)swiGetPitchTableOffset);
	dbg_printf("\n\n");*/
}

static void patchMirrorCheck(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version > 0x5000000) {
		return;
	}

	extern u32 relocationStart;
	u32* offset = (u32*)relocationStart;
	offset--;
	offset--;
	offset--;
	offset--;
	for (int i = 0; i < 0xA0/sizeof(u32); i++) {
		if (offset[-1] == 0xE12FFF1E) {
			break;
		}
		offset--;
	}

	offset[0] = (valueBits & dsiMode) ? 0xE3A01002 : 0xE3A01001; // mov r1, #dsiModeConfirmed ? 2 : 1
	offset[1] = 0xE59F2004; // ldr r2, =0x027FFFFA
	offset[2] = 0xE1C210B0; // strh r1, [r2]
	offset[3] = 0xE12FFF1E; // bx lr
	offset[4] = 0x027FFFFA;

	/* dbg_printf("RAM mirror check location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n"); */
}

static void patchSleepMode(const tNDSHeader* ndsHeader) {
	// Sleep
	u32* sleepPatchOffset = findSleepPatchOffset(ndsHeader);
	if (!sleepPatchOffset) {
		//dbg_printf("Trying thumb...\n");
		sleepPatchOffset = (u32*)findSleepPatchOffsetThumb(ndsHeader);
	}
	/*if (REG_SCFG_EXT == 0 || (REG_SCFG_MC & BIT(0)) || (!(REG_SCFG_MC & BIT(2)) && !(REG_SCFG_MC & BIT(3)))
	 || forceSleepPatch) {*/
		if (sleepPatchOffset) {
			// Patch
			*((u16*)sleepPatchOffset + 2) = 0;
			*((u16*)sleepPatchOffset + 3) = 0;
		}
	//}
}

bool a7PatchCardIrqEnable(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card irq enable
	u32* cardIrqEnableOffset = findCardIrqEnableOffset(ndsHeader, moduleParams);
	if (!cardIrqEnableOffset) {
		return false;
	}
	bool usesThumb = (*(u16*)cardIrqEnableOffset == 0xB510);
	if (usesThumb) {
		u16* cardIrqEnablePatch = (u16*)ce7->patches->thumb_card_irq_enable_arm7;
		__aeabi_memcpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x20);
	} else {
		u32* cardIrqEnablePatch = ce7->patches->card_irq_enable_arm7;
		__aeabi_memcpy(cardIrqEnableOffset, cardIrqEnablePatch, 0x30);
	}

    /*dbg_printf("cardIrqEnable location : ");
    dbg_hexa((u32)cardIrqEnableOffset);
    dbg_printf("\n\n");*/
	return true;
}

static void patchCardCheckPullOut(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	// Card check pull out
	u32* cardCheckPullOutOffset = findCardCheckPullOutOffset(ndsHeader, moduleParams);
	if (cardCheckPullOutOffset) {
		u32* cardCheckPullOutPatch = ce7->patches->card_pull_out_arm9;
		__aeabi_memcpy(cardCheckPullOutOffset, cardCheckPullOutPatch, 0x4);
	}
}

u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams
) {
	patchSleepMode(ndsHeader);

	if (!a7PatchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
		return ERR_LOAD_OTHR;
	}

	if (!(valueBits & ROMinRAM) && !(valueBits & gameOnFlashcard)) {
		patchCardCheckPullOut(ce7, ndsHeader, moduleParams);
	}

	if (a7GetReloc(ndsHeader, moduleParams)) {
		patchMirrorCheck(ndsHeader, moduleParams);
		u32 saveResult = 0;
		
		if (ndsHeader->arm7binarySize==0x2352C || ndsHeader->arm7binarySize==0x235DC || ndsHeader->arm7binarySize==0x23CAC || ndsHeader->arm7binarySize==0x245C0 || ndsHeader->arm7binarySize==0x245C4) {
			saveResult = savePatchInvertedThumb(ce7, ndsHeader, moduleParams);    
		} else if (moduleParams->sdk_version > 0x5000000) {
			// SDK 5
			saveResult = savePatchV5(ce7, ndsHeader);
		} else {
			saveResult = savePatchV1(ce7, ndsHeader, moduleParams);
			if (!saveResult) {
				saveResult = savePatchV2(ce7, ndsHeader, moduleParams);
			}
			if (!saveResult) {
				saveResult = savePatchUniversal(ce7, ndsHeader, moduleParams);
			}
		}
	}

	fixForDifferentBios(ce7, ndsHeader, moduleParams);

	//dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
