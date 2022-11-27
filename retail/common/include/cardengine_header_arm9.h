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
	u32* dsiSaveGetResultCode;
	u32* dsiSaveCreate;
	u32* dsiSaveDelete;
	u32* dsiSaveGetInfo;
	u32* dsiSaveSetLength;
	u32* dsiSaveOpen;
	u32* dsiSaveOpenR;
	u32* dsiSaveClose;
	u32* dsiSaveGetLength;
	u32* dsiSaveSeek;
	u32* dsiSaveRead;
	u32* dsiSaveWrite;
	u32* cardStructArm9;
	u32* waitSysCycles;
	u32* cart_read;
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
    u32* cart_read;
    u32* cacheFlushRef;
    u32* cardEndReadDmaRef;
    u32* sleepRef;
    u32* reset_arm9;
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
    u32 saveSize;
    u32 cardStruct0;
    u32 cacheStruct;
	u32 valueBits;
	/*
		0: saveOnFlashcard
		1: ROMinRAM
		2: eSdk2
		3: dsiMode
		4: enableExceptionHandler
		5: isSdk5
		6: overlaysCached
		7: cacheFlushFlag
		8: cardReadFix
		9: cacheDisabled
		10: slowSoftReset
		11: dsiBios
		12: asyncCardRead
		13: softResetMb
		14: cloneboot
	*/
    u32 overlaysSize;
	u32 consoleModel;
    u32* irqTable;
	// Below not used for ROM in RAM ce9 binary
    u32 romLocation;
    u32 cacheAddress;
    u16 cacheSlots;
    u16 cacheBlockSize;
    u32 romPartSrc;
    u32 romPartSize;
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
	u32* dsiSaveGetResultCode;
	u32* dsiSaveCreate;
	u32* dsiSaveDelete;
	u32* dsiSaveGetInfo;
	u32* dsiSaveSetLength;
	u32* dsiSaveOpen;
	u32* dsiSaveOpenR;
	u32* dsiSaveClose;
	u32* dsiSaveGetLength;
	u32* dsiSaveSeek;
	u32* dsiSaveRead;
	u32* dsiSaveWrite;
	u32* musicPlay;
	u32* musicStopEffect;
	u32* cardStructArm9;
	u32* card_pull;
	u32* cacheFlushRef;
	u32* readCachedRef;
	u32* reset_arm9;
	u32* rumble_arm9[2];
	u32 needFlushDCCache;
	u32* pdash_read;
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
    u32* reset_arm9;
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
    u32 saveSize;
    u32 romFatTableCache;
    u32 savFatTableCache;
    u16 romFatTableCompressed;
    u16 savFatTableCompressed;
    u32 musicFatTableCache;
    u32 ramDumpCluster;
    u32 srParamsCluster;
	u32 screenshotCluster;
	u32 musicCluster;
	u32 musicsSize;
    u32 pageFileCluster;
    u32 manualCluster;
    u32 sharedFontCluster;
    u32 cardStruct0;
	u32 valueBits;
	/*
		0: expansionPakFound
		1: extendedMemory
		2: ROMinRAM
		3: dsDebugRam
		4: enableExceptionHandler
		5: isSdk5
		6: overlaysCached
	*/
    u16 s2FlashcardId;
    u16 padding;
    u32 overlaysSize;
    u32 ioverlaysSize;
    u32* irqTable;
    u32 romLocation;
	u32 rumbleFrames[2];
	u32 rumbleForce[2];
	VoidFn prepareScreenshot;
	VoidFn saveScreenshot;
	void (* readManual)(int);
} __attribute__ ((__packed__)) cardengineArm9;
#endif

#endif // CARDENGINE_HEADER_ARM9_H
