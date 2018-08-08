#ifndef LOAD_CRT0_H
#define LOAD_CRT0_H

#include <nds/ndstypes.h>
//#include "cardengine_header_arm7.h"
//#include "cardengine_header_arm9.h"

typedef struct loadCrt0 {
    u32 start;
    u32 stored_file_cluster;
    u32 init_disc;
    u32 want_to_patch_dldi;
    u32 arg_start;
    u32 arg_size;
    u32 dldi;
    u32 have_dsisd;
    u32 sav;
    u32 sav_size;
    u32 language;
    u32 dsimode; // SDK 5
    u32 donor_sdk_ver;
    u32 patch_mpu_region;
    u32 patch_mpu_size;
    u32 console_model;
    u32 loading_screen;
    u32 romread_led;
    u32 game_soft_reset;
    u32 async_prefetch;
    u32 cardengine_arm7_offset; //cardengineArm7* cardengine_arm7;
    u32 cardengine_arm9_offset; //cardengineArm9* cardengine_arm9;
    u32 logging;
} __attribute__ ((__packed__)) loadCrt0;

#endif // LOAD_CRT0_H
