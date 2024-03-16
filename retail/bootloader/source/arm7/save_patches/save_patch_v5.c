#include <nds/ndstypes.h>
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"
#include "aeabi.h"

extern u32 newArm7binarySize;
extern u32 vAddrOfRelocSrc;
extern u32 relocDestAtSharedMem;

//
// Subroutine function signatures ARM7
//

static const u32 a7JumpTableSignatureUniversal[3]      = {0xE592000C, 0xE5921010, 0xE5922014};
static const u32 a7JumpTableSignatureUniversal_2[3]    = {0xE593000C, 0xE5931010, 0xE5932014};
static const u16 a7JumpTableSignatureUniversalThumb[4] = {0x6822, 0x68D0, 0x6911, 0x6952};

u32 savePatchV5(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, u32 saveFileCluster) {
	dbg_printf("\nArm7 (patch v5)\n");

	bool usesThumb = patchOffsetCache.a7IsThumb;

	u32 JumpTableFunc = patchOffsetCache.a7JumpTableFuncOffset;

	if (!patchOffsetCache.a7JumpTableFuncOffset) {
		JumpTableFunc = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize,
			a7JumpTableSignatureUniversal, 3
		);

		if(!JumpTableFunc){
			JumpTableFunc = (u32)findOffset(
				(u32*)ndsHeader->arm7destination, newArm7binarySize,
				a7JumpTableSignatureUniversal_2, 3
			);
		}

		if(!JumpTableFunc){
			usesThumb = true;
			patchOffsetCache.a7IsThumb = true;
			JumpTableFunc = (u32)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, newArm7binarySize,
				a7JumpTableSignatureUniversalThumb, 4
			);
		}

		if (JumpTableFunc) {
			patchOffsetCache.a7JumpTableFuncOffset = JumpTableFunc;
		}
	}

	if(!JumpTableFunc){
		return 0;
	}

	dbg_printf("JumpTableFunc: ");
	dbg_hexa(JumpTableFunc);
	dbg_printf("\n");


	u32 srcAddr;

	if (usesThumb) {
		/* u32* cardId = (u32*) (JumpTableFunc - 0xE);
		dbg_printf("card id:\t");
		dbg_hexa((u32)cardId);
		dbg_printf("\n");
		srcAddr = JumpTableFunc - 0xE  - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchCardId = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->cardId);
		cardId[0] = patchCardId[0];
        cardId[1] = patchCardId[1]; */

		u16* eepromReadBranch = (u16*)(JumpTableFunc + 0x8);
		dbg_printf("Eeprom read branch:\t");
		dbg_hexa((u32)eepromReadBranch);
		dbg_printf("\n");
		u16* eepromRead = getOffsetFromBLThumb(eepromReadBranch);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		__aeabi_memcpy(eepromRead, (u16*)ce7->patches->arm7FunctionsThumb->eepromRead, 0x14);

		u16* eepromPageWriteBranch = (u16*)(JumpTableFunc + 0x16);
		dbg_printf("Eeprom page write branch:\t");
		dbg_hexa((u32)eepromPageWriteBranch);
		dbg_printf("\n");
		u16* eepromPageWrite = getOffsetFromBLThumb(eepromPageWriteBranch);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		__aeabi_memcpy(eepromPageWrite, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageWrite, 0x14);

		u16* eepromPageProgBranch = (u16*)(JumpTableFunc + 0x24);
		dbg_printf("Eeprom page prog branch:\t");
		dbg_hexa((u32)eepromPageProgBranch);
		dbg_printf("\n");
		u16* eepromPageProg = getOffsetFromBLThumb(eepromPageProgBranch);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		__aeabi_memcpy(eepromPageProg, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageProg, 0x14);

		u16* eepromPageVerifyBranch = (u16*)(JumpTableFunc + 0x32);
		dbg_printf("Eeprom verify branch:\t");
		dbg_hexa((u32)eepromPageVerifyBranch);
		dbg_printf("\n");
		u16* eepromPageVerify = getOffsetFromBLThumb(eepromPageVerifyBranch);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		__aeabi_memcpy(eepromPageVerify, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageVerify, 0x14);

		u16* eepromPageEraseBranch = (u16*)(JumpTableFunc + 0x3E);
		dbg_printf("Eeprom page erase branch:\t");
		dbg_hexa((u32)eepromPageEraseBranch);
		dbg_printf("\n");
		u16* eepromPageErase = getOffsetFromBLThumb(eepromPageEraseBranch);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		__aeabi_memcpy(eepromPageErase, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageErase, 0x14);
	} else {
		if (*(u32*)(JumpTableFunc - 0x24) == 0xEBFFFFB3) {
			u32* cardId = (u32*) (JumpTableFunc - 0x24);
			dbg_printf("card id:\t");
			dbg_hexa((u32)cardId);
			dbg_printf("\n");
			srcAddr = JumpTableFunc - 0x24  - vAddrOfRelocSrc + relocDestAtSharedMem ;
			u32 patchCardId = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardId);
			*cardId=patchCardId;

			u32* cardRead = (u32*) (JumpTableFunc - 0x14);
			dbg_printf("card read:\t");
			dbg_hexa((u32)cardRead);
			dbg_printf("\n");
			srcAddr = JumpTableFunc - 0x14  - vAddrOfRelocSrc + relocDestAtSharedMem ;
			u32 patchCardRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardRead);
			*cardRead=patchCardRead;
		} else {
			u32* cardId = (u32*) (JumpTableFunc - 0x18);
			dbg_printf("card id:\t");
			dbg_hexa((u32)cardId);
			dbg_printf("\n");
			srcAddr = JumpTableFunc - 0x18  - vAddrOfRelocSrc + relocDestAtSharedMem ;
			u32 patchCardId = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardId);
			*cardId=patchCardId;

			u32* cardRead = (u32*) (JumpTableFunc - 0x8);
			dbg_printf("card read:\t");
			dbg_hexa((u32)cardRead);
			dbg_printf("\n");
			srcAddr = JumpTableFunc - 0x8  - vAddrOfRelocSrc + relocDestAtSharedMem ;
			u32 patchCardRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardRead);
			*cardRead=patchCardRead;
		}

		u32* eepromRead = (u32*) (JumpTableFunc + 0xC);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xC  - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromRead);
		*eepromRead=patchRead;

		u32* eepromPageWrite = (u32*) (JumpTableFunc + 0x24);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x24 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchWrite = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageWrite);
		*eepromPageWrite=patchWrite;

		u32* eepromPageProg = (u32*) (JumpTableFunc + 0x3C);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x3C - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchProg = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageProg);
		*eepromPageProg=patchProg;

		u32* eepromPageVerify = (u32*) (JumpTableFunc + 0x54);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x54 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchVerify = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageVerify);
		*eepromPageVerify=patchVerify;


		u32* eepromPageErase = (u32*) (JumpTableFunc + 0x68);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x68 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		u32 patchErase = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageErase);
		*eepromPageErase=patchErase;
	}
	ce7->patches->arm7Functions->saveCluster = saveFileCluster;

	return 1;
}
