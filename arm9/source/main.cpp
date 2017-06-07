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
#include <fat.h>
#include <limits.h>

#include <stdio.h>
#include <stdarg.h>

#include <nds/fifocommon.h>

#include "nds_loader_arm9.h"
#include "inifile.h"

using namespace std;

static bool debug = false;

static inline int dbg_printf( const char* format, ... )
{
	if(!debug) return 0;
	
	static FILE * debugFile;
	debugFile = fopen ("sd:/NDSBTSRP.LOG","a");
	
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

void runFile(string filename, string savPath, string arm7DonorPath, u32 patchMpuRegion, u32 patchMpuSize) {
	vector<char*> argarray;
	
	if(debug) dopause();
	
	if ( strcasecmp (filename.c_str() + filename.size() - 5, ".argv") == 0) {
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

	if ( strcasecmp (filename.c_str() + filename.size() - 4, ".nds") != 0 || argarray.size() == 0 ) {
		dbg_printf("no nds file specified\n");
	} else {
		dbg_printf("Running %s with %d parameters\n", argarray[0], argarray.size());
		int err = runNdsFile (argarray[0], strdup(savPath.c_str()), strdup(arm7DonorPath.c_str()), patchMpuRegion, patchMpuSize, argarray.size(), (const char **)&argarray[0]);
		dbg_printf("Start failed. Error %i\n", err);

	}
}

typedef struct {
	char gameTitle[12];			//!< 12 characters for the game title.
	char gameCode[4];			//!< 4 characters for the game code.
} sNDSHeadertitlecodeonly;

void getSFCG_ARM9() {
	iprintf( "SCFG_ROM ARM9 %x\n", REG_SCFG_ROM ); 
	iprintf( "SCFG_CLK ARM9 %x\n", REG_SCFG_CLK ); 
	iprintf( "SCFG_EXT ARM9 %x\n", REG_SCFG_EXT ); 
}

/* void getSFCG_ARM7() {
	
	iprintf( "SCFG_ROM ARM7\n" );

	nocashMessage("fifoSendValue32(FIFO_USER_01,MSG_SCFG_ROM);\n");	
	fifoSendValue32(FIFO_USER_01,(long unsigned int)&REG_SCFG_ROM);	
	
	nocashMessage("dbg_printf\n");	
		  
	iprintf( "SCFG_CLK ARM7\n" );
	
	nocashMessage("fifoSendValue32(FIFO_USER_01,MSG_SCFG_CLK);\n");	
	fifoSendValue32(FIFO_USER_01,(long unsigned int)&REG_SCFG_CLK);
	
	iprintf( "SCFG_EXT ARM7\n" );
	
	nocashMessage("fifoSendValue32(FIFO_USER_01,MSG_SCFG_EXT);\n");	
	fifoSendValue32(FIFO_USER_01,(long unsigned int)&REG_SCFG_EXT);

}

void myFIFOValue32Handler(u32 value,void* data)
{
	nocashMessage("myFIFOValue32Handler\n");	
	iprintf( "ARM7 data %x\n", value );
} */


void initMBK() {
	// default dsiware settings
	
	// WRAM-B fully mapped to arm7 // inverted order
	*((vu32*)REG_MBK2)=0x9195999D;
	*((vu32*)REG_MBK3)=0x8185898D;
	
	// WRAM-C fully mapped to arm7 // inverted order
	*((vu32*)REG_MBK4)=0x9195999D;
	*((vu32*)REG_MBK5)=0x8185898D;
		
	// WRAM-A not mapped (reserved to arm7)
	REG_MBK6=0x00000000;
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
}

int main( int argc, char **argv) {	

	// switch to NTR mode
	REG_SCFG_EXT = 0x83000000; // NAND/SD Access
	
	if (fatInitDefault()) {
		nocashMessage("fatInitDefault");
		CIniFile bootstrapini( "sd:/_nds/nds-bootstrap.ini" );
		
		if(bootstrapini.GetInt("NDS-BOOTSTRAP","DEBUG",0) == 1) {	
			debug=true;			
			
			consoleDemoInit();
			
			// fifoSetValue32Handler(FIFO_USER_02,myFIFOValue32Handler,0);
			
			getSFCG_ARM9();
			// getSFCG_ARM7();		// Returns 0 on DSi
		}
		
		switch(bootstrapini.GetInt("NDS-BOOTSTRAP","ROMREAD_LED",1)) {
			case 0:
			default:
				break;
			case 1:
				fifoSendValue32(FIFO_USER_01, 1);	// Set to use WiFi LED as card read indicator
				break;
			case 2:
				fifoSendValue32(FIFO_USER_02, 1);	// Set to use power LED (turn to purple) as card read indicator
				break;
		}
		
		if (bootstrapini.GetInt("NDS-BOOTSTRAP","SOFT_RESET",0) == 0) {
			fifoSendValue32(FIFO_USER_06, 1);	// Set to soft-reset to DSi Menu
		}

		std::string	ndsPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "NDS_PATH", "");	
		
		// adjust TSC[1:26h] and TSC[1:27h]
		// for certain gamecodes
		FILE *f_nds_file = fopen(ndsPath.c_str(), "rb");

		char game_TID[5];
		fseek(f_nds_file, offsetof(sNDSHeadertitlecodeonly, gameCode), SEEK_SET);
		fread(game_TID, 1, 4, f_nds_file);
		game_TID[4] = 0;
		game_TID[3] = 0;
		fclose(f_nds_file);
		
		if 	(	strcmp(game_TID, "ABX") == 0	// NTR-ABXE Bomberman Land Touch!
			|| strcmp(game_TID, "YO9") == 0	// NTR-YO9J Bokura no TV Game Kentei - Pikotto! Udedameshi
			|| strcmp(game_TID, "ALH") == 0	// NTR-ALHE Flushed Away
			|| strcmp(game_TID, "ACC") == 0	// NTR-ACCE Cooking Mama
			|| strcmp(game_TID, "YCQ") == 0	// NTR-YCQE Cooking Mama 2 - Dinner with Friends
			|| strcmp(game_TID, "YYK") == 0	// NTR-YYKE Trauma Center - Under the Knife 2
			|| strcmp(game_TID, "AZW") == 0	// NTR-AZWE WarioWare - Touched!
			|| strcmp(game_TID, "AKA") == 0	// NTR-AKAE Rub Rabbits!, The
			|| strcmp(game_TID, "AN9") == 0	// NTR-AN9E Little Mermaid - Ariel's Undersea Adventure, The
			|| strcmp(game_TID, "AKE") == 0	// NTR-AKEJ Keroro Gunsou - Enshuu da Yo! Zenin Shuugou Part 2
			|| strcmp(game_TID, "YFS") == 0	// NTR-YFSJ Frogman Show - DS Datte, Shouganaijanai, The
			|| strcmp(game_TID, "YG8") == 0	// NTR-YG8E Yu-Gi-Oh! World Championship 2008
			|| strcmp(game_TID, "AY7") == 0	// NTR-AY7E Yu-Gi-Oh! World Championship 2007
			|| strcmp(game_TID, "YON") == 0	// NTR-YONJ Minna no DS Seminar - Kantan Ongakuryoku
			|| strcmp(game_TID, "A5H") == 0	// NTR-A5HE Interactive Storybook DS - Series 2
			|| strcmp(game_TID, "A5I") == 0	// NTR-A5IE Interactive Storybook DS - Series 3
			|| strcmp(game_TID, "AMH") == 0	// NTR-AMHE Metroid Prime Hunters
			|| strcmp(game_TID, "A3T") == 0	// NTR-A3TE Tak - The Great Juju Challenge
			|| strcmp(game_TID, "YBO") == 0	// NTR-YBOE Boogie
			|| strcmp(game_TID, "ADA") == 0	// NTR-ADAE PKMN Diamond
			|| strcmp(game_TID, "APA") == 0	// NTR-APAE PKMN Pearl
			|| strcmp(game_TID, "CPU") == 0	// NTR-CPUE PKMN Platinum
			|| strcmp(game_TID, "APY") == 0	// NTR-APYE Puyo Pop Fever
			|| strcmp(game_TID, "AWH") == 0	// NTR-AWHE Bubble Bobble Double Shot
			|| strcmp(game_TID, "AXB") == 0	// NTR-AXBJ Daigassou! Band Brothers DX
			|| strcmp(game_TID, "A4U") == 0	// NTR-A4UJ Wi-Fi Taiou - Morita Shogi
			|| strcmp(game_TID, "A8N") == 0	// NTR-A8NE Planet Puzzle League
			|| strcmp(game_TID, "ABJ") == 0	// NTR-ABJE Harvest Moon DS - Island of Happiness
			|| strcmp(game_TID, "ABN") == 0	// NTR-ABNE Bomberman Story DS
			|| strcmp(game_TID, "ACL") == 0	// NTR-ACLE Custom Robo Arena
			|| strcmp(game_TID, "ART") == 0	// NTR-ARTJ Shin Lucky Star Moe Drill - Tabidachi
			|| strcmp(game_TID, "AVT") == 0	// NTR-AVTJ Kou Rate Ura Mahjong Retsuden Mukoubuchi - Goburei, Shuuryou desu ne
			|| strcmp(game_TID, "AWY") == 0	// NTR-AWYJ Wi-Fi Taiou - Gensen Table Game DS
			|| strcmp(game_TID, "AXJ") == 0	// NTR-AXJE Dungeon Explorer - Warriors of Ancient Arts
			|| strcmp(game_TID, "AYK") == 0	// NTR-AYKJ Wi-Fi Taiou - Yakuman DS
			|| strcmp(game_TID, "YB2") == 0	// NTR-YB2E Bomberman Land Touch! 2
			|| strcmp(game_TID, "YB3") == 0	// NTR-YB3E Harvest Moon DS - Sunshine Islands
			|| strcmp(game_TID, "YCH") == 0	// NTR-YCHJ Kousoku Card Battle - Card Hero
			|| strcmp(game_TID, "YFE") == 0	// NTR-YFEE Fire Emblem - Shadow Dragon
			|| strcmp(game_TID, "YGD") == 0	// NTR-YGDE Diary Girl
			|| strcmp(game_TID, "YKR") == 0	// NTR-YKRJ Culdcept DS
			|| strcmp(game_TID, "YRM") == 0	// NTR-YRME My Secret World by Imagine
			|| strcmp(game_TID, "YW2") == 0	// NTR-YW2E Advance Wars - Days of Ruin
			|| strcmp(game_TID, "AJU") == 0	// NTR-AJUJ Jump! Ultimate Stars
			|| strcmp(game_TID, "ACZ") == 0	// NTR-ACZE Cars
			|| strcmp(game_TID, "AHD") == 0	// NTR-AHDE Jam Sessions
			|| strcmp(game_TID, "ANR") == 0	// NTR-ANRE Naruto - Saikyou Ninja Daikesshu 3
			|| strcmp(game_TID, "YT3") == 0	// NTR-YT3E Tamagotchi Connection - Corner Shop 3
			|| strcmp(game_TID, "AVI") == 0	// NTR-AVIJ Kodomo no Tame no Yomi Kikase - Ehon de Asobou 1-Kan
			|| strcmp(game_TID, "AV2") == 0	// NTR-AV2J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 2-Kan
			|| strcmp(game_TID, "AV3") == 0	// NTR-AV3J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 3-Kan
			|| strcmp(game_TID, "AV4") == 0	// NTR-AV4J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 4-Kan
			|| strcmp(game_TID, "AV5") == 0	// NTR-AV5J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 5-Kan
			|| strcmp(game_TID, "AV6") == 0	// NTR-AV6J Kodomo no Tame no Yomi Kikase - Ehon de Asobou 6-Kan
			|| strcmp(game_TID, "YNZ") == 0	// NTR-YNZE Petz - Dogz Fashion
			)
		{
			fifoSendValue32(FIFO_USER_04, 1);	// special setting (when found special gamecode)
		}

		fifoSendValue32(FIFO_USER_03, 1);
		fifoWaitValue32(FIFO_USER_05);
		for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }

		if(bootstrapini.GetInt("NDS-BOOTSTRAP","LOGGING",0) == 1) {			
			static FILE * debugFile;
			debugFile = fopen ("sd:/NDSBTSRP.LOG","w");
			fprintf(debugFile, "DEBUG MODE\n");			
			fclose (debugFile);
			
			// create a big file (minimal sdengine libfat cannot append to a file)
			debugFile = fopen ("sd:/NDSBTSRP.LOG","a");
			for (int i=0; i<1000; i++) {
				fprintf(debugFile, "                                                                                                                                          \n");			
			}
			fclose (debugFile);			
		} else {
			remove ("sd:/NDSBTSRP.LOG");
		}

		std::string	savPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "SAV_PATH", "");	
		
		bool useArm7Donor = bootstrapini.GetInt( "NDS-BOOTSTRAP", "USE_ARM7_DONOR", 1);	

		std::string	arm7DonorPath;	

		if (useArm7Donor)
			arm7DonorPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "ARM7_DONOR_PATH", "");	
		else
			arm7DonorPath = "";
		
		u32	patchMpuRegion = bootstrapini.GetInt( "NDS-BOOTSTRAP", "PATCH_MPU_REGION", 0);	
		
		u32	patchMpuSize = bootstrapini.GetInt( "NDS-BOOTSTRAP", "PATCH_MPU_SIZE", 0);	

		if(bootstrapini.GetInt("NDS-BOOTSTRAP","BOOST_CPU",0) == 1) {	
			dbg_printf("CPU boosted\n");
			// libnds sets TWL clock speeds on arm7/arm9 scfg_clk at boot now. No changes needed.
			if(bootstrapini.GetInt("NDS-BOOTSTRAP","BOOST_VRAM",0) == 1) {
				// This is nested in BOOT_CPU check as it won't make sense to enable this without TWL clock speeds being active.
				dbg_printf("VRAM boosted\n");
				REG_SCFG_EXT = 0x83002000;
			} else {
				// Do nothing for now. Default set at boot.
			}
		} else {
			REG_SCFG_CLK = 0x80;
			fifoSendValue32(FIFO_USER_07, 1);
		}

		/* Can't seem to do it here for some reason. It hangs if I do. I have lock scfg code occuring in the boost_cpu check instead.
		if(bootstrapini.GetInt("NDS-BOOTSTRAP","LOCK_ARM9_SCFG_EXT",0) == 1) {	
			dbg_printf("ARM9_SCFG_EXT locked\n");
			REG_SCFG_EXT &= 0x7FFFFFFF; // Only lock bit 31
			fifoSendValue32(FIFO_USER_08, 1);
		}
		*/

		// Options from INI file set. Now tell Arm7 to check to apply changes if any were requested.
		// fifoSendValue32(FIFO_USER_06, 1);	// ARM7 SCFG is locked on DSi
		
		initMBK();
	
		dbg_printf("Running %s\n", ndsPath.c_str());				
		runFile(ndsPath.c_str(), savPath.c_str(), arm7DonorPath.c_str(), patchMpuRegion, patchMpuSize);	
	} else {
		consoleDemoInit();
		printf("SD init failed!\n");
	}

	while(1) { swiWaitForVBlank(); }
}

