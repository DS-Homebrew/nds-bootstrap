#ifndef CARDENGINE_HEADER_ARM9_H
#define CARDENGINE_HEADER_ARM9_H

//
// ARM9 cardengine
//
#define CE9_PATCHES_OFFSET                  0
#define CE9_THUMB_PATCHES_OFFSET            1
#define CE9_INTR_FIFO_ORIG_RETURN_OFFSET    2
#define CE9_SDK_VERSION_OFFSET              3
#define CE9_FILE_CLUSTER_OFFSET             4
#define CE9_CARD_STRUCT0_OFFSET             5
#define CE9_CACHE_STRUCT_OFFSET             6
#define CE9_ROM_IN_RAM_OFFSET               7
#define CE9_ROM_TID_OFFSET                  8
#define CE9_ROM_HEADERCRC_OFFSET            9
#define CE9_ARM9_LEN_OFFSET                 10
#define CE9_ROM_SIZE_OFFSET                 11
#define CE9_DSI_MODE_OFFSET                 12
#define CE9_ENABLE_EXCEPTION_HANDLER_OFFSET 13
#define CE9_CONSOLE_MODEL_OFFSET            14
#define CE9_ASYNC_PREFETCH_OFFSET           15


//
// ARM9 cardengine patches
//
#define CE9_P_CARD_READ_ARM9_OFFSET      0
#define CE9_P_CARD_PULL_OUT_ARM9_OFFSET  1
//                                       2
#define CE9_P_CARD_ID_ARM9_OFFSET        3
#define CE9_P_CARD_DMA_ARM9_OFFSET       4
#define CE9_P_CARD_STRUCT_ARM9_OFFSET    5
#define CE9_P_CARD_PULL_OFFSET           6
#define CE9_P_CACHE_FLUSH_REF_OFFSET     7
#define CE9_P_READ_CACHED_REF_OFFSET     8
//                                       9
#define CE9_P_NEED_FLUSH_DC_CACHE_OFFSET 10


//
// ARM9 cardengine thumb patches
//
#define CE9_TP_CARD_READ_ARM9_OFFSET     0 
#define CE9_TP_CARD_PULL_OUT_ARM9_OFFSET 1
//                                       2
#define CE9_TP_CARD_ID_ARM9_OFFSET       3
#define CE9_TP_CARD_DMA_ARM9_OFFSET      4
#define CE9_TP_CARD_STRUCT_ARM9_OFFSET   5
#define CE9_TP_CARD_PULL_OFFSET          6
#define CE9_TP_CACHE_FLUSH_REF_OFFSET    7
#define CE9_TP_READ_CACHED_REF_OFFSET    8
//                                       9

#endif // CARDENGINE_HEADER_ARM9_H
