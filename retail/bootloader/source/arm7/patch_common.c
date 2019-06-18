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

//#include <stddef.h>
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"
#include "patch.h"
#include "common.h"
#include "loading_screen.h"
#include "debug_file.h"

u32 patchOffsetCacheFileVersion = 3;	// Change when new functions are being patched, some offsets removed
										// the offset order changed, and/or the function signatures changed

patchOffsetCacheContents patchOffsetCache;

bool patchOffsetCacheChanged = false;

extern bool logging;

void patchBinary(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);

	// The World Ends With You (USA/Europe)
	if (strcmp(romTid, "AWLE") == 0 || strcmp(romTid, "AWLP") == 0) {
		*(u32*)0x203E7B0 = 0;
	}

	// Subarashiki Kono Sekai - It's a Wonderful World (Japan)
	if (strcmp(romTid, "AWLJ") == 0) {
		*(u32*)0x203F114 = 0;
	}

	// Miami Nights - Singles in the City (USA)
	if (strcmp(romTid, "AVWE") == 0) {
		// Fix not enough memory error
		*(u32*)0x0204CCCC = 0xe1a00000; //nop
	}

	// Miami Nights - Singles in the City (Europe)
	if (strcmp(romTid, "AVWP") == 0) {
		// Fix not enough memory error
		*(u32*)0x0204CDBC = 0xe1a00000; //nop
	}
	
	// 0735 - Castlevania - Portrait of Ruin (USA)
	if (strcmp(romTid, "ACBE") == 0) {
		*(u32*)0x02007910 = 0xeb02508e;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025052;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0676 - Akumajou Dracula - Gallery of Labyrinth (Japan)
	if (strcmp(romTid, "ACBJ") == 0) {
		*(u32*)0x02007910 = 0xeb0250b0;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025074;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0881 - Castlevania - Portrait of Ruin (Europe) (En,Fr,De,Es,It)
	if (strcmp(romTid, "ACBP") == 0) {
		*(u32*)0x02007b00 = 0xeb025370;
		*(u32*)0x02007b08 = 0xea000004;
		*(u32*)0x02007bf0 = 0xeb025334;
		*(u32*)0x02007bf8 = 0xe59f1030;
		*(u32*)0x02007bfc = 0xe59f0028;
		*(u32*)0x02007c00 = 0xe0281097;
		*(u32*)0x02007c04 = 0xea000003;
	}

	// Chrono Trigger (Japan)
	if (strcmp(romTid, "YQUJ") == 0) {
		*(u32*)0x0204e364 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e368 = 0xe12fff1e; //bx lr
		*(u32*)0x0204e6c4 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e6c8 = 0xe12fff1e; //bx lr
	}

	// Chrono Trigger (USA/Europe)
	if (strcmp(romTid, "YQUE") == 0 || strcmp(romTid, "YQUP") == 0) {
		*(u32*)0x0204e334 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e338 = 0xe12fff1e; //bx lr
		*(u32*)0x0204e694 = 0xe3a00000; //mov r0, #0
		*(u32*)0x0204e698 = 0xe12fff1e; //bx lr
	}
	
	// Dementium II (USA/EUR)
	if (strcmp(romTid, "BDEE") == 0 || strcmp(romTid, "BDEP") == 0) {
		*(u32*)0x020e9120 = 0xe3a00002;
		*(u32*)0x020e9124 = 0xea000029;
	}
	
	// Dementium II: Tozasareta Byoutou (JPN)
	if (strcmp(romTid, "BDEJ") == 0) {
		*(u32*)0x020d9f60 = 0xe3a00005;
		*(u32*)0x020d9f68 = 0xea000029;
	}

	// Grand Theft Auto - Chinatown Wars (USA/Europe) (En,Fr,De,Es,It)
	if (strcmp(romTid, "YGXE") == 0 || strcmp(romTid, "YGXP") == 0) {
		*(u16*)0x02037a34 = 0x46c0;
		*(u32*)0x0216ac0c = 0x0001fffb;
		
        //test patches 4 dma
        // flag = common + 0x114
	}

	// WarioWare: DIY (USA)
	if (strcmp(romTid, "UORE") == 0) {
		*(u32*)0x02003114 = 0xE12FFF1E; //mov r0, #0
	}
    
    // Pokemon Dash
	if (strcmp(romTid, "APDJ") == 0) {
		//*(u32*)0x0206AE70 = 0xE3A00000; //mov r0, #0
        //*(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		//*(u32*)0x0206AE74 = 0xe12fff1e; //bx lr
        
        *(u32*)0x02000B94 = 0xE1A00000; //nop

		//*(u32*)0x020D5010 = 0xe12fff1e; //bx lr
	}

    // Pokemon Dash
	if (strcmp(romTid, "APDE") == 0 || strcmp(romTid, "APDP") == 0) {
        /*unsigned char pdash_patch_chars[64] =
        {
          0xFE, 0x40, 0x2D, 0xE9, 
          0x28, 0x10, 0xA0, 0xE3, 
          0x00, 0x20, 0xA0, 0xE3, 
          0x24, 0x30, 0x9F, 0xE5, 
          0x02, 0x40, 0x90, 0xE7, 
          0x02, 0x40, 0x83, 0xE7, 
          0x04, 0x20, 0x82, 0xE2, 
          0x01, 0x00, 0x52, 0xE1, 
          0xFA, 0xFF, 0xFF, 0x1A, 
          0x10, 0x30, 0x9F, 0xE5, 
          0x33, 0xFF, 0x2F, 0xE1, 
          0xFE, 0x80, 0xBD, 0xE8, 
          0x01, 0x00, 0xA0, 0xE3, 
          0x1E, 0xFF, 0x2F, 0xE1, 
          0x00, 0xA6, 0x0D, 0x02,              d
          0x78, 0x47, 0x0A, 0x02
        };

        //6D38C
        PatchMem(KArm9,s32(ii+1),0xe1a05000); //mov r5, r0
        PatchMem(KArm9,s32(ii+2),0xe1a00001); //mov r0, r1
        PatchMem(KArm9,s32(ii+3),0xe28fe004); //adr lr, [pc, #4]
        PatchMem(KArm9,s32(ii+4),0xe51ff004); //ldr pc, [pc, #-4]
        PatchMem(KArm9,s32(ii+5),(u32)iDmaFuncs.iFunc2);
        PatchMem(KArm9,s32(ii+6),0xe1a00005); //mov r0, r5
        PatchMem(KArm9,s32(ii+7),0xe28ff048); //adr pc, xxx  jump+48 (12*4)
        //6D3FC
        PatchMem(KArm9,s32(ii+28),0xe1a00000); //nop
        
        // r0 : ROMCTRL
        // r1 : ROMCTRL
        // r2 : ...
        // r3 : ...
        // r4 : DST
        // r5 : SRC
        // r6 : LEN
        // ..
        // r10 : cardstruct
        
        for(int i =0; i<64; i++) {
            *(((u8*)0x0206D2C4)+i) = pdash_patch_chars[i];    
        }*/
        
        //*((u32*)0x02000BB0) = 0xE1A00000; //nop 
    
		//*(u32*)0x0206D2C4 = 0xE3A00000; //mov r0, #0
        //*(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		//*(u32*)0x0206D2C8 = 0xe12fff1e; //bx lr
        
		//*(u32*)0x020D5010 = 0xe12fff1e; //bx lr
	}
    
    // Pokemon Dash
	if (strcmp(romTid, "APDK") == 0) {
        *(u32*)0x02000C14 = 0xE1A00000; //nop
	}
    
    
    // Golden Sun
    if (strcmp(romTid, "BO5E") == 0) {
        // patch "refresh" function
        *(u32*)0x204995C = 0xe12fff1e; //bx lr
        *(u32*)0x20499C4 = 0xe12fff1e; //bx lr
    }  
    
}

u32 patchCardNds(
	cardengineArm7* ce7,
	cardengineArm9* ce9,
	const tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 patchMpuRegion,
	u32 patchMpuSize,
	u32 ROMinRAM,
	u32 saveFileCluster,
	u32 saveSize
) {
	dbg_printf("patchCardNds\n\n");

	if (patchOffsetCache.ver != patchOffsetCacheFileVersion) {
		pleaseWaitOutput();
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.a9Swi12Offset = 0;
		patchOffsetCache.a9IsThumb = 0;
		patchOffsetCache.cardReadStartOffset = 0;
		patchOffsetCache.cardReadEndOffset = 0;
		patchOffsetCache.cardPullOutOffset = 0;
		patchOffsetCache.cardIdOffset = 0;
		patchOffsetCache.cardIdChecked = 0;
		patchOffsetCache.cardReadDmaOffset = 0;
		patchOffsetCache.cardEndReadDmaOffset = 0;
		patchOffsetCache.cardReadDmaChecked = 0;
		patchOffsetCache.sleepOffset = 0;
		patchOffsetCache.patchMpuRegion = 0;
		patchOffsetCache.mpuStartOffset = 0;
		patchOffsetCache.mpuDataOffset = 0;
		patchOffsetCache.randomPatchOffset = 0;
		patchOffsetCache.randomPatchChecked = 0;
		patchOffsetCache.randomPatch5Offset = 0;
		patchOffsetCache.randomPatch5Checked = 0;
		patchOffsetCache.randomPatch5SecondOffset = 0;
		patchOffsetCache.randomPatch5SecondChecked = 0;
		patchOffsetCache.a7IsThumb = 0;
		patchOffsetCache.a7Swi12Offset = 0;
		patchOffsetCache.swiGetPitchTableOffset = 0;
		patchOffsetCache.sleepPatchOffset = 0;
		patchOffsetCache.a7CardIrqEnableOffset = 0;
		patchOffsetCache.cardCheckPullOutOffset = 0;
		patchOffsetCache.cardCheckPullOutChecked = 0;
		patchOffsetCache.a7IrqHandlerOffset = 0;
		patchOffsetCache.savePatchType = 0;
		patchOffsetCache.relocateStartOffset = 0;
		patchOffsetCache.relocateValidateOffset = 0;
		patchOffsetCache.a7JumpTableFuncOffset = 0;
	}

	bool sdk5 = isSdk5(moduleParams);
	if (sdk5) {
		dbg_printf("[SDK 5]\n\n");
	}

	u32 errorCodeArm9 = patchCardNdsArm9(ce9, ndsHeader, moduleParams, patchMpuRegion, patchMpuSize);
	
	//if (cardReadFound || ndsHeader->fatSize == 0) {
	if (errorCodeArm9 == ERR_NONE || ndsHeader->fatSize == 0) {
		patchCardNdsArm7(ce7, ndsHeader, moduleParams, ROMinRAM, saveFileCluster);

		dbg_printf("ERR_NONE");
		return ERR_NONE;
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}
