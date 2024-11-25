// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "calico_types.h"
//#include "../../system/mailbox.h"
#include "ar6k_sdio.h"
#include "ar6k_base.h"
#include "ar6k_htc.h"
#include "ar6k_wmi.h"

#define AR6K_WORK_BUF_SIZE (sizeof(NetBuf) + AR6K_HTC_MAX_PACKET_SZ)

MK_EXTERN_C_START

typedef struct Ar6kEndpoint {
	u16 service_id;
	u16 max_msg_size;
} Ar6kEndpoint;

typedef struct Ar6kIrqEnableRegs {
	u8 int_status_enable;
	u8 cpu_int_status_enable;
	u8 error_status_enable;
	u8 counter_int_status_enable;
} Ar6kIrqEnableRegs;

struct Ar6kDev {
	SdioCard* sdio;
	u32 chip_id;
	u32 hia_addr;

	// Interrupt handling
	Mailbox irq_mbox;
	u32 irq_flag;
	Ar6kIrqEnableRegs irq_regs;

	// HTC
	void* workbuf;
	u32 lookahead;
	u16 credit_size;
	u16 credit_avail;
	u16 max_msg_credits;
	Ar6kEndpoint endpoints[Ar6kHtcEndpointId_Count-1];

	// WMI
	bool wmi_ready;
	u8 macaddr[6];
	Ar6kHtcEndpointId wmi_ctrl_epid;
	Ar6kHtcEndpointId wmi_data_epids[4];
	u32 wmi_channel_mask;

	// Callbacks
	void (* cb_onBssInfo)(Ar6kDev* dev, Ar6kWmiBssInfoHdr* bssInfo, NetBuf* pPacket);
	void (* cb_onScanComplete)(Ar6kDev* dev, int status);
	void (* cb_onAssoc)(Ar6kDev* dev, Ar6kWmiEvtConnected* info);
	void (* cb_onDisassoc)(Ar6kDev* dev, Ar6kWmiEvtDisconnected* info);
	void (* cb_rx)(Ar6kDev* dev, NetBuf* pPacket);
};

bool ar6kDevInit(Ar6kDev* dev, SdioCard* sdio, void* workbuf);
int ar6kDevThreadMain(Ar6kDev* dev);
void ar6kDevThreadCancel(Ar6kDev* dev);

bool ar6kDevReadRegDiag(Ar6kDev* dev, u32 addr, u32* out);
bool ar6kDevWriteRegDiag(Ar6kDev* dev, u32 addr, u32 value);

MK_EXTERN_C_END
