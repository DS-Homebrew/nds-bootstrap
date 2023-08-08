#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include "igm_text.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

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
	if (EXCEPTION_VECTOR == enterException && *exceptionC == userException) return;

	exceptionStack = (u32)EXCEPTION_STACK_LOCATION_SDK5;
	EXCEPTION_VECTOR = enterException;
	*exceptionC = userException;
}

