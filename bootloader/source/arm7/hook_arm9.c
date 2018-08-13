#include <nds/ndstypes.h>
#include "hook.h"
#include "cardengine_header_arm9.h"

extern u32 ROMinRAM;
extern bool dsiModeConfirmed; // SDK 5
extern u32 enableExceptionHandler;
extern unsigned long consoleModel;
extern unsigned long asyncPrefetch;

void hookNdsRetailArm9(cardengineArm9* ce9) {
	ce9->ROMinRAM                 = ROMinRAM;
	ce9->dsiMode                  = dsiModeConfirmed; // SDK 5
	ce9->enableExceptionHandler   = enableExceptionHandler;
	ce9->consoleModel             = consoleModel;
	ce9->asyncPrefetch            = asyncPrefetch;
}
