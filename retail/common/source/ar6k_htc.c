// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#include "ar6k_common.h"
#include <string.h>

static ThrListNode s_ar6kCreditWaitList;

typedef struct _Ar6kHtcCtrlPktMem {
	alignas(4) u8 mem[SDIO_BLOCK_SZ];
} _Ar6kHtcCtrlPktMem;

MK_CONSTEXPR Ar6kHtcEndpointId _ar6kHtcLookaheadGetEndpointId(u32 lookahead)
{
	return (Ar6kHtcEndpointId)(lookahead & 0xff);
}

MK_CONSTEXPR u16 _ar6kHtcLookaheadGetPayloadLen(u32 lookahead)
{
	return lookahead >> 16;
}

MK_CONSTEXPR unsigned _ar6kHtcBytesToCredits(Ar6kDev* dev, unsigned pkt_size)
{
	// Calculate number of needed credits for sending a packet of the given size.
	// This is actually just (pkt_size + dev->credit_size - 1) / dev->credit_size,
	// however as you might already know, there is no hardware division instruction
	// in the processor's instruction set. Given that most packets will only need
	// one or two credits, a loop-based division is probably an idea better than
	// having to call the software emulated division routine.
	unsigned credits = 0;
	pkt_size += dev->credit_size - 1;
	while (pkt_size >= dev->credit_size) {
		pkt_size -= dev->credit_size;
		credits ++;
	}

	return credits;
}

static void _ar6kHtcProcessCreditRpt(Ar6kDev* dev, Ar6kHtcCreditReport* pRpt, unsigned num_entries)
{
	// XX: Currently using naïve implementation - we simply collect all credits
	// and deliver them first-come-first-serve based on thread prio. No attempt
	// is made to implement a credit distribution algorithm based on QoS.
	unsigned total_credits = 0;
	for (unsigned i = 0; i < num_entries; i ++) {
		total_credits += pRpt[i].credits;
	}

	// Wake up threads waiting for credits
	ArmIrqState st = armIrqLockByPsr();
	dev->credit_avail += total_credits;
	if (s_ar6kCreditWaitList.next) {
		threadUnblockAllByValue(&s_ar6kCreditWaitList, (u32)dev);
	}
	armIrqUnlockByPsr(st);
}

static unsigned _ar6kHtcCheckCredits(Ar6kDev* dev, unsigned needed_credits)
{
	// If we don't have enough credirs, wait until we do
	ArmIrqState st = armIrqLockByPsr();
	unsigned avail;
	while ((avail = dev->credit_avail) < needed_credits) {
		threadBlock(&s_ar6kCreditWaitList, (u32)dev); // izQKJJ9tyZc
	}
	avail -= needed_credits;
	dev->credit_avail = avail;
	armIrqUnlockByPsr(st);

	// Return how many credits are left
	return avail;
}

static bool _ar6kHtcProcessTrailer(Ar6kDev* dev, void* trailer, size_t size)
{
	while (size) {
		// Ensure the record header fits
		if (size < sizeof(Ar6kHtcRecordHdr)) {
			dietPrint("[AR6K] bad trailer record hdr\n");
			return false;
		}

		// Retrieve record header
		Ar6kHtcRecordHdr* hdr = (Ar6kHtcRecordHdr*)trailer;
		trailer = hdr+1;
		size -= sizeof(Ar6kHtcRecordHdr);

		// Ensure the record data fits
		if (size < hdr->length) {
			dietPrint("[AR6K] bad trailer record size\n");
			return false;
		}

		// Retrieve record data
		void* data = trailer;
		trailer = (u8*)data + hdr->length;
		size -= hdr->length;

		// Process record
		switch (hdr->record_id) {
			default: {
				dietPrint("[AR6K] unk record id: %.2X\n", hdr->record_id);
				break; // ignore (without failing)
			}

			case Ar6kHtcRecordId_Null:
				break; // nop

			case Ar6kHtcRecordId_Credit: {
				_ar6kHtcProcessCreditRpt(dev, (Ar6kHtcCreditReport*)data, hdr->length/sizeof(Ar6kHtcCreditReport));
				break;
			}

			case Ar6kHtcRecordId_Lookahead: {
				Ar6kHtcLookaheadReport* rep = (Ar6kHtcLookaheadReport*)data;
				if ((rep->pre_valid ^ rep->post_valid) == 0xff) {
					// This report is valid - update lookahead
					memcpy(&dev->lookahead, rep->lookahead, sizeof(u32));
				}
				break;
			}
		}
	}

	return true;
}

static bool _ar6kHtcWaitforControlMessage(Ar6kDev* dev, _Ar6kHtcCtrlPktMem* mem)
{
	u32 lookahead = 0;
	if (!_ar6kDevPollMboxMsgRecv(dev, &lookahead, 200)) {
		return false;
	}

	Ar6kHtcEndpointId epid = _ar6kHtcLookaheadGetEndpointId(lookahead);
	if (epid != Ar6kHtcEndpointId_Control) {
		dietPrint("[AR6K] Unexpected epID: %u\n", epid);
		return false;
	}

	size_t len = sizeof(Ar6kHtcFrameHdr) + _ar6kHtcLookaheadGetPayloadLen(lookahead);
	if (!_ar6kDevRecvPacket(dev, mem, len)) {
		return false;
	}

	if (*(u32*)mem != lookahead) {
		dietPrint("[AR6K] Lookahead mismatch\n");
		return false;
	}

	return true;
}

bool _ar6kHtcRecvMessagePendingHandler(Ar6kDev* dev)
{
	do {
		// Grab lookahead
		u32 lookahead = dev->lookahead;
		dev->lookahead = 0;

		// Retrieve and validate endpoint ID
		Ar6kHtcEndpointId epid = _ar6kHtcLookaheadGetEndpointId(lookahead);
		if (epid >= Ar6kHtcEndpointId_Last) {
			dietPrint("[AR6K] RX invalid epid = %u\n", epid);
			return false;
		}

		// Retrieve service ID
		Ar6kHtcSrvId srvid = Ar6kHtcSrvId_HtcControl;
		if (epid != Ar6kHtcEndpointId_Control) {
			srvid = dev->endpoints[epid-1].service_id;
			if (!srvid) {
				dietPrint("[AR6K] RX unconnected epid = %u\n", epid);
				return false;
			}
		}

		// Retrieve and validate packet length
		size_t len = sizeof(Ar6kHtcFrameHdr) + _ar6kHtcLookaheadGetPayloadLen(lookahead);
		if (len > AR6K_HTC_MAX_PACKET_SZ) {
			dietPrint("[AR6K] RX pkt too big = %zu\n", len);
			return false;
		}

		// Allocate a new packet buffer
		NetBuf* pPacket;
		bool is_data = srvid >= Ar6kHtcSrvId_WmiDataBe && srvid <= Ar6kHtcSrvId_WmiDataVo;
		if (is_data) {
			while (!(pPacket = netbufAlloc(0, len, NetBufPool_Rx))) {
				// Try again after a little while
				threadSleep(1000);
			}
		} else {
			// Using the (statically allocated) work buffer for non-data packets
			pPacket = (NetBuf*)dev->workbuf;
			pPacket->capacity = AR6K_HTC_MAX_PACKET_SZ;
			pPacket->pos = 0;
			pPacket->len = len;
		}

		// Read packet!
		if (!_ar6kDevRecvPacket(dev, netbufGet(pPacket), len)) {
			dietPrint("[AR6K] RX pkt fail\n");
			return false;
		}

		// Compiler optimization
		MK_ASSUME(pPacket->pos == 0 && pPacket->len == len);

		// Validate packet against lookahead
		if (*(u32*)netbufGet(pPacket) != lookahead) {
			dietPrint("[AR6K] RX lookahead mismatch\n");
			if (is_data) netbufFree(pPacket);
			return false;
		}

		// Handle trailer
		Ar6kHtcFrameHdr* htchdr = netbufPopHeaderType(pPacket, Ar6kHtcFrameHdr);
		if (htchdr->flags & AR6K_HTC_FLAG_RECV_TRAILER) {
			void* trailer = netbufPopTrailer(pPacket, htchdr->recv_trailer_len);
			if (!trailer || !_ar6kHtcProcessTrailer(dev, trailer, htchdr->recv_trailer_len)) {
				if (is_data) netbufFree(pPacket);
				return false;
			}
		}

		// Drop the packet if it's empty
		if (pPacket->len == 0) {
			if (is_data) netbufFree(pPacket);
			continue;
		}

		// Handle packet!
		switch (srvid) {
			case Ar6kHtcSrvId_HtcControl: {
				dietPrint("[AR6K] Unexpected HTC ctrl pkt\n");
				// Ignore the packet
				break;
			}

			case Ar6kHtcSrvId_WmiControl: {
				_ar6kWmiEventRx(dev, pPacket);
				break;
			}

			default: /* Ar6kHtcSrvId_WmiDataXX */ {
				_ar6kWmiDataRx(dev, srvid, pPacket);
				break;
			}
		}

	} while (dev->lookahead != 0);

	return true;
}

bool _ar6kHtcSendPacket(Ar6kDev* dev, Ar6kHtcEndpointId epid, NetBuf* pPacket)
{
	if (epid == Ar6kHtcEndpointId_Control) {
		dietPrint("[AR6K] TX attempt on HTC ctrl\n");
		return false;
	}

	Ar6kEndpoint* ep = &dev->endpoints[epid-1];
	if (!ep->service_id) {
		dietPrint("[AR6K] TX attempt on unconn ep\n");
		return false;
	}

	u16 payload_len = pPacket->len;
	Ar6kHtcFrameHdr* htchdr = netbufPushHeaderType(pPacket, Ar6kHtcFrameHdr);
	if (!htchdr) {
		dietPrint("[AR6K] TX insufficient headroom\n");
		return false;
	}

	// Fill in HTC frame header
	htchdr->endpoint_id = epid;
	htchdr->flags = 0;
	htchdr->payload_len = payload_len;

	// Ensure that we actually have credits for sending this packet
	unsigned avail_credits = _ar6kHtcCheckCredits(dev,
		_ar6kHtcBytesToCredits(dev, pPacket->len));

	// If we're sending a WMI control message or dangerously running low on
	// credits, ask the device to send us a credit update report.
	// XX: Technically speaking, there's a race condition here where multiple
	// threads could end up requesting credit update reports; however we are
	// actually OK with that.
	if (ep->service_id == Ar6kHtcSrvId_WmiControl || avail_credits < dev->max_msg_credits) {
		htchdr->flags |= AR6K_HTC_FLAG_NEED_CREDIT_UPDATE;
	}

#if 0
	// XX: Dump packet
	dietPrint("[AR6K] TX");
	for (unsigned i = 0; i < pPacket->len; i ++) {
		dietPrint(" %.2X", ((u8*)netbufGet(pPacket))[i]);
	}
	dietPrint("\n");
#endif

	// Send the packet!
	return _ar6kDevSendPacket(dev, netbufGet(pPacket), pPacket->len);
}

bool ar6kHtcInit(Ar6kDev* dev)
{
	union {
		_Ar6kHtcCtrlPktMem mem;
		struct {
			Ar6kHtcFrameHdr hdr;
			Ar6kHtcCtrlMsgReady msg;
		};
	} u;

	if (!_ar6kHtcWaitforControlMessage(dev, &u.mem)) {
		return false;
	}

	if (u.msg.hdr.id != Ar6kHtcCtrlMsgId_Ready) {
		dietPrint("[AR6K] Unexpected HTC msg\n");
		return false;
	}

	dev->credit_size  = u.msg.credit_size;
	dev->credit_avail = u.msg.credit_count;
	dev->max_msg_credits = 0;

	dietPrint("[AR6K] Max endpoints: %u\n", u.msg.max_endpoints);
	dietPrint("[AR6K]   Credit size: %u\n", dev->credit_size);
	dietPrint("[AR6K]  Credit count: %u\n", dev->credit_avail);

	return true;
}

Ar6kHtcSrvStatus ar6kHtcConnectService(Ar6kDev* dev, Ar6kHtcSrvId service_id, u16 flags, Ar6kHtcEndpointId* out_ep)
{
	union {
		_Ar6kHtcCtrlPktMem mem;
		struct {
			Ar6kHtcFrameHdr hdr;
			union {
				Ar6kHtcCtrlCmdConnSrv req;
				Ar6kHtcCtrlCmdConnSrvReply resp;
			};
		};
	} u;

	u.hdr.endpoint_id = Ar6kHtcEndpointId_Control;
	u.hdr.flags = 0;
	u.hdr.payload_len = sizeof(Ar6kHtcFrameHdr) + sizeof(Ar6kHtcCtrlCmdConnSrv);
	u.req.hdr.id = Ar6kHtcCtrlMsgId_ConnectService;
	u.req.service_id = service_id;
	u.req.flags = flags;
	u.req.extra_len = 0;

	if (!_ar6kDevSendPacket(dev, &u.mem, u.hdr.payload_len)) {
		return Ar6kHtcSrvStatus_Failed;
	}

	if (!_ar6kHtcWaitforControlMessage(dev, &u.mem)) {
		return Ar6kHtcSrvStatus_Failed;
	}

	if (u.resp.hdr.id != Ar6kHtcCtrlMsgId_ConnectServiceReply) {
		dietPrint("[AR6K] Unexpected HTC msg\n");
		return Ar6kHtcSrvStatus_Failed;
	}

	if (u.resp.service_id != service_id) {
		dietPrint("[AR6K] Unexp. srv%.4X (%.4X)\n", u.resp.service_id, service_id);
		return Ar6kHtcSrvStatus_Failed;
	}

	if (u.resp.status != Ar6kHtcSrvStatus_Success) {
		dietPrint("[AR6K] srv%.4X fail (%u)\n", service_id, u.resp.status);
		return (Ar6kHtcSrvStatus)u.resp.status;
	}

	if (u.resp.endpoint_id == Ar6kHtcEndpointId_Control || u.resp.endpoint_id > Ar6kHtcEndpointId_Last) {
		dietPrint("[AR6K] srv%.4X bad ep (%u)\n", service_id, u.resp.endpoint_id);
		return Ar6kHtcSrvStatus_Failed;
	}

	if (out_ep) {
		*out_ep = (Ar6kHtcEndpointId)u.resp.endpoint_id;
	}

	Ar6kEndpoint* ep = &dev->endpoints[u.resp.endpoint_id-1];
	ep->service_id = service_id;
	ep->max_msg_size = u.resp.max_msg_size;

	unsigned msg_credits = _ar6kHtcBytesToCredits(dev, ep->max_msg_size);
	if (msg_credits > dev->max_msg_credits) {
		dev->max_msg_credits = msg_credits;
	}

	return Ar6kHtcSrvStatus_Success;
}

bool ar6kHtcSetupComplete(Ar6kDev* dev)
{
	union {
		_Ar6kHtcCtrlPktMem mem;
		struct {
			Ar6kHtcFrameHdr hdr;
			Ar6kHtcCtrlMsgHdr cmd;
		};
	} u;

	u.hdr.endpoint_id = Ar6kHtcEndpointId_Control;
	u.hdr.flags = 0;
	u.hdr.payload_len = sizeof(Ar6kHtcFrameHdr) + sizeof(Ar6kHtcCtrlMsgHdr);
	u.cmd.id = Ar6kHtcCtrlMsgId_SetupComplete;

	return _ar6kDevSendPacket(dev, &u.mem, u.hdr.payload_len);
}
