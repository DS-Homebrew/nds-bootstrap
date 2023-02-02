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

extern u8 gameOnFlashcard;
extern u8 saveOnFlashcard;
extern u8 valueBits3;
#define memoryPit (valueBits3 & BIT(1))
#define twlSharedFont (valueBits3 & BIT(3))
#define chnSharedFont (valueBits3 & BIT(4))
#define korSharedFont (valueBits3 & BIT(5))

u16 patchOffsetCacheFilePrevCrc = 0;
u16 patchOffsetCacheFileNewCrc = 0;

patchOffsetCacheContents patchOffsetCache;

extern bool logging;
extern bool gbaRomFound;
extern u8 dsiSD;
extern int sharedFontRegion;

static inline void doubleNopT(u32 addr) {
	*(u16*)(addr)   = 0x46C0;
	*(u16*)(addr+2) = 0x46C0;
}

void dsiWarePatch(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
	extern u8 consoleModel;
	extern u16 a9ScfgRom;
	const char* romTid = getRomTid(ndsHeader);
	const char* dataPub = "dataPub:";
	const char* dataPrv = "dataPrv:";

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

	const bool twlFontFound = ((sharedFontRegion == 0 && !gameOnFlashcard) || twlSharedFont);
	//const bool chnFontFound = ((sharedFontRegion == 1 && !gameOnFlashcard) || chnSharedFont);
	const bool korFontFound = ((sharedFontRegion == 2 && !gameOnFlashcard) || korSharedFont);

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

	// GO Series: 10 Second Run (USA)
	// GO Series: 10 Second Run (Europe)
	else if (strcmp(romTid, "KJUE") == 0 || strcmp(romTid, "KJUP") == 0) {
		if (saveOnFlashcard) {
			*(u32*)0x020150FC = 0xE12FFF1E; // bx lr
			// Save patch causes the game to crash on panic function?
			/*setBL(0x02015C60, (u32)dsiSaveGetInfo);
			setBL(0x02015C9C, (u32)dsiSaveGetInfo);
			setBL(0x02015CFC, (u32)dsiSaveCreate);
			setBL(0x02015D38, (u32)dsiSaveGetInfo);
			setBL(0x02015D50, (u32)dsiSaveDelete);
			setBL(0x02015DB0, (u32)dsiSaveOpen);
			setBL(0x02015DDC, (u32)dsiSaveGetLength);
			setBL(0x02015DF0, (u32)dsiSaveRead);
			setBL(0x02015E0C, (u32)dsiSaveClose);
			setBL(0x02015E20, (u32)dsiSaveClose);
			setBL(0x02015EB4, (u32)dsiSaveOpen);
			setBL(0x02015EE8, (u32)dsiSaveWrite);
			setBL(0x02015F04, (u32)dsiSaveClose);
			setBL(0x02015F18, (u32)dsiSaveClose);*/
			// tonccpy((u32*)0x02031660, dsiSaveGetResultCode, 0xC);
		}
		if (!twlFontFound) {
			*(u32*)0x020193E0 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
			*(u32*)0x02019D20 = 0xE12FFF1E; // bx lr
		}
	}

	// 10 Byou Sou (Japan)
	else if (strcmp(romTid, "KJUJ") == 0) {
		if (saveOnFlashcard) {
			*(u32*)0x02014628 = 0xE12FFF1E; // bx lr
		}
		if (!twlFontFound) {
			*(u32*)0x02018914 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
			*(u32*)0x02019254 = 0xE12FFF1E; // bx lr
		}
	}

	// 18th Gate (USA)
	else if (strcmp(romTid, "KXOE") == 0 && saveOnFlashcard) {
		setBL(0x020D172C, (u32)dsiSaveGetInfo);
		setBL(0x020D1740, (u32)dsiSaveOpen);
		setBL(0x020D1754, (u32)dsiSaveCreate);
		setBL(0x020D1764, (u32)dsiSaveOpen);
		setBL(0x020D1774, (u32)dsiSaveGetResultCode);
		setBL(0x020D1790, (u32)dsiSaveCreate);
		setBL(0x020D17A0, (u32)dsiSaveOpen);
		setBL(0x020D187C, (u32)dsiSaveSeek);
		setBL(0x020D1894, (u32)dsiSaveWrite);
		setBL(0x020D18A4, (u32)dsiSaveSeek);
		setBL(0x020D18B4, (u32)dsiSaveWrite);
		setBL(0x020D18C4, (u32)dsiSaveSeek);
		setBL(0x020D18D4, (u32)dsiSaveWrite);
		setBL(0x020D1940, (u32)dsiSaveSeek);
		setBL(0x020D1950, (u32)dsiSaveWrite);
		setBL(0x020D1958, (u32)dsiSaveClose);
		setBL(0x020D19A4, (u32)dsiSaveOpen);
		setBL(0x020D1A90, (u32)dsiSaveSeek);
		setBL(0x020D1AA4, (u32)dsiSaveRead);
		setBL(0x020D1ACC, (u32)dsiSaveClose);
		setBL(0x020D1BF4, (u32)dsiSaveOpen);
		setBL(0x020D1C08, (u32)dsiSaveSeek);
		setBL(0x020D1C18, (u32)dsiSaveWrite);
		setBL(0x020D1C20, (u32)dsiSaveClose);
	}

	// 18th Gate (Europe, Australia)
	else if (strcmp(romTid, "KXOV") == 0 && saveOnFlashcard) {
		setBL(0x0209F7F8, (u32)dsiSaveGetInfo);
		setBL(0x0209F80C, (u32)dsiSaveOpen);
		setBL(0x0209F820, (u32)dsiSaveCreate);
		setBL(0x0209F830, (u32)dsiSaveOpen);
		setBL(0x0209F840, (u32)dsiSaveGetResultCode);
		setBL(0x0209F85C, (u32)dsiSaveCreate);
		setBL(0x0209F86C, (u32)dsiSaveOpen);
		setBL(0x0209F948, (u32)dsiSaveSeek);
		setBL(0x0209F960, (u32)dsiSaveWrite);
		setBL(0x0209F970, (u32)dsiSaveSeek);
		setBL(0x0209F980, (u32)dsiSaveWrite);
		setBL(0x0209F990, (u32)dsiSaveSeek);
		setBL(0x0209F9A0, (u32)dsiSaveWrite);
		setBL(0x0209FA0C, (u32)dsiSaveSeek);
		setBL(0x0209FA1C, (u32)dsiSaveWrite);
		setBL(0x0209FA24, (u32)dsiSaveClose);
		setBL(0x0209FA70, (u32)dsiSaveOpen);
		setBL(0x0209FB5C, (u32)dsiSaveSeek);
		setBL(0x0209FB70, (u32)dsiSaveRead);
		setBL(0x0209FB98, (u32)dsiSaveClose);
		setBL(0x0209FCC0, (u32)dsiSaveOpen);
		setBL(0x0209FCD4, (u32)dsiSaveSeek);
		setBL(0x0209FCE4, (u32)dsiSaveWrite);
		setBL(0x0209FCEC, (u32)dsiSaveClose);
	}

	// 1001 Crystal Mazes Collection (USA)
	else if (strcmp(romTid, "KOKE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200C78C, (u32)dsiSaveOpen);
			setBL(0x0200C7A0, (u32)dsiSaveClose);
			setBL(0x0200C7C0, (u32)dsiSaveCreate);
			setBL(0x0200C7D8, (u32)dsiSaveOpen);
			setBL(0x0200C7F0, (u32)dsiSaveClose);
			setBL(0x0200C7F8, (u32)dsiSaveDelete);
			setBL(0x0200C980, (u32)dsiSaveOpen);
			setBL(0x0200C998, (u32)dsiSaveGetLength);
			setBL(0x0200C9BC, (u32)dsiSaveRead);
			setBL(0x0200C9C4, (u32)dsiSaveClose);
			setBL(0x0200CA00, (u32)dsiSaveOpen);
			setBL(0x0200CA14, (u32)dsiSaveClose);
			setBL(0x0200CA28, (u32)dsiSaveCreate);
			setBL(0x0200CA40, (u32)dsiSaveOpen);
			setBL(0x0200CA5C, (u32)dsiSaveClose);
			setBL(0x0200CA64, (u32)dsiSaveDelete);
			setBL(0x0200CA78, (u32)dsiSaveCreate);
			setBL(0x0200CA88, (u32)dsiSaveOpen);
			setBL(0x0200CA98, (u32)dsiSaveGetResultCode);
			setBL(0x0200CAB0, (u32)dsiSaveSetLength);
			setBL(0x0200CAC0, (u32)dsiSaveWrite);
			setBL(0x0200CAC8, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x0200EF88 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// 1001 Crystal Mazes Collection (Europe)
	else if (strcmp(romTid, "KOKP") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200C734, (u32)dsiSaveOpen);
			setBL(0x0200C748, (u32)dsiSaveClose);
			setBL(0x0200C768, (u32)dsiSaveCreate);
			setBL(0x0200C784, (u32)dsiSaveOpen);
			setBL(0x0200C79C, (u32)dsiSaveClose);
			setBL(0x0200C7A4, (u32)dsiSaveDelete);
			setBL(0x0200C940, (u32)dsiSaveOpen);
			setBL(0x0200C958, (u32)dsiSaveGetLength);
			setBL(0x0200C97C, (u32)dsiSaveRead);
			setBL(0x0200C984, (u32)dsiSaveClose);
			setBL(0x0200C9C8, (u32)dsiSaveOpen);
			setBL(0x0200C9DC, (u32)dsiSaveClose);
			setBL(0x0200C9F0, (u32)dsiSaveCreate);
			setBL(0x0200CA0C, (u32)dsiSaveOpen);
			setBL(0x0200CA28, (u32)dsiSaveClose);
			setBL(0x0200CA30, (u32)dsiSaveDelete);
			setBL(0x0200CA48, (u32)dsiSaveCreate);
			setBL(0x0200CA58, (u32)dsiSaveOpen);
			setBL(0x0200CA68, (u32)dsiSaveGetResultCode);
			setBL(0x0200CA84, (u32)dsiSaveSetLength);
			setBL(0x0200CA94, (u32)dsiSaveWrite);
			setBL(0x0200CA9C, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x0200CAAC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// 200 Vmaja: Charen Ji Supirittsu (Japan)
	else if (strcmp(romTid, "KVMJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020053EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			*(u32*)0x02075390 = 0xE3A0000B; // mov r0, #0xB
			*(u32*)0x02075FFC = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
			*(u32*)0x02076018 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
			*(u32*)0x02076244 = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
			*(u32*)0x02076254 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
			*(u32*)0x02076280 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02076284 = 0xE12FFF1E; // bx lr
			setBL(0x020762FC, (u32)dsiSaveOpen);
			setBL(0x02076314, (u32)dsiSaveGetLength);
			setBL(0x02076340, (u32)dsiSaveRead);
			setBL(0x02076348, (u32)dsiSaveClose);
			setBL(0x02076420, (u32)dsiSaveCreate);
			setBL(0x02076430, (u32)dsiSaveOpen);
			setBL(0x02076440, (u32)dsiSaveGetResultCode);
			setBL(0x02076474, (u32)dsiSaveSetLength);
			setBL(0x020764A4, (u32)dsiSaveWrite);
			setBL(0x020764AC, (u32)dsiSaveClose);
		}
		toncset((char*)0x020A05F4, 0, 9); // Redirect otherPrv to dataPrv
		tonccpy((char*)0x020A05F4, dataPrv, strlen(dataPrv));
		toncset((char*)0x020A0608, 0, 9);
		tonccpy((char*)0x020A0608, dataPrv, strlen(dataPrv));
	}

	// 24/7 Solitaire (USA)
	else if (strcmp(romTid, "K4IE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200E83C, (u32)dsiSaveOpen);
			setBL(0x0200E8B0, (u32)dsiSaveGetLength);
			setBL(0x0200E8C4, (u32)dsiSaveClose);
			setBL(0x0200E8E4, (u32)dsiSaveSeek);
			setBL(0x0200E8FC, (u32)dsiSaveRead);
			setBL(0x0200E910, (u32)dsiSaveClose);
			setBL(0x0200E964, (u32)dsiSaveClose);
			*(u32*)0x0200E9A8 = 0xE1A00000; // nop
			setBL(0x0200EA04, (u32)dsiSaveCreate);
			setBL(0x0200EA58, (u32)dsiSaveOpen);
			setBL(0x0200EAC0, (u32)dsiSaveSetLength);
			setBL(0x0200EAD8, (u32)dsiSaveClose);
			setBL(0x0200EB2C, (u32)dsiSaveGetLength);
			setBL(0x0200EB40, (u32)dsiSaveClose);
			setBL(0x0200EB60, (u32)dsiSaveSeek);
			setBL(0x0200EB78, (u32)dsiSaveWrite);
			setBL(0x0200EB8C, (u32)dsiSaveClose);
			setBL(0x0200EBD8, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x020359EC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// 24/7 Solitaire (Europe)
	else if (strcmp(romTid, "K4IP") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200E6D0, (u32)dsiSaveOpen);
			setBL(0x0200E744, (u32)dsiSaveGetLength);
			setBL(0x0200E758, (u32)dsiSaveClose);
			setBL(0x0200E778, (u32)dsiSaveSeek);
			setBL(0x0200E790, (u32)dsiSaveRead);
			setBL(0x0200E7A4, (u32)dsiSaveClose);
			setBL(0x0200E7F8, (u32)dsiSaveClose);
			*(u32*)0x0200E7F8 = 0xE1A00000; // nop
			setBL(0x0200E898, (u32)dsiSaveCreate);
			setBL(0x0200E8EC, (u32)dsiSaveOpen);
			setBL(0x0200E954, (u32)dsiSaveSetLength);
			setBL(0x0200E96C, (u32)dsiSaveClose);
			setBL(0x0200E9C0, (u32)dsiSaveGetLength);
			setBL(0x0200E9D4, (u32)dsiSaveClose);
			setBL(0x0200E9F4, (u32)dsiSaveSeek);
			setBL(0x0200EA0C, (u32)dsiSaveWrite);
			setBL(0x0200EA20, (u32)dsiSaveClose);
			setBL(0x0200EA6C, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x02035880 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// Soritea Korekusho (Japan)
	else if (strcmp(romTid, "K4IJ") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200E7C4, (u32)dsiSaveOpen);
			setBL(0x0200E838, (u32)dsiSaveGetLength);
			setBL(0x0200E84C, (u32)dsiSaveClose);
			setBL(0x0200E86C, (u32)dsiSaveSeek);
			setBL(0x0200E884, (u32)dsiSaveRead);
			setBL(0x0200E898, (u32)dsiSaveClose);
			setBL(0x0200E8EC, (u32)dsiSaveClose);
			*(u32*)0x0200E930 = 0xE1A00000; // nop
			setBL(0x0200E98C, (u32)dsiSaveCreate);
			setBL(0x0200E9E0, (u32)dsiSaveOpen);
			setBL(0x0200EA48, (u32)dsiSaveSetLength);
			setBL(0x0200EA60, (u32)dsiSaveClose);
			setBL(0x0200EAB4, (u32)dsiSaveGetLength);
			setBL(0x0200EAC8, (u32)dsiSaveClose);
			setBL(0x0200EAE8, (u32)dsiSaveSeek);
			setBL(0x0200EB00, (u32)dsiSaveWrite);
			setBL(0x0200EB14, (u32)dsiSaveClose);
			setBL(0x0200EB60, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x02035654 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// 3D Mahjong (USA)
	else if (strcmp(romTid, "KMJE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020093D8 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			setBL(0x0200DA0C, (u32)dsiSaveOpen);
			setBL(0x0200DA80, (u32)dsiSaveGetLength);
			setBL(0x0200DA94, (u32)dsiSaveClose);
			setBL(0x0200DAB4, (u32)dsiSaveSeek);
			setBL(0x0200DACC, (u32)dsiSaveRead);
			setBL(0x0200DAE0, (u32)dsiSaveClose);
			setBL(0x0200DB34, (u32)dsiSaveClose);
			*(u32*)0x0200DB78 = 0xE1A00000; // nop
			setBL(0x0200DBD4, (u32)dsiSaveCreate);
			setBL(0x0200DC28, (u32)dsiSaveOpen);
			setBL(0x0200DC90, (u32)dsiSaveSetLength);
			setBL(0x0200DCA8, (u32)dsiSaveClose);
			setBL(0x0200DCFC, (u32)dsiSaveGetLength);
			setBL(0x0200DD10, (u32)dsiSaveClose);
			setBL(0x0200DD30, (u32)dsiSaveSeek);
			setBL(0x0200DD48, (u32)dsiSaveWrite);
			setBL(0x0200DD5C, (u32)dsiSaveClose);
			setBL(0x0200DDA8, (u32)dsiSaveClose);
		}
	}

	// 3D Mahjong (Europe)
	else if (strcmp(romTid, "KMJP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200937C = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			setBL(0x0200D9B0, (u32)dsiSaveOpen);
			setBL(0x0200DA24, (u32)dsiSaveGetLength);
			setBL(0x0200DA38, (u32)dsiSaveClose);
			setBL(0x0200DA58, (u32)dsiSaveSeek);
			setBL(0x0200DA70, (u32)dsiSaveRead);
			setBL(0x0200DA84, (u32)dsiSaveClose);
			setBL(0x0200DAD8, (u32)dsiSaveClose);
			*(u32*)0x0200DB1C = 0xE1A00000; // nop
			setBL(0x0200DB78, (u32)dsiSaveCreate);
			setBL(0x0200DBCC, (u32)dsiSaveOpen);
			setBL(0x0200DC34, (u32)dsiSaveSetLength);
			setBL(0x0200DC4C, (u32)dsiSaveClose);
			setBL(0x0200DCA0, (u32)dsiSaveGetLength);
			setBL(0x0200DCB4, (u32)dsiSaveClose);
			setBL(0x0200DCD4, (u32)dsiSaveSeek);
			setBL(0x0200DCEC, (u32)dsiSaveWrite);
			setBL(0x0200DD00, (u32)dsiSaveClose);
			setBL(0x0200DD4C, (u32)dsiSaveClose);
		}
	}

	// 3 Heroes: Crystal Soul (USA)
	else if (strcmp(romTid, "K3YE") == 0 && saveOnFlashcard) {
		setBL(0x02027A28, (u32)dsiSaveGetInfo);
		setBL(0x02027A3C, (u32)dsiSaveOpen);
		setBL(0x02027A50, (u32)dsiSaveCreate);
		setBL(0x02027A60, (u32)dsiSaveOpen);
		setBL(0x02027A70, (u32)dsiSaveGetResultCode);
		setBL(0x02027A8C, (u32)dsiSaveCreate);
		setBL(0x02027A9C, (u32)dsiSaveOpen);
		setBL(0x02027AC8, (u32)dsiSaveSeek);
		setBL(0x02027AE0, (u32)dsiSaveWrite);
		setBL(0x02027AF0, (u32)dsiSaveSeek);
		setBL(0x02027B00, (u32)dsiSaveWrite);
		setBL(0x02027B10, (u32)dsiSaveSeek);
		setBL(0x02027B20, (u32)dsiSaveWrite);
		setBL(0x02027B8C, (u32)dsiSaveSeek);
		setBL(0x02027B9C, (u32)dsiSaveWrite);
		setBL(0x02027BA4, (u32)dsiSaveClose);
		setBL(0x02027BF0, (u32)dsiSaveOpen);
		setBL(0x02027C2C, (u32)dsiSaveSeek);
		setBL(0x02027C40, (u32)dsiSaveRead);
		setBL(0x02027C68, (u32)dsiSaveClose);
		setBL(0x02027CE0, (u32)dsiSaveOpen);
		setBL(0x02027CF4, (u32)dsiSaveSeek);
		setBL(0x02027D04, (u32)dsiSaveWrite);
		setBL(0x02027D0C, (u32)dsiSaveClose);
	}

	// 3 Heroes: Crystal Soul (Europe, Australia)
	else if (strcmp(romTid, "K3YV") == 0 && saveOnFlashcard) {
		setBL(0x020314EC, (u32)dsiSaveGetInfo);
		setBL(0x02031500, (u32)dsiSaveOpen);
		setBL(0x02031514, (u32)dsiSaveCreate);
		setBL(0x02031524, (u32)dsiSaveOpen);
		setBL(0x02031534, (u32)dsiSaveGetResultCode);
		setBL(0x02031550, (u32)dsiSaveCreate);
		setBL(0x02031560, (u32)dsiSaveOpen);
		setBL(0x0203158C, (u32)dsiSaveSeek);
		setBL(0x020315A4, (u32)dsiSaveWrite);
		setBL(0x020315B4, (u32)dsiSaveSeek);
		setBL(0x020315C4, (u32)dsiSaveWrite);
		setBL(0x020315D4, (u32)dsiSaveSeek);
		setBL(0x020315E4, (u32)dsiSaveWrite);
		setBL(0x02031650, (u32)dsiSaveSeek);
		setBL(0x02031660, (u32)dsiSaveWrite);
		setBL(0x02031668, (u32)dsiSaveClose);
		setBL(0x020316B4, (u32)dsiSaveOpen);
		setBL(0x020316F0, (u32)dsiSaveSeek);
		setBL(0x02031704, (u32)dsiSaveRead);
		setBL(0x0203172C, (u32)dsiSaveClose);
		setBL(0x020317A4, (u32)dsiSaveOpen);
		setBL(0x020317B8, (u32)dsiSaveSeek);
		setBL(0x020317C8, (u32)dsiSaveWrite);
		setBL(0x020317D0, (u32)dsiSaveClose);
	}

	// 3 Punten Katou Itsu: Bakumatsu Kuizu He (Japan)
	// `dataPub:/user.bin` is read when exists, but only `dataPub:/common.bin` is created and written, leaving `dataPub:/user.bin` unused (pretty sure)
	else if (strcmp(romTid, "K3BJ") == 0 && saveOnFlashcard) {
		setBL(0x0202E680, (u32)dsiSaveOpen);
		setBL(0x0202E6A8, (u32)dsiSaveRead);
		setBL(0x0202E6C0, (u32)dsiSaveClose);
		setBL(0x0202E708, (u32)dsiSaveCreate);
		setBL(0x0202E718, (u32)dsiSaveOpen);
		setBL(0x0202E744, (u32)dsiSaveSetLength);
		setBL(0x0202E764, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202E7C0, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202E7D0, (u32)dsiSaveOpen);
		setBL(0x0202E7F8, (u32)dsiSaveSetLength);
		setBL(0x0202E818, (u32)dsiSaveWrite);
		setBL(0x0202E830, (u32)dsiSaveClose);
		setBL(0x0202E880, (u32)dsiSaveClose);
	}

	// 3 Punten Katou Itsu: Higashi Nihon Sengoku Kuizu He (Japan)
	// 3 Punten Katou Itsu: Nishinihon Sengoku Kuizu He (Japan)
	// `dataPub:/user.bin` is read when exists, but only `dataPub:/common.bin` is created and written, leaving `dataPub:/user.bin` unused (pretty sure)
	else if ((strcmp(romTid, "KHGJ") == 0 || strcmp(romTid, "K24J") == 0) && saveOnFlashcard) {
		setBL(0x0202EEAC, (u32)dsiSaveOpen);
		setBL(0x0202EED4, (u32)dsiSaveRead);
		setBL(0x0202EEEC, (u32)dsiSaveClose);
		setBL(0x0202EF34, (u32)dsiSaveCreate);
		setBL(0x0202EF44, (u32)dsiSaveOpen);
		setBL(0x0202EF70, (u32)dsiSaveSetLength);
		setBL(0x0202EF90, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202EFEC, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202EFFC, (u32)dsiSaveOpen);
		setBL(0x0202F024, (u32)dsiSaveSetLength);
		setBL(0x0202F044, (u32)dsiSaveWrite);
		setBL(0x0202F05C, (u32)dsiSaveClose);
		setBL(0x0202F0AC, (u32)dsiSaveClose);
	}

	// 3450 Algo (Japan)
	else if (strcmp(romTid, "K5RJ") == 0 && saveOnFlashcard) {
		setBL(0x02041D54, (u32)dsiSaveOpen);
		setBL(0x02041D6C, (u32)dsiSaveGetLength);
		setBL(0x02041D98, (u32)dsiSaveRead);
		setBL(0x02041DA0, (u32)dsiSaveClose);
		setBL(0x02041DDC, (u32)dsiSaveCreate);
		setBL(0x02041DEC, (u32)dsiSaveOpen);
		setBL(0x02041DFC, (u32)dsiSaveGetResultCode);
		setBL(0x02041E20, (u32)dsiSaveSetLength);
		setBL(0x02041E30, (u32)dsiSaveWrite);
		setBL(0x02041E38, (u32)dsiSaveClose);
		*(u32*)0x02086F54 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
	}

	// 4 Elements (USA)
	else if (strcmp(romTid, "K7AE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0207B9E8, (u32)dsiSaveGetLength);
			setBL(0x0207B9FC, (u32)dsiSaveSetLength);
			setBL(0x0207BA10, (u32)dsiSaveSeek);
			setBL(0x0207BA20, (u32)dsiSaveWrite);
			setBL(0x0207BA28, (u32)dsiSaveClose);
			setBL(0x0207BAFC, (u32)dsiSaveGetLength);
			setBL(0x0207BB10, (u32)dsiSaveSetLength);
			setBL(0x0207BB24, (u32)dsiSaveSeek);
			setBL(0x0207BB34, (u32)dsiSaveRead);
			setBL(0x0207BB3C, (u32)dsiSaveClose);
			setBL(0x0207BBD8, (u32)dsiSaveOpen);
			setBL(0x0207BC2C, (u32)dsiSaveCreate);
			setBL(0x0207BC4C, (u32)dsiSaveGetResultCode);
			*(u32*)0x0207BC84 = 0xE3A00001; // mov r0, #1
		}
		if (!twlFontFound) {
			*(u32*)0x0207C760 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
	}

	// 4 Elements (Europe, Australia)
	else if (strcmp(romTid, "K7AV") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0207BA30, (u32)dsiSaveGetLength);
			setBL(0x0207BA44, (u32)dsiSaveSetLength);
			setBL(0x0207BA58, (u32)dsiSaveSeek);
			setBL(0x0207BA68, (u32)dsiSaveWrite);
			setBL(0x0207BA70, (u32)dsiSaveClose);
			setBL(0x0207BB44, (u32)dsiSaveGetLength);
			setBL(0x0207BB58, (u32)dsiSaveSetLength);
			setBL(0x0207BB6C, (u32)dsiSaveSeek);
			setBL(0x0207BB7C, (u32)dsiSaveRead);
			setBL(0x0207BB84, (u32)dsiSaveClose);
			setBL(0x0207BC20, (u32)dsiSaveOpen);
			setBL(0x0207BC74, (u32)dsiSaveCreate);
			setBL(0x0207BC94, (u32)dsiSaveGetResultCode);
			*(u32*)0x0207BCCC = 0xE3A00001; // mov r0, #1
		}
		if (!twlFontFound) {
			*(u32*)0x0207C7A8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
	}

	// 4 Travellers: Play French (USA)
	else if (strcmp(romTid, "KTFE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005320 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			setBL(0x0205086C, (u32)dsiSaveCreate);
			setBL(0x02050880, (u32)dsiSaveCreate);
			setBL(0x0205089C, (u32)dsiSaveGetResultCode);
			setBL(0x02050930, (u32)dsiSaveOpen);
			setBL(0x02050980, (u32)dsiSaveSeek);
			setBL(0x02050990, (u32)dsiSaveRead);
			setBL(0x020509A4, (u32)dsiSaveRead);
			setBL(0x020509B0, (u32)dsiSaveClose);
			setBL(0x02050A10, (u32)dsiSaveOpen);
			setBL(0x02050A28, (u32)dsiSaveSeek);
			setBL(0x02050A38, (u32)dsiSaveRead);
			setBL(0x02050A48, (u32)dsiSaveRead);
			setBL(0x02050A50, (u32)dsiSaveClose);
			setBL(0x02050B08, (u32)dsiSaveOpen);
			setBL(0x02050B58, (u32)dsiSaveCreate);
			setBL(0x02050B64, (u32)dsiSaveCreate);
			setBL(0x02050B94, (u32)dsiSaveOpen);
			setBL(0x02050C10, (u32)dsiSaveSeek);
			setBL(0x02050C24, (u32)dsiSaveWrite);
			setBL(0x02050C38, (u32)dsiSaveWrite);
			setBL(0x02050C40, (u32)dsiSaveClose);
			setBL(0x02050C50, (u32)dsiSaveOpen);
			setBL(0x02050C88, (u32)dsiSaveSeek);
			setBL(0x02050C98, (u32)dsiSaveWrite);
			setBL(0x02050CAC, (u32)dsiSaveWrite);
			setBL(0x02050CB8, (u32)dsiSaveClose);
			setBL(0x02052874, (u32)dsiSaveCreate);
			setBL(0x02052880, (u32)dsiSaveCreate);
		}
	}

	// 4 Travellers: Play French (Europe)
	// 4 Travellers: Play French (Australia)
	else if (strcmp(romTid, "KTFP") == 0 || strcmp(romTid, "KTFU") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020052DC = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			if (ndsHeader->gameCode[3] == 'P') {
				setBL(0x02040298, (u32)dsiSaveCreate);
				setBL(0x020402AC, (u32)dsiSaveCreate);
				setBL(0x020402C8, (u32)dsiSaveGetResultCode);
				setBL(0x0204035C, (u32)dsiSaveOpen);
				setBL(0x020403AC, (u32)dsiSaveSeek);
				setBL(0x020403BC, (u32)dsiSaveRead);
				setBL(0x020403D0, (u32)dsiSaveRead);
				setBL(0x020403DC, (u32)dsiSaveClose);
				setBL(0x0204043C, (u32)dsiSaveOpen);
				setBL(0x02040454, (u32)dsiSaveSeek);
				setBL(0x02040464, (u32)dsiSaveRead);
				setBL(0x02040474, (u32)dsiSaveRead);
				setBL(0x0204047C, (u32)dsiSaveClose);
				setBL(0x02040534, (u32)dsiSaveOpen);
				setBL(0x02040584, (u32)dsiSaveCreate);
				setBL(0x02040590, (u32)dsiSaveCreate);
				setBL(0x020405C0, (u32)dsiSaveOpen);
				setBL(0x0204063C, (u32)dsiSaveSeek);
				setBL(0x02040650, (u32)dsiSaveWrite);
				setBL(0x02040664, (u32)dsiSaveWrite);
				setBL(0x0204066C, (u32)dsiSaveClose);
				setBL(0x0204067C, (u32)dsiSaveOpen);
				setBL(0x020406B4, (u32)dsiSaveSeek);
				setBL(0x020406C4, (u32)dsiSaveWrite);
				setBL(0x020406D8, (u32)dsiSaveWrite);
				setBL(0x020406E4, (u32)dsiSaveClose);
				setBL(0x020422A0, (u32)dsiSaveCreate);
				setBL(0x020422AC, (u32)dsiSaveCreate);
			} else {
				setBL(0x02050B18, (u32)dsiSaveCreate);
				setBL(0x02050B2C, (u32)dsiSaveCreate);
				setBL(0x02050B48, (u32)dsiSaveGetResultCode);
				setBL(0x02050BDC, (u32)dsiSaveOpen);
				setBL(0x02050C2C, (u32)dsiSaveSeek);
				setBL(0x02050C3C, (u32)dsiSaveRead);
				setBL(0x02050C50, (u32)dsiSaveRead);
				setBL(0x02050C5C, (u32)dsiSaveClose);
				setBL(0x02050CBC, (u32)dsiSaveOpen);
				setBL(0x02050CD4, (u32)dsiSaveSeek);
				setBL(0x02050CE4, (u32)dsiSaveRead);
				setBL(0x02050CF4, (u32)dsiSaveRead);
				setBL(0x02050CFC, (u32)dsiSaveClose);
				setBL(0x02050DB4, (u32)dsiSaveOpen);
				setBL(0x02050E04, (u32)dsiSaveCreate);
				setBL(0x02050E10, (u32)dsiSaveCreate);
				setBL(0x02050E40, (u32)dsiSaveOpen);
				setBL(0x02050EBC, (u32)dsiSaveSeek);
				setBL(0x02050ED0, (u32)dsiSaveWrite);
				setBL(0x02050EE4, (u32)dsiSaveWrite);
				setBL(0x02050EEC, (u32)dsiSaveClose);
				setBL(0x02050EFC, (u32)dsiSaveOpen);
				setBL(0x02050F34, (u32)dsiSaveSeek);
				setBL(0x02050F44, (u32)dsiSaveWrite);
				setBL(0x02050F58, (u32)dsiSaveWrite);
				setBL(0x02050F64, (u32)dsiSaveClose);
				setBL(0x02052B20, (u32)dsiSaveCreate);
				setBL(0x02052B2C, (u32)dsiSaveCreate);
			}
		}
	}

	// 4 Travellers: Play Spanish (USA)
	else if (strcmp(romTid, "KTSE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02004CC0 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			setBL(0x02050E8C, (u32)dsiSaveCreate);
			setBL(0x02050EA0, (u32)dsiSaveCreate);
			setBL(0x02050EBC, (u32)dsiSaveGetResultCode);
			setBL(0x02050F50, (u32)dsiSaveOpen);
			setBL(0x02050FA0, (u32)dsiSaveSeek);
			setBL(0x02050FB0, (u32)dsiSaveRead);
			setBL(0x02050FC4, (u32)dsiSaveRead);
			setBL(0x02050FD0, (u32)dsiSaveClose);
			setBL(0x02051030, (u32)dsiSaveOpen);
			setBL(0x02051048, (u32)dsiSaveSeek);
			setBL(0x02051058, (u32)dsiSaveRead);
			setBL(0x02051068, (u32)dsiSaveRead);
			setBL(0x02051070, (u32)dsiSaveClose);
			setBL(0x02051128, (u32)dsiSaveOpen);
			setBL(0x02051178, (u32)dsiSaveCreate);
			setBL(0x02051184, (u32)dsiSaveCreate);
			setBL(0x020511B4, (u32)dsiSaveOpen);
			setBL(0x02051230, (u32)dsiSaveSeek);
			setBL(0x02051244, (u32)dsiSaveWrite);
			setBL(0x02051258, (u32)dsiSaveWrite);
			setBL(0x02051260, (u32)dsiSaveClose);
			setBL(0x02051270, (u32)dsiSaveOpen);
			setBL(0x020512A8, (u32)dsiSaveSeek);
			setBL(0x020512B8, (u32)dsiSaveWrite);
			setBL(0x020512CC, (u32)dsiSaveWrite);
			setBL(0x020512D8, (u32)dsiSaveClose);
			setBL(0x02052E80, (u32)dsiSaveCreate);
			setBL(0x02052E8C, (u32)dsiSaveCreate);
		}
	}

	// 4 Travellers: Play Spanish (Europe)
	// 4 Travellers: Play Spanish (Australia)
	else if (strcmp(romTid, "KTSP") == 0 || strcmp(romTid, "KTSU") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02004C7C = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			u32 offsetChange = (ndsHeader->gameCode[3] == 'U') ? 0x4C : 0;

			setBL(0x020511A4-offsetChange, (u32)dsiSaveCreate);
			setBL(0x020511B8-offsetChange, (u32)dsiSaveCreate);
			setBL(0x020511D4-offsetChange, (u32)dsiSaveGetResultCode);
			setBL(0x02051268-offsetChange, (u32)dsiSaveOpen);
			setBL(0x020512B8-offsetChange, (u32)dsiSaveSeek);
			setBL(0x020512C8-offsetChange, (u32)dsiSaveRead);
			setBL(0x020512DC-offsetChange, (u32)dsiSaveRead);
			setBL(0x020512E8-offsetChange, (u32)dsiSaveClose);
			setBL(0x02051348-offsetChange, (u32)dsiSaveOpen);
			setBL(0x02051360-offsetChange, (u32)dsiSaveSeek);
			setBL(0x02051370-offsetChange, (u32)dsiSaveRead);
			setBL(0x02051380-offsetChange, (u32)dsiSaveRead);
			setBL(0x02051388-offsetChange, (u32)dsiSaveClose);
			setBL(0x02051440-offsetChange, (u32)dsiSaveOpen);
			setBL(0x02051490-offsetChange, (u32)dsiSaveCreate);
			setBL(0x0205149C-offsetChange, (u32)dsiSaveCreate);
			setBL(0x020514CC-offsetChange, (u32)dsiSaveOpen);
			setBL(0x02051548-offsetChange, (u32)dsiSaveSeek);
			setBL(0x0205155C-offsetChange, (u32)dsiSaveWrite);
			setBL(0x02051570-offsetChange, (u32)dsiSaveWrite);
			setBL(0x02051578-offsetChange, (u32)dsiSaveClose);
			setBL(0x02051588-offsetChange, (u32)dsiSaveOpen);
			setBL(0x020515C0-offsetChange, (u32)dsiSaveSeek);
			setBL(0x020515D0-offsetChange, (u32)dsiSaveWrite);
			setBL(0x020515E4-offsetChange, (u32)dsiSaveWrite);
			setBL(0x020515F0-offsetChange, (u32)dsiSaveClose);
			setBL(0x020531AC-offsetChange, (u32)dsiSaveCreate);
			setBL(0x020531B8-offsetChange, (u32)dsiSaveCreate);
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
		// *(u32*)0x0200E010 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
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
		// *(u32*)0x0200DFC0 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
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

	// 5 in 1 Mahjong (USA)
	// 5 in 1 Mahjong (Europe)
	else if (strcmp(romTid, "KRJE") == 0 || strcmp(romTid, "KRJP") == 0) {
		if (saveOnFlashcard) {
			tonccpy((u32*)0x02013098, dsiSaveGetResultCode, 0xC);
			setBL(0x02030C18, (u32)dsiSaveOpen);
			setBL(0x02030C28, (u32)dsiSaveClose);
			setBL(0x0203104C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02031070, (u32)dsiSaveOpen);
			setBL(0x02031084, (u32)dsiSaveSetLength);
			setBL(0x02031094, (u32)dsiSaveClose);
			setBL(0x02031118, (u32)dsiSaveOpen);
			setBL(0x020311A8, (u32)dsiSaveClose);
			setBL(0x02031230, (u32)dsiSaveSeek);
			setBL(0x02031248, (u32)dsiSaveRead);
			setBL(0x020312D0, (u32)dsiSaveSeek);
			setBL(0x020312E8, (u32)dsiSaveWrite);
		}
		if (!twlFontFound) {
			*(u32*)0x02056970 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// 505 Tangram (USA)
	else if (strcmp(romTid, "K2OE") == 0 && saveOnFlashcard) {
		setBL(0x020100B8, (u32)dsiSaveOpen);
		setBL(0x0201012C, (u32)dsiSaveGetLength);
		setBL(0x02010140, (u32)dsiSaveClose);
		setBL(0x02010160, (u32)dsiSaveSeek);
		setBL(0x02010178, (u32)dsiSaveRead);
		setBL(0x0201018C, (u32)dsiSaveClose);
		setBL(0x020101E0, (u32)dsiSaveClose);
		*(u32*)0x02010224 = 0xE1A00000; // nop
		setBL(0x02010280, (u32)dsiSaveCreate);
		setBL(0x020102D4, (u32)dsiSaveOpen);
		setBL(0x0201033C, (u32)dsiSaveSetLength);
		setBL(0x02010354, (u32)dsiSaveClose);
		setBL(0x020103A8, (u32)dsiSaveGetLength);
		setBL(0x020103BC, (u32)dsiSaveClose);
		setBL(0x020103DC, (u32)dsiSaveSeek);
		setBL(0x020103F4, (u32)dsiSaveWrite);
		setBL(0x02010408, (u32)dsiSaveClose);
		setBL(0x02010454, (u32)dsiSaveClose);
	}

	// 505 Tangram (Europe)
	else if (strcmp(romTid, "K2OP") == 0 && saveOnFlashcard) {
		setBL(0x0200FED0, (u32)dsiSaveOpen);
		setBL(0x0200FF44, (u32)dsiSaveGetLength);
		setBL(0x0200FF58, (u32)dsiSaveClose);
		setBL(0x0200FF78, (u32)dsiSaveSeek);
		setBL(0x0200FF90, (u32)dsiSaveRead);
		setBL(0x0200FFA4, (u32)dsiSaveClose);
		setBL(0x0200FFF8, (u32)dsiSaveClose);
		*(u32*)0x0201003C = 0xE1A00000; // nop
		setBL(0x02010098, (u32)dsiSaveCreate);
		setBL(0x020100EC, (u32)dsiSaveOpen);
		setBL(0x02010154, (u32)dsiSaveSetLength);
		setBL(0x0201016C, (u32)dsiSaveClose);
		setBL(0x020101C0, (u32)dsiSaveGetLength);
		setBL(0x020101D4, (u32)dsiSaveClose);
		setBL(0x020101F4, (u32)dsiSaveSeek);
		setBL(0x0201020C, (u32)dsiSaveWrite);
		setBL(0x02010220, (u32)dsiSaveClose);
		setBL(0x0201026C, (u32)dsiSaveClose);
	}

	// 505 Tangram (Japan)
	else if (strcmp(romTid, "K2OJ") == 0 && saveOnFlashcard) {
		setBL(0x0200F858, (u32)dsiSaveOpen);
		setBL(0x0200F8CC, (u32)dsiSaveGetLength);
		setBL(0x0200F8E0, (u32)dsiSaveClose);
		setBL(0x0200F900, (u32)dsiSaveSeek);
		setBL(0x0200F918, (u32)dsiSaveRead);
		setBL(0x0200F92C, (u32)dsiSaveClose);
		setBL(0x0200F980, (u32)dsiSaveClose);
		*(u32*)0x0200F9C4 = 0xE1A00000; // nop
		setBL(0x0200FA20, (u32)dsiSaveCreate);
		setBL(0x0200FA74, (u32)dsiSaveOpen);
		setBL(0x0200FADC, (u32)dsiSaveSetLength);
		setBL(0x0200FAF4, (u32)dsiSaveClose);
		setBL(0x0200FB48, (u32)dsiSaveGetLength);
		setBL(0x0200FB5C, (u32)dsiSaveClose);
		setBL(0x0200FB7C, (u32)dsiSaveSeek);
		setBL(0x0200FB94, (u32)dsiSaveWrite);
		setBL(0x0200FBA8, (u32)dsiSaveClose);
		setBL(0x0200FBF4, (u32)dsiSaveClose);
	}

	// 7 Card Games (USA)
	else if (strcmp(romTid, "K7CE") == 0) {
		if (saveOnFlashcard) {
			tonccpy((u32*)0x020130EC, dsiSaveGetResultCode, 0xC);
			setBL(0x02034820, (u32)dsiSaveOpen);
			setBL(0x02034830, (u32)dsiSaveClose);
			setBL(0x02034C24, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02034C48, (u32)dsiSaveOpen);
			setBL(0x02034C5C, (u32)dsiSaveSetLength);
			setBL(0x02034C6C, (u32)dsiSaveClose);
			setBL(0x02034CF4, (u32)dsiSaveOpen);
			setBL(0x02034D84, (u32)dsiSaveClose);
			setBL(0x02034E0C, (u32)dsiSaveSeek);
			setBL(0x02034E24, (u32)dsiSaveRead);
			setBL(0x02034EAC, (u32)dsiSaveSeek);
			setBL(0x02034EC4, (u32)dsiSaveWrite);
		}
		if (!twlFontFound) {
			*(u32*)0x020417CC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
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

	// Aa! Nikaku Dori (Japan)
	else if (strcmp(romTid, "K2KJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050D4 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
			setBL(0x02028DF0, (u32)dsiSaveOpen);
			setBL(0x02028E0C, (u32)dsiSaveGetLength);
			setBL(0x02028E1C, (u32)dsiSaveRead);
			setBL(0x02028E24, (u32)dsiSaveClose);
			setBL(0x02028EBC, (u32)dsiSaveOpen);
			setBL(0x02028EE4, (u32)dsiSaveCreate);
			setBL(0x02028F0C, (u32)dsiSaveOpen);
			setBL(0x02028F38, (u32)dsiSaveSetLength);
			setBL(0x02028F50, (u32)dsiSaveWrite);
			setBL(0x02028F58, (u32)dsiSaveClose);
			setBL(0x02029340, (u32)dsiSaveGetResultCode);
		}
	}

	// Absolute BrickBuster (USA)
	else if (strcmp(romTid, "K6QE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020053E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (!saveOnFlashcard) { // Redirect otherPub to dataPub
			toncset((char*)0x02095CD4, 0, 9);
			tonccpy((char*)0x02095CD4, dataPub, strlen(dataPub));
			toncset((char*)0x02095CE8, 0, 9);
			tonccpy((char*)0x02095CE8, dataPub, strlen(dataPub));
		} else {
			*(u32*)0x02055B74 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02055B78 = 0xE12FFF1E; // bx lr
			*(u32*)0x02055C48 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02055C4C = 0xE12FFF1E; // bx lr
		}
	}

	// Absolute Chess (USA)
	else if (strcmp(romTid, "KCZE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020053E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (!saveOnFlashcard) { // Redirect otherPub to dataPub
			toncset((char*)0x0209E9C8, 0, 9);
			tonccpy((char*)0x0209E9C8, dataPub, strlen(dataPub));
			toncset((char*)0x0209E9DC, 0, 9);
			tonccpy((char*)0x0209E9DC, dataPub, strlen(dataPub));
		}
	}

	// Absolute Reversi (USA)
	else if (strcmp(romTid, "KA8E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020053E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (!saveOnFlashcard) { // Redirect otherPub to dataPub
			toncset((char*)0x0209D220, 0, 9);
			tonccpy((char*)0x0209D220, dataPub, strlen(dataPub));
			toncset((char*)0x0209D234, 0, 9);
			tonccpy((char*)0x0209D234, dataPub, strlen(dataPub));
		}
	}

	// Ah! Heaven (USA)
	// Ah! Heaven (Europe)
	else if (strcmp(romTid, "K5HE") == 0 || strcmp(romTid, "K5HP") == 0) {
		//tonccpy((u32*)0x020102FC, dsiSaveGetResultCode, 0xC);
		/* setBL(0x0201E3C0, (u32)dsiSaveCreate);
		setBL(0x0201E3E8, (u32)dsiSaveOpen);
		setBL(0x0201E428, (u32)dsiSaveCreate);
		setBL(0x0201E438, (u32)dsiSaveOpen);
		setBL(0x0201E468, (u32)dsiSaveOpen);
		setBL(0x0201E540, (u32)dsiSaveRead);
		setBL(0x0201E560, (u32)dsiSaveSeek);
		setBL(0x0201E578, (u32)dsiSaveWrite);
		setBL(0x0201E588, (u32)dsiSaveSeek); */
		if (!twlFontFound) {
			*(u32*)0x0201FD04 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			*(u32*)0x02029C68 = 0xE12FFF1E; // bx lr
			*(u32*)0x02029D14 = 0xE12FFF1E; // bx lr
		}
		/* setBL(0x02029CFC, (u32)dsiSaveClose);
		setBL(0x02029DB8, (u32)dsiSaveClose);
		setBL(0x02029DCC, (u32)dsiSaveClose); */
	}

	// Alien Puzzle Adventure (USA)
	// Alien Puzzle Adventure (Europe, Australia)
	else if (strcmp(romTid, "KP7E") == 0 || strcmp(romTid, "KP7V") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200D894, (u32)dsiSaveCreate);
			setBL(0x0200D8A8, (u32)dsiSaveOpen);
			setBL(0x0200D8D0, (u32)dsiSaveCreate);
			setBL(0x0200D8E8, (u32)dsiSaveOpen);
			setBL(0x0200D8F4, (u32)dsiSaveSetLength);
			setBL(0x0200D90C, (u32)dsiSaveWrite);
			setBL(0x0200D91C, (u32)dsiSaveClose);
			setBL(0x0200D9A0, (u32)dsiSaveWrite);
			setBL(0x0200D9A8, (u32)dsiSaveClose);
			*(u32*)0x0200D9D8 = 0xE12FFF1E; // bx lr
			setBL(0x0200DA24, (u32)dsiSaveOpen);
			*(u32*)0x0200DA3C = 0xE1A00000; // nop
			*(u32*)0x0200DA48 = 0xE3A00008; // mov r0, #8
			*(u32*)0x0200DB2C = 0xE1A00000; // nop (dsiSaveCreateDir)
			*(u32*)0x0200DB34 = 0xE3A00008; // mov r0, #8
			*(u32*)0x0200DBA8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			*(u32*)0x0200DBC0 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
			setBL(0x0200DC44, (u32)dsiSaveGetLength);
			setBL(0x0200DC64, (u32)dsiSaveRead);
			setBL(0x0200DC6C, (u32)dsiSaveClose);
			*(u32*)0x0200DDB4 = 0xE1A00000; // nop
			if (ndsHeader->gameCode[3] == 'E') {
				tonccpy((u32*)0x0203FD80, dsiSaveGetResultCode, 0xC);
			} else {
				tonccpy((u32*)0x0203FCD4, dsiSaveGetResultCode, 0xC);
			}
		}
		if (!twlFontFound) {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x02017864 = 0xE1A00000; // nop (Skip Manual screen)
			} else {
				*(u32*)0x020177AC = 0xE1A00000; // nop (Skip Manual screen)
			}
		}
	}

	// All-Star Air Hockey (USA)
	else if (strcmp(romTid, "KAOE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x020056F8, (u32)dsiSaveOpen);
			setBL(0x0200570C, (u32)dsiSaveGetLength);
			setBL(0x02005720, (u32)dsiSaveRead);
			setBL(0x02005730, (u32)dsiSaveClose);
			setBL(0x020057E0, (u32)dsiSaveCreate);
			setBL(0x020057FC, (u32)dsiSaveOpen);
			setBL(0x02005820, (u32)dsiSaveSetLength);
			setBL(0x02005830, (u32)dsiSaveWrite);
			setBL(0x02005838, (u32)dsiSaveClose);
			setBL(0x020058E0, (u32)dsiSaveOpen);
			setBL(0x020058F8, (u32)dsiSaveSetLength);
			setBL(0x02005908, (u32)dsiSaveWrite);
			setBL(0x02005914, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			setB(0x020104DC, 0x020105D4); // Skip Manual screen
		}
	}

	// Amakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF2J") == 0 && saveOnFlashcard) {
		setBL(0x0203C1C8, (u32)dsiSaveOpen);
		setBL(0x0203C1F4, (u32)dsiSaveRead);
		setBL(0x0203C204, (u32)dsiSaveClose);
		setBL(0x0203C220, (u32)dsiSaveClose);
		*(u32*)0x0203C274 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0203C2B0 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0203C2BC, (u32)dsiSaveCreate);
		setBL(0x0203C2CC, (u32)dsiSaveOpen);
		setBL(0x0203C2F8, (u32)dsiSaveSetLength);
		setBL(0x0203C308, (u32)dsiSaveClose);
		setBL(0x0203C32C, (u32)dsiSaveWrite);
		setBL(0x0203C33C, (u32)dsiSaveClose);
		setBL(0x0203C358, (u32)dsiSaveClose);
	}

	// Animal Puzzle Adventure (USA)
	else if (strcmp(romTid, "KPCE") == 0 && saveOnFlashcard) {
		setBL(0x020265B0, (u32)dsiSaveOpen);
		setBL(0x020265C4, (u32)dsiSaveGetLength);
		setBL(0x020265D4, (u32)dsiSaveRead);
		setBL(0x020265DC, (u32)dsiSaveClose);
		*(u32*)0x0202663C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0202667C = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02026708, (u32)dsiSaveGetInfo);
		setBL(0x02026714, (u32)dsiSaveCreate);
		setBL(0x02026724, (u32)dsiSaveOpen);
		setBL(0x02026754, (u32)dsiSaveSetLength);
		setBL(0x02026780, (u32)dsiSaveWrite);
		setBL(0x02026788, (u32)dsiSaveClose);
		setBL(0x020268FC, (u32)dsiSaveGetInfo);
		setBL(0x02026930, (u32)dsiSaveCreate);
		setBL(0x02026940, (u32)dsiSaveOpen);
		setBL(0x02026988, (u32)dsiSaveSetLength);
		setBL(0x020269D8, (u32)dsiSaveWrite);
		setBL(0x020269E0, (u32)dsiSaveClose);
		tonccpy((u32*)0x0202E838, dsiSaveGetResultCode, 0xC);
	}

	// Animal Puzzle Adventure (Europe, Australia)
	else if (strcmp(romTid, "KPCV") == 0 && saveOnFlashcard) {
		setBL(0x02025674, (u32)dsiSaveOpen);
		setBL(0x02025688, (u32)dsiSaveGetLength);
		setBL(0x02025698, (u32)dsiSaveRead);
		setBL(0x020256A0, (u32)dsiSaveClose);
		*(u32*)0x02025700 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02025740 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x020257CC, (u32)dsiSaveGetInfo);
		setBL(0x020257D8, (u32)dsiSaveCreate);
		setBL(0x020257E8, (u32)dsiSaveOpen);
		setBL(0x02025818, (u32)dsiSaveSetLength);
		setBL(0x02025844, (u32)dsiSaveWrite);
		setBL(0x0202584C, (u32)dsiSaveClose);
		setBL(0x020259C0, (u32)dsiSaveGetInfo);
		setBL(0x020259F4, (u32)dsiSaveCreate);
		setBL(0x02025A04, (u32)dsiSaveOpen);
		setBL(0x02025A4C, (u32)dsiSaveSetLength);
		setBL(0x02025A9C, (u32)dsiSaveWrite);
		setBL(0x02025AA4, (u32)dsiSaveClose);
		tonccpy((u32*)0x0202D8FC, dsiSaveGetResultCode, 0xC);
	}

	// Anne's Doll Studio: Antique Collection (USA)
	// Anne's Doll Studio: Antique Collection (Europe)
	// Anne's Doll Studio: Princess Collection (USA)
	// Anne's Doll Studio: Princess Collection (Europe)
	else if (strcmp(romTid, "KY8E") == 0 || strcmp(romTid, "KY8P") == 0
		   || strcmp(romTid, "K2SE") == 0 || strcmp(romTid, "K2SP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Anne's Doll Studio: Gothic Collection (USA)
	else if (strcmp(romTid, "K54E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Anne's Doll Studio: Lolita Collection (USA)
	// Anne's Doll Studio: Lolita Collection (Europe)
	else if (strcmp(romTid, "KLQE") == 0 || strcmp(romTid, "KLQP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Anne's Doll Studio: Tokyo Collection (USA)
	else if (strcmp(romTid, "KSQE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Anonymous Notes 1: From The Abyss (USA & Europe)
	// Anonymous Notes 2: From The Abyss (USA & Europe)
	else if ((strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0) && ndsHeader->gameCode[3] != 'J') {
		if (saveOnFlashcard) {
			// *(u32*)0x02023DB0 = 0xE3A00001; // mov r0, #1
			// *(u32*)0x02023DB4 = 0xE12FFF1E; // bx lr
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
		}
		if (!twlFontFound) {
			if (ndsHeader->gameCode[2] == 'I') {
				if (ndsHeader->gameCode[3] == 'E') {
					// *(u32*)0x020CE830 = 0xE12FFF1E; // bx lr
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
	}

	// Anonymous Notes 1: From The Abyss (Japan)
	// Anonymous Notes 2: From The Abyss (Japan)
	else if (strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			if (ndsHeader->gameCode[2] == 'I') {
				*(u32*)0x020CF970 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			} else if (ndsHeader->gameCode[2] == '2') {
				*(u32*)0x020D050C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		}
	}

	// Anonymous Notes 3: From The Abyss (USA & Japan)
	// Anonymous Notes 4: From The Abyss (USA & Japan)
	else if (strncmp(romTid, "KV3", 3) == 0 || strncmp(romTid, "KV4", 3) == 0) {
		if (ndsHeader->gameCode[2] == '3') {
			if (ndsHeader->gameCode[3] == 'E') {
				if (saveOnFlashcard) {
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
				}
				if (!twlFontFound) {
					*(u32*)0x020D0120 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			} else {
				if (saveOnFlashcard) {
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
				}
				if (!twlFontFound) {
					*(u32*)0x020CF8AC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			}
		} else if (ndsHeader->gameCode[2] == '4') {
			if (ndsHeader->gameCode[3] == 'E') {
				if (saveOnFlashcard) {
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
				}
				if (!twlFontFound) {
					*(u32*)0x020D0FFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			} else {
				if (saveOnFlashcard) {
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
				}
				if (!twlFontFound) {
					*(u32*)0x020D0590 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			}
		}
	}

	// Anyohaseyo!: Kankokugo Wado Pazuru (Japan)
	else if (strcmp(romTid, "KL8J") == 0) {
		if (saveOnFlashcard) {
			setBL(0x02024128, (u32)dsiSaveClose);
			setBL(0x0202426C, (u32)dsiSaveClose);
			setBL(0x02024408, (u32)dsiSaveOpen);
			setBL(0x02024430, (u32)dsiSaveSeek);
			setBL(0x0202444C, (u32)dsiSaveClose);
			setBL(0x02024464, (u32)dsiSaveRead);
			setBL(0x02024484, (u32)dsiSaveClose);
			setBL(0x02024494, (u32)dsiSaveClose);
			setBL(0x020244D0, (u32)dsiSaveOpen);
			setBL(0x020244E8, (u32)dsiSaveSeek);
			setBL(0x02024500, (u32)dsiSaveRead); // dsiSaveReadAsync
			setBL(0x02024534, (u32)dsiSaveOpen);
			setBL(0x02024554, (u32)dsiSaveSetLength);
			setBL(0x02024564, (u32)dsiSaveClose);
			setBL(0x02024580, (u32)dsiSaveSeek);
			setBL(0x0202459C, (u32)dsiSaveClose);
			setBL(0x020245B4, (u32)dsiSaveWrite);
			setBL(0x020245D8, (u32)dsiSaveClose);
			setBL(0x020245E4, (u32)dsiSaveClose);
			setBL(0x02024620, (u32)dsiSaveOpen);
			setBL(0x02024634, (u32)dsiSaveSetLength);
			setBL(0x0202464C, (u32)dsiSaveSeek);
			setBL(0x02024664, (u32)dsiSaveWrite); // dsiSaveWriteAsync
			setBL(0x020246B8, (u32)dsiSaveCreate);
			setBL(0x020246C0, (u32)dsiSaveGetResultCode);
		}
		if (!twlFontFound) {
			*(u32*)0x020321BC = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x020398E4 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		}
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
	else if (strcmp(romTid, "KAZE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020050A8 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
			setBL(0x02059E20, (u32)dsiSaveCreate);
			setBL(0x02059E3C, (u32)dsiSaveOpen);
			setBL(0x02059E68, (u32)dsiSaveSetLength);
			setBL(0x02059E84, (u32)dsiSaveWrite);
			setBL(0x02059E90, (u32)dsiSaveClose);
			setBL(0x02059F2C, (u32)dsiSaveOpen);
			setBL(0x02059F9C, (u32)dsiSaveRead);
			setBL(0x02059FA8, (u32)dsiSaveClose);
		}
	}

	// ARC Style: Everyday Football (Europe, Australia)
	else if (strcmp(romTid, "KAZV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020050A8 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
			setBL(0x02059EF4, (u32)dsiSaveCreate);
			setBL(0x02059F10, (u32)dsiSaveOpen);
			setBL(0x02059F3C, (u32)dsiSaveSetLength);
			setBL(0x02059F58, (u32)dsiSaveWrite);
			setBL(0x02059F64, (u32)dsiSaveClose);
			setBL(0x0205A000, (u32)dsiSaveOpen);
			setBL(0x0205A070, (u32)dsiSaveRead);
			setBL(0x0205A07C, (u32)dsiSaveClose);
		}
	}

	// ARC Style: Soccer! (Japan)
	// ARC Style: Soccer! (Korea)
	else if (strcmp(romTid, "KAZJ") == 0 || strcmp(romTid, "KAZK") == 0) {
		if (ndsHeader->gameCode[3] == 'J' ? twlFontFound : korFontFound) {
			*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020050A8 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
			setBL(0x02059E04, (u32)dsiSaveCreate);
			setBL(0x02059E20, (u32)dsiSaveOpen);
			setBL(0x02059E4C, (u32)dsiSaveSetLength);
			setBL(0x02059E68, (u32)dsiSaveWrite);
			setBL(0x02059E74, (u32)dsiSaveClose);
			setBL(0x02059F04, (u32)dsiSaveOpen);
			setBL(0x02059F74, (u32)dsiSaveRead);
			setBL(0x02059F80, (u32)dsiSaveClose);
		}
	}

	// Arcade Bowling (USA)
	else if (strcmp(romTid, "K4BE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0201F8EC, (u32)dsiSaveOpen);
			setBL(0x0201F900, (u32)dsiSaveGetLength);
			setBL(0x0201F914, (u32)dsiSaveRead);
			setBL(0x0201F924, (u32)dsiSaveClose);
			setBL(0x0201F9D4, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0201F9F0, (u32)dsiSaveOpen);
			setBL(0x0201FA08, (u32)dsiSaveSetLength);
			setBL(0x0201FA18, (u32)dsiSaveWrite);
			setBL(0x0201FA20, (u32)dsiSaveClose);
			setBL(0x0201FAA0, (u32)dsiSaveOpen);
			setBL(0x0201FAB8, (u32)dsiSaveSetLength);
			setBL(0x0201FAC8, (u32)dsiSaveWrite);
			setBL(0x0201FAD4, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			setB(0x0202036C, 0x02020A78); // Skip Manual screen
		}
	}

	// Arcade Hoops Basketball (USA)
	else if (strcmp(romTid, "KSAE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x020066D0, (u32)dsiSaveOpen);
			setBL(0x020066E4, (u32)dsiSaveGetLength);
			setBL(0x020066F8, (u32)dsiSaveRead);
			setBL(0x02006708, (u32)dsiSaveClose);
			setBL(0x020067B8, (u32)dsiSaveCreate);
			setBL(0x020067D4, (u32)dsiSaveOpen);
			setBL(0x020067EC, (u32)dsiSaveSetLength);
			setBL(0x020067FC, (u32)dsiSaveWrite);
			setBL(0x02006804, (u32)dsiSaveClose);
			setBL(0x02006890, (u32)dsiSaveOpen);
			setBL(0x020068A8, (u32)dsiSaveSetLength);
			setBL(0x020068B8, (u32)dsiSaveWrite);
			setBL(0x020068C4, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			setB(0x02007064, 0x02007168); // Skip Manual screen
		}
	}

	// Armada (USA)
	else if (strcmp(romTid, "KRDE") == 0) {
		if (saveOnFlashcard) {
			*(u32*)0x0202D11C = 0xE3A00003; // mov r0, #3
			*(u32*)0x0202D210 = 0xE3A00003; // mov r0, #3
			*(u32*)0x0202D290 = 0xE3A00003; // mov r0, #3
			*(u32*)0x0202E704 = 0xE3A00003; // mov r0, #3
			*(u32*)0x020420B4 = 0xE3A00003; // mov r0, #3
			setBL(0x020421CC, (u32)dsiSaveOpen);
			setBL(0x020421E4, (u32)dsiSaveClose);
			setBL(0x02042264, (u32)dsiSaveOpen);
			setBL(0x0204227C, (u32)dsiSaveGetLength);
			setBL(0x02042288, (u32)dsiSaveClose);
			setBL(0x020422F8, (u32)dsiSaveOpen);
			setBL(0x02042314, (u32)dsiSaveGetLength);
			setBL(0x02042330, (u32)dsiSaveRead);
			setBL(0x0204233C, (u32)dsiSaveClose);
			setBL(0x020423BC, (u32)dsiSaveCreate);
			setBL(0x020423D0, (u32)dsiSaveOpen);
			setBL(0x020423F4, (u32)dsiSaveSeek);
			setBL(0x02042404, (u32)dsiSaveWrite);
			setBL(0x02042410, (u32)dsiSaveClose);
		}
		*(u32*)0x020424FC = 0xE3A00000; // mov r0, #?
		*(u32*)0x020424FC += twlFontFound ? 3 : 1;
	}

	// Army Defender (USA)
	// Army Defender (Europe)
	else if (strncmp(romTid, "KAY", 3) == 0 && saveOnFlashcard) {
		// *(u32*)0x020051BC = 0xE3A00000; // mov r0, #0
		// *(u32*)0x020051C0 = 0xE12FFF1E; // bx lr
		// *(u32*)0x02005204 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02005208 = 0xE12FFF1E; // bx lr
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

	// Artillery: Knights vs. Orcs (Europe)
	else if (strcmp(romTid, "K9ZP") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x02073568 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02073614 = 0xE1A00000; // nop
			*(u32*)0x0207361C = 0xE1A00000; // nop
			*(u32*)0x02073628 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
			setBL(0x020896DC, (u32)dsiSaveCreate);
			setBL(0x020896EC, (u32)dsiSaveOpen);
			setBL(0x020896FC, (u32)dsiSaveGetResultCode);
			setBL(0x02089718, (u32)dsiSaveSetLength);
			setBL(0x02089728, (u32)dsiSaveWrite);
			setBL(0x02089730, (u32)dsiSaveClose);
			setBL(0x0208976C, (u32)dsiSaveOpen);
			setBL(0x0208977C, (u32)dsiSaveGetResultCode);
			setBL(0x02089794, (u32)dsiSaveGetLength);
			setBL(0x020897A4, (u32)dsiSaveRead);
			setBL(0x020897AC, (u32)dsiSaveClose);
			setBL(0x020897E4, (u32)dsiSaveOpen);
			setBL(0x020897F4, (u32)dsiSaveGetResultCode);
			setBL(0x0208980C, (u32)dsiSaveClose);
		}
	}

	// Astro (USA)
	else if (strcmp(romTid, "K7DE") == 0 && saveOnFlashcard) {
		setBL(0x02048AA8, (u32)dsiSaveOpenR);
		setBL(0x02048AC8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02048CC0, (u32)dsiSaveOpen);
		setBL(0x02048CD4, (u32)dsiSaveGetResultCode);
		setBL(0x02048600, (u32)dsiSaveOpen);
		setBL(0x0204861C, (u32)dsiSaveWrite);
		setBL(0x02048628, (u32)dsiSaveClose);
		setBL(0x0204867C, (u32)dsiSaveOpen);
		setBL(0x02048690, (u32)dsiSaveGetLength);
		setBL(0x020486A4, (u32)dsiSaveRead);
		setBL(0x020486B0, (u32)dsiSaveClose);
	}

	// ATV Fever (USA)
	else if (strcmp(romTid, "KVUE") == 0 && saveOnFlashcard) {
		setBL(0x0205BB28, (u32)dsiSaveOpen);
		setBL(0x0205BB38, (u32)dsiSaveGetLength);
		setBL(0x0205BB48, (u32)dsiSaveRead);
		setBL(0x0205BB50, (u32)dsiSaveClose);
		setBL(0x0205BBCC, (u32)dsiSaveClose);
		setBL(0x0205BC70, (u32)dsiSaveOpen);
		setBL(0x0205BC88, (u32)dsiSaveWrite);
		setBL(0x0205BC9C, (u32)dsiSaveClose);
		setBL(0x02088A00, (u32)dsiSaveOpenR);
		*(u32*)0x02088A24 = (u32)dsiSaveCreate; // dsiSaveCreateAuto
		setBL(0x02088A50, (u32)dsiSaveOpen);
		setBL(0x02088A64, (u32)dsiSaveGetResultCode);
	}

	// ATV Quad Kings (USA)
	else if (strcmp(romTid, "K9UE") == 0 && saveOnFlashcard) {
		setBL(0x0205D1D8, (u32)dsiSaveOpenR);
		*(u32*)0x0205D1FC = (u32)dsiSaveCreate; // dsiSaveCreateAuto
		setBL(0x0205D228, (u32)dsiSaveOpen);
		setBL(0x0205D23C, (u32)dsiSaveGetResultCode);
		setBL(0x0208A3AC, (u32)dsiSaveOpen);
		setBL(0x0208A3BC, (u32)dsiSaveGetLength);
		setBL(0x0208A3CC, (u32)dsiSaveRead);
		setBL(0x0208A3D4, (u32)dsiSaveClose);
		setBL(0x0208A450, (u32)dsiSaveClose);
		setBL(0x0208A4F4, (u32)dsiSaveOpen);
		setBL(0x0208A50C, (u32)dsiSaveWrite);
		setBL(0x0208A520, (u32)dsiSaveClose);
	}

	// Aura-Aura Climber (USA)
	// Save code too advanced to patch, preventing support
	else if (strcmp(romTid, "KSRE") == 0 && saveOnFlashcard) {
		*(u32*)0x02026760 = 0xE12FFF1E; // bx lr
		/* *(u32*)0x02026788 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x020267D4, (u32)dsiSaveOpen);
		setBL(0x020267E8, (u32)dsiSaveCreate);
		setBL(0x02026814, (u32)dsiSaveOpen);
		// *(u32*)0x02026834 = 0xE3A01B0B; // mov r1, #0x2C00
		setBL(0x0202683C, (u32)dsiSaveSetLength);
		setBL(0x0202684C, (u32)dsiSaveClose);
		setBL(0x02026870, (u32)dsiSaveWrite);
		setBL(0x0202687C, (u32)dsiSaveGetLength);
		*(u32*)0x02026880 = 0xE1A02000; // mov r2, r0
		*(u32*)0x02026884 = 0xE3A01000; // mov r1, #0
		// *(u32*)0x02026888 = 0xE3A03000; // mov r3, #0
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

	// Sukai Janpa Soru (Japan)
	else if (strcmp(romTid, "KSRJ") == 0 && saveOnFlashcard) {
		*(u32*)0x02026848 = 0xE12FFF1E; // bx lr
	}

	// Beauty Academy (Europe)
	else if (strcmp(romTid, "K8BP") == 0) {
		if (saveOnFlashcard) {
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
		}

		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x02092FDC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02093070 = 0xE1A00000; // nop
			*(u32*)0x02093078 = 0xE1A00000; // nop
			*(u32*)0x02093084 = 0xE1A00000; // nop
		}
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

	// Big Bass Arcade (USA)
	else if (strcmp(romTid, "K9GE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005120 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x0200E3C0, dsiSaveGetResultCode, 0xC);
			setBL(0x0203AF74, (u32)dsiSaveCreate);
			setBL(0x0203AF90, (u32)dsiSaveOpen);
			setBL(0x0203AFA4, (u32)dsiSaveSetLength);
			setBL(0x0203AFD0, (u32)dsiSaveClose);
			setBL(0x0203B080, (u32)dsiSaveDelete);
			setBL(0x0203B12C, (u32)dsiSaveOpen);
			setBL(0x0203B144, (u32)dsiSaveSeek);
			setBL(0x0203B158, (u32)dsiSaveRead);
			setBL(0x0203B168, (u32)dsiSaveClose);
			setBL(0x0203B250, (u32)dsiSaveOpen);
			setBL(0x0203B268, (u32)dsiSaveSeek);
			setBL(0x0203B27C, (u32)dsiSaveWrite);
			setBL(0x0203B28C, (u32)dsiSaveClose);
		}
	}

	// Blockado: Puzzle Island (USA)
	// Locks up on black screens (Cause unknown)
	else if (strcmp(romTid, "KZ4E") == 0) {
		if (!twlFontFound) {
			*(u16*)0x0202DFB8 = 0x4770; // bx lr (Skip NFTR font rendering)
			*(u16*)0x0202E1F0 = 0x4770; // bx lr (Skip NFTR font rendering)
			*(u16*)0x0202E504 = 0x4770; // bx lr (Skip NFTR font rendering)
			*(u16*)0x0202F928 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		}

		if (saveOnFlashcard) {
			const u32 dsiSaveCreateT = 0x02019C98;
			*(u16*)dsiSaveCreateT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

			const u32 dsiSaveGetInfoT = 0x02019CA8;
			*(u16*)dsiSaveGetInfoT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveGetInfoT + 4), dsiSaveGetInfo, 0xC);

			const u32 dsiSaveOpenT = 0x02019CB8;
			*(u16*)dsiSaveOpenT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

			const u32 dsiSaveCloseT = 0x02019CC8;
			*(u16*)dsiSaveCloseT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

			const u32 dsiSaveGetLengthT = 0x02019CD8;
			*(u16*)dsiSaveGetLengthT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveGetLengthT + 4), dsiSaveGetLength, 0xC);

			const u32 dsiSaveSeekT = 0x02019CE8;
			*(u16*)dsiSaveSeekT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

			const u32 dsiSaveReadT = 0x02019CF8;
			*(u16*)dsiSaveReadT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

			const u32 dsiSaveWriteT = 0x02019D08;
			*(u16*)dsiSaveWriteT = 0x4778; // bx pc
			tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

			*(u16*)0x0204B0C0 = 0x2001; // movs r0, #1 (dsiSaveGetArcSrc)
			*(u16*)0x0204B0C2 = 0x46C0; // nop
			setBLThumb(0x0204B0E6, dsiSaveGetInfoT);
			setBLThumb(0x0204B136, dsiSaveCreateT);
			setBLThumb(0x0204B170, dsiSaveOpenT);
			setBLThumb(0x0204B17E, dsiSaveWriteT);
			setBLThumb(0x0204B18C, dsiSaveCloseT);
			setBLThumb(0x0204B1A6, dsiSaveGetInfoT);
			setBLThumb(0x0204B23A, dsiSaveOpenT);
			setBLThumb(0x0204B248, dsiSaveReadT);
			setBLThumb(0x0204B24E, dsiSaveCloseT);
			setBLThumb(0x0204B358, dsiSaveOpenT);
			setBLThumb(0x0204B368, dsiSaveGetLengthT);
			setBLThumb(0x0204B378, dsiSaveSeekT);
			setBLThumb(0x0204B386, dsiSaveWriteT);
			setBLThumb(0x0204B38C, dsiSaveCloseT);
			setBLThumb(0x0204B3AE, dsiSaveOpenT);
			setBLThumb(0x0204B3C4, dsiSaveSeekT);
			setBLThumb(0x0204B3D2, dsiSaveReadT);
			setBLThumb(0x0204B3EE, dsiSaveCloseT);
		}
	}

	// Bloons TD (USA)
	// Bloons TD (Europe)
	// A weird bug is preventing save support
	else if ((strcmp(romTid, "KLNE") == 0 || strcmp(romTid, "KLNP") == 0) && saveOnFlashcard) {
		*(u32*)0x02005158 = 0xE1A00000; // nop (Work around save-related crash)
		/* tonccpy((u32*)0x02014E88, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205EDAC = 0xE3A00000; // mov r0, #0
		setBL(0x0205F3E8, (u32)dsiSaveDelete);
		setBL(0x0205F460, (u32)dsiSaveOpen);
		setBL(0x0205F478, (u32)dsiSaveGetLength);
		setBL(0x0205F4A8, (u32)dsiSaveRead);
		setBL(0x0205F4B8, (u32)dsiSaveClose);
		setBL(0x0205F52C, (u32)dsiSaveClose);
		setBL(0x0205F5CC, (u32)dsiSaveOpen);
		setBL(0x0205F5EC, (u32)dsiSaveCreate);
		setBL(0x0205F604, (u32)dsiSaveClose);
		setBL(0x0205F710, (u32)dsiSaveOpen);
		setBL(0x0205F734, (u32)dsiSaveSetLength);
		setBL(0x0205F744, (u32)dsiSaveClose);
		setBL(0x0205F774, (u32)dsiSaveWrite);
		setBL(0x0205F788, (u32)dsiSaveClose);
		setBL(0x0205F7A8, (u32)dsiSaveClose); */
	}

	// Bloons TD 4 (USA)
	else if (strcmp(romTid, "KUVE") == 0 && saveOnFlashcard) {
		*(u32*)0x020AF184 = 0xE3A00001; // mov r0, #1
		setBL(0x020AF5C0, (u32)dsiSaveOpen);
		setBL(0x020AF5E4, (u32)dsiSaveClose);
		setBL(0x020AF69C, (u32)dsiSaveOpen);
		setBL(0x020AF6C0, (u32)dsiSaveGetLength);
		setBL(0x020AF6F4, (u32)dsiSaveRead);
		setBL(0x020AF704, (u32)dsiSaveRead);
		setBL(0x020AF728, (u32)dsiSaveClose);
		setBL(0x020AF7C8, (u32)dsiSaveClose);
		setBL(0x020AF7E0, (u32)dsiSaveClose);
		setBL(0x020AFAAC, (u32)dsiSaveOpen);
		setBL(0x020AFAD0, (u32)dsiSaveCreate);
		setBL(0x020AFB08, (u32)dsiSaveOpen);
		setBL(0x020AFB34, (u32)dsiSaveSetLength);
		setBL(0x020AFB44, (u32)dsiSaveClose);
		setBL(0x020AFB68, (u32)dsiSaveWrite);
		setBL(0x020AFB78, (u32)dsiSaveWrite);
		setBL(0x020AFB8C, (u32)dsiSaveClose);
		setBL(0x020AFBA4, (u32)dsiSaveClose);
		setBL(0x020AFC20, (u32)dsiSaveOpen);
		setBL(0x020AFC44, (u32)dsiSaveGetLength);
		setBL(0x020AFC50, (u32)dsiSaveClose);
		setBL(0x020AFD40, (u32)dsiSaveDelete);
	}

	// Bloons TD 4 (Europe)
	else if (strcmp(romTid, "KUVP") == 0 && saveOnFlashcard) {
		*(u32*)0x020AF600 = 0xE3A00001; // mov r0, #1
		setBL(0x020AFA3C, (u32)dsiSaveOpen);
		setBL(0x020AFA60, (u32)dsiSaveClose);
		setBL(0x020AFB18, (u32)dsiSaveOpen);
		setBL(0x020AFB3C, (u32)dsiSaveGetLength);
		setBL(0x020AFB70, (u32)dsiSaveRead);
		setBL(0x020AFB80, (u32)dsiSaveRead);
		setBL(0x020AFBA4, (u32)dsiSaveClose);
		setBL(0x020AFC44, (u32)dsiSaveClose);
		setBL(0x020AFC5C, (u32)dsiSaveClose);
		setBL(0x020AFF28, (u32)dsiSaveOpen);
		setBL(0x020AFF4C, (u32)dsiSaveCreate);
		setBL(0x020AFF84, (u32)dsiSaveOpen);
		setBL(0x020AFFB0, (u32)dsiSaveSetLength);
		setBL(0x020AFFC0, (u32)dsiSaveClose);
		setBL(0x020AFFE4, (u32)dsiSaveWrite);
		setBL(0x020AFFF4, (u32)dsiSaveWrite);
		setBL(0x020B0008, (u32)dsiSaveClose);
		setBL(0x020B0020, (u32)dsiSaveClose);
		setBL(0x020B009C, (u32)dsiSaveOpen);
		setBL(0x020B00C0, (u32)dsiSaveGetLength);
		setBL(0x020B00CC, (u32)dsiSaveClose);
		setBL(0x020B01BC, (u32)dsiSaveDelete);
	}

	// Bomberman Blitz (USA)
	else if (strcmp(romTid, "KBBE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02009670, dsiSaveGetResultCode, 0xC);
		// *(u32*)0x020437AC = 0xE3A00001; // mov r0, #1
		// *(u32*)0x020437B0 = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x02043878 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x0204387C = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x020434D8 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x020434DC = 0xE12FFF1E; // bx lr
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
	// Saving is not supported due to using more than one file
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
		// *(u32*)0x020355D8 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x020355DC = 0xE12FFF1E; // bx lr
		setBL(0x02035608, (u32)dsiSaveOpen);
		setBL(0x02035658, (u32)dsiSaveRead);
		setBL(0x0203569C, (u32)dsiSaveClose);
		// *(u32*)0x020356C4 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x020356C8 = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x02035220 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02035224 = 0xE12FFF1E; // bx lr
		setBL(0x02035250, (u32)dsiSaveOpen);
		setBL(0x020352A0, (u32)dsiSaveRead);
		setBL(0x020352E4, (u32)dsiSaveClose);
		// *(u32*)0x0203530C = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02035310 = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x0203456C = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02034570 = 0xE12FFF1E; // bx lr
		setBL(0x0203459C, (u32)dsiSaveOpen);
		setBL(0x020345EC, (u32)dsiSaveRead);
		setBL(0x02034630, (u32)dsiSaveClose);
		// *(u32*)0x02034658 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203465C = 0xE12FFF1E; // bx lr
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
			*(u32*)0x0205A598 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205A804 = 0xE3A00000; // mov r0, #0
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
			*(u32*)0x0205BB7C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205BE3C = 0xE3A00000; // mov r0, #0
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
		// *(u32*)0x02008918 = 0xE12FFF1E; // bx lr (NO$GBA fix)
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

	// Candle Route (USA)
	else if (strcmp(romTid, "K9YE") == 0 && !twlFontFound) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020AE76C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Candle Route (Europe)
	else if (strcmp(romTid, "K9YP") == 0 && !twlFontFound) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020AE810;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// GO Series: Captain Sub (USA)
	// GO Series: Captain Sub (Europe)
	else if (strcmp(romTid, "K3NE") == 0 || strcmp(romTid, "K3NP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005538 = 0xE1A00000; // nop
			*(u32*)0x0200A22C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200B550 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0200F1B8 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
			*(u32*)0x0204D2D8 = 0xE12FFF1E; // bx lr

			// Skip Manual screen, Part 2
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x0200F2F4;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			setBL(0x0200AA38, (u32)dsiSaveOpen);
			setBL(0x0200AA70, (u32)dsiSaveRead);
			setBL(0x0200AA90, (u32)dsiSaveClose);
			setBL(0x0200AB28, (u32)dsiSaveCreate);
			setBL(0x0200AB68, (u32)dsiSaveOpen);
			setBL(0x0200ABA0, (u32)dsiSaveSetLength);
			setBL(0x0200ABB8, (u32)dsiSaveWrite);
			setBL(0x0200ABDC, (u32)dsiSaveClose);
			setBL(0x0200AC6C, (u32)dsiSaveGetInfo);
			tonccpy((u32*)0x02051FD0, dsiSaveGetResultCode, 0xC);
		}
	}

	// Castle Conqueror (USA)
	else if (strcmp(romTid, "KCNE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x020252B8, dsiSaveGetResultCode, 0xC);
		// *(u32*)0x0204AFD8 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0204B084 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0204B44C = 0xE12FFF1E; // bx lr
		setBL(0x0204AFF4, (u32)dsiSaveCreate);
		setBL(0x0204B014, (u32)dsiSaveOpen);
		setBL(0x0204B040, (u32)dsiSaveCreate);
		setBL(0x0204B050, (u32)dsiSaveOpen);
		setBL(0x0204B074, (u32)dsiSaveWrite);
		setBL(0x0204B0AC, (u32)dsiSaveOpen);
		setBL(0x0204B0D8, (u32)dsiSaveRead);
		setBL(0x0204B0E0, (u32)dsiSaveClose);
		setBL(0x0204B474, (u32)dsiSaveGetInfo);
		setBL(0x0204B488, (u32)dsiSaveOpen);
		setBL(0x0204B49C, (u32)dsiSaveCreate);
		setBL(0x0204B4B8, (u32)dsiSaveOpen);
		setBL(0x0204B4E4, (u32)dsiSaveCreate);
		setBL(0x0204B4F4, (u32)dsiSaveOpen);
		setBL(0x0204B504, (u32)dsiSaveWrite);
		setBL(0x0204B868, (u32)dsiSaveSeek);
		setBL(0x0204B878, (u32)dsiSaveWrite);
		setBL(0x0204B880, (u32)dsiSaveClose);
		setBL(0x0204B8C0, (u32)dsiSaveOpen);
		setBL(0x0204B8DC, (u32)dsiSaveRead);
		setBL(0x0204BF08, (u32)dsiSaveClose);
	}

	// Castle Conqueror (Europe)
	else if (strcmp(romTid, "KCNP") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02016EC4, dsiSaveGetResultCode, 0xC);
		// *(u32*)0x0203A5A4 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203A7D4 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203AB6C = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203B134 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203BA14 = 0xE12FFF1E; // bx lr
		setBL(0x0203A75C, (u32)dsiSaveOpen);
		setBL(0x0203A78C, (u32)dsiSaveWrite);
		setBL(0x0203A7AC, (u32)dsiSaveSeek);
		setBL(0x0203A7BC, (u32)dsiSaveWrite);
		setBL(0x0203A7C4, (u32)dsiSaveClose);
		setBL(0x0203A7F4, (u32)dsiSaveCreate);
		setBL(0x0203A9A0, (u32)dsiSaveOpen);
		setBL(0x0203A9CC, (u32)dsiSaveCreate);
		setBL(0x0203A9DC, (u32)dsiSaveOpen);
		setBL(0x0203AA0C, (u32)dsiSaveWrite);
		setBL(0x0203AA2C, (u32)dsiSaveSeek);
		setBL(0x0203AA3C, (u32)dsiSaveWrite);
		setBL(0x0203AA44, (u32)dsiSaveClose);
		setBL(0x0203ABB0, (u32)dsiSaveOpen);
		setBL(0x0203AF14, (u32)dsiSaveSeek);
		setBL(0x0203AF30, (u32)dsiSaveRead);
		setBL(0x0203AF40, (u32)dsiSaveSeek);
		setBL(0x0203AF54, (u32)dsiSaveRead);
		setBL(0x0203AF5C, (u32)dsiSaveClose);
		setBL(0x0203B180, (u32)dsiSaveOpen);
		setBL(0x0203B4C8, (u32)dsiSaveSeek);
		setBL(0x0203B4E0, (u32)dsiSaveRead);
		setBL(0x0203B4F0, (u32)dsiSaveSeek);
		setBL(0x0203B504, (u32)dsiSaveRead);
		setBL(0x0203B50C, (u32)dsiSaveClose);
		setBL(0x0203BA3C, (u32)dsiSaveGetInfo);
		setBL(0x0203BA50, (u32)dsiSaveOpen);
		setBL(0x0203BA64, (u32)dsiSaveCreate);
		setBL(0x0203BC0C, (u32)dsiSaveOpen);
		setBL(0x0203BC38, (u32)dsiSaveCreate);
		setBL(0x0203BC48, (u32)dsiSaveOpen);
		setBL(0x0203BC58, (u32)dsiSaveWrite);
		setBL(0x0203C174, (u32)dsiSaveSeek);
		setBL(0x0203C184, (u32)dsiSaveWrite);
		setBL(0x0203C18C, (u32)dsiSaveClose);
		setBL(0x0203C4F8, (u32)dsiSaveSeek);
		setBL(0x0203C508, (u32)dsiSaveWrite);
		setBL(0x0203C510, (u32)dsiSaveClose);
		setBL(0x0203C554, (u32)dsiSaveOpen);
		setBL(0x0203C704, (u32)dsiSaveRead);
		setBL(0x0203C724, (u32)dsiSaveSeek);
		setBL(0x0203C734, (u32)dsiSaveRead);
		setBL(0x0203CD78, (u32)dsiSaveClose);
	}

	// Castle Conqueror: Heroes (USA)
	else if (strcmp(romTid, "KC5E") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201831C, dsiSaveGetResultCode, 0xC);
		setBL(0x02065C8C, (u32)dsiSaveGetInfo);
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
		setBL(0x020660FC, (u32)dsiSaveGetInfo);
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
		setBL(0x02026F94, (u32)dsiSaveGetInfo);
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

	// Castle Conqueror: Heroes 2 (USA)
	else if (strcmp(romTid, "KXCE") == 0) {
		tonccpy((u32*)0x02013CF4, dsiSaveGetResultCode, 0xC);
		setBL(0x02035478, (u32)dsiSaveGetInfo);
		setBL(0x0203548C, (u32)dsiSaveOpen);
		setBL(0x020354A4, (u32)dsiSaveCreate);
		setBL(0x020354B4, (u32)dsiSaveOpen);
		setBL(0x020354E0, (u32)dsiSaveCreate);
		setBL(0x020354F0, (u32)dsiSaveOpen);
		setBL(0x02035A24, (u32)dsiSaveSeek);
		setBL(0x02035A38, (u32)dsiSaveWrite);
		setBL(0x02035A48, (u32)dsiSaveSeek);
		setBL(0x02035A58, (u32)dsiSaveWrite);
		setBL(0x02035A68, (u32)dsiSaveSeek);
		setBL(0x02035A78, (u32)dsiSaveWrite);
		setBL(0x02035A90, (u32)dsiSaveClose);
		setBL(0x02035ADC, (u32)dsiSaveSeek);
		setBL(0x02035AEC, (u32)dsiSaveWrite);
		setBL(0x02035AF4, (u32)dsiSaveClose);
		setBL(0x02035B44, (u32)dsiSaveOpen);
		setBL(0x02035EB0, (u32)dsiSaveSeek);
		setBL(0x02035EC0, (u32)dsiSaveRead);
		setBL(0x02035EE8, (u32)dsiSaveClose);
		setBL(0x020364B0, (u32)dsiSaveOpen);
		setBL(0x020364C4, (u32)dsiSaveSeek);
		setBL(0x020364D4, (u32)dsiSaveWrite);
		setBL(0x020364DC, (u32)dsiSaveClose);
	}

	// Castle Conqueror: Heroes 2 (Europe, Australia)
	else if (strcmp(romTid, "KXCV") == 0) {
		tonccpy((u32*)0x02013CF4, dsiSaveGetResultCode, 0xC);
		setBL(0x0206A44C, (u32)dsiSaveGetInfo);
		setBL(0x0206A460, (u32)dsiSaveOpen);
		setBL(0x0206A478, (u32)dsiSaveCreate);
		setBL(0x0206A488, (u32)dsiSaveOpen);
		setBL(0x0206A4B4, (u32)dsiSaveCreate);
		setBL(0x0206A4C4, (u32)dsiSaveOpen);
		setBL(0x0206A9F8, (u32)dsiSaveSeek);
		setBL(0x0206AA0C, (u32)dsiSaveWrite);
		setBL(0x0206AA1C, (u32)dsiSaveSeek);
		setBL(0x0206AA2C, (u32)dsiSaveWrite);
		setBL(0x0206AA3C, (u32)dsiSaveSeek);
		setBL(0x0206AA4C, (u32)dsiSaveWrite);
		setBL(0x0206AA64, (u32)dsiSaveClose);
		setBL(0x0206AAB0, (u32)dsiSaveSeek);
		setBL(0x0206AAC0, (u32)dsiSaveWrite);
		setBL(0x0206AAC8, (u32)dsiSaveClose);
		setBL(0x0206AB18, (u32)dsiSaveOpen);
		setBL(0x0206AE84, (u32)dsiSaveSeek);
		setBL(0x0206AE94, (u32)dsiSaveRead);
		setBL(0x0206AEBC, (u32)dsiSaveClose);
		setBL(0x0206B484, (u32)dsiSaveOpen);
		setBL(0x0206B498, (u32)dsiSaveSeek);
		setBL(0x0206B4A8, (u32)dsiSaveWrite);
		setBL(0x0206B4B0, (u32)dsiSaveClose);
	}

	// Castle Conqueror: Heroes 2 (Japan)
	else if (strcmp(romTid, "KXCJ") == 0) {
		tonccpy((u32*)0x02013CF4, dsiSaveGetResultCode, 0xC);
		setBL(0x02026EAC, (u32)dsiSaveGetInfo);
		setBL(0x02026EC0, (u32)dsiSaveOpen);
		setBL(0x02026ED8, (u32)dsiSaveCreate);
		setBL(0x02026EE8, (u32)dsiSaveOpen);
		setBL(0x02026F14, (u32)dsiSaveCreate);
		setBL(0x02026F24, (u32)dsiSaveOpen);
		setBL(0x02027458, (u32)dsiSaveSeek);
		setBL(0x0202746C, (u32)dsiSaveWrite);
		setBL(0x0202747C, (u32)dsiSaveSeek);
		setBL(0x0202748C, (u32)dsiSaveWrite);
		setBL(0x0202749C, (u32)dsiSaveSeek);
		setBL(0x020274AC, (u32)dsiSaveWrite);
		setBL(0x020274C4, (u32)dsiSaveClose);
		setBL(0x02027510, (u32)dsiSaveSeek);
		setBL(0x02027520, (u32)dsiSaveWrite);
		setBL(0x02027528, (u32)dsiSaveClose);
		setBL(0x02027578, (u32)dsiSaveOpen);
		setBL(0x020278E4, (u32)dsiSaveSeek);
		setBL(0x020278F4, (u32)dsiSaveRead);
		setBL(0x0202791C, (u32)dsiSaveClose);
		setBL(0x02027EE4, (u32)dsiSaveOpen);
		setBL(0x02027EF8, (u32)dsiSaveSeek);
		setBL(0x02027F08, (u32)dsiSaveWrite);
		setBL(0x02027F10, (u32)dsiSaveClose);
	}

	// Cave Story (USA)
	else if (strcmp(romTid, "KCVE") == 0) {
		if (saveOnFlashcard) {
			// *(u32*)0x02005980 = 0xE12FFF1E; // bx lr
			// *(u32*)0x02005A68 = 0xE12FFF1E; // bx lr
			// *(u32*)0x02005B60 = 0xE12FFF1E; // bx lr
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
			tonccpy((u32*)0x02073FA4, dsiSaveGetResultCode, 0xC);
		}
		if (!twlFontFound) {
			*(u32*)0x0200A12C = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Chuck E. Cheese's Alien Defense Force (USA)
	else if (strcmp(romTid, "KUQE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0201BBA4, (u32)dsiSaveCreate);
			setBL(0x0201BBB4, (u32)dsiSaveOpen);
			setBL(0x0201BBD0, (u32)dsiSaveSeek);
			setBL(0x0201BBE0, (u32)dsiSaveWrite);
			setBL(0x0201BBE8, (u32)dsiSaveClose);
			setBL(0x0201BD10, (u32)dsiSaveOpenR);
			setBL(0x0201BD28, (u32)dsiSaveSeek);
			setBL(0x0201BD38, (u32)dsiSaveRead);
			setBL(0x0201BD40, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x0201B9E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 4; i++) {
				u32* offset = (u32*)0x0202D43C;
				offset[i] = 0xE1A00000; // nop
			}
		}
	}

	// Chuck E. Cheese's Arcade Room (USA)
	else if (strcmp(romTid, "KUCE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02032550 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x020459F0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Chuukara! Dairoujou (Japan)
	else if (strcmp(romTid, "KQLJ") == 0 && saveOnFlashcard) {
		setBL(0x020446E4, (u32)dsiSaveOpen);
		setBL(0x02044710, (u32)dsiSaveRead);
		setBL(0x02044720, (u32)dsiSaveClose);
		setBL(0x0204473C, (u32)dsiSaveClose);
		*(u32*)0x02044790 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x020447CC = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x020447D8, (u32)dsiSaveCreate);
		setBL(0x020447E8, (u32)dsiSaveOpen);
		setBL(0x02044814, (u32)dsiSaveSetLength);
		setBL(0x02044824, (u32)dsiSaveClose);
		setBL(0x02044848, (u32)dsiSaveWrite);
		setBL(0x02044858, (u32)dsiSaveClose);
		setBL(0x02044874, (u32)dsiSaveClose);
	}

	// Crash-Course Domo (USA)
	else if (strcmp(romTid, "KDCE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound || gameOnFlashcard) {
			*(u16*)0x020153C4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		}
	}

	// CuteWitch! runner (USA)
	// CuteWitch! runner (Europe)
	else if (strncmp(romTid, "K32", 3) == 0 && saveOnFlashcard) {
		u32* saveFuncOffsets[22] = {NULL};

		tonccpy((u32*)0x0201C450, dsiSaveGetResultCode, 0xC);
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x02062328 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02062600 = 0xE3A00000; // mov r0, #0
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
			*(u32*)0x02093D64 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0209403C = 0xE3A00000; // mov r0, #0
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
		*(u32*)0x02044BE8 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02044C24 = 0xE1A00000; // nop (dsiSaveCloseDir)
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
		*(u32*)0x0204472C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02044768 = 0xE1A00000; // nop (dsiSaveCloseDir)
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
	else if (strcmp(romTid, "KWTE") == 0 || strcmp(romTid, "KWTP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200B350 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x0200CC98;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			// *(u32*)0x0200C584 = 0xE1A00000; // nop
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
			*(u32*)0x0200C910 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc & dsiSaveFreeSpaceAvailable)
			*(u32*)0x0200C914 = 0xE12FFF1E; // bx lr
			tonccpy((u32*)0x0204AAEC, dsiSaveGetResultCode, 0xC);
		}
	}

	// DotMan (USA)
	else if (strcmp(romTid, "KHEE") == 0 && !twlFontFound) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02022600;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (Europe)
	else if (strcmp(romTid, "KHEP") == 0 && !twlFontFound) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x020226DC;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// DotMan (Japan)
	else if (strcmp(romTid, "KHEJ") == 0 && !twlFontFound) {
		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0202248C;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Dr. Mario Express (USA)
	// A Little Bit of... Dr. Mario (Europe, Australia)
	else if (strcmp(romTid, "KD9E") == 0 || strcmp(romTid, "KD9V") == 0) {
		if (saveOnFlashcard) {
			tonccpy((u32*)0x02011160, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0203D228 = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
			// *(u32*)0x0203D488 = 0xE3A00000; // mov r0, #0
			// *(u32*)0x0203D48C = 0xE12FFF1E; // bx lr
		}
		if (!twlFontFound) {
			*(u32*)0x020248C4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02025CD4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (ndsHeader->gameCode[3] == 'E') {
			if (saveOnFlashcard) {
				// *(u32*)0x02044B00 = 0xE3A00000; // mov r0, #0
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
			}
			if (!twlFontFound) {
				*(u32*)0x0207347C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x020736DC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x02074054 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
			}
		} else {
			if (saveOnFlashcard) {
				// *(u32*)0x02044A9C = 0xE3A00000; // mov r0, #0
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
			}
			if (!twlFontFound) {
				*(u32*)0x0207336C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x020735CC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x02073F44 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
			}
		}
	}

	// Chotto Dr. Mario (Japan)
	else if (strcmp(romTid, "KD9J") == 0) {
		if (saveOnFlashcard) {
			tonccpy((u32*)0x020118A4, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0205824C = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
			// *(u32*)0x020584B4 = 0xE3A00000; // mov r0, #0
			// *(u32*)0x020584B8 = 0xE12FFF1E; // bx lr
			// *(u32*)0x0205F6F0 = 0xE3A00000; // mov r0, #0
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
		if (!twlFontFound) {
			*(u32*)0x02024CF4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02026104 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202D3B4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202D644 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202DFF0 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		}
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
	else if (strcmp(romTid, "KDQE") == 0 && !twlFontFound) {
		*(u32*)0x0201F208 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Dragon Quest Wars (Europe, Australia)
	// DSi save function patching not needed
	else if (strcmp(romTid, "KDQV") == 0 && !twlFontFound) {
		*(u32*)0x0201F250 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Dragon Quest Wars (Japan)
	// DSi save function patching not needed
	else if (strcmp(romTid, "KDQJ") == 0 && !twlFontFound) {
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
	else if (strcmp(romTid, "KB8E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200A3D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200B800 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02014AB0 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
			*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr

			// Skip Manual screen, Part 2
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x02014BEC;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			// *(u32*)0x0200A898 = 0xE12FFF1E; // bx lr
			setBL(0x0200AC14, (u32)dsiSaveOpen);
			setBL(0x0200AC50, (u32)dsiSaveRead);
			setBL(0x0200AC70, (u32)dsiSaveClose);
			setBL(0x0200AD0C, (u32)dsiSaveCreate);
			setBL(0x0200AD4C, (u32)dsiSaveOpen);
			setBL(0x0200AD84, (u32)dsiSaveSetLength);
			setBL(0x0200ADA0, (u32)dsiSaveWrite);
			setBL(0x0200ADC4, (u32)dsiSaveClose);
			setBL(0x0200AE58, (u32)dsiSaveGetInfo);
			tonccpy((u32*)0x0204CB6C, dsiSaveGetResultCode, 0xC);
		}
	}

	// GO Series: Earth Saver (Europe)
	else if (strcmp(romTid, "KB8P") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200A310 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200B710 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020149B4 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
			*(u32*)0x02047D50 = 0xE12FFF1E; // bx lr

			// Skip Manual screen, Part 2
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x02014AF0;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			setBL(0x0200AB24, (u32)dsiSaveOpen);
			setBL(0x0200AB60, (u32)dsiSaveRead);
			setBL(0x0200AB80, (u32)dsiSaveClose);
			setBL(0x0200AC1C, (u32)dsiSaveCreate);
			setBL(0x0200AC5C, (u32)dsiSaveOpen);
			setBL(0x0200AC94, (u32)dsiSaveSetLength);
			setBL(0x0200ACB0, (u32)dsiSaveWrite);
			setBL(0x0200ACD4, (u32)dsiSaveClose);
			setBL(0x0200AD68, (u32)dsiSaveGetInfo);
			tonccpy((u32*)0x0204CA70, dsiSaveGetResultCode, 0xC);
		}
	}

	// Earth Saver: Inseki Bakuha Dai Sakuse (Japan)
	else if (strcmp(romTid, "KB9J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200B9C4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200A7CC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0200FF44 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
			*(u32*)0x0203092C = 0xE12FFF1E; // bx lr

			// Skip Manual screen, Part 2
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x02010080;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			setBL(0x02009CF4, (u32)dsiSaveOpen);
			setBL(0x02009D2C, (u32)dsiSaveRead);
			setBL(0x02009D4C, (u32)dsiSaveClose);
			setBL(0x02009DE4, (u32)dsiSaveCreate);
			setBL(0x02009E24, (u32)dsiSaveOpen);
			setBL(0x02009E5C, (u32)dsiSaveSetLength);
			setBL(0x02009E74, (u32)dsiSaveWrite);
			setBL(0x02009E98, (u32)dsiSaveClose);
			setBL(0x02009F28, (u32)dsiSaveGetInfo);
			tonccpy((u32*)0x0203564C, dsiSaveGetResultCode, 0xC);
		}
	}

	// Earth Saver Plus: Inseki Bakuha Dai Sakuse (Japan)
	else if (strcmp(romTid, "KB8J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200A038 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200B438 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02014708 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
			*(u32*)0x02047A88 = 0xE12FFF1E; // bx lr

			// Skip Manual screen, Part 2
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x02014844;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			setBL(0x0200A84C, (u32)dsiSaveOpen);
			setBL(0x0200A888, (u32)dsiSaveRead);
			setBL(0x0200A8A8, (u32)dsiSaveClose);
			setBL(0x0200A944, (u32)dsiSaveCreate);
			setBL(0x0200A984, (u32)dsiSaveOpen);
			setBL(0x0200A9BC, (u32)dsiSaveSetLength);
			setBL(0x0200A9D8, (u32)dsiSaveWrite);
			setBL(0x0200A9FC, (u32)dsiSaveClose);
			setBL(0x0200AA90, (u32)dsiSaveGetInfo);
			tonccpy((u32*)0x0204C79C, dsiSaveGetResultCode, 0xC);
		}
	}

	// Fall in the Dark (Japan)
	// A bit hard/confusing to add save support
	else if (strcmp(romTid, "K4EJ") == 0 && !twlFontFound) {
		*(u32*)0x0203EE0C = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Farm Frenzy (USA)
	else if (strcmp(romTid, "KFKE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0200F4CC, dsiSaveGetResultCode, 0xC);
		setBL(0x02035B30, (u32)dsiSaveCreate);
		setBL(0x02035B4C, (u32)dsiSaveOpen);
		setBL(0x02035B9C, (u32)dsiSaveWrite);
		setBL(0x02035BA4, (u32)dsiSaveClose);
		*(u32*)0x02035C78 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02035C88 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02035C9C, (u32)dsiSaveOpen);
		setBL(0x02035CAC, (u32)dsiSaveClose);
		setBL(0x02036828, (u32)dsiSaveOpen);
		setBL(0x02036848, (u32)dsiSaveClose);
		setBL(0x02041190, (u32)dsiSaveOpen);
		setBL(0x020411B0, (u32)dsiSaveSeek);
		setBL(0x020411D0, (u32)dsiSaveWrite);
		setBL(0x020411D8, (u32)dsiSaveClose);
		setBL(0x020411F4, (u32)dsiSaveOpen);
		setBL(0x02041214, (u32)dsiSaveSeek);
		setBL(0x02041234, (u32)dsiSaveWrite);
		setBL(0x0204123C, (u32)dsiSaveClose);
		setBL(0x02041268, (u32)dsiSaveOpen);
		setBL(0x02041288, (u32)dsiSaveSeek);
		setBL(0x020412A8, (u32)dsiSaveRead);
		setBL(0x020412B0, (u32)dsiSaveClose);
		setBL(0x020412C4, (u32)dsiSaveOpen);
		setBL(0x020412E4, (u32)dsiSaveSeek);
		setBL(0x02041304, (u32)dsiSaveRead);
		setBL(0x0204130C, (u32)dsiSaveClose);
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
		// *(u16*)0x0203728C = 0x2000; // movs r0, #0 (dsiSaveOpenDir)
		// *(u16*)0x0203728E = 0x4770; // bx lr
		// *(u16*)0x020372D8 = 0x2000; // movs r0, #0 (dsiSaveCloseDir)
		// *(u16*)0x020372DA = 0x4770; // bx lr
	}

	// Fizz (USA)
	else if (strcmp(romTid, "KZZE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02011260, dsiSaveGetResultCode, 0xC);
		setBL(0x02029FE0, (u32)dsiSaveOpen);
		setBL(0x0202A030, (u32)dsiSaveGetLength);
		setBL(0x0202A044, (u32)dsiSaveRead);
		setBL(0x0202A05C, (u32)dsiSaveClose);
		setBL(0x0202A3A4, (u32)dsiSaveOpen);
		setBL(0x0202A3E0, (u32)dsiSaveCreate);
		setBL(0x0202A3F4, (u32)dsiSaveOpen);
		setBL(0x0202A414, (u32)dsiSaveSetLength);
		setBL(0x0202A434, (u32)dsiSaveWrite);
		setBL(0x0202A44C, (u32)dsiSaveClose);
	}

	// Flashlight (USA)
	else if (strcmp(romTid, "KFSE") == 0 && !twlFontFound) {
		*(u32*)0x02005134 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Frogger Returns (USA)
	else if (strcmp(romTid, "KFGE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x0204B968 = 0xE1A00000; // nop
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x0204B98C;
				offset[i] = 0xE1A00000; // nop
			}
		}
	}

	// Fuuu! Dairoujou Kai (Japan)
	else if (strcmp(romTid, "K6JJ") == 0 && saveOnFlashcard) {
		setBL(0x02045468, (u32)dsiSaveOpen);
		setBL(0x02045498, (u32)dsiSaveRead);
		setBL(0x020454A8, (u32)dsiSaveClose);
		setBL(0x020454C4, (u32)dsiSaveClose);
		*(u32*)0x0204551C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02045558 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02045564, (u32)dsiSaveCreate);
		setBL(0x02045574, (u32)dsiSaveOpen);
		setBL(0x020455A0, (u32)dsiSaveSetLength);
		setBL(0x020455B0, (u32)dsiSaveClose);
		setBL(0x020455DC, (u32)dsiSaveWrite);
		setBL(0x020455EC, (u32)dsiSaveClose);
		setBL(0x02045608, (u32)dsiSaveClose);
	}

	// Game & Watch: Ball (USA, Europe)
	// Game & Watch: Helmet (USA, Europe)
	// Game & Watch: Judge (USA, Europe)
	// Game & Watch: Manhole (USA, Europe)
	// Game & Watch: Vermin (USA, Europe)
	// Ball: Softlocks after a miss or exiting gameplay
	// Helmet, Manhole & Vermin: Softlocks after 3 misses or exiting gameplay
	// Judge: Softlocks after limit is reached or exiting gameplay
	// Save code seems confusing to patch, preventing support
	else if ((strcmp(romTid, "KGBO") == 0 || strcmp(romTid, "KGHO") == 0 || strcmp(romTid, "KGJO") == 0 || strcmp(romTid, "KGMO") == 0 || strcmp(romTid, "KGVO") == 0) && saveOnFlashcard) {
		if (strncmp(romTid, "KGB", 3) == 0) {
			/* setBL(0x02033ABC, (u32)dsiSaveCreate);
			setBL(0x02033AF8, (u32)dsiSaveOpen);
			setBL(0x02033B38, (u32)dsiSaveClose);
			setBL(0x02033D60, (u32)dsiSaveOpen); */
			*(u32*)0x02035078 = 0xE12FFF1E; // bx lr
			/* *(u32*)0x0203E124 = 0xE1A00000; // nop
			*(u32*)0x0203E148 = 0xE1A00000; // nop
			*(u32*)0x0203E150 = 0xE1A00000; // nop
			*(u32*)0x0203E180 = 0xE1A00000; // nop
			*(u32*)0x0203E188 = 0xE1A00000; // nop
			*(u32*)0x0203E19C = 0xE1A00000; // nop
			setBL(0x0203E160, (u32)dsiSaveWrite);
			setBL(0x0203E178, (u32)dsiSaveWrite);
			setBL(0x0203E1E8, (u32)dsiSaveRead); */
		} else if (strncmp(romTid, "KGH", 3) == 0 || strncmp(romTid, "KGM", 3) == 0) {
			*(u32*)0x0202D5E4 = 0xE12FFF1E; // bx lr
		} else if (strncmp(romTid, "KGJ", 3) == 0) {
			*(u32*)0x0202D158 = 0xE12FFF1E; // bx lr
		} else {
			*(u32*)0x0202D0D4 = 0xE12FFF1E; // bx lr
		}
	}

	// Game & Watch: Ball (Japan)
	// Game & Watch: Helmet (Japan)
	// Game & Watch: Judge (Japan)
	// Game & Watch: Manhole (Japan)
	// Game & Watch: Vermin (Japan)
	// Ball: Softlocks after a miss or exiting gameplay
	// Helmet, Manhole & Vermin: Softlocks after 3 misses or exiting gameplay
	// Judge: Softlocks after limit is reached or exiting gameplay
	else if ((strcmp(romTid, "KGBJ") == 0 || strcmp(romTid, "KGHJ") == 0 || strcmp(romTid, "KGJJ") == 0 || strcmp(romTid, "KGMJ") == 0 || strcmp(romTid, "KGVJ") == 0) && saveOnFlashcard) {
		if (strncmp(romTid, "KGB", 3) == 0) {
			*(u32*)0x02034BC8 = 0xE12FFF1E; // bx lr
		} else if (strncmp(romTid, "KGH", 3) == 0 || strncmp(romTid, "KGM", 3) == 0) {
			*(u32*)0x0202D384 = 0xE12FFF1E; // bx lr
		} else if (strncmp(romTid, "KGJ", 3) == 0) {
			*(u32*)0x0202CEF8 = 0xE12FFF1E; // bx lr
		} else {
			*(u32*)0x0202CE74 = 0xE12FFF1E; // bx lr
		}
	}

	// Game & Watch: Chef (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGCO") == 0 && saveOnFlashcard) {
		*(u32*)0x0202F0FC = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Chef (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGCJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0202EE9C = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Donkey Kong Jr. (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGDO") == 0 && saveOnFlashcard) {
		*(u32*)0x0202D860 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Donkey Kong Jr. (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGDJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0202D600 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Flagman (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGGO") == 0 && saveOnFlashcard) {
		*(u32*)0x0202D520 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Flagman (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGGJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0202D2C0 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Mario's Cement Factory (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGFO") == 0 && saveOnFlashcard) {
		*(u32*)0x0202F188 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Mario's Cement Factory (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGFJ") == 0 && saveOnFlashcard) {
		*(u32*)0x0202EF28 = 0xE12FFF1E; // bx lr
	}

	// Goooooal America (USA)
	else if (strcmp(romTid, "K9AE") == 0 && saveOnFlashcard) {
		setBL(0x02048390, (u32)dsiSaveCreate);
		setBL(0x020483A0, (u32)dsiSaveOpen);
		setBL(0x020483BC, (u32)dsiSaveGetResultCode);
		setBL(0x020483E0, (u32)dsiSaveSeek);
		setBL(0x020483F8, (u32)dsiSaveGetResultCode);
		setBL(0x0204841C, (u32)dsiSaveWrite);
		setBL(0x0204843C, (u32)dsiSaveClose);
		setBL(0x02048444, (u32)dsiSaveGetResultCode);
		setBL(0x02048460, (u32)dsiSaveGetResultCode);
		setBL(0x0204849C, (u32)dsiSaveOpenR);
		setBL(0x020484AC, (u32)dsiSaveGetLength);
		setBL(0x020484E0, (u32)dsiSaveRead);
		setBL(0x020484F8, (u32)dsiSaveClose);
		setBL(0x02048504, (u32)dsiSaveGetResultCode);
		setBL(0x0204D0AC, 0x02048560);
	}

	// Goooooal Europa 2012 (Europe)
	else if (strcmp(romTid, "K9AP") == 0 && saveOnFlashcard) {
		setBL(0x0203DD34, (u32)dsiSaveCreate);
		setBL(0x0203DD44, (u32)dsiSaveOpen);
		setBL(0x0203DD60, (u32)dsiSaveGetResultCode);
		setBL(0x0203DD84, (u32)dsiSaveSeek);
		setBL(0x0203DD9C, (u32)dsiSaveGetResultCode);
		setBL(0x0203DDC0, (u32)dsiSaveWrite);
		setBL(0x0203DDE0, (u32)dsiSaveClose);
		setBL(0x0203DDE8, (u32)dsiSaveGetResultCode);
		setBL(0x0203DE04, (u32)dsiSaveGetResultCode);
		setBL(0x0203DE40, (u32)dsiSaveOpenR);
		setBL(0x0203DE50, (u32)dsiSaveGetLength);
		setBL(0x0203DE84, (u32)dsiSaveRead);
		setBL(0x0203DE9C, (u32)dsiSaveClose);
		setBL(0x0203DEA8, (u32)dsiSaveGetResultCode);
		setBL(0x02042A50, 0x0203DF04);
	}

	// Go! Go! Kokopolo (USA)
	// Go! Go! Kokopolo (Europe)
	else if (strcmp(romTid, "K3GE") == 0 || strcmp(romTid, "K3GP") == 0) {
		const u32 readCodeCopy = 0x02013CF4;

		if (ndsHeader->romversion == 0) {
			if (ndsHeader->gameCode[3] == 'E') {
				if (saveOnFlashcard) {
					tonccpy((u32*)readCodeCopy, (u32*)0x020BCB48, 0x70);

					setBL(0x02043290, readCodeCopy);
					setBL(0x02043328, readCodeCopy);

					setBL(readCodeCopy+0x18, 0x02013CC8);
					setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
					setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
					setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
					setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
					setBL(readCodeCopy+0x54, 0x020BCA0C);

					setBL(0x020BCCF4, (u32)dsiSaveCreate);
					setBL(0x020BCD04, (u32)dsiSaveOpen);
					setBL(0x020BCD18, (u32)dsiSaveSetLength);
					setBL(0x020BCD28, (u32)dsiSaveWrite);
					setBL(0x020BCD30, (u32)dsiSaveClose);
				}
				if (!twlFontFound) {
					*(u32*)0x0208EB74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
				}
			} else {
				if (saveOnFlashcard) {
					tonccpy((u32*)readCodeCopy, (u32*)0x020BCC6C, 0x70);

					setBL(0x020432EC, readCodeCopy);
					setBL(0x02043384, readCodeCopy);

					setBL(readCodeCopy+0x18, 0x02013CC8);
					setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
					setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
					setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
					setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
					setBL(readCodeCopy+0x54, 0x020BCB20);

					setBL(0x020BCE18, (u32)dsiSaveCreate);
					setBL(0x020BCE28, (u32)dsiSaveOpen);
					setBL(0x020BCE3C, (u32)dsiSaveSetLength);
					setBL(0x020BCE4C, (u32)dsiSaveWrite);
					setBL(0x020BCE54, (u32)dsiSaveClose);
				}
				if (!twlFontFound) {
					*(u32*)0x0208EC38 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
				}
			}
		} else {
			if (saveOnFlashcard) {
				tonccpy((u32*)readCodeCopy, (u32*)0x020BCDA4, 0x70);

				setBL(0x020432EC, readCodeCopy);
				setBL(0x02043384, readCodeCopy);

				setBL(readCodeCopy+0x18, 0x02013CC8);
				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);
				setBL(readCodeCopy+0x54, 0x020BCC58);

				setBL(0x020BCF50, (u32)dsiSaveCreate);
				setBL(0x020BCF60, (u32)dsiSaveOpen);
				setBL(0x020BCF74, (u32)dsiSaveSetLength);
				setBL(0x020BCF84, (u32)dsiSaveWrite);
				setBL(0x020BCF8C, (u32)dsiSaveClose);
			}
			if (!twlFontFound) {
				*(u32*)0x0208EE9C = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
			}
		}
	}

	// Go! Go! Kokopolo (Japan)
	else if (strcmp(romTid, "K3GJ") == 0) {
		if (saveOnFlashcard) {
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

			setBL(0x020BCF3C, (u32)dsiSaveCreate);
			setBL(0x020BCF4C, (u32)dsiSaveOpen);
			setBL(0x020BCF60, (u32)dsiSaveSetLength);
			setBL(0x020BCF70, (u32)dsiSaveWrite);
			setBL(0x020BCF78, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x0208EC74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		}
	}

	// Hard-Hat Domo (USA)
	else if (strcmp(romTid, "KDHE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound || gameOnFlashcard) {
			*(u16*)0x020140AC = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		}
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
	else if (strcmp(romTid, "DD3P") == 0 && consoleModel == 0 && (u8)a9ScfgRom != 1) {
		//*(u32*)0x0201BC14 -= 0xD000; // Shift heap
		*(u32*)0x0201BC14 -= 0x6000; // Shift heap
	}

	// Wimmelbild Creator (German)
	else if (strcmp(romTid, "DD3D") == 0 && consoleModel == 0 && (u8)a9ScfgRom != 1) {
		//*(u32*)0x0201B9E4 -= 0xD000; // Shift heap
		*(u32*)0x0201B9E4 -= 0x6000; // Shift heap
	}

	// JellyCar 2 (USA)
	else if (strcmp(romTid, "KJYE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x020067C4, (u32)dsiSaveOpen);
			*(u32*)0x020067DC = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			*(u32*)0x020067F4 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
			setBL(0x02006810, (u32)dsiSaveClose);
			setBL(0x020070A0, (u32)dsiSaveOpen);
			setBL(0x0200710C, (u32)dsiSaveClose);
			setBL(0x0200761C, (u32)dsiSaveOpen);
			setBL(0x020076A8, (u32)dsiSaveClose);
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
		if (!twlFontFound) {
			*(u32*)0x02019F94 = 0xE1A00000; // nop (Skip Manual screen, Part 1)
			for (int i = 0; i < 11; i++) { // Skip Manual screen, Part 2
				u32* offset = (u32*)0x0201A028;
				offset[i] = 0xE1A00000; // nop
			}
		}
	}

	// Kami Hikouki (Japan)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "KAMJ") == 0 && !twlFontFound) {
		*(u32*)0x02021E48 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x02021FEC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
	}

	// A Kappa's Trail (USA)
	else if (strcmp(romTid, "KPAE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201A020, dsiSaveGetResultCode, 0xC);
		setBL(0x02032000, (u32)dsiSaveOpenR);
		setBL(0x02032038, (u32)dsiSaveRead);
		setBL(0x0203205C, (u32)dsiSaveClose);
		setBL(0x02032070, (u32)dsiSaveClose);
		setBL(0x020320C8, (u32)dsiSaveCreate);
		setBL(0x020320D8, (u32)dsiSaveOpen);
		setBL(0x0203210C, (u32)dsiSaveSetLength);
		setBL(0x0203211C, (u32)dsiSaveWrite);
		setBL(0x02032144, (u32)dsiSaveClose);
		setBL(0x02032158, (u32)dsiSaveClose);
	}

	// Kappa Michi (Japan)
	else if (strcmp(romTid, "KPAJ") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201A554, dsiSaveGetResultCode, 0xC);
		setBL(0x02032CAC, (u32)dsiSaveOpenR);
		setBL(0x02032CE4, (u32)dsiSaveRead);
		setBL(0x02032D08, (u32)dsiSaveClose);
		setBL(0x02032D1C, (u32)dsiSaveClose);
		setBL(0x02032D74, (u32)dsiSaveCreate);
		setBL(0x02032D84, (u32)dsiSaveOpen);
		setBL(0x02032DB8, (u32)dsiSaveSetLength);
		setBL(0x02032DC8, (u32)dsiSaveWrite);
		setBL(0x02032DF0, (u32)dsiSaveClose);
		setBL(0x02032E04, (u32)dsiSaveClose);
	}

	// Kung Fu Dragon (USA)
	// Kung Fu Dragon (Europe)
	else if (strcmp(romTid, "KT9E") == 0 || strcmp(romTid, "KT9P") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005310 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x0201D8EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Akushon Gemu: Tobeyo!! Dorago! (Japan)
	else if (strcmp(romTid, "KT9J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020052F0 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x0201D8C0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Kyou Hanan no hi Hyakka: Hyakkajiten Maipedea Yori (Japan)
	else if (strcmp(romTid, "K47J") == 0 && !twlFontFound) {
		/*setBL(0x02020CD4, (u32)dsiSaveOpen);
		setBL(0x02020CF8, (u32)dsiSaveGetLength);
		setBL(0x02020D1C, (u32)dsiSaveRead);
		setBL(0x02020D24, (u32)dsiSaveClose);
		setBL(0x02020D7C, (u32)dsiSaveCreate);
		setBL(0x02020D8C, (u32)dsiSaveOpen);
		setBL(0x02020D9C, (u32)dsiSaveGetResultCode);
		setBL(0x02020DBC, (u32)dsiSaveSetLength);
		setBL(0x02020DCC, (u32)dsiSaveWrite);
		setBL(0x02020DD4, (u32)dsiSaveClose);
		*(u32*)0x02020E18 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02020E34 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x02020E60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020E74 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020E9C = 0xE1A00000; // nop
		*(u32*)0x02020EA0 = 0xE1A00000; // nop
		*(u32*)0x02020EB4 = 0xE1A00000; // nop
		*(u32*)0x02020EE4 = 0xE1A00000; // nop (dsiSaveCloseDir)*/
		*(u32*)0x0202F3F0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// The Legend of Zelda: Four Swords: Anniversary Edition (USA)
	// The Legend of Zelda: Four Swords: Anniversary Edition (Europe, Australia)
	// Zelda no Densetsu: 4-tsu no Tsurugi: 25th Kinen Edition (Japan)
	else if (strncmp(romTid, "KQ9", 3) == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0200F860, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02056644 = 0xE3A00003; // mov r0, #3
		*(u32*)0x02056744 = 0xE3A00003; // mov r0, #3
		setBL(0x02056B9C, (u32)dsiSaveCreate);
		setBL(0x02056BAC, (u32)dsiSaveOpen);
		setBL(0x02056BD4, (u32)dsiSaveSetLength);
		setBL(0x02056BE4, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02056C5C, (u32)dsiSaveClose);
		setBL(0x02056CA8, (u32)dsiSaveOpen);
		setBL(0x02056CC8, (u32)dsiSaveGetLength);
		setBL(0x02056CE0, (u32)dsiSaveRead);
		setBL(0x02056CE8, (u32)dsiSaveClose);
		*(u32*)0x02056E38 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02056E58 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
	}

	// Libera Wing (Europe)
	else if (strcmp(romTid, "KLWP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02044668 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0204585C = 0xE3A00001; // mov r0, #1
			*(u32*)0x02045860 = 0xE12FFF1E; // bx lr
		}
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

	// Little Twin Stars (Japan)
	else if (strcmp(romTid, "KQ3J") == 0 && !twlFontFound) {
		*(u32*)0x020162C4 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Littlest Pet Shop (USA)
	// Littlest Pet Shop (Europe, Australia)
	else if (strcmp(romTid, "KLPE") == 0 || strcmp(romTid, "KLPV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200509C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020055BC;
				offset[i] = 0xE1A00000; // nop
			}
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x020159F0, dsiSaveGetResultCode, 0xC);
			setBL(0x0205DE18, (u32)dsiSaveOpen);
			setBL(0x0205DE70, (u32)dsiSaveRead);
			setBL(0x0205DF24, (u32)dsiSaveCreate);
			setBL(0x0205DF34, (u32)dsiSaveOpen);
			setBL(0x0205DF7C, (u32)dsiSaveSetLength);
			setBL(0x0205DF8C, (u32)dsiSaveWrite);
			setBL(0x0205DF94, (u32)dsiSaveClose);
		}
	}

	// Lola's Alphabet Train (USA)
	// Lola's Alphabet Train (Europe)
	else if ((strcmp(romTid, "KLKE") == 0 || strcmp(romTid, "KLKP") == 0) && !twlFontFound) {
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Lola's Fruit Shop Sudoku (USA)
	// Lola's Fruit Shop Sudoku (Europe)
	else if ((strcmp(romTid, "KOFE") == 0 || strcmp(romTid, "KOFP") == 0) && !twlFontFound) {
		*(u32*)0x02005108 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		if (ndsHeader->gameCode[3] == 'E') {
			*(u32*)0x0201CF70 = 0xE1A00000; // nop (Skip Manual screen)
		} else {
			*(u32*)0x0201CFCC = 0xE1A00000; // nop (Skip Manual screen)
		}
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
	else if (strcmp(romTid, "KMAJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005D08 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201942C = 0xE1A00000; // nop
			*(u32*)0x0201C89C = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
			*(u32*)0x0202DA1C = 0xE12FFF1E; // bx lr
			*(u32*)0x0202E65C = 0xE12FFF1E; // bx lr
			*(u32*)0x0203DE70 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		/*if (saveOnFlashcard) {
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
			tonccpy((u32*)0x020350D0, dsiSaveGetResultCode, 0xC);
			tonccpy((u32*)0x02035C7C, dsiSaveGetInfo, 0xC);
		}*/
	}

	// Magical Whip (USA)
	else if (strcmp(romTid, "KWME") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0201D4F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02030288 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Magical Whip (Europe)
	else if (strcmp(romTid, "KWMP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0201D5D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02030368 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Magnetic Joe (USA)
	else if (strcmp(romTid, "KJOE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02036A30 = 0xE1A00000; // nop (Skip
			*(u32*)0x02036A34 = 0xE1A00000; // nop  Manual screen)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0204E5AC = 0xE3A00002; // mov r0, #2
			*(u32*)0x0205DAAC = 0xE3A00001; // mov r0, #1
			*(u32*)0x0205DAB0 = 0xE12FFF1E; // bx lr
		}
	}

	// Make Up & Style (USA)
	else if (strcmp(romTid, "KYLE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005348 = 0xE1A00000; // nop (Disable NFTR font loading)
			*(u32*)0x0200534C = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Make Up & Style (Europe)
	else if (strcmp(romTid, "KYLP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005360 = 0xE1A00000; // nop (Disable NFTR font loading)
			*(u32*)0x02005364 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Mario vs. Donkey Kong: Minis March Again! (USA)
	// Save code too advanced to patch, preventing support
	else if (strcmp(romTid, "KDME") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005190 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0202BE84 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0202BE88 = 0xE12FFF1E; // bx lr
			/* *(u32*)0x0202BDB0 = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
			*(u32*)0x0202BEAC = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			setBL(0x0202BEF8, (u32)dsiSaveOpen);
			setBL(0x0202BF0C, (u32)dsiSaveCreate);
			setBL(0x0202BF38, (u32)dsiSaveOpen);
			setBL(0x0202BF60, (u32)dsiSaveSetLength);
			setBL(0x0202BF70, (u32)dsiSaveClose);
			setBL(0x0202BF94, (u32)dsiSaveWrite);
			setBL(0x0202BFA0, (u32)dsiSaveGetLength);
			setBL(0x0202BFDC, (u32)dsiSaveSeek);
			setBL(0x0202BFF8, (u32)dsiSaveRead);
			setBL(0x0202C2F0, (u32)dsiSaveSeek);
			setBL(0x0202C314, (u32)dsiSaveRead);
			setBL(0x0202C3DC, (u32)dsiSaveSeek);
			setBL(0x0202C3F8, (u32)dsiSaveRead);
			setBL(0x0202C66C, (u32)dsiSaveSeek);
			setBL(0x0202C684, (u32)dsiSaveWrite);
			setBL(0x0202C8F8, (u32)dsiSaveSeek);
			setBL(0x0202C910, (u32)dsiSaveWrite);
			setBL(0x0202C950, (u32)dsiSaveSeek);
			setBL(0x0202C96C, (u32)dsiSaveWrite);
			setBL(0x0202CA38, (u32)dsiSaveSeek);
			setBL(0x0202CA50, (u32)dsiSaveRead);
			setBL(0x0202CAE4, (u32)dsiSaveSeek);
			setBL(0x0202CAFC, (u32)dsiSaveRead);
			setBL(0x0202CCBC, (u32)dsiSaveSeek);
			setBL(0x0202CCD4, (u32)dsiSaveWrite);
			setBL(0x0202CD74, (u32)dsiSaveClose); */
		}
	}

	// Mario vs. Donkey Kong: Minis March Again! (Europe, Australia)
	// Save code too advanced to patch, preventing support
	else if (strcmp(romTid, "KDMV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200519C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0202C6D0 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0202C6D4 = 0xE12FFF1E; // bx lr
		}
	}

	// Mario vs. Donkey Kong: Mini Mini Sai Koushin! (Japan)
	// Save code too advanced to patch, preventing support
	else if (strcmp(romTid, "KDMJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200519C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0202C6B0 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0202C6B4 = 0xE12FFF1E; // bx lr
		}
	}

	// Maestro! Green Groove (USA)
	// Does not save due to unknown cause
	else if (strcmp(romTid, "KMUE") == 0 && saveOnFlashcard) {
		*(u32*)0x02029F34 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BC568 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BC904 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BC908 = 0xE12FFF1E; // bx lr
		/* setBL(0x020BC7BC, (u32)dsiSaveOpenR);
		setBL(0x020BC7D0, (u32)dsiSaveCreate);
		setBL(0x020BC7DC, (u32)dsiSaveClose);
		setBL(0x020BC7F0, (u32)dsiSaveOpen);
		setBL(0x020BC85C, (u32)dsiSaveWrite);
		setBL(0x020BC864, (u32)dsiSaveClose);
		setBL(0x020BC8AC, (u32)dsiSaveCreate);
		setBL(0x020BC8BC, (u32)dsiSaveOpen);
		setBL(0x020BC8DC, (u32)dsiSaveSetLength);
		setBL(0x020BC8EC, (u32)dsiSaveWrite);
		setBL(0x020BC8F4, (u32)dsiSaveClose);
		setBL(0x020BC930, (u32)dsiSaveOpenR);
		setBL(0x020BC97C, (u32)dsiSaveGetLength);
		setBL(0x020BC9B0, (u32)dsiSaveRead);
		setBL(0x020BC9BC, (u32)dsiSaveClose); */
	}

	// Maestro! Green Groove (Europe, Australia)
	// Does not save due to unknown cause
	else if (strcmp(romTid, "KM6V") == 0 && saveOnFlashcard) {
		*(u32*)0x02029F34 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BCA40 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BCDDC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BCDE0 = 0xE12FFF1E; // bx lr
		/* setBL(0x020BCC94, (u32)dsiSaveOpenR);
		setBL(0x020BCCA8, (u32)dsiSaveCreate);
		setBL(0x020BCCB4, (u32)dsiSaveClose);
		setBL(0x020BCCC8, (u32)dsiSaveOpen);
		setBL(0x020BCD34, (u32)dsiSaveWrite);
		setBL(0x020BCD3C, (u32)dsiSaveClose);
		setBL(0x020BCD84, (u32)dsiSaveCreate);
		setBL(0x020BCD94, (u32)dsiSaveOpen);
		setBL(0x020BCDB4, (u32)dsiSaveSetLength);
		setBL(0x020BCDC4, (u32)dsiSaveWrite);
		setBL(0x020BCDCC, (u32)dsiSaveClose);
		setBL(0x020BCE08, (u32)dsiSaveOpenR);
		setBL(0x020BCE54, (u32)dsiSaveGetLength);
		setBL(0x020BCE88, (u32)dsiSaveRead);
		setBL(0x020BCE94, (u32)dsiSaveClose); */
	}

	// Meikyou Kokugo: Rakubiki Jiten (Japan)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "KD4J") == 0 && !twlFontFound) {
		*(u32*)0x0205ECAC = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Metal Torrent (USA)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02045F88 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
			*(u32*)0x020DDB00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020DDD60 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020DDF00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020DE0A4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0206E8FC = 0xE3A07000; // mov r7, #0
		}
	}

	// Metal Torrent (Europe, Australia)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59V") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02045FA0 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
			*(u32*)0x020DD918 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020DDB78 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020DDD18 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x020DDEBC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0206E894 = 0xE3A07000; // mov r7, #0
		}
	}

	// A Mujou Setsuna (Japan)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02042684 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02042930 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02042AD0 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02042C74 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02089E08 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0210190C = 0xE3A07000; // mov r7, #0
		}
	}

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200B048, (u32)dsiSaveCreate);
			setBL(0x0200B070, (u32)dsiSaveGetResultCode);
			setBL(0x0200B090, (u32)dsiSaveCreate);
			setBL(0x0200B0E8, (u32)dsiSaveOpen);
			setBL(0x0200B114, (u32)dsiSaveOpen);
			setBL(0x0200B124, (u32)dsiSaveRead);
			setBL(0x0200B12C, (u32)dsiSaveClose);
			setBL(0x0200B388, (u32)dsiSaveCreate);
			setBL(0x0200B39C, (u32)dsiSaveOpen);
			setBL(0x0200B5A4, (u32)dsiSaveSetLength);
			setBL(0x0200B5B4, (u32)dsiSaveWrite);
			setBL(0x0200B5BC, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
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
	}

	// Mighty Flip Champs! (Europe, Australia)
	else if (strcmp(romTid, "KMGV") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200B350, (u32)dsiSaveCreate);
			setBL(0x0200B378, (u32)dsiSaveGetResultCode);
			setBL(0x0200B398, (u32)dsiSaveCreate);
			setBL(0x0200B3F0, (u32)dsiSaveOpen);
			setBL(0x0200B41C, (u32)dsiSaveOpen);
			setBL(0x0200B42C, (u32)dsiSaveRead);
			setBL(0x0200B434, (u32)dsiSaveClose);
			setBL(0x0200B690, (u32)dsiSaveCreate);
			setBL(0x0200B6A4, (u32)dsiSaveOpen);
			setBL(0x0200B8AC, (u32)dsiSaveSetLength);
			setBL(0x0200B8BC, (u32)dsiSaveWrite);
			setBL(0x0200B8C4, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
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
	}

	// Mighty Flip Champs! (Japan)
	else if (strcmp(romTid, "KMGJ") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200B134, (u32)dsiSaveCreate);
			setBL(0x0200B158, (u32)dsiSaveGetResultCode);
			setBL(0x0200B174, (u32)dsiSaveCreate);
			setBL(0x0200B1D4, (u32)dsiSaveOpen);
			setBL(0x0200B1FC, (u32)dsiSaveOpen);
			setBL(0x0200B210, (u32)dsiSaveRead);
			setBL(0x0200B218, (u32)dsiSaveClose);
			setBL(0x0200B478, (u32)dsiSaveCreate);
			setBL(0x0200B488, (u32)dsiSaveOpen);
			setBL(0x0200B694, (u32)dsiSaveSetLength);
			setBL(0x0200B6A4, (u32)dsiSaveWrite);
			setBL(0x0200B6AC, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
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
	}

	// Mighty Milky Way (USA)
	// Mighty Milky Way (Europe)
	// Mighty Milky Way (Japan)
	else if (strncmp(romTid, "KWY", 3) == 0) {
		if (saveOnFlashcard) {
			setBL(0x0200547C, (u32)dsiSaveCreate);
			setBL(0x020054A0, (u32)dsiSaveGetResultCode);
			setBL(0x020054BC, (u32)dsiSaveCreate);
			setBL(0x02005534, (u32)dsiSaveOpen);
			setBL(0x0200555C, (u32)dsiSaveOpen);
			setBL(0x02005570, (u32)dsiSaveRead);
			setBL(0x02005578, (u32)dsiSaveClose);
			setBL(0x020057E4, (u32)dsiSaveCreate);
			setBL(0x020057F4, (u32)dsiSaveOpen);
			setBL(0x020059FC, (u32)dsiSaveSetLength);
			setBL(0x02005A0C, (u32)dsiSaveWrite);
			setBL(0x02005A14, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
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
	}

	// Mixed Messages (USA)
	// Mixed Messages (Europe, Australia)
	else if (strcmp(romTid, "KMME") == 0 || strcmp(romTid, "KMMV") == 0) {
		if (saveOnFlashcard) {
			*(u32*)0x02031A40 = 0xE3A00008; // mov r0, #8
			*(u32*)0x02031A44 = 0xE12FFF1E; // bx lr
		}
		if (!twlFontFound) {
			*(u32*)0x02033B00 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Model Academy (Europe)
	else if (strcmp(romTid, "K8MP") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x020B2800 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020B2894 = 0xE1A00000; // nop
			*(u32*)0x020B289C = 0xE1A00000; // nop
			*(u32*)0x020B28A8 = 0xE1A00000; // nop
		}
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
		// *(u32*)0x0207F17C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
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
		// *(u32*)0x0207F084 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
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
	else if (strcmp(romTid, "K9SJ") == 0) {
		if (saveOnFlashcard) {
			tonccpy((u32*)0x0200F3A8, dsiSaveGetResultCode, 0xC);
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
		if (!twlFontFound) {
			*(u32*)0x0203F378 = 0xE1A00000; // nop (Skip Manual screen)
		}
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
	else if (strcmp(romTid, "KDRE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
			*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (saveOnFlashcard) {
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
	}

	// Mr. Driller: Drill Till You Drop (Europe, Australia)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
			*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (saveOnFlashcard) {
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
	}

	// Sakutto Hamareru Hori Hori Action: Mr. Driller (Japan)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0201FE10 = 0xE1A00000; // nop (Disable NFTR font loading)
			*(u32*)0x0202000C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202027C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (saveOnFlashcard) {
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
	}

	// Music on: Acoustic Guitar (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KG6E") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02008864 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008A10 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008A24 = 0xE12FFF1E; // bx lr
	}

	// Music on: Acoustic Guitar (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KG6V") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02008864 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008A34 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008A48 = 0xE12FFF1E; // bx lr
	}

	// Music on: Drums (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KQDE") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x0200A158 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200A304 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200A318 = 0xE12FFF1E; // bx lr
	}

	// Music on: Drums (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KQDV") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x0200A158 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200A328 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200A33C = 0xE12FFF1E; // bx lr
	}

	// Anata no Raku Raku: Doramu Mashin (Japan)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KQDJ") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x0200DE78 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E024 = 0xE12FFF1E; // bx lr
		*(u32*)0x0200E038 = 0xE12FFF1E; // bx lr
	}

	// Music on: Electric Guitar (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KIEE") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02009B20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02009CCC = 0xE12FFF1E; // bx lr
		*(u32*)0x02009CE0 = 0xE12FFF1E; // bx lr
	}

	// Music on: Electric Guitar (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KIEV") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02009B20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02009CF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02009D04 = 0xE12FFF1E; // bx lr
	}

	// Anata no Rakuraku: Erekutorikku Gita (Japan)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KIEJ") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02009CC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02009E70 = 0xE12FFF1E; // bx lr
		*(u32*)0x02009E84 = 0xE12FFF1E; // bx lr
	}

	// Music on: Electronic Keyboard (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KK7E") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02008850 = 0xE12FFF1E; // bx lr
		*(u32*)0x020089D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020089E4 = 0xE12FFF1E; // bx lr
	}

	// Music on: Electronic Keyboard (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KK7V") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02008850 = 0xE12FFF1E; // bx lr
		*(u32*)0x020089D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020089E4 = 0xE12FFF1E; // bx lr
	}

	// Anata no Rakuraku: Erekutoronikku Kibodo (Japan)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KK7J") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02008BF8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008D78 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008D8C = 0xE12FFF1E; // bx lr
	}

	// Music on: Playing Piano (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KICE") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x0200CC58 = 0xE1A00000; // nop
		*(u32*)0x0200CC5C = 0xE1A00000; // nop
		*(u32*)0x0200CC60 = 0xE1A00000; // nop
	}

	// Music on: Playing Piano (Europe)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KICP") == 0 && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x0200CC7C = 0xE1A00000; // nop
		*(u32*)0x0200CC80 = 0xE1A00000; // nop
		*(u32*)0x0200CC84 = 0xE1A00000; // nop
	}

	// Music on: Retro Keyboard (USA)
	// Music on: Retro Keyboard (Europe, Australia)
	// Saving is difficult to implement
	else if ((strcmp(romTid, "KRHE") == 0 || strcmp(romTid, "KRHV") == 0) && !twlFontFound) {
		// Skip Manual screen
		*(u32*)0x02008CD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008E50 = 0xE12FFF1E; // bx lr
		*(u32*)0x02008E64 = 0xE12FFF1E; // bx lr
	}

	// My Aquarium: Seven Oceans (USA)
	else if (strcmp(romTid, "K7ZE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0203DC00, (u32)dsiSaveOpen);
			setBL(0x0203DC1C, (u32)dsiSaveCreate);
			setBL(0x0203DC48, (u32)dsiSaveOpen);
			setBL(0x0203DC60, (u32)dsiSaveRead);
			setBL(0x0203DC7C, (u32)dsiSaveClose);
			setBL(0x0203DC94, (u32)dsiSaveClose);
			setBL(0x0203DD54, (u32)dsiSaveOpen);
			setBL(0x0203DD6C, (u32)dsiSaveClose);
			setBL(0x0203DD8C, (u32)dsiSaveRead);
			setBL(0x0203DDA8, (u32)dsiSaveClose);
			setBL(0x0203DDC4, (u32)dsiSaveSeek);
			setBL(0x0203DDD4, (u32)dsiSaveSeek);
			setBL(0x0203DDE8, (u32)dsiSaveWrite);
			setBL(0x0203DE04, (u32)dsiSaveClose);
			setBL(0x0203DE24, (u32)dsiSaveSeek);
			setBL(0x0203DE34, (u32)dsiSaveWrite);
			setBL(0x0203DE50, (u32)dsiSaveClose);
			setBL(0x0203DE70, (u32)dsiSaveSeek);
			setBL(0x0203DE84, (u32)dsiSaveWrite);
			setBL(0x0203DEA0, (u32)dsiSaveClose);
			setBL(0x0203DEB8, (u32)dsiSaveClose);
			setBL(0x0203DF64, (u32)dsiSaveOpen);
			setBL(0x0203DF7C, (u32)dsiSaveClose);
			setBL(0x0203DFB8, (u32)dsiSaveSeek);
			setBL(0x0203DFCC, (u32)dsiSaveRead);
			setBL(0x0203DFF4, (u32)dsiSaveClose);
			setBL(0x0203E034, (u32)dsiSaveClose);
			setBL(0x0203E088, (u32)dsiSaveClose);
			setBL(0x0203E0C0, (u32)dsiSaveClose);
			setBL(0x0203E200, (u32)dsiSaveOpen);
			setBL(0x0203E224, (u32)dsiSaveClose);
			setBL(0x02045158, 0x02011384); // Branch to fixed code
			*(u32*)0x0206DCFC = 0xE1A00000; // nop
			*(u32*)0x0206DD0C = 0xE1A00000; // nop

			// Fixed code with added branch to save write code
			*(u32*)0x02011384 = 0xE92D4003; // STMFD SP!, {R0-R1,LR}
			*(u32*)0x02011388 = 0xE5901008; // ldr r1, [r0,#8]
			*(u32*)0x0201138C = 0xE5910004; // ldr r0, [r1,#4]
			*(u32*)0x02011390 = 0xE3500005; // cmp r0, #5
			*(u32*)0x02011394 = 0x13700001; // cmpne r0, #1
			*(u32*)0x02011398 = 0x03E00001; // moveq r0, #0xFFFFFFFE
			*(u32*)0x0201139C = 0x05810004; // streq r0, [r1,#4]
			*(u32*)0x020113A0 = 0x18BD8003; // LDMNEFD SP!, {R0-R1,PC}
			setBL(0x020113A4, 0x0206DCD8);
			*(u32*)0x020113A8 = 0xE8BD8003; // LDMFD SP!, {R0-R1,PC}
		}
		if (!twlFontFound) {
			*(u32*)0x0206DA88 = 0xE3A01001; // mov r1, #1 (Skip Manual screen)
		}
	}

	// My Aquarium: Seven Oceans (Europe)
	else if (strcmp(romTid, "K9RP") == 0) {
		if (saveOnFlashcard) {
			setBL(0x0203DC4C, (u32)dsiSaveOpen);
			setBL(0x0203DC68, (u32)dsiSaveCreate);
			setBL(0x0203DC94, (u32)dsiSaveOpen);
			setBL(0x0203DCAC, (u32)dsiSaveRead);
			setBL(0x0203DCC8, (u32)dsiSaveClose);
			setBL(0x0203DCE0, (u32)dsiSaveClose);
			setBL(0x0203DDF8, (u32)dsiSaveOpen);
			setBL(0x0203DE14, (u32)dsiSaveClose);
			setBL(0x0203DE34, (u32)dsiSaveRead);
			setBL(0x0203DE50, (u32)dsiSaveClose);
			setBL(0x0203DE70, (u32)dsiSaveSeek);
			setBL(0x0203DE80, (u32)dsiSaveWrite);
			setBL(0x0203DE8C, (u32)dsiSaveClose);
			setBL(0x0203DF44, (u32)dsiSaveOpen);
			setBL(0x0203DF5C, (u32)dsiSaveClose);
			setBL(0x0203DF98, (u32)dsiSaveSeek);
			setBL(0x0203DFAC, (u32)dsiSaveRead);
			setBL(0x0203DFD4, (u32)dsiSaveClose);
			setBL(0x0203E014, (u32)dsiSaveClose);
			setBL(0x0203E068, (u32)dsiSaveClose);
			setBL(0x0203E0A0, (u32)dsiSaveClose);
			setBL(0x0203E1B4, (u32)dsiSaveOpen);
			setBL(0x0203E1CC, (u32)dsiSaveClose);
			setBL(0x0203E1E8, (u32)dsiSaveWrite);
			setBL(0x0203E1F0, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x0206DAA0 = 0xE3A01001; // mov r1, #1 (Skip Manual screen)
		}
	}

	// My Farm (USA)
	else if (strcmp(romTid, "KMRE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x020126DC, dsiSaveGetResultCode, 0xC);
		setBL(0x0207A09C, (u32)dsiSaveCreate);
		setBL(0x0207A118, (u32)dsiSaveOpen);
		setBL(0x0207A164, (u32)dsiSaveSetLength);
		setBL(0x0207A178, (u32)dsiSaveClose);
		setBL(0x0207A1A8, (u32)dsiSaveWrite);
		setBL(0x0207A1C0, (u32)dsiSaveClose);
		setBL(0x0207A1EC, (u32)dsiSaveClose);
		setBL(0x0207A284, (u32)dsiSaveOpen);
		setBL(0x0207A2CC, (u32)dsiSaveGetLength);
		setBL(0x0207A2E0, (u32)dsiSaveClose);
		setBL(0x0207A300, (u32)dsiSaveRead);
		setBL(0x0207A318, (u32)dsiSaveClose);
		setBL(0x0207A344, (u32)dsiSaveClose);
	}

	// My Farm (Europe, Australia)
	else if (strcmp(romTid, "KMRV") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02012608, dsiSaveGetResultCode, 0xC);
		setBL(0x02079FA0, (u32)dsiSaveCreate);
		setBL(0x0207A01C, (u32)dsiSaveOpen);
		setBL(0x0207A068, (u32)dsiSaveSetLength);
		setBL(0x0207A07C, (u32)dsiSaveClose);
		setBL(0x0207A0AC, (u32)dsiSaveWrite);
		setBL(0x0207A0C4, (u32)dsiSaveClose);
		setBL(0x0207A0F0, (u32)dsiSaveClose);
		setBL(0x0207A188, (u32)dsiSaveOpen);
		setBL(0x0207A1D0, (u32)dsiSaveGetLength);
		setBL(0x0207A1E4, (u32)dsiSaveClose);
		setBL(0x0207A204, (u32)dsiSaveRead);
		setBL(0x0207A21C, (u32)dsiSaveClose);
		setBL(0x0207A248, (u32)dsiSaveClose);
	}

	// My Asian Farm (USA)
	// My Australian Farm (USA)
	else if ((strcmp(romTid, "KL3E") == 0 || strcmp(romTid, "KL4E") == 0) && saveOnFlashcard) {
		tonccpy((u32*)0x0201270C, dsiSaveGetResultCode, 0xC);
		setBL(0x02077428, (u32)dsiSaveCreate);
		setBL(0x020774A4, (u32)dsiSaveOpen);
		setBL(0x020774F0, (u32)dsiSaveSetLength);
		setBL(0x02077504, (u32)dsiSaveClose);
		setBL(0x02077534, (u32)dsiSaveWrite);
		setBL(0x0207754C, (u32)dsiSaveClose);
		setBL(0x02077578, (u32)dsiSaveClose);
		setBL(0x02077610, (u32)dsiSaveOpen);
		setBL(0x02077658, (u32)dsiSaveGetLength);
		setBL(0x0207766C, (u32)dsiSaveClose);
		setBL(0x0207768C, (u32)dsiSaveRead);
		setBL(0x020776A4, (u32)dsiSaveClose);
		setBL(0x020776D0, (u32)dsiSaveClose);
	}

	// My Asian Farm (Europe)
	// My Australian Farm (Europe)
	else if ((strcmp(romTid, "KL3P") == 0 || strcmp(romTid, "KL4P") == 0) && saveOnFlashcard) {
		tonccpy((u32*)0x0201270C, dsiSaveGetResultCode, 0xC);
		setBL(0x02077474, (u32)dsiSaveCreate);
		setBL(0x020774F0, (u32)dsiSaveOpen);
		setBL(0x0207753C, (u32)dsiSaveSetLength);
		setBL(0x02077550, (u32)dsiSaveClose);
		setBL(0x02077580, (u32)dsiSaveWrite);
		setBL(0x02077598, (u32)dsiSaveClose);
		setBL(0x020775C4, (u32)dsiSaveClose);
		setBL(0x0207765C, (u32)dsiSaveOpen);
		setBL(0x020776A4, (u32)dsiSaveGetLength);
		setBL(0x020776B8, (u32)dsiSaveClose);
		setBL(0x020776D8, (u32)dsiSaveRead);
		setBL(0x020776F0, (u32)dsiSaveClose);
		setBL(0x0207771C, (u32)dsiSaveClose);
	}

	// My Exotic Farm (USA)
	else if (strcmp(romTid, "KMVE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x020126DC, dsiSaveGetResultCode, 0xC);
		setBL(0x0207A0A4, (u32)dsiSaveCreate);
		setBL(0x0207A120, (u32)dsiSaveOpen);
		setBL(0x0207A16C, (u32)dsiSaveSetLength);
		setBL(0x0207A180, (u32)dsiSaveClose);
		setBL(0x0207A1B0, (u32)dsiSaveWrite);
		setBL(0x0207A1C8, (u32)dsiSaveClose);
		setBL(0x0207A1F4, (u32)dsiSaveClose);
		setBL(0x0207A28C, (u32)dsiSaveOpen);
		setBL(0x0207A2D4, (u32)dsiSaveGetLength);
		setBL(0x0207A2E8, (u32)dsiSaveClose);
		setBL(0x0207A308, (u32)dsiSaveRead);
		setBL(0x0207A320, (u32)dsiSaveClose);
		setBL(0x0207A34C, (u32)dsiSaveClose);
	}

	// My Exotic Farm (Europe, Australia)
	else if (strcmp(romTid, "KMVV") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x020126DC, dsiSaveGetResultCode, 0xC);
		setBL(0x0207A07C, (u32)dsiSaveCreate);
		setBL(0x0207A0F8, (u32)dsiSaveOpen);
		setBL(0x0207A144, (u32)dsiSaveSetLength);
		setBL(0x0207A158, (u32)dsiSaveClose);
		setBL(0x0207A188, (u32)dsiSaveWrite);
		setBL(0x0207A1A0, (u32)dsiSaveClose);
		setBL(0x0207A1CC, (u32)dsiSaveClose);
		setBL(0x0207A264, (u32)dsiSaveOpen);
		setBL(0x0207A2AC, (u32)dsiSaveGetLength);
		setBL(0x0207A2C0, (u32)dsiSaveClose);
		setBL(0x0207A2E0, (u32)dsiSaveRead);
		setBL(0x0207A2F8, (u32)dsiSaveClose);
		setBL(0x0207A324, (u32)dsiSaveClose);
	}

	// My Little Restaurant (USA)
	else if (strcmp(romTid, "KLTE") == 0 && saveOnFlashcard) {
		setBL(0x02018BCC, (u32)dsiSaveClose);
		setBL(0x02018C28, (u32)dsiSaveClose);
		setBL(0x02018CD0, (u32)dsiSaveOpen);
		setBL(0x02018CE8, (u32)dsiSaveSeek);
		setBL(0x02018CFC, (u32)dsiSaveRead);
		setBL(0x02018D9C, (u32)dsiSaveCreate);
		setBL(0x02018DCC, (u32)dsiSaveOpen);
		setBL(0x02018DFC, (u32)dsiSaveSetLength);
		setBL(0x02018E24, (u32)dsiSaveSeek);
		setBL(0x02018E38, (u32)dsiSaveWrite);
		setBL(0x02018EE8, (u32)dsiSaveCreate);
		setBL(0x02018F20, (u32)dsiSaveOpen);
		setBL(0x02018F58, (u32)dsiSaveSetLength);
		setBL(0x02018F74, (u32)dsiSaveSeek);
		setBL(0x02018F88, (u32)dsiSaveWrite);
		setBL(0x020190E8, (u32)dsiSaveSeek);
		setBL(0x020190F8, (u32)dsiSaveWrite);
		setBL(0x02019290, (u32)dsiSaveGetResultCode);
		*(u32*)0x020192D4 = 0xE3A00000; // mov r0, #0
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

	// Neko Reversi (Japan)
	else if (strcmp(romTid, "KNVJ") == 0 && !twlFontFound) {
		*(u32*)0x0203FCA4 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Nintendo Countdown Calendar (USA)
	else if (strcmp(romTid, "KAUE") == 0) {
		if (saveOnFlashcard) {
			setBL(0x02012480, (u32)dsiSaveGetLength);
			setBL(0x020124C0, (u32)dsiSaveRead);
			setBL(0x0201253C, (u32)dsiSaveWrite);
			setBL(0x02012B20, (u32)dsiSaveOpen);
			setBL(0x02012BA0, (u32)dsiSaveClose);
			setBL(0x02012F40, (u32)dsiSaveCreate);
			setBL(0x02012F50, (u32)dsiSaveOpen);
			setBL(0x02012F64, (u32)dsiSaveSetLength);
			setBL(0x02012FAC, (u32)dsiSaveClose);
			tonccpy((u32*)0x02086F2C, dsiSaveGetResultCode, 0xC);
		}
		if (!twlFontFound) {
			*(u32*)0x0205C1B4 = 0xE1A00000; // nop
		}
	}

	// Nintendo Countdown Calendar (Europe, Australia)
	else if (strcmp(romTid, "KAUV") == 0) {
		if (saveOnFlashcard) {
			setBL(0x020124DC, (u32)dsiSaveGetLength);
			setBL(0x0201251C, (u32)dsiSaveRead);
			setBL(0x02012598, (u32)dsiSaveWrite);
			setBL(0x02012B7C, (u32)dsiSaveOpen);
			setBL(0x02012BFC, (u32)dsiSaveClose);
			setBL(0x02012F9C, (u32)dsiSaveCreate);
			setBL(0x02012FAC, (u32)dsiSaveOpen);
			setBL(0x02012FC0, (u32)dsiSaveSetLength);
			setBL(0x02013008, (u32)dsiSaveClose);
			tonccpy((u32*)0x02087164, dsiSaveGetResultCode, 0xC);
		}
		if (!twlFontFound) {
			*(u32*)0x0205C3EC = 0xE1A00000; // nop
		}
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

	// Nintendoji (Japan)
	// Due to our save implementation, save data is stored in both slots
	else if (strcmp(romTid, "K9KJ") == 0 && saveOnFlashcard) {
		//setBL(0x0209F040, (u32)dsiSaveClose);
		setBL(0x0209F3A8, (u32)dsiSaveOpen);
		setBL(0x0209F400, (u32)dsiSaveGetLength);
		setBL(0x0209F484, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0209F4E8, (u32)dsiSaveOpen);
		setBL(0x0209F500, (u32)dsiSaveClose);
		setBL(0x0209F5A4, (u32)dsiSaveCreate);
		setBL(0x0209F5B4, (u32)dsiSaveGetResultCode);
		setBL(0x0209F6DC, (u32)dsiSaveOpen);
		setBL(0x0209F73C, (u32)dsiSaveSetLength);
		setBL(0x0209F754, (u32)dsiSaveClose);
		setBL(0x0209F770, (u32)dsiSaveWrite);
		setBL(0x0209F790, (u32)dsiSaveClose);
		setBL(0x0209F7AC, (u32)dsiSaveClose);
		setBL(0x0209F7BC, (u32)dsiSaveClose);
	}

	// Number Battle
	else if (strcmp(romTid, "KSUE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			*(u32*)0x02021C20 = 0xE1A00000; // nop (Skip Manual screen)
		}
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
		// *(u32*)0x0203A20C = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x0203A730 = 0xE3A00001; // mov r0, #1
	}

	// Peg Solitaire (USA)
	else if (strcmp(romTid, "KP8E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0203DDA8 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Peg Solitaire (Europe)
	else if (strcmp(romTid, "KP8P") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0203E03C = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
		// *(u32*)0x020059E4 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x020059E8 = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x02005A8C = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02005A90 = 0xE12FFF1E; // bx lr
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
	else if (strcmp(romTid, "KPYE") == 0 || strcmp(romTid, "KPYP") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen (Crashes)
			*(u32*)0x02040264 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0204030C = 0xE1A00000; // nop
			*(u32*)0x02040318 = 0xE1A00000; // nop
			*(u32*)0x02040330 = 0xE1A00000; // nop

			// Disable NFTR loading from TWLNAND
			*(u32*)0x02045264 = 0xE1A00000; // nop
			*(u32*)0x02045268 = 0xE1A00000; // nop 
			*(u32*)0x02045270 = 0xE1A00000; // nop
			*(u32*)0x0204527C = 0xE1A00000; // nop
			*(u32*)0x02045290 = 0xE1A00000; // nop
			*(u32*)0x0204529C = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Pinball Attack! (Japan)
	else if (strcmp(romTid, "KPYJ") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen (Crashes)
			*(u32*)0x0203FCEC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0203FD34 = 0xE1A00000; // nop
			*(u32*)0x0203FD3C = 0xE1A00000; // nop
			*(u32*)0x0203FD50 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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

	// Pirates Assault (Japan)
	else if (strcmp(romTid, "KXAJ") == 0 && saveOnFlashcard) {
		setBL(0x02020BB8, (u32)dsiSaveGetInfo);
		setBL(0x02020BCC, (u32)dsiSaveOpen);
		setBL(0x02020BE0, (u32)dsiSaveCreate);
		setBL(0x02020BF0, (u32)dsiSaveOpen);
		setBL(0x02020C00, (u32)dsiSaveGetResultCode);
		setBL(0x02020C1C, (u32)dsiSaveCreate);
		setBL(0x02020C2C, (u32)dsiSaveOpen);
		setBL(0x02020C78, (u32)dsiSaveSeek);
		setBL(0x02020C88, (u32)dsiSaveWrite);
		setBL(0x02020CF4, (u32)dsiSaveSeek);
		setBL(0x02020D04, (u32)dsiSaveWrite);
		setBL(0x02020D0C, (u32)dsiSaveClose);
		setBL(0x02020D58, (u32)dsiSaveOpen);
		setBL(0x02020DB8, (u32)dsiSaveSeek);
		setBL(0x02020DCC, (u32)dsiSaveRead);
		setBL(0x02020DF4, (u32)dsiSaveClose);
		setBL(0x02020E90, (u32)dsiSaveOpen);
		setBL(0x02020EA4, (u32)dsiSaveSeek);
		setBL(0x02020EB4, (u32)dsiSaveWrite);
		setBL(0x02020EBC, (u32)dsiSaveClose);
		setBL(0x0202114C, (u32)dsiSaveGetInfo);
		setBL(0x0202115C, (u32)dsiSaveOpen);
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
		// *(u32*)0x020C2F94 = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x020C41F8 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x020FA794, dsiSaveGetResultCode, 0xC);
	}

	// Pop+ Solo (USA)
	// Pop+ Solo (Europe, Australia)
	else if (strcmp(romTid, "KPIE") == 0 || strcmp(romTid, "KPIV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			setBL(0x0201C210, (u32)dsiSaveOpen);
			setBL(0x0201C234, (u32)dsiSaveGetLength);
			setBL(0x0201C24C, (u32)dsiSaveRead);
			setBL(0x0201C258, (u32)dsiSaveClose);
			*(u32*)0x0201C2B4 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			*(u32*)0x0201C2D8 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
			setBL(0x0201C308, (u32)dsiSaveCreate);
			setBL(0x0201C31C, (u32)dsiSaveOpen);
			setBL(0x0201C350, (u32)dsiSaveSetLength);
			setBL(0x0201C368, (u32)dsiSaveWrite);
			setBL(0x0201C384, (u32)dsiSaveClose);
		}
	}

	// GO Series: Portable Shrine Wars (USA)
	// GO Series: Portable Shrine Wars (Europe)
	else if (strcmp(romTid, "KOQE") == 0 || strcmp(romTid, "KOQP") == 0) {
		if (saveOnFlashcard) {
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
			*(u32*)0x0200D214 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc & dsiSaveFreeSpaceAvailable)
			*(u32*)0x0200D218 = 0xE12FFF1E; // bx lr
			tonccpy((u32*)0x0204ED3C, dsiSaveGetResultCode, 0xC);
		}
		if (!twlFontFound) {
			*(u32*)0x0200E004 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x0200DE78;
				offset[i] = 0xE1A00000; // nop
			}
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
		// *(u32*)0x02007768 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0200776C = 0xE12FFF1E; // bx lr
		setBL(0x0200779C, (u32)dsiSaveCreate);
		setBL(0x020077AC, (u32)dsiSaveGetResultCode);
		setBL(0x020077CC, (u32)dsiSaveOpen);
		setBL(0x020077EC, (u32)dsiSaveSetLength);
		setBL(0x02007808, (u32)dsiSaveWrite);
		setBL(0x02007834, (u32)dsiSaveClose);
	}

	// Prehistorik Man (USA)
	else if (strcmp(romTid, "KPHE") == 0) {
		if (!twlFontFound) {
			setB(0x0204A0D4, 0x0204A39C); // Skip Manual screen
		}
		if (saveOnFlashcard) {
			setBL(0x0204D4C8, (u32)dsiSaveOpen);
			setBL(0x0204D4E0, (u32)dsiSaveGetLength);
			setBL(0x0204D4F8, (u32)dsiSaveRead);
			setBL(0x0204D500, (u32)dsiSaveClose);
			setBL(0x0204D540, (u32)dsiSaveGetInfo);
			setBL(0x0204D550, (u32)dsiSaveGetResultCode);
			setBL(0x0204D59C, (u32)dsiSaveCreate);
			setBL(0x0204D5A4, (u32)dsiSaveGetResultCode);
			setBL(0x0204D5F8, (u32)dsiSaveOpen);
			setBL(0x0204D608, (u32)dsiSaveGetResultCode);
			setBL(0x0204D64C, (u32)dsiSaveSetLength);
			setBL(0x0204D65C, (u32)dsiSaveWrite);
			setBL(0x0204D664, (u32)dsiSaveClose);
		}
	}

	// Prehistorik Man (Europe, Australia)
	else if (strcmp(romTid, "KPHV") == 0) {
		if (!twlFontFound) {
			setB(0x0204A100, 0x0204A398); // Skip Manual screen
		}
		if (saveOnFlashcard) {
			setBL(0x020624A4, (u32)dsiSaveOpen);
			setBL(0x020624BC, (u32)dsiSaveGetLength);
			setBL(0x020624D4, (u32)dsiSaveRead);
			setBL(0x020624DC, (u32)dsiSaveClose);
			setBL(0x0206251C, (u32)dsiSaveGetInfo);
			setBL(0x0206252C, (u32)dsiSaveGetResultCode);
			setBL(0x02062578, (u32)dsiSaveCreate);
			setBL(0x02062580, (u32)dsiSaveGetResultCode);
			setBL(0x020625D4, (u32)dsiSaveOpen);
			setBL(0x020625E4, (u32)dsiSaveGetResultCode);
			setBL(0x02062628, (u32)dsiSaveSetLength);
			setBL(0x02062638, (u32)dsiSaveWrite);
			setBL(0x02062640, (u32)dsiSaveClose);
		}
	}

	// Zekkyou Genshiji: Samu no Daibouken (Japan)
	else if (strcmp(romTid, "KPHJ") == 0) {
		if (!twlFontFound) {
			setB(0x02038A68, 0x02038CFC); // Skip Manual screen
		}
		if (saveOnFlashcard) {
			setBL(0x020487DC, (u32)dsiSaveOpen);
			setBL(0x020487F8, (u32)dsiSaveGetLength);
			setBL(0x0204880C, (u32)dsiSaveRead);
			setBL(0x02048814, (u32)dsiSaveClose);
			setBL(0x02048854, (u32)dsiSaveGetInfo);
			setBL(0x02048864, (u32)dsiSaveGetResultCode);
			setBL(0x020488B0, (u32)dsiSaveCreate);
			setBL(0x020488B8, (u32)dsiSaveGetResultCode);
			setBL(0x0204890C, (u32)dsiSaveOpen);
			setBL(0x0204891C, (u32)dsiSaveGetResultCode);
			setBL(0x02048960, (u32)dsiSaveSetLength);
			setBL(0x02048970, (u32)dsiSaveWrite);
			setBL(0x02048968, (u32)dsiSaveClose);
		}
	}

	// Pro-Putt Domo (USA)
	else if (strcmp(romTid, "KDPE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound || gameOnFlashcard) {
			*(u16*)0x020179F4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		}
	}

	// Puffins: Let's Fish! (USA)
	// Puffins: Let's Fish! (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if ((strcmp(romTid, "KLFE") == 0 || strcmp(romTid, "KLFP") == 0) && saveOnFlashcard) {
		u32 offsetChange = (ndsHeader->gameCode[3] == 'P') ? 8 : 0;

		setBL(0x0202BA80-offsetChange, (u32)dsiSaveGetLength);
		setBL(0x0202BB78-offsetChange, (u32)dsiSaveRead);
		setBL(0x0202BC68-offsetChange, (u32)dsiSaveSeek);
		setBL(0x0202BD00-offsetChange, (u32)dsiSaveWrite);
		setBL(0x0202BDBC-offsetChange, (u32)dsiSaveClose);
		setBL(0x0202BE08-offsetChange, (u32)dsiSaveOpen);
		setBL(0x0202BE68-offsetChange, (u32)dsiSaveCreate);
	}

	// Puffins: Let's Race! (USA)
	// Puffins: Let's Race! (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if ((strcmp(romTid, "KLRE") == 0 || strcmp(romTid, "KLRP") == 0) && saveOnFlashcard) {
		setBL(0x020289D0, (u32)dsiSaveGetLength);
		setBL(0x02028AC8, (u32)dsiSaveRead);
		setBL(0x02028BB8, (u32)dsiSaveSeek);
		setBL(0x02028C50, (u32)dsiSaveWrite);
		setBL(0x02028D0C, (u32)dsiSaveClose);
		setBL(0x02028D58, (u32)dsiSaveOpen);
		setBL(0x02028DB8, (u32)dsiSaveCreate);
	}

	// Puffins: Let's Roll! (USA)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KL2E") == 0 && saveOnFlashcard) {
		*(u32*)0x0204BF1C = (u32)dsiSaveGetLength;
		setBL(0x0204BFA0, (u32)dsiSaveRead);
		setBL(0x0204C074, (u32)dsiSaveSeek);
		setBL(0x0204C10C, (u32)dsiSaveWrite);
		setBL(0x0204C1B4, (u32)dsiSaveClose);
		setBL(0x0204C1E8, (u32)dsiSaveOpen);
		setBL(0x0204C230, (u32)dsiSaveCreate);
	}

	// Puffins: Let's Roll! (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KL2P") == 0 && saveOnFlashcard) {
		*(u32*)0x0204C4D4 = (u32)dsiSaveGetLength;
		setBL(0x0204C558, (u32)dsiSaveRead);
		setBL(0x0204C62C, (u32)dsiSaveSeek);
		setBL(0x0204C6C4, (u32)dsiSaveWrite);
		setBL(0x0204C76C, (u32)dsiSaveClose);
		setBL(0x0204C7A0, (u32)dsiSaveOpen);
		setBL(0x0204C7E8, (u32)dsiSaveCreate);
	}

	// Puzzler Brain Games (USA)
	else if (strcmp(romTid, "KYEE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020051BC = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x02015AD4, dsiSaveGetResultCode, 0xC);
			setBL(0x02026984, (u32)dsiSaveOpen);
			setBL(0x020269CC, (u32)dsiSaveGetLength);
			setBL(0x02026A14, (u32)dsiSaveRead);
			setBL(0x02026A30, (u32)dsiSaveClose);
			setBL(0x02026B24, (u32)dsiSaveCreate);
			setBL(0x02026B34, (u32)dsiSaveOpen);
			setBL(0x02026B80, (u32)dsiSaveSetLength);
			setBL(0x02026B94, (u32)dsiSaveWrite);
			setBL(0x02026BA8, (u32)dsiSaveClose);
		}
	}

	// Puzzler World 2013 (USA)
	else if (strcmp(romTid, "KYGE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020051BC = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x0201542C, dsiSaveGetResultCode, 0xC);
			setBL(0x0202663C, (u32)dsiSaveOpen);
			setBL(0x02026684, (u32)dsiSaveGetLength);
			setBL(0x020266CC, (u32)dsiSaveRead);
			setBL(0x020266E8, (u32)dsiSaveClose);
			setBL(0x020267DC, (u32)dsiSaveCreate);
			setBL(0x020267EC, (u32)dsiSaveOpen);
			setBL(0x02026838, (u32)dsiSaveSetLength);
			setBL(0x0202684C, (u32)dsiSaveWrite);
			setBL(0x02026860, (u32)dsiSaveClose);
		}
	}

	// Puzzler World XL (USA)
	else if (strcmp(romTid, "KUOE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020050F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020051BC = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x02015AD0, dsiSaveGetResultCode, 0xC);
			setBL(0x0202693C, (u32)dsiSaveOpen);
			setBL(0x02026984, (u32)dsiSaveGetLength);
			setBL(0x020269CC, (u32)dsiSaveRead);
			setBL(0x020269E8, (u32)dsiSaveClose);
			setBL(0x02026ADC, (u32)dsiSaveCreate);
			setBL(0x02026AEC, (u32)dsiSaveOpen);
			setBL(0x02026B38, (u32)dsiSaveSetLength);
			setBL(0x02026B4C, (u32)dsiSaveWrite);
			setBL(0x02026B60, (u32)dsiSaveClose);
		}
	}

	// Puzzle to Go: Baby Animals (Europe)
	else if (strcmp(romTid, "KBYP") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x02032AFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02032B80 = 0xE1A00000; // nop
			*(u32*)0x02032B88 = 0xE1A00000; // nop
			*(u32*)0x02032B94 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Puzzle to Go: Diddl (Europe)
	// Puzzle to Go: Wildlife (Europe)
	else if (strcmp(romTid, "KPUP") == 0 || strcmp(romTid, "KPDP") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x0203ECC4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0203ED28 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0203ED4C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0203ED80 = 0xE1A00000; // nop
			*(u32*)0x0203ED8C = 0xE1A00000; // nop
		}
	}

	// Puzzle to Go: Planets and Universe (Europe)
	// Puzzle to Go: Sightseeing (Europe)
	else if (strcmp(romTid, "KBXP") == 0 || strcmp(romTid, "KB3P") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen
			*(u32*)0x02032BF0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02032C74 = 0xE1A00000; // nop
			*(u32*)0x02032C7C = 0xE1A00000; // nop
			*(u32*)0x02032C88 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Quick Fill Q (USA)
	// Quick Fill Q (Europe)
	// A bit hard/confusing to add save support
	else if ((strcmp(romTid, "KUME") == 0 || strcmp(romTid, "KUMP") == 0) && !twlFontFound) {
		// *(u32*)0x0203EA70 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02040240 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Anaume Pazuru Gemu Q (Japan)
	// A bit hard/confusing to add save support
	else if (strcmp(romTid, "KUMJ") == 0 && !twlFontFound) {
		*(u32*)0x02040460 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Rabi Laby (USA)
	// Rabi Laby (Europe)
	else if (strcmp(romTid, "KLBE") == 0 || strcmp(romTid, "KLBP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020051C8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020053A8 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Akushon Pazuru: Rabi x Rabi (Japan)
	else if (strcmp(romTid, "KLBJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005190 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02005360 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Rabi Laby 2 (USA)
	// Rabi Laby 2 (Europe)
	// Akushon Pazuru: Rabi x Rabi Episodo 2 (Japan)
	else if (strncmp(romTid, "KLV", 3) == 0) {
		if (!twlFontFound) {
			*(u32*)0x020051E8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200540C = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Real Crimes: Jack the Ripper (USA)
	else if (strcmp(romTid, "KRCE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			*(u16*)0x020471B4 = 0x2100; // movs r1, #0 (Skip Manual screen)
		}
	}

	// Real Crimes: Jack the Ripper (Europe, Australia)
	else if (strcmp(romTid, "KRCV") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound) {
			*(u16*)0x020471E4 = 0x2100; // movs r1, #0 (Skip Manual screen)
		}
	}

	// Redau Shirizu: Gunjin Shougi (Japan)
	else if (strcmp(romTid, "KLXJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005254 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	else if (strcmp(romTid, "KJZJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02049A64 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x020532B8 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		}
		if (saveOnFlashcard) {
			setBL(0x02029C0C, (u32)dsiSaveCreate);
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
	}

	// Renjuku Kanji: Shougaku 2 Nensei (Japan)
	// Renjuku Kanji: Shougaku 3 Nensei (Japan)
	else if (strcmp(romTid, "KJ2J") == 0 || strcmp(romTid, "KJ3J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02049A4C = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x020532A0 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		}
		if (saveOnFlashcard) {
			setBL(0x02029C0C, (u32)dsiSaveCreate);
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
	}

	// Renjuku Kanji: Shougaku 4 Nensei (Japan)
	// Renjuku Kanji: Shougaku 5 Nensei (Japan)
	// Renjuku Kanji: Shougaku 6 Nensei (Japan)
	else if (strcmp(romTid, "KJ4J") == 0 || strcmp(romTid, "KJ5J") == 0 || strcmp(romTid, "KJ6J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02049AE8 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x0205333C = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		}
		if (saveOnFlashcard) {
			setBL(0x02029C0C, (u32)dsiSaveCreate);
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
	}

	// Renjuku Kanji: Chuugakusei (Japan)
	else if (strcmp(romTid, "KJ8J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02049A68 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x020532BC = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		}
		if (saveOnFlashcard) {
			setBL(0x02029C0C, (u32)dsiSaveCreate);
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
	}

	// Robot Rescue (USA)
	else if (strcmp(romTid, "KRTE") == 0) {
		if (saveOnFlashcard) {
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
			setBL(0x0200C8D0, (u32)dsiSaveGetResultCode);
			setBL(0x0200C904, (u32)dsiSaveSetLength);
			setBL(0x0200C914, (u32)dsiSaveWrite);
			setBL(0x0200C91C, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x020108A4 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Robot Rescue (Europe, Australia)
	else if (strcmp(romTid, "KRTV") == 0) {
		if (saveOnFlashcard) {
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
			setBL(0x0200C870, (u32)dsiSaveClose);
			setBL(0x0200C878, (u32)dsiSaveDelete);
			setBL(0x0200C890, (u32)dsiSaveCreate);
			setBL(0x0200C8A0, (u32)dsiSaveOpen);
			setBL(0x0200C8B0, (u32)dsiSaveGetResultCode);
			setBL(0x0200C8CC, (u32)dsiSaveSetLength);
			setBL(0x0200C8DC, (u32)dsiSaveWrite);
			setBL(0x0200C8E4, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x02010C30 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// ARC Style: Robot Rescue (Japan)
	else if (strcmp(romTid, "KRTJ") == 0) {
		if (saveOnFlashcard) {
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
			setBL(0x0200F8C8, (u32)dsiSaveGetLength);
			setBL(0x0200F8EC, (u32)dsiSaveRead);
			setBL(0x0200F8F4, (u32)dsiSaveClose);
			setBL(0x0200F938, (u32)dsiSaveOpen);
			setBL(0x0200F94C, (u32)dsiSaveClose);
			setBL(0x0200F960, (u32)dsiSaveCreate);
			setBL(0x0200F97C, (u32)dsiSaveOpen);
			setBL(0x0200F998, (u32)dsiSaveClose);
			setBL(0x0200F9A0, (u32)dsiSaveDelete);
			setBL(0x0200F9B8, (u32)dsiSaveCreate);
			setBL(0x0200F9C8, (u32)dsiSaveOpen);
			setBL(0x0200F9D8, (u32)dsiSaveGetResultCode);
			setBL(0x0200F9F4, (u32)dsiSaveSetLength);
			setBL(0x0200FA04, (u32)dsiSaveWrite);
			setBL(0x0200FA0C, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			*(u32*)0x02013BC8 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Robot Rescue 2 (USA)
	// Robot Rescue 2 (Europe)
	else if (strcmp(romTid, "KRRE") == 0 || strcmp(romTid, "KRRP") == 0) {
		if (saveOnFlashcard) {
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
			setBL(0x0200C898, (u32)dsiSaveClose);
			setBL(0x0200C8A0, (u32)dsiSaveDelete);
			setBL(0x0200C8B8, (u32)dsiSaveCreate);
			setBL(0x0200C8C8, (u32)dsiSaveOpen);
			setBL(0x0200C8D8, (u32)dsiSaveGetResultCode);
			setBL(0x0200C8F4, (u32)dsiSaveSetLength);
			setBL(0x0200C904, (u32)dsiSaveWrite);
			setBL(0x0200C90C, (u32)dsiSaveClose);
		}
		if (!twlFontFound) {
			if (ndsHeader->gameCode[3] == 'E') {
				*(u32*)0x02010888 = 0xE1A00000; // nop (Skip Manual screen)
			} else if (ndsHeader->gameCode[3] == 'P') {
				*(u32*)0x02010BE4 = 0xE1A00000; // nop (Skip Manual screen)
			}
		}
	}

	// Rock-n-Roll Domo (USA)
	else if (strcmp(romTid, "KD6E") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound || gameOnFlashcard) {
			*(u16*)0x02016514 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		}
	}

	// Sea Battle (USA)
	else if (strcmp(romTid, "KRWE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005248 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			setBL(0x02030F00, (u32)dsiSaveCreate);
			setBL(0x02030F10, (u32)dsiSaveOpen);
			setBL(0x02030F3C, (u32)dsiSaveWrite);
			setBL(0x02030F4C, (u32)dsiSaveClose);
			setBL(0x02030F68, (u32)dsiSaveClose);
			setBL(0x02030FD4, (u32)dsiSaveOpen);
			setBL(0x02030FE4, (u32)dsiSaveGetLength);
			setBL(0x02030FFC, (u32)dsiSaveRead);
			setBL(0x02031040, (u32)dsiSaveClose);
			setBL(0x0203105C, (u32)dsiSaveClose);
		}
	}

	// Sea Battle (Europe)
	else if (strcmp(romTid, "KRWP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005248 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			setBL(0x02030FBC, (u32)dsiSaveCreate);
			setBL(0x02030FCC, (u32)dsiSaveOpen);
			setBL(0x02030FF8, (u32)dsiSaveWrite);
			setBL(0x02031008, (u32)dsiSaveClose);
			setBL(0x02031024, (u32)dsiSaveClose);
			setBL(0x02031090, (u32)dsiSaveOpen);
			setBL(0x020310A0, (u32)dsiSaveGetLength);
			setBL(0x020310B8, (u32)dsiSaveRead);
			setBL(0x020310FC, (u32)dsiSaveClose);
			setBL(0x02031118, (u32)dsiSaveClose);
		}
	}

	// Shantae: Risky's Revenge (USA)
	else if (strcmp(romTid, "KS3E") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen
			/* *(u32*)0x02016130 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020161C8 = 0xE1A00000; // nop
			*(u32*)0x020161D0 = 0xE1A00000; // nop
			*(u32*)0x020161DC = 0xE1A00000; // nop
			*(u32*)0x020166C8 = 0xE3A06901; // mov r6, #0x4000 */

			// Hide help button
			*(u32*)0x02016688 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Shantae: Risky's Revenge (Europe)
	else if (strcmp(romTid, "KS3P") == 0) {
		if (!twlFontFound) {
			// Skip Manual screen
			/* *(u32*)0x020163B0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02016448 = 0xE1A00000; // nop
			*(u32*)0x02016450 = 0xE1A00000; // nop
			*(u32*)0x0201645C = 0xE1A00000; // nop
			*(u32*)0x02016940 = 0xE3A06901; // mov r6, #0x4000 */

			// Hide help button
			*(u32*)0x02016904 = 0xE1A00000; // nop
		}
		if (saveOnFlashcard) {
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
	}

	// Shawn Johnson Gymnastics (USA)
	else if (strcmp(romTid, "KSJE") == 0) {
		if (!twlFontFound) {
			setB(0x02005384, 0x020053C0); // Disable NFTR loading from TWLNAND
		}
		if (saveOnFlashcard) {
			*(u32*)0x02090C7C = 0xE3A00003; // mov r0, #3
			setBL(0x02090D84, (u32)dsiSaveCreate);
			*(u32*)0x02090D98 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			setBL(0x02090DC4, (u32)dsiSaveGetResultCode);
			setBL(0x02090DE4, (u32)dsiSaveOpen);
			setBL(0x02090E14, (u32)dsiSaveSetLength);
			*(u32*)0x02090E24 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			setBL(0x02090E68, (u32)dsiSaveWrite);
			setBL(0x02090E70, (u32)dsiSaveClose);
			setBL(0x02090F54, (u32)dsiSaveOpen);
			setBL(0x02090F84, (u32)dsiSaveGetLength);
			setBL(0x02090FAC, (u32)dsiSaveClose);
			setBL(0x02090FEC, (u32)dsiSaveClose);
			setBL(0x0209100C, (u32)dsiSaveRead);
			setBL(0x02091020, (u32)dsiSaveClose);
			setBL(0x02091084, (u32)dsiSaveDelete);
			setBL(0x02091150, (u32)dsiSaveOpen);
			setBL(0x02091160, (u32)dsiSaveClose);
		}
	}

	// Simple DS Series Vol. 1: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KM4J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200F91C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			// *(u32*)0x020128BC = 0xE3A00000; // mov r0, #0
			// *(u32*)0x020128C0 = 0xE12FFF1E; // bx lr
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
	}

	// Simple DS Series Vol. 2: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KM5J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200F9EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Simple DS Series Vol. 3: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "K5QJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200EE50 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Simple DS Series Vol. 4: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KEYJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200F9FC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Simple DS Series Vol. 5: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "K5KJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200F990 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Simple DS Series Vol. 6: The Misshitsukara no Dasshutsu (Japan)
	else if (strcmp(romTid, "KLHJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200F9A8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Simply Mahjong (USA)
	else if (strcmp(romTid, "K4JE") == 0 && saveOnFlashcard) {
		*(u32*)0x02014288 = 0xE12FFF1E; // bx lr (Skip NAND error checking)
		setBL(0x02014350, (u32)dsiSaveClose);
		setBL(0x020143AC, (u32)dsiSaveOpen);
		*(u32*)0x02014400 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x02014410, (u32)dsiSaveGetResultCode);
		setBL(0x0201442C, (u32)dsiSaveCreate);
		setBL(0x02014458, (u32)dsiSaveOpen);
		setBL(0x02014480, (u32)dsiSaveSetLength);
		*(u32*)0x02014498 = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
		setBL(0x020144B0, (u32)dsiSaveGetLength);
		setBL(0x02014524, (u32)dsiSaveSeek);
		setBL(0x02014550, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201459C, (u32)dsiSaveSeek);
		setBL(0x020145C8, (u32)dsiSaveRead); // dsiSaveReadAsync
	}

	// Simply Mahjong (Europe)
	else if (strcmp(romTid, "K4JP") == 0 && saveOnFlashcard) {
		*(u32*)0x02014278 = 0xE12FFF1E; // bx lr (Skip NAND error checking)
		setBL(0x02014340, (u32)dsiSaveClose);
		setBL(0x0201439C, (u32)dsiSaveOpen);
		*(u32*)0x020143F0 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x02014400, (u32)dsiSaveGetResultCode);
		setBL(0x0201441C, (u32)dsiSaveCreate);
		setBL(0x02014448, (u32)dsiSaveOpen);
		setBL(0x02014470, (u32)dsiSaveSetLength);
		*(u32*)0x02014488 = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
		setBL(0x020144A0, (u32)dsiSaveGetLength);
		setBL(0x02014514, (u32)dsiSaveSeek);
		setBL(0x02014540, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201458C, (u32)dsiSaveSeek);
		setBL(0x020145B8, (u32)dsiSaveRead); // dsiSaveReadAsync
	}

	// Simply Minesweeper (USA)
	// Simply Minesweeper (Europe)
	else if ((strcmp(romTid, "KM3E") == 0 || strcmp(romTid, "KM3P") == 0) && saveOnFlashcard) {
		*(u32*)0x02012370 = 0xE12FFF1E; // bx lr (Skip NAND error checking)
		setBL(0x02012438, (u32)dsiSaveClose);
		setBL(0x02012494, (u32)dsiSaveOpen);
		*(u32*)0x020124E8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x020124F8, (u32)dsiSaveGetResultCode);
		setBL(0x02012514, (u32)dsiSaveCreate);
		setBL(0x02012540, (u32)dsiSaveOpen);
		setBL(0x02012568, (u32)dsiSaveSetLength);
		*(u32*)0x02012580 = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
		setBL(0x02012598, (u32)dsiSaveGetLength);
		setBL(0x0201260C, (u32)dsiSaveSeek);
		setBL(0x02012638, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02012684, (u32)dsiSaveSeek);
		setBL(0x020126B0, (u32)dsiSaveRead); // dsiSaveReadAsync
	}

	// Simply Solitaire (USA)
	// Simply Solitaire (Europe)
	else if ((strcmp(romTid, "K4LE") == 0 || strcmp(romTid, "K4LP") == 0) && saveOnFlashcard) {
		*(u32*)0x02013504 = 0xE12FFF1E; // bx lr (Skip NAND error checking)
		setBL(0x020135CC, (u32)dsiSaveClose);
		setBL(0x02013628, (u32)dsiSaveOpen);
		*(u32*)0x0201367C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x0201368C, (u32)dsiSaveGetResultCode);
		setBL(0x020136A8, (u32)dsiSaveCreate);
		setBL(0x020136D4, (u32)dsiSaveOpen);
		setBL(0x020136FC, (u32)dsiSaveSetLength);
		*(u32*)0x02013714 = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
		setBL(0x0201372C, (u32)dsiSaveGetLength);
		setBL(0x020137A0, (u32)dsiSaveSeek);
		setBL(0x020137CC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02013818, (u32)dsiSaveSeek);
		setBL(0x02013844, (u32)dsiSaveRead); // dsiSaveReadAsync
	}

	// Simply Sudoku (Europe)
	else if (strcmp(romTid, "KS4P") == 0 && saveOnFlashcard) {
		*(u32*)0x02013970 = 0xE12FFF1E; // bx lr (Skip NAND error checking)
		setBL(0x02013A38, (u32)dsiSaveClose);
		setBL(0x02013A94, (u32)dsiSaveOpen);
		*(u32*)0x02013AE8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x02013AF8, (u32)dsiSaveGetResultCode);
		setBL(0x02013B14, (u32)dsiSaveCreate);
		setBL(0x02013B40, (u32)dsiSaveOpen);
		setBL(0x02013B68, (u32)dsiSaveSetLength);
		*(u32*)0x02013B80 = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
		setBL(0x02013B98, (u32)dsiSaveGetLength);
		setBL(0x02013C0C, (u32)dsiSaveSeek);
		setBL(0x02013C38, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02013C84, (u32)dsiSaveSeek);
		setBL(0x02013CB0, (u32)dsiSaveRead); // dsiSaveReadAsync
	}

	// Smart Girl's Playhouse Mini (USA)
	else if (strcmp(romTid, "K2FE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02026128 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x0202E6F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Snakenoid Deluxe (USA)
	else if (strcmp(romTid, "K4NE") == 0 && saveOnFlashcard) {
		setBL(0x0204D990, (u32)dsiSaveCreate);
		setBL(0x0204D9A0, (u32)dsiSaveOpen);
		setBL(0x0204D9BC, (u32)dsiSaveGetResultCode);
		setBL(0x0204D9E0, (u32)dsiSaveSeek);
		setBL(0x0204D9F8, (u32)dsiSaveGetResultCode);
		setBL(0x0204DA1C, (u32)dsiSaveWrite);
		setBL(0x0204DA3C, (u32)dsiSaveClose);
		setBL(0x0204DA44, (u32)dsiSaveGetResultCode);
		setBL(0x0204DA60, (u32)dsiSaveGetResultCode);
		setBL(0x0204DA9C, (u32)dsiSaveOpenR);
		setBL(0x0204DAAC, (u32)dsiSaveGetLength);
		setBL(0x0204DAE0, (u32)dsiSaveRead);
		setBL(0x0204DAF8, (u32)dsiSaveClose);
		setBL(0x0204DB04, (u32)dsiSaveGetResultCode);
		setBL(0x020545F4, 0x0204DB60);
	}

	// Snakenoid (Europe)
	else if (strcmp(romTid, "K4NP") == 0 && saveOnFlashcard) {
		setBL(0x02040644, (u32)dsiSaveCreate);
		setBL(0x02040654, (u32)dsiSaveOpen);
		setBL(0x02040670, (u32)dsiSaveGetResultCode);
		setBL(0x02040698, (u32)dsiSaveSeek);
		setBL(0x020406B0, (u32)dsiSaveGetResultCode);
		setBL(0x020406D8, (u32)dsiSaveWrite);
		setBL(0x020406E0, (u32)dsiSaveClose);
		setBL(0x020406E8, (u32)dsiSaveGetResultCode);
		setBL(0x02040708, (u32)dsiSaveGetResultCode);
		setBL(0x02040748, (u32)dsiSaveOpenR);
		setBL(0x02040758, (u32)dsiSaveGetLength);
		setBL(0x0204078C, (u32)dsiSaveRead);
		setBL(0x020407A4, (u32)dsiSaveClose);
		setBL(0x020471BC, 0x020407F8);
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
	else if ((strcmp(romTid, "KL9J") == 0 || strcmp(romTid, "KH2J") == 0) && !twlFontFound) {
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
		   || strcmp(romTid, "KO6J") == 0 || strcmp(romTid, "KO7J") == 0) && !twlFontFound) {
		*(u32*)0x02027CD8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

		// Skip Manual screen
		*(u32*)0x02027EFC = 0xE1A00000; // nop
		*(u32*)0x02027F04 = 0xE1A00000; // nop
		*(u32*)0x02027F10 = 0xE1A00000; // nop
	}

	// Space Ace (USA)
	else if (strcmp(romTid, "KA6E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Spin Six (USA)
	else if (strcmp(romTid, "KQ6E") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02013184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Spin Six (Europe, Australia)
	else if (strcmp(romTid, "KQ6V") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02013184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Kuru Kuru Akushon: Kuru Pachi 6 (Japan)
	else if (strcmp(romTid, "KQ6J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02013760 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
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
	}

	// Sudoku (USA)
	// Sudoku (USA) (Rev 1)
	else if (strcmp(romTid, "K4DE") == 0) {
		if (ndsHeader->romversion == 1) {
			if (!twlFontFound) {
				*(u32*)0x0200698C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
			if (saveOnFlashcard) {
				// *(u32*)0x0203701C = 0xE3A00001; // mov r0, #1
				// *(u32*)0x02037020 = 0xE12FFF1E; // bx lr
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
			}
		} else {
			if (!twlFontFound) {
				*(u32*)0x0200695C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
			if (saveOnFlashcard) {
				// *(u32*)0x0203609C = 0xE3A00001; // mov r0, #1
				// *(u32*)0x020360A0 = 0xE12FFF1E; // bx lr
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
	}

	// Sudoku (Europe, Australia) (Rev 1)
	else if (strcmp(romTid, "K4DV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0200698C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (saveOnFlashcard) {
			setBL(0x020375AC, (u32)dsiSaveOpen);
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
	}

	// Sudoku 4Pockets (USA)
	// Sudoku 4Pockets (Europe)
	else if (strcmp(romTid, "K4FE") == 0 || strcmp(romTid, "K4FP") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02004C4C = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			*(u32*)0x0202E888 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0202E88C = 0xE12FFF1E; // bx lr
		}
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
	else if (strcmp(romTid, "KTEE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02054C30 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			setBL(0x0205A768, (u32)dsiSaveOpenR);
			setBL(0x0205A778, (u32)dsiSaveGetLength);
			setBL(0x0205A7B0, (u32)dsiSaveRead);
			setBL(0x0205A7CC, (u32)dsiSaveClose);
			// *(u32*)0x0205A83C = 0xE12FFF1E; // bx lr
			setBL(0x0205A864, (u32)dsiSaveCreate);
			setBL(0x0205A874, (u32)dsiSaveGetResultCode);
			setBL(0x0205A89C, (u32)dsiSaveOpen);
			setBL(0x0205A8C0, (u32)dsiSaveWrite);
			setBL(0x0205A8F0, (u32)dsiSaveClose);
			// *(u32*)0x0205A92C = 0xE12FFF1E; // bx lr
			setBL(0x0205A95C, (u32)dsiSaveOpen);
			setBL(0x0205A99C, (u32)dsiSaveSeek);
			setBL(0x0205A9DC, (u32)dsiSaveWrite);
			setBL(0x0205AA54, (u32)dsiSaveSeek);
			setBL(0x0205AA80, (u32)dsiSaveWrite);
			setBL(0x0205AABC, (u32)dsiSaveClose);
		}
	}

	// Tetris Party Live (Europe, Australia)
	else if (strcmp(romTid, "KTEV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02054C30 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			setBL(0x0205A754, (u32)dsiSaveOpenR);
			setBL(0x0205A764, (u32)dsiSaveGetLength);
			setBL(0x0205A79C, (u32)dsiSaveRead);
			setBL(0x0205A7B8, (u32)dsiSaveClose);
			// *(u32*)0x0205A828 = 0xE12FFF1E; // bx lr
			setBL(0x0205A850, (u32)dsiSaveCreate);
			setBL(0x0205A860, (u32)dsiSaveGetResultCode);
			setBL(0x0205A888, (u32)dsiSaveOpen);
			setBL(0x0205A8AC, (u32)dsiSaveWrite);
			setBL(0x0205A8DC, (u32)dsiSaveClose);
			// *(u32*)0x0205A918 = 0xE12FFF1E; // bx lr
			setBL(0x0205A948, (u32)dsiSaveOpen);
			setBL(0x0205A988, (u32)dsiSaveSeek);
			setBL(0x0205A9C8, (u32)dsiSaveWrite);
			setBL(0x0205AA40, (u32)dsiSaveSeek);
			setBL(0x0205AA6C, (u32)dsiSaveWrite);
			setBL(0x0205AAA8, (u32)dsiSaveClose);
		}
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
		// *(u32*)0x02009A84 = 0xE12FFF1E; // bx lr
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
	else if (strcmp(romTid, "K6PJ") == 0 && !twlFontFound) {
		*(u32*)0x02006E84 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		// *(u32*)0x02007344 = 0xE1A00000; // nop (Skip directory browse)
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

	// Tori to Mame (Japan)
	else if (strcmp(romTid, "KP6J") == 0) {
		if (!twlFontFound) {
			*(u32*)0x020217B0 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
			*(u32*)0x02021954 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		if (saveOnFlashcard) {
			setBL(0x02023348, (u32)dsiSaveOpen);
			setBL(0x02023360, (u32)dsiSaveGetLength);
			setBL(0x02023398, (u32)dsiSaveRead);
			setBL(0x020233BC, (u32)dsiSaveClose);
			*(u32*)0x020233FC = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
			setBL(0x02023430, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02023440, (u32)dsiSaveOpen);
			setBL(0x02023460, (u32)dsiSaveSetLength);
			setBL(0x02023480, (u32)dsiSaveWrite);
			setBL(0x02023498, (u32)dsiSaveClose);
		}
	}

	// VT Tennis (USA)
	else if (strcmp(romTid, "KVTE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0205C000 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x0201B634, dsiSaveGetResultCode, 0xC);
			setBL(0x0209C1B0, (u32)dsiSaveCreate);
			setBL(0x0209C1C0, (u32)dsiSaveOpen);
			setBL(0x0209C1E8, (u32)dsiSaveSetLength);
			setBL(0x0209C1F8, (u32)dsiSaveWrite);
			setBL(0x0209C200, (u32)dsiSaveClose);
			setBL(0x0209C284, (u32)dsiSaveOpen);
			setBL(0x0209C2B0, (u32)dsiSaveRead);
			setBL(0x0209C2BC, (u32)dsiSaveClose);
		}
	}

	// VT Tennis (Europe, Australia)
	else if (strcmp(romTid, "KVTV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0205B314 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		}
		if (saveOnFlashcard) {
			tonccpy((u32*)0x0201AD00, dsiSaveGetResultCode, 0xC);
			setBL(0x0209B120, (u32)dsiSaveCreate);
			setBL(0x0209B130, (u32)dsiSaveOpen);
			setBL(0x0209B158, (u32)dsiSaveSetLength);
			setBL(0x0209B168, (u32)dsiSaveWrite);
			setBL(0x0209B170, (u32)dsiSaveClose);
			setBL(0x0209B1F4, (u32)dsiSaveOpen);
			setBL(0x0209B220, (u32)dsiSaveRead);
			setBL(0x0209B22C, (u32)dsiSaveClose);
		}
	}

	// Wakugumi: Monochrome Puzzle (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KK4V") == 0) {
		if (saveOnFlashcard) {
			*(u32*)0x0204F240 = 0xE3A00000; // mov r0, #0
		}
		if (!twlFontFound) {
			*(u32*)0x02050114 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// White-Water Domo (USA)
	else if (strcmp(romTid, "KDWE") == 0) {
		if (saveOnFlashcard) {
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
		}
		if (!twlFontFound || gameOnFlashcard) {
			*(u16*)0x02013B10 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		}
	}

	// Wonderful Sports: Bowling (Japan)
	else if (strcmp(romTid, "KBSJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			*(u32*)0x02029AF0 = 0xE1A00000; // nop
			*(u32*)0x02029AF8 = 0xE1A00000; // nop

			// Skip NFTR font rendering
			*(u32*)0x02028C50 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02033D88 = 0xE12FFF1E; // bx lr
		}
		if (saveOnFlashcard) {
			setBL(0x02027AF4, (u32)dsiSaveOpen);
			setBL(0x02027B04, (u32)dsiSaveGetLength);
			setBL(0x02027B1C, (u32)dsiSaveRead);
			setBL(0x02027B6C, (u32)dsiSaveClose);
			setBL(0x02027BFC, (u32)dsiSaveCreate);
			setBL(0x02027C0C, (u32)dsiSaveOpen);
			setBL(0x02027C20, (u32)dsiSaveSetLength);
			setBL(0x02027C30, (u32)dsiSaveWrite);
			setBL(0x02027C38, (u32)dsiSaveClose);
		}
	}

	// X-Scape (USA)
	/*else if (strcmp(romTid, "KDXE") == 0) {
		// Speed up file loading by 0.5 seconds
		*(u16*)0x020B13D0 = 0x2001; // movs r0, #1
		*(u16*)0x020B13D2 = 0x46C0; // nop
	}*/

	// Yummy Yummy Cooking Jam (USA)
	else if (strcmp(romTid, "KYUE") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x0201DA10, dsiSaveGetResultCode, 0xC);
		setBL(0x02069A78, (u32)dsiSaveOpen);
		setBL(0x02069AB4, (u32)dsiSaveRead);
		setBL(0x02069ACC, (u32)dsiSaveRead);
		setBL(0x02069AD4, (u32)dsiSaveClose);
		setBL(0x02069B60, (u32)dsiSaveCreate);
		setBL(0x02069B70, (u32)dsiSaveOpen);
		setBL(0x02069BA8, (u32)dsiSaveSetLength);
		setBL(0x02069BB8, (u32)dsiSaveWrite);
		setBL(0x02069BE0, (u32)dsiSaveWrite);
		setBL(0x02069BE8, (u32)dsiSaveClose);
	}

	// Yummy Yummy Cooking Jam (Europe, Australia)
	else if (strcmp(romTid, "KYUV") == 0 && saveOnFlashcard) {
		tonccpy((u32*)0x02019DE4, dsiSaveGetResultCode, 0xC);
		setBL(0x02065E1C, (u32)dsiSaveOpen);
		setBL(0x02065E58, (u32)dsiSaveRead);
		setBL(0x02065E70, (u32)dsiSaveRead);
		setBL(0x02065E78, (u32)dsiSaveClose);
		setBL(0x02065F04, (u32)dsiSaveCreate);
		setBL(0x02065F14, (u32)dsiSaveOpen);
		setBL(0x02065F4C, (u32)dsiSaveSetLength);
		setBL(0x02065F5C, (u32)dsiSaveWrite);
		setBL(0x02065F84, (u32)dsiSaveWrite);
		setBL(0x02065F8C, (u32)dsiSaveClose);
	}

	// Art Style: ZENGAGE (USA)
	// Art Style: NEMREM (Europe, Australia)
	else if ((strcmp(romTid, "KASE") == 0 || strcmp(romTid, "KASV") == 0) && saveOnFlashcard) {
		setBL(0x0200E984, (u32)dsiSaveOpen);
		setBL(0x0200EA8C, (u32)dsiSaveOpen);
		setBL(0x0200EAE4, (u32)dsiSaveRead);
		setBL(0x0200EB28, (u32)dsiSaveClose);
		// *(u32*)0x0200EBC8 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0200EBCC = 0xE12FFF1E; // bx lr
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
		// *(u32*)0x0200EE98 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0200EE9C = 0xE12FFF1E; // bx lr
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
	else if (strcmp(romTid, "KZSE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02012794 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Zoonies: Escape from Makatu (Europe, Australia)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KZSV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x02012778 = 0xE1A00000; // nop (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
		// *(u32*)0x020815F8 = 0xE12FFF1E; // bx lr
	}

	else if (dsiSD) {
		return;
	}

	// Stub out save functions (and some others)

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

	// Face Pilot: Fly With Your Nintendo DSi Camera! (USA)
	else if (strcmp(romTid, "KYBE") == 0) {
		*(u32*)0x0200BB54 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203C928 = 0xE12FFF1E; // bx lr
	}

	// Face Pilot: Fly With Your Nintendo DSi Camera! (Europe, Australia)
	else if (strcmp(romTid, "KYBV") == 0) {
		*(u32*)0x0200BB44 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203C9E4 = 0xE12FFF1E; // bx lr
	}

	// Ferrari GT: Evolution (USA)
	else if (strcmp(romTid, "KFRE") == 0) {
		*(u32*)0x0205FDA8 = 0xE12FFF1E; // bx lr
	}

	// Ferrari GT: Evolution (Europe, Australia)
	else if (strcmp(romTid, "KFRV") == 0) {
		*(u32*)0x0205FC88 = 0xE12FFF1E; // bx lr
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
	// const u32* dsiSaveOpenR = ce9->patches->dsiSaveOpenR;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveGetLength = ce9->patches->dsiSaveGetLength;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;
	const u32* dsiSaveWrite = ce9->patches->dsiSaveWrite;

	const bool twlFontFound = ((sharedFontRegion == 0 && !gameOnFlashcard) || twlSharedFont);
	//const bool chnFontFound = ((sharedFontRegion == 1 && !gameOnFlashcard) || chnSharedFont);
	//const bool korFontFound = ((sharedFontRegion == 2 && !gameOnFlashcard) || korSharedFont);

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
	else if (strcmp(romTid, "ACBJ") == 0 && ndsHeader->romversion == 0) {
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
		// *(u32*)0x0206AE70 = 0xE3A00000; //mov r0, #0
        // *(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		// *(u32*)0x0206AE74 = 0xe12fff1e; //bx lr

        // *(u32*)0x02000B94 = 0xE1A00000; //nop

		// *(u32*)0x020D5010 = 0xe12fff1e; //bx lr
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

        // *((u32*)0x02000BB0) = 0xE1A00000; //nop 

		// *(u32*)0x0206D2C4 = 0xE3A00000; //mov r0, #0
        // *(u32*)0x0206D2C4 = 0xE3A00001; //mov r0, #1
		// *(u32*)0x0206D2C8 = 0xe12fff1e; //bx lr

		// *(u32*)0x020D5010 = 0xe12fff1e; //bx lr
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

	// 1st Class Poker & BlackJack (USA)
	else if (strcmp(romTid, "KYPE") == 0 && saveOnFlashcard) {
		setBL(0x02012E50, (u32)dsiSaveOpen);
		setBL(0x02012EC4, (u32)dsiSaveGetLength);
		setBL(0x02012ED8, (u32)dsiSaveClose);
		setBL(0x02012EF8, (u32)dsiSaveSeek);
		setBL(0x02012F10, (u32)dsiSaveRead);
		setBL(0x02012F24, (u32)dsiSaveClose);
		setBL(0x02012F78, (u32)dsiSaveClose);
		*(u32*)0x02012FBC = 0xE1A00000; // nop
		setBL(0x02013028, (u32)dsiSaveCreate);
		setBL(0x0201307C, (u32)dsiSaveOpen);
		setBL(0x020130E4, (u32)dsiSaveSetLength);
		setBL(0x020130FC, (u32)dsiSaveClose);
		setBL(0x02013150, (u32)dsiSaveGetLength);
		setBL(0x02013164, (u32)dsiSaveClose);
		setBL(0x02013184, (u32)dsiSaveSeek);
		setBL(0x0201319C, (u32)dsiSaveWrite);
		setBL(0x020131B0, (u32)dsiSaveClose);
		setBL(0x020131FC, (u32)dsiSaveClose);
	}

	// 1st Class Poker & BlackJack (Europe)
	else if (strcmp(romTid, "KYPP") == 0 && saveOnFlashcard) {
		setBL(0x02012E40, (u32)dsiSaveOpen);
		setBL(0x02012EB4, (u32)dsiSaveGetLength);
		setBL(0x02012EC8, (u32)dsiSaveClose);
		setBL(0x02012EE8, (u32)dsiSaveSeek);
		setBL(0x02012F00, (u32)dsiSaveRead);
		setBL(0x02012F14, (u32)dsiSaveClose);
		setBL(0x02012F68, (u32)dsiSaveClose);
		*(u32*)0x02012FAC = 0xE1A00000; // nop
		setBL(0x02013018, (u32)dsiSaveCreate);
		setBL(0x0201306C, (u32)dsiSaveOpen);
		setBL(0x020130D4, (u32)dsiSaveSetLength);
		setBL(0x020130EC, (u32)dsiSaveClose);
		setBL(0x02013140, (u32)dsiSaveGetLength);
		setBL(0x02013154, (u32)dsiSaveClose);
		setBL(0x02013174, (u32)dsiSaveSeek);
		setBL(0x0201318C, (u32)dsiSaveWrite);
		setBL(0x020131A0, (u32)dsiSaveClose);
		setBL(0x020131EC, (u32)dsiSaveClose);
	}

	// Art Style: BASE 10 (USA)
	else if (strcmp(romTid, "KADE") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0202D25C = 0xEB00007C; // bl 0x0202D454 (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Art Style: CODE (Europe, Australia)
	else if (strcmp(romTid, "KADV") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0202D288 = 0xEB00007C; // bl 0x0202D480 (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Art Style: DECODE (Japan)
	else if (strcmp(romTid, "KADJ") == 0) {
		if (!twlFontFound) {
			*(u32*)0x0202E2AC = 0xEB000071; // bl 0x0202E478 (Skip Manual screen)
		}
		if (saveOnFlashcard) {
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
	}

	// Clubhouse Games Express: Card Classics (USA)
	else if (strcmp(romTid, "KTRT") == 0 && saveOnFlashcard) {
		setBL(0x020373A8, (u32)dsiSaveGetInfo);
		setBL(0x020373E0, (u32)dsiSaveCreate);
		setBL(0x02037434, (u32)dsiSaveOpen);
		setBL(0x02037480, (u32)dsiSaveGetLength);
		setBL(0x02037494, (u32)dsiSaveSetLength);
		setBL(0x02037528, (u32)dsiSaveSeek);
		setBL(0x02037558, (u32)dsiSaveWrite);
		setBL(0x02037594, (u32)dsiSaveClose);
		setBL(0x020376A8, (u32)dsiSaveRead);
		setBL(0x020376E4, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205D0A8, dsiSaveGetResultCode, 0xC);
	}

	// A Little Bit of... All-Time Classics: Card Classics (Europe)
	else if (strcmp(romTid, "KTRP") == 0 && saveOnFlashcard) {
		setBL(0x02037354, (u32)dsiSaveGetInfo);
		setBL(0x0203738C, (u32)dsiSaveCreate);
		setBL(0x020373E0, (u32)dsiSaveOpen);
		setBL(0x0203742C, (u32)dsiSaveGetLength);
		setBL(0x02037440, (u32)dsiSaveSetLength);
		setBL(0x020374D4, (u32)dsiSaveSeek);
		setBL(0x02037504, (u32)dsiSaveWrite);
		setBL(0x02037540, (u32)dsiSaveClose);
		setBL(0x02037654, (u32)dsiSaveRead);
		setBL(0x02037690, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205D0FC, dsiSaveGetResultCode, 0xC);
	}

	// Chotto Asobi Taizen: Jikkuri Toranpu (Japan)
	else if (strcmp(romTid, "KTRJ") == 0 && saveOnFlashcard) {
		setBL(0x02038564, (u32)dsiSaveGetInfo);
		setBL(0x0203859C, (u32)dsiSaveCreate);
		setBL(0x020385F0, (u32)dsiSaveOpen);
		setBL(0x0203863C, (u32)dsiSaveGetLength);
		setBL(0x02038650, (u32)dsiSaveSetLength);
		setBL(0x020386E4, (u32)dsiSaveSeek);
		setBL(0x02038714, (u32)dsiSaveWrite);
		setBL(0x02038750, (u32)dsiSaveClose);
		setBL(0x02038864, (u32)dsiSaveRead);
		setBL(0x020388A0, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205E154, dsiSaveGetResultCode, 0xC);
	}

	// Yixia Xia Ming Liu Daquan: Zhiyong Shuangquan (China)
	else if (strcmp(romTid, "KTRC") == 0 && saveOnFlashcard) {
		setBL(0x02036FE0, (u32)dsiSaveGetInfo);
		setBL(0x02037014, (u32)dsiSaveCreate);
		setBL(0x02037068, (u32)dsiSaveOpen);
		setBL(0x020370AC, (u32)dsiSaveGetLength);
		setBL(0x020370C0, (u32)dsiSaveSetLength);
		setBL(0x0203715C, (u32)dsiSaveSeek);
		setBL(0x02037180, (u32)dsiSaveWrite);
		setBL(0x020371AC, (u32)dsiSaveClose);
		setBL(0x020372BC, (u32)dsiSaveRead);
		setBL(0x020372F0, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205C1FC, dsiSaveGetResultCode, 0xC);
	}

	// Clubhouse Games Express: Card Classics (Korea)
	else if (strcmp(romTid, "KTRK") == 0 && saveOnFlashcard) {
		setBL(0x02036E78, (u32)dsiSaveGetInfo);
		setBL(0x02036EAC, (u32)dsiSaveCreate);
		setBL(0x02036F00, (u32)dsiSaveOpen);
		setBL(0x02036F44, (u32)dsiSaveGetLength);
		setBL(0x02036F58, (u32)dsiSaveSetLength);
		setBL(0x02036FF4, (u32)dsiSaveSeek);
		setBL(0x02037018, (u32)dsiSaveWrite);
		setBL(0x02037044, (u32)dsiSaveClose);
		setBL(0x02037154, (u32)dsiSaveRead);
		setBL(0x02037188, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205EB40, dsiSaveGetResultCode, 0xC);
	}

	// Clubhouse Games Express: Family Favorites (USA, Australia)
	else if (strcmp(romTid, "KTCT") == 0 && saveOnFlashcard) {
		setBL(0x020388E8, (u32)dsiSaveGetInfo);
		setBL(0x02038920, (u32)dsiSaveCreate);
		setBL(0x02038974, (u32)dsiSaveOpen);
		setBL(0x020389C0, (u32)dsiSaveGetLength);
		setBL(0x020389D4, (u32)dsiSaveSetLength);
		setBL(0x02038A68, (u32)dsiSaveSeek);
		setBL(0x02038A98, (u32)dsiSaveWrite);
		setBL(0x02038AD4, (u32)dsiSaveClose);
		setBL(0x02038BE8, (u32)dsiSaveRead);
		setBL(0x02038C24, (u32)dsiSaveClose);
		tonccpy((u32*)0x02060D9C, dsiSaveGetResultCode, 0xC);
	}

	// A Little Bit of... All-Time Classics: Family Games (Europe)
	else if (strcmp(romTid, "KTPP") == 0 && saveOnFlashcard) {
		setBL(0x020388A8, (u32)dsiSaveGetInfo);
		setBL(0x020388E0, (u32)dsiSaveCreate);
		setBL(0x02038934, (u32)dsiSaveOpen);
		setBL(0x02038980, (u32)dsiSaveGetLength);
		setBL(0x02038994, (u32)dsiSaveSetLength);
		setBL(0x02038A28, (u32)dsiSaveSeek);
		setBL(0x02038A58, (u32)dsiSaveWrite);
		setBL(0x02038A94, (u32)dsiSaveClose);
		setBL(0x02038BA8, (u32)dsiSaveRead);
		setBL(0x02038BE4, (u32)dsiSaveClose);
		tonccpy((u32*)0x02060CE4, dsiSaveGetResultCode, 0xC);
	}

	// Chotto Asobi Taizen: Otegaru Toranpu (Japan)
	else if (strcmp(romTid, "KTPJ") == 0 && saveOnFlashcard) {
		setBL(0x02037784, (u32)dsiSaveGetInfo);
		setBL(0x020377BC, (u32)dsiSaveCreate);
		setBL(0x02037810, (u32)dsiSaveOpen);
		setBL(0x0203785C, (u32)dsiSaveGetLength);
		setBL(0x02037870, (u32)dsiSaveSetLength);
		setBL(0x02037904, (u32)dsiSaveSeek);
		setBL(0x02037934, (u32)dsiSaveWrite);
		setBL(0x02037970, (u32)dsiSaveClose);
		setBL(0x02037A84, (u32)dsiSaveRead);
		setBL(0x02037AC0, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205D388, dsiSaveGetResultCode, 0xC);
	}

	// Yixia Xia Ming Liu Daquan: Qingsong Xiuxian (China)
	else if (strcmp(romTid, "KTPC") == 0 && saveOnFlashcard) {
		setBL(0x02037070, (u32)dsiSaveGetInfo);
		setBL(0x020370A4, (u32)dsiSaveCreate);
		setBL(0x020370F8, (u32)dsiSaveOpen);
		setBL(0x0203713C, (u32)dsiSaveGetLength);
		setBL(0x02037150, (u32)dsiSaveSetLength);
		setBL(0x020371EC, (u32)dsiSaveSeek);
		setBL(0x02037210, (u32)dsiSaveWrite);
		setBL(0x0203723C, (u32)dsiSaveClose);
		setBL(0x0203734C, (u32)dsiSaveRead);
		setBL(0x02037380, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205E388, dsiSaveGetResultCode, 0xC);
	}

	// Clubhouse Games Express: Family Favorites (Korea)
	else if (strcmp(romTid, "KTPK") == 0 && saveOnFlashcard) {
		setBL(0x02037848, (u32)dsiSaveGetInfo);
		setBL(0x0203787C, (u32)dsiSaveCreate);
		setBL(0x020378D0, (u32)dsiSaveOpen);
		setBL(0x02037914, (u32)dsiSaveGetLength);
		setBL(0x02037928, (u32)dsiSaveSetLength);
		setBL(0x020379C4, (u32)dsiSaveSeek);
		setBL(0x020379E8, (u32)dsiSaveWrite);
		setBL(0x02037A14, (u32)dsiSaveClose);
		setBL(0x02037B24, (u32)dsiSaveRead);
		setBL(0x02037B58, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205CB04, dsiSaveGetResultCode, 0xC);
	}

	// Clubhouse Games Express: Strategy Pack (USA, Australia)
	else if (strcmp(romTid, "KTDT") == 0 && saveOnFlashcard) {
		setBL(0x020386E8, (u32)dsiSaveGetInfo);
		setBL(0x02038720, (u32)dsiSaveCreate);
		setBL(0x02038774, (u32)dsiSaveOpen);
		setBL(0x020387C0, (u32)dsiSaveGetLength);
		setBL(0x020387D4, (u32)dsiSaveSetLength);
		setBL(0x02038868, (u32)dsiSaveSeek);
		setBL(0x02038898, (u32)dsiSaveWrite);
		setBL(0x020388D4, (u32)dsiSaveClose);
		setBL(0x020389E8, (u32)dsiSaveRead);
		setBL(0x02038A24, (u32)dsiSaveClose);
		tonccpy((u32*)0x02060C24, dsiSaveGetResultCode, 0xC);
	}

	// A Little Bit of... All-Time Classics: Strategy Games (Europe)
	else if (strcmp(romTid, "KTBP") == 0 && saveOnFlashcard) {
		setBL(0x0203869C, (u32)dsiSaveGetInfo);
		setBL(0x020386D4, (u32)dsiSaveCreate);
		setBL(0x02038728, (u32)dsiSaveOpen);
		setBL(0x02038774, (u32)dsiSaveGetLength);
		setBL(0x02038788, (u32)dsiSaveSetLength);
		setBL(0x0203881C, (u32)dsiSaveSeek);
		setBL(0x0203884C, (u32)dsiSaveWrite);
		setBL(0x02038888, (u32)dsiSaveClose);
		setBL(0x0203899C, (u32)dsiSaveRead);
		setBL(0x020389D8, (u32)dsiSaveClose);
		tonccpy((u32*)0x02060B60, dsiSaveGetResultCode, 0xC);
	}

	// Chotto Asobi Taizen: Onajimi Teburu (Japan)
	else if (strcmp(romTid, "KTBJ") == 0 && saveOnFlashcard) {
		setBL(0x0203795C, (u32)dsiSaveGetInfo);
		setBL(0x02037994, (u32)dsiSaveCreate);
		setBL(0x020379E8, (u32)dsiSaveOpen);
		setBL(0x02037A34, (u32)dsiSaveGetLength);
		setBL(0x02037A48, (u32)dsiSaveSetLength);
		setBL(0x02037ADC, (u32)dsiSaveSeek);
		setBL(0x02037B0C, (u32)dsiSaveWrite);
		setBL(0x02037B48, (u32)dsiSaveClose);
		setBL(0x02037C5C, (u32)dsiSaveRead);
		setBL(0x02037C98, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205D744, dsiSaveGetResultCode, 0xC);
	}

	// Yixia Xia Ming Liu Daquan: Jingdian Zhongwen (China)
	else if (strcmp(romTid, "KTBC") == 0 && saveOnFlashcard) {
		setBL(0x02036F00, (u32)dsiSaveGetInfo);
		setBL(0x02036F34, (u32)dsiSaveCreate);
		setBL(0x02036F88, (u32)dsiSaveOpen);
		setBL(0x02036FCC, (u32)dsiSaveGetLength);
		setBL(0x02036FE0, (u32)dsiSaveSetLength);
		setBL(0x0203707C, (u32)dsiSaveSeek);
		setBL(0x020370A0, (u32)dsiSaveWrite);
		setBL(0x020370CC, (u32)dsiSaveClose);
		setBL(0x020371DC, (u32)dsiSaveRead);
		setBL(0x02037210, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205C078, dsiSaveGetResultCode, 0xC);
	}

	// Clubhouse Games Express: Strategy Pack (Korea)
	else if (strcmp(romTid, "KTBK") == 0 && saveOnFlashcard) {
		setBL(0x02036EB0, (u32)dsiSaveGetInfo);
		setBL(0x02036EE4, (u32)dsiSaveCreate);
		setBL(0x02036F38, (u32)dsiSaveOpen);
		setBL(0x02036F7C, (u32)dsiSaveGetLength);
		setBL(0x02036F90, (u32)dsiSaveSetLength);
		setBL(0x0203702C, (u32)dsiSaveSeek);
		setBL(0x02037050, (u32)dsiSaveWrite);
		setBL(0x0203707C, (u32)dsiSaveClose);
		setBL(0x0203718C, (u32)dsiSaveRead);
		setBL(0x020371C0, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205E270, dsiSaveGetResultCode, 0xC);
	}

	// Globulos Party (USA)
	// Globulos Party (Europe)
	else if (strncmp(romTid, "KGS", 3) == 0 && saveOnFlashcard) {
		setBL(0x02040D0C, (u32)dsiSaveOpen);
		setBL(0x02040D44, (u32)dsiSaveClose);
		setBL(0x02040DA0, (u32)dsiSaveCreate);
		setBL(0x02040E30, (u32)dsiSaveDelete);
		setBL(0x02040EFC, (u32)dsiSaveSeek);
		setBL(0x02040F1C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02040F30, (u32)dsiSaveRead);
		setBL(0x02040F4C, (u32)dsiSaveClose);
		setBL(0x02040F68, (u32)dsiSaveClose);
		setBL(0x02041020, (u32)dsiSaveSeek);
		setBL(0x02041040, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02041054, (u32)dsiSaveWrite);
		setBL(0x02041070, (u32)dsiSaveClose);
		setBL(0x0204108C, (u32)dsiSaveClose);
		tonccpy((u32*)0x0205AEB4, dsiSaveGetResultCode, 0xC);
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
	const ltd_module_params_t* ltdModuleParams,
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

	u32 errorCodeArm9 = patchCardNdsArm9(ce9, ndsHeader, moduleParams, ltdModuleParams, ROMinRAM, patchMpuRegion, usesCloneboot);
	
	if (errorCodeArm9 == ERR_NONE || ndsHeader->fatSize == 0) {
		return patchCardNdsArm7(ce7, ndsHeader, moduleParams, ROMinRAM, saveFileCluster);
	}

	dbg_printf("ERR_LOAD_OTHR");
	return ERR_LOAD_OTHR;
}