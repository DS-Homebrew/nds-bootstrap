#ifndef LOAD_CRT0_H
#define LOAD_CRT0_H

#include <nds/ndstypes.h>
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"

typedef struct loadCrt0 {
    u32 _start;
    u32 storedFileCluster;
    u32 initDisc;
    u32 wantToPatchDLDI;
    u32 argStart;
    u32 argSize;
    u32 dldiOffset;
    u32 dsiSD;
    u32 saveFileCluster;
    u32 saveSize;
    u32 language; //u8
    u32 dsiMode; // SDK 5
    u32 donorSdkVer;
    u32 patchMpuRegion;
    u32 patchMpuSize;
    u32 ceCached; // SDK 1-4
    u32 consoleModel;
    u32 loadingScreen;
    u32 loadingDarkTheme;
    u32 loadingSwapLcds;
	u32 loadingFrames;
	u32 loadingFps;
    u32 loadingBar;
    u32 loadingBarYpos;
    u32 romread_LED;
    u32 boostVram;
    u32 gameSoftReset;
    u32 forceSleepPatch;
    u32 preciseVolumeControl;
    u32 logging;
    u32 cardengine_arm7_offset; //cardengineArm7* cardengine_arm7;
    u32 cardengine_arm9_offset; //cardengineArm9* cardengine_arm9;
} __attribute__ ((__packed__)) loadCrt0;

inline cardengineArm7* getCardengineArm7(const loadCrt0* lc0) {
    return (cardengineArm7*)((u32)lc0 + lc0->cardengine_arm7_offset);
}

#endif // LOAD_CRT0_H
