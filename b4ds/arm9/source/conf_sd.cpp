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
#include "lzss.h"
#include <easykey.h>
#include "hex.h"
#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"
#include "nitrofs.h"
#include "locations.h"

extern std::string patchOffsetCacheFilePath;

extern u8 lz77ImageBuffer[0x10000];

static off_t getFileSize(const char* path) {
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

	// AP-patch path
	Key.Data = (char*)"";
	Key.Name = (char*)"AP_FIX_PATH";
	iniGetKey(IniData, IniCount, &Key);
	conf->apPatchPath = strdup(Key.Data);

	// Language
	Key.Data = (char*)"";
	Key.Name = (char*)"LANGUAGE";
	iniGetKey(IniData, IniCount, &Key);
	conf->language = strtol(Key.Data, NULL, 0);

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

	// Force sleep patch
	Key.Data = (char*)"";
	Key.Name = (char*)"FORCE_SLEEP_PATCH";
	iniGetKey(IniData, IniCount, &Key);
	conf->forceSleepPatch = (bool)strtol(Key.Data, NULL, 0);

	// Logging
	Key.Data = (char*)"";
	Key.Name = (char*)"LOGGING";
	iniGetKey(IniData, IniCount, &Key);
	conf->logging = (bool)strtol(Key.Data, NULL, 0);

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
		printf("FAT init failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	if ((access("fat:/", F_OK) != 0) && (access("sd:/", F_OK) == 0)) {
		consoleDemoInit();
		printf("This edition of nds-bootstrap:\n");
		printf("B4DS, can only be used\n");
		printf("on a flashcard.\n");
		return -1;
	}
	
	load_conf(conf, "fat:/_nds/nds-bootstrap.ini");
	mkdir("fat:/_nds", 0777);
	mkdir("fat:/_nds/nds-bootstrap", 0777);
	mkdir("fat:/_nds/nds-bootstrap/B4DS-patchOffsetCache", 0777);

	if (!nitroFSInit(bootstrapPath)) {
		consoleDemoInit();
		printf("nitroFSInit failed!\n");
		return -1;
	}
	
	// Load ce7 binary
	FILE* cebin = fopen("nitro:/cardengine_arm7.bin", "rb");
	if (cebin) {
		fread((void*)CARDENGINE_ARM7_LOCATION, 1, 0x800, cebin);
	}
	fclose(cebin);
    
	// Load ce9 binary 1
	cebin = fopen("nitro:/cardengine_arm9.bin", "rb");
	if (cebin) {
		fread((void*)CARDENGINE_ARM9_LOCATION_BUFFERED1, 1, 0x5000, cebin);
	}
	fclose(cebin);
    
	// Load ce9 binary 2
	cebin = fopen("nitro:/cardengine_arm9_8kb.bin", "rb");
	if (cebin) {
		fread((void*)CARDENGINE_ARM9_LOCATION_BUFFERED2, 1, 0x4000, cebin);
	}
	fclose(cebin);
    
	conf->romSize = getFileSize(conf->ndsPath);
	conf->saveSize = getFileSize(conf->savPath);
	conf->apPatchSize = getFileSize(conf->apPatchPath);

	FILE* bootstrapImages = fopen("nitro:/bootloader_images.lz77", "rb");
	if (bootstrapImages) {
		fread(lz77ImageBuffer, 1, 0x8000, bootstrapImages);
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION);
	}
	fclose(bootstrapImages);

	std::string romFilename = ReplaceAll(conf->ndsPath, ".nds", ".bin");
	const size_t last_slash_idx = romFilename.find_last_of("/");
	if (std::string::npos != last_slash_idx)
	{
		romFilename.erase(0, last_slash_idx + 1);
	}

	patchOffsetCacheFilePath = "fat:/_nds/nds-bootstrap/B4DS-patchOffsetCache/"+romFilename;
	
	if (access(patchOffsetCacheFilePath.c_str(), F_OK) != 0) {
		char buffer[0x200] = {0};

		FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath.c_str(), "wb");
		fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
		fclose(patchOffsetCacheFile);
	}

	return 0;
}
