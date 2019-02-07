#include <stdlib.h> // strtol
//#include <stdio.h>
//#include <nds.h>
#include <string.h>
#include <limits.h> // PATH_MAX
#include <nds/ndstypes.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>
#include <nds/debug.h>
#include <fat.h>
#include "minIni.h"
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

static inline bool match(const char* section, const char* s, const char* key, const char* k) {
	return (strcmp(section, s) == 0 && strcmp(key, k) == 0);
}

static int callback(const char *section, const char *key, const char *value, void *userdata) {
	configuration* conf = (configuration*)userdata;

	if (match(section, "NDS-BOOTSTRAP", key, "DEBUG")) {
		// Debug
		conf->debug = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "NDS_PATH")) {
		// NDS path
		//conf->ndsPath = malloc((strlen(value) + 1)*sizeof(char));
		//strcpy(conf->ndsPath, value);
		conf->ndsPath = strdup(value);

	} else if (match(section, "NDS-BOOTSTRAP", key, "SAV_PATH")) {
		// SAV path
		//conf->savPath = malloc((strlen(value) + 1)*sizeof(char));
		//strcpy(conf->savPath, value);
		conf->savPath = strdup(value);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LANGUAGE")) {
		// Language
		conf->language = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "DSI_MODE")) {
		// DSi mode
		conf->dsiMode = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "DONOR_SDK_VER")) {
		// Donor SDK version
		conf->donorSdkVer = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "PATCH_MPU_REGION")) {
		// Patch MPU region
		conf->patchMpuRegion = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "PATCH_MPU_SIZE")) {
		// Patch MPU size
		conf->patchMpuSize = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "CONSOLE_MODEL")) {
		// Console model
		conf->consoleModel = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_SCREEN")) {
		// Loading screen
		conf->loadingScreen = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_DARK_THEME")) {
		// Loading screen (Dark theme)
		conf->loadingDarkTheme = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_SWAP_LCDS")) {
		// Swap screens in loading screen
		conf->loadingSwapLcds = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_SCREEN_FOLDER")) {
		// Loading screen .bmp folder path
		conf->loadingImagePath = strdup(value);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_FRAMES")) {
		// Number of loading screen frames
		conf->loadingFrames = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_FPS")) {
		// FPS of animated loading screen
		conf->loadingFps = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_BAR")) {
		// Show/Hide loading bar
		conf->loadingBar = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOADING_BAR_Y")) {
		// Loading bar Y position
		conf->loadingBarYpos = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "ROMREAD_LED")) {
		// ROM read LED
		conf->romread_LED = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "GAME_SOFT_RESET")) {
		// Game soft reset
		conf->gameSoftReset = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "FORCE_SLEEP_PATCH")) {
		// Force sleep patch
		conf->forceSleepPatch = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "LOGGING")) {
		// Logging
		conf->logging = (bool)strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "CHEAT_DATA")) {
		// Cheat data
		conf->cheat_data = malloc(CHEAT_DATA_MAX_SIZE);
		conf->cheat_data_len = 0;
		char* str = strdup(value);
		char* cheat = strtok(str, " ");
		if (cheat != NULL) {
			printf("Cheat data present\n");
		}
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

	} else if (match(section, "NDS-BOOTSTRAP", key, "BACKLIGHT_MODE")) {
		// Backlight mode
		conf->backlightMode = strtol(value, NULL, 0);

	} else if (match(section, "NDS-BOOTSTRAP", key, "BOOST_CPU")) {
		// Boost CPU
		if (conf->dsiMode) {
			conf->boostCpu = true;
		} else {
			conf->boostCpu = (bool)strtol(value, NULL, 0);
		}

	} else if (match(section, "NDS-BOOTSTRAP", key, "BOOST_VRAM")) {
		// Boost VRAM
		if (conf->dsiMode) {
			conf->boostVram = true;
		} else {
			conf->boostVram = (bool)strtol(value, NULL, 0);
		}

	} else {
		// Unknown section/name
		//return 0; // Error
	}
	
	return 1; // Continue searching
}

int loadFromSD(configuration* conf, const char *bootstrapPath) {
	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("FAT init failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	ini_browse(callback, conf, "fat:/_nds/nds-bootstrap.ini");

	nitroFSInit(bootstrapPath);
	
	if (conf->loadingScreen == 5) {
		FILE* loadingScreenImage;
		//char loadingImagePath[256];

		//for (int i = 0; i <= conf->loadingFrames; i++) {
		for (int i = 0; i < 1; i++) {
			//snprintf(loadingImagePath, sizeof(loadingImagePath), "%s%i.bmp", conf->loadingImagePath, i);
			//loadingScreenImage = fopen(loadingImagePath, "rb");
			//if (!loadingScreenImage && i == 0) {
				loadingScreenImage = fopen("nitro:/loading_metalBG.bmp", "rb");
				conf->loadingFps = 0;
				conf->loadingBar = true;
				conf->loadingBarYpos = 89;
			//}
			if (loadingScreenImage) {
				// Start loading
				fseek(loadingScreenImage, 0xe, SEEK_SET);
				u8 pixelStart = (u8)fgetc(loadingScreenImage) + 0xe;
				fseek(loadingScreenImage, pixelStart, SEEK_SET);
				fread(bmpImageBuffer, 2, 0x1A000, loadingScreenImage);
				u16* src = bmpImageBuffer;
				int x = 0;
				int y = 191;
				for (int i=0; i<256*192; i++) {
					if (x >= 256) {
						x = 0;
						y--;
					}
					u16 val = *(src++);
					renderedImageBuffer[y*256+x] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
					x++;
				}
				memcpy((void*)0x02360000+(i*0x18000), renderedImageBuffer, sizeof(renderedImageBuffer));
			}
			fclose(loadingScreenImage);

			if (conf->loadingFps == 0 || i == 29) break;
		}
	}

	conf->saveSize = getSaveSize(conf->savPath);

	conf->argc = 0;
	conf->argv = malloc(ARG_MAX);
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

	return 0;
}
