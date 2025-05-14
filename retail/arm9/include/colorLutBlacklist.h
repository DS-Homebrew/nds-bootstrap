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
	"B6Z", // MegaMan Zero Collection
	"ARZ", // MegaMan ZX
	"YZX", // MegaMan ZX Advent
	"B3N", // Power Rangers: Samurai
	"B8I", // Spider-Man: Edge of Time
	"CP3", // Viva Pinata: Pocket Paradise
	"CY8", // Yu-Gi-Oh! 5D's: Stardust Accelerator: World Championship 2009
	"BYX", // Yu-Gi-Oh! 5D's: World Championship 2010: Reverse of Arcadia
	"BYY", // Yu-Gi-Oh! 5D's: World Championship 2011: Over The Nexus
};

/* Blacklist reasons

The Amazing Spider-Man:
- IRQ is not hooked on arm9

Chou Gekijouban Keroro Gunsou: Gekishin Dragon Warriors de Arimasu!,
Crayon Shin-chan DS: Arashi o Yobu Nutte Crayoon Daisakusen!,
Crayon Shin-chan: Arashi o Yobu Cinemaland Kachinko Gachinko Daikatsugeki!,
Crayon Shin-chan: Arashi o Yobu Nendororoon Daihenshin!,
Crayon Shin-chan: Obaka Dainin Den: Susume! Kasukabe Ninja Tai!,
Crayon Shin-chan: Shock Gahn!: Densetsu o Yobu Omake Daiketsusen!!:
- Runs very slowly
- Flickers between original and custom colors in some areas

Diddy Kong Racing DS:
- IRQ is not hooked on arm9

Dokonjou Shougakusei Bon Biita: Hadaka no Choujou Ketsusen!!: Biita vs Dokuro Dei!,
Doraemon: Nobita to Midori no Kyojinden DS:
- Runs very slowly
- Flickers between original and custom colors in some areas

Golden Sun: Dark Dawn:
- IRQ is not hooked on arm9

Kabu Trader Shun,
Keshikasu-kun: Battle Kasu-tival:
- Runs very slowly
- Flickers between original and custom colors in some areas

MegaMan Zero Collection,
MegaMan ZX,
MegaMan ZX Advent:
- Runs very slowly
- Flickers between original and custom colors in some areas

Power Rangers: Samurai:
- Runs very slowly
- Flickers between original and custom colors in some areas

Spider-Man: Edge of Time,
Viva Pinata: Pocket Paradise:
- IRQ is not hooked on arm9

Yu-Gi-Oh! 5D's: Stardust Accelerator: World Championship 2009,
Yu-Gi-Oh! 5D's: World Championship 2010: Reverse of Arcadia,
Yu-Gi-Oh! 5D's: World Championship 2011: Over The Nexus:
- Runs very slowly in some areas

*/

#endif //  COLORLUTLBLACKLISTMAP_H
