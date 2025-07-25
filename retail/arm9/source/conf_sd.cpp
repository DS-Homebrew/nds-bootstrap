#include <stdlib.h> // strtol
#include <errno.h>
#include <unistd.h>
//#include <stdio.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include "io_m3_common.h"
#include "io_g6_common.h"
#include "io_sc_common.h"
#include "exptools.h"
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
#include "lzx.h"
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

#include "nandio.h"
#include "f_xy.h"
#include "dsi.h"
#include "u128_math.h"

#include "dsiwaresSetForBootloader.h"
#include "asyncReadExcludeMap.h"
#include "dmaExcludeMap.h"
#include "twlClockExcludeMap.h"
#include "colorLutBlacklist.h"

#define REG_SCFG_EXT7 *(u32*)0x02FFFDF0

#define ntrPageFileSize 0x400000
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

static const char* twlmenuResetGamePath = "sdmc:/_nds/TWiLightMenu/main.srldr";

extern const DISC_INTERFACE __my_io_dsisd;

extern char patchOffsetCacheFilePath[64];
extern std::string wideCheatFilePath;
extern std::string cheatFilePath;
extern std::string ramDumpPath;
extern std::string srParamsFilePath;
extern std::string screenshotPath;
extern std::string apFixOverlaysPath;
extern std::string musicsFilePath;
extern std::string pageFilePath;
extern std::string sharedFontPath;

extern u8* lz77ImageBuffer;
#define sizeof_lz77ImageBuffer 0x30000

extern bool colorTable;
extern bool invertedColors;
extern bool noWhiteFade;

extern void myConsoleDemoInit(void);

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

off_t getFileSize(FILE* fp) {
	fseek(fp, 0, SEEK_END);
	off_t fsize = ftell(fp);
	if (!fsize) fsize = 0;

	return fsize;
}

void addTwlDevice(const char letter, u8 flags, u8 accessRights, const char* name, const char* path) {
	static char currentLetter = 'A';
	static u8* deviceList = (u8*)0x02EFF000;
	if (deviceList == (u8*)0x02EFF000) {
		toncset(deviceList, 0, 0x400);
	}

	toncset(deviceList, (letter == 0) ? currentLetter : letter, 1);
	toncset(deviceList+1, flags, 1);
	toncset(deviceList+2, accessRights, 1);
	tonccpy(deviceList+4, name, strlen(name));
	tonccpy(deviceList+0x14, path, strlen(path));

	deviceList += 0x54;
	if (letter == 0) {
		currentLetter++;
	}
}

static const char* getLangString(const int language) {
	switch (language) {
		case 0:
			return "ja";
		case 1:
		default:
			return "en";
		case 2:
			return "fr";
		case 3:
			return "de";
		case 4:
			return "it";
		case 5:
			return "es";
		case 6:
			return "zh";
		case 7:
			return "ko";
	}
	return "en";
}

extern std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
extern bool extension(const std::string& filename, const char* ext);

extern bool loadPreLoadSettings(configuration* conf, const char* pckPath, const char* romTid, const u16 headerCRC);
extern void loadAsyncLoadSettings(configuration* conf, const char* romTid, const u16 headerCRC);
extern void loadApFix(configuration* conf, const char* bootstrapPath, const char* romTid, const u16 headerCRC);
extern void loadMobiclipOffsets(configuration* conf, const char* bootstrapPath, const char* romTid, const u8 romVersion, const u16 headerCRC);
extern void loadDSi2DSSavePatch(configuration* conf, const char* bootstrapPath, const char* romTid, const u8 romVersion, const u16 headerCRC);

static int loadCardEngineBinary(const char* cardenginePath, u8* location) {
	FILE* cebin = fopen(cardenginePath, "rb");
	int size;
	if(!cebin) {
		return -ENOENT;
	}
	fseek(cebin, 0, SEEK_END);
	size = ftell(cebin);
	fseek(cebin, 0, SEEK_SET);
	if(extension(cardenginePath, ".lz77")) {
		if(size <= sizeof_lz77ImageBuffer) {
			toncset32(lz77ImageBuffer, 0, sizeof_lz77ImageBuffer/sizeof(u32));
			fread(lz77ImageBuffer, 1, size, cebin);
			LZ77_Decompress(lz77ImageBuffer, location);
		}
		else {
			return -ENOMEM;
		}
	}
	else {
		fread(location, 1, size, cebin);
	}
	fclose(cebin);
	return 0;
}

static void createRamDumpBin(configuration* conf) {
	int ramDumpSize;
	if (dsiFeatures() && !conf->b4dsMode)
	{
		ramDumpPath = conf->bootstrapOnFlashcard ? "fat:" : "sd:";
		ramDumpPath.append("/_nds/nds-bootstrap/ramDump.bin");
		ramDumpSize = 0x02000000;
	}
	else {
		ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";
		ramDumpSize = 0x800000;
	}

	if (getFileSize(ramDumpPath.c_str()) < ramDumpSize) {
		myConsoleDemoInit();
		iprintf("Allocating space for\n");
		iprintf("creating a RAM dump.\n");
		iprintf("Please wait...");

		if (access(ramDumpPath.c_str(), F_OK) == 0) {
			remove(ramDumpPath.c_str());
		}

		FILE *ramDumpFile = fopen(ramDumpPath.c_str(), "wb");
		if (ramDumpFile) {
			fseek(ramDumpFile, ramDumpSize - 1, SEEK_SET);
			fputc('\0', ramDumpFile);
			fclose(ramDumpFile);
		}

		consoleClear();
		if (getFileSize(ramDumpPath.c_str()) < ramDumpSize) {
			iprintf("Failed to create RAM dump file.");
			while (1) swiWaitForVBlank();
		}
	}
}

static void createApFixOverlayBin(configuration* conf) {
	if (dsiFeatures() && !conf->b4dsMode)
	{
		apFixOverlaysPath = conf->gameOnFlashcard ? "fat:" : "sd:";
		apFixOverlaysPath.append("/_nds/nds-bootstrap/apFixOverlays.bin");
	}
	else {
		apFixOverlaysPath = "fat:/_nds/nds-bootstrap/apfixOverlays.bin";
	}

	if (!conf->isDSiWare && getFileSize(apFixOverlaysPath.c_str()) < 0xA00000) {
		myConsoleDemoInit();
		iprintf("Allocating space for\n");
		iprintf("AP-fixed overlays.\n");
		iprintf("Please wait...");

		if (access(apFixOverlaysPath.c_str(), F_OK) == 0) {
			remove(apFixOverlaysPath.c_str());
		}

		FILE *apFixOverlaysFile = fopen(apFixOverlaysPath.c_str(), "wb");
		if (apFixOverlaysFile) {
			fseek(apFixOverlaysFile, 0xA00000 - 1, SEEK_SET);
			fputc('\0', apFixOverlaysFile);
			fclose(apFixOverlaysFile);
		}

		consoleClear();
		if (getFileSize(apFixOverlaysPath.c_str()) < 0xA00000) {
			iprintf("Failed to allocate space\n");
			iprintf("for AP-fixed overlays.");
			while (1) swiWaitForVBlank();
		}
	}
}

static void loadColorLut(const bool isRunFromFlashcard, const bool phatColors) {
	if (phatColors) {
		FILE* file = fopen("nitro:/NTR-001.lut", "rb");
		fread(lz77ImageBuffer, 1, 0x10000, file);
		fclose(file);

		vramSetBankE(VRAM_E_LCD);
		tonccpy(VRAM_E, lz77ImageBuffer, 0x10000); // Copy LUT to VRAM
		tonccpy((u16*)COLOR_LUT_BUFFERED_LOCATION, lz77ImageBuffer, 0x10000);

		colorTable = true;
	}

	const char* txtPath = isRunFromFlashcard ? "fat:/_nds/colorLut/currentSetting.txt" : "sd:/_nds/colorLut/currentSetting.txt";
	if (access(txtPath, F_OK) == 0) {
		// Load color LUT
		char lutName[128] = {0};
		FILE* file = fopen(txtPath, "rb");
		fread(lutName, 1, 128, file);
		fclose(file);

		char colorLutPath[256];
		sprintf(colorLutPath, "%s:/_nds/colorLut/%s.lut", isRunFromFlashcard ? "fat" : "sd", lutName);

		if (getFileSize(colorLutPath) == 0x10000) {
			file = fopen(colorLutPath, "rb");
			fread(lz77ImageBuffer, 1, 0x10000, file);
			fclose(file);

			if (colorTable) {
				u16* newColorTable = (u16*)lz77ImageBuffer;
				for (u16 i = 0; i < 0x8000; i++) {
					VRAM_E[i] = newColorTable[VRAM_E[i] % 0x8000];
				}
			} else {
				vramSetBankE(VRAM_E_LCD);
				tonccpy(VRAM_E, lz77ImageBuffer, 0x10000); // Copy LUT to VRAM

				colorTable = true;
			}

			const u16 color0 = VRAM_E[0] | BIT(15);
			const u16 color7FFF = VRAM_E[0x7FFF] | BIT(15);

			invertedColors =
			  (color0 >= 0xF000 && color0 <= 0xFFFF
			&& color7FFF >= 0x8000 && color7FFF <= 0x8FFF);
			if (!invertedColors) noWhiteFade = (color7FFF < 0xF000);

			if (invertedColors || noWhiteFade) {
				powerOff(PM_BACKLIGHT_TOP);
				powerOff(PM_BACKLIGHT_BOTTOM);
			}
		}
	}
}

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

	// SDK2.0 Donor NDS path
	conf->donor20Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONOR20_NDS_PATH").c_str());

	// SDK5.x (NTR) Donor NDS path
	conf->donor5Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONOR5_NDS_PATH").c_str());

	// SDK5.x (NTR) Donor NDS path (VRAM Wireless alternative)
	conf->donor5PathAlt = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONOR5_NDS_PATH_ALT").c_str());

	// SDK5.0 (TWL) DSi-Enhanced Donor NDS path
	conf->donorTwl0Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWL0_NDS_PATH").c_str());

	// SDK5.x (TWL) DSi-Enhanced Donor NDS path
	conf->donorTwlPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWL_NDS_PATH").c_str());

	// SDK5.0 (TWL) DSi-Exclusive Donor NDS path
	conf->donorTwlOnly0Path = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWLONLY0_NDS_PATH").c_str());

	// SDK5.x (TWL) DSi-Exclusive Donor NDS path
	conf->donorTwlOnlyPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "DONORTWLONLY_NDS_PATH").c_str());

	// GBA path
	// conf->gbaPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "GBA_PATH").c_str());

	// GBA SAV path
	// conf->gbaSavPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "GBA_SAV_PATH").c_str());

	// AP-patch path
	// conf->apPatchPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "AP_FIX_PATH").c_str());

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

	// Console model
	conf->consoleModel = strtol(config_file.fetch("NDS-BOOTSTRAP", "CONSOLE_MODEL", "0").c_str(), NULL, 0);

	// Color mode
	//conf->colorMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "COLOR_MODE", "0").c_str(), NULL, 0);

	// ROM read LED
	conf->romRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "ROMREAD_LED", "0").c_str(), NULL, 0);

	// DMA ROM read LED
	conf->dmaRomRead_LED = strtol(config_file.fetch("NDS-BOOTSTRAP", "DMA_ROMREAD_LED", "0").c_str(), NULL, 0);

	// get the gamecode
	FILE* romFile = fopen(conf->ndsPath, "rb");
	char gameTid[5];
	fseek(romFile, 0xC, SEEK_SET);
	fread(gameTid, 4, 1, romFile);
	fclose(romFile);

	// Async card read
	switch(strtol(config_file.fetch("NDS-BOOTSTRAP", "ASYNC_CARD_READ", "-1").c_str(), NULL, 0)) {
		case 0:
			conf->asyncCardRead = false;
			break;
		case 1:
			conf->asyncCardRead = true;
			break;
		default:
#ifdef DEFAULT_ASYNC_CARD_READ
			// TODO: If the list gets large enough, switch to bsearch().
			for (unsigned int i = 0; i < sizeof(asyncReadExcludeList)/sizeof(asyncReadExcludeList[0]); i++) {
				if (memcmp(gameTid, asyncReadExcludeList[i], 3) == 0) {
					// Found match
					conf->asyncCardRead = false;
					break;
				}
			}
			conf->asyncCardRead = true;
#else
			conf->asyncCardRead = false;
#endif
			break;
	}

	// Card read DMA
	switch(strtol(config_file.fetch("NDS-BOOTSTRAP", "CARD_READ_DMA", "-1").c_str(), NULL, 0)) {
		case 0:
			conf->cardReadDMA = false;
			break;
		case 1:
			conf->cardReadDMA = true;
		default:
#ifdef DEFAULT_CARD_READ_DMA
			// TODO: If the list gets large enough, switch to bsearch().
			for (unsigned int i = 0; i < sizeof(cardReadDMAExcludeList)/sizeof(cardReadDMAExcludeList[0]); i++) {
				if (memcmp(gameTid, cardReadDMAExcludeList[i], 3) == 0) {
					// Found match
					conf->cardReadDMA = false;
					break;
				}
			}
			// default settings if not in list
			conf->cardReadDMA = true;
#else
			conf->cardReadDMA = false;
#endif
			break;
	}

	// Force sleep patch
	conf->forceSleepPatch = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", "0").c_str(), NULL, 0);

	// Precise volume control
	conf->preciseVolumeControl = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "PRECISE_VOLUME_CONTROL", "0").c_str(), NULL, 0);

	// Logging
	conf->logging = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "LOGGING", "0").c_str(), NULL, 0);

	// Macro mode
	conf->macroMode = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "MACRO_MODE", "0").c_str(), NULL, 0);

	// Sleep mode
	conf->sleepMode = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "SLEEP_MODE", "1").c_str(), NULL, 0);

	// Boost CPU
	switch(strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_CPU", "-1").c_str(), NULL, 0)) {
		case 0:
			conf->boostCpu = false;
			break;
		case 1:
			conf->boostCpu = true;
			break;
		default:
#ifdef DEFAULT_CPU_BOOST
			// TODO: If the list gets large enough, switch to bsearch().
			for (unsigned int i = 0; i < sizeof(twlClockExcludeList)/sizeof(twlClockExcludeList[0]); i++) {
				if (memcmp(gameTid, twlClockExcludeList[i], 3) == 0) {
					// Found match
					if(dsiFeatures() && conf->dsiMode != 0) conf->dsiMode = 0;
					conf->boostCpu = false;
					break;
				}
			}
			conf->boostCpu = true;
#else
			conf->boostCpu = false;
#endif
			break;
	}

	// Boost VRAM
	conf->boostVram = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_VRAM", "0").c_str(), NULL, 0);

	// Sound/Mic frequency
	conf->soundFreq = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "SOUND_FREQ", "0").c_str(), NULL, 0);

	// DS Phat colors
	conf->phatColors = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "PHAT_COLORS", "0").c_str(), NULL, 0);

	// GUI Language
	conf->guiLanguage = strdup(config_file.fetch("NDS-BOOTSTRAP", "GUI_LANGUAGE").c_str());

	// Hotkey
	conf->hotkey = strtol(config_file.fetch("NDS-BOOTSTRAP", "HOTKEY", "284").c_str(), NULL, 16);

	// Manual file path
	conf->manualPath = strdup(config_file.fetch("NDS-BOOTSTRAP", "MANUAL_PATH").c_str());

	// Save Relocation
	conf->saveRelocation = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "SAVE_RELOCATION", "1").c_str(), NULL, 0);
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
	} else if (strncmp(conf->guiLanguage, "zh", 2) == 0) {
		extendedFont = IgmFont::chinese;
		extendedFontPath = "nitro:/fonts/chinese.lz77";
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
	} else if (strcmp(conf->guiLanguage, "ja") == 0 || strcmp(conf->guiLanguage, "ry") == 0) {
		extendedFont = IgmFont::japanese;
		extendedFontPath = "nitro:/fonts/japanese.lz77";
	} else if (strncmp(conf->guiLanguage, "vi", 2) == 0) {
		extendedFont = IgmFont::vietnamese;
		extendedFontPath = "nitro:/fonts/vietnamese.lz77";
	}

	FILE *font = fopen("nitro:/fonts/ascii.lz77", "rb");
	if (font) {
		fread(lz77ImageBuffer, 1, sizeof_lz77ImageBuffer, font);
		LZ77_Decompress(lz77ImageBuffer, igmText->font);
		fclose(font);
	}
	font = fopen(extendedFontPath, "rb");
	if (font) {
		fread(lz77ImageBuffer, 1, sizeof_lz77ImageBuffer, font);
		LZ77_Decompress(lz77ImageBuffer, igmText->font + 0x400);
		fclose(font);
	}

	// Set In-Game Menu hotkey
	igmText->hotkey = conf->hotkey;

	if(b4ds) {
		cardengineArm7B4DS* ce7 = (cardengineArm7B4DS*)CARDENGINE_ARM7_LOCATION_BUFFERED;
		ce7->igmHotkey = conf->hotkey;
	} else {
		cardengineArm7* ce7 = (cardengineArm7*)CARDENGINEI_ARM7_BUFFERED_LOCATION;
		ce7->igmHotkey = conf->hotkey;
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
	int rc = 0;
	fatMountSimple("sd", &__my_io_dsisd);
	fatMountSimple("fat", dldiGetInternal());

	conf->sdFound = (access("sd:/", F_OK) == 0);
	const bool flashcardFound = (access("fat:/", F_OK) == 0);
	const bool scfgSdmmcEnabled = (*(u8*)0x02FFFDF4 == 1);

	if (!conf->sdFound && !flashcardFound) {
		myConsoleDemoInit();
		iprintf("FAT init failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	if ((strncmp (bootstrapPath, "sd:/", 4) != 0) && (strncmp (bootstrapPath, "fat:/", 5) != 0)) {
		//bootstrapPath = "sd:/_nds/nds-bootstrap-release.nds";
		bootstrapPath = conf->sdFound ? "sd:/_nds/nds-bootstrap-nightly.nds" : "fat:/_nds/nds-bootstrap-nightly.nds";
	}
	if (!nitroFSInit(bootstrapPath)) {
		myConsoleDemoInit();
		iprintf("nitroFSInit failed!\n");
		return -1;
	}
	
	if (*(u16*)0x02FFFC30 == 0) {
		sysSetCartOwner(BUS_OWNER_ARM9); // Allow arm9 to access GBA ROM
		if (*(u16*)(0x020000C0) != 0x334D && *(u16*)(0x020000C0) != 0x3647 && *(u16*)(0x020000C0) != 0x4353 && *(u16*)(0x020000C0) != 0x5A45) {
			*(u16*)(0x020000C0) = 0;	// Clear Slot-2 flashcard flag
		}

		if (*(u16*)(0x020000C0) == 0) {
			if (io_dldi_data->ioInterface.features & FEATURE_SLOT_NDS) {
				*(vu16*)(0x08000000) = 0x4D54;	// Write test
				if (*(vu16*)(0x08000000) != 0x4D54) {	// If not writeable
					_M3_changeMode(M3_MODE_RAM);	// Try again with M3
					*(u16*)(0x020000C0) = 0x334D;
					*(vu16*)(0x08000000) = 0x4D54;
				}
				if (*(vu16*)(0x08000000) != 0x4D54) {
					_G6_SelectOperation(G6_MODE_RAM);	// Try again with G6
					*(u16*)(0x020000C0) = 0x3647;
					*(vu16*)(0x08000000) = 0x4D54;
				}
				if (*(vu16*)(0x08000000) != 0x4D54) {
					_SC_changeMode(SC_MODE_RAM);	// Try again with SuperCard
					*(u16*)(0x020000C0) = 0x4353;
					*(vu16*)(0x08000000) = 0x4D54;
				}
				/* if (*(vu16*)(0x08000000) != 0x4D54) {
					cExpansion::SetRompage(381);	// Try again with EZ Flash (TWLMenu++ required)
					cExpansion::OpenNorWrite();
					cExpansion::SetSerialMode();
					*(u16*)(0x020000C0) = 0x5A45;
					*(vu16*)(0x08000000) = 0x4D54;
				} */
				if (*(vu16*)(0x08000000) != 0x4D54) {
					*(u16*)(0x020000C0) = 0;
				}
			} else if (io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA) {
				if (memcmp(io_dldi_data->friendlyName, "M3 Adapter", 10) == 0) {
					*(u16*)(0x020000C0) = 0x334D;
				} else if (memcmp(io_dldi_data->friendlyName, "G6", 2) == 0) {
					*(u16*)(0x020000C0) = 0x3647;
				} else if (memcmp(io_dldi_data->friendlyName, "SuperCard", 9) == 0 || memcmp(io_dldi_data->friendlyName, "SCSD", 4) == 0) {
					_SC_changeMode(SC_MODE_RAM);
					*(vu16*)(0x08000000) = 0x4D54;
					if(*(vu16*)(0x08000000) == 0x4D54) {
						*(u16*)(0x020000C0) = 0x4353;
					}
					_SC_changeMode(SC_MODE_MEDIA);
				}
			}
		}
	}

	// Fix DLDI driver size
	if ((memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0
	  || memcmp(io_dldi_data->friendlyName, "SuperCard Rumble (MiniSD)", 25) == 0)
	&& io_dldi_data->driverSize >= 0x0E) {
		DLDI_INTERFACE* dldiWrite = (DLDI_INTERFACE*)io_dldi_data;
		dldiWrite->driverSize = 0x0D;
	}

	conf->bootstrapOnFlashcard = ((bootstrapPath[0] == 'f' && bootstrapPath[1] == 'a' && bootstrapPath[2] == 't') || !conf->sdFound);

	load_conf(conf, conf->bootstrapOnFlashcard ? "fat:/_nds/nds-bootstrap.ini" : "sd:/_nds/nds-bootstrap.ini");

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

	if (dsiFeatures() && !conf->b4dsMode) {
		loadColorLut(conf->bootstrapOnFlashcard, conf->phatColors);
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
	/* if (extension(conf->apPatchPath, ".bin")) {
		conf->valueBits |= BIT(5);
	} */
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
	if (conf->sleepMode) {
		conf->valueBits3 |= BIT(2);
	}
	if (conf->sdFound) {
		FILE* pit = fopen("sd:/private/ds/app/484E494A/pit.bin", "rb");
		if (pit) {
			fseek(pit, 0x18, SEEK_SET);
			u32 word = 0;
			fread(&word, sizeof(u32), 1, pit);
			if (word == 0x022D0000 || word == 0x023213A0) {
				// Memory Pit found
				conf->valueBits3 |= BIT(1);
			}
			fclose(pit);
		}
	}

	if (!conf->gameOnFlashcard) {
		if (access("sd:/_nds/nds-bootstrap/TWLFontTable.dat", F_OK) == 0) {
			conf->valueBits3 |= BIT(3);
		}
		if (access("sd:/_nds/nds-bootstrap/CHNFontTable.dat", F_OK) == 0) {
			conf->valueBits3 |= BIT(4);
		}
		if (access("sd:/_nds/nds-bootstrap/KORFontTable.dat", F_OK) == 0) {
			conf->valueBits3 |= BIT(5);
		}
	}

	if (conf->sdFound) {
		mkdir("sd:/_nds", 0777);
		mkdir("sd:/_nds/nds-bootstrap", 0777);
		mkdir("sd:/_nds/nds-bootstrap/patchOffsetCache", 0777);
	}
	if (flashcardFound) {
		mkdir("fat:/_nds", 0777);
		mkdir("fat:/_nds/nds-bootstrap", 0777);
		mkdir("fat:/_nds/nds-bootstrap/patchOffsetCache", 0777);
	}

	pageFilePath = "sd:/_nds/pagefile.sys";
	if (conf->b4dsMode || conf->bootstrapOnFlashcard) {
		pageFilePath = "fat:/_nds/pagefile.sys";	
	}

	conf->romSize = getFileSize(conf->ndsPath);

	char romTid[5] = {0};
	u8 unitCode = 0;
	u8 romVersion = 0;
	u32 ndsArm9BinOffset = 0;
	u32 ndsArm9Offset = 0;
	u32 ndsArm7BinOffset = 0;
	u32 ndsArm7Size = 0;
	u32 fatAddr = 0;
	u32 internalRomSize = 0;
	u16 headerCRC = 0;
	u32 a7mbk6 = 0;
	u32 accessControl = 0;
	u32 ndsArm9isrc = 0;
	u32 ndsArm9idst = 0;
	u32 ndsArm9ilen = 0;
	u32 ndsArm7isrc = 0;
	u32 ndsArm7idst = 0;
	u32 ndsArm7ilen = 0;
	u8 shared2len = 0;
	u32 modcrypt1off = 0;
	u32 modcrypt1len = 0;
	u32 modcrypt2len = 0;
	u32 pubSize = 0;
	u32 prvSize = 0;
	FILE* ndsFile = fopen(conf->ndsPath, "rb");
	if (ndsFile) {
		fseek(ndsFile, 0xC, SEEK_SET);
		fread(&romTid, 1, 4, ndsFile);
		fseek(ndsFile, 0x12, SEEK_SET);
		fread(&unitCode, 1, 1, ndsFile);
		fseek(ndsFile, 0x1E, SEEK_SET);
		fread(&romVersion, 1, 1, ndsFile);
		fseek(ndsFile, 0x20, SEEK_SET);
		fread(&ndsArm9BinOffset, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x28, SEEK_SET);
		fread(&ndsArm9Offset, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x30, SEEK_SET);
		fread(&ndsArm7BinOffset, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x3C, SEEK_SET);
		fread(&ndsArm7Size, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x48, SEEK_SET);
		fread(&fatAddr, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x80, SEEK_SET);
		fread(&internalRomSize, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x15E, SEEK_SET);
		fread(&headerCRC, sizeof(u16), 1, ndsFile);
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
		fseek(ndsFile, 0x20C, SEEK_SET);
		fread(&shared2len, sizeof(u8), 1, ndsFile);
		fseek(ndsFile, 0x220, SEEK_SET);
		fread(&modcrypt1off, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x224, SEEK_SET);
		fread(&modcrypt1len, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x22C, SEEK_SET);
		fread(&modcrypt2len, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x238, SEEK_SET);
		fread(&pubSize, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x23C, SEEK_SET);
		fread(&prvSize, sizeof(u32), 1, ndsFile);
	}

	conf->useSdk20Donor = (memcmp(romTid, "AYI", 3) == 0 && unitCode == 0 && ndsArm7Size == 0x25F70);

	u32 donorArm7iOffset = 0;
	u32 donorModcrypt2len = 0;

	FILE* cebin = NULL;
	FILE* donorNdsFile = NULL;
	bool donorLoaded = false;
	conf->isDSiWare = (dsiFeatures() && !conf->b4dsMode && ((unitCode == 3 && (accessControl & BIT(4)))
					|| (unitCode == 2 && conf->dsiMode && romTid[0] == 'K')));
	bool dsiEnhancedMbk = false;
	bool b4dsDebugRam = false;
	bool romFSInited = false;
	bool donorInsideNds = (strncmp(romTid, "KCX", 3) == 0 || strncmp(romTid, "KAV", 3) == 0 || strncmp(romTid, "KNK", 3) == 0 || strncmp(romTid, "KE3", 3) == 0); // B4DS-specific
	bool startMultibootSrl = false; // B4DS-specific

	const char* multibootSrl = "rom:/child.srl"; // Art Style: DIGIDRIVE (strncmp(romTid, "KAV", 3) == 0)
												// PictureBook Games: The Royal Bluff (USA) (strcmp(romTid, "KE3E") == 0)
	if (strncmp(romTid, "KCX", 3) == 0) {
		multibootSrl = "rom:/mb_main.srl"; // Cosmo Fighters
	} else if (strncmp(romTid, "KNK", 3) == 0) {
		multibootSrl = "rom:/main_rom.srl"; // Ideyou Sukeno: Kenkou Maja DSi
	} else if (strcmp(romTid, "KE3V") == 0) {
		multibootSrl = "rom:/child_eng.srl"; // PictureBook Games: The Royal Bluff (Europe, Australia)
	} else if (strcmp(romTid, "KE3P") == 0) {
		multibootSrl = "rom:/child_fre.srl"; // PictureBook Games: The Royal Bluff (Europe)
	}

	if (conf->isDSiWare) {
		conf->valueBits2 |= BIT(0);

	}
	if (conf->gameOnFlashcard && (conf->isDSiWare || (accessControl & BIT(4)))) {
		if (romTid[3] == 'K') {
			sharedFontPath = "fat:/_nds/nds-bootstrap/KORFontTable.dat";
			if (access(sharedFontPath.c_str(), F_OK) == 0) {
				conf->valueBits3 |= BIT(5);
			}
		} else if (romTid[3] == 'C') {
			sharedFontPath = "fat:/_nds/nds-bootstrap/CHNFontTable.dat";
			if (access(sharedFontPath.c_str(), F_OK) == 0) {
				conf->valueBits3 |= BIT(4);
			}
		} else {
			sharedFontPath = "fat:/_nds/nds-bootstrap/TWLFontTable.dat";
			if (access(sharedFontPath.c_str(), F_OK) == 0) {
				conf->valueBits3 |= BIT(3);
			}
		}
	}

	bool useTwlCfg = (dsiFeatures() && (*(u8*)0x02000400 != 0) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (*(u8*)0x02000448 != 0));
	bool nandMounted = false;
	if (!useTwlCfg && !conf->b4dsMode && isDSiMode() && conf->sdFound && conf->consoleModel < 2) {
		bool sdNandFound = (conf->sdNand && access("sd:/shared1/TWLCFG0.dat", F_OK) == 0 && access("sd:/sys/HWINFO_N.dat", F_OK) == 0 && REG_SCFG_EXT7 == 0);
		if (!sdNandFound) {
			nandMounted = fatMountSimple("nand", &io_dsi_nand);
		}

		if (nandMounted || sdNandFound) {
			FILE* twlCfgFile = fopen(nandMounted ? "nand:/shared1/TWLCFG0.dat" : "sd:/shared1/TWLCFG0.dat", "rb");
			fseek(twlCfgFile, 0x88, SEEK_SET);
			fread((void*)0x02000400, 1, 0x128, twlCfgFile);
			fclose(twlCfgFile);

			u32 srBackendId[2] = {*(u32*)0x02000428, *(u32*)0x0200042C};
			if ((srBackendId[0] != 0x53524C41 || srBackendId[1] != 0x00030004) && isDSiMode() && !nandMounted) {
				nandMounted = fatMountSimple("nand", &io_dsi_nand);
				if (nandMounted) {
					twlCfgFile = fopen("nand:/shared1/TWLCFG0.dat", "rb");
					fseek(twlCfgFile, 0x88, SEEK_SET);
					fread((void*)0x02000400, 1, 0x128, twlCfgFile);
					fclose(twlCfgFile);
				}
			}

			// WiFi RAM data
			u8* twlCfg = (u8*)0x02000400;
			readFirmware(0x1FD, twlCfg+0x1E0, 1); // WlFirm Type (1=DWM-W015, 2=W024, 3=W028)
			u32 wlFirmVars = 0x500400;
			u32 wlFirmBase = 0x500000;
			u32 wlFirmSize = 0x02E000;
			if (twlCfg[0x1E0] == 2 || twlCfg[0x1E0] == 3) {
				wlFirmVars = 0x520000;
				wlFirmBase = 0x520000;
				wlFirmSize = 0x020000;
			}
			toncset32(twlCfg+0x1E4, wlFirmVars, 1); // WlFirm RAM vars
			toncset32(twlCfg+0x1E8, wlFirmBase, 1); // WlFirm RAM base
			toncset32(twlCfg+0x1EC, wlFirmSize, 1); // WlFirm RAM size
			twlCfgFile = fopen(nandMounted ? "nand:/sys/HWINFO_N.dat" : "sd:/sys/HWINFO_N.dat", "rb");
			fseek(twlCfgFile, 0x88, SEEK_SET);
			fread((void*)0x02000600, 1, 0x14, twlCfgFile);
			fclose(twlCfgFile);

			useTwlCfg = true;
		}
	}

	// Get region
	u8 twlCfgCountry = (useTwlCfg ? *(u8*)0x02000405 : 0);
	u8 newRegion = 0;
	if ((conf->useRomRegion || (conf->region == -1 && twlCfgCountry == 0)) && romTid[3] != 'A' && romTid[3] != 'O') {
		// Determine region by TID
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
	} else if (conf->region == -1) {
		if (twlCfgCountry != 0) {
			// Determine region by country
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
			// Determine region by language
			const int language = (conf->language >= 0 && conf->language <= 7) ? conf->language : PersonalData->language;
			if (language == 0) {
				newRegion = 0;	// Japan
			} else if (language == 6) {
				newRegion = 4;	// China
			} else if (language == 7) {
				newRegion = 5;	// Korea
			} else if (language == 1) {
				newRegion = 1;	// USA
			} else if (language >= 2 && language <= 5) {
				newRegion = 2;	// Europe
			}
		}
	} else {
		newRegion = conf->region;
	}

	const bool displayEsrb = (newRegion == 1 && memcmp(romTid, "UBR", 3) != 0 && memcmp(romTid, "HND", 3) != 0 && memcmp(romTid, "HNE", 3) != 0);

	if (dsiFeatures() && !conf->b4dsMode) {
		if (!isDSiMode()) {
			toncset((u32*)0x02400000, 0, 0x300000); // Clear leftover garbage data
		}

		dsiEnhancedMbk = (isDSiMode() && *(u32*)0x02FFE1A0 == 0x00403000 && ((REG_SCFG_EXT7 == 0) || (strncmp((const char*)0x04FFFA00, "no$gba", 6) == 0)));
		bool twlDonor = true;

		// Load donor ROM's arm7 binary, if needed
		if (conf->useSdk20Donor) {
			donorNdsFile = fopen(conf->donor20Path, "rb");
			twlDonor = false;
		} else if ((REG_SCFG_EXT7 == 0 || (!conf->isDSiWare && a7mbk6 == 0x00403000))
				&& (conf->dsiMode > 0 || conf->isDSiWare) && (a7mbk6 == (dsiEnhancedMbk ? 0x080037C0 : 0x00403000) || (romTid[0] == 'H' && ndsArm7Size < 0xC000 && ndsArm7idst == 0x02E80000 && (REG_MBK9 & 0x00FFFFFF) != 0x00FFFF0F))) {
			if (romTid[0] == 'H' && ndsArm7Size < 0xC000 && ndsArm7idst == 0x02E80000) {
				if (!nandMounted && strncmp((dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path), "nand:", 5) == 0) {
					fatMountSimple("nand", &io_dsi_nand);
				}
				donorNdsFile = fopen(dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path, "rb"); // System titles can only use an SDK 5.0 donor ROM
			} else if (strncmp(romTid, "KCX", 3) == 0 && dsiEnhancedMbk) {
				// Set cloneboot/multiboot SRL file as Donor ROM
				if (romFSInit(conf->ndsPath)) {
					donorNdsFile = fopen(multibootSrl, "rb");
				}
			} else {
				const bool sdk50 = (
				   ( dsiEnhancedMbk && ndsArm7Size == 0x1511C)
				|| ( dsiEnhancedMbk && ndsArm7Size == 0x26CC8)
				|| ( dsiEnhancedMbk && ndsArm7Size == 0x28E54)
				|| (!dsiEnhancedMbk && ndsArm7Size == 0x29EE8)
				);
				if (!nandMounted && strncmp((sdk50 ? (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path) : (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath)), "nand:", 5) == 0) {
					nandMounted = fatMountSimple("nand", &io_dsi_nand);
				}
				donorNdsFile = fopen(sdk50 ? (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path) : (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath), "rb");
				if (!donorNdsFile) {
					if (!nandMounted && (strncmp((sdk50 ? (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath) : (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path)), "nand:", 5) == 0)) {
						nandMounted = fatMountSimple("nand", &io_dsi_nand);
					}
					donorNdsFile = fopen(sdk50 ? (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath) : (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path), "rb");
				}
			}
		}

		if (donorNdsFile) {
			u32 donorArm7Offset = 0;
			fseek(donorNdsFile, 0x30, SEEK_SET);
			fread(&donorArm7Offset, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x3C, SEEK_SET);
			fread((u32*)DONOR_ROM_ARM7_SIZE_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x1A0, SEEK_SET);
			fread((u32*)DONOR_ROM_MBK6_LOCATION, sizeof(u32), 1, donorNdsFile);
			if (twlDonor) {
				a7mbk6 = *(u32*)DONOR_ROM_MBK6_LOCATION;
			}
			fseek(donorNdsFile, 0x1D0, SEEK_SET);
			fread(&donorArm7iOffset, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x1D4, SEEK_SET);
			fread((u32*)DONOR_ROM_DEVICE_LIST_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x1DC, SEEK_SET);
			fread((u32*)DONOR_ROM_ARM7I_SIZE_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x22C, SEEK_SET);
			fread(&donorModcrypt2len, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, donorArm7Offset, SEEK_SET);
			fread((u8*)DONOR_ROM_ARM7_LOCATION, 1, *(u32*)DONOR_ROM_ARM7_SIZE_LOCATION, donorNdsFile);
			donorLoaded = true;
		}
	}

	if (dsiFeatures() && !conf->b4dsMode) {
		toncset((u32*)0x02E80000, 0, 0x800); // Clear nds-bootstrap arm7i binary from RAM
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

			if (ndsArm9Offset >= 0x02400000) {
				fseek(ndsFile, ndsArm9BinOffset, SEEK_SET);
				fread((u32*)ndsArm9Offset, 1, ndsArm9ilen, ndsFile);
			}
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

			if (target[0x1C] & 2)
			{
				u8 key[16] = {0} ;
				u8 keyp[16] = {0} ;
				if ((target[0x1C] & 4) || (target[0x1BF] & 0x80))
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
					decrypt_modcrypt_area(&ctx, (u8*)((modcrypt1off == ndsArm9BinOffset) ? ndsArm9Offset : ndsArm9idst), modcrypt1len);
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

				if (target[0x1C] & 2)
				{
					u8 key[16] = {0} ;
					u8 keyp[16] = {0} ;
					if ((target[0x1C] & 4) || (target[0x1BF] & 0x80))
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
			if (romTid[0] != 'I' && memcmp(romTid, "UZP", 3) != 0 && memcmp(romTid, "HND", 3) != 0 && memcmp(romTid, "UEI", 3) != 0) {
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

				// Leave Slot-1 enabled for IR cartridges, Battle & Get: Pokémon Typing DS, DS Download Play, and Hoshizora Navi (Direction Sensing Card)
				conf->specialCard = (headerData[0xC] == 'I' || memcmp(headerData + 0xC, "UZP", 3) == 0 || memcmp(romTid, "HND", 3) == 0 || memcmp(romTid, "UEI", 3) == 0);
				if (conf->specialCard) {
					conf->valueBits2 |= BIT(4);
				} else {
					disableSlot1();
				}
			}
		}

		u32 srBackendId[2] = {0};
		// Load srBackendId
		if (REG_SCFG_EXT7 == 0 && conf->bootstrapOnFlashcard) {
			/*srBackendId[0] = 0x464B4356; // "VCKF" (My Cooking Coach)
			srBackendId[1] = 0x00030000;*/
		} else {
			cebin = fopen("sd:/_nds/nds-bootstrap/srBackendId.bin", "rb");
			fread(&srBackendId, sizeof(u32), 2, cebin);
			fclose(cebin);
		}

		if (isDSiMode() && unitCode > 0 && scfgSdmmcEnabled) {
			const bool sdNandFound = conf->sdNand && (access(conf->gameOnFlashcard ? "fat:/shared1" : "sd:/shared1", F_OK) == 0);
			const bool sdPhotoFound = conf->sdNand && (access(conf->gameOnFlashcard ? "fat:/photo" : "sd:/photo", F_OK) == 0);

			// Load device list
			addTwlDevice(0, (u8)(sdNandFound ? 0 : 0x81), 0x06, "nand", "/");
			if (!sdNandFound) {
				addTwlDevice(0, 0xA1, 0x06, "nand2", "/");
			} else {
				addTwlDevice(0, 0, 0x06, "sdmc", "/");
			}
			if (shared2len > 0) {
				addTwlDevice(0, (u8)(sdNandFound ? 0x08 : 0x09), 0x06, "share", "nand:/shared2/0000");
			}
			if (!sdNandFound) {
				addTwlDevice(0, 0, 0x06, "sdmc", "/");
			}
			if (romTid[0] == 'H') {
				addTwlDevice(0, (u8)(sdNandFound ? 0x10 : 0x11), 0x06, "shared1", "nand:/shared1");
				if (shared2len == 0) {
					addTwlDevice(0, (u8)(sdNandFound ? 0x10 : 0x11), 0x06, "shared2", "nand:/shared2");
				}
			}
			addTwlDevice(0, (u8)((sdNandFound || sdPhotoFound) ? 0x10 : 0x31), 0x06, "photo", ((sdNandFound || sdPhotoFound) ? "sdmc:/photo" : "nand2:/photo"));
			if (conf->saveOnFlashcard == conf->gameOnFlashcard) {
				if (strlen(conf->prvPath) < 62 && prvSize > 0) {
					if (strncasecmp(conf->prvPath, "sd:", 3) != 0 && strncasecmp(conf->prvPath, "fat:", 4) != 0) {
						const bool isSdmc = (strncasecmp(conf->prvPath, "sdmc:", 5) == 0);
						addTwlDevice(0, (u8)((sdNandFound || isSdmc) ? 0x08 : 0x09), 0x06, "dataPrv", conf->prvPath);
					} else {
						char twlPath[64];
						sprintf(twlPath, "sdmc%s", conf->prvPath+(conf->saveOnFlashcard ? 3 : 2));

						addTwlDevice(0, 0x08, 0x06, "dataPrv", twlPath);
					}
				}
				if (strlen(conf->savPath) < 62 && pubSize > 0) {
					if (strncasecmp(conf->savPath, "sd:", 3) != 0 && strncasecmp(conf->savPath, "fat:", 4) != 0) {
						const bool isSdmc = (strncasecmp(conf->savPath, "sdmc:", 5) == 0);
						addTwlDevice(0, (u8)((sdNandFound || isSdmc) ? 0x08 : 0x09), 0x06, "dataPub", conf->savPath);
					} else {
						char twlPath[64];
						sprintf(twlPath, "sdmc%s", conf->savPath+(conf->saveOnFlashcard ? 3 : 2));

						addTwlDevice(0, 0x08, 0x06, "dataPub", twlPath);
					}
				}
			}

			char sdmcText[4] = {'s','d','m','c'};
			tonccpy((char*)(conf->gameOnFlashcard ? 0x02EFF3C1 : 0x02EFF3C2), conf->appPath, strlen(conf->appPath));
			tonccpy((char*)0x02EFF3C0, sdmcText, 4);
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
			u32 wordBak = *(vu32*)0x03700000;
			*(vu32*)0x03700000 = 0x414C5253;
			if (*(vu32*)0x03700000 == 0x414C5253 && *(vu32*)0x03708000 == 0x414C5253) {
				conf->valueBits3 |= BIT(6); // DSi WRAM is mirrored by 32KB
			}
			*(vu32*)0x03700000 = wordBak;
		}
		if (isDSiMode() && conf->consoleModel < 2 && access("sd:/hiya.dsi", F_OK) == 0 && access("sd:/shared1/TWLCFG0.dat", F_OK) == 0 && access("sd:/sys/HWINFO_N.dat", F_OK) == 0 && REG_SCFG_EXT7 != 0) {
			FILE* twlCfgFile = fopen("sd:/shared1/TWLCFG0.dat", "rb");
			fseek(twlCfgFile, 0x88, SEEK_SET);
			fread((void*)0x02000400, 1, 0x128, twlCfgFile);
			fclose(twlCfgFile);

			u32 srBackendId[2] = {*(u32*)0x02000428, *(u32*)0x0200042C};
			if (srBackendId[0] != 0x53524C41 || srBackendId[1] != 0x00030004) {
				conf->valueBits2 |= BIT(6);
			}
		}

		if (!conf->isDSiWare || !scfgSdmmcEnabled) {
			// Load external cheat engine binary
			loadCardEngineBinary("nitro:/cardenginei_arm7_cheat.bin", (u8*)CHEAT_ENGINE_BUFFERED_LOCATION);

			if ((unitCode > 0 && conf->dsiMode) || conf->isDSiWare) {
				const bool binary3 = (REG_SCFG_EXT7 == 0 ? !dsiEnhancedMbk : (a7mbk6 != 0x00403000));

				// Load SDK5 ce7 binary
				rc = loadCardEngineBinary(
					binary3 ? "nitro:/cardenginei_arm7_twlsdk3.lz77" : "nitro:/cardenginei_arm7_twlsdk.lz77",
					(u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION
				);
				if (rc == 0) {
					if (REG_SCFG_EXT7 != 0 && !(conf->valueBits2 & BIT(6))) {
						tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION, twlmenuResetGamePath, 256);
					}
					tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION+0x100, &srBackendId, 8);
				}

				if (conf->gameOnFlashcard) {
					// Load SDK5 DLDI ce9 binary
					loadCardEngineBinary(
						binary3 ? "nitro:/cardenginei_arm9_twlsdk3_dldi.lz77" : "nitro:/cardenginei_arm9_twlsdk_dldi.lz77",
						(u8*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION
					);
				} else {
					// Load SDK5 ce9 binary
					loadCardEngineBinary(
						binary3 ? "nitro:/cardenginei_arm9_twlsdk3.lz77" : "nitro:/cardenginei_arm9_twlsdk.lz77",
						(u8*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION
					);
				}
			} else {
				// Load ce7 binary
				rc = loadCardEngineBinary(
					dsiEnhancedMbk ? "nitro:/cardenginei_arm7_alt.lz77" : "nitro:/cardenginei_arm7.lz77",
					(u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION
				);
				if (rc == 0) {
					if (REG_SCFG_EXT7 != 0 && !(conf->valueBits2 & BIT(6))) {
						tonccpy((u8*)LOADER_RETURN_LOCATION, twlmenuResetGamePath, 256);
					}
					tonccpy((u8*)LOADER_RETURN_LOCATION+0x100, &srBackendId, 8);
				}

				const bool gsdd = (memcmp(romTid, "BO5", 3) == 0);

				// Load DLDI ce9 binary
				loadCardEngineBinary(
					conf->gameOnFlashcard
				?	(gsdd ? "nitro:/cardenginei_arm9_gsdd_dldi.lz77" : "nitro:/cardenginei_arm9_dldi.lz77")
				:	(gsdd ? "nitro:/cardenginei_arm9_gsdd.lz77" : "nitro:/cardenginei_arm9.lz77")
				, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
			}

			bool found = (access(pageFilePath.c_str(), F_OK) == 0);
			if (!found) {
				myConsoleDemoInit();
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

			// Load touch fix for SM64DS (U) v1.0
			cebin = fopen("nitro:/arm7fix.bin", "rb");
			if (cebin) {
				fread((u8*)ARM7_FIX_BUFFERED_LOCATION, 1, 0x140, cebin);
			}
			fclose(cebin);

			const int language = (conf->language >= 0 && conf->language <= 7) ? conf->language : PersonalData->language;
			char path[256];
			sprintf(path, "nitro:/preLoadSettings%s-%s.pck", conf->consoleModel > 0 ? "3DS" : "DSi", getLangString(language));

			bool preLoadRes = loadPreLoadSettings(conf, path, romTid, headerCRC);
			if (!preLoadRes && language != 1) {
				sprintf(path, "nitro:/preLoadSettings%s-%s.pck", conf->consoleModel > 0 ? "3DS" : "DSi", getLangString(1));
				preLoadRes = loadPreLoadSettings(conf, path, romTid, headerCRC);
			}
			if (!preLoadRes) {
				sprintf(path, "nitro:/preLoadSettings%s.pck", conf->consoleModel > 0 ? "3DS" : "DSi");
				preLoadRes = loadPreLoadSettings(conf, path, romTid, headerCRC);
			}
			if (!preLoadRes) {
				// Set NitroFS pre-load, in case if full ROM pre-load fails
				conf->dataToPreloadAddr[0] = ndsArm7BinOffset+ndsArm7Size;
				conf->dataToPreloadSize[0] = ((internalRomSize == 0 || internalRomSize > conf->romSize) ? conf->romSize : internalRomSize)-conf->dataToPreloadAddr[0];
				// conf->dataToPreloadFrame = 0;
			}
			if (!conf->gameOnFlashcard) {
				loadAsyncLoadSettings(conf, romTid, headerCRC);
			}
		} else {
			const bool binary3 = (REG_SCFG_EXT7 == 0 ? !dsiEnhancedMbk : (a7mbk6 != 0x00403000));

			// Load ce7 binary
			rc = loadCardEngineBinary(
				binary3 ? "nitro:/cardenginei_arm7_dsiware3.lz77" : "nitro:/cardenginei_arm7_dsiware.lz77",
				(u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION
			);
			if (rc == 0) {
				if (REG_SCFG_EXT7 != 0 && !(conf->valueBits2 & BIT(6))) {
					tonccpy((u8*)LOADER_RETURN_DSIWARE_LOCATION, twlmenuResetGamePath, 256);
				}
				tonccpy((u8*)LOADER_RETURN_DSIWARE_LOCATION+0x100, &srBackendId, 8);
			}

			// Load external cheat engine binary
			loadCardEngineBinary("nitro:/cardenginei_arm7_cheat.bin", (u8*)CHEAT_ENGINE_BUFFERED_LOCATION);

			// Load ce9 binary
			loadCardEngineBinary(
				binary3 ? "nitro:/cardenginei_arm9_dsiware3.lz77" : "nitro:/cardenginei_arm9_dsiware.lz77",
				(u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION
			);

			bool found = (access(pageFilePath.c_str(), F_OK) == 0);
			if (!found) {
				myConsoleDemoInit();
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

		if (colorTable) {
			loadCardEngineBinary("nitro:/cardenginei_arm9_colorlut.bin", (u8*)CARDENGINEI_ARM9_CLUT_BUFFERED_LOCATION);

			if (invertedColors || noWhiteFade) {
				for (unsigned int i = 0; i < sizeof(colorLutMasterBrightBlacklist)/sizeof(colorLutMasterBrightBlacklist[0]); i++) {
					if (memcmp(romTid, colorLutMasterBrightBlacklist[i], 3) == 0) {
						// Found match
						invertedColors = false;
						noWhiteFade = false;

						if (conf->phatColors) {
							tonccpy(VRAM_E, (u16*)COLOR_LUT_BUFFERED_LOCATION, 0x10000); // Restore phat colors with no existing LUT applied
						} else colorTable = false;

						break;
					}
				}
			}
		}

		if (colorTable) {
			// TODO: If the list gets large enough, switch to bsearch().
			for (unsigned int i = 0; i < sizeof(colorLutBlacklist)/sizeof(colorLutBlacklist[0]); i++) {
				if (memcmp(romTid, colorLutBlacklist[i], 3) == 0) {
					// Found match
					colorTable = false;
					break;
				}
			}
		}

		if (colorTable) {
			u32 flags = 0;
			if (invertedColors) {
				flags |= BIT(0);
			} else if (noWhiteFade) {
				flags |= BIT(1);
			}

			// TODO: If the list gets large enough, switch to bsearch().
			for (unsigned int i = 0; i < sizeof(colorLutVCountBlacklist)/sizeof(colorLutVCountBlacklist[0]); i++) {
				if (memcmp(romTid, colorLutVCountBlacklist[i], 3) == 0) {
					// Found match
					flags |= BIT(2);
					break;
				}
			}

			if (conf->boostCpu || (unitCode > 0 && conf->dsiMode) || conf->isDSiWare) {
				flags |= BIT(3);
			}

			*(u32*)(CARDENGINEI_ARM9_CLUT_BUFFERED_LOCATION+4) = flags;
		}

		// Load in-game menu ce9 binary
		rc = loadCardEngineBinary("nitro:/cardenginei_arm9_igm.lz77", (u8*)igmText);
		if (rc == 0) {
			getIgmStrings(conf, false);

			screenshotPath = "sd:/_nds/nds-bootstrap/screenshots.tar";
			if (conf->bootstrapOnFlashcard) {
				screenshotPath = "fat:/_nds/nds-bootstrap/screenshots.tar";
			}

			if (getFileSize(screenshotPath.c_str()) < 0x4BCC00) {
				char buffer[2][0x100] = {{0}};

				myConsoleDemoInit();
				iprintf("Creating screenshots.tar\n");
				iprintf("Please wait...");

				if (access(screenshotPath.c_str(), F_OK) == 0) {
					remove(screenshotPath.c_str());
				}

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
				if (getFileSize(screenshotPath.c_str()) < 0x4BCC00) {
					iprintf("Failed to create screenshots.tar");
					while (1) swiWaitForVBlank();
				}
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

			cebin = fopen(pageFilePath.c_str(), "r+");
			if (colorTable) {
				u16* igmPals = (u16*)igmText;
				igmPals += IGM_PALS/2;
				for (int i = 0; i < 8; i++) {
					igmPals[i] = VRAM_E[igmPals[i] % 0x8000];
				}
			}
			fwrite((u8*)igmText, 1, 0xA000, cebin);
			fclose(cebin);
			toncset((u8*)igmText, 0, 0xA000);
		}

		// Load DS blowfish
		cebin = fopen("nitro:/encr_data.bin", "rb");
		if (cebin) {
			fread((void*)BLOWFISH_LOCATION, 1, 0x1048, cebin);
		}
		fclose(cebin);

		if (!isDSiMode() && unitCode>0 && (conf->dsiMode || conf->isDSiWare)) {
			// Load DSi ARM9 BIOS
			const u32 relocAddr9 = 0x02F70000;
			cebin = fopen("sd:/_nds/bios9i.bin", "rb");
			if (!cebin) {
				cebin = fopen("sd:/_nds/bios9i_part1.bin", "rb");
			}
			if (cebin) {
				fread((u32*)relocAddr9, 1, 0x8000, cebin);

				u16 biosAddrs[12] = {0x00CC, 0x3264, 0x3268, 0x326C, 0x33E0, 0x42C0, 0x4B88, 0x4B90, 0x4B9C, 0x4BA0, 0x4E1C, 0x4F18};
				// Relocate addresses
				for (int i = 0; i < 12; i++) {
					*(u32*)(relocAddr9+biosAddrs[i]) -= 0xFFFF0000;
					*(u32*)(relocAddr9+biosAddrs[i]) += relocAddr9;
				}
			}
			fclose(cebin);

			// Load DSi ARM7 BIOS
			const u32 relocAddr7 = relocAddr9+0x8000;
			cebin = fopen("sd:/_nds/bios7i.bin", "rb");
			if (!cebin) {
				cebin = fopen("sd:/_nds/bios7i_part1.bin", "rb");
			}
			if (cebin) {
				fread((u8*)relocAddr7, 1, 0x8000, cebin);

				// Relocate address
				*(u32*)(relocAddr7+0x58A8) += relocAddr7;
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

		conf->saveSize = getFileSize(conf->savPath);
		// conf->gbaRomSize = getFileSize(conf->gbaPath);
		// conf->gbaSaveSize = getFileSize(conf->gbaSavPath);
		conf->wideCheatSize = getFileSize(wideCheatFilePath.c_str());
		// conf->apPatchSize = getFileSize(conf->apPatchPath);
		conf->cheatSize = getFileSize(cheatFilePath.c_str());

		//bool wideCheatFound = (access(wideCheatFilePath.c_str(), F_OK) == 0);

		FILE* bootstrapImages = fopen("nitro:/bootloader_images.lz77", "rb");
		if (bootstrapImages) {
			fread(lz77ImageBuffer, 1, sizeof_lz77ImageBuffer, bootstrapImages);
			fclose(bootstrapImages);
			LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION);

			bootstrapImages = fopen("nitro:/esrbOnlineNotice.lz77", "rb");
			if (bootstrapImages) {
				fread(lz77ImageBuffer, 1, sizeof_lz77ImageBuffer, bootstrapImages);
				fclose(bootstrapImages);
				LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION+0x30000);
			}

			// Convert BMP16 images to BMP8 for bottom screen
			u16* palLocation = (u16*)lz77ImageBuffer;
			u8* buffer8 = lz77ImageBuffer+0x200;
			toncset16(palLocation, 0, 256);
			u16* buffer = (u16*)IMAGES_LOCATION;
			for (int i = 0; i < ((256*192)*2)+(256*40); i++) {
				int p = 0;
				for (p = 0; p < 256; p++) {
					if (palLocation[p] == 0) {
						palLocation[p] = buffer[i];
						break;
					} else if (palLocation[p] == buffer[i]) {
						break;
					}
				}
				buffer8[i] = p;
			}
			if (colorTable) {
				for (int i = 0; i < 256; i++) {
					palLocation[i] = VRAM_E[palLocation[i] % 0x8000] | BIT(15);
				}
			}
			toncset16((u8*)IMAGES_LOCATION, 0, ((256*192)*2)+(256*40));
			tonccpy((u8*)IMAGES_LOCATION+0x18000, lz77ImageBuffer, 0x200+((256*192)*2)+(256*40));
		}
		fclose(bootstrapImages);

		if (displayEsrb) {
			// Read ESRB rating and descriptor(s) for current title
			bootstrapImages = fopen(conf->bootstrapOnFlashcard ? "fat:/_nds/nds-bootstrap/esrb.bin" : "sd:/_nds/nds-bootstrap/esrb.bin", "rb");
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
				if (colorTable) {
					u16* buffer = (u16*)IMAGES_LOCATION;
					const int start = (*(u32*)IMAGES_LOCATION == 0x494C4E4F) ? 2 : 0; // Skip online notice flag
					for (int i = start; i < 256*192; i++) {
						buffer[i] = VRAM_E[buffer[i] % 0x8000] | BIT(15);
					}
				}
			} else {
				toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
			}
			fclose(bootstrapImages);
		} else {
			toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
		}

		if (colorTable) {
			*(u32*)(COLOR_LUT_BUFFERED_LOCATION-4) = 0x54554C63; // 'cLUT'
			tonccpy((u16*)COLOR_LUT_BUFFERED_LOCATION, VRAM_E, 0x10000);

			loadMobiclipOffsets(conf, bootstrapPath, romTid, romVersion, headerCRC);
		}
	} else {
		if (accessControl & BIT(4)) {
			uint8_t *target = new uint8_t[0x1000];
			fseek(ndsFile, 0, SEEK_SET);
			fread(target, 1, 0x1000, ndsFile);

			if (ndsArm9ilen && ndsArm9ilen <= 0x8000) {
				fseek(ndsFile, ndsArm9isrc, SEEK_SET);
				fread((u32*)0x023B8000, 1, ndsArm9ilen, ndsFile);
			}

			if ((target[0x1C] & 2) && ndsArm9ilen <= 0x8000)
			{
				u8 key[16] = {0} ;
				u8 keyp[16] = {0} ;
				if ((target[0x1C] & 4) || (target[0x1BF] & 0x80))
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
					decrypt_modcrypt_area(&ctx, (u8*)0x023B8000, modcrypt1len);
				}
			}

			delete[] target;
		}
		fclose(ndsFile);
		fclose(donorNdsFile);

		if (dsiFeatures() && (strncmp(conf->donor20Path, "fat", 3) != 0 || strncmp(conf->donor5Path, "fat", 3) != 0 || strncmp(conf->donorTwl0Path, "fat", 3) != 0 || strncmp(conf->donorTwlPath, "fat", 3) != 0)) {
			easysave::ini config_file_b4ds("fat:/_nds/nds-bootstrap.ini");

			// SDK2.0 Donor NDS path
			conf->donor20Path = strdup(config_file_b4ds.fetch("NDS-BOOTSTRAP", "DONOR20_NDS_PATH").c_str());

			// SDK5.x (NTR) Donor NDS path
			conf->donor5Path = strdup(config_file_b4ds.fetch("NDS-BOOTSTRAP", "DONOR5_NDS_PATH").c_str());

			// SDK5.0 (TWL) DSi-Enhanced Donor NDS path
			conf->donorTwl0Path = strdup(config_file_b4ds.fetch("NDS-BOOTSTRAP", "DONORTWL0_NDS_PATH").c_str());

			// SDK5.x (TWL) DSi-Enhanced Donor NDS path
			conf->donorTwlPath = strdup(config_file_b4ds.fetch("NDS-BOOTSTRAP", "DONORTWL_NDS_PATH").c_str());
		}

		conf->donorFileSize = getFileSize("fat:/_nds/nds-bootstrap/b4dsTwlDonor.bin");

		// Load external cheat engine binary
		loadCardEngineBinary("nitro:/cardenginei_arm7_cheat.bin", (u8*)CHEAT_ENGINE_LOCATION_B4DS_BUFFERED);

		// Load ce7 binary
		if (strncmp(romTid, "KWY", 3) == 0 // Mighty Milky Way
		||	strncmp(romTid, "KS3", 3) == 0 // Shantae: Risky's Revenge
		) {
			loadCardEngineBinary("nitro:/cardengine_arm7_music.bin", (u8*)CARDENGINE_ARM7_LOCATION_BUFFERED);
		} else {
			loadCardEngineBinary((strcmp(io_dldi_data->friendlyName, "Ace3DS+") == 0) ? "nitro:/cardengine_arm7_ace3dsp.bin" : "nitro:/cardengine_arm7.bin", (u8*)CARDENGINE_ARM7_LOCATION_BUFFERED);
		}

		bool found = (access(pageFilePath.c_str(), F_OK) == 0);
		if (!found) {
			myConsoleDemoInit();
			iprintf("Creating pagefile.sys\n");
			iprintf("Please wait...\n");
		}

		cebin = fopen(pageFilePath.c_str(), found ? "r+" : "wb");
		fseek(cebin, ntrPageFileSize - 1, SEEK_SET);
		fputc('\0', cebin);
		fclose(cebin);

		if (!found) {
			consoleClear();
		}

		if (conf->b4dsMode == 0) {
			*(vu32*)(0x02800000) = 0x314D454D;
			*(vu32*)(0x02C00000) = 0x324D454D;
		}

		b4dsDebugRam = (conf->b4dsMode == 2 || (*(vu32*)(0x02800000) == 0x314D454D && *(vu32*)(0x02C00000) == 0x324D454D));

		loadPreLoadSettings(conf, "nitro:/preLoadSettingsMEP.pck", romTid, headerCRC);
		if (conf->dataToPreloadAddr[0] == 0) {
			// Set NitroFS pre-load, in case if full ROM pre-load fails
			conf->dataToPreloadAddr[0] = ndsArm7BinOffset+ndsArm7Size;
			conf->dataToPreloadSize[0] = ((internalRomSize == 0 || internalRomSize > conf->romSize) ? conf->romSize : internalRomSize)-conf->dataToPreloadAddr[0];
			// conf->dataToPreloadFrame = 0;
		}

		// Load in-game menu ce9 binary
		rc = loadCardEngineBinary(
			b4dsDebugRam ? "nitro:/cardengine_arm9_igm_extmem.lz77" : "nitro:/cardengine_arm9_igm.lz77",
			b4dsDebugRam ? (u8*)INGAME_MENU_LOCATION_B4DS_EXTMEM : (u8*)INGAME_MENU_LOCATION_B4DS
		);
		if (rc == 0) {
			igmText = (struct IgmText *)(b4dsDebugRam ? INGAME_MENU_LOCATION_B4DS_EXTMEM : INGAME_MENU_LOCATION_B4DS);

			getIgmStrings(conf, true);

			screenshotPath = "fat:/_nds/nds-bootstrap/screenshots.tar";

			if (getFileSize(screenshotPath.c_str()) < 0x4BCC00) {
				char buffer[2][0x100] = {{0}};

				myConsoleDemoInit();
				iprintf("Creating screenshots.tar\n");
				iprintf("Please wait...");

				if (access(screenshotPath.c_str(), F_OK) == 0) {
					remove(screenshotPath.c_str());
				}

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
				if (getFileSize(screenshotPath.c_str()) < 0x4BCC00) {
					iprintf("Failed to create screenshots.tar");
					while (1) swiWaitForVBlank();
				}
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

			cebin = fopen(pageFilePath.c_str(), "r+");
			fwrite((u8*)igmText, 1, 0xA000, cebin);
			fclose(cebin);
			toncset((u8*)igmText, 0, 0xA000);
		}

		if (accessControl & BIT(4)) {
			romFSInited = (romFSInit(conf->ndsPath));
			startMultibootSrl = (strncmp(romTid, "KCX", 3) == 0 || (!b4dsDebugRam && strncmp(romTid, "KAV", 3) == 0) || strncmp(romTid, "KNK", 3) == 0);
		}

		const char* donorNdsPath = "";
		bool standaloneDonor = false;
		if (!b4dsDebugRam && !donorInsideNds) {
			if (a7mbk6 == 0x080037C0) {
				conf->useSdk5DonorAlt = ( // Use alternate ARM7 donor in order for below games to use more of the main RAM
					strncmp(romTid, "KII", 3) == 0 // 101 Pinball World
				||	strncmp(romTid, "KAT", 3) == 0 // AiRace: Tunnel
				||	strncmp(romTid, "K2Z", 3) == 0 // G.G Series: Altered Weapon
				||	strncmp(romTid, "KSR", 3) == 0 // Aura-Aura Climber
				||	strcmp(romTid, "KBEV") == 0 // Bejeweled Twist (Europe, Australia)
				||	strncmp(romTid, "K9G", 3) == 0 // Big Bass Arcade
				||	strncmp(romTid, "KUG", 3) == 0 // G.G Series: Drift Circuit 2
				||	strncmp(romTid, "KEI", 3) == 0 // Electroplankton: Beatnes
				||	strncmp(romTid, "KEG", 3) == 0 // Electroplankton: Lumiloop
				||	strncmp(romTid, "KEA", 3) == 0 // Electroplankton: Trapy
				||	strncmp(romTid, "KFO", 3) == 0 // Frenzic
				||	strncmp(romTid, "K5M", 3) == 0 // G.G Series: The Last Knight
				||	strncmp(romTid, "KPT", 3) == 0 // Link 'n' Launch
				||	strncmp(romTid, "KNP", 3) == 0 // Need for Speed: Nitro-X
				||	strncmp(romTid, "K9K", 3) == 0 // Nintendoji
				||	strncmp(romTid, "K6T", 3) == 0 // Orion's Odyssey
				||	strncmp(romTid, "KPS", 3) == 0 // Phantasy Star 0 Mini
				||	strncmp(romTid, "KHR", 3) == 0 // Picture Perfect: Pocket Stylist
				|| ((strncmp(romTid, "KS3", 3) == 0) && (headerCRC == 0x57FE || headerCRC == 0x2BFA)) // Shantae: Risky's Revenge (Non-proto builds and clean ROMs)
				||	strncmp(romTid, "KZU", 3) == 0 // Tales to Enjoy!: Little Red Riding Hood
				||	strncmp(romTid, "KZV", 3) == 0 // Tales to Enjoy!: Puss in Boots
				||	strncmp(romTid, "KZ7", 3) == 0 // Tales to Enjoy!: The Three Little Pigs
				||	strncmp(romTid, "KZ8", 3) == 0 // Tales to Enjoy!: The Ugly Duckling
				);
				if (!conf->useSdk5DonorAlt && io_dldi_data->driverSize >= 0x0E) {
					conf->useSdk5DonorAlt = ( // Do not use alternate ARM7 donor for games with wireless features and/or made with debugger SDK
						strncmp(romTid, "K7A", 3) != 0 // 4 Elements
					&&	strncmp(romTid, "K45", 3) != 0 // 40-in-1: Explosive Megamix
					&&	strncmp(romTid, "KV3", 3) != 0 // Anonymous Notes 3: From The Abyss
					&&	strncmp(romTid, "KV4", 3) != 0 // Anonymous Notes 4: From The Abyss
					&&	strncmp(romTid, "KAZ", 3) != 0 // ARC Style: Soccer!
					&&	strncmp(romTid, "K7B", 3) != 0 // Around the World in 80 Days
					&&	strncmp(romTid, "KVU", 3) != 0 // ATV Fever
					&&	strncmp(romTid, "K9U", 3) != 0 // ATV Quad Kings
					&&	strncmp(romTid, "KBE", 3) != 0 // Bejeweled Twist
					&&	strncmp(romTid, "KBB", 3) != 0 // Bomberman Blitz
					&&	strncmp(romTid, "K2J", 3) != 0 // Cake Ninja
					&&	strncmp(romTid, "K2N", 3) != 0 // Cake Ninja 2
					&&	strncmp(romTid, "KQL", 3) != 0 // Chuukara! Dairoujou
					&&	strncmp(romTid, "KVL", 3) != 0 // Clash of Elementalists
					&&	strncmp(romTid, "KZG", 3) != 0 // Crazy Golf
					&&	strncmp(romTid, "KF3", 3) != 0 // Dairojo! Samurai Defenders
					&&	strncmp(romTid, "K6B", 3) != 0 // Deep Sea Creatures
					&&	strncmp(romTid, "KIF", 3) != 0 // Drift Street International
					&&	strncmp(romTid, "B88", 3) != 0 // DS WiFi Settings
					&&	strncmp(romTid, "K42", 3) != 0 // Elite Forces: Unit 77
					&&	strncmp(romTid, "Z2E", 3) != 0 // Famicom Wars DS: Ushinawareta Hikari
					&&	strncmp(romTid, "DMF", 3) != 0 // Foto Showdown
					&&	strncmp(romTid, "K6J", 3) != 0 // Fuuu! Dairoujou Kai
					&&	strncmp(romTid, "KGK", 3) != 0 // Glory Days: Tactical Defense
					&&	strncmp(romTid, "KKF", 3) != 0 // Go Fetch! 2
					&&	strncmp(romTid, "KHO", 3) != 0 // Handy Hockey
					&&	strncmp(romTid, "KHM", 3) != 0 // Handy Mahjong
					&&	strncmp(romTid, "K6S", 3) != 0 // Heathcliff: Spot On
					&&	strncmp(romTid, "KHL", 3) != 0 // Hell's Kitchen VS
					&&	strncmp(romTid, "KTX", 3) != 0 // High Stakes Texas Hold'em
					&&	strncmp(romTid, "K3J", 3) != 0 // iSpot Japan
					&&	strncmp(romTid, "KIK", 3) != 0 // Ivy the Kiwi? mini
					&&	strncmp(romTid, "K9B", 3) != 0 // Jazzy Billiards
					&&	strncmp(romTid, "KD3", 3) != 0 // Jinia Supasonaru: Eiwa Rakubiki Jiten
					&&	strncmp(romTid, "KD5", 3) != 0 // Jinia Supasonaru: Waei Rakubiki Jiten
					// &&	strncmp(romTid, "K69", 3) != 0 // Katamukusho
					// &&	strncmp(romTid, "KVF", 3) != 0 // Kuizu Ongaku Nojika
					&&	strncmp(romTid, "KQ9", 3) != 0 // The Legend of Zelda: Four Swords: Anniversary Edition
					&&	strncmp(romTid, "KJO", 3) != 0 // Magnetic Joe
					&&	strncmp(romTid, "KD4", 3) != 0 // Meikyou Kokugo: Rakubiki Jiten
					&&	strncmp(romTid, "K7Z", 3) != 0 && strncmp(romTid, "K9R", 3) != 0 // My Aquarium: Seven Oceans
					&&	strncmp(romTid, "KMR", 3) != 0 // My Farm
					&&	strncmp(romTid, "KMV", 3) != 0 // My Exotic Farm
					&&	strncmp(romTid, "KL3", 3) != 0 // My Asian Farm
					&&	strncmp(romTid, "KL4", 3) != 0 // My Australian Farm
					&&	strncmp(romTid, "KSU", 3) != 0 // Number Battle
					&&	strncmp(romTid, "KUS", 3) != 0 // Paul's Shooting Adventure 2
					&&	strncmp(romTid, "DHS", 3) != 0 // Picture Perfect Hair Salon
					&&	strncmp(romTid, "KE3", 3) != 0 // PictureBook Games: The Royal Bluff
					&&	strncmp(romTid, "KPM", 3) != 0 // Pomjong
					&&	strncmp(romTid, "KLR", 3) != 0 // Puffins: Let's Race!
					&&	strncmp(romTid, "KLX", 3) != 0 // Redau Shirizu: Gunjin Shougi
					&&	strncmp(romTid, "KG4", 3) != 0 // Saikyou Ginsei Shougi
					&&	strncmp(romTid, "KRW", 3) != 0 // Sea Battle
					&&  strncmp(romTid, "KS3", 3) != 0 // Shantae: Risky's Revenge (Proto builds and non-clean ROMs)
					&&	strncmp(romTid, "KTE", 3) != 0 // Tetris Party Live
					&&	strncmp(romTid, "KTW", 3) != 0 // Thorium Wars
					&&	strncmp(romTid, "K72", 3) != 0 // True Swing Golf Express
					&&	strncmp(romTid, "KTI", 3) != 0 // Turn: The Lost Artifact
					&&	strncmp(romTid, "KUB", 3) != 0 // Ubongo
					&&	strncmp(romTid, "K7K", 3) != 0 // Zombie Blaster
					);
				}

				if (access("fat:/_nds/nds-bootstrap/b4dsTwlDonor.bin", F_OK) == 0) {
					donorNdsPath = "fat:/_nds/nds-bootstrap/b4dsTwlDonor.bin";
					standaloneDonor = true;
				} else if (conf->useSdk5DonorAlt && access(conf->donor5PathAlt, F_OK) == 0) {
					donorNdsPath = conf->donor5PathAlt;
				}
				if (strlen(donorNdsPath) <= 5) {
					if (access(conf->donorTwlPath, F_OK) == 0) {
						donorNdsPath = conf->donorTwlPath;
					} else if (access(conf->donorTwl0Path, F_OK) == 0) {
						donorNdsPath = conf->donorTwl0Path;
					} else if (access(conf->donor5Path, F_OK) == 0) {
						donorNdsPath = conf->donor5Path;
					} else if (!conf->donor5PathAlt && access(conf->donor5PathAlt, F_OK) == 0) {
						donorNdsPath = conf->donor5PathAlt;
					}
				}
			} else if (conf->useSdk20Donor) {
				if (access(conf->donor20Path, F_OK) == 0) {
					donorNdsPath = conf->donor20Path;
				}
			}
		}

		const bool gsdd = (memcmp(romTid, "BO5", 3) == 0);
		const bool foto = (strncmp(romTid, "DMF", 3) == 0 || strncmp(romTid, "DSY", 3) == 0 || strncmp(romTid, "KSY", 3) == 0);

		// Load ce9 binary
		if (b4dsDebugRam) {
			if (!startMultibootSrl && ((accessControl & BIT(4)) || (a7mbk6 == 0x080037C0 && ndsArm9Offset >= 0x02004000))) {
				if (foto) {
					loadCardEngineBinary("nitro:/cardengine_arm9_extmem_foto.lz77", (u8*)CARDENGINE_ARM9_LOCATION_BUFFERED);
				} else {
					loadCardEngineBinary(
						ndsArm9Offset >= 0x02004000 ? "nitro:/cardengine_arm9_extmem_start.lz77" : "nitro:/cardengine_arm9_extmem.lz77",
						(u8*)CARDENGINE_ARM9_LOCATION_BUFFERED
					);
				}
			} else if (gsdd) {
				loadCardEngineBinary("nitro:/cardengine_arm9_extmem_gsdd.lz77", (u8*)CARDENGINE_ARM9_LOCATION_BUFFERED);
			} else {
				loadCardEngineBinary("nitro:/cardengine_arm9_extmem.lz77", (u8*)CARDENGINE_ARM9_LOCATION_BUFFERED);
			}
		} else if (!startMultibootSrl && ((accessControl & BIT(4)) || (a7mbk6 == 0x080037C0 && ndsArm9Offset >= 0x02004000)
				 || strncmp(romTid, "AP2", 3) == 0 // Metroid Prime Pinball
				 || strncmp(romTid, "VSO", 3) == 0 // Sonic Classic Collection
		)) {
			if (foto) {
				loadCardEngineBinary("nitro:/cardengine_arm9_start_foto.lz77", (u8*)CARDENGINE_ARM9_LOCATION_BUFFERED);
			} else {
				loadCardEngineBinary(
					ndsArm9Offset >= 0x02004000 ? "nitro:/cardengine_arm9_start.lz77" : (io_dldi_data->driverSize >= 0x0E) ? "nitro:/cardengine_arm9_32.lz77" : "nitro:/cardengine_arm9.lz77",
					(u8*)CARDENGINE_ARM9_LOCATION_BUFFERED
				);
			}
		} else {
			const char* ce9path = gsdd ? "nitro:/cardengine_arm9_alt_gsdd.lz77" : "nitro:/cardengine_arm9_alt.lz77";
			if (romFSInited && donorInsideNds) {
				donorNdsPath = multibootSrl;
			}
			FILE* ndsFile = (strlen(donorNdsPath) > 5) ? fopen(donorNdsPath, "rb") : fopen(conf->ndsPath, "rb");
			if (ndsFile) {
				u32 ndsArm7Offset = 0;
				u32 ndsArm7Size = 0;

				if (standaloneDonor) {
					ndsArm7Size = conf->donorFileSize;
				} else {
					fseek(ndsFile, 0x30, SEEK_SET);
					fread(&ndsArm7Offset, sizeof(u32), 1, ndsFile);
					fseek(ndsFile, 0x3C, SEEK_SET);
					fread(&ndsArm7Size, sizeof(u32), 1, ndsFile);
				}

				if ((memcmp(romTid, "ADM", 3) == 0) && ndsArm7Size == 0x28ADC) {
					ndsArm7Size -= 0xE4; // Fix for AC:WW - Singleplayer Nookingtons
				}

				u32 arm7allocOffset = 0;

				for (int i = 0; i < 0x80; i += 4) {
					fseek(ndsFile, (ndsArm7Offset+(ndsArm7Size-0x80))+i, SEEK_SET);
					fread(&arm7allocOffset, sizeof(u32), 1, ndsFile);
					if (arm7allocOffset == 0x027C0000 || arm7allocOffset == 0x027E0000 || arm7allocOffset == 0x02FE0000) {
						break;
					}
				}

				if (arm7allocOffset != 0x027C0000) {
					// Not SDK 2.0
					u32 arm7alloc1 = 0;
					u32 arm7alloc2 = 0;

					fseek(ndsFile, (ndsArm7Offset+ndsArm7Size)-4, SEEK_SET);
					fread(&arm7alloc2, sizeof(u32), 1, ndsFile);
					fseek(ndsFile, (ndsArm7Offset+ndsArm7Size)-8, SEEK_SET);
					fread(&arm7alloc1, sizeof(u32), 1, ndsFile);
					if ((arm7alloc1 > 0x02FE0000 && arm7alloc1 < 0x03000000) || (arm7alloc1 > 0x06000000 && arm7alloc1 < 0x06020000)) {
						// TWL binary found
						fseek(ndsFile, (ndsArm7Offset+ndsArm7Size)-0xC, SEEK_SET);
						fread(&arm7alloc1, sizeof(u32), 1, ndsFile);
					}

					if ((arm7alloc1+arm7alloc2) > 0x1A800) {
						ce9path = (unitCode > 0 && ndsArm9Offset >= 0x02004000) ? "nitro:/cardengine_arm9_start.lz77" : (io_dldi_data->driverSize >= 0x0E) ? "nitro:/cardengine_arm9_32.lz77" : "nitro:/cardengine_arm9.lz77";
					} else if ((arm7alloc1+arm7alloc2) > 0x19C00) {
						ce9path = "nitro:/cardengine_arm9_alt2.lz77";
					}
				} else if (io_dldi_data->driverSize >= 0x0E) {
					ce9path = "nitro:/cardengine_arm9_alt3.lz77";
				}

				fclose(ndsFile);
			}
			loadCardEngineBinary(ce9path, (u8*)CARDENGINE_ARM9_LOCATION_BUFFERED);
		}

		// Load DS blowfish
		cebin = fopen("nitro:/encr_data.bin", "rb");
		if (cebin) {
			fread((void*)BLOWFISH_LOCATION_B4DS, 1, 0x1048, cebin);
		}
		fclose(cebin);

		cheatFilePath = "fat:/_nds/nds-bootstrap/cheatData.bin";

		conf->saveSize = getFileSize(conf->savPath);
		// conf->apPatchSize = getFileSize(conf->apPatchPath);
		conf->cheatSize = getFileSize(cheatFilePath.c_str());

		FILE* bootstrapImages = fopen("nitro:/bootloader_images.lz77", "rb");
		if (bootstrapImages) {
			fread(lz77ImageBuffer, 1, sizeof_lz77ImageBuffer, bootstrapImages);
			fclose(bootstrapImages);
			LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION);

			bootstrapImages = fopen("nitro:/esrbOnlineNotice.lz77", "rb");
			if (bootstrapImages) {
				fread(lz77ImageBuffer, 1, sizeof_lz77ImageBuffer, bootstrapImages);
				fclose(bootstrapImages);
				LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION+0x30000);
			}

			// Convert BMP16 images to BMP8 for bottom screen
			u16* palLocation = (u16*)lz77ImageBuffer;
			u8* buffer8 = lz77ImageBuffer+0x200;
			toncset16(palLocation, 0, 256);
			u16* buffer = (u16*)IMAGES_LOCATION;
			for (int i = 0; i < ((256*192)*2)+(256*40); i++) {
				int p = 0;
				for (p = 0; p < 256; p++) {
					if (palLocation[p] == 0) {
						palLocation[p] = buffer[i];
						break;
					} else if (palLocation[p] == buffer[i]) {
						break;
					}
				}
				buffer8[i] = p;
			}
			toncset16((u8*)IMAGES_LOCATION, 0, ((256*192)*2)+(256*40));
			tonccpy((u8*)IMAGES_LOCATION+0x18000, lz77ImageBuffer, 0x200+((256*192)*2)+(256*40));
		}

		if (displayEsrb) {
			// Read ESRB rating and descriptor(s) for current title
			bootstrapImages = fopen(conf->bootstrapOnFlashcard ? "fat:/_nds/nds-bootstrap/esrb.bin" : "sd:/_nds/nds-bootstrap/esrb.bin", "rb");
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

		if (foto) {
			sprintf(patchOffsetCacheFilePath, "fat:/_nds/nds-bootstrap/dsiCamera/%s-%04X.bin", romTid, headerCRC);
			if (access(patchOffsetCacheFilePath, F_OK) != 0) {
				sprintf(patchOffsetCacheFilePath, "fat:/_nds/nds-bootstrap/dsiCamera/%s.bin", romTid);
			}
			if (access(patchOffsetCacheFilePath, F_OK) != 0) {
				sprintf(patchOffsetCacheFilePath, "fat:/_nds/nds-bootstrap/dsiCamera/default.bin");
			}
		} else {
			sprintf(patchOffsetCacheFilePath, "fat:/_nds/nds-bootstrap/musicPacks/%s-%04X.pck", romTid, headerCRC);
		}

		musicsFilePath = patchOffsetCacheFilePath;
		conf->musicsSize = getFileSize(patchOffsetCacheFilePath);
	}

	conf->loaderType = 0;
	if (accessControl & BIT(4)) {
		bool loaderSet = false;

		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(dsiWareForBootloader1)/sizeof(dsiWareForBootloader1[0]); i++) {
			if (memcmp(romTid, dsiWareForBootloader1[i], 3) == 0) {
				// Found match
				loaderSet = true;
				break;
			}
		}
		if (!loaderSet) {
			for (unsigned int i = 0; i < sizeof(dsiWareForBootloader3)/sizeof(dsiWareForBootloader3[0]); i++) {
				if (memcmp(romTid, dsiWareForBootloader3[i], 3) == 0) {
					// Found match
					conf->loaderType = 2;
					loaderSet = true;
					break;
				}
			}
		}

		if (!isDSiMode() || !scfgSdmmcEnabled) {
			loadDSi2DSSavePatch(conf, bootstrapPath, romTid, romVersion, headerCRC);
		}
	} else {
		loadApFix(conf, bootstrapPath, romTid, headerCRC);
	}

	if (conf->loaderType < 2 && (strcmp(romTid, "NTRJ") == 0) && (headerCRC == 0x9B41 || headerCRC == 0x69D6)) { // Use bootloader2 for Shantae: Risky's Revenge (USA) (Review Build)
		conf->loaderType = 2;
	}

	srParamsFilePath = "sd:/_nds/nds-bootstrap/softResetParams.bin";
	if (conf->gameOnFlashcard) {
		srParamsFilePath = "fat:/_nds/nds-bootstrap/softResetParams.bin";
	}
	
	if (getFileSize(srParamsFilePath.c_str()) < 0x50) {
		const u32 buffer = 0xFFFFFFFF;

		FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "wb");
		fwrite(&buffer, sizeof(u32), 1, srParamsFile);
		fseek(srParamsFile, 0x50 - 1, SEEK_SET);
		fputc('\0', srParamsFile);
		fclose(srParamsFile);
	}

	conf->donorFileOffset = 0;

	if (romFSInited && (!dsiFeatures() || conf->b4dsMode)) {
		if (!b4dsDebugRam && (strncmp(romTid, "KAA", 3) == 0)) {
			// Decompress sdat file for Art Style: Aquia
			if (getFileSize("rom:/sound_data.sdat")) {
				u8 sdatSize0[4];
				FILE* sdatFile = fopen("rom:/sound_data.sdat", "rb");
				fread(sdatSize0, 1, 4, sdatFile);
				const u32 sdatSize = LZ77_GetLength(sdatSize0);

				u32 bssEnd = 0;
				ndsFile = fopen(conf->ndsPath, "rb");
				fseek(ndsFile, ndsArm9BinOffset+0xFB8, SEEK_SET);
				fread(&bssEnd, sizeof(u32), 1, ndsFile);
				fclose(ndsFile);

				if (bssEnd+sdatSize < 0x02280000) {
					*(u32*)(bssEnd-4) = sdatSize;
					u8* sdat = (u8*)bssEnd;
					LZX_DecodeFromFile(sdat, sdatFile, sdatSize);
				}
				fclose(sdatFile);
			}
		} else if (!b4dsDebugRam && (strncmp(romTid, "KEG", 3) == 0)) {
			// Convert stereo title intro music to mono in Electroplankton: Lumiloop
			const u32 sdatSize = getFileSize("rom:/sound_data_hw.sdat");
			if (sdatSize == 0x183300) {
				u32 bssEnd = 0;
				ndsFile = fopen(conf->ndsPath, "rb");
				fseek(ndsFile, ndsArm9BinOffset+((romTid[3] == 'J') ? 0xFC0 : 0xFD0), SEEK_SET);
				fread(&bssEnd, sizeof(u32), 1, ndsFile);
				fclose(ndsFile);

				if (bssEnd+sdatSize < 0x02280000) {
					u32 offset = 0;
					FILE* sdatFile = fopen("rom:/sound_data_hw.sdat", "rb");
					fseek(sdatFile, 0xA80+0x58, SEEK_SET);
					fread(&offset, 1, 4, sdatFile);

					if (offset == 0x609D8) {
						fseek(sdatFile, 0, SEEK_SET);

						fread((u8*)bssEnd, 1, 0xA80+0x354D4, sdatFile);
						u8* ssar = (u8*)bssEnd+0x4A0;
						toncset(ssar+0xA7, 0, 1); // Play left-channel sound on right-side speaker as well
						u8* swar = (u8*)bssEnd+0xA80;
						toncset32(swar+0x354D4, 0x56220002, 1); // Replace
						toncset32(swar+0x354D8, 0x000102F7, 1); // right-channel
						toncset32(swar+0x354DC, 0x000000E2, 1); // sound with
						toncset32(swar+0x354E0, 0x000F0000, 1); // blank sound
						toncset(swar+0x354E4, 0, 0x388);        // to reduce RAM usage
						fseek(sdatFile, 0xA80+0x609D8, SEEK_SET);
						fread(swar+0x3586C, 1, 0x121EA8, sdatFile);

						FILE* header = fopen("nitro:/dsi2dsFileMods/lumiloopSwarHeader.bin", "rb");
						fread(swar, 1, 0xD8, header);
						fclose(header);

						*(u32*)(bssEnd-4) = 0xA80+0x157714; // New sdat size
					}
					fclose(sdatFile);
				}
			}
		} else if (donorInsideNds) {
			// Set cloneboot/multiboot SRL file either to boot instead, or as Donor ROM
			FILE* ndsFile = fopen(multibootSrl, "rb");
			if (ndsFile) {
				conf->donorFileOffset = offsetOfOpenedNitroFile;
			}
			if (startMultibootSrl) {
				FILE* pageFile = fopen(pageFilePath.c_str(), "rb+");
				if (ndsFile && pageFile) {
					FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "rb+");
					fseek(srParamsFile, 0xC, SEEK_SET);
					fwrite(&offsetOfOpenedNitroFile, sizeof(u32), 1, srParamsFile);
					fclose(srParamsFile);
				}
			}
			fclose(ndsFile);
		}
	}

	u32 srlAddr = 0;
	bool srlFromPageFile = false;

	if (access(srParamsFilePath.c_str(), F_OK) == 0) {
		u32 buffer = 0;
		FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "rb");
		fseek(srParamsFile, 8, SEEK_SET);
		fread(&buffer, sizeof(u32), 1, srParamsFile);
		//fseek(srParamsFile, 0xC, SEEK_SET);
		fread(&srlAddr, sizeof(u32), 1, srParamsFile);
		fclose(srParamsFile);
		srlFromPageFile = (buffer == 0x44414F4C); // 'LOAD'
	}

	if (srlFromPageFile) {
		FILE* pageFile = fopen(pageFilePath.c_str(), "rb");
		fseek(pageFile, 0x2BFE0C, SEEK_SET);
		fread(&romTid, 1, 4, pageFile);
		fseek(pageFile, 0x2BFF5E, SEEK_SET);
		fread(&headerCRC, sizeof(u16), 1, pageFile);
		fclose(pageFile);
	} else if (srlAddr > 0) {
		FILE* ndsFile = fopen(conf->ndsPath, "rb");
		fseek(ndsFile, srlAddr+0xC, SEEK_SET);
		fread(&romTid, 1, 4, ndsFile);
		fseek(ndsFile, srlAddr+0x15E, SEEK_SET);
		fread(&headerCRC, sizeof(u16), 1, ndsFile);
		fclose(ndsFile);
	}

	sprintf(patchOffsetCacheFilePath, "%s:/_nds/nds-bootstrap/patchOffsetCache/%s-%04X.bin", conf->gameOnFlashcard ? "fat" : "sd", romTid, headerCRC);

	if (access(patchOffsetCacheFilePath, F_OK) != 0) {
		char buffer[0x200] = {0};

		FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath, "wb");
		fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
		fclose(patchOffsetCacheFile);
	}

	// Create RAM dump binary
	createRamDumpBin(conf);

	// Create AP-fixed overlay binary
	createApFixOverlayBin(conf);

	if ((!dsiFeatures() || conf->b4dsMode) || !scfgSdmmcEnabled || !conf->isDSiWare) {
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
