// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "ar6k_netbuf.h"

/*! @addtogroup wlan
	@{
*/

/*! @name Common Organizationally-Unique Identifiers
	@{
*/

#define WLAN_OUI_NINTENDO  0x0009bf
#define WLAN_OUI_IEEE      0x000fac
#define WLAN_OUI_MICROSOFT 0x0050f2

//! @}

#define WLAN_MAX_BSS_ENTRIES 32 //!< Maximum number of entries in a @ref WlanBssDesc array
#define WLAN_MAX_SSID_LEN    32 //!< Maximum length in bytes of a SSID
#define WLAN_WEP_40_LEN      5  //!< Maximum length in bytes of a WEP-40 key
#define WLAN_WEP_104_LEN     13 //!< Maximum length in bytes of a WEP-104 key
#define WLAN_WEP_128_LEN     16 //!< Maximum length in bytes of a WEP-128 key
#define WLAN_WPA_PSK_LEN     32 //!< Maximum length in bytes of a WPA-PSK key

#define WLAN_RSSI_STRENGTH_1 10 //!< RSSI for one bar of signal strength
#define WLAN_RSSI_STRENGTH_2 16 //!< RSSI for two bars of signal strength
#define WLAN_RSSI_STRENGTH_3 21 //!< RSSI for three bars of signal strength

MK_EXTERN_C_START

/*! @name Potentially misaligned integer datatypes
	@{
*/

typedef u8 Wlan16[2]; //!< Potentially misaligned 16-bit integer
typedef u8 Wlan24[3]; //!< Potentially misaligned 24-bit integer
typedef u8 Wlan32[4]; //!< Potentially misaligned 32-bit integer

//! @}

//! IEEE 802.11 frame types
typedef enum WlanFrameType {
	WlanFrameType_Management = 0,
	WlanFrameType_Control    = 1,
	WlanFrameType_Data       = 2,
} WlanFrameType;

//! IEEE 802.11 management frame types
typedef enum WlanMgmtType {
	WlanMgmtType_AssocReq    = 0,
	WlanMgmtType_AssocResp   = 1,
	WlanMgmtType_ReassocReq  = 2,
	WlanMgmtType_ReassocResp = 3,
	WlanMgmtType_ProbeReq    = 4,
	WlanMgmtType_ProbeResp   = 5,
	WlanMgmtType_Beacon      = 8,
	WlanMgmtType_ATIM        = 9,
	WlanMgmtType_Disassoc    = 10,
	WlanMgmtType_Auth        = 11,
	WlanMgmtType_Deauth      = 12,
	WlanMgmtType_Action      = 13,
} WlanMgmtType;

//! IEEE 802.11 control frame types
typedef enum WlanCtrlType {
	WlanCtrlType_BlockAckReq = 8,
	WlanCtrlType_BlockAck    = 9,
	WlanCtrlType_PSPoll      = 10,
	WlanCtrlType_RTS         = 11,
	WlanCtrlType_CTS         = 12,
	WlanCtrlType_ACK         = 13,
	WlanCtrlType_CFEnd       = 14,
	WlanCtrlType_CFEnd_CFAck = 15,
} WlanCtrlType;

/*! @name IEEE 802.11 data frame flags
	@{
*/

// "WlanDataType" is just a collection of flags
#define WLAN_DATA_CF_ACK  (1U<<0)
#define WLAN_DATA_CF_POLL (1U<<1)
#define WLAN_DATA_IS_NULL (1U<<2)
#define WLAN_DATA_IS_QOS  (1U<<3)

//! @}

//! IEEE 802.11 information element IDs
typedef enum WlanEid {
	WlanEid_SSID             = 0,
	WlanEid_SupportedRates   = 1,
	WlanEid_DSParamSet       = 3,
	WlanEid_CFParamSet       = 4,
	WlanEid_TIM              = 5,
	WlanEid_ChallengeText    = 16,
	WlanEid_RSN              = 48,
	WlanEid_SupportedRatesEx = 50,
	WlanEid_Vendor           = 221, // also used for NN-specific data
} WlanEid;

//! IEEE 802.11 frame control field
typedef union WlanFrameCtrl {
	u16 value;
	struct {
		u16 version    : 2;
		u16 type       : 2; // WlanFrameType
		u16 subtype    : 4; // WlanMgmtType, WlanCtrlType, WLAN_DATA_* flags
		u16 to_ds      : 1;
		u16 from_ds    : 1;
		u16 more_frag  : 1;
		u16 retry      : 1;
		u16 power_mgmt : 1;
		u16 more_data  : 1;
		u16 wep        : 1;
		u16 order      : 1;
	};
} WlanFrameCtrl;

//! IEEE 802.11 sequence control field
typedef union WlanSeqCtrl {
	u16 value;
	struct {
		u16 frag_num : 4;
		u16 seq_num  : 12;
	};
} WlanSeqCtrl;

//! IEEE 802.11 frame header
typedef struct WlanMacHdr {
	WlanFrameCtrl fc;
	u16 duration;
	u8  rx_addr[6];
	u8  tx_addr[6];
	u8  xtra_addr[6];
	WlanSeqCtrl sc;
	// WDS/Mesh routing has a 4th addr here (Data frame with ToDS=1 FromDS=1)
} WlanMacHdr;

//! IEEE 802.11 beacon header
typedef struct WlanBeaconHdr {
	u32 timestamp[2];
	u16 interval;
	u16 capabilities;
	// Information elements follow: WlanIeHdr[]
} WlanBeaconHdr;

//! IEEE 802.11 authentication header
typedef struct WlanAuthHdr {
	u16 algorithm_id;
	u16 sequence_num;
	u16 status;
	// Information elements follow: WlanIeHdr[]
} WlanAuthHdr;

//! IEEE 802.11 association request header
typedef struct WlanAssocReqHdr {
	u16 capabilities;
	u16 interval;
	// Information elements follow: WlanIeHdr[]
} WlanAssocReqHdr;

//! IEEE 802.11 association response header
typedef struct WlanAssocRespHdr {
	u16 capabilities;
	u16 status;
	u16 aid;
	// Information elements follow: WlanIeHdr[]
} WlanAssocRespHdr;

//! IEEE 802.11 information element header
typedef struct WlanIeHdr {
	u8 id;
	u8 len;
	u8 data[];
} WlanIeHdr;

//! IEEE 802.11 TIM information element
typedef struct WlanIeTim {
	u8 dtim_count;
	u8 dtim_period;
	u8 bitmap_ctrl;
	u8 bitmap[];
} WlanIeTim;

//! IEEE 802.11 CFP information element
typedef struct WlanIeCfp {
	u8 count;
	u8 period;
	Wlan16 max_duration;
	Wlan16 dur_remaining;
} WlanIeCfp;

//! IEEE 802.11 RSN information element
typedef struct WlanIeRsn {
	Wlan16 version;
	u8 group_cipher[4];
	Wlan16 num_pairwise_ciphers;
	u8 pairwise_ciphers[];
} WlanIeRsn;

//! IEEE 802.11 vendor-specific information element
typedef struct WlanIeVendor {
	Wlan24 oui;
	u8 type;
	u8 data[];
} WlanIeVendor;

//! Nintendo-specific information element
typedef struct WlanIeNin {
	Wlan16 active_zone;
	Wlan16 vtsf;

	Wlan16 magic;
	u8 version;
	u8 platform;
	Wlan32 game_id;
	Wlan16 stream_code;
	u8 data_sz;
	u8 attrib;
	Wlan16 cmd_sz;
	Wlan16 reply_sz;

	u8 data[];
} WlanIeNin;

//! Wireless network authentication types
typedef enum WlanBssAuthType {
	WlanBssAuthType_Open          = 0,
	WlanBssAuthType_WEP_40        = 1,
	WlanBssAuthType_WEP_104       = 2,
	WlanBssAuthType_WEP_128       = 3,
	WlanBssAuthType_WPA_PSK_TKIP  = 4,
	WlanBssAuthType_WPA2_PSK_TKIP = 5,
	WlanBssAuthType_WPA_PSK_AES   = 6,
	WlanBssAuthType_WPA2_PSK_AES  = 7,
} WlanBssAuthType;

//! Structure describing an IEEE 802.11 Basic Service Set (i.e. wireless network)
typedef struct WlanBssDesc {
	u8 bssid[6];                   //!< BSSID of the network
	u16 ssid_len;                  //!< Length of the SSID in bytes (or 0 if not provided)
	char ssid[WLAN_MAX_SSID_LEN];  //!< SSID of the network
	u16 ieee_caps;                 //!< Bitmask of IEEE 802.11 capabilities
	u16 ieee_basic_rates;          //!< Bitmask of basic supported data rates
	u16 ieee_all_rates;            //!< Bitmask of all supported data rates
	union {
		u8 auth_mask;              //!< Bitmask of supported authentication types (one bit for each member of @ref WlanBssAuthType)
		WlanBssAuthType auth_type; //!< Authentication type to use when connecting to this network (see @ref WlanBssAuthType)
	};
	u8 rssi;                       //!< RSSI of the network
	u8 channel;                    //!< 802.11 channel number where the network operates
} WlanBssDesc;

//! Extra information pertaining to a BSS
typedef struct WlanBssExtra {
	WlanIeTim* tim;
	WlanIeCfp* cfp;
	WlanIeNin* nin;
	u8 tim_bitmap_sz;
} WlanBssExtra;

//! BSS scan filtering parameters
typedef struct WlanBssScanFilter {
	u32 channel_mask;
	u8 target_bssid[6];
	u16 target_ssid_len;
	char target_ssid[WLAN_MAX_SSID_LEN];
} WlanBssScanFilter;

//! Wireless authentication data
typedef union WlanAuthData {
	u8 wpa_psk[WLAN_WPA_PSK_LEN];
	u8 wep_key[WLAN_WEP_128_LEN];
} WlanAuthData;

//! Returns the signal strength (0..3 bars) corresponding to the given @p rssi
MK_CONSTEXPR unsigned wlanCalcSignalStrength(unsigned rssi)
{
	if (rssi >= WLAN_RSSI_STRENGTH_3) return 3;
	if (rssi >= WLAN_RSSI_STRENGTH_2) return 2;
	if (rssi >= WLAN_RSSI_STRENGTH_1) return 1;
	return 0;
}

//! Converts @p freq_mhz to an IEEE 802.11 channel number
MK_CONSTEXPR unsigned wlanFreqToChannel(unsigned freq_mhz)
{
	if (freq_mhz == 2484) {
		return 14;
	} else if (freq_mhz < 2484) {
		return (freq_mhz - 2407) / 5;
	} else if (freq_mhz < 5000) {
		return 15 + ((freq_mhz - 2512) / 20);
	} else {
		return (freq_mhz - 5000) / 5;
	}
}

//! Converts an IEEE 802.11 channel number @p ch to a frequency in MHz
MK_CONSTEXPR unsigned wlanChannelToFreq(unsigned ch)
{
	if (ch == 14) {
		return 2484;
	} else if (ch < 14) {
		return 2407 + 5*ch;
	} else if (ch < 27) {
		return 2512 + 20*(ch-15);
	} else {
		return 5000 + 5*ch;
	}
}

//! Decodes the 16-bit integer at @p data
MK_INLINE unsigned wlanDecode16(const u8* data)
{
	return data[0] | (data[1]<<8);
}

//! Decodes the 24-bit integer at @p data
MK_INLINE unsigned wlanDecode24(const u8* data)
{
	return data[0] | (data[1]<<8) | (data[2]<<16);
}

//! Decodes the OUI (24-bit) at @p data
MK_INLINE unsigned wlanDecodeOui(const u8* data)
{
	return data[2] | (data[1]<<8) | (data[0]<<16);
}

//! Decodes the 32-bit integer at @p data
MK_INLINE unsigned wlanDecode32(const u8* data)
{
	return data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
}

//! Returns the associated bit for the given data @p rate (measured in half-megabit per second)
unsigned wlanGetRateBit(unsigned rate);

/*! @brief Finds a BSS within a BSS array, or adds a new entry if not found
	@param[inout] desc_table Pointer to @ref WlanBssDesc array (must have capacity for @ref WLAN_MAX_BSS_ENTRIES)
	@param[inout] num_entries Number of entries in the array
	@param[in] bssid BSSID of the network
	@param[in] rssi RSSI value at which the BSS was detected
	@return Pointer to entry within the array, or NULL if not found and the array is full
*/
WlanBssDesc* wlanFindOrAddBss(WlanBssDesc* desc_table, unsigned* num_entries, void* bssid, unsigned rssi);

/*! @brief Finds a WPA1 or WPA2 (RSN) information element within an information element array
	@param[in] rawdata Pointer to information element array to search
	@param[in] rawdata_len Size in bytes of the element array
	@return Pointer to information element if found, or NULL if not found
*/
WlanIeHdr* wlanFindRsnOrWpaIe(void* rawdata, unsigned rawdata_len);

/*! @brief Parses an IEEE 802.11 beacon frame
	@param[out] desc Output BSS description structure (see @ref WlanBssDesc)
	@param[out] extra Output structure for extra data (see @ref WlanBssExtra)
	@param[in] pPacket Network buffer containing the beacon frame
*/
void wlanParseBeacon(WlanBssDesc* desc, WlanBssExtra* extra, NetBuf* pPacket);

MK_EXTERN_C_END

//! @}
