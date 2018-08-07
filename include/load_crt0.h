#ifndef LOAD_CRT0_H
#define LOAD_CRT0_H

#define LOAD_CRT0_LOCATION 0x06840000 // LCDC_BANK_C

#define LC0_START_OFFSET               0
#define LC0_STORED_FILE_CLUSTER_OFFSET 1
#define LC0_INIT_DISC_OFFSET           2
#define LC0_WANT_TO_PATCH_DLDI_OFFSET  3
#define LC0_ARG_START_OFFSET           4
#define LC0_ARG_SIZE_OFFSET            5
#define LC0_DLDI_OFFSET                6
#define LC0_HAVE_DSISD_OFFSET          7
#define LC0_SAV_OFFSET                 8
#define LC0_SAV_SIZE_OFFSET            9
#define LC0_LANGUAGE_OFFSET            10
#define LC0_DSIMODE_OFFSET             11 // SDK 5
#define LC0_DONOR_SDK_VER_OFFSET       12
#define LC0_PATCH_MPU_REGION_OFFSET    13
#define LC0_PATCH_MPU_SIZE_OFFSET      14
#define LC0_CONSOLE_MODEL_OFFSET       15
#define LC0_LOADING_SCREEN_OFFSET      16
#define LC0_ROMREAD_LED_OFFSET         17
#define LC0_GAME_SOFT_RESET_OFFSET     18
#define LC0_ASYNC_PREFETCH_OFFSET      19
#define LC0_CARDENGINE_ARM7_OFFSET     20
#define LC0_CARDENGINE_ARM9_OFFSET     21
#define LC0_LOGGING_OFFSET             22

#endif // LOAD_CRT0_H
