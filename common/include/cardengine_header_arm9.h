#ifndef CARDENGINE_HEADER_ARM9_H
#define CARDENGINE_HEADER_ARM9_H

#include <nds/ndstypes.h>

//
// ARM9 cardengine thumb patches
//
typedef struct cardengineArm9ThumbPatches {
    u32* card_read_arm9;
    u32* card_pull_out_arm9; // Unused
    u32 offset2;
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* card_struct_arm9;
    u32* card_pull;
    u32* cache_flush_ref;
    u32* read_cached_ref;
    u32 offset9;
} __attribute__ ((__packed__)) cardengineArm9ThumbPatches;


//
// ARM9 cardengine patches
//
typedef struct cardengineArm9Patches {
    u32* card_read_arm9;
    u32* card_pull_out_arm9; // Unused
    u32 offset2;
    u32* card_id_arm9;
    u32* card_dma_arm9;
    u32* card_struct_arm9;
    u32* card_pull;
    u32* cache_flush_ref;
    u32* read_cached_ref;
    u32 offset9;
    u32 need_flush_dc_cache;
} __attribute__ ((__packed__)) cardengineArm9Patches;


//
// ARM9 cardengine
//
typedef struct cardengineArm9 {
    cardengineArm9Patches* patches;
    cardengineArm9ThumbPatches* thumb_patches;
    u32 intr_fifo_orig_return;
    u32 sdk_version;
    u32 file_cluster;
    u32 card_struct0;
    u32 cache_struct;
    u32 rom_in_ram;
    u32 rom_headercrc;
    u32 arm9_len;
    u32 rom_size;
    u32 dsi_mode;
    u32 enable_exception_handler;
    u32 console_model;
    u32 async_prefetch;
} __attribute__ ((__packed__)) cardengineArm9;

#endif // CARDENGINE_HEADER_ARM9_H
