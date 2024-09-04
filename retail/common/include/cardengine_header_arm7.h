#ifndef CARDENGINE_HEADER_ARM7_H
#define CARDENGINE_HEADER_ARM7_H

#include <nds/ndstypes.h>
#include "module_params.h"

//
// ARM7 cardengine patches of ARM7 functions
//
typedef struct cardengineArm7PatchesArm7Functions {
    u32 eepromProtect;
    u32 eepromPageErase;
    u32 eepromPageVerify;
    u32 eepromPageWrite;
    u32 eepromPageProg;
    u32 eepromRead;
    u32 cardRead;
    u32 cardId;
    u32 saveCluster;
    u32 saveSize;
} cardengineArm7PatchesArm7Functions;


//
// ARM7 cardengine patches of ARM7 thumb functions
//
typedef struct cardengineArm7PatchesArm7FunctionsThumb {
    u32 eepromProtect;
    u32 eepromPageErase;
    u32 eepromPageVerify;
    u32 eepromPageWrite;
    u32 eepromPageProg;
    u32 eepromRead;
    u32 cardRead;
    u32 cardId;
} cardengineArm7PatchesArm7FunctionsThumb;

#ifndef B4DS
//
// ARM7 cardengine patches
//
typedef struct cardengineArm7Patches {
    u32* card_pull_out_arm9;
    u32* card_irq_enable_arm7;
    u32* thumb_card_irq_enable_arm7;
    u32* j_irqHandler;
    u32 vblankHandler;
    u32 fifoHandler;
    u32 card_pull;
    cardengineArm7PatchesArm7FunctionsThumb* arm7FunctionsDirect;
    cardengineArm7PatchesArm7Functions* arm7Functions;
    cardengineArm7PatchesArm7FunctionsThumb* arm7FunctionsThumb;
    u32* swi02;
    u32* swi24;
    u32* swi25;
    u32* swi26;
    u32* swi27;
    u32* j_newSwiHalt;
    u32* newSwiHaltThumb;
    u32* j_twlGetPitchTable;
    u32* j_twlGetPitchTableThumb;
} cardengineArm7Patches;

//
// ARM7 cardengine
//
typedef struct cardengineArm7 {
    u32 ce7;
    cardengineArm7Patches* patches;
    u32 intr_vblank_orig_return;
    u32 intr_fifo_orig_return;
	u32 cheatEngineAddr;
    const module_params_t* moduleParams;
    u32 fileCluster;
    u32 patchOffsetCacheFileCluster;
	u32 srParamsCluster;
	u32 ramDumpCluster;
	u32 screenshotCluster;
	u32 pageFileCluster;
	u32 manualCluster;
    u32 cardStruct;
	u32 valueBits;
	/*
		0: gameOnFlashcard
		1: saveOnFlashcard
		2: extendedMemory
		3: ROMinRAM
		4: dsiMode
		5: dsiSD
		6: preciseVolumeControl
		7: powerCodeOnVBlank
		8: runCardEngineCheck
		9: igmAccessible
		10: hiyaCfwFound
		11: slowSoftReset
		12: wideCheatUsed
		13: isSdk5
		14: asyncCardRead
		15: twlTouch
		16: cloneboot
		17: sleepMode
		18: dsiBios
		19: bootstrapOnFlashcard
		20: ndmaDisabled
		21: isDlp
		31: scfgLocked
	*/
    s32 mainScreen;
    u32* languageAddr;
    u8 language;
    u8 consoleModel;
    u8 romRead_LED;
    u8 dmaRomRead_LED;
    u32* irqTable_offset;
    u16 scfgRomBak;
    u16 igmHotkey;
	u32 romLocation;
	u32 romMapLines;
	u32 romMap[5][3]; // 0: ROM part start, 1: ROM part start in RAM, 2: ROM part end in RAM
} cardengineArm7;

//
// ARM7 cardengine
//
typedef struct cardengineArm7B4DS {
	u32 ce7;
	cardengineArm7Patches* patches;
	u32 intr_vblank_orig_return;
    u32 intr_fifo_orig_return;
	u32 cheatEngineAddr;
	u32 musicBuffer;
	const module_params_t* moduleParams;
	u32 cardStruct;
	u32 valueBits;
	/*
		17: sleepMode
	*/
	s32 mainScreen;
	u32 language; //u8
	u32* languageAddr;
	u16 igmHotkey;
} cardengineArm7B4DS;
#else
//
// ARM7 cardengine patches
//
typedef struct cardengineArm7Patches {
    u32* card_pull_out_arm9;
    u32* card_irq_enable_arm7;
    u32* thumb_card_irq_enable_arm7;
    u32 vblankHandler;
    u32 fifoHandler;
    u32* j_twlGetPitchTable;
    cardengineArm7PatchesArm7FunctionsThumb* arm7FunctionsDirect;
    cardengineArm7PatchesArm7Functions* arm7Functions;
    cardengineArm7PatchesArm7FunctionsThumb* arm7FunctionsThumb;
} cardengineArm7Patches;

//
// ARM7 cardengine
//
typedef struct cardengineArm7 {
	u32 ce7;
	cardengineArm7Patches* patches;
	u32 intr_vblank_orig_return;
    u32 intr_fifo_orig_return;
	u32 cheatEngineAddr;
	u32 musicBuffer;
	const module_params_t* moduleParams;
	u32 cardStruct;
	u32 valueBits;
	/*
		17: sleepMode
	*/
	s32 mainScreen;
	u32 language; //u8
	u32* languageAddr;
	u16 igmHotkey;
	u8 RumblePakType;
} cardengineArm7;
#endif

#endif // CARDENGINE_HEADER_ARM7_H
