#ifndef CARDENGINE_HEADER_ARM9_H
#define CARDENGINE_HEADER_ARM9_H

#include <nds/ndstypes.h>
#include "module_params.h"

#ifndef B4DS
//
// ARM9 cardengine patches
//
typedef struct cardengineArm9Patches {
    u32* card_read_arm9;
    u32* card_irq_enable;
    u32* card_pull_out_arm9;
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* card_set_dma_arm9;
    u32* nand_read_arm9;
    u32* nand_write_arm9;
    u32* cardStructArm9;
    u32* card_pull; // Unused
    u32* slot2_exists_fix;
    u32* slot2_read;
    u32* cacheFlushRef;
    u32* cardEndReadDmaRef;
    u32* sleepRef;
    u32* swi02;
    u32* reset_arm9;
    u32 needFlushDCCache;
    u32* pdash_read;
    u32* vblankHandlerRef;
    u32* ipcSyncHandlerRef;
} __attribute__ ((__packed__)) cardengineArm9Patches;


//
// ARM9 cardengine thumb patches
//
typedef struct cardengineArm9ThumbPatches {
    u32* card_read_arm9;
    u32* card_irq_enable;
    u32* card_pull_out_arm9;
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* card_set_dma_arm9;
    u32* nand_read_arm9;
    u32* nand_write_arm9;
    u32* cardStructArm9;
    u32* card_pull; // Unused
    u32* slot2_read;
    u32* cacheFlushRef;
    u32* cardEndReadDmaRef;
    u32* sleepRef;
} __attribute__ ((__packed__)) cardengineArm9ThumbPatches;


//
// ARM9 cardengine
//
typedef struct cardengineArm9 {
    u32 ce9;
    cardengineArm9Patches* patches;
    cardengineArm9ThumbPatches* thumbPatches;
    u32 intr_vblank_orig_return;
    u32 intr_ipc_orig_return;
    u32 fileCluster;
    u32 saveCluster;
    u32 cardStruct0;
    u32 cacheStruct;
	u32 valueBits;
	/*
		0: saveOnFlashcard
		1: extendedMemory
		2: ROMinRAM
		3: dsiMode
		4: enableExceptionHandler
		5: isSdk5
		6: overlaysInRam
		7: cacheFlushFlag
		8: cardReadFix
		9: a7HaltPatched
	*/
    u32 overlaysSize;
	u32 consoleModel;
    u32* irqTable;
    u32 romLocation;
	// Below not used for ROMs in RAM
    u32 cacheAddress;
    u16 cacheSlots;
    u16 cacheBlockSize;
} __attribute__ ((__packed__)) cardengineArm9;
#else
//
// ARM9 cardengine patches
//
typedef struct cardengineArm9Patches {
    u32* card_read_arm9;
    u32* card_irq_enable;
    u32* card_pull_out_arm9; // Unused
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* nand_read_arm9;
    u32* nand_write_arm9;
    u32* cardStructArm9;
    u32* card_pull;
    u32* cacheFlushRef;
    u32* readCachedRef;
    u32* reset_arm9;
    u32 needFlushDCCache;
    u32* ipcSyncHandlerRef;
} __attribute__ ((__packed__)) cardengineArm9Patches;


//
// ARM9 cardengine thumb patches
//
typedef struct cardengineArm9ThumbPatches {
    u32* card_read_arm9;
    u32* card_irq_enable;
    u32* card_pull_out_arm9; // Unused
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* nand_read_arm9;
    u32* nand_write_arm9;
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
    u32 ce9;
    cardengineArm9Patches* patches;
    cardengineArm9ThumbPatches* thumbPatches;
    u32 intr_ipc_orig_return;
    u32 fileCluster;
    u32 saveCluster;
    u32 srParamsCluster;
    u32 pageFileCluster;
    u32 cardStruct0;
	u32 valueBits;
	/*
		0: expansionPakFound
		1: extendedMemory
		2: ROMinRAM
		3: dsDebugRam
		5: isSdk5
		6: overlaysInRam
	*/
    u32 overlaysSize;
    u32 maxClusterCacheSize;
    u32 fatTableAddr;
    u32* irqTable;
    u32 romLocation;
} __attribute__ ((__packed__)) cardengineArm9;
#endif

#endif // CARDENGINE_HEADER_ARM9_H
