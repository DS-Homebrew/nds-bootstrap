#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include "igm_text.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define bypassExceptionHandler BIT(16)

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
	if (EXCEPTION_VECTOR == ((ce9->valueBits & bypassExceptionHandler) ? 0 : enterException) && *exceptionC == userException) return;

	exceptionStack = (u32)EXCEPTION_STACK_LOCATION_SDK5;
	EXCEPTION_VECTOR = (ce9->valueBits & bypassExceptionHandler) ? 0 : enterException;
	*exceptionC = userException;
}

