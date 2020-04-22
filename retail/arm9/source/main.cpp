/*-----------------------------------------------------------------

 Copyright (C) 2010  Dave "WinterMute" Murphy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, 
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

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

#include "configuration.h"
#include "nds_loader_arm9.h"
#include "conf_sd.h"

u8 lz77ImageBuffer[0x12000];

std::string patchOffsetCacheFilePath;
std::string fatTableFilePath;
std::string wideCheatFilePath;
std::string cheatFilePath;
std::string ramDumpPath;
std::string srParamsFilePath;

typedef struct {
	char gameTitle[12];			//!< 12 characters for the game title.
	char gameCode[4];			//!< 4 characters for the game code.
} sNDSHeaderTitleCodeOnly;

//extern bool logging;
//bool logging = false;

static bool debug = false;

static inline const char* btoa(bool x) {
	return x ? "true" : "false";
}

static int dbg_printf(const char* format, ...) { // static int...
	if (!debug) {
		return 0;
	}

	static FILE* debugFile;
	debugFile = fopen("sd:/NDSBTSRP.LOG", "a");

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
	printf("Press start...\n");
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

bool extention(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

static void getSFCG_ARM9(void) {
	printf("SCFG_ROM ARM9 %X\n", REG_SCFG_ROM); 
	printf("SCFG_CLK ARM9 %X\n", REG_SCFG_CLK); 
	//printf("SCFG_EXT ARM9 %X\n", REG_SCFG_EXT); 
}

static void getSFCG_ARM7(void) {
	//printf("SCFG_ROM ARM7\n");

	//nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_ROM);\n");
	//fifoSendValue32(FIFO_USER_01, (u32)&REG_SCFG_ROM);

	//nocashMessage("dbg_printf\n");

	printf("SCFG_CLK ARM7\n");

	nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_CLK);\n");
	fifoSendValue32(FIFO_USER_01, (u32)&REG_SCFG_CLK);

	printf("SCFG_EXT ARM7\n");

	nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_EXT);\n");
	fifoSendValue32(FIFO_USER_01, (u32)&REG_SCFG_EXT);
}

static void myFIFOValue32Handler(u32 value, void* userdata) {
	nocashMessage("myFIFOValue32Handler\n");
	printf("ARM7 data %lX\n", value);
}

static inline void debugConf(configuration* conf) {
	dbg_printf("debug: %s\n", btoa(conf->debug));
	dbg_printf("ndsPath: \"%s\"\n", conf->ndsPath);
	dbg_printf("savPath: \"%s\"\n", conf->savPath);
	dbg_printf("donorPath: \"%s\"\n", conf->donorPath);
	dbg_printf("gbaPath: \"%s\"\n", conf->gbaPath);
	if (debug) {
		dopause();
	}
	dbg_printf("saveSize: %lX\n", conf->saveSize);
	dbg_printf("language: %hhX\n", conf->language);
	dbg_printf("dsiMode: %i\n", conf->dsiMode);
	dbg_printf("donorSdkVer: %lX\n", conf->donorSdkVer);
	dbg_printf("patchMpuRegion: %lX\n", conf->patchMpuRegion);
	dbg_printf("patchMpuSize: %lX\n", conf->patchMpuSize);
	dbg_printf("ceCached: %s\n", btoa(conf->ceCached));
	dbg_printf("cacheBlockSize: %s\n", conf->cacheBlockSize==1 ? "8000" : "4000");
	dbg_printf("extendedMemory: %s\n", btoa(conf->extendedMemory));
	dbg_printf("consoleModel: %lX\n", conf->consoleModel);
	dbg_printf("colorMode: %lX\n", conf->colorMode);
	dbg_printf("romRead_LED: %lX\n", conf->romRead_LED);
	dbg_printf("dmaRomRead_LED: %lX\n", conf->dmaRomRead_LED);
	dbg_printf("boostCpu: %s\n", btoa(conf->boostCpu));
	dbg_printf("boostVram: %s\n", btoa(conf->boostVram));
	dbg_printf("soundFreq: %s\n", btoa(conf->soundFreq));
	dbg_printf("forceSleepPatch: %s\n", btoa(conf->forceSleepPatch));
	dbg_printf("logging: %s\n", btoa(conf->logging));
	dbg_printf("initDisc: %s\n", btoa(conf->initDisc));
	dbg_printf("gameOnFlashcard: %s\n", btoa(conf->gameOnFlashcard));
	dbg_printf("saveOnFlashcard: %s\n", btoa(conf->saveOnFlashcard));
	dbg_printf("backlightMode: %lX\n", conf->backlightMode);
}

static int runNdsFile(configuration* conf) {
	// Debug
	debug = conf->debug;
	if (debug) {
		consoleDemoInit();

		fifoSetValue32Handler(FIFO_USER_02, myFIFOValue32Handler, NULL);

		getSFCG_ARM9();
		getSFCG_ARM7();

		/*for (int i = 0; i < 60; i++) {
			swiWaitForVBlank();
		}*/
	}

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

	fifoSendValue32(FIFO_USER_03, 1);
	fifoWaitValue32(FIFO_USER_05);

	// Logging
	const char *logFilePath = (conf->sdFound ? "sd:/NDSBTSRP.LOG" : "fat:/NDSBTSRP.LOG");
	if (conf->logging) {
		static FILE* loggingFile;
		loggingFile = fopen(logFilePath, "w");
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

	debugConf(conf);

	if ((extention(conf->ndsPath, ".nds") != 0)
	&& (extention(conf->ndsPath, ".dsi") != 0)
	&& (extention(conf->ndsPath, ".ids") != 0)
	&& (extention(conf->ndsPath, ".srl") != 0)
	&& (extention(conf->ndsPath, ".app") != 0)) {
		if (debug) {
			dbg_printf("No NDS file specified\n");
			dopause();
		} else {
			consoleDemoInit();
			printf("No NDS file specified\n");
		}
		return -1;
	}

	//dbg_printf("Running \"%s\" with %d parameters\n", conf->ndsPath, conf->argc);
	if (debug) {
		dopause();
	}

	switch(conf->backlightMode) {
		case 0:
		default:
			break;
		case 1:
			powerOff(PM_BACKLIGHT_BOTTOM);
			break;
		case 2:
			powerOff(PM_BACKLIGHT_TOP);
			break;
		case 3:
			powerOff(PM_BACKLIGHT_TOP);
			powerOff(PM_BACKLIGHT_BOTTOM);
			break;
	}

	struct stat st;
	struct stat stSav;
	struct stat stDonor[2];
	struct stat stGba;
	struct stat stWideCheat;
	struct stat stApPatch;
	struct stat stCheat;
	struct stat stPatchOffsetCache;
	struct stat stFatTable;
	struct stat stRamDump;
	struct stat stSrParams;
	u32 clusterSav = 0;
	u32 clusterDonor[2] = {0};
	u32 clusterGba = 0;
	u32 clusterWideCheat = 0;
	u32 clusterApPatch = 0;
	u32 clusterCheat = 0;
	u32 clusterPatchOffsetCache = 0;
	u32 clusterFatTable = 0;
	u32 clusterRamDump = 0;
	u32 clusterSrParams = 0;

	if (stat(conf->ndsPath, &st) < 0) {
		return -2;
	}

	if (stat(conf->savPath, &stSav) >= 0) {
		clusterSav = stSav.st_ino;
	}

	if (stat(conf->donor2Path, &stDonor[0]) >= 0) {
		clusterDonor[0] = stDonor[0].st_ino;
	}

	if (stat(conf->donorPath, &stDonor[1]) >= 0) {
		clusterDonor[1] = stDonor[1].st_ino;
	}

	if (stat(conf->gbaPath, &stGba) >= 0) {
		clusterGba = stGba.st_ino;
	}

	if (stat(wideCheatFilePath.c_str(), &stWideCheat) >= 0) {
		clusterWideCheat = stWideCheat.st_ino;
	}

	if (stat(conf->apPatchPath, &stApPatch) >= 0) {
		clusterApPatch = stApPatch.st_ino;
	}

	if (stat(cheatFilePath.c_str(), &stCheat) >= 0) {
		clusterCheat = stCheat.st_ino;
	}

	if (stat(patchOffsetCacheFilePath.c_str(), &stPatchOffsetCache) >= 0) {
		clusterPatchOffsetCache = stPatchOffsetCache.st_ino;
	}

	if (conf->cacheFatTable && stat(fatTableFilePath.c_str(), &stFatTable) >= 0) {
		clusterFatTable = stFatTable.st_ino;
	}

	if (stat(ramDumpPath.c_str(), &stRamDump) >= 0) {
		clusterRamDump = stRamDump.st_ino;
	}

	if (stat(srParamsFilePath.c_str(), &stSrParams) >= 0) {
		clusterSrParams = stSrParams.st_ino;
	}

	runNds(st.st_ino, clusterSav, clusterDonor[0], clusterDonor[1], clusterGba, clusterWideCheat, clusterApPatch, clusterCheat, clusterPatchOffsetCache, clusterFatTable, clusterRamDump, clusterSrParams, conf);

	return 0;
}

int main(int argc, char** argv) {
	configuration* conf = (configuration*)malloc(sizeof(configuration));
	conf->initDisc = false;

	int status = loadFromSD(conf, argv[0]);

	if (status == 0) {
		status = runNdsFile(conf);
		if (status != 0) {
			powerOff(PM_BACKLIGHT_TOP);
			debug ? dbg_printf("Start failed. Error %i\n", status) : printf("Start failed. Error %i\n", status);
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
