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
#include "hex.h"
#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"
#include "nitrofs.h"

static u16 bmpImageBuffer[256*192];
static u16 renderedImageBuffer[256*192];

static off_t getSaveSize(const char* path) {
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

	// Loading screen
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_SCREEN";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingScreen = strtol(Key.Data, NULL, 0);

	// Loading screen (Dark theme)
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_DARK_THEME";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingDarkTheme = (bool)strtol(Key.Data, NULL, 0);

	// Swap screens in loading screen
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_SWAP_LCDS";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingSwapLcds = (bool)strtol(Key.Data, NULL, 0);

	// Loading screen .bmp folder path
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_SCREEN_FOLDER";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingImagePath = strdup(Key.Data);

	// Number of loading screen frames
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_FRAMES";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingFrames = strtol(Key.Data, NULL, 0);

	// FPS of animated loading screen
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_FPS";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingFps = strtol(Key.Data, NULL, 0);

	// Show/Hide loading bar
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_BAR";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingBar = (bool)strtol(Key.Data, NULL, 0);

	// Loading bar Y position
	Key.Data = (char*)"";
	Key.Name = (char*)"LOADING_BAR_Y";
	iniGetKey(IniData, IniCount, &Key);
	conf->loadingBarYpos = strtol(Key.Data, NULL, 0);

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
			memset(conf->cheat_data, 0, conf->cheat_data_len*sizeof(u32)); //cheats.clear();
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

u16 convertToDsBmp(int colorMode, u16 val) {
	if (colorMode == 1) {
		u16 newVal = ((val>>10)&31) | (val&31<<5) | (val&31)<<10 | BIT(15);

		u8 b,g,r,max,min;
		b = ((newVal)>>10)&31;
		g = ((newVal)>>5)&31;
		r = (newVal)&31;
		// Key.Data decomposition of hsv
		max = (b > g) ? b : g;
		max = (max > r) ? max : r;

		// Desaturate
		min = (b < g) ? b : g;
		min = (min < r) ? min : r;
		max = (max + min) / 2;
		
		return 32768|(max<<10)|(max<<5)|(max);
	} else {
		return ((val>>10)&31) | (val&31<<5) | (val&31)<<10 | BIT(15);
	}
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

	nitroFSInit(bootstrapPath);
	
	// Load ce7 binary
	FILE* ce7bin = fopen("nitro:/cardengine_arm7.bin", "rb");
	fread((void*)0x027E0000, 1, 0x10000, ce7bin);

	if (conf->loadingScreen == 5) {
		FILE* loadingScreenImage;
		char loadingImagePath[256];

		for (int i = 0; i <= conf->loadingFrames; i++) {
			snprintf(loadingImagePath, sizeof(loadingImagePath), "%s%i.bmp", conf->loadingImagePath, i);
			loadingScreenImage = fopen(loadingImagePath, "rb");
			if (!loadingScreenImage && i == 0) {
				loadingScreenImage = fopen("nitro:/loading_metalBG.bmp", "rb");
				conf->loadingFps = 0;
				conf->loadingBar = true;
				conf->loadingBarYpos = 89;
			}
			if (loadingScreenImage) {
				// Start loading
				fseek(loadingScreenImage, 0xe, SEEK_SET);
				u8 pixelStart = (u8)fgetc(loadingScreenImage) + 0xe;
				fseek(loadingScreenImage, pixelStart, SEEK_SET);
				fread(bmpImageBuffer, 2, 0x18000, loadingScreenImage);
				u16* src = bmpImageBuffer;
				int x = 0;
				int y = 191;
				for (int i=0; i<256*192; i++) {
					if (x >= 256) {
						x = 0;
						y--;
					}
					u16 val = *(src++);
					renderedImageBuffer[y*256+x] = convertToDsBmp(conf->colorMode, val);
					x++;
				}
				memcpy((void*)0x02800000+(i*0x18000), renderedImageBuffer, sizeof(renderedImageBuffer));
			}
			fclose(loadingScreenImage);

			if (conf->loadingFps == 0 || i == 29) break;
		}
	}

	conf->saveSize = getSaveSize(conf->savPath);

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
		FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath.c_str(), "wb");
		char buffer[0x200] = {0};
		fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
		fclose(patchOffsetCacheFile);
	}

	if (access("sd:/_nds/nds-bootstrap/fatTable.bin", F_OK) != 0) {
		FILE* fatTableFile = fopen("sd:/_nds/nds-bootstrap/fatTable.bin", "wb");
		char buffer[0x80200] = {0};
		fwrite(buffer, 1, sizeof(buffer), fatTableFile);
		fclose(fatTableFile);
	}

	return 0;
}
