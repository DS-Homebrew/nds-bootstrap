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
#include <string.h>
#include <stdarg.h>
#include <limits.h> // PATH_MAX
#include <unistd.h>
#include <sys/stat.h>
#include <nds/ndstypes.h>
#include <nds/arm9/input.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>
#include <nds/system.h>
#include <nds/debug.h>

#include "configuration.h"
#include "nds_loader_arm9.h"
#include "conf_sd.h"

#include "load_bin.h"

/* typedef struct {
	char gameTitle[12];			//!< 12 characters for the game title.
	char gameCode[4];			//!< 4 characters for the game code.
} sNDSHeaderTitleCodeOnly; */

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
	iprintf("Press start...\n");
	while(1) {
		scanKeys();
		if (keysDown() & KEY_START)
			break;
		swiWaitForVBlank();
	}
	scanKeys();
}

static void getSFCG_ARM9(void) {
	iprintf("SCFG_ROM ARM9 %X\n", REG_SCFG_ROM); 
	iprintf("SCFG_CLK ARM9 %X\n", REG_SCFG_CLK); 
	//iprintf("SCFG_EXT ARM9 %X\n", REG_SCFG_EXT); 
}

static void getSFCG_ARM7(void) {
	//iprintf("SCFG_ROM ARM7\n");

	//nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_ROM);\n");
	//fifoSendValue32(FIFO_USER_01, (long unsigned int)&REG_SCFG_ROM);

	//nocashMessage("dbg_printf\n");

	iprintf("SCFG_CLK ARM7\n");

	nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_CLK);\n");
	fifoSendValue32(FIFO_USER_01, (long unsigned int)&REG_SCFG_CLK);

	iprintf("SCFG_EXT ARM7\n");

	nocashMessage("fifoSendValue32(FIFO_USER_01, MSG_SCFG_EXT);\n");
	fifoSendValue32(FIFO_USER_01, (long unsigned int)&REG_SCFG_EXT);
}

static void myFIFOValue32Handler(u32 value, void* data) {
	nocashMessage("myFIFOValue32Handler\n");
	iprintf("ARM7 data %lX\n", value);
}

static inline void debugConf(configuration* conf) {
	dbg_printf("debug: %s\n", btoa(conf->debug));
	dbg_printf("ndsPath: \"%s\"\n", conf->ndsPath);
	dbg_printf("savPath: \"%s\"\n", conf->savPath);
	if (debug) {
		dopause();
	}
	dbg_printf("saveSize: %lX\n", conf->saveSize);
	dbg_printf("language: %hhX\n", conf->language);
	dbg_printf("dsiMode: %s\n", btoa(conf->dsiMode));
	dbg_printf("donorSdkVer: %lX\n", conf->donorSdkVer);
	dbg_printf("patchMpuRegion: %lX\n", conf->patchMpuRegion);
	dbg_printf("patchMpuSize: %lX\n", conf->patchMpuSize);
	dbg_printf("consoleModel: %lX\n", conf->consoleModel);
	dbg_printf("loadingScreen: %lX\n", conf->loadingScreen);
	dbg_printf("romread_LED: %lX\n", conf->romread_LED);
	dbg_printf("boostCpu: %s\n", btoa(conf->boostCpu));
	dbg_printf("gameSoftReset: %s\n", btoa(conf->gameSoftReset));
	dbg_printf("asyncPrefetch: %s\n", btoa(conf->asyncPrefetch));
	dbg_printf("soundFix: %s\n", btoa(conf->soundFix));
	dbg_printf("logging: %s\n", btoa(conf->logging));
	dbg_printf("initDisc: %s\n", btoa(conf->initDisc));
	dbg_printf("dldiPatchNds: %s\n", btoa(conf->dldiPatchNds));
	dbg_printf("argc: %lu\n", conf->argc);
	//const char** argv;
	//u32 cheat_data[CHEAT_DATA_MAX_LEN];
	dbg_printf("cheat_data_len: %lu\n", conf->cheat_data_len);
	dbg_printf("backlightMode: %lX\n", conf->backlightMode);
}

static int runNdsFile(configuration* conf) {
	// Debug
	debug = conf->debug;
	if (debug) {
		powerOff(PM_BACKLIGHT_TOP);
		consoleDemoInit();

		fifoSetValue32Handler(FIFO_USER_02, myFIFOValue32Handler, 0);

		getSFCG_ARM9();
		getSFCG_ARM7();

		/*for (int i = 0; i < 60; i++) {
			swiWaitForVBlank();
		}*/
	}

	// ROM read LED
	switch(conf->romread_LED) {
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

	// adjust TSC[1:26h] and TSC[1:27h]
	// for certain gamecodes
	/*FILE* f_nds_file = fopen(conf->ndsPath, "rb");

	char romTid[5];
	fseek(f_nds_file, offsetof(sNDSHeaderTitleCodeOnly, gameCode), SEEK_SET);
	fread(romTid, 1, 4, f_nds_file);
	romTid[4] = 0;
	romTid[3] = 0;
	//romTid[2] = 0; // SDK 5
	//romTid[1] = 0; // SDK 5
	fclose(f_nds_file);

	// SDK 5
	//if (strcmp(romTid, "I") != 0) {
	//	fifoSendValue32(FIFO_USER_08, 1); // Disable Slot-1 access for games with no built-in infrared port
	//}

	if (strcmp(romTid, "ABX") == 0	// NTR-ABXE Bomberman Land Touch!
		|| strcmp(romTid, "YO9") == 0	// NTR-YO9J Bokura no TV Game Kentei - Pikotto! Udedameshi
		|| strcmp(romTid, "ALH") == 0	// NTR-ALHE Flushed Away
		|| strcmp(romTid, "ACC") == 0	// NTR-ACCE Cooking Mama
		|| strcmp(romTid, "YCQ") == 0	// NTR-YCQE Cooking Mama 2 - Dinner with Friends
		|| strcmp(romTid, "YYK") == 0	// NTR-YYKE Trauma Center - Under the Knife 2
		|| strcmp(romTid, "AZW") == 0	// NTR-AZWE WarioWare - Touched!
		|| strcmp(romTid, "AKA") == 0	// NTR-AKAE Rub Rabbits!, The
		|| strcmp(romTid, "AN9") == 0	// NTR-AN9E Little Mermaid - Ariel's Undersea Adventure, The
		|| strcmp(romTid, "AKE") == 0	// NTR-AKEJ Keroro Gunsou - Enshuu da Yo! Zenin Shuugou Part 2
		|| strcmp(romTid, "YFS") == 0	// NTR-YFSJ Frogman Show - DS Datte, Shouganaijanai, The
		|| strcmp(romTid, "YG8") == 0	// NTR-YG8E Yu-Gi-Oh! World Championship 2008
		|| strcmp(romTid, "AY7") == 0	// NTR-AY7E Yu-Gi-Oh! World Championship 2007
		|| strcmp(romTid, "YON") == 0	// NTR-YONJ Minna no DS Seminar - Kantan Ongakuryoku
		|| strcmp(romTid, "A5H") == 0	// NTR-A5HE Interactive Storybook DS - Series 2
		|| strcmp(romTid, "A5I") == 0	// NTR-A5IE Interactive Storybook DS - Series 3
		|| strcmp(romTid, "AMH") == 0	// NTR-AMHE Metroid Prime Hunters
		|| strcmp(romTid, "A3T") == 0	// NTR-A3TE Tak - The Great Juju Challenge
		|| strcmp(romTid, "YBO") == 0	// NTR-YBOE Boogie
		|| strcmp(romTid, "ADA") == 0	// NTR-ADAE PKMN Diamond
		|| strcmp(romTid, "APA") == 0	// NTR-APAE PKMN Pearl
		|| strcmp(romTid, "CPU") == 0	// NTR-CPUE PKMN Platinum
		|| strcmp(romTid, "APY") == 0	// NTR-APYE Puyo Pop Fever
		|| strcmp(romTid, "AWH") == 0	// NTR-AWHE Bubble Bobble Double Shot
		|| strcmp(romTid, "AXB") == 0	// NTR-AXBJ Daigassou! Band Brothers DX
		|| strcmp(romTid, "A4U") == 0	// NTR-A4UJ Wi-Fi Taiou - Morita Shogi
		|| strcmp(romTid, "A8N") == 0	// NTR-A8NE Planet Puzzle League
		|| strcmp(romTid, "ABJ") == 0	// NTR-ABJE Harvest Moon DS - Island of Happiness
		|| strcmp(romTid, "ABN") == 0	// NTR-ABNE Bomberman Story DS
		|| strcmp(romTid, "ACL") == 0	// NTR-ACLE Custom Robo Arena
		|| strcmp(romTid, "ART") == 0	// NTR-ARTJ Shin Lucky Star Moe Drill - Tabidachi
		|| strcmp(romTid, "AVT") == 0	// NTR-AVTJ Kou Rate Ura Mahjong Retsuden Mukoubuchi - Goburei, Shuuryou desu ne
		|| strcmp(romTid, "AWY") == 0	// NTR-AWYJ Wi-Fi Taiou - Gensen Table Game DS
		|| strcmp(romTid, "AXJ") == 0	// NTR-AXJE Dungeon Explorer - Warriors of Ancient Arts
		|| strcmp(romTid, "AYK") == 0	// NTR-AYKJ Wi-Fi Taiou - Yakuman DS
		|| strcmp(romTid, "YB2") == 0	// NTR-YB2E Bomberman Land Touch! 2
		|| strcmp(romTid, "YB3") == 0	// NTR-YB3E Harvest Moon DS - Sunshine Islands
		|| strcmp(romTid, "YCH") == 0	// NTR-YCHJ Kousoku Card Battle - Card Hero
		|| strcmp(romTid, "YFE") == 0	// NTR-YFEE Fire Emblem - Shadow Dragon
		|| strcmp(romTid, "YGD") == 0	// NTR-YGDE Diary Girl
		|| strcmp(romTid, "YKR") == 0	// NTR-YKRJ Culdcept DS
		|| strcmp(romTid, "YRM") == 0	// NTR-YRME My Secret World by Imagine
		|| strcmp(romTid, "YW2") == 0	// NTR-YW2E Advance Wars - Days of Ruin
		|| strcmp(romTid, "AJU") == 0	// NTR-AJUJ Jump! Ultimate Stars
		|| strcmp(romTid, "ACZ") == 0	// NTR-ACZE Cars
		|| strcmp(romTid, "AHD") == 0	// NTR-AHDE Jam Sessions
		|| strcmp(romTid, "ANR") == 0	// NTR-ANRE Naruto - Saikyou Ninja Daikesshu 3
		|| strcmp(romTid, "YT3") == 0	// NTR-YT3E Tamagotchi Connection - Corner Shop 3
		|| strcmp(romTid, "AVI") == 0	// NTR-AVIJ Kodomo no Tame no Yomi Kikase - Ehon de Asobou 1-Kan
		|| strcmp(romTid, "AV2") == 0	// NTR-AV2J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 2-Kan
		|| strcmp(romTid, "AV3") == 0	// NTR-AV3J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 3-Kan
		|| strcmp(romTid, "AV4") == 0	// NTR-AV4J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 4-Kan
		|| strcmp(romTid, "AV5") == 0	// NTR-AV5J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 5-Kan
		|| strcmp(romTid, "AV6") == 0	// NTR-AV6J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 6-Kan
		|| strcmp(romTid, "YNZ") == 0	// NTR-YNZE Petz - Dogz Fashion
	)
	{
		fifoSendValue32(FIFO_MAXMOD, 1); // Special setting (when found special gamecode)
	}*/

	// Boost CPU
	if (conf->boostCpu) {
		dbg_printf("CPU boosted\n");
		// libnds sets TWL clock speeds on arm7/arm9 scfg_clk at boot now. No changes needed.
	} else {
		REG_SCFG_CLK = 0x80;
		fifoSendValue32(FIFO_USER_06, 1);
	}

	fifoSendValue32(FIFO_USER_03, 1);
	fifoWaitValue32(FIFO_USER_05);

	// Logging
	if (conf->logging) {
		static FILE* loggingFile;
		loggingFile = fopen("sd:/NDSBTSRP.LOG", "w");
		fprintf(loggingFile, "LOGGING MODE\n");
		fclose(loggingFile);

		// Create a big file (minimal sdengine libfat cannot append to a file)
		loggingFile = fopen("sd:/NDSBTSRP.LOG", "a");
		for (int i = 0; i < 1000; i++) {
			fprintf(loggingFile, "                                                                                                                                          \n");
		}

		fclose(loggingFile);
	} else {
		remove("sd:/NDSBTSRP.LOG");
	}

	debugConf(conf);

	if (strcasecmp(conf->ndsPath + strlen(conf->ndsPath) - 4, ".nds") != 0 || conf->argc == 0) {
		dbg_printf("No NDS file specified\n");
		if (debug) {
			dopause();
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
			powerOn(PM_BACKLIGHT_TOP);
			//powerOn(PM_BACKLIGHT_BOTTOM);
			break;
		case 1:
			powerOn(PM_BACKLIGHT_TOP);
			powerOff(PM_BACKLIGHT_BOTTOM);
			break;
		case 2:
			powerOff(PM_BACKLIGHT_TOP);
			powerOn(PM_BACKLIGHT_BOTTOM);
			break;
		case 3:
			powerOff(PM_BACKLIGHT_TOP);
			powerOff(PM_BACKLIGHT_BOTTOM);
			break;
	}

	struct stat st;
	struct stat stSav;
	u32 clusterSav = 0;
	char filePath[PATH_MAX];
	int pathLen;
	//const char* args[1];

	if (stat(conf->ndsPath, &st) < 0) {
		return -2;
	}
	
	if (stat(conf->savPath, &stSav) >= 0) {
		clusterSav = stSav.st_ino;
	}
	
	if (conf->argc <= 0 || !conf->argv) {
		// Construct a command line if we weren't supplied with one
		if (!getcwd(filePath, PATH_MAX)) {
			return -3;
		}
		pathLen = strlen(filePath);
		strcpy(filePath + pathLen, conf->ndsPath);
		//args[0] = filePath;
		//conf->argv = args;
		memset(conf->argv, 0, conf->argc*sizeof(const char*));
		conf->argv[0] = filePath;
		conf->argc = 1;
	}

	//bool havedsiSD = false;
	//bool havedsiSD = (argv[0][0] == 's' && argv[0][1] == 'd');

	runNds(load_bin, load_bin_size, st.st_ino, clusterSav, conf);

	return 0;
}

int main(int argc, char** argv) {
	configuration* conf = malloc(sizeof(configuration));
	conf->initDisc = true;
	conf->dldiPatchNds = true;

	int status = 0;

	if (access("fat:/", F_OK) == 0) {
		consoleDemoInit();
		printf("This edition of nds-bootstrap\n");
		printf("can only be used on the\n");
		printf("SD card.\n");
		status = -1;
	} else {
		status = loadFromSD(conf);
	}

	if (status == 0) {
		status = runNdsFile(conf);
		if (status != 0) {
			powerOff(PM_BACKLIGHT_TOP);
			dbg_printf("Start failed. Error %i\n", status);
		}
	}

	if (status != 0) {
		free(conf->ndsPath);
		free(conf->savPath);
		free(conf->argv);
		free(conf->cheat_data);
		free(conf);
		
		stop();
	}

	return 0;
}
