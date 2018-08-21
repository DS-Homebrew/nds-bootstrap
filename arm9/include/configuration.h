#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <nds/ndstypes.h>
#include "cheat_engine.h"

typedef struct configuration {
    u32 status;
	bool debug;
	char* filename; // ndsPath
	char* savPath;
	u32 saveSize;
	u32 language;
	bool dsiMode; // SDK 5
	u32 donorSdkVer;
	u32 patchMpuRegion;
	u32 patchMpuSize;
	u32 consoleModel;
	u32 loadingScreen;
	u32 romread_LED;
	bool boostCpu;
	bool gameSoftReset;
	bool asyncPrefetch;
	bool logging;
	bool initDisc;
	bool dldiPatchNds;
	int argc;
	const char** argv;
	u32 cheat_data[CHEAT_DATA_MAX_LEN];
	u32 cheat_data_len;
	u32 backlightMode;
} configuration;

#endif // CONFIGURATION_H
