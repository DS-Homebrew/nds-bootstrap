#include <nds/ndstypes.h>
#include "module_params.h"
#include "patch.h"
#include "find.h"
#include "cardengine_header_arm7.h"
#include "debug_file.h"

//
// Subroutine function signatures ARM7
//

static const u32 relocateStartSignature[1]    = {0x3381C0DE}; //  33 81 C0 DE  DE C0 81 33 00 00 00 00 is the marker for the beggining of the relocated area :-)
static const u32 relocateStartSignatureAlt[1] = {0x2106C0DE};

static const u32 relocateValidateSignature[1] = {0x400010C};

static const u32 a7JumpTableSignatureUniversal[3]      = {0xE592000C, 0xE5921010, 0xE5922014};
static const u32 a7JumpTableSignatureUniversal_2[3]    = {0xE593000C, 0xE5931010, 0xE5932014};
static const u16 a7JumpTableSignatureUniversalThumb[4] = {0x6822, 0x68D0, 0x6911, 0x6952};

u32 savePatchV5 (const tNDSHeader* ndsHeader, const cardengineArm7* ce7, module_params_t* moduleParams, u32 saveFileCluster, u32 saveSize) {
    //dbg_printf("\nArm7 (patch vAll)\n");
	dbg_printf("\nArm7 (patch v5)\n");

	// Find the relocation signature
    u32 relocationStart = (u32)findOffset(
        (u32*)ndsHeader->arm7destination, 0x800,
        relocateStartSignature, 1
    );
    if (!relocationStart) {
        dbg_printf("Relocation start not found. Trying alt\n");
		relocationStart = (u32)findOffset(
            (u32*)ndsHeader->arm7destination, 0x800,
            relocateStartSignatureAlt, 1
        );
		if (relocationStart>0) relocationStart += 0x28;
	}
    if (!relocationStart) {
        dbg_printf("Relocation start alt not found\n");
		return 0;
    }

	// Validate the relocation signature
    u32 vAddrOfRelocSrc = relocationStart + 0x8;
    // sanity checks
    u32 relocationCheck = (u32)findOffset(
        (u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
        relocateValidateSignature, 1
    );
    u32 relocationCheck2 =
        *(u32*)(relocationCheck - 0x4);

    if (relocationCheck + 0xC - vAddrOfRelocSrc + 0x37F8000 > relocationCheck2) {
        dbg_printf("Error in relocation checking\n");
        dbg_hexa(relocationCheck + 0xC - vAddrOfRelocSrc + 0x37F8000);
        dbg_hexa(relocationCheck2);
        
		vAddrOfRelocSrc =  relocationCheck + 0xC - relocationCheck2 + 0x37F8000;
        dbg_printf("vAddrOfRelocSrc\n");
        dbg_hexa(vAddrOfRelocSrc); 
    }

    dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");

	bool usesThumb = false;

	u32 JumpTableFunc = (u32)findOffset(
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
		JumpTableFunc = (u32)findOffsetThumb(
            (u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		    a7JumpTableSignatureUniversalThumb, 4
        );
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
		srcAddr = JumpTableFunc - 0xE  - vAddrOfRelocSrc + 0x37F8000 ;
		u16* patchCardRead = generateA7InstrThumb(srcAddr, ce7->patches->arm7_functions_thumb->card_read);
		cardRead[0] = patchCardRead[0];
        cardRead[1] = patchCardRead[1]; */

		u16* eepromRead = (u16*) (JumpTableFunc + 0x8);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x8  - vAddrOfRelocSrc + 0x37F8000 ;
		u16* patchRead = generateA7InstrThumb(srcAddr, ce7->patches->arm7_functions_thumb->eeprom_read);
		eepromRead[0] = patchRead[0];
		eepromRead[1] = patchRead[1];
		
		u16* eepromPageWrite = (u16*) (JumpTableFunc + 0x16);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x16 - vAddrOfRelocSrc + 0x37F8000 ;
		u16* patchWrite = generateA7InstrThumb(srcAddr, ce7->patches->arm7_functions_thumb->eeprom_page_write);
        eepromPageWrite[0] = patchWrite[0];
		eepromPageWrite[1] = patchWrite[1];

		u16* eepromPageProg = (u16*) (JumpTableFunc + 0x24);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x24 - vAddrOfRelocSrc + 0x37F8000 ;
		u16* patchProg = generateA7InstrThumb(srcAddr, ce7->patches->arm7_functions_thumb->eeprom_page_prog);
        eepromPageProg[0] = patchProg[0];
		eepromPageProg[1] = patchProg[1];

		u16* eepromPageVerify = (u16*) (JumpTableFunc + 0x32);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x32 - vAddrOfRelocSrc + 0x37F8000 ;
		u16* patchVerify = generateA7InstrThumb(srcAddr, ce7->patches->arm7_functions_thumb->eeprom_page_verify);
        eepromPageVerify[0] = patchVerify[0];
		eepromPageVerify[1] = patchVerify[1];


		u16* eepromPageErase = (u16*) (JumpTableFunc + 0x3E);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x3E - vAddrOfRelocSrc + 0x37F8000 ;
		u16* patchErase = generateA7InstrThumb(srcAddr, ce7->patches->arm7_functions_thumb->eeprom_page_erase);        
        eepromPageErase[0] = patchErase[0];
		eepromPageErase[1] = patchErase[1];

	} else {
        u32* cardRead = (u32*) (JumpTableFunc - 0x18);
		dbg_printf("card read:\t");
		dbg_hexa((u32)cardRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc - 0x18  - vAddrOfRelocSrc + 0x37F8000 ;
		u32 patchCardRead = generateA7Instr(srcAddr, ce7->patches->arm7_functions->card_read);
		*cardRead=patchCardRead;

		u32* eepromRead = (u32*) (JumpTableFunc + 0xC);
		dbg_printf("Eeprom read:\t");
		dbg_hexa((u32)eepromRead);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0xC  - vAddrOfRelocSrc + 0x37F8000 ;
		u32 patchRead = generateA7Instr(srcAddr, ce7->patches->arm7_functions->eeprom_read);
		*eepromRead=patchRead;

		u32* eepromPageWrite = (u32*) (JumpTableFunc + 0x24);
		dbg_printf("Eeprom page write:\t");
		dbg_hexa((u32)eepromPageWrite);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x24 - vAddrOfRelocSrc + 0x37F8000 ;
		u32 patchWrite = generateA7Instr(srcAddr, ce7->patches->arm7_functions->eeprom_page_write);
		*eepromPageWrite=patchWrite;

		u32* eepromPageProg = (u32*) (JumpTableFunc + 0x3C);
		dbg_printf("Eeprom page prog:\t");
		dbg_hexa((u32)eepromPageProg);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x3C - vAddrOfRelocSrc + 0x37F8000 ;
		u32 patchProg = generateA7Instr(srcAddr, ce7->patches->arm7_functions->eeprom_page_prog);
		*eepromPageProg=patchProg;

		u32* eepromPageVerify = (u32*) (JumpTableFunc + 0x54);
		dbg_printf("Eeprom verify:\t");
		dbg_hexa((u32)eepromPageVerify);
		dbg_printf("\n");
		srcAddr =  JumpTableFunc + 0x54 - vAddrOfRelocSrc + 0x37F8000 ;
		u32 patchVerify = generateA7Instr(srcAddr, ce7->patches->arm7_functions->eeprom_page_verify);
		*eepromPageVerify=patchVerify;


		u32* eepromPageErase = (u32*) (JumpTableFunc + 0x68);
		dbg_printf("Eeprom page erase:\t");
		dbg_hexa((u32)eepromPageErase);
		dbg_printf("\n");
		srcAddr = JumpTableFunc + 0x68 - vAddrOfRelocSrc + 0x37F8000 ;
		u32 patchErase = generateA7Instr(srcAddr, ce7->patches->arm7_functions->eeprom_page_erase);
		*eepromPageErase=patchErase; 

	}
	ce7->patches->arm7_functions->save_cluster = saveFileCluster;
	ce7->patches->arm7_functions->save_size    = saveSize;

	return 1;
}
