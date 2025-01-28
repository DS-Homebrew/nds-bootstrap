#include <stddef.h> // NULL
#include "patch.h"
#include "find.h"
#include "debug_file.h"

extern bool dsiModeConfirmed;
extern u32 newArm7binarySize;
extern u32 newArm7ibinarySize;

//
// Subroutine function signatures ARM7
//

static const u32 relocateStartSignature[1]      = {0x027FFFFA};
static const u32 relocateStartSignature5[1]     = {0x3381C0DE}; //  33 81 C0 DE  DE C0 81 33 00 00 00 00 is the marker for the beggining of the relocated area :-)
static const u32 relocateStartSignature5Alt[1]  = {0x2106C0DE};
static const u32 relocateStartSignature5Alt2[1] = {0x02FFFFFA};

static const u32 nextFunctiontSignature[1] = {0xE92D4000};
static const u32 relocateValidateSignature[1] = {0x400010C};

static const u32 swiHaltSignature1[1] = {0xE59FC004};
static const u32 swiHaltSignature2[1] = {0xE59FC000};
static const u16 swiHaltCmpSignature[1] = {0x2800};
static const u16 swiHaltMovSignature[1] = {0x200C};
static const u32 swiHaltConstSignature[1] = {0x4000004};
static const u32 swiHaltConstSignatureAlt[1] = {0x4000208};

static const u32 swi12Signature[1] = {0x4770DF12}; // LZ77UnCompReadByCallbackWrite16bit
static const u16 swi24Signature[2] = {0xDF24,0x4770}; // SHA1_Init
static const u16 swi25Signature[2] = {0xDF25,0x4770}; // SHA1_Update
static const u16 swi26Signature[2] = {0xDF26,0x4770}; // SHA1_Finish
static const u16 swi27Signature[2] = {0xDF27,0x4770}; // SHA1_Init_update_fin

static const u32 scfgExtSignature[1] = {0x4004008};

static const u16 swiGetPitchTableSignatureThumb[4]    = {0xB570, 0x1C05, 0x2400, 0x4248};
static const u16 swiGetPitchTableSignatureThumbAlt[4] = {0xB570, 0x1C05, 0x4248, 0x2103};
static const u32 swiGetPitchTableSignature1[4]      = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004721};
static const u32 swiGetPitchTableSignature1Alt1[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004ACD};
static const u32 swiGetPitchTableSignature1Alt2[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BB9};
static const u32 swiGetPitchTableSignature1Alt3[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BC9};
static const u32 swiGetPitchTableSignature1Alt4[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004BE5};
static const u32 swiGetPitchTableSignature1Alt5[4]  = {0xE59FC004, 0xE08FC00C, 0xE12FFF1C, 0x00004CA5};
static const u32 swiGetPitchTableSignature1Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x038039D5};
static const u32 swiGetPitchTableSignature1Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803BE9};
static const u32 swiGetPitchTableSignature1Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E05};
static const u32 swiGetPitchTableSignature1Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803E09};
static const u32 swiGetPitchTableSignature1Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03803F21};
static const u32 swiGetPitchTableSignature1Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x03804189};
static const u32 swiGetPitchTableSignature1Alt12[3] = {0xE59FC000, 0xE12FFF1C, 0x038049D5};
static const u32 swiGetPitchTableSignature1Alt13[3] = {0xE59FC000, 0xE12FFF1C, 0x03804BE9};
static const u32 swiGetPitchTableSignature1Alt14[3] = {0xE59FC000, 0xE12FFF1C, 0x03804E35};
static const u32 swiGetPitchTableSignature1Alt15[3] = {0xE59FC000, 0xE12FFF1C, 0x03800D89};
static const u32 swiGetPitchTableSignature3[3]      = {0xE59FC000, 0xE12FFF1C, 0x03800FD5};
static const u32 swiGetPitchTableSignature3Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801149};
static const u32 swiGetPitchTableSignature3Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03801215};
static const u32 swiGetPitchTableSignature3Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804119};
static const u32 swiGetPitchTableSignature3Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804301};
static const u32 swiGetPitchTableSignature3Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804305};
static const u32 swiGetPitchTableSignature3Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804395};
static const u32 swiGetPitchTableSignature3Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804439};
static const u32 swiGetPitchTableSignature3Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804559};
static const u32 swiGetPitchTableSignature3Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03804615};
static const u32 swiGetPitchTableSignature3Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x038053E1};
static const u32 swiGetPitchTableSignature3Alt11[3] = {0xE59FC000, 0xE12FFF1C, 0x03805485};
static const u32 swiGetPitchTableSignature3Alt12[3] = {0xE59FC000, 0xE12FFF1C, 0x038055A5};
static const u32 swiGetPitchTableSignature4[3]      = {0xE59FC000, 0xE12FFF1C, 0x038006A1};
static const u32 swiGetPitchTableSignature4Alt1[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800811};
static const u32 swiGetPitchTableSignature4Alt2[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800919};
static const u32 swiGetPitchTableSignature4Alt3[3]  = {0xE59FC000, 0xE12FFF1C, 0x03800925};
static const u32 swiGetPitchTableSignature4Alt4[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035C5};
static const u32 swiGetPitchTableSignature4Alt5[3]  = {0xE59FC000, 0xE12FFF1C, 0x038035ED};
static const u32 swiGetPitchTableSignature4Alt6[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803715};
static const u32 swiGetPitchTableSignature4Alt7[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803829};
static const u32 swiGetPitchTableSignature4Alt8[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803DC1};
static const u32 swiGetPitchTableSignature4Alt9[3]  = {0xE59FC000, 0xE12FFF1C, 0x03803ED5};
static const u32 swiGetPitchTableSignature4Alt10[3] = {0xE59FC000, 0xE12FFF1C, 0x03803F15};
static const u32 swiGetPitchTableSignature5[4]      = {0x781A4B06, 0xD3030791, 0xD20106D1, 0x1A404904};

// User data address
//static const u32 userDataAddr1[1]      = {0x027FFC80};
//static const u32 userDataAddr5[1]      = {0x02FFFC80};

// Sleep patch
static const u32 sleepPatch[2]         = {0x0A000001, 0xE3A00601};
static const u16 sleepPatchThumb[2]    = {0xD002, 0x4831};
static const u16 sleepPatchThumbAlt[2] = {0xD002, 0x0440};

// Sleep input write
static const u32 sleepInputWriteEndSignature1[2]     = {0x04000136, 0x027FFFA8};
static const u32 sleepInputWriteEndSignature5[2]     = {0x04000136, 0x02FFFFA8};
static const u32 sleepInputWriteSignature[1]         = {0x13A04902};
static const u32 sleepInputWriteSignatureAlt[1]      = {0x11A05004};
static const u16 sleepInputWriteBeqSignatureThumb[1] = {0xD000};

// RAM clear
static const u32 ramClearSignature[2]        = {0x02FFC000, 0x02FFF000};
static const u32 ramClearISignature[1]       = {0x02FE0000};
static const u32 ramClearI2Signature[1]      = {0xE26617C1};
static const u32 ramClearI2SignatureAlt[1]   = {0xE26717C1};
static const u16 ramClearI2SignatureThumb[2] = {0x27C1, 0x04BF};

// Post-boot code
static const u32 postBootStartSignature[1]      = {0xE92D47F0};
static const u16 postBootStartSignatureThumb[1] = {0xB5F8};
static const u32 postBootEndSignature[1]        = {0x04000300};

// Card check pull out
static const u32 cardCheckPullOutSignature1[4] = {0xE92D4000, 0xE24DD004, 0xE59F00B4, 0xE5900000}; // Pokemon Dash, early sdk2
static const u32 cardCheckPullOutSignature2[4] = {0xE92D4018, 0xE24DD004, 0xE59F204C, 0xE1D210B0}; // SDK != 3
static const u32 cardCheckPullOutSignature3[4] = {0xE92D4000, 0xE24DD004, 0xE59F002C, 0xE1D000B0}; // SDK 3

// irq enable
static const u32 irqEnableStartSignature1[4]      = {0xE59FC028, 0xE1DC30B0, 0xE3A01000, 0xE1CC10B0}; // SDK <= 3
static const u32 irqEnableStartSignature4[4]      = {0xE92D4010, 0xE1A04000, 0xEBFFFFF6, 0xE59FC020}; // SDK >= 4
static const u32 irqEnableStartSignature4Alt[4]   = {0xE92D4010, 0xE1A04000, 0xEBFFFFE9, 0xE59FC020}; // SDK 5
static const u16 irqEnableStartSignatureThumb[5]  = {0xB530, 0xB081, 0x4D07, 0x882C, 0x2100};
static const u16 irqEnableStartSignatureThumb3[5] = {0xB510, 0x1C04, 0xF7FF, 0xFFF4, 0x4B05}; // SDK 3
static const u16 irqEnableStartSignatureThumb5[5] = {0xB510, 0x1C04, 0xF7FF, 0xFFE4, 0x4B05}; // SDK 5

// SRL start
static const u32 srlStartSignature3[4] = {0xE92D4070, 0xE59F0098, 0xE5904004, 0xE3540000}; // eoo.dat (Pokemon)

// ARM7i start (SDK 5)
//static const u32 a7iStartSignatureConstant[3] = {0x6F696473, 0x616C775F, 0x0000006E}; // 'sdio_wlan'
//static const u32 a7iStartSignatureType1[5] = {0xE12FFF1E, 0xE92D47F8, 0xE24DD014, 0xE1A07000, 0xE5971000};
//static const u32 a7iStartSignatureType2[5] = {0xE12FFF1E, 0xE92D4FF0, 0xE24DD014, 0xE1A0A000, 0xE59A0000};

// SD card reset (SDK 5)
static const u32 sdCardResetSignatureType1[4]      = {0xEBFFFE3C, 0xEBFFFF7B, 0xEB000029, 0xE1A05000};
static const u32 sdCardResetSignatureType2[4]      = {0xEBFFFE35, 0xEBFFFF77, 0xEB00002A, 0xE1A05000};
static const u32 sdCardResetSignatureType3[4]      = {0xEBFFFE48, 0xEBFFFF8E, 0xEB000024, 0xE1A05000};
static const u32 sdCardResetSignatureType4[4]      = {0xEBFFFE57, 0xEBFFFF8E, 0xEB000024, 0xE1A05000};
static const u32 sdCardResetSignatureType5[4]      = {0xEBFFFE3D, 0xEBFFFF7E, 0xEB000028, 0xE1A05000};
static const u32 sdCardResetSignatureType6[4]      = {0xEBFFFE4E, 0xEBFFFF89, 0xEB000024, 0xE1A05000};
static const u32 sdCardResetSignatureType7[4]      = {0xEBFFFE48, 0xEBFFFF82, 0xEB000025, 0xE1A05000};
static const u16 sdCardResetSignatureThumbType1[7] = {0xF7FF, 0xFDAB, 0xF7FF, 0xFF5F, 0xF000, 0xF871, 0x1C05};
static const u16 sdCardResetSignatureThumbType2[7] = {0xF7FF, 0xFDBC, 0xF7FF, 0xFF58, 0xF000, 0xF84A, 0x1C05};
static const u16 sdCardResetSignatureThumbType3[7] = {0xF7FF, 0xFD9C, 0xF7FF, 0xFF50, 0xF000, 0xF87A, 0x1C05};
static const u16 sdCardResetSignatureThumbType4[7] = {0xF7FF, 0xFDB2, 0xF7FF, 0xFF50, 0xF000, 0xF84A, 0x1C05};
static const u16 sdCardResetSignatureThumbType5[7] = {0xF7FF, 0xFDC1, 0xF7FF, 0xFF5F, 0xF000, 0xF849, 0x1C05};
static const u16 sdCardResetSignatureThumbType6[7] = {0xF7FF, 0xFDCE, 0xF7FF, 0xFF68, 0xF000, 0xF85A, 0x1C05};

// SD card functions (SDK 5)
static const u32 sdCardFuncsSignature[4]      = {0xE92D4018, 0xE24DDF5D, 0xE24DDB01, 0xE59FE050};
static const u32 sdCardFuncsSignatureThumb[4] = {0xB0FFB518, 0xB0DFB0FF, 0x4A0E490D, 0x64CA4469};

// Auto power-off (SDK 5)
static const u32 autoPowerOffSignature[4]      = {0xE92D41F0, 0xE59F4070, 0xE1A08000, 0xE1A07001};
static const u16 autoPowerOffSignatureThumb[6] = {0xB5F8, 0x1C05, 0x1C0E, 0x2400, 0x27C5, 0xE019};

bool a7GetReloc(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	extern u32 vAddrOfRelocSrc;
	extern u32 relocDestAtSharedMem;

	if (isSdk5(moduleParams)) {
		// Find the relocation signature
		u32 relocationStart = patchOffsetCache.relocateStartOffset;
		if (!patchOffsetCache.relocateStartOffset) {
			relocationStart = (u32)findOffset(
				(u32*)ndsHeader->arm7destination, 0x800,
				relocateStartSignature5, 1
			);
			if (!relocationStart) {
				dbg_printf("Relocation start not found. Trying alt\n");
				relocationStart = (u32)findOffset(
					(u32*)ndsHeader->arm7destination, 0x800,
					relocateStartSignature5Alt, 1
				);
				if (relocationStart) relocationStart += 0x28;
			}

			if (!relocationStart) {
				dbg_printf("Relocation start not found. Trying alt 2\n");
				relocationStart = (u32)findOffset(
					(u32*)ndsHeader->arm7destination, 0x800,
					relocateStartSignature5Alt2, 1
				);
				if (relocationStart) {
					int i = 0;
					while ((*(u32*)relocationStart != 0) && (i < 0x100)) {
						relocationStart += 4;
						i += 4;
					}
					if (*(u32*)relocationStart != 0) {
						relocationStart = 0;
					} else {
						relocationStart -= 8;
					}
				}
			}

			if (relocationStart) {
				patchOffsetCache.relocateStartOffset = relocationStart;
			}
		}
		if (!relocationStart) {
			dbg_printf("Relocation start alt not found\n");
			return false;
		}

		// Validate the relocation signature
		vAddrOfRelocSrc = relocationStart + 0x8;
		// sanity checks
		u32 relocationCheck = patchOffsetCache.relocateValidateOffset;
		if (!patchOffsetCache.relocateValidateOffset) {
			relocationCheck = (u32)findOffset(
				(u32*)ndsHeader->arm7destination, newArm7binarySize,
				relocateValidateSignature, 1
			);
			if (relocationCheck) {
				patchOffsetCache.relocateValidateOffset = relocationCheck;
			}
		}
		u32 relocationCheck2 =
			*(u32*)(relocationCheck - 0x4);

		relocDestAtSharedMem = *(u32*)0x02FFE1A0==0x080037C0 ? 0x37C0000 : 0x37F8000;
		if (relocationCheck + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem > relocationCheck2) {
			relocationCheck -= 4;
		}
		if (relocationCheck + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem > relocationCheck2) {
			relocationCheck += 4;
			dbg_printf("Error in relocation checking\n");
			dbg_hexa(relocationCheck + 0xC - vAddrOfRelocSrc + relocDestAtSharedMem);
			dbg_printf(" ");
			dbg_hexa(relocationCheck2);
			dbg_printf("\n");

			vAddrOfRelocSrc =  relocationCheck + 0xC - relocationCheck2 + relocDestAtSharedMem;
			dbg_printf("vAddrOfRelocSrc: ");
		} else {
			dbg_printf("Relocation src: ");
		}
		dbg_hexa(vAddrOfRelocSrc);
		dbg_printf("\n");

		return true;
	}

	// Find the relocation signature
    u32 relocationStart = patchOffsetCache.relocateStartOffset;
	if (!patchOffsetCache.relocateStartOffset) {
		relocationStart = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize,
			relocateStartSignature, 1
		);

		if (relocationStart) {
			patchOffsetCache.relocateStartOffset = relocationStart;
		}
	}
	if (!relocationStart) {
		dbg_printf("Relocation start not found\n");
		return false;
	}

    // Validate the relocation signature
	u32 forwardedRelocStartAddr = relocationStart + 4;
	while (!*(u32*)forwardedRelocStartAddr || *(u32*)forwardedRelocStartAddr < 0x02000000 || *(u32*)forwardedRelocStartAddr > 0x03000000) {
		forwardedRelocStartAddr += 4;
	}
	vAddrOfRelocSrc = *(u32*)(forwardedRelocStartAddr + 8);
    
    dbg_printf("forwardedRelocStartAddr\n");
    dbg_hexa(forwardedRelocStartAddr);   
    dbg_printf("\nvAddrOfRelocSrc\n");
    dbg_hexa(vAddrOfRelocSrc);
    dbg_printf("\n");  
	
	// Sanity checks
	u32 relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
	u32 relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
	if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
		dbg_printf("Error in relocation checking method 1\n");
		
		// Find the beginning of the next function
		u32 nextFunction = patchOffsetCache.relocateValidateOffset;
		if (!patchOffsetCache.relocateValidateOffset) {
			nextFunction = (u32)findOffset(
				(u32*)relocationStart, newArm7binarySize,
				nextFunctiontSignature, 1
			);
			if (nextFunction) {
				patchOffsetCache.relocateValidateOffset = nextFunction;
			}
		}
	
		// Validate the relocation signature
		forwardedRelocStartAddr = nextFunction - 0x14;
		
		// Validate the relocation signature
		vAddrOfRelocSrc = *(u32*)(nextFunction - 0xC);
		
		// Sanity checks
		relocationCheck1 = *(u32*)(forwardedRelocStartAddr + 0xC);
		relocationCheck2 = *(u32*)(forwardedRelocStartAddr + 0x10);
		if (vAddrOfRelocSrc != relocationCheck1 || vAddrOfRelocSrc != relocationCheck2) {
			dbg_printf("Error in relocation checking method 2\n");
			return false;
		}
	}

	// Get the remaining details regarding relocation
	u32 valueAtRelocStart = *(u32*)forwardedRelocStartAddr;
	relocDestAtSharedMem = *(u32*)valueAtRelocStart;
	if (relocDestAtSharedMem != 0x37F8000) { // Shared memory in RAM
		// Try again
		vAddrOfRelocSrc += *(u32*)(valueAtRelocStart + 4);
		relocDestAtSharedMem = *(u32*)(valueAtRelocStart + 0xC);
		if (relocDestAtSharedMem != 0x37F8000) {
			dbg_printf("Error in finding shared memory relocation area\n");
			return false;
		}
	}

	dbg_printf("Relocation src: ");
	dbg_hexa(vAddrOfRelocSrc);
	dbg_printf("\n");
	dbg_printf("Relocation dst: ");
	dbg_hexa(relocDestAtSharedMem);
	dbg_printf("\n");

	return true;
}

u32* findSwiHaltOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSwiHaltOffset:\n");

	u32* swiHaltOffset = NULL;
	u32 dispStatAddr = (u32)findOffset(
		(u32*)ndsHeader->arm7destination, 0x00001000,//, newArm7binarySize,
		swiHaltConstSignature, 1
	);
	if (!dispStatAddr) {
		dispStatAddr = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, 0x00001000,//, newArm7binarySize,
			swiHaltConstSignatureAlt, 1
		);
	}
	if (dispStatAddr) {
		dispStatAddr += 0x20;
		swiHaltOffset =
			findOffsetBackwards((u32*)dispStatAddr, 0x40,
				swiHaltSignature2, 1
		);
	}
	if (!swiHaltOffset && moduleParams->sdk_version < 0x2008000) {
		swiHaltOffset =
			findOffsetBackwards((u32*)dispStatAddr, 0x40,
				swiHaltSignature1, 1
		);
	}
	if (swiHaltOffset) {
		dbg_printf("swiHalt call found\n");
	} else {
		dbg_printf("swiHalt call not found\n");
	}

	dbg_printf("\n");
	return swiHaltOffset;
}

u16* findSwiHaltThumbOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSwiHaltThumbOffset:\n");

	u32 swiHaltOffset = 0;
	if (isSdk5(moduleParams)) {
		extern u32 vAddrOfRelocSrc;

		swiHaltOffset =
			(u32)findOffsetThumb((u16*)vAddrOfRelocSrc, 0x200,
				swiHaltCmpSignature, 1
		);
		if (!findOffsetBackwardsThumb((u16*)swiHaltOffset, 0x20, swiHaltMovSignature, 1)) {
			const u32 oldOffset = swiHaltOffset;
			const u16 cmpBak = *(u16*)swiHaltOffset;
			*(u16*)oldOffset = 0; // Avoid re-searching for wrong offset

			swiHaltOffset =
				(u32)findOffsetThumb((u16*)vAddrOfRelocSrc, 0x200,
					swiHaltCmpSignature, 1
			);
			*(u16*)oldOffset = cmpBak;
			// if (!findOffsetBackwardsThumb((u16*)swiHaltOffset, 0x20, swiHaltMovSignature, 1)) {
			// 	swiHaltOffset = 0;
			// }
		}
	}
	if (!swiHaltOffset) {
		u32 dispStatAddr = (u32)findOffset(
			(u32*)ndsHeader->arm7destination, 0x00001000,//, newArm7binarySize,
			swiHaltConstSignature, 1
		);
		if (!dispStatAddr) {
			dispStatAddr = (u32)findOffset(
				(u32*)ndsHeader->arm7destination, 0x00001000,//, newArm7binarySize,
				swiHaltConstSignatureAlt, 1
			);
		}
		if (dispStatAddr) {
			swiHaltOffset =
				(u32)findOffsetBackwardsThumb((u16*)dispStatAddr, 0x40,
					swiHaltCmpSignature, 1
			);
		}
	}
	if (swiHaltOffset) {
		swiHaltOffset -= 8;
		dbg_printf("swiHalt call found\n");
	} else {
		dbg_printf("swiHalt call not found\n");
	}

	dbg_printf("\n");
	return (u16*)swiHaltOffset;
}

u32* a7_findSwi12Offset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSwi12Offset:\n");

	u32* swi12Offset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
		swi12Signature, 1
	);
	if (swi12Offset) {
		dbg_printf("swi 0x12 call found\n");
	} else {
		dbg_printf("swi 0x12 call not found\n");
	}

	dbg_printf("\n");
	return swi12Offset;
}

u16* a7_findSwi24Offset(void) {
	dbg_printf("findSwi24Offset:\n");

	u16* swi24Offset = findOffsetThumb(
		(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
		swi24Signature, 2
	);
	if (swi24Offset) {
		dbg_printf("swi 0x24 call found\n");
	} else {
		dbg_printf("swi 0x24 call not found\n");
	}

	dbg_printf("\n");
	return swi24Offset;
}

u16* a7_findSwi25Offset(void) {
	dbg_printf("findSwi25Offset:\n");

	u16* swi25Offset = findOffsetThumb(
		(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
		swi25Signature, 2
	);
	if (swi25Offset) {
		dbg_printf("swi 0x25 call found\n");
	} else {
		dbg_printf("swi 0x25 call not found\n");
	}

	dbg_printf("\n");
	return swi25Offset;
}

u16* a7_findSwi26Offset(void) {
	dbg_printf("findSwi24Offset:\n");

	u16* swi26Offset = findOffsetThumb(
		(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
		swi26Signature, 2
	);
	if (swi26Offset) {
		dbg_printf("swi 0x26 call found\n");
	} else {
		dbg_printf("swi 0x26 call not found\n");
	}

	dbg_printf("\n");
	return swi26Offset;
}

u16* a7_findSwi27Offset(void) {
	dbg_printf("findSwi27Offset:\n");

	u16* swi27Offset = findOffsetThumb(
		(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
		swi27Signature, 2
	);
	if (swi27Offset) {
		dbg_printf("swi 0x27 call found\n");
	} else {
		dbg_printf("swi 0x27 call not found\n");
	}

	dbg_printf("\n");
	return swi27Offset;
}

u32* a7_findScfgExtOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("a7_findScfgExtOffset:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
		scfgExtSignature, 1
	);
	if (offset) {
		dbg_printf("SCFG_EXT found\n");
	} else {
		dbg_printf("SCFG_EXT not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u16* findSwiGetPitchTableThumbBranchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSwiGetPitchTableThumbOffset:\n");

	u16* offset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, newArm7binarySize,
		swiGetPitchTableSignatureThumb, 4
	);
	if (!offset) {
		offset = findOffsetThumb(
			(u16*)ndsHeader->arm7destination, newArm7binarySize,
			swiGetPitchTableSignatureThumbAlt, 4
		);
	}

	if (offset) {
		dbg_printf("swiGetPitchTable thumb branch found\n");
	} else {
		dbg_printf("swiGetPitchTable thumb branch not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSwiGetPitchTableOffset:\n");

	u32* swiGetPitchTableOffset = NULL;

	if (moduleParams->sdk_version > 0x5000000) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature5, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable call SDK 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable call SDK 5 not found\n");
		}

		if (!swiGetPitchTableOffset && dsiModeConfirmed && ndsHeader->unitCode > 0 && *(u32*)0x02FFE1A0 != 0x00403000) {
			swiGetPitchTableOffset = findOffset(
				(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize > 0x10000 ? 0x10000 : newArm7ibinarySize,
				swiGetPitchTableSignature5, 4
			);
			if (swiGetPitchTableOffset) {
				dbg_printf("swiGetPitchTable call SDK 5 (ARM7i) found\n");
			} else {
				dbg_printf("swiGetPitchTable call SDK 5 (ARM7i) not found\n");
			}
		}
	}

	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call not found\n");
		}
	}

	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt1, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 1 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 1 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt2, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 2 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 2 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt3, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 3 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 3 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt4, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 4 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 4 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt5, 4
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 5 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt6, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 6 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 6 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt7, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 7 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 7 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt8, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 8 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 8 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt9, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 9 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 9 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt10, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 10 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 10 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt11, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 11 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 11 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt12, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 12 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 12 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt13, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 13 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 13 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt14, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 14 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 14 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature1Alt15, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 15 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK <= 2 call alt 15 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt1, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 1 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 1 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt2, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 2 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 2 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt3, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 3 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 3 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 4 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 4 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt5, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 5 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt6, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 6 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 6 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt7, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 7 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 7 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt8, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 8 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 8 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt9, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 9 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 9 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt10, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 10 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 10 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt11, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 11 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 11 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature3Alt12, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 3 call alt 12 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 3 call alt 12 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt1, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 1 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 1 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt2, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 2 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 2 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt3, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 3 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 3 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt4, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 4 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 4 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt5, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 5 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 5 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt6, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 6 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 6 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt7, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 7 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 7 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt8, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 8 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 8 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt9, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 9 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 9 not found\n");
		}
	}
	if (!swiGetPitchTableOffset) {
		swiGetPitchTableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize > 0x10000 ? 0x10000 : newArm7binarySize,
			swiGetPitchTableSignature4Alt10, 3
		);
		if (swiGetPitchTableOffset) {
			dbg_printf("swiGetPitchTable SDK 4 call alt 10 found\n");
		} else {
			dbg_printf("swiGetPitchTable SDK 4 call alt 10 not found\n");
		}
	}

	dbg_printf("\n");
	return swiGetPitchTableOffset;
}

/*u32* findUserDataAddrOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findUserDataAddrOffset:\n");

	u32* userDataAddrOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		isSdk5(moduleParams) ? userDataAddr5 : userDataAddr1, 1
	);
	if (userDataAddrOffset) {
		dbg_printf("User data address found\n");
	} else {
		dbg_printf("User data address not found\n");
	}

	dbg_printf("\n");
	return userDataAddrOffset;
}*/

u32* findSleepPatchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSleepPatchOffset:\n");

	u32* sleepPatchOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		sleepPatch, 2
	);
	if (sleepPatchOffset) {
		dbg_printf("Sleep patch found\n");
	} else {
		dbg_printf("Sleep patch not found\n");
	}

	dbg_printf("\n");
	return sleepPatchOffset;
}

u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findSleepPatchOffsetThumb:\n");
	
	u16* sleepPatchOffset = findOffsetThumb(
		(u16*)ndsHeader->arm7destination, newArm7binarySize,
		sleepPatchThumb, 2
	);
	if (sleepPatchOffset) {
		dbg_printf("Thumb sleep patch thumb found\n");
	} else {
		dbg_printf("Thumb sleep patch thumb not found\n");
	}

	if (!sleepPatchOffset) {
		sleepPatchOffset = findOffsetThumb(
			(u16*)ndsHeader->arm7destination, newArm7binarySize,
			sleepPatchThumbAlt, 2
		);
		if (sleepPatchOffset) {
			dbg_printf("Thumb sleep patch thumb alt found\n");
		} else {
			dbg_printf("Thumb sleep patch thumb alt not found\n");
		}
	}

	dbg_printf("\n");
	return sleepPatchOffset;
}

u32* findSleepInputWriteOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSleepInputWriteOffset:\n");

	u32* offset = NULL;
	u32* endOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		isSdk5(moduleParams) ? sleepInputWriteEndSignature5 : sleepInputWriteEndSignature1, 2
	);
	if (!endOffset && dsiModeConfirmed && ndsHeader->unitCode > 0 && *(u32*)0x02FFE1A0 != 0x00403000) {
		endOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sleepInputWriteEndSignature5, 2
		);
	}
	if (endOffset) {
		offset = findOffsetBackwards(
			endOffset, 0x38,
			sleepInputWriteSignature, 1
		);
		if (!offset) {
			offset = findOffsetBackwards(
				endOffset, 0x3C,
				sleepInputWriteSignatureAlt, 1
			);
		}
		if (!offset) {
			u32 thumbOffset = (u32)findOffsetBackwardsThumb(
				(u16*)endOffset, 0x30,
				sleepInputWriteBeqSignatureThumb, 1
			);
			if (thumbOffset) {
				thumbOffset += 2;
				offset = (u32*)thumbOffset;
			}
		}
	}
	if (offset) {
		dbg_printf("Sleep input write found\n");
	} else {
		dbg_printf("Sleep input write not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findRamClearOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRamClearOffset:\n");

	u32* ramClearOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		ramClearSignature, 2
	);
	if (ramClearOffset) {
		dbg_printf("RAM clear found\n");
	} else {
		dbg_printf("RAM clear not found\n");
	}

	dbg_printf("\n");
	return ramClearOffset;
}

u32* findRamClearIOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRamClearIOffset:\n");

	u32* ramClearOffset = NULL;
	if (*(u32*)0x02FFE1A0 == 0x00403000) {
		ramClearOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			ramClearISignature, 1
		);
	} else {
		ramClearOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize,
			ramClearSignature, 1
		);
		if (!ramClearOffset) {
			ramClearOffset = findOffset(
				(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
				ramClearSignature, 1
			);
		}
	}
	if (ramClearOffset) {
		dbg_printf("RAM clear I found\n");
	} else {
		dbg_printf("RAM clear I not found\n");
	}

	dbg_printf("\n");
	return ramClearOffset;
}

u32* findRamClearI2Offset(const u32* ramClearIOffset) {
	dbg_printf("findRamClearI2Offset:\n");

	u32* ramClearOffset = findOffsetBackwards(
		ramClearIOffset, 0x200,
		ramClearI2Signature, 1
	);
	if (ramClearOffset) {
		dbg_printf("RAM clear I 2 found\n");
	} else {
		dbg_printf("RAM clear I 2 not found\n");
	}

	if (!ramClearOffset) {
		ramClearOffset = findOffsetBackwards(
			ramClearIOffset, 0x200,
			ramClearI2SignatureAlt, 1
		);
		if (ramClearOffset) {
			dbg_printf("RAM clear I 2 alt found\n");
		} else {
			dbg_printf("RAM clear I 2 alt not found\n");
		}
	}

	if (!ramClearOffset) {
		ramClearOffset = (u32*)findOffsetBackwardsThumb(
			(u16*)ramClearIOffset, 0x100,
			ramClearI2SignatureThumb, 2
		);
		if (ramClearOffset) {
			dbg_printf("RAM clear I 2 THUMB found\n");
		} else {
			dbg_printf("RAM clear I 2 THUMB not found\n");
		}
	}

	dbg_printf("\n");
	return ramClearOffset;
}

u32* findPostBootOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findPostBootOffset:\n");

	u32* startOffset = NULL;
	u32* endOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		postBootEndSignature, 1
	);
	if (endOffset) {
		dbg_printf("Post boot end found: ");
		dbg_hexa((u32)endOffset);
		dbg_printf("\n");

		startOffset = findOffsetBackwards(
			endOffset, 0x200,
			postBootStartSignature, 1
		);
		if (startOffset) {
			dbg_printf("Post boot start found\n");
		} else {
			dbg_printf("Post boot start not found\n");
		}
		if (!startOffset) {
			startOffset = (u32*)findOffsetBackwardsThumb(
				(u16*)endOffset, 0x100,
				postBootStartSignatureThumb, 1
			);
			if (startOffset) {
				dbg_printf("Post boot start thumb found\n");
			} else {
				dbg_printf("Post boot start thumb not found\n");
			}
		}
	} else {
		dbg_printf("Post boot not found\n");
	}

	dbg_printf("\n");
	return startOffset;
}

u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardCheckPullOutOffset:\n");
	
	const u32* cardCheckPullOutSignature = cardCheckPullOutSignature1;
    if (moduleParams->sdk_version > 0x2004FFF && moduleParams->sdk_version < 0x3000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature2;
    } else if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
		cardCheckPullOutSignature = cardCheckPullOutSignature3;
	}

	u32* cardCheckPullOutOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		cardCheckPullOutSignature, 4
	);
	if (cardCheckPullOutOffset) {
		dbg_printf("Card check pull out found: ");
	} else {
		dbg_printf("Card check pull out not found\n");
	}

	if (cardCheckPullOutOffset) {
		dbg_hexa((u32)cardCheckPullOutOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardCheckPullOutOffset;
}

u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardIrqEnableOffset:\n");
	
	const u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (ndsHeader->arm7binarySize != 0x289C0 && moduleParams->sdk_version > 0x4000000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32* cardIrqEnableOffset = findOffset(
		(u32*)ndsHeader->arm7destination, newArm7binarySize,
		irqEnableStartSignature, 4
	);
	if (cardIrqEnableOffset) {
		dbg_printf("irq enable found\n");
	} else {
		dbg_printf("irq enable not found\n");
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version < 0x4000000) {
		// SDK 4
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize,
            irqEnableStartSignature4, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 4 found\n");
		} else {
			dbg_printf("irq enable SDK 4 not found\n");
		}
	}

	if (!cardIrqEnableOffset) {
		// SDK 5
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm7destination, newArm7binarySize,
            irqEnableStartSignature4Alt, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable alt found\n");
		} else {
			dbg_printf("irq enable alt not found\n");
		}
	}

	if (!cardIrqEnableOffset) {
		cardIrqEnableOffset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm7destination, newArm7binarySize,
            irqEnableStartSignatureThumb, 5
		);
		if (cardIrqEnableOffset) {
			// Find again
			cardIrqEnableOffset = (u32*)findOffsetThumb(
				(u16*)cardIrqEnableOffset+4, newArm7binarySize,
				irqEnableStartSignatureThumb, 5
			);
		}
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable thumb found\n");
		} else {
			dbg_printf("irq enable thumb not found\n");
		}
	}

	if (!cardIrqEnableOffset) {
		// SDK 3
		cardIrqEnableOffset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm7destination, newArm7binarySize,
            irqEnableStartSignatureThumb3, 5
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable thumb SDK 3 found\n");
		} else {
			dbg_printf("irq enable thumb SDK 3 not found\n");
		}
	}

	if (!cardIrqEnableOffset && isSdk5(moduleParams)) {
		// SDK 5
		cardIrqEnableOffset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm7destination, newArm7binarySize,
            irqEnableStartSignatureThumb5, 5
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable thumb SDK 5 found\n");
		} else {
			dbg_printf("irq enable thumb SDK 5 not found\n");
		}
	}

	if (dsiModeConfirmed && ndsHeader->unitCode > 0 && *(u32*)0x02FFE1A0 != 0x00403000) {
		if (!cardIrqEnableOffset) {
			// SDK 5
			cardIrqEnableOffset = findOffset(
				(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
				irqEnableStartSignature4Alt, 4
			);
			if (cardIrqEnableOffset) {
				dbg_printf("irq enable alt (ARM7i) found\n");
			} else {
				dbg_printf("irq enable alt (ARM7i) not found\n");
			}
		}

		if (!cardIrqEnableOffset) {
			// SDK 5
			cardIrqEnableOffset = (u32*)findOffsetThumb(
				(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
				irqEnableStartSignatureThumb5, 5
			);
			if (cardIrqEnableOffset) {
				dbg_printf("irq enable thumb (ARM7i) found\n");
			} else {
				dbg_printf("irq enable thumb (ARM7i) not found\n");
			}
		}
	}

	dbg_printf("\n");
	return cardIrqEnableOffset;
}

u32* findSrlStartOffset7(const tNDSHeader* ndsHeader) {
	dbg_printf("findSrlStartOffset7\n");

    u32* offset = findOffset(
		ndsHeader->arm7destination, newArm7binarySize,
		srlStartSignature3, 4
	);

	if (offset) {
		dbg_printf("SRL start function found\n");
	} else {
		dbg_printf("SRL start function not found\n");
	}

	dbg_printf("\n");
	return offset;
}

/*u32* findA7iStartOffset(void) {
	dbg_printf("findA7iStartOffset:\n");

	u32* offset = findOffset(
		(u32*)__DSiHeader->arm7idestination, 0x8000,
		a7iStartSignatureConstant, 3
	);
	if (offset) {
		offset = offset+3;
	}

	if (!offset) {
		offset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			a7iStartSignatureType1, 5
		);
		if (offset) {
			offset = offset+1;
		}
	}

	if (!offset) {
		offset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			a7iStartSignatureType2, 5
		);
		if (offset) {
			offset = offset+1;
		}
	}

	if (offset) {
		dbg_printf("ARM7i start found\n");
	} else {
		dbg_printf("ARM7i start not found\n");
	}

	dbg_printf("\n");
	return offset;
}*/

u32* findSdCardResetOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findSdCardResetOffset:\n");

	u32* sdCardResetOffset = findOffset(
		(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
		sdCardResetSignatureType1, 4
	);

	if (!sdCardResetOffset) {
		sdCardResetOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureType2, 4
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureType3, 4
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureType4, 4
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureType5, 4
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureType6, 4
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = findOffset(
			(u32*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureType7, 4
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureThumbType1, 7
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureThumbType2, 7
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureThumbType3, 7
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureThumbType4, 7
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureThumbType5, 7
		);
	}

	if (!sdCardResetOffset) {
		sdCardResetOffset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardResetSignatureThumbType6, 7
		);
	}

	if (sdCardResetOffset) {
		dbg_printf("SD Card reset found\n");
	} else {
		dbg_printf("SD Card reset not found\n");
	}

	dbg_printf("\n");
	return sdCardResetOffset;
}

u32* findSdCardFuncsOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSdCardFuncsOffset:\n");

	u32* offset = findOffset(
			__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardFuncsSignature, 4
		);
	if (!offset) {
		offset = (u32*)findOffset(
			__DSiHeader->arm7idestination, newArm7ibinarySize,
			sdCardFuncsSignatureThumb, 4
		);
	}

	if (offset) {
		dbg_printf("SD Card functions found\n");
	} else {
		dbg_printf("SD Card functions not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findAutoPowerOffOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findAutoPowerOffOffset:\n");

	u32* offset = NULL;

	if (*(u32*)0x02FFE1A0 != 0x00403000) {
		offset = findOffset(
			ndsHeader->arm7destination, newArm7binarySize,
			autoPowerOffSignature, 4
		);
		if (!offset) {
			offset = (u32*)findOffsetThumb(
				(u16*)ndsHeader->arm7destination, newArm7binarySize,
				autoPowerOffSignatureThumb, 6
			);
		}
	}
	if (!offset) {
		offset = findOffset(
			__DSiHeader->arm7idestination, newArm7ibinarySize,
			autoPowerOffSignature, 4
		);
	}
	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)__DSiHeader->arm7idestination, newArm7ibinarySize,
			autoPowerOffSignatureThumb, 6
		);
	}

	if (offset) {
		dbg_printf("SD Card reset found\n");
	} else {
		dbg_printf("SD Card reset not found\n");
	}

	dbg_printf("\n");
	return offset;
}
