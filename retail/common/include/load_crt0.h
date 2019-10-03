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
    u32 dldiOffset;
    u32 dsiSD;
    u32 saveFileCluster;
    u32 saveSize;
    u32 apPatchFileCluster;
    u32 apPatchSize;
    u32 language; //u8
    u32 dsiMode; // SDK 5
    u32 donorSdkVer;
    u32 patchMpuRegion;
    u32 patchMpuSize;
    u32 consoleModel;
    u32 romread_LED;
    u32 boostVram;
    u32 gameSoftReset;
    u32 forceSleepPatch;
    u32 dsiModeConsole;
    u32 logging;
} __attribute__ ((__packed__)) loadCrt0;

#endif // LOAD_CRT0_H
