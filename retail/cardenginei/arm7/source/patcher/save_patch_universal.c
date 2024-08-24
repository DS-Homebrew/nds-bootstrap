#include <nds/ndstypes.h>
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "cardengine_header_arm7.h"
//#include "debug_file.h"
#include "tonccpy.h"

extern u32 vAddrOfRelocSrc;
extern u32 relocDestAtSharedMem;
extern u32 newSwiHaltAddr;

//
// Subroutine function signatures ARM7
//

//static const u32 a7cardReadSignature[2] = {0x04100010, 0x040001A4};

/*
static const u32 a7JumpTableSignature[4]                        = {0xE5950024, 0xE3500000, 0x13A00001, 0x03A00000};
static const u32 a7JumpTableSignatureV3_1[3]                    = {0xE92D4FF0, 0xE24DD004, 0xE59F91F8};
static const u32 a7JumpTableSignatureV3_2[3]                    = {0xE92D4FF0, 0xE24DD004, 0xE59F91D4};
static const u32 a7JumpTableSignatureV4_1[3]                    = {0xE92D41F0, 0xE59F4224, 0xE3A05000};
static const u32 a7JumpTableSignatureV4_2[3]                    = {0xE92D41F0, 0xE59F4200, 0xE3A05000};
*/
static const u32 a7JumpTableSignatureUniversal[3]               = {0xE592000C, 0xE5921010, 0xE5922014};
static const u32 a7JumpTableSignatureUniversal_pt2[3]           = {0xE5920010, 0xE592100C, 0xE5922014};
static const u32 a7JumpTableSignatureUniversal_pt3[2]           = {0xE5920010, 0xE5921014};
static const u32 a7JumpTableSignatureUniversal_pt3_alt[2]       = {0xE5910010, 0xE5911014};
static const u32 a7JumpTableSignatureUniversal_2[3]             = {0xE593000C, 0xE5931010, 0xE5932014};
static const u32 a7JumpTableSignatureUniversal_2_pt2[3]         = {0xE5930010, 0xE593100C, 0xE5932014};
static const u32 a7JumpTableSignatureUniversal_2_pt3[2]         = {0xE5930010, 0xE5931014};
static const u16 a7JumpTableSignatureUniversalThumb[3]          = {0x68D0, 0x6911, 0x6952};
static const u16 a7JumpTableSignatureUniversalThumb_pt2[3]      = {0x6910, 0x68D1, 0x6952};
static const u16 a7JumpTableSignatureUniversalThumb_pt3[2]      = {0x6908, 0x6949};
static const u16 a7JumpTableSignatureUniversalThumb_pt3_alt[2]  = {0x6910, 0x6951};
static const u16 a7JumpTableSignatureUniversalThumb_pt3_alt2[2] = {0x6800, 0x6900};
static const u16 a7JumpTableSignatureUniversalThumb_pt3_alt3[2] = {0x6800, 0x6900};


u32 savePatchUniversal(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	//dbg_printf("\nArm7 (patch vAll)\n");

	// Find the card read
	/*u32 cardReadEndAddr = findOffset(
		(u32*)ndsHeader->arm7destination, 0x00020000, 
		a7cardReadSignature, 2
	);
	if (!cardReadEndAddr) {
		dbg_printf("[Error!] Card read addr not found\n");
		return 0;
	}
	dbg_printf("cardReadEndAddr: ");
	dbg_hexa(cardReadEndAddr);
	dbg_printf("\n");*/

	u32* JumpTableFunc = 0;
	u32* EepromReadJump = 0;
	u32* EepromWriteJump = 0;
	u32* EepromProgJump = 0;
	u32* EepromVerifyJump = 0;
	u32* EepromEraseJump = 0;
	
	extern bool a7IsThumb;
	int JumpTableFuncType = 0;

	if (JumpTableFuncType == 0) {
		if (!JumpTableFunc) {
			JumpTableFunc = findOffset(
				(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal, 3
			);
		}
		if (JumpTableFunc) {
			EepromReadJump = findOffset(
				(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal, 3
			);
			EepromWriteJump = findOffset(
				EepromReadJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_pt2, 3
			);
			EepromProgJump = findOffset(
				EepromWriteJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_pt2, 3
			);
			EepromVerifyJump = findOffset(
				EepromProgJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_pt2, 3
			);
			EepromEraseJump = findOffset(
				EepromVerifyJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_pt3, 2
			);
			if(!EepromEraseJump) EepromEraseJump = findOffset(
				EepromVerifyJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_pt3_alt, 2
			);
		} else JumpTableFuncType++;
	}
	if (JumpTableFuncType == 1) {
		if (!JumpTableFunc) {
			JumpTableFunc = findOffset(
				(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2, 3
			);
		}
		if (JumpTableFunc) {
			EepromReadJump = findOffset(
				(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2, 3
			);
			EepromWriteJump = findOffset(
				EepromReadJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt2, 3
			);
			EepromProgJump = findOffset(
				EepromWriteJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt2, 3
			);
			EepromVerifyJump = findOffset(
				EepromProgJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt2, 3
			);
			EepromEraseJump = findOffset(
				EepromVerifyJump + 4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt3, 2
			);
		} else {
			JumpTableFuncType = 0;
			a7IsThumb = true;
		}
	}
	if (a7IsThumb) {
		if (!JumpTableFunc) {
			JumpTableFunc = (u32*)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb, 3
			);
		}

		if (!JumpTableFunc) {
			return 0;
		}

		/*dbg_printf("usesThumb\n");
		dbg_printf("JumpTableFunc\n");
		dbg_hexa((u32)JumpTableFunc);*/

		EepromReadJump = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversalThumb, 3
		);

		if (JumpTableFuncType == 0) {
			EepromWriteJump = (u32*)findOffsetThumb(
				(u16*)EepromReadJump + 2, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb_pt2, 3
			);
			if (EepromWriteJump) {
				EepromProgJump = (u32*)findOffsetThumb(
					(u16*)EepromWriteJump + 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 3
				);
				EepromVerifyJump = (u32*)findOffsetThumb(
					(u16*)EepromProgJump + 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 3
				);
				EepromEraseJump = (u32*)findOffsetThumb(
					(u16*)EepromVerifyJump + 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt3, 2
				);
				if (!EepromEraseJump) {
					EepromEraseJump = (u32*)findOffsetThumb(
						(u16*)EepromVerifyJump + 2, ndsHeader->arm7binarySize,
						a7JumpTableSignatureUniversalThumb_pt3_alt, 2
					);
				}
				if (!EepromEraseJump) {
					EepromEraseJump = (u32*)findOffsetThumb(
						(u16*)EepromVerifyJump + 2, ndsHeader->arm7binarySize,
						a7JumpTableSignatureUniversalThumb_pt3_alt2, 2
					);
				}
			} else JumpTableFuncType++;
		}
		if (JumpTableFuncType == 1) {
			// alternate v1 order
			EepromProgJump = (u32*)findOffsetThumb(
				(u16*)JumpTableFunc, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb_pt2, 2
			);
			EepromWriteJump = (u32*)findOffsetThumb(
				(u16*)EepromProgJump - 2, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb_pt2, 2
			);
			EepromVerifyJump = (u32*)findOffsetThumb(
				(u16*)EepromWriteJump - 2, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb_pt2, 2
			);
			EepromEraseJump = (u32*)findOffsetThumb(
				(u16*)EepromVerifyJump - 2, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb_pt3_alt3, 2
			);
		}
	}
	/*if (!JumpTableFunc) {
		return 0;
	}*/

	/*dbg_printf("\nJumpTableFunc: ");
	dbg_hexa((u32)JumpTableFunc);
	dbg_printf("\n");*/

	u32 srcAddr;

	if (a7IsThumb) {
		if (JumpTableFuncType == 0) {
			u16* eepromReadBranch = (u16*)((u32)EepromReadJump + 0x6);
			/*dbg_printf("Eeprom read branch:\t");
			dbg_hexa((u32)eepromReadBranch);
			dbg_printf("\n");*/
			u16* eepromRead = getOffsetFromBLThumb(eepromReadBranch);
			/*dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");*/
			tonccpy(eepromRead, (u16*)ce7->patches->arm7FunctionsThumb->eepromRead, 0x14);

			u16* eepromPageWriteBranch = (u16*)((u32)EepromWriteJump + 0x6);
			/*dbg_printf("Eeprom page write branch:\t");
			dbg_hexa((u32)eepromPageWriteBranch);
			dbg_printf("\n");*/
			u16* eepromPageWrite = getOffsetFromBLThumb(eepromPageWriteBranch);
			/*dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");*/
			tonccpy(eepromPageWrite, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageWrite, 0x14);
			newSwiHaltAddr = (u32)eepromPageWrite+0x14;

			u16* eepromPageProgBranch = (u16*)((u32)EepromProgJump + 0x6);
			/*dbg_printf("Eeprom page prog branch:\t");
			dbg_hexa((u32)eepromPageProgBranch);
			dbg_printf("\n");*/
			u16* eepromPageProg = getOffsetFromBLThumb(eepromPageProgBranch);
			/*dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");*/
			tonccpy(eepromPageProg, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageProg, 0x14);

			u16* eepromPageVerifyBranch = (u16*)((u32)EepromVerifyJump + 0x6);
			/*dbg_printf("Eeprom verify branch:\t");
			dbg_hexa((u32)eepromPageVerifyBranch);
			dbg_printf("\n");*/
			u16* eepromPageVerify = getOffsetFromBLThumb(eepromPageVerifyBranch);
			/*dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");*/
			tonccpy(eepromPageVerify, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageVerify, 0x14);

			u16* eepromPageEraseBranch = (u16*)((u32)EepromEraseJump + 0x4);
			/*dbg_printf("Eeprom page erase branch:\t");
			dbg_hexa((u32)eepromPageEraseBranch);
			dbg_printf("\n");*/
			u16* eepromPageErase = getOffsetFromBLThumb(eepromPageEraseBranch);
			/*dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");*/
			tonccpy(eepromPageErase, (u16*)ce7->patches->arm7FunctionsThumb->eepromPageErase, 0x14);
		} else {
			u32* eepromRead = (u32*)((u32)EepromReadJump + 0xA);
			/*dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");*/
			*eepromRead = ce7->patches->arm7Functions->eepromRead;

			u32* eepromPageWrite = (u32*)((u32)EepromWriteJump + 0xA);
			newSwiHaltAddr = *eepromPageWrite;
			newSwiHaltAddr -= 0x37F8000;
			newSwiHaltAddr += vAddrOfRelocSrc;
			newSwiHaltAddr--;
			/*dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");*/
			*eepromPageWrite = ce7->patches->arm7Functions->eepromPageWrite;

			u32* eepromPageProg = (u32*)((u32)EepromProgJump + 0xA);
			/*dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");*/
			*eepromPageProg = ce7->patches->arm7Functions->eepromPageProg;

			u32* eepromPageVerify = (u32*)((u32)EepromVerifyJump + 0xA);
			/*dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");*/
			*eepromPageVerify = ce7->patches->arm7Functions->eepromPageVerify;

			u32* eepromPageErase = (u32*)((u32)EepromEraseJump + 0x8);
			/*dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");*/
			*eepromPageErase = ce7->patches->arm7Functions->eepromPageErase;
		}
	} else {
		u32* eepromRead = (u32*)((u32)EepromReadJump + 0xC);
		/*dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");*/
		srcAddr = (u32)EepromReadJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromRead);
		*eepromRead = patchRead;
	
		u32* eepromPageWrite = (u32*)((u32)EepromWriteJump + 0xC);
		/*dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");*/
		srcAddr = (u32)EepromWriteJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchWrite = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageWrite);
		*eepromPageWrite = patchWrite;
	
		u32* eepromPageProg = (u32*)((u32)EepromProgJump + 0xC);
		/*dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");*/
		srcAddr = (u32)EepromProgJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchProg = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageProg);
		*eepromPageProg = patchProg;
	
		u32* eepromPageVerify = (u32*)((u32)EepromVerifyJump + 0xC);
		/*dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");*/
		srcAddr = (u32)EepromVerifyJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchVerify = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageVerify);
		*eepromPageVerify = patchVerify;
	
		u32* eepromPageErase = (u32*)((u32)EepromEraseJump + 0x8);
		/*dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");*/
		srcAddr = (u32)EepromEraseJump + 0x8 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchErase = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageErase);
		*eepromPageErase = patchErase;
	}

	return 1;
}

u32 savePatchInvertedThumb(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	// dbg_printf("\nArm7 (patch kirby specific)\n");
	
	//u32* JumpTableFunc;
	u32* EepromReadJump;
	u32* EepromWriteJump;
	u32* EepromProgJump;
	u32* EepromVerifyJump;
	u32* EepromEraseJump;

    // inverted order
    EepromEraseJump = (u32*)findOffsetThumb(
    	(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
    		a7JumpTableSignatureUniversalThumb_pt3_alt3, 2
	);

    EepromVerifyJump = (u32*)findOffsetThumb(
		(u16*)EepromEraseJump + 2, ndsHeader->arm7binarySize,
		a7JumpTableSignatureUniversalThumb_pt2, 3
	);

    EepromProgJump = (u32*)findOffsetThumb(
		(u16*)EepromVerifyJump + 2, ndsHeader->arm7binarySize,
		a7JumpTableSignatureUniversalThumb_pt2, 3
	);

    EepromWriteJump = (u32*)findOffsetThumb(
		(u16*)EepromProgJump + 2, ndsHeader->arm7binarySize,
		a7JumpTableSignatureUniversalThumb_pt2, 3
	);

    EepromReadJump = (u32*)findOffsetThumb(
		(u16*)EepromWriteJump, ndsHeader->arm7binarySize,
		a7JumpTableSignatureUniversalThumb, 3
	);

	/* dbg_printf("usesThumb\n");
    dbg_printf("inverted order\n");
	dbg_printf("EepromEraseJump\n");
	dbg_hexa((u32)EepromEraseJump);
    dbg_printf("\n"); */

    //u32 srcAddr;

	u32* eepromRead = (u32*)((u32)EepromReadJump + 0xA);
	/* dbg_printf("Eeprom read :\t");
	dbg_hexa((u32)eepromRead);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromRead);
	dbg_printf("\n"); */
    *eepromRead = ce7->patches->arm7FunctionsDirect->eepromRead;
    /* dbg_printf("Eeprom read after:\t");
	dbg_hexa((u32)eepromRead);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromRead);
	dbg_printf("\n"); */

	u32* eepromPageWrite = (u32*)((u32)EepromWriteJump + 0xA);
	newSwiHaltAddr = *eepromPageWrite;
	newSwiHaltAddr -= 0x37F8000;
	newSwiHaltAddr += vAddrOfRelocSrc;
	newSwiHaltAddr--;
	/* dbg_printf("Eeprom page write:\t");
	dbg_hexa((u32)eepromPageWrite);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageWrite);
	dbg_printf("\n"); */
    *eepromPageWrite = ce7->patches->arm7FunctionsDirect->eepromPageWrite;
    /* dbg_printf("Eeprom page write after:\t");
	dbg_hexa((u32)eepromPageWrite);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageWrite);
	dbg_printf("\n"); */

	u32* eepromPageProg = (u32*)((u32)EepromProgJump + 0xA);
	/* dbg_printf("Eeprom page prog:\t");
	dbg_hexa((u32)eepromPageProg);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageProg);
	dbg_printf("\n"); */
    *eepromPageProg = ce7->patches->arm7FunctionsDirect->eepromPageProg;
    /* dbg_printf("Eeprom page prog after:\t");
	dbg_hexa((u32)eepromPageProg);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageProg);
	dbg_printf("\n"); */

	u32* eepromPageVerify = (u32*)((u32)EepromVerifyJump + 0xA);
	/* dbg_printf("Eeprom verify:\t");
	dbg_hexa((u32)eepromPageVerify);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageVerify);
	dbg_printf("\n"); */
    *eepromPageVerify = ce7->patches->arm7FunctionsDirect->eepromPageVerify;
    /* dbg_printf("Eeprom verify after:\t");
	dbg_hexa((u32)eepromPageVerify);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageVerify);
	dbg_printf("\n"); */

	u32* eepromPageErase = (u32*)((u32)EepromEraseJump + 0x8);
	/* dbg_printf("Eeprom page erase:\t");
	dbg_hexa((u32)eepromPageErase);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageErase);
	dbg_printf("\n"); */
    *eepromPageErase = ce7->patches->arm7FunctionsDirect->eepromPageErase;
    /* dbg_printf("Eeprom page erase after:\t");
	dbg_hexa((u32)eepromPageErase);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageErase);
	dbg_printf("\n"); */

    return 1;
}
