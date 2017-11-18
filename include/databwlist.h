#ifndef _DATABWLIST_H
#define _DATABWLIST_H

// ROM data include list.
// 1 = start of data address, 2 = end of data address, 3 = data size
u32 dataWhitelist_AZWE0[3] = {0x0011E9F8, 0x00F9B800, 0x00E7CE08};	// WarioWare: Touched (U)
u32 dataWhitelist_AZWP0[3] = {0x00114C78, 0x010C7A00, 0x00FB2D88};	// WarioWare: Touched (E)
u32 dataWhitelist_ADME0[3] = {0x012E2BFC, 0x01D17A7C, 0x00A34E80};	// Animal Crossing: Wild World (U)
u32 dataWhitelist_ADME1[3] = {0x012E3D14, 0x01D179B4, 0x00A347E0};	// Animal Crossing: Wild World (U) (v01)
u32 dataWhitelist_ABHE0[3] = {0x00300E00, 0x00D20E70, 0x00A20070};	// Resident Evil: Deadly Silence (U)
u32 dataWhitelist_ARZE0[3] = {0x0238DC00, 0x02B94600, 0x00806A00};	// MegaMan ZX (U)
u32 dataWhitelist_ARZP0[3] = {0x0287A400, 0x03058F6C, 0x007DEB6C};	// MegaMan ZX (E)
u32 dataWhitelist_ALKE0[3] = {0x06694E00, 0x073CF444, 0x00D3A644};	// Lunar Knights (U)
u32 dataWhitelist_ALKP0[3] = {0x06697400, 0x073D1A44, 0x00D3A644};	// Lunar Knights (E)
u32 dataWhitelist_ADAE0[3] = {0x00339200, 0x00CBB160, 0x00981F60};	// Pokemon Diamond & Pearl (U)
u32 dataWhitelist_YZXE0[3] = {0x02221600, 0x02DC07A8, 0x00B9F1A8};	// MegaMan ZX Advent (U)
u32 dataWhitelist_YZXP0[3] = {0x02405A00, 0x02FA4BA8, 0x00B9F1A8};	// MegaMan ZX Advent (E)
u32 dataWhitelist_YKWE0[3] = {0x01632600, 0x024D7E00, 0x00EA5800};	// Kirby Super Star Ultra (U)

// ROM data exclude list.
// 1 = start of data address, 2 = end of data address, 3 = data size
u32 dataBlacklist_ACVE0[3] = {0x01AC1600, 0x02F6DA0C, 0x014AC40C};	// Castlevania: Dawn of Sorrow (U)
u32 dataBlacklist_AGYE0[3] = {0x000F8E00, 0x030656A8, 0x02F6C8A8};	// Phoenix Wright: Ace Attorney (U)
u32 dataBlacklist_ASCE0[3] = {0x0171E600, 0x03CA8DE0, 0x0258A7E0};	// Sonic Rush (U)
u32 dataBlacklist_ARME0[3] = {0x00D42600, 0x02879FA8, 0x01B379A8};	// Mario & Luigi: Partners in Time (U)
u32 dataBlacklist_ARMP0[3] = {0x00842E00, 0x0372762C, 0x02EE482C};	// Mario & Luigi: Partners in Time (E)
u32 dataBlacklist_APHE0[3] = {0x00399400, 0x0145EC70, 0x010C5870};	// Pokemon Mystery Dungeon: Blue Rescue Team (U)
u32 dataBlacklist_AYWE0[3] = {0x01635E00, 0x01D9F240, 0x00769440};	// Yoshi's Island DS (U)
u32 dataBlacklist_AKWE0[3] = {0x00BEB000, 0x02819A00, 0x01C2EA00};	// Kirby Squeak Squad (U)
u32 dataBlacklist_AKWP0[3] = {0x00D61800, 0x0357D400, 0x0281BC00};	// Kirby Mouse Attack (E)
u32 dataBlacklist_A3YE0[3] = {0x0100A400, 0x02961B20, 0x01957720};	// Sonic Rush Adventure (U)

#endif // _DATABWLIST_H
