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
// extern u32* debugArea;

#ifndef NODSIWARE
//---------------------------------------------------------------------------------
bool readOutsideWord() {
//---------------------------------------------------------------------------------
	int regSrc = 0;
	extern u32 regDst;
	regDst = 0;

	extern u32 regToAdd;
	extern u32 regAddCount;
	regToAdd = 12;

	static u32 opcode = 0;
	opcode = (*(u32*)0x027FFD98)-8;
	opcode = *(u32*)opcode;

	// debugArea[0] = opcode;

	{
		while (opcode >= 0x10000000) {
			opcode -= 0x10000000;
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
			regSrc++;
			if (regSrc > 12) return false;
		}

		while (opcodeTemp >= 0x00001000) {
			opcodeTemp -= 0x00001000;
			opcode -= 0x00001000;
			regDst++;
			if (regDst > 12) return false;
		}
	}

	/* debugArea[0] = opcode;
	debugArea[1] = regDst;
	debugArea[2] = regSrc; */

	const bool regInRange = (exceptionRegisters[regSrc] >= 0x0C000000 && exceptionRegisters[regSrc] < 0x10000000);
	if (!regInRange) {
		return false;
	}

	const u8 opcodeLastByte = (opcode & 0xFF);
	extern void readRomBlock(u32 src, u8 len);

	if (opcode >= 0x05900000 && opcode <= 0x05900FFF) { // ldr rDst, [rSrc]
		const u16 offsetChange = (u16)opcode;
		readRomBlock(exceptionRegisters[regSrc]+offsetChange, 4);
		return true;
	} else
	if (opcode >= 0x04900000 && opcode <= 0x04900FFF) { // ldr rDst, [rSrc],#0-#0xFFF
		readRomBlock(exceptionRegisters[regSrc], 4);
		regToAdd = regSrc;
		regAddCount = (u16)opcode;
		return true;
	} else
	if (opcode >= 0x07900000 && opcode <= 0x0790000C) { // ldr rDst, [rSrc, r0-r12]
		readRomBlock(exceptionRegisters[regSrc]+exceptionRegisters[opcodeLastByte], 4);
		return true;
	} else
	if (opcode >= 0x01D00000 && opcode <= 0x01D00FFF && opcodeLastByte >= 0xB0 && opcodeLastByte <= 0xBF) { // ldrh rDst, [rSrc]
		u8 offsetChange = opcodeLastByte - 0xB0;

		opcode -= 0x01D00000;

		while (opcode >= 0x00000100) {
			opcode -= 0x00000100;
			offsetChange += 0x10;
		}

		readRomBlock(exceptionRegisters[regSrc]+offsetChange, 2);
		return true;
	} else
	if (opcode >= 0x00D00000 && opcode <= 0x00D00FFF && opcodeLastByte >= 0xB0 && opcodeLastByte <= 0xBF) { // ldrh rDst, [rSrc],#0-#0xFF
		readRomBlock(exceptionRegisters[regSrc], 2);
		regToAdd = regSrc;
		regAddCount = opcodeLastByte - 0xB0;

		opcode -= 0x00D00000;

		while (opcode >= 0x00000100) {
			opcode -= 0x00000100;
			regAddCount += 0x10;
		}

		return true;
	} else
	if (opcode >= 0x05D00000 && opcode <= 0x05D00FFF) { // ldrb rDst, [rSrc]
		const u16 offsetChange = (u16)opcode;
		readRomBlock(exceptionRegisters[regSrc]+offsetChange, 1);
		return true;
	} else
	if (opcode >= 0x04D00000 && opcode <= 0x04D00FFF) { // ldrb rDst, [rSrc],#0-#0xFFF
		readRomBlock(exceptionRegisters[regSrc], 1);
		regToAdd = regSrc;
		regAddCount = (u16)opcode;
		return true;
	}

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

