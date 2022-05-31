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

#define REG_SCFG_EXT7 *(u32*)0x02FFFDF0

#define twlPageFileSize 0x600000

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION;

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
extern std::string wideCheatFilePath;
extern std::string cheatFilePath;
extern std::string ramDumpPath;
extern std::string srParamsFilePath;
extern std::string screenshotPath;
extern std::string pageFilePath;

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

	// B4DS mode
	conf->b4dsMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "B4DS_MODE", "0").c_str(), NULL, 0);

	// NDS path
	conf->ndsPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "NDS_PATH").c_str());

	// APP path (SFN version of NDS path)
	conf->appPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "APP_PATH").c_str());

	// SAV/PUB path
	conf->savPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "SAV_PATH").c_str());

	// PRV path
	conf->prvPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "PRV_PATH").c_str());

	// SDK5.0 (TWL) DSi-Enhanced Donor NDS path
	conf->donorTwl0Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWL0_NDS_PATH").c_str());

	// SDK5.x (TWL) DSi-Enhanced Donor NDS path
	conf->donorTwlPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWL_NDS_PATH").c_str());

	// SDK5.0 (TWL) DSi-Exclusive Donor NDS path
	conf->donorTwlOnly0Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWLONLY0_NDS_PATH").c_str());

	// SDK5.x (TWL) DSi-Exclusive Donor NDS path
	conf->donorTwlOnlyPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWLONLY_NDS_PATH").c_str());

	// GBA path
	conf->gbaPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "GBA_PATH").c_str());

	// GBA SAV path
	conf->gbaSavPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "GBA_SAV_PATH").c_str());

	// AP-patch path
	conf->apPatchPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "AP_FIX_PATH").c_str());

	// Language
	conf->language = strtol(config_file.fetch("NDS-BOOTSTRAP", "LANGUAGE", "-1").c_str(), NULL, 0);
	if (conf->language < -1) conf->language = -1;

	// Region
	conf->region = strtol(config_file.fetch("NDS-BOOTSTRAP", "REGION", "-1").c_str(), NULL, 0);
	if (conf->region < -1) conf->region = -1;

	// Use ROM Region
	conf->useRomRegion = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "USE_ROM_REGION", "1").c_str(), NULL, 0);

	// SDNAND
	conf->sdNand = strtol(config_file.fetch("NDS-BOOTSTRAP", "SDNAND", "0").c_str(), NULL, 0);

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
	//conf->colorMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "COLOR_MODE", "0").c_str(), NULL, 0);

	// ROM read LED
	conf->romRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "ROMREAD_LED", "0").c_str(), NULL, 0);

	// DMA ROM read LED
	conf->dmaRomRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "DMA_ROMREAD_LED", "0").c_str(), NULL, 0);

	// Async card read
	conf->asyncCardRead = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "ASYNC_CARD_READ", "0").c_str(), NULL, 0);

	// Card read DMA
	conf->cardReadDMA = strtol(config_file.fetch("NDS-BOOTSTRAP", "CARD_READ_DMA", "1").c_str(), NULL, 0);

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

	// Manual file path
	conf->manualPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "MANUAL_PATH").c_str());
}

/*static void load_game_conf(configuration* conf, const char* fn, char* romTid) {
	easysave::ini config_file(fn);

	// SDK5 (TWL) Donor NDS path
	conf->cleanDonorPath = strdup(config_file.fetch(romTid, "DONOR_NDS_PATH").c_str());
}*/

void getIgmStrings(configuration* conf, bool b4ds) {
	// Set In-Game Menu strings
	tonccpy(igmText->version, VER_NUMBER, sizeof(VER_NUMBER));
	tonccpy(igmText->ndsBootstrap, "nds-bootstrap", 28);
	igmText->rtl = false;

	// Load In-Game Menu font
	extendedFont = IgmFont::extendedLatin;
	const char *extendedFontPath = "nitro:/fonts/extended_latin.lz77";
	if (strcmp(conf->guiLanguage, "ar") == 0) {
		extendedFont = IgmFont::arabic;
		extendedFontPath = "nitro:/fonts/arabic.lz77";
		igmText->rtl = true;
	} else if (strcmp(conf->guiLanguage, "ru") == 0 || strcmp(conf->guiLanguage, "uk") == 0) {
		extendedFont = IgmFont::cyrillic;
		extendedFontPath = "nitro:/fonts/cyrillic.lz77";
	} else if (strcmp(conf->guiLanguage, "el") == 0) {
		extendedFont = IgmFont::greek;
		extendedFontPath = "nitro:/fonts/greek.lz77";
	} else if (strcmp(conf->guiLanguage, "ko") == 0) {
		extendedFont = IgmFont::hangul;
		extendedFontPath = "nitro:/fonts/hangul.lz77";
	} else if (strcmp(conf->guiLanguage, "he") == 0) {
		extendedFont = IgmFont::hebrew;
		extendedFontPath = "nitro:/fonts/hebrew.lz77";
		igmText->rtl = true;
	} else if (strcmp(conf->guiLanguage, "ja") == 0 || strncmp(conf->guiLanguage, "zh", 2) == 0) {
		extendedFont = IgmFont::kanaChinese;
		extendedFontPath = "nitro:/fonts/kana_chinese.lz77";
	}

	FILE *font = fopen("nitro:/fonts/ascii.lz77", "rb");
	if (font) {
		fread(lz77ImageBuffer, 1, 0x400, font);
		LZ77_Decompress(lz77ImageBuffer, igmText->font);
		fclose(font);
	}
	font = fopen(extendedFontPath, "rb");
	if (font) {
		fread(lz77ImageBuffer, 1, 0x400, font);
		LZ77_Decompress(lz77ImageBuffer, igmText->font + 0x400);
		fclose(font);
	}

	// Set In-Game Menu hotkey
	igmText->hotkey = conf->hotkey != 0 ? conf->hotkey : (KEY_L | KEY_DOWN | KEY_SELECT);

	if(b4ds) {
		cardengineArm7B4DS* ce7 = (cardengineArm7B4DS*)CARDENGINE_ARM7_LOCATION_BUFFERED;
		ce7->igmHotkey = igmText->hotkey;
	} else {
		cardengineArm7* ce7 = (cardengineArm7*)CARDENGINEI_ARM7_BUFFERED_LOCATION;
		ce7->igmHotkey = igmText->hotkey;
		if (*(u32*)CARDENGINEI_ARM7_SDK5_BUFFERED_LOCATION != 0) {
			cardengineArm7* ce7sdk5 = (cardengineArm7*)CARDENGINEI_ARM7_SDK5_BUFFERED_LOCATION;
			ce7sdk5->igmHotkey = igmText->hotkey;
		}
	}

	char path[40];
	snprintf(path, sizeof(path), "nitro:/languages/%s/in_game_menu.ini", conf->guiLanguage);
	easysave::ini lang(path);

	setIgmString(lang.fetch("TITLES", "RAM_VIEWER", "RAM Viewer").c_str(), igmText->ramViewer);
	setIgmString(lang.fetch("TITLES", "JUMP_ADDRESS", "Jump to Address").c_str(), igmText->jumpAddress);
	setIgmString(lang.fetch("TITLES", "SELECT_BANK", "Select VRAM Bank").c_str(), igmText->selectBank);
	setIgmString(lang.fetch("TITLES", "COUNT", "Count:").c_str(), igmText->count);

	setIgmString(lang.fetch("MENU", "RETURN_TO_GAME", "Return to Game").c_str(), igmText->menu[0]);
	setIgmString(lang.fetch("MENU", "RESET_GAME", "Reset Game").c_str(), igmText->menu[1]);
	setIgmString(lang.fetch("MENU", "SCREENSHOT", "Screenshot...").c_str(), igmText->menu[2]);
	setIgmString(lang.fetch("MENU", "MANUAL", "Manual...").c_str(), igmText->menu[3]);
	setIgmString(lang.fetch("MENU", "DUMP_RAM", "Dump RAM").c_str(), igmText->menu[4]);
	setIgmString(lang.fetch("MENU", "OPTIONS", "Options...").c_str(), igmText->menu[5]);
	// setIgmString(lang.fetch("MENU", "CHEATS", "Cheats...").c_str(), igmText->menu[6]);
	setIgmString(lang.fetch("MENU", "RAM_VIEWER", "RAM Viewer...").c_str(), igmText->menu[6]);
	setIgmString(lang.fetch("MENU", "QUIT_GAME", "Quit Game").c_str(), igmText->menu[7]);

	setIgmString(lang.fetch("OPTIONS", "MAIN_SCREEN", "Main Screen").c_str(), igmText->optionsLabels[0]);
	setIgmString(lang.fetch("OPTIONS", "BRIGHTNESS", "Brightness").c_str(), igmText->optionsLabels[1]);
	setIgmString(lang.fetch("OPTIONS", "VOLUME", "Volume").c_str(), igmText->optionsLabels[2]);
	setIgmString(lang.fetch("OPTIONS", "CLOCK_SPEED", "Clock Speed").c_str(), igmText->optionsLabels[3]);
	setIgmString(lang.fetch("OPTIONS", "VRAM_MODE", "VRAM Mode").c_str(), igmText->optionsLabels[4]);
	setIgmString(lang.fetch("OPTIONS", "AUTO", "Auto").c_str(), igmText->optionsValues[0]);
	setIgmString(lang.fetch("OPTIONS", "BOTTOM", "Bottom").c_str(), igmText->optionsValues[1]);
	setIgmString(lang.fetch("OPTIONS", "TOP", "Top").c_str(), igmText->optionsValues[2]);
	setIgmString(lang.fetch("OPTIONS", "67_MHZ", "67 MHz").c_str(), igmText->optionsValues[3]);
	setIgmString(lang.fetch("OPTIONS", "133_MHZ", "133 MHz").c_str(), igmText->optionsValues[4]);
	setIgmString(lang.fetch("OPTIONS", "DS_MODE", "DS mode").c_str(), igmText->optionsValues[5]);
	setIgmString(lang.fetch("OPTIONS", "DSI_MODE", "DSi mode").c_str(), igmText->optionsValues[6]);

	// Get manual line count if there's a manual
	FILE *manualFile = fopen(conf->manualPath, "r");
	if(manualFile) {
		char buffer[32];
		igmText->manualMaxLine = 0;
		long manualOffset = 0;
		size_t bytesRead;
		do {
			fseek(manualFile, manualOffset, SEEK_SET);
			bytesRead = fread(buffer, 1, 32, manualFile);
			for(int i = 0; i < 32; i++) {
				if(buffer[i] == '\n') {
					manualOffset += i + 1;
					igmText->manualMaxLine++;
					break;
				} else if(i == 31) {
					manualOffset += i + 1;
					break;
				}
			}
		} while(bytesRead == 32);
	}
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

	conf->initDisc = (REG_SCFG_EXT == 0);

	conf->gameOnFlashcard = (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't');
	conf->saveOnFlashcard = (conf->savPath[0] == 'f' && conf->savPath[1] == 'a' && conf->savPath[2] == 't');

	if (conf->b4dsMode) {
		if (!dsiFeatures() || !conf->gameOnFlashcard || !conf->saveOnFlashcard) {
			conf->b4dsMode = 0;
		} else {
			conf->initDisc = true;
		}
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
	if (conf->asyncCardRead) {
		conf->valueBits2 |= BIT(1);
	}
	if (conf->cardReadDMA) {
		conf->valueBits2 |= BIT(2);
	}
	if (conf->useRomRegion) {
		conf->valueBits2 |= BIT(7);
	}
	if (conf->boostCpu) {
		conf->valueBits3 |= BIT(0);
	}
	/*if (conf->cardReadDMA == 2) {
		conf->valueBits3 |= BIT(1);
	}*/

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

	pageFilePath = "sd:/_nds/pagefile.sys";
	if (conf->b4dsMode || !conf->sdFound) {
		pageFilePath = "fat:/_nds/pagefile.sys";	
	}

	char romTid[5] = {0};
	u8 unitCode = 0;
	u32 ndsArm7Size = 0;
	u32 fatAddr = 0;
	u32 a7mbk6 = 0;
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
		fseek(ndsFile, 0x48, SEEK_SET);
		fread(&fatAddr, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x1A0, SEEK_SET);
		fread(&a7mbk6, sizeof(u32), 1, ndsFile);
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

	u32 donorArm7iOffset = 0;
	u32 donorModcrypt2len = 0;

	FILE* cebin = NULL;
	FILE* donorNdsFile = NULL;
	bool donorLoaded = false;
	conf->isDSiWare = (dsiFeatures() && !conf->b4dsMode && ((unitCode == 3 && (accessControl & BIT(4)))
					|| (unitCode == 2 && conf->dsiMode && romTid[0] == 'K')));
	bool dsiEnhancedMbk = false;

	if (conf->isDSiWare) {
		conf->valueBits2 |= BIT(0);
	}

	// Get region
	u8 twlCfgCountry = (dsiFeatures() ? *(u8*)0x02000405 : 0);
	u8 newRegion = 0;
	if (conf->useRomRegion && romTid[3] != 'A' && romTid[3] != 'O') {
		if (romTid[3] == 'J') {
			newRegion = 0;
		} else if (romTid[3] == 'E' || romTid[3] == 'T') {
			newRegion = 1;
		} else if (romTid[3] == 'P' || romTid[3] == 'V') {
			newRegion = 2;
		} else if (romTid[3] == 'U') {
			newRegion = 3;
		} else if (romTid[3] == 'C') {
			newRegion = 4;
		} else if (romTid[3] == 'K') {
			newRegion = 5;
		}
	} else if (conf->region == 0xFF && twlCfgCountry != 0) {
		if (twlCfgCountry == 0x01) {
			newRegion = 0;	// Japan
		} else if (twlCfgCountry == 0xA0) {
			newRegion = 4;	// China
		} else if (twlCfgCountry == 0x88) {
			newRegion = 5;	// Korea
		} else if (twlCfgCountry == 0x41 || twlCfgCountry == 0x5F) {
			newRegion = 3;	// Australia
		} else if ((twlCfgCountry >= 0x08 && twlCfgCountry <= 0x34) || twlCfgCountry == 0x99 || twlCfgCountry == 0xA8) {
			newRegion = 1;	// USA
		} else if (twlCfgCountry >= 0x40 && twlCfgCountry <= 0x70) {
			newRegion = 2;	// Europe
		}
	} else {
		newRegion = conf->region;
	}

	if (dsiFeatures() && !conf->b4dsMode) {
		dsiEnhancedMbk = (isDSiMode() && *(u32*)0x02FFE1A0 == 0x00403000 && REG_SCFG_EXT7 == 0);
		u32 srlAddr = 0;

		// Load donor ROM's arm7 binary, if needed
		if (REG_SCFG_EXT7 == 0 && (conf->dsiMode > 0 || conf->isDSiWare) && (a7mbk6 == (dsiEnhancedMbk ? 0x080037C0 : 0x00403000) || (romTid[0] == 'H' && ndsArm7Size < 0xC000 && ndsArm7idst == 0x02E80000 && (REG_MBK9 & 0x00FFFFFF) != 0x00FFFF0F))) {
			if (romTid[0] == 'H' && ndsArm7Size < 0xC000 && ndsArm7idst == 0x02E80000) {
				donorNdsFile = fopen(dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path, "rb"); // System titles can only use an SDK 5.0 donor ROM
			} else {
				bool sdk50 = (
				   ( dsiEnhancedMbk && ndsArm7Size == 0x1511C)
				|| ( dsiEnhancedMbk && ndsArm7Size == 0x26CC8)
				|| ( dsiEnhancedMbk && ndsArm7Size == 0x28E54)
				|| (!dsiEnhancedMbk && ndsArm7Size == 0x29EE8)
				);
				donorNdsFile = fopen(sdk50 ? (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path) : (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath), "rb");
				if (!donorNdsFile) {
					if (donorNdsFile) {
						fclose(donorNdsFile);
					}
					FILE* donorNdsFile2 = fopen(sdk50 ? (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath) : (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path), "rb");
					if (donorNdsFile2) {
						donorNdsFile = donorNdsFile2;
					}
				}
			}
		}

		if (donorNdsFile) {
			u32 donorArm7Offset = 0;
			fseek(donorNdsFile, srlAddr+0x30, SEEK_SET);
			fread(&donorArm7Offset, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+0x3C, SEEK_SET);
			fread((u32*)DONOR_ROM_ARM7_SIZE_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+0x1A0, SEEK_SET);
			fread((u32*)DONOR_ROM_MBK6_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+0x1D0, SEEK_SET);
			fread(&donorArm7iOffset, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+0x1D4, SEEK_SET);
			fread((u32*)DONOR_ROM_DEVICE_LIST_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+0x1DC, SEEK_SET);
			fread((u32*)DONOR_ROM_ARM7I_SIZE_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+0x22C, SEEK_SET);
			fread(&donorModcrypt2len, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, srlAddr+donorArm7Offset, SEEK_SET);
			fread((u8*)DONOR_ROM_ARM7_LOCATION, 1, *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION, donorNdsFile);
			donorLoaded = true;
		}
	}

  if (dsiFeatures() && !conf->b4dsMode) {
	if ((conf->dsiMode > 0 && unitCode > 0) || conf->isDSiWare) {
		uint8_t *target = (uint8_t *)TARGETBUFFERHEADER ;
		fseek(ndsFile, 0, SEEK_SET);
		fread(target, 1, 0x1000, ndsFile);

		/*if (conf->dsiMode > 0 && unitCode > 0 && !conf->isDSiWare) {
			load_game_conf(conf, conf->sdFound ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini", (char*)romTid);

			if (std::string(conf->cleanDonorPath) != std::string(conf->ndsPath) && strlen(conf->cleanDonorPath) > 8) {
				fclose(ndsFile);
				FILE* ndsFile = fopen(conf->cleanDonorPath, "rb");
				if (ndsFile) {
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

					fseek(ndsFile, 0, SEEK_SET);
					fread(target, 1, 0x180, ndsFile);
				}
			}
		}*/

		if (ndsArm9ilen) {
			fseek(ndsFile, ndsArm9isrc, SEEK_SET);
			fread((u32*)ndsArm9idst, 1, ndsArm9ilen, ndsFile);
		}
		if (donorLoaded) {
			if (*(u32*)DONOR_ROM_ARM7I_SIZE_LOCATION) {
				fseek(donorNdsFile, donorArm7iOffset, SEEK_SET);
				fread((u32*)ndsArm7idst, 1, *(u32*)DONOR_ROM_ARM7I_SIZE_LOCATION, donorNdsFile);
			}
		} else {
			if (ndsArm7ilen) {
				fseek(ndsFile, ndsArm7isrc, SEEK_SET);
				fread((u32*)ndsArm7idst, 1, ndsArm7ilen, ndsFile);
			}
		}

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

			dsi_context ctx;
			dsi_set_key(&ctx, key);
			dsi_set_ctr(&ctx, &target[0x300]);
			if (modcrypt1len)
			{
				decrypt_modcrypt_area(&ctx, (u8*)ndsArm9idst, modcrypt1len);
			}

			dsi_set_key(&ctx, key);
			dsi_set_ctr(&ctx, &target[0x314]);
			if (modcrypt2len && !donorLoaded)
			{
				decrypt_modcrypt_area(&ctx, (u8*)ndsArm7idst, modcrypt2len);
			}
		}
		if (donorLoaded && donorModcrypt2len) {
			fseek(donorNdsFile, 0, SEEK_SET);
			fread(target, 1, 0x1000, donorNdsFile);

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

				dsi_context ctx;
				dsi_set_key(&ctx, key);
				dsi_set_ctr(&ctx, &target[0x314]);
				//if (donorModcrypt2len)
				//{
					decrypt_modcrypt_area(&ctx, (u8*)ndsArm7idst, donorModcrypt2len);
				//}
			}
		}
	}
	fclose(ndsFile);
	fclose(donorNdsFile);

	if (!conf->gameOnFlashcard && !conf->saveOnFlashcard) {
		if (romTid[0] != 'I' && memcmp(romTid, "UZP", 3) != 0) {
			disableSlot1();
		} else {
			// Initialize card and read header, HGSS IR doesn't work if you don't read the full header
			sysSetCardOwner(BUS_OWNER_ARM9); // Allow arm9 to access NDS cart
			if (isDSiMode()) {
				// Reset card slot
				disableSlot1();
				for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
				enableSlot1();
				for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }

				// Dummy command sent after card reset
				cardParamCommand (CARD_CMD_DUMMY, 0,
					CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(1) | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F),
					NULL, 0);
			}

			REG_ROMCTRL = 0;
			REG_AUXSPICNT = 0;
			for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
			REG_AUXSPICNT = CARD_CR1_ENABLE | CARD_CR1_IRQ;
			REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;
			while(REG_ROMCTRL & CARD_BUSY);
			cardReset();
			while(REG_ROMCTRL & CARD_BUSY);

			u32 iCardId = cardReadID(CARD_CLK_SLOW);
			while(REG_ROMCTRL & CARD_BUSY);

			bool normalChip = (iCardId & BIT(31)) != 0; // ROM chip ID MSB

			// Read the header
			char headerData[0x1000];
			cardParamCommand (CARD_CMD_HEADER_READ, 0,
				CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(1) | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F),
				(u32 *)headerData, 0x200 / sizeof(u32));

			if ((headerData[0x12] != 0) || (headerData[0x1BF] != 0)) {
				// Extended header found
				if(normalChip) {
					for(int i = 0; i < 8; i++) {
						cardParamCommand (CARD_CMD_HEADER_READ, i * 0x200,
							CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(1) | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F),
							(u32 *)headerData + i * 0x200 / sizeof(u32), 0x200 / sizeof(u32));
					}
				} else {
					cardParamCommand (CARD_CMD_HEADER_READ, 0,
						CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(4) | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F),
						(u32 *)headerData, 0x1000 / sizeof(u32));
				}
			}

			sysSetCardOwner(BUS_OWNER_ARM7);

			// Leave Slot-1 enabled for IR cartridges and Battle & Get: Pokémon Typing DS
			conf->specialCard = (headerData[0xC] == 'I' || memcmp(headerData + 0xC, "UZP", 3) == 0);
			if (conf->specialCard) {
				conf->valueBits2 |= BIT(4);
			} else {
				disableSlot1();
			}
		}
	}

	u32 srBackendId[2] = {0};
	// Load srBackendId
	if (REG_SCFG_EXT7 == 0 && conf->gameOnFlashcard) {
		/*srBackendId[0] = 0x464B4356; // "VCKF" (My Cooking Coach)
		srBackendId[1] = 0x00030000;*/
	} else {
		cebin = fopen("sd:/_nds/nds-bootstrap/srBackendId.bin", "rb");
		fread(&srBackendId, sizeof(u32), 2, cebin);
		fclose(cebin);
	}

	if (isDSiMode() && unitCode > 0 && (REG_SCFG_EXT7 == 0 ? !conf->gameOnFlashcard : conf->sdFound)) {
		// Load device list
		cebin = fopen("nitro:/deviceList.bin", "rb");
		if (cebin) {
			char sdmcText[4] = {'s','d','m','c'};
			fread((u8*)0x02EFF000, 1, 0x400, cebin);
			if (conf->sdNand) {
				//*(u8*)0x02EFF055 = 0; // nand
				//*(u8*)0x02EFF0A9 = 0; // nand2
				bool shared1Found = (access("sd:/shared1", F_OK) == 0);
				bool shared2Found = (access("sd:/shared2", F_OK) == 0);
				if (shared1Found) {
					toncset((u8*)0x02EFF0FD, 0x10, 1);
					tonccpy((char*)0x02EFF110, sdmcText, 4);
				}
				const char* photoPath = "sd:/photo";
				mkdir(photoPath, 0777);
				//if (access(photoPath, F_OK) == 0) {
					toncset((u8*)0x02EFF151, 0x10, 1);
					toncset((char*)0x02EFF164, 0, 0x40);
					tonccpy((char*)0x02EFF166, photoPath, strlen(photoPath));
					tonccpy((char*)0x02EFF164, sdmcText, 4);
				//}
				if ((strncmp(romTid, "HNK", 3) == 0 || strncmp(romTid, "KGU", 3) == 0) && shared2Found) {
					const char* filePath = "sdmc:/shared2/0000";
					const char* share = "share";
					u8 cPath[3] = {'C', 0x08, 0x06};
					tonccpy((char*)0x02EFF24C, cPath, 3);
					tonccpy((char*)0x02EFF250, share, strlen(share));
					tonccpy((char*)0x02EFF260, filePath, strlen(filePath));
				}
			}
			if (!conf->gameOnFlashcard && strlen(conf->appPath) < 62) {
				tonccpy((char*)0x02EFF3C2, conf->appPath, strlen(conf->appPath));
				tonccpy((char*)0x02EFF3C0, sdmcText, 4);
			}
			if (!conf->saveOnFlashcard) {
				if (strlen(conf->prvPath) < 62) {
					if (strncasecmp(conf->prvPath, "sd:", 3) != 0) {
						tonccpy((char*)0x02EFF1B8, conf->prvPath, strlen(conf->prvPath));
					} else {
						tonccpy((char*)0x02EFF1BA, conf->prvPath, strlen(conf->prvPath));
						tonccpy((char*)0x02EFF1BA, sdmcText+2, 2);
					}
				}
				if (strlen(conf->savPath) < 62) {
					if (strncasecmp(conf->savPath, "sd:", 3) != 0) {
						tonccpy((char*)0x02EFF20C, conf->savPath, strlen(conf->savPath));
					} else {
						tonccpy((char*)0x02EFF20E, conf->savPath, strlen(conf->savPath));
						tonccpy((char*)0x02EFF20E, sdmcText+2, 2);
					}
				}
			}
		}
		fclose(cebin);
	}

	if (REG_SCFG_EXT7 == 0) {
		if (conf->gameOnFlashcard && !conf->sdFound) {
			// MBK and WRAM are inaccessible, but DSi titles and cardEngine seem to access those fine?
			conf->dsiWramAccess = (memcmp(romTid, "VDE", 3) != 0); // Fossil Fighters: Champions
		} else {
			u32 wordBak = *(vu32*)0x03700000;
			*(vu32*)0x03700000 = 0x414C5253;
			conf->dsiWramAccess = *(vu32*)0x03700000 == 0x414C5253;
			*(vu32*)0x03700000 = wordBak;
		}
	} else {
		conf->dsiWramAccess = true;
	}
	if (conf->dsiWramAccess) {
		conf->valueBits2 |= BIT(5);
	}
	if (access("sd:/hiya.dsi", F_OK) == 0) {
		conf->valueBits2 |= BIT(6);
	}

  if (conf->gameOnFlashcard || !conf->isDSiWare) {
	// Load ce7 binary
	cebin = fopen(dsiEnhancedMbk ? "nitro:/cardenginei_arm7_alt.lz77" : "nitro:/cardenginei_arm7.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION);
		if (REG_SCFG_EXT7 != 0) {
			tonccpy((u8*)LOADER_RETURN_LOCATION, twlmenuResetGamePath, 256);
		}
		tonccpy((u8*)LOADER_RETURN_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

	// Load SDK5 ce7 binary
	cebin = fopen("nitro:/cardenginei_arm7_sdk5.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_SDK5_BUFFERED_LOCATION);
		if (REG_SCFG_EXT7 != 0) {
			tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION, twlmenuResetGamePath, 256);
		}
		tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_BUFFERED_LOCATION, 1, 0x400, cebin);
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
	} else {
		// Load ce9 binary
		cebin = fopen("nitro:/cardenginei_arm9.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x3000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
			if (conf->dsiWramAccess) {
				// Relocate
				u32* addr = (u32*)CARDENGINEI_ARM9_BUFFERED_LOCATION;
				for (u16 i = 0; i < 0x5000/sizeof(u32); i++) {
					if (addr[i] >= CARDENGINEI_ARM9_LOCATION && addr[i] < CARDENGINEI_ARM9_LOCATION+0x7A00) {
						addr[i] -= CARDENGINEI_ARM9_LOCATION;
						addr[i] += CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
					}
				}
			}
		}
		fclose(cebin);
	}

	if (conf->gameOnFlashcard) {
		// Load SDK5 DLDI ce9 binary
		cebin = fopen(unitCode>0&&conf->dsiMode ? "nitro:/cardenginei_arm9_twlsdk_dldi.lz77" : "nitro:/cardenginei_arm9_sdk5_dldi.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x7000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_SDK5_DLDI_BUFFERED_LOCATION);
		}
		fclose(cebin);
	} else {
		// Load SDK5 ce9 binary
		cebin = fopen(unitCode>0&&conf->dsiMode ? "nitro:/cardenginei_arm9_twlsdk.lz77" : "nitro:/cardenginei_arm9_sdk5.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, 0x3000, cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION);
			if (conf->dsiWramAccess && (unitCode == 0 || !conf->dsiMode)) {
				// Relocate
				u32* addr = (u32*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION;
				for (u16 i = 0; i < 0x5000/sizeof(u32); i++) {
					if (addr[i] >= CARDENGINEI_ARM9_SDK5_LOCATION && addr[i] < CARDENGINEI_ARM9_SDK5_LOCATION+0x7A00) {
						addr[i] -= CARDENGINEI_ARM9_SDK5_LOCATION;
						addr[i] += CARDENGINEI_ARM9_LOCATION_DSI_WRAM;
					}
				}
			}
		}
		fclose(cebin);
	}
	if ((conf->gameOnFlashcard || !conf->isDSiWare) && (conf->extendedMemory || conf->dsiMode)) {
		bool found = (access(pageFilePath.c_str(), F_OK) == 0);
		if (!found) {
			consoleDemoInit();
			iprintf("Creating pagefile.sys\n");
			iprintf("Please wait...\n");
		}

		cebin = fopen(pageFilePath.c_str(), found ? "r+" : "wb");
		fseek(cebin, twlPageFileSize - 1, SEEK_SET);
		fputc('\0', cebin);
		fclose(cebin);

		if (!found) {
			consoleClear();
		}
	}

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

		getIgmStrings(conf, false);

		if (conf->dsiMode > 0 && unitCode > 0) {
			// Relocate
			u32* addr = (u32*)INGAME_MENU_LOCATION;
			for (u16 i = 0; i < 0x4000/sizeof(u32); i++) {
				if (addr[i] >= INGAME_MENU_LOCATION && addr[i] < INGAME_MENU_LOCATION+0x4000) {
					addr[i] -= INGAME_MENU_LOCATION;
					addr[i] += INGAME_MENU_LOCATION_TWLSDK;
				}
			}
		}
	}
	fclose(cebin);

	// Load touch fix for SM64DS (U) v1.0
	cebin = fopen("nitro:/arm7fix.bin", "rb");
	if (cebin) {
		fread((u8*)ARM7_FIX_BUFFERED_LOCATION, 1, 0x140, cebin);
	}
	fclose(cebin);
  } else if (ndsArm7idst <= 0x02E80000) {
	// Load ce7 binary
	cebin = fopen("nitro:/cardenginei_arm7_dsiware.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x8000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION);
		if (REG_SCFG_EXT7 != 0) {
			tonccpy((u8*)LOADER_RETURN_DSIWARE_LOCATION, twlmenuResetGamePath, 256);
		}
		tonccpy((u8*)LOADER_RETURN_DSIWARE_LOCATION+0x100, &srBackendId, 8);
	}
	fclose(cebin);

	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_BUFFERED_LOCATION, 1, 0x400, cebin);
	}
	fclose(cebin);

	// Load ce9 binary
	cebin = fopen("nitro:/cardenginei_arm9_dsiware.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x3000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
	}
	fclose(cebin);

	// Load in-game menu ce9 binary
	cebin = fopen("nitro:/cardenginei_arm9_igm.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x4000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)INGAME_MENU_LOCATION);

		getIgmStrings(conf, false);

		// Relocate
		u32* addr = (u32*)INGAME_MENU_LOCATION;
		for (u16 i = 0; i < 0x4000/sizeof(u32); i++) {
			if (addr[i] >= INGAME_MENU_LOCATION && addr[i] < INGAME_MENU_LOCATION+0x4000) {
				addr[i] -= INGAME_MENU_LOCATION;
				addr[i] += INGAME_MENU_LOCATION_DSIWARE;
			}
		}
	}
	fclose(cebin);

	bool found = (access(pageFilePath.c_str(), F_OK) == 0);
	if (!found) {
		consoleDemoInit();
		iprintf("Creating pagefile.sys\n");
		iprintf("Please wait...\n");
	}

	cebin = fopen(pageFilePath.c_str(), found ? "r+" : "wb");
	fseek(cebin, twlPageFileSize - 1, SEEK_SET);
	fputc('\0', cebin);
	fclose(cebin);

	if (!found) {
		consoleClear();
	}
  }

	// Load DS blowfish
	cebin = fopen("nitro:/encr_data.bin", "rb");
	if (cebin) {
		fread((void*)BLOWFISH_LOCATION, 1, 0x1048, cebin);
	}
	fclose(cebin);

	/*if (!isDSiMode() && unitCode>0 && conf->dsiMode) {
		// Load DSi ARM7 BIOS
		cebin = fopen("sd:/_nds/bios7i.bin", "rb");
		if (cebin) {
			fread((u32*)0x02EC0000, 1, 0x10000, cebin);

			// Relocate addresses
			*(u32*)0x02EC58A8 += 0x02EC0000;
		}
		fclose(cebin);
	}*/

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

	//bool wideCheatFound = (access(wideCheatFilePath.c_str(), F_OK) == 0);

	FILE* bootstrapImages = fopen("nitro:/bootloader_images.lz77", "rb");
	if (bootstrapImages) {
		fread(lz77ImageBuffer, 1, 0x8000, bootstrapImages);
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION+0x18000);
	}
	fclose(bootstrapImages);

	if (newRegion == 1) {
		// Read ESRB rating and descriptor(s) for current title
		bootstrapImages = fopen(conf->sdFound ? "sd:/_nds/nds-bootstrap/esrb.bin" : "fat:/_nds/nds-bootstrap/esrb.bin", "rb");
		if (bootstrapImages) {
			// Read width & height
			/*fseek(bootstrapImages, 0x12, SEEK_SET);
			u32 width, height;
			fread(&width, 1, sizeof(width), bootstrapImages);
			fread(&height, 1, sizeof(height), bootstrapImages);

			if (width > 256 || height > 192) {
				fclose(bootstrapImages);
				return false;
			}

			fseek(bootstrapImages, 0xE, SEEK_SET);
			u8 headerSize = fgetc(bootstrapImages);
			bool rgb565 = false;
			if(headerSize == 0x38) {
				// Check the upper byte green mask for if it's got 5 or 6 bits
				fseek(bootstrapImages, 0x2C, SEEK_CUR);
				rgb565 = fgetc(bootstrapImages) == 0x07;
				fseek(bootstrapImages, headerSize - 0x2E, SEEK_CUR);
			} else {
				fseek(bootstrapImages, headerSize - 1, SEEK_CUR);
			}
			u16 *bmpImageBuffer = new u16[width * height];
			fread(bmpImageBuffer, 2, width * height, bootstrapImages);
			u16 *dst = (u16*)IMAGES_LOCATION + ((191 - ((192 - height) / 2)) * 256) + (256 - width) / 2;
			u16 *src = bmpImageBuffer;
			for (uint y = 0; y < height; y++, dst -= 256) {
				for (uint x = 0; x < width; x++) {
					u16 val = *(src++);
					*(dst + x) = ((val >> (rgb565 ? 11 : 10)) & 0x1F) | ((val >> (rgb565 ? 1 : 0)) & (0x1F << 5)) | (val & 0x1F) << 10 | BIT(15);
				}
			}

			delete[] bmpImageBuffer;*/
			fread((u16*)IMAGES_LOCATION, 1, 0x18000, bootstrapImages);
		} else {
			toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
		}
		fclose(bootstrapImages);
	} else {
		toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
	}

  } else {
	fclose(ndsFile);
	fclose(donorNdsFile);

	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)(conf->b4dsMode == 1 ? CHEAT_ENGINE_LOCATION_B4DS-0x400000 : CHEAT_ENGINE_LOCATION_B4DS), 1, 0x400, cebin);
	}
	fclose(cebin);

	// Load ce7 binary
	cebin = fopen("nitro:/cardengine_arm7.bin", "rb");
	if (cebin) {
		fread((void*)CARDENGINE_ARM7_LOCATION_BUFFERED, 1, 0x1000, cebin);
	}
	fclose(cebin);

	// Load in-game menu ce9 binary
	cebin = fopen("nitro:/cardengine_arm9_igm.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, 0x4000, cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)INGAME_MENU_LOCATION_B4DS);

		igmText = (struct IgmText *)INGAME_MENU_LOCATION_B4DS;

		getIgmStrings(conf, true);

		fclose(cebin);

		if (access(pageFilePath.c_str(), F_OK) != 0) {
			cebin = fopen(pageFilePath.c_str(), "wb");
			fseek(cebin, 0x14000 - 1, SEEK_SET);
			fputc('\0', cebin);
			fclose(cebin);
		}
		cebin = fopen(pageFilePath.c_str(), "r+");
		fwrite((u8*)INGAME_MENU_LOCATION_B4DS, 1, 0xA000, cebin);
		fclose(cebin);
		toncset((u8*)INGAME_MENU_LOCATION_B4DS, 0, 0xA000);
	}

	if (conf->b4dsMode == 0) {
		*(vu32*)(0x02800000) = 0x314D454D;
		*(vu32*)(0x02C00000) = 0x324D454D;
	}

	// Load ce9 binary
	if (conf->b4dsMode == 2 || (*(vu32*)(0x02800000) == 0x314D454D && *(vu32*)(0x02C00000) == 0x324D454D)) {
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
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION+0x18000);
	}
	fclose(bootstrapImages);

	if (newRegion == 1) {
		// Read ESRB rating and descriptor(s) for current title
		bootstrapImages = fopen(conf->sdFound ? "sd:/_nds/nds-bootstrap/esrb.bin" : "fat:/_nds/nds-bootstrap/esrb.bin", "rb");
		if (bootstrapImages) {
			// Read width & height
			/*fseek(bootstrapImages, 0x12, SEEK_SET);
			u32 width, height;
			fread(&width, 1, sizeof(width), bootstrapImages);
			fread(&height, 1, sizeof(height), bootstrapImages);

			if (width > 256 || height > 192) {
				fclose(bootstrapImages);
				return false;
			}

			fseek(bootstrapImages, 0xE, SEEK_SET);
			u8 headerSize = fgetc(bootstrapImages);
			bool rgb565 = false;
			if(headerSize == 0x38) {
				// Check the upper byte green mask for if it's got 5 or 6 bits
				fseek(bootstrapImages, 0x2C, SEEK_CUR);
				rgb565 = fgetc(bootstrapImages) == 0x07;
				fseek(bootstrapImages, headerSize - 0x2E, SEEK_CUR);
			} else {
				fseek(bootstrapImages, headerSize - 1, SEEK_CUR);
			}
			u16 *bmpImageBuffer = new u16[width * height];
			fread(bmpImageBuffer, 2, width * height, bootstrapImages);
			u16 *dst = (u16*)IMAGES_LOCATION + ((191 - ((192 - height) / 2)) * 256) + (256 - width) / 2;
			u16 *src = bmpImageBuffer;
			for (uint y = 0; y < height; y++, dst -= 256) {
				for (uint x = 0; x < width; x++) {
					u16 val = *(src++);
					*(dst + x) = ((val >> (rgb565 ? 11 : 10)) & 0x1F) | ((val >> (rgb565 ? 1 : 0)) & (0x1F << 5)) | (val & 0x1F) << 10 | BIT(15);
				}
			}

			delete[] bmpImageBuffer;*/
			fread((u16*)IMAGES_LOCATION, 1, 0x18000, bootstrapImages);
		} else {
			toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
		}
		fclose(bootstrapImages);
	} else {
		toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
	}
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
	
	if (getFileSize(srParamsFilePath.c_str()) < 0x10) {
		u32 buffer = 0xFFFFFFFF;

		FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "wb");
		fwrite(&buffer, sizeof(u32), 1, srParamsFile);
		fseek(srParamsFile, 0x10 - 1, SEEK_SET);
		fputc('\0', srParamsFile);
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

	if (dsiFeatures() && !conf->b4dsMode) {	// Not for B4DS
		ramDumpPath = "sd:/_nds/nds-bootstrap/ramDump.bin";
		if (!conf->sdFound) {
			ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";
		}

		if (conf->sdFound && access(ramDumpPath.c_str(), F_OK) != 0) {
			consoleDemoInit();
			iprintf("Creating RAM dump file.\n");
			iprintf("Please wait...\n");
			/* printf("\n");
			if (conf->consoleModel >= 2) {
				iprintf("If this takes a while, press\n");
				iprintf("HOME, then press B.\n");
			} else {
				iprintf("If this takes a while, close\n");
				iprintf("the lid, and open it again.\n");
			} */

			FILE *ramDumpFile = fopen(ramDumpPath.c_str(), "wb");
			if (ramDumpFile) {
				fseek(ramDumpFile, 0x02000000 - 1, SEEK_SET);
				fputc('\0', ramDumpFile);
				fclose(ramDumpFile);
			}

			consoleClear();
		}

		screenshotPath = "sd:/_nds/nds-bootstrap/screenshots.tar";
		if (!conf->sdFound) {
			screenshotPath = "fat:/_nds/nds-bootstrap/screenshots.tar";
		}

		if (access(screenshotPath.c_str(), F_OK) != 0) {
			char buffer[2][0x100] = {{0}};

			consoleDemoInit();
			iprintf("Creating screenshots.tar\n");
			iprintf("Please wait...\n");

			FILE *headerFile = fopen("nitro:/screenshotTarHeaders.bin", "rb");
			if (headerFile) {
				fread(buffer[0], 1, 0x100, headerFile);
				FILE *screenshotFile = fopen(screenshotPath.c_str(), "wb");
				if (screenshotFile) {
					fseek(screenshotFile, 0x4BCC00 - 1, SEEK_SET);
					fputc('\0', screenshotFile);

					for (int i = 0; i < 50; i++) {
						fseek(screenshotFile, i*0x18400, SEEK_SET);
						fread(buffer[1], 1, 0x100, headerFile);
						fwrite(buffer[1], 1, 0x100, screenshotFile);
						fwrite(buffer[0], 1, 0x100, screenshotFile);
					}

					fclose(screenshotFile);
				}
				fclose(headerFile);
			}

			consoleClear();
			igmText->currentScreenshot = 0;
		} else {
			FILE *screenshotFile = fopen(screenshotPath.c_str(), "rb");
			igmText->currentScreenshot = 50;
			if (screenshotFile) {
				fseek(screenshotFile, 0x200, SEEK_SET);
				for (int i = 0; i < 50; i++) {
					if(fgetc(screenshotFile) != 'B') {
						igmText->currentScreenshot = i;
						break;
					}

					fseek(screenshotFile, 0x18400 - 1, SEEK_CUR);
				}

				fclose(screenshotFile);
			}
		}
	} else {
		ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";

		if (access(ramDumpPath.c_str(), F_OK) != 0) {
			consoleDemoInit();
			iprintf("Creating RAM dump file.\n");
			iprintf("Please wait...\n");

			FILE *ramDumpFile = fopen(ramDumpPath.c_str(), "wb");
			if (ramDumpFile) {
				fseek(ramDumpFile, 0x800000 - 1, SEEK_SET);
				fputc('\0', ramDumpFile);
				fclose(ramDumpFile);
			}

			consoleClear();
		}
	}

	if (conf->gameOnFlashcard || !conf->isDSiWare) {
		// Update modified date
		FILE *savFile = fopen(conf->savPath, "r+");
		if (savFile) {
			u8 buffer = 0;
			fread(&buffer, 1, 1, savFile);
			fseek(savFile, 0, SEEK_SET);
			fwrite(&buffer, 1, 1, savFile);
			fclose(savFile);
		}
	}

	return 0;
}