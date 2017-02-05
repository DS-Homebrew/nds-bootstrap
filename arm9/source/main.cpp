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
	debugFile = fopen ("fat:/NDSBTSRPCARD.LOG","a");
	
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

void getSFCG_ARM9() {
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
}


void initMBK() {
	// default dsiware settings
	//REG_MBK_1=0x8185898D;
	//REG_MBK_2=0x8084888C;
	//REG_MBK_3=0x9094989C;
	//REG_MBK_4=0x8084888C;
	//REG_MBK_5=0x9094989C;
	
	// WRAM-B fully mapped to arm7
	REG_MBK_2=0x8D898581;
	REG_MBK_3=0x9D999591;
	
	// WRAM-C fully mapped to arm7
	REG_MBK_4=0x8D898581;
	REG_MBK_5=0x9D999591;
		
	// WRAM-A not mapped (reserved to arm7)
	REG_MBK_6=0x00000000;
	// WRAM-B mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK_7=0x07403700;
	// WRAM-C mapped to the 0x3740000 - 0x377FFFF area : 256k
	REG_MBK_8=0x07803740;
}

int main( int argc, char **argv) {

	initMBK();

	bool ntrMode = false;

	// No! broke no$gba compatibility
	//REG_SCFG_CLK = 0x85;

	if (argc >= 2) {
		if ( strcasecmp (argv[1], "NTR") == 0 ) {
			ntrMode = true;
		}		
	}
	
	if(ntrMode) {
		// REG_SCFG_CLK = 0x80;
		REG_SCFG_EXT = 0x83000000; // NAND/SD Access
		fifoSendValue32(FIFO_USER_06, 1);
	}
	
	if (fatInitDefault()) {
		CIniFile bootstrapini( "fat:/_nds/nds-bootstrap.ini" );

		if(bootstrapini.GetInt("NDS-BOOTSTRAP","DEBUG",0) == 1) {	
			debug=true;			
			
			consoleDemoInit();
			
			fifoSetValue32Handler(FIFO_USER_02,myFIFOValue32Handler,0);
			
			getSFCG_ARM9();
			getSFCG_ARM7();
			
			
		} else {
		
		}

		// consoleDemoInit();
		
		if(bootstrapini.GetInt("NDS-BOOTSTRAP","RESETSLOT1",0) == 1) {
			if(REG_SCFG_MC == 0x11) { 
				printf("Please insert a cartridge...\n");
				do { swiWaitForVBlank(); } 
				while (REG_SCFG_MC == 0x11);
			}
			fifoSendValue32(FIFO_USER_04, 1);
		}

		fifoSendValue32(FIFO_USER_03, 1);
		fifoWaitValue32(FIFO_USER_05);
		for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }
		
		if (0 != argc ) {
			dbg_printf("arguments passed\n");
			int i;
			for (i=0; i<argc; i++ ) {
				if (argv[i]) dbg_printf("[%d] %s\n", i, argv[i]);
			}
			dbg_printf("\n");
		} else {
			dbg_printf("No arguments passed!\n");
		}
		
		if(ntrMode) {
			dbg_printf("NTR mode enabled\n");
		}
		
		if(bootstrapini.GetInt("NDS-BOOTSTRAP","LOGGING",1) == 1) {			
			static FILE * debugFile;
			debugFile = fopen ("fat:/NDSBTSRP.LOG","w");
			fprintf(debugFile, "DEBUG MODE\n");			
			fclose (debugFile);
			
			// create a big file (minimal sdengine libfat cannot append to a file)
			debugFile = fopen ("fat:/NDSBTSRP.LOG","a");
			for (int i=0; i<1000; i++) {
				fprintf(debugFile, "                                                                                                                                          \n");			
			}
			fclose (debugFile);
			
		} else {
			remove ("fat:/NDSBTSRP.LOG");
		}

		std::string	ndsPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "NDS_PATH", "");	
		
		std::string	savPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "SAV_PATH", "");	
		
		std::string	arm7DonorPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "ARM7_DONOR_PATH", "");	
		
		u32	patchMpuRegion = bootstrapini.GetInt( "NDS-BOOTSTRAP", "PATCH_MPU_REGION", 0);	
		
		u32	patchMpuSize = bootstrapini.GetInt( "NDS-BOOTSTRAP", "PATCH_MPU_SIZE", 0);	

		if(bootstrapini.GetInt("NDS-BOOTSTRAP","BOOST_CPU",0) == 1) {	
			dbg_printf("CPU boosted\n");
			REG_SCFG_CLK |= 0x1;
		} else {
			REG_SCFG_CLK = 0x80;
			fifoSendValue32(FIFO_USER_07, 1);
		}
		
		if(bootstrapini.GetInt("NDS-BOOTSTRAP","LOCK_ARM9_SCFG_EXT",0) == 1) {	
			dbg_printf("ARM9_SCFG_EXT locked\n");
			REG_SCFG_EXT &= 0x7FFFFFFF; // Only lock bit 31
		}
		
		if(bootstrapini.GetInt("NDS-BOOTSTRAP","NTR_MODE_SWITCH",0) == 1) {		
			std::string	bootstrapPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "BOOTSTRAP_PATH", "");
			
			// run an argv file
			
			dbg_printf("NTR MODE SWITCH\n");		
			
			if(!ntrMode) {
				dbg_printf("RERUN BOOTSTRAP in NTR mode via argv\n");				
				dbg_printf("Running %s\n", bootstrapPath.c_str());
				
				runFile(bootstrapPath.c_str(), savPath, arm7DonorPath.c_str(), patchMpuRegion, patchMpuSize);
			} else {				
				dbg_printf("Running %s\n", ndsPath.c_str());
				
				runFile(ndsPath.c_str(), savPath.c_str(), arm7DonorPath.c_str(), patchMpuRegion, patchMpuSize);
			}
		} else {
			dbg_printf("TWL MODE enabled\n");			
			dbg_printf("Running %s\n", ndsPath.c_str());
					
			runFile(ndsPath.c_str(), savPath.c_str(), arm7DonorPath.c_str(), patchMpuRegion, patchMpuSize);
		}		
	} else {
		consoleDemoInit();
		printf("SD init failed!\n");
	}

	while(1) { swiWaitForVBlank(); }
}

