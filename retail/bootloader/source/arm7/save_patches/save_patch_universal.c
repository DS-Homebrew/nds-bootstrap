#include <nds/ndstypes.h>
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"

//
// Subroutine function signatures ARM7
//

static const u32 relocateStartSignature[1] = {0x027FFFFA};

static const u32 nextFunctiontSignature[1] = {0xE92D4000};

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


u32 savePatchUniversal(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, module_params_t* moduleParams, u32 saveFileCluster) {
	dbg_printf("\nArm7 (patch vAll)\n");
	
	bool usesThumb = false;
	int thumbType = 0;

	// Find the relocation signature
	u32 relocationStart = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		relocateStartSignature, 1
	);
	if (!relocationStart) {
		dbg_printf("Relocation start not found\n");
		return 0;
	}

    // Validate the relocation signature
	u32 forwardedRelocStartAddr = relocationStart + 4;
	if (!*(u32*)forwardedRelocStartAddr) {
		forwardedRelocStartAddr += 4;
	}
	u32 vAddrOfRelocSrc = *(u32*)(forwardedRelocStartAddr + 8);
	
	// Sanity checks
	u32 relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
	u32 relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
	if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
		dbg_printf("Error in relocation checking method 1\n");
		
		// Find the beginning of the next function
		u32 nextFunction = (u32)findOffset(
			(u32*)relocationStart, ndsHeader->arm7binarySize,
			nextFunctiontSignature, 1
		);
	
		// Validate the relocation signature
		forwardedRelocStartAddr = nextFunction - 0x14;
		
		// Validate the relocation signature
		vAddrOfRelocSrc = *(u32*)(nextFunction - 0xC);
		
		// Sanity checks
		relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
		relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
		if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
			dbg_printf("Error in relocation checking method 2\n");
			return 0;
		}
	}

	// Get the remaining details regarding relocation
	u32 valueAtRelocStart = *(u32*)forwardedRelocStartAddr;
	u32 relocDestAtSharedMem = *(u32*)valueAtRelocStart;
	if (relocDestAtSharedMem != 0x37F8000) { // Shared memory in RAM
		// Try again
		vAddrOfRelocSrc += *(u32*)(valueAtRelocStart + 4);
		relocDestAtSharedMem = *(u32*)(valueAtRelocStart + 0xC);
		if (relocDestAtSharedMem != 0x37F8000) {
			dbg_printf("Error in finding shared memory relocation area\n");
			return 0;
		}
	}

	dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");
	dbg_printf("Relocation dst: ");
	dbg_hexa(relocDestAtSharedMem);
	dbg_printf("\n");

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

	u32* JumpTableFunc;
	u32* EepromReadJump;
	u32* EepromWriteJump;
	u32* EepromProgJump;
	u32* EepromVerifyJump;
	u32* EepromEraseJump;
	
	JumpTableFunc = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		a7JumpTableSignatureUniversal, 3
	);
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
	} else {
		JumpTableFunc = findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2, 3
		);
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
			usesThumb = true;

			JumpTableFunc = (u32*)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb, 3
			);

			if (!JumpTableFunc) {
				return 0;
			}

			dbg_printf("usesThumb\n");
			dbg_printf("JumpTableFunc\n");
			dbg_hexa((u32)JumpTableFunc);
	
			EepromReadJump = (u32*)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb, 3
			);

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
			} else {
				// alternate v1 order
				thumbType = 1;

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
					a7JumpTableSignatureUniversalThumb_pt3_alt2, 2
				);
			}
		}
	}
	/*if (!JumpTableFunc) {
		return 0;
	}*/
		
	dbg_printf("\nJumpTableFunc: ");
	dbg_hexa((u32)JumpTableFunc);
	dbg_printf("\n");

	u32 srcAddr;
	
	if (usesThumb) {
		if (thumbType == 0) {
			u16* eepromRead = (u16*)((u32)EepromReadJump + 0x6);
			dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");
			srcAddr = (u32)EepromReadJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			const u16* patchRead = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromRead);
			eepromRead[0] = patchRead[0];
			eepromRead[1] = patchRead[1];
		
			u16* eepromPageWrite = (u16*)((u32)EepromWriteJump + 0x6);
			dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");
			srcAddr = (u32)EepromWriteJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			const u16* patchWrite = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageWrite);
			eepromPageWrite[0] = patchWrite[0];
			eepromPageWrite[1] = patchWrite[1];
	
			u16* eepromPageProg = (u16*)((u32)EepromProgJump + 0x6);
			dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");
			srcAddr = (u32)EepromProgJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			const u16* patchProg = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageProg);
			eepromPageProg[0] = patchProg[0];
			eepromPageProg[1] = patchProg[1];
	
			u16* eepromPageVerify = (u16*)((u32)EepromVerifyJump + 0x6);
			dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");
			srcAddr = (u32)EepromVerifyJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			const u16* patchVerify = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageVerify);
			eepromPageVerify[0] = patchVerify[0];
			eepromPageVerify[1] = patchVerify[1];

			u16* eepromPageErase = (u16*)((u32)EepromEraseJump + 0x4);
			dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");
			srcAddr = (u32)EepromEraseJump + 0x4 - vAddrOfRelocSrc + 0x37F8000;
			const u16* patchErase = generateA7InstrThumb(srcAddr, ce7->patches->arm7FunctionsThumb->eepromPageErase);
			eepromPageErase[0] = patchErase[0];
			eepromPageErase[1] = patchErase[1];
		} else {
			u32* eepromRead = (u32*)((u32)EepromReadJump + 0xA);
			dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");
			*eepromRead = ce7->patches->arm7FunctionsThumb->eepromRead;
			
			u32* eepromPageWrite = (u32*)((u32)EepromWriteJump + 0xA);
			dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");
			*eepromPageWrite = ce7->patches->arm7FunctionsThumb->eepromPageWrite;
			
			u32* eepromPageProg = (u32*)((u32)EepromProgJump + 0xA);
			dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");
			*eepromPageProg = ce7->patches->arm7FunctionsThumb->eepromPageProg;

			u32* eepromPageVerify = (u32*)((u32)EepromVerifyJump + 0xA);
			dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");
			*eepromPageVerify = ce7->patches->arm7FunctionsThumb->eepromPageVerify;
	
			u32* eepromPageErase = (u32*)((u32)EepromEraseJump + 0x8);
			dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");
			*eepromPageErase = ce7->patches->arm7FunctionsThumb->eepromPageErase;
		}
	} else {
		u32* eepromRead = (u32*)((u32)EepromReadJump + 0xC);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = (u32)EepromReadJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromRead);
		*eepromRead = patchRead;
	
		u32* eepromPageWrite = (u32*)((u32)EepromWriteJump + 0xC);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = (u32)EepromWriteJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchWrite = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageWrite);
		*eepromPageWrite = patchWrite;
	
		u32* eepromPageProg = (u32*)((u32)EepromProgJump + 0xC);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = (u32)EepromProgJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchProg = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageProg);
		*eepromPageProg = patchProg;
	
		u32* eepromPageVerify = (u32*)((u32)EepromVerifyJump + 0xC);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr = (u32)EepromVerifyJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchVerify = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageVerify);
		*eepromPageVerify = patchVerify;
	
		u32* eepromPageErase = (u32*)((u32)EepromEraseJump + 0x8);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = (u32)EepromEraseJump + 0x8 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchErase = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageErase);
		*eepromPageErase = patchErase;
	}

	ce7->patches->arm7Functions->saveCluster = saveFileCluster;

	return 1;
}

u32 savePatchInvertedThumb(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, module_params_t* moduleParams, u32 saveFileCluster) {
    dbg_printf("\nArm7 (patch kirby specific)\n");
	
	bool usesThumb = false;
	int thumbType = 0;

	// Find the relocation signature
	u32 relocationStart = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		relocateStartSignature, 1
	);
	if (!relocationStart) {
		dbg_printf("Relocation start not found\n");
		return 0;
	}

    // Validate the relocation signature
	u32 forwardedRelocStartAddr = relocationStart + 4;
	if (!*(u32*)forwardedRelocStartAddr) {
		forwardedRelocStartAddr += 4;
	}
	u32 vAddrOfRelocSrc = *(u32*)(forwardedRelocStartAddr + 8);
	
	// Sanity checks
	u32 relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
	u32 relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
	if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
		dbg_printf("Error in relocation checking method 1\n");
		
		// Find the beginning of the next function
		u32 nextFunction = (u32)findOffset(
			(u32*)relocationStart, ndsHeader->arm7binarySize,
			nextFunctiontSignature, 1
		);
	
		// Validate the relocation signature
		forwardedRelocStartAddr = nextFunction - 0x14;
		
		// Validate the relocation signature
		vAddrOfRelocSrc = *(u32*)(nextFunction - 0xC);
		
		// Sanity checks
		relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
		relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
		if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
			dbg_printf("Error in relocation checking method 2\n");
			return 0;
		}
	}

	// Get the remaining details regarding relocation
	u32 valueAtRelocStart = *(u32*)forwardedRelocStartAddr;
	u32 relocDestAtSharedMem = *(u32*)valueAtRelocStart;
	if (relocDestAtSharedMem != 0x37F8000) { // Shared memory in RAM
		// Try again
		vAddrOfRelocSrc += *(u32*)(valueAtRelocStart + 4);
		relocDestAtSharedMem = *(u32*)(valueAtRelocStart + 0xC);
		if (relocDestAtSharedMem != 0x37F8000) {
			dbg_printf("Error in finding shared memory relocation area\n");
			return 0;
		}
	}

	dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");
	dbg_printf("Relocation dst: ");
	dbg_hexa(relocDestAtSharedMem);
	dbg_printf("\n");

	u32* JumpTableFunc;
	u32* EepromReadJump;
	u32* EepromWriteJump;
	u32* EepromProgJump;
	u32* EepromVerifyJump;
	u32* EepromEraseJump;
	
    usesThumb = true;

    // inverted order
    EepromEraseJump = (u32*)findOffsetThumb(
    	(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
    		a7JumpTableSignatureUniversalThumb_pt3_alt2, 2
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

	dbg_printf("usesThumb\n");
    dbg_printf("inverted order\n");
	dbg_printf("EepromEraseJump\n");
	dbg_hexa((u32)EepromEraseJump);
    dbg_printf("\n");
    
    u32 srcAddr;
    
	u32* eepromRead = (u16*)((u32)EepromReadJump + 0xA);
	dbg_printf("Eeprom read :\t");
	dbg_hexa((u32)eepromRead);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromRead);
	dbg_printf("\n");
    *eepromRead = ce7->patches->arm7FunctionsThumb->eepromRead+1;
    dbg_printf("Eeprom read after:\t");
	dbg_hexa((u32)eepromRead);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromRead);
	dbg_printf("\n");

	u32* eepromPageWrite = (u16*)((u32)EepromWriteJump + 0xA);
	dbg_printf("Eeprom page write:\t");
	dbg_hexa((u32)eepromPageWrite);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageWrite);
	dbg_printf("\n");
    *eepromPageWrite = ce7->patches->arm7FunctionsThumb->eepromPageWrite+1;
    dbg_printf("Eeprom page write after:\t");
	dbg_hexa((u32)eepromPageWrite);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageWrite);
	dbg_printf("\n");

	u32* eepromPageProg = (u16*)((u32)EepromProgJump + 0xA);
	dbg_printf("Eeprom page prog:\t");
	dbg_hexa((u32)eepromPageProg);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageProg);
	dbg_printf("\n");
    *eepromPageProg = ce7->patches->arm7FunctionsThumb->eepromPageProg+1;
    dbg_printf("Eeprom page prog after:\t");
	dbg_hexa((u32)eepromPageProg);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageProg);
	dbg_printf("\n");

	u32* eepromPageVerify = (u16*)((u32)EepromVerifyJump + 0xA);
	dbg_printf("Eeprom verify:\t");
	dbg_hexa((u32)eepromPageVerify);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageVerify);
	dbg_printf("\n");
    *eepromPageVerify = ce7->patches->arm7FunctionsThumb->eepromPageVerify+1;
    dbg_printf("Eeprom verify after:\t");
	dbg_hexa((u32)eepromPageVerify);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageVerify);
	dbg_printf("\n");

	u16* eepromPageErase = (u16*)((u32)EepromEraseJump + 0xA);
	dbg_printf("Eeprom page erase:\t");
	dbg_hexa((u32)eepromPageErase);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageErase);
	dbg_printf("\n");  
    *eepromPageErase = ce7->patches->arm7FunctionsThumb->eepromPageErase+1;
    dbg_printf("Eeprom page erase after:\t");
	dbg_hexa((u32)eepromPageErase);
    dbg_printf("\t:\t");
    dbg_hexa(*eepromPageErase);
	dbg_printf("\n");
	
	ce7->patches->arm7Functions->saveCluster = saveFileCluster;
    
    return 1;
}
