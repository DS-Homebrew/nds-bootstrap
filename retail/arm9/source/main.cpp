#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdarg.h>
#include <limits.h> // PATH_MAX
#include <unistd.h>
#include <sys/stat.h>
#include <nds.h>
/*#include <nds/ndstypes.h>
#include <nds/arm9/input.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>
#include <nds/system.h>
#include <nds/debug.h>*/

#include "myDSiMode.h"
#include "configuration.h"
#include "nds_loader_arm9.h"
#include "conf_sd.h"
#include "version.h"

#define REG_SCFG_EXT7 *(u32*)0x02FFFDF0

u8* lz77ImageBuffer = (u8*)0x02004000;

char patchOffsetCacheFilePath[64];
std::string wideCheatFilePath;
std::string cheatFilePath;
std::string ramDumpPath;
std::string srParamsFilePath;
std::string screenshotPath;
std::string apFixOverlaysPath;
std::string musicsFilePath;
std::string pageFilePath;
std::string sharedFontPath;

typedef struct {
	char gameTitle[12];			//!< 12 characters for the game title.
	char gameCode[4];			//!< 4 characters for the game code.
} sNDSHeaderTitleCodeOnly;

//extern bool logging;
//bool logging = false;

static bool debug = false;
static bool sdFound = false;
static bool bootstrapOnFlashcard = false;
bool colorTable = false;

void myConsoleDemoInit(void) {
	static bool inited = false;
	if (inited) return;

	consoleDemoInit();
	if (colorTable) {
		for (int i = 0; i < 256; i++) {
			BG_PALETTE_SUB[i] = VRAM_E[BG_PALETTE_SUB[i] % 0x8000];
		}
	}

	inited = true;
}

static inline const char* btoa(bool x) {
	return x ? "true" : "false";
}

static int dbg_printf(const char* format, ...) { // static int...
	if (!debug) {
		return 0;
	}

	static FILE* debugFile;
	debugFile = fopen(bootstrapOnFlashcard ? "fat:/NDSBTSRP.LOG" : "sd:/NDSBTSRP.LOG", "a");

	va_list args;
	va_start(args, format);
	int ret = vprintf(format, args);
	ret = vfprintf(debugFile, format, args);
	va_end(args);

	fclose(debugFile);

	return ret;
}

static void stop(void) {
	while (1) {
		swiWaitForVBlank();
	}
}

static void dopause(void) {
	iprintf("Press start...\n");
	while(1) {
		scanKeys();
		if (keysDown() & KEY_START)
			break;
		swiWaitForVBlank();
	}
	scanKeys();
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

bool extension(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

static void getSFCG_ARM9(void) {
	iprintf("SCFG_ROM ARM9 %X\n", REG_SCFG_ROM); 
	iprintf("SCFG_CLK ARM9 %X\n", REG_SCFG_CLK); 
	//iprintf("SCFG_EXT ARM9 %X\n", REG_SCFG_EXT); 
}

static void getSFCG_ARM7(void) {
	//iprintf("SCFG_ROM ARM7\n");

	//nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_ROM);\n");
	//fifoSendValue32(FIFO_USER_01, (u32)&REG_SCFG_ROM);

	//nocashMessage("dbg_printf\n");

	iprintf("SCFG_CLK ARM7\n");

	nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_CLK);\n");
	fifoSendValue32(FIFO_USER_01, (u32)&REG_SCFG_CLK);

	iprintf("SCFG_EXT ARM7\n");

	nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_EXT);\n");
	fifoSendValue32(FIFO_USER_01, (u32)&REG_SCFG_EXT);
}

static void myFIFOValue32Handler(u32 value, void* userdata) {
	nocashMessage("myFIFOValue32Handler\n");
	iprintf("ARM7 data %lX\n", value);
}

static inline void debugConfB4DS(configuration* conf) {
	dbg_printf("debug: %s\n", btoa(conf->debug));
	dbg_printf("ndsPath: \"%s\"\n", conf->ndsPath);
	dbg_printf("savPath: \"%s\"\n", conf->savPath);
	if (debug) {
		dopause();
	}
	dbg_printf("donor20Path: \"%s\"\n", conf->donor20Path);
	dbg_printf("donor5Path: \"%s\"\n", conf->donor5Path);
	dbg_printf("donorTwlPath: \"%s\"\n", conf->donorTwlPath);
	if (debug) {
		dopause();
	}
	dbg_printf("saveSize: %lX\n", conf->saveSize);
	dbg_printf("language: %hhX\n", conf->language);
	dbg_printf("region: %hhX\n", conf->region);
	dbg_printf("donorSdkVer: %lX\n", conf->donorSdkVer);
	dbg_printf("patchMpuRegion: %lX\n", conf->patchMpuRegion);
	dbg_printf("patchMpuSize: %lX\n", conf->patchMpuSize);
	dbg_printf("apPatchPath: %s\n", conf->apPatchPath);
	if (dsiFeatures()) {
		dbg_printf("boostCpu: %s\n", btoa(conf->boostCpu));
		dbg_printf("boostVram: %s\n", btoa(conf->boostVram));
	}
	dbg_printf("forceSleepPatch: %s\n", btoa(conf->forceSleepPatch));
	dbg_printf("logging: %s\n", btoa(conf->logging));
	dbg_printf("initDisc: %s\n", btoa(conf->initDisc));
	dbg_printf("macroMode: %s\n", btoa(conf->macroMode));
}

static inline void debugConf(configuration* conf) {
	dbg_printf("debug: %s\n", btoa(conf->debug));
	dbg_printf("ndsPath: \"%s\"\n", conf->ndsPath);
	dbg_printf("savPath: \"%s\"\n", conf->savPath);
	if (debug) {
		dopause();
	}
	dbg_printf("prvPath: \"%s\"\n", conf->prvPath);
	//dbg_printf("gbaPath: \"%s\"\n", conf->gbaPath);
	if (isDSiMode() && REG_SCFG_EXT7 == 0) {
		if (*(u32*)0x02FFE1A0 == 0x00403000) {
			dbg_printf("donorTwlPath: \"%s\"\n", conf->donorTwlPath);
		} else {
			dbg_printf("donorTwlOnlyPath: \"%s\"\n", conf->donorTwlOnlyPath);
		}
	}
	if (debug) {
		dopause();
	}
	dbg_printf("saveSize: %lX\n", conf->saveSize);
	dbg_printf("language: %hhX\n", conf->language);
	if (dsiFeatures()) {
		dbg_printf("region: %hhX\n", conf->region);
		dbg_printf("dsiMode: %i\n", conf->dsiMode);
	}
	dbg_printf("donorSdkVer: %lX\n", conf->donorSdkVer);
	dbg_printf("patchMpuRegion: %lX\n", conf->patchMpuRegion);
	dbg_printf("patchMpuSize: %lX\n", conf->patchMpuSize);
	dbg_printf("consoleModel: %lX\n", conf->consoleModel);
	//dbg_printf("colorMode: %lX\n", conf->colorMode);
	dbg_printf("romRead_LED: %lX\n", conf->romRead_LED);
	dbg_printf("dmaRomRead_LED: %lX\n", conf->dmaRomRead_LED);
	dbg_printf("apPatchPath: \"%s\"\n", conf->apPatchPath);
	dbg_printf("asyncCardRead: %s\n", btoa(conf->asyncCardRead));
	dbg_printf("cardReadDMA: %i\n", conf->cardReadDMA);
	dbg_printf("boostCpu: %s\n", btoa(conf->boostCpu));
	dbg_printf("boostVram: %s\n", btoa(conf->boostVram));
	dbg_printf("soundFreq: %s\n", btoa(conf->soundFreq));
	dbg_printf("forceSleepPatch: %s\n", btoa(conf->forceSleepPatch));
	dbg_printf("logging: %s\n", btoa(conf->logging));
	dbg_printf("initDisc: %s\n", btoa(conf->initDisc));
	dbg_printf("gameOnFlashcard: %s\n", btoa(conf->gameOnFlashcard));
	dbg_printf("saveOnFlashcard: %s\n", btoa(conf->saveOnFlashcard));
	dbg_printf("macroMode: %s\n", btoa(conf->macroMode));
}

static int runNdsFile(configuration* conf) {
	// Debug
	debug = conf->debug;
	if (debug) {
		myConsoleDemoInit();

		if (dsiFeatures()) {
			fifoSetValue32Handler(FIFO_USER_02, myFIFOValue32Handler, NULL);

			getSFCG_ARM9();
			getSFCG_ARM7();

			/*for (int i = 0; i < 60; i++) {
				swiWaitForVBlank();
			}*/
		}
	}

	if (dsiFeatures()) {
		// ROM read LED
		switch(conf->romRead_LED) {
			case 0:
			default:
				break;
			case 1:
				dbg_printf("Using WiFi LED\n");
				break;
			case 2:
				dbg_printf("Using Power LED\n");
				break;
			case 3:
				dbg_printf("Using Camera LED\n");
				break;
		}

		// DMA ROM read LED
		switch(conf->dmaRomRead_LED) {
			case 0:
			default:
				break;
			case 1:
				dbg_printf("DMA: Using WiFi LED\n");
				break;
			case 2:
				dbg_printf("DMA: Using Power LED\n");
				break;
			case 3:
				dbg_printf("DMA: Using Camera LED\n");
				break;
		}

		// adjust TSC[1:26h] and TSC[1:27h]
		// for certain gamecodes
		FILE* f_nds_file = fopen(conf->ndsPath, "rb");

		char romTid[5];
		fseek(f_nds_file, offsetof(sNDSHeaderTitleCodeOnly, gameCode), SEEK_SET);
		fread(romTid, 1, 4, f_nds_file);
		fclose(f_nds_file);

		static const char list[][4] = {
			"ABX",	// NTR-ABXE Bomberman Land Touch!
			"YO9",	// NTR-YO9J Bokura no TV Game Kentei - Pikotto! Udedameshi
			"ALH",	// NTR-ALHE Flushed Away
			"ACC",	// NTR-ACCE Cooking Mama
			"YCQ",	// NTR-YCQE Cooking Mama 2 - Dinner with Friends
			"YYK",	// NTR-YYKE Trauma Center - Under the Knife 2
			"AZW",	// NTR-AZWE WarioWare - Touched!
			"AKA",	// NTR-AKAE Rub Rabbits!, The
			"AN9",	// NTR-AN9E Little Mermaid - Ariel's Undersea Adventure, The
			"AKE",	// NTR-AKEJ Keroro Gunsou - Enshuu da Yo! Zenin Shuugou Part 2
			"YFS",	// NTR-YFSJ Frogman Show - DS Datte, Shouganaijanai, The
			"YG8",	// NTR-YG8E Yu-Gi-Oh! World Championship 2008
			"AY7",	// NTR-AY7E Yu-Gi-Oh! World Championship 2007
			"YON",	// NTR-YONJ Minna no DS Seminar - Kantan Ongakuryoku
			"A5H",	// NTR-A5HE Interactive Storybook DS - Series 2
			"A5I",	// NTR-A5IE Interactive Storybook DS - Series 3
			"AMH",	// NTR-AMHE Metroid Prime Hunters
			"A3T",	// NTR-A3TE Tak - The Great Juju Challenge
			"YBO",	// NTR-YBOE Boogie
			"ADA",	// NTR-ADAE PKMN Diamond
			"APA",	// NTR-APAE PKMN Pearl
			"CPU",	// NTR-CPUE PKMN Platinum
			"APY",	// NTR-APYE Puyo Pop Fever
			"AWH",	// NTR-AWHE Bubble Bobble Double Shot
			"AXB",	// NTR-AXBJ Daigassou! Band Brothers DX
			"A4U",	// NTR-A4UJ Wi-Fi Taiou - Morita Shogi
			"A8N",	// NTR-A8NE Planet Puzzle League
			"ABJ",	// NTR-ABJE Harvest Moon DS - Island of Happiness
			"ABN",	// NTR-ABNE Bomberman Story DS
			"ACL",	// NTR-ACLE Custom Robo Arena
			"ART",	// NTR-ARTJ Shin Lucky Star Moe Drill - Tabidachi
			"AVT",	// NTR-AVTJ Kou Rate Ura Mahjong Retsuden Mukoubuchi - Goburei, Shuuryou desu ne
			"AWY",	// NTR-AWYJ Wi-Fi Taiou - Gensen Table Game DS
			"AXJ",	// NTR-AXJE Dungeon Explorer - Warriors of Ancient Arts
			"AYK",	// NTR-AYKJ Wi-Fi Taiou - Yakuman DS
			"YB2",	// NTR-YB2E Bomberman Land Touch! 2
			"YB3",	// NTR-YB3E Harvest Moon DS - Sunshine Islands
			"YCH",	// NTR-YCHJ Kousoku Card Battle - Card Hero
			"YFE",	// NTR-YFEE Fire Emblem - Shadow Dragon
			"YGD",	// NTR-YGDE Diary Girl
			"YKR",	// NTR-YKRJ Culdcept DS
			"YRM",	// NTR-YRME My Secret World by Imagine
			"YW2",	// NTR-YW2E Advance Wars - Days of Ruin
			"AJU",	// NTR-AJUJ Jump! Ultimate Stars
			"ACZ",	// NTR-ACZE Cars
			"AHD",	// NTR-AHDE Jam Sessions
			"ANR",	// NTR-ANRE Naruto - Saikyou Ninja Daikesshu 3
			"YT3",	// NTR-YT3E Tamagotchi Connection - Corner Shop 3
			"AVI",	// NTR-AVIJ Kodomo no Tame no Yomi Kikase - Ehon de Asobou 1-Kan
			"AV2",	// NTR-AV2J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 2-Kan
			"AV3",	// NTR-AV3J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 3-Kan
			"AV4",	// NTR-AV4J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 4-Kan
			"AV5",	// NTR-AV5J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 5-Kan
			"AV6",	// NTR-AV6J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 6-Kan
			"YNZ",	// NTR-YNZE Petz - Dogz Fashion
		};

		for (unsigned int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
			if (memcmp(romTid, list[i], 3) == 0) {
				// Found a match.
				conf->volumeFix = true; // Special setting (when found special gamecode)
				conf->valueBits |= BIT(3);
				break;
			}
		}

		// Boost CPU
		if (conf->boostCpu) {
			dbg_printf("CPU boosted\n");
			setCpuClock(true); // libnds sets TWL clock speeds on arm7/arm9 scfg_clk at boot now. No changes needed.
		} else {
			setCpuClock(false); //REG_SCFG_CLK = 0x80;
			fifoSendValue32(FIFO_USER_06, 1);
		}

		// Boost VRAM
		if (conf->boostVram) {
			dbg_printf("VRAM boosted\n");
		}
	}

	fifoSendValue32(FIFO_USER_03, 1);
	fifoWaitValue32(FIFO_USER_05);

	// Logging
	const char *logFilePath = (conf->sdFound && !conf->b4dsMode ? "sd:/NDSBTSRP.LOG" : "fat:/NDSBTSRP.LOG");
	if (conf->logging) {
		static FILE* loggingFile = fopen(logFilePath, "w");
		fprintf(loggingFile, "LOGGING MODE\n");
		fclose(loggingFile);

		// Create a big file (minimal sdengine libfat cannot append to a file)
		loggingFile = fopen(logFilePath, "a");
		for (int i = 0; i < 1000; i++) {
			fprintf(loggingFile, "                                                                                                                                          \n");
		}

		fclose(loggingFile);
	} else {
		remove(logFilePath);
	}

	dbg_printf("version: " VER_NUMBER "\n");
	(dsiFeatures() && !conf->b4dsMode) ? debugConf(conf) : debugConfB4DS(conf);

	if ((!extension(conf->ndsPath, ".nds"))
	&& (!extension(conf->ndsPath, ".dsi"))
	&& (!extension(conf->ndsPath, ".ids"))
	&& (!extension(conf->ndsPath, ".srl"))
	&& (!extension(conf->ndsPath, ".app"))) {
		if (debug) {
			dbg_printf("No NDS file specified\n");
			dopause();
		} else {
			myConsoleDemoInit();
			iprintf("No NDS file specified\n");
		}
		return -1;
	}

	//dbg_printf("Running \"%s\" with %d parameters\n", conf->ndsPath, conf->argc);
	if (debug) {
		dopause();
	}

	if (conf->macroMode) {
		powerOff(PM_BACKLIGHT_TOP);
	}

	struct stat st;
	struct stat stSav;
	struct stat stDonorStandalone;
	struct stat stDonor;
	struct stat stDonor0;
	struct stat stDonor5;
	struct stat stDonor5Alt;
	// struct stat stGba;
	// struct stat stGbaSav;
	struct stat stWideCheat;
	struct stat stApPatch;
	struct stat stDSi2DSSavePatch;
	struct stat stCheat;
	struct stat stPatchOffsetCache;
	struct stat stRamDump;
	struct stat stSrParams;
	struct stat stScreenshot;
	struct stat stApFixOverlays;
	struct stat stMusic;
	struct stat stPage;
	struct stat stManual;
	struct stat stTwlFont;
	u32 clusterSav = 0;
	u32 clusterDonor = 0;
	// u32 clusterGba = 0;
	// u32 clusterGbaSav = 0;
	u32 clusterWideCheat = 0;
	u32 clusterApPatch = 0;
	u32 clusterDSi2DSSave = 0;
	u32 clusterCheat = 0;
	u32 clusterPatchOffsetCache = 0;
	u32 clusterRamDump = 0;
	u32 clusterSrParams = 0;
	u32 clusterScreenshot = 0;
	u32 apFixOverlaysCluster = 0;
	u32 musicCluster = 0;
	u32 clusterPageFile = 0;
	u32 clusterManual = 0;
	u32 clusterTwlFont = 0;

	if (stat(conf->ndsPath, &st) < 0) {
		return -2;
	}

	if (stat(conf->savPath, &stSav) >= 0) {
		clusterSav = stSav.st_ino;
	}

	if (!dsiFeatures() || conf->b4dsMode) {
		if (conf->donorFileOffset) {
			clusterDonor = st.st_ino;
		} else if (conf->useSdk20Donor) {
			if (stat(conf->donor20Path, &stDonor) >= 0) {
				clusterDonor = stDonor.st_ino;
			}
		} else if (stat("fat:/_nds/nds-bootstrap/b4dsTwlDonor.bin", &stDonorStandalone) >= 0) {
			clusterDonor = stDonorStandalone.st_ino;
		} else if (conf->useSdk5DonorAlt) {
			if (stat(conf->donor5PathAlt, &stDonor5Alt) >= 0) {
				clusterDonor = stDonor5Alt.st_ino;
			} else if (stat(conf->donorTwlPath, &stDonor) >= 0) {
				clusterDonor = stDonor.st_ino;
			} else if (stat(conf->donorTwl0Path, &stDonor0) >= 0) {
				clusterDonor = stDonor0.st_ino;
			} else if (stat(conf->donor5Path, &stDonor5) >= 0) {
				clusterDonor = stDonor5.st_ino;
			}
		} else if (stat(conf->donorTwlPath, &stDonor) >= 0) {
			clusterDonor = stDonor.st_ino;
		} else if (stat(conf->donorTwl0Path, &stDonor0) >= 0) {
			clusterDonor = stDonor0.st_ino;
		} else if (stat(conf->donor5Path, &stDonor5) >= 0) {
			clusterDonor = stDonor5.st_ino;
		} else if (stat(conf->donor5PathAlt, &stDonor5Alt) >= 0) {
			clusterDonor = stDonor5Alt.st_ino;
		}
	}

	if (stat(conf->apPatchPath, &stApPatch) >= 0) {
		clusterApPatch = stApPatch.st_ino;
	}

	if (stat(conf->dsi2dsSavePatchPath, &stDSi2DSSavePatch) >= 0) {
		clusterDSi2DSSave = stDSi2DSSavePatch.st_ino;
	}

	if (stat(patchOffsetCacheFilePath, &stPatchOffsetCache) >= 0) {
		clusterPatchOffsetCache = stPatchOffsetCache.st_ino;
	}

	if (stat(srParamsFilePath.c_str(), &stSrParams) >= 0) {
		clusterSrParams = stSrParams.st_ino;
	}

	if (stat(cheatFilePath.c_str(), &stCheat) >= 0) {
		clusterCheat = stCheat.st_ino;
	}

	if (dsiFeatures() && !conf->b4dsMode) {
		/* if (stat(conf->gbaPath, &stGba) >= 0) {
			clusterGba = stGba.st_ino;
		}

		if (stat(conf->gbaSavPath, &stGbaSav) >= 0) {
			clusterGbaSav = stGbaSav.st_ino;
		} */

		if (stat(wideCheatFilePath.c_str(), &stWideCheat) >= 0) {
			clusterWideCheat = stWideCheat.st_ino;
		}
	}

	if (stat(conf->manualPath, &stManual) >= 0) {
		clusterManual = stManual.st_ino;
	}

	if (stat(screenshotPath.c_str(), &stScreenshot) >= 0) {
		clusterScreenshot = stScreenshot.st_ino;
	}

	if (stat(apFixOverlaysPath.c_str(), &stApFixOverlays) >= 0) {
		apFixOverlaysCluster = stApFixOverlays.st_ino;
	}

	if (stat(musicsFilePath.c_str(), &stMusic) >= 0) {
		musicCluster = stMusic.st_ino;
	}

	if (stat(ramDumpPath.c_str(), &stRamDump) >= 0) {
		clusterRamDump = stRamDump.st_ino;
	}

	if (stat(pageFilePath.c_str(), &stPage) >= 0) {
		clusterPageFile = stPage.st_ino;
	}

	if (stat(sharedFontPath.c_str(), &stTwlFont) >= 0) {
		clusterTwlFont = stTwlFont.st_ino;
	}

	return runNds(st.st_ino, clusterSav, clusterDonor, /* clusterGba, clusterGbaSav, */ clusterWideCheat, clusterApPatch, clusterDSi2DSSave, clusterCheat, clusterPatchOffsetCache, clusterRamDump, clusterSrParams, clusterScreenshot, apFixOverlaysCluster, musicCluster, clusterPageFile, clusterManual, clusterTwlFont, conf);
}

int main(int argc, char** argv) {
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);

	configuration* conf = (configuration*)malloc(sizeof(configuration));

	int status = loadFromSD(conf, argv[0]);
	sdFound = (conf->sdFound && !conf->b4dsMode);
	bootstrapOnFlashcard = conf->bootstrapOnFlashcard;

	if (status == 0) {
		status = runNdsFile(conf);
		if (status != 0) {
			powerOff(PM_BACKLIGHT_TOP);
			if (debug) {
				dbg_printf("Start failed. Error %i\n", status);
			} else {
				myConsoleDemoInit();
				iprintf("Start failed. Error %i\n", status);
			}
		}
	}

	if (status != 0) {
		free(conf->ndsPath);
		free(conf->savPath);
		free(conf);

		stop();
	}

	return 0;
}
