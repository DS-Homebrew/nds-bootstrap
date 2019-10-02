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
#include "locations.h"

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

	if (!nitroFSInit(bootstrapPath)) {
		consoleDemoInit();
		printf("nitroFSInit failed!\n");
		return -1;
	}
	
	// Load ce7 binary
	FILE* cebin = fopen("nitro:/cardengine_arm7.bin", "rb");
	if (cebin) {
		fread((void*)CARDENGINE_ARM7_LOCATION_BUFFER, 1, 0x10000, cebin);
	}
	fclose(cebin);
    
	conf->saveSize = getSaveSize(conf->savPath);

	return 0;
}
