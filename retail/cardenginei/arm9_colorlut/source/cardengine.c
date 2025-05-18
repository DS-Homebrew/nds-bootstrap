#include <string.h>
#include <nds/ndstypes.h>
#include <nds/arm9/video.h>
#include <nds/system.h>

extern u16 flags;
extern u16 bankProcessSize;

#define invertedColors BIT(0)
#define noWhiteFade BIT(1)

void applyColorLut(bool processExtPalettes) {
	u32* storedPals = (u32*)0x0374B800;
	u32* palettes = (u32*)0x05000000;
	u16* colorTable = (u16*)0x03770000;

	if (processExtPalettes) {
		goto processExtPalettesFunc;
	}

	if (flags & invertedColors) {
		static u16 storedMasterBright = 0;
		static u16 storedMasterBrightSub = 0;
		static u8 storedBldCnt = 0;
		static u8 storedBldCntSub = 0;

		// Invert Black/White fades
		if (storedMasterBright != REG_MASTER_BRIGHT) {
			bool masterBrightChanged = false;
			if (REG_MASTER_BRIGHT >= 0x8000) {
				REG_MASTER_BRIGHT -= 0x4000;
				masterBrightChanged = true;
			}
			if (!masterBrightChanged && REG_MASTER_BRIGHT >= 0x4000 && REG_MASTER_BRIGHT < 0x8000) {
				REG_MASTER_BRIGHT += 0x4000;
			}
			storedMasterBright = REG_MASTER_BRIGHT;
		}
		if (storedMasterBrightSub != REG_MASTER_BRIGHT_SUB) {
			bool masterBrightSubChanged = false;
			if (REG_MASTER_BRIGHT_SUB >= 0x8000) {
				REG_MASTER_BRIGHT_SUB -= 0x4000;
				masterBrightSubChanged = true;
			}
			if (!masterBrightSubChanged && REG_MASTER_BRIGHT_SUB >= 0x4000 && REG_MASTER_BRIGHT_SUB < 0x8000) {
				REG_MASTER_BRIGHT_SUB += 0x4000;
			}
			storedMasterBrightSub = REG_MASTER_BRIGHT_SUB;
		}
		if (storedBldCnt != REG_BLDCNT) {
			bool bldCntChanged = false;
			if (REG_BLDCNT == 0xFF) {
				REG_BLDCNT = 0xBF;
				bldCntChanged = true;
			}
			if (!bldCntChanged && REG_BLDCNT == 0xBF) {
				REG_BLDCNT = 0xFF;
			}
			storedBldCnt = REG_BLDCNT;
		}
		if (storedBldCntSub != REG_BLDCNT_SUB) {
			bool bldCntSubChanged = false;
			if (REG_BLDCNT_SUB == 0xFF) {
				REG_BLDCNT_SUB = 0xBF;
				bldCntSubChanged = true;
			}
			if (!bldCntSubChanged && REG_BLDCNT_SUB == 0xBF) {
				REG_BLDCNT_SUB = 0xFF;
			}
			storedBldCntSub = REG_BLDCNT_SUB;
		}
	} else if (flags & noWhiteFade) {
		// Invert white to black fades
		if (REG_MASTER_BRIGHT >= 0x4000 && REG_MASTER_BRIGHT < 0x8000) {
			REG_MASTER_BRIGHT += 0x4000;
		}
		if (REG_MASTER_BRIGHT_SUB >= 0x4000 && REG_MASTER_BRIGHT_SUB < 0x8000) {
			REG_MASTER_BRIGHT_SUB += 0x4000;
		}
		if (REG_BLDCNT == 0xBF) {
			REG_BLDCNT = 0xFF;
		}
		if (REG_BLDCNT_SUB == 0xBF) {
			REG_BLDCNT_SUB = 0xFF;
		}
	}

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
	return;

processExtPalettesFunc:
	u8 vramCr = VRAM_E_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x88) {
		vramCr -= 8;
	}
	if (vramCr == 0x83 || vramCr == 0x84) { // 3dTexPal/BgPalA
		u8 vramCnt = VRAM_E_CR;
		static int block = 0;
		storedPals = (u32*)0x0374C000+(block/4);
		palettes = (u32*)0x06880000+(block/4);

		VRAM_E_CR = 0x80;
		for (int i = 0; i < bankProcessSize/4; i++) {
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

		block += bankProcessSize;
		if (block == 0x10000) block = 0;
	}

	vramCr = VRAM_F_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x88) {
		vramCr -= 8;
	}
	if (vramCr >= 0x83 && vramCr <= 0x85) { // 3dTexPal/BgPalA/ObjPalA
		u8 vramCnt = VRAM_F_CR;
		static int block = 0;
		storedPals = (u32*)0x0375C000+(block/4);
		palettes = (u32*)0x06890000+(block/4);

		VRAM_F_CR = 0x80;
		for (int i = 0; i < bankProcessSize/4; i++) {
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

		block += bankProcessSize;
		if (block == 0x4000) block = 0;
	}

	vramCr = VRAM_G_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x88) {
		vramCr -= 8;
	}
	if (vramCr >= 0x83 && vramCr <= 0x85) { // 3dTexPal/BgPalA/ObjPalA
		u8 vramCnt = VRAM_G_CR;
		static int block = 0;
		storedPals = (u32*)0x03760000+(block/4);
		palettes = (u32*)0x06894000+(block/4);

		VRAM_G_CR = 0x80;
		for (int i = 0; i < bankProcessSize/4; i++) {
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

		block += bankProcessSize;
		if (block == 0x4000) block = 0;
	}

	vramCr = VRAM_H_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x88) {
		vramCr -= 8;
	}
	if (vramCr == 0x82) { // BgPalB
		u8 vramCnt = VRAM_H_CR;
		static int block = 0;
		storedPals = (u32*)0x03764000+(block/4);
		palettes = (u32*)0x06898000+(block/4);

		VRAM_H_CR = 0x80;
		for (int i = 0; i < bankProcessSize/4; i++) {
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

		block += bankProcessSize;
		if (block == 0x8000) block = 0;
	}

	vramCr = VRAM_I_CR;
	while (vramCr >= 0x90) {
		vramCr -= 0x10;
	}
	if (vramCr >= 0x88) {
		vramCr -= 8;
	}
	if (vramCr == 0x83) { // ObjPalB
		u8 vramCnt = VRAM_I_CR;
		static int block = 0;
		storedPals = (u32*)0x0376C000+(block/4);
		palettes = (u32*)0x068A0000+(block/4);

		VRAM_I_CR = 0x80;
		for (int i = 0; i < bankProcessSize/4; i++) {
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

		block += bankProcessSize;
		if (block == 0x4000) block = 0;
	}
}
