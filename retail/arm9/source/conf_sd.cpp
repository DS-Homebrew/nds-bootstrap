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
#include <easysave/ini.hpp>
#include "lzss.h"
#include "tonccpy.h"
#include "hex.h"
#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"
#include "nitrofs.h"
#include "locations.h"
#include "version.h"

static const char* twlmenuResetGamePath = "sdmc:/_nds/TWiLightMenu/resetgame.srldr";

extern std::string patchOffsetCacheFilePath;
extern std::string fatTableFilePath;
extern std::string wideCheatFilePath;
extern std::string cheatFilePath;
extern std::string ramDumpPath;
extern std::string srParamsFilePath;

extern u8 lz77ImageBuffer[0x12000];

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
extern bool extention(const std::string& filename, const char* ext);

static void load_conf(configuration* conf, const char* fn) {
	easysave::ini config_file(fn);

	// Debug
	conf->debug = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "DEBUG").c_str(), NULL, 0);

	// Cache FAT table
	conf->cacheFatTable = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "CACHE_FAT_TABLE").c_str(), NULL, 0);

	// NDS path
	conf->ndsPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "NDS_PATH").c_str());

	// SAV path
	conf->savPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "SAV_PATH").c_str());

	// Early SDK2 Donor NDS path
	conf->donorE2Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORE2_NDS_PATH").c_str());

	// Late SDK2 Donor NDS path
	conf->donor2Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONOR2_NDS_PATH").c_str());

	// SDK3-4 Donor NDS path
	conf->donor3Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONOR3_NDS_PATH").c_str());

	// SDK5 Donor NDS path
	conf->donorPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONOR_NDS_PATH").c_str());

	// SDK5 (TWL) Donor NDS path
	conf->donorTwlPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWL_NDS_PATH").c_str());

	// GBA path
	conf->gbaPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "GBA_PATH").c_str());

	// GBA SAV path
	conf->gbaSavPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "GBA_SAV_PATH").c_str());

	// AP-patch path
	conf->apPatchPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "AP_FIX_PATH").c_str());

	// Language
	conf->language = strtol(config_file.fetch("NDS-BOOTSTRAP", "LANGUAGE").c_str(), NULL, 0);

	// DSi mode
	conf->dsiMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "DSI_MODE").c_str(), NULL, 0);

	// Donor SDK version
	conf->donorSdkVer = strtol(config_file.fetch("NDS-BOOTSTRAP", "DONOR_SDK_VER").c_str(), NULL, 0);

	// Patch MPU region
	conf->patchMpuRegion = strtol(config_file.fetch("NDS-BOOTSTRAP", "PATCH_MPU_REGION").c_str(), NULL, 0);

	// Patch MPU size
	conf->patchMpuSize = strtol(config_file.fetch("NDS-BOOTSTRAP", "PATCH_MPU_SIZE").c_str(), NULL, 0);

	// Card engine (arm9) cached
	conf->ceCached = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "CARDENGINE_CACHED").c_str(), NULL, 0);

	// Extended memory
	conf->extendedMemory = strtol(config_file.fetch("NDS-BOOTSTRAP", "EXTENDED_MEMORY").c_str(), NULL, 0);

	// Console model
	conf->consoleModel = strtol(config_file.fetch("NDS-BOOTSTRAP", "CONSOLE_MODEL").c_str(), NULL, 0);

	// Color mode
	conf->colorMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "COLOR_MODE").c_str(), NULL, 0);

	// ROM read LED
	conf->romRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "ROMREAD_LED").c_str(), NULL, 0);

	// DMA ROM read LED
	conf->dmaRomRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "DMA_ROMREAD_LED").c_str(), NULL, 0);

	// Force sleep patch
	conf->forceSleepPatch = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH").c_str(), NULL, 0);

	// Precise volume control
	conf->preciseVolumeControl = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "PRECISE_VOLUME_CONTROL").c_str(), NULL, 0);

	// Logging
	conf->logging = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "LOGGING").c_str(), NULL, 0);

	// Macro mode
	conf->macroMode = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "MACRO_MODE").c_str(), NULL, 0);

	// Boost CPU
	// If DSi mode, then always boost CPU
	conf->dsiMode ? conf->boostCpu = true : conf->boostCpu = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_CPU").c_str(), NULL, 0);

	// Boost VRAM
	// If DSi mode, then always boost VRAM
	conf->dsiMode ? conf->boostVram = true : conf->boostVram = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_VRAM").c_str(), NULL, 0);

	// Sound/Mic frequency
	conf->soundFreq = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "SOUND_FREQ").c_str(), NULL, 0);
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
	conf->donorOnFlashcard = 0;
	if (conf->donorE2Path[0] == 'f' && conf->donorE2Path[1] == 'a' && conf->donorE2Path[2] == 't') {
		conf->donorOnFlashcard |= BIT(1);
	}
	if (conf->donor2Path[0] == 'f' && conf->donor2Path[1] == 'a' && conf->donor2Path[2] == 't') {
		conf->donorOnFlashcard |= BIT(2);
	}
	if (conf->donor3Path[0] == 'f' && conf->donor3Path[1] == 'a' && conf->donor3Path[2] == 't') {
		conf->donorOnFlashcard |= BIT(3);
	}
	if (conf->donorPath[0] == 'f' && conf->donorPath[1] == 'a' && conf->donorPath[2] == 't') {
		conf->donorOnFlashcard |= BIT(4);
	}
	if (conf->donorTwlPath[0] == 'f' && conf->donorTwlPath[1] == 'a' && conf->donorTwlPath[2] == 't') {
		conf->donorOnFlashcard |= BIT(5);
	}

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

	u32 srBackendId[2] = {0};
	// Load srBackendId
	FILE* cebin = fopen("sd:/_nds/nds-bootstrap/srBackendId.bin", "rb");
	if (cebin) {
		fread(&srBackendId, sizeof(u32), 2, cebin);
	}
	fclose(cebin);

	// Load ce7 binary
	cebin = fopen(conf->sdFound ? "nitro:/cardengine_arm7.lz77" : "nitro:/cardengine_arm7_alt.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM7_BUFFERED_LOCATION);
		tonccpy((u8*)LOADER_RETURN_LOCATION, twlmenuResetGamePath, 256);
		tonccpy((u8*)LOADER_RETURN_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

	// Load SDK5 ce7 binary
	cebin = fopen("nitro:/cardengine_arm7_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM7_SDK5_BUFFERED_LOCATION);
		tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION, twlmenuResetGamePath, 256);
		tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

    // Load ce9 binary
	cebin = fopen("nitro:/cardengine_arm9.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_BUFFERED_LOCATION);
	}
	fclose(cebin);

    // Load mem-cached ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_cached.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_CACHED_BUFFERED_LOCATION);
	}
	fclose(cebin);

    // Load reloc ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_reloc.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_RELOC_BUFFERED_LOCATION);
	}
	fclose(cebin);

    // Load ROMinRAM ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_romInRam.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x2000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_ROMINRAM_BUFFERED_LOCATION);
	}
	fclose(cebin);

    // Load in-game menu ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_igm.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x4000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)INGAME_MENU_LOCATION);

		tonccpy((char*)INGAME_MENU_LOCATION, VER_NUMBER, sizeof(VER_NUMBER));
	}
	fclose(cebin);

	// Load touch fix for SM64DS (U) v1.0
	cebin = fopen("nitro:/arm7fix.bin", "rb");
	if (cebin) {
		fread((u8*)ARM7_FIX_BUFFERED_LOCATION, 1, 0x140, cebin);
	}
	fclose(cebin);

	if (conf->gameOnFlashcard) {
		// Load DLDI ce9 binary
		cebin = fopen("nitro:/cardengine_arm9_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x5000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_DLDI_BUFFERED_LOCATION);
		}
		fclose(cebin);
	}

	// Load DS blowfish
	cebin = fopen("nitro:/encr_data.bin", "rb");
	if (cebin) {
		fread((void*)BLOWFISH_LOCATION, 1, 0x1048, cebin);
	}
	fclose(cebin);

	// Load SDK5 ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_SDK5_BUFFERED_LOCATION);
	}
	fclose(cebin);

	if (conf->gameOnFlashcard) {
		// Load SDK5 DLDI ce9 binary
		cebin = fopen("nitro:/cardengine_arm9_sdk5_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x7000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_SDK5_DLDI_BUFFERED_LOCATION);
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
	conf->gbaRomSize = getFileSize(conf->gbaPath);
	conf->gbaSaveSize = getFileSize(conf->gbaSavPath);
	conf->wideCheatSize = getFileSize(wideCheatFilePath.c_str());
	conf->apPatchSize = getFileSize(conf->apPatchPath);
	conf->cheatSize = getFileSize(cheatFilePath.c_str());

	bool wideCheatFound = (access(wideCheatFilePath.c_str(), F_OK) == 0);

	FILE* bootstrapImages = fopen(wideCheatFound&&!conf->macroMode ? "nitro:/bootloader_wideImages.lz77" : "nitro:/bootloader_images.lz77", "rb");
	if (bootstrapImages) {
		fread(lz77ImageBuffer, 1, 0x8000, bootstrapImages);
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION);
	}
	fclose(bootstrapImages);

	const char *typeToReplace = ".nds";
	if (extention(conf->ndsPath, ".dsi")) {
		typeToReplace = ".dsi";
	} else if (extention(conf->ndsPath, ".ids")) {
		typeToReplace = ".ids";
	} else if (extention(conf->ndsPath, ".srl")) {
		typeToReplace = ".srl";
	} else if (extention(conf->ndsPath, ".app")) {
		typeToReplace = ".app";
	}

	std::string romFilename = ReplaceAll(conf->ndsPath, typeToReplace, ".bin");
	const size_t last_slash_idx = romFilename.find_last_of("/");
	if (std::string::npos != last_slash_idx)
	{
		romFilename.erase(0, last_slash_idx + 1);
	}

	srParamsFilePath = "sd:/_nds/nds-bootstrap/softResetParams.bin";
	if (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't') {
		srParamsFilePath = "fat:/_nds/nds-bootstrap/softResetParams.bin";
	}
	
	if (access(srParamsFilePath.c_str(), F_OK) != 0) {
		u32 buffer = 0xFFFFFFFF;

		FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "wb");
		fwrite(&buffer, sizeof(u32), 1, srParamsFile);
		fclose(srParamsFile);
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

	if (conf->cacheFatTable && getFileSize(fatTableFilePath.c_str()) < 0x80180) {
		consoleDemoInit();
		printf("Creating FAT table file.\n");
		printf("Please wait...\n");

		FILE *fatTableFile = fopen(fatTableFilePath.c_str(), "wb");
		if (fatTableFile) {
			fseek(fatTableFile, 0x80200 - 1, SEEK_SET);
			fputc('\0', fatTableFile);
			fclose(fatTableFile);
		}

		consoleClear();
	}

	ramDumpPath = "sd:/_nds/nds-bootstrap/ramDump.bin";
	/*if (!conf->sdFound) {
		ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";
	}*/

	if (conf->sdFound && access(ramDumpPath.c_str(), F_OK) != 0) {
		consoleDemoInit();
		printf("Creating RAM dump file.\n");
		printf("Please wait...\n");
		/* printf("\n");
		if (conf->consoleModel >= 2) {
			printf("If this takes a while, press\n");
			printf("HOME, then press B.\n");
		} else {
			printf("If this takes a while, close\n");
			printf("the lid, and open it again.\n");
		} */

		FILE *ramDumpFile = fopen(ramDumpPath.c_str(), "wb");
		if (ramDumpFile) {
			fseek(ramDumpFile, 0x02000000 - 1, SEEK_SET);
			fputc('\0', ramDumpFile);
			fclose(ramDumpFile);
		}

		consoleClear();
	}

	return 0;
}