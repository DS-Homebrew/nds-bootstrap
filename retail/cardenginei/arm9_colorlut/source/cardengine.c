#include <string.h>
#include <nds/ndstypes.h>
#include <nds/arm9/video.h>
#include <nds/system.h>

void applyColorLut(void) {
	u32* storedPals = (u32*)0x03755800;
	u32* palettes = (u32*)0x05000000;
	u16* colorTable = (u16*)0x03770000;

	for (int i = 0; i < 0x800/4; i++) {
		if (*storedPals != *palettes) {
			u16* storedPals16 = (u16*)storedPals;
			u16* palettes16 = (u16*)palettes;
			for (int p = 0; p < 2; p++) {
				if (storedPals16[p] != palettes16[p]) {
					palettes16[p] = colorTable[palettes16[p] % 0x8000];
					storedPals16[p] = palettes16[p];
				}
			}
		}
		storedPals++;
		palettes++;
	}

	static int framesPassed = 19;
	framesPassed++;
	if (framesPassed != 20) return;
	framesPassed = 0;

	u8 vramCr = VRAM_E_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr == 0x83 || vramCr == 0x84) { // 3dTexPal/BgPalA
		u8 vramCnt = VRAM_E_CR;
		storedPals = (u32*)0x03756000;
		palettes = (u32*)0x06888000;

		VRAM_E_CR = 0x80;
		for (int i = 0; i < 0x8000/4; i++) {
			if (*storedPals != *palettes) {
				u16* storedPals16 = (u16*)storedPals;
				u16* palettes16 = (u16*)palettes;
				for (int p = 0; p < 2; p++) {
					if (storedPals[p] != palettes16[p]) {
						palettes16[p] = colorTable[palettes16[p] % 0x8000];
						storedPals16[p] = palettes16[p];
					}
				}
			}
			storedPals++;
			palettes++;
		}
		VRAM_E_CR = vramCnt;
	}

	vramCr = VRAM_F_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x83 && vramCr <= 0x85) { // 3dTexPal/BgPalA/ObjPalA
		u8 vramCnt = VRAM_F_CR;
		storedPals = (u32*)0x0375E000;
		palettes = (u32*)0x06890000;

		VRAM_F_CR = 0x80;
		for (int i = 0; i < 0x4000/4; i++) {
			if (*storedPals != *palettes) {
				u16* storedPals16 = (u16*)storedPals;
				u16* palettes16 = (u16*)palettes;
				for (int p = 0; p < 2; p++) {
					if (storedPals[p] != palettes16[p]) {
						palettes16[p] = colorTable[palettes16[p] % 0x8000];
						storedPals16[p] = palettes16[p];
					}
				}
			}
			storedPals++;
			palettes++;
		}
		VRAM_F_CR = vramCnt;
	}

	vramCr = VRAM_G_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x83 && vramCr <= 0x85) { // 3dTexPal/BgPalA/ObjPalA
		u8 vramCnt = VRAM_G_CR;
		storedPals = (u32*)0x03762000;
		palettes = (u32*)0x06894000;

		VRAM_G_CR = 0x80;
		for (int i = 0; i < 0x4000/4; i++) {
			if (*storedPals != *palettes) {
				u16* storedPals16 = (u16*)storedPals;
				u16* palettes16 = (u16*)palettes;
				for (int p = 0; p < 2; p++) {
					if (storedPals[p] != palettes16[p]) {
						palettes16[p] = colorTable[palettes16[p] % 0x8000];
						storedPals16[p] = palettes16[p];
					}
				}
			}
			storedPals++;
			palettes++;
		}
		VRAM_G_CR = vramCnt;
	}

	vramCr = VRAM_H_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr == 0x82) { // BgPalB
		u8 vramCnt = VRAM_H_CR;
		storedPals = (u32*)0x03766000;
		palettes = (u32*)0x06898000;

		VRAM_H_CR = 0x80;
		for (int i = 0; i < 0x8000/4; i++) {
			if (*storedPals != *palettes) {
				u16* storedPals16 = (u16*)storedPals;
				u16* palettes16 = (u16*)palettes;
				for (int p = 0; p < 2; p++) {
					if (storedPals[p] != palettes16[p]) {
						palettes16[p] = colorTable[palettes16[p] % 0x8000];
						storedPals16[p] = palettes16[p];
					}
				}
			}
			storedPals++;
			palettes++;
		}
		VRAM_H_CR = vramCnt;
	}

	vramCr = VRAM_I_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr == 0x83) { // ObjPalB
		u8 vramCnt = VRAM_I_CR;
		storedPals = (u32*)0x0376E000;
		palettes = (u32*)0x068A2000;

		VRAM_I_CR = 0x80;
		for (int i = 0; i < 0x2000/4; i++) {
			if (*storedPals != *palettes) {
				u16* storedPals16 = (u16*)storedPals;
				u16* palettes16 = (u16*)palettes;
				for (int p = 0; p < 2; p++) {
					if (storedPals[p] != palettes16[p]) {
						palettes16[p] = colorTable[palettes16[p] % 0x8000];
						storedPals16[p] = palettes16[p];
					}
				}
			}
			storedPals++;
			palettes++;
		}
		VRAM_I_CR = vramCnt;
	}
}
