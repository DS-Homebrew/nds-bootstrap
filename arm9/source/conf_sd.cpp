#include "inifile.h"
//#include <stdio.h>
//#include <nds.h>
#include <cstring>
#include <climits> // PATH_MAX
#include <nds/ndstypes.h>
#include <nds/fifocommon.h>
#include <nds/arm9/console.h>

extern "C" {
#include <nds/debug.h>
#include "hex.h"
}

#include <fat.h>

#include "cheat_engine.h"
#include "configuration.h"
#include "conf_sd.h"

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

int loadFromSD(configuration* conf) {
	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("SD init failed!\n");
		return -1;
	}
	nocashMessage("fatInitDefault");

	CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

	conf->debug          = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "DEBUG", 0);

	std::string ndsPathStr = bootstrapini.GetString("NDS-BOOTSTRAP", "NDS_PATH", "");
	conf->ndsPath = new char[ndsPathStr.length() + 1];
	std::strcpy(conf->ndsPath, ndsPathStr.c_str());

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

	conf->argc = 0;
	conf->argv = (const char**)malloc(ARG_MAX);
	//std::vector<char*> argv;
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
				conf->argv[conf->argc] = strdup(pstr); //strcpy(conf->argv[conf->argc], pstr);
				++conf->argc;
				//argv.push_back(strdup(pstr));

				pstr = strtok(NULL, seps);
			}
		}
		fclose(argfile);
		//conf->ndsPath = strdup(conf->argv[0]);
		//conf->ndsPath = argv.at(0);
	} else {
		conf->argv[0] = strdup(conf->ndsPath); //strcpy(conf->argv[0], conf->ndsPath);
		conf->argc = 1; //++conf->argc;
		//argv.push_back(strdup(conf->ndsPath));
	}
	realloc(conf->argv, conf->argc*sizeof(const char*));
	//conf->argc = argv.size();
	//conf->argv = (const char**)&argv[0];

	std::vector<std::string> cheats;
	//static std::vector<u32> cheat_data;
	bootstrapini.GetStringVector("NDS-BOOTSTRAP", "CHEAT_DATA", cheats, ' ');
	conf->cheat_data_len = cheats.size();
	conf->cheat_data = (u32*)malloc(conf->cheat_data_len*sizeof(u32));
	if (conf->cheat_data_len > 0) {
		printf("Cheat data present\n");
		
		if (!checkCheatDataLen(conf->cheat_data_len)) {
			printf("Cheat data size limit reached, the cheats are ignored!\n");
			cheats.clear();
			conf->cheat_data_len = 0;
		}
		
		for (size_t i = 0; i < conf->cheat_data_len; i++) {
			const char* cheat = cheats[i].c_str();
			
			printf(cheat);
			nocashMessage(cheat);
			printf(" ");

			conf->cheat_data[i] = strtoul(cheat, NULL, 16);
			//cheat_data.push_back(strtoul(cheat, NULL, 16));
			
			nocashMessage(tohex(conf->cheat_data[i]));
			printf(" "); 
		}
	}
	//conf->cheat_data = (u32*)&cheat_data[0];

	conf->backlightMode = bootstrapini.GetInt("NDS-BOOTSTRAP", "BACKLIGHT_MODE", 0);
	conf->boostCpu      = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);

	return 0;
}