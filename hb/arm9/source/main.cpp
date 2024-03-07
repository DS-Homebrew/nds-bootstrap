/*-----------------------------------------------------------------

 Copyright (C) 2010  Dave "WinterMute" Murphy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include <nds.h>
#include <nds/fifocommon.h>
#include <fat.h>
#include <limits.h>

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>

#include <easysave/ini.hpp>

#include "nds_loader_arm9.h"
#include "nitrofs.h"

using namespace std;

static bool debug = false;

static inline int dbg_printf( const char* format, ... )
{
	if(!debug) return 0;
	
	static FILE * debugFile;
	debugFile = fopen ("fat:/NDSBTSRPHB.LOG","a");
	
	va_list args;
    va_start( args, format );
    int ret = vprintf( format, args );
	ret = vfprintf(debugFile, format, args );
	va_end(args);
	
	fclose (debugFile);
	
    return ret;
}

//---------------------------------------------------------------------------------
void dopause() {
//---------------------------------------------------------------------------------
	iprintf("Press start...\n");
	while(1) {
		scanKeys();
		if(keysDown() & KEY_START)
			break;
		swiWaitForVBlank();
	}
	scanKeys();
}

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

void runFile(string filename, string fullPath, string homebrewArg, string ramDiskFilename, u32 ramDiskSize, string srParamsFilePath, const char* patchOffsetCacheFilePath, u32 cfgSize, int language, int dsiMode, bool boostVram, int consoleModel, u32 srTid1, u32 srTid2) {
	char filePath[256];

	getcwd (filePath, 256);
	int pathLen = strlen (filePath);
	vector<char*> argarray;
	
	if(debug) dopause();
	
	if ( strcasecmp (filename.c_str() + filename.size() - 5, ".argv") == 0 && ramDiskSize == 0) {
		FILE *argfile = fopen(filename.c_str(),"rb");
		char str[PATH_MAX], *pstr;
		const char seps[]= "\n\r\t ";

		while( fgets(str, PATH_MAX, argfile) ) {
			// Find comment and end string there
			if( (pstr = strchr(str, '#')) )
				*pstr= '\0';

			// Tokenize arguments
			pstr= strtok(str, seps);

			while( pstr != NULL ) {
				argarray.push_back(strdup(pstr));
				pstr= strtok(NULL, seps);
			}
		}
		fclose(argfile);
		filename = argarray.at(0);
	} else {
		argarray.push_back(strdup(filename.c_str()));
	}
	
	if (homebrewArg != "") {
		argarray.push_back((char*)homebrewArg.c_str());
	}

	int romFileType = -1;
	bool romIsCompressed = false;
	if ((strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 4, ".gen") == 0)
	 || (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 3, ".md") == 0))
	{
		romFileType = 0;
		romIsCompressed = ((strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 9, ".lz77.gen") == 0)
						|| (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 8, ".lz77.md") == 0));
	}
	else if ((strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 4, ".smc") == 0)
		  || (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 4, ".sfc") == 0))
	{
		romFileType = 1;
		romIsCompressed = ((strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 9, ".lz77.smc") == 0)
						|| (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 9, ".lz77.sfc") == 0));
	}
	else if (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 4, ".sms") == 0)
	{
		romFileType = 2;
		romIsCompressed = (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 9, ".lz77.sms") == 0);
	}
	else if (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 3, ".gg") == 0)
	{
		romFileType = 3;
		romIsCompressed = (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 8, ".lz77.gg") == 0);
	}
	else if (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 4, ".pce") == 0)
	{
		romFileType = 4;
		romIsCompressed = (strcasecmp (ramDiskFilename.c_str() + ramDiskFilename.size() - 9, ".lz77.pce") == 0);
	}

	if ((strcasecmp (filename.c_str() + filename.size() - 4, ".nds") != 0
	  && strcasecmp (filename.c_str() + filename.size() - 4, ".dsi") != 0
	  && strcasecmp (filename.c_str() + filename.size() - 4, ".ids") != 0
	  && strcasecmp (filename.c_str() + filename.size() - 4, ".srl") != 0
	  && strcasecmp (filename.c_str() + filename.size() - 4, ".app") != 0)
	|| argarray.size() == 0) {
		dbg_printf("no nds file specified\n");
	} else {
		char *name = argarray.at(0);
		strcpy (filePath + pathLen, name);
		free(argarray.at(0));
		argarray.at(0) = filePath;
		dbg_printf("Running %s with %d parameters\n", argarray[0], argarray.size());
		int err = runNdsFile (fullPath.c_str(), ramDiskFilename.c_str(), "fat:/snemul.cfg", ramDiskSize, srParamsFilePath.c_str(), patchOffsetCacheFilePath, cfgSize, romFileType, romIsCompressed, argarray.size(), (const char **)&argarray[0], language, dsiMode, boostVram, consoleModel, srTid1, srTid2);
		dbg_printf("Start failed. Error %i\n", err);

	}
}

/*void getSFCG_ARM9() {
	dbg_printf( "SCFG_ROM ARM9 %x\n", REG_SCFG_ROM ); 
	dbg_printf( "SCFG_CLK ARM9 %x\n", REG_SCFG_CLK ); 
	dbg_printf( "SCFG_EXT ARM9 %x\n", REG_SCFG_EXT ); 
}

void getSFCG_ARM7() {
	
	dbg_printf( "SCFG_ROM ARM7\n" );

	nocashMessage("fifoSendValue32(FIFO_USER_01,MSG_SCFG_ROM);\n");	
	fifoSendValue32(FIFO_USER_01,(long unsigned int)&REG_SCFG_ROM);	
		  
	dbg_printf( "SCFG_CLK ARM7\n" );
	
	nocashMessage("fifoSendValue32(FIFO_USER_01,MSG_SCFG_CLK);\n");	
	fifoSendValue32(FIFO_USER_01,(long unsigned int)&REG_SCFG_CLK);
	
	dbg_printf( "SCFG_EXT ARM7\n" );
	
	nocashMessage("fifoSendValue32(FIFO_USER_01,MSG_SCFG_EXT);\n");	
	fifoSendValue32(FIFO_USER_01,(long unsigned int)&REG_SCFG_EXT);

}

static void myFIFOValue32Handler(u32 value,void* data)
{
	dbg_printf( "ARM7 data %x\n", value );
}*/

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

bool extention(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

int main( int argc, char **argv) {
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);

    //consoleDemoInit();

	// No! broke no$gba compatibility
	//REG_SCFG_CLK = 0x85;

    //printf("fat init ...");    

	if (fatMountSimple("fat", get_io_dsisd())) {
    	nocashMessage("fat inited");

  		easysave::ini config_file("fat:/_nds/nds-bootstrap.ini");

        // REG_SCFG_CLK = 0x80;
		//REG_SCFG_EXT = 0x83000000; // NAND/SD Access

		debug = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "DEBUG").c_str(), NULL, 0);
		if (debug)
			consoleDemoInit();

		std::string	bootstrapPath = argv[0];
        std::string	substr = "sd:/";
        if(strncmp(bootstrapPath.c_str(), substr.c_str(), substr.size()) == 0) bootstrapPath = ReplaceAll(bootstrapPath, "sd:/", "fat:/");

		nitroFSInit(bootstrapPath.c_str());

		mkdir("fat:/_nds", 0777);
		mkdir("fat:/_nds/nds-bootstrap", 0777);
		mkdir("fat:/_nds/nds-bootstrap/patchOffsetCache", 0777);

		if ((bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "RESETSLOT1").c_str(), NULL, 0)) {
			if(REG_SCFG_MC == 0x11) { 
				iprintf("Please insert a cartridge...\n");
				do { swiWaitForVBlank(); } 
				while (REG_SCFG_MC == 0x11);
			}
			fifoSendValue32(FIFO_USER_04, 1);
		}

		// Language
		int language = strtol(config_file.fetch("NDS-BOOTSTRAP", "LANGUAGE").c_str(), NULL, 0);

		// DSi Mode
		int dsiMode = strtol(config_file.fetch("NDS-BOOTSTRAP", "DSI_MODE").c_str(), NULL, 0);

		if (dsiMode>0 || (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_CPU").c_str(), NULL, 0)) {	
			dbg_printf("CPU boosted\n");
			//REG_SCFG_CLK |= 0x1;
		} else {
			REG_SCFG_CLK = 0x80;
			fifoSendValue32(FIFO_USER_07, 1);
		}

		bool boostVram = (bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "BOOST_VRAM").c_str(), NULL, 0);
		if (dsiMode>0 || boostVram) {	
			dbg_printf("VRAM boosted\n");
		}

		// Console model
		int consoleModel = strtol(config_file.fetch("NDS-BOOTSTRAP", "CONSOLE_MODEL").c_str(), NULL, 0);

		fifoSendValue32(FIFO_USER_03, 1);
		fifoWaitValue32(FIFO_USER_05);
		for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }

		if (0 != argc ) {
			dbg_printf("arguments passed\n");
			int i;
			for (i=0; i<argc; i++ ) {
				if (argv[i]) printf("[%d] %s\n", i, argv[i]);
			}
			dbg_printf("\n");
		} else {
			dbg_printf("No arguments passed!\n");
		}

		if ((bool)strtol(config_file.fetch("NDS-BOOTSTRAP", "LOGGING").c_str(), NULL, 0)) {
			static FILE * loggingFile = fopen ("fat:/NDSBTSRP.LOG","w");
			fprintf(loggingFile, "LOGGING MODE\n");			
			fclose (loggingFile);

			// Create a big file (minimal sdengine libfat cannot append to a file)
			loggingFile = fopen("fat:/NDSBTSRP.LOG", "a");
			for (int i = 0; i < 1000; i++) {
				fprintf(loggingFile, "                                                                                                                                          \n");
			}

			fclose(loggingFile);
		} else {
			remove ("fat:/NDSBTSRP.LOG");
		}

		std::string	ndsPath(config_file.fetch("NDS-BOOTSTRAP", "NDS_PATH"));
        if(strncmp(ndsPath.c_str(), substr.c_str(), substr.size()) == 0)
			ndsPath = ReplaceAll(ndsPath, "sd:/", "fat:/");

		std::string	homebrewArg(config_file.fetch("NDS-BOOTSTRAP", "HOMEBREW_ARG"));
		if (homebrewArg != "") {
			if(strncmp(homebrewArg.c_str(), substr.c_str(), substr.size()) == 0)
				homebrewArg = ReplaceAll(homebrewArg, "sd:/", "fat:/");
		}

		std::string	ramDrivePath(config_file.fetch("NDS-BOOTSTRAP", "RAM_DRIVE_PATH"));
		if (ramDrivePath != "") {
			if(strncmp(ramDrivePath.c_str(), substr.c_str(), substr.size()) == 0)
				ramDrivePath = ReplaceAll(ramDrivePath, "sd:/", "fat:/");
		}

		std::string	romfolder = ndsPath;
		while (!romfolder.empty() && romfolder[romfolder.size()-1] != '/') {
			romfolder.resize(romfolder.size()-1);
		}
		chdir(romfolder.c_str());

		std::string	filename = ndsPath;
		const size_t last_slash_idx = filename.find_last_of("/");
		if (std::string::npos != last_slash_idx)
		{
			filename.erase(0, last_slash_idx + 1);
		}

		u32 ramDiskSize = getFileSize(ramDrivePath.c_str());
		if (ramDiskSize > 0) {
			chdir("fat:/");	// Change directory to root for RAM disk usage
		}

		u32 cfgSize = getFileSize("fat:/snemul.cfg");

		dbg_printf("Running %s\n", ndsPath.c_str());
		if (ramDiskSize > 0) {
			dbg_printf("RAM disk: %s\n", ramDrivePath.c_str());
			dbg_printf("RAM disk size: %x\n", ramDiskSize);
		}

		std::string srParamsFilePath = "fat:/_nds/nds-bootstrap/softResetParams.bin";
	
		if (getFileSize(srParamsFilePath.c_str()) < 0x10) {
			u32 buffer = 0xFFFFFFFF;

			FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "wb");
			fwrite(&buffer, sizeof(u32), 1, srParamsFile);
			fseek(srParamsFile, 0x10 - 1, SEEK_SET);
			fputc('\0', srParamsFile);
			fclose(srParamsFile);
		}

		bool tidCrcFromSrParams = false;
		char romTid[5] = {0};
		u16 headerCRC = 0;

		if (access(srParamsFilePath.c_str(), F_OK) == 0) {
			u32 buffer = 0;
			FILE* srParamsFile = fopen(srParamsFilePath.c_str(), "rb");
			fread(&buffer, sizeof(u32), 1, srParamsFile);
			tidCrcFromSrParams = (buffer != 0xFFFFFFFF);
			if (tidCrcFromSrParams) {
				fread(romTid, 1, 4, srParamsFile);
				fread(&headerCRC, sizeof(u16), 1, srParamsFile);
			}
			fclose(srParamsFile);
		}

		if (!tidCrcFromSrParams) {
			FILE* ndsFile = fopen(ndsPath.c_str(), "rb");
			fseek(ndsFile, 0xC, SEEK_SET);
			fread(romTid, 1, 4, ndsFile);
			fseek(ndsFile, 0x15E, SEEK_SET);
			fread(&headerCRC, sizeof(u16), 1, ndsFile);
			fclose(ndsFile);
		}

		if (romTid[0] == 0) {
			// Fix blank TID
			for (int i = 0; i < 4; i++) {
				romTid[i] = '#';
			}
		}

		char patchOffsetCacheFilePath[64];
		sprintf(patchOffsetCacheFilePath, "fat:/_nds/nds-bootstrap/patchOffsetCache/%s-%04X.bin", romTid, headerCRC);
		if (access(patchOffsetCacheFilePath, F_OK) != 0) {
			char buffer[0x200] = {0};

			FILE* patchOffsetCacheFile = fopen(patchOffsetCacheFilePath, "wb");
			fwrite(buffer, 1, sizeof(buffer), patchOffsetCacheFile);
			fclose(patchOffsetCacheFile);
		}

		u32 srBackendId[2] = {0};
		FILE* srBackendBin = fopen("fat:/_nds/nds-bootstrap/srBackendId.bin", "rb");
		fread(&srBackendId, sizeof(u32), 2, srBackendBin);
		fclose(srBackendBin);

		runFile(filename, ndsPath, homebrewArg, ramDrivePath, ramDiskSize, srParamsFilePath, patchOffsetCacheFilePath, cfgSize, language, dsiMode, boostVram, consoleModel, srBackendId[0], srBackendId[1]);
	} else {
		consoleDemoInit();
		printf("SD init failed!\n");
	}

	while(1) { swiWaitForVBlank(); }
}