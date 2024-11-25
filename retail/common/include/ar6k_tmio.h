// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "calico_types.h"
//#include "../system/mailbox.h"
//#include "../system/sysclock.h"
#include "ar6k_tmio_regs.h"

MK_EXTERN_C_START

typedef struct TmioPort TmioPort;
typedef struct TmioResp TmioResp;
typedef struct TmioCtl  TmioCtl;
typedef struct TmioTx   TmioTx;

typedef void (* TmioInsRemHandler)(TmioCtl* ctl, unsigned port, bool inserted);
typedef void (* TmioCardIrqHandler)(TmioCtl* ctl, unsigned port);

struct TmioPort {
	u16 clock;
	u8  num;
	u8  width;
};

struct TmioResp {
	u32 value[4];
};


struct TmioCtl {
	uptr reg_base;
	uptr fifo_base;

	TmioPort cur_port;
	TmioTx* cur_tx;

	Mailbox mbox;

	u16 num_pending_blocks;
	bool cardirq_deferred;
	bool cardirq_ack_deferred;

	struct {
		TmioInsRemHandler insrem;
		TmioCardIrqHandler cardirq;
		void* user;
	} port_isr[1];
};

struct TmioTx {
	u32 status;

	TmioPort port;
	union {
		u32 arg;
		TmioResp resp;
	};

	u16 type;
	u16 block_size;
	u16 num_blocks;
	u16 _pad;

	void (* callback)(TmioCtl* ctl, TmioTx* tx);
	void (* xfer_isr)(TmioCtl* ctl, TmioTx* tx);
	void* user;
};

MK_CONSTEXPR u16 tmioSelectClock(unsigned freq)
{
	if (freq < (SYSTEM_CLOCK>>8)) // HCLK/256
		return 0x80; // HCLK/512
	if (freq < (SYSTEM_CLOCK>>7)) // HCLK/128
		return 0x40; // HCLK/256
	if (freq < (SYSTEM_CLOCK>>6)) // HCLK/64
		return 0x20; // HCLK/128
	if (freq < (SYSTEM_CLOCK>>5)) // HCLK/32
		return 0x10; // HCLK/64
	if (freq < (SYSTEM_CLOCK>>4)) // HCLK/16
		return 0x08; // HCLK/32
	if (freq < (SYSTEM_CLOCK>>3)) // HCLK/8
		return 0x04; // HCLK/16
	if (freq < (SYSTEM_CLOCK>>2)) // HCLK/4
		return 0x02; // HCLK/8
	if (freq < (SYSTEM_CLOCK>>1)) // HCLK/2
		return 0x01; // HCLK/4
	return 0x00; // HCLK/2
}

MK_INLINE void* tmioGetPortUserData(TmioCtl* ctl, unsigned port)
{
	return ctl->port_isr[port].user;
}

MK_INLINE void tmioSetPortUserData(TmioCtl* ctl, unsigned port, void* data)
{
	ctl->port_isr[port].user = data;
}

bool tmioInit(TmioCtl* ctl, uptr reg_base, uptr fifo_base, u32* mbox_slots, unsigned num_mbox_slots);
void tmioSetPortInsRemHandler(TmioCtl* ctl, unsigned port, TmioInsRemHandler isr);
void tmioSetPortCardIrqHandler(TmioCtl* ctl, unsigned port, TmioCardIrqHandler isr);
void tmioAckPortCardIrq(TmioCtl* ctl, unsigned port);

void tmioIrqHandler(TmioCtl* ctl);
int tmioThreadMain(TmioCtl* ctl);
void tmioThreadCancel(TmioCtl* ctl);

bool tmioTransact(TmioCtl* ctl, TmioTx* tx);

void tmioXferRecvByCpu(TmioCtl* ctl, TmioTx* tx);
void tmioXferSendByCpu(TmioCtl* ctl, TmioTx* tx);

unsigned tmioDecodeTranSpeed(u8 tran_speed);

MK_EXTERN_C_END
