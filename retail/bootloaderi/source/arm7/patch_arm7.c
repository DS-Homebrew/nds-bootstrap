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
#include <nds/card.h>

extern u8 gameOnFlashcard;
extern u8 saveOnFlashcard;
extern u8 saveRelocation;
extern u16 a9ScfgRom;
//extern u8 dsiSD;

extern bool i2cBricked;
extern bool sdRead;

extern u32 newArm7binarySize;
extern u32 newArm7ibinarySize;
extern u32 oldArm7mbk;

u32 savePatchV1(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32 saveFileCluster, const u32 saveSize);
u32 savePatchV2(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32 saveFileCluster, const u32 saveSize);
u32 savePatchUniversal(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32 saveFileCluster, const u32 saveSize);
u32 savePatchInvertedThumb(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32 saveFileCluster, const u32 saveSize);
u32 savePatchV5(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const u32 saveFileCluster, const u32 saveSize); // SDK 5

u32 generateA7Instr(int arg1, int arg2) {
	return (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

void setB(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEA000000;
}

void setBEQ(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0x0A000000;
}

void setBL(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

void setBLX(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 10) >> 2) & 0xFFFFFF) | 0xFB000000;
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

u32* getOffsetFromBLX(u32* blxOffset) {
	if (*blxOffset < 0xFB000000 && *blxOffset >= 0xFC000000) {
		return NULL;
	}
	u32 opCode = (*blxOffset) - 0xFB000000;

	if (opCode >= 0x00800000 && opCode <= 0x00FFFFFF) {
		u32 offset = (u32)blxOffset + 10;
		for (u32 i = opCode; i <= 0x00FFFFFF; i++) {
			offset -= 4;
		}

		return (u32*)offset;
	}
	return (u32*)((u32)blxOffset + (opCode*4) + 10);
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

void setBLXThumb(int arg1, int arg2) {
	u16 instrs[2];

	// 23 bit offset
	u32 offset = (u32)(arg2 - arg1 - 4);
	//dbg_printf("generateA7InstrThumb offset\n");
	//dbg_hexa(offset);

	// 1st instruction contains the upper 11 bit of the offset
	instrs[0] = ((offset >> 12) & 0x7FF) | 0xF000;

	// 2nd instruction contains the lower 11 bit of the offset
	instrs[1] = ((offset >> 1) & 0x7FF) | 0xE800;
	if ((instrs[1] % 2) != 0) {
		instrs[1]++;
	}

	*(u16*)arg1 = instrs[0];
	*(u16*)(arg1 + 2) = instrs[1];
}

u16* getOffsetFromBLThumb(const u16* blOffset) {
	const u32* instructionPointer = (u32*)blOffset;
	u32 blInstruction1 = ((u16*)instructionPointer)[0];
	u32 blInstruction2 = ((u16*)instructionPointer)[1];
	u32 res = (u32)instructionPointer + 5 + ((int)((((blInstruction1 & 0x7FF) << 11) | (blInstruction2 & 0x7FF)) << 10) >> 9);
	res--;
	return (u16*)res;
}

u32 vAddrOfRelocSrc = 0;
u32 relocDestAtSharedMem = 0;
u32 newSwiHaltAddr = 0;
// bool swiHaltPatched = false;

static void patchSwiHalt(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern bool pkmnGen5;
	if (pkmnGen5) return;

	u32* swiHaltOffset = patchOffsetCache.swiHaltOffset;
	if (!patchOffsetCache.swiHaltOffset) {
		swiHaltOffset = patchOffsetCache.a7IsThumb ? (u32*)findSwiHaltThumbOffset(ndsHeader, moduleParams) : findSwiHaltOffset(ndsHeader, moduleParams);
		if (swiHaltOffset) {
			patchOffsetCache.swiHaltOffset = swiHaltOffset;
		}
	}

	if (swiHaltOffset) {
		// Patch
		if (patchOffsetCache.a7IsThumb) {
			tonccpy((u16*)newSwiHaltAddr, ce7->patches->newSwiHaltThumb, 0x18);
			u32 srcAddr = (u32)swiHaltOffset - vAddrOfRelocSrc + (*(u32*)0x02FFE1A0==0x080037C0 ? 0x37C0000 : 0x37F8000);
			u32 dstAddr = (u32)newSwiHaltAddr - vAddrOfRelocSrc + (*(u32*)0x02FFE1A0==0x080037C0 ? 0x37C0000 : 0x37F8000);
			const u16* swiHaltPatch = generateA7InstrThumb(srcAddr, dstAddr);
			tonccpy(swiHaltOffset, swiHaltPatch, 0x4);

			dbg_printf("swiHalt new location : ");
			dbg_hexa(newSwiHaltAddr);
			dbg_printf("\n\n");
		} else {
			u32* swiHaltPatch = ce7->patches->j_newSwiHalt;
			tonccpy(swiHaltOffset, swiHaltPatch, 0xC);
		}
		// swiHaltPatched = true;
		dbg_printf("swiHalt hooked\n");
	}

    dbg_printf("swiHalt location : ");
    dbg_hexa((u32)swiHaltOffset);
    dbg_printf("\n\n");
}

void patchScfgExt(const tNDSHeader* ndsHeader) {
	if (ndsHeader->unitCode == 0) return;

	const u32 scfgLoc = 0x2FFFD00;

	if (dsiModeConfirmed) {
		*(u16*)(scfgLoc+0x00) = 0x0101;
		//*(u16*)(scfgLoc+0x04) = 0x0187;
		//*(u16*)(scfgLoc+0x06) = 0;
		*(u32*)(scfgLoc+0x08) = 0x93FFFB06;
		//*(u16*)(scfgLoc+0x20) = 1;
		//*(u16*)(scfgLoc+0x24) = 0;
	}

	if (newArm7binarySize == 0x44C) return;

	u32* scfgExtOffset = patchOffsetCache.a7ScfgExtOffset;
	if (!patchOffsetCache.a7ScfgExtOffset) {
		scfgExtOffset = a7_findScfgExtOffset(ndsHeader);
		if (scfgExtOffset) {
			patchOffsetCache.a7ScfgExtOffset = scfgExtOffset;
		}
	}
	if (scfgExtOffset) {
		scfgExtOffset[0] = scfgLoc+0x08;
		//scfgExtOffset[1] = scfgLoc+0x20;
		//scfgExtOffset[2] = scfgLoc+0x04;
		//scfgExtOffset[4] = scfgLoc+0x24;
		scfgExtOffset[5] = scfgLoc+0x00;
		scfgExtOffset[6] = scfgLoc+0x01;
		//scfgExtOffset[7] = scfgLoc+0x06;
	}

    dbg_printf("SCFG_EXT location : ");
    dbg_hexa((u32)scfgExtOffset);
    dbg_printf("\n\n");
}

static void fixForDifferentBios(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
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

	// swi 0x12 call
	if (swi12Offset && !(REG_SCFG_ROM & BIT(9))) {
		// Patch to call swi 0x02 instead of 0x12
		u32* swi12Patch = ce7->patches->swi02;
		tonccpy(swi12Offset, swi12Patch, 0x4);
	}

	// swi get pitch table
	if (swiGetPitchTableOffset && (!(REG_SCFG_ROM & BIT(9)) || ((REG_SCFG_ROM & BIT(9)) && ndsHeader->unitCode > 0 && dsiModeConfirmed))) {
		// Patch
		if (useGetPitchTableBranch) {
			tonccpy(swiGetPitchTableOffset, ce7->patches->j_twlGetPitchTableThumb, 0x40);
		} else if (isSdk5(moduleParams)) {
			toncset16(swiGetPitchTableOffset, 0x46C0, 6);
			if ((REG_SCFG_ROM & BIT(9)) && dsiModeConfirmed) {
				u16* swiGetPitchTableOffsetThumb = (u16*)patchOffsetCache.swiGetPitchTableOffset;
				toncset16(swiGetPitchTableOffsetThumb+2, 0x46C0, 6);
			}
		} else if (!patchOffsetCache.a7IsThumb) {
			u32* swiGetPitchTablePatch = ce7->patches->j_twlGetPitchTable;
			tonccpy(swiGetPitchTableOffset, swiGetPitchTablePatch, 0xC);
		}
	}

	dbg_printf("swi12 location : ");
	dbg_hexa((u32)swi12Offset);
	dbg_printf("\n\n");
	dbg_printf(useGetPitchTableBranch ? "swiGetPitchTableBranch location : " : "swiGetPitchTable location : ");
	dbg_hexa((u32)swiGetPitchTableOffset);
	dbg_printf("\n\n");

	if ((REG_SCFG_ROM & BIT(9)) && *(u32*)0x02F78020 == 0xEA001FF6 && ndsHeader->unitCode > 0 && dsiModeConfirmed) {
		u16* swi24Offset = patchOffsetCache.a7Swi24Offset;
		u16* swi25Offset = patchOffsetCache.a7Swi25Offset;
		u16* swi26Offset = patchOffsetCache.a7Swi26Offset;
		u16* swi27Offset = patchOffsetCache.a7Swi27Offset;
		if (!patchOffsetCache.a7Swi24Offset) {
			swi24Offset = a7_findSwi24Offset();
			if (swi24Offset) {
				patchOffsetCache.a7Swi24Offset = swi24Offset;
			}
		}
		if (!patchOffsetCache.a7Swi25Offset) {
			swi25Offset = a7_findSwi25Offset();
			if (swi25Offset) {
				patchOffsetCache.a7Swi25Offset = swi25Offset;
			}
		}
		if (!patchOffsetCache.a7Swi26Offset) {
			swi26Offset = a7_findSwi26Offset();
			if (swi26Offset) {
				patchOffsetCache.a7Swi26Offset = swi26Offset;
			}
		}
		if (!patchOffsetCache.a7Swi27Offset) {
			swi27Offset = a7_findSwi27Offset();
			if (swi27Offset) {
				patchOffsetCache.a7Swi27Offset = swi27Offset;
			}
		}

		if (swi24Offset) {
			*swi24Offset = 0xE77E;

			u32 dst = ((u32)swi24Offset) - 0x100;
			tonccpy((u32*)dst, ce7->patches->swi24, 0x8);
		}
		if (swi25Offset) {
			*swi25Offset = 0xE780;

			u32 dst = ((u32)swi25Offset) - 0xFC;
			tonccpy((u32*)dst, ce7->patches->swi25, 0x8);
		}
		if (swi26Offset) {
			*swi26Offset = 0xE77F;

			u32 dst = ((u32)swi26Offset) - 0xFE;
			tonccpy((u32*)dst, ce7->patches->swi26, 0x8);
		}
		if (swi27Offset) {
			*swi27Offset = 0xE780;

			u32 dst = ((u32)swi27Offset) - 0xFC;
			tonccpy((u32*)dst, ce7->patches->swi27, 0x8);
		}

		//u32 mainMemA7iStart = (*(u32*)0x02FFE1A0 != 0x00403000) ? 0x02F88000 : 0x02F80000;

		dbg_printf("swi24 location : ");
		dbg_hexa((u32)swi24Offset);
		//dbg_printf(" ");
		//dbg_hexa((u32)swi24Offset - a7iStartOffset + mainMemA7iStart);
		dbg_printf("\n\n");
		dbg_printf("swi25 location : ");
		dbg_hexa((u32)swi25Offset);
		//dbg_printf(" ");
		//dbg_hexa((u32)swi25Offset - a7iStartOffset + mainMemA7iStart);
		dbg_printf("\n\n");
		dbg_printf("swi26 location : ");
		dbg_hexa((u32)swi26Offset);
		//dbg_printf(" ");
		//dbg_hexa((u32)swi26Offset - a7iStartOffset + mainMemA7iStart);
		dbg_printf("\n\n");
		dbg_printf("swi27 location : ");
		dbg_hexa((u32)swi27Offset);
		//dbg_printf(" ");
		//dbg_hexa((u32)swi27Offset - a7iStartOffset + mainMemA7iStart);
		dbg_printf("\n\n");
	}
}

static void patchMirrorCheck(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (isSdk5(moduleParams)) {
		return;
	}

	u32* offset = (u32*)patchOffsetCache.relocateStartOffset;
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

	offset[0] = dsiModeConfirmed ? 0xE3A01002 : 0xE3A01001; // mov r1, #dsiModeConfirmed ? 2 : 1
	offset[1] = 0xE59F2004; // ldr r2, =0x027FFFFA
	offset[2] = 0xE1C210B0; // strh r1, [r2]
	offset[3] = 0xE12FFF1E; // bx lr
	offset[4] = 0x027FFFFA;

	dbg_printf("RAM mirror check location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

bool hasVramWifiBinary = false;
static void patchVramWifiBinaryLoad(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (ndsHeader->unitCode > 0 || moduleParams->sdk_version < 0x2008000) return;

	// Relocate VRAM WiFi binary from Main RAM to DSi WRAM
	u32* offset = (u32*)patchOffsetCache.relocateStartOffset;
	const u32 add = ((u32)ce7 == CARDENGINEI_ARM7_LOCATION) ? 0x00FE0000 : 0x00820000; // 0x037C0000 : 0x03000000
	bool found = false;
	for (int i = 0; i < 0x100/sizeof(u32); i++) {
		if (*offset >= 0x027E0000 && *offset < 0x027E0200) {
			*offset += add;
			found = true;
			break;
		}
		offset++;
	}
	if (!found) {
		return;
	}

	hasVramWifiBinary = true;

	dbg_printf("VRAM WiFi binary load location end : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");

	offset = (u32*)patchOffsetCache.relocateStartOffset;

	for (int i = 0; i < 0x80/sizeof(u32); i++) {
		if (offset[i] == 0x0A000000) {
			offset[i+1] = 0xE1A00000; // nop
			break;
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
	if (REG_SCFG_EXT == 0 || (REG_SCFG_MC & BIT(0)) || (!(REG_SCFG_MC & BIT(2)) && !(REG_SCFG_MC & BIT(3)))
	 || forceSleepPatch) {
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

void patchSleepInputWrite(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	u32* offset = patchOffsetCache.sleepInputWriteOffset;
	if (!patchOffsetCache.sleepInputWriteOffset) {
		offset = findSleepInputWriteOffset(ndsHeader, moduleParams);
		if (offset) {
			patchOffsetCache.sleepInputWriteOffset = offset;
		}
	}
	if (!offset) {
		return;
	}

	if (!sleepMode) {
		if (*offset == 0x13A04902 || *offset == 0x11A05004) {
			*offset = 0xE1A00000; // nop
		} else {
			u16* offsetThumb = (u16*)offset;
			*offsetThumb = 0x46C0; // nop
		}
	}

	dbg_printf("Sleep input write location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

static void patchRamClear(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000 || ndsHeader->unitCode == 0 || dsiModeConfirmed) {
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
		*(ramClearOffset) = 0x02FFC000;
		*(ramClearOffset + 1) = 0x02FFC000;
		dbg_printf("RAM clear location : ");
		dbg_hexa((u32)ramClearOffset);
		dbg_printf("\n\n");
	}
	patchOffsetCache.ramClearChecked = true;
}

void patchRamClearI(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const bool _isDSiWare) {
	if (moduleParams->sdk_version < 0x5000000 || ndsHeader->unitCode == 0 || !dsiModeConfirmed) {
		return;
	}

	u32* ramClearOffset = patchOffsetCache.ramClearIOffset;
	if (!patchOffsetCache.ramClearIOffset) {
		ramClearOffset = findRamClearIOffset(ndsHeader);
		if (ramClearOffset) {
			patchOffsetCache.ramClearIOffset = ramClearOffset;
		}
	}
	if (!ramClearOffset) {
		return;
	}

	if (*(u32*)0x02FFE1A0 != 0x00403000) {
		// extern u8 consoleModel;
		extern u32 ce7Location;
		// extern u32 cheatSizeTotal;
		// const bool cheatsEnabled = (cheatSizeTotal > 4 && cheatSizeTotal <= 0x8000);

		// ramClearOffset[0] = (consoleModel == 0 && _isDSiWare && cheatsEnabled && newArm7binarySize != 0x28E54) ? CHEAT_ENGINE_DSIWARE_LOCATION3 : 0x02FFC000;
		ramClearOffset[2] = ce7Location;
	} else {
		extern u32 ce9Location;
		ramClearOffset[0] = ce9Location;
	}
	dbg_printf("RAM clear I location : ");
	dbg_hexa((u32)ramClearOffset);
	dbg_printf("\n\n");

	if (*(u32*)0x02FFE1A0 != 0x00403000) {
		return;
	}

	ramClearOffset = patchOffsetCache.ramClearI2Offset;
	if (!patchOffsetCache.ramClearI2Offset) {
		ramClearOffset = findRamClearI2Offset(patchOffsetCache.ramClearIOffset);
		if (ramClearOffset) {
			patchOffsetCache.ramClearI2Offset = ramClearOffset;
		}
	}
	if (!ramClearOffset) {
		return;
	}

	if (*(u16*)ramClearOffset == 0x27C1) { // THUMB
		u16* offset = (u16*)ramClearOffset;
		offset[0] = 0x2700; // movs r7, #0
		offset[1] = 0x46C0; // nop
	} else {
		*ramClearOffset = 0xE3A01000; // mov r1, #0
	}
	dbg_printf("RAM clear I 2 location : ");
	dbg_hexa((u32)ramClearOffset);
	dbg_printf("\n\n");
}

void patchPostBoot(const tNDSHeader* ndsHeader) {
	if (REG_SCFG_EXT != 0 || ndsHeader->unitCode == 0 || !dsiModeConfirmed || (oldArm7mbk == 0x00403000 && *(u32*)0x02FFE1A0 == 0x00403000) || *(u32*)0x02FFE1A0 == 0x080037C0) {
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
		const bool usesThumb = (*(u16*)postBootOffset == 0xB5F8);
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

bool a7PatchCardIrqEnable(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
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
	const bool usesThumb = (*(u16*)cardIrqEnableOffset == 0xB510 || *(u16*)cardIrqEnableOffset == 0xB530);
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

static void patchSrlStart(cardengineArm7* ce7, const tNDSHeader* ndsHeader) {
	u32* offset = findSrlStartOffset7(ndsHeader);
	if (!offset) {
		return;
	}

	offset[0] = 0xE3A00001; // mov r0, #1
	offset[1] = 0xE59FC000; // ldr r12, =reset
	offset[2] = 0xE12FFF1C; // bx r12
	offset[3] = (u32)ce7->patches->reset;
}

static void patchSdCardReset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (ndsHeader->unitCode == 0 || !dsiModeConfirmed) return;

	// DSi SD Card reset
	u32* sdCardResetOffset = patchOffsetCache.sdCardResetOffset;
	if (!patchOffsetCache.sdCardResetOffset) {
		sdCardResetOffset = findSdCardResetOffset(ndsHeader, moduleParams);
		if (sdCardResetOffset) {
			patchOffsetCache.sdCardResetOffset = sdCardResetOffset;
		}
	}
	if (sdCardResetOffset) {
		*((u16*)sdCardResetOffset+4) = 0;
		*((u16*)sdCardResetOffset+5) = 0;

		dbg_printf("sdCardReset location : ");
		dbg_hexa((u32)sdCardResetOffset);
		dbg_printf("\n\n");
	}
}

bool patchSdCardFuncs(cardengineArm7* ce7, const tNDSHeader* ndsHeader) {
	u32* offset = patchOffsetCache.sdCardFuncsOffset;
	if (!patchOffsetCache.sdCardFuncsOffset) {
		offset = findSdCardFuncsOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.sdCardFuncsOffset = offset;
		}
	}

	if (!offset) {
		return false;
	}

	u16* offsetThumb = (u16*)offset;
	if (*offsetThumb == 0xB518) {
		ce7->romPartLocation = (u32)getOffsetFromBLThumb((u16*)((u8*)offset - 0x20)); // getDriveStructAddr
		ce7->romPartLocation++;
		*(u32*)((u8*)offset + 0x44) = ce7->patches->arm7Functions->eepromProtect; // __patch_dsisdredirect_io
		*(u32*)((u8*)offset + 0x48) = ce7->patches->arm7Functions->eepromPageErase; // __patch_dsisdredirect_control
	} else {
		ce7->romPartLocation = (u32)getOffsetFromBL((u32*)((u8*)offset - 0x20)); // getDriveStructAddr
		*(u32*)((u8*)offset + 0x64) = ce7->patches->arm7Functions->eepromProtect; // __patch_dsisdredirect_io
		*(u32*)((u8*)offset + 0x68) = ce7->patches->arm7Functions->eepromPageErase; // __patch_dsisdredirect_control
	}

	dbg_printf("sdCardFuncs location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
	return true;
}

void patchAutoPowerOff(const tNDSHeader* ndsHeader) {
	if (!i2cBricked || ndsHeader->unitCode == 0 || !dsiModeConfirmed) return;

	u32* offset = patchOffsetCache.autoPowerOffOffset;
	if (!patchOffsetCache.autoPowerOffOffset) {
		offset = findAutoPowerOffOffset(ndsHeader);
		if (offset) {
			patchOffsetCache.autoPowerOffOffset = offset;
		}
	}

	if (!offset) {
		return;
	}

	u16* offsetThumb = (u16*)offset;
	if (offsetThumb[0] == 0xB5F8) {
		offsetThumb[0] = 0x2004; // movs r0, #4
		offsetThumb[1] = 0x4770; // bx lr
	} else {
		offset[0] = 0xE3A00004; // mov r0, #4
		offset[1] = 0xE12FFF1E; // bx lr
	}

	dbg_printf("autoPowerOff location : ");
	dbg_hexa((u32)offset);
	dbg_printf("\n\n");
}

static void operaRamPatch(void) {
	// Opera RAM patch (ARM7)
	*(u32*)0x0238C7BC = 0xC3E0000;
	*(u32*)0x0238C7C0 = 0xC3E00CE;

	//*(u32*)0x0238C950 = 0xC3E0000;
}

extern void rsetA7Cache(void);

u32 patchCardNdsArm7(
	cardengineArm7* ce7,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 saveFileCluster,
	u32 saveSize
) {
	newArm7binarySize = ndsHeader->arm7binarySize;
	newArm7ibinarySize = __DSiHeader->arm7ibinarySize;

	if (((ndsHeader->unitCode > 0) ? (REG_SCFG_EXT == 0) : (memcmp(ndsHeader->gameCode, "AYI", 3) == 0 && ndsHeader->arm7binarySize == 0x25F70)) && ((u32)ndsHeader->arm9destination+ndsHeader->arm9binarySize) < DONOR_ROM_ARM7_LOCATION && *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION != 0) {
		// Replace incompatible ARM7 binary
		newArm7binarySize = *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION;
		newArm7ibinarySize = *(u32*)DONOR_ROM_ARM7I_SIZE_LOCATION;
		tonccpy(ndsHeader->arm7destination, (u8*)DONOR_ROM_ARM7_LOCATION, newArm7binarySize);
		toncset((u8*)DONOR_ROM_ARM7_LOCATION, 0, 0x30010);
	}

	if (newArm7binarySize != patchOffsetCache.a7BinSize) {
		rsetA7Cache();
		patchOffsetCache.a7BinSize = newArm7binarySize;
	}

	patchPostBoot(ndsHeader);
	patchScfgExt(ndsHeader);
	patchSleepMode(ndsHeader);
	patchSleepInputWrite(ndsHeader, moduleParams);

	patchRamClear(ndsHeader, moduleParams);
	patchRamClearI(ndsHeader, moduleParams, false);

	// Touch fix for SM64DS (U) v1.0
	if (newArm7binarySize == 0x24B64
	 && *(u32*)0x023825E4 == 0xE92D4030
	 && *(u32*)0x023825E8 == 0xE24DD004) {
		tonccpy((char*)0x023825E4, (char*)ARM7_FIX_BUFFERED_LOCATION, 0x140);
	}
	toncset((char*)ARM7_FIX_BUFFERED_LOCATION, 0, 0x140);

	if (!a7PatchCardIrqEnable(ce7, ndsHeader, moduleParams)) {
		dbg_printf("ERR_LOAD_OTHR");
		return ERR_LOAD_OTHR;
	}

	const char* romTid = getRomTid(ndsHeader);

	if ((strncmp(romTid, "UOR", 3) == 0 && !saveOnFlashcard)
	|| (strncmp(romTid, "UXB", 3) == 0 && !saveOnFlashcard)
	|| (!ROMinRAM && !gameOnFlashcard)) {
		patchCardCheckPullOut(ce7, ndsHeader, moduleParams);
	}

	if (patchOffsetCache.srlStartOffset9) {
		patchSrlStart(ce7, ndsHeader);
	}

	if (a7GetReloc(ndsHeader, moduleParams)) {
		patchMirrorCheck(ndsHeader, moduleParams);
		patchVramWifiBinaryLoad(ce7, ndsHeader, moduleParams);
		u32 saveResult = 0;
		// Save Relocation Switch
		// Only when saveRelocation is turning off(FLASE) and GameCodeMatch is matching(TRUE),
		// will the save file be located into DS Cartridge as original. 
		// If saveRelocation is turning on(TRUE), 
		// or saveRelocation is turning off(FLASE) but GameCodeMatch isn't matching(FALSE),
		// the save file will be relocated into SD card.
		// saveRelocation: option value write in nds-bootstrap.ini (default value is 1)
		// GameCodeMatch: Compare gamecodes between DS Cartridge and Rom on SD
		// This file can active save located into DS Cartridge for dsi/3ds mode.
		char headerData[0x200] = {0};
		bool GameCodeMatch = FALSE;
		cardParamCommand (CARD_CMD_HEADER_READ, 0, 
			CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(1) | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F), 
			(u32 *)headerData , 0x200 / sizeof(u32));
		if(memcmp((const void*)(headerData + 0xC), ndsHeader->gameCode, 4) == 0)
			GameCodeMatch = TRUE;
		dbg_printf("romTid:");
		dbg_printf(ndsHeader->gameCode);
		dbg_printf("\nCardTid:");
		dbg_printf((headerData+0xC));
		dbg_printf("\nGameCodeMatch:");
		dbg_hexa(GameCodeMatch);
		dbg_printf("\nSaveRelocation:");
		dbg_printf(saveRelocation);
		dbg_printf("\n");
		if (!(saveRelocation == FALSE && GameCodeMatch == TRUE)){
		if (newArm7binarySize==0x2352C || newArm7binarySize==0x235DC || newArm7binarySize==0x23CAC || newArm7binarySize==0x245C0 || newArm7binarySize==0x245C4) {
			saveResult = savePatchInvertedThumb(ce7, ndsHeader, moduleParams, saveFileCluster, saveSize);    
		} else if (isSdk5(moduleParams)) {
			// SDK 5
			saveResult = savePatchV5(ce7, ndsHeader, saveFileCluster, saveSize);
		} else {
			if (patchOffsetCache.savePatchType == 0) {
				saveResult = savePatchV1(ce7, ndsHeader, moduleParams, saveFileCluster, saveSize);
				if (!saveResult) {
					patchOffsetCache.savePatchType = 1;
				}
			}
			if (!saveResult && patchOffsetCache.savePatchType == 1) {
				saveResult = savePatchV2(ce7, ndsHeader, moduleParams, saveFileCluster, saveSize);
				if (!saveResult) {
					patchOffsetCache.savePatchType = 2;
				}
			}
			if (!saveResult && patchOffsetCache.savePatchType == 2) {
				saveResult = savePatchUniversal(ce7, ndsHeader, moduleParams, saveFileCluster, saveSize);
			}
		}
		}
		if (!saveResult) {
			patchOffsetCache.savePatchType = 0;
		} /*else if (strncmp(romTid, "AMH", 3) == 0) {
			aFile* savFile = (aFile*)(dsiSD ? SAV_FILE_LOCATION : SAV_FILE_LOCATION_ALT);
			fileRead((char*)0x02440000, *savFile, 0, 0x40000, 0);
		}*/
	}

	patchSwiHalt(ce7, ndsHeader, moduleParams);

	if (strcmp(romTid, "UBRP") == 0) {
		operaRamPatch();
	}

	fixForDifferentBios(ce7, ndsHeader, moduleParams);

	//if (!gameOnFlashcard) {
		patchSdCardReset(ndsHeader, moduleParams);
	//}

	patchAutoPowerOff(ndsHeader);

	dbg_printf("ERR_NONE\n\n");
	return ERR_NONE;
}
