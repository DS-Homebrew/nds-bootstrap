#include <nds/ndstypes.h>
#include "hook.h"

//extern bool sdk5;

extern u32 ROMinRAM;
extern u32 ROM_TID;
extern u32 ROM_HEADERCRC;
extern u32 ARM9_LEN;
extern u32 ARM7_LEN; // SDK 5
extern u32 romSize;
extern bool dsiModeConfirmed; // SDK 5
extern u32 enableExceptionHandler;
extern unsigned long consoleModel;
extern unsigned long asyncPrefetch;

void hookNdsRetailArm9(u32* cardEngineLocationArm9) {
	cardEngineLocationArm9[7]  = ROMinRAM;
	cardEngineLocationArm9[8]  = ROM_TID;
	cardEngineLocationArm9[9]  = ROM_HEADERCRC;
	cardEngineLocationArm9[10] = ARM9_LEN;
	cardEngineLocationArm9[11] = romSize;
	cardEngineLocationArm9[12] = dsiModeConfirmed; // SDK 5
	cardEngineLocationArm9[13] = enableExceptionHandler;
	cardEngineLocationArm9[14] = consoleModel;
	cardEngineLocationArm9[15] = asyncPrefetch;
}
