#ifndef LOAD_CRT0_H
#define LOAD_CRT0_H

#include <nds/ndstypes.h>
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"

typedef struct loadCrt0 {
    u32 _start;
    u32 storedFileCluster;
    u32 initDisc;
    u16 bootstrapOnFlashcard;
    u8 gameOnFlashcard;
    u8 saveOnFlashcard;
    u32 dldiOffset;
    u16 a9ScfgRom;
    u8 dsiSD;
    u8 valueBits;
    u32 saveFileCluster;
	u32 donorFileCluster;
	u32 donorFileSize;
	u32 donorFileOffset;
    // u32 gbaFileCluster;
    // u32 gbaSaveFileCluster;
    u32 romSize;
    u32 saveSize;
    // u32 gbaRomSize;
    // u32 gbaSaveSize;
	u32 dataToPreloadAddr[4];
	u32 dataToPreloadSize[4];
	// u32 dataToPreloadFrame;
    u32 wideCheatFileCluster;
    u32 wideCheatSize;
    u32 apPatchFileCluster;
    u32 apPatchOffset;
    u32 apPatchSize;
    u32 dsi2dsSavePatchFileCluster;
    u32 dsi2dsSavePatchOffset;
    u32 dsi2dsSavePatchSize;
    u32 cheatFileCluster;
    u32 cheatSize;
    u32 patchOffsetCacheFileCluster;
    u32 ramDumpCluster;
	u32 srParamsFileCluster;
	u32 screenshotCluster;
	u32 apFixOverlaysCluster;
	u32 musicCluster;
	u32 musicsSize;
	u32 pageFileCluster;
	u32 manualCluster;
	u32 sharedFontCluster;
    u32 patchMpuSize;
    u8 patchMpuRegion;
    u8 language;
    s8 region; // SDK 5
    u8 dsiMode; // SDK 5
    u8 valueBits2;
    u8 donorSdkVer;
    u8 consoleModel;
    u8 romRead_LED;
    u8 dmaRomRead_LED;
    u8 soundFreq;
    u8 valueBits3;
} loadCrt0;

#endif // LOAD_CRT0_H
