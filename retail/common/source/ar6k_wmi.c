// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#include "ar6k_common.h"
#include <string.h>

static void _ar6kWmixEventRx(Ar6kDev* dev, NetBuf* pPacket)
{
	Ar6kWmiGeneric32* evthdr = netbufPopHeaderType(pPacket, Ar6kWmiGeneric32);
	if (!evthdr) {
		dietPrint("[AR6K] WMIX event RX too small\n");
		return;
	}

	switch (evthdr->value) {
		default: {
			dietPrint("[AR6K] WMIX unkevt %.lX (%u)\n", evthdr->value, pPacket->len);
			break;
		}

		case Ar6kWmixEventId_DbgLog: {
			dietPrint("[AR6K] WMIX dbglog size=%u\n", pPacket->len);
			break;
		}
	}
}

void _ar6kWmiEventRx(Ar6kDev* dev, NetBuf* pPacket)
{
	Ar6kWmiCtrlHdr* evthdr = netbufPopHeaderType(pPacket, Ar6kWmiCtrlHdr);
	if (!evthdr) {
		dietPrint("[AR6K] WMI event RX too small\n");
		return;
	}

	switch (evthdr->id) {
		default: {
			dietPrint("[AR6K] WMI unkevt %.4X (%u)\n", evthdr->id, pPacket->len);
			break;
		}

		case Ar6kWmiEventId_Extension: {
			_ar6kWmixEventRx(dev, pPacket);
			break;
		}

		case Ar6kWmiEventId_Ready: {
			if (dev->wmi_ready) {
				dietPrint("[AR6K] dupe WMI_READY\n");
				break;
			}

			Ar6kWmiEvtReady* evt = netbufPopHeaderType(pPacket, Ar6kWmiEvtReady);
			if (!evt) {
				dietPrint("[AR6K] invalid WMI event size\n");
				break;
			}

			// Retrieve address to app host interest area
			u32 hi_app_host_interest = 0;
			if (!ar6kDevReadRegDiag(dev, dev->hia_addr+0x00, &hi_app_host_interest)) {
				dietPrint("[AR6K] cannot get AHI area\n");
				break;
			}

			// Set WMI protocol version
			if (!ar6kDevWriteRegDiag(dev, hi_app_host_interest, AR6K_WMI_PROTOCOL_VER)) {
				dietPrint("[AR6K] cannot set WMI ver\n");
				break;
			}

			// WMI is now ready for use!
			memcpy(dev->macaddr, evt->macaddr, 6);
			dev->wmi_ready = true;
			break;
		}

		case Ar6kWmiEventId_GetChannelListReply: {
			Ar6kWmiChannelList* list = (Ar6kWmiChannelList*)netbufGet(pPacket);
			u32 mask = 0;
			for (unsigned i = 0; i < list->num_channels; i ++) {
				mask |= 1U << wlanFreqToChannel(list->channel_mhz[i]);
			}
			dev->wmi_channel_mask = mask;
			break;
		}

		case Ar6kWmiEventId_BssInfo: {
			Ar6kWmiBssInfoHdr* hdr = netbufPopHeaderType(pPacket, Ar6kWmiBssInfoHdr);
			if (hdr && dev->cb_onBssInfo) {
				dev->cb_onBssInfo(dev, hdr, pPacket);
			}
			break;
		}

		case Ar6kWmiEventId_ScanComplete: {
			Ar6kWmiGeneric32* hdr = netbufPopHeaderType(pPacket, Ar6kWmiGeneric32);
			if (hdr && dev->cb_onScanComplete) {
				dev->cb_onScanComplete(dev, hdr->value);
			}
			break;
		}

		case Ar6kWmiEventId_Connected: {
			Ar6kWmiEvtConnected* hdr = netbufPopHeaderType(pPacket, Ar6kWmiEvtConnected);
			if (hdr && dev->cb_onAssoc) {
				dev->cb_onAssoc(dev, hdr);
			}
			break;
		}

		case Ar6kWmiEventId_Disconnected: {
			Ar6kWmiEvtDisconnected* hdr = netbufPopHeaderType(pPacket, Ar6kWmiEvtDisconnected);
			if (hdr && dev->cb_onDisassoc) {
				dev->cb_onDisassoc(dev, hdr);
			}
			break;
		}
	}
}

MK_CONSTEXPR bool _ar6kWmiIsLlcSnapPacket(NetMacHdr* machdr)
{
	return __builtin_bswap16(machdr->len_or_ethertype_be) < NetEtherType_First;
}

void _ar6kWmiDataRx(Ar6kDev* dev, Ar6kHtcSrvId srvid, NetBuf* pPacket)
{
	Ar6kWmiDataHdr* datahdr = netbufPopHeaderType(pPacket, Ar6kWmiDataHdr);
	if (!datahdr) {
		dietPrint("[AR6K] WMI data RX too small\n");
		return;
	}

	if (pPacket->len < sizeof(NetMacHdr)) {
		dietPrint("[AR6K] WMI RX missing MAC hdr\n");
		return;
	}

	NetMacHdr* machdr = (NetMacHdr*)netbufGet(pPacket);
	if (_ar6kWmiIsLlcSnapPacket(machdr)) {
		// LLC-SNAP header follows - back up MAC header and remove both headers
		NetMacHdr machdrdata = *netbufPopHeaderType(pPacket, NetMacHdr);
		NetLlcSnapHdr* llcsnaphdr = netbufPopHeaderType(pPacket, NetLlcSnapHdr);
		if (!llcsnaphdr) {
			dietPrint("[AR6K] WMI RX missing LLC SNAP\n");
			return;
		}

		// Convert MAC+LLC-SNAP to DIX (i.e. regular Ethernet II frame)
		// XX: Not validating LLC-SNAP header
		MK_ASSUME(pPacket->pos > sizeof(NetMacHdr));
		machdr = netbufPushHeaderType(pPacket, NetMacHdr);
		machdrdata.len_or_ethertype_be = llcsnaphdr->ethertype_be;
		memcpy(machdr, &machdrdata, sizeof(NetMacHdr));
	}

	if (dev->cb_rx) {
		int rssi = datahdr->rssi;
		pPacket->reserved[0] = rssi >= 0 ? rssi : 0;
		dev->cb_rx(dev, pPacket);
	}
}

bool ar6kWmiStartup(Ar6kDev* dev)
{
	// Wait for WMI to be ready
	while (!dev->wmi_ready) {
		threadSleep(1000);
	}

	dietPrint("[AR6K] WMI ready!\n       MAC %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
		dev->macaddr[0], dev->macaddr[1], dev->macaddr[2],
		dev->macaddr[3], dev->macaddr[4], dev->macaddr[5]);

	// Enable all error report events
	if (!ar6kWmiSetErrorReportBitmask(dev, AR6K_WMI_ERPT_ALL)) {
		dietPrint("[AR6K] can't set wmi erpt\n");
		return false;
	}

	// Shut up! :p
	if (!ar6kWmixConfigDebugModuleCmd(dev, UINT32_MAX, 0)) {
		dietPrint("[AR6K] can't set dbglog mask\n");
		return false;
	}

	// Disable heartbeat mechanism
	if (!ar6kWmiSetHbTimeout(dev, 0)) {
		dietPrint("[AR6K] can't disable heartbeat\n");
		return false;
	}

	// Retrieve channel list
	if (!ar6kWmiGetChannelList(dev)) {
		dietPrint("[AR6K] can't get chnlist\n");
		return false;
	}

	// Wait for the channel list (converted to bitmask) to be populated
	while (!dev->wmi_channel_mask) {
		threadSleep(1000);
	}

	dietPrint("[AR6K] channel mask %.8lX\n", dev->wmi_channel_mask);

	return true;
}

bool ar6kWmiTx(Ar6kDev* dev, NetBuf* pPacket)
{
	if (pPacket->len < sizeof(NetMacHdr)) {
		dietPrint("[AR6K] WMI bad TX packet\n");
		return false;
	}

	NetMacHdr* machdr = (NetMacHdr*)netbufGet(pPacket);
	if (!_ar6kWmiIsLlcSnapPacket(machdr)) {
		// Convert packet to LLC-SNAP
		NetMacHdr machdrdata = *netbufPopHeaderType(pPacket, NetMacHdr);
		NetLlcSnapHdr* llcsnaphdr = netbufPushHeaderType(pPacket, NetLlcSnapHdr);
		if (!llcsnaphdr) {
			dietPrint("[AR6K] WMI TX LLC SNAP fail\n");
			return false;
		}

		// Fill in LLC-SNAP header
		llcsnaphdr->dsap = 0xaa;
		llcsnaphdr->ssap = 0xaa;
		llcsnaphdr->control = 0x03;
		llcsnaphdr->oui[0] = 0;
		llcsnaphdr->oui[1] = 0;
		llcsnaphdr->oui[2] = 0;
		llcsnaphdr->ethertype_be = machdrdata.len_or_ethertype_be;
		machdrdata.len_or_ethertype_be = __builtin_bswap16(pPacket->len); // backup

		// Add new MAC header
		machdr = netbufPushHeaderType(pPacket, NetMacHdr);
		if (!machdr) {
			dietPrint("[AR6K] WMI TX MAC fail\n");
			return false;
		}

		// Fill in new MAC header
		*machdr = machdrdata;
	}

	// Add WMI data header
	Ar6kWmiDataHdr* datahdr = netbufPushHeaderType(pPacket, Ar6kWmiDataHdr);
	if (!datahdr) {
		dietPrint("[AR6K] WMI TX datahdr fail\n");
		return false;
	}

	// Fill in WMI data header
	datahdr->rssi = 0;
	datahdr->msg_type = 0;
	datahdr->user_prio = 0; // TODO: do something useful with this
	datahdr->_pad = 0;

	// Send packet! (TODO: select QoS endpoint)
	return _ar6kHtcSendPacket(dev, dev->wmi_data_epids[2], pPacket);
}

static NetBuf* _ar6kWmiAllocPacket(unsigned headroom, unsigned size)
{
	NetBuf* pPacket;
	while (!(pPacket = netbufAlloc(headroom, size, NetBufPool_Tx))) {
		// Try again after a little while
		threadSleep(1000);
	}
	return pPacket;
}

MK_INLINE NetBuf* _ar6kWmiAllocCmdPacket(unsigned size)
{
	unsigned hdr_len = sizeof(Ar6kHtcFrameHdr) + sizeof(Ar6kWmiCtrlHdr);
	NetBuf* pPacket = _ar6kWmiAllocPacket(hdr_len, size);
	pPacket->len = 0;
	MK_ASSUME(pPacket->pos == hdr_len);
	MK_ASSUME(pPacket->capacity >= (size + hdr_len));
	return pPacket;
}

static bool _ar6kWmiSendCmdPacket(Ar6kDev* dev, Ar6kWmiCmdId cmdid, NetBuf* pPacket)
{
	bool rc = false;
	Ar6kWmiCtrlHdr* hdr = netbufPushHeaderType(pPacket, Ar6kWmiCtrlHdr);
	if (hdr) {
		hdr->id = cmdid;
		rc = _ar6kHtcSendPacket(dev, dev->wmi_ctrl_epid, pPacket);
	}
	netbufFree(pPacket);
	return rc;
}

bool ar6kWmiSimpleCmd(Ar6kDev* dev, Ar6kWmiCmdId cmdid)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(0);
	return _ar6kWmiSendCmdPacket(dev, cmdid, pPacket);
}

bool ar6kWmiSimpleCmdWithParam8(Ar6kDev* dev, Ar6kWmiCmdId cmdid, u8 param)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiGeneric8));
	netbufPushTrailerType(pPacket, Ar6kWmiGeneric8)->value = param;
	return _ar6kWmiSendCmdPacket(dev, cmdid, pPacket);
}

bool ar6kWmiSimpleCmdWithParam32(Ar6kDev* dev, Ar6kWmiCmdId cmdid, u32 param)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiGeneric32));
	netbufPushTrailerType(pPacket, Ar6kWmiGeneric32)->value = param;
	return _ar6kWmiSendCmdPacket(dev, cmdid, pPacket);
}

bool ar6kWmiConnect(Ar6kDev* dev, Ar6kWmiConnectParams const* params)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiConnectParams));
	*netbufPushTrailerType(pPacket, Ar6kWmiConnectParams) = *params;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_Connect, pPacket);
}

bool ar6kWmiCreatePstream(Ar6kDev* dev, Ar6kWmiPstreamConfig const* config)
{
	u32 pkt_size = sizeof(Ar6kWmiPstreamConfig)-1;
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(pkt_size);
	memcpy(netbufPushTrailer(pPacket, pkt_size), config, pkt_size);
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_CreatePstream, pPacket);
}

bool ar6kWmiStartScan(Ar6kDev* dev, Ar6kWmiScanType type, u32 home_dwell_time_ms)
{
	u32 packet_size = offsetof(Ar6kWmiCmdStartScan, channel_mhz);
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(packet_size);
	Ar6kWmiCmdStartScan* cmd = (Ar6kWmiCmdStartScan*)netbufPushTrailer(pPacket, packet_size);
	// XX: maybe make some more of these configurable?
	cmd->force_bg_scan = 0;
	cmd->is_legacy = 0;
	cmd->home_dwell_time_ms = home_dwell_time_ms;
	cmd->force_scan_interval_ms = 0;
	cmd->scan_type = type;
	cmd->num_channels = 0;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_StartScan, pPacket);
}

bool ar6kWmiSetScanParams(Ar6kDev* dev, Ar6kWmiScanParams const* params)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiScanParams));
	*netbufPushTrailerType(pPacket, Ar6kWmiScanParams) = *params;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_SetScanParams, pPacket);
}

bool ar6kWmiSetBssFilter(Ar6kDev* dev, Ar6kWmiBssFilter filter, u32 ie_mask)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiCmdBssFilter));
	Ar6kWmiCmdBssFilter* cmd = netbufPushTrailerType(pPacket, Ar6kWmiCmdBssFilter);
	cmd->bss_filter = filter;
	cmd->ie_mask = ie_mask;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_SetBssFilter, pPacket);
}

bool ar6kWmiSetProbedSsid(Ar6kDev* dev, Ar6kWmiProbedSsid const* probed_ssid)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiProbedSsid));
	*netbufPushTrailerType(pPacket, Ar6kWmiProbedSsid) = *probed_ssid;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_SetProbedSsid, pPacket);
}

bool ar6kWmiSetChannelParams(Ar6kDev* dev, u8 scan_param, u32 chan_mask)
{
	static const u32 chan_mask_a = 0x7fc000; // Channels 14..22
	static const u32 chan_mask_g = 0x003fff; // Channels  0..13
	chan_mask &= chan_mask_a|chan_mask_g;

	u16 num_channels = __builtin_popcount(chan_mask);
	unsigned pkt_len = sizeof(Ar6kWmiChannelParams) + num_channels*sizeof(u16);
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(pkt_len);
	Ar6kWmiChannelParams* cmd = (Ar6kWmiChannelParams*)netbufPushTrailer(pPacket, pkt_len);

	cmd->scan_param = scan_param;
	cmd->num_channels = num_channels;

	if (chan_mask) {
		if (chan_mask & chan_mask_g) {
			if (cmd->phy_mode & chan_mask_a) {
				cmd->phy_mode = Ar6kWmiPhyMode_11AG;
			} else {
				cmd->phy_mode = Ar6kWmiPhyMode_11G;
			}
		} else /* if (chan_mask & chan_mask_a) */ {
			cmd->phy_mode = Ar6kWmiPhyMode_11A;
		}

		for (unsigned i = 0, j = 0; j < 23; j ++) {
			if (chan_mask & (1U << j)) {
				cmd->channel_mhz[i++] = wlanChannelToFreq(j);
			}
		}
	} else {
		cmd->phy_mode = Ar6kWmiPhyMode_11G;
	}

	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_SetChannelParams, pPacket);
}

bool ar6kWmiAddCipherKey(Ar6kDev* dev, Ar6kWmiCipherKey const* key)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiCipherKey));
	*netbufPushTrailerType(pPacket, Ar6kWmiCipherKey) = *key;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_AddCipherKey, pPacket);
}

bool ar6kWmiSetFrameRate(Ar6kDev* dev, unsigned ieee_frame_type, unsigned ieee_frame_subtype, unsigned rate_mask)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiCmdSetFrameRate));
	Ar6kWmiCmdSetFrameRate* cmd = netbufPushTrailerType(pPacket, Ar6kWmiCmdSetFrameRate);
	cmd->enable_frame_mask = rate_mask ? 1 : 0;
	cmd->frame_type = ieee_frame_type;
	cmd->frame_subtype = ieee_frame_subtype;
	cmd->frame_rate_mask = rate_mask;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_SetFrameRate, pPacket);
}

bool ar6kWmiSetBitRate(Ar6kDev* dev, Ar6kWmiBitRate data_rate, Ar6kWmiBitRate mgmt_rate, Ar6kWmiBitRate ctrl_rate)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiCmdSetBitRate));
	Ar6kWmiCmdSetBitRate* cmd = netbufPushTrailerType(pPacket, Ar6kWmiCmdSetBitRate);
	cmd->data_rate_idx = data_rate;
	cmd->mgmt_rate_idx = mgmt_rate;
	cmd->ctrl_rate_idx = ctrl_rate;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_SetBitRate, pPacket);
}

bool ar6kWmixConfigDebugModuleCmd(Ar6kDev* dev, u32 cfgmask, u32 config)
{
	NetBuf* pPacket = _ar6kWmiAllocCmdPacket(sizeof(Ar6kWmiGeneric32) + sizeof(Ar6kWmixDbgLogCfgModule));
	netbufPushTrailerType(pPacket, Ar6kWmiGeneric32)->value = Ar6kWmixCmdId_DbgLogCfgModule;
	Ar6kWmixDbgLogCfgModule* cmd = netbufPushTrailerType(pPacket, Ar6kWmixDbgLogCfgModule);
	cmd->cfgvalid = cfgmask;
	cmd->dbglog_config = config;
	return _ar6kWmiSendCmdPacket(dev, Ar6kWmiCmdId_Extension, pPacket);
}
