#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include "igm_text.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define dsiBios BIT(11)

#define EXCEPTION_VECTOR_SDK1	(*(VoidFn *)(0x27FFD9C))

extern cardengineArm9* volatile ce9;

extern u32 exceptionAddr;

extern s8 mainScreen;
extern vu32* volatile sharedAddr;

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
	#ifdef TWLSDK
	if (EXCEPTION_VECTOR == enterException && *exceptionC == userException) return;
	#else
	if (EXCEPTION_VECTOR_SDK1 == enterException && *exceptionC == userException) return;
	#endif

	#ifndef TWLSDK
	if (!(ce9->valueBits & dsiBios)) {
		exceptionAddr = 0x027FFD90;
	}
	#endif
	#ifdef TWLSDK
	exceptionStack = (u32)EXCEPTION_STACK_LOCATION_SDK5;
	EXCEPTION_VECTOR = enterException;
	#else
	exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
	EXCEPTION_VECTOR_SDK1 = enterException;
	#endif
	*exceptionC = userException;
}

