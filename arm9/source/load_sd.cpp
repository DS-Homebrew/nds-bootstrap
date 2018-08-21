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
#include "nds_loader_arm9.h"
#include "load_sd.h"

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

void runFile(
	bool debug_local,
	std::string filename,
	std::string savPath,
	u32 saveSize,
	u32 language,
	bool dsiMode, // SDK 5
	u32 donorSdkVer,
	u32 patchMpuRegion,
	u32 patchMpuSize,
	u32 consoleModel,
	u32 loadingScreen,
	u32 romread_LED,
	bool boostCpu,
	bool gameSoftReset,
	bool asyncPrefetch,
	bool logging,
	u32* cheat_data, u32 cheat_data_len,
	u32 backlightMode
);

void loadFromSD() {
	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("SD init failed!\n");
		return;
	}
	nocashMessage("fatInitDefault");

	if (access("fat:/", F_OK) == 0) {
		consoleDemoInit();
		printf("This edition of nds-bootstrap\n");
		printf("can only be used on the\n");
		printf("SD card.\n");
		return; //stop();
	}

	CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

	bool debug          = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "DEBUG", 0);
	std::string ndsPath = bootstrapini.GetString("NDS-BOOTSTRAP", "NDS_PATH", "");
	std::string savPath = bootstrapini.GetString("NDS-BOOTSTRAP", "SAV_PATH", "");
	u32 language        = bootstrapini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", -1);
	bool dsiMode        = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", 0); // SDK 5
	u32 donorSdkVer     = bootstrapini.GetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", 0);
	u32	patchMpuRegion  = bootstrapini.GetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", 0);
	u32	patchMpuSize    = bootstrapini.GetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", 0);
	u32 consoleModel    = bootstrapini.GetInt("NDS-BOOTSTRAP", "CONSOLE_MODEL", 0);
	u32 loadingScreen   = bootstrapini.GetInt("NDS-BOOTSTRAP", "LOADING_SCREEN", 1);
	u32 romread_LED     = bootstrapini.GetInt("NDS-BOOTSTRAP", "ROMREAD_LED", 1);
	bool gameSoftReset  = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "GAME_SOFT_RESET", 0);
	bool asyncPrefetch  = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "ASYNC_PREFETCH", 0);
	bool logging        = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "LOGGING", 0);

	std::vector<std::string> cheats;
	bootstrapini.GetStringVector("NDS-BOOTSTRAP", "CHEAT_DATA", cheats, ' ');
	static u32 cheat_data[CHEAT_DATA_MAX_LEN];
	size_t cheat_data_len = cheats.size();
	if (cheat_data_len > 0) {
		dbg_printf("Cheat data present\n");
		
		if (!checkCheatDataLen(cheat_data_len)) {
			dbg_printf("Cheat data size limit reached, the cheats are ignored!\n");
			//cheats.clear();
			cheat_data_len = 0;
		}
		
		for (size_t i = 0; i < cheat_data_len; i++) {
			const char* cheat = cheats[i].c_str();
			
			dbg_printf(cheat);
			nocashMessage(cheat);
			dbg_printf(" ");

			cheat_data[i] = strtoul(cheat, NULL, 16);
			
			nocashMessage(tohex(cheat_data[i]));
			dbg_printf(" "); 
		}
	}

	u32 backlightMode   = bootstrapini.GetInt("NDS-BOOTSTRAP", "BACKLIGHT_MODE", 0);

	bool boostCpu       = (bool)bootstrapini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);

	runFile(
		debug,
		ndsPath.c_str(),
		savPath.c_str(),
		getSaveSize(savPath.c_str()),
		language,
		dsiMode, // SDK 5
		donorSdkVer,
		patchMpuRegion,
		patchMpuSize,
		consoleModel,
		loadingScreen,
		romread_LED,
		boostCpu,
		gameSoftReset,
		asyncPrefetch,
		logging,
		cheat_data, cheat_data_len,
		backlightMode
	);
}