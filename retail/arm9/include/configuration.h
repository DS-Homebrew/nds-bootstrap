#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//#include <limits.h> // ARG_MAX
#include <nds/ndstypes.h>
#include "cheat_engine.h"

typedef struct configuration {
    bool debug;
	char* ndsPath;
	char* savPath;
	char* gbaPath;
	char* apPatchPath;
	u32 romSize;
	u32 saveSize;
	u32 wideCheatSize;
	u32 apPatchSize;
	u32 cheatSize;
	u8 language;
	u8 dsiMode; // SDK 5
	u32 donorSdkVer;
	u32 patchMpuRegion;
	u32 patchMpuSize;
	bool ceCached;	// SDK 1-4
	u32 consoleModel;
	int colorMode;
	u32 romread_LED;
	bool boostCpu;
	bool boostVram;
	bool soundFreq;
	bool gameSoftReset;
	bool forceSleepPatch;
	bool volumeFix;
	bool preciseVolumeControl;
	bool logging;
	bool initDisc;
	bool sdFound;
	bool gameOnFlashcard;
	bool saveOnFlashcard;
	u32 backlightMode;
} configuration;

#endif // CONFIGURATION_H