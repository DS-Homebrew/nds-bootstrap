#include <stdlib.h> // strtol
#include <unistd.h>
//#include <stdio.h>
#include <nds.h>
#include <string>
#include <string.h>
#include <limits.h> // PATH_MAX
/*#include <nds/ndstypes.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>
#include <nds/debug.h>*/
#include <fat.h>
#include <easykey.h>
#include "tonccpy.h"
#include "hex.h"
#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"
#include "nitrofs.h"

off_t getFileSize(const char* path) {
	FILE* fp = fopen(path, "rb");
	off_t fsize = 0;
	if (fp) {
		fseek(fp, 0, SEEK_END);
		fsize = ftell(fp);
		if (!fsize) fsize = 0;
		fclose(fp);
	}
	return fsize;
}

extern std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);

static void load_conf(configuration* conf, const char* fn) {
	ek_key IniData[64];
	int IniCount = iniLoad(fn, IniData);

	ek_key Key = {(char*)"NDS-BOOTSTRAP", NULL, NULL};

	// Debug
	Key.Data = (char*)"";
	Key.Name = (char*)"DEBUG";
	iniGetKey(IniData, IniCount, &Key);
	conf->debug = (bool)strtol(Key.Data, NULL, 0);

	// NDS path
	Key.Data = (char*)"";
	Key.Name = (char*)"NDS_PATH";
	iniGetKey(IniData, IniCount, &Key);
	conf->ndsPath = strdup(Key.Data);

	// SAV path
	Key.Data = (char*)"";
	Key.Name = (char*)"SAV_PATH";
	iniGetKey(IniData, IniCount, &Key);
	conf->savPath = strdup(Key.Data);

	// Language
	Key.Data = (char*)"";
	Key.Name = (char*)"LANGUAGE";
	iniGetKey(IniData, IniCount, &Key);
	conf->language = strtol(Key.Data, NULL, 0);

	// DSi mode
	Key.Data = (char*)"";
	Key.Name = (char*)"DSI_MODE";
	iniGetKey(IniData, IniCount, &Key);
	conf->dsiMode = strtol(Key.Data, NULL, 0);

	// Donor SDK version
	Key.Data = (char*)"";
	Key.Name = (char*)"DONOR_SDK_VER";
	iniGetKey(IniData, IniCount, &Key);
	conf->donorSdkVer = strtol(Key.Data, NULL, 0);

	// Patch MPU region
	Key.Data = (char*)"";
	Key.Name = (char*)"PATCH_MPU_REGION";
	iniGetKey(IniData, IniCount, &Key);
	conf->patchMpuRegion = strtol(Key.Data, NULL, 0);

	// Patch MPU size
	Key.Data = (char*)"";
	Key.Name = (char*)"PATCH_MPU_SIZE";
	iniGetKey(IniData, IniCount, &Key);
	conf->patchMpuSize = strtol(Key.Data, NULL, 0);

	// Card engine (arm9) cached
	Key.Data = (char*)"";
	Key.Name = (char*)"CARDENGINE_CACHED";
	iniGetKey(IniData, IniCount, &Key);
	conf->ceCached = (bool)strtol(Key.Data, NULL, 0);
    // conf->ceCached = true;

	// Console model
	Key.Data = (char*)"";
	Key.Name = (char*)"CONSOLE_MODEL";
	iniGetKey(IniData, IniCount, &Key);
	conf->consoleModel = strtol(Key.Data, NULL, 0);

	// Color mode
	Key.Data = (char*)"";
	Key.Name = (char*)"COLOR_MODE";
	iniGetKey(IniData, IniCount, &Key);
	conf->colorMode = strtol(Key.Data, NULL, 0);

	// ROM read LED
	Key.Data = (char*)"";
	Key.Name = (char*)"ROMREAD_LED";
	iniGetKey(IniData, IniCount, &Key);
	conf->romread_LED = strtol(Key.Data, NULL, 0);

	// Game soft reset
	Key.Data = (char*)"";
	Key.Name = (char*)"GAME_SOFT_RESET";
	iniGetKey(IniData, IniCount, &Key);
	conf->gameSoftReset = (bool)strtol(Key.Data, NULL, 0);

	// Force sleep patch
	Key.Data = (char*)"";
	Key.Name = (char*)"FORCE_SLEEP_PATCH";
	iniGetKey(IniData, IniCount, &Key);
	conf->forceSleepPatch = (bool)strtol(Key.Data, NULL, 0);

	// Precise volume control
	Key.Data = (char*)"";
	Key.Name = (char*)"PRECISE_VOLUME_CONTROL";
	iniGetKey(IniData, IniCount, &Key);
	conf->preciseVolumeControl = (bool)strtol(Key.Data, NULL, 0);

	// Logging
	Key.Data = (char*)"";
	Key.Name = (char*)"LOGGING";
	iniGetKey(IniData, IniCount, &Key);
	conf->logging = (bool)strtol(Key.Data, NULL, 0);

	// Cheat data
	Key.Data = (char*)"";
	Key.Name = (char*)"CHEAT_DATA";
	iniGetKey(IniData, IniCount, &Key);
	conf->cheat_data = (u32*)malloc(CHEAT_DATA_MAX_SIZE);
	conf->cheat_data_len = 0;
	char* str = strdup(Key.Data);
	char* cheat = strtok(str, " ");
	if (cheat != NULL)
		printf("Cheat data present\n");
	while (cheat != NULL) {
		if (!checkCheatDataLen(conf->cheat_data_len)) {
			printf("Cheat data size limit reached, the cheats are ignored!\n");
			toncset(conf->cheat_data, 0, conf->cheat_data_len*sizeof(u32)); //cheats.clear();
			conf->cheat_data_len = 0;
			break;
		}
		printf(cheat);
		nocashMessage(cheat);
		printf(" ");

		conf->cheat_data[conf->cheat_data_len] = strtoul(cheat, NULL, 16);

		nocashMessage(tohex(conf->cheat_data[conf->cheat_data_len]));
		printf(" ");

		++conf->cheat_data_len;

		cheat = strtok(NULL, " ");
	}
	free(str);
	realloc(conf->cheat_data, conf->cheat_data_len*sizeof(u32));

	// Backlight mode
	Key.Data = (char*)"";
	Key.Name = (char*)"BACKLIGHT_MODE";
	iniGetKey(IniData, IniCount, &Key);
	conf->backlightMode = strtol(Key.Data, NULL, 0);

	// Boost CPU
	Key.Data = (char*)"";
	Key.Name = (char*)"BOOST_CPU";
	iniGetKey(IniData, IniCount, &Key);
	// If DSi mode, then always boost CPU
	conf->dsiMode ? conf->boostCpu = true : conf->boostCpu = (bool)strtol(Key.Data, NULL, 0);

	// Boost VRAM
	Key.Data = (char*)"";
	Key.Name = (char*)"BOOST_VRAM";
	iniGetKey(IniData, IniCount, &Key);
	// If DSi mode, then always boost VRAM
	conf->dsiMode ? conf->boostVram = true : conf->boostVram = (bool)strtol(Key.Data, NULL, 0);

	iniFree(IniData, IniCount);
}

int loadFromSD(configuration* conf, const char *bootstrapPath) {
	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("fatInitDefault failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	if (access("fat:/", F_OK) == 0) {
		consoleDemoInit();
		printf("This edition of nds-bootstrap\n");
		printf("can only be used on the\n");
		printf("SD card.\n");
		return -1;
	}
	
	load_conf(conf, "sd:/_nds/nds-bootstrap.ini");
	mkdir("sd:/_nds/nds-bootstrap", 0777);
	mkdir("sd:/_nds/nds-bootstrap/patchOffsetCache", 0777);
	mkdir("sd:/_nds/nds-bootstrap/fatTable", 0777);

	nitroFSInit(bootstrapPath);
	
	// Load ce7 binary
	FILE* ce7bin = fopen("nitro:/cardengine_arm7.bin", "rb");
	fread((void*)0x027E0000, 1, 0x10000, ce7bin);
	fclose(ce7bin);

	conf->romSize = getFileSize(conf->ndsPath);
	conf->saveSize = getFileSize(conf->savPath);

	conf->argc = 0;
	conf->argv = (const char**)malloc(ARG_MAX);
	if (strcasecmp(conf->ndsPath + strlen(conf->ndsPath) - 5, ".argv") == 0) {
		FILE* argfile = fopen(conf->ndsPath, "rb");

		char str[PATH_MAX];
		char* pstr;
		const char* seps = "\n\r\t ";

		while (fgets(str, PATH_MAX, argfile)) {
			// Find comment and end string there
			if ((pstr = strchr(str, '#'))) {
				*pstr = '\0';
			}

			// Tokenize arguments
			pstr = strtok(str, seps);

			while (pstr != NULL) {
				conf->argv[conf->argc] = strdup(pstr);
				++conf->argc;

				pstr = strtok(NULL, seps);
			}
		}
		fclose(argfile);

		free(conf->ndsPath);
		conf->ndsPath = strdup(conf->argv[0]);
	} else {
		conf->argv[0] = strdup(conf->ndsPath);
		conf->argc = 1; //++conf->argc;
	}
	realloc(conf->argv, conf->argc*sizeof(const char*));
	
	std::string romFilename = ReplaceAll(conf->ndsPath, ".nds", ".bin");
	const size_t last_slash_idx = romFilename.find_last_of("/");
	if (std::string::npos != last_slash_idx)
	{
		romFilename.erase(0, last_slash_idx + 1);
	}

	std::string patchOffsetCacheFilePath = "sd:/_nds/nds-bootstrap/patchOffsetCache/"+romFilename;
	
	if (access(patchOffsetCacheFilePath.c_str(), F_OK) != 0) {
		char buffer[0x200] = {0};

		FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath.c_str(), "wb");
		fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
		fclose(patchOffsetCacheFile);
	}

	std::string fatTableFilePath = "sd:/_nds/nds-bootstrap/fatTable/"+romFilename;

	if (access(fatTableFilePath.c_str(), F_OK) != 0) {
		static const int BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		toncset(buffer, 0, sizeof(buffer));

		FILE *fatTableFile = fopen(fatTableFilePath.c_str(), "wb");
		for (int i = 0x80200; i > 0; i -= BUFFER_SIZE) {
			fwrite(buffer, 1, sizeof(buffer), fatTableFile);
		}
		fclose(fatTableFile);
	}

	FILE *fatTableFile = fopen(fatTableFilePath.c_str(), "rb");
	if (fatTableFile) {
		fread((void*)0x02700000, 1, 0x80200, fatTableFile);
	}
	fclose(fatTableFile);

	return 0;
}
