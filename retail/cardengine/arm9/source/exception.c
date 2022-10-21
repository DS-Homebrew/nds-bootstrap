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
	*(u32*)(INGAME_MENU_LOCATION_B4DS + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
	volatile void (*inGameMenu)(s8*, u32, s32*) = (volatile void*)INGAME_MENU_LOCATION_B4DS + IGM_TEXT_SIZE_ALIGNED + 0x10;
	(*inGameMenu)(&mainScreen, 0, exceptionRegisters);
}

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	if (EXCEPTION_VECTOR_SDK1 == enterException && *exceptionC == userException) return;

	exceptionStack = (u32)EXCEPTION_STACK_LOCATION_B4DS;
	EXCEPTION_VECTOR_SDK1 = enterException;
	*exceptionC = userException;
}

