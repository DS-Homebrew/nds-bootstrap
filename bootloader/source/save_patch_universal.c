#include "save_patch.h"

u32 relocateStartSignature[1] = {0x027FFFFA};
u32 nextFunctiontSignature[1] = {0xE92D4000};
u32 a7cardReadSignature[2]    = {0x04100010, 0x040001A4};
u32 a7something2Signature[2]  = {0x0000A040, 0x040001A0};

u32 a7JumpTableSignatureUniversal[3]               = {0xE592000C, 0xE5921010, 0xE5922014};
u32 a7JumpTableSignatureUniversal_pt2[3]           = {0xE5920010, 0xE592100C, 0xE5922014};
u32 a7JumpTableSignatureUniversal_pt3[2]           = {0xE5920010, 0xE5921014};
u32 a7JumpTableSignatureUniversal_2[3]             = {0xE593000C, 0xE5931010, 0xE5932014};
u32 a7JumpTableSignatureUniversal_2_pt2[3]         = {0xE5930010, 0xE593100C, 0xE5932014};
u32 a7JumpTableSignatureUniversal_2_pt3[2]         = {0xE5930010, 0xE5931014};
u16 a7JumpTableSignatureUniversalThumb[3]          = {0x68D0, 0x6911, 0x6952};
u16 a7JumpTableSignatureUniversalThumb_pt2[3]      = {0x6910, 0x68D1, 0x6952};
u16 a7JumpTableSignatureUniversalThumb_pt3[2]      = {0x6908, 0x6949};
u16 a7JumpTableSignatureUniversalThumb_pt3_alt[2]  = {0x6910, 0x6951};
u16 a7JumpTableSignatureUniversalThumb_pt3_alt2[2] = {0x6800, 0x6900};

u32 savePatchUniversal(const tNDSHeader* ndsHeader, u32* cardEngineLocation, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {
	dbg_printf("\nArm7 (patch vAll)\n");
	
	bool usesThumb = false;
	int thumbType = 0;

	// Find the relocation signature
	u32 relocationStart = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		relocateStartSignature, 1,
		1
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
			nextFunctiontSignature, 1,
			1
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
		(u32*)ndsHeader->arm7destination, 0x00400000, 
		a7cardReadSignature, 2,
		1
	);
	if (!cardReadEndAddr) {
		dbg_printf("[Error!] Card read addr not found\n");
		return 0;
	}
	dbg_printf("cardReadEndAddr: ");
	dbg_hexa(cardReadEndAddr);
	dbg_printf("\n");*/

	u32 JumpTableFunc;
	u32 EepromReadJump;
	u32 EepromWriteJump;
	u32 EepromProgJump;
	u32 EepromVerifyJump;
	u32 EepromEraseJump;
	
	JumpTableFunc = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		a7JumpTableSignatureUniversal, 3,
		1
	);
	if (JumpTableFunc) {
		EepromReadJump = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal, 3,
			1
		);
		EepromWriteJump = (u32)findOffset(
			(u32*)EepromReadJump + 4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_pt2, 3,
			1
		);
		EepromProgJump = (u32)findOffset(
			(u32*)EepromWriteJump + 4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_pt2, 3,
			1
		);
		EepromVerifyJump = (u32)findOffset(
			(u32*)EepromProgJump + 4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_pt2, 3,
			1
		);
		EepromEraseJump = (u32)findOffset(
			(u32*)EepromVerifyJump + 4, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_pt3, 2,
			1
		);
	} else {
		JumpTableFunc = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
			a7JumpTableSignatureUniversal_2, 3,
			1
		);
		if (JumpTableFunc) {
			EepromReadJump = (u32)findOffset(
				(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2, 3,
				1
			);
			EepromWriteJump = (u32)findOffset(
				(u32*)EepromReadJump+4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt2, 3,
				1
			);
			EepromProgJump = (u32)findOffset(
				(u32*)EepromWriteJump+4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt2, 3,
				1
			);
			EepromVerifyJump = (u32)findOffset(
				(u32*)EepromProgJump+4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt2, 3,
				1
			);
			EepromEraseJump = (u32)findOffset(
				(u32*)EepromVerifyJump+4, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversal_2_pt3, 2,
				1
			);
		} else {
			usesThumb = true;

			JumpTableFunc = (u32)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb, 3,
				1
			);

			if (!JumpTableFunc) {
				return 0;
			}

			dbg_printf("usesThumb");
			dbg_printf("JumpTableFunc");
			dbg_hexa(JumpTableFunc);
	
			EepromReadJump = (u32)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb, 3,
				1
			);

			EepromWriteJump = (u32)findOffsetThumb(
				(u16*)EepromReadJump+2, ndsHeader->arm7binarySize,
				a7JumpTableSignatureUniversalThumb_pt2, 3,
				1
			);
			if (EepromWriteJump) {
				EepromProgJump = (u32)findOffsetThumb(
					(u16*)EepromWriteJump + 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 3,
					1
				);
				EepromVerifyJump = (u32)findOffsetThumb(
					(u16*)EepromProgJump + 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 3,
					1
				);
				EepromEraseJump = (u32)findOffsetThumb(
					(u16*)EepromVerifyJump + 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt3, 2,
					1
				);
				if (!EepromEraseJump) {
					EepromEraseJump = (u32)findOffsetThumb(
						(u16*)EepromVerifyJump + 2, ndsHeader->arm7binarySize,
						a7JumpTableSignatureUniversalThumb_pt3_alt, 2,
						1
					);
				}
			} else {
				// alternate v1 order
				thumbType = 1;

				EepromProgJump = (u32)findOffsetThumb(
					(u16*)JumpTableFunc, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 2,
					-1
				);
				EepromWriteJump = (u32)findOffsetThumb(
					(u16*)EepromProgJump - 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 2,
					-1
				);
				EepromVerifyJump = (u32)findOffsetThumb(
					(u16*)EepromWriteJump - 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt2, 2,
					-1
				);
				EepromEraseJump = (u32)findOffsetThumb(
					(u16*)EepromVerifyJump - 2, ndsHeader->arm7binarySize,
					a7JumpTableSignatureUniversalThumb_pt3_alt2, 2,
					-1
				);
			}
		}
	}
	/*if (!JumpTableFunc) {
		return 0;
	}*/
		
	dbg_printf("JumpTableFunc: ");
	dbg_hexa(JumpTableFunc);
	dbg_printf("\n");

	u32* patches           = (u32*)cardEngineLocation[0];
	u32* arm7Function      = (u32*)patches[9];
	u32* arm7FunctionThumb = (u32*)patches[14];
	u32 srcAddr;
	
	if (usesThumb) {
		if (thumbType == 1) {
			u16* eepromRead = (u16*)(EepromReadJump + 0x6);
			dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");
			srcAddr = EepromReadJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			u16* patchRead = generateA7InstrThumb(srcAddr, arm7FunctionThumb[5]);
			eepromRead[0] = patchRead[0];
			eepromRead[1] = patchRead[1];
		
			u16* eepromPageWrite = (u16*)(EepromWriteJump + 0x6);
			dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");
			srcAddr = EepromWriteJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			u16* patchWrite = generateA7InstrThumb(srcAddr, arm7FunctionThumb[3]);
			eepromPageWrite[0] = patchWrite[0];
			eepromPageWrite[1] = patchWrite[1];
	
			u16* eepromPageProg = (u16*)(EepromProgJump + 0x6);
			dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");
			srcAddr = EepromProgJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			u16* patchProg = generateA7InstrThumb(srcAddr, arm7FunctionThumb[4]);
			eepromPageProg[0] = patchProg[0];
			eepromPageProg[1] = patchProg[1];
	
			u16* eepromPageVerify = (u16*)(EepromVerifyJump + 0x6);
			dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");
			srcAddr = EepromVerifyJump + 0x6 - vAddrOfRelocSrc + 0x37F8000;
			u16* patchVerify = generateA7InstrThumb(srcAddr, arm7FunctionThumb[2]);
			eepromPageVerify[0] = patchVerify[0];
			eepromPageVerify[1] = patchVerify[1];

			u16* eepromPageErase = (u16*) (EepromEraseJump + 0x4);
			dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");
			srcAddr = EepromEraseJump + 0x4 - vAddrOfRelocSrc + 0x37F8000;
			u16* patchErase = generateA7InstrThumb(srcAddr, arm7FunctionThumb[1]);
			eepromPageErase[0] = patchErase[0];
			eepromPageErase[1] = patchErase[1];
		} else {
			u32* eepromRead = (u32*)(EepromReadJump + 0xA);
			dbg_printf("Eeprom read:\t");
			dbg_hexa((u32)eepromRead);
			dbg_printf("\n");
			*eepromRead = arm7FunctionThumb[5];
			
			u32* eepromPageWrite = (u32*)(EepromWriteJump + 0xA);
			dbg_printf("Eeprom page write:\t");
			dbg_hexa((u32)eepromPageWrite);
			dbg_printf("\n");
			*eepromPageWrite = arm7FunctionThumb[3];
			
			u32* eepromPageProg = (u32*)(EepromProgJump + 0xA);
			dbg_printf("Eeprom page prog:\t");
			dbg_hexa((u32)eepromPageProg);
			dbg_printf("\n");
			*eepromPageProg = arm7FunctionThumb[4];

			u32* eepromPageVerify = (u32*)(EepromVerifyJump + 0xA);
			dbg_printf("Eeprom verify:\t");
			dbg_hexa((u32)eepromPageVerify);
			dbg_printf("\n");
			*eepromPageVerify = arm7FunctionThumb[2];
	
			u32* eepromPageErase = (u32*)(EepromEraseJump + 0x8);
			dbg_printf("Eeprom page erase:\t");
			dbg_hexa((u32)eepromPageErase);
			dbg_printf("\n");
			*eepromPageErase = arm7FunctionThumb[1];
		}
	} else {
		u32* eepromRead = (u32*)(EepromReadJump + 0xC);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = EepromReadJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchRead = generateA7Instr(srcAddr, arm7Function[5]);
		*eepromRead = patchRead;
	
		u32* eepromPageWrite = (u32*)(EepromWriteJump + 0xC);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = EepromWriteJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchWrite = generateA7Instr(srcAddr, arm7Function[3]);
		*eepromPageWrite = patchWrite;
	
		u32* eepromPageProg = (u32*)(EepromProgJump + 0xC);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = EepromProgJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchProg = generateA7Instr(srcAddr, arm7Function[4]);
		*eepromPageProg = patchProg;
	
		u32* eepromPageVerify = (u32*)(EepromVerifyJump + 0xC);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr = EepromVerifyJump + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchVerify = generateA7Instr(srcAddr, arm7Function[2]);
		*eepromPageVerify = patchVerify;
	
		u32* eepromPageErase = (u32*)(EepromEraseJump + 0x8);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = EepromEraseJump + 0x8 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchErase = generateA7Instr(srcAddr, arm7Function[1]);
		*eepromPageErase = patchErase;
	}

	arm7Function[8] = saveFileCluster;
	arm7Function[9] = saveSize;

	return 1;
}
