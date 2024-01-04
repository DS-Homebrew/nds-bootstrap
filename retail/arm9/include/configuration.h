#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//#include <limits.h> // ARG_MAX
#include <nds/ndstypes.h>

typedef struct configuration {
    bool debug;
	bool dsiWramAccess;
	int b4dsMode;
	bool loader2;
	char* ndsPath;
	char* appPath;
	char* savPath;
	char* prvPath;
	bool useSdk20Donor;
	bool useSdk5DonorAlt;
	char* donor20Path;
	char* donor5Path;
	char* donor5PathAlt;
	char* donorTwl0Path;
	char* donorTwlPath;
	char* donorTwlOnly0Path;
	char* donorTwlOnlyPath;
	//char* cleanDonorPath;
	char* gbaPath;
	char* gbaSavPath;
	char* apPatchPath;
	u32 donorFileSize;
	u32 donorFileOffset;
	u32 romSize;
	u32 saveSize;
	u32 gbaRomSize;
	u32 gbaSaveSize;
	u32 wideCheatSize;
	u32 apPatchSize;
	u32 cheatSize;
	u32 musicsSize;
	u32 dataToPreloadAddr[2];
	u32 dataToPreloadSize[2];
	u8 language;
	char* guiLanguage;
	s8 region; // SDK 5
	bool useRomRegion;
	bool sdNand; // SDK 5
	u8 dsiMode; // SDK 5
	u8 isDSiWare; // SDK 5
	u8 valueBits;
	u8 valueBits2;
	u8 valueBits3;
	u8 donorSdkVer;
	u8 patchMpuRegion;
	u32 patchMpuSize;
	u8 consoleModel;
	//int colorMode;
	u8 romRead_LED;
	u8 dmaRomRead_LED;
	bool asyncCardRead;
	int cardReadDMA;
	bool boostCpu;
	bool boostVram;
	bool soundFreq;
	bool forceSleepPatch;
	bool volumeFix;
	bool preciseVolumeControl;
	bool logging;
	bool initDisc;
	bool sdFound;
	bool bootstrapOnFlashcard;
	bool gameOnFlashcard;
	bool saveOnFlashcard;
	bool macroMode;
	bool sleepMode;
	u16 hotkey;
	bool specialCard;
	char* manualPath;
} configuration;

#endif // CONFIGURATION_H