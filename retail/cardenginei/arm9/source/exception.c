#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include "igm_text.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define dsiBios BIT(11)
#define bypassExceptionHandler BIT(16)

#define EXCEPTION_VECTOR_SDK1	(*(VoidFn *)(0x27FFD9C))

extern cardengineArm9* volatile ce9;

extern u32 exceptionAddr;

extern s8 mainScreen;
extern vu32* volatile sharedAddr;

//---------------------------------------------------------------------------------
void userException() {
//---------------------------------------------------------------------------------
	sharedAddr[0] = 0x524F5245; // 'EROR'
	sharedAddr[5] = 0x4C4D4749; // 'IGML'

	extern void inGameMenu(s32* exRegisters);
	while (1) {
		inGameMenu(exceptionRegisters);
	}
}

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	#ifdef TWLSDK
	if (EXCEPTION_VECTOR == ((ce9->valueBits & bypassExceptionHandler) ? 0 : enterException) && *exceptionC == userException) return;
	#else
	if (!(ce9->valueBits & dsiBios)) {
		if (EXCEPTION_VECTOR_SDK1 == ((ce9->valueBits & bypassExceptionHandler) ? 0 : enterException) && *exceptionC == userException) return;
	} else {
		if (EXCEPTION_VECTOR == ((ce9->valueBits & bypassExceptionHandler) ? 0 : enterException) && *exceptionC == userException) return;
	}
	#endif

	#ifndef TWLSDK
	if (!(ce9->valueBits & dsiBios)) {
		exceptionAddr = 0x027FFD90;
	}
	#endif
	#ifdef TWLSDK
	exceptionStack = (u32)EXCEPTION_STACK_LOCATION_SDK5;
	EXCEPTION_VECTOR = (ce9->valueBits & bypassExceptionHandler) ? 0 : enterException;
	#else
	exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
	if (!(ce9->valueBits & dsiBios)) {
		EXCEPTION_VECTOR_SDK1 = (ce9->valueBits & bypassExceptionHandler) ? 0 : enterException;
	} else {
		EXCEPTION_VECTOR = (ce9->valueBits & bypassExceptionHandler) ? 0 : enterException;
	}
	#endif
	*exceptionC = userException;
}

