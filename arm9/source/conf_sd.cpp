#include "inifile.h"
//#include <stdio.h>
//#include <nds.h>
#include <cstring>
#include <cstdarg>
#include <climits>
#include <unistd.h>
#include <nds/ndstypes.h>
#include <nds/arm9/input.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>

extern "C" {
#include <nds/system.h>
#include <nds/debug.h>
#include "hex.h"
}

#include <fat.h>

#include "cheat_engine.h"
#include "configuration.h"
#include "nds_loader_arm9.h"
#include "conf_sd.h"

int dbg_printf(const char* format, ...);

off_t getSaveSize(const char* path) {
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

void loadFromSD(configuration* conf) {
	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("SD init failed!\n");
		return;
	}
	nocashMessage("fatInitDefault");

	conf->status = 0;

	CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

	conf->debug          = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "DEBUG", 0);

	std::string filenameStr = bootstrapini.GetString("NDS-BOOTSTRAP", "NDS_PATH", "");
	conf->filename = new char[filenameStr.length() + 1];
	std::strcpy(conf->filename, filenameStr.c_str());

	std::string savPathStr = bootstrapini.GetString("NDS-BOOTSTRAP", "SAV_PATH", "");
	conf->savPath = new char[savPathStr.length() + 1];
	std::strcpy(conf->savPath, savPathStr.c_str());

	conf->saveSize       = getSaveSize(conf->savPath);
	conf->language       = bootstrapini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", -1);
	conf->dsiMode        = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", 0); // SDK 5
	conf->donorSdkVer    = bootstrapini.GetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", 0);
	conf->patchMpuRegion = bootstrapini.GetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", 0);
	conf->patchMpuSize   = bootstrapini.GetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", 0);
	conf->consoleModel   = bootstrapini.GetInt("NDS-BOOTSTRAP", "CONSOLE_MODEL", 0);
	conf->loadingScreen  = bootstrapini.GetInt("NDS-BOOTSTRAP", "LOADING_SCREEN", 1);
	conf->romread_LED    = bootstrapini.GetInt("NDS-BOOTSTRAP", "ROMREAD_LED", 1);
	conf->gameSoftReset  = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "GAME_SOFT_RESET", 0);
	conf->asyncPrefetch  = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "ASYNC_PREFETCH", 0);
	conf->logging        = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "LOGGING", 0);

	std::vector<char*> argarray;
	if (strcasecmp(conf->filename + strlen(conf->filename) - 5, ".argv") == 0) {
		FILE* argfile = fopen(conf->filename, "rb");
		char str[PATH_MAX], *pstr;
		const char seps[] = "\n\r\t ";

		while (fgets(str, PATH_MAX, argfile)) {
			// Find comment and end string there
			if ((pstr = strchr(str, '#'))) {
				*pstr = '\0';
			}

			// Tokenize arguments
			pstr = strtok(str, seps);

			while (pstr != NULL) {
				argarray.push_back(strdup(pstr));
				pstr= strtok(NULL, seps);
			}
		}
		fclose(argfile);
		conf->filename = argarray.at(0);
	} else {
		argarray.push_back(strdup(conf->filename));
	}
	conf->argc = argarray.size();
	conf->argv = (const char**)&argarray[0];

	std::vector<std::string> cheats;
	bootstrapini.GetStringVector("NDS-BOOTSTRAP", "CHEAT_DATA", cheats, ' ');
	conf->cheat_data_len = cheats.size();
	if (conf->cheat_data_len > 0) {
		dbg_printf("Cheat data present\n");
		
		if (!checkCheatDataLen(conf->cheat_data_len)) {
			dbg_printf("Cheat data size limit reached, the cheats are ignored!\n");
			//cheats.clear();
			conf->cheat_data_len = 0;
		}
		
		for (size_t i = 0; i < conf->cheat_data_len; i++) {
			const char* cheat = cheats[i].c_str();
			
			dbg_printf(cheat);
			nocashMessage(cheat);
			dbg_printf(" ");

			conf->cheat_data[i] = strtoul(cheat, NULL, 16);
			
			nocashMessage(tohex(conf->cheat_data[i]));
			dbg_printf(" "); 
		}
	}

	conf->backlightMode = bootstrapini.GetInt("NDS-BOOTSTRAP", "BACKLIGHT_MODE", 0);
	conf->boostCpu      = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
}