#include <string.h>
#include <nds/ndstypes.h>
// #include <nds/arm9/background.h>
#include <nds/arm9/video.h>
#include <nds/system.h>
#include "dmaTwl.h"

extern u32 flags;

#define invertedColors BIT(0)
#define noWhiteFade BIT(1)
#define oneIrqOnly BIT(2)
#define twlClock BIT(3)

static u16* colorTable = (u16*)0x03770000;

void applyColorLut(bool processExtPalettes) {
	u32* storedPals = (u32*)0x0374B800;
	u32* palettes = (u32*)0x05000000;

	if (processExtPalettes && !(flags & oneIrqOnly)) {
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

	static bool initialized = false;
	if (!initialized) {
		u16* storedPals16 = (u16*)storedPals;
		u16* palettes16 = (u16*)palettes;
		for (int i = 0; i < 0x800/2; i++) {
			palettes16[i] = colorTable[palettes16[i] % 0x8000];
			storedPals16[i] = palettes16[i];
		}
		initialized = true;
	} else {
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
	}
	if (!(flags & oneIrqOnly)) return;

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
		static u8 initialized = 0;
		static int block = 0;
		storedPals = (u32*)0x0374C000+(block*(0x2000/4));
		palettes = (u32*)0x06880000+(block*(0x2000/4));

		if (!(initialized & BIT(block))) {
			VRAM_E_CR = 0x80;
			u16* storedPals16 = (u16*)storedPals;
			u16* palettes16 = (u16*)palettes;
			for (int i = 0; i < 0x2000/2; i++) {
				palettes16[i] = colorTable[palettes16[i] % 0x8000];
				storedPals16[i] = palettes16[i];
			}
			VRAM_E_CR = vramCnt;
			initialized |= BIT(block);
		} else {
			VRAM_E_CR = 0x80;
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
			VRAM_E_CR = vramCnt;
		}

		block++;
		if (block == 8) block = 0;
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
		static u8 initialized = 0;
		static int block = 0;
		storedPals = (u32*)0x0375C000+(block*(0x2000/4));
		palettes = (u32*)0x06890000+(block*(0x2000/4));

		if (!(initialized & BIT(block))) {
			VRAM_F_CR = 0x80;
			u16* storedPals16 = (u16*)storedPals;
			u16* palettes16 = (u16*)palettes;
			for (int i = 0; i < 0x2000/2; i++) {
				palettes16[i] = colorTable[palettes16[i] % 0x8000];
				storedPals16[i] = palettes16[i];
			}
			VRAM_F_CR = vramCnt;
			initialized |= BIT(block);
		} else {
			VRAM_F_CR = 0x80;
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
			VRAM_F_CR = vramCnt;
		}

		block++;
		if (block == 2) block = 0;
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
		static u8 initialized = 0;
		static int block = 0;
		storedPals = (u32*)0x03760000+(block*(0x2000/4));
		palettes = (u32*)0x06894000+(block*(0x2000/4));

		if (!(initialized & BIT(block))) {
			VRAM_G_CR = 0x80;
			u16* storedPals16 = (u16*)storedPals;
			u16* palettes16 = (u16*)palettes;
			for (int i = 0; i < 0x2000/2; i++) {
				palettes16[i] = colorTable[palettes16[i] % 0x8000];
				storedPals16[i] = palettes16[i];
			}
			VRAM_G_CR = vramCnt;
			initialized |= BIT(block);
		} else {
			VRAM_G_CR = 0x80;
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
			VRAM_G_CR = vramCnt;
		}

		block++;
		if (block == 2) block = 0;
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
		static u8 initialized = 0;
		static int block = 0;
		storedPals = (u32*)0x03764000+(block*(0x2000/4));
		palettes = (u32*)0x06898000+(block*(0x2000/4));

		if (!(initialized & BIT(block))) {
			VRAM_H_CR = 0x80;
			u16* storedPals16 = (u16*)storedPals;
			u16* palettes16 = (u16*)palettes;
			for (int i = 0; i < 0x2000/2; i++) {
				palettes16[i] = colorTable[palettes16[i] % 0x8000];
				storedPals16[i] = palettes16[i];
			}
			VRAM_H_CR = vramCnt;
			initialized |= BIT(block);
		} else {
			VRAM_H_CR = 0x80;
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
			VRAM_H_CR = vramCnt;
		}

		block++;
		if (block == 4) block = 0;
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
		static u8 initialized = 0;
		static int block = 0;
		storedPals = (u32*)0x0376C000+(block*(0x2000/4));
		palettes = (u32*)0x068A0000+(block*(0x2000/4));

		if (!(initialized & BIT(block))) {
			VRAM_I_CR = 0x80;
			u16* storedPals16 = (u16*)storedPals;
			u16* palettes16 = (u16*)palettes;
			for (int i = 0; i < 0x2000/2; i++) {
				palettes16[i] = colorTable[palettes16[i] % 0x8000];
				storedPals16[i] = palettes16[i];
			}
			VRAM_I_CR = vramCnt;
			initialized |= BIT(block);
		} else {
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

		block++;
		if (block == 2) block = 0;
	}

	// Commented due to slow code
	/* if ((REG_DISPCNT_SUB & DISPLAY_BG3_ACTIVE) && (REG_BG3CNT_SUB & BG_BMP8_256x256)) {
		const int mapBase = (REG_BG3CNT_SUB >> MAP_BASE_SHIFT) & 31;

		static int block = 0;
		storedPals = (u32*)0x03733800+(block/4);
		palettes = (u32*)BG_MAP_RAM_SUB(mapBase)+(block/4);

		for (int i = 0; i < 0x4000/4; i++) {
			if (*storedPals != *palettes) {
				u16* storedPals16 = (u16*)storedPals;
				u16* palettes16 = (u16*)palettes;
				for (int p = 0; p < 2; p++) {
					if (storedPals[p] != palettes16[p]) {
						const bool visibleBit = (palettes16[p] & BIT(15));
						palettes16[p] = colorTable[palettes16[p] % 0x8000];
						if (visibleBit) {
							palettes16[p] |= BIT(15);
						} else {
							palettes16[p] &= ~BIT(15);
						}
						storedPals16[p] = palettes16[p];
					}
				}
			}
			storedPals++;
			palettes++;
		}

		block += 0x4000;
		if (block == 0x18000) block = 0;
	} */
}

void applyColorLutBitmap(u32* frameBuffer) {
	extern u32 mobiclipFrameHeight;
	extern u32* mobiclipFrameDst;

	frameBuffer -= (256/2)*mobiclipFrameHeight;

	if (flags & twlClock) {
		for (int i = 0; i < (256/2)*mobiclipFrameHeight; i++) {
			u16* palettes16 = (u16*)frameBuffer;
			*mobiclipFrameDst++ = (colorTable[palettes16[0] % 0x8000] | BIT(15)) | (colorTable[palettes16[1] % 0x8000] | BIT(15)) << 16;
			frameBuffer++;
		}
	} else {
		// Draw frame with halved resolution to slightly speed up process for NTR clock speed
		int w = 0;
		for (int i = 0; i < (256/2)*(mobiclipFrameHeight/2); i++) {
			u16* palettes16 = (u16*)frameBuffer;
			const u16 colorSingle = colorTable[palettes16[0] % 0x8000] | BIT(15);
			*mobiclipFrameDst++ = colorSingle | (colorSingle << 16); // Cut horizontal resolution in half
			frameBuffer++;
			w++;
			if (w == 256/2) { // Cut vertical resolution in half
				dma_twlCopy32Async(3, mobiclipFrameDst-(256/2), mobiclipFrameDst, 256*2);
				mobiclipFrameDst += w;
				frameBuffer += w;
				w = 0;
			}
		}
	}
}