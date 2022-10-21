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

#ifdef TWLSDK
	if (ce9->consoleModel > 0) {
		*(u32*)(INGAME_MENU_LOCATION_DSIWARE + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
		volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION_DSIWARE + IGM_TEXT_SIZE_ALIGNED + 0x10;
		(*inGameMenu)(&mainScreen, ce9->consoleModel);
	} else {
		*(u32*)(INGAME_MENU_LOCATION_TWLSDK + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
		volatile void (*inGameMenu)(s8*, u32) = (volatile void*)INGAME_MENU_LOCATION_TWLSDK + IGM_TEXT_SIZE_ALIGNED + 0x10;
		(*inGameMenu)(&mainScreen, ce9->consoleModel);
	}
#else
	*(u32*)(INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
	volatile void (*inGameMenu)(s8*, u32, s32*) = (volatile void*)INGAME_MENU_LOCATION + IGM_TEXT_SIZE_ALIGNED + 0x10;
	(*inGameMenu)(&mainScreen, ce9->consoleModel, exceptionRegisters);
#endif
}

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	#ifdef TWLSDK
	if (EXCEPTION_VECTOR == enterException && *exceptionC == userException) return;
	#else
	if (EXCEPTION_VECTOR_SDK1 == enterException && *exceptionC == userException) return;
	#endif

	if (ce9->valueBits & dsiBios) {
		exceptionAddr = 0x02FFFD90;
	}
	exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
	#ifdef TWLSDK
	EXCEPTION_VECTOR = enterException;
	#else
	EXCEPTION_VECTOR_SDK1 = enterException;
	#endif
	*exceptionC = userException;
}

