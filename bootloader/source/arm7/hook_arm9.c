#include <nds/ndstypes.h>
#include "hook.h"
#include "common.h"
#include "cardengine_header_arm9.h"

//extern u32 ROMinRAM;
//extern bool dsiModeConfirmed; // SDK 5
//extern u32 enableExceptionHandler;
//extern unsigned long consoleModel;
//extern unsigned long asyncPrefetch;

int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 dsiMode, // SDK 5
	u32 enableExceptionHandler,
	u32 consoleModel,
	u32 asyncPrefetch
) {
	ce9->moduleParams           = moduleParams;
	ce9->ROMinRAM               = ROMinRAM;
	ce9->dsiMode                = dsiMode; // SDK 5
	ce9->enableExceptionHandler = enableExceptionHandler;
	ce9->consoleModel           = consoleModel;
	ce9->asyncPrefetch          = asyncPrefetch;

	return ERR_NONE;
}
