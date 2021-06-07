#include <stdlib.h> // strtol
#include <unistd.h>
//#include <stdio.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <string>
#include <string.h>
#include <limits.h> // PATH_MAX
/*#include <nds/ndstypes.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>
#include <nds/debug.h>*/
#include <fat.h>
#include <easysave/ini.hpp>
#include "myDSiMode.h"
#include "lzss.h"
#include "text.h"
#include "tonccpy.h"
#include "hex.h"
#include "cardengine_header_arm7.h"
#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"
#include "nitrofs.h"
#include "igm_text.h"
#include "locations.h"
#include "version.h"

#include "f_xy.h"
#include "dsi.h"
#include "u128_math.h"

void decrypt_modcrypt_area(dsi_context* ctx, u8 *buffer, unsigned int size)
{
	uint32_t len = size / 0x10;
	u8 block[0x10];

	while(len>0)
	{
		toncset(block, 0, 0x10);
		dsi_crypt_ctr_block(ctx, buffer, block);
		tonccpy(buffer, block, 0x10);
		buffer+=0x10;
		len--;
	}
}

static const char* twlmenuResetGamePath = "sdmc:/_nds/TWiLightMenu/resetgame.srldr";

extern const DISC_INTERFACE __my_io_dsisd;

extern std::string patchOffsetCacheFilePath;
extern std::string fatTableFilePath;
extern std::string wideCheatFilePath;
extern std::string cheatFilePath;
extern std::string ramDumpPath;
extern std::string srParamsFilePath;

extern u8 lz77ImageBuffer[0x20000];

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
	conf->debug = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "DEBUG", "0").c_str(), NULL, 0);

	// Cache FAT table
	conf->cacheFatTable = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "CACHE_FAT_TABLE", "0").c_str(), NULL, 0);

	// NDS path
	conf->ndsPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "NDS_PATH").c_str());

	// APP path (SFN version of NDS path)
	conf->appPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "APP_PATH").c_str());

	// SAV/PUB path
	conf->savPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "SAV_PATH").c_str());

	// PRV path
	conf->prvPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "PRV_PATH").c_str());

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
	conf->language = strtol(config_file.fetch("NDS-BOOTSTRAP", "LANGUAGE", "-1").c_str(), NULL, 0);

	// Region
	conf->region = strtol(config_file.fetch("NDS-BOOTSTRAP", "REGION", "-2").c_str(), NULL, 0);

	if (dsiFeatures()) {
		// DSi mode
		conf->dsiMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "DSI_MODE", "1").c_str(), NULL, 0);
	}

	// Donor SDK version
	conf->donorSdkVer = strtol(config_file.fetch("NDS-BOOTSTRAP", "DONOR_SDK_VER", "0").c_str(), NULL, 0);

	// Patch MPU region
	conf->patchMpuRegion = strtol(config_file.fetch("NDS-BOOTSTRAP", "PATCH_MPU_REGION", "0").c_str(), NULL, 0);

	// Patch MPU size
	conf->patchMpuSize = strtol(config_file.fetch("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", "0").c_str(), NULL, 0);

	// Extended memory
	conf->extendedMemory = strtol(config_file.fetch("NDS-BOOTSTRAP", "EXTENDED_MEMORY", "0").c_str(), NULL, 0);

	// Console model
	conf->consoleModel = strtol(config_file.fetch("NDS-BOOTSTRAP", "CONSOLE_MODEL", "0").c_str(), NULL, 0);

	// Color mode
	conf->colorMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "COLOR_MODE", "0").c_str(), NULL, 0);

	// ROM read LED
	conf->romRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "ROMREAD_LED", "0").c_str(), NULL, 0);

	// DMA ROM read LED
	conf->dmaRomRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "DMA_ROMREAD_LED", "0").c_str(), NULL, 0);

	// Card read DMA
	conf->cardReadDMA = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "CARD_READ_DMA", "1").c_str(), NULL, 0);

	// Force sleep patch
	conf->forceSleepPatch = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", "0").c_str(), NULL, 0);

	// Precise volume control
	conf->preciseVolumeControl = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "PRECISE_VOLUME_CONTROL", "0").c_str(), NULL, 0);

	// Logging
	conf->logging = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "LOGGING", "0").c_str(), NULL, 0);

	// Macro mode
	conf->macroMode = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "MACRO_MODE", "0").c_str(), NULL, 0);

	// Boost CPU
	conf->boostCpu = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_CPU", "0").c_str(), NULL, 0);

	// Boost VRAM
	conf->boostVram = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_VRAM", "0").c_str(), NULL, 0);

	// Sound/Mic frequency
	conf->soundFreq = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "SOUND_FREQ", "0").c_str(), NULL, 0);

	// GUI Language
	conf->guiLanguage = strdup(config_file.fetch("NDS-BOOTSTRAP", "GUI_LANGUAGE").c_str());

	// Hotkey
	conf->hotkey = strtol(config_file.fetch("NDS-BOOTSTRAP", "HOTKEY").c_str(), NULL, 16);
}

int loadFromSD(configuration* conf, const char *bootstrapPath) {
	fatMountSimple("sd", &__my_io_dsisd);
	fatMountSimple("fat", dldiGetInternal());

	conf->sdFound = (access("sd:/", F_OK) == 0);
	bool flashcardFound = (access("fat:/", F_OK) == 0);

	if (!conf->sdFound && !flashcardFound) {
		consoleDemoInit();
		printf("FAT init failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	if ((strncmp (bootstrapPath, "sd:/", 4) != 0) && (strncmp (bootstrapPath, "fat:/", 5) != 0)) {
		//bootstrapPath = "sd:/_nds/nds-bootstrap-release.nds";
		bootstrapPath = conf->sdFound ? "sd:/_nds/nds-bootstrap-nightly.nds" : "fat:/_nds/nds-bootstrap-nightly.nds";
	}
	if (!nitroFSInit(bootstrapPath)) {
		consoleDemoInit();
		printf("nitroFSInit failed!\n");
		return -1;
	}
	
	load_conf(conf, conf->sdFound ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");

	conf->gameOnFlashcard = (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't');
	conf->saveOnFlashcard = (conf->savPath[0] == 'f' && conf->savPath[1] == 'a' && conf->savPath[2] == 't');

	if (conf->cacheFatTable) {
		conf->valueBits |= BIT(0);
	}
	if (conf->boostVram) {
		conf->valueBits |= BIT(1);
	}
	if (conf->forceSleepPatch) {
		conf->valueBits |= BIT(2);
	}
	if (conf->preciseVolumeControl) {
		conf->valueBits |= BIT(4);
	}
	if (extention(conf->apPatchPath, ".bin")) {
		conf->valueBits |= BIT(5);
	}
	if (conf->macroMode) {
		conf->valueBits |= BIT(6);
	}
	if (conf->logging) {
		conf->valueBits |= BIT(7);
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

	char romTid[5] = {0};
	u8 unitCode = 0;
	u32 ndsArm7Size = 0;
	u32 accessControl = 0;
	u32 ndsArm9isrc = 0;
	u32 ndsArm9idst = 0;
	u32 ndsArm9ilen = 0;
	u32 ndsArm7isrc = 0;
	u32 ndsArm7idst = 0;
	u32 ndsArm7ilen = 0;
	u32 modcrypt1len = 0;
	u32 modcrypt2len = 0;
	FILE* ndsFile = fopen(conf->ndsPath, "rb");
	if (ndsFile) {
		fseek(ndsFile, 0xC, SEEK_SET);
		fread(&romTid, 1, 4, ndsFile);
		fseek(ndsFile, 0x12, SEEK_SET);
		fread(&unitCode, 1, 1, ndsFile);
		fseek(ndsFile, 0x3C, SEEK_SET);
		fread(&ndsArm7Size, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1B4, SEEK_SET);
		fread(&accessControl, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1C0, SEEK_SET);
		fread(&ndsArm9isrc, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1C8, SEEK_SET);
		fread(&ndsArm9idst, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1CC, SEEK_SET);
		fread(&ndsArm9ilen, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1D0, SEEK_SET);
		fread(&ndsArm7isrc, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1D8, SEEK_SET);
		fread(&ndsArm7idst, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1DC, SEEK_SET);
		fread(&ndsArm7ilen, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x224, SEEK_SET);
		fread(&modcrypt1len, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x22C, SEEK_SET);
		fread(&modcrypt2len, sizeof(u32), 1, ndsFile);
	}

	FILE* cebin = NULL;
	bool donorLoaded = false;
	conf->isDSiWare = (dsiFeatures() && ((unitCode == 3 && (accessControl & BIT(4)))
					|| (unitCode == 2 && conf->dsiMode && romTid[0] == 'K')));

	if (dsiFeatures()) {
		bool hasCycloDSi = (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0);

		// Load donor ROM's arm7 binary, if needed
		switch (ndsArm7Size) {
			case 0x22B40:
			case 0x22BCC:
				if (hasCycloDSi) cebin = fopen(conf->donorTwlPath, "rb");
				break;
			case 0x23708:
			case 0x2378C:
			case 0x237F0:
				if (hasCycloDSi) cebin = fopen(conf->donorPath, "rb");
				break;
			case 0x2352C:
			case 0x235DC:
			case 0x23CAC:
				if (hasCycloDSi) cebin = fopen(conf->donorE2Path, "rb");
				break;
			case 0x245C4:
			case 0x24DA8:
			case 0x24F50:
				cebin = fopen(conf->donor2Path, "rb");
				break;
			case 0x2434C:
			case 0x2484C:
			case 0x249DC:
			case 0x25D04:
			case 0x25D94:
			case 0x25FFC:
				if (hasCycloDSi) cebin = fopen(conf->donor3Path, "rb");
				break;
			case 0x27618:
			case 0x2762C:
			case 0x29CEC:
				cebin = fopen(conf->donorPath, "rb");
				break;
			default:
				break;
		}

		if (cebin) {
			u32 donorArm7Offset = 0;
			fseek(cebin, 0x30, SEEK_SET);
			fread(&donorArm7Offset, sizeof(u32), 1, cebin);
			fseek(cebin, 0x3C, SEEK_SET);
			fread((u32*)DONOR_ROM_ARM7_SIZE_LOCATION, sizeof(u32), 1, cebin);
			fseek(cebin, donorArm7Offset, SEEK_SET);
			fread((u8*)DONOR_ROM_ARM7_LOCATION, 1, *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION, cebin);
			fclose(cebin);
			donorLoaded = true;
		}
	}

  if (dsiFeatures()) {
	if (!conf->gameOnFlashcard && !conf->saveOnFlashcard && strncmp(romTid, "I", 1) != 0) {
		disableSlot1();
	}

	if ((conf->dsiMode > 0 && unitCode > 0) || conf->isDSiWare) {
		if (ndsArm9ilen) {
			fseek(ndsFile, ndsArm9isrc, SEEK_SET);
			fread((u32*)ndsArm9idst, 1, ndsArm9ilen, ndsFile);
		}
		if (ndsArm7ilen) {
			fseek(ndsFile, ndsArm7isrc, SEEK_SET);
			fread((u32*)ndsArm7idst, 1, ndsArm7ilen, ndsFile);
		}
		uint8_t *target = (uint8_t *)TARGETBUFFERHEADER ;
		fseek(ndsFile, 0, SEEK_SET);
		fread(target, 1, 0x1000, ndsFile);

		if (target[0x01C] & 2)
		{
			u8 key[16] = {0} ;
			u8 keyp[16] = {0} ;
			if (target[0x01C] & 4)
			{
				// Debug Key
				tonccpy(key, target, 16) ;
			} else
			{
				//Retail key
				char modcrypt_shared_key[8] = {'N','i','n','t','e','n','d','o'};
				tonccpy(keyp, modcrypt_shared_key, 8) ;
				for (int i=0;i<4;i++)
				{
					keyp[8+i] = target[0x0c+i] ;
					keyp[15-i] = target[0x0c+i] ;
				}
				tonccpy(key, target+0x350, 16) ;
				
				u128_xor(key, keyp);
				u128_add(key, DSi_KEY_MAGIC);
		  u128_lrot(key, 42) ;
			}

			uint32_t rk[4];
			tonccpy(rk, key, 16) ;
			
			dsi_context ctx;
			dsi_set_key(&ctx, key);
			dsi_set_ctr(&ctx, &target[0x300]);
			if (modcrypt1len)
			{
				decrypt_modcrypt_area(&ctx, (u8*)ndsArm9idst, modcrypt1len);
			}

			dsi_set_key(&ctx, key);
			dsi_set_ctr(&ctx, &target[0x314]);
			if (modcrypt2len)
			{
				decrypt_modcrypt_area(&ctx, (u8*)ndsArm7idst, modcrypt2len);
			}
		}
	}
	fclose(ndsFile);

	u32 srBackendId[2] = {0};
	// Load srBackendId
	cebin = fopen("sd:/_nds/nds-bootstrap/srBackendId.bin", "rb");
	if (cebin) {
		fread(&srBackendId, sizeof(u32), 2, cebin);
	}
	fclose(cebin);

	if (isDSiMode() && !donorLoaded && unitCode > 0) {
		// Load device list
		cebin = fopen("nitro:/deviceList.bin", "rb");
		if (cebin) {
			fread((u8*)0x02EFF000, 1, 0x400, cebin);
			if (!conf->gameOnFlashcard) {
				if (strlen(conf->appPath) < 62) {
					tonccpy((char*)0x02EFF3C2, conf->appPath, strlen(conf->appPath));
					*(char*)0x02EFF3C2 = 'm';
					*(char*)0x02EFF3C3 = 'c';
				}
				if (strlen(conf->prvPath) < 62) {
					tonccpy((char*)0x02EFF20E, conf->prvPath, strlen(conf->prvPath));
					*(char*)0x02EFF20E = 'm';
					*(char*)0x02EFF20F = 'c';
				}
				if (strlen(conf->savPath) < 62) {
					tonccpy((char*)0x02EFF262, conf->savPath, strlen(conf->savPath));
					*(char*)0x02EFF262 = 'm';
					*(char*)0x02EFF263 = 'c';
				}
			}
		}
		fclose(cebin);
	}

  if (conf->gameOnFlashcard || !conf->isDSiWare) {
	// Load ce7 binary
	cebin = fopen(conf->sdFound ? "nitro:/cardenginei_arm7.lz77" : "nitro:/cardenginei_arm7_alt.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION);
		tonccpy((u8*)LOADER_RETURN_LOCATION, twlmenuResetGamePath, 256);
		tonccpy((u8*)LOADER_RETURN_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

	// Load SDK5 ce7 binary
	cebin = fopen("nitro:/cardenginei_arm7_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_SDK5_BUFFERED_LOCATION);
		tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION, twlmenuResetGamePath, 256);
		tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_BUFFERED_LOCATION, 1, 0x400, cebin);
	}
	fclose(cebin);

    // Load ce9 binary
	cebin = fopen("nitro:/cardenginei_arm9.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
	}
	fclose(cebin);

    // Load mem-cached ce9 binary
	cebin = fopen(strncmp(romTid, "ADM", 3)==0||strncmp(romTid, "A62", 3)==0 ? "nitro:/cardenginei_arm9_cached_end.lz77" : "nitro:/cardenginei_arm9_cached.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_CACHED_BUFFERED_LOCATION);
	}
	fclose(cebin);

    // Load ROMinRAM ce9 binary
	cebin = fopen("nitro:/cardenginei_arm9_romInRam.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x2000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_ROMINRAM_BUFFERED_LOCATION);
	}
	fclose(cebin);

	// Load in-game menu ce9 binary
	cebin = fopen("nitro:/cardenginei_arm9_igm.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x4000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)INGAME_MENU_LOCATION);

		// Set In-Game Menu strings
		tonccpy(igmText->version, VER_NUMBER, sizeof(VER_NUMBER));
		tonccpy(igmText->ndsBootstrap, u"nds-bootstrap", 28);
		igmText->rtl = strcmp(conf->guiLanguage, "he") == 0;

		// Set In-Game Menu hotkey
		igmText->hotkey = conf->hotkey != 0 ? conf->hotkey : (KEY_L | KEY_DOWN | KEY_SELECT);

		cardengineArm7* ce7 = (cardengineArm7*)CARDENGINEI_ARM7_BUFFERED_LOCATION;
		cardengineArm7* ce7sdk5 = (cardengineArm7*)CARDENGINEI_ARM7_SDK5_BUFFERED_LOCATION;
		ce7->igmHotkey = igmText->hotkey;
		ce7sdk5->igmHotkey = igmText->hotkey;

		char path[40];
		snprintf(path, sizeof(path), "nitro:/languages/%s/in_game_menu.ini", conf->guiLanguage);
		easysave::ini lang(path);

		setIgmString(lang.fetch("TITLES", "RAM_VIEWER", "RAM Viewer").c_str(), igmText->ramViewer);
		setIgmString(lang.fetch("TITLES", "JUMP_ADDRESS", "Jump to Address").c_str(), igmText->jumpAddress);

		setIgmString(lang.fetch("MENU", "RETURN_TO_GAME", "Return to Game").c_str(), igmText->menu[0]);
		setIgmString(lang.fetch("MENU", "RESET_GAME", "Reset Game").c_str(), igmText->menu[1]);
		setIgmString(lang.fetch("MENU", "DUMP_RAM", "Dump RAM").c_str(), igmText->menu[2]);
		setIgmString(lang.fetch("MENU", "OPTIONS", "Options...").c_str(), igmText->menu[3]);
		// setIgmString(lang.fetch("MENU", "CHEATS", "Cheats...").c_str(), igmText->menu[4]);
		setIgmString(lang.fetch("MENU", "RAM_VIEWER", "RAM Viewer...").c_str(), igmText->menu[4]);
		setIgmString(lang.fetch("MENU", "QUIT_GAME", "Quit Game").c_str(), igmText->menu[5]);

		setIgmString(lang.fetch("OPTIONS", "MAIN_SCREEN", "Main Screen").c_str(), igmText->options[0]);
		setIgmString(lang.fetch("OPTIONS", "CLOCK_SPEED", "Clock Speed").c_str(), igmText->options[1]);
		setIgmString(lang.fetch("OPTIONS", "VRAM_BOOST", "VRAM Boost").c_str(), igmText->options[2]);
		setIgmString(lang.fetch("OPTIONS", "AUTO", "Auto").c_str(), igmText->options[3]);
		setIgmString(lang.fetch("OPTIONS", "BOTTOM", "Bottom").c_str(), igmText->options[4]);
		setIgmString(lang.fetch("OPTIONS", "TOP", "Top").c_str(), igmText->options[5]);
		setIgmString(lang.fetch("OPTIONS", "67_MHZ", "67 MHz").c_str(), igmText->options[6]);
		setIgmString(lang.fetch("OPTIONS", "133_MHZ", "133 MHz").c_str(), igmText->options[7]);
		setIgmString(lang.fetch("OPTIONS", "OFF", "Off").c_str(), igmText->options[8]);
		setIgmString(lang.fetch("OPTIONS", "ON", "On").c_str(), igmText->options[9]);
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
		cebin = fopen("nitro:/cardenginei_arm9_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x5000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_DLDI_BUFFERED_LOCATION);
		}
		fclose(cebin);
	}

	// Load SDK5 ce9 binary
	cebin = fopen(unitCode>0&&conf->dsiMode ? "nitro:/cardenginei_arm9_twlsdk.lz77" : "nitro:/cardenginei_arm9_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION);
	}
	fclose(cebin);

	if (conf->gameOnFlashcard) {
		// Load SDK5 DLDI ce9 binary
		cebin = fopen(unitCode>0&&conf->dsiMode ? "nitro:/cardenginei_arm9_twlsdk_dldi.lz77" : "nitro:/cardenginei_arm9_sdk5_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x7000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_SDK5_DLDI_BUFFERED_LOCATION);
		}
		fclose(cebin);
	}
  } else {
	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheatonly.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_BUFFERED_LOCATION, 1, 0x400, cebin);
	}
	fclose(cebin);
  }

	// Load DS blowfish
	cebin = fopen("nitro:/encr_data.bin", "rb");
	if (cebin) {
		fread((void*)BLOWFISH_LOCATION, 1, 0x1048, cebin);
	}
	fclose(cebin);

	if (!isDSiMode() && unitCode>0 && conf->dsiMode) {
		// Load DSi ARM7 BIOS
		cebin = fopen("sd:/_nds/bios7i.bin", "rb");
		if (cebin) {
			fread((u32*)0x02ED0000, 1, 0x10000, cebin);

			// Relocate addresses
			*(u32*)0x02ED58A8 += 0x02ED0000;
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

  } else {
	fclose(ndsFile);

	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_LOCATION_B4DS, 1, 0x400, cebin);
	}
	fclose(cebin);

	// Load ce7 binary
	cebin = fopen("nitro:/cardengine_arm7.bin", "rb");
	if (cebin) {
		fread((void*)CARDENGINE_ARM7_LOCATION, 1, 0x800, cebin);
	}
	fclose(cebin);

	*(vu32*)(0x02000000) = 0x314D454D;
	*(vu32*)(0x02400000) = 0x324D454D;

	// Load ce9 binary
	if ((*(vu32*)(0x02000000) == 0x314D454D) && (*(vu32*)(0x02400000) == 0x324D454D)) {
		cebin = fopen("nitro:/cardengine_arm9_extmem.lz77", "rb");
	} else {
		cebin = fopen("nitro:/cardengine_arm9.lz77", "rb");
	}
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x7000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINE_ARM9_LOCATION_BUFFERED);
	}
	fclose(cebin);

	// Load DS blowfish
	cebin = fopen("nitro:/encr_data.bin", "rb");
	if (cebin) {
		fread((void*)BLOWFISH_LOCATION_B4DS, 1, 0x1048, cebin);
	}
	fclose(cebin);

	cheatFilePath = "fat:/_nds/nds-bootstrap/cheatData.bin";

	conf->romSize = getFileSize(conf->ndsPath);
	conf->saveSize = getFileSize(conf->savPath);
	conf->apPatchSize = getFileSize(conf->apPatchPath);
	conf->cheatSize = getFileSize(cheatFilePath.c_str());

	FILE* bootstrapImages = fopen("nitro:/bootloader_images.lz77", "rb");
	if (bootstrapImages) {
		fread(lz77ImageBuffer, 1, 0x8000, bootstrapImages);
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION);
	}
	fclose(bootstrapImages);
  }

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

	if (dsiFeatures()) {	// Not for B4DS
		fatTableFilePath = "sd:/_nds/nds-bootstrap/fatTable/"+romFilename;
		if (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't') {
			fatTableFilePath = "fat:/_nds/nds-bootstrap/fatTable/"+romFilename;
		}

		if ((conf->gameOnFlashcard || !conf->isDSiWare) && conf->cacheFatTable && getFileSize(fatTableFilePath.c_str()) < 0x80180) {
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
		if (!conf->sdFound) {
			ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";
		}

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
	}

	return 0;
}