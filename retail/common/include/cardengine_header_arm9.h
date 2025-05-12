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
	u32* dsiSaveCheckExists;
	u32* dsiSaveGetResultCode;
	u32* dsiSaveCreate;
	u32* dsiSaveDelete;
	u32* dsiSaveGetInfo;
	u32* dsiSaveSetLength;
	u32* dsiSaveOpen;
	u32* dsiSaveOpenR;
	u32* dsiSaveClose;
	u32* dsiSaveGetLength;
	u32* dsiSaveGetPosition;
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
	u32* gsdd_fix;
	u32* vblankHandlerRef;
	u32* ipcSyncHandlerRef;
} cardengineArm9Patches;


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
} cardengineArm9ThumbPatches;


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
		3: pingIpc
		4: enableExceptionHandler
		5: isSdk5
		6: overlaysCached
		7: cacheFlushFlag
		8: cardReadFix
		9: cacheDisabled
		10: slowSoftReset
		11: dsiBios
		12: asyncCardRead
		13: useSharedWram
		14: cloneboot
		15: isDlp
		16: bypassExceptionHandler
		17: fntFatCached
		18: waitForPreloadToFinish
		19: resetOnFirstException
		20: resetOnEveryException
		21: useColorLut
	*/
	s32 mainScreen;
	u32 consoleModel;
	u32* irqTable;
	// Below not used for DSiWare ce9 binary
	u32 fntSrc;
	u32 fntFatSize;
	u32 overlaysSrc;
	u32 overlaysSrcAlign;
	u32 overlaysSize;
	u32 overlaysSizeAlign;
	u32 romPaddingSize;
	u32 romLocation;
	u32 cacheAddress;
	u16 cacheSlots;
	u16 cacheBlockSize;
	u32 romPartSrc[4];
	u32 romPartSize[4];
	u32 romMapLines;
	u32 romMap[8][3]; // 0: ROM part start, 1: ROM part start in RAM, 2: ROM part end in RAM
} cardengineArm9;

#else
//
// ARM9 cardengine patches
//
typedef struct cardengineArm9Patches {
	u32* card_read_arm9;
	u32* card_save_arm9;
	u32* card_irq_enable;
	u32* card_dma_arm9;
	u32* card_set_dma_arm9;
	u32* nand_read_arm9;
	u32* nand_write_arm9;
	u32* cardStructArm9;
	u32* cacheFlushRef;
	u32* cardEndReadDmaRef;
	u32* reset_arm9;
	u32* pdash_read;
	u32* gsdd_fix;
	u32* ipcSyncHandlerRef;
	u32* rumble_arm9[2];
	u32* ndmaCopy;
	u32* dsiSaveCheckExists;
	u32* dsiSaveGetResultCode;
	u32* dsiSaveCreate;
	u32* dsiSaveDelete;
	u32* dsiSaveGetInfo;
	u32* dsiSaveSetLength;
	u32* dsiSaveOpen;
	u32* dsiSaveOpenR;
	u32* dsiSaveClose;
	u32* dsiSaveGetLength;
	u32* dsiSaveGetPosition;
	u32* dsiSaveSeek;
	u32* dsiSaveRead;
	u32* dsiSaveWrite;
	u32* musicPlay;
	u32* musicStopEffect;
} cardengineArm9Patches;


//
// ARM9 cardengine thumb patches
//
typedef struct cardengineArm9ThumbPatches {
	u32* card_save_arm9;
	u32* cardStructArm9;
	u32* cacheFlushRef;
	u32* cardEndReadDmaRef;
	u32* reset_arm9;
} cardengineArm9ThumbPatches;


//
// ARM9 cardengine
//
typedef struct cardengineArm9 {
    u32 ce9;
	u32 dldiOffset;
    cardengineArm9Patches* patches;
    cardengineArm9ThumbPatches* thumbPatches;
    u32 intr_ipc_orig_return;
    u32 bootNdsCluster;
    u32 fileCluster;
    u32 saveCluster;
    u32 saveSize;
    u32 romFatTableCache;
    u32 savFatTableCache;
    u8 romFatTableCompressed;
    u8 savFatTableCompressed;
    u8 musicsFatTableCompressed;
	u8 cardSaveCmdPos;
    u32 patchOffsetCacheFileCluster;
    u32 musicFatTableCache;
    u32 ramDumpCluster;
    u32 srParamsCluster;
	u32 screenshotCluster;
	u32 apFixOverlaysCluster;
	u32 musicCluster;
	u32 musicsSize;
	u32 musicBuffer;
    u32 pageFileCluster;
    u32 manualCluster;
    u32 sharedFontCluster;
    u32 cardStruct0;
    u32 cardStruct1;
	u32 valueBits;
	/*
		0: expansionPakFound
		1: extendedMemory
		2: ROMinRAM
		3: dsDebugRam
		4: enableExceptionHandler
		5: isSdk5
		6: overlaysCached
		7: cacheFlushFlag
		8: cardReadFix
		9: bypassExceptionHandler
	*/
    s32 mainScreen;
	u32* irqTable;
	u16 s2FlashcardId;
	u16 padding;
	u32 overlaysSrc;
	u32 overlaysSize;
	u32 ioverlaysSize;
	u32 arm9iromOffset;
	u32 arm9ibinarySize;
	u32 romPaddingSize;
	u32 romLocation;
	u32 romPartSrc;
	u32 romPartSize;
	u32 rumbleFrames[2];
	u32 rumbleForce[2];
	u32 prepareScreenshot;
	u32 saveScreenshot;
	u32 prepareManual;
	u32 readManual;
	u32 restorePreManual;
	u32 saveMainScreenSetting;
} cardengineArm9;
#endif

#endif // CARDENGINE_HEADER_ARM9_H
