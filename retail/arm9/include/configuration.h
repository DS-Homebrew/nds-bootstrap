#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//#include <limits.h> // ARG_MAX
#include <nds/ndstypes.h>
#include "cheat_engine.h"

typedef struct configuration {
    bool debug;
    bool cacheFatTable;
	char* ndsPath;
	char* savPath;
	char* donorE2Path;
	char* donor2Path;
	char* donor3Path;
	char* donorPath;
	char* donorTwlPath;
	char* gbaPath;
	char* gbaSavPath;
	char* apPatchPath;
	u32 romSize;
	u32 saveSize;
	u32 gbaRomSize;
	u32 gbaSaveSize;
	u32 wideCheatSize;
	u32 apPatchSize;
	u32 cheatSize;
	u8 language;
	u8 dsiMode; // SDK 5
	u32 donorSdkVer;
	u32 patchMpuRegion;
	u32 patchMpuSize;
	bool ceCached;	// SDK 1-4
	int cacheBlockSize;
	int extendedMemory;
	u32 consoleModel;
	int colorMode;
	u32 romRead_LED;
	u32 dmaRomRead_LED;
	bool boostCpu;
	bool boostVram;
	bool soundFreq;
	bool forceSleepPatch;
	bool volumeFix;
	bool preciseVolumeControl;
	bool logging;
	bool initDisc;
	bool sdFound;
	bool gameOnFlashcard;
	bool saveOnFlashcard;
	u32 donorOnFlashcard;
	bool macroMode;
} configuration;

#endif // CONFIGURATION_H