#include "inGameMenu.h"

#include <nds/arm9/background.h>

#include "igm_text.h"
#include "tonccpy.h"

bool exceptionPrinted = false;

static const char *registerNames[] = {
	"r0", "r1", "r2",  "r3",  "r4",  "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
};

static s32 *exceptionRegisters;
static const u32 __itcm_start = 0;

u32 getCPSR();

//---------------------------------------------------------------------------------
// From libnds
// https://github.com/devkitPro/libnds/blob/154a21cc3d57716f773ff2b10f815511c1b8ba9f/source/arm9/gurumeditation.c#L36
u32 ARMShift(u32 value, u8 shift) {
//---------------------------------------------------------------------------------
	// no shift at all
	if(shift == 0x0B)
		return value;

	int index;
	if(shift & 0x01) // shift index is a register
		index = exceptionRegisters[(shift >> 4) & 0x0F];
	else // constant shift index
		index = ((shift >> 3) & 0x1F);

	switch(shift & 0x06) {
		case 0x00: // logical left
			return value << index;
		case 0x02: // logical right
			return (value >> index);
		case 0x04: // arithmetical right
		{
			bool isN = value & 0x80000000;
			value = value >> index;
			if(isN) {
				for(int i = 31; i > 31 - index; i--) {
					value = value | (1 << i);
				}
			}
			return value;
		}
		case 0x06: // rotate right
			index = index & 0x1F;
			value = (value >> index) | (value << (32-index));
			return value;
	}

	return value;
}


//---------------------------------------------------------------------------------
// From libnds
// https://github.com/devkitPro/libnds/blob/154a21cc3d57716f773ff2b10f815511c1b8ba9f/source/arm9/gurumeditation.c#L78
u32 getExceptionAddress(u32 opcodeAddress, u32 thumbState) {
//---------------------------------------------------------------------------------
	int Rf, Rb, Rd, Rn, Rm;

	if(thumbState) {
		// Thumb

		u16 opcode = *(u16 *)opcodeAddress;
		// ldr r,[pc,###]			01001ddd ffffffff
		// ldr r,[r,r]				0101xx0f ffbbbddd
		// ldrsh					0101xx1f ffbbbddd
		// ldr r,[r,imm]			011xxfff ffbbbddd
		// ldrh						1000xfff ffbbbddd
		// ldr r,[sp,###]			1001xddd ffffffff
		// push						1011x10l llllllll
		// ldm						1100xbbb llllllll


		if((opcode & 0xF800) == 0x4800) {
			// ldr r,[pc,###]
			s8 offset = opcode & 0xff;
			return exceptionRegisters[15] + offset;
		} else if((opcode & 0xF200) == 0x5000) {
			// ldr r,[r,r]
			Rb = (opcode >> 3) & 0x07;
			Rf = (opcode >> 6) & 0x07;
			return exceptionRegisters[Rb] + exceptionRegisters[Rf];

		} else if((opcode & 0xF200) == 0x5200) {
			// ldrsh
			Rb = (opcode >> 3) & 0x07;
			Rf = (opcode >> 6) & 0x03;
			return exceptionRegisters[Rb] + exceptionRegisters[Rf];

		} else if((opcode & 0xE000) == 0x6000) {
			// ldr r,[r,imm]
			Rb = (opcode >> 3) & 0x07;
			Rf = (opcode >> 6) & 0x1F;
			return exceptionRegisters[Rb] + (Rf << 2);
		} else if((opcode & 0xF000) == 0x8000) {
			// ldrh
			Rb = (opcode >> 3) & 0x07;
			Rf = (opcode >> 6) & 0x1F;
			return exceptionRegisters[Rb] + (Rf << 2);
		} else if((opcode & 0xF000) == 0x9000) {
			// ldr r,[sp,#imm]
			s8 offset = opcode & 0xff;
			return exceptionRegisters[13] + offset;
		} else if((opcode & 0xF700) == 0xB500) {
			// push/pop
			return exceptionRegisters[13];
		} else if((opcode & 0xF000) == 0xC000) {
			// ldm/stm
			Rd = (opcode >> 8) & 0x07;
			return exceptionRegisters[Rd];
		}
	} else {
		// arm32

		u32 opcode = *(u32 *)opcodeAddress;
		// SWP			xxxx0001 0x00nnnn dddd0000 1001mmmm
		// STR/LDR		xxxx01xx xxxxnnnn ddddffff ffffffff
		// STRH/LDRH	xxxx000x x0xxnnnn dddd0000 1xx1mmmm
		// STRH/LDRH	xxxx000x x1xxnnnn ddddffff 1xx1ffff
		// STM/LDM		xxxx100x xxxxnnnn llllllll llllllll

		if((opcode & 0x0FB00FF0) == 0x01000090) {
			// SWP
			Rn = (opcode >> 16) & 0x0F;
			return exceptionRegisters[Rn];
		} else if((opcode & 0x0C000000) == 0x04000000) {
			// STR/LDR
			Rn = (opcode >> 16) & 0x0F;
			if(opcode & 0x02000000) {
				// Register offset
				Rm = opcode & 0x0F;
				if(opcode & 0x01000000) {
					u16 shift = (u16)((opcode >> 4) & 0xFF);
					// pre indexing
					s32 Offset = ARMShift(exceptionRegisters[Rm],shift);
					// add or sub the offset depending on the U-Bit
					return exceptionRegisters[Rn] + ((opcode & 0x00800000)?Offset:-Offset);
				} else {
					// post indexing
					return exceptionRegisters[Rn];
				}
			} else {
				// Immediate offset
				u32 Offset = (opcode & 0xFFF);
				if(opcode & 0x01000000) {
					// pre indexing
					// add or sub the offset depending on the U-Bit
					return exceptionRegisters[Rn] + ((opcode & 0x00800000)?Offset:-Offset);
				} else {
					// post indexing
					return exceptionRegisters[Rn];
				}
			}
		} else if((opcode & 0x0E400F90) == 0x00000090) {
			// LDRH/STRH with register Rm
			Rn = (opcode >> 16) & 0x0F;
			Rd = (opcode >> 12) & 0x0F;
			Rm = opcode & 0x0F;
			u16 shift = (u16)((opcode >> 4) & 0xFF);
			s32 Offset = ARMShift(exceptionRegisters[Rm],shift);
			// add or sub the offset depending on the U-Bit
			return exceptionRegisters[Rn] + ((opcode & 0x00800000)?Offset:-Offset);
		} else if((opcode & 0x0E400F90) == 0x00400090) {
			// LDRH/STRH with immediate offset
			Rn = (opcode >> 16) & 0x0F;
			Rd = (opcode >> 12) & 0x0F;
			u32 Offset = (opcode & 0xF) | ((opcode & 0xF00)>>8);
			// add or sub the offset depending on the U-Bit
			return exceptionRegisters[Rn] + ((opcode & 0x00800000)?Offset:-Offset);
		} else if((opcode & 0x0E000000) == 0x08000000) {
			// LDM/STM
			Rn = (opcode >> 16) & 0x0F;
			return exceptionRegisters[Rn];
		}
	}

	return 0;
}


void showException(s32 *expReg) {
	if (exceptionPrinted) return;
	exceptionPrinted = true;

	exceptionRegisters = expReg;

	// Take over the main screen
	REG_DISPCNT = MODE_0_2D | DISPLAY_BG3_ACTIVE;
	REG_BG0CNT = 0;
	REG_BG1CNT = 0;
	REG_BG2CNT = 0;
	REG_BG3CNT = (u16)(BG_MAP_BASE(15) | BG_TILE_BASE(0) | BgSize_T_256x256);

	VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;

	REG_BG3VOFS = 0;
	REG_BG3HOFS = 0;

	SetBrightness(0, 0);
	REG_BLDY = 0;

	clearScreen(true);

	toncset16(BG_PALETTE, 0, 256);
	for(int i = 0; i < sizeof(igmPal) / sizeof(igmPal[0]); i++) {
		BG_PALETTE[i * 0x10 + 1] = igmPal[i];
	}

	for(int i = 0; i < sizeof(igmText.font); i++) {	// Load font from 1bpp to 4bpp
		u8 val = igmText.font[i];
		BG_GFX[i * 2]     = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
		val >>= 4;
		BG_GFX[i * 2 + 1] = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
	}

	// Make the background red
	BG_PALETTE[0] = 0x0010;
	BG_PALETTE_SUB[0] = 0x0010;

	// Print out the exception
	u32 currentMode = getCPSR() & 0x1f;
	u32 thumbStateAddr = (u32)sharedAddr;
	thumbStateAddr += 4;
	thumbStateAddr += 0x80;
	u32 thumbState = ((*(u32*)thumbStateAddr) & 0x20);

	u32 codeAddress, exceptionAddress = 0;

	int offset = 8;

	if(currentMode == 0x17) {
		printCenter(15, 1, (const u8 *)"Error: Data Abort!", FONT_WHITE, true);
		codeAddress = exceptionRegisters[15] - offset;
		if((codeAddress > 0x02000000 && codeAddress < 0x02400000) || (codeAddress > (u32)__itcm_start && codeAddress < (u32)(__itcm_start + 32768)))
			exceptionAddress = getExceptionAddress(codeAddress, thumbState);
		else
			exceptionAddress = codeAddress;
	} else {
		if(thumbState)
			offset = 2;
		else
			offset = 4;
		printCenter(15, 1, (const u8 *)"Error: Undefined Instruction!", FONT_WHITE, true);
		codeAddress = exceptionRegisters[15] - offset;
		exceptionAddress = codeAddress;
	}

	print(2, 3, (const u8 *)"PC:           ADDR:", FONT_WHITE, true);
	printHex(6, 3, codeAddress, 4, FONT_LIGHT_BLUE, true);
	printHex(22, 3, exceptionAddress, 4, FONT_LIGHT_BLUE, true);

	for(int i = 0; i < 8; i++) {
		print(2, 5 + i, (const u8 *)registerNames[i], FONT_WHITE, true);
		printHex(6, 5 + i, exceptionRegisters[i], 4, FONT_WHITE, true);
		print(17, 5 + i, (const u8 *)registerNames[i + 8], FONT_WHITE, true);
		printHex(22, 5 + i, exceptionRegisters[i + 8], 4, FONT_WHITE, true);
	}

	u32 *stack = (u32 *)exceptionRegisters[13];
	for(int i = 0; i < 10; i++) {
		printHex(2, 14 + i, (u32)stack, 4, FONT_LIGHT_BLUE, true);
		printChar(10, 14 + i, ':', FONT_WHITE, true);
		printHex(13, 14 + i, *(stack++), 4, FONT_WHITE, true);
		printHex(22, 14 + i, *(stack++), 4, FONT_WHITE, true);
	}
}
