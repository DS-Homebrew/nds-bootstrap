// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "calico_types.h"
#include "ar6k_sdio.h"
#include "ar6k.h"

//#define AR6K_DEBUG

#define dietPrint(...) ((void)0)
#endif

typedef struct Ar6kIrqProcRegs {
	u8 host_int_status;
	u8 cpu_int_status;
	u8 error_int_status;
	u8 counter_int_status;
	u8 mbox_frame;
	u8 rx_lookahead_valid;
	u8 _pad[2];
	u32 rx_lookahead[2];
} Ar6kIrqProcRegs;

// Internal API
bool _ar6kDevSetIrqEnable(Ar6kDev* dev, bool enable);
bool _ar6kDevPollMboxMsgRecv(Ar6kDev* dev, u32* lookahead, unsigned attempts);
bool _ar6kDevSendPacket(Ar6kDev* dev, const void* pktmem, size_t pktsize);
bool _ar6kDevRecvPacket(Ar6kDev* dev, void* pktmem, size_t pktsize);

bool _ar6kHtcRecvMessagePendingHandler(Ar6kDev* dev);
bool _ar6kHtcSendPacket(Ar6kDev* dev, Ar6kHtcEndpointId epid, NetBuf* pPacket);

void _ar6kWmiEventRx(Ar6kDev* dev, NetBuf* pPacket);
void _ar6kWmiDataRx(Ar6kDev* dev, Ar6kHtcSrvId srvid, NetBuf* pPacket);
