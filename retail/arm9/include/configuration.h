#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//#include <limits.h> // ARG_MAX
#include <nds/ndstypes.h>
#include "cheat_engine.h"

typedef struct configuration {
    bool debug;
	char* ndsPath;
	char* savPath;
	char* apPatchPath;
	u32 saveSize;
	u32 apPatchSize;
	u8 language;
	u8 dsiMode; // SDK 5
	u32 donorSdkVer;
	u32 patchMpuRegion;
	u32 patchMpuSize;
	u32 consoleModel;
	u32 romread_LED;
	bool boostCpu;
	bool boostVram;
	bool gameSoftReset;
	bool forceSleepPatch;
	bool soundFix;
	bool logging;
	bool initDisc;
	bool dldiPatchNds;
	u32 backlightMode;
} configuration;

#endif // CONFIGURATION_H
