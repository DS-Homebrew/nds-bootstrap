#include <stdio.h>
#include <nds/ndstypes.h>
#include <nds/debug.h>
#include <nds/system.h>
#include "hook.h"
#include "common.h"
#include "find.h"
#include "cardengine_header_arm9.h"

int hookNdsRetailArm9(
	cardengineArm9* ce9,
	const module_params_t* moduleParams,
	u32 fileCluster,
	u32 saveCluster,
	u32 saveOnFlashcard,
	u32 ROMinRAM,
	u32 dsiMode, // SDK 5
	u32 enableExceptionHandler,
	u32 consoleModel
) {
	nocashMessage("hookNdsRetailArm9");

	ce9->moduleParams           = moduleParams;
	ce9->fileCluster            = fileCluster;
	ce9->saveCluster            = saveCluster;
	ce9->saveOnFlashcard        = saveOnFlashcard;
	ce9->ROMinRAM               = ROMinRAM;
	ce9->dsiMode                = dsiMode; // SDK 5
	ce9->enableExceptionHandler = enableExceptionHandler;
	ce9->consoleModel           = consoleModel;
    
	nocashMessage("ERR_NONE");
	return ERR_NONE;
}
