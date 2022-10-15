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

extern u16 saveOnFlashcard;
extern u8 valueBits3;
#define memoryPit (valueBits3 & BIT(1))

u16 patchOffsetCacheFilePrevCrc = 0;
u16 patchOffsetCacheFileNewCrc = 0;

patchOffsetCacheContents patchOffsetCache;

extern bool logging;
extern bool gbaRomFound;
extern u8 dsiSD;

static inline void doubleNopT(u32 addr) {
	*(u16*)(addr)   = 0x46C0;
	*(u16*)(addr+2) = 0x46C0;
}

void dsiWarePatch(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
	extern u8 consoleModel;
	const char* romTid = getRomTid(ndsHeader);
	const char* dataPub = "dataPub:";
	//const char* chnFontPath = "sdmc:/sys/CHNFontTable.dat";

	const u32* dsiSaveGetResultCode = ce9->patches->dsiSaveGetResultCode;
	const u32* dsiSaveCreate = ce9->patches->dsiSaveCreate;
	const u32* dsiSaveDelete = ce9->patches->dsiSaveDelete;
	const u32* dsiSaveGetInfo = ce9->patches->dsiSaveGetInfo;
	const u32* dsiSaveSetLength = ce9->patches->dsiSaveSetLength;
	const u32* dsiSaveOpen = ce9->patches->dsiSaveOpen;
	const u32* dsiSaveOpenR = ce9->patches->dsiSaveOpenR;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveGetLength = ce9->patches->dsiSaveGetLength;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;
	const u32* dsiSaveWrite = ce9->patches->dsiSaveWrite;

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

	// 40-in-1: Explosive Megamix (USA)
	else if (strcmp(romTid, "K45E") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0200DFCC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200DFD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E2D0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E2D4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E408 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E40C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E54C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E550 = 0xE12FFF1E; // bx lr */
		//*(u32*)0x0200E010 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0200E0D4, (u32)dsiSaveCreate);
		setBL(0x0200E0E4, (u32)dsiSaveOpen);
		setBL(0x0200E11C, (u32)dsiSaveSetLength);
		setBL(0x0200E138, (u32)dsiSaveWrite);
		setBL(0x0200E14C, (u32)dsiSaveClose);
		*(u32*)0x0200E304 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E39C, (u32)dsiSaveOpen);
		setBL(0x0200E3D0, (u32)dsiSaveRead);
		setBL(0x0200E3E4, (u32)dsiSaveClose);
		*(u32*)0x0200E440 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E470 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E494, 0x0200E4D4);
		*(u32*)0x0200E4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E578 = 0xE3A00001; // mov r0, #1
		tonccpy((u32*)0x020FDDC8, dsiSaveGetResultCode, 0xC);
	}

	// 40-in-1: Explosive Megamix (Europe)
	else if (strcmp(romTid, "K45P") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0200DF7C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200DF80 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E280 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E284 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E3B8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E3BC = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E4FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E500 = 0xE12FFF1E; // bx lr */
		//*(u32*)0x0200DFC0 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0200E084, (u32)dsiSaveCreate);
		setBL(0x0200E094, (u32)dsiSaveOpen);
		setBL(0x0200E0CC, (u32)dsiSaveSetLength);
		setBL(0x0200E0E8, (u32)dsiSaveWrite);
		setBL(0x0200E0FC, (u32)dsiSaveClose);
		*(u32*)0x0200E2B4 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E34C, (u32)dsiSaveOpen);
		setBL(0x0200E380, (u32)dsiSaveRead);
		setBL(0x0200E394, (u32)dsiSaveClose);
		*(u32*)0x0200E3F0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E420 = 0xE3A00001; // mov r0, #1
		setBL(0x0200E444, 0x0200E484);
		*(u32*)0x0200E4A8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200E528 = 0xE3A00001; // mov r0, #1
		tonccpy((u32*)0x020FD438, dsiSaveGetResultCode, 0xC);
	}

	// 99Bullets (USA)
	else if (strcmp(romTid, "K99E") == 0 && saveOnFlashcard) {
		*(u32*)0x02013E8C = 0xE3A00001; // mov r0, #1
		setBL(0x02031FE8, (u32)dsiSaveOpen);
		setBL(0x02032000, (u32)dsiSaveGetLength);
		setBL(0x02032010, (u32)dsiSaveSeek);
		setBL(0x02032020, (u32)dsiSaveWrite);
		setBL(0x02032028, (u32)dsiSaveClose);
		setBL(0x02032098, (u32)dsiSaveOpen);
		setBL(0x020320B0, (u32)dsiSaveGetLength);
		setBL(0x020320C4, (u32)dsiSaveSeek);
		setBL(0x020320D4, (u32)dsiSaveRead);
		setBL(0x020320DC, (u32)dsiSaveClose);
		setBL(0x02032154, (u32)dsiSaveCreate);
		setBL(0x02032180, (u32)dsiSaveOpen);
		setBL(0x020321BC, (u32)dsiSaveWrite);
		setBL(0x020321CC, (u32)dsiSaveClose);
	}

	// 99Bullets (Europe)
	else if (strcmp(romTid, "K99P") == 0 && saveOnFlashcard) {
		*(u32*)0x02012F1C = 0xE3A00001; // mov r0, #1
		setBL(0x020310C8, (u32)dsiSaveOpen);
		setBL(0x020310E0, (u32)dsiSaveGetLength);
		setBL(0x020310F0, (u32)dsiSaveSeek);
		setBL(0x02031100, (u32)dsiSaveWrite);
		setBL(0x02031108, (u32)dsiSaveClose);
		setBL(0x02031178, (u32)dsiSaveOpen);
		setBL(0x02031090, (u32)dsiSaveGetLength);
		setBL(0x020311A4, (u32)dsiSaveSeek);
		setBL(0x020311B4, (u32)dsiSaveRead);
		setBL(0x020311BC, (u32)dsiSaveClose);
		setBL(0x02031234, (u32)dsiSaveCreate);
		setBL(0x02031260, (u32)dsiSaveOpen);
		setBL(0x0203129C, (u32)dsiSaveWrite);
		setBL(0x020312AC, (u32)dsiSaveClose); 
	}

	// 99Bullets (Japan)
	else if (strcmp(romTid, "K99J") == 0 && saveOnFlashcard) {
		setBL(0x02012E48, (u32)dsiSaveCreate);
		*(u32*)0x02012E68 = 0xE3A00001; // mov r0, #1
		setBL(0x02012F0C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012F34 = 0xE3A00001; // mov r0, #1
		setBL(0x02030FCC, (u32)dsiSaveOpen);
		setBL(0x02030FE4, (u32)dsiSaveGetLength);
		setBL(0x02030FF4, (u32)dsiSaveSeek);
		setBL(0x02031004, (u32)dsiSaveWrite);
		setBL(0x0203100C, (u32)dsiSaveClose);
		setBL(0x0203107C, (u32)dsiSaveOpen);
		setBL(0x02031094, (u32)dsiSaveGetLength);
		setBL(0x020310A8, (u32)dsiSaveSeek);
		setBL(0x020310B8, (u32)dsiSaveRead);
		setBL(0x020310C0, (u32)dsiSaveClose);
		setBL(0x020311C8, (u32)dsiSaveCreate);
		setBL(0x02031164, (u32)dsiSaveOpen);
		setBL(0x020311A0, (u32)dsiSaveWrite);
		setBL(0x020311B0, (u32)dsiSaveClose);
	}

	// 99Moves (USA)
	// 99Moves (Europe)
	else if ((strcmp(romTid, "K9WE") == 0 || strcmp(romTid, "K9WP") == 0) && saveOnFlashcard) {
		setBL(0x02012BD4, (u32)dsiSaveCreate);
		*(u32*)0x02012BF4 = 0xE3A00001; // mov r0, #1
		setBL(0x02012C98, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012CC0 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x02031820, (u32)dsiSaveOpen);
			setBL(0x02031838, (u32)dsiSaveGetLength);
			setBL(0x02031848, (u32)dsiSaveSeek);
			setBL(0x02031858, (u32)dsiSaveWrite);
			setBL(0x02031860, (u32)dsiSaveClose);
			setBL(0x020318D0, (u32)dsiSaveOpen);
			setBL(0x020318E8, (u32)dsiSaveGetLength);
			setBL(0x020318FC, (u32)dsiSaveSeek);
			setBL(0x0203190C, (u32)dsiSaveRead);
			setBL(0x02031914, (u32)dsiSaveClose);
			setBL(0x0203198C, (u32)dsiSaveCreate);
			setBL(0x020319B8, (u32)dsiSaveOpen);
			setBL(0x020319F4, (u32)dsiSaveWrite);
			setBL(0x02031A04, (u32)dsiSaveClose);
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x02031870, (u32)dsiSaveOpen);
			setBL(0x02031888, (u32)dsiSaveGetLength);
			setBL(0x02031898, (u32)dsiSaveSeek);
			setBL(0x020318A8, (u32)dsiSaveWrite);
			setBL(0x020318B0, (u32)dsiSaveClose);
			setBL(0x02031920, (u32)dsiSaveOpen);
			setBL(0x02031938, (u32)dsiSaveGetLength);
			setBL(0x0203196C, (u32)dsiSaveSeek);
			setBL(0x0203195C, (u32)dsiSaveRead);
			setBL(0x02031964, (u32)dsiSaveClose);
			setBL(0x020319DC, (u32)dsiSaveCreate);
			setBL(0x02031A08, (u32)dsiSaveOpen);
			setBL(0x02031A44, (u32)dsiSaveWrite);
			setBL(0x02031A54, (u32)dsiSaveClose);
		}
	}

	// 99Seconds (USA)
	// 99Seconds (Europe)
	else if ((strcmp(romTid, "KXTE") == 0 || strcmp(romTid, "KXTP") == 0) && saveOnFlashcard) {
		setBL(0x02011918, (u32)dsiSaveCreate);
		*(u32*)0x02011938 = 0xE3A00001; // mov r0, #1
		setBL(0x020119E0, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011A08 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x020302D4, (u32)dsiSaveOpen);
			setBL(0x020302EC, (u32)dsiSaveGetLength);
			setBL(0x020302FC, (u32)dsiSaveSeek);
			setBL(0x0203030C, (u32)dsiSaveWrite);
			setBL(0x02030314, (u32)dsiSaveClose);
			setBL(0x02030384, (u32)dsiSaveOpen);
			setBL(0x0203039C, (u32)dsiSaveGetLength);
			setBL(0x020303B0, (u32)dsiSaveSeek);
			setBL(0x020303C0, (u32)dsiSaveRead);
			setBL(0x020303C8, (u32)dsiSaveClose);
			setBL(0x02030440, (u32)dsiSaveCreate);
			setBL(0x0203046C, (u32)dsiSaveOpen);
			setBL(0x020304A8, (u32)dsiSaveWrite);
			setBL(0x020304B8, (u32)dsiSaveClose);
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x02030324, (u32)dsiSaveOpen);
			setBL(0x0203033C, (u32)dsiSaveGetLength);
			setBL(0x0203034C, (u32)dsiSaveSeek);
			setBL(0x0203035C, (u32)dsiSaveWrite);
			setBL(0x02030364, (u32)dsiSaveClose);
			setBL(0x020303D4, (u32)dsiSaveOpen);
			setBL(0x020303EC, (u32)dsiSaveGetLength);
			setBL(0x02030400, (u32)dsiSaveSeek);
			setBL(0x02030410, (u32)dsiSaveRead);
			setBL(0x02030418, (u32)dsiSaveClose);
			setBL(0x02030490, (u32)dsiSaveCreate);
			setBL(0x020304BC, (u32)dsiSaveOpen);
			setBL(0x020304F8, (u32)dsiSaveWrite);
			setBL(0x02030508, (u32)dsiSaveClose);
		}
	}

	// Absolute BrickBuster (USA)
	else if (strcmp(romTid, "K6QE") == 0) {
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

	// Amakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF2J") == 0 && saveOnFlashcard) {
		setBL(0x0203C1C8, (u32)dsiSaveOpen);
		setBL(0x0203C1F4, (u32)dsiSaveRead);
		setBL(0x0203C204, (u32)dsiSaveClose);
		setBL(0x0203C220, (u32)dsiSaveClose);
		*(u32*)0x0203C274 = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x0203C2B0 = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x0203C2BC, (u32)dsiSaveCreate);
		setBL(0x0203C2CC, (u32)dsiSaveOpen);
		setBL(0x0203C2F8, (u32)dsiSaveSetLength);
		setBL(0x0203C308, (u32)dsiSaveClose);
		setBL(0x0203C32C, (u32)dsiSaveWrite);
		setBL(0x0203C33C, (u32)dsiSaveClose);
		setBL(0x0203C358, (u32)dsiSaveClose);
	}

	// Anne's Doll Studio: Antique Collection (USA)
	// Anne's Doll Studio: Antique Collection (Europe)
	// Anne's Doll Studio: Princess Collection (USA)
	// Anne's Doll Studio: Princess Collection (Europe)
	else if ((strcmp(romTid, "KY8E") == 0 || strcmp(romTid, "KY8P") == 0
		   || strcmp(romTid, "K2SE") == 0 || strcmp(romTid, "K2SP") == 0) && saveOnFlashcard) {
		setBL(0x0202A164, (u32)dsiSaveGetResultCode);
		setBL(0x0202A288, (u32)dsiSaveOpen);
		setBL(0x0202A2BC, (u32)dsiSaveRead);
		setBL(0x0202A2E4, (u32)dsiSaveClose);
		setBL(0x0202A344, (u32)dsiSaveOpen);
		setBL(0x0202A38C, (u32)dsiSaveWrite);
		setBL(0x0202A3AC, (u32)dsiSaveClose);
		setBL(0x0202A3F0, (u32)dsiSaveCreate);
		setBL(0x0202A44C, (u32)dsiSaveDelete);
		if (strncmp(romTid, "KY8", 3) == 0) {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x0203B89C = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAFC = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BB00 = 0xE12FFF1E; // bx lr
			} else {
				*(u32*)0x0203B844 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAA4 = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BAA8 = 0xE12FFF1E; // bx lr
			}
		} else {
			*(u32*)0x0203B678 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x0203B8D8 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x0203B8DC = 0xE12FFF1E; // bx lr
		}
	}

	// Anne's Doll Studio: Gothic Collection (USA)
	else if (strcmp(romTid, "K54E") == 0 && saveOnFlashcard) {
		*(u32*)0x02033850 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x02033AB0 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x02033AB4 = 0xE12FFF1E; // bx lr
		setBL(0x02035614, (u32)dsiSaveGetResultCode);
		setBL(0x02035738, (u32)dsiSaveOpen);
		setBL(0x0203576C, (u32)dsiSaveRead);
		setBL(0x02035794, (u32)dsiSaveClose);
		setBL(0x020357F4, (u32)dsiSaveOpen);
		setBL(0x0203583C, (u32)dsiSaveWrite);
		setBL(0x0203585C, (u32)dsiSaveClose);
		setBL(0x020358A0, (u32)dsiSaveCreate);
		setBL(0x020358FC, (u32)dsiSaveDelete);
	}

	// Anne's Doll Studio: Lolita Collection (USA)
	// Anne's Doll Studio: Lolita Collection (Europe)
	else if ((strcmp(romTid, "KLQE") == 0 || strcmp(romTid, "KLQP") == 0) && saveOnFlashcard) {
		*(u32*)0x020337B0 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x02033A10 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x02033A14 = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x020355C4, (u32)dsiSaveGetResultCode);
			setBL(0x020356E8, (u32)dsiSaveOpen);
			setBL(0x0203571C, (u32)dsiSaveRead);
			setBL(0x02035744, (u32)dsiSaveClose);
			setBL(0x020357A4, (u32)dsiSaveOpen);
			setBL(0x020357EC, (u32)dsiSaveWrite);
			setBL(0x0203580C, (u32)dsiSaveClose);
			setBL(0x02035850, (u32)dsiSaveCreate);
			setBL(0x020358AC, (u32)dsiSaveDelete);
		} else {
			setBL(0x02035570, (u32)dsiSaveGetResultCode);
			setBL(0x02035694, (u32)dsiSaveOpen);
			setBL(0x020356C8, (u32)dsiSaveRead);
			setBL(0x020356F0, (u32)dsiSaveClose);
			setBL(0x02035750, (u32)dsiSaveOpen);
			setBL(0x020357EC, (u32)dsiSaveWrite);
			setBL(0x02035798, (u32)dsiSaveClose);
			setBL(0x020357FC, (u32)dsiSaveCreate);
			setBL(0x02035858, (u32)dsiSaveDelete);
		}
	}

	// Anne's Doll Studio: Tokyo Collection (USA)
	else if (strcmp(romTid, "KSQE") == 0 && saveOnFlashcard) {
		setBL(0x02027F34, (u32)dsiSaveGetResultCode);
		setBL(0x02028058, (u32)dsiSaveOpen);
		setBL(0x0202808C, (u32)dsiSaveRead);
		setBL(0x020280B4, (u32)dsiSaveClose);
		setBL(0x02028114, (u32)dsiSaveOpen);
		setBL(0x0202815C, (u32)dsiSaveWrite);
		setBL(0x0202817C, (u32)dsiSaveClose);
		setBL(0x020281C0, (u32)dsiSaveCreate);
		setBL(0x0202821C, (u32)dsiSaveDelete);
		*(u32*)0x0203A534 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x0203A794 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x0203A798 = 0xE12FFF1E; // bx lr
	}

	// Anonymous Notes 1: From The Abyss (USA & Europe)
	// Anonymous Notes 2: From The Abyss (USA & Europe)
	else if ((strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0)
	  && ndsHeader->gameCode[3] != 'J' && saveOnFlashcard) {
		//*(u32*)0x02023DB0 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x02023DB4 = 0xE12FFF1E; // bx lr
		setBL(0x02024220, (u32)dsiSaveOpen);
		setBL(0x02024258, (u32)dsiSaveCreate);
		setBL(0x02024268, (u32)dsiSaveGetResultCode);
		setBL(0x02024290, (u32)dsiSaveOpen);
		setBL(0x020242BC, (u32)dsiSaveGetLength);
		setBL(0x020242D8, (u32)dsiSaveSetLength);
		setBL(0x02024328, (u32)dsiSaveWrite);
		setBL(0x02024338, (u32)dsiSaveClose);
		setBL(0x02024358, (u32)dsiSaveClose);
		setBL(0x020243A0, (u32)dsiSaveOpen);
		setBL(0x020243D8, (u32)dsiSaveSeek);
		setBL(0x020243E8, (u32)dsiSaveClose);
		setBL(0x02024400, (u32)dsiSaveWrite);
		setBL(0x02024410, (u32)dsiSaveClose);
		setBL(0x02024420, (u32)dsiSaveClose);
		setBL(0x02024460, (u32)dsiSaveOpen);
		setBL(0x0202448C, (u32)dsiSaveGetLength);
		setBL(0x020244AC, (u32)dsiSaveSeek);
		setBL(0x020244BC, (u32)dsiSaveClose);
		setBL(0x020244D4, (u32)dsiSaveRead);
		setBL(0x020244E4, (u32)dsiSaveClose);
		setBL(0x020244F4, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[2] == 'I') {
			if (ndsHeader->gameCode[3] == 'E') {
				//*(u32*)0x020CE830 = 0xE12FFF1E; // bx lr
				*(u32*)0x020CFCD0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				*(u32*)0x020CFAE0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		} else if (ndsHeader->gameCode[2] == '2') {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x020D0874 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				*(u32*)0x020CFAE0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		}
	}

	// Anonymous Notes 1: From The Abyss (Japan)
	// Anonymous Notes 2: From The Abyss (Japan)
	else if ((strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0) && saveOnFlashcard) {
		setBL(0x0202481C, (u32)dsiSaveOpen);
		setBL(0x02024854, (u32)dsiSaveCreate);
		setBL(0x02024864, (u32)dsiSaveGetResultCode);
		setBL(0x0202488C, (u32)dsiSaveOpen);
		setBL(0x020248B8, (u32)dsiSaveGetLength);
		setBL(0x020248D4, (u32)dsiSaveSetLength);
		setBL(0x02024924, (u32)dsiSaveWrite);
		setBL(0x02024934, (u32)dsiSaveClose);
		setBL(0x02024954, (u32)dsiSaveClose);
		setBL(0x0202499C, (u32)dsiSaveOpen);
		setBL(0x020249D4, (u32)dsiSaveSeek);
		setBL(0x020249E4, (u32)dsiSaveClose);
		setBL(0x020249FC, (u32)dsiSaveWrite);
		setBL(0x02024A0C, (u32)dsiSaveClose);
		setBL(0x02024A1C, (u32)dsiSaveClose);
		setBL(0x02024A5C, (u32)dsiSaveOpen);
		setBL(0x02024A88, (u32)dsiSaveGetLength);
		setBL(0x02024AA8, (u32)dsiSaveSeek);
		setBL(0x02024AB8, (u32)dsiSaveClose);
		setBL(0x02024AD0, (u32)dsiSaveRead);
		setBL(0x02024AE0, (u32)dsiSaveClose);
		setBL(0x02024AF0, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[2] == 'I') {
			*(u32*)0x020CF970 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		} else if (ndsHeader->gameCode[2] == '2') {
			*(u32*)0x020D050C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
	}

	// Anonymous Notes 3: From The Abyss (USA & Japan)
	// Anonymous Notes 4: From The Abyss (USA & Japan)
	else if ((strncmp(romTid, "KV3", 3) == 0 || strncmp(romTid, "KV4", 3) == 0) && saveOnFlashcard) {
		if (ndsHeader->gameCode[2] == '3') {
			if (ndsHeader->gameCode[3] == 'E') {
				setBL(0x0202424C, (u32)dsiSaveOpen);
				setBL(0x02024284, (u32)dsiSaveCreate);
				setBL(0x02024294, (u32)dsiSaveGetResultCode);
				setBL(0x020242BC, (u32)dsiSaveOpen);
				setBL(0x020242E8, (u32)dsiSaveGetLength);
				setBL(0x02024304, (u32)dsiSaveSetLength);
				setBL(0x02024354, (u32)dsiSaveWrite);
				setBL(0x02024364, (u32)dsiSaveClose);
				setBL(0x02024384, (u32)dsiSaveClose);
				setBL(0x020243CC, (u32)dsiSaveOpen);
				setBL(0x02024404, (u32)dsiSaveSeek);
				setBL(0x02024414, (u32)dsiSaveClose);
				setBL(0x0202442C, (u32)dsiSaveWrite);
				setBL(0x0202443C, (u32)dsiSaveClose);
				setBL(0x0202444C, (u32)dsiSaveClose);
				setBL(0x0202448C, (u32)dsiSaveOpen);
				setBL(0x020244B8, (u32)dsiSaveGetLength);
				setBL(0x020244D8, (u32)dsiSaveSeek);
				setBL(0x020244E8, (u32)dsiSaveClose);
				setBL(0x02024500, (u32)dsiSaveRead);
				setBL(0x02024510, (u32)dsiSaveClose);
				setBL(0x02024520, (u32)dsiSaveClose);
				*(u32*)0x020D0120 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				setBL(0x02024270, (u32)dsiSaveOpen);
				setBL(0x020242A8, (u32)dsiSaveCreate);
				setBL(0x020242B8, (u32)dsiSaveGetResultCode);
				setBL(0x020242E0, (u32)dsiSaveOpen);
				setBL(0x0202430C, (u32)dsiSaveGetLength);
				setBL(0x02024328, (u32)dsiSaveSetLength);
				setBL(0x02024378, (u32)dsiSaveWrite);
				setBL(0x02024388, (u32)dsiSaveClose);
				setBL(0x020243A8, (u32)dsiSaveClose);
				setBL(0x020243F0, (u32)dsiSaveOpen);
				setBL(0x02024428, (u32)dsiSaveSeek);
				setBL(0x02024438, (u32)dsiSaveClose);
				setBL(0x02024450, (u32)dsiSaveWrite);
				setBL(0x02024460, (u32)dsiSaveClose);
				setBL(0x02024470, (u32)dsiSaveClose);
				setBL(0x020244B0, (u32)dsiSaveOpen);
				setBL(0x020244DC, (u32)dsiSaveGetLength);
				setBL(0x020244FC, (u32)dsiSaveSeek);
				setBL(0x0202450C, (u32)dsiSaveClose);
				setBL(0x02024524, (u32)dsiSaveRead);
				setBL(0x02024534, (u32)dsiSaveClose);
				setBL(0x02024544, (u32)dsiSaveClose);
				*(u32*)0x020CF8AC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		} else if (ndsHeader->gameCode[2] == '4') {
			if (ndsHeader->gameCode[3] == 'E') {
				setBL(0x0202425C, (u32)dsiSaveOpen);
				setBL(0x02024294, (u32)dsiSaveCreate);
				setBL(0x020242A4, (u32)dsiSaveGetResultCode);
				setBL(0x020242CC, (u32)dsiSaveOpen);
				setBL(0x020242F8, (u32)dsiSaveGetLength);
				setBL(0x02024314, (u32)dsiSaveSetLength);
				setBL(0x02024364, (u32)dsiSaveWrite);
				setBL(0x02024374, (u32)dsiSaveClose);
				setBL(0x02024394, (u32)dsiSaveClose);
				setBL(0x020243DC, (u32)dsiSaveOpen);
				setBL(0x02024414, (u32)dsiSaveSeek);
				setBL(0x02024424, (u32)dsiSaveClose);
				setBL(0x0202443C, (u32)dsiSaveWrite);
				setBL(0x0202444C, (u32)dsiSaveClose);
				setBL(0x0202445C, (u32)dsiSaveClose);
				setBL(0x0202449C, (u32)dsiSaveOpen);
				setBL(0x020244C8, (u32)dsiSaveGetLength);
				setBL(0x020244E8, (u32)dsiSaveSeek);
				setBL(0x020244F8, (u32)dsiSaveClose);
				setBL(0x02024510, (u32)dsiSaveRead);
				setBL(0x02024520, (u32)dsiSaveClose);
				setBL(0x02024530, (u32)dsiSaveClose);
				*(u32*)0x020D0FFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else {
				setBL(0x02024280, (u32)dsiSaveOpen);
				setBL(0x020242B8, (u32)dsiSaveCreate);
				setBL(0x020242C8, (u32)dsiSaveGetResultCode);
				setBL(0x020242F0, (u32)dsiSaveOpen);
				setBL(0x0202431C, (u32)dsiSaveGetLength);
				setBL(0x02024338, (u32)dsiSaveSetLength);
				setBL(0x02024388, (u32)dsiSaveWrite);
				setBL(0x02024398, (u32)dsiSaveClose);
				setBL(0x020243B8, (u32)dsiSaveClose);
				setBL(0x02024400, (u32)dsiSaveOpen);
				setBL(0x02024438, (u32)dsiSaveSeek);
				setBL(0x02024448, (u32)dsiSaveClose);
				setBL(0x02024460, (u32)dsiSaveWrite);
				setBL(0x02024470, (u32)dsiSaveClose);
				setBL(0x02024480, (u32)dsiSaveClose);
				setBL(0x020244C0, (u32)dsiSaveOpen);
				setBL(0x020244EC, (u32)dsiSaveGetLength);
				setBL(0x0202450C, (u32)dsiSaveSeek);
				setBL(0x0202451C, (u32)dsiSaveClose);
				setBL(0x02024524, (u32)dsiSaveRead);
				setBL(0x02024534, (u32)dsiSaveClose);
				setBL(0x02024544, (u32)dsiSaveClose);
				*(u32*)0x020D0590 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		}
	}

	// Army Defender (USA)
	// Army Defender (Europe)
	else if (strncmp(romTid, "KAY", 3) == 0 && saveOnFlashcard) {
		//*(u32*)0x020051BC = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020051C0 = 0xE12FFF1E; // bx lr
		//*(u32*)0x02005204 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02005208 = 0xE12FFF1E; // bx lr
		setBL(0x02020A28, (u32)dsiSaveCreate);
		setBL(0x02020A38, (u32)dsiSaveOpen);
		setBL(0x02020A8C, (u32)dsiSaveWrite);
		setBL(0x02020A94, (u32)dsiSaveClose);
		setBL(0x02020ADC, (u32)dsiSaveOpen);
		setBL(0x02020B08, (u32)dsiSaveGetLength);
		setBL(0x02020B18, (u32)dsiSaveRead);
		setBL(0x02020B20, (u32)dsiSaveClose);
		tonccpy((u32*)0x02043360, dsiSaveGetResultCode, 0xC);
	}

	// Art Style: AQUIA (USA)
	else if (strcmp(romTid, "KAAE") == 0 && saveOnFlashcard) {
		setBL(0x0203BBE4, (u32)dsiSaveOpen);
		setBL(0x0203BC08, (u32)dsiSaveClose);
		*(u32*)0x0203BC4C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203BC70 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203BC90, (u32)dsiSaveCreate);
		setBL(0x0203BCA0, (u32)dsiSaveOpen);
		setBL(0x0203BCC8, (u32)dsiSaveWrite);
		setBL(0x0203BCE0, (u32)dsiSaveClose);
		setBL(0x0203BD2C, (u32)dsiSaveOpen);
		setBL(0x0203BD54, (u32)dsiSaveRead);
		setBL(0x0203BD80, (u32)dsiSaveClose);
		setBL(0x0203BE70, (u32)dsiSaveOpen);
		setBL(0x0203BE98, (u32)dsiSaveWrite);
		setBL(0x0203BEB4, (u32)dsiSaveClose);
	}

	// Art Style: AQUITE (Europe, Australia)
	else if (strcmp(romTid, "KAAV") == 0 && saveOnFlashcard) {
		setBL(0x0203BCF4, (u32)dsiSaveOpen);
		setBL(0x0203BD18, (u32)dsiSaveClose);
		*(u32*)0x0203BD5C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203BD80 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203BDA0, (u32)dsiSaveCreate);
		setBL(0x0203BDB0, (u32)dsiSaveOpen);
		setBL(0x0203BDC8, (u32)dsiSaveWrite);
		setBL(0x0203BDF0, (u32)dsiSaveClose);
		setBL(0x0203BE3C, (u32)dsiSaveOpen);
		setBL(0x0203BE64, (u32)dsiSaveRead);
		setBL(0x0203BE90, (u32)dsiSaveClose);
		setBL(0x0203BF80, (u32)dsiSaveOpen);
		setBL(0x0203BFA8, (u32)dsiSaveWrite);
		setBL(0x0203BFC4, (u32)dsiSaveClose);
	}

	// Art Style: AQUARIO (Japan)
	else if (strcmp(romTid, "KAAJ") == 0 && saveOnFlashcard) {
		setBL(0x0203E2F0, (u32)dsiSaveOpen);
		setBL(0x0203E314, (u32)dsiSaveClose);
		*(u32*)0x0203E34C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203E370 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203E38C, (u32)dsiSaveCreate);
		setBL(0x0203E39C, (u32)dsiSaveOpen);
		setBL(0x0203E3C8, (u32)dsiSaveWrite);
		setBL(0x0203E3E4, (u32)dsiSaveClose);
		setBL(0x0203E42C, (u32)dsiSaveOpen);
		setBL(0x0203E458, (u32)dsiSaveRead);
		setBL(0x0203E488, (u32)dsiSaveClose);
		setBL(0x0203E574, (u32)dsiSaveOpen);
		setBL(0x0203E5A0, (u32)dsiSaveWrite);
		setBL(0x0203E5BC, (u32)dsiSaveClose);
	}

	// Everyday Soccer (USA)
	else if (strcmp(romTid, "KAZE") == 0 && saveOnFlashcard) {
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		setBL(0x02059E20, (u32)dsiSaveCreate);
		setBL(0x02059E3C, (u32)dsiSaveOpen);
		setBL(0x02059E68, (u32)dsiSaveSetLength);
		setBL(0x02059E84, (u32)dsiSaveWrite);
		setBL(0x02059E90, (u32)dsiSaveClose);
		setBL(0x02059F2C, (u32)dsiSaveOpen);
		setBL(0x02059F9C, (u32)dsiSaveRead);
		setBL(0x02059FA8, (u32)dsiSaveClose);
	}

	// ARC Style: Everyday Football (Europe, Australia)
	else if (strcmp(romTid, "KAZV") == 0 && saveOnFlashcard) {
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		setBL(0x02059EF4, (u32)dsiSaveCreate);
		setBL(0x02059F10, (u32)dsiSaveOpen);
		setBL(0x02059F3C, (u32)dsiSaveSetLength);
		setBL(0x02059F58, (u32)dsiSaveWrite);
		setBL(0x02059F64, (u32)dsiSaveClose);
		setBL(0x0205A000, (u32)dsiSaveOpen);
		setBL(0x0205A070, (u32)dsiSaveRead);
		setBL(0x0205A07C, (u32)dsiSaveClose);
	}

	// ARC Style: Soccer! (Japan)
	// ARC Style: Soccer! (Korea)
	else if ((strcmp(romTid, "KAZJ") == 0 || strcmp(romTid, "KAZK") == 0) && saveOnFlashcard) {
		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		setBL(0x02059E04, (u32)dsiSaveCreate);
		setBL(0x02059E20, (u32)dsiSaveOpen);
		setBL(0x02059E4C, (u32)dsiSaveSetLength);
		setBL(0x02059E68, (u32)dsiSaveWrite);
		setBL(0x02059E74, (u32)dsiSaveClose);
		setBL(0x02059F04, (u32)dsiSaveOpen);
		setBL(0x02059F74, (u32)dsiSaveRead);
		setBL(0x02059F80, (u32)dsiSaveClose);
	}

	// Aura-Aura Climber (USA)
	// Save code too advanced to patch, preventing support
	else if (strcmp(romTid, "KSRE") == 0 && saveOnFlashcard) {
		*(u32*)0x02026760 = 0xE12FFF1E; // bx lr
		/* *(u32*)0x02026788 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x020267D4, (u32)dsiSaveOpen);
		setBL(0x020267E8, (u32)dsiSaveCreate);
		setBL(0x02026814, (u32)dsiSaveOpen);
		//*(u32*)0x02026834 = 0xE3A01B0B; // mov r1, #0x2C00
		setBL(0x0202683C, (u32)dsiSaveSetLength);
		setBL(0x0202684C, (u32)dsiSaveClose);
		setBL(0x02026870, (u32)dsiSaveWrite);
		setBL(0x0202687C, (u32)dsiSaveGetLength);
		*(u32*)0x02026880 = 0xE1A02000; // mov r2, r0
		*(u32*)0x02026884 = 0xE3A01000; // mov r1, #0
		//*(u32*)0x02026888 = 0xE3A03000; // mov r3, #0
		setBL(0x020268B8, (u32)dsiSaveSeek);
		setBL(0x020268D4, (u32)dsiSaveRead);
		setBL(0x02026BDC, (u32)dsiSaveSeek);
		setBL(0x02026C00, (u32)dsiSaveRead);
		setBL(0x02026CC0, (u32)dsiSaveSeek);
		setBL(0x02026CDC, (u32)dsiSaveRead);
		setBL(0x02026F6C, (u32)dsiSaveSeek);
		setBL(0x02026F84, (u32)dsiSaveWrite);
		setBL(0x020271E4, (u32)dsiSaveSeek);
		setBL(0x020271FC, (u32)dsiSaveWrite);
		setBL(0x0202723C, (u32)dsiSaveSeek);
		setBL(0x02027258, (u32)dsiSaveWrite);
		setBL(0x020273AC, (u32)dsiSaveSeek);
		setBL(0x020273C4, (u32)dsiSaveRead);
		setBL(0x020275A4, (u32)dsiSaveSeek);
		setBL(0x020275BC, (u32)dsiSaveWrite); */
	}

	// Aura-Aura Climber (Europe, Australia)
	else if (strcmp(romTid, "KSRV") == 0 && saveOnFlashcard) {
		*(u32*)0x020265A8 = 0xE12FFF1E; // bx lr
	}

	// Beauty Academy (Europe)
	else if (strcmp(romTid, "K8BP") == 0 && saveOnFlashcard) {
		setBL(0x02092D44, (u32)dsiSaveCreate);
		setBL(0x02092D54, (u32)dsiSaveOpen);
		setBL(0x02092D64, (u32)dsiSaveGetResultCode);
		setBL(0x02092DA0, (u32)dsiSaveSetLength);
		setBL(0x02092DB0, (u32)dsiSaveWrite);
		setBL(0x02092DB8, (u32)dsiSaveClose);
		setBL(0x02092DF4, (u32)dsiSaveOpen);
		setBL(0x02092E04, (u32)dsiSaveGetResultCode);
		setBL(0x02092E1C, (u32)dsiSaveGetLength);
		setBL(0x02092E2C, (u32)dsiSaveRead);
		setBL(0x02092E34, (u32)dsiSaveClose);
		setBL(0x02092E6C, (u32)dsiSaveOpen);
		setBL(0x02092E7C, (u32)dsiSaveGetResultCode);
		setBL(0x02092E94, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x02092FDC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02093070 = 0xE1A00000; // nop
		*(u32*)0x02093078 = 0xE1A00000; // nop
		*(u32*)0x02093084 = 0xE1A00000; // nop
	}

	// Bejeweled Twist (USA)
	else if (strcmp(romTid, "KBEE") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x02095E90;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x02095EA0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02095EB0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02095EC0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02095ED0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveGetResultCodeT = 0x02095794;
		*(u16*)dsiSaveGetResultCodeT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetResultCodeT + 4), dsiSaveGetResultCode, 0xC);

		const u32 dsiSaveSetLengthT = 0x02096254;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveWriteT = 0x02096444;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		doubleNopT(0x020368DE); // dsiSaveCreateDirAuto
		setBLThumb(0x020368E6, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x020368F0, dsiSaveOpenT);
		//setBLThumb(0x020368FA, dsiSaveGetResultCodeT);
		doubleNopT(0x0203691C); // dsiSaveCreateDirAuto
		setBLThumb(0x02036924, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x0203692E, dsiSaveOpenT);
		//setBLThumb(0x02036938, dsiSaveGetResultCodeT);
		//setBLThumb(0x0203696E, dsiSaveSetLengthT);
		setBLThumb(0x02036978, dsiSaveSeekT);
		//setBLThumb(0x02036982, dsiSaveWriteT);
		setBLThumb(0x02036988, dsiSaveCloseT);
		setBLThumb(0x020369B6, dsiSaveOpenT);
		setBLThumb(0x020369D0, dsiSaveSeekT);
		setBLThumb(0x020369DA, dsiSaveReadT);
		setBLThumb(0x020369E0, dsiSaveCloseT);
	}

	// Bejeweled Twist (Europe, Australia)
	else if (strcmp(romTid, "KBEV") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x02094A78;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x02094A88;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02094A98;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02094AA8;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02094AB8;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveGetResultCodeT = 0x020943F0;
		*(u16*)dsiSaveGetResultCodeT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetResultCodeT + 4), dsiSaveGetResultCode, 0xC);

		const u32 dsiSaveSetLengthT = 0x02094E3C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveWriteT = 0x02094FF4;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		doubleNopT(0x0203601E); // dsiSaveCreateDirAuto
		setBLThumb(0x02036026, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x02036030, dsiSaveOpenT);
		//setBLThumb(0x0203603A, dsiSaveGetResultCodeT);
		doubleNopT(0x0203605C); // dsiSaveCreateDirAuto
		setBLThumb(0x02036064, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x0203606E, dsiSaveOpenT);
		//setBLThumb(0x02036078, dsiSaveGetResultCodeT);
		//setBLThumb(0x020360AE, dsiSaveSetLengthT);
		setBLThumb(0x020360B8, dsiSaveSeekT);
		//setBLThumb(0x020360C2, dsiSaveWriteT);
		setBLThumb(0x020360C8, dsiSaveCloseT);
		setBLThumb(0x020360F6, dsiSaveOpenT);
		setBLThumb(0x02036110, dsiSaveSeekT);
		setBLThumb(0x0203611A, dsiSaveReadT);
		setBLThumb(0x02036120, dsiSaveCloseT);
	}

	// Bomberman Blitz (USA)
	else if (strcmp(romTid, "KBBE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02009670, dsiSaveGetResultCode, 0xC);
		//*(u32*)0x020437AC = 0xE3A00001; // mov r0, #1
		//*(u32*)0x020437B0 = 0xE12FFF1E; // bx lr
		setBL(0x02043950, (u32)dsiSaveOpen);
		setBL(0x020439D0, (u32)dsiSaveCreate);
		setBL(0x02043A5C, (u32)dsiSaveWrite);
		setBL(0x02043A70, (u32)dsiSaveClose);
		setBL(0x02043AE8, (u32)dsiSaveClose);
		setBL(0x02046394, (u32)dsiSaveOpen);
		setBL(0x02046428, (u32)dsiSaveRead);
		setBL(0x0204649C, (u32)dsiSaveClose);
	}

	// Bomberman Blitz (Europe, Australia)
	else if (strcmp(romTid, "KBBV") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02009670, dsiSaveGetResultCode, 0xC);
		//*(u32*)0x02043878 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x0204387C = 0xE12FFF1E; // bx lr
		setBL(0x02043A1C, (u32)dsiSaveOpen);
		setBL(0x02043A9C, (u32)dsiSaveCreate);
		setBL(0x02043B28, (u32)dsiSaveWrite);
		setBL(0x02043B28, (u32)dsiSaveClose);
		setBL(0x02043BB4, (u32)dsiSaveClose);
		setBL(0x02046460, (u32)dsiSaveOpen);
		setBL(0x020464F4, (u32)dsiSaveRead);
		setBL(0x02046568, (u32)dsiSaveClose);
	}

	// Itsudemo Bomberman (Japan)
	else if (strcmp(romTid, "KBBJ") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02009670, dsiSaveGetResultCode, 0xC);
		//*(u32*)0x020434D8 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x020434DC = 0xE12FFF1E; // bx lr
		setBL(0x0204367C, (u32)dsiSaveOpen);
		setBL(0x020436FC, (u32)dsiSaveCreate);
		setBL(0x02043788, (u32)dsiSaveWrite);
		setBL(0x0204379C, (u32)dsiSaveClose);
		setBL(0x02043814, (u32)dsiSaveClose);
		setBL(0x020460C0, (u32)dsiSaveOpen);
		setBL(0x02046154, (u32)dsiSaveRead);
		setBL(0x020461C8, (u32)dsiSaveClose);
	}

	// Bookworm (USA)
	/*else if (strcmp(romTid, "KBKE") == 0 && saveOnFlashcard) {
		setBL(0x0204E744, (u32)dsiSaveClose);
		setBL(0x0204EA68, (u32)dsiSaveSeek);
		setBL(0x0204EAA8, (u32)dsiSaveWrite);
		setBL(0x0204EAF0, (u32)dsiSaveSeek);
		setBL(0x0204EB18, (u32)dsiSaveWrite);
		*(u32*)0x0204EB48 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0204EB90, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0204EBA8, (u32)dsiSaveOpen);
		setBL(0x0204EBD8, (u32)dsiSaveSetLength);
		setBL(0x0204EC8C, (u32)dsiSaveWrite);
		*(u32*)0x0204ECC4 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0204ECD0, (u32)dsiSaveGetResultCode);
		setBL(0x0204ED04, (u32)dsiSaveOpen);
		setBL(0x0204ED30, (u32)dsiSaveGetLength);
		setBL(0x0204ED88, (u32)dsiSaveRead);
	}*/

	// Art Style: BOXLIFE (USA)
	else if (strcmp(romTid, "KAHE") == 0 && saveOnFlashcard) {
		setBL(0x020353B4, (u32)dsiSaveOpen);
		//*(u32*)0x020355D8 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020355DC = 0xE12FFF1E; // bx lr
		setBL(0x02035608, (u32)dsiSaveOpen);
		setBL(0x02035658, (u32)dsiSaveRead);
		setBL(0x0203569C, (u32)dsiSaveClose);
		//*(u32*)0x020356C4 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020356C8 = 0xE12FFF1E; // bx lr
		setBL(0x020356E8, (u32)dsiSaveCreate);
		setBL(0x020356F8, (u32)dsiSaveGetResultCode);
		setBL(0x0203571C, (u32)dsiSaveOpen);
		setBL(0x02035738, (u32)dsiSaveSetLength);
		setBL(0x02035754, (u32)dsiSaveWrite);
		setBL(0x02035770, (u32)dsiSaveClose);
	}

	// Art Style: BOXLIFE (Europe, Australia)
	else if (strcmp(romTid, "KAHV") == 0 && saveOnFlashcard) {
		setBL(0x02034FFC, (u32)dsiSaveOpen);
		//*(u32*)0x02035220 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02035224 = 0xE12FFF1E; // bx lr
		setBL(0x02035250, (u32)dsiSaveOpen);
		setBL(0x020352A0, (u32)dsiSaveRead);
		setBL(0x020352E4, (u32)dsiSaveClose);
		//*(u32*)0x0203530C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02035310 = 0xE12FFF1E; // bx lr
		setBL(0x02035330, (u32)dsiSaveCreate);
		setBL(0x02035340, (u32)dsiSaveGetResultCode);
		setBL(0x02035364, (u32)dsiSaveOpen);
		setBL(0x02035380, (u32)dsiSaveSetLength);
		setBL(0x0203539C, (u32)dsiSaveWrite);
		setBL(0x020353B8, (u32)dsiSaveClose);
	}

	// Art Style: Hacolife (Japan)
	else if (strcmp(romTid, "KAHJ") == 0 && saveOnFlashcard) {
		setBL(0x02034348, (u32)dsiSaveOpen);
		//*(u32*)0x0203456C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02034570 = 0xE12FFF1E; // bx lr
		setBL(0x0203459C, (u32)dsiSaveOpen);
		setBL(0x020345EC, (u32)dsiSaveRead);
		setBL(0x02034630, (u32)dsiSaveClose);
		//*(u32*)0x02034658 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203465C = 0xE12FFF1E; // bx lr
		setBL(0x0203467C, (u32)dsiSaveCreate);
		setBL(0x0203468C, (u32)dsiSaveGetResultCode);
		setBL(0x020346B0, (u32)dsiSaveOpen);
		setBL(0x020346CC, (u32)dsiSaveSetLength);
		setBL(0x020346E8, (u32)dsiSaveWrite);
		setBL(0x02034704, (u32)dsiSaveClose);
	}

	// Bugs'N'Balls (USA)
	// Bugs'N'Balls (Europe)
	else if (strncmp(romTid, "KKQ", 3) == 0 && saveOnFlashcard) {
		u32* saveFuncOffsets[22] = {NULL};

		tonccpy((u32*)0x0201BEB8, dsiSaveGetResultCode, 0xC);
		if (ndsHeader->gameCode[3] == 'E') {
			saveFuncOffsets[0] = (u32*)0x0205B148;
			saveFuncOffsets[1] = (u32*)0x0205B160;
			saveFuncOffsets[2] = (u32*)0x0205B174;
			saveFuncOffsets[3] = (u32*)0x0205B18C;
			saveFuncOffsets[4] = (u32*)0x0205B1A0;
			saveFuncOffsets[5] = (u32*)0x0205B1B8;
			saveFuncOffsets[6] = (u32*)0x0205B1CC;
			saveFuncOffsets[7] = (u32*)0x0205B1DC;
			saveFuncOffsets[8] = (u32*)0x0205B24C;
			saveFuncOffsets[9] = (u32*)0x0205B264;
			saveFuncOffsets[10] = (u32*)0x0205B278;
			saveFuncOffsets[11] = (u32*)0x0205B290;
			saveFuncOffsets[12] = (u32*)0x0205B2A4;
			saveFuncOffsets[13] = (u32*)0x0205B2BC;
			saveFuncOffsets[14] = (u32*)0x0205B2D0;
			saveFuncOffsets[15] = (u32*)0x0205B2E0;
			saveFuncOffsets[16] = (u32*)0x0205B350;
			saveFuncOffsets[17] = (u32*)0x0205B384;
			saveFuncOffsets[18] = (u32*)0x0205B3A4;
			saveFuncOffsets[19] = (u32*)0x0205B3AC;
			saveFuncOffsets[20] = (u32*)0x0205B40C;
			saveFuncOffsets[21] = (u32*)0x0205B424;
		} else if (ndsHeader->gameCode[3] == 'P') {
			saveFuncOffsets[0] = (u32*)0x0205C6F8;
			saveFuncOffsets[1] = (u32*)0x0205C710;
			saveFuncOffsets[2] = (u32*)0x0205C724;
			saveFuncOffsets[3] = (u32*)0x0205C73C;
			saveFuncOffsets[4] = (u32*)0x0205C750;
			saveFuncOffsets[5] = (u32*)0x0205C768;
			saveFuncOffsets[6] = (u32*)0x0205C77C;
			saveFuncOffsets[7] = (u32*)0x0205C78C;
			saveFuncOffsets[8] = (u32*)0x0205C7FC;
			saveFuncOffsets[9] = (u32*)0x0205C814;
			saveFuncOffsets[10] = (u32*)0x0205C828;
			saveFuncOffsets[11] = (u32*)0x0205C840;
			saveFuncOffsets[12] = (u32*)0x0205C854;
			saveFuncOffsets[13] = (u32*)0x0205C86C;
			saveFuncOffsets[14] = (u32*)0x0205C880;
			saveFuncOffsets[15] = (u32*)0x0205C890;
			saveFuncOffsets[16] = (u32*)0x0205C900;
			saveFuncOffsets[17] = (u32*)0x0205C934;
			saveFuncOffsets[18] = (u32*)0x0205C954;
			saveFuncOffsets[19] = (u32*)0x0205C95C;
			saveFuncOffsets[20] = (u32*)0x0205C9BC;
			saveFuncOffsets[21] = (u32*)0x0205C9D4; 
		}

		setBL((u32)saveFuncOffsets[0], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[1], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[2], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[3], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[4], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[5], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[6], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[7], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[8], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[9], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[10], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[11], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[12], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[13], (u32)dsiSaveRead);
		setBL((u32)saveFuncOffsets[14], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[15], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[16], (u32)dsiSaveCreate);
		setBL((u32)saveFuncOffsets[17], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[18], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[19], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[20], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[21], (u32)dsiSaveClose);
	}

	// Cake Ninja (USA)
	else if (strcmp(romTid, "K2JE") == 0 && saveOnFlashcard) {
		//*(u32*)0x02008918 = 0xE12FFF1E; // bx lr (NO$GBA fix)
		setBL(0x0202CDF0, (u32)dsiSaveOpen);
		setBL(0x0202CE48, (u32)dsiSaveCreate);
		setBL(0x0202CE7C, (u32)dsiSaveOpen);
		setBL(0x0202CE90, (u32)dsiSaveSetLength);
		setBL(0x0202CEA0, (u32)dsiSaveGetLength);
		setBL(0x0202CEA8, (u32)dsiSaveClose);
		setBL(0x0202CEE0, (u32)dsiSaveSetLength);
		setBL(0x0202CEF0, (u32)dsiSaveGetLength);
		setBL(0x0202CEF8, (u32)dsiSaveClose);
		setBL(0x0202D100, (u32)dsiSaveOpen);
		setBL(0x0202D128, (u32)dsiSaveSeek);
		setBL(0x0202D13C, (u32)dsiSaveRead);
		setBL(0x0202D154, (u32)dsiSaveClose);
		setBL(0x0202D21C, (u32)dsiSaveOpen);
		setBL(0x0202D244, (u32)dsiSaveSeek);
		setBL(0x0202D258, (u32)dsiSaveWrite);
		setBL(0x0202D264, (u32)dsiSaveClose);
		tonccpy((u32*)0x020584CC, dsiSaveGetResultCode, 0xC);
	}

	// Cake Ninja (Europe)
	else if (strcmp(romTid, "K2JP") == 0 && saveOnFlashcard) {
		setBL(0x0202CEC8, (u32)dsiSaveOpen);
		setBL(0x0202CF20, (u32)dsiSaveCreate);
		setBL(0x0202CF54, (u32)dsiSaveOpen);
		setBL(0x0202CF68, (u32)dsiSaveSetLength);
		setBL(0x0202CF78, (u32)dsiSaveGetLength);
		setBL(0x0202CF80, (u32)dsiSaveClose);
		setBL(0x0202CFB8, (u32)dsiSaveSetLength);
		setBL(0x0202CFC8, (u32)dsiSaveGetLength);
		setBL(0x0202CFD0, (u32)dsiSaveClose);
		setBL(0x0202D1D8, (u32)dsiSaveOpen);
		setBL(0x0202D200, (u32)dsiSaveSeek);
		setBL(0x0202D214, (u32)dsiSaveRead);
		setBL(0x0202D22C, (u32)dsiSaveClose);
		setBL(0x0202D2F4, (u32)dsiSaveOpen);
		setBL(0x0202D31C, (u32)dsiSaveSeek);
		setBL(0x0202D330, (u32)dsiSaveWrite);
		setBL(0x0202D33C, (u32)dsiSaveClose);
		tonccpy((u32*)0x020585A4, dsiSaveGetResultCode, 0xC);
	}

	// Cake Ninja 2 (USA)
	else if (strcmp(romTid, "K2NE") == 0 && saveOnFlashcard) {
		setBL(0x0204C918, (u32)dsiSaveOpen);
		setBL(0x0204C970, (u32)dsiSaveCreate);
		setBL(0x0204C9A4, (u32)dsiSaveOpen);
		setBL(0x0204C9B8, (u32)dsiSaveSetLength);
		setBL(0x0204C9C8, (u32)dsiSaveGetLength);
		setBL(0x0204C9D0, (u32)dsiSaveClose);
		setBL(0x0204CA08, (u32)dsiSaveSetLength);
		setBL(0x0204CA18, (u32)dsiSaveGetLength);
		setBL(0x0204CA20, (u32)dsiSaveClose);
		setBL(0x0204CC28, (u32)dsiSaveOpen);
		setBL(0x0204CC50, (u32)dsiSaveSeek);
		setBL(0x0204CC64, (u32)dsiSaveRead);
		setBL(0x0204CC7C, (u32)dsiSaveClose);
		setBL(0x0204CD44, (u32)dsiSaveOpen);
		setBL(0x0204CD6C, (u32)dsiSaveSeek);
		setBL(0x0204CD80, (u32)dsiSaveWrite);
		setBL(0x0204CD8C, (u32)dsiSaveClose);
		tonccpy((u32*)0x02078040, dsiSaveGetResultCode, 0xC);
	}

	// Cake Ninja 2 (Europe)
	else if (strcmp(romTid, "K2NP") == 0 && saveOnFlashcard) {
		setBL(0x0204C974, (u32)dsiSaveOpen);
		setBL(0x0204C9CC, (u32)dsiSaveCreate);
		setBL(0x0204CA00, (u32)dsiSaveOpen);
		setBL(0x0204CA14, (u32)dsiSaveSetLength);
		setBL(0x0204CA24, (u32)dsiSaveGetLength);
		setBL(0x0204CA2C, (u32)dsiSaveClose);
		setBL(0x0204CA64, (u32)dsiSaveSetLength);
		setBL(0x0204CA74, (u32)dsiSaveGetLength);
		setBL(0x0204CA7C, (u32)dsiSaveClose);
		setBL(0x0204CC84, (u32)dsiSaveOpen);
		setBL(0x0204CCAC, (u32)dsiSaveSeek);
		setBL(0x0204CCC0, (u32)dsiSaveRead);
		setBL(0x0204CCD8, (u32)dsiSaveClose);
		setBL(0x0204CDA0, (u32)dsiSaveOpen);
		setBL(0x0204CDC8, (u32)dsiSaveSeek);
		setBL(0x0204CDDC, (u32)dsiSaveWrite);
		setBL(0x0204CDE8, (u32)dsiSaveClose);
		tonccpy((u32*)0x0207809C, dsiSaveGetResultCode, 0xC);
	}

	// Cake Ninja: XMAS (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYNE") == 0 && saveOnFlashcard) {
		setBL(0x0202571C, (u32)dsiSaveOpen);
		setBL(0x02025774, (u32)dsiSaveCreate);
		setBL(0x020257A8, (u32)dsiSaveOpen);
		setBL(0x020257BC, (u32)dsiSaveSetLength);
		setBL(0x020257CC, (u32)dsiSaveGetLength);
		setBL(0x020257D4, (u32)dsiSaveClose);
		setBL(0x0202580C, (u32)dsiSaveSetLength);
		setBL(0x0202581C, (u32)dsiSaveGetLength);
		setBL(0x02025824, (u32)dsiSaveClose);
		setBL(0x02025A2C, (u32)dsiSaveOpen);
		setBL(0x02025A54, (u32)dsiSaveSeek);
		setBL(0x02025A68, (u32)dsiSaveRead);
		setBL(0x02025A80, (u32)dsiSaveClose);
		setBL(0x02025B48, (u32)dsiSaveOpen);
		setBL(0x02025B70, (u32)dsiSaveSeek);
		setBL(0x02025B84, (u32)dsiSaveWrite);
		setBL(0x02025B90, (u32)dsiSaveClose);
		tonccpy((u32*)0x02050EDC, dsiSaveGetResultCode, 0xC);
	}

	// Cake Ninja: XMAS (Europe)
	else if (strcmp(romTid, "KYNP") == 0 && saveOnFlashcard) {
		setBL(0x020257A8, (u32)dsiSaveOpen);
		setBL(0x02025800, (u32)dsiSaveCreate);
		setBL(0x02025834, (u32)dsiSaveOpen);
		setBL(0x02025848, (u32)dsiSaveSetLength);
		setBL(0x02025858, (u32)dsiSaveGetLength);
		setBL(0x02025860, (u32)dsiSaveClose);
		setBL(0x02025898, (u32)dsiSaveSetLength);
		setBL(0x020258A8, (u32)dsiSaveGetLength);
		setBL(0x020258B0, (u32)dsiSaveClose);
		setBL(0x02025AB8, (u32)dsiSaveOpen);
		setBL(0x02025AE0, (u32)dsiSaveSeek);
		setBL(0x02025AF4, (u32)dsiSaveRead);
		setBL(0x02025B0C, (u32)dsiSaveClose);
		setBL(0x02025BD4, (u32)dsiSaveOpen);
		setBL(0x02025BFC, (u32)dsiSaveSeek);
		setBL(0x02025C10, (u32)dsiSaveWrite);
		tonccpy((u32*)0x02050F68, dsiSaveGetResultCode, 0xC);
	}

	// Castle Conqueror: Heroes (USA)
	else if (strcmp(romTid, "KC5E") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201831C, dsiSaveGetResultCode, 0xC);
		setBL(0x02065CA0, (u32)dsiSaveOpen);
		setBL(0x02065CB4, (u32)dsiSaveCreate);
		setBL(0x02065CC4, (u32)dsiSaveOpen);
		setBL(0x02065CF0, (u32)dsiSaveCreate);
		setBL(0x02065D00, (u32)dsiSaveOpen);
		setBL(0x02066208, (u32)dsiSaveSeek);
		setBL(0x0206621C, (u32)dsiSaveWrite);
		setBL(0x0206622C, (u32)dsiSaveSeek);
		setBL(0x0206623C, (u32)dsiSaveWrite);
		setBL(0x0206624C, (u32)dsiSaveSeek);
		setBL(0x0206625C, (u32)dsiSaveWrite);
		setBL(0x020662C0, (u32)dsiSaveSeek);
		setBL(0x020662D4, (u32)dsiSaveWrite);
		setBL(0x020662DC, (u32)dsiSaveClose);
		setBL(0x02066330, (u32)dsiSaveOpen);
		setBL(0x02066648, (u32)dsiSaveSeek);
		setBL(0x02066658, (u32)dsiSaveRead);
		setBL(0x02066684, (u32)dsiSaveClose);
		setBL(0x02066BE4, (u32)dsiSaveOpen);
		setBL(0x02066BF8, (u32)dsiSaveSeek);
		setBL(0x02066C08, (u32)dsiSaveWrite);
		setBL(0x02066C10, (u32)dsiSaveClose);
	}

	// Castle Conqueror: Heroes (Europe, Australia)
	else if (strcmp(romTid, "KC5V") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02018248, dsiSaveGetResultCode, 0xC);
		setBL(0x02066110, (u32)dsiSaveOpen);
		setBL(0x02066128, (u32)dsiSaveCreate);
		setBL(0x02066138, (u32)dsiSaveOpen);
		setBL(0x02066164, (u32)dsiSaveCreate);
		setBL(0x02066174, (u32)dsiSaveOpen);
		setBL(0x02066680, (u32)dsiSaveSeek);
		setBL(0x02066694, (u32)dsiSaveWrite);
		setBL(0x020666A4, (u32)dsiSaveSeek);
		setBL(0x020666B4, (u32)dsiSaveWrite);
		setBL(0x020666C4, (u32)dsiSaveSeek);
		setBL(0x020666D4, (u32)dsiSaveWrite);
		setBL(0x02066748, (u32)dsiSaveSeek);
		setBL(0x0206675C, (u32)dsiSaveWrite);
		setBL(0x02066764, (u32)dsiSaveClose);
		setBL(0x020667BC, (u32)dsiSaveOpen);
		setBL(0x0206682C, (u32)dsiSaveSeek);
		setBL(0x0206683C, (u32)dsiSaveRead);
		setBL(0x0206686C, (u32)dsiSaveClose);
		setBL(0x02066E90, (u32)dsiSaveOpen);
		setBL(0x02066EA4, (u32)dsiSaveSeek);
		setBL(0x02066EB4, (u32)dsiSaveWrite);
		setBL(0x02066EBC, (u32)dsiSaveClose);
	}

	// Castle Conqueror: Heroes (Japan)
	else if (strcmp(romTid, "KC5J") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201831C, dsiSaveGetResultCode, 0xC);
		setBL(0x02026FA8, (u32)dsiSaveOpen);
		setBL(0x02026FC0, (u32)dsiSaveCreate);
		setBL(0x02026FD0, (u32)dsiSaveOpen);
		setBL(0x02026FFC, (u32)dsiSaveCreate);
		setBL(0x0202700C, (u32)dsiSaveOpen);
		setBL(0x02027518, (u32)dsiSaveSeek);
		setBL(0x0202752C, (u32)dsiSaveWrite);
		setBL(0x0202753C, (u32)dsiSaveSeek);
		setBL(0x0202754C, (u32)dsiSaveWrite);
		setBL(0x0202755C, (u32)dsiSaveSeek);
		setBL(0x0202756C, (u32)dsiSaveWrite);
		setBL(0x020275E0, (u32)dsiSaveSeek);
		setBL(0x020275F4, (u32)dsiSaveWrite);
		setBL(0x020275FC, (u32)dsiSaveClose);
		setBL(0x02027654, (u32)dsiSaveOpen);
		setBL(0x020276C4, (u32)dsiSaveSeek);
		setBL(0x020276D4, (u32)dsiSaveRead);
		setBL(0x02027704, (u32)dsiSaveClose);
		setBL(0x02027D28, (u32)dsiSaveOpen);
		setBL(0x02027D3C, (u32)dsiSaveSeek);
		setBL(0x02027D4C, (u32)dsiSaveWrite);
		setBL(0x02027D54, (u32)dsiSaveClose);
	}

	// Cave Story (USA)
	else if (strcmp(romTid, "KCVE") == 0 && saveOnFlashcard) {
		//*(u32*)0x02005980 = 0xE12FFF1E; // bx lr
		//*(u32*)0x02005A68 = 0xE12FFF1E; // bx lr
		//*(u32*)0x02005B60 = 0xE12FFF1E; // bx lr
		setBL(0x02005994, (u32)dsiSaveCreate);
		setBL(0x020059D0, (u32)dsiSaveOpen);
		setBL(0x02005A28, (u32)dsiSaveWrite);
		setBL(0x02005A40, (u32)dsiSaveClose);
		setBL(0x02005ADC, (u32)dsiSaveOpen);
		setBL(0x02005A28, (u32)dsiSaveWrite);
		setBL(0x02005AB0, (u32)dsiSaveOpen);
		setBL(0x02005ADC, (u32)dsiSaveOpen);
		setBL(0x02005AF8, (u32)dsiSaveGetLength);
		setBL(0x02005B0C, (u32)dsiSaveClose);
		setBL(0x02005B2C, (u32)dsiSaveSeek);
		setBL(0x02005B3C, (u32)dsiSaveRead);
		setBL(0x02005B44, (u32)dsiSaveClose);
		setBL(0x02005BAC, (u32)dsiSaveOpen);
		setBL(0x02005BD8, (u32)dsiSaveOpen);
		setBL(0x02005BF4, (u32)dsiSaveGetLength);
		setBL(0x02005C08, (u32)dsiSaveClose);
		setBL(0x02005C28, (u32)dsiSaveSeek);
		setBL(0x02005C38, (u32)dsiSaveWrite);
		setBL(0x02005C40, (u32)dsiSaveClose);
		//if (!dsiSD) {
			*(u32*)0x0200A12C = 0xE1A00000; // nop (Skip Manual screen)
		//}
		tonccpy((u32*)0x02073FA4, dsiSaveGetResultCode, 0xC);
	}

	// Chuck E. Cheese's Alien Defense Force (USA)
	else if (strcmp(romTid, "KUQE") == 0 && saveOnFlashcard) {
		setBL(0x0201BBA4, (u32)dsiSaveCreate);
		setBL(0x0201BBB4, (u32)dsiSaveOpen);
		setBL(0x0201BBD0, (u32)dsiSaveSeek);
		setBL(0x0201BBE0, (u32)dsiSaveWrite);
		setBL(0x0201BBE8, (u32)dsiSaveClose);
		setBL(0x0201BD10, (u32)dsiSaveOpenR);
		setBL(0x0201BD28, (u32)dsiSaveSeek);
		setBL(0x0201BD38, (u32)dsiSaveRead);
		setBL(0x0201BD40, (u32)dsiSaveClose);

		//if (!dsiSD) {
			*(u32*)0x0201B9E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 4; i++) {
				u32* offset = (u32*)0x0202D43C;
				offset[i] = 0xE1A00000; // nop
			}
		//}
	}

	// Chuck E. Cheese's Arcade Room (USA)
	else if (strcmp(romTid, "KUCE") == 0 && saveOnFlashcard) {
		//if (!dsiSD) {
			*(u32*)0x02032550 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x020459F0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		//}
		setBL(0x02045BAC, (u32)dsiSaveCreate);
		setBL(0x02045BBC, (u32)dsiSaveOpen);
		setBL(0x02045BD8, (u32)dsiSaveSeek);
		setBL(0x02045BE8, (u32)dsiSaveWrite);
		setBL(0x02045BF0, (u32)dsiSaveClose);
		setBL(0x02045D1C, (u32)dsiSaveOpenR);
		setBL(0x02045D34, (u32)dsiSaveSeek);
		setBL(0x02045D44, (u32)dsiSaveRead);
		setBL(0x02045D4C, (u32)dsiSaveClose);
	}

	// Chuukara! Dairoujou (Japan)
	else if (strcmp(romTid, "KQLJ") == 0 && saveOnFlashcard) {
		setBL(0x020446E4, (u32)dsiSaveOpen);
		setBL(0x02044710, (u32)dsiSaveRead);
		setBL(0x02044720, (u32)dsiSaveClose);
		setBL(0x0204473C, (u32)dsiSaveClose);
		*(u32*)0x02044790 = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x020447CC = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x020447D8, (u32)dsiSaveCreate);
		setBL(0x020447E8, (u32)dsiSaveOpen);
		setBL(0x02044814, (u32)dsiSaveSetLength);
		setBL(0x02044824, (u32)dsiSaveClose);
		setBL(0x02044848, (u32)dsiSaveWrite);
		setBL(0x02044858, (u32)dsiSaveClose);
		setBL(0x02044874, (u32)dsiSaveClose);
	}

	// Crash-Course Domo (USA)
	else if (strcmp(romTid, "KDCE") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x02024B0C;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02024B1C;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02024B2C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02024B3C;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02024B4C;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02024B5C;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02024B6C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0200DF38 = 0x2001; // movs r0, #1
		*(u16*)0x0200DF3A = 0x4770; // bx lr
		//doubleNopT(0x0200DF8A); // dsiSaveGetArcSrc
		*(u16*)0x0200E228 = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0200E22A = 0x4770; // bx lr
		setBLThumb(0x0200E28E, dsiSaveCreateT);
		setBLThumb(0x0200E2A4, dsiSaveOpenT);
		setBLThumb(0x0200E2C0, dsiSaveSetLengthT);
		setBLThumb(0x0200E2D4, dsiSaveWriteT);
		setBLThumb(0x0200E2E6, dsiSaveCloseT);
		*(u16*)0x0200E30C = 0x4778; // bx pc
		tonccpy((u32*)0x0200E310, dsiSaveGetLength, 0xC);
		setBLThumb(0x0200E33C, dsiSaveOpenT);
		setBLThumb(0x0200E362, dsiSaveCloseT);
		setBLThumb(0x0200E374, dsiSaveReadT);
		setBLThumb(0x0200E37A, dsiSaveCloseT);
		setBLThumb(0x0200E38E, dsiSaveDeleteT);
		*(u16*)0x020153C4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// CuteWitch! runner (USA)
	// CuteWitch! runner (Europe)
	else if (strncmp(romTid, "K32", 3) == 0 && saveOnFlashcard) {
		u32* saveFuncOffsets[22] = {NULL};

		tonccpy((u32*)0x0201C450, dsiSaveGetResultCode, 0xC);
		if (ndsHeader->gameCode[3] == 'E') {
			saveFuncOffsets[0] = (u32*)0x02062EA4;
			saveFuncOffsets[1] = (u32*)0x02062EBC;
			saveFuncOffsets[2] = (u32*)0x02062ED0;
			saveFuncOffsets[3] = (u32*)0x02062EE8;
			saveFuncOffsets[4] = (u32*)0x02062EFC;
			saveFuncOffsets[5] = (u32*)0x02062F14;
			saveFuncOffsets[6] = (u32*)0x02062F28;
			saveFuncOffsets[7] = (u32*)0x02062F38;
			saveFuncOffsets[8] = (u32*)0x02062FA8;
			saveFuncOffsets[9] = (u32*)0x02062FC0;
			saveFuncOffsets[10] = (u32*)0x02062FD4;
			saveFuncOffsets[11] = (u32*)0x02062FEC;
			saveFuncOffsets[12] = (u32*)0x02063000;
			saveFuncOffsets[13] = (u32*)0x02063018;
			saveFuncOffsets[14] = (u32*)0x0206302C;
			saveFuncOffsets[15] = (u32*)0x0206303C;
			saveFuncOffsets[16] = (u32*)0x020630AC;
			saveFuncOffsets[17] = (u32*)0x020630E0;
			saveFuncOffsets[18] = (u32*)0x02063100;
			saveFuncOffsets[19] = (u32*)0x02063108;
			saveFuncOffsets[20] = (u32*)0x02063168;
			saveFuncOffsets[21] = (u32*)0x02063180;
		} else if (ndsHeader->gameCode[3] == 'P') {
			saveFuncOffsets[0] = (u32*)0x020948E0;
			saveFuncOffsets[1] = (u32*)0x020948F8;
			saveFuncOffsets[2] = (u32*)0x0209490C;
			saveFuncOffsets[3] = (u32*)0x02094924;
			saveFuncOffsets[4] = (u32*)0x02094938;
			saveFuncOffsets[5] = (u32*)0x02094950;
			saveFuncOffsets[6] = (u32*)0x02094964;
			saveFuncOffsets[7] = (u32*)0x02094974;
			saveFuncOffsets[8] = (u32*)0x020949E4;
			saveFuncOffsets[9] = (u32*)0x020949FC;
			saveFuncOffsets[10] = (u32*)0x02094A10;
			saveFuncOffsets[11] = (u32*)0x02094A28;
			saveFuncOffsets[12] = (u32*)0x02094A3C;
			saveFuncOffsets[13] = (u32*)0x02094A54;
			saveFuncOffsets[14] = (u32*)0x02094A68;
			saveFuncOffsets[15] = (u32*)0x02094A78;
			saveFuncOffsets[16] = (u32*)0x02094AE8;
			saveFuncOffsets[17] = (u32*)0x02094B1C;
			saveFuncOffsets[18] = (u32*)0x02094B3C;
			saveFuncOffsets[19] = (u32*)0x02094B44;
			saveFuncOffsets[20] = (u32*)0x02094BA4;
			saveFuncOffsets[21] = (u32*)0x02094BBC;
		}

		setBL((u32)saveFuncOffsets[0], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[1], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[2], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[3], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[4], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[5], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[6], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[7], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[8], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[9], (u32)dsiSaveGetLength);
		setBL((u32)saveFuncOffsets[10], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[11], (u32)dsiSaveSeek);
		setBL((u32)saveFuncOffsets[12], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[13], (u32)dsiSaveRead);
		setBL((u32)saveFuncOffsets[14], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[15], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[16], (u32)dsiSaveCreate);
		setBL((u32)saveFuncOffsets[17], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[18], (u32)dsiSaveWrite);
		setBL((u32)saveFuncOffsets[19], (u32)dsiSaveClose);
		setBL((u32)saveFuncOffsets[20], (u32)dsiSaveOpen);
		setBL((u32)saveFuncOffsets[21], (u32)dsiSaveClose);
	}

	// Dairojo! Samurai Defenders (USA)
	else if (strcmp(romTid, "KF3E") == 0 && saveOnFlashcard) {
		setBL(0x02044B3C, (u32)dsiSaveOpen);
		setBL(0x02044B68, (u32)dsiSaveRead);
		setBL(0x02044B78, (u32)dsiSaveClose);
		setBL(0x02044B94, (u32)dsiSaveClose);
		*(u32*)0x02044BE8 = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x02044C24 = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x02044C30, (u32)dsiSaveCreate);
		setBL(0x02044C40, (u32)dsiSaveOpen);
		setBL(0x02044C6C, (u32)dsiSaveSetLength);
		setBL(0x02044C7C, (u32)dsiSaveClose);
		setBL(0x02044CA0, (u32)dsiSaveWrite);
		setBL(0x02044CB0, (u32)dsiSaveClose);
		setBL(0x02044CCC, (u32)dsiSaveClose);
	}

	// Karakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF3J") == 0 && saveOnFlashcard) {
		setBL(0x02044680, (u32)dsiSaveOpen);
		setBL(0x020446AC, (u32)dsiSaveRead);
		setBL(0x020446BC, (u32)dsiSaveClose);
		setBL(0x020446D8, (u32)dsiSaveClose);
		*(u32*)0x0204472C = 0xE3A00001; // mov r0, #1 (OpenDirectory)
		*(u32*)0x02044768 = 0xE1A00000; // nop (CloseDirectory)
		setBL(0x02044774, (u32)dsiSaveCreate);
		setBL(0x02044784, (u32)dsiSaveOpen);
		setBL(0x020447B0, (u32)dsiSaveSetLength);
		setBL(0x020447C0, (u32)dsiSaveClose);
		setBL(0x020447E4, (u32)dsiSaveWrite);
		setBL(0x020447F4, (u32)dsiSaveClose);
		setBL(0x02044810, (u32)dsiSaveClose);
	}

	// Dark Void Zero (USA)
	// Dark Void Zero (Europe, Australia)
	else if ((strcmp(romTid, "KDVE") == 0 || strcmp(romTid, "KDVV") == 0) && saveOnFlashcard) {
		tonccpy((u32*)0x02043DDC, dsiSaveGetResultCode, 0xC);
		setBL(0x0208AE90, (u32)dsiSaveOpen);
		setBL(0x0208AEA4, (u32)dsiSaveCreate);
		setBL(0x0208AEC4, (u32)dsiSaveOpen);
		setBL(0x0208AEE8, (u32)dsiSaveWrite);
		setBL(0x0208AF08, (u32)dsiSaveWrite);
		setBL(0x0208AF34, (u32)dsiSaveWrite);
		setBL(0x0208AF50, (u32)dsiSaveWrite);
		setBL(0x0208AF6C, (u32)dsiSaveWrite);
		setBL(0x0208AF7C, (u32)dsiSaveWrite);
		setBL(0x0208AF8C, (u32)dsiSaveWrite);
		setBL(0x0208AF9C, (u32)dsiSaveWrite);
		setBL(0x0208AFAC, (u32)dsiSaveWrite);
		setBL(0x0208AFB4, (u32)dsiSaveClose);
		setBL(0x0208B04C, (u32)dsiSaveOpen);
		setBL(0x0208B0DC, (u32)dsiSaveRead);
		setBL(0x0208B134, (u32)dsiSaveClose);
		setBL(0x0208B174, (u32)dsiSaveDelete);
		setBL(0x0208B1BC, (u32)dsiSaveRead);
		setBL(0x0208B234, (u32)dsiSaveRead);
		setBL(0x0208B290, (u32)dsiSaveRead);
		setBL(0x0208B2F4, (u32)dsiSaveRead);
		setBL(0x0208B340, (u32)dsiSaveRead);
		setBL(0x0208B38C, (u32)dsiSaveRead);
		setBL(0x0208B3D8, (u32)dsiSaveRead);
		setBL(0x0208B424, (u32)dsiSaveRead);
		setBL(0x0208B488, (u32)dsiSaveClose);
		setBL(0x0208B4C8, (u32)dsiSaveDelete);
		setBL(0x0208B50C, (u32)dsiSaveClose);
	}

	// GO Series: Defense Wars (USA)
	// GO Series: Defence Wars (Europe)
	else if ((strcmp(romTid, "KWTE") == 0 || strcmp(romTid, "KWTP") == 0) && saveOnFlashcard) {
		*(u32*)0x0200B350 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		//*(u32*)0x0200C584 = 0xE1A00000; // nop
		setBL(0x0200C5C0, (u32)dsiSaveCreate);
		setBL(0x0200C5FC, (u32)dsiSaveOpen);
		setBL(0x0200C634, (u32)dsiSaveSetLength);
		setBL(0x0200C644, (u32)dsiSaveWrite);
		setBL(0x0200C65C, (u32)dsiSaveClose);
		setBL(0x0200C6E4, (u32)dsiSaveOpen);
		setBL(0x0200C71C, (u32)dsiSaveSetLength);
		setBL(0x0200C72C, (u32)dsiSaveWrite);
		setBL(0x0200C744, (u32)dsiSaveClose);
		setBL(0x0200C7C4, (u32)dsiSaveOpen);
		setBL(0x0200C7FC, (u32)dsiSaveRead);
		setBL(0x0200C810, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveDelete);
		setBL(0x0200C8CC, (u32)dsiSaveGetInfo);
		*(u32*)0x0200C910 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0200C914 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x02044AEC, dsiSaveGetResultCode, 0xC);

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200CC98;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Dr. Mario Express (USA)
	// A Little Bit of... Dr. Mario (Europe, Australia)
	else if ((strcmp(romTid, "KD9E") == 0 || strcmp(romTid, "KD9V") == 0) && saveOnFlashcard) {
		tonccpy((u32*)0x02011160, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020248C4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02025CD4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0203D228 = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
		//*(u32*)0x0203D488 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0203D48C = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[3] == 'E') {
			//*(u32*)0x02044B00 = 0xE3A00000; // mov r0, #0
			setBL(0x020590C0, (u32)dsiSaveCreate);
			setBL(0x02059270, (u32)dsiSaveOpen);
			setBL(0x020593CC, (u32)dsiSaveClose);
			setBL(0x020594E8, (u32)dsiSaveSeek);
			setBL(0x020594F8, (u32)dsiSaveRead);
			setBL(0x02059674, (u32)dsiSaveSeek);
			setBL(0x02059684, (u32)dsiSaveWrite);
			setBL(0x020597FC, (u32)dsiSaveOpenR);
			setBL(0x020598A0, (u32)dsiSaveClose);
			*(u32*)0x02059920 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0207347C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020736DC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02074054 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		} else {
			//*(u32*)0x02044A9C = 0xE3A00000; // mov r0, #0
			setBL(0x02058FB0, (u32)dsiSaveCreate);
			setBL(0x02059160, (u32)dsiSaveOpen);
			setBL(0x020592BC, (u32)dsiSaveClose);
			setBL(0x020593D8, (u32)dsiSaveSeek);
			setBL(0x020593E8, (u32)dsiSaveRead);
			setBL(0x02059564, (u32)dsiSaveSeek);
			setBL(0x02059574, (u32)dsiSaveWrite);
			setBL(0x020596EC, (u32)dsiSaveOpenR);
			setBL(0x02059790, (u32)dsiSaveClose);
			*(u32*)0x02059810 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0207336C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020735CC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02073F44 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		}
	}

	// Chotto Dr. Mario (Japan)
	else if (strcmp(romTid, "KD9J") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x020118A4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02024CF4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02026104 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202D3B4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202D644 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202DFF0 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		*(u32*)0x0205824C = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
		//*(u32*)0x020584B4 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020584B8 = 0xE12FFF1E; // bx lr
		//*(u32*)0x0205F6F0 = 0xE3A00000; // mov r0, #0
		setBL(0x020736C4, (u32)dsiSaveCreate);
		setBL(0x02073874, (u32)dsiSaveOpen);
		setBL(0x020739D0, (u32)dsiSaveClose);
		setBL(0x02073AEC, (u32)dsiSaveSeek);
		setBL(0x02073AFC, (u32)dsiSaveRead);
		setBL(0x02073C78, (u32)dsiSaveSeek);
		setBL(0x02073C88, (u32)dsiSaveWrite);
		setBL(0x02073E00, (u32)dsiSaveOpenR);
		setBL(0x02073EA4, (u32)dsiSaveClose);
		*(u32*)0x02073F24 = 0xE3A00000; // mov r0, #0
	}

	// Dragon's Lair (USA)
	else if (strcmp(romTid, "KDLE") == 0 && saveOnFlashcard) {
		*(u32*)0x020051E4 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0201B8E8, (u32)dsiSaveOpen);
		setBL(0x0201B900, (u32)dsiSaveRead);
		setBL(0x0201B928, (u32)dsiSaveClose);
		setBL(0x0201B98C, (u32)dsiSaveCreate);
		setBL(0x0201B9BC, (u32)dsiSaveOpen);
		setBL(0x0201B9EC, (u32)dsiSaveWrite);
		setBL(0x0201BA14, (u32)dsiSaveClose);
		setBL(0x0201BAB4, (u32)dsiSaveOpen);
		setBL(0x0201BAFC, (u32)dsiSaveSeek);
		setBL(0x0201BB2C, (u32)dsiSaveWrite);
		setBL(0x0201BB54, (u32)dsiSaveClose);
		setBL(0x0201BBAC, (u32)dsiSaveGetResultCode);
		setBL(0x0201BBE8, (u32)dsiSaveClose);
		setBL(0x0201BC00, (u32)dsiSaveClose);
	}

	// Dragon's Lair (Europe, Australia)
	else if (strcmp(romTid, "KDLV") == 0 && saveOnFlashcard) {
		*(u32*)0x020051E4 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0201B8DC, (u32)dsiSaveOpen);
		setBL(0x0201B8F4, (u32)dsiSaveRead);
		setBL(0x0201B91C, (u32)dsiSaveClose);
		setBL(0x0201B980, (u32)dsiSaveCreate);
		setBL(0x0201B9B0, (u32)dsiSaveOpen);
		setBL(0x0201B9E0, (u32)dsiSaveWrite);
		setBL(0x0201BA08, (u32)dsiSaveClose);
		setBL(0x0201BAA8, (u32)dsiSaveOpen);
		setBL(0x0201BAF0, (u32)dsiSaveSeek);
		setBL(0x0201BB20, (u32)dsiSaveWrite);
		setBL(0x0201BB48, (u32)dsiSaveClose);
		setBL(0x0201BBA0, (u32)dsiSaveGetResultCode);
		setBL(0x0201BBDC, (u32)dsiSaveClose);
		setBL(0x0201BBF4, (u32)dsiSaveClose);
	}

	// Dragon's Lair II: Time Warp (USA)
	else if (strcmp(romTid, "KLYE") == 0 && saveOnFlashcard) {
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02020034, (u32)dsiSaveOpen);
		setBL(0x0202004C, (u32)dsiSaveRead);
		setBL(0x02020074, (u32)dsiSaveClose);
		setBL(0x02020110, (u32)dsiSaveCreate);
		setBL(0x02020140, (u32)dsiSaveOpen);
		setBL(0x02020170, (u32)dsiSaveWrite);
		setBL(0x02020198, (u32)dsiSaveClose);
		setBL(0x02020274, (u32)dsiSaveOpen);
		setBL(0x020202B0, (u32)dsiSaveSeek);
		setBL(0x020202E0, (u32)dsiSaveWrite);
		setBL(0x02020308, (u32)dsiSaveClose);
		setBL(0x02020374, (u32)dsiSaveGetResultCode);
		setBL(0x020203A4, (u32)dsiSaveClose);
		setBL(0x020203BC, (u32)dsiSaveClose);
	}

	// Dragon's Lair II: Time Warp (Europe, Australia)
	else if (strcmp(romTid, "KLYV") == 0 && saveOnFlashcard) {
		*(u32*)0x020051E0 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0202004C, (u32)dsiSaveOpen);
		setBL(0x02020064, (u32)dsiSaveRead);
		setBL(0x0202008C, (u32)dsiSaveClose);
		setBL(0x02020128, (u32)dsiSaveCreate);
		setBL(0x02020158, (u32)dsiSaveOpen);
		setBL(0x02020188, (u32)dsiSaveWrite);
		setBL(0x020201B0, (u32)dsiSaveClose);
		setBL(0x0202028C, (u32)dsiSaveOpen);
		setBL(0x020202C8, (u32)dsiSaveSeek);
		setBL(0x020202F8, (u32)dsiSaveWrite);
		setBL(0x02020320, (u32)dsiSaveClose);
		setBL(0x0202038C, (u32)dsiSaveGetResultCode);
		setBL(0x020203BC, (u32)dsiSaveClose);
		setBL(0x020203D4, (u32)dsiSaveClose);
		setBL(0x02020424, (u32)dsiSaveCreate);
	}

	// Dragon Quest Wars (USA)
	// DSi save function patching not needed
	else if (strcmp(romTid, "KDQE") == 0 && saveOnFlashcard) {
		*(u32*)0x0201F208 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Dragon Quest Wars (Europe, Australia)
	// DSi save function patching not needed
	else if (strcmp(romTid, "KDQV") == 0 && saveOnFlashcard) {
		*(u32*)0x0201F250 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Dragon Quest Wars (Japan)
	// DSi save function patching not needed
	else if (strcmp(romTid, "KDQJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0201EF84 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Dreamwalker (USA)
	else if (strcmp(romTid, "K9EE") == 0 && saveOnFlashcard) {
		setBL(0x0202963C, (u32)dsiSaveOpen);
		setBL(0x02029654, (u32)dsiSaveRead);
		setBL(0x0202967C, (u32)dsiSaveClose);
		setBL(0x020296F4, (u32)dsiSaveCreate);
		setBL(0x02029724, (u32)dsiSaveOpen);
		setBL(0x02029754, (u32)dsiSaveWrite);
		setBL(0x0202977C, (u32)dsiSaveClose);
		setBL(0x0202983C, (u32)dsiSaveOpen);
		setBL(0x02029878, (u32)dsiSaveSeek);
		setBL(0x020298A8, (u32)dsiSaveWrite);
		setBL(0x020298D0, (u32)dsiSaveClose);
		setBL(0x02029938, (u32)dsiSaveGetResultCode);
		setBL(0x0202996C, (u32)dsiSaveClose);
		setBL(0x02029984, (u32)dsiSaveClose);
		setBL(0x020299DC, (u32)dsiSaveSeek);
		setBL(0x020299F0, (u32)dsiSaveWrite);
	}

	// DS WiFi Settings
	else if (strcmp(romTid, "B88A") == 0) {
		tonccpy((void*)0x023C0000, ce9->thumbPatches->reset_arm9, 0x18);

		const u16* branchCode = generateA7InstrThumb(0x020051F4, 0x023C0000);

		*(u16*)0x020051F4 = branchCode[0];
		*(u16*)0x020051F6 = branchCode[1];
	}

	// GO Series: Earth Saver (USA)
	else if (strcmp(romTid, "KB8E") == 0 && saveOnFlashcard) {
		*(u32*)0x02005530 = 0xE1A00000; // nop
		//*(u32*)0x02005534 = 0xE1A00000; // nop
		*(u32*)0x0200A3D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		//*(u32*)0x0200A898 = 0xE12FFF1E; // bx lr
		setBL(0x0200AC14, (u32)dsiSaveOpen);
		setBL(0x0200AC50, (u32)dsiSaveRead);
		setBL(0x0200AC70, (u32)dsiSaveClose);
		setBL(0x0200AD0C, (u32)dsiSaveCreate);
		setBL(0x0200AD4C, (u32)dsiSaveOpen);
		setBL(0x0200AD84, (u32)dsiSaveSetLength);
		setBL(0x0200ADA0, (u32)dsiSaveWrite);
		setBL(0x0200ADC4, (u32)dsiSaveClose);
		setBL(0x0200AE58, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B800 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02014AB0 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x0204CB6C, dsiSaveGetResultCode, 0xC);

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014BEC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// GO Series: Earth Saver (Europe)
	else if (strcmp(romTid, "KB8P") == 0 && saveOnFlashcard) {
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x0200A310 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0200AB24, (u32)dsiSaveOpen);
		setBL(0x0200AB60, (u32)dsiSaveRead);
		setBL(0x0200AB80, (u32)dsiSaveClose);
		setBL(0x0200AC1C, (u32)dsiSaveCreate);
		setBL(0x0200AC5C, (u32)dsiSaveOpen);
		setBL(0x0200AC94, (u32)dsiSaveSetLength);
		setBL(0x0200ACB0, (u32)dsiSaveWrite);
		setBL(0x0200ACD4, (u32)dsiSaveClose);
		setBL(0x0200AD68, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B710 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020149B4 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		*(u32*)0x02047D50 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x0204CA70, dsiSaveGetResultCode, 0xC);

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014AF0;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Fashion Tycoon (USA)
	// Saving not supported due to some weirdness with the code going on
	else if (strcmp(romTid, "KU7E") == 0 && saveOnFlashcard) {
		/* const u32 dsiSaveCreateT = 0x020370F4;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC); // Original function overwritten, no BL setting needed

		const u32 dsiSaveSetLengthT = 0x02037104;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02037114;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveOpenRT = 0x02037124;
		*(u16*)dsiSaveOpenRT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenRT + 4), dsiSaveOpenR, 0x10);

		const u32 dsiSaveCloseT = 0x02037138;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02037148;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02037300;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020372F0;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC); // Original function overwritten, no BL setting needed */

		*(u16*)0x020271CC = 0x2001; // movs r0, #1
		*(u16*)0x020271CE = 0x4770; // bx lr
		*(u16*)0x02027A40 = 0x2001; // movs r0, #1
		*(u16*)0x02027A42 = 0x4770; // bx lr
		/* setBLThumb(0x020271F2, dsiSaveOpenRT);
		setBLThumb(0x020271FC, 0x020274A6);
		setBLThumb(0x020274C2, dsiSaveOpenT);
		setBLThumb(0x020274D6, dsiSaveSeekT);
		setBLThumb(0x020274E2, dsiSaveReadT);
		doubleNopT(0x020274F4);
		setBLThumb(0x02027500, dsiSaveCloseT);
		setBLThumb(0x02027828, dsiSaveOpenT);
		setBLThumb(0x02027838, dsiSaveSetLengthT);
		doubleNopT(0x0202783E); // dsiSaveFlush
		setBLThumb(0x02027844, dsiSaveCloseT);
		setBLThumb(0x0202784E, dsiSaveCloseT);
		setBLThumb(0x02027A6E, dsiSaveOpenRT);
		setBLThumb(0x02027A78, 0x02027D3E);
		setBLThumb(0x02027DF8, dsiSaveSetLengthT);
		doubleNopT(0x02027DFE); // dsiSaveFlush
		setBLThumb(0x02027E04, dsiSaveCloseT);
		setBLThumb(0x02027E2C, dsiSaveSeekT);
		setBLThumb(0x02027E36, dsiSaveReadT);
		doubleNopT(0x02027E4C);
		setBLThumb(0x02027E5E, dsiSaveCloseT);
		setBLThumb(0x02028170, dsiSaveOpenT);
		setBLThumb(0x02028198, dsiSaveSetLengthT);
		doubleNopT(0x0202819E); // dsiSaveFlush
		setBLThumb(0x020281A4, dsiSaveCloseT);
		setBLThumb(0x020281C6, dsiSaveCloseT);
		setBLThumb(0x02028200, dsiSaveOpenT);
		setBLThumb(0x02028216, dsiSaveSeekT);
		setBLThumb(0x02028220, dsiSaveReadT); // dsiSaveReadAsync
		doubleNopT(0x02028230); // dsiSaveFlush
		setBLThumb(0x02028236, dsiSaveCloseT); */
		//*(u16*)0x0203728C = 0x2000; // movs r0, #0 (dsiSaveOpenDir)
		//*(u16*)0x0203728E = 0x4770; // bx lr
		//*(u16*)0x020372D8 = 0x2000; // movs r0, #0 (dsiSaveCloseDir)
		//*(u16*)0x020372DA = 0x4770; // bx lr
	}

	// Frogger Returns (USA)
	else if (strcmp(romTid, "KFGE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201234C, dsiSaveGetResultCode, 0xC);
		setBL(0x02038250, (u32)dsiSaveGetInfo);
		setBL(0x02038294, (u32)dsiSaveOpen);
		setBL(0x020382B0, (u32)dsiSaveRead);
		setBL(0x020382BC, (u32)dsiSaveClose);
		setBL(0x020383F0, (u32)dsiSaveGetInfo);
		setBL(0x02038418, (u32)dsiSaveDelete);
		setBL(0x02038424, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02038454, (u32)dsiSaveOpen);
		setBL(0x02038470, (u32)dsiSaveWrite);
		setBL(0x02038478, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x0204B968 = 0xE1A00000; // nop
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0204B98C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Go! Go! Kokopolo (USA)
	// Go! Go! Kokopolo (Europe)
	else if ((strcmp(romTid, "K3GE") == 0 || strcmp(romTid, "K3GP") == 0) && saveOnFlashcard) {
		const u32 readCodeCopy = 0x02013CF4;

		if (ndsHeader->romversion == 0) {
			if (ndsHeader->gameCode[3] == 'E') {
				tonccpy((u32*)readCodeCopy, (u32*)0x020BCB48, 0x70);

				setBL(0x02043290, readCodeCopy);
				setBL(0x02043328, readCodeCopy);

				setBL(readCodeCopy+0x18, 0x02013CC8);
				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
				setBL(readCodeCopy+0x54, 0x020BCA0C);

				*(u32*)0x0208EB74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

				setBL(0x020BCCF4, (u32)dsiSaveCreate);
				setBL(0x020BCD04, (u32)dsiSaveOpen);
				setBL(0x020BCD18, (u32)dsiSaveSetLength);
				setBL(0x020BCD28, (u32)dsiSaveWrite);
				setBL(0x020BCD30, (u32)dsiSaveClose);
			} else {
				tonccpy((u32*)readCodeCopy, (u32*)0x020BCC6C, 0x70);

				setBL(0x020432EC, readCodeCopy);
				setBL(0x02043384, readCodeCopy);

				setBL(readCodeCopy+0x18, 0x02013CC8);
				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
				setBL(readCodeCopy+0x54, 0x020BCB20);

				*(u32*)0x0208EC38 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

				setBL(0x020BCE18, (u32)dsiSaveCreate);
				setBL(0x020BCE28, (u32)dsiSaveOpen);
				setBL(0x020BCE3C, (u32)dsiSaveSetLength);
				setBL(0x020BCE4C, (u32)dsiSaveWrite);
				setBL(0x020BCE54, (u32)dsiSaveClose);
			}
		} else {
			tonccpy((u32*)readCodeCopy, (u32*)0x020BCDA4, 0x70);

			setBL(0x020432EC, readCodeCopy);
			setBL(0x02043384, readCodeCopy);

			setBL(readCodeCopy+0x18, 0x02013CC8);
			setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
			setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
			setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
			setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
			setBL(readCodeCopy+0x54, 0x020BCC58);

			*(u32*)0x0208EE9C = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

			setBL(0x020BCF50, (u32)dsiSaveCreate);
			setBL(0x020BCF60, (u32)dsiSaveOpen);
			setBL(0x020BCF74, (u32)dsiSaveSetLength);
			setBL(0x020BCF84, (u32)dsiSaveWrite);
			setBL(0x020BCF8C, (u32)dsiSaveClose);
		}
	}

	// Go! Go! Kokopolo (Japan)
	else if (strcmp(romTid, "K3GJ") == 0 && saveOnFlashcard) {
		const u32 readCodeCopy = 0x02013D24;
		tonccpy((u32*)readCodeCopy, (u32*)0x020BCD90, 0x70);

		setBL(0x0204324C, readCodeCopy);
		setBL(0x020432E4, readCodeCopy);

		setBL(readCodeCopy+0x18, 0x02013CF8);
		setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
		setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
		setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
		setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
		setBL(readCodeCopy+0x54, 0x020BCC44);

		*(u32*)0x0208EC74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

		setBL(0x020BCF3C, (u32)dsiSaveCreate);
		setBL(0x020BCF4C, (u32)dsiSaveOpen);
		setBL(0x020BCF60, (u32)dsiSaveSetLength);
		setBL(0x020BCF70, (u32)dsiSaveWrite);
		setBL(0x020BCF78, (u32)dsiSaveClose);
	}

	// Hard-Hat Domo (USA)
	else if (strcmp(romTid, "KDHE") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x020238C8;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x020238D8;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x020238E8;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x020238F8;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02023908;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02023918;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02023928;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0200D060 = 0x2001; // movs r0, #1
		*(u16*)0x0200D062 = 0x4770; // bx lr
		*(u16*)0x0200D350 = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0200D352 = 0x4770; // bx lr
		doubleNopT(0x0200D30E);
		setBLThumb(0x0200D3B6, dsiSaveCreateT);
		setBLThumb(0x0200D3CC, dsiSaveOpenT);
		setBLThumb(0x0200D3E8, dsiSaveSetLengthT);
		setBLThumb(0x0200D3FC, dsiSaveWriteT);
		setBLThumb(0x0200D40E, dsiSaveCloseT);
		*(u16*)0x0200D434 = 0x4778; // bx pc
		tonccpy((u32*)0x0200D438, dsiSaveGetLength, 0xC);
		setBLThumb(0x0200D464, dsiSaveOpenT);
		setBLThumb(0x0200D48A, dsiSaveCloseT);
		setBLThumb(0x0200D49C, dsiSaveReadT);
		setBLThumb(0x0200D4A2, dsiSaveCloseT);
		setBLThumb(0x0200D4B6, dsiSaveDeleteT);
		*(u16*)0x020140AC = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Heathcliff: Spot On (USA)
	else if (strcmp(romTid, "K6SE") == 0 && saveOnFlashcard) {
		/* *(u32*)0x02005248 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005280 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020052C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205B604 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205B608 = 0xE12FFF1E; // bx lr */

		setBL(0x0204BF68, (u32)dsiSaveOpenR);
		setBL(0x0204BF88, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0204BFC0, (u32)dsiSaveOpen);
		setBL(0x0204BFD4, (u32)dsiSaveGetResultCode);
		setBL(0x0205B640, (u32)dsiSaveOpen);
		setBL(0x0205B658, (u32)dsiSaveSeek);
		setBL(0x0205B668, (u32)dsiSaveRead);
		setBL(0x0205B680, (u32)dsiSaveClose);
		setBL(0x0205B69C, (u32)dsiSaveOpen);
		setBL(0x0205B6B4, (u32)dsiSaveSeek);
		setBL(0x0205B6C4, (u32)dsiSaveWrite);
		setBL(0x0205B6CC, (u32)dsiSaveClose);
		setBL(0x0205B71C, (u32)dsiSaveOpen);
		setBL(0x0205B738, (u32)dsiSaveSeek);
		setBL(0x0205B748, (u32)dsiSaveRead);
		setBL(0x0205B750, (u32)dsiSaveClose);
	}

	// Hidden Photo (Europe)
	else if (strcmp(romTid, "DD3P") == 0 && consoleModel == 0) {
		*(u32*)0x0201BC14 -= 0xD000; // Shift heap
	}

	// Wimmelbild Creator (German)
	else if (strcmp(romTid, "DD3D") == 0 && consoleModel == 0) {
		*(u32*)0x0201B9E4 -= 0xD000; // Shift heap
	}

	// JellyCar 2 (USA)
	else if (strcmp(romTid, "KJYE") == 0 && saveOnFlashcard) {
		setBL(0x020067C4, (u32)dsiSaveOpen);
		*(u32*)0x020067DC = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x020067F4 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x02006810, (u32)dsiSaveClose);
		setBL(0x020070A0, (u32)dsiSaveOpen);
		setBL(0x0200710C, (u32)dsiSaveClose);
		setBL(0x0200761C, (u32)dsiSaveOpen);
		setBL(0x020076A8, (u32)dsiSaveClose);
		*(u32*)0x02019F94 = 0xE1A00000; // nop (Skip Manual screen, Part 1)
		for (int i = 0; i < 11; i++) { // Skip Manual screen, Part 2
			u32* offset = (u32*)0x0201A028;
			offset[i] = 0xE1A00000; // nop
		}
		setBL(0x020B2934, (u32)dsiSaveOpen);
		setBL(0x020B294C, (u32)dsiSaveCreate);
		setBL(0x020B297C, (u32)dsiSaveOpen);
		setBL(0x020B29C4, (u32)dsiSaveWrite);
		setBL(0x020B29E4, (u32)dsiSaveClose);
		setBL(0x020B29FC, (u32)dsiSaveClose);
		setBL(0x020B2A14, (u32)dsiSaveOpen);
		setBL(0x020B2A24, (u32)dsiSaveGetLength);
		setBL(0x020B2A64, (u32)dsiSaveWrite);
		setBL(0x020B2A9C, (u32)dsiSaveSeek);
		setBL(0x020B2AD0, (u32)dsiSaveWrite);
		setBL(0x020B2ADC, (u32)dsiSaveClose);
		setBL(0x020B2BD0, (u32)dsiSaveOpen);
		setBL(0x020B2BF4, (u32)dsiSaveGetLength);
		setBL(0x020B2C48, (u32)dsiSaveRead);
		setBL(0x020B2C80, (u32)dsiSaveSeek);
		setBL(0x020B2CC0, (u32)dsiSaveRead);
		setBL(0x020B2CCC, (u32)dsiSaveClose);
		setBL(0x020B2E00, (u32)dsiSaveDelete);
	}

	// Kung Fu Dragon (USA)
	// Kung Fu Dragon (Europe)
	else if ((strcmp(romTid, "KT9E") == 0 || strcmp(romTid, "KT9P") == 0) && saveOnFlashcard) {
		*(u32*)0x02005310 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0201D8EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0201DA28, (u32)dsiSaveOpen);
		setBL(0x0201DA50, (u32)dsiSaveRead);
		setBL(0x0201DA60, (u32)dsiSaveRead);
		setBL(0x0201DA68, (u32)dsiSaveClose);
		setBL(0x0201DD24, (u32)dsiSaveOpen);
		setBL(0x0201DE6C, (u32)dsiSaveWrite);
		setBL(0x0201DE74, (u32)dsiSaveClose);
		setBL(0x020249A0, (u32)dsiSaveCreate);
		setBL(0x02024A58, (u32)dsiSaveCreate);
	}

	// Akushon Gemu: Tobeyo!! Dorago! (Japan)
	else if (strcmp(romTid, "KT9J") == 0 && saveOnFlashcard) {
		*(u32*)0x020052F0 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0201D8C0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0201D9FC, (u32)dsiSaveOpen);
		setBL(0x0201DA24, (u32)dsiSaveRead);
		setBL(0x0201DA34, (u32)dsiSaveRead);
		setBL(0x0201DA3C, (u32)dsiSaveClose);
		setBL(0x0201DCF8, (u32)dsiSaveOpen);
		setBL(0x0201DE40, (u32)dsiSaveWrite);
		setBL(0x0201DE48, (u32)dsiSaveClose);
		setBL(0x020248C8, (u32)dsiSaveCreate);
		setBL(0x02024980, (u32)dsiSaveCreate);
	}

	// Little Red Riding Hood's Zombie BBQ (USA)
	else if (strcmp(romTid, "KZBE") == 0 && saveOnFlashcard) {
		*(u32*)0x02026BFC = 0xE3A00001; // mov r0, #1
		setBL(0x0204A7D4, (u32)dsiSaveOpen);
		setBL(0x0204A7EC, (u32)dsiSaveGetLength);
		setBL(0x0204A7FC, (u32)dsiSaveSeek);
		setBL(0x0204A80C, (u32)dsiSaveWrite);
		setBL(0x0204A814, (u32)dsiSaveClose);
		setBL(0x0204A884, (u32)dsiSaveOpen);
		setBL(0x0204A89C, (u32)dsiSaveGetLength);
		setBL(0x0204A8AC, (u32)dsiSaveSeek);
		setBL(0x0204A8BC, (u32)dsiSaveRead);
		setBL(0x0204A8C4, (u32)dsiSaveClose);
		setBL(0x0204A93C, (u32)dsiSaveCreate);
		setBL(0x0204A968, (u32)dsiSaveOpen);
		setBL(0x0204A9A4, (u32)dsiSaveWrite);
		setBL(0x0204A9B4, (u32)dsiSaveClose);
	}

	// Little Red Riding Hood's Zombie BBQ (Europe)
	else if (strcmp(romTid, "KZBP") == 0 && saveOnFlashcard) {
		*(u32*)0x02031F5C = 0xE3A00001; // mov r0, #1
		setBL(0x02055AC8, (u32)dsiSaveOpen);
		setBL(0x02055AE0, (u32)dsiSaveGetLength);
		setBL(0x02055AF0, (u32)dsiSaveSeek);
		setBL(0x02055B00, (u32)dsiSaveWrite);
		setBL(0x02055B08, (u32)dsiSaveClose);
		setBL(0x02055B78, (u32)dsiSaveOpen);
		setBL(0x02055B90, (u32)dsiSaveGetLength);
		setBL(0x02055BA0, (u32)dsiSaveSeek);
		setBL(0x02055BB0, (u32)dsiSaveRead);
		setBL(0x02055BB8, (u32)dsiSaveClose);
		setBL(0x02055C30, (u32)dsiSaveCreate);
		setBL(0x02055C5C, (u32)dsiSaveOpen);
		setBL(0x02055C98, (u32)dsiSaveWrite);
		setBL(0x02055CA8, (u32)dsiSaveClose);
	}

	// Littlest Pet Shop (USA)
	// Littlest Pet Shop (Europe, Australia)
	else if ((strcmp(romTid, "KLPE") == 0 || strcmp(romTid, "KLPV") == 0) && saveOnFlashcard) {
		*(u32*)0x0200509C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020055BC;
			offset[i] = 0xE1A00000; // nop
		}
		tonccpy((u32*)0x020159F0, dsiSaveGetResultCode, 0xC);
		setBL(0x0205DE18, (u32)dsiSaveOpen);
		setBL(0x0205DE70, (u32)dsiSaveRead);
		setBL(0x0205DF24, (u32)dsiSaveCreate);
		setBL(0x0205DF34, (u32)dsiSaveOpen);
		setBL(0x0205DF7C, (u32)dsiSaveSetLength);
		setBL(0x0205DF8C, (u32)dsiSaveWrite);
		setBL(0x0205DF94, (u32)dsiSaveClose);
	}

	// Magical Diary: Secrets Sharing (USA)
	else if (strcmp(romTid, "K73E") == 0 && saveOnFlashcard) {
		*(u32*)0x0201A17C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0201A22C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x0201A9BC, (u32)dsiSaveOpen);
		setBL(0x0201A9D0, (u32)dsiSaveSeek);
		setBL(0x0201A9E4, (u32)dsiSaveRead);
		setBL(0x0201AA18, (u32)dsiSaveClose);
		setBL(0x0201AA34, (u32)dsiSaveRead);
		setBL(0x0201AA70, (u32)dsiSaveClose);
		setBL(0x0201AACC, (u32)dsiSaveRead);
		setBL(0x0201AAFC, (u32)dsiSaveClose);
		setBL(0x0201AB40, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0201AB54, (u32)dsiSaveRead);
		setBL(0x0201AB88, (u32)dsiSaveClose);
		setBL(0x0201ABE0, (u32)dsiSaveClose);
		setBL(0x0201ABFC, (u32)dsiSaveClose);
		setBL(0x0201AC30, (u32)dsiSaveClose);
		setBL(0x0201AD50, (u32)dsiSaveOpen);
		setBL(0x0201AD60, (u32)dsiSaveRead);
		setBL(0x0201AD8C, (u32)dsiSaveClose);
		setBL(0x0201ADAC, (u32)dsiSaveSeek);
		setBL(0x0201ADC0, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201ADF0, (u32)dsiSaveClose);
		setBL(0x0201AE28, (u32)dsiSaveOpen);
		setBL(0x0201AE38, (u32)dsiSaveRead);
		setBL(0x0201AE64, (u32)dsiSaveClose);
		setBL(0x0201AE84, (u32)dsiSaveSeek);
		setBL(0x0201AE98, (u32)dsiSaveWrite);
		setBL(0x0201AEC0, (u32)dsiSaveClose);
		setBL(0x0201AEE0, (u32)dsiSaveClose);
		setBL(0x0201AF5C, (u32)dsiSaveOpen);
		setBL(0x0201AF70, (u32)dsiSaveCreate);
		setBL(0x0201AF90, (u32)dsiSaveCreate);
		setBL(0x0201AFB4, (u32)dsiSaveClose);
		setBL(0x0201B058, (u32)dsiSaveOpen);
		setBL(0x0201B06C, (u32)dsiSaveCreate);
		setBL(0x0201B090, (u32)dsiSaveClose);
		setBL(0x0201B0E4, (u32)dsiSaveOpen);
		setBL(0x0201B0F4, (u32)dsiSaveRead);
		setBL(0x0201B118, (u32)dsiSaveClose);
		setBL(0x0201B12C, (u32)dsiSaveClose);
	}

	// Tomodachi Tsukurou!: Mahou no Koukan Nikki (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K85J") == 0 && saveOnFlashcard) {
		*(u32*)0x0201A3A8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0201A420 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x0201AB94, (u32)dsiSaveOpen);
		setBL(0x0201ABA8, (u32)dsiSaveSeek);
		setBL(0x0201ABBC, (u32)dsiSaveRead);
		setBL(0x0201ABF0, (u32)dsiSaveClose);
		setBL(0x0201AC0C, (u32)dsiSaveRead);
		setBL(0x0201AC48, (u32)dsiSaveClose);
		setBL(0x0201ACA4, (u32)dsiSaveRead);
		setBL(0x0201ACD4, (u32)dsiSaveClose);
		setBL(0x0201AD18, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0201AD2C, (u32)dsiSaveRead);
		setBL(0x0201AD60, (u32)dsiSaveClose);
		setBL(0x0201ADB8, (u32)dsiSaveClose);
		setBL(0x0201ADD4, (u32)dsiSaveClose);
		setBL(0x0201AE08, (u32)dsiSaveClose);
		setBL(0x0201AF28, (u32)dsiSaveOpen);
		setBL(0x0201AF38, (u32)dsiSaveRead);
		setBL(0x0201AF64, (u32)dsiSaveClose);
		setBL(0x0201AF84, (u32)dsiSaveSeek);
		setBL(0x0201AF98, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201AFC8, (u32)dsiSaveClose);
		setBL(0x0201B000, (u32)dsiSaveOpen);
		setBL(0x0201B010, (u32)dsiSaveRead);
		setBL(0x0201B03C, (u32)dsiSaveClose);
		setBL(0x0201B05C, (u32)dsiSaveSeek);
		setBL(0x0201B070, (u32)dsiSaveWrite);
		setBL(0x0201B098, (u32)dsiSaveClose);
		setBL(0x0201B0B8, (u32)dsiSaveClose);
		setBL(0x0201B128, (u32)dsiSaveOpen);
		setBL(0x0201B13C, (u32)dsiSaveCreate);
		setBL(0x0201B15C, (u32)dsiSaveCreate);
		setBL(0x0201B178, (u32)dsiSaveClose);
		setBL(0x0201B1F0, (u32)dsiSaveOpen);
		setBL(0x0201B204, (u32)dsiSaveCreate);
		setBL(0x0201B21C, (u32)dsiSaveClose);
		setBL(0x0201B270, (u32)dsiSaveOpen);
		setBL(0x0201B280, (u32)dsiSaveRead);
		setBL(0x0201B2A0, (u32)dsiSaveClose);
	}

	// Magical Drop Yurutto (Japan)
	else if (strcmp(romTid, "KMAJ") == 0 && saveOnFlashcard) {
		*(u32*)0x020159D4 = 0xE1A00000; // nop (Disable NFTR font loading)
		setBL(0x020197AC, (u32)dsiSaveCreate);
		setBL(0x02019800, (u32)dsiSaveDelete);
		setBL(0x02019860, (u32)dsiSaveOpen);
		setBL(0x0201988C, (u32)dsiSaveGetLength);
		setBL(0x020198A0, (u32)dsiSaveRead);
		setBL(0x020198BC, (u32)dsiSaveClose);
		setBL(0x020198D0, (u32)dsiSaveClose);
		setBL(0x02019964, (u32)dsiSaveOpen);
		setBL(0x02019998, (u32)dsiSaveWrite);
		setBL(0x020199B4, (u32)dsiSaveClose);
		setBL(0x020199C8, (u32)dsiSaveClose);
		*(u32*)0x0201CED4 = 0xE1A00000; // nop (Disable NFTR font loading)
		tonccpy((u32*)0x020350D0, dsiSaveGetResultCode, 0xC);
		tonccpy((u32*)0x02035C7C, dsiSaveGetInfo, 0xC);
	}

	// Magical Whip (USA)
	else if (strcmp(romTid, "KWME") == 0 && saveOnFlashcard) {
		*(u32*)0x0201D4F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02030288 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0203F434, (u32)dsiSaveOpen);
		setBL(0x0203F46C, (u32)dsiSaveSetLength);
		setBL(0x0203F50C, (u32)dsiSaveWrite);
		setBL(0x0203F514, (u32)dsiSaveClose);
		setBL(0x0203F584, (u32)dsiSaveOpen);
		setBL(0x0203F5A8, (u32)dsiSaveGetLength);
		setBL(0x0203F5B8, (u32)dsiSaveRead);
		setBL(0x0203F5C8, (u32)dsiSaveRead);
		setBL(0x0203F5D0, (u32)dsiSaveClose);
		setBL(0x0203F920, (u32)dsiSaveCreate);
		setBL(0x0203FB34, (u32)dsiSaveCreate);
	}

	// Magical Whip (Europe)
	else if (strcmp(romTid, "KWMP") == 0 && saveOnFlashcard) {
		*(u32*)0x0201D5D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02030368 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0203F514, (u32)dsiSaveOpen);
		setBL(0x0203F54C, (u32)dsiSaveSetLength);
		setBL(0x0203F5EC, (u32)dsiSaveWrite);
		setBL(0x0203F5F4, (u32)dsiSaveClose);
		setBL(0x0203F664, (u32)dsiSaveOpen);
		setBL(0x0203F688, (u32)dsiSaveGetLength);
		setBL(0x0203F698, (u32)dsiSaveRead);
		setBL(0x0203F6A8, (u32)dsiSaveRead);
		setBL(0x0203F6B0, (u32)dsiSaveClose);
		setBL(0x0203FA00, (u32)dsiSaveCreate);
		setBL(0x0203FC14, (u32)dsiSaveCreate);
	}

	// Make Up & Style (USA)
	else if (strcmp(romTid, "KYLE") == 0 && saveOnFlashcard) {
		*(u32*)0x02005348 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0200534C = 0xE1A00000; // nop
		setBL(0x0202A34C, (u32)dsiSaveOpen);
		setBL(0x0202A360, (u32)dsiSaveCreate);
		setBL(0x0202A384, (u32)dsiSaveOpen);
		setBL(0x0202A398, (u32)dsiSaveSetLength);
		setBL(0x0202A3A4, (u32)dsiSaveClose);
		setBL(0x0202A3C4, (u32)dsiSaveSetLength);
		setBL(0x0202A3CC, (u32)dsiSaveClose);
		setBL(0x0202A5C4, (u32)dsiSaveOpen);
		setBL(0x0202A5EC, (u32)dsiSaveSeek);
		setBL(0x0202A600, (u32)dsiSaveRead);
		setBL(0x0202A60C, (u32)dsiSaveClose);
		setBL(0x0202A6D4, (u32)dsiSaveOpen);
		setBL(0x0202A6FC, (u32)dsiSaveSeek);
		setBL(0x0202A710, (u32)dsiSaveWrite);
		setBL(0x0202A71C, (u32)dsiSaveClose);
		tonccpy((u32*)0x02056468, dsiSaveGetResultCode, 0xC);
	}

	// Make Up & Style (Europe)
	else if (strcmp(romTid, "KYLP") == 0 && saveOnFlashcard) {
		*(u32*)0x02005360 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x02005364 = 0xE1A00000; // nop
		setBL(0x0202A484, (u32)dsiSaveOpen);
		setBL(0x0202A498, (u32)dsiSaveCreate);
		setBL(0x0202A4BC, (u32)dsiSaveOpen);
		setBL(0x0202A4D0, (u32)dsiSaveSetLength);
		setBL(0x0202A4DC, (u32)dsiSaveClose);
		setBL(0x0202A4FC, (u32)dsiSaveSetLength);
		setBL(0x0202A504, (u32)dsiSaveClose);
		setBL(0x0202A6FC, (u32)dsiSaveOpen);
		setBL(0x0202A724, (u32)dsiSaveSeek);
		setBL(0x0202A738, (u32)dsiSaveRead);
		setBL(0x0202A744, (u32)dsiSaveClose);
		setBL(0x0202A80C, (u32)dsiSaveOpen);
		setBL(0x0202A834, (u32)dsiSaveSeek);
		setBL(0x0202A848, (u32)dsiSaveWrite);
		setBL(0x0202A854, (u32)dsiSaveClose);
		tonccpy((u32*)0x020565A0, dsiSaveGetResultCode, 0xC);
	}

	// Metal Torrent (USA)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59E") == 0 && saveOnFlashcard) {
		*(u32*)0x02045F88 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x0206E8FC = 0xE3A07000; // mov r7, #0
		*(u32*)0x020DDB00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDD60 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDF00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DE0A4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
	}

	// Metal Torrent (Europe, Australia)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59V") == 0 && saveOnFlashcard) {
		*(u32*)0x02045FA0 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x0206E894 = 0xE3A07000; // mov r7, #0
		*(u32*)0x020DD918 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDB78 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDD18 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDEBC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
	}

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0 && saveOnFlashcard) {
		setBL(0x0200B048, (u32)dsiSaveCreate);
		setBL(0x0200B070, (u32)dsiSaveGetResultCode);
		setBL(0x0200B090, (u32)dsiSaveCreate);
		//*(u32*)0x0200B0A0 = 0xE1A00000; // nop
		setBL(0x0200B0E8, (u32)dsiSaveOpen);
		setBL(0x0200B114, (u32)dsiSaveOpen);
		setBL(0x0200B124, (u32)dsiSaveRead);
		setBL(0x0200B12C, (u32)dsiSaveClose);
		setBL(0x0200B388, (u32)dsiSaveCreate);
		setBL(0x0200B39C, (u32)dsiSaveOpen);
		setBL(0x0200B5A4, (u32)dsiSaveSetLength);
		setBL(0x0200B5B4, (u32)dsiSaveWrite);
		setBL(0x0200B5BC, (u32)dsiSaveClose);

		// Skip Manual screen
		/* *(u32*)0x0200F4EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200F59C = 0xE1A00000; // nop
		*(u32*)0x0200F5A4 = 0xE1A00000; // nop
		*(u32*)0x0200F5B4 = 0xE1A00000; // nop
		*(u32*)0x0200FC3C = 0xE3A00901; // mov r0, #0x4000
		*(u32*)0x0200FC5C = 0xE3A02901; // mov r2, #0x4000
		*(u32*)0x0200FC68 = 0xE3A01901; // mov r1, #0x4000 */

		// Hide help button
		*(u32*)0x0200FAE8 = 0xE1A00000; // nop
		*(u32*)0x0200FB08 = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Europe, Australia)
	else if (strcmp(romTid, "KMGV") == 0 && saveOnFlashcard) {
		setBL(0x0200B350, (u32)dsiSaveCreate);
		setBL(0x0200B378, (u32)dsiSaveGetResultCode);
		setBL(0x0200B398, (u32)dsiSaveCreate);
		//*(u32*)0x0200B3A8 = 0xE1A00000; // nop
		setBL(0x0200B3F0, (u32)dsiSaveOpen);
		setBL(0x0200B41C, (u32)dsiSaveOpen);
		setBL(0x0200B42C, (u32)dsiSaveRead);
		setBL(0x0200B434, (u32)dsiSaveClose);
		setBL(0x0200B690, (u32)dsiSaveCreate);
		setBL(0x0200B6A4, (u32)dsiSaveOpen);
		setBL(0x0200B8AC, (u32)dsiSaveSetLength);
		setBL(0x0200B8BC, (u32)dsiSaveWrite);
		setBL(0x0200B8C4, (u32)dsiSaveClose);

		// Skip Manual screen
		/* *(u32*)0x0200F974 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200FA1C = 0xE1A00000; // nop
		*(u32*)0x0200FA24 = 0xE1A00000; // nop
		*(u32*)0x0200FA34 = 0xE1A00000; // nop
		*(u32*)0x02010138 = 0xE3A00901; // mov r0, #0x4000
		*(u32*)0x02010158 = 0xE3A02901; // mov r2, #0x4000
		*(u32*)0x02010164 = 0xE3A01901; // mov r1, #0x4000 */

		// Hide help button
		*(u32*)0x0200FFC4 = 0xE1A00000; // nop
		*(u32*)0x0200FFF0 = 0xE1A00000; // nop
	}

	// Mighty Flip Champs! (Japan)
	else if (strcmp(romTid, "KMGJ") == 0 && saveOnFlashcard) {
		setBL(0x0200B134, (u32)dsiSaveCreate);
		setBL(0x0200B158, (u32)dsiSaveGetResultCode);
		setBL(0x0200B174, (u32)dsiSaveCreate);
		//*(u32*)0x0200B184 = 0xE1A00000; // nop
		setBL(0x0200B1D4, (u32)dsiSaveOpen);
		setBL(0x0200B1FC, (u32)dsiSaveOpen);
		setBL(0x0200B210, (u32)dsiSaveRead);
		setBL(0x0200B218, (u32)dsiSaveClose);
		setBL(0x0200B478, (u32)dsiSaveCreate);
		setBL(0x0200B488, (u32)dsiSaveOpen);
		setBL(0x0200B694, (u32)dsiSaveSetLength);
		setBL(0x0200B6A4, (u32)dsiSaveWrite);
		setBL(0x0200B6AC, (u32)dsiSaveClose);

		// Skip Manual screen
		/* *(u32*)0x0200F31C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200F3B4 = 0xE1A00000; // nop
		*(u32*)0x0200F3BC = 0xE1A00000; // nop
		*(u32*)0x0200F3C8 = 0xE1A00000; // nop
		*(u32*)0x0200FAA4 = 0xE3A06901; // mov r6, #0x4000 */

		// Hide help button
		*(u32*)0x0200F934 = 0xE1A00000; // nop
		*(u32*)0x0200F960 = 0xE1A00000; // nop
	}

	// Mighty Milky Way (USA)
	// Mighty Milky Way (Europe)
	// Mighty Milky Way (Japan)
	else if (strncmp(romTid, "KWY", 3) == 0 && saveOnFlashcard) {
		setBL(0x0200547C, (u32)dsiSaveCreate);
		setBL(0x020054A0, (u32)dsiSaveGetResultCode);
		setBL(0x020054BC, (u32)dsiSaveCreate);
		//*(u32*)0x020054E4 = 0xE1A00000; // nop
		setBL(0x02005534, (u32)dsiSaveOpen);
		setBL(0x0200555C, (u32)dsiSaveOpen);
		setBL(0x02005570, (u32)dsiSaveRead);
		setBL(0x02005578, (u32)dsiSaveClose);
		setBL(0x020057E4, (u32)dsiSaveCreate);
		setBL(0x020057F4, (u32)dsiSaveOpen);
		setBL(0x020059FC, (u32)dsiSaveSetLength);
		setBL(0x02005A0C, (u32)dsiSaveWrite);
		setBL(0x02005A14, (u32)dsiSaveClose);

		// Skip Manual screen
		if (ndsHeader->gameCode[3] == 'J') {
			*(u32*)0x02013694 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02013760 = 0xE1A00000; // nop
			*(u32*)0x02013768 = 0xE1A00000; // nop
			*(u32*)0x02013774 = 0xE1A00000; // nop
			*(u32*)0x02013E58 = 0xE3A06901; // mov r6, #0x4000
		} else {
			*(u32*)0x02013648 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02013714 = 0xE1A00000; // nop
			*(u32*)0x0201371C = 0xE1A00000; // nop
			*(u32*)0x02013728 = 0xE1A00000; // nop
			*(u32*)0x02013E04 = 0xE3A06901; // mov r6, #0x4000
		}
	}

	// Model Academy (Europe)
	else if (strcmp(romTid, "K8MP") == 0 && saveOnFlashcard) {
		setBL(0x020B19E4, (u32)dsiSaveCreate);
		setBL(0x020B19F4, (u32)dsiSaveOpen);
		setBL(0x020B1A04, (u32)dsiSaveGetResultCode);
		setBL(0x020B1A40, (u32)dsiSaveSetLength);
		setBL(0x020B1A50, (u32)dsiSaveWrite);
		setBL(0x020B1A58, (u32)dsiSaveClose);
		setBL(0x020B1A94, (u32)dsiSaveOpen);
		setBL(0x020B1AA4, (u32)dsiSaveGetResultCode);
		setBL(0x020B1ABC, (u32)dsiSaveGetLength);
		setBL(0x020B1ACC, (u32)dsiSaveRead);
		setBL(0x020B1AD4, (u32)dsiSaveClose);
		setBL(0x020B1B0C, (u32)dsiSaveOpen);
		setBL(0x020B1B1C, (u32)dsiSaveGetResultCode);
		setBL(0x020B1B34, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x020B2800 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020B2894 = 0xE1A00000; // nop
		*(u32*)0x020B289C = 0xE1A00000; // nop
		*(u32*)0x020B28A8 = 0xE1A00000; // nop
	}

	// Monster Buster Club (USA)
	else if (strcmp(romTid, "KXBE") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0207F058 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F05C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F138 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F13C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F4FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F63C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F640 = 0xE12FFF1E; // bx lr */
		*(u32*)0x0207F0B8 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x0207F17C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0207F244, (u32)dsiSaveCreate);
		setBL(0x0207F254, (u32)dsiSaveOpen);
		setBL(0x0207F298, (u32)dsiSaveSetLength);
		setBL(0x0207F2B4, (u32)dsiSaveWrite);
		setBL(0x0207F2C8, (u32)dsiSaveClose);
		setBL(0x0207F494, (u32)dsiSaveOpen);
		setBL(0x0207F4C4, (u32)dsiSaveRead);
		setBL(0x0207F4D8, (u32)dsiSaveClose);
		*(u32*)0x0207F530 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F560 = 0xE3A00001; // mov r0, #1
		setBL(0x0207F584, 0x0207F5C4);
		*(u32*)0x0207F5E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F654 = 0xE3A00001; // mov r0, #1
		tonccpy((u32*)0x02094EAC, dsiSaveGetResultCode, 0xC);
	}

	// Monster Buster Club (Europe)
	else if (strcmp(romTid, "KXBP") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0207EF64 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207EF68 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F048 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F414 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F418 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F558 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F55C = 0xE12FFF1E; // bx lr */
		*(u32*)0x0207EFC4 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x0207F084 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0207F14C, (u32)dsiSaveCreate);
		setBL(0x0207F15C, (u32)dsiSaveOpen);
		setBL(0x0207F1A0, (u32)dsiSaveSetLength);
		setBL(0x0207F1BC, (u32)dsiSaveWrite);
		setBL(0x0207F1D0, (u32)dsiSaveClose);
		setBL(0x0207F3AC, (u32)dsiSaveOpen);
		setBL(0x0207F3E0, (u32)dsiSaveRead);
		setBL(0x0207F3F4, (u32)dsiSaveClose);
		*(u32*)0x0207F44C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F47C = 0xE3A00001; // mov r0, #1
		setBL(0x0207F4A0, 0x0207F4E0);
		*(u32*)0x0207F504 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F584 = 0xE3A00001; // mov r0, #1
		tonccpy((u32*)0x02094DE0, dsiSaveGetResultCode, 0xC);
	}

	// Motto Me de Unou o Kitaeru: DS Sokudoku Jutsu Light (Japan)
	else if (strcmp(romTid, "K9SJ") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0200F3A8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0203F378 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02047350, (u32)dsiSaveOpen);
		setBL(0x0204739C, (u32)dsiSaveCreate);
		setBL(0x020473D0, (u32)dsiSaveGetLength);
		setBL(0x020473E0, (u32)dsiSaveRead);
		setBL(0x020473E8, (u32)dsiSaveClose);
		setBL(0x02047480, (u32)dsiSaveOpen);
		setBL(0x020474CC, (u32)dsiSaveCreate);
		setBL(0x02047504, (u32)dsiSaveOpen);
		setBL(0x02047548, (u32)dsiSaveSetLength);
		setBL(0x02047558, (u32)dsiSaveClose);
		setBL(0x02047578, (u32)dsiSaveWrite);
		setBL(0x02047580, (u32)dsiSaveClose);
	}

	// Mr. Brain (Japan)
	else if (strcmp(romTid, "KMBJ") == 0 && saveOnFlashcard) {
		setBL(0x02005A40, (u32)dsiSaveOpen);
		setBL(0x02005A5C, (u32)dsiSaveGetLength);
		setBL(0x02005A78, (u32)dsiSaveRead);
		setBL(0x02005AA0, (u32)dsiSaveClose);
		setBL(0x02005AF8, (u32)dsiSaveCreate);
		setBL(0x02005B08, (u32)dsiSaveOpen);
		setBL(0x02005B34, (u32)dsiSaveSetLength);
		setBL(0x02005B54, (u32)dsiSaveWrite);
		setBL(0x02005B6C, (u32)dsiSaveClose);
		tonccpy((u32*)0x02026748, dsiSaveGetResultCode, 0xC);
	}

	// Mr. Driller: Drill Till You Drop (USA)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRE") == 0 && saveOnFlashcard) {
		*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x02032410, (u32)dsiSaveOpen);
		setBL(0x02032428, (u32)dsiSaveClose);
		*(u32*)0x02032450 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x020324A8, (u32)dsiSaveCreate);
		setBL(0x020324CC, (u32)dsiSaveGetResultCode);
		setBL(0x0203277C, (u32)dsiSaveOpen);
		setBL(0x02032790, (u32)dsiSaveSeek);
		setBL(0x020327A0, (u32)dsiSaveRead);
		setBL(0x020327C0, (u32)dsiSaveClose);
		setBL(0x02032858, (u32)dsiSaveOpen);
		setBL(0x02032888, (u32)dsiSaveWrite);
		setBL(0x02032948, (u32)dsiSaveClose);
	}

	// Mr. Driller: Drill Till You Drop (Europe, Australia)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRV") == 0 && saveOnFlashcard) {
		*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x02032160, (u32)dsiSaveOpen);
		setBL(0x02032178, (u32)dsiSaveClose);
		*(u32*)0x020321A0 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x020321F8, (u32)dsiSaveCreate);
		setBL(0x0203221C, (u32)dsiSaveGetResultCode);
		setBL(0x020324CC, (u32)dsiSaveOpen);
		setBL(0x020324E0, (u32)dsiSaveSeek);
		setBL(0x020324F0, (u32)dsiSaveRead);
		setBL(0x02032510, (u32)dsiSaveClose);
		setBL(0x020325A8, (u32)dsiSaveOpen);
		setBL(0x020325D8, (u32)dsiSaveWrite);
		setBL(0x02032698, (u32)dsiSaveClose);
	}

	// Sakutto Hamareru Hori Hori Action: Mr. Driller (Japan)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0201FE10 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202000C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202027C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x0203204C, (u32)dsiSaveOpen);
		setBL(0x02032064, (u32)dsiSaveClose);
		*(u32*)0x0203208C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x020320E4, (u32)dsiSaveCreate);
		setBL(0x02032108, (u32)dsiSaveGetResultCode);
		setBL(0x020323B8, (u32)dsiSaveOpen);
		setBL(0x020323CC, (u32)dsiSaveSeek);
		setBL(0x020323DC, (u32)dsiSaveRead);
		setBL(0x020323FC, (u32)dsiSaveClose);
		setBL(0x02032494, (u32)dsiSaveOpen);
		setBL(0x020324C4, (u32)dsiSaveWrite);
		setBL(0x02032584, (u32)dsiSaveClose);
	}

	// Need for Speed: Nitro-X (USA)
	// Need for Speed: Nitro-X (Europe, Australia)
	else if (strncmp(romTid, "KNP", 3) == 0 && saveOnFlashcard) {
		/*const u32 dsiSaveCreateT = 0x0201D090;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveGetInfoT = 0x0201D0A0;
		*(u16*)dsiSaveGetInfoT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetInfoT + 4), dsiSaveGetInfo, 0xC);

		const u32 dsiSaveOpenT = 0x0201D0B0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x0201D0C0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x0201D0D0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x0201D0E0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x0201D0F0;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);*/

		//tonccpy((u32*)0x0201C5A8, dsiSaveGetResultCode, 0xC);
		//tonccpy((u32*)0x0201D178, dsiSaveSetLength, 0xC);
		*(u16*)0x020EBFC4 = 0x4770; // bx lr
		/*setBLThumb(0x020EBFDC, dsiSaveOpenT);
		setBLThumb(0x020EBFEA, dsiSaveCloseT);
		setBLThumb(0x020EBFF6, dsiSaveGetInfoT);
		setBLThumb(0x020EC01A, dsiSaveCreateT);
		setBLThumb(0x020EC02C, dsiSaveOpenT);
		setBLThumb(0x020EC03C, dsiSaveCloseT);
		setBLThumb(0x020EC05A, dsiSaveOpenT);
		setBLThumb(0x020EC07A, dsiSaveSeekT);
		setBLThumb(0x020EC090, dsiSaveWriteT);
		setBLThumb(0x020EC098, dsiSaveCloseT);*/
		*(u16*)0x020EC0C0 = 0x4770; // bx lr
		/*setBLThumb(0x020EC0D8, dsiSaveOpenT);
		setBLThumb(0x020EC0E6, dsiSaveCloseT);
		setBLThumb(0x020EC0F2, dsiSaveGetInfoT);
		setBLThumb(0x020EC118, dsiSaveCreateT);
		setBLThumb(0x020EC126, dsiSaveOpenT);
		setBLThumb(0x020EC136, dsiSaveCloseT);
		setBLThumb(0x020EC14C, dsiSaveOpenT);
		setBLThumb(0x020EC162, dsiSaveSeekT);
		setBLThumb(0x020EC176, dsiSaveReadT);
		setBLThumb(0x020EC17E, dsiSaveCloseT);*/
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

	// Nintendogs (China)
	else if (strcmp(romTid, "KDOC") == 0 && saveOnFlashcard) {
		setBL(0x0202A6EC, (u32)dsiSaveSeek);
		setBL(0x0202A6FC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202A8C8, (u32)dsiSaveSeek);
		setBL(0x0202A8D8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202A95C, (u32)dsiSaveSeek);
		setBL(0x0202A96C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202AB28, (u32)dsiSaveSeek);
		setBL(0x0202AB38, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202BB94, (u32)dsiSaveOpen);
		setBL(0x0202BBA8, (u32)dsiSaveCreate);
		setBL(0x0202BBB8, (u32)dsiSaveGetResultCode);
		setBL(0x0202BBD4, (u32)dsiSaveOpen);
		setBL(0x0202BBFC, (u32)dsiSaveSetLength);
		setBL(0x0202BC04, (u32)dsiSaveClose);
		setBL(0x0202BC14, (u32)dsiSaveOpen);
		*(u32*)0x0202BD3C = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0202BD44, (u32)dsiSaveClose);
		setBL(0x0202BE14, (u32)dsiSaveSeek);
		setBL(0x0202BE24, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0202BE44, (u32)dsiSaveSeek);
		setBL(0x0202BE54, (u32)dsiSaveRead); // dsiSaveReadAsync
		*(u32*)0x0202BF0C = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0202BF14, (u32)dsiSaveClose);
	}

	// Orion's Odyssey (USA)
	// Due to our save implementation, save data is stored in both slots
	else if (strcmp(romTid, "K6TE") == 0 && saveOnFlashcard) {
		setBL(0x020284D0, (u32)dsiSaveDelete);
		setBL(0x020284DC, (u32)dsiSaveCreate);
		setBL(0x020284EC, (u32)dsiSaveOpen);
		setBL(0x0202851C, (u32)dsiSaveSetLength);
		setBL(0x02028538, (u32)dsiSaveDelete);
		setBL(0x02028544, (u32)dsiSaveCreate);
		setBL(0x02028554, (u32)dsiSaveOpen);
		setBL(0x0202857C, (u32)dsiSaveSetLength);
		setBL(0x0202858C, (u32)dsiSaveWrite);
		setBL(0x02028594, (u32)dsiSaveClose);
		*(u32*)0x02028C3C = 0xE1A00000; // nop (dsiSaveOpenDir)
		setBL(0x02028C50, (u32)dsiSaveOpen);
		setBL(0x02028C6C, (u32)dsiSaveOpen);
		setBL(0x02028CB0, (u32)dsiSaveGetLength);
		setBL(0x02028CD4, (u32)dsiSaveRead);
		setBL(0x02028E0C, (u32)dsiSaveClose);
		setBL(0x02028E54, (u32)dsiSaveGetLength);
		setBL(0x02028E78, (u32)dsiSaveRead);
		setBL(0x02028FB0, (u32)dsiSaveClose);
		*(u32*)0x02028FC8 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x02029030 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02029048 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x02029060 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0202907C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x020290DC = 0xE1A00000; // nop (dsiSaveCreateDir)
		setBL(0x020290FC, (u32)dsiSaveCreate);
		setBL(0x02029130, (u32)dsiSaveOpen);
		setBL(0x02029148, (u32)dsiSaveSetLength);
		setBL(0x02029158, (u32)dsiSaveWrite);
		setBL(0x02029160, (u32)dsiSaveClose);
		setBL(0x0202917C, (u32)dsiSaveSetLength);
	}

	// Paul's Monster Adventure (USA)
	else if (strcmp(romTid, "KP9E") == 0 && saveOnFlashcard) {
		setBL(0x02047940, (u32)dsiSaveOpen);
		setBL(0x02047958, (u32)dsiSaveCreate);
		setBL(0x02047970, (u32)dsiSaveOpen);
		setBL(0x02047990, (u32)dsiSaveWrite);
		setBL(0x020479A0, (u32)dsiSaveClose);
		setBL(0x020479BC, (u32)dsiSaveClose);
		setBL(0x020479F8, (u32)dsiSaveOpen);
		setBL(0x02047A18, (u32)dsiSaveRead);
		setBL(0x02047A28, (u32)dsiSaveClose);
		setBL(0x02047A44, (u32)dsiSaveClose);
		setBL(0x02047AF4, (u32)dsiSaveCreate);
		setBL(0x02047B04, (u32)dsiSaveOpen);
		setBL(0x02047B30, (u32)dsiSaveClose);
		setBL(0x02047B5C, (u32)dsiSaveCreate);
		setBL(0x02047B6C, (u32)dsiSaveOpen);
		setBL(0x02047B98, (u32)dsiSaveClose);
	}

	// Paul's Shooting Adventure (USA)
	else if (strcmp(romTid, "KPJE") == 0 && saveOnFlashcard) {
		//*(u32*)0x0203A20C = 0xE12FFF1E; // bx lr
		setBL(0x02048524, (u32)dsiSaveOpen);
		setBL(0x0204853C, (u32)dsiSaveCreate);
		setBL(0x02048554, (u32)dsiSaveOpen);
		setBL(0x02048574, (u32)dsiSaveWrite);
		setBL(0x02048584, (u32)dsiSaveClose);
		setBL(0x020485A0, (u32)dsiSaveClose);
		setBL(0x020485DC, (u32)dsiSaveOpen);
		setBL(0x020485FC, (u32)dsiSaveRead);
		setBL(0x0204860C, (u32)dsiSaveClose);
		setBL(0x02048628, (u32)dsiSaveClose);
		setBL(0x020486D8, (u32)dsiSaveCreate);
		setBL(0x020486E8, (u32)dsiSaveOpen);
		setBL(0x02048714, (u32)dsiSaveClose);
		setBL(0x02048740, (u32)dsiSaveCreate);
		setBL(0x02048750, (u32)dsiSaveOpen);
		setBL(0x0204877C, (u32)dsiSaveClose);
	}

	// Paul's Shooting Adventure 2 (USA)
	else if (strcmp(romTid, "KUSE") == 0 && saveOnFlashcard) {
		setBL(0x0202EE44, (u32)dsiSaveOpen);
		setBL(0x0202EE5C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202EE74, (u32)dsiSaveOpen);
		setBL(0x0202EE94, (u32)dsiSaveWrite);
		setBL(0x0202EEA4, (u32)dsiSaveClose);
		setBL(0x0202EEB4, (u32)dsiSaveClose);
		setBL(0x0202EEF4, (u32)dsiSaveOpen);
		setBL(0x0202EF18, (u32)dsiSaveRead);
		setBL(0x0202EF28, (u32)dsiSaveClose);
		setBL(0x0202EF38, (u32)dsiSaveClose);
		setBL(0x0202F018, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202F028, (u32)dsiSaveOpen);
		setBL(0x0202F054, (u32)dsiSaveClose);
		setBL(0x0202F08C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202F09C, (u32)dsiSaveOpen);
		setBL(0x0202F0C8, (u32)dsiSaveClose);
		//*(u32*)0x0203A730 = 0xE3A00001; // mov r0, #1
	}

	// Peg Solitaire (USA)
	else if (strcmp(romTid, "KP8E") == 0 && saveOnFlashcard) {
		*(u32*)0x0203DDA8 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0204616C, (u32)dsiSaveOpen);
		setBL(0x02046184, (u32)dsiSaveCreate);
		setBL(0x0204618C, (u32)dsiSaveGetResultCode);
		setBL(0x020461A0, (u32)dsiSaveOpen);
		setBL(0x020461B0, (u32)dsiSaveGetResultCode);
		setBL(0x020461CC, (u32)dsiSaveCreate);
		setBL(0x020461DC, (u32)dsiSaveOpen);
		setBL(0x020461E8, (u32)dsiSaveClose);
		setBL(0x02046324, (u32)dsiSaveGetInfo);
		setBL(0x02046334, (u32)dsiSaveOpen);
		setBL(0x0204635C, (u32)dsiSaveClose);
		setBL(0x02046394, (u32)dsiSaveSeek);
		setBL(0x020463A4, (u32)dsiSaveWrite);
		setBL(0x020463DC, (u32)dsiSaveOpen);
		setBL(0x02046400, (u32)dsiSaveClose);
		setBL(0x02046438, (u32)dsiSaveSeek);
		setBL(0x02046448, (u32)dsiSaveRead);
	}

	// Peg Solitaire (Europe)
	else if (strcmp(romTid, "KP8P") == 0 && saveOnFlashcard) {
		*(u32*)0x0203E03C = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02046400, (u32)dsiSaveOpen);
		setBL(0x02046410, (u32)dsiSaveGetResultCode);
		setBL(0x0204642C, (u32)dsiSaveCreate);
		setBL(0x02046444, (u32)dsiSaveOpen);
		setBL(0x02046454, (u32)dsiSaveGetResultCode);
		setBL(0x02046470, (u32)dsiSaveCreate);
		setBL(0x02046480, (u32)dsiSaveOpen);
		setBL(0x02046494, (u32)dsiSaveGetLength);
		setBL(0x020464A4, (u32)dsiSaveClose);
		setBL(0x020465E0, (u32)dsiSaveGetInfo);
		setBL(0x020465F0, (u32)dsiSaveOpen);
		setBL(0x02046618, (u32)dsiSaveClose);
		setBL(0x02046650, (u32)dsiSaveSeek);
		setBL(0x02046660, (u32)dsiSaveWrite);
		setBL(0x02046698, (u32)dsiSaveOpen);
		setBL(0x020466BC, (u32)dsiSaveClose);
		setBL(0x020466F4, (u32)dsiSaveSeek);
		setBL(0x02046704, (u32)dsiSaveRead);
	}

	// Petz Catz: Family (USA)
	else if (strcmp(romTid, "KP5E") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x020A721C;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveGetInfoT = 0x020A722C;
		*(u16*)dsiSaveGetInfoT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetInfoT + 4), dsiSaveGetInfo, 0xC);

		const u32 dsiSaveOpenT = 0x020A723C;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020A724C;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x020A725C;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x020A726C;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020A727C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		setBLThumb(0x0203810A, dsiSaveOpenT);
		setBLThumb(0x02038152, dsiSaveCloseT);
		setBLThumb(0x02038176, dsiSaveGetInfoT);
		setBLThumb(0x0203819C, dsiSaveCreateT);
		setBLThumb(0x020381BE, dsiSaveSeekT);
		setBLThumb(0x020381D4, dsiSaveReadT);
		setBLThumb(0x020381F6, dsiSaveSeekT);
		setBLThumb(0x0203820C, dsiSaveWriteT);
		tonccpy((u32*)0x020A67EC, dsiSaveGetResultCode, 0xC);
	}

	// Petz Cat: Superstar (Europe, Australia)
	else if (strcmp(romTid, "KP5V") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x020A7264;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveGetInfoT = 0x020A7274;
		*(u16*)dsiSaveGetInfoT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetInfoT + 4), dsiSaveGetInfo, 0xC);

		const u32 dsiSaveOpenT = 0x020A7284;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020A7294;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x020A72A4;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x020A72B4;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020A72C4;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		setBLThumb(0x02038172, dsiSaveOpenT);
		setBLThumb(0x020381BA, dsiSaveCloseT);
		setBLThumb(0x020381DE, dsiSaveGetInfoT);
		setBLThumb(0x02038204, dsiSaveCreateT);
		setBLThumb(0x02038226, dsiSaveSeekT);
		setBLThumb(0x0203823C, dsiSaveReadT);
		setBLThumb(0x0203825E, dsiSaveSeekT);
		setBLThumb(0x02038274, dsiSaveWriteT);
		tonccpy((u32*)0x020A6834, dsiSaveGetResultCode, 0xC);
	}

	// GO Series: Picdun (USA)
	// GO Series: Picdun (Europe)
	else if ((strcmp(romTid, "KPQE") == 0 || strcmp(romTid, "KPQP") == 0) && saveOnFlashcard) {
		setBL(0x0200B038, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0200B088, (u32)dsiSaveOpen);
		setBL(0x0200B11C, (u32)dsiSaveGetLength);
		setBL(0x0200B138, (u32)dsiSaveRead);
		setBL(0x0200B140, (u32)dsiSaveClose);
		setBL(0x0200B184, (u32)dsiSaveOpen);
		setBL(0x0200B20C, (u32)dsiSaveSeek);
		setBL(0x0200B288, (u32)dsiSaveClose);
		setBL(0x0200B2B0, (u32)dsiSaveWrite);
		setBL(0x0200B2BC, (u32)dsiSaveClose);
		tonccpy((u32*)0x02064E50, dsiSaveGetResultCode, 0xC);
	}

	// Danjo RPG: Picudan (Japan)
	else if (strcmp(romTid, "KPQJ") == 0 && saveOnFlashcard) {
		setBL(0x02009F24, (u32)dsiSaveOpen);
		setBL(0x02009F8C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02009FD8, (u32)dsiSaveGetLength);
		setBL(0x02009FF4, (u32)dsiSaveRead);
		setBL(0x02009FFC, (u32)dsiSaveClose);
		setBL(0x0200A040, (u32)dsiSaveOpen);
		setBL(0x0200A0C0, (u32)dsiSaveGetLength);
		setBL(0x0200A0D0, (u32)dsiSaveSeek);
		setBL(0x0200A14C, (u32)dsiSaveClose);
		setBL(0x0200A174, (u32)dsiSaveWrite);
		setBL(0x0200A180, (u32)dsiSaveClose);
		tonccpy((u32*)0x020627B8, dsiSaveGetResultCode, 0xC);
	}

	// Art Style: PiCTOBiTS (USA)
	// Art Style: PiCOPiCT (Europe, Australia)
	else if ((strcmp(romTid, "KAPE") == 0 || strcmp(romTid, "KAPV") == 0) && saveOnFlashcard) {
		setBL(0x02005828, (u32)dsiSaveOpen);
		setBL(0x020058E8, (u32)dsiSaveOpen);
		setBL(0x02005928, (u32)dsiSaveRead);
		setBL(0x0200595C, (u32)dsiSaveClose);
		//*(u32*)0x020059E4 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020059E8 = 0xE12FFF1E; // bx lr
		setBL(0x02005A18, (u32)dsiSaveCreate);
		setBL(0x02005A28, (u32)dsiSaveGetResultCode);
		setBL(0x02005A4C, (u32)dsiSaveOpen);
		setBL(0x02005A6C, (u32)dsiSaveSetLength);
		setBL(0x02005A88, (u32)dsiSaveWrite);
		setBL(0x02005AB4, (u32)dsiSaveClose);
	}

	// Art Style: PiCOPiCT (Japan)
	else if (strcmp(romTid, "KAPJ") == 0 && saveOnFlashcard) {
		setBL(0x020058B0, (u32)dsiSaveOpen);
		setBL(0x02005968, (u32)dsiSaveOpen);
		setBL(0x020059B0, (u32)dsiSaveRead);
		setBL(0x020059F4, (u32)dsiSaveClose);
		//*(u32*)0x02005A8C = 0xE3A00000; // mov r0, #0
		//*(u32*)0x02005A90 = 0xE12FFF1E; // bx lr
		setBL(0x02005ABC, (u32)dsiSaveCreate);
		setBL(0x02005ACC, (u32)dsiSaveGetResultCode);
		setBL(0x02005AE8, (u32)dsiSaveOpen);
		setBL(0x02005B04, (u32)dsiSaveSetLength);
		setBL(0x02005B20, (u32)dsiSaveWrite);
		setBL(0x02005B4C, (u32)dsiSaveClose);
	}

	// PictureBook Games: The Royal Bluff (USA)
	else if (strcmp(romTid, "KE3E") == 0 && saveOnFlashcard) {
		setBL(0x02048408, (u32)dsiSaveClose);
		setBL(0x02048584, (u32)dsiSaveClose);
		setBL(0x02048604, (u32)dsiSaveClose);
		setBL(0x020486F0, (u32)dsiSaveOpen);
		setBL(0x02048760, (u32)dsiSaveGetLength);
		setBL(0x0204880C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x020488BC, (u32)dsiSaveOpen);
		setBL(0x020488D4, (u32)dsiSaveClose);
		setBL(0x02048958, (u32)dsiSaveCreate);
		setBL(0x020489CC, (u32)dsiSaveOpen);
		setBL(0x02048A2C, (u32)dsiSaveSetLength);
		setBL(0x02048A68, (u32)dsiSaveWrite); // dsiSaveWriteAsync

		// Skip Manual screen
		*(u32*)0x0205F0C0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205F0D4 = 0xE3A00000; // mov r0, #0

		// Change help button (No blank file found to hide it)
		//const char* lp0 = "1p.dt0";
		//tonccpy((char*)0x0211281E, lp0, strlen(lp0)+1);
		//tonccpy((char*)0x02112844, lp0, strlen(lp0)+1);
	}

	// PictureBook Games: The Royal Bluff (Europe, Australia)
	else if (strcmp(romTid, "KE3V") == 0 && saveOnFlashcard) {
		setBL(0x020484B0, (u32)dsiSaveClose);
		setBL(0x0204862C, (u32)dsiSaveClose);
		setBL(0x020486AC, (u32)dsiSaveClose);
		setBL(0x02048798, (u32)dsiSaveOpen);
		setBL(0x02048808, (u32)dsiSaveGetLength);
		setBL(0x020488B4, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02048964, (u32)dsiSaveOpen);
		setBL(0x0204897C, (u32)dsiSaveClose);
		setBL(0x02048A00, (u32)dsiSaveCreate);
		setBL(0x02048A74, (u32)dsiSaveOpen);
		setBL(0x02048AD4, (u32)dsiSaveSetLength);
		setBL(0x02048B10, (u32)dsiSaveWrite); // dsiSaveWriteAsync

		// Skip Manual screen
		*(u32*)0x0205F550 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205F564 = 0xE3A00000; // mov r0, #0

		// Change help button (No blank file found to hide it)
		//const char* lp0 = "t_1p.dt0";
		//tonccpy((char*)0x0211ABC6, lp0, strlen(lp0)+1);
		//tonccpy((char*)0x0211ABF0, lp0, strlen(lp0)+1);
	}

	// GO Series: Pinball Attack! (USA)
	// GO Series: Pinball Attack! (Europe)
	else if ((strcmp(romTid, "KPYE") == 0 || strcmp(romTid, "KPYP") == 0) && saveOnFlashcard) {
		// Skip Manual screen (Crashes)
		*(u32*)0x02040264 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204030C = 0xE1A00000; // nop
		*(u32*)0x02040318 = 0xE1A00000; // nop
		*(u32*)0x02040330 = 0xE1A00000; // nop

		setBL(0x02044A18, (u32)dsiSaveCreate);
		setBL(0x02044A28, (u32)dsiSaveGetResultCode);
		setBL(0x02044A48, (u32)dsiSaveCreate);
		setBL(0x02044A58, (u32)dsiSaveGetResultCode);
		setBL(0x02044A94, (u32)dsiSaveOpen);
		setBL(0x02044AA4, (u32)dsiSaveGetResultCode);
		setBL(0x02044AC4, (u32)dsiSaveSetLength);
		setBL(0x02044AE4, (u32)dsiSaveWrite);
		setBL(0x02044AF4, (u32)dsiSaveWrite);
		setBL(0x02044B04, (u32)dsiSaveWrite);
		setBL(0x02044B18, (u32)dsiSaveWrite);
		setBL(0x02044B28, (u32)dsiSaveWrite);
		setBL(0x02044B38, (u32)dsiSaveWrite);
		setBL(0x02044B40, (u32)dsiSaveClose);
		setBL(0x02044B88, (u32)dsiSaveOpen);
		setBL(0x02044BB8, (u32)dsiSaveGetLength);
		setBL(0x02044BD0, (u32)dsiSaveRead);
		setBL(0x02044BE4, (u32)dsiSaveRead);
		setBL(0x02044C10, (u32)dsiSaveRead);
		setBL(0x02044C20, (u32)dsiSaveRead);
		setBL(0x02044C30, (u32)dsiSaveRead);
		setBL(0x02044C40, (u32)dsiSaveRead);
		setBL(0x02044C48, (u32)dsiSaveClose);
	}

	// Pinball Attack! (Japan)
	else if (strcmp(romTid, "KPYJ") == 0 && saveOnFlashcard) {
		// Skip Manual screen (Crashes)
		*(u32*)0x0203FCEC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0203FD34 = 0xE1A00000; // nop
		*(u32*)0x0203FD3C = 0xE1A00000; // nop
		*(u32*)0x0203FD50 = 0xE1A00000; // nop

		setBL(0x02044410, (u32)dsiSaveCreate);
		setBL(0x02044420, (u32)dsiSaveGetResultCode);
		setBL(0x02044440, (u32)dsiSaveCreate);
		setBL(0x02044450, (u32)dsiSaveGetResultCode);
		setBL(0x0204448C, (u32)dsiSaveOpen);
		setBL(0x0204449C, (u32)dsiSaveGetResultCode);
		setBL(0x020444BC, (u32)dsiSaveSetLength);
		setBL(0x020444DC, (u32)dsiSaveWrite);
		setBL(0x020444EC, (u32)dsiSaveWrite);
		setBL(0x020444FC, (u32)dsiSaveWrite);
		setBL(0x02044510, (u32)dsiSaveWrite);
		setBL(0x02044520, (u32)dsiSaveWrite);
		setBL(0x02044528, (u32)dsiSaveClose);
		setBL(0x02044570, (u32)dsiSaveOpen);
		setBL(0x020445A0, (u32)dsiSaveGetLength);
		setBL(0x020445B8, (u32)dsiSaveRead);
		setBL(0x020445CC, (u32)dsiSaveRead);
		setBL(0x020445F8, (u32)dsiSaveRead);
		setBL(0x02044608, (u32)dsiSaveRead);
		setBL(0x02044618, (u32)dsiSaveRead);
		setBL(0x02044620, (u32)dsiSaveClose);
	}

	// Pirates Assault (USA)
	else if (strcmp(romTid, "KXAE") == 0 && saveOnFlashcard) {
		setBL(0x0204E8A4, (u32)dsiSaveGetInfo);
		setBL(0x0204E8B8, (u32)dsiSaveOpen);
		setBL(0x0204E8CC, (u32)dsiSaveCreate);
		setBL(0x0204E8DC, (u32)dsiSaveOpen);
		setBL(0x0204E8EC, (u32)dsiSaveGetResultCode);
		setBL(0x0204E908, (u32)dsiSaveCreate);
		setBL(0x0204E918, (u32)dsiSaveOpen);
		setBL(0x0204E964, (u32)dsiSaveSeek);
		setBL(0x0204E974, (u32)dsiSaveWrite);
		setBL(0x0204E9E0, (u32)dsiSaveSeek);
		setBL(0x0204E9F0, (u32)dsiSaveWrite);
		setBL(0x0204E9F8, (u32)dsiSaveClose);
		setBL(0x0204EA44, (u32)dsiSaveOpen);
		setBL(0x0204EAA4, (u32)dsiSaveSeek);
		setBL(0x0204EAB8, (u32)dsiSaveRead);
		setBL(0x0204EAE0, (u32)dsiSaveClose);
		setBL(0x0204EB7C, (u32)dsiSaveOpen);
		setBL(0x0204EB90, (u32)dsiSaveSeek);
		setBL(0x0204EBA0, (u32)dsiSaveWrite);
		setBL(0x0204EBA8, (u32)dsiSaveClose);
		setBL(0x0204EE38, (u32)dsiSaveGetInfo);
		setBL(0x0204EE48, (u32)dsiSaveOpen);
	}

	// Pirates Assault (Europe, Australia)
	else if (strcmp(romTid, "KXAV") == 0 && saveOnFlashcard) {
		setBL(0x02052368, (u32)dsiSaveGetInfo);
		setBL(0x0205237C, (u32)dsiSaveOpen);
		setBL(0x02052390, (u32)dsiSaveCreate);
		setBL(0x020523A0, (u32)dsiSaveOpen);
		setBL(0x020523B0, (u32)dsiSaveGetResultCode);
		setBL(0x020523CC, (u32)dsiSaveCreate);
		setBL(0x020523DC, (u32)dsiSaveOpen);
		setBL(0x02052428, (u32)dsiSaveSeek);
		setBL(0x02052438, (u32)dsiSaveWrite);
		setBL(0x020524A4, (u32)dsiSaveSeek);
		setBL(0x020524B4, (u32)dsiSaveWrite);
		setBL(0x020524BC, (u32)dsiSaveClose);
		setBL(0x02052508, (u32)dsiSaveOpen);
		setBL(0x02052568, (u32)dsiSaveSeek);
		setBL(0x0205257C, (u32)dsiSaveRead);
		setBL(0x020525A4, (u32)dsiSaveClose);
		setBL(0x02052640, (u32)dsiSaveOpen);
		setBL(0x02052654, (u32)dsiSaveSeek);
		setBL(0x02052664, (u32)dsiSaveWrite);
		setBL(0x0205266C, (u32)dsiSaveClose);
		setBL(0x020528CC, (u32)dsiSaveGetInfo);
		setBL(0x020528DC, (u32)dsiSaveOpen);
	}

	// Plants vs. Zombies (USA)
	else if (strcmp(romTid, "KZLE") == 0 && saveOnFlashcard) {
		setBL(0x02099244, (u32)dsiSaveOpen);
		setBL(0x02099268, (u32)dsiSaveGetLength);
		setBL(0x0209927C, (u32)dsiSaveRead);
		setBL(0x020992AC, (u32)dsiSaveClose);
		setBL(0x02099324, (u32)dsiSaveOpen);
		setBL(0x02099350, (u32)dsiSaveSetLength);
		setBL(0x02099374, (u32)dsiSaveWrite);
		setBL(0x02099390, (u32)dsiSaveClose);
		*(u32*)0x020993C8 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x020993D4, (u32)dsiSaveCreate);
		//*(u32*)0x020C2F94 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x020F8B24, dsiSaveGetResultCode, 0xC);
	}

	// Plants vs. Zombies (Europe, Australia)
	else if (strcmp(romTid, "KZLV") == 0 && saveOnFlashcard) {
		setBL(0x02099AB0, (u32)dsiSaveOpen);
		setBL(0x02099AD4, (u32)dsiSaveGetLength);
		setBL(0x02099AE8, (u32)dsiSaveRead);
		setBL(0x02099B18, (u32)dsiSaveClose);
		setBL(0x02099B90, (u32)dsiSaveOpen);
		setBL(0x02099BBC, (u32)dsiSaveSetLength);
		setBL(0x02099BE0, (u32)dsiSaveWrite);
		setBL(0x02099BFC, (u32)dsiSaveClose);
		*(u32*)0x02099C34 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x02099C40, (u32)dsiSaveCreate);
		//*(u32*)0x020C41F8 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x020FA794, dsiSaveGetResultCode, 0xC);
	}

	// GO Series: Portable Shrine Wars (USA)
	// GO Series: Portable Shrine Wars (Europe)
	else if ((strcmp(romTid, "KOQE") == 0 || strcmp(romTid, "KOQP") == 0) && saveOnFlashcard) {
		setBL(0x0200CEC4, (u32)dsiSaveCreate);
		setBL(0x0200CF00, (u32)dsiSaveOpen);
		setBL(0x0200CF38, (u32)dsiSaveSetLength);
		setBL(0x0200CF48, (u32)dsiSaveWrite);
		setBL(0x0200CF60, (u32)dsiSaveClose);
		setBL(0x0200CFE8, (u32)dsiSaveOpen);
		setBL(0x0200D020, (u32)dsiSaveSetLength);
		setBL(0x0200D030, (u32)dsiSaveWrite);
		setBL(0x0200D048, (u32)dsiSaveClose);
		setBL(0x0200D0C8, (u32)dsiSaveOpen);
		setBL(0x0200D100, (u32)dsiSaveRead);
		setBL(0x0200D114, (u32)dsiSaveClose);
		setBL(0x0200D164, (u32)dsiSaveDelete);
		setBL(0x0200D1D0, (u32)dsiSaveGetInfo);
		*(u32*)0x0200D214 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0200D218 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E004 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		tonccpy((u32*)0x0204ED3C, dsiSaveGetResultCode, 0xC);

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200DE78;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Art Style: precipice (USA)
	// Art Style: KUBOS (Europe, Australia)
	// Art Style: nalaku (Japan)
	else if ((strncmp(romTid, "KAK", 3) == 0) && saveOnFlashcard) {
		setBL(0x020075A8, (u32)dsiSaveOpen);
		setBL(0x02007668, (u32)dsiSaveOpen);
		setBL(0x020076AC, (u32)dsiSaveRead);
		setBL(0x020076E0, (u32)dsiSaveClose);
		//*(u32*)0x02007768 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200776C = 0xE12FFF1E; // bx lr
		setBL(0x0200779C, (u32)dsiSaveCreate);
		setBL(0x020077AC, (u32)dsiSaveGetResultCode);
		setBL(0x020077CC, (u32)dsiSaveOpen);
		setBL(0x020077EC, (u32)dsiSaveSetLength);
		setBL(0x02007808, (u32)dsiSaveWrite);
		setBL(0x02007834, (u32)dsiSaveClose);
	}

	// Prehistorik Man (USA)
	else if (strcmp(romTid, "KPHE") == 0 && saveOnFlashcard) {
		setB(0x0204A0D4, 0x0204A39C); // Skip Manual screen
		setBL(0x0204D4C8, (u32)dsiSaveOpen);
		setBL(0x0204D4E0, (u32)dsiSaveGetLength);
		setBL(0x0204D4F8, (u32)dsiSaveRead);
		setBL(0x0204D500, (u32)dsiSaveClose);
		setBL(0x0204D540, (u32)dsiSaveGetInfo);
		setBL(0x0204D550, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204D574 = 0xE1A00000; // nop
		setBL(0x0204D59C, (u32)dsiSaveCreate);
		setBL(0x0204D5A4, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204D5C8 = 0xE1A00000; // nop
		setBL(0x0204D5F8, (u32)dsiSaveOpen);
		setBL(0x0204D608, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204D620 = 0xE1A00000; // nop
		setBL(0x0204D64C, (u32)dsiSaveSetLength);
		setBL(0x0204D65C, (u32)dsiSaveWrite);
		setBL(0x0204D664, (u32)dsiSaveClose);
	}

	// Prehistorik Man (Europe, Australia)
	else if (strcmp(romTid, "KPHV") == 0 && saveOnFlashcard) {
		setB(0x0204A100, 0x0204A398); // Skip Manual screen
		setBL(0x020624A4, (u32)dsiSaveOpen);
		setBL(0x020624BC, (u32)dsiSaveGetLength);
		setBL(0x020624D4, (u32)dsiSaveRead);
		setBL(0x020624DC, (u32)dsiSaveClose);
		setBL(0x0206251C, (u32)dsiSaveGetInfo);
		setBL(0x0206252C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02062550 = 0xE1A00000; // nop
		setBL(0x02062578, (u32)dsiSaveCreate);
		setBL(0x02062580, (u32)dsiSaveGetResultCode);
		*(u32*)0x020625A4 = 0xE1A00000; // nop
		setBL(0x020625D4, (u32)dsiSaveOpen);
		setBL(0x020625E4, (u32)dsiSaveGetResultCode);
		*(u32*)0x020625FC = 0xE1A00000; // nop
		setBL(0x02062628, (u32)dsiSaveSetLength);
		setBL(0x02062638, (u32)dsiSaveWrite);
		setBL(0x02062640, (u32)dsiSaveClose);
	}

	// Pro-Putt Domo (USA)
	else if (strcmp(romTid, "KDPE") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x020270FC;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x0202710C;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x0202711C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x0202712C;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x0202713C;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x0202714C;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x0202715C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x020106BC = 0x2001; // movs r0, #1
		*(u16*)0x020106BE = 0x4770; // bx lr
		*(u16*)0x020109AC = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x020109AE = 0x4770; // bx lr
		setBLThumb(0x02010A12, dsiSaveCreateT);
		setBLThumb(0x02010A28, dsiSaveOpenT);
		setBLThumb(0x02010A44, dsiSaveSetLengthT);
		setBLThumb(0x02010A58, dsiSaveWriteT);
		setBLThumb(0x02010A6A, dsiSaveCloseT);
		*(u16*)0x02010A90 = 0x4778; // bx pc
		tonccpy((u32*)0x02010A90, dsiSaveGetLength, 0xC);
		setBLThumb(0x02010AC0, dsiSaveOpenT);
		setBLThumb(0x02010AE6, dsiSaveCloseT);
		setBLThumb(0x02010AF8, dsiSaveReadT);
		setBLThumb(0x02010AFE, dsiSaveCloseT);
		setBLThumb(0x02010B12, dsiSaveDeleteT);
		*(u16*)0x020179F4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Puzzle to Go: Baby Animals (Europe)
	else if (strcmp(romTid, "KBYP") == 0 && saveOnFlashcard) {
		// Skip Manual screen
		*(u32*)0x02032AFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02032B80 = 0xE1A00000; // nop
		*(u32*)0x02032B88 = 0xE1A00000; // nop
		*(u32*)0x02032B94 = 0xE1A00000; // nop

		setBL(0x02036570, (u32)dsiSaveCreate);
		setBL(0x02036580, (u32)dsiSaveOpen);
		setBL(0x02036590, (u32)dsiSaveGetResultCode);
		setBL(0x020365AC, (u32)dsiSaveSetLength);
		setBL(0x020365BC, (u32)dsiSaveWrite);
		setBL(0x020365C4, (u32)dsiSaveClose);
		setBL(0x020365FC, (u32)dsiSaveOpen);
		setBL(0x0203660C, (u32)dsiSaveGetResultCode);
		setBL(0x02036624, (u32)dsiSaveGetLength);
		setBL(0x02036634, (u32)dsiSaveRead);
		setBL(0x0203663C, (u32)dsiSaveClose);
		setBL(0x02036674, (u32)dsiSaveOpen);
		setBL(0x02036684, (u32)dsiSaveGetResultCode);
		setBL(0x0203669C, (u32)dsiSaveClose);
	}

	// Puzzle to Go: Diddl (Europe)
	// Puzzle to Go: Wildlife (Europe)
	else if ((strcmp(romTid, "KPUP") == 0 || strcmp(romTid, "KPDP") == 0) && saveOnFlashcard) {
		setBL(0x0203EA60, (u32)dsiSaveCreate);
		setBL(0x0203EA7C, (u32)dsiSaveOpen);
		setBL(0x0203EA8C, (u32)dsiSaveGetResultCode);
		setBL(0x0203EAA8, (u32)dsiSaveSetLength);
		setBL(0x0203EAB8, (u32)dsiSaveWrite);
		setBL(0x0203EAC0, (u32)dsiSaveClose);
		setBL(0x0203EAF8, (u32)dsiSaveOpen);
		setBL(0x0203EB08, (u32)dsiSaveGetResultCode);
		setBL(0x0203EB20, (u32)dsiSaveGetLength);
		setBL(0x0203EB30, (u32)dsiSaveRead);
		setBL(0x0203EB38, (u32)dsiSaveClose);
		setBL(0x0203EB70, (u32)dsiSaveOpen);
		setBL(0x0203EB80, (u32)dsiSaveGetResultCode);
		setBL(0x0203EB98, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x0203ECC4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0203ED28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203ED4C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0203ED80 = 0xE1A00000; // nop
		*(u32*)0x0203ED8C = 0xE1A00000; // nop
	}

	// Puzzle to Go: Planets and Universe (Europe)
	// Puzzle to Go: Sightseeing (Europe)
	else if ((strcmp(romTid, "KBXP") == 0 || strcmp(romTid, "KB3P") == 0) && saveOnFlashcard) {
		// Skip Manual screen
		*(u32*)0x02032BF0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02032C74 = 0xE1A00000; // nop
		*(u32*)0x02032C7C = 0xE1A00000; // nop
		*(u32*)0x02032C88 = 0xE1A00000; // nop

		setBL(0x02036680, (u32)dsiSaveCreate);
		setBL(0x02036690, (u32)dsiSaveOpen);
		setBL(0x020366A0, (u32)dsiSaveGetResultCode);
		setBL(0x020366BC, (u32)dsiSaveSetLength);
		setBL(0x020366CC, (u32)dsiSaveWrite);
		setBL(0x020366D4, (u32)dsiSaveClose);
		setBL(0x0203670C, (u32)dsiSaveOpen);
		setBL(0x0203671C, (u32)dsiSaveGetResultCode);
		setBL(0x02036734, (u32)dsiSaveGetLength);
		setBL(0x02036744, (u32)dsiSaveRead);
		setBL(0x0203674C, (u32)dsiSaveClose);
		setBL(0x02036784, (u32)dsiSaveOpen);
		setBL(0x02036794, (u32)dsiSaveGetResultCode);
		setBL(0x020367AC, (u32)dsiSaveClose);
	}

	// Quick Fill Q (USA)
	// Quick Fill Q (Europe)
	// A bit hard/confusing to add save support
	else if ((strcmp(romTid, "KUME") == 0 || strcmp(romTid, "KUMP") == 0) && saveOnFlashcard) {
		//*(u32*)0x0203EA70 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02040240 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rabi Laby (USA)
	// Rabi Laby (Europe)
	else if ((strcmp(romTid, "KLBE") == 0 || strcmp(romTid, "KLBP") == 0) && saveOnFlashcard) {
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020053A8 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0201DEF8, (u32)dsiSaveOpen);
		setBL(0x0201DF1C, (u32)dsiSaveRead);
		setBL(0x0201DF2C, (u32)dsiSaveRead);
		setBL(0x0201DF34, (u32)dsiSaveClose);
		setBL(0x0201E1E4, (u32)dsiSaveOpen);
		setBL(0x0201E354, (u32)dsiSaveWrite);
		setBL(0x0201E35C, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x02026648, (u32)dsiSaveCreate);
			setBL(0x02026704, (u32)dsiSaveCreate);
		} else {
			setBL(0x020266D8, (u32)dsiSaveCreate);
			setBL(0x02026794, (u32)dsiSaveCreate);
		}
	}

	// Akushon Pazuru: Rabi x Rabi (Japan)
	else if (strcmp(romTid, "KLBJ") == 0 && saveOnFlashcard) {
		*(u32*)0x02005190 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02005360 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0201DD80, (u32)dsiSaveOpen);
		setBL(0x0201DDA4, (u32)dsiSaveRead);
		setBL(0x0201DDB4, (u32)dsiSaveRead);
		setBL(0x0201DDBC, (u32)dsiSaveClose);
		setBL(0x0201E1E4, (u32)dsiSaveOpen);
		setBL(0x0201E354, (u32)dsiSaveWrite);
		setBL(0x0201E35C, (u32)dsiSaveClose);
		setBL(0x0201E06C, (u32)dsiSaveOpen);
		setBL(0x0201E1DC, (u32)dsiSaveWrite);
		setBL(0x0201E1E4, (u32)dsiSaveClose);
		setBL(0x02026BDC, (u32)dsiSaveCreate);
		setBL(0x02026CC0, (u32)dsiSaveCreate);
	}

	// Rabi Laby 2 (USA)
	// Rabi Laby 2 (Europe)
	// Akushon Pazuru: Rabi x Rabi Episodo 2 (Japan)
	else if (strncmp(romTid, "KLV", 3) == 0 && saveOnFlashcard) {
		*(u32*)0x020051E8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200540C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0203571C = 0xE1A00000; // nop
		*(u32*)0x02035720 = 0xE1A00000; // nop
		*(u32*)0x02035724 = 0xE1A00000; // nop
		// The non-save branches are patching out "crc.dat" R/Ws
		setBL(0x02035738, (u32)dsiSaveOpen);
		*(u32*)0x0203573C = 0xE1A00000; // nop
		*(u32*)0x02035740 = 0xE1A00000; // nop
		*(u32*)0x02035744 = 0xE1A00000; // nop
		*(u32*)0x02035748 = 0xE1A00000; // nop
		*(u32*)0x0203574C = 0xE1A00000; // nop
		*(u32*)0x02035750 = 0xE1A00000; // nop
		*(u32*)0x02035758 = 0x028DDFA5; // addeq sp, sp, #0x294
		*(u32*)0x0203575C = 0x08BD81F8; // ldmeqfd sp!, {r3-r8,pc}
		*(u32*)0x02035760 = 0xE1A00000; // nop
		*(u32*)0x02035764 = 0xE1A00000; // nop
		*(u32*)0x02035768 = 0xE1A00000; // nop
		*(u32*)0x0203576C = 0xE1A00000; // nop
		setBL(0x02035784, (u32)dsiSaveRead);
		*(u32*)0x02035788 = 0xE1A00000; // nop
		*(u32*)0x0203578C = 0xE1A00000; // nop
		*(u32*)0x02035790 = 0xE1A00000; // nop
		*(u32*)0x02035794 = 0xE1A00000; // nop
		setBL(0x0203579C, (u32)dsiSaveClose);
		*(u32*)0x020357A0 = 0xE1A00000; // nop
		*(u32*)0x020357A4 = 0xE1A00000; // nop
		*(u32*)0x02035810 = 0xE1A00000; // nop
		*(u32*)0x02035814 = 0xE1A00000; // nop
		*(u32*)0x02035818 = 0xE1A00000; // nop
		setBL(0x0203582C, (u32)dsiSaveOpen);
		*(u32*)0x020357DC = 0xE1A00000; // nop
		*(u32*)0x020357E0 = 0xE3A04000; // mov r4, #0
		*(u32*)0x02035840 = 0xE1A00000; // nop
		*(u32*)0x02035844 = 0xE1A00000; // nop
		*(u32*)0x02035848 = 0xE1A00000; // nop
		*(u32*)0x0203584C = 0xE1A00000; // nop
		*(u32*)0x02035850 = 0xE1A00000; // nop
		*(u32*)0x02035854 = 0xE1A00000; // nop
		*(u32*)0x02035858 = 0xE1A00000; // nop
		*(u32*)0x0203585C = 0xE1A00000; // nop
		setBL(0x020358A8, (u32)dsiSaveWrite);
		*(u32*)0x020358AC = 0xE1A00000; // nop
		*(u32*)0x020358B0 = 0xE1A00000; // nop
		*(u32*)0x020358B4 = 0xE1A00000; // nop
		*(u32*)0x020358B8 = 0xE1A00000; // nop
		setBL(0x020358C0, (u32)dsiSaveClose);
		*(u32*)0x020358C4 = 0xE1A00000; // nop
		*(u32*)0x020358C8 = 0xE1A00000; // nop
		setBL(0x020358F8, (u32)dsiSaveCreate);
		*(u32*)0x020358FC = 0xE1A00000; // nop
		*(u32*)0x02035900 = 0xE1A00000; // nop
		*(u32*)0x02035904 = 0xE1A00000; // nop
	}

	// Real Crimes: Jack the Ripper (USA)
	else if (strcmp(romTid, "KRCE") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x020119A0;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x020119B0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020119C0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x020119D0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x020119E0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020119F0;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		setBLThumb(0x02026358, dsiSaveOpenT);
		setBLThumb(0x02026370, dsiSaveSeekT);
		setBLThumb(0x02026382, dsiSaveReadT);
		setBLThumb(0x02026398, dsiSaveReadT);
		setBLThumb(0x020263C0, dsiSaveCloseT);
		doubleNopT(0x020263DA);
		setBLThumb(0x0202642E, dsiSaveOpenT);
		setBLThumb(0x0202644E, dsiSaveSeekT);
		setBLThumb(0x02026460, dsiSaveReadT);
		setBLThumb(0x02026476, dsiSaveReadT);
		setBLThumb(0x0202649E, dsiSaveCloseT);
		doubleNopT(0x020264B6);
		setBLThumb(0x020264F8, dsiSaveOpenT);
		setBLThumb(0x0202650C, dsiSaveCloseT);
		doubleNopT(0x0202651E);
		setBLThumb(0x02026558, dsiSaveOpenT);
		setBLThumb(0x0202656A, dsiSaveCloseT);
		doubleNopT(0x0202657C);
		*(u16*)0x020265B6 = 0x2001; // movs r0, #1 (dsiSaveOpenDir)
		*(u16*)0x020265B8 = 0x46C0; // nop
		doubleNopT(0x020265C0); // dsiSaveCloseDir
		setBLThumb(0x020265CC, dsiSaveOpenT);
		setBLThumb(0x020265D6, dsiSaveCloseT);
		setBLThumb(0x02026656, dsiSaveCreateT);
		setBLThumb(0x02026666, dsiSaveOpenT);
		setBLThumb(0x02026674, dsiSaveWriteT);
		setBLThumb(0x020266DA, dsiSaveCloseT);
		doubleNopT(0x020266EE);
		setBLThumb(0x02026754, dsiSaveSeekT);
		setBLThumb(0x0202675E, dsiSaveWriteT);
		setBLThumb(0x02026774, dsiSaveWriteT);
		setBLThumb(0x020267AA, dsiSaveSeekT);
		setBLThumb(0x020267B4, dsiSaveWriteT);
		setBLThumb(0x020267CA, dsiSaveWriteT);
		*(u16*)0x020471B4 = 0x2100; // movs r1, #0 (Skip Manual screen)
	}

	// Real Crimes: Jack the Ripper (Europe, Australia)
	else if (strcmp(romTid, "KRCV") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x020119A4;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x020119B4;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020119C4;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x020119D4;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x020119E4;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020119F4;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		setBLThumb(0x02026338, dsiSaveOpenT);
		setBLThumb(0x0202634E, dsiSaveSeekT);
		setBLThumb(0x02026360, dsiSaveReadT);
		setBLThumb(0x02026376, dsiSaveReadT);
		setBLThumb(0x0202639E, dsiSaveCloseT);
		doubleNopT(0x020263B8);
		setBLThumb(0x0202640E, dsiSaveOpenT);
		setBLThumb(0x0202642C, dsiSaveSeekT);
		setBLThumb(0x0202643E, dsiSaveReadT);
		setBLThumb(0x02026454, dsiSaveReadT);
		setBLThumb(0x0202647C, dsiSaveCloseT);
		doubleNopT(0x02026494);
		setBLThumb(0x020264D8, dsiSaveOpenT);
		setBLThumb(0x020264EC, dsiSaveCloseT);
		doubleNopT(0x020264FE);
		setBLThumb(0x02026538, dsiSaveOpenT);
		setBLThumb(0x0202654A, dsiSaveCloseT);
		doubleNopT(0x0202655C);
		*(u16*)0x02026596 = 0x2001; // movs r0, #1 (dsiSaveOpenDir)
		*(u16*)0x02026598 = 0x46C0; // nop
		doubleNopT(0x020265A0); // dsiSaveCloseDir
		setBLThumb(0x020265AC, dsiSaveOpenT);
		setBLThumb(0x020265B6, dsiSaveCloseT);
		setBLThumb(0x02026636, dsiSaveCreateT);
		setBLThumb(0x02026646, dsiSaveOpenT);
		setBLThumb(0x020266B0, dsiSaveCloseT);
		doubleNopT(0x020266C4);
		setBLThumb(0x02026726, dsiSaveSeekT);
		setBLThumb(0x02026730, dsiSaveWriteT);
		setBLThumb(0x02026746, dsiSaveWriteT);
		setBLThumb(0x0202677C, dsiSaveSeekT);
		setBLThumb(0x02026786, dsiSaveWriteT);
		setBLThumb(0x0202679C, dsiSaveWriteT);
		*(u16*)0x020471E4 = 0x2100; // movs r1, #0 (Skip Manual screen)
	}

	// Redau Shirizu: Gunjin Shougi (Japan)
	else if (strcmp(romTid, "KLXJ") == 0 && saveOnFlashcard) {
		*(u32*)0x02005254 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0203342C, (u32)dsiSaveCreate);
		setBL(0x0203343C, (u32)dsiSaveOpen);
		setBL(0x02033468, (u32)dsiSaveWrite);
		setBL(0x02033478, (u32)dsiSaveClose);
		setBL(0x02033494, (u32)dsiSaveClose);
		setBL(0x02033500, (u32)dsiSaveOpen);
		setBL(0x02033510, (u32)dsiSaveGetLength);
		setBL(0x02033528, (u32)dsiSaveRead);
		setBL(0x0203356C, (u32)dsiSaveClose);
		setBL(0x02033588, (u32)dsiSaveClose);
	}

	// Remote Racers (USA)
	else if (strcmp(romTid, "KQRE") == 0 && saveOnFlashcard) {
		setBL(0x0209A074, (u32)dsiSaveClose);
		setBL(0x0209A0D0, (u32)dsiSaveClose);
		setBL(0x0209A178, (u32)dsiSaveOpen);
		setBL(0x0209A190, (u32)dsiSaveSeek);
		setBL(0x0209A1A4, (u32)dsiSaveRead);
		setBL(0x0209A244, (u32)dsiSaveCreate);
		setBL(0x0209A274, (u32)dsiSaveOpen);
		setBL(0x0209A2A4, (u32)dsiSaveSetLength);
		setBL(0x0209A2CC, (u32)dsiSaveSeek);
		setBL(0x0209A2E0, (u32)dsiSaveWrite);
		setBL(0x0209A390, (u32)dsiSaveCreate);
		setBL(0x0209A3C8, (u32)dsiSaveOpen);
		setBL(0x0209A400, (u32)dsiSaveSetLength);
		setBL(0x0209A41C, (u32)dsiSaveSeek);
		setBL(0x0209A430, (u32)dsiSaveWrite);
		setBL(0x0209A590, (u32)dsiSaveSeek);
		setBL(0x0209A5A0, (u32)dsiSaveWrite);
		setBL(0x0209A718, (u32)dsiSaveGetResultCode);
		*(u32*)0x0209A75C = 0xE3A00000; // mov r0, #0
	}

	// Remote Racers (Europe, Australia)
	else if (strcmp(romTid, "KQRV") == 0 && saveOnFlashcard) {
		setBL(0x0209A338, (u32)dsiSaveClose);
		setBL(0x0209A394, (u32)dsiSaveClose);
		setBL(0x0209A43C, (u32)dsiSaveOpen);
		setBL(0x0209A454, (u32)dsiSaveSeek);
		setBL(0x0209A468, (u32)dsiSaveRead);
		setBL(0x0209A508, (u32)dsiSaveCreate);
		setBL(0x0209A538, (u32)dsiSaveOpen);
		setBL(0x0209A568, (u32)dsiSaveSetLength);
		setBL(0x0209A590, (u32)dsiSaveSeek);
		setBL(0x0209A5A4, (u32)dsiSaveWrite);
		setBL(0x0209A654, (u32)dsiSaveCreate);
		setBL(0x0209A68C, (u32)dsiSaveOpen);
		setBL(0x0209A6C4, (u32)dsiSaveSetLength);
		setBL(0x0209A6E0, (u32)dsiSaveSeek);
		setBL(0x0209A6F4, (u32)dsiSaveWrite);
		setBL(0x0209A854, (u32)dsiSaveSeek);
		setBL(0x0209A864, (u32)dsiSaveWrite);
		setBL(0x0209A9DC, (u32)dsiSaveGetResultCode);
		*(u32*)0x0209AA20 = 0xE3A00000; // mov r0, #0
	}

	// Renjuku Kanji: Shougaku 1 Nensei (Japan)
	else if (strcmp(romTid, "KJZJ") == 0 && saveOnFlashcard) {
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		*(u32*)0x02049A64 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x020532B8 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x02064ED0, (u32)dsiSaveOpen);
		setBL(0x02064EEC, (u32)dsiSaveSeek);
		setBL(0x02064F00, (u32)dsiSaveClose);
		setBL(0x02064F18, (u32)dsiSaveRead);
		setBL(0x02064F28, (u32)dsiSaveClose);
		setBL(0x02064F34, (u32)dsiSaveClose);
		setBL(0x02064F68, (u32)dsiSaveOpen);
		setBL(0x02064F80, (u32)dsiSaveSeek);
		setBL(0x02064F98, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02064FC8, (u32)dsiSaveOpen);
		setBL(0x02064FE0, (u32)dsiSaveSetLength);
		setBL(0x02064FF0, (u32)dsiSaveClose);
		setBL(0x02065004, (u32)dsiSaveSeek);
		setBL(0x02065018, (u32)dsiSaveClose);
		setBL(0x02065030, (u32)dsiSaveWrite);
		setBL(0x02065040, (u32)dsiSaveClose);
		setBL(0x0206504C, (u32)dsiSaveClose);
		setBL(0x02065080, (u32)dsiSaveOpen);
		setBL(0x02065094, (u32)dsiSaveSetLength);
		setBL(0x020650AC, (u32)dsiSaveSeek);
		setBL(0x020650C4, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x02065150 = 0xE12FFF1E; // bx lr
	}

	// Renjuku Kanji: Shougaku 2 Nensei (Japan)
	// Renjuku Kanji: Shougaku 3 Nensei (Japan)
	else if ((strcmp(romTid, "KJ2J") == 0 || strcmp(romTid, "KJ3J") == 0) && saveOnFlashcard) {
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		*(u32*)0x02049A4C = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x020532A0 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x02064EB8, (u32)dsiSaveOpen);
		setBL(0x02064ED4, (u32)dsiSaveSeek);
		setBL(0x02064EE8, (u32)dsiSaveClose);
		setBL(0x02064F00, (u32)dsiSaveRead);
		setBL(0x02064F10, (u32)dsiSaveClose);
		setBL(0x02064F1C, (u32)dsiSaveClose);
		setBL(0x02064F50, (u32)dsiSaveOpen);
		setBL(0x02064F68, (u32)dsiSaveSeek);
		setBL(0x02064F80, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02064FB0, (u32)dsiSaveOpen);
		setBL(0x02064FC8, (u32)dsiSaveSetLength);
		setBL(0x02064FD8, (u32)dsiSaveClose);
		setBL(0x02064FEC, (u32)dsiSaveSeek);
		setBL(0x02065000, (u32)dsiSaveClose);
		setBL(0x02065018, (u32)dsiSaveWrite);
		setBL(0x02065028, (u32)dsiSaveClose);
		setBL(0x02065034, (u32)dsiSaveClose);
		setBL(0x02065068, (u32)dsiSaveOpen);
		setBL(0x0206507C, (u32)dsiSaveSetLength);
		setBL(0x02065094, (u32)dsiSaveSeek);
		setBL(0x020650AC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x02065138 = 0xE12FFF1E; // bx lr
	}

	// Renjuku Kanji: Shougaku 4 Nensei (Japan)
	// Renjuku Kanji: Shougaku 5 Nensei (Japan)
	// Renjuku Kanji: Shougaku 6 Nensei (Japan)
	else if ((strcmp(romTid, "KJ4J") == 0 || strcmp(romTid, "KJ5J") == 0 || strcmp(romTid, "KJ6J") == 0) && saveOnFlashcard) {
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		*(u32*)0x02049AE8 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x0205333C = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x02064F54, (u32)dsiSaveOpen);
		setBL(0x02064F70, (u32)dsiSaveSeek);
		setBL(0x02064F84, (u32)dsiSaveClose);
		setBL(0x02064F9C, (u32)dsiSaveRead);
		setBL(0x02064FAC, (u32)dsiSaveClose);
		setBL(0x02064FB8, (u32)dsiSaveClose);
		setBL(0x02064FEC, (u32)dsiSaveOpen);
		setBL(0x02065004, (u32)dsiSaveSeek);
		setBL(0x0206501C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0206504C, (u32)dsiSaveOpen);
		setBL(0x02065064, (u32)dsiSaveSetLength);
		setBL(0x02065074, (u32)dsiSaveClose);
		setBL(0x02065088, (u32)dsiSaveSeek);
		setBL(0x0206509C, (u32)dsiSaveClose);
		setBL(0x020650B4, (u32)dsiSaveWrite);
		setBL(0x020650C4, (u32)dsiSaveClose);
		setBL(0x020650D0, (u32)dsiSaveClose);
		setBL(0x02065104, (u32)dsiSaveOpen);
		setBL(0x02065118, (u32)dsiSaveSetLength);
		setBL(0x02065130, (u32)dsiSaveSeek);
		setBL(0x02065148, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x020651D4 = 0xE12FFF1E; // bx lr
	}

	// Renjuku Kanji: Chuugakusei (Japan)
	else if (strcmp(romTid, "KJ8J") == 0 && saveOnFlashcard) {
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		*(u32*)0x02049A68 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x020532BC = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x02064ED4, (u32)dsiSaveOpen);
		setBL(0x02064EF0, (u32)dsiSaveSeek);
		setBL(0x02064F04, (u32)dsiSaveClose);
		setBL(0x02064F1C, (u32)dsiSaveRead);
		setBL(0x02064F2C, (u32)dsiSaveClose);
		setBL(0x02064F38, (u32)dsiSaveClose);
		setBL(0x02064F6C, (u32)dsiSaveOpen);
		setBL(0x02064F84, (u32)dsiSaveSeek);
		setBL(0x02064F9C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02064FCC, (u32)dsiSaveOpen);
		setBL(0x02064FE4, (u32)dsiSaveSetLength);
		setBL(0x02064FF4, (u32)dsiSaveClose);
		setBL(0x02065008, (u32)dsiSaveSeek);
		setBL(0x0206501C, (u32)dsiSaveClose);
		setBL(0x02065034, (u32)dsiSaveWrite);
		setBL(0x02065044, (u32)dsiSaveClose);
		setBL(0x02065050, (u32)dsiSaveClose);
		setBL(0x02065084, (u32)dsiSaveOpen);
		setBL(0x02065098, (u32)dsiSaveSetLength);
		setBL(0x020650B0, (u32)dsiSaveSeek);
		setBL(0x020650C8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x02065154 = 0xE12FFF1E; // bx lr
	}

	// Robot Rescue (USA)
	else if (strcmp(romTid, "KRTE") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0200C2DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2E0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C39C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C570 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C574 = 0xE12FFF1E; // bx lr */
		setBL(0x0200C594, (u32)dsiSaveOpen);
		setBL(0x0200C5A8, (u32)dsiSaveClose);
		setBL(0x0200C5C8, (u32)dsiSaveCreate);
		setBL(0x0200C5E0, (u32)dsiSaveOpen);
		setBL(0x0200C5F8, (u32)dsiSaveClose);
		setBL(0x0200C600, (u32)dsiSaveDelete);
		setBL(0x0200C76C, (u32)dsiSaveOpen);
		setBL(0x0200C784, (u32)dsiSaveGetLength);
		setBL(0x0200C7A8, (u32)dsiSaveRead);
		setBL(0x0200C7B0, (u32)dsiSaveClose);
		setBL(0x0200C838, (u32)dsiSaveOpen);
		setBL(0x0200C84C, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveCreate);
		setBL(0x0200C878, (u32)dsiSaveOpen);
		setBL(0x0200C894, (u32)dsiSaveClose);
		setBL(0x0200C89C, (u32)dsiSaveDelete);
		setBL(0x0200C8B0, (u32)dsiSaveCreate);
		setBL(0x0200C8C0, (u32)dsiSaveOpen);
		setBL(0x0200C904, (u32)dsiSaveSetLength);
		setBL(0x0200C914, (u32)dsiSaveWrite);
		setBL(0x0200C91C, (u32)dsiSaveClose);
		*(u32*)0x020108A4 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Robot Rescue (Europe, Australia)
	else if (strcmp(romTid, "KRTV") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0200C2CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C388 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C38C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C550 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C554 = 0xE12FFF1E; // bx lr */
		setBL(0x0200C57C, (u32)dsiSaveOpen);
		setBL(0x0200C590, (u32)dsiSaveClose);
		setBL(0x0200C5B0, (u32)dsiSaveCreate);
		setBL(0x0200C5CC, (u32)dsiSaveOpen);
		setBL(0x0200C5E4, (u32)dsiSaveClose);
		setBL(0x0200C5EC, (u32)dsiSaveDelete);
		setBL(0x0200C788, (u32)dsiSaveOpen);
		setBL(0x0200C7A0, (u32)dsiSaveGetLength);
		setBL(0x0200C7C4, (u32)dsiSaveRead);
		setBL(0x0200C7CC, (u32)dsiSaveClose);
		setBL(0x0200C810, (u32)dsiSaveOpen);
		setBL(0x0200C824, (u32)dsiSaveClose);
		setBL(0x0200C838, (u32)dsiSaveCreate);
		setBL(0x0200C854, (u32)dsiSaveOpen);
		*(u32*)0x0200C864 = 0xE1A00000; // nop
		setBL(0x0200C870, (u32)dsiSaveClose);
		setBL(0x0200C878, (u32)dsiSaveDelete);
		setBL(0x0200C890, (u32)dsiSaveCreate);
		setBL(0x0200C8A0, (u32)dsiSaveOpen);
		*(u32*)0x0200C8CC = 0xE1A00000; // nop (dsiSaveSetLength)
		setBL(0x0200C8DC, (u32)dsiSaveWrite);
		setBL(0x0200C8E4, (u32)dsiSaveClose);
		*(u32*)0x02010C30 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// ARC Style: Robot Rescue (Japan)
	else if (strcmp(romTid, "KRTJ") == 0 && saveOnFlashcard) {
		/* *(u32*)0x0200F460 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F464 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F51C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F520 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200F6E4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200F6E8 = 0xE12FFF1E; // bx lr */
		setBL(0x0200F710, (u32)dsiSaveOpen);
		setBL(0x0200F724, (u32)dsiSaveClose);
		setBL(0x0200F724, (u32)dsiSaveCreate);
		setBL(0x0200F760, (u32)dsiSaveOpen);
		setBL(0x0200F778, (u32)dsiSaveClose);
		setBL(0x0200F780, (u32)dsiSaveDelete);
		setBL(0x0200F8B0, (u32)dsiSaveOpen);
		*(u32*)0x0200F8C8 = 0xE1A00000; // nop (dsiSaveGetLength)
		setBL(0x0200F8EC, (u32)dsiSaveRead);
		setBL(0x0200F8F4, (u32)dsiSaveClose);
		setBL(0x0200F938, (u32)dsiSaveOpen);
		setBL(0x0200F94C, (u32)dsiSaveClose);
		setBL(0x0200F960, (u32)dsiSaveCreate);
		setBL(0x0200F97C, (u32)dsiSaveOpen);
		*(u32*)0x0200F98C = 0xE1A00000; // nop
		setBL(0x0200F998, (u32)dsiSaveClose);
		setBL(0x0200F9A0, (u32)dsiSaveDelete);
		setBL(0x0200F9B8, (u32)dsiSaveCreate);
		setBL(0x0200F9C8, (u32)dsiSaveOpen);
		setBL(0x0200C8CC, (u32)dsiSaveSetLength);
		setBL(0x0200FA04, (u32)dsiSaveWrite);
		setBL(0x0200FA0C, (u32)dsiSaveClose);
		*(u32*)0x02013BC8 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Robot Rescue 2 (USA)
	// Robot Rescue 2 (Europe)
	else if ((strcmp(romTid, "KRRE") == 0 || strcmp(romTid, "KRRP") == 0) && saveOnFlashcard) {
		/* *(u32*)0x0200C2F4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C2F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C3B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C3B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200C578 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0200C57C = 0xE12FFF1E; // bx lr */
		setBL(0x0200C5A4, (u32)dsiSaveOpen);
		setBL(0x0200C5B8, (u32)dsiSaveClose);
		setBL(0x0200C5D8, (u32)dsiSaveCreate);
		setBL(0x0200C5F4, (u32)dsiSaveOpen);
		setBL(0x0200C60C, (u32)dsiSaveClose);
		setBL(0x0200C614, (u32)dsiSaveDelete);
		setBL(0x0200C7B0, (u32)dsiSaveOpen);
		setBL(0x0200C7C8, (u32)dsiSaveGetLength);
		setBL(0x0200C7EC, (u32)dsiSaveRead);
		setBL(0x0200C7F4, (u32)dsiSaveClose);
		setBL(0x0200C838, (u32)dsiSaveOpen);
		setBL(0x0200C84C, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveCreate);
		setBL(0x0200C87C, (u32)dsiSaveOpen);
		*(u32*)0x0200C88C = 0xE1A00000; // nop
		setBL(0x0200C898, (u32)dsiSaveClose);
		setBL(0x0200C8A0, (u32)dsiSaveDelete);
		setBL(0x0200C8B8, (u32)dsiSaveCreate);
		setBL(0x0200C8C8, (u32)dsiSaveOpen);
		setBL(0x0200C8F4, (u32)dsiSaveSetLength);
		setBL(0x0200C904, (u32)dsiSaveWrite);
		setBL(0x0200C90C, (u32)dsiSaveClose);
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02010888 = 0xE1A00000; // nop (Skip Manual screen)
		} else if (ndsHeader->gameCode[3] == 'P') {
			*(u32*)0x02010BE4 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Rock-n-Roll Domo (USA)
	else if (strcmp(romTid, "KD6E") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x02025C20;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02025C30;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02025C40;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02025C40;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02025C50;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02025C60;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02025C70;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x02010164 = 0x2001; // movs r0, #1
		*(u16*)0x02010166 = 0x4770; // bx lr
		*(u16*)0x0201045C = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0201045E = 0x4770; // bx lr
		setBLThumb(0x020104C2, dsiSaveCreateT);
		setBLThumb(0x020104D8, dsiSaveOpenT);
		setBLThumb(0x020104F4, dsiSaveSetLengthT);
		setBLThumb(0x02010508, dsiSaveWriteT);
		setBLThumb(0x0201051A, dsiSaveCloseT);
		*(u16*)0x02010540 = 0x4778; // bx pc
		tonccpy((u32*)0x02010544, dsiSaveGetLength, 0xC);
		setBLThumb(0x02010570, dsiSaveOpenT);
		setBLThumb(0x02010596, dsiSaveCloseT);
		setBLThumb(0x020105A8, dsiSaveReadT);
		setBLThumb(0x020105AE, dsiSaveCloseT);
		setBLThumb(0x020105C2, dsiSaveDeleteT);
		*(u16*)0x02016514 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Shantae: Risky's Revenge (USA)
	else if (strcmp(romTid, "KS3E") == 0 && saveOnFlashcard) {
		// Skip Manual screen
		/* *(u32*)0x02016130 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020161C8 = 0xE1A00000; // nop
		*(u32*)0x020161D0 = 0xE1A00000; // nop
		*(u32*)0x020161DC = 0xE1A00000; // nop
		*(u32*)0x020166C8 = 0xE3A06901; // mov r6, #0x4000 */

		// Hide help button
		*(u32*)0x02016688 = 0xE1A00000; // nop

		setBL(0x0209201C, (u32)dsiSaveCreate);
		setBL(0x02092040, (u32)dsiSaveGetResultCode);
		setBL(0x0209205C, (u32)dsiSaveCreate);
		setBL(0x0209291C, (u32)dsiSaveOpen);
		setBL(0x02092944, (u32)dsiSaveOpen);
		setBL(0x02092958, (u32)dsiSaveRead);
		setBL(0x02092960, (u32)dsiSaveClose);
		setBL(0x02092BCC, (u32)dsiSaveCreate);
		setBL(0x02092BDC, (u32)dsiSaveOpen);
		setBL(0x02092DE4, (u32)dsiSaveSetLength);
		setBL(0x02092DF4, (u32)dsiSaveWrite);
		setBL(0x02092DFC, (u32)dsiSaveClose);
	}

	// Shantae: Risky's Revenge (Europe)
	else if (strcmp(romTid, "KS3P") == 0 && saveOnFlashcard) {
		// Skip Manual screen
		/* *(u32*)0x020163B0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02016448 = 0xE1A00000; // nop
		*(u32*)0x02016450 = 0xE1A00000; // nop
		*(u32*)0x0201645C = 0xE1A00000; // nop
		*(u32*)0x02016940 = 0xE3A06901; // mov r6, #0x4000 */

		// Hide help button
		*(u32*)0x02016904 = 0xE1A00000; // nop

		setBL(0x020922A0, (u32)dsiSaveCreate);
		setBL(0x020922C4, (u32)dsiSaveGetResultCode);
		setBL(0x020922E0, (u32)dsiSaveCreate);
		setBL(0x02092D4C, (u32)dsiSaveOpen);
		setBL(0x02092D74, (u32)dsiSaveOpen);
		setBL(0x02092D88, (u32)dsiSaveRead);
		setBL(0x02092D90, (u32)dsiSaveClose);
		setBL(0x02092FFC, (u32)dsiSaveCreate);
		setBL(0x0209300C, (u32)dsiSaveOpen);
		setBL(0x02093214, (u32)dsiSaveSetLength);
		setBL(0x02093224, (u32)dsiSaveWrite);
		setBL(0x0209322C, (u32)dsiSaveClose);
	}

	// Simple DS Series Vol. 1: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KM4J") == 0 && saveOnFlashcard) {
		*(u32*)0x0200F91C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		//*(u32*)0x020128BC = 0xE3A00000; // mov r0, #0
		//*(u32*)0x020128C0 = 0xE12FFF1E; // bx lr
		setBL(0x020132D0, (u32)dsiSaveOpen);
		setBL(0x02013338, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02013384, (u32)dsiSaveGetLength);
		setBL(0x020133A0, (u32)dsiSaveRead);
		setBL(0x020133A8, (u32)dsiSaveClose);
		setBL(0x020133EC, (u32)dsiSaveOpen);
		setBL(0x0201346C, (u32)dsiSaveGetLength);
		setBL(0x0201347C, (u32)dsiSaveSeek);
		setBL(0x020134F8, (u32)dsiSaveClose);
		setBL(0x02013520, (u32)dsiSaveWrite);
		setBL(0x0201352C, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205F0D0, dsiSaveGetResultCode, 0xC);
	}

	// Simple DS Series Vol. 2: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KM5J") == 0 && saveOnFlashcard) {
		*(u32*)0x0200F9EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02013370, (u32)dsiSaveOpen);
		setBL(0x020133E0, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0201342C, (u32)dsiSaveGetLength);
		setBL(0x02013448, (u32)dsiSaveRead);
		setBL(0x02013450, (u32)dsiSaveClose);
		setBL(0x02013494, (u32)dsiSaveOpen);
		setBL(0x02013514, (u32)dsiSaveGetLength);
		setBL(0x02013524, (u32)dsiSaveSeek);
		setBL(0x020135A0, (u32)dsiSaveClose);
		setBL(0x020135C8, (u32)dsiSaveWrite);
		setBL(0x020135D4, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205E620, dsiSaveGetResultCode, 0xC);
	}

	// Simple DS Series Vol. 3: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "K5QJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0200EE50 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x020118CC, (u32)dsiSaveOpen);
		setBL(0x02011934, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02011980, (u32)dsiSaveGetLength);
		setBL(0x0201199C, (u32)dsiSaveRead);
		setBL(0x020119A4, (u32)dsiSaveClose);
		setBL(0x020119E8, (u32)dsiSaveOpen);
		setBL(0x02011A68, (u32)dsiSaveGetLength);
		setBL(0x02011A78, (u32)dsiSaveSeek);
		setBL(0x02011AF4, (u32)dsiSaveClose);
		setBL(0x02011B1C, (u32)dsiSaveWrite);
		setBL(0x02011B28, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205E16C, dsiSaveGetResultCode, 0xC);
	}

	// Simple DS Series Vol. 4: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KEYJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0200F9FC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02013410, (u32)dsiSaveOpen);
		setBL(0x02013478, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x020134C4, (u32)dsiSaveGetLength);
		setBL(0x020134E0, (u32)dsiSaveRead);
		setBL(0x020134E8, (u32)dsiSaveClose);
		setBL(0x0201352C, (u32)dsiSaveOpen);
		setBL(0x020135AC, (u32)dsiSaveGetLength);
		setBL(0x020135BC, (u32)dsiSaveSeek);
		setBL(0x02013638, (u32)dsiSaveClose);
		setBL(0x02013660, (u32)dsiSaveWrite);
		setBL(0x0201366C, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205F924, dsiSaveGetResultCode, 0xC);
	}

	// Simple DS Series Vol. 5: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "K5KJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0200F990 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x020133C8, (u32)dsiSaveOpen);
		setBL(0x02013430, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0201347C, (u32)dsiSaveGetLength);
		setBL(0x02013498, (u32)dsiSaveRead);
		setBL(0x020134A0, (u32)dsiSaveClose);
		setBL(0x020134E4, (u32)dsiSaveOpen);
		setBL(0x02013564, (u32)dsiSaveGetLength);
		setBL(0x02013574, (u32)dsiSaveSeek);
		setBL(0x020135F0, (u32)dsiSaveClose);
		setBL(0x02013618, (u32)dsiSaveWrite);
		setBL(0x02013624, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205F4A8, dsiSaveGetResultCode, 0xC);
	}

	// Simple DS Series Vol. 6: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KLHJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0200F9A8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02013460, (u32)dsiSaveOpen);
		setBL(0x020134C8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02013514, (u32)dsiSaveGetLength);
		setBL(0x02013530, (u32)dsiSaveRead);
		setBL(0x02013538, (u32)dsiSaveClose);
		setBL(0x0201357C, (u32)dsiSaveOpen);
		setBL(0x020135FC, (u32)dsiSaveGetLength);
		setBL(0x0201360C, (u32)dsiSaveSeek);
		setBL(0x02013688, (u32)dsiSaveClose);
		setBL(0x020136B0, (u32)dsiSaveWrite);
		setBL(0x020136BC, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205F574, dsiSaveGetResultCode, 0xC);
	}

	// Smart Girl's Playhouse Mini (USA)
	else if (strcmp(romTid, "K2FE") == 0 && saveOnFlashcard) {
		*(u32*)0x02026128 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202E6F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0202E8B8, (u32)dsiSaveCreate);
		setBL(0x0202E8C8, (u32)dsiSaveOpen);
		setBL(0x0202E8E4, (u32)dsiSaveSeek);
		setBL(0x0202E8F4, (u32)dsiSaveWrite);
		setBL(0x0202E8FC, (u32)dsiSaveClose);
		setBL(0x0202EA24, (u32)dsiSaveOpenR);
		setBL(0x0202EA3C, (u32)dsiSaveSeek);
		setBL(0x0202EA4C, (u32)dsiSaveRead);
		setBL(0x0202EA54, (u32)dsiSaveClose);
	}

	// SnowBoard Xtreme (USA)
	// SnowBoard Xtreme (Europe)
	else if ((strcmp(romTid, "KX5E") == 0 || strcmp(romTid, "KX5P") == 0) && saveOnFlashcard) {
		setBL(0x02011B70, (u32)dsiSaveCreate);
		*(u32*)0x02011B90 = 0xE3A00001; // mov r0, #1
		setBL(0x02011C24, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011C48 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x020313AC, (u32)dsiSaveOpen);
			setBL(0x020313C4, (u32)dsiSaveGetLength);
			setBL(0x020313D4, (u32)dsiSaveSeek);
			setBL(0x020313E4, (u32)dsiSaveWrite);
			setBL(0x020313EC, (u32)dsiSaveClose);
			setBL(0x0203145C, (u32)dsiSaveOpen);
			setBL(0x02031474, (u32)dsiSaveGetLength);
			setBL(0x02031488, (u32)dsiSaveSeek);
			setBL(0x02031498, (u32)dsiSaveRead);
			setBL(0x020314A0, (u32)dsiSaveClose);
			setBL(0x02031518, (u32)dsiSaveCreate);
			setBL(0x02031544, (u32)dsiSaveOpen);
			setBL(0x02031580, (u32)dsiSaveWrite);
			setBL(0x02031590, (u32)dsiSaveClose);
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x020313FC, (u32)dsiSaveOpen);
			setBL(0x02031414, (u32)dsiSaveGetLength);
			setBL(0x02031424, (u32)dsiSaveSeek);
			setBL(0x02031434, (u32)dsiSaveWrite);
			setBL(0x0203143C, (u32)dsiSaveClose);
			setBL(0x020314AC, (u32)dsiSaveOpen);
			setBL(0x020314C4, (u32)dsiSaveGetLength);
			setBL(0x020314D8, (u32)dsiSaveSeek);
			setBL(0x020314E8, (u32)dsiSaveRead);
			setBL(0x020314F0, (u32)dsiSaveClose);
			setBL(0x02031568, (u32)dsiSaveCreate);
			setBL(0x02031594, (u32)dsiSaveOpen);
			setBL(0x020315D0, (u32)dsiSaveWrite);
			setBL(0x020315E0, (u32)dsiSaveClose);
		}
	}

	// Sokuren Keisa: Shougaku 1 Nensei (Japan)
	// Sokuren Keisa: Shougaku 2 Nensei (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if ((strcmp(romTid, "KL9J") == 0 || strcmp(romTid, "KH2J") == 0) && saveOnFlashcard) {
		*(u32*)0x02027C98 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

		// Skip Manual screen
		*(u32*)0x02027EBC = 0xE1A00000; // nop
		*(u32*)0x02027EC4 = 0xE1A00000; // nop
		*(u32*)0x02027ED0 = 0xE1A00000; // nop
	}

	// Sokuren Keisa: Shougaku 3 Nensei (Japan)
	// Sokuren Keisa: Shougaku 4 Nensei (Japan)
	// Sokuren Keisa: Shougaku 5 Nensei (Japan)
	// Sokuren Keisa: Shougaku 6 Nensei (Japan)
	// Sokuren Keisa: Nanmon-Hen (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if ((strcmp(romTid, "KH3J") == 0 || strcmp(romTid, "KH4J") == 0 || strcmp(romTid, "KO5J") == 0
		   || strcmp(romTid, "KO6J") == 0 || strcmp(romTid, "KO7J") == 0) && saveOnFlashcard) {
		*(u32*)0x02027CD8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

		// Skip Manual screen
		*(u32*)0x02027EFC = 0xE1A00000; // nop
		*(u32*)0x02027F04 = 0xE1A00000; // nop
		*(u32*)0x02027F10 = 0xE1A00000; // nop
	}

	// Space Ace (USA)
	else if (strcmp(romTid, "KA6E") == 0 && saveOnFlashcard) {
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0201F8BC, (u32)dsiSaveOpen);
		setBL(0x0201F8D4, (u32)dsiSaveRead);
		setBL(0x0201F8FC, (u32)dsiSaveClose);
		setBL(0x0201F998, (u32)dsiSaveCreate);
		setBL(0x0201F9C8, (u32)dsiSaveOpen);
		setBL(0x0201F9F8, (u32)dsiSaveWrite);
		setBL(0x0201FA20, (u32)dsiSaveClose);
		setBL(0x0201FAFC, (u32)dsiSaveOpen);
		setBL(0x0201FB38, (u32)dsiSaveSeek);
		setBL(0x0201FB68, (u32)dsiSaveWrite);
		setBL(0x0201FB90, (u32)dsiSaveClose);
		setBL(0x0201FBFC, (u32)dsiSaveGetResultCode);
		setBL(0x0201FC2C, (u32)dsiSaveClose);
		setBL(0x0201FC44, (u32)dsiSaveClose);
	}

	// Spin Six (USA)
	else if (strcmp(romTid, "KQ6E") == 0 && saveOnFlashcard) {
		*(u32*)0x02013184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x020137B8, (u32)dsiSaveOpen);
		setBL(0x0201382C, (u32)dsiSaveGetLength);
		setBL(0x0201383C, (u32)dsiSaveGetLength);
		setBL(0x02013854, (u32)dsiSaveRead);
		setBL(0x0201385C, (u32)dsiSaveClose);
		setBL(0x020138D4, (u32)dsiSaveCreate);
		setBL(0x0201390C, (u32)dsiSaveOpen);
		setBL(0x0201393C, (u32)dsiSaveSetLength);
		setBL(0x02013960, (u32)dsiSaveWrite);
		setBL(0x02013968, (u32)dsiSaveClose);
		tonccpy((u32*)0x02068398, dsiSaveGetResultCode, 0xC);
	}

	// Spin Six (Europe, Australia)
	else if (strcmp(romTid, "KQ6V") == 0 && saveOnFlashcard) {
		*(u32*)0x02013184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x020137C4, (u32)dsiSaveOpen);
		setBL(0x02013838, (u32)dsiSaveGetLength);
		setBL(0x02013848, (u32)dsiSaveGetLength);
		setBL(0x02013860, (u32)dsiSaveRead);
		setBL(0x02013868, (u32)dsiSaveClose);
		setBL(0x020138E0, (u32)dsiSaveCreate);
		setBL(0x02013918, (u32)dsiSaveOpen);
		setBL(0x02013948, (u32)dsiSaveSetLength);
		setBL(0x0201396C, (u32)dsiSaveWrite);
		setBL(0x02013974, (u32)dsiSaveClose);
		tonccpy((u32*)0x020683A4, dsiSaveGetResultCode, 0xC);
	}

	// Kuru Kuru Akushon: Kuru Pachi 6 (Japan)
	else if (strcmp(romTid, "KQ6J") == 0 && saveOnFlashcard) {
		*(u32*)0x02013760 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02013D40, (u32)dsiSaveOpen);
		setBL(0x02013DA4, (u32)dsiSaveGetLength);
		setBL(0x02013DB4, (u32)dsiSaveGetLength);
		setBL(0x02013DC8, (u32)dsiSaveRead);
		setBL(0x02013DD0, (u32)dsiSaveClose);
		setBL(0x02013E44, (u32)dsiSaveCreate);
		setBL(0x02013E74, (u32)dsiSaveOpen);
		setBL(0x02013EAC, (u32)dsiSaveSetLength);
		setBL(0x02013ECC, (u32)dsiSaveWrite);
		setBL(0x02013ED4, (u32)dsiSaveClose);
		tonccpy((u32*)0x0206928C, dsiSaveGetResultCode, 0xC);
	}

	// Sudoku (USA)
	// Sudoku (USA) (Rev 1)
	else if (strcmp(romTid, "K4DE") == 0 && saveOnFlashcard) {
		if (ndsHeader->romversion == 1) {
			setBL(0x02037560, (u32)dsiSaveOpen);
			setBL(0x020375B0, (u32)dsiSaveCreate);
			setBL(0x020375F0, (u32)dsiSaveOpen);
			setBL(0x0203762C, (u32)dsiSaveSetLength);
			setBL(0x0203767C, (u32)dsiSaveSeek);
			setBL(0x0203768C, (u32)dsiSaveWrite);
			setBL(0x020376C8, (u32)dsiSaveClose);
			setBL(0x020376D8, (u32)dsiSaveClose);
			setBL(0x02037714, (u32)dsiSaveOpen);
			setBL(0x02037754, (u32)dsiSaveSeek);
			setBL(0x02037794, (u32)dsiSaveRead);
			setBL(0x020377A0, (u32)dsiSaveClose);
			setBL(0x02037810, (u32)dsiSaveOpen);
			setBL(0x02037850, (u32)dsiSaveSeek);
			setBL(0x02037890, (u32)dsiSaveWrite);
			setBL(0x0203789C, (u32)dsiSaveGetLength);
			setBL(0x020378C4, (u32)dsiSaveWrite);
		} else {
			setBL(0x020364A4, (u32)dsiSaveOpen);
			setBL(0x020364F0, (u32)dsiSaveCreate);
			setBL(0x02036530, (u32)dsiSaveOpen);
			setBL(0x0203656C, (u32)dsiSaveSetLength);
			setBL(0x020365B0, (u32)dsiSaveSeek);
			setBL(0x020365C0, (u32)dsiSaveWrite);
			setBL(0x020365FC, (u32)dsiSaveClose);
			setBL(0x02036614, (u32)dsiSaveClose);
			setBL(0x0203665C, (u32)dsiSaveOpen);
			setBL(0x02036694, (u32)dsiSaveSeek);
			setBL(0x020366C8, (u32)dsiSaveRead);
			setBL(0x020366D4, (u32)dsiSaveClose);
			setBL(0x02036724, (u32)dsiSaveOpen);
			setBL(0x0203675C, (u32)dsiSaveSeek);
			setBL(0x02036790, (u32)dsiSaveWrite);
			setBL(0x0203679C, (u32)dsiSaveGetLength);
			setBL(0x020367BC, (u32)dsiSaveClose);
		}
	}

	// Sudoku (Europe, Australia) (Rev 1)
	else if (strcmp(romTid, "K4DV") == 0 && saveOnFlashcard) {
		//*(u32*)0x020360E8 = 0xE3A00001; // mov r0, #1
		//*(u32*)0x020360EC = 0xE12FFF1E; // bx lr
		setBL(0x020375AC, (u32)dsiSaveOpen);
		*(u32*)0x020375CC = 0xE1A00000; // nop
		setBL(0x020375FC, (u32)dsiSaveCreate);
		setBL(0x0203763C, (u32)dsiSaveOpen);
		setBL(0x02037678, (u32)dsiSaveSetLength);
		setBL(0x020376C8, (u32)dsiSaveSeek);
		setBL(0x020376D8, (u32)dsiSaveWrite);
		setBL(0x02037714, (u32)dsiSaveClose);
		setBL(0x02037724, (u32)dsiSaveClose);
		setBL(0x02037760, (u32)dsiSaveOpen);
		setBL(0x020377A0, (u32)dsiSaveSeek);
		setBL(0x020377E0, (u32)dsiSaveRead);
		setBL(0x020377EC, (u32)dsiSaveClose);
		setBL(0x0203785C, (u32)dsiSaveOpen);
		setBL(0x0203789C, (u32)dsiSaveSeek);
		setBL(0x020378DC, (u32)dsiSaveWrite);
		setBL(0x020378E8, (u32)dsiSaveGetLength);
		setBL(0x02037910, (u32)dsiSaveWrite);
	}

	// System Flaw: Recruit (USA)
	else if (strcmp(romTid, "KSYE") == 0 && saveOnFlashcard) {
		setBL(0x02042080, (u32)dsiSaveOpen);
		setBL(0x020420D4, (u32)dsiSaveGetLength);
		setBL(0x020420E4, (u32)dsiSaveRead);
		setBL(0x020420EC, (u32)dsiSaveClose);
		setBL(0x02042128, (u32)dsiSaveOpen);
		*(u32*)0x02042140 = 0xE1A00000; // nop (dsiSaveGetArcSrc)
		*(u32*)0x02042150 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0204216C, (u32)dsiSaveCreate);
		setBL(0x02042178, (u32)dsiSaveClose);
		setBL(0x02042188, (u32)dsiSaveOpen);
		setBL(0x020421DC, (u32)dsiSaveSetLength);
		setBL(0x020421EC, (u32)dsiSaveWrite);
		setBL(0x020421F4, (u32)dsiSaveClose);
	}

	// System Flaw: Recruit (Europe)
	else if (strcmp(romTid, "KSYP") == 0 && saveOnFlashcard) {
		setBL(0x0200DAAC, (u32)dsiSaveOpen);
		setBL(0x0200DB00, (u32)dsiSaveGetLength);
		setBL(0x0200DB10, (u32)dsiSaveRead);
		setBL(0x0200DB18, (u32)dsiSaveClose);
		setBL(0x0200DB54, (u32)dsiSaveOpen);
		*(u32*)0x0200DB6C = 0xE1A00000; // nop (dsiSaveGetArcSrc)
		*(u32*)0x0200DB7C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0200DB98, (u32)dsiSaveCreate);
		setBL(0x0200DBA4, (u32)dsiSaveClose);
		setBL(0x0200DBB4, (u32)dsiSaveOpen);
		setBL(0x0200DC08, (u32)dsiSaveSetLength);
		setBL(0x0200DC18, (u32)dsiSaveWrite);
		setBL(0x0200DC20, (u32)dsiSaveClose);
	}

	// Tales to Enjoy!: Little Red Riding Hood (USA)
	// Tales to Enjoy!: Puss in Boots (USA)
	// Tales to Enjoy!: The Three Little Pigs (USA)
	// Tales to Enjoy!: The Ugly Duckling (USA)
	else if ((strcmp(romTid, "KZUE") == 0 || strcmp(romTid, "KZVE") == 0 || strcmp(romTid, "KZ7E") == 0 || strcmp(romTid, "KZ8E") == 0) && saveOnFlashcard) {
		setBL(0x0204D500, (u32)dsiSaveOpen);
		setBL(0x0204D560, (u32)dsiSaveGetLength);
		setBL(0x0204D570, (u32)dsiSaveRead);
		setBL(0x0204D578, (u32)dsiSaveClose);
		*(u32*)0x0204D59C = 0xE3A00000; // mov r0, #0
		setBL(0x0204D5C8, (u32)dsiSaveOpen);
		*(u32*)0x0204D5E0 = 0xE1A00000; // nop (dsiSaveGetArcSrc)
		*(u32*)0x0204D5F0 = 0xE3A00001; // mov r0, #1
		setBL(0x0204D60C, (u32)dsiSaveCreate);
		setBL(0x0204D618, (u32)dsiSaveClose);
		setBL(0x0204D62C, (u32)dsiSaveOpen);
		setBL(0x0204D63C, (u32)dsiSaveGetResultCode);
		setBL(0x0204D684, (u32)dsiSaveSetLength);
		setBL(0x0204D694, (u32)dsiSaveWrite);
		setBL(0x0204D69C, (u32)dsiSaveClose);
	}

	// Tangrams (USA)
	else if (strcmp(romTid, "KYYE") == 0 && saveOnFlashcard) {
		*(u32*)0x02039854 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203986C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0203987C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x020398F8 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x02039910, (u32)dsiSaveCreate);
		setBL(0x02039944, (u32)dsiSaveOpen);
		setBL(0x02039968, (u32)dsiSaveSetLength);
		setBL(0x020399A0, (u32)dsiSaveWrite);
		setBL(0x020399A8, (u32)dsiSaveClose);
		*(u32*)0x02039A6C = 0xE1A00000; // nop (dsiSaveOpenDir)
		setBL(0x02039A80, (u32)dsiSaveOpen);
		setBL(0x02039AD4, (u32)dsiSaveGetLength);
		setBL(0x02039AF8, (u32)dsiSaveRead);
		*(u32*)0x02039B90 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		setBL(0x02039BC8, (u32)dsiSaveClose);
		setBL(0x02039C70, (u32)dsiSaveDelete);
		setBL(0x02039C7C, (u32)dsiSaveCreate);
		setBL(0x02039CB0, (u32)dsiSaveOpen);
		setBL(0x02039D04, (u32)dsiSaveSetLength);
		setBL(0x02039D3C, (u32)dsiSaveWrite);
		setBL(0x02039D44, (u32)dsiSaveClose);
	}

	// Tantei Jinguuji Saburou: Tsubaki no Yukue (Japan)
	else if (strcmp(romTid, "KJTJ") == 0 && saveOnFlashcard) {
		setBL(0x02005F80, (u32)dsiSaveOpen);
		setBL(0x02005F9C, (u32)dsiSaveCreate);
		*(u32*)0x02005FE0 = 0xE3A00000; // mov r0, #0
		setBL(0x02006008, (u32)dsiSaveCreate);
		setBL(0x02006024, (u32)dsiSaveOpen);
		setBL(0x02006070, (u32)dsiSaveWrite);
		setBL(0x02006080, (u32)dsiSaveClose);
		setBL(0x020060DC, (u32)dsiSaveOpen);
		setBL(0x02006130, (u32)dsiSaveSeek);
		setBL(0x02006140, (u32)dsiSaveRead);
		setBL(0x02006150, (u32)dsiSaveClose);
		setBL(0x020061B4, (u32)dsiSaveOpen);
		setBL(0x02006208, (u32)dsiSaveRead);
		setBL(0x02006238, (u32)dsiSaveClose);
		setBL(0x02006254, (u32)dsiSaveSeek);
		setBL(0x02006264, (u32)dsiSaveWrite);
		setBL(0x02006294, (u32)dsiSaveSeek);
		setBL(0x020062A4, (u32)dsiSaveWrite);
		setBL(0x020062C0, (u32)dsiSaveClose);
		tonccpy((u32*)0x0202E118, dsiSaveGetResultCode, 0xC);
	}

	// Tantei Jinguuji Saburou: Akenaiyoru ni (Japan)
	// Tantei Jinguuji Saburou: Kadannoitte (Japan)
	// Tantei Jinguuji Saburou: Rensa Suru Noroi (Japan)
	// Tantei Jinguuji Saburou: Nakiko no Shouzou (Japan)
	else if ((strcmp(romTid, "KJAJ") == 0 || strcmp(romTid, "KJQJ") == 0 || strcmp(romTid, "KJLJ") == 0 || strcmp(romTid, "KJ7J") == 0) && saveOnFlashcard) {
		setBL(0x02005FD0, (u32)dsiSaveOpen);
		setBL(0x02005FEC, (u32)dsiSaveCreate);
		*(u32*)0x02006030 = 0xE3A00000; // mov r0, #0
		setBL(0x02006058, (u32)dsiSaveCreate);
		setBL(0x02006074, (u32)dsiSaveOpen);
		setBL(0x020060C0, (u32)dsiSaveWrite);
		setBL(0x020060D0, (u32)dsiSaveClose);
		setBL(0x0200612C, (u32)dsiSaveOpen);
		setBL(0x02006180, (u32)dsiSaveSeek);
		setBL(0x02006190, (u32)dsiSaveRead);
		setBL(0x020061A0, (u32)dsiSaveClose);
		setBL(0x02006204, (u32)dsiSaveOpen);
		setBL(0x02006258, (u32)dsiSaveRead);
		setBL(0x02006288, (u32)dsiSaveClose);
		setBL(0x020062A4, (u32)dsiSaveSeek);
		setBL(0x020062B4, (u32)dsiSaveWrite);
		setBL(0x020062E4, (u32)dsiSaveSeek);
		setBL(0x020062F4, (u32)dsiSaveWrite);
		setBL(0x02006310, (u32)dsiSaveClose);
		if (strncmp(romTid, "KJA", 3) == 0) {
			tonccpy((u32*)0x0202E130, dsiSaveGetResultCode, 0xC);
		} else if (strncmp(romTid, "KJ7", 3) != 0) {
			tonccpy((u32*)0x0202E148, dsiSaveGetResultCode, 0xC);
		} else {
			tonccpy((u32*)0x0202E1B0, dsiSaveGetResultCode, 0xC);
		}
	}

	// Tetris Party Live (USA)
	else if (strcmp(romTid, "KTEE") == 0 && saveOnFlashcard) {
		*(u32*)0x02054C30 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0205A768, (u32)dsiSaveOpenR);
		setBL(0x0205A778, (u32)dsiSaveGetLength);
		setBL(0x0205A7B0, (u32)dsiSaveRead);
		setBL(0x0205A7CC, (u32)dsiSaveClose);
		//*(u32*)0x0205A83C = 0xE12FFF1E; // bx lr
		setBL(0x0205A864, (u32)dsiSaveCreate);
		setBL(0x0205A874, (u32)dsiSaveGetResultCode);
		setBL(0x0205A89C, (u32)dsiSaveOpen);
		setBL(0x0205A8C0, (u32)dsiSaveWrite);
		setBL(0x0205A8F0, (u32)dsiSaveClose);
		//*(u32*)0x0205A92C = 0xE12FFF1E; // bx lr
		setBL(0x0205A95C, (u32)dsiSaveOpen);
		setBL(0x0205A99C, (u32)dsiSaveSeek);
		setBL(0x0205A9DC, (u32)dsiSaveWrite);
		setBL(0x0205AA54, (u32)dsiSaveSeek);
		setBL(0x0205AA80, (u32)dsiSaveWrite);
		setBL(0x0205AABC, (u32)dsiSaveClose);
	}

	// Tetris Party Live (Europe, Australia)
	else if (strcmp(romTid, "KTEV") == 0 && saveOnFlashcard) {
		*(u32*)0x02054C30 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0205A754, (u32)dsiSaveOpenR);
		setBL(0x0205A764, (u32)dsiSaveGetLength);
		setBL(0x0205A79C, (u32)dsiSaveRead);
		setBL(0x0205A7B8, (u32)dsiSaveClose);
		//*(u32*)0x0205A828 = 0xE12FFF1E; // bx lr
		setBL(0x0205A850, (u32)dsiSaveCreate);
		setBL(0x0205A860, (u32)dsiSaveGetResultCode);
		setBL(0x0205A888, (u32)dsiSaveOpen);
		setBL(0x0205A8AC, (u32)dsiSaveWrite);
		setBL(0x0205A8DC, (u32)dsiSaveClose);
		//*(u32*)0x0205A918 = 0xE12FFF1E; // bx lr
		setBL(0x0205A948, (u32)dsiSaveOpen);
		setBL(0x0205A988, (u32)dsiSaveSeek);
		setBL(0x0205A9C8, (u32)dsiSaveWrite);
		setBL(0x0205AA40, (u32)dsiSaveSeek);
		setBL(0x0205AA6C, (u32)dsiSaveWrite);
		setBL(0x0205AAA8, (u32)dsiSaveClose);
	}

	// Topoloco (USA)
	// Topoloco (Europe)
	/*else if (strncmp(romTid, "KT5", 3) == 0 && saveOnFlashcard) {
		setBL(0x02051B38, (u32)dsiSaveDelete);
		setBL(0x02051BB8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02051CE0, (u32)dsiSaveOpen);
		setBL(0x02051D64, (u32)dsiSaveGetLength);
		setBL(0x02051DB4, (u32)dsiSaveClose);
		setBL(0x02051DE0, (u32)dsiSaveSetLength);
		setBL(0x02051E00, (u32)dsiSaveClose);
		setBL(0x02051E80, (u32)dsiSaveRead);
		setBL(0x02051ECC, (u32)dsiSaveClose);
		setBL(0x02051EE4, (u32)dsiSaveRead);
		setBL(0x02051EF4, (u32)dsiSaveClose);
		setBL(0x02051F30, (u32)dsiSaveWrite);
		setBL(0x02051F74, (u32)dsiSaveClose);
		setBL(0x02051F8C, (u32)dsiSaveWrite);
		setBL(0x02051FA0, (u32)dsiSaveClose);
		setBL(0x02051FB8, (u32)dsiSaveClose);
	}*/

	// True Swing Golf Express (USA)
	// A Little Bit of... Nintendo Touch Golf (Europe, Australia)
	if ((strcmp(romTid, "K72E") == 0 || strcmp(romTid, "K72V") == 0) && saveOnFlashcard) {
		//*(u32*)0x02009A84 = 0xE12FFF1E; // bx lr
		setBL(0x02009AC0, (u32)dsiSaveOpen);
		setBL(0x02009AE0, (u32)dsiSaveGetLength);
		setBL(0x02009B48, (u32)dsiSaveClose);
		setBL(0x02009BEC, (u32)dsiSaveOpen);
		setBL(0x02009C00, (u32)dsiSaveCreate);
		setBL(0x02009C18, (u32)dsiSaveOpen);
		setBL(0x02009C2C, (u32)dsiSaveSetLength);
		setBL(0x02009C64, (u32)dsiSaveGetLength);
		setBL(0x02009CA0, (u32)dsiSaveSeek);
		setBL(0x02009CB8, (u32)dsiSaveWrite);
		setBL(0x02009D34, (u32)dsiSaveOpen);
		setBL(0x02009D4C, (u32)dsiSaveSeek);
		setBL(0x02009D5C, (u32)dsiSaveSeek);
		setBL(0x02009D6C, (u32)dsiSaveRead);
		setBL(0x02009D80, (u32)dsiSaveClose);
		setBL(0x02009E4C, (u32)dsiSaveOpen);
		setBL(0x02009E64, (u32)dsiSaveSeek);
		setBL(0x02009E78, (u32)dsiSaveSeek);
		setBL(0x02009E88, (u32)dsiSaveWrite);
		setBL(0x02009E9C, (u32)dsiSaveClose);
		setBL(0x02009F2C, (u32)dsiSaveOpen);
		setBL(0x02009F70, (u32)dsiSaveSeek);
		setBL(0x02009F80, (u32)dsiSaveSeek);
		setBL(0x02009F90, (u32)dsiSaveWrite);
		setBL(0x02009FC4, (u32)dsiSaveClose);
		setBL(0x0200A024, (u32)dsiSaveOpen);
		setBL(0x0200A03C, (u32)dsiSaveSeek);
		setBL(0x0200A04C, (u32)dsiSaveSeek);
		setBL(0x0200A05C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0200A0AC, (u32)dsiSaveClose);
		setBL(0x0200A134, (u32)dsiSaveClose);
	}

	// Unou to Sanougaren Sasuru: Uranoura (Japan)
	// Unable to read saved data
	else if (strcmp(romTid, "K6PJ") == 0 && saveOnFlashcard) {
		*(u32*)0x02006E84 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		//*(u32*)0x02007344 = 0xE1A00000; // nop (Skip directory browse)
		*(u32*)0x020092D4 = 0xE3A00000; // mov r0, #0 (Disable NFTR loading from TWLNAND)
		for (int i = 0; i < 11; i++) { // Skip Manual screen
			u32* offset = (u32*)0x0200A608;
			offset[i] = 0xE1A00000; // nop
		}
		/*setBL(0x02020B50, (u32)dsiSaveOpen);
		setBL(0x02020B94, (u32)dsiSaveGetLength);
		setBL(0x02020BB4, (u32)dsiSaveRead);
		setBL(0x02020BE4, (u32)dsiSaveClose);
		setBL(0x02020C50, (u32)dsiSaveOpen); // dsiSaveOpenDir
		*(u32*)0x02020C98 = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		setBL(0x02020D9C, (u32)dsiSaveClose); // dsiSaveCloseDir
		setBL(0x02020F58, (u32)dsiSaveCreate);
		setBL(0x02020F90, (u32)dsiSaveOpen);
		setBL(0x02020FE8, (u32)dsiSaveSetLength);
		setBL(0x02020FF8, (u32)dsiSaveWrite);
		setBL(0x02021028, (u32)dsiSaveClose);
		tonccpy((u32*)0x02039874, dsiSaveGetResultCode, 0xC);*/
	}

	// WarioWare: Touched! DL (USA, Australia)
	else if (strcmp(romTid, "Z2AT") == 0 && saveOnFlashcard) {
		setBL(0x0200BCA4, (u32)dsiSaveOpen);
		setBL(0x0200BCB4, (u32)dsiSaveGetLength);
		setBL(0x0200BCC8, (u32)dsiSaveSetLength);
		setBL(0x0200BCD0, (u32)dsiSaveClose);
		setBL(0x0200BD0C, (u32)dsiSaveCreate);
		setBL(0x0200BD1C, (u32)dsiSaveOpen);
		setBL(0x0200BD30, (u32)dsiSaveSetLength);
		setBL(0x0200BD38, (u32)dsiSaveClose);
		setBL(0x0200BDE0, (u32)dsiSaveOpen);
		setBL(0x0200BE10, (u32)dsiSaveSeek);
		setBL(0x0200BE20, (u32)dsiSaveRead);
		setBL(0x0200BE28, (u32)dsiSaveClose);
		setBL(0x0200BE84, (u32)dsiSaveOpen);
		setBL(0x0200BEC8, (u32)dsiSaveOpen);
		setBL(0x0200BEFC, (u32)dsiSaveSeek);
		setBL(0x0200BF0C, (u32)dsiSaveWrite);
		setBL(0x0200BF14, (u32)dsiSaveClose);
		setBL(0x0200BF7C, (u32)dsiSaveCreate);
		setBL(0x0200BF90, (u32)dsiSaveOpen);
		setBL(0x0200BFA4, (u32)dsiSaveSetLength);
		setBL(0x0200BFAC, (u32)dsiSaveClose);
	}

	// WarioWare: Touched! DL (Europe)
	else if (strcmp(romTid, "Z2AP") == 0 && saveOnFlashcard) {
		setBL(0x0200BD04, (u32)dsiSaveOpen);
		setBL(0x0200BD14, (u32)dsiSaveGetLength);
		setBL(0x0200BD28, (u32)dsiSaveSetLength);
		setBL(0x0200BD30, (u32)dsiSaveClose);
		setBL(0x0200BD6C, (u32)dsiSaveCreate);
		setBL(0x0200BD7C, (u32)dsiSaveOpen);
		setBL(0x0200BD90, (u32)dsiSaveSetLength);
		setBL(0x0200BD98, (u32)dsiSaveClose);
		setBL(0x0200BE40, (u32)dsiSaveOpen);
		setBL(0x0200BE70, (u32)dsiSaveSeek);
		setBL(0x0200BE80, (u32)dsiSaveRead);
		setBL(0x0200BE88, (u32)dsiSaveClose);
		setBL(0x0200BEE4, (u32)dsiSaveOpen);
		setBL(0x0200BF28, (u32)dsiSaveOpen);
		setBL(0x0200BF5C, (u32)dsiSaveSeek);
		setBL(0x0200BF6C, (u32)dsiSaveWrite);
		setBL(0x0200BF74, (u32)dsiSaveClose);
		setBL(0x0200BFDC, (u32)dsiSaveCreate);
		setBL(0x0200BFF0, (u32)dsiSaveOpen);
		setBL(0x0200C004, (u32)dsiSaveSetLength);
		setBL(0x0200C00C, (u32)dsiSaveClose);
	}

	// Sawaru Made in Wario DL (Japan)
	else if (strcmp(romTid, "Z2AJ") == 0 && saveOnFlashcard) {
		setBL(0x0200BCA0, (u32)dsiSaveOpen);
		setBL(0x0200BCB0, (u32)dsiSaveGetLength);
		setBL(0x0200BCC4, (u32)dsiSaveSetLength);
		setBL(0x0200BCCC, (u32)dsiSaveClose);
		setBL(0x0200BD08, (u32)dsiSaveCreate);
		setBL(0x0200BD18, (u32)dsiSaveOpen);
		setBL(0x0200BD2C, (u32)dsiSaveSetLength);
		setBL(0x0200BD34, (u32)dsiSaveClose);
		setBL(0x0200BDDC, (u32)dsiSaveOpen);
		setBL(0x0200BE0C, (u32)dsiSaveSeek);
		setBL(0x0200BE1C, (u32)dsiSaveRead);
		setBL(0x0200BE24, (u32)dsiSaveClose);
		setBL(0x0200BE80, (u32)dsiSaveOpen);
		setBL(0x0200BEC4, (u32)dsiSaveOpen);
		setBL(0x0200BEF8, (u32)dsiSaveSeek);
		setBL(0x0200BF08, (u32)dsiSaveWrite);
		setBL(0x0200BF10, (u32)dsiSaveClose);
		setBL(0x0200BF78, (u32)dsiSaveCreate);
		setBL(0x0200BF8C, (u32)dsiSaveOpen);
		setBL(0x0200BFA0, (u32)dsiSaveSetLength);
		setBL(0x0200BFA8, (u32)dsiSaveClose);
	}

	// VT Tennis (USA)
	else if (strcmp(romTid, "KVTE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201B634, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205C000 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		setBL(0x0209C1B0, (u32)dsiSaveCreate);
		setBL(0x0209C1C0, (u32)dsiSaveOpen);
		setBL(0x0209C1E8, (u32)dsiSaveSetLength);
		setBL(0x0209C1F8, (u32)dsiSaveWrite);
		setBL(0x0209C200, (u32)dsiSaveClose);
		setBL(0x0209C284, (u32)dsiSaveOpen);
		setBL(0x0209C2B0, (u32)dsiSaveRead);
		setBL(0x0209C2BC, (u32)dsiSaveClose);
	}

	// VT Tennis (Europe, Australia)
	else if (strcmp(romTid, "KVTV") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201AD00, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B314 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		setBL(0x0209B120, (u32)dsiSaveCreate);
		setBL(0x0209B130, (u32)dsiSaveOpen);
		setBL(0x0209B158, (u32)dsiSaveSetLength);
		setBL(0x0209B168, (u32)dsiSaveWrite);
		setBL(0x0209B170, (u32)dsiSaveClose);
		setBL(0x0209B1F4, (u32)dsiSaveOpen);
		setBL(0x0209B220, (u32)dsiSaveRead);
		setBL(0x0209B22C, (u32)dsiSaveClose);
	}

	// Wakugumi: Monochrome Puzzle (Europe, Australia)
	else if (strcmp(romTid, "KK4V") == 0 && saveOnFlashcard) {
		*(u32*)0x0204F240 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02050114 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// White-Water Domo (USA)
	else if (strcmp(romTid, "KDWE") == 0 && saveOnFlashcard) {
		const u32 dsiSaveCreateT = 0x02023258;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02023268;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02023278;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02023288;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02023298;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x020232A8;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x020232B8;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0200C918 = 0x2001; // movs r0, #1
		*(u16*)0x0200C91A = 0x4770; // bx lr
		*(u16*)0x0200CC08 = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x0200CC0A = 0x4770; // bx lr
		setBLThumb(0x0200CC6E, dsiSaveCreateT);
		setBLThumb(0x0200CC84, dsiSaveOpenT);
		setBLThumb(0x0200CCA0, dsiSaveSetLengthT);
		setBLThumb(0x0200CCB4, dsiSaveWriteT);
		setBLThumb(0x0200CCC6, dsiSaveCloseT);
		*(u16*)0x0200CCEC = 0x4778; // bx pc
		tonccpy((u32*)0x0200CCF0, dsiSaveGetLength, 0xC);
		setBLThumb(0x0200CD1C, dsiSaveOpenT);
		setBLThumb(0x0200CD42, dsiSaveCloseT);
		setBLThumb(0x0200CD54, dsiSaveReadT);
		setBLThumb(0x0200CD5A, dsiSaveCloseT);
		setBLThumb(0x0200CD6E, dsiSaveDeleteT);
		*(u16*)0x02013B10 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Wonderful Sports: Bowling (Japan)
	else if (strcmp(romTid, "KBSJ") == 0 && saveOnFlashcard) {
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02027AF4, (u32)dsiSaveOpen);
		setBL(0x02027B04, (u32)dsiSaveGetLength);
		setBL(0x02027B1C, (u32)dsiSaveRead);
		setBL(0x02027B6C, (u32)dsiSaveClose);
		setBL(0x02027BFC, (u32)dsiSaveCreate);
		setBL(0x02027C0C, (u32)dsiSaveOpen);
		setBL(0x02027C20, (u32)dsiSaveSetLength);
		setBL(0x02027C30, (u32)dsiSaveWrite);
		setBL(0x02027C38, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)0x02029AF0 = 0xE1A00000; // nop
		*(u32*)0x02029AF8 = 0xE1A00000; // nop

		// Skip NFTR font rendering
		*(u32*)0x02028C50 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02033D88 = 0xE12FFF1E; // bx lr
	}

	// Art Style: ZENGAGE (USA)
	// Art Style: NEMREM (Europe, Australia)
	else if ((strcmp(romTid, "KASE") == 0 || strcmp(romTid, "KASV") == 0) && saveOnFlashcard) {
		setBL(0x0200E984, (u32)dsiSaveOpen);
		setBL(0x0200EA8C, (u32)dsiSaveOpen);
		setBL(0x0200EAE4, (u32)dsiSaveRead);
		setBL(0x0200EB28, (u32)dsiSaveClose);
		//*(u32*)0x0200EBC8 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200EBCC = 0xE12FFF1E; // bx lr
		setBL(0x0200EBF4, (u32)dsiSaveCreate);
		setBL(0x0200EC04, (u32)dsiSaveGetResultCode);
		setBL(0x0200EC28, (u32)dsiSaveOpen);
		setBL(0x0200EC48, (u32)dsiSaveSetLength);
		setBL(0x0200EC68, (u32)dsiSaveWrite);
		setBL(0x0200EC84, (u32)dsiSaveClose);
	}

	// Art Style: SOMNIUM (Japan)
	else if (strcmp(romTid, "KASJ") == 0 && saveOnFlashcard) {
		setBL(0x0200EC4C, (u32)dsiSaveOpen);
		setBL(0x0200ED4C, (u32)dsiSaveOpen);
		setBL(0x0200EDA0, (u32)dsiSaveRead);
		setBL(0x0200EDF0, (u32)dsiSaveClose);
		//*(u32*)0x0200EE98 = 0xE3A00000; // mov r0, #0
		//*(u32*)0x0200EE9C = 0xE12FFF1E; // bx lr
		setBL(0x0200EEC0, (u32)dsiSaveCreate);
		setBL(0x0200EED0, (u32)dsiSaveGetResultCode);
		setBL(0x0200EEEC, (u32)dsiSaveOpen);
		setBL(0x0200EF08, (u32)dsiSaveSetLength);
		setBL(0x0200EF28, (u32)dsiSaveWrite);
		setBL(0x0200EF44, (u32)dsiSaveClose);
	}


	// Zombie Blaster (USA)
	else if (strcmp(romTid, "K7KE") == 0 && saveOnFlashcard) {
		/* *(u32*)0x020055B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020055C4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02005600 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02067048 = 0xE12FFF1E; // bx lr */

		setBL(0x02065568, (u32)dsiSaveOpenR);
		setBL(0x02065584, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x020655C0, (u32)dsiSaveOpen);
		setBL(0x020655D4, (u32)dsiSaveGetResultCode);
		setBL(0x02067068, (u32)dsiSaveOpen);
		setBL(0x02067084, (u32)dsiSaveWrite);
		setBL(0x02067090, (u32)dsiSaveClose);
		setBL(0x020670E4, (u32)dsiSaveOpen);
		setBL(0x020670F8, (u32)dsiSaveGetLength);
		setBL(0x0206710C, (u32)dsiSaveRead);
		setBL(0x02067118, (u32)dsiSaveClose);
	}

	// Zombie Skape (USA)
	// Zombie Skape (Europe)
	else if ((strcmp(romTid, "KZYE") == 0 || strcmp(romTid, "KZYP") == 0) && saveOnFlashcard) {
		setBL(0x02011D58, (u32)dsiSaveCreate);
		*(u32*)0x02011D78 = 0xE3A00001; // mov r0, #1
		setBL(0x02011E0C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011E30 = 0xE3A00001; // mov r0, #1
		if (ndsHeader->gameCode[3] == 'E') {
			setBL(0x02032118, (u32)dsiSaveOpen);
			setBL(0x02032130, (u32)dsiSaveGetLength);
			setBL(0x02032140, (u32)dsiSaveSeek);
			setBL(0x02032150, (u32)dsiSaveWrite);
			setBL(0x02032158, (u32)dsiSaveClose);
			setBL(0x020321C8, (u32)dsiSaveOpen);
			setBL(0x020321E0, (u32)dsiSaveGetLength);
			setBL(0x020321F4, (u32)dsiSaveSeek);
			setBL(0x02032204, (u32)dsiSaveRead);
			setBL(0x0203220C, (u32)dsiSaveClose);
			setBL(0x02032284, (u32)dsiSaveCreate);
			setBL(0x020322B0, (u32)dsiSaveOpen);
			setBL(0x020322EC, (u32)dsiSaveWrite);
			setBL(0x020322FC, (u32)dsiSaveClose);
		} else if (ndsHeader->gameCode[3] == 'P') {
			setBL(0x02032164, (u32)dsiSaveOpen);
			setBL(0x0203217C, (u32)dsiSaveGetLength);
			setBL(0x0203218C, (u32)dsiSaveSeek);
			setBL(0x0203219C, (u32)dsiSaveWrite);
			setBL(0x020321A4, (u32)dsiSaveClose);
			setBL(0x02032214, (u32)dsiSaveOpen);
			setBL(0x0203222C, (u32)dsiSaveGetLength);
			setBL(0x02032240, (u32)dsiSaveSeek);
			setBL(0x02032250, (u32)dsiSaveRead);
			setBL(0x02032258, (u32)dsiSaveClose);
			setBL(0x020322D0, (u32)dsiSaveCreate);
			setBL(0x020322FC, (u32)dsiSaveOpen);
			setBL(0x02032338, (u32)dsiSaveWrite);
			setBL(0x02032348, (u32)dsiSaveClose);
		}
	}

	// Zoonies: Escape from Makatu (USA)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KZSE") == 0 && saveOnFlashcard) {
		*(u32*)0x02012794 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02022DC4, (u32)dsiSaveOpen);
		setBL(0x02022DE4, (u32)dsiSaveRead);
		setBL(0x02022E04, (u32)dsiSaveClose);
		setBL(0x02022E88, (u32)dsiSaveDelete);
		setBL(0x02022F40, (u32)dsiSaveCreate);
		setBL(0x020230A0, (u32)dsiSaveOpen);
		setBL(0x020230B8, (u32)dsiSaveWrite);
		setBL(0x020230D4, (u32)dsiSaveClose);
		setBL(0x02023178, (u32)dsiSaveOpen);
		setBL(0x0202318C, (u32)dsiSaveGetLength);
		setBL(0x020231B0, (u32)dsiSaveRead);
		setBL(0x020231E8, (u32)dsiSaveRead);
		setBL(0x0202320C, (u32)dsiSaveRead);
		setBL(0x020232C4, (u32)dsiSaveClose);
	}

	// Zoonies: Escape from Makatu (Europe, Australia)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KZSV") == 0 && saveOnFlashcard) {
		*(u32*)0x02012778 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02022DA8, (u32)dsiSaveOpen);
		setBL(0x02022DC8, (u32)dsiSaveRead);
		setBL(0x02022DE8, (u32)dsiSaveClose);
		setBL(0x02022E6C, (u32)dsiSaveDelete);
		setBL(0x02022F24, (u32)dsiSaveCreate);
		setBL(0x02023084, (u32)dsiSaveOpen);
		setBL(0x0202309C, (u32)dsiSaveWrite);
		setBL(0x020230B8, (u32)dsiSaveClose);
		setBL(0x0202315C, (u32)dsiSaveOpen);
		setBL(0x02023170, (u32)dsiSaveGetLength);
		setBL(0x02023194, (u32)dsiSaveRead);
		setBL(0x020231C8, (u32)dsiSaveRead);
		setBL(0x020231EC, (u32)dsiSaveRead);
		setBL(0x020232A4, (u32)dsiSaveClose);
	}

	// Zuma's Revenge! (USA)
	// Zuma's Revenge! (Europe, Australia)
	else if ((strcmp(romTid, "KZTE") == 0 || strcmp(romTid, "KZTV") == 0) && saveOnFlashcard) {
		setBL(0x02014134, (u32)dsiSaveOpen);
		setBL(0x02014158, (u32)dsiSaveGetLength);
		setBL(0x0201416C, (u32)dsiSaveRead);
		setBL(0x0201419C, (u32)dsiSaveClose);
		setBL(0x02014214, (u32)dsiSaveOpen);
		setBL(0x02014240, (u32)dsiSaveSetLength);
		setBL(0x02014264, (u32)dsiSaveWrite);
		setBL(0x02014280, (u32)dsiSaveClose);
		*(u32*)0x020142B8 = 0xE1A00000; // nop (dsiSaveCreateDirAuto)
		setBL(0x020142C4, (u32)dsiSaveCreate);
		tonccpy((u32*)0x02016E10, dsiSaveGetResultCode, 0xC);
		//*(u32*)0x020815F8 = 0xE12FFF1E; // bx lr
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

	// Advanced Circuits (USA)
	// Advanced Circuits (Europe, Australia)
	else if (strncmp(romTid, "KAC", 3) == 0) {
		*(u32*)0x0202CDA4 = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02053F90 = 0xE1A00000; // nop
		} else if (ndsHeader->gameCode[3] == 'V') {
			*(u32*)0x02053FB8 = 0xE1A00000; // nop
		}
	}

	// Ah! Heaven (USA)
	// Ah! Heaven (Europe)
	else if (strcmp(romTid, "K5HE") == 0 || strcmp(romTid, "K5HP") == 0) {
		*(u32*)0x0201FD04 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02029C68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02029D14 = 0xE12FFF1E; // bx lr
	}

	// Asphalt 4: Elite Racing (USA)
	else if (strcmp(romTid, "KA4E") == 0) {
		*(u32*)0x0204FA6C = 0xE12FFF1E; // bx lr
	}

	// Asphalt 4: Elite Racing (Europe, Australia)
	else if (strcmp(romTid, "KA4V") == 0) {
		*(u32*)0x0204FAE0 = 0xE12FFF1E; // bx lr
	}

	// Brain Challenge (USA)
	else if (strcmp(romTid, "KBCE") == 0) {
		*(u32*)0x0200EBD8 = 0xE12FFF1E; // bx lr
	}

	// Brain Challenge (Europe, Australia)
	else if (strcmp(romTid, "KBCV") == 0) {
		*(u32*)0x0200EBF4 = 0xE12FFF1E; // bx lr
	}

	// Candle Route (USA)
	else if (strcmp(romTid, "K9YE") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020AE76C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Candle Route (Europe)
	else if (strcmp(romTid, "K9YP") == 0) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020AE810;
			offset[i] = 0xE1A00000; // nop
		}
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

	// Invasion of the Alien Blobs (USA)
	else if (strcmp(romTid, "KBTE") == 0) {
		*(u32*)0x020224EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020224F0 = 0xE12FFF1E; // bx lr
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

	// Lola's Fruit Shop Sudoku (USA)
	// Lola's Fruit Shop Sudoku (Europe)
	else if (strcmp(romTid, "KOFE") == 0 || strcmp(romTid, "KOFP") == 0) {
		*(u32*)0x02005108 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0201CF70 = 0xE1A00000; // nop (Skip Manual screen)
		} else {
			*(u32*)0x0201CFCC = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Magnetic Joe (USA)
	else if (strcmp(romTid, "KJOE") == 0) {
		*(u32*)0x02036A30 = 0xE1A00000; // nop (Skip
		*(u32*)0x02036A34 = 0xE1A00000; // nop  Manual screen)
		*(u32*)0x0204E5AC = 0xE3A00002; // mov r0, #2
		*(u32*)0x0205DAAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205DAB0 = 0xE12FFF1E; // bx lr
	}

	// Mixed Messages (USA)
	// Mixed Messages (Europe, Australia)
	else if (strcmp(romTid, "KMME") == 0 || strcmp(romTid, "KMMV") == 0) {
		*(u32*)0x02031A40 = 0xE3A00008; // mov r0, #8
		*(u32*)0x02031A44 = 0xE12FFF1E; // bx lr
		*(u32*)0x02033B00 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Neko Reversi (Japan)
	else if (strcmp(romTid, "KNVJ") == 0) {
		*(u32*)0x0203FCA4 = 0xE1A00000; // nop (Skip Manual screen)
	}

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

	// Sudoku 4Pockets (USA)
	// Sudoku 4Pockets (Europe)
	else if (strcmp(romTid, "K4FE") == 0 || strcmp(romTid, "K4FP") == 0) {
		*(u32*)0x02004C4C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202E888 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202E88C = 0xE12FFF1E; // bx lr
	}
}

void patchBinary(cardengineArm9* ce9, const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	if (ndsHeader->unitCode == 3) {
		dsiWarePatch(ce9, ndsHeader);
		return;
	}

	const char* romTid = getRomTid(ndsHeader);

	const u32* dsiSaveGetResultCode = ce9->patches->dsiSaveGetResultCode;
	const u32* dsiSaveCreate = ce9->patches->dsiSaveCreate;
	const u32* dsiSaveDelete = ce9->patches->dsiSaveDelete;
	const u32* dsiSaveGetInfo = ce9->patches->dsiSaveGetInfo;
	const u32* dsiSaveSetLength = ce9->patches->dsiSaveSetLength;
	const u32* dsiSaveOpen = ce9->patches->dsiSaveOpen;
	const u32* dsiSaveOpenR = ce9->patches->dsiSaveOpenR;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveGetLength = ce9->patches->dsiSaveGetLength;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;
	const u32* dsiSaveWrite = ce9->patches->dsiSaveWrite;

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
	else if (strcmp(romTid, "KADE") == 0 && saveOnFlashcard) {
		*(u32*)0x0202D25C = 0xEB00007C; // bl 0x0202D454 (Skip Manual screen)
		setBL(0x0203A248, (u32)dsiSaveOpen);
		setBL(0x0203A26C, (u32)dsiSaveClose);
		*(u32*)0x0203A2B8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203A2DC = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203A2FC, (u32)dsiSaveCreate);
		setBL(0x0203A30C, (u32)dsiSaveOpen);
		setBL(0x0203A338, (u32)dsiSaveWrite);
		setBL(0x0203A350, (u32)dsiSaveClose);
		setBL(0x0203A39C, (u32)dsiSaveOpen);
		setBL(0x0203A3C4, (u32)dsiSaveRead);
		setBL(0x0203A3F0, (u32)dsiSaveClose);
		setBL(0x0203A4E0, (u32)dsiSaveOpen);
		setBL(0x0203A508, (u32)dsiSaveWrite);
		setBL(0x0203A524, (u32)dsiSaveClose);
	}

	// Art Style: CODE (Europe, Australia)
	else if (strcmp(romTid, "KADV") == 0 && saveOnFlashcard) {
		*(u32*)0x0202D288 = 0xEB00007C; // bl 0x0202D480 (Skip Manual screen)
		setBL(0x0203A2D8, (u32)dsiSaveOpen);
		setBL(0x0203A2FC, (u32)dsiSaveClose);
		*(u32*)0x0203A348 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203A36C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203A38C, (u32)dsiSaveCreate);
		setBL(0x0203A39C, (u32)dsiSaveOpen);
		setBL(0x0203A3C8, (u32)dsiSaveWrite);
		setBL(0x0203A3E0, (u32)dsiSaveClose);
		setBL(0x0203A42C, (u32)dsiSaveOpen);
		setBL(0x0203A454, (u32)dsiSaveRead);
		setBL(0x0203A480, (u32)dsiSaveClose);
		setBL(0x0203A570, (u32)dsiSaveOpen);
		setBL(0x0203A598, (u32)dsiSaveWrite);
		setBL(0x0203A5B4, (u32)dsiSaveClose);
	}

	// Art Style: DECODE (Japan)
	else if (strcmp(romTid, "KADJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0202E2AC = 0xEB000071; // bl 0x0202E478 (Skip Manual screen)
		setBL(0x0203B108, (u32)dsiSaveOpen);
		setBL(0x0203B12C, (u32)dsiSaveClose);
		*(u32*)0x0203B170 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0203B194 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0203B1B0, (u32)dsiSaveCreate);
		setBL(0x0203B1C0, (u32)dsiSaveOpen);
		setBL(0x0203B1EC, (u32)dsiSaveWrite);
		setBL(0x0203B208, (u32)dsiSaveClose);
		setBL(0x0203B250, (u32)dsiSaveOpen);
		setBL(0x0203B27C, (u32)dsiSaveRead);
		setBL(0x0203B2AC, (u32)dsiSaveClose);
		setBL(0x0203B398, (u32)dsiSaveOpen);
		setBL(0x0203B3C4, (u32)dsiSaveWrite);
		setBL(0x0203B3E0, (u32)dsiSaveClose);
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