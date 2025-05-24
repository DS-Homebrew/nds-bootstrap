#ifndef COLORLUTLBLACKLISTMAP_H
#define COLORLUTLBLACKLISTMAP_H

static const char colorLutBlacklist[][4] = {
	"TAM", // The Amazing Spider-Man
	"YCO", // Call of Duty 4: Modern Warfare
	"CAL", // Call of Duty: World at War
	"C62", // Call of Duty: Modern Warfare: Mobilized
	"BDY", // Call of Duty: Black Ops
	"B5B", // Call of Duty: Modern Warfare 3: Defiance
	"C66", // Chou Gekijouban Keroro Gunsou: Gekishin Dragon Warriors de Arimasu!
	"AQC", // Crayon Shin-chan DS: Arashi o Yobu Nutte Crayoon Daisakusen!
	"YRC", // Crayon Shin-chan: Arashi o Yobu Cinemaland Kachinko Gachinko Daikatsugeki!
	"CL4", // Crayon Shin-chan: Arashi o Yobu Nendororoon Daihenshin!
	"BQB", // Crayon Shin-chan: Obaka Dainin Den: Susume! Kasukabe Ninja Tai!
	"BUC", // Crayon Shin-chan: Shock Gahn!: Densetsu o Yobu Omake Daiketsusen!!
	"AWD", // Diddy Kong Racing DS
	"BVI", // Dokonjou Shougakusei Bon Biita: Hadaka no Choujou Ketsusen!!: Biita vs Dokuro Dei!
	"YD8", // Doraemon: Nobita to Midori no Kyojinden DS
	"ATI", // Electroplankton
	"KEI", // Electroplankton: Beatnes
	"KEB", // Electroplankton: Hanenbow
	"KEG", // Electroplankton: Lumiloop
	"KEC", // Electroplankton: Luminarrow
	"KEH", // Electroplankton: Marine-Crystals
	"KEF", // Electroplankton: Nanocarp
	"KEE", // Electroplankton: Rec-Rec
	"KED", // Electroplankton: Sun-Animalcule
	"KEA", // Electroplankton: Trapy
	"KEJ", // Electroplankton: Varvoice
	"KGU", // Flipnote Studio
	"AGE", // GoldenEye: Rogue Agent
	"BJC", // GoldenEye 007
	"BO5", // Golden Sun: Dark Dawn
	"Y8L", // Golden Sun: Dark Dawn (Demo Version)
	"CI7", // Hannah Montana: The Movie
	"BJB", // James Bond 007: Blood Stone
	"AK4", // Kabu Trader Shun
	"BKS", // Keshikasu-kun: Battle Kasu-tival
	"ARM", // Mario & Luigi: Partners in Time
	"CLJ", // Mario & Luigi: Bowser's Inside Story
	"C4M", // Marvel Ultimate Alliance 2
	"B6Z", // MegaMan Zero Collection
	"ARZ", // MegaMan ZX
	"YZX", // MegaMan ZX Advent
	"AMH", // Metroid Prime Hunters
	"B3N", // Power Rangers: Samurai
	"B8I", // Spider-Man: Edge of Time
	"CSW", // Star Wars: Battlefont: Elite Squadron
	"YST", // Star Wars: The Force Unleashed
	"AZL", // Style Savvy
	"VT3", // Toy Story 3
	"VTE", // Tron: Evolution
	"CP3", // Viva Pinata: Pocket Paradise
	"CY8", // Yu-Gi-Oh! 5D's: Stardust Accelerator: World Championship 2009
	"BYX", // Yu-Gi-Oh! 5D's: World Championship 2010: Reverse of Arcadia
	"BYY", // Yu-Gi-Oh! 5D's: World Championship 2011: Over The Nexus
};

/* Blacklist reasons

The Amazing Spider-Man:
- IRQ is not hooked on arm9

Call of Duty 4: Modern Warfare,
Call of Duty: World at War,
Call of Duty: Modern Warfare: Mobilized,
Call of Duty: Black Ops,
Call of Duty: Modern Warfare 3: Defiance:
- Crashes on black screens

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

Electroplankton titles:
- Color blending effects used everywhere(?)

Flipnote Studio:
- No effect due to bitmap mode being used
- Randomly swaps the top and bottom screens for a frame

GoldenEye: Rogue Agent,
GoldenEye 007:
- Crashes on black screens

Golden Sun: Dark Dawn:
- IRQ is not hooked on arm9

Hannah Montana: The Movie,
James Bond 007: Blood Stone:
- Crashes on black screens

Kabu Trader Shun,
Keshikasu-kun: Battle Kasu-tival:
- Runs very slowly
- Flickers between original and custom colors in some areas

Mario & Luigi: Partners in Time:
- Crashes with data abort after pressing START

Mario & Luigi: Bowser's Inside Story:
- Crashes on white screens after Nintendo/AlphaDream screen

Marvel Ultimate Alliance 2:
- Crashes on black screens

MegaMan Zero Collection,
MegaMan ZX,
MegaMan ZX Advent:
- Runs very slowly
- Flickers between original and custom colors in some areas

Metroid Prime Hunters:
- Color blending effects used for many textures and in title screen
- Black lines appearing in title screen

Power Rangers: Samurai:
- Runs very slowly
- Flickers between original and custom colors in some areas

Spider-Man: Edge of Time,
Viva Pinata: Pocket Paradise:
- IRQ is not hooked on arm9

Star Wars: Battlefont: Elite Squadron,
Star Wars: The Force Unleashed:
- Crashes on black screens

Style Savvy:
- Character skin and hair use color blending
- Crashes when the master brightness register gets changed, and would occur when using a specific LUT which has inverted black/white or a non-white white

Toy Story 3,
Tron: Evolution:
- Crashes on black screens

Yu-Gi-Oh! 5D's: Stardust Accelerator: World Championship 2009,
Yu-Gi-Oh! 5D's: World Championship 2010: Reverse of Arcadia,
Yu-Gi-Oh! 5D's: World Championship 2011: Over The Nexus:
- Runs very slowly in some areas

*/

static const char colorLutVCountBlacklist[][4] = {
	"K2J", // Cake Ninja
	"K2N", // Cake Ninja 2
	"KYN", // Cake Ninja: XMAS
	"KCQ", // Crazy Cheebo: Puzzle Party
	"KVC", // Curling Super Championship
	"K5L", // Forgotten Legions
	"ATK", // Kirby: Canvas Curse
	"KYL", // Make Up & Style
	"AB3", // Mario Hoops 3-on-3
	"AGF", // True Swing Golf
	"K72", // True Swing Golf Express
};

/* VCount IRQ will not be hooked to the color LUT code for these titles, in order to work around these issues...

Cake Ninja,
Cake Ninja 2,
Cake Ninja: XMAS:
- Crashes with black top screen after logos

Crazy Cheebo: Puzzle Party:
- Crashes after intro finishes playing or selecting a mode

Curling Super Championship:
- Crashes after finishing or skipping the tutorial

Forgotten Legions:
- Crashes after the bottom screen fades out during the opening video

Kirby: Canvas Curse:
- Original colors still seen at the top of each screen

Make Up & Style:
- Crashes after selecting a mode

Mario Hoops 3-on-3:
- Crashes on white screens

True Swing Golf,
True Swing Golf Express:
- Top screen text is glitched
- May flicker between original and custom colors at some point

*/

#endif //  COLORLUTLBLACKLISTMAP_H
