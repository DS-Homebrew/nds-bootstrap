// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "ar6k_base.h"
#include "ar6k_htc.h"

#define AR6K_WMI_PROTOCOL_VER 2

MK_EXTERN_C_START

typedef enum Ar6kWmiCmdId {
	Ar6kWmiCmdId_Connect                  = 0x0001,
	Ar6kWmiCmdId_Disconnect               = 0x0003,
	Ar6kWmiCmdId_Synchronize              = 0x0004,
	Ar6kWmiCmdId_CreatePstream            = 0x0005,
	Ar6kWmiCmdId_StartScan                = 0x0007,
	Ar6kWmiCmdId_SetScanParams            = 0x0008,
	Ar6kWmiCmdId_SetBssFilter             = 0x0009,
	Ar6kWmiCmdId_SetProbedSsid            = 0x000a,
	Ar6kWmiCmdId_SetDiscTimeout           = 0x000d,
	Ar6kWmiCmdId_GetChannelList           = 0x000e,
	Ar6kWmiCmdId_SetChannelParams         = 0x0011,
	Ar6kWmiCmdId_SetPowerMode             = 0x0012,
	Ar6kWmiCmdId_AddCipherKey             = 0x0016,
	Ar6kWmiCmdId_TargetErrorReportBitmask = 0x0022,
	Ar6kWmiCmdId_Extension                = 0x002e,
	Ar6kWmiCmdId_SetKeepAlive             = 0x003d,
	Ar6kWmiCmdId_SetWscStatus             = 0x0041,
	Ar6kWmiCmdId_SetHbTimeout             = 0x0047,
	Ar6kWmiCmdId_SetFrameRate             = 0x0048,
	Ar6kWmiCmdId_HostExitNotify           = 0x0049,
	Ar6kWmiCmdId_SetBitRate               = 0xf000,
} Ar6kWmiCmdId;

typedef enum Ar6kWmixCmdId {
	Ar6kWmixCmdId_DbgLogCfgModule         = 0x2009,
} Ar6kWmixCmdId;

typedef enum Ar6kWmiEventId {
	Ar6kWmiEventId_GetChannelListReply    = 0x000e,
	Ar6kWmiEventId_Ready                  = 0x1001,
	Ar6kWmiEventId_Connected              = 0x1002,
	Ar6kWmiEventId_Disconnected           = 0x1003,
	Ar6kWmiEventId_BssInfo                = 0x1004,
	Ar6kWmiEventId_ScanComplete           = 0x100a,
	Ar6kWmiEventId_Extension              = 0x1010,
} Ar6kWmiEventId;

typedef enum Ar6kWmixEventId {
	Ar6kWmixEventId_DbgLog                = 0x3008,
} Ar6kWmixEventId;

typedef struct Ar6kWmiCtrlHdr {
	u16 id; // Ar6kWmiCmdId or Ar6kWmiEventId
} Ar6kWmiCtrlHdr;

typedef struct Ar6kWmiDataHdr {
	s8 rssi;
	u8 msg_type  : 2;
	u8 user_prio : 3;
	u8 _pad      : 3;
} Ar6kWmiDataHdr;

typedef struct Ar6kWmiGeneric8 {
	u8 value;
} Ar6kWmiGeneric8;

typedef struct Ar6kWmiGeneric16 {
	u16 value;
} Ar6kWmiGeneric16;

typedef struct Ar6kWmiGeneric32 {
	u32 value;
} Ar6kWmiGeneric32;

// Error report bits
#define AR6K_WMI_ERPT_TARGET_PM_ERR_FAIL    (1U<<0)
#define AR6K_WMI_ERPT_TARGET_KEY_NOT_FOUND  (1U<<1)
#define AR6K_WMI_ERPT_TARGET_DECRYPTION_ERR (1U<<2)
#define AR6K_WMI_ERPT_TARGET_BMISS          (1U<<3)
#define AR6K_WMI_ERPT_PSDISABLE_NODE_JOIN   (1U<<4)
#define AR6K_WMI_ERPT_TARGET_COM_ERR        (1U<<5)
#define AR6K_WMI_ERPT_TARGET_FATAL_ERR      (1U<<6)
#define AR6K_WMI_ERPT_ALL                   0x7f

typedef enum Ar6kWmiPhyMode {
	Ar6kWmiPhyMode_11A     = 1,
	Ar6kWmiPhyMode_11G     = 2,
	Ar6kWmiPhyMode_11AG    = 3,
	Ar6kWmiPhyMode_11B     = 4,
	Ar6kWmiPhyMode_11Gonly = 5,
} Ar6kWmiPhyMode;

typedef enum Ar6kWmiPowerMode {
	Ar6kWmiPowerMode_Recommended    = 1,
	Ar6kWmiPowerMode_MaxPerformance = 2,
} Ar6kWmiPowerMode;

typedef enum Ar6kWmiTrafficDir {
	Ar6kWmiTrafficDir_Uplink   = 0,
	Ar6kWmiTrafficDir_Downlink = 1,
	Ar6kWmiTrafficDir_Bidir    = 2,
} Ar6kWmiTrafficDir;

typedef enum Ar6kWmiTrafficType {
	Ar6kWmiTrafficType_Aperiodic = 0,
	Ar6kWmiTrafficType_Periodic  = 1,
} Ar6kWmiTrafficType;

typedef enum Ar6kWmiVoicePSCap {
	Ar6kWmiVoicePSCap_DisableForThisAC = 0,
	Ar6kWmiVoicePSCap_EnableForThisAC  = 1,
	Ar6kWmiVoicePSCap_EnableForAllAC   = 2,
} Ar6kWmiVoicePSCap;

typedef enum Ar6kWmiNetworkType {
	Ar6kWmiNetworkType_Infrastructure = 0x01,
	Ar6kWmiNetworkType_Adhoc          = 0x02,
	Ar6kWmiNetworkType_AdhocCreator   = 0x04,
	Ar6kWmiNetworkType_Opt            = 0x08,
	Ar6kWmiNetworkType_AccessPoint    = 0x10,
} Ar6kWmiNetworkType;

typedef enum Ar6kWmiAuthModeIeee {
	Ar6kWmiAuthModeIeee_Open   = 0x01,
	Ar6kWmiAuthModeIeee_Shared = 0x02,
	Ar6kWmiAuthModeIeee_Leap   = 0x04, // not actually IEEE_AUTH_MODE
} Ar6kWmiAuthModeIeee;

typedef enum Ar6kWmiAuthMode {
	Ar6kWmiAuthMode_Open          = 1,
	Ar6kWmiAuthType_WPA           = 2, // i.e. Enterprise (RADIUS)
	Ar6kWmiAuthType_WPA_PSK       = 3,
	Ar6kWmiAuthType_WPA2          = 4, // i.e. Enterprise (RADIUS)
	Ar6kWmiAuthType_WPA2_PSK      = 5,
	Ar6kWmiAuthType_WPA_CCKM      = 6, // Related to Cisco/LEAP
	Ar6kWmiAuthType_WPA2_CCKM     = 7, // As above
} Ar6kWmiAuthMode;

typedef enum Ar6kWmiCipherType {
	Ar6kWmiCipherType_None = 1,
	Ar6kWmiCipherType_WEP  = 2,
	Ar6kWmiCipherType_TKIP = 3,
	Ar6kWmiCipherType_AES  = 4,
	Ar6kWmiCipherType_CCKM = 5,
} Ar6kWmiCipherType;

#define AR6K_WMI_CIPHER_USAGE_PAIRWISE (0U<<0)
#define AR6K_WMI_CIPHER_USAGE_GROUP    (1U<<0)
#define AR6K_WMI_CIPHER_USAGE_TX       (1U<<1)

//-----------------------------------------------------------------------------
// WMI commands
//-----------------------------------------------------------------------------

typedef struct Ar6kWmiConnectParams {
	u8   network_type;         // Ar6kWmiNetworkType
	u8   auth_mode_ieee;       // Ar6kWmiAuthModeIeee
	u8   auth_mode;            // Ar6kWmiAuthMode
	u8   pairwise_cipher_type; // Ar6kWmiCipherType
	u8   pairwise_key_len;
	u8   group_cipher_type;    // Ar6kWmiCipherType
	u8   group_key_len;
	u8   ssid_len;
	char ssid[32];
	u16  channel_mhz;
	u8   bssid[6];
	u32  ctrl_flags;
} Ar6kWmiConnectParams;

typedef struct Ar6kWmiPstreamConfig {
	u32 min_service_int_msec;
	u32 max_service_int_msec;
	u32 inactivity_int_msec;
	u32 suspension_int_msec;
	u32 srv_start_time;
	u32 min_data_rate_bps;
	u32 mean_data_rate_bps;
	u32 peak_data_rate_bps;
	u32 max_burst_size;
	u32 delay_bound;
	u32 min_phy_rate_bps;
	u32 sba;
	u32 medium_time;
	u16 nominal_msdu;
	u16 max_msdu;
	u8  traffic_class;
	u8  traffic_dir;   // Ar6kWmiTrafficDir
	u8  rx_queue_num;
	u8  traffic_type;  // Ar6kWmiTrafficType
	u8  voice_ps_cap;  // Ar6kWmiVoicePSCap
	u8  tsid;
	u8  user_prio;     // "802.1d user priority"
	// no padding
} Ar6kWmiPstreamConfig;

typedef enum Ar6kWmiScanType {
	Ar6kWmiScanType_Long  = 0,
	Ar6kWmiScanType_Short = 1,
} Ar6kWmiScanType;

typedef struct Ar6kWmiCmdStartScan {
	u32 force_bg_scan;          // bool
	u32 is_legacy;              // bool - "for legacy Cisco AP compatibility"
	u32 home_dwell_time_ms;     // "Maximum duration in the home channel"
	u32 force_scan_interval_ms; // "Time interval between scans"
	u8 scan_type;               // Ar6kWmiScanType
	u8 num_channels;
	u16 channel_mhz[];
} Ar6kWmiCmdStartScan;

#define AR6K_WMI_SCAN_CONNECT        (1U<<0) // "set if can scan in the Connect cmd" (sic)
#define AR6K_WMI_SCAN_CONNECTED      (1U<<1) // "set if scan for the SSID it is already connected to"
#define AR6K_WMI_SCAN_ACTIVE         (1U<<2) // active scan (as opposed to passive scan)
#define AR6K_WMI_SCAN_ROAM           (1U<<3) // "enable roam scan when bmiss and lowrssi"
#define AR6K_WMI_SCAN_REPORT_BSSINFO (1U<<4) // presumably enables bssinfo events
#define AR6K_WMI_SCAN_ENABLE_AUTO    (1U<<5) // "if disabled, target doesn't scan after a disconnect event"
#define AR6K_WMI_SCAN_ENABLE_ABORT   (1U<<6) // "Scan complete event with canceled status will be generated when a scan is prempted before it gets completed"

typedef struct Ar6kWmiScanParams {
	u16 fg_start_period_secs;
	u16 fg_end_period_secs;
	u16 bg_period_secs;
	u16 maxact_chdwell_time_ms;
	u16 pas_chdwell_time_ms;
	u8  short_scan_ratio;
	u8  scan_ctrl_flags;
	u16 minact_chdwell_time_ms;
	u16 _pad;
	u32 max_dfsch_act_time_ms;
} Ar6kWmiScanParams;

typedef enum Ar6kWmiBssFilter {
	Ar6kWmiBssFilter_None          = 0, // "no beacons forwarded"
	Ar6kWmiBssFilter_All           = 1, // "all beacons forwarded"
	Ar6kWmiBssFilter_Profile       = 2, // "only beacons matching profile"
	Ar6kWmiBssFilter_AllButProfile = 3, // "all but beacons matching profile"
	Ar6kWmiBssFilter_CurrentBss    = 4, // "only beacons matching current BSS"
	Ar6kWmiBssFilter_AllButBss     = 5, // "all but beacons matching BSS"
	Ar6kWmiBssFilter_ProbedSsid    = 6, // "beacons matching probed ssid"
} Ar6kWmiBssFilter;

typedef struct Ar6kWmiCmdBssFilter {
	u8 bss_filter; // Ar6kWmiBssFilter
	u8 _pad[3];
	u32 ie_mask;
} Ar6kWmiCmdBssFilter;

typedef enum Ar6kWmiSsidProbeMode {
	Ar6kWmiSsidProbeMode_Disable  = 0,
	Ar6kWmiSsidProbeMode_Specific = 1,
	Ar6kWmiSsidProbeMode_Any      = 2,
} Ar6kWmiSsidProbeMode;

typedef struct Ar6kWmiProbedSsid {
	u8 entry_idx;
	u8 mode; // Ar6kWmiSsidProbeMode
	u8 ssid_len;
	char ssid[32];
} Ar6kWmiProbedSsid;

typedef struct Ar6kWmiChannelParams {
	u8  reserved;
	u8  scan_param;   // ???
	u8  phy_mode;     // Ar6kWmiPhyMode
	u8  num_channels;
	u16 channel_mhz[];
} Ar6kWmiChannelParams;

#define AR6K_WMI_KEY_OP_INIT_TSC (1U<<0)
#define AR6K_WMI_KEY_OP_INIT_RSC (1U<<1)

typedef struct Ar6kWmiCipherKey {
	u8 index;
	u8 type;    // Ar6kWmiCipherType
	u8 usage;   // bitmask of AR6K_WMI_CIPHER_USAGE_*
	u8 length;
	u8 rsc[8];  // replay sequence counter
	u8 key[32];
	u8 op_ctrl; // bitmask of AR6K_WMI_KEY_OP_*
} Ar6kWmiCipherKey;

typedef enum Ar6kWmiBitRate {
	Ar6kWmiBitRate_Auto    = -1,
	Ar6kWmiBitRate_1Mbps   = 0,
	Ar6kWmiBitRate_2Mbps   = 1,
	Ar6kWmiBitRate_5_5Mbps = 2,
	Ar6kWmiBitRate_11Mbps  = 3,
	Ar6kWmiBitRate_6Mbps   = 4,
	Ar6kWmiBitRate_9Mbps   = 5,
	Ar6kWmiBitRate_12Mbps  = 6,
	Ar6kWmiBitRate_18Mbps  = 7,
	Ar6kWmiBitRate_24Mbps  = 8,
	Ar6kWmiBitRate_36Mbps  = 9,
	Ar6kWmiBitRate_48Mbps  = 10,
	Ar6kWmiBitRate_54Mbps  = 11,
} Ar6kWmiBitRate;

typedef struct Ar6kWmiCmdSetFrameRate {
	u8 enable_frame_mask; // bool
	u8 frame_type    : 4; // 802.11 frame type (bit0..3) (only 0=mgmt 4=ctrl allowed)
	u8 frame_subtype : 4; // 802.11 frame subtype (bit4..7)
	u16 frame_rate_mask;  // one bit per Ar6kWmiBitRate enum member (except -1)
} Ar6kWmiCmdSetFrameRate;

typedef struct Ar6kWmiCmdSetBitRate {
	s8 data_rate_idx; // Ar6kWmiBitRate
	s8 mgmt_rate_idx; // Ar6kWmiBitRate
	s8 ctrl_rate_idx; // Ar6kWmiBitRate
} Ar6kWmiCmdSetBitRate;

typedef struct Ar6kWmixDbgLogCfgModule {
	u32 cfgvalid;
	u32 dbglog_config;
} Ar6kWmixDbgLogCfgModule;

//-----------------------------------------------------------------------------
// WMI events
//-----------------------------------------------------------------------------

typedef struct Ar6kWmiEvtReady {
	u8 macaddr[6];
	u8 phy_capability;
} Ar6kWmiEvtReady;

typedef struct __attribute__((packed)) Ar6kWmiEvtConnected {
	u16 channel_mhz;
	u8 bssid[6];
	u16 listen_interval;
	u16 beacon_interval;
	u32 network_type; // Ar6kWmiNetworkType
	u8 beacon_ie_len;
	u8 assoc_req_len;
	u8 assoc_resp_len;
	u8 assoc_info[]; // beacon_ie + assoc_req + assoc_resp
} Ar6kWmiEvtConnected;

typedef struct Ar6kWmiEvtDisconnected {
	u16 reason_ieee;
	u8 bssid[6];
	u8 reason;
	u8 assoc_resp_len;
	u8 assoc_resp[];
} Ar6kWmiEvtDisconnected;

typedef struct Ar6kWmiChannelList {
	u8  reserved;
	u8  num_channels;
	u16 channel_mhz[];
} Ar6kWmiChannelList;

typedef enum Ar6kWmiBIFrameType {
	Ar6kWmiBIFrameType_Beacon     = 1,
	Ar6kWmiBIFrameType_ProbeResp  = 2,
	Ar6kWmiBIFrameType_ActionMgmt = 3,
	Ar6kWmiBIFrameType_ProbeReq   = 4,
	Ar6kWmiBIFrameType_AssocReq   = 5,
	Ar6kWmiBIFrameType_AssocResp  = 6,
} Ar6kWmiBIFrameType;

typedef struct Ar6kWmiBssInfoHdr {
	u16 channel_mhz;
	u8  frame_type; // Ar6kWmiBIFrameType
	s8  snr;
	s16 rssi;
	u8  bssid[6];
	u32 ie_mask;
	// beacon or probe-response frame body follows (sans 802.11 frame header)
} Ar6kWmiBssInfoHdr;

//-----------------------------------------------------------------------------
// WMI APIs
//-----------------------------------------------------------------------------

bool ar6kWmiStartup(Ar6kDev* dev);

bool ar6kWmiTx(Ar6kDev* dev, NetBuf* pPacket);

bool ar6kWmiSimpleCmd(Ar6kDev* dev, Ar6kWmiCmdId cmdid);
bool ar6kWmiSimpleCmdWithParam8(Ar6kDev* dev, Ar6kWmiCmdId cmdid, u8 param);
bool ar6kWmiSimpleCmdWithParam32(Ar6kDev* dev, Ar6kWmiCmdId cmdid, u32 param);

bool ar6kWmiConnect(Ar6kDev* dev, Ar6kWmiConnectParams const* params);
bool ar6kWmiCreatePstream(Ar6kDev* dev, Ar6kWmiPstreamConfig const* config);
bool ar6kWmiStartScan(Ar6kDev* dev, Ar6kWmiScanType type, u32 home_dwell_time_ms);
bool ar6kWmiSetScanParams(Ar6kDev* dev, Ar6kWmiScanParams const* params);
bool ar6kWmiSetBssFilter(Ar6kDev* dev, Ar6kWmiBssFilter filter, u32 ie_mask);
bool ar6kWmiSetProbedSsid(Ar6kDev* dev, Ar6kWmiProbedSsid const* probed_ssid);
bool ar6kWmiSetChannelParams(Ar6kDev* dev, u8 scan_param, u32 chan_mask);
bool ar6kWmiAddCipherKey(Ar6kDev* dev, Ar6kWmiCipherKey const* key);
bool ar6kWmiSetFrameRate(Ar6kDev* dev, unsigned ieee_frame_type, unsigned ieee_frame_subtype, unsigned rate_mask);
bool ar6kWmiSetBitRate(Ar6kDev* dev, Ar6kWmiBitRate data_rate, Ar6kWmiBitRate mgmt_rate, Ar6kWmiBitRate ctrl_rate);

bool ar6kWmixConfigDebugModuleCmd(Ar6kDev* dev, u32 cfgmask, u32 config);

MK_INLINE bool ar6kWmiDisconnect(Ar6kDev* dev)
{
	return ar6kWmiSimpleCmd(dev, Ar6kWmiCmdId_Disconnect);
}

MK_INLINE bool ar6kWmiSetDiscTimeout(Ar6kDev* dev, u8 timeout)
{
	return ar6kWmiSimpleCmdWithParam8(dev, Ar6kWmiCmdId_SetDiscTimeout, timeout);
}

MK_INLINE bool ar6kWmiGetChannelList(Ar6kDev* dev)
{
	return ar6kWmiSimpleCmd(dev, Ar6kWmiCmdId_GetChannelList);
}

MK_INLINE bool ar6kWmiSetPowerMode(Ar6kDev* dev, Ar6kWmiPowerMode mode)
{
	return ar6kWmiSimpleCmdWithParam8(dev, Ar6kWmiCmdId_SetPowerMode, mode);
}

MK_INLINE bool ar6kWmiSetKeepAlive(Ar6kDev* dev, u8 interval)
{
	return ar6kWmiSimpleCmdWithParam8(dev, Ar6kWmiCmdId_SetKeepAlive, interval);
}

MK_INLINE bool ar6kWmiSetWscStatus(Ar6kDev* dev, bool enable)
{
	return ar6kWmiSimpleCmdWithParam8(dev, Ar6kWmiCmdId_SetWscStatus, enable?1:0);
}

MK_INLINE bool ar6kWmiSetHbTimeout(Ar6kDev* dev, u32 timeout)
{
	return ar6kWmiSimpleCmdWithParam32(dev, Ar6kWmiCmdId_SetHbTimeout, timeout);
}

MK_INLINE bool ar6kWmiSetErrorReportBitmask(Ar6kDev* dev, u32 bitmask)
{
	return ar6kWmiSimpleCmdWithParam32(dev, Ar6kWmiCmdId_TargetErrorReportBitmask, bitmask);
}

MK_INLINE bool ar6kWmiHostExitNotify(Ar6kDev* dev)
{
	return ar6kWmiSimpleCmd(dev, Ar6kWmiCmdId_HostExitNotify);
}

MK_EXTERN_C_END
