/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdio.h>
#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
//#include <nds/interrupts.h>
//#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#include "tonccpy.h"
#include "my_fat.h"

extern cardengineArm9* volatile ce9;

extern u32 _io_dldi_features;

extern vu32* volatile cardStruct0;
//extern vu32* volatile cacheStruct;

extern u32* lastClusterCacheUsed;
extern u32 clusterCacheSize;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

static u32 accessCounter = 0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;

static aFile romFile;
static aFile tmpFile;

static bool flagsSet = false;

static inline int cardReadNormal(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	accessCounter++;

	/*nocashMessage("begin\n");

	dbg_hexa(dst);
	nocashMessage("\n");
	dbg_hexa(src);
	nocashMessage("\n");
	dbg_hexa(len);
	nocashMessage("\n");*/

	//nocashMessage("aaaaaaaaaa\n");
	fileRead((char*)dst, romFile, src, len, 0);

	//nocashMessage("end\n");

	if(strncmp(getRomTid(ndsHeader), "CLJ", 3) == 0){
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}
	
	return 0;
}

static inline int cardReadRAM(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	//u32 commandRead;
	while (len > 0) {
		// Copy directly
		tonccpy(dst, (u8*)((ce9->romLocation - 0x4000 - ndsHeader->arm9binarySize)+src),len);

		// Update cardi common
		cardStruct[0] = src + len;
		cardStruct[1] = (vu32)(dst + len);	
		cardStruct[2] = len - len;

		len = cardStruct[2];
		if (len > 0) {
			src = cardStruct[0];
			dst = (u8*)cardStruct[1];
			page = (src / 512) * 512;
		}
	}

	return 0;
}

//Currently used for NSMBDS romhacks
/*void __attribute__((target("arm"))) debug8mbMpuFix(){
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}*/

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");

	//sysSetCartOwner (BUS_OWNER_ARM9);

	if (!flagsSet) {
		if (_io_dldi_features & 0x00000010) {
			sysSetCartOwner (BUS_OWNER_ARM9);
		}

		if (!ce9->ROMinRAM) {
			if (!FAT_InitFiles(true, 0)) {
				//nocashMessage("!FAT_InitFiles");
				return -1;
			}

			lastClusterCacheUsed = (u32*)ce9->fatTableAddr;
			clusterCacheSize = 0x4000;

			romFile = getFileFromCluster(ce9->fileCluster);

			buildFatTableCache(&romFile, 0);

			/*const char* tmpName = "BTSTRP.TMP";
			tmpFile = getBootFileCluster(tmpName, 0);

			fileWrite((char*)0x2780000, tmpFile, 0, 0x40000, 0);*/
		}

		if (isSdk5(ce9->moduleParams)) {
			ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
		} /*else {
			debug8mbMpuFix();
		}*/

		flagsSet = true;
	}

	vu32* volatile cardStruct = (isSdk5(ce9->moduleParams) ? (vu32* volatile)((u8*)CARDENGINE_ARM9_LOCATION + 0x3FC0) : ce9->cardStruct0);

	u32 src = (isSdk5(ce9->moduleParams) ? src0 : cardStruct[0]);
	if (isSdk5(ce9->moduleParams)) {
		cardStruct[0] = src;
	}

	u8* dst = (isSdk5(ce9->moduleParams) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = (isSdk5(ce9->moduleParams) ? len0 : cardStruct[2]);

	if (isSdk5(ce9->moduleParams)) {
		cardStruct[1] = (vu32)dst;
		cardStruct[2] = len;
	}

	u32 page = (src / 512) * 512;

	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;

	#ifdef DEBUG
	u32 commandRead;

	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	//IPC_SendSync(0xEE24);

	waitForArm7();
	// -------------------------------------*/
	#endif

	// SDK 5 --> White screen
	/*if (!isSdk5(moduleParams) && *(vu32*)0x2800010 != 1) {
		if (readNum >= 0x100){ // Don't set too early or some games will crash
			*(vu32*)(*(vu32*)(0x2800000)) = *(vu32*)0x2800004;
			*(vu32*)(*(vu32*)(0x2800008)) = *(vu32*)0x280000C;
			alreadySetMpu = true;
		} else {
			readNum += 1;
		}
	}*/

	if (src == 0) {
		// If ROM read location is 0, do not proceed.
		return 0;
	}

	/*if(!ce9->ROMinRAM && (*(u32*)(0x2780000)) == 0){
		fileRead((char*)0x2780000, tmpFile, 0, 0x40000, 0);
	}*/

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}

	return ce9->ROMinRAM ? cardReadRAM(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage) : cardReadNormal(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage);
}
