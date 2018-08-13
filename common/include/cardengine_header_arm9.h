#ifndef CARDENGINE_HEADER_ARM9_H
#define CARDENGINE_HEADER_ARM9_H

#include <nds/ndstypes.h>
#include "module_params.h"

//
// ARM9 cardengine patches
//
typedef struct cardengineArm9Patches {
    u32* card_read_arm9;
    u32* card_pull_out_arm9; // Unused
    u32 offset2;
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* cardStructArm9;
    u32* card_pull;
    u32* cacheFlushRef;
    u32* readCachedRef;
    u32 offset9;
    u32 needFlushDCCache;
} __attribute__ ((__packed__)) cardengineArm9Patches;


//
// ARM9 cardengine thumb patches
//
typedef struct cardengineArm9ThumbPatches {
    u32* card_read_arm9;
    u32* card_pull_out_arm9; // Unused
    u32 offset2;
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* cardStructArm9;
    u32* card_pull;
    u32* cacheFlushRef;
    u32* readCachedRef;
    u32 offset9;
} __attribute__ ((__packed__)) cardengineArm9ThumbPatches;


//
// ARM9 cardengine
//
typedef struct cardengineArm9 {
    cardengineArm9Patches* patches;
    cardengineArm9ThumbPatches* thumbPatches;
    u32 intr_fifo_orig_return;
    const module_params_t* moduleParams;
    u32 fileCluster;
    u32 cardStruct0;
    u32 cacheStruct;
    u32 ROMinRAM;
    u32 dsiMode;
    u32 enableExceptionHandler;
    u32 consoleModel;
    u32 asyncPrefetch;
} __attribute__ ((__packed__)) cardengineArm9;

#endif // CARDENGINE_HEADER_ARM9_H
