#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include "igm_text.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define EXCEPTION_VECTOR_SDK1	(*(VoidFn *)(0x27FFD9C))

extern cardengineArm9* volatile ce9;

extern u32 exceptionAddr;

extern s8 mainScreen;
extern vu32* volatile sharedAddr;

#ifndef NODSIWARE
// static u32* debugArea = (u32*)0x02004000;

//---------------------------------------------------------------------------------
bool newSlot2Access() {
//---------------------------------------------------------------------------------
	if ((getCPSR() & 0x1f) != 0x17) {
		return false;
	}

	u32 opcode = 0;
	opcode = (*(u32*)0x027FFD98)-8;
	if (opcode < 0x01FF8000 && opcode >= 0x02400000) {
		return false;
	}
	opcode = *(u32*)opcode;

	// debugArea[0] = opcode;
	// debugArea[1] = 0;

	int regL = 0, regR = 0;
	u32 opcodePushPop = opcode;

	{
		while (opcode >= 0x10000000) {
			opcode -= 0x10000000;
			opcodePushPop -= 0x10000000;
		}

		u32 opcodeTemp = opcode;
		while (opcodeTemp >= 0x01000000) {
			opcodeTemp -= 0x01000000;
		}

		while (opcodeTemp >= 0x00100000) {
			opcodeTemp -= 0x00100000;
		}

		while (opcodeTemp >= 0x00010000) {
			opcodeTemp -= 0x00010000;
			opcode -= 0x00010000;
			opcodePushPop -= 0x00010000;
			regR++;
			if (regR > 12) return false;
		}

		while (opcodeTemp >= 0x00001000) {
			opcodeTemp -= 0x00001000;
			opcode -= 0x00001000;
			regL++;
			if (regL > 12) return false;
		}
	}

	/* debugArea[0] = opcode;
	debugArea[1] = 1; */
	/* debugArea[1] = regR;
	debugArea[2] = regL; */

	const bool regInRange = (exceptionRegisters[regR] >= 0x0C000000 && exceptionRegisters[regR] < 0x10000000);
	if (!regInRange) {
		return false;
	}

	const u32 fixedRegR = exceptionRegisters[regR] - 0x04000000;

	const u16 offsetChange = (u16)opcode;
	const u8 opcodeLastByte = (opcode & 0xFF);

	u32 opcodeAlt = opcode;
	opcode -= offsetChange;

	/* debugArea[0] = opcode;
	debugArea[1] = 2; */

	if (opcodePushPop == 0x09900003) { // ldmib [rR], r0, r1
		exceptionRegisters[0] = *(u32*)(fixedRegR+4);
		exceptionRegisters[1] = *(u32*)(fixedRegR+8);
		return true;
	} else
	if (opcodePushPop == 0x09800003) { // stmib [rR], r0, r1
		*(u32*)(fixedRegR+4) = exceptionRegisters[0];
		*(u32*)(fixedRegR+8) = exceptionRegisters[1];
		return true;
	} else
	if (opcode == 0x05900000) { // ldr rL, [rR]
		exceptionRegisters[regL] = *(u32*)(fixedRegR+offsetChange);
		return true;
	} else
	if (opcode == 0x05800000) { // str rL, [rR]
		*(u32*)(fixedRegR+offsetChange) = exceptionRegisters[regL];
		return true;
	} else
	if (opcode == 0x05100000) { // ldr rL, [rR, #-?]
		exceptionRegisters[regL] = *(u32*)(fixedRegR-offsetChange);
		return true;
	} else
	if (opcode == 0x05000000) { // str rL, [rR, #-?]
		*(u32*)(fixedRegR-offsetChange) = exceptionRegisters[regL];
		return true;
	} else
	if (opcode == 0x04900000) { // ldr rL, [rR],#0-#0xFFF
		exceptionRegisters[regL] = *(u32*)fixedRegR;
		exceptionRegisters[regR] += offsetChange;
		return true;
	} else
	if (opcode == 0x04800000) { // str rL, [rR],#0-#0xFFF
		*(u32*)fixedRegR = exceptionRegisters[regL];
		exceptionRegisters[regR] += offsetChange;
		return true;
	} else
	if (opcodeAlt >= 0x07900000 && opcodeAlt <= 0x0790000C) { // ldr rL, [rR, r0-r12]
		exceptionRegisters[regL] = *(u32*)(fixedRegR+exceptionRegisters[opcodeLastByte]);
		return true;
	} else
	if (opcodeAlt >= 0x07800000 && opcodeAlt <= 0x0780000C) { // str rL, [rR, r0-r12]
		*(u32*)(fixedRegR+exceptionRegisters[opcodeLastByte]) = exceptionRegisters[regL];
		return true;
	} else
	if ((opcode == 0x01D00000 || opcode == 0x01C00000) && opcodeLastByte >= 0xB0 && opcodeLastByte <= 0xBF) { // ldrh/strh rL, [rR]
		u8 offsetChange = opcodeLastByte - 0xB0;

		opcodeAlt -= opcode;

		while (opcodeAlt >= 0x00000100) {
			opcodeAlt -= 0x00000100;
			offsetChange += 0x10;
		}

		if (opcode == 0x01D00000) {
			exceptionRegisters[regL] = *(u16*)(fixedRegR+offsetChange);
		} else {
			*(u16*)(fixedRegR+offsetChange) = (u16)exceptionRegisters[regL];
		}
		return true;
	} else
	if ((opcode == 0x00D00000 || opcode == 0x00C00000) && opcodeLastByte >= 0xB0 && opcodeLastByte <= 0xBF) { // ldrh/strh rL, [rR],#0-#0xFF
		if (opcode == 0x00D00000) {
			exceptionRegisters[regL] = *(u16*)fixedRegR;
		} else {
			*(u16*)fixedRegR = (u16)exceptionRegisters[regL];
		}
		u8 regAddCount = opcodeLastByte - 0xB0;

		opcodeAlt -= opcode;

		while (opcodeAlt >= 0x00000100) {
			opcodeAlt -= 0x00000100;
			regAddCount += 0x10;
		}

		exceptionRegisters[regR] += regAddCount;
		return true;
	} else
	if (opcode == 0x05D00000) { // ldrb rL, [rR]
		exceptionRegisters[regL] = *(u8*)(fixedRegR+offsetChange);
		return true;
	} else
	if (opcode == 0x04D00000) { // ldrb rL, [rR],#0-#0xFFF
		exceptionRegisters[regL] = *(u8*)fixedRegR;
		exceptionRegisters[regR] += offsetChange;
		return true;
	} else
	if ((opcode == 0x07D00000) && ((opcodeLastByte >= 0x00 && opcodeLastByte <= 0x0C) || (opcodeLastByte >= 0x80 && opcodeLastByte <= 0x8C))) { // ldrb rL, [rR, r0-r12, lsl #?]
		u8 lsl = (opcodeLastByte >= 0x80 && opcodeLastByte <= 0x8C) ? 1 : 0;

		opcodeAlt -= opcode;

		while (opcodeAlt >= 0x00000100) {
			opcodeAlt -= 0x00000100;
			lsl += 2;
		}

		u32 add = exceptionRegisters[opcodeLastByte - 0x80];
		while (lsl > 1) {
			add += add;
			lsl--;
		}

		exceptionRegisters[regL] = *(u8*)(fixedRegR+add);
		return true;
	}

	// debugArea[1] = 3;

	return false;
}
#endif

//---------------------------------------------------------------------------------
void userException() {
//---------------------------------------------------------------------------------
	sharedAddr[0] = 0x524F5245; // 'EROR'

	extern void inGameMenu(s32* exRegisters);
	while (1) {
		inGameMenu(exceptionRegisters);
	}
}

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	if (EXCEPTION_VECTOR_SDK1 == enterException && *exceptionC == userException) return;

	exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
	EXCEPTION_VECTOR_SDK1 = enterException;
	*exceptionC = userException;
}

