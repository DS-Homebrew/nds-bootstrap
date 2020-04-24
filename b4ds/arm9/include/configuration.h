#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//#include <limits.h> // ARG_MAX
#include <nds/ndstypes.h>
#include "cheat_engine.h"

typedef struct configuration {
    bool debug;
	char* ndsPath;
	char* savPath;
	char* donorE2Path;
	char* donor2Path;
	char* donorPath;
	char* apPatchPath;
	u32 romSize;
	u32 saveSize;
	u32 apPatchSize;
	u8 language;
	u32 donorSdkVer;
	u32 patchMpuRegion;
	u32 patchMpuSize;
	bool ceCached;	// SDK 1-4
	bool boostCpu;
	bool boostVram;
	bool forceSleepPatch;
	bool logging;
	bool initDisc;
	bool dldiPatchNds;
	u32 backlightMode;
} configuration;

#endif // CONFIGURATION_H