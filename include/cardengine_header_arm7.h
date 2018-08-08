#ifndef CARDENGINE_HEADER_ARM7_H
#define CARDENGINE_HEADER_ARM7_H

#include <nds/ndstypes.h>

//
// ARM7 cardengine patches of ARM7 functions
//
typedef struct cardengineArm7PatchesArm7Functions {
    u32 eeprom_protect;
    u32 eeprom_page_erase;
    u32 eeprom_page_verify;
    u32 eeprom_page_write;
    u32 eeprom_page_prog;
    u32 eeprom_read;
    u32 card_read;
    u32 card_id;
    u32 save_cluster;
    u32 save_size;
} __attribute__ ((__packed__)) cardengineArm7PatchesArm7Functions;


//
// ARM7 cardengine patches of ARM7 thumb functions
//
typedef struct cardengineArm7PatchesArm7FunctionsThumb {
    u32 eeprom_protect;
    u32 eeprom_page_erase;
    u32 eeprom_page_verify;
    u32 eeprom_page_write;
    u32 eeprom_page_prog;
    u32 eeprom_read;
    u32 card_read;
    u32 card_id;
    u32 swi_halt;
} __attribute__ ((__packed__)) cardengineArm7PatchesArm7FunctionsThumb;

//
// ARM7 cardengine patches
//
typedef struct cardengineArm7Patches {
    u32 card_read_arm9;
    u32* card_pull_out_arm9;
    u32* card_irq_enable_arm7;
    u32 vblank_handler;
    u32 fifo_handler;
    u32 card_struct_arm9;
    u32 card_pull;
    u32 cache_flush_ref;
    u32 read_cached_ref;
    cardengineArm7PatchesArm7Functions* arm7_functions;
    u32* swi02;
    u32* j_thumb_new_swi_halt;
    u32* j_new_swi_halt;
    u32* j_twl_get_pitch_table;
    u32* get_pitch_table_stub;
    cardengineArm7PatchesArm7FunctionsThumb* arm7_functions_thumb;
} __attribute__ ((__packed__)) cardengineArm7Patches;

//
// ARM7 cardengine
//
typedef struct cardengineArm7 {
    cardengineArm7Patches* patches;
    u32 intr_vblank_orig_return;
    u32 intr_fifo_orig_return;
    u32 sdk_version;
    u32 file_cluster;
    u32 card_struct;
    u32 language;
    u32 gotten_scfg_ext;
    u32 dsi_mode;
    u32 rom_in_ram;
    u32 console_model;
    u32 romread_led;
    u32 game_soft_reset;
    u32* cheat_data;
    u32 rom_file;

} __attribute__ ((__packed__)) cardengineArm7;

#endif // CARDENGINE_HEADER_ARM7_H
