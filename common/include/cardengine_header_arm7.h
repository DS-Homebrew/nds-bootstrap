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
} __attribute__ ((__packed__)) cardengineArm7PatchesArm7Functions;


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
} __attribute__ ((__packed__)) cardengineArm7PatchesArm7FunctionsThumb;

//
// ARM7 cardengine patches
//
typedef struct cardengineArm7Patches {
    u32* card_pull_out_arm9;
    u32* card_irq_enable_arm7;
    u32 vblankHandler;
    u32 timer0Handler;
    u32 timer1Handler;
    u32 timer2Handler;
    u32 timer3Handler;
    u32 fifoHandler;
    u32 card_pull;
    cardengineArm7PatchesArm7Functions* arm7Functions;
    u32* swi02;
    u32* j_twlGetPitchTable;
    u32* getPitchTableStub;
    cardengineArm7PatchesArm7FunctionsThumb* arm7FunctionsThumb;
} __attribute__ ((__packed__)) cardengineArm7Patches;

//
// ARM7 cardengine
//
typedef struct cardengineArm7 {
    cardengineArm7Patches* patches;
    u32 intr_vblank_orig_return;
    u32 intr_timer0_orig_return;
    u32 intr_timer1_orig_return;
    u32 intr_timer2_orig_return;
    u32 intr_timer3_orig_return;
    u32 intr_fifo_orig_return;
    const module_params_t* moduleParams;
    u32 fileCluster;
    u32 cardStruct;
    u32 language; //u8
    u32 gottenSCFGExt;
    u32 dsiMode;
    u32 ROMinRAM;
    u32 consoleModel;
    u32 romread_LED;
    u32 gameSoftReset;
    u32 cheat_data_offset; //u32* cheat_data;
    u32 cheat_data_len;

} __attribute__ ((__packed__)) cardengineArm7;

inline u32* getCheatData(const cardengineArm7* ce7) {
    return (u32*)((u32)ce7 + ce7->cheat_data_offset);
}

#endif // CARDENGINE_HEADER_ARM7_H
