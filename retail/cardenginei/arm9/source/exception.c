#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include "locations.h"
#include "cardengine_header_arm9.h"

#define dsiBios BIT(11)

#define EXCEPTION_VECTOR_SDK1	(*(VoidFn *)(0x27FFD9C))

extern cardengineArm9* volatile ce9;

void user_exception(void);
extern u32 exceptionAddr;

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	#ifdef TWLSDK
	if (EXCEPTION_VECTOR == enterException && *exceptionC == user_exception) return;
	#else
	if (EXCEPTION_VECTOR_SDK1 == enterException && *exceptionC == user_exception) return;
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
	*exceptionC = user_exception;
}

