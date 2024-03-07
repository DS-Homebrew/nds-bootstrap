#include <stdlib.h> // strtol
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
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), font);
		LZ77_Decompress(lz77ImageBuffer, igmText->font);
		fclose(font);
	}
	font = fopen(extendedFontPath, "rb");
	if (font) {
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), font);
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
				*(u16*)(0x020000C0) = 0x4353;
			}
		  }
		}
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

	char romTid[5] = {0};
	u8 unitCode = 0;
	u32 ndsArm9Offset = 0;
	u32 ndsArm7Size = 0;
	u32 fatAddr = 0;
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
		fseek(ndsFile, 0x28, SEEK_SET);
		fread(&ndsArm9Offset, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x3C, SEEK_SET);
		fread(&ndsArm7Size, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x48, SEEK_SET);
		fread(&fatAddr, sizeof(u32), 1, ndsFile);
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
		fread(&ndsArm7idst, sizeof(u32), 1, ndsFile); if (ndsArm7idst > 0x02E80000) ndsArm7idst = 0x02E80000;
		fseek(ndsFile, 0x1DC, SEEK_SET);
		fread(&ndsArm7ilen, sizeof(u32), 1, ndsFile);
		fseek(ndsFile, 0x20C, SEEK_SET);
		fread(&shared2len, sizeof(u8), 1, ndsFile);
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
			if (twlCfg[0x1E0] == 2 || twlCfg[0x1E0] == 3) {
				toncset32(twlCfg+0x1E4, 0x520000, 1); // WlFirm RAM vars
				toncset32(twlCfg+0x1E8, 0x520000, 1); // WlFirm RAM base
				toncset32(twlCfg+0x1EC, 0x020000, 1); // WlFirm RAM size
			} else {
				toncset32(twlCfg+0x1E4, 0x500400, 1); // WlFirm RAM vars
				toncset32(twlCfg+0x1E8, 0x500000, 1); // WlFirm RAM base
				toncset32(twlCfg+0x1EC, 0x02E000, 1); // WlFirm RAM size
			}
			*(u16*)(twlCfg+0x1E2) = swiCRC16(0xFFFF, twlCfg+0x1E4, 0xC); // WlFirm CRC16

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
			int language = (conf->language >= 0 && conf->language <= 7) ? conf->language : PersonalData->language;
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

	bool displayEsrb = (newRegion == 1 && memcmp(romTid, "UBR", 3) != 0 && memcmp(romTid, "HND", 3) != 0 && memcmp(romTid, "HNE", 3) != 0);

	if (dsiFeatures() && !conf->b4dsMode) {
		dsiEnhancedMbk = (isDSiMode() && *(u32*)0x02FFE1A0 == 0x00403000 && ((REG_SCFG_EXT7 == 0) || (strncmp((const char*)0x04FFFA00, "no$gba", 6) == 0)));

		// Load donor ROM's arm7 binary, if needed
		if (conf->useSdk20Donor) {
			donorNdsFile = fopen(conf->donor20Path, "rb");
		} else if (REG_SCFG_EXT7 == 0 && (conf->dsiMode > 0 || conf->isDSiWare) && (a7mbk6 == (dsiEnhancedMbk ? 0x080037C0 : 0x00403000) || (romTid[0] == 'H' && ndsArm7Size < 0xC000 && ndsArm7idst == 0x02E80000 && (REG_MBK9 & 0x00FFFFFF) != 0x00FFFF0F))) {
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
					if (donorNdsFile) {
						fclose(donorNdsFile);
					}
					if (!nandMounted && (strncmp((sdk50 ? (dsiEnhancedMbk ? conf->donorTwlPath : conf->donorTwlOnlyPath) : (dsiEnhancedMbk ? conf->donorTwl0Path : conf->donorTwlOnly0Path)), "nand:", 5) == 0)) {
						nandMounted = fatMountSimple("nand", &io_dsi_nand);
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
			fseek(donorNdsFile, 0x30, SEEK_SET);
			fread(&donorArm7Offset, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x3C, SEEK_SET);
			fread((u32*)DONOR_ROM_ARM7_SIZE_LOCATION, sizeof(u32), 1, donorNdsFile);
			fseek(donorNdsFile, 0x1A0, SEEK_SET);
			fread((u32*)DONOR_ROM_MBK6_LOCATION, sizeof(u32), 1, donorNdsFile);
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
	if ((conf->dsiMode > 0 && unitCode > 0) || conf->isDSiWare) {
		uint8_t *target = (uint8_t *)TARGETBUFFERHEADER ;
		fseek(ndsFile, 0, SEEK_SET);
		fread(target, 1, 0x1000, ndsFile);
		toncset32((u8*)target+0x1D8, ndsArm7idst, 1);

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
		if (romTid[0] != 'I' && memcmp(romTid, "UZP", 3) != 0 && memcmp(romTid, "HND", 3) != 0) {
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

			// Leave Slot-1 enabled for IR cartridges, Battle & Get: Pokémon Typing DS, and DS Download Play
			conf->specialCard = (headerData[0xC] == 'I' || memcmp(headerData + 0xC, "UZP", 3) == 0 || memcmp(romTid, "HND", 3) == 0);
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

	if (isDSiMode() && unitCode > 0 && !conf->gameOnFlashcard) {
		const bool sdNandFound = conf->sdNand && (access("sd:/shared1", F_OK) == 0);
		const bool sdPhotoFound = conf->sdNand && (access("sd:/photo", F_OK) == 0);

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
		if (!conf->saveOnFlashcard) {
			if (strlen(conf->prvPath) < 62 && prvSize > 0) {
				if (strncasecmp(conf->prvPath, "sd:", 3) != 0) {
					const bool isSdmc = (strncasecmp(conf->prvPath, "sdmc:", 5) == 0);
					addTwlDevice(0, (u8)((sdNandFound || isSdmc) ? 0x08 : 0x09), 0x06, "dataPrv", conf->prvPath);
				} else {
					char twlPath[64];
					sprintf(twlPath, "sdmc%s", conf->prvPath+2);

					addTwlDevice(0, 0x08, 0x06, "dataPrv", twlPath);
				}
			}
			if (strlen(conf->savPath) < 62 && pubSize > 0) {
				if (strncasecmp(conf->savPath, "sd:", 3) != 0) {
					const bool isSdmc = (strncasecmp(conf->savPath, "sdmc:", 5) == 0);
					addTwlDevice(0, (u8)((sdNandFound || isSdmc) ? 0x08 : 0x09), 0x06, "dataPub", conf->savPath);
				} else {
					char twlPath[64];
					sprintf(twlPath, "sdmc%s", conf->savPath+2);

					addTwlDevice(0, 0x08, 0x06, "dataPub", twlPath);
				}
			}
		}

		if (!conf->gameOnFlashcard && strlen(conf->appPath) < 62) {
			char sdmcText[4] = {'s','d','m','c'};
			tonccpy((char*)0x02EFF3C2, conf->appPath, strlen(conf->appPath));
			tonccpy((char*)0x02EFF3C0, sdmcText, 4);
		}
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
		if (strncmp(romTid, "HND", 3) == 0 // DS Download Play
		 || strncmp(romTid, "ADA", 3) == 0 // Diamond
		 || strncmp(romTid, "APA", 3) == 0 // Pearl
		 || strncmp(romTid, "Y3E", 3) == 0 // 2006-Nen 10-Gatsu Taikenban Soft
		 || strncmp(romTid, "CPU", 3) == 0 // Platinum
		 || strncmp(romTid, "IPK", 3) == 0 // HG
		 || strncmp(romTid, "IPG", 3) == 0 // SS
		) {
			conf->dsiWramAccess = false;
		} else {
			conf->valueBits2 |= BIT(5);
		}
	}
	if (access("sd:/hiya.dsi", F_OK) == 0) {
		conf->valueBits2 |= BIT(6);
	}

  if (conf->gameOnFlashcard || !conf->isDSiWare) {
	// Load external cheat engine binary
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_BUFFERED_LOCATION, 1, 0x400, cebin);
	}
	fclose(cebin);

	if ((unitCode > 0 && conf->dsiMode) || conf->isDSiWare) {
		const bool binary3 = (REG_SCFG_EXT7 == 0 ? !dsiEnhancedMbk : (a7mbk6 != 0x00403000));

		// Load SDK5 ce7 binary
		cebin = fopen(binary3 ? "nitro:/cardenginei_arm7_twlsdk3.lz77" : "nitro:/cardenginei_arm7_twlsdk.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION);
			if (REG_SCFG_EXT7 != 0) {
				tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION, twlmenuResetGamePath, 256);
			}
			tonccpy((u8*)LOADER_RETURN_SDK5_LOCATION+0x100, &srBackendId, 8);
		}
		fclose(cebin);

		if (conf->gameOnFlashcard) {
			// Load SDK5 DLDI ce9 binary
			cebin = fopen(binary3 ? "nitro:/cardenginei_arm9_twlsdk3_dldi.lz77" : "nitro:/cardenginei_arm9_twlsdk_dldi.lz77", "rb");
			if (cebin) {
				fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
				LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION);
			}
			fclose(cebin);
		} else {
			// Load SDK5 ce9 binary
			cebin = fopen(binary3 ? "nitro:/cardenginei_arm9_twlsdk3.lz77" : "nitro:/cardenginei_arm9_twlsdk.lz77", "rb");
			if (cebin) {
				fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
				LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_SDK5_BUFFERED_LOCATION);
			}
			fclose(cebin);
		}
	} else {
		// Load ce7 binary
		cebin = fopen(dsiEnhancedMbk ? "nitro:/cardenginei_arm7_alt.lz77" : "nitro:/cardenginei_arm7.lz77", "rb");
		if (cebin) {
			fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
			LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM7_BUFFERED_LOCATION);
			if (REG_SCFG_EXT7 != 0) {
				tonccpy((u8*)LOADER_RETURN_LOCATION, twlmenuResetGamePath, 256);
			}
			tonccpy((u8*)LOADER_RETURN_LOCATION+0x100, &srBackendId, 8);
		}
		fclose(cebin);

		const bool dlp = (memcmp(romTid, "HND", 3) == 0);
		const bool gsdd = (memcmp(romTid, "BO5", 3) == 0);

		if (conf->gameOnFlashcard) {
			const char* ce9Path = "nitro:/cardenginei_arm9_dldi.lz77";
			if (gsdd) {
				ce9Path = "nitro:/cardenginei_arm9_gsdd_dldi.lz77";
			}

			// Load DLDI ce9 binary
			cebin = fopen(ce9Path, "rb");
			if (cebin) {
				fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
				LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
			}
			fclose(cebin);
		} else {
			const char* ce9Path = conf->dsiWramAccess ? "nitro:/cardenginei_arm9.lz77" : "nitro:/cardenginei_arm9_alt.lz77";
			if (dlp) {
				ce9Path = "nitro:/cardenginei_arm9_dlp.lz77";
			} else if (gsdd) {
				ce9Path = conf->dsiWramAccess ? "nitro:/cardenginei_arm9_gsdd.lz77" : "nitro:/cardenginei_arm9_gsdd_alt.lz77";
			}

			// Load ce9 binary
			cebin = fopen(ce9Path, "rb");
			if (cebin) {
				fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
				LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
			}
			fclose(cebin);

			if (!conf->dsiWramAccess && !dlp && !gsdd) {
				// Load ce9 binary (alt 2)
				cebin = fopen("nitro:/cardenginei_arm9_alt2.lz77", "rb");
				if (cebin) {
					fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
					LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION2);
				}
				fclose(cebin);
			}
		}
	}

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

	// Load touch fix for SM64DS (U) v1.0
	cebin = fopen("nitro:/arm7fix.bin", "rb");
	if (cebin) {
		fread((u8*)ARM7_FIX_BUFFERED_LOCATION, 1, 0x140, cebin);
	}
	fclose(cebin);

	FILE *file = fopen(conf->consoleModel > 0 ? (conf->bootstrapOnFlashcard ? "fat:/_nds/nds-bootstrap/preLoadSettings3DS.pck" : "sd:/_nds/nds-bootstrap/preLoadSettings3DS.pck")
											  : (conf->bootstrapOnFlashcard ? "fat:/_nds/nds-bootstrap/preLoadSettingsDSi.pck" : "sd:/_nds/nds-bootstrap/preLoadSettingsDSi.pck"), "rb");
	if (file) {
		char buf[5] = {0};
		fread(buf, 1, 4, file);
	  if (strcmp(buf, ".PCK") == 0) {

		u32 fileCount;
		fread(&fileCount, 1, sizeof(fileCount), file);

		u32 offset = 0, size = 0;

		// Try binary search for the game
		int left = 0;
		int right = fileCount;

		while (left <= right) {
			int mid = left + ((right - left) / 2);
			fseek(file, 16 + mid * 16, SEEK_SET);
			fread(buf, 1, 4, file);
			int cmp = strcmp(buf, romTid);
			if (cmp == 0) { // TID matches, check CRC
				u16 crc;
				fread(&crc, 1, sizeof(crc), file);

				if (crc == headerCRC) { // CRC matches
					fread(&offset, 1, sizeof(offset), file);
					fread(&size, 1, sizeof(size), file);
					break;
				} else if (crc < headerCRC) {
					left = mid + 1;
				} else {
					right = mid - 1;
				}
			} else if (cmp < 0) {
				left = mid + 1;
			} else {
				right = mid - 1;
			}
		}

		if (offset > 0) {
			if (size > 0x10) {
				size = 0x10;
			}
			fseek(file, offset, SEEK_SET);
			u32 *buffer = new u32[size/4];
			fread(buffer, 1, size, file);

			for (u32 i = 0; i < size/8; i++) {
				conf->dataToPreloadAddr[i] = buffer[0+(i*2)];
				conf->dataToPreloadSize[i] = buffer[1+(i*2)];
			}
			delete[] buffer;
		}
	  }
		fclose(file);
	}
  } else if (ndsArm7idst <= 0x02E80000) {
	const bool binary3 = (REG_SCFG_EXT7 == 0 ? !dsiEnhancedMbk : (a7mbk6 != 0x00403000));

	// Load ce7 binary
	cebin = fopen(binary3 ? "nitro:/cardenginei_arm7_dsiware3.lz77" : "nitro:/cardenginei_arm7_dsiware.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
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
	cebin = fopen(binary3 ? "nitro:/cardenginei_arm9_dsiware3.lz77" : "nitro:/cardenginei_arm9_dsiware.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)CARDENGINEI_ARM9_BUFFERED_LOCATION);
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

	// Load in-game menu ce9 binary
	cebin = fopen("nitro:/cardenginei_arm9_igm.lz77", "rb");
	if (cebin) {
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)igmText);

		getIgmStrings(conf, false);

		fclose(cebin);

		screenshotPath = "sd:/_nds/nds-bootstrap/screenshots.tar";
		if (conf->bootstrapOnFlashcard) {
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

		cebin = fopen(pageFilePath.c_str(), "r+");
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
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), bootstrapImages);
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION+0x18000);
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
		} else {
			toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
		}
		fclose(bootstrapImages);
	} else {
		toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
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
	cebin = fopen("nitro:/cardenginei_arm7_cheat.bin", "rb");
	if (cebin) {
		fread((u8*)CHEAT_ENGINE_LOCATION_B4DS_BUFFERED, 1, 0x400, cebin);
	}
	fclose(cebin);

	// Load ce7 binary
	if (strncmp(romTid, "KWY", 3) == 0 // Mighty Milky Way
	||	strncmp(romTid, "KS3", 3) == 0 // Shantae: Risky's Revenge
	) {
		cebin = fopen("nitro:/cardengine_arm7_music.bin", "rb");
	} else {
		cebin = fopen("nitro:/cardengine_arm7.bin", "rb");
	}
	if (cebin) {
		fread((u8*)CARDENGINE_ARM7_LOCATION_BUFFERED, 1, 0x1000, cebin);
	}
	fclose(cebin);

	bool found = (access(pageFilePath.c_str(), F_OK) == 0);
	if (!found) {
		consoleDemoInit();
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

	// Load in-game menu ce9 binary
	cebin = fopen(b4dsDebugRam ? "nitro:/cardengine_arm9_igm_extmem.lz77" : "nitro:/cardengine_arm9_igm.lz77", "rb");
	if (cebin) {
		igmText = (struct IgmText *)(b4dsDebugRam ? INGAME_MENU_LOCATION_B4DS_EXTMEM : INGAME_MENU_LOCATION_B4DS);

		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
		LZ77_Decompress(lz77ImageBuffer, (u8*)igmText);

		getIgmStrings(conf, true);

		fclose(cebin);

		screenshotPath = "fat:/_nds/nds-bootstrap/screenshots.tar";

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
			||	strncmp(romTid, "KEA", 3) == 0 // Electroplankton: Trapy
			||	strncmp(romTid, "KFO", 3) == 0 // Frenzic
			||	strncmp(romTid, "K5M", 3) == 0 // G.G Series: The Last Knight
			||	strncmp(romTid, "KPT", 3) == 0 // Link 'n' Launch
			||	strncmp(romTid, "KNP", 3) == 0 // Need for Speed: Nitro-X
			||	strncmp(romTid, "K9K", 3) == 0 // Nintendoji
			||	strncmp(romTid, "K6T", 3) == 0 // Orion's Odyssey
			||	strncmp(romTid, "KPS", 3) == 0 // Phantasy Star 0 Mini
			||	strncmp(romTid, "KHR", 3) == 0 // Picture Perfect: Hair Stylist
			|| ((strncmp(romTid, "KS3", 3) == 0) && (headerCRC == 0x57FE || headerCRC == 0x2BFA)) // Shantae: Risky's Revenge (Non-proto builds and clean ROMs)
			||	strncmp(romTid, "KZU", 3) == 0 // Tales to Enjoy!: Little Red Riding Hood
			||	strncmp(romTid, "KZV", 3) == 0 // Tales to Enjoy!: Puss in Boots
			||	strncmp(romTid, "KZ7", 3) == 0 // Tales to Enjoy!: The Three Little Pigs
			||	strncmp(romTid, "KZ8", 3) == 0 // Tales to Enjoy!: The Ugly Duckling
			);

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

	// Load ce9 binary
	if (b4dsDebugRam) {
		cebin = fopen("nitro:/cardengine_arm9_extmem.lz77", "rb");
	} else if (!startMultibootSrl && ((accessControl & BIT(4)) || (a7mbk6 == 0x080037C0 && ndsArm9Offset >= 0x02004000) || (strncmp(romTid, "AP2", 3) == 0))) {
		cebin = fopen(ndsArm9Offset >= 0x02004000 ? "nitro:/cardengine_arm9_start.lz77" : "nitro:/cardengine_arm9.lz77", "rb");
	} else {
		const char* ce9path = "nitro:/cardengine_arm9_alt.lz77";
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
					ce9path = (unitCode > 0 && ndsArm9Offset >= 0x02004000) ? "nitro:/cardengine_arm9_start.lz77" : "nitro:/cardengine_arm9.lz77";
				} else if ((arm7alloc1+arm7alloc2) > 0x19C00) {
					ce9path = "nitro:/cardengine_arm9_alt2.lz77";
				}
			}

			fclose(ndsFile);
		}
		cebin = fopen(ce9path, "rb");
	}
	if (cebin) {
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), cebin);
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
		fread(lz77ImageBuffer, 1, sizeof(lz77ImageBuffer), bootstrapImages);
		LZ77_Decompress(lz77ImageBuffer, (u8*)IMAGES_LOCATION+0x18000);
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
		} else {
			toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
		}
		fclose(bootstrapImages);
	} else {
		toncset16((u16*)IMAGES_LOCATION, 0, 256*192);
	}

	sprintf(patchOffsetCacheFilePath, "fat:/_nds/nds-bootstrap/musicPacks/%s-%04X.pck", romTid, headerCRC);

	musicsFilePath = patchOffsetCacheFilePath;
	conf->musicsSize = getFileSize(patchOffsetCacheFilePath);
  }

	conf->loader2 = false;
	if (accessControl & BIT(4)) {
		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(dsiWareForBootloader2)/sizeof(dsiWareForBootloader2[0]); i++) {
			if (memcmp(romTid, dsiWareForBootloader2[i], 3) == 0) {
				// Found match
				conf->loader2 = true;
				break;
			}
		}
	}

	if (!conf->loader2 && (strcmp(romTid, "NTRJ") == 0) && (headerCRC == 0x9B41 || headerCRC == 0x69D6)) { // Use bootloader2 for Shantae: Risky's Revenge (USA) (Review Build)
		conf->loader2 = true;
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
	if (conf->gameOnFlashcard) {
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

	conf->donorFileOffset = 0;

	if (romFSInited && (!dsiFeatures() || conf->b4dsMode) && donorInsideNds) {
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

	sprintf(patchOffsetCacheFilePath, "%s:/_nds/nds-bootstrap/patchOffsetCache/%s-%04X.bin", (conf->ndsPath[0] == 'f' && conf->ndsPath[1] == 'a' && conf->ndsPath[2] == 't') ? "fat" : "sd", romTid, headerCRC);

	if (access(patchOffsetCacheFilePath, F_OK) != 0) {
		char buffer[0x200] = {0};

		FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath, "wb");
		fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
		fclose(patchOffsetCacheFile);
	}

	if (dsiFeatures() && !conf->b4dsMode) {	// Not for B4DS
		ramDumpPath = "sd:/_nds/nds-bootstrap/ramDump.bin";
		if (conf->bootstrapOnFlashcard) {
			ramDumpPath = "fat:/_nds/nds-bootstrap/ramDump.bin";
		}

		if (access(ramDumpPath.c_str(), F_OK) != 0) {
			consoleDemoInit();
			iprintf("Allocating space for\n");
			iprintf("creating a RAM dump.\n");
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

		apFixOverlaysPath = "sd:/_nds/nds-bootstrap/apFixOverlays.bin";
		if (conf->gameOnFlashcard) {
			apFixOverlaysPath = "fat:/_nds/nds-bootstrap/apFixOverlays.bin";	
		}

		if (!conf->isDSiWare && access(apFixOverlaysPath.c_str(), F_OK) != 0) {
			consoleDemoInit();
			iprintf("Allocating space for\n");
			iprintf("AP-fixed overlays.\n");
			iprintf("Please wait...\n");

			FILE *apFixOverlaysFile = fopen(apFixOverlaysPath.c_str(), "wb");
			if (apFixOverlaysFile) {
				fseek(apFixOverlaysFile, 0x800000 - 1, SEEK_SET);
				fputc('\0', apFixOverlaysFile);
				fclose(apFixOverlaysFile);
			}

			consoleClear();
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

		apFixOverlaysPath = "fat:/_nds/nds-bootstrap/apFixOverlays.bin";

		if (!conf->isDSiWare && access(apFixOverlaysPath.c_str(), F_OK) != 0) {
			consoleDemoInit();
			iprintf("Allocating space for\n");
			iprintf("AP-fixed overlays.\n");
			iprintf("Please wait...\n");

			FILE *apFixOverlaysFile = fopen(apFixOverlaysPath.c_str(), "wb");
			if (apFixOverlaysFile) {
				fseek(apFixOverlaysFile, 0x800000 - 1, SEEK_SET);
				fputc('\0', apFixOverlaysFile);
				fclose(apFixOverlaysFile);
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