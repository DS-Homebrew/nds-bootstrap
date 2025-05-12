#include <string.h>
#include <nds/ndstypes.h>
#include <nds/arm9/video.h>
#include <nds/system.h>

void applyColorLut(void) {
	u16* storedPals = (u16*)0x03755800;
	u16* palletes = (u16*)0x05000000;
	u16* colorTable = (u16*)0x03770000;

	for (int i = 0; i < 0x800/2; i++) {
		if (storedPals[i] != palletes[i]) {
			palletes[i] = colorTable[palletes[i] % 0x8000];
			storedPals[i] = palletes[i];
		}
	}

	u8 vramCr = VRAM_E_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr == 0x83 || vramCr == 0x84) { // 3dTexPal/BgPalA
		u8 vramCnt = VRAM_E_CR;
		u16* storedPals = (u16*)0x03756000;
		u16* palletes = (u16*)0x06888000;

		VRAM_E_CR = 0x80;
		for (int i = 0; i < 0x8000/2; i++) {
			if (storedPals[i] != palletes[i]) {
				palletes[i] = colorTable[palletes[i] % 0x8000];
				storedPals[i] = palletes[i];
			}
		}
		VRAM_E_CR = vramCnt;
	}

	vramCr = VRAM_F_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x83 && vramCr <= 0x85) { // 3dTexPal/BgPalA/ObjPalA
		u8 vramCnt = VRAM_F_CR;
		u16* storedPals = (u16*)0x0375E000;
		u16* palletes = (u16*)0x06890000;

		VRAM_F_CR = 0x80;
		for (int i = 0; i < 0x4000/2; i++) {
			if (storedPals[i] != palletes[i]) {
				palletes[i] = colorTable[palletes[i] % 0x8000];
				storedPals[i] = palletes[i];
			}
		}
		VRAM_F_CR = vramCnt;
	}

	vramCr = VRAM_G_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x83 && vramCr <= 0x85) { // 3dTexPal/BgPalA/ObjPalA
		u8 vramCnt = VRAM_G_CR;
		u16* storedPals = (u16*)0x03762000;
		u16* palletes = (u16*)0x06894000;

		VRAM_G_CR = 0x80;
		for (int i = 0; i < 0x4000/2; i++) {
			if (storedPals[i] != palletes[i]) {
				palletes[i] = colorTable[palletes[i] % 0x8000];
				storedPals[i] = palletes[i];
			}
		}
		VRAM_G_CR = vramCnt;
	}

	vramCr = VRAM_H_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr == 0x82) { // BgPalB
		u8 vramCnt = VRAM_H_CR;
		u16* storedPals = (u16*)0x03766000;
		u16* palletes = (u16*)0x06898000;

		VRAM_H_CR = 0x80;
		for (int i = 0; i < 0x8000/2; i++) {
			if (storedPals[i] != palletes[i]) {
				palletes[i] = colorTable[palletes[i] % 0x8000];
				storedPals[i] = palletes[i];
			}
		}
		VRAM_H_CR = vramCnt;
	}

	vramCr = VRAM_I_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr == 0x83) { // ObjPalB
		u8 vramCnt = VRAM_I_CR;
		u16* storedPals = (u16*)0x0376E000;
		u16* palletes = (u16*)0x068A2000;

		VRAM_I_CR = 0x80;
		for (int i = 0; i < 0x2000/2; i++) {
			if (storedPals[i] != palletes[i]) {
				palletes[i] = colorTable[palletes[i] % 0x8000];
				storedPals[i] = palletes[i];
			}
		}
		VRAM_I_CR = vramCnt;
	}
}
