// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "calico_types.h"
#include "ar6k_tmio.h"

#define SDIO_CMD_GO_IDLE           (TMIO_CMD_INDEX(0)  | TMIO_CMD_RESP_NONE)
#define SDIO_CMD_GET_RELATIVE_ADDR (TMIO_CMD_INDEX(3)  | TMIO_CMD_RESP_48)
#define SDIO_CMD_SEND_OP_COND      (TMIO_CMD_INDEX(5)  | TMIO_CMD_RESP_48_NOCRC)
#define SDIO_CMD_SELECT_CARD       (TMIO_CMD_INDEX(7)  | TMIO_CMD_RESP_48_BUSY)
#define SDIO_CMD_RW_DIRECT         (TMIO_CMD_INDEX(52) | TMIO_CMD_RESP_48)
#define SDIO_CMD_RW_EXTENDED       (TMIO_CMD_INDEX(53) | TMIO_CMD_RESP_48 | TMIO_CMD_TX | TMIO_CMD_TX_SDIO)

#define SDIO_RW_DIRECT_DATA(_x) ((_x)&0xff)
#define SDIO_RW_DIRECT_ADDR(_x) (((_x)&0x1ffff)<<9)
#define SDIO_RW_DIRECT_WR_RD    (1U<<27)
#define SDIO_RW_DIRECT_FUNC(_x) (((_x)&7)<<28)
#define SDIO_RW_DIRECT_READ     (0U<<31)
#define SDIO_RW_DIRECT_WRITE    (1U<<31)

#define SDIO_RW_EXTENDED_COUNT(_x) ((_x)&0x1ff)
#define SDIO_RW_EXTENDED_ADDR(_x)  (((_x)&0x1ffff)<<9)
#define SDIO_RW_EXTENDED_FIXED     (0U<<26)
#define SDIO_RW_EXTENDED_INCR      (1U<<26)
#define SDIO_RW_EXTENDED_BYTES     (0U<<27)
#define SDIO_RW_EXTENDED_BLOCKS    (1U<<27)
#define SDIO_RW_EXTENDED_FUNC(_x)  (((_x)&7)<<28)
#define SDIO_RW_EXTENDED_READ      (0U<<31)
#define SDIO_RW_EXTENDED_WRITE     (1U<<31)

#define SDIO_BLOCK_SZ 128

MK_EXTERN_C_START

typedef struct SdioManfid {
	u16 code;
	u16 id;
} SdioManfid;

typedef struct SdioCard {
	TmioCtl* ctl;
	TmioPort port;

	void (* dma_cb)(TmioCtl* ctl, TmioTx* tx);

	u16 rca;
	SdioManfid manfid;
	u8 revision;
	u8 caps;
} SdioCard;

bool sdioCardInit(SdioCard* card, TmioCtl* ctl, unsigned port);
bool sdioCardSetIrqEnable(SdioCard* card, unsigned func, bool enable);
bool sdioCardReadDirect(SdioCard* card, unsigned func, unsigned addr, void* out, size_t size);
bool sdioCardWriteDirect(SdioCard* card, unsigned func, unsigned addr, const void* in, size_t size);
bool sdioCardReadExtended(SdioCard* card, unsigned func, unsigned addr, void* out, size_t size);
bool sdioCardWriteExtended(SdioCard* card, unsigned func, unsigned addr, const void* in, size_t size);

MK_EXTERN_C_END
