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

static const u32 a7cardReadSignature[2] = {0x04100010, 0x040001A4};

static const u32 a7something2Signature[2] = {0x0000A040, 0x040001A0};

u32 savePatchV1(const cardengineArm7* ce7, const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 saveFileCluster) {
	//dbg_printf("\nArm7 (patch v1.0)\n");
	dbg_printf("\nArm7 (patch v1)\n");

	// Find the card read
	u32 cardReadEndAddr = patchOffsetCache.a7CardReadEndOffset;
	if (!patchOffsetCache.a7CardReadEndOffset) {
		cardReadEndAddr = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, 0x00020000, 
			a7cardReadSignature, 2
		);
		
		if (cardReadEndAddr) {
			patchOffsetCache.a7CardReadEndOffset = cardReadEndAddr;
		}
	}
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

	u32 specificWramAddr = *(u32*)(JumpTableFunc + 0x10);
	// if out of specific ram range...
	if (specificWramAddr < 0x37F8000 || specificWramAddr > 0x380FFFF) {
		dbg_printf("Retry the search\n");
		JumpTableFunc = (u32)findOffset(
			(u32*)JumpTableFunc, 0x18000 - JumpTableFunc,
			&cardstructAddr, 1
		) + 4;
		dbg_printf("JumpTableFunc: ");
		dbg_hexa(JumpTableFunc);
		dbg_printf("\n");	  
		specificWramAddr = *(u32*)(JumpTableFunc + 0x10);
		if (specificWramAddr < 0x37F8000 || specificWramAddr > 0x380FFFF) {
			return 0;
		}
	}

	dbg_printf("specificWramAddr: ");
	dbg_hexa(specificWramAddr);
	dbg_printf("\n");

	u32 someAddr_799C = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, 0x18000,
		a7something2Signature, 2
	);
	if (!someAddr_799C) {
		dbg_printf("[Error!] ___ someOffset not found\n");
		return 0;
	}
	dbg_printf("someAddr_799C: ");
	dbg_hexa(someAddr_799C);
	dbg_printf("\n");

	u32* eepromPageErase = (u32*)(JumpTableFunc + 0x10);
	dbg_printf("Eeprom page erase:\t");
	dbg_hexa((u32)eepromPageErase);
	dbg_printf("\n");
	*eepromPageErase = ce7->patches->arm7FunctionsDirect->eepromPageErase;

	u32* eepromPageVerify = (u32*)(JumpTableFunc + 0x2C);
	dbg_printf("Eeprom verify:\t");
	dbg_hexa((u32)eepromPageVerify);
	dbg_printf("\n");
	*eepromPageVerify = ce7->patches->arm7FunctionsDirect->eepromPageVerify;

	u32* eepromPageWrite = (u32*)(JumpTableFunc + 0x48);
	dbg_printf("Eeprom page write:\t");
	dbg_hexa((u32)eepromPageWrite);
	dbg_printf("\n");
	*eepromPageWrite = ce7->patches->arm7FunctionsDirect->eepromPageWrite;

	u32* eepromPageProg = (u32*)(JumpTableFunc + 0x64);
	dbg_printf("Eeprom page prog:\t");
	dbg_hexa((u32)eepromPageProg);
	dbg_printf("\n");
	*eepromPageProg = ce7->patches->arm7FunctionsDirect->eepromPageProg;

	u32* eepromRead = (u32*)(JumpTableFunc + 0x80);
	dbg_printf("Eeprom read:\t");
	dbg_hexa((u32)eepromRead);
	dbg_printf("\n");
	*eepromRead = ce7->patches->arm7FunctionsDirect->eepromRead;

	u32* cardRead = (u32*)(JumpTableFunc + 0xA0);
	dbg_printf("Card read:\t");
	dbg_hexa((u32)cardRead);
	dbg_printf("\n");
	*cardRead = ce7->patches->arm7FunctionsDirect->cardRead;

	// different patch for card id
	u32* cardId = (u32*)(JumpTableFunc + 0xAC);
	dbg_printf("Card id:\t");
	dbg_hexa((u32)cardId);
	dbg_printf("\n");
	u32 srcAddr = JumpTableFunc + 0xAC - vAddrOfRelocSrc + relocDestAtSharedMem;
	u32 patchCardID = generateA7Instr(srcAddr, ce7->patches->arm7Functions->cardId);
	*cardId = patchCardID;

	u32 anotherWramAddr = *(u32*)(JumpTableFunc + 0xD0);
	if (anotherWramAddr > 0x37F7FFF && anotherWramAddr < 0x3810000) {
		u32* current = (u32*)(JumpTableFunc + 0xD0);
		dbg_printf("Identify backup:\t\t\t");
		dbg_hexa((u32)current);
		dbg_printf("\n");
        // TODO : maybe write a specfic patch for Identify backup 
		*current = ce7->patches->arm7FunctionsDirect->eepromProtect;
	}


	ce7->patches->arm7Functions->saveCluster = saveFileCluster;

	return 1;
}
