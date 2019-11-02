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
#include "lzss.h"
#include "tonccpy.h"
#include "hex.h"
#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"
#include "nitrofs.h"
#include "locations.h"

extern std::string patchOffsetCacheFilePath;
extern std::string fatTableFilePath;
extern std::string wideCheatFilePath;
extern std::string cheatFilePath;
extern std::string ramDumpPath;

extern u8 lz77ImageBuffer[0x10000];

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

	// Sound/Mic frequency
	Key.Data = (char*)"";
	Key.Name = (char*)"SOUND_FREQ";
	iniGetKey(IniData, IniCount, &Key);
	conf->soundFreq = (bool)strtol(Key.Data, NULL, 0);

	iniFree(IniData, IniCount);
}

int loadFromSD(configuration* conf, const char *bootstrapPath) {
	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("fatInitDefault failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	if (!isDSiMode()) {
		consoleDemoInit();
		printf("This edition of nds-bootstrap\n");
		printf("can only be used in DSi mode.\n");
		return -1;
	}

	if ((strncmp (bootstrapPath, "sd:/", 4) != 0) && (strncmp (bootstrapPath, "fat:/", 5) != 0)) {
		//bootstrapPath = "sd:/_nds/nds-bootstrap-release.nds";
		bootstrapPath = "sd:/_nds/nds-bootstrap-nightly.nds";
	}
	if (!nitroFSInit(bootstrapPath)) {
		consoleDemoInit();
		printf("nitroFSInit failed!\n");
		return -1;
	}
	
	conf->sdFound = (access("sd:/", F_OK) == 0);
	bool flashcardFound = (access("fat:/", F_OK) == 0);

	load_conf(conf, conf->sdFound ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");

	conf->gameOnFlashcard = (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't');
	conf->saveOnFlashcard = (conf->savPath[0] == 'f' && conf->savPath[1] == 'a' && conf->savPath[2] == 't');

	if (conf->sdFound) {
		mkdir("sd:/_nds", 0777);
		mkdir("sd:/_nds/nds-bootstrap", 0777);
		mkdir("sd:/_nds/nds-bootstrap/patchOffsetCache", 0777);
		mkdir("sd:/_nds/nds-bootstrap/fatTable", 0777);
	}
	if (flashcardFound) {
		mkdir("fat:/_nds", 0777);
		mkdir("fat:/_nds/nds-bootstrap", 0777);
		mkdir("fat:/_nds/nds-bootstrap/patchOffsetCache", 0777);
		mkdir("fat:/_nds/nds-bootstrap/fatTable", 0777);
	}

	// Load ce7 binary
	FILE* cebin = fopen(conf->sdFound ? "nitro:/cardengine_arm7.lz77" : "nitro:/cardengine_arm7_alt.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM7_BUFFERED_LOCATION);
		//fread((void*)CARDENGINE_ARM7_BUFFERED_LOCATION, 1, 0x12000, cebin);
	}
	fclose(cebin);
    
	// Load SDK5 ce7 binary
	cebin = fopen("nitro:/cardengine_arm7_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM7_SDK5_BUFFERED_LOCATION);
		//fread((void*)CARDENGINE_ARM7_SDK5_BUFFERED_LOCATION, 1, 0x12000, cebin);
	}
	fclose(cebin);
    
    // Load ce9 binary
	cebin = fopen("nitro:/cardengine_arm9.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_BUFFERED_LOCATION);
		//fread((void*)CARDENGINE_ARM9_BUFFERED_LOCATION, 1, 0x3000, cebin);
	}
	fclose(cebin);

    // Load reloc ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_reloc.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_RELOC_BUFFERED_LOCATION);
		//fread((void*)CARDENGINE_ARM9_RELOC_BUFFERED_LOCATION, 1, 0x3000, cebin);
	}
	fclose(cebin);

	if (conf->gameOnFlashcard) {
		// Load DLDI ce9 binary
		cebin = fopen("nitro:/cardengine_arm9_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x7000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_DLDI_BUFFERED_LOCATION);
			//fread((void*)CARDENGINE_ARM9_DLDI_BUFFERED_LOCATION, 1, 0x7000, cebin);
		}
		fclose(cebin);
	}

	// Load DSi blowfish
	cebin = fopen("nitro:/dsi_encr_data.bin", "rb");
	if (cebin) {
		fread((void*)DSI_BLOWFISH_LOCATION, 1, 0x1048, cebin);
	}
	fclose(cebin);

	// Load SDK5 ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_SDK5_BUFFERED_LOCATION);
		//fread((void*)CARDENGINE_ARM9_SDK5_BUFFERED_LOCATION, 1, 0x3000, cebin);
	}
	fclose(cebin);

	if (conf->gameOnFlashcard) {
		// Load SDK5 DLDI ce9 binary
		cebin = fopen("nitro:/cardengine_arm9_sdk5_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x7000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_SDK5_DLDI_BUFFERED_LOCATION);
			//fread((void*)CARDENGINE_ARM9_SDK5_DLDI_BUFFERED_LOCATION, 1, 0x7000, cebin);
		}
		fclose(cebin);
	}

	if (conf->gameOnFlashcard) {
		wideCheatFilePath = "fat:/_nds/nds-bootstrap/wideCheatData.bin";
		cheatFilePath = "fat:/_nds/nds-bootstrap/cheatData.bin";
	} else {
		wideCheatFilePath = "sd:/_nds/nds-bootstrap/wideCheatData.bin";
		cheatFilePath = "sd:/_nds/nds-bootstrap/cheatData.bin";
	}

	conf->romSize = getFileSize(conf->ndsPath);
	conf->saveSize = getFileSize(conf->savPath);
	conf->wideCheatSize = getFileSize(wideCheatFilePath.c_str());
	conf->apPatchSize = getFileSize(conf->apPatchPath);
	conf->cheatSize = getFileSize(cheatFilePath.c_str());

	bool wideCheatFound = (access(wideCheatFilePath.c_str(), F_OK) == 0);

	FILE* bootstrapImages = fopen(wideCheatFound ? "nitro:/bootloader_wideImages.lz77" : "nitro:/bootloader_images.lz77", "rb");
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

	patchOffsetCacheFilePath = "sd:/_nds/nds-bootstrap/patchOffsetCache/"+romFilename;
	if (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't') {
		patchOffsetCacheFilePath = "fat:/_nds/nds-bootstrap/patchOffsetCache/"+romFilename;
	}
	
	if (access(patchOffsetCacheFilePath.c_str(), F_OK) != 0) {
		char buffer[0x200] = {0};

		FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath.c_str(), "wb");
		fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
		fclose(patchOffsetCacheFile);
	}

	fatTableFilePath = "sd:/_nds/nds-bootstrap/fatTable/"+romFilename;
	if (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't') {
		fatTableFilePath = "fat:/_nds/nds-bootstrap/fatTable/"+romFilename;
	}

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

	ramDumpPath = "sd:/_nds/nds-bootstrap/ramDump.bin";
	/*if (!conf->sdFound) {
		ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";
	}*/

	if (conf->sdFound && access(ramDumpPath.c_str(), F_OK) != 0) {
		static const int BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		toncset(buffer, 0, sizeof(buffer));

		FILE *ramDumpFile = fopen(ramDumpPath.c_str(), "wb");
		for (u32 i = 0x02000000; i > 0; i -= BUFFER_SIZE) {
			fwrite(buffer, 1, sizeof(buffer), ramDumpFile);
		}
		fclose(ramDumpFile);
	}

	return 0;
}
