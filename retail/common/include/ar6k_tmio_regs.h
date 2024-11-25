// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "calico_types.h"

// ~ /!\ HARDWARE BUG WARNING /!\ ~
// - 32-bit accesses to 32-bit (or larger) TMIO registers only works reliably
//   for _reads_. On the other hand, _writes_ *MUST* be done as two separate
//   16-bit accesses, otherwise corruption *CAN* and *HAS* been observed.
//   This can be *CATASTROPHIC*, considering CMDARG (!), STAT/MASK are 32-bit.

// ~ /!\ HARDWARE BUG WARNING /!\ ~
// - The 32-bit FIFO does not work properly with small block sizes, and causes
//   transactions to *HANG* (all data tx'd but no DATAEND received). Presently
//   the smallest size that works reliably is not known. Exercise caution
//   (probably >=0x80 works well, considering it's used by Atheros SDIO DMA).

// Register offsets
#define TMIO_CMD        0x000
#define TMIO_PORTSEL    0x002
#define TMIO_CMDARG     0x004 // 32-bit (2x 16-bit)
#define TMIO_STOP       0x008
#define TMIO_BLKCNT     0x00a
#define TMIO_CMDRESP    0x00c // 128-bit (8x 16-bit)
#define TMIO_STAT       0x01c // 32-bit (2x 16-bit)
#define TMIO_MASK       0x020 // 32-bit (2x 16-bit)
#define TMIO_CLKCTL     0x024
#define TMIO_BLKLEN     0x026
#define TMIO_OPTION     0x028
#define TMIO_ERROR      0x02c // 32-bit (2x 16-bit)
#define TMIO_FIFO16     0x030
#define TMIO_SDIOCTL    0x034
#define TMIO_SDIO_STAT  0x036
#define TMIO_SDIO_MASK  0x038
#define TMIO_FIFOCTL    0x0d8
#define TMIO_RESET      0x0e0
#define TMIO_VERSION    0x0e2
#define TMIO_POWER      0x0f2 // ????
#define TMIO_EXT_SDIO   0x0f4
#define TMIO_EXT_WRPROT 0x0f6
#define TMIO_EXT_STAT   0x0f8 // 32-bit (2x 16-bit)
#define TMIO_EXT_MASK   0x0fc // 32-bit (2x 16-bit)
#define TMIO_CNT32      0x100
#define TMIO_BLKLEN32   0x104
#define TMIO_BLKCNT32   0x108

// TMIO_CMD bits
#define TMIO_CMD_INDEX(_x)     ((_x)&0x3f)
#define TMIO_CMD_TYPE_CMD      (0U<<6)
#define TMIO_CMD_TYPE_ACMD     (1U<<6)
#define TMIO_CMD_RESP_AUTO     (0U<<8)
#define TMIO_CMD_RESP_NONE     (3U<<8)
#define TMIO_CMD_RESP_48       (4U<<8)
#define TMIO_CMD_RESP_48_BUSY  (5U<<8)
#define TMIO_CMD_RESP_136      (6U<<8)
#define TMIO_CMD_RESP_48_NOCRC (7U<<8)
#define TMIO_CMD_RESP_MASK     (7U<<8)
#define TMIO_CMD_NO_TX         (0U<<11)
#define TMIO_CMD_TX            (1U<<11)
#define TMIO_CMD_TX_WRITE      (0U<<12)
#define TMIO_CMD_TX_READ       (1U<<12)
#define TMIO_CMD_TX_SINGLE     (0U<<13)
#define TMIO_CMD_TX_MULTI      (1U<<13)
#define TMIO_CMD_TX_SDIO       (1U<<14)

// TMIO_PORTSEL bits
#define TMIO_PORTSEL_NUMPORTS(_x) (((_x)>>8)&7)
#define TMIO_PORTSEL_PORT(_x)     ((_x)&3)

// TMIO_STOP bits
#define TMIO_STOP_DO_STOP   (1U<<0)
#define TMIO_STOP_AUTO_STOP (1U<<8)

// Interrupt TMIO_STAT bits
#define TMIO_STAT_CMD_RESPEND     (1U<<0)
#define TMIO_STAT_CMD_DATAEND     (1U<<2)
#define TMIO_STAT_PORT0_REMOVE    (1U<<3)
#define TMIO_STAT_PORT0_INSERT    (1U<<4)
#define TMIO_STAT_PORT0_D3_REMOVE (1U<<8)
#define TMIO_STAT_PORT0_D3_INSERT (1U<<9)
#define TMIO_STAT_BAD_CMD_INDEX   (1U<<16)
#define TMIO_STAT_BAD_CRC         (1U<<17)
#define TMIO_STAT_BAD_STOP_BIT    (1U<<18)
#define TMIO_STAT_DATA_TIMEOUT    (1U<<19)
#define TMIO_STAT_RX_OVERFLOW     (1U<<20)
#define TMIO_STAT_TX_UNDERRUN     (1U<<21)
#define TMIO_STAT_CMD_TIMEOUT     (1U<<22)
#define TMIO_STAT_FIFO16_RECV     (1U<<24)
#define TMIO_STAT_FIFO16_SEND     (1U<<25)
#define TMIO_STAT_UNK27           (1U<<27)
#define TMIO_STAT_ILL_ACCESS      (1U<<31)

// Non-interrupt TMIO_STAT bits
#define TMIO_STAT_PORT0_DETECT    (1U<<5)
#define TMIO_STAT_PORT0_NOWRPROT  (1U<<7)
#define TMIO_STAT_PORT0_D3_DETECT (1U<<10)
#define TMIO_STAT_SD_DATA0_PIN    (1U<<23)
#define TMIO_STAT_UNK29           (1U<<29)
#define TMIO_STAT_CMD_BUSY        (1U<<30)

// TMIO_CLKCTL
#define TMIO_CLKCTL_DIV(_x) ((_x)&0xff)
#define TMIO_CLKCTL_ENABLE  (1U<<8)
#define TMIO_CLKCTL_AUTO    (1U<<9)
#define TMIO_CLKCTL_UNK10   (1U<<10)

// TMIO_OPTION bits
#define TMIO_OPTION_DETECT(_x)  ((_x)&0xf)
#define TMIO_OPTION_TIMEOUT(_x) (((_x)&0xf)<<4)
#define TMIO_OPTION_NO_C2       (1U<<14)
#define TMIO_OPTION_BUS_WIDTH1  (1U<<15)
#define TMIO_OPTION_BUS_WIDTH4  (0U<<15)

// TMIO_FIFOCTL bits
#define TMIO_FIFOCTL_MODE16 (0U<<1)
#define TMIO_FIFOCTL_MODE32 (1U<<1)
#define TMIO_FIFOCTL_UNK5   (1U<<5)

// TMIO_EXT_SDIO bits
#define TMIO_EXT_SDIO_IRQ_ENABLE (1U<<0)

// TMIO_CNT32 bits
#define TMIO_CNT32_ENABLE        (1U<<1)
#define TMIO_CNT32_STAT_RECV     (1U<<8)
#define TMIO_CNT32_STAT_NOT_SEND (1U<<9) // Yes, this is the opposite
#define TMIO_CNT32_FIFO_CLEAR    (1U<<10)
#define TMIO_CNT32_IE_RECV       (1U<<11)
#define TMIO_CNT32_IE_SEND       (1U<<12)
