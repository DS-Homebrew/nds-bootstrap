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
#include "tonccpy.h"
#include "loading_screen.h"
#include "debug_file.h"

extern u8 valueBits3;
#define memoryPit (valueBits3 & BIT(1))

u16 patchOffsetCacheFileVersion = 83;	// Change when new functions are being patched, some offsets removed,
										// the offset order changed, and/or the function signatures changed (not added)

patchOffsetCacheContents patchOffsetCache;

u16 patchOffsetCacheFilePrevCrc = 0;
u16 patchOffsetCacheFileNewCrc = 0;

extern bool logging;
extern bool gbaRomFound;
extern u8 dsiSD;

void dsiWarePatch(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
	extern u8 consoleModel;
	const char* romTid = getRomTid(ndsHeader);
	const char* dataPub = "dataPub:";
	//const char* chnFontPath = "sdmc:/sys/CHNFontTable.dat";

	/*if (ndsHeader->arm7binarySize == 0xF548) {
		tonccpy((char*)0x02E929BC, chnFontPath, strlen(chnFontPath));
	}*/

	if (ndsHeader->arm7binarySize == 0x44C) {
		if (*(u32*)0x023803BC >= 0x02F00000 && *(u32*)0x023803BC < 0x02F80000) {
			*(u32*)0x023803BC -= 0x80000;
		}
		if (*(u32*)0x023803C0 >= 0x02F00000 && *(u32*)0x023803C0 < 0x02F80000) {
			*(u32*)0x023803C0 -= 0x80000;
		}
		if (*(u32*)0x023803C4 >= 0x02F00000 && *(u32*)0x023803C4 < 0x02F80000) {
			*(u32*)0x023803C4 -= 0x80000;
		}
	}

	// Absolute BrickBuster (USA)
	if (strcmp(romTid, "K6QE") == 0) {
		if (dsiSD) { // Redirect otherPub to dataPub
			toncset((char*)0x02095CD4, 0, 9);
			tonccpy((char*)0x02095CD4, dataPub, strlen(dataPub));
			toncset((char*)0x02095CE8, 0, 9);
			tonccpy((char*)0x02095CE8, dataPub, strlen(dataPub));
		} else {
			*(u32*)0x020053E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02055B74 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02055B78 = 0xE12FFF1E; // bx lr
			*(u32*)0x02055C48 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02055C4C = 0xE12FFF1E; // bx lr
		}
	}

	// Absolute Chess (USA)
	else if (strcmp(romTid, "KCZE") == 0 && dsiSD) {
		// Redirect otherPub to dataPub
		toncset((char*)0x0209E9C8, 0, 9);
		tonccpy((char*)0x0209E9C8, dataPub, strlen(dataPub));
		toncset((char*)0x0209E9DC, 0, 9);
		tonccpy((char*)0x0209E9DC, dataPub, strlen(dataPub));
	}

	// Absolute Reversi (USA)
	else if (strcmp(romTid, "KA8E") == 0 && dsiSD) {
		// Redirect otherPub to dataPub
		toncset((char*)0x0209D220, 0, 9);
		tonccpy((char*)0x0209D220, dataPub, strlen(dataPub));
		toncset((char*)0x0209D234, 0, 9);
		tonccpy((char*)0x0209D234, dataPub, strlen(dataPub));
	}

	// DS WiFi Settings
	else if (strcmp(romTid, "B88A") == 0) {
		tonccpy((void*)0x023C0000, ce9->thumbPatches->reset_arm9, 0x18);

		const u16* branchCode = generateA7InstrThumb(0x020051F4, 0x023C0000);

		*(u16*)0x020051F4 = branchCode[0];
		*(u16*)0x020051F6 = branchCode[1];
	}

	// Hidden Photo (Europe)
	else if (strcmp(romTid, "DD3P") == 0 && consoleModel == 0) {
		*(u32*)0x0201BC14 -= 0xD000; // Shift heap
	}

	// Wimmelbild Creator (German)
	else if (strcmp(romTid, "DD3D") == 0 && consoleModel == 0) {
		*(u32*)0x0201B9E4 -= 0xD000; // Shift heap
	}

	// Nintendo DSi Camera
	else if (strncmp(romTid, "HNI", 3) == 0 && memoryPit) {
		extern u32 iUncompressedSize;
		// Find and replace pit.bin text string with tip.bin to avoid conflicting with Memory Pit
		char* addr = (char*)ndsHeader->arm9destination+0xC0000;
		for (u32 i = 0; i < iUncompressedSize; i++) {
			if (memcmp(addr+i, "pit.bin", 8) == 0)
			{
				const char* textReplace = "tip.bin";
				tonccpy(addr+i, textReplace, sizeof(textReplace));
				break;
			}
		}
	}

	else if (dsiSD) {
		return;
	}

	// Stub out save functions (and some others)

	// GO Series: 10 Second Run (USA)
	// GO Series: 10 Second Run (Europe)
	else if (strcmp(romTid, "KJUE") == 0 || strcmp(romTid, "KJUP") == 0) {
		*(u32*)0x020150FC = 0xE12FFF1E; // bx lr
		*(u32*)0x020193E0 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
		*(u32*)0x02019D20 = 0xE12FFF1E; // bx lr
	}

	// Absolute Baseball (USA)
	else if (strcmp(romTid, "KE9E") == 0) {
		*(u32*)0x0205FAD0 = 0xE1A00000; // nop
		*(u32*)0x02072554 = 0xE3A00001; // mov r0, #1
	}

	// Ah! Heaven (USA)
	// Ah! Heaven (Europe)
	else if (strcmp(romTid, "K5HE") == 0 || strcmp(romTid, "K5HP") == 0) {
		*(u32*)0x0201FD04 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02029C68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02029D14 = 0xE12FFF1E; // bx lr
	}

	// Anonymous Notes 1: From The Abyss (USA)
	else if (strcmp(romTid, "KVIE") == 0) {
		*(u32*)0x02023DB0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02023DB4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020CE830 = 0xE12FFF1E; // bx lr
	}

	// Everyday Soccer (USA)
	// ARC Style: Everyday Football (Europe, Australia)
	// ARC Style: Soccer! (Japan)
	// ARC Style: Soccer! (Korea)
	else if (strncmp(romTid, "KAZ", 3) == 0) {
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Army Defender (USA)
	// Army Defender (Europe)
	else if (strncmp(romTid, "KAY", 3) == 0) {
		*(u32*)0x020051BC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020051C0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005204 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005208 = 0xE12FFF1E; // bx lr
	}

	// Asphalt 4: Elite Racing (USA)
	else if (strcmp(romTid, "KA4E") == 0) {
		*(u32*)0x0204FA6C = 0xE12FFF1E; // bx lr
	}

	// Asphalt 4: Elite Racing (Europe, Australia)
	else if (strcmp(romTid, "KA4V") == 0) {
		*(u32*)0x0204FAE0 = 0xE12FFF1E; // bx lr
	}

	// Aura-Aura Climber (USA)
	else if (strcmp(romTid, "KSRE") == 0) {
		*(u32*)0x02026760 = 0xE12FFF1E; // bx lr
	}

	// Aura-Aura Climber (Europe, Australia)
	else if (strcmp(romTid, "KSRV") == 0) {
		*(u32*)0x020265A8 = 0xE12FFF1E; // bx lr
	}

	// Bomberman Blitz (USA)
	else if (strcmp(romTid, "KBBE") == 0) {
		*(u32*)0x020437AC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020437B0 = 0xE12FFF1E; // bx lr
	}

	// Bomberman Blitz (Europe, Australia)
	else if (strcmp(romTid, "KBBV") == 0) {
		*(u32*)0x02043878 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0204387C = 0xE12FFF1E; // bx lr
	}

	// Itsudemo Bomberman (Japan)
	else if (strcmp(romTid, "KBBJ") == 0) {
		*(u32*)0x020434D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020434DC = 0xE12FFF1E; // bx lr
	}

	// Brain Challenge (USA)
	else if (strcmp(romTid, "KBCE") == 0) {
		*(u32*)0x0200EBD8 = 0xE12FFF1E; // bx lr
	}

	// Brain Challenge (Europe, Australia)
	else if (strcmp(romTid, "KBCV") == 0) {
		*(u32*)0x0200EBF4 = 0xE12FFF1E; // bx lr
	}

	// Cave Story (USA)
	else if (strcmp(romTid, "KCVE") == 0) {
		*(u32*)0x02005980 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005A68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005B60 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200A12C = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Chuck E. Cheese's Alien Defense Force (USA)
	else if (strcmp(romTid, "KUQE") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 4; i++) {
			u32* offset = (u32*)0x0202D43C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Chuck E. Cheese's Arcade Room (USA)
	else if (strcmp(romTid, "KUCE") == 0) {
		*(u32*)0x02032550 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Crash-Course Domo (USA)
	else if (strcmp(romTid, "KDCE") == 0) {
		*(u16*)0x0200DF38 = 0x2001; // movs r0, #1
		*(u16*)0x0200DF3A = 0x4770; // bx lr
		*(u16*)0x020153C4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Crazy Chicken: Director's Cut (Europe)
	else if (strcmp(romTid, "KQZP") == 0) {
		*(u32*)0x0207DAC0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207DD1C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207DF6C = 0xE12FFF1E; // bx lr
	}

	// Crazy Chicken: Pirates (Europe)
	else if (strcmp(romTid, "KCVP") == 0) {
		*(u32*)0x020771D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207742C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207767C = 0xE12FFF1E; // bx lr
	}

	// CuteWitch! runner (USA)
	// CuteWitch! runner (Europe)
	else if (strncmp(romTid, "K32", 3) == 0) {
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02062068 = 0xE12FFF1E; // bx lr
		} else if (ndsHeader->gameCode[3] == 'P') {
			*(u32*)0x02093AA4 = 0xE12FFF1E; // bx lr
		}
	}

	// GO Series: Defense Wars (USA)
	// GO Series: Defence Wars (Europe)
	else if (strcmp(romTid, "KWTE") == 0 || strcmp(romTid, "KWTP") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200CC98;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (USA)
	else if (strcmp(romTid, "KHEE") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02022600;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (Europe)
	else if (strcmp(romTid, "KHEP") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020226DC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (Japan)
	else if (strcmp(romTid, "KHEJ") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0202248C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// GO Series: Earth Saver (USA)
	else if (strcmp(romTid, "KB8E") == 0) {
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x02005534 = 0xE1A00000; // nop
		*(u32*)0x0200A3D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200A898 = 0xE12FFF1E; // bx lr
		*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014BEC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Face Pilot: Fly With Your Nintendo DSi Camera! (USA)
	else if (strcmp(romTid, "KYBE") == 0) {
		*(u32*)0x0200BB54 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0203C928 = 0xE12FFF1E; // bx lr
	}

	// Face Pilot: Fly With Your Nintendo DSi Camera! (Europe, Australia)
	else if (strcmp(romTid, "KYBV") == 0) {
		*(u32*)0x0200BB44 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0203C9E4 = 0xE12FFF1E; // bx lr
	}

	// Ferrari GT: Evolution (USA)
	else if (strcmp(romTid, "KFRE") == 0) {
		*(u32*)0x0205FDA8 = 0xE12FFF1E; // bx lr
	}

	// Ferrari GT: Evolution (Europe, Australia)
	else if (strcmp(romTid, "KFRV") == 0) {
		*(u32*)0x0205FC88 = 0xE12FFF1E; // bx lr
	}

	// Flashlight (USA)
	else if (strcmp(romTid, "KFSE") == 0) {
		*(u32*)0x02005134 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Frogger Returns (USA)
	else if (strcmp(romTid, "KFGE") == 0) {
		// Skip Manual screen
		*(u32*)0x0204B968 = 0xE1A00000; // nop
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0204B98C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Hard-Hat Domo (USA)
	else if (strcmp(romTid, "KDHE") == 0) {
		*(u16*)0x0200D060 = 0x2001; // movs r0, #1
		*(u16*)0x0200D062 = 0x4770; // bx lr
		*(u16*)0x020140AC = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Heathcliff: Spot On (USA)
	else if (strcmp(romTid, "K6SE") == 0) {
		*(u32*)0x02005248 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005280 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020052C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205B604 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205B608 = 0xE12FFF1E; // bx lr
	}

	// Invasion of the Alien Blobs (USA)
	else if (strcmp(romTid, "KBTE") == 0) {
		*(u32*)0x020224EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020224F0 = 0xE12FFF1E; // bx lr
	}

	// JellyCar 2 (USA)
	else if (strcmp(romTid, "KJYE") == 0) {
		*(u32*)0x02019F94 = 0xE1A00000; // nop (Skip Manual screen, Part 1)
		for (int i = 0; i < 11; i++) { // Skip Manual screen, Part 2
			u32* offset = (u32*)0x0201A028;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Jump Trials (USA)
	else if (strcmp(romTid, "KJPE") == 0) {
		*(u32*)0x0201E88C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E890 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EA40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EA44 = 0xE12FFF1E; // bx lr
	}

	// Jump Trials Extreme (USA)
	else if (strcmp(romTid, "KZCE") == 0) {
		*(u32*)0x020215F0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020215F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020217A4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020217A8 = 0xE12FFF1E; // bx lr
	}

	// Kung Fu Dragon (USA)
	// Kung Fu Dragon (Europe)
	else if (strcmp(romTid, "KT9E") == 0 || strcmp(romTid, "KT9P") == 0) {
		*(u32*)0x02005310 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Akushon Gemu: Tobeyo!! Dorago! (Japan)
	else if (strcmp(romTid, "KT9J") == 0) {
		*(u32*)0x020052F0 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Libera Wing (Europe)
	else if (strcmp(romTid, "KLWP") == 0) {
		*(u32*)0x02044668 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204585C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02045860 = 0xE12FFF1E; // bx lr
	}

	// Little Twin Stars (Japan)
	else if (strcmp(romTid, "KQ3J") == 0) {
		*(u32*)0x020162C4 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Lola's Alphabet Train (USA)
	// Lola's Alphabet Train (Europe)
	else if (strcmp(romTid, "KLKE") == 0 || strcmp(romTid, "KLKP") == 0) {
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Magnetic Joe (USA)
	else if (strcmp(romTid, "KJOE") == 0) {
		*(u32*)0x02036A30 = 0xE1A00000; // nop (Skip
		*(u32*)0x02036A34 = 0xE1A00000; // nop  Manual screen)
		*(u32*)0x0204E5AC = 0xE3A00002; // mov r0, #2
		*(u32*)0x0205DAAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205DAB0 = 0xE12FFF1E; // bx lr
	}

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0) {
		*(u32*)0x0200B0A0 = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Europe, Australia)
	else if (strcmp(romTid, "KMGV") == 0) {
		*(u32*)0x0200B3A8 = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Japan)
	else if (strcmp(romTid, "KMGJ") == 0) {
		*(u32*)0x0200B184 = 0xE1A00000; // nop
	}

	// Mighty Milky Way (USA)
	// Mighty Milky Way (Europe)
	// Mighty Milky Way (Japan)
	else if (strncmp(romTid, "KWY", 3) == 0) {
		*(u32*)0x020054E4 = 0xE1A00000; // nop
	}

	// Mixed Messages (USA)
	// Mixed Messages (Europe, Australia)
	else if (strcmp(romTid, "KMME") == 0 || strcmp(romTid, "KMMV") == 0) {
		*(u32*)0x02031A40 = 0xE3A00008; // mov r0, #8
		*(u32*)0x02031A44 = 0xE12FFF1E; // bx lr
		*(u32*)0x02033B00 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Monster Buster Club (USA)
	else if (strcmp(romTid, "KXBE") == 0) {
		*(u32*)0x0207F058 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F05C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F138 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F13C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F4FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F63C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F640 = 0xE12FFF1E; // bx lr
	}

	// Monster Buster Club (Europe)
	else if (strcmp(romTid, "KXBP") == 0) {
		*(u32*)0x0207EF64 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207EF68 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F048 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F414 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F418 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F558 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F55C = 0xE12FFF1E; // bx lr
	}

	// Nintendogs (China)
	/*else if (strcmp(romTid, "KDOC") == 0) {
		*(u32*)0x020AA90C = 0xE12FFF1E; // bx lr
	}*/

	// Number Battle
	else if (strcmp(romTid, "KSUE") == 0) {
		*(u32*)0x02005EA4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02005EA8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02005FA0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02005FA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02006130 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02006134 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200619C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020061A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02006384 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020063F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200657C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020065CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02006B68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02021C20 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Plants vs. Zombies (USA)
	else if (strcmp(romTid, "KZLE") == 0) {
		*(u32*)0x020C2F94 = 0xE12FFF1E; // bx lr
	}

	// Plants vs. Zombies (Europe, Australia)
	else if (strcmp(romTid, "KZLV") == 0) {
		*(u32*)0x020C41F8 = 0xE12FFF1E; // bx lr
	}

	// GO Series: Portable Shrine Wars (USA)
	// GO Series: Portable Shrine Wars (Europe)
	else if (strcmp(romTid, "KOQE") == 0 || strcmp(romTid, "KOQP") == 0) {
		*(u32*)0x0200E004 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200DE78;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Pro-Putt Domo (USA)
	else if (strcmp(romTid, "KDPE") == 0) {
		*(u16*)0x020106BC = 0x2001; // movs r0, #1
		*(u16*)0x020106BE = 0x4770; // bx lr
		*(u16*)0x020179F4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Puzzle League: Express (USA)
	else if (strcmp(romTid, "KPNE") == 0) {
		*(u32*)0x0205663C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02056640 = 0xE12FFF1E; // bx lr
		*(u32*)0x02056A28 = 0xE12FFF1E; // bx lr
	}

	// A Little Bit of... Puzzle League (Europe, Australia)
	else if (strcmp(romTid, "KPNV") == 0) {
		*(u32*)0x020575FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02057600 = 0xE12FFF1E; // bx lr
		*(u32*)0x020579E8 = 0xE12FFF1E; // bx lr
	}

	// Chotto Panel de Pon (Japan)
	else if (strcmp(romTid, "KPNJ") == 0) {
		*(u32*)0x02056128 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205612C = 0xE12FFF1E; // bx lr
		*(u32*)0x02056514 = 0xE12FFF1E; // bx lr
	}

	// Quick Fill Q (USA)
	// Quick Fill Q (Europe)
	else if (strcmp(romTid, "KUME") == 0 || strcmp(romTid, "KUMP") == 0) {
		*(u32*)0x02040240 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rabi Laby (USA)
	// Rabi Laby (Europe)
	else if (strcmp(romTid, "KLBE") == 0 || strcmp(romTid, "KLBP") == 0) {
		*(u32*)0x020053A8 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Akushon Pazuru: Rabi x Rabi (Japan)
	else if (strcmp(romTid, "KLBJ") == 0) {
		*(u32*)0x02005360 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rabi Laby 2 (USA)
	// Rabi Laby 2 (Europe)
	// Akushon Pazuru: Rabi x Rabi Episodo 2 (Japan)
	else if (strncmp(romTid, "KLV", 3) == 0) {
		*(u32*)0x0200540C = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Robot Rescue (USA)
	else if (strcmp(romTid, "KRTE") == 0) {
		*(u32*)0x0200C2DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2E0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C39C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C570 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C574 = 0xE12FFF1E; // bx lr
		*(u32*)0x020108A4 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Robot Rescue (Europe, Australia)
	else if (strcmp(romTid, "KRTV") == 0) {
		*(u32*)0x0200C2CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C388 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C38C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C550 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C554 = 0xE12FFF1E; // bx lr
		*(u32*)0x02010C30 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// ARC Style: Robot Rescue (Japan)
	else if (strcmp(romTid, "KRTJ") == 0) {
		*(u32*)0x0200F460 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F464 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F51C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F520 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F6E4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F6E8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02013BC8 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rock-n-Roll Domo (USA)
	else if (strcmp(romTid, "KD6E") == 0) {
		*(u16*)0x02010164 = 0x2001; // movs r0, #1
		*(u16*)0x02010166 = 0x4770; // bx lr
		*(u16*)0x02016514 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Space Invaders Extreme Z (Japan)
	else if (strcmp(romTid, "KEVJ") == 0) {
		*(u32*)0x020E3E4C = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E3E50 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E43A4 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E43A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E4624 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E4628 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E4854 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E4858 = 0xE12FFF1E; // bx lr
	}

	// Spotto! (USA)
	else if (strcmp(romTid, "KSPE") == 0) {
		*(u32*)0x0202D6B8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202D6BC = 0xE12FFF1E; // bx lr
	}

	// Bird & Bombs (Europe, Australia)
	else if (strcmp(romTid, "KSPV") == 0) {
		*(u32*)0x0202D6A8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202D6AC = 0xE12FFF1E; // bx lr
	}

	// Neratte Supotto! (Japan)
	else if (strcmp(romTid, "KSPJ") == 0) {
		*(u32*)0x0202DB94 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202DB98 = 0xE12FFF1E; // bx lr
	}

	// Sudoku (USA)
	// Sudoku (USA) (Rev 1)
	else if (strcmp(romTid, "K4DE") == 0) {
		if (ndsHeader->romversion == 1) {
			*(u32*)0x0203701C = 0xE3A00001; // mov r0, #1
			*(u32*)0x02037020 = 0xE12FFF1E; // bx lr
		} else {
			*(u32*)0x0203609C = 0xE3A00001; // mov r0, #1
			*(u32*)0x020360A0 = 0xE12FFF1E; // bx lr
		}
	}

	// Sudoku (Europe, Australia) (Rev 1)
	else if (strcmp(romTid, "K4DV") == 0) {
		*(u32*)0x020360E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020360EC = 0xE12FFF1E; // bx lr
	}

	// Sudoku 4Pockets (USA)
	// Sudoku 4Pockets (Europe)
	else if (strcmp(romTid, "K4FE") == 0 || strcmp(romTid, "K4FP") == 0) {
		*(u32*)0x02004C4C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202E888 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202E88C = 0xE12FFF1E; // bx lr
	}

	// Tetris Party Live (USA)
	else if (strcmp(romTid, "KTEE") == 0) {
		*(u32*)0x0205A83C = 0xE12FFF1E; // bx lr
		*(u32*)0x0205A92C = 0xE12FFF1E; // bx lr
	}

	// Tetris Party Live (Europe, Australia)
	else if (strcmp(romTid, "KTEV") == 0) {
		*(u32*)0x0205A828 = 0xE12FFF1E; // bx lr
		*(u32*)0x0205A918 = 0xE12FFF1E; // bx lr
	}

	// True Swing Golf Express (USA)
	// A Little Bit of... Nintendo Touch Golf (Europe, Australia)
	if (strcmp(romTid, "K72E") == 0 || strcmp(romTid, "K72V") == 0) {
		*(u32*)0x02009A84 = 0xE12FFF1E; // bx lr
	}

	// Wakugumi: Monochrome Puzzle (Europe, Australia)
	else if (strcmp(romTid, "KK4V") == 0) {
		*(u32*)0x0204F240 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02050114 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// White-Water Domo (USA)
	else if (strcmp(romTid, "KDWE") == 0) {
		*(u16*)0x0200C918 = 0x2001; // movs r0, #1
		*(u16*)0x0200C91A = 0x4770; // bx lr
		*(u16*)0x02013B10 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}
}

void patchBinary(cardengineArm9* ce9, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	if (ndsHeader->unitCode == 3) {
		dsiWarePatch(ce9, ndsHeader);
		return;
	}

	const char* romTid = getRomTid(ndsHeader);

	// Trauma Center: Under the Knife (USA)
	if (strcmp(romTid, "AKDE") == 0) {
		*(u32*)0x2007434 = 0;
	}

	// Trauma Center: Under the Knife (Europe)
	else if (strcmp(romTid, "AKDP") == 0) {
		*(u32*)0x20A6B90 = 0;
	}

	// Chou Shittou Caduceus (Japan)
	else if (strcmp(romTid, "AKDJ") == 0 && ndsHeader->romversion == 1) {
		*(u32*)0x20CCB18 = 0;
	}

	// The World Ends With You (USA/Europe)
	else if (strcmp(romTid, "AWLE") == 0 || strcmp(romTid, "AWLP") == 0) {
		*(u32*)0x203E7B0 = 0;
	}

	// Subarashiki Kono Sekai - It's a Wonderful World (Japan)
	else if (strcmp(romTid, "AWLJ") == 0) {
		*(u32*)0x203F114 = 0;
	}

	// Miami Nights - Singles in the City (USA)
	else if (strcmp(romTid, "AVWE") == 0) {
		// Fix not enough memory error
		*(u32*)0x0204CCCC = 0xe1a00000; //nop
	}

	// Miami Nights - Singles in the City (Europe)
	else if (strcmp(romTid, "AVWP") == 0) {
		// Fix not enough memory error
		*(u32*)0x0204CDBC = 0xe1a00000; //nop
	}
	
	// 0735 - Castlevania - Portrait of Ruin (USA)
	else if (strcmp(romTid, "ACBE") == 0) {
		*(u32*)0x02007910 = 0xeb02508e;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025052;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0676 - Akumajou Dracula - Gallery of Labyrinth (Japan)
	else if (strcmp(romTid, "ACBJ") == 0) {
		*(u32*)0x02007910 = 0xeb0250b0;
		*(u32*)0x02007918 = 0xea000004;
		*(u32*)0x02007a00 = 0xeb025074;
		*(u32*)0x02007a08 = 0xe59f1030;
		*(u32*)0x02007a0c = 0xe59f0028;
		*(u32*)0x02007a10 = 0xe0281097;
		*(u32*)0x02007a14 = 0xea000003;
	}
	
	// 0881 - Castlevania - Portrait of Ruin (Europe) (En,Fr,De,Es,It)
	else if (strcmp(romTid, "ACBP") == 0) {
		*(u32*)0x02007b00 = 0xeb025370;
		*(u32*)0x02007b08 = 0xea000004;
		*(u32*)0x02007bf0 = 0xeb025334;
		*(u32*)0x02007bf8 = 0xe59f1030;
		*(u32*)0x02007bfc = 0xe59f0028;
		*(u32*)0x02007c00 = 0xe0281097;
		*(u32*)0x02007c04 = 0xea000003;
	}

	// Catan (Europe) (En,De)
	else if (strcmp(romTid, "CN7P") == 0) {
		*(u32*)0x02000bc0 = 0xe3540000;
		*(u32*)0x02000bc4 = 0x12441001;
		*(u32*)0x02000bc8 = 0x112fff1e;
		*(u32*)0x02000bcc = 0xe28dd070;
		*(u32*)0x02000bd0 = 0xe3a00000;
		*(u32*)0x02000bd4 = 0xe8bd8ff8;
		*(u32*)0x0207a9f0 = 0xebfe1872;
	}

	// De Kolonisten van Catan (Netherlands)
	else if (strcmp(romTid, "CN7H") == 0) {
		*(u32*)0x02000bc0 = 0xe3540000;
		*(u32*)0x02000bc4 = 0x12441001;
		*(u32*)0x02000bc8 = 0x112fff1e;
		*(u32*)0x02000bcc = 0xe28dd070;
		*(u32*)0x02000bd0 = 0xe3a00000;
		*(u32*)0x02000bd4 = 0xe8bd8ff8;
		*(u32*)0x0207af40 = 0xebfe271e;
	}
	
	// Power Rangers - Samurai (USA) (En,Fr,Es)
	else if (strcmp(romTid, "B3NE") == 0) {
		*(u32*)0x02060608 = 0xe3a00001; //mov r0, #1
	}

	// Power Rangers - Samurai (Europe) (En,Fr,De,Es,It)
	else if (strcmp(romTid, "B3NP") == 0) {
		*(u32*)0x02060724 = 0xe3a00001; //mov r0, #1
	}

	// Learn with Pokemon - Typing Adventure (Europe)
	else if (strcmp(romTid, "UZPP") == 0) {
		*(u32*)0x02000560 = 0xe92d401f;
		*(u32*)0x02000564 = 0xe28f0024;
		*(u32*)0x02000568 = 0xe5901000;
		*(u32*)0x0200056c = 0xe3510001;
		*(u32*)0x02000570 = 0x08bd801f;
		*(u32*)0x02000574 = 0xe5912000;
		*(u32*)0x02000578 = 0xe5903004;
		*(u32*)0x0200057c = 0xe1520003;
		*(u32*)0x02000580 = 0x05904008;
		*(u32*)0x02000584 = 0x05814000;
		*(u32*)0x02000588 = 0xe280000c;
		*(u32*)0x0200058c = 0xeafffff5;
		*(u32*)0x02000590 = 0x020f7c48;
		*(u32*)0x02000594 = 0x0000af81;
		*(u32*)0x02000598 = 0x0000a883;
		*(u32*)0x0200059c = 0x020f83f4;
		*(u32*)0x020005a0 = 0x0000b975;
		*(u32*)0x020005a4 = 0x0000c127;
		*(u32*)0x020005a8 = 0x02105498; 
		*(u32*)0x020005ac = 0x02105179;
		*(u32*)0x020005b0 = 0x0200162d; 
		*(u32*)0x020005b4 = 0x0210c030;
		*(u32*)0x020005b8 = 0x0210bd11;
		*(u32*)0x020005bc = 0x0200162d;
		*(u32*)0x020005c0 = 0x021022b4;
		*(u32*)0x020005c4 = 0x02101f95;
		*(u32*)0x020005c8 = 0x0200162d;
		*(u32*)0x020005cc = 0x021022d0;
		*(u32*)0x020005d0 = 0x02101ff9;
		*(u32*)0x020005d4 = 0x0200162d;
		*(u32*)0x020005d8 = 0x0210c058;
		*(u32*)0x020005dc = 0x0210be25;
		*(u32*)0x020005e0 = 0x0200162d;
		*(u32*)0x020005e4 = 0x00000001;
		*(u32*)0x020009f8 = 0xeafffed8;
		*(u32*)0x0200147c = 0x46c02800;
		*(u32*)0x02004d30 = 0xf9d8f3f7; // ldmia r8, {r0-r2,r4-r9,r12-pc}
		*(u32*)0x02018f6c = 0x00001000; // andeq r1, r0, r0
		*(u32*)0x02019658 = 0xfcd2f3e2; // ldc2, cp3, crf, [r2], {e2}
		*(u32*)0x0205b24c = 0x63212100; // msrvs cpsr_c, #0
		*(u32*)0x02383f28 = 0xebb000fc; // bl 00f84320
		*(u32*)0x023fc000 = 0x480db4ff; // stmdami sp, {r0-r7,r10,r12-sp,pc}
		*(u32*)0x023fc004 = 0x1c016800; // stcne cp8, cr6, [r1], {0}
		*(u32*)0x023fc008 = 0x31f031f0; // ldrshcc r3, [r0, #10]!
		*(u32*)0x023fc00c = 0x1c0c7809; // stcne cp8, cr7, [r12], {9}
		*(u32*)0x023fc010 = 0x43512278; // cmpmi r1, #80000007
		*(u32*)0x023fc014 = 0x300c1840; // andcc r1, r12, r0, asr #10
		*(u32*)0x023fc018 = 0xf41e2100; // ldr r2, [lr], -#100!
		*(u32*)0x023fc01c = 0x1c01facd; // stcne cpa, crf, [r1], {cd}
		*(u32*)0x023fc020 = 0x02122210; // andeqs r2, r2, #1
		*(u32*)0x023fc024 = 0x1c204354; // stcne ,cp3 cr4, [r0], #-150
		*(u32*)0x023fc028 = 0x6018a355; // andvss r10, r8, r5, asr r3
		*(u32*)0x023fc02c = 0x609a6059; // addvss r6, r10, r9, asr r0
		*(u32*)0x023fc030 = 0x60dc2401; // sbcvss r2, r12, r1, lsl #8
		*(u32*)0x023fc034 = 0x4718bcff; // 
		*(u32*)0x023fc038 = 0x020c30dc; // andeq r3 ,r12, #dc
		*(u32*)0x023fc03c = 0xe2810001; // add r0 , r1, #1
		*(u32*)0x023fc040 = 0xe92d401f; // stmdb sp!, {r0-r4,lr}
		*(u32*)0x023fc044 = 0xe59f4140; // ldr r4, 023fc18c
		*(u32*)0x023fc048 = 0xe3540001; // cmp r4, #1
		*(u32*)0x023fc04c = 0x1a000005; // bne 023fc068
		*(u32*)0x023fc050 = 0xe59f0128; // ldr r0, 023fc180
		*(u32*)0x023fc054 = 0xe59f1128; // ldr r1, 023fc184
		*(u32*)0x023fc058 = 0xe59f2128; // ldr r2, 023fc188
		*(u32*)0x023fc05c = 0xe28fe06c; // add lr, pc, #6c
		*(u32*)0x023fc060 = 0xe59f3074; // ldr r3, 023fc0dc
		*(u32*)0x023fc064 = 0xe12fff13; // bx r3
		*(u32*)0x023fc068 = 0xe3540002; // cmp r4, #2
		*(u32*)0x023fc06c = 0x1a000017; // bne 023fc0d0
		*(u32*)0x023fc070 = 0xe59f0108; // ldr r0, 023fc180
		*(u32*)0x023fc074 = 0xe59f1108; // ldr r1, 023fc184
		*(u32*)0x023fc078 = 0xe59f2108; // ldr r2, 023fc188
		*(u32*)0x023fc07c = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc080 = 0xe59f3058; // ldr r3, 023fc0e0
		*(u32*)0x023fc084 = 0xe12fff13; // bx r3
		*(u32*)0x023fc088 = 0xe59f0100; // ldr r0, 023fc190
		*(u32*)0x023fc08c = 0xe59f1100; // ldr r1, 023fc194
		*(u32*)0x023fc090 = 0xe59f2100; // ldr r2, 023fc198
		*(u32*)0x023fc094 = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc098 = 0xe59f3040; // ldr r3, 023fc0e0
		*(u32*)0x023fc09c = 0xe12fff13; // bx r3
		*(u32*)0x023fc0a0 = 0xe59f00f8; // ldr r0, 023fc1a0
		*(u32*)0x023fc0a4 = 0xe59f10f8; // ldr r1, 023fc1a4
		*(u32*)0x023fc0a8 = 0xe59f20f8; // ldr r2, 023fc1a8
		*(u32*)0x023fc0ac = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc0b0 = 0xe59f3028; // ldr r3, 023fc0e0
		*(u32*)0x023fc0b4 = 0xe12fff13; // bx r3
		*(u32*)0x023fc0b8 = 0xe59f00f0; // ldr r0, 023fc1b0
		*(u32*)0x023fc0bc = 0xe59f10f0; // ldr r1, 023fc1b4
		*(u32*)0x023fc0c0 = 0xe59f20f0; // ldr r2, 023fc1b8
		*(u32*)0x023fc0c4 = 0xe28fe004; // add lr, pc, #4
		*(u32*)0x023fc0c8 = 0xe59f3010; // ldr r3, 023fc0e0
		*(u32*)0x023fc0cc = 0xe12fff13; // bx r3
		*(u32*)0x023fc0d0 = 0xe3a04000; // mov r4, #0
		*(u32*)0x023fc0d4 = 0xe58f40b0; // str r4, [pc, #b0]
		*(u32*)0x023fc0d8 = 0xe8bd801f; // ldmia sp!, {r0-r4,pc}
		*(u32*)0x023fc0dc = 0x038040b5; // orreq r4, r0, #b5
		*(u32*)0x023fc0e0 = 0x03804071; // orreq r4, r0, #71
		*(u32*)0x023fc0e4 = 0xf505b500; // str r11, [r5, -#500]
		*(u32*)0x023fc0e8 = 0xb4fffc6b; // ldrblt pc, [pc], #46b!
		*(u32*)0x023fc0ec = 0x4d164c15; // ldcmi cpc, cr4, [r6, #-54]
		*(u32*)0x023fc0f0 = 0x350c682d; // strcc r6, [r12, -@2d]
		*(u32*)0x023fc0f4 = 0x2000a622; // andcs r10, r0, r2, lsr #c
		*(u32*)0x023fc0f8 = 0x1c286030; // stcne, cp0, cr6, [r8], #-c0
		*(u32*)0x023fc0fc = 0xf41e2100; // ldr r2, [lr], -#100
		*(u32*)0x023fc100 = 0x6070fa5b; // rsbvss pc, r0, r11, asr r10
		*(u32*)0x023fc104 = 0x613460b4; // ldrhvs r6, [r4, -r4]!
		*(u32*)0x023fc108 = 0x1c283578; // stcne cp5, cr3, [r8], #-1e0
		*(u32*)0x023fc10c = 0xf41e2100; // ldr r2, [lr], #-100
		*(u32*)0x023fc110 = 0x6170fa53; // cmnvs r0, r3, asr r10
		*(u32*)0x023fc114 = 0x006061b4; // 
		*(u32*)0x023fc118 = 0x35786230; // ldrbcc r6, [r8, -#230]!
		*(u32*)0x023fc11c = 0x21001c28; // 
		*(u32*)0x023fc120 = 0xfa4af41e; // blx 0331d1a0
		*(u32*)0x023fc124 = 0x62b46270; // adcvss r6, r4, #7
		*(u32*)0x023fc128 = 0x19000060; // stmdbne r0, {r5-r6}
		*(u32*)0x023fc12c = 0x35786330; // ldrbcc r6, [r8, -#330]!
		*(u32*)0x023fc130 = 0x21001c28; // 
		*(u32*)0x023fc134 = 0xfa40f41e; // blx 0309d1b4
		*(u32*)0x023fc138 = 0x63b46370; // movvss r6, #c0000001
		*(u32*)0x023fc13c = 0x60f42402; // rscvss r2, r4, r2, lsl #8
		*(u32*)0x023fc140 = 0xbd00bcff; // stclt cpc ,crb, [r0-#-3fc]
		*(u32*)0x023fc144 = 0x00001000; // andeq r1, r0, r0
		*(u32*)0x023fc148 = 0x020c30dc; // andeq r3, r12, #dc
	}

	// WarioWare: DIY (USA)
	else if (strcmp(romTid, "UORE") == 0) {
		*(u32*)0x02003114 = 0xE12FFF1E; //bx lr
	}
	// WarioWare: Do It Yourself (Europe)
	else if (strcmp(romTid, "UORP") == 0) {
		*(u32*)0x020031B4 = 0xE12FFF1E; //bx lr
	}
	// Made in Ore (Japan)
	else if (strcmp(romTid, "UORJ") == 0) {
		*(u32*)0x020030F4 = 0xE12FFF1E; //bx lr
	}

    // Pokemon Dash (Japan)
	//else if (strcmp(romTid, "APDJ") == 0) {
		//*(u32*)0x0206AE70 = 0xE3A00000; //mov r0, #0
        //*(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		//*(u32*)0x0206AE74 = 0xe12fff1e; //bx lr

        //*(u32*)0x02000B94 = 0xE1A00000; //nop

		//*(u32*)0x020D5010 = 0xe12fff1e; //bx lr
	//}

    // Pokemon Dash
	//else if (strcmp(romTid, "APDE") == 0 || strcmp(romTid, "APDP") == 0) {
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
	//}

    /* // Pokemon Dash (Kiosk Demo)
	else if (strcmp(romTid, "A24E") == 0) {
        *(u32*)0x02000BB0 = 0xE1A00000; //nop
	}

    // Pokemon Dash (Korea)
	else if (strcmp(romTid, "APDK") == 0) {
        *(u32*)0x02000C14 = 0xE1A00000; //nop
	}*/

    // Pokemon HeartGold & SoulSilver
	/*else if (strcmp(romTid, "IPKJ") == 0 || strcmp(romTid, "IPGJ") == 0) {
        *(u32*)0x20DD9E4 = 0xE1A00000; //nop
	} else if (strcmp(romTid, "IPKK") == 0 || strcmp(romTid, "IPGK") == 0) {
        *(u32*)0x20DE860 = 0xE1A00000; //nop
	} else if (strncmp(romTid, "IPK", 3) == 0 || strncmp(romTid, "IPG", 3) == 0) {
        *(u32*)0x20DE16C = 0xE1A00000; //nop
	}*/

    // Golden Sun
    /*else if (strcmp(romTid, "BO5E") == 0) {
        // patch "refresh" function
        *(u32*)0x204995C = 0xe12fff1e; //bx lr
        *(u32*)0x20499C4 = 0xe12fff1e; //bx lr
    }*/

	// Tropix! Your Island Getaway
    else if (strcmp(romTid, "CTXE") == 0) {
		extern u32 baseChipID;
		u32 cardIdFunc[2] = {0, 0};
		tonccpy(cardIdFunc, ce9->thumbPatches->card_id_arm9, 0x4);
		cardIdFunc[1] = baseChipID;

		const u16* branchCode1 = generateA7InstrThumb(0x020BA666, 0x020BA670);

		*(u16*)0x020BA666 = branchCode1[0];
		*(u16*)0x020BA668 = branchCode1[1];

		tonccpy((void*)0x020BA670, cardIdFunc, 0x8);

		const u16* branchCode2 = generateA7InstrThumb(0x020BA66A, 0x020BA6C0);

		*(u16*)0x020BA66A = branchCode2[0];
		*(u16*)0x020BA66C = branchCode2[1];

		tonccpy((void*)0x020BA728, ce9->thumbPatches->card_set_dma_arm9, 0x30);

		const u16* branchCode3 = generateA7InstrThumb(0x020BA70C, 0x020BA728);

		*(u16*)0x020BA70C = branchCode3[0];
		*(u16*)0x020BA70E = branchCode3[1];
		*(u16*)0x020BA710 = 0xBDF8;

		const u16* branchCode4 = generateA7InstrThumb(0x020BAAA2, 0x020BAAAC);

		*(u16*)0x020BAAA2 = branchCode4[0];
		*(u16*)0x020BAAA4 = branchCode4[1];

		tonccpy((void*)0x020BAAAC, cardIdFunc, 0x8);

		const u16* branchCode5 = generateA7InstrThumb(0x020BAAA6, 0x020BAAFC);

		*(u16*)0x020BAAA6 = branchCode5[0];
		*(u16*)0x020BAAA8 = branchCode5[1];

		const u16* branchCode6 = generateA7InstrThumb(0x020BAC5C, 0x020BAC64);

		*(u16*)0x020BAC5C = branchCode6[0];
		*(u16*)0x020BAC5E = branchCode6[1];

		tonccpy((void*)0x020BAC64, cardIdFunc, 0x8);

		const u16* branchCode7 = generateA7InstrThumb(0x020BAC60, 0x020BACB6);

		*(u16*)0x020BAC60 = branchCode7[0];
		*(u16*)0x020BAC62 = branchCode7[1];
	}

	// DSiWare containing Cloneboot

	// Art Style: BASE 10 (USA)
	else if (strcmp(romTid, "KADE") == 0 && !dsiSD) {
		*(u32*)0x0202D25C = 0xEB00007C; // bl 0x0202D454 (Skip Manual screen)
	}

	// Art Style: CODE (Europe, Australia)
	else if (strcmp(romTid, "KADV") == 0 && !dsiSD) {
		*(u32*)0x0202D288 = 0xEB00007C; // bl 0x0202D480 (Skip manual screen)
	}

	// Art Style: DECODE (Japan)
	else if (strcmp(romTid, "KADJ") == 0 && !dsiSD) {
		*(u32*)0x0202E2AC = 0xEB000071; // bl 0x0202E478 (Skip manual screen)
	}

	// Pop Island (USA)
    else if (strcmp(romTid, "KPPE") == 0) {
        // Show "HELP" instead of "DEMO"
        *(u32*)0x202E220 = 0xE1A00000; //nop
        *(u32*)0x202E224 = 0xE1A00000; //nop
        *(u32*)0x202E228 = 0xE1A00000; //nop
    }

	// Pop Island (Europe)
    else if (strcmp(romTid, "KPPP") == 0) {
        // Show "HELP" instead of "DEMO"
        *(u32*)0x202DC1C = 0xE1A00000; //nop
        *(u32*)0x202DC20 = 0xE1A00000; //nop
        *(u32*)0x202DC24 = 0xE1A00000; //nop
    }

	// Pop Island: Paperfield (USA)
    else if (strcmp(romTid, "KPFE") == 0) {
        // Show "HELP" instead of "DEMO"
        *(u32*)0x202E6C8 = 0xE1A00000; //nop
        *(u32*)0x202E6CC = 0xE1A00000; //nop
        *(u32*)0x202E6D0 = 0xE1A00000; //nop
    }

	// Pop Island: Paperfield (Europe)
    else if (strcmp(romTid, "KPFP") == 0) {
        // Show "HELP" instead of "DEMO"
        *(u32*)0x202E698 = 0xE1A00000; //nop
        *(u32*)0x202E69C = 0xE1A00000; //nop
        *(u32*)0x202E6A0 = 0xE1A00000; //nop
    }
}

void bannerSavPatch(const tNDSHeader* ndsHeader) {
	// Patch out banner.sav check
	//const char* newBannerPath = "dataPrv:";
	const char* romTid = getRomTid(ndsHeader);

	// Touhoku Daigaku Karei Igaku Kenkyuusho Kawashi Maryuuta Kyouji Kanchuu: Chotto Nou o Kitaeru Otona no DSi Training: Sudoku-Hen (Japan)
	if (strcmp(romTid, "KN9J") == 0) {
		*(u32*)0x0201161C = 0xE3A00001; // mov r0, #1
	}

	// Brain Age Express: Sudoku (USA)
	else if (strcmp(romTid, "KN9E") == 0) {
		//toncset((char*)0x020925A0, 0, 0xE);
		//tonccpy((char*)0x020925A0, newBannerPath, 8);

		*(u32*)0x0201178C = 0xE3A00001; // mov r0, #1
	}

	// A Little Bit of... Brain Training: Sudoku (Europe, Australia)
	else if (strcmp(romTid, "KN9V") == 0) {
		*(u32*)0x02011774 = 0xE3A00001; // mov r0, #1
	}

	// Even Tijd Voor... Brain Training: Sudoku (Netherlands)
	else if (strcmp(romTid, "KN9H") == 0) {
		*(u32*)0x02011718 = 0xE3A00001; // mov r0, #1
	}
}

/*void patchSlot2Addr(const tNDSHeader* ndsHeader) {
	extern u32 gbaAddrToDsi[];

	if (!gbaRomFound) {
		return;
	}

	const char* romTid = getRomTid(ndsHeader);

	if (strcmp(romTid, "ARZE") == 0) {	// MegaMan ZX
		for (u32 addr = 0x0203740C; addr <= 0x02044790; addr += 4) {
			if (*(u32*)addr >= 0x08000000 && *(u32*)addr < 0x08020000) {
				*(u32*)addr += 0x05000000;
			}
		}
		*(u32*)0x0203A260 = 0x0D000800;	// Originally 0xC000800, for some weird reason
		*(u32*)0x0203A708 = 0x0D000800;	// Originally 0xC000800, for some weird reason
		*(u32*)0x0203AFC0 = 0x0D000800;	// Originally 0xC000800, for some weird reason
		*(u32*)0x0203C178 = 0x0D010001;	// Originally 0xC010001, for some weird reason
		*(u32*)0x0203D448 = 0x0D010001;	// Originally 0xC010001, for some weird reason
		*(u32*)0x0203D678 = 0x0D000800;	// Originally 0xC000800, for some weird reason
		*(u32*)0x02041D64 = 0x0D010000;	// Originally 0xC010000, for some weird reason
		for (u32 addr = 0x020CA234; addr <= 0x020CA2C0; addr += 4) {
			*(u32*)addr += 0x05000000;
		}
		return;
	}

	else if (strcmp(romTid, "CPUE") == 0 && ndsHeader->romversion == 0) {	// Pokemon Platinum Version
		*(u32*)0x020D0A60 = gbaAddrToDsi[0];
		*(u32*)0x020D0AA8 = gbaAddrToDsi[0];
		*(u32*)0x020D0B0C = 0x0D0000CE;
		*(u32*)0x020D0C68 = gbaAddrToDsi[1];
		*(u32*)0x020D0CB0 = 0x02610000;
		*(u32*)0x020D1248 = 0x0D000080;
		*(u32*)0x020D14D4 = gbaAddrToDsi[2];
		*(u32*)0x020D14E0 = 0x02605555;
		*(u32*)0x020D14E4 = 0x02602AAA;
		*(u32*)0x020D1560 = gbaAddrToDsi[3];
		*(u32*)0x020D15E8 = 0x02605555;
		*(u32*)0x020D15EC = 0x02602AAA;
		*(u32*)0x020D15F0 = 0x02600001;
		*(u32*)0x020D172C = 0x02605555;
		*(u32*)0x020D17E0 = 0x02605555;
		*(u32*)0x020D1880 = gbaAddrToDsi[4];
		*(u32*)0x020D1884 = gbaAddrToDsi[5];
		*(u32*)0x020D19A4 = gbaAddrToDsi[6];
		*(u32*)0x020D19A8 = gbaAddrToDsi[7];
		*(u32*)0x020D1B90 = gbaAddrToDsi[2];
		*(u32*)0x020D1BDC = 0x02605555;
		*(u32*)0x020D1BE0 = 0x02602AAA;
		*(u32*)0x020D1C20 = gbaAddrToDsi[8];
		*(u32*)0x020D1C24 = gbaAddrToDsi[9];
		*(u32*)0x020D1D00 = 0x02605555;
		*(u32*)0x020D1D04 = 0x02602AAA;
		*(u32*)0x020D1E4C = gbaAddrToDsi[10];
		*(u32*)0x020D1E50 = gbaAddrToDsi[11];
		*(u32*)0x020D1EC4 = 0x02605555;
		*(u32*)0x020D1EC8 = 0x02602AAA;
		*(u32*)0x020D21A4 = gbaAddrToDsi[2];
		*(u32*)0x020D21F0 = 0x02605555;
		*(u32*)0x020D21F4 = 0x02602AAA;
		*(u32*)0x020D22B0 = gbaAddrToDsi[12];
		*(u32*)0x020D22B4 = gbaAddrToDsi[13];
		*(u32*)0x020D2324 = 0x02605555;
		*(u32*)0x020D2328 = 0x02602AAA;
		*(u32*)0x020D2378 = 0x02605555;
		*(u32*)0x020D237C = 0x02602AAA;
		*(u32*)0x020D23DC = gbaAddrToDsi[14];
		*(u32*)0x020D23E0 = gbaAddrToDsi[15];
		*(u32*)0x020D2784 = gbaAddrToDsi[2];
		*(u32*)0x020D27D0 = 0x02605555;
		*(u32*)0x020D27D4 = 0x02602AAA;
		*(u32*)0x020D28CC = gbaAddrToDsi[6];
		*(u32*)0x020D28D0 = gbaAddrToDsi[7];
		*(u32*)0x020D2954 = 0x02605555;
		*(u32*)0x020D295C = 0x02602AAA;
		*(u32*)0x020D29Ac = 0x02605555;
		*(u32*)0x020D29B0 = 0x02602AAA;
		*(u32*)0x020D2A90 = gbaAddrToDsi[16];
		*(u32*)0x020D2A94 = gbaAddrToDsi[17];
		*(u32*)0x020D2CC4 = gbaAddrToDsi[18];
		*(u32*)0x020D2CC8 = gbaAddrToDsi[19];
		return;
	}

	gbaRomFound = false;	// Do not load GBA ROM
}*/

static bool rsetA7CacheDone = false;

void rsetA7Cache(void)
{
	if (rsetA7CacheDone) return;

	patchOffsetCache.a7BinSize = 0;
	patchOffsetCache.a7IsThumb = 0;
	patchOffsetCache.swiHaltOffset = 0;
	patchOffsetCache.a7Swi12Offset = 0;
	patchOffsetCache.a7Swi24Offset = 0;
	patchOffsetCache.a7Swi25Offset = 0;
	patchOffsetCache.a7Swi26Offset = 0;
	patchOffsetCache.a7Swi27Offset = 0;
	patchOffsetCache.a7ScfgExtOffset = 0;
	patchOffsetCache.swiGetPitchTableOffset = 0;
	patchOffsetCache.swiGetPitchTableChecked = 0;
	patchOffsetCache.sleepPatchOffset = 0;
	patchOffsetCache.postBootOffset = 0;
	patchOffsetCache.a7CardIrqEnableOffset = 0;
	patchOffsetCache.cardCheckPullOutOffset = 0;
	patchOffsetCache.cardCheckPullOutChecked = 0;
	patchOffsetCache.sdCardResetOffset = 0;
	patchOffsetCache.a7IrqHandlerOffset = 0;
	patchOffsetCache.a7IrqHandlerWordsOffset = 0;
	patchOffsetCache.a7IrqHookOffset = 0;
	patchOffsetCache.savePatchType = 0;
	patchOffsetCache.relocateStartOffset = 0;
	patchOffsetCache.relocateValidateOffset = 0;
	patchOffsetCache.a7iStartOffset = 0;
	patchOffsetCache.a7CardReadEndOffset = 0;
	patchOffsetCache.a7JumpTableFuncOffset = 0;
	patchOffsetCache.a7JumpTableType = 0;

	rsetA7CacheDone = true;
}

void rsetPatchCache(bool dsiWare)
{
	extern u32 srlAddr;

	if (patchOffsetCache.ver != patchOffsetCacheFileVersion
	 || patchOffsetCache.type != 0) {
		if (srlAddr == 0 && !dsiWare && !esrbScreenPrepared) pleaseWaitOutput();
		u32* moduleParamsOffset = patchOffsetCache.moduleParamsOffset;
		u32* ltdModuleParamsOffset = patchOffsetCache.ltdModuleParamsOffset;
		toncset(&patchOffsetCache, 0, sizeof(patchOffsetCacheContents));
		patchOffsetCache.ver = patchOffsetCacheFileVersion;
		patchOffsetCache.type = 0;	// 0 = Regular, 1 = B4DS, 2 = HB
		patchOffsetCache.moduleParamsOffset = moduleParamsOffset;
		patchOffsetCache.ltdModuleParamsOffset = ltdModuleParamsOffset;
		rsetA7CacheDone = true;
	}
}

u32 patchCardNds(
	cardengineArm7* ce7,
	cardengineArm9* ce9,
	tNDSHeader* ndsHeader,
	const module_params_t* moduleParams,
	u32 patchMpuRegion,
	bool usesCloneboot,
	u32 ROMinRAM,
	u32 saveFileCluster,
	u32 saveSize
) {
	dbg_printf("patchCardNds\n\n");

	bool sdk5 = isSdk5(moduleParams);
	if (sdk5) {
		dbg_printf("[SDK 5]\n\n");
	}

	u32 errorCodeArm9 = patchCardNdsArm9(ce9, ndsHeader, moduleParams, ROMinRAM, patchMpuRegion, usesCloneboot);
	
	if (errorCodeArm9 == ERR_NONE || ndsHeader->fatSize == 0) {
		return patchCardNdsArm7(ce7, ndsHeader, moduleParams, ROMinRAM, saveFileCluster);
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}