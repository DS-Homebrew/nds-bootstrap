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

static const u32 a7cardReadSignature[2] = {0x04100010, 0x040001A4};

static const u32 a7something1Signature[2] = {0xE350000C, 0x908FF100};
static const u32 a7something2Signature[2] = {0x0000A040, 0x040001A0};

u32 savePatchV2(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster) {
	//dbg_printf("\nArm7 (patch v2.0)\n");
	dbg_printf("\nArm7 (patch v2)\n");

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
	if (!*(u32*)forwardedRelocStartAddr || *(u32*)forwardedRelocStartAddr < 0x02000000 || *(u32*)forwardedRelocStartAddr > 0x03000000) {
		forwardedRelocStartAddr += 4;
	}
	u32 vAddrOfRelocSrc = *(u32*)(forwardedRelocStartAddr + 8);

	// Sanity checks
	u32 relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
	u32 relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
	if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
		dbg_printf("Error in relocation checking method 1\n");
		
		// Found the beginning of the next function
		u32 nextFunction = (u32)findOffset(
			(u32*)relocationStart, ndsHeader->arm7binarySize,
			  nextFunctiontSignature, 1
		);
	
		   // Validate the relocation signature
		forwardedRelocStartAddr = nextFunction - 0x14;
		
		// Validate the relocation signature
		vAddrOfRelocSrc = *(u32*)(nextFunction - 0xC);
		
		// sanity checks
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
	if (relocDestAtSharedMem != 0x37F8000) { // shared memory in RAM
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
	u32 cardReadEndAddr = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, 0x00400000, 
		a7cardReadSignature, 2
	);
	if (!cardReadEndAddr) {
		dbg_printf("[Error!] Card read addr not found\n");
		return 0;
	}
	dbg_printf("cardReadEndAddr: ");
	dbg_hexa(cardReadEndAddr);
	dbg_printf("\n");

	// Nonsense variable names below
	u32 cardstructAddr = *(u32*)(cardReadEndAddr - 4);
	dbg_printf("cardstructAddr: ");
	dbg_hexa(cardstructAddr);
	dbg_printf("\n");

	u32 readCacheEnd = (u32)findOffset(
		(u32*)cardReadEndAddr, 0x18000 - cardReadEndAddr,
		&cardstructAddr, 1
	);
	dbg_printf("readCacheEnd: ");
	dbg_hexa(readCacheEnd);
	dbg_printf("\n");

	if (!readCacheEnd) {
		dbg_printf("[Error!] ___ addr not found\n");
		return 0;
	}

	u32 JumpTableFunc = readCacheEnd + 4;
	dbg_printf("JumpTableFunc: ");
	dbg_hexa(JumpTableFunc);
	dbg_printf("\n");

	//
	// Here is where the differences in the retry begin
	//

	u32 returned_A0_with_MKDS = (u32)findOffset(
		(u32*)JumpTableFunc, 0x100,
		a7something1Signature, 2
	);
	if (!returned_A0_with_MKDS) {
		dbg_printf("[Error!]...\n");
		return 0;
	}

	dbg_printf("returned_A0_with_MKDS: ");
	dbg_hexa(returned_A0_with_MKDS);
	dbg_printf("\n");

	u32 addrOfSomething_85C0 = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, 0x18000,
		a7something2Signature, 2
	);
	if (!addrOfSomething_85C0) {
		dbg_printf("[Error!] ...\n");
		return 0;
	}

	dbg_printf("addrOfSomething_85C0: ");
	dbg_hexa(addrOfSomething_85C0);
	dbg_printf("\n");

	u32 anotherLocinA7WRAM = *(u32*)(addrOfSomething_85C0 - 4);

	dbg_printf("anotherLocinA7WRAM: ");
	dbg_hexa(anotherLocinA7WRAM);
	dbg_printf("\n");

	u32 amal_8CBC = returned_A0_with_MKDS;

	dbg_printf("amal_8CBC: ");
	dbg_hexa((u32)amal_8CBC);
	dbg_printf("\n");

	// no, no idea what this is yet
	// and no idea how to cleanly fix this warning yet.
	// but it should be (in MKDS), 0x7F54
	u8* aFinalLocation = (u8*)(
		JumpTableFunc
		+ 4 * (*(u32*)(JumpTableFunc + 0x38) & 0xFFFFFF)
		+ 0x48
		+ 4 * (*(u32*)((
					4 * (*(u32*)(JumpTableFunc + 0x38) & 0xFFFFFF) + 0x48
				) + JumpTableFunc) | 0xFF000000
			)
		+ 8
	);

	dbg_printf("aFinalLocation: ");
	dbg_hexa((u32)aFinalLocation);
	dbg_printf("\n");

	u32 srcAddr;

	u32* eepromProtect = (u32*)(JumpTableFunc + 0xE0);
	u32* cardRead      = (u32*)(JumpTableFunc + 0x108);
	if ((*eepromProtect & 0xFF000000) == 0xEB000000 && (*cardRead & 0xFF000000) == 0xEB000000) {
		dbg_printf("Eeprom protect:\t");
		dbg_hexa((u32)eepromProtect);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xE0 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchProtect = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromProtect);
		*eepromProtect = patchProtect;

		u32* cardId = (u32*)(JumpTableFunc + 0xE8);
		dbg_printf("Card id:\t");
		dbg_hexa((u32)cardId);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xE8 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchCardId = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardId);
		*cardId = patchCardId;

		dbg_printf("Card read:\t");
		dbg_hexa((u32)cardRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x108 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchCardRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardRead);
		*cardRead = patchCardRead;

		u32* eepromRead = (u32*) (JumpTableFunc + 0x120);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x120 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromRead);
		*eepromRead = patchRead;

		u32* eepromPageWrite = (u32*)(JumpTableFunc + 0x138);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x138 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchWrite = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageWrite);
		*eepromPageWrite = patchWrite;

		u32* eepromPageProg = (u32*)(JumpTableFunc + 0x150);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x150 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchProg = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageProg);
		*eepromPageProg = patchProg;

		u32* eepromPageVerify = (u32*)(JumpTableFunc + 0x168);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x168 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchVerify = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageVerify);
		*eepromPageVerify = patchVerify;

		u32* eepromPageErase = (u32*) (JumpTableFunc + 0x178);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x178 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchErase = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageErase);
		*eepromPageErase = patchErase;

		ce7->patches->arm7Functions->saveCluster = saveFileCluster;
	} else {
		dbg_printf("[Warning] Eeprom protect not found \n");

		cardRead = (u32*)(JumpTableFunc + 0x100);
		if ((*cardRead & 0xFF000000) != 0xEB000000) {
			dbg_printf("[Error] CardRead not found:\n");
			dbg_hexa((u32)cardRead);
			dbg_printf("\n");
			return 0;
		}

		u32* cardId = (u32*)(JumpTableFunc + 0xE0);
		dbg_printf("Card id:\t");
		dbg_hexa((u32)cardId);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xE0 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchCardId = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardId);
		*cardId = patchCardId;

		dbg_printf("Card read:\t");
		dbg_hexa((u32)cardRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x100 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchCardRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardRead);
		*cardRead = patchCardRead;

		u32* eepromRead = (u32*)(JumpTableFunc + 0x118);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x118 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchRead = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromRead);
		*eepromRead = patchRead;

		u32* eepromPageWrite = (u32*)(JumpTableFunc + 0x130);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x130 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchWrite = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageWrite);
		*eepromPageWrite = patchWrite;

		u32* eepromPageProg = (u32*)(JumpTableFunc + 0x148);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x148 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchProg = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageProg);
		*eepromPageProg = patchProg;

		u32* eepromPageVerify = (u32*)(JumpTableFunc + 0x160);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x160 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchVerify = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageVerify);
		*eepromPageVerify = patchVerify;

		u32* eepromPageErase = (u32*)(JumpTableFunc + 0x170);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x170 - vAddrOfRelocSrc + relocDestAtSharedMem;
		u32 patchErase = generateA7Instr(srcAddr, ce7->patches->arm7Functions->eepromPageErase);
		*eepromPageErase = patchErase;

		ce7->patches->arm7Functions->saveCluster = saveFileCluster;
	}    

	return 1;
}
