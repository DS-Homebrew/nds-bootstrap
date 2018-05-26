#ifndef _DATABWLIST_H
#define _DATABWLIST_H

// ROM data include list.
// 1 = start of data address, 2 = end of data address, 3 = data size, 4 = DATAINCLUDE,
// 5 = GAME_CACHE_ADRESS_START, 6 = GAME_CACHE_SLOTS, 7 = GAME_READ_SIZE
u32 dataWhitelist_AZWJ0[7] = {0x000D9B98, 0x00F54000, 0x00E7A468, 0x00000001,	// Sawaru - Made in Wario (J)
							0x0D680000, 0x00000026, 0x00040000};
u32 dataWhitelist_AZWJ2[7] = {0x000D9BB8, 0x00F54400, 0x00E7A848, 0x00000001,	// Sawaru - Made in Wario (J) (v02)
							0x0D680000, 0x00000026, 0x00040000};
u32 dataWhitelist_AZWE0[7] = {0x0011E9F8, 0x00F9B800, 0x00E7CE08, 0x00000001,	// WarioWare: Touched (U)
							0x0D680000, 0x00000026, 0x00040000};
u32 dataWhitelist_AZWP0[7] = {0x00114C78, 0x010C7A00, 0x00FB2D88, 0x00000001,	// WarioWare: Touched (E)
							0x0D7C0000, 0x00000021, 0x00040000};
u32 dataWhitelist_AZWC0[7] = {0x000E74D8, 0x00FE1C00, 0x00EFA728, 0x00000001,	// Momo Waliou Zhizao (C)
							0x0D740000, 0x0000001F, 0x00040000};
u32 dataWhitelist_AZWK0[7] = {0x000D7378, 0x0112DE00, 0x00DA5A90, 0x00000001,	// Manjyeora! Made in Wario (KS)
							0x0D5C0000, 0x00000029, 0x00040000};
u32 dataWhitelist_ADME0[7] = {0x012E2BFC, 0x01D17A7C, 0x00A34E80, 0x00000001,	// Animal Crossing: Wild World (U)
							0x0D240000, 0x00000037, 0x00040000};
u32 dataWhitelist_ADME1[7] = {0x012E3D14, 0x01D179B4, 0x00A347E0, 0x00000001,	// Animal Crossing: Wild World (U) (v01)
							0x0D240000, 0x00000037, 0x00040000};
u32 dataWhitelist_A2SE0[7] = {0x00103A00, 0x00A17800, 0x00914400, 0x00000001,	// Dragon Ball Z: Supersonic Warriors 2 (U)
							0x0D180000, 0x00000013, 0x00080000};
u32 dataWhitelist_ADBP0[7] = {0x0010C400, 0x00A21000, 0x00914C00, 0x00000001,	// Dragon Ball Z: Supersonic Warriors 2 (E)
							0x0D180000, 0x00000013, 0x00080000};
u32 dataWhitelist_ABHE0[7] = {0x00300E00, 0x00D20E70, 0x00A20070, 0x00000001,	// Resident Evil: Deadly Silence (U)
							0x0D300000, 0x0000000D, 0x00100000};
u32 dataWhitelist_ARZJ0[7] = {0x0284F200, 0x0302DD6C, 0x007DEB6C, 0x00000001,	// Rockman ZX (J) (FMV cutscenes)
							0x0D000000, 0x00000040, 0x00040000};
u32 dataWhitelist_ARZE0[7] = {0x0238DC00, 0x02B94600, 0x00806A00, 0x00000001,	// MegaMan ZX (U) (FMV cutscenes)
							0x0D040000, 0x0000003F, 0x00040000};
u32 dataWhitelist_ARZP0[7] = {0x0287A400, 0x03058F6C, 0x007DEB6C, 0x00000001,	// MegaMan ZX (E) (FMV cutscenes)
							0x0D000000, 0x00000040, 0x00040000};
u32 dataWhitelist_AFFJ0[7] = {0x012E2000, 0x01AC4400, 0x007E2400, 0x00000001,	// Final Fantasy III (J)
							0x0D000000, 0x0000001F, 0x00080000};
u32 dataWhitelist_AFFE0[7] = {0x012E2600, 0x01AC4A00, 0x007E2400, 0x00000001,	// Final Fantasy III (U)
							0x0D000000, 0x0000001F, 0x00080000};
u32 dataWhitelist_AFFP0[7] = {0x01466600, 0x02282200, 0x00E1BC00, 0x00000001,	// Final Fantasy III (E)
							0x0D680000, 0x00000015, 0x00080000};
u32 dataWhitelist_ALKE0_0[7] = {0x06694E00, 0x073CF444, 0x00D3A644, 0x00000001,	// Lunar Knights (U) (FMV cutscenes)
							0x0D580000, 0x00000054, 0x00020000};
u32 dataWhitelist_ALKE0_1[3] = {0x063A9E00, 0x063CAAC0, 0x00020CC0};	// Lunar Knights (U) (Title call)
u32 dataWhitelist_ALKP0_0[7] = {0x06697400, 0x073D1A44, 0x00D3A644, 0x00000001,	// Lunar Knights (E) (FMV cutscenes)
							0x0D580000, 0x00000054, 0x00020000};
u32 dataWhitelist_ALKP0_1[3] = {0x063AC400, 0x063CD0C0, 0x00020CC0};	// Lunar Knights (E) (Title call)
//u32 dataWhitelist_AMQP0[7] = {0x0024EE00, 0x00F2E300, 0x00CDF500, 0x00000001,	// Mario Vs Donkey Kong 2: March of the Minis (E)
//							0x0D500000, 0x0000002C, 0x00040000};
//u32 dataWhitelist_ACBE0[7] = {0x018D6000, 0x02C15200, 0x0133F200, 0x00000001,	// Castlevania: Portrait of Ruin (U)
//							0x0DB40000, 0x00000013, 0x00040000};
//u32 dataWhitelist_ADAE0_0[7] = {0x01C35400, 0x01D61C00, 0x0012C800, 0x00000001,	// Pokemon Diamond & Pearl (U) (part 1)
//							0x0D780000, 0x00000022, 0x00040000};
//u32 dataWhitelist_ADAE0_1[3] = {0x00339200, 0x00CBB160, 0x00981F60};	// Pokemon Diamond & Pearl (U) (part 2)
//u32 dataWhitelist_ADAE0_2[3] = {0x02119C00, 0x0225E200, 0x00144600};	// Pokemon Diamond & Pearl (U) (part 3)
//u32 dataWhitelist_ADAE0_3[3] = {0x01DA6200, 0x0207E400, 0x002D8200};	// Pokemon Diamond & Pearl (U) (part 4)
u32 dataWhitelist_YZXJ0[7] = {0x0217DE00, 0x02D1E200, 0x00BA0400, 0x00000001,	// Rockman ZX Advent (J) (FMV cutscenes)
							0x0D3C0000, 0x00000031, 0x00040000};
u32 dataWhitelist_YZXE0[7] = {0x02221600, 0x02DC07A8, 0x00B9F1A8, 0x00000001,	// MegaMan ZX Advent (U) (FMV cutscenes)
							0x0D3C0000, 0x00000031, 0x00040000};
u32 dataWhitelist_YZXP0[7] = {0x02405A00, 0x02FA4BA8, 0x00B9F1A8, 0x00000001,	// MegaMan ZX Advent (E) (FMV cutscenes)
							0x0D3C0000, 0x00000031, 0x00040000};
u32 dataWhitelist_A5FE0[7] = {0x017FA000, 0x02D39400, 0x0153F400, 0x00000001,	// Professor Layton and the Curious Village (U)
							0x0DD40000, 0x0000000B, 0x00040000};
//u32 dataWhitelist_YF4E0[7] = {0x069A2600, 0x06FC3400, 0x0061FE00, 0x00000001,	// Final Fantasy IV (U)
//							0x0CE80000, 0x00000023, 0x00080000};
//u32 dataWhitelist_YF4P0[7] = {0x069A3400, 0x06EA4E00, 0x00501A00, 0x00000001,	// Final Fantasy IV (E)
//							0x0CD80000, 0x00000025, 0x00080000};
//u32 dataWhitelist_YQUE0[7] = {0x03F5AC00, 0x05066600, 0x0110BA00, 0x00000001,	// Chrono Trigger (U)
//							0x0D940000, 0x0000001C, 0x00040000};
u32 dataWhitelist_CCUJ0_0[7] = {0x00255C00, 0x010DF400, 0x00E89800, 0x00000001,	// Tomodachi Collection (J) (part 1)
							0x0DDE0000, 0x00000011, 0x00020000};
u32 dataWhitelist_CCUJ0_1[3] = {0x016D3E00, 0x01E1E000, 0x0074A200};	// Tomodachi Collection (J) (part 2)
u32 dataWhitelist_CCUJ1_0[7] = {0x00255E00, 0x010DF600, 0x00E89800, 0x00000001,	// Tomodachi Collection (J) (Rev 1) (part 1)
							0x0DDE0000, 0x00000011, 0x00020000};
u32 dataWhitelist_CCUJ1_1[3] = {0x016D4000, 0x01E1E200, 0x0074A200};	// Tomodachi Collection (J) (Rev 1) (part 2)

// ROM data exclude list.
// 1 = start of data address, 2 = end of data address, 3 = data size
u32 dataBlacklist_ARRE0[7] = {0x0096D400, 0x01AC4800, 0x01157400, 0x00000000,	// Ridge Racer DS (U)
							0x0D380000, 0x00000019, 0x00080000};
u32 dataBlacklist_AD2E0[7] = {0x00442200, 0x00A7DC00, 0x0063BA00, 0x00000000,	// Nintendogs: First 4 (U)
							0x0DD00000, 0x0000000C, 0x00040000};
u32 dataBlacklist_AGYE0[7] = {0x000F8E00, 0x030656A8, 0x02F6C8A8, 0x00000000,	// Phoenix Wright: Ace Attorney (U)
							0x0CE80000, 0x00000044, 0x00040000};
u32 dataBlacklist_AGYP0[7] = {0x000FAE00, 0x0323C598, 0x03141798, 0x00000000,	// Phoenix Wright: Ace Attorney (E)
							0x0D180000, 0x00000038, 0x00040000};
u32 dataBlacklist_ARME0[7] = {0x00803800, 0x0287A000, 0x02076800, 0x00000000,	// Mario & Luigi: Partners in Time (U)
							0x0DA40000, 0x00000017, 0x00040000};
u32 dataBlacklist_ARMP0[7] = {0x00842E00, 0x0372762C, 0x02EE482C, 0x00000000,	// Mario & Luigi: Partners in Time (E)
							0x0D000000, 0x00000040, 0x00040000};
u32 dataBlacklist_AB3J0[7] = {0x00F77200, 0x03442200, 0x024CB000, 0x00000000,	// Mario Basketball: 3 on 3 (J)
							0x0D900000, 0x0000003C, 0x00020000};
u32 dataBlacklist_AB3E0[7] = {0x00F76E00, 0x03222200, 0x022AB400, 0x00000000,	// Mario Hoops 3 on 3 (U)
							0x0D900000, 0x0000003C, 0x00020000};
u32 dataBlacklist_AB3P0[7] = {0x01940200, 0x03441E00, 0x01B01C00, 0x00000000,	// Mario Slam Basketball (E)
							0x0DF00000, 0x00000008, 0x00020000};
u32 dataBlacklist_APHJ0[7] = {0x0039FE00, 0x01465670, 0x010C5870, 0x00000000,	// Pokemon Fushigi no Dungeon: Ao no Kyuujotai (J)
							0x0D400000, 0x00000018, 0x00080000};
u32 dataBlacklist_APHJ1[7] = {0x003A0000, 0x01465870, 0x010C5870, 0x00000000,	// Pokemon Fushigi no Dungeon: Ao no Kyuujotai (J) (Rev 1)
							0x0D400000, 0x00000018, 0x00080000};
u32 dataBlacklist_APHE0[7] = {0x00399400, 0x0145EC70, 0x010C5870, 0x00000000,	// Pokemon Mystery Dungeon: Blue Rescue Team (U)
							0x0D400000, 0x00000018, 0x00080000};
u32 dataBlacklist_APHP0[7] = {0x0036AC00, 0x0134BC70, 0x00FE1070, 0x00000000,	// Pokemon Mystery Dungeon: Blue Rescue Team (E)
							0x0D600000, 0x00000014, 0x00080000};
u32 dataBlacklist_APHK0[7] = {0x00390400, 0x01455C70, 0x010C5870, 0x00000000,	// Pokemon Bulgasaui Dungeon: Parang Gujodae (KS)
							0x0D400000, 0x00000018, 0x00080000};
u32 dataBlacklist_ARGE0[7] = {0x003FCE00, 0x00ADAC00, 0x006DDE00, 0x00000000,	// Pokemon Ranger (U)
							0x0DA80000, 0x0000002C, 0x00020000};
//u32 dataBlacklist_AYWJ0[7] = {0x0163A600, 0x01DCA840, 0x00790240, 0x00000000,	// Yoshi's Island DS (J)
//							0x0DF00000, 0x00000008, 0x00020000};
//u32 dataBlacklist_AYWE0[7] = {0x01635E00, 0x01D9F240, 0x00769440, 0x00000000,	// Yoshi's Island DS (U)
//							0x0DEC0000, 0x0000000A, 0x00020000};
u32 dataBlacklist_ADNJ0[7] = {0x013EFA00, 0x02CEC200, 0x018FC800, 0x00000000,	// Digimon Story (J)
							0x0DEC0000, 0x00000005, 0x00040000};
u32 dataBlacklist_ADNE0[7] = {0x00F0EC00, 0x0320EA00, 0x022FFE00, 0x00000000,	// Digimon World DS (U)
							0x0DA00000, 0x00000018, 0x00040000};
u32 dataBlacklist_AKWJ0[7] = {0x00BE4400, 0x02821600, 0x01C3D200, 0x00000000,	// Hoshi no Kirby: Sanjou! Dorotche Dan (J)
							0x0DBA0000, 0x00000022, 0x00020000};
u32 dataBlacklist_AKWE0[7] = {0x00BEB000, 0x02819A00, 0x01C2EA00, 0x00000000,	// Kirby Squeak Squad (U)
							0x0DBA0000, 0x00000022, 0x00020000};
u32 dataBlacklist_AKWP0[7] = {0x00D61800, 0x0357D400, 0x0281BC00, 0x00000000,	// Kirby Mouse Attack (E)
							0x0DC00000, 0x0000001F, 0x00020000};
u32 dataBlacklist_A3YJ0[7] = {0x0100A400, 0x02961000, 0x01956C00, 0x00000000,	// Sonic Rush Adventure (J)
							0x0DF00000, 0x00000002, 0x00080000};
u32 dataBlacklist_A3YE0[7] = {0x0100A400, 0x02961B20, 0x01957720, 0x00000000,	// Sonic Rush Adventure (UE)
							0x0DF00000, 0x00000002, 0x00080000};
u32 dataBlacklist_A3YK0[7] = {0x01009400, 0x02908800, 0x018FF400, 0x00000000,	// Sonic Rush Adventure (KS)
							0x0DD80000, 0x00000005, 0x00080000};
u32 dataBlacklist_YCOE0[7] = {0x0075DA00, 0x03D36800, 0x035D8E00, 0x00000000,	// Call of Duty 4: Modern Warfare (U)
							0x0CF00000, 0x00000022, 0x00080000};
u32 dataBlacklist_YFYE0[7] = {0x00751400, 0x03196600, 0x02A45200, 0x00000000,	// Pokemon Mystery Dungeon: Explorers of Darkness and Time (U)
							0x0DB00000, 0x00000014, 0x00040000};
u32 dataBlacklist_YKWJ0[7] = {0x00B62200, 0x042A3A00, 0x03741800, 0x00000000,	// Hoshi no Kirby: Ultra Super Deluxe (J)
							0x0DA00000, 0x0000000E, 0x00080000};
u32 dataBlacklist_YKWE0[7] = {0x00B3A200, 0x04276000, 0x0373BE00, 0x00000000,	// Kirby Super Star Ultra (U)
							0x0DA00000, 0x0000000E, 0x00080000};
u32 dataBlacklist_YKWK0[7] = {0x00B39C00, 0x04273400, 0x03739800, 0x00000000,	// Kirby Ultra Super Deluxe (KS)
							0x0DA00000, 0x0000000E, 0x00080000};
u32 dataBlacklist_CJCE0[7] = {0x01102E00, 0x02E3EE00, 0x01D3C000, 0x00000000,	// My Japanese Coach: Learn a New Language (U)
							0x0DA00000, 0x00000030, 0x00040000};
u32 dataBlacklist_CRRE0[7] = {0x0039BA00, 0x005D1A00, 0x00236000, 0x00000000,	// Megaman Star Force 3: Red Joker (U)
							0x0DE80000, 0x0000000C, 0x00020000};
u32 dataBlacklist_CRBE0[7] = {0x0039AC00, 0x005D0C00, 0x00236000, 0x00000000,	// Megaman Star Force 3: Black Ace (U)
							0x0DE80000, 0x0000000C, 0x00020000};
u32 dataBlacklist_CLJE0[7] = {0x01022600, 0x05944A00, 0x04922400, 0x00000000,	// Mario & Luigi: Bowser's Inside Story (U)
							0x0D800000, 0x00000020, 0x00040000};
u32 dataBlacklist_BKIE0[7] = {0x00A2E200, 0x0523F600, 0x04811400, 0x00000000,	// The Legend of Zelda: Spirit Tracks (U)
							0x0D980000, 0x0000001A, 0x00040000};
u32 dataBlacklist_BKIE0_patch[7] = {0x01447600, 0x057F7E00, 0x043B0800, 0x00000000,	// The Legend of Zelda: Spirit Tracks (U) (XPA's AP-patch)
							0x0DE00000, 0x00000008, 0x00040000};
u32 dataBlacklist_B6ZE0[7] = {0x013CE000, 0x03AB6E00, 0x026E8E00, 0x00000000,	// MegaMan Zero Collection (U)
							0x0DF60000, 0x00000014, 0x00008000};

#endif // _DATABWLIST_H
