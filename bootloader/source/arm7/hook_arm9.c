#include <nds/ndstypes.h>
#include <nds/debug.h>
#include "hook.h"
#include "common.h"
#include "cardengine_header_arm9.h"

int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const module_params_t* moduleParams,
	u32 ROMinRAM,
	u32 dsiMode, // SDK 5
	u32 enableExceptionHandler,
	u32 consoleModel
) {
	nocashMessage("hookNdsRetailArm9");

	ce9->moduleParams           = moduleParams;
	ce9->ROMinRAM               = ROMinRAM;
	ce9->dsiMode                = dsiMode; // SDK 5
	ce9->enableExceptionHandler = enableExceptionHandler;
	ce9->consoleModel           = consoleModel;

	nocashMessage("ERR_NONE");
	return ERR_NONE;
}
