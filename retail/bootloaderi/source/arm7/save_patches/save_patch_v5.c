#include <nds/ndstypes.h>
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"

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
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal, 3
		);

		if(!JumpTableFunc){
			JumpTableFunc = (u32)findOffset(
				(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2, 3
			);
		}

		if(!JumpTableFunc){
			usesThumb = true;
			patchOffsetCache.a7IsThumb = true;
			JumpTableFunc = (u32)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
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
		/* u32* cardRead = (u32*) (JumpTableFunc - 0xE);
		dbg_printf("card read:\t");
		dbg_hexa((u32)cardRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc - 0xE  - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchCardRead = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->cardRead);
		cardRead[0] = patchCardRead[0];
        cardRead[1] = patchCardRead[1]; */

		u16* eepromRead = (u16*) (JumpTableFunc + 0x8);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x8  - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchRead = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromRead);
		eepromRead[0] = patchRead[0];
		eepromRead[1] = patchRead[1];
		
		u16* eepromPageWrite = (u16*) (JumpTableFunc + 0x16);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x16 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchWrite = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageWrite);
        eepromPageWrite[0] = patchWrite[0];
		eepromPageWrite[1] = patchWrite[1];

		u16* eepromPageProg = (u16*) (JumpTableFunc + 0x24);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x24 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchProg = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageProg);
        eepromPageProg[0] = patchProg[0];
		eepromPageProg[1] = patchProg[1];

		u16* eepromPageVerify = (u16*) (JumpTableFunc + 0x32);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x32 - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchVerify = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageVerify);
        eepromPageVerify[0] = patchVerify[0];
		eepromPageVerify[1] = patchVerify[1];


		u16* eepromPageErase = (u16*) (JumpTableFunc + 0x3E);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x3E - vAddrOfRelocSrc + relocDestAtSharedMem ;
		const u16* patchErase = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageErase);        
        eepromPageErase[0] = patchErase[0];
		eepromPageErase[1] = patchErase[1];

	} else {
		if (*(u32*)(JumpTableFunc - 0x24) == 0xEBFFFFB3) {
			u32* cardRead = (u32*) (JumpTableFunc - 0x24);
			dbg_printf("card read:\t");
			dbg_hexa((u32)cardRead);
			dbg_printf("\n");
			srcAddr = JumpTableFunc - 0x24  - vAddrOfRelocSrc + relocDestAtSharedMem ;
			u32 patchCardRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardRead);
			*cardRead=patchCardRead;
		} else {
			u32* cardRead = (u32*) (JumpTableFunc - 0x18);
			dbg_printf("card read:\t");
			dbg_hexa((u32)cardRead);
			dbg_printf("\n");
			srcAddr = JumpTableFunc - 0x18  - vAddrOfRelocSrc + relocDestAtSharedMem ;
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
