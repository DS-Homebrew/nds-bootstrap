// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "ar6k_base.h"

// *** HTC - probably standing for Host-Target Communication ***
// This is the main encapsulation format used by Atheros firmware.
// It is basically a network-esque protocol, with several "endpoints"
// (serving as connected services), and with its own header/PDU format.

// Structure of the PDU:
// [Ar6kFrameHdr] (6 bytes)
// [Payload] (hdr.payload_len)
// If hdr.flags & FLAG_RECV_TRAILER:
//   [Trailer] (hdr.recv_trailer_len), containing one or more records:
//     [Ar6kHtcRecordHdr] + {data} with size rechdr.length

#define AR6K_HTC_PROTOCOL_VER 2
#define AR6K_HTC_MAX_PACKET_SZ 2048

#define AR6K_HTC_FLAG_NEED_CREDIT_UPDATE (1U<<0)
#define AR6K_HTC_FLAG_RECV_TRAILER       (1U<<1)

MK_EXTERN_C_START

typedef enum Ar6kHtcEndpointId {
	Ar6kHtcEndpointId_Control = 0, // Always used for control messages
	Ar6kHtcEndpointId_First   = 1,
	Ar6kHtcEndpointId_Last    = 8,

	Ar6kHtcEndpointId_Count,
} Ar6kHtcEndpointId;

typedef enum Ar6kHtcRecordId {
	Ar6kHtcRecordId_Null      = 0,
	Ar6kHtcRecordId_Credit    = 1,
	Ar6kHtcRecordId_Lookahead = 2,
} Ar6kHtcRecordId;

typedef struct Ar6kHtcFrameHdr {
	// Lookahead region
	u8 endpoint_id;
	u8 flags;
	u16 payload_len; // not including this header

	// Control bytes
	u8 recv_trailer_len;
	u8 unused;
} Ar6kHtcFrameHdr;

typedef struct Ar6kHtcRecordHdr {
	u8 record_id; // see Ar6kHtcRecordId
	u8 length;
} Ar6kHtcRecordHdr;

typedef struct Ar6kHtcCreditReport {
	u8 endpoint_id;
	u8 credits;
} Ar6kHtcCreditReport;

typedef struct Ar6kHtcLookaheadReport {
	u8 pre_valid;
	u8 lookahead[4];
	u8 post_valid;
} Ar6kHtcLookaheadReport;

// *** HTC control messages ***

typedef enum Ar6kHtcCtrlMsgId {
	Ar6kHtcCtrlMsgId_Ready               = 1,
	Ar6kHtcCtrlMsgId_ConnectService      = 2,
	Ar6kHtcCtrlMsgId_ConnectServiceReply = 3,
	Ar6kHtcCtrlMsgId_SetupComplete       = 4,
} Ar6kHtcCtrlMsgId;

typedef enum Ar6kHtcSrvId {
	Ar6kHtcSrvId_HtcControl = 0x0001,
	Ar6kHtcSrvId_WmiControl = 0x0100,
	Ar6kHtcSrvId_WmiDataBe  = 0x0101,
	Ar6kHtcSrvId_WmiDataBk  = 0x0102,
	Ar6kHtcSrvId_WmiDataVi  = 0x0103,
	Ar6kHtcSrvId_WmiDataVo  = 0x0104,
} Ar6kHtcSrvId;

typedef enum Ar6kHtcSrvStatus {
	Ar6kHtcSrvStatus_Success     = 0,
	Ar6kHtcSrvStatus_NotFound    = 1,
	Ar6kHtcSrvStatus_Failed      = 2,
	Ar6kHtcSrvStatus_NoResources = 3,
	Ar6kHtcSrvStatus_NoMoreEp    = 4,
} Ar6kHtcSrvStatus;

typedef struct Ar6kHtcCtrlMsgHdr {
	u16 id;
} Ar6kHtcCtrlMsgHdr;

typedef struct Ar6kHtcCtrlCmdReady {
	Ar6kHtcCtrlMsgHdr hdr;
	u16 credit_count;
	u16 credit_size;
	u8 max_endpoints;
	u8 _pad;
} Ar6kHtcCtrlMsgReady;

#define AR6K_HTC_CONN_FLAG_REDUCE_CREDIT_DRIBBLE (1U<<2)
#define AR6K_HTC_CONN_FLAG_THRESHOLD_0_25        (0U<<0)
#define AR6K_HTC_CONN_FLAG_THRESHOLD_0_5         (1U<<0)
#define AR6K_HTC_CONN_FLAG_THRESHOLD_0_75        (2U<<0)
#define AR6K_HTC_CONN_FLAG_THRESHOLD_1           (3U<<0)

typedef struct Ar6kHtcCtrlCmdConnSrv {
	Ar6kHtcCtrlMsgHdr hdr;
	u16 service_id;
	u16 flags;
	u8 extra_len;
	u8 _pad;
	// service-specific extra data follows (extra_len)
} Ar6kHtcCtrlCmdConnSrv;

typedef struct Ar6kHtcCtrlCmdConnSrvReply {
	Ar6kHtcCtrlMsgHdr hdr;
	u16 service_id;
	u8 status;
	u8 endpoint_id;
	u16 max_msg_size;
	u8 extra_len;
	u8 _pad;
	// service-specific extra data follows (extra_len)
} Ar6kHtcCtrlCmdConnSrvReply;

bool ar6kHtcInit(Ar6kDev* dev);
Ar6kHtcSrvStatus ar6kHtcConnectService(Ar6kDev* dev, Ar6kHtcSrvId service_id, u16 flags, Ar6kHtcEndpointId* out_ep);
bool ar6kHtcSetupComplete(Ar6kDev* dev);

MK_EXTERN_C_END
