#include <nds/ndstypes.h>
#include "hook.h"
#include "cardengine_header_arm9.h"

//extern bool sdk5;

extern u32 ROMinRAM;
extern u32 ROM_TID;
extern u32 ROM_HEADERCRC;
extern u32 ARM9_LEN;
extern u32 romSize;
extern bool dsiModeConfirmed; // SDK 5
extern u32 enableExceptionHandler;
extern unsigned long consoleModel;
extern unsigned long asyncPrefetch;

void hookNdsRetailArm9(u32* cardEngineLocationArm9) {
	cardEngineLocationArm9[CE9_ROM_IN_RAM_OFFSET]               = ROMinRAM;
	cardEngineLocationArm9[CE9_ROM_TID_OFFSET]                  = ROM_TID;
	cardEngineLocationArm9[CE9_ROM_HEADERCRC_OFFSET]            = ROM_HEADERCRC;
	cardEngineLocationArm9[CE9_ARM9_LEN_OFFSET]                 = ARM9_LEN;
	cardEngineLocationArm9[CE9_ROM_SIZE_OFFSET]                 = romSize;
	cardEngineLocationArm9[CE9_DSI_MODE_OFFSET]                 = dsiModeConfirmed; // SDK 5
	cardEngineLocationArm9[CE9_ENABLE_EXCEPTION_HANDLER_OFFSET] = enableExceptionHandler;
	cardEngineLocationArm9[CE9_CONSOLE_MODEL_OFFSET]            = consoleModel;
	cardEngineLocationArm9[CE9_ASYNC_PREFETCH_OFFSET]           = asyncPrefetch;
}
