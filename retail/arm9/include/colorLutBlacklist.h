#ifndef COLORLUTLBLACKLISTMAP_H
#define COLORLUTLBLACKLISTMAP_H

static const char colorLutBlacklist[][4] = {
	"TAM", // The Amazing Spider-Man
	"C66", // Chou Gekijouban Keroro Gunsou: Gekishin Dragon Warriors de Arimasu!
	"AQC", // Crayon Shin-chan DS: Arashi o Yobu Nutte Crayoon Daisakusen!
	"YRC", // Crayon Shin-chan: Arashi o Yobu Cinemaland Kachinko Gachinko Daikatsugeki!
	"CL4", // Crayon Shin-chan: Arashi o Yobu Nendororoon Daihenshin!
	"BQB", // Crayon Shin-chan: Obaka Dainin Den: Susume! Kasukabe Ninja Tai!
	"BUC", // Crayon Shin-chan: Shock Gahn!: Densetsu o Yobu Omake Daiketsusen!!
	"AWD", // Diddy Kong Racing DS
	"BVI", // Dokonjou Shougakusei Bon Biita: Hadaka no Choujou Ketsusen!!: Biita vs Dokuro Dei!
	"YD8", // Doraemon: Nobita to Midori no Kyojinden DS
	"BO5", // Golden Sun: Dark Dawn
	"Y8L", // Golden Sun: Dark Dawn (Demo Version)
	"AK4", // Kabu Trader Shun
	"BKS", // Keshikasu-kun: Battle Kasu-tival
	"AKW", // Kirby: Squeak Squad
	"Y2M", // Kirby: Squeak Squad (Demo)
	"B6Z", // MegaMan Zero Collection
	"ARZ", // MegaMan ZX
	"YZX", // MegaMan ZX Advent
	"ADA", // Pokemon: Diamond Version
	"APA", // Pokemon: Pearl Version
	"CPU", // Pokemon: Platinum Version
	"IPK", // Pokemon: HeartGold Version
	"IPG", // Pokemon: SoulSilver Version
	"IRB", // Pokemon: Black Version
	"IRA", // Pokemon: White Version
	"IRE", // Pokemon: Black Version 2
	"IRD", // Pokemon: White Version 2
	"B3N", // Power Rangers: Samurai
	"B8I", // Spider-Man: Edge of Time
	"CP3", // Viva Pinata: Pocket Paradise
};

/*

The Amazing Spider-Man:
- IPC Sync is not hooked on arm9

Chou Gekijouban Keroro Gunsou: Gekishin Dragon Warriors de Arimasu!,
Crayon Shin-chan DS: Arashi o Yobu Nutte Crayoon Daisakusen!,
Crayon Shin-chan: Arashi o Yobu Cinemaland Kachinko Gachinko Daikatsugeki!,
Crayon Shin-chan: Arashi o Yobu Nendororoon Daihenshin!,
Crayon Shin-chan: Obaka Dainin Den: Susume! Kasukabe Ninja Tai!,
Crayon Shin-chan: Shock Gahn!: Densetsu o Yobu Omake Daiketsusen!!:
- Either locks up on white screens or runs very slowly
- Flickers between original and custom colors

Diddy Kong Racing DS:
- IPC Sync is not hooked on arm9

Dokonjou Shougakusei Bon Biita: Hadaka no Choujou Ketsusen!!: Biita vs Dokuro Dei!,
Doraemon: Nobita to Midori no Kyojinden DS:
- Either locks up on white screens or runs very slowly
- Flickers between original and custom colors

Golden Sun: Dark Dawn:
- IPC Sync is not hooked on arm9

Kabu Trader Shun,
Keshikasu-kun: Battle Kasu-tival:
- Either locks up on white screens or runs very slowly
- Flickers between original and custom colors

Kirby: Squeak Squad:
- DMA functions overwrite the custom BG colors with the originals
- Only sprites have custom colors

MegaMan Zero Collection,
MegaMan ZX,
MegaMan ZX Advent:
- Either locks up on white screens or runs very slowly
- Flickers between original and custom colors

Pokemon: Diamond Version,
Pokemon: Pearl Version,
Pokemon: Platinum Version,
Pokemon: HeartGold Version,
Pokemon: SoulSilver Version
Pokemon: Black Version,
Pokemon: White Version,
Pokemon: Black Version 2,
Pokemon: White Version 2:
- Flickers between original and custom colors in some areas

Power Rangers: Samurai:
- Either locks up on white screens or runs very slowly
- Flickers between original and custom colors

Spider-Man: Edge of Time:
- IPC Sync is not hooked on arm9

Viva Pinata: Pocket Paradise:
- IPC Sync is not hooked on arm9

*/

#endif //  COLORLUTLBLACKLISTMAP_H
