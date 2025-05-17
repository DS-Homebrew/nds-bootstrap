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
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "hex.h"
#include "igm_text.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"
#include "unpatched_funcs.h"

#include "io_m3_common.h"
#include "io_g6_common.h"
#include "io_sc_common.h"

#define EXCEPTION_VECTOR_SDK1	(*(VoidFn *)(0x27FFD9C))

#define FEATURE_SLOT_GBA			0x00000010
#define FEATURE_SLOT_NDS			0x00000020

#define expansionPakFound BIT(0)
#define extendedMemory BIT(1)
#define ROMinRAM BIT(2)
#define dsDebugRam BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define overlaysCached BIT(6)
#define cacheFlushFlag BIT(7)
#define cardReadFix BIT(8)

#define videoFrameDelayMax 10
#define videoFrameDelayMaxFlaw 1

#include "tonccpy.h"
#include "card.h"
#include "my_fat.h"

extern cardengineArm9* volatile ce9;
struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION_B4DS;

extern void ndsCodeStart(u32* addr);
extern u32 getDtcmBase(void);

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS_SDK1;

// static unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

extern vu32* volatile cardStruct0;

#ifdef GSDD
tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
#else
tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
#endif

static aFile bootNds;
static aFile romFile;
static aFile savFile;
static aFile patchOffsetCacheFile;
static aFile ramDumpFile;
static aFile srParamsFile;
static aFile screenshotFile;
static aFile apFixOverlaysFile;
#ifndef NODSIWARE
static aFile musicsFile;
#endif
static aFile pageFile;
static aFile manualFile;

#ifndef NODSIWARE
static aFile sharedFontFile;

#ifdef FOTO
static bool videoFrameLoading = false;
static u32 videoPos = 0;
static int videoFrameDelay = videoFrameDelayMax;
#else
void updateMusic(void);

static bool musicInited = false;
static bool musicMagicStringChecked = false;

static bool musicPlaying = false;
static int musicBufferNo = 1;
static u32 musicFileCount = 0;
const u16 musicReadLen = 0x2000;
static u32 musicPos = 0x2000;
static u32 musicLoopPos = 0;
static u32 musicPosRev = 0;
static u32 musicPosInFile = 0x0;
static u32 musicFileSize = 0x0;
#endif // FOTO
#endif // NODSIWARE

bool cardReadInProgress = false;
// static int cardReadCount = 0;

extern void setExceptionHandler2();
// fixes mpu protection settings when using a slot2 flashcart
// to allow running games accessing the slot2 like pokemon
extern void slot2MpuFixGbaDldi();

void setDeviceOwner(void) {
	if (__myio_dldi.features & FEATURE_SLOT_GBA) {
		REG_IE &= ~IRQ_CART;
		slot2MpuFixGbaDldi();
	}
	if ((ce9->valueBits & expansionPakFound) || (__myio_dldi.features & FEATURE_SLOT_GBA)) {
		sysSetCartOwner(BUS_OWNER_ARM9);
	}
	if (__myio_dldi.features & FEATURE_SLOT_NDS) {
		sysSetCardOwner(BUS_OWNER_ARM9);
	}
}

void s2RamAccess(bool open) {
	if (__myio_dldi.features & FEATURE_SLOT_NDS) return;

	const u16 s2FlashcardId = ce9->s2FlashcardId;
	if (open) {
		if (s2FlashcardId == 0x334D) {
			_M3_changeMode(M3_MODE_RAM);
		} else if (s2FlashcardId == 0x3647) {
			_G6_SelectOperation(G6_MODE_RAM);
		} else if (s2FlashcardId == 0x4353) {
			_SC_changeMode(SC_MODE_RAM);
		}
	} else {
		if (s2FlashcardId == 0x334D) {
			_M3_changeMode(M3_MODE_MEDIA);
		} else if (s2FlashcardId == 0x3647) {
			_G6_SelectOperation(G6_MODE_MEDIA);
		} else if (s2FlashcardId == 0x4353) {
			_SC_changeMode(SC_MODE_MEDIA);
		}
	}
}

static bool initialized = false;
// static bool region0FixNeeded = false;
static bool igmReset = false;
static bool mpuSet = false;
#ifndef GSDD
static bool isDSiWare = false;

extern void continueCardReadDmaArm9();

static bool IPC_SYNC_hooked = false;
static void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
        //u32* vblankHandler = ce9->irqTable;
        u32* ipcSyncHandler = ce9->irqTable + 16;
        //ce9->intr_vblank_orig_return = *vblankHandler;
        ce9->intr_ipc_orig_return = *ipcSyncHandler;
        //*vblankHandler = ce9->patches->vblankHandlerRef;
        *ipcSyncHandler = (u32)ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}
#endif

extern void slot2MpuFix();
// extern void region0Fix(); // Revert region 0 patch
extern void sdk5MpuFix();
extern void resetMpu();
extern u32 getDtcmBase(void);

extern bool dldiPatchBinary (unsigned char *binData, u32 binSize);

void reset(u32 param, u32 param2) {
	setDeviceOwner();
	#ifndef GSDD
	const u32 resetParams = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	#else
	const u32 resetParams = RESET_PARAM_SDK5;
	#endif
	*(u32*)resetParams = param;

	u32 iUncompressedSize = 0;
	fileRead((char*)&iUncompressedSize, &pageFile, 0x3FFFF0, sizeof(u32));
	const bool downloadedSrl = (param2 == 0x4C525344); // 'DSRL'
	const bool loadNitroSrl = (*(u32*)(resetParams+0xC) > 0);
	#ifndef GSDD
	if (isDSiWare || iUncompressedSize > 0x280000 /*|| param == 0xFFFFFFFF*/ || downloadedSrl || loadNitroSrl) {
		enterCriticalSection();
		if (isDSiWare || iUncompressedSize > 0x280000) {
			sharedAddr[1] = 0x57495344; // 'DSIW'
		}
		if (param != 0xFFFFFFFF && !igmReset && downloadedSrl) {
			*(u32*)resetParams = 0;
			*(u32*)(resetParams+8) = 0x44414F4C; // 'LOAD'
			fileWrite((char*)ndsHeader, &pageFile, 0x2BFE00, 0x160);
			fileWrite((char*)ndsHeader->arm9destination, &pageFile, 0x14000, ndsHeader->arm9binarySize);
			fileWrite((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, ndsHeader->arm7binarySize);
		}
		fileWrite((char*)resetParams, &srParamsFile, 0, 0x10);
		fileWrite((char*)resetParams+0x20, &srParamsFile, 0x10, 0x40);
		if (sharedAddr[1] == 0x57495344 || param == 0xFFFFFFFF) {
		//	sharedAddr[3] = 0x52534554;
		 	sharedAddr[3] = 0x4E445352; // 'RSDN'
			while (1);
		}
	}
	#endif
	// sharedAddr[3] = 0x52534554;
	sharedAddr[3] = 0x4E445352; // 'RSDN'

	//EXCEPTION_VECTOR_SDK1 = 0;

	//volatile u32 arm9_BLANK_RAM = 0;
 	// register int i;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	//toncset((u32*)0x01FF8000, 0, 0x8000);

	cacheFlush();

	if (igmReset) {
		igmReset = false;
	}

	// Clear out ARM9 DMA channels
	/* vu32* dma = &DMA0_SRC;
	vu16* tmr = &TIMER0_DATA;
	for (i = 0; i < 4; i++) {
		dma[2] = 0; //CR
		dma[0] = 0; //SRC
		dma[1] = 0; //DST
		dma += 3;

		tmr[1] = 0; //CR
		tmr[0] = 0; //DATA
		tmr += 2;
	} */
	toncset16((u32*)0x040000B0, 0, 0x40/2); // Clear DMA channels
	toncset16((u32*)0x04000100, 0, 0x10/2); // Clear timers

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	mpuSet = false;
	#ifndef GSDD
	IPC_SYNC_hooked = false;
	#endif

	toncset((char*)((ce9->valueBits & isSdk5) ? 0x02FFFD80 : 0x027FFD80), 0, 0x80);
	toncset((char*)((ce9->valueBits & isSdk5) ? 0x02FFFF80 : 0x027FFF80), 0, 0x80);

	if (param == 0xFFFFFFFF || downloadedSrl || loadNitroSrl) {
		resetMpu();

		REG_DISPSTAT = 0;
		REG_DISPCNT = 0;
		REG_DISPCNT_SUB = 0;
		GFX_STATUS = 0;

		toncset((u16*)0x04000000, 0, 0x56);
		toncset((u16*)0x04001000, 0, 0x56);

		*(vu32*)&VRAM_A_CR = 0x80808080; //ABCD
		*(vu16*)&VRAM_E_CR = 0x8080; //EF
		VRAM_G_CR = 0x80; //G
		*(vu16*)&VRAM_H_CR = 0x8080; //HI

		toncset16(BG_PALETTE, 0, 512); // Clear main and sub palettes
		toncset(VRAM, 0, 0xC0000); // Clear VRAM

		*(vu32*)&VRAM_A_CR = 0; //ABCD
		*(vu16*)&VRAM_E_CR = 0; //EF
		VRAM_G_CR = 0; //G
		*(vu16*)&VRAM_H_CR = 0; //HI

		sdk5MpuFix();
		ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;

		// *(u32*)(0x02000000) = BIT(0) | BIT(1);
		// if (param == 0xFFFFFFFF) {
		// 	*(u32*)(0x02000000) |= BIT(2);
		// }
		//toncset((u32*)0x02000004, 0, 0x3DA000 - 4);
		#ifdef NODSIWARE
		toncset((u32*)0x02000000, 0, 0x3C0000);
		#else
		toncset((u32*)0x02004000, 0, 0x3BC000);
		#endif
		/* #ifdef NODSIWARE
		toncset((u32*)0x023C4000, 0, 0x1C000);
		toncset((u32*)0x02FE4000, 0, 0x19C00-0x4000);
		#else
		toncset((u32*)0x023C4000, 0, 0x16800);
		toncset((u32*)0x02FE4000, 0, 0x1D000-0x4000);
		#endif */
		if (param == 0xFFFFFFFF) {
			*(u32*)(0x02000000) = BIT(0) | BIT(1) | BIT(2);
		}
		#ifndef NODSIWARE
		else {
			*(u32*)(0x02000000) = 0;
		}
		#endif
		*(u32*)0x02000004 = 0x54455352; // 'RSET'

		WRAM_CR = 0; // Set shared ram to ARM9

		fileRead((char*)ndsHeader, &bootNds, 0, 0x170);
		fileRead((char*)ndsHeader->arm9destination, &bootNds, ndsHeader->arm9romOffset, ndsHeader->arm9binarySize);
		fileRead((char*)ndsHeader->arm7destination, &bootNds, ndsHeader->arm7romOffset, ndsHeader->arm7binarySize);

		WRAM_CR = 0x03; // Set shared ram to ARM7
		sharedAddr[1] = 0x48495344; // 'DSIH'

		if (!dldiPatchBinary(ndsHeader->arm9destination, ndsHeader->arm9binarySize)) {
			sharedAddr[1] = 0x57495344;
		}

		toncset((u32*)0x02FFD000, 0, 0x2000);
	} else {
		u32 newArm7binarySize = 0;
		fileRead((char*)&newArm7binarySize, &pageFile, 0x3FFFF4, sizeof(u32));
		fileRead((char*)ndsHeader->arm9destination, &pageFile, 0x14000, iUncompressedSize);
		fileRead((char*)ndsHeader->arm7destination, &pageFile, 0x2C0000, newArm7binarySize);
		/* #ifdef EXTMEM
		fileRead((char*)CHEAT_ENGINE_LOCATION_B4DS, &pageFile, 0x2FE000, 0x2000);
		#else
		fileRead((char*)CHEAT_ENGINE_LOCATION_B4DS-0x400000, &pageFile, 0x2FE000, 0x2000);
		#endif */

		#ifdef NODSIWARE
		tonccpy((u32*)0x02370000, ce9, 0x2800);
		#endif

		resetMpu();
	}

	sharedAddr[0] = 0x544F4F42; // 'BOOT'
	sharedAddr[3] = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	ndsCodeStart(ndsHeader->arm9executeAddress);
}

void prepareScreenshot(void) {
	fileWrite((char*)INGAME_MENU_EXT_LOCATION_B4DS, &pageFile, 0x340000, 0x40000);
}

void saveScreenshot(void) {
	if (igmText->currentScreenshot >= 50) return;

	fileWrite((char*)INGAME_MENU_EXT_LOCATION_B4DS, &screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 0x18046);

	// Skip until next blank slot
	char magic;
	do {
		igmText->currentScreenshot++;
		fileRead(&magic, &screenshotFile, 0x200 + (igmText->currentScreenshot * 0x18400), 1);
	} while(magic == 'B' && igmText->currentScreenshot < 50);

	fileRead((char*)INGAME_MENU_EXT_LOCATION_B4DS, &pageFile, 0x340000, 0x40000);
}

void prepareManual(void) {
	fileWrite((char*)0x027FF200, &pageFile, 0x3FF200, 32 * 24);
}

void readManual(int line) {
	static int currentManualLine = 0;
	static int currentManualOffset = 0;
	char buffer[32];

	// Seek for desired line
	bool firstLoop = true;
	while(currentManualLine != line) {
		if(line > currentManualLine) {
			fileRead(buffer, &manualFile, currentManualOffset, 32);

			for(int i = 0; i < 32; i++) {
				if(buffer[i] == '\n') {
					currentManualOffset += i + 1;
					currentManualLine++;
					break;
				} else if(i == 31) {
					currentManualOffset += i + 1;
					break;
				}
			}
		} else {
			currentManualOffset -= 32;
			fileRead(buffer, &manualFile, currentManualOffset, 32);
			int i = firstLoop ? 30 : 31;
			firstLoop = false;
			for(; i >= 0; i--) {
				if((buffer[i] == '\n') || currentManualOffset + i == -1) {
					currentManualOffset += i + 1;
					currentManualLine--;
					firstLoop = true;
					break;
				}
			}
		}
	}

	toncset((u8*)0x027FF200, ' ', 32 * 24);
	((vu8*)0x027FF200)[32 * 24] = '\0';

	// Read in 24 lines
	u32 tempManualOffset = currentManualOffset;
	bool fullLine = false;
	for(int line = 0; line < 24 && line < igmText->manualMaxLine; line++) {
		fileRead(buffer, &manualFile, tempManualOffset, 32);

		// Fix for exactly 32 char lines
		if(fullLine && buffer[0] == '\n')
			fileRead(buffer, &manualFile, ++tempManualOffset, 32);

		for(int i = 0; i <= 32; i++) {
			if(i == 32 || buffer[i] == '\n' || buffer[i] == '\0') {
				tempManualOffset += i;
				if(buffer[i] == '\n')
					tempManualOffset++;
				fullLine = i == 32;
				tonccpy((char*)0x027FF200 + line * 32, buffer, i);
				break;
			}
		}
	}
}

void restorePreManual(void) {
	fileRead((char*)0x027FF200, &pageFile, 0x3FF200, 32 * 24);
}

void saveMainScreenSetting(void) {
	fileWrite((char*)&ce9->mainScreen, &patchOffsetCacheFile, 0x1FC, sizeof(u32));
}

//---------------------------------------------------------------------------------
/*void myIrqHandlerVBlank(void) {
//---------------------------------------------------------------------------------
	#ifdef DEBUG
	nocashMessage("myIrqHandlerVBlank");
	#endif

	if (sharedAddr[4] == 0x554E454D) {
		while (sharedAddr[4] != 0x54495845);
	}
}*/

#ifdef EXTMEM
#define igmLocation INGAME_MENU_LOCATION_B4DS_EXTMEM
#else
#define igmLocation INGAME_MENU_LOCATION_B4DS
#endif

void writeIgm(u32 dst) {
	fileWrite((char*)igmLocation, &pageFile, dst, 0xA000);
}

void readIgm(u32 src) {
	fileRead((char*)igmLocation, &pageFile, src, 0xA000);
}

void inGameMenu(s32* exRegisters) {
	#ifdef FOTO
	if (videoFrameLoading) {
		while (1);
	}
	#endif

	static bool opening = false;
	static bool opened = false;
	if (opening) { // If an exception error occured while reading in-game menu...
		while (1);
	}

	int oldIME = enterCriticalSection();
	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();

	if (!opened) {
		opening = true;
		writeIgm(0xA000);	// Backup part of game RAM to page file
		readIgm(0);	// Read in-game menu
	}
	opening = false;

	opened = true;

	*(u32*)(igmLocation + IGM_TEXT_SIZE_ALIGNED) = (u32)sharedAddr;
	volatile u32 (*inGameMenu)(s32*, u32, s32*) = (volatile void*)igmLocation + IGM_TEXT_SIZE_ALIGNED + 0x10;
	const u32 res = (*inGameMenu)(&ce9->mainScreen, (u32)ce9, exRegisters);

	opened = false;

	/* if ((sharedAddr[3] == 0x52534554 || sharedAddr[3] == 0x54495845) && isDSiWare) {
		sharedAddr[0] = 0x57495344;
	} */

	writeIgm(0);	// Store in-game menu
	readIgm(0xA000);	// Restore part of game RAM from page file

	if (res == 0x54495845) {
		igmReset = true;
		reset(0xFFFFFFFF, 0);
	} else if (res == 0x52534554) {
		igmReset = true;
		reset(0, 0);
	} else if (res == 0x444D4152) { // RAMD
		#ifdef EXTMEM
		fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x800000);
		// fileWrite((char*)((ce9->valueBits & isSdk5) ? 0x02FE0000 : 0x027E0000), &ramDumpFile, 0x7E0000, 0x20000);
		#else
		fileWrite((char*)0x02000000, &ramDumpFile, 0, 0x400000);
		#endif
		sharedAddr[3] = 0;
	}

	REG_EXMEMCNT = exmemcnt;
	leaveCriticalSection(oldIME);
}

#ifndef GSDD
void cardSave(u32 src, u32 dst, u32 len, const bool write) {
	const u16 exmemcnt = REG_EXMEMCNT;
	// Read/Write save
	setDeviceOwner();

	if ((src % ce9->saveSize)+len > ce9->saveSize) {
		u32 len2 = len;
		u32 len3 = 0;
		while ((src % ce9->saveSize)+len2 > ce9->saveSize) {
			len2--;
			len3++;
		}
		if (write) {
			fileWrite((char*)src, &savFile, (dst % ce9->saveSize), len2);
			fileWrite((char*)src+len2, &savFile, ((dst+len2) % ce9->saveSize), len3);
		} else {
			fileRead((char*)dst, &savFile, (src % ce9->saveSize), len2);
			fileRead((char*)dst+len2, &savFile, ((src+len2) % ce9->saveSize), len3);
		}
	} else if (write) {
		fileWrite((char*)src, &savFile, (dst % ce9->saveSize), len);
	} else {
		fileRead((char*)dst, &savFile, (src % ce9->saveSize), len);
	}

	REG_EXMEMCNT = exmemcnt;
}

bool cardSaveA(u32 src, u32 dst, u32 len) {
	cardSave(src, dst, len, (src >= 0x01FF8000 && src < 0x03000000));
	return true;
}
#endif

//---------------------------------------------------------------------------------
void myIrqHandlerIPC(void) {
//---------------------------------------------------------------------------------
#ifndef GSDD
	switch (IPC_GetSync()) {
		case 0x3:
			continueCardReadDmaArm9();
			break;
		case 0x4:
		switch (sharedAddr[3]) {
			case 0x53415652:
			case 0x53415657: {
				cardSave(*(vu32*)(sharedAddr), *(vu32*)(sharedAddr+1), *(vu32*)(sharedAddr+2), (sharedAddr[3] == 0x53415657));
				sharedAddr[3] = 0;
			} break;
			/* case 0x524F4D52: {
				u32 src = *(vu32*)(sharedAddr);
				u32 dst = *(vu32*)(sharedAddr+1);
				u32 len = *(vu32*)(sharedAddr+2);

				const u16 exmemcnt = REG_EXMEMCNT;
				// Read ROM (redirected from arm7)
				setDeviceOwner();

				fileRead((char*)dst, &romFile, src, len);

				REG_EXMEMCNT = exmemcnt;
				sharedAddr[3] = 0;
			} break; */
		} break;
		#ifndef NODSIWARE
		#ifndef FOTO
		case 0x5:
			updateMusic();
			break;
		#endif // FOTO
		#endif // NODSIWARE
		case 0x6:
			if(ce9->mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(ce9->mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
			break;
		/* case 0x7: {
			ce9->mainScreen++;
			if(ce9->mainScreen > 2)
				ce9->mainScreen = 0;

			if(ce9->mainScreen == 1)
				REG_POWERCNT &= ~POWER_SWAP_LCDS;
			else if(ce9->mainScreen == 2)
				REG_POWERCNT |= POWER_SWAP_LCDS;
		}
			break; */
		case 0x9:
			inGameMenu((s32*)0);
			break;
	}
#endif
}

static void initialize(void) {
	if (initialized) {
		return;
	}

	extern u32 dldiOffsetNew;
	if (dldiOffsetNew < 0x02000000 || dldiOffsetNew >= 0x03000000) {
		extern u8 dldiDriverSize;
		const u16 dldiFileSize = 1 << dldiDriverSize;

		tonccpy((u32*)dldiOffsetNew, (u32*)ce9->dldiOffset, dldiFileSize); // Move to ITCM
		toncset((u32*)ce9->dldiOffset, 0, dldiFileSize);
	}

	if (!FAT_InitFiles(true)) {
		//nocashMessage("!FAT_InitFiles");
		while (1);
	}

	getFileFromCluster(&bootNds, ce9->bootNdsCluster);
	getFileFromCluster(&romFile, ce9->fileCluster);
	getFileFromCluster(&savFile, ce9->saveCluster);
	getFileFromCluster(&patchOffsetCacheFile, ce9->patchOffsetCacheFileCluster);
	#ifndef NODSIWARE
	getFileFromCluster(&musicsFile, ce9->musicCluster);
	#endif

	if (ce9->romFatTableCache != 0) {
		romFile.fatTableCache = (u32*)ce9->romFatTableCache;
		romFile.fatTableSettings |= fatCached;
		if (ce9->romFatTableCompressed) {
			romFile.fatTableSettings |= fatCompressed;
		}
	}
	if (ce9->savFatTableCache != 0) {
		savFile.fatTableCache = (u32*)ce9->savFatTableCache;
		savFile.fatTableSettings |= fatCached;
		if (ce9->savFatTableCompressed) {
			savFile.fatTableSettings |= fatCompressed;
		}
	}
	#ifndef NODSIWARE
	if (ce9->musicFatTableCache != 0) {
		musicsFile.fatTableCache = (u32*)ce9->musicFatTableCache;
		musicsFile.fatTableSettings |= fatCached;
		if (ce9->musicsFatTableCompressed) {
			musicsFile.fatTableSettings |= fatCompressed;
		}
	}
	#endif

	getFileFromCluster(&ramDumpFile, ce9->ramDumpCluster);
	getFileFromCluster(&srParamsFile, ce9->srParamsCluster);
	getFileFromCluster(&screenshotFile, ce9->screenshotCluster);
	getFileFromCluster(&apFixOverlaysFile, ce9->apFixOverlaysCluster);
	getFileFromCluster(&pageFile, ce9->pageFileCluster);
	getFileFromCluster(&manualFile, ce9->manualCluster);
	#ifndef NODSIWARE
	getFileFromCluster(&sharedFontFile, ce9->sharedFontCluster);
	#endif

	// bool cloneboot = (ce9->valueBits & isSdk5) ? *(u16*)0x02FFFC40 == 2 : *(u16*)0x027FFC40 == 2;

	#ifndef GSDD
	if (ce9->valueBits & isSdk5) {
		ndsHeader = (tNDSHeader*)NDS_HEADER_SDK5;
	}

	if (ndsHeader->unitCode > 0) {
		u32 buffer = 0;
		fileRead((char*)&buffer, &romFile, 0x234, sizeof(u32));
		isDSiWare = (buffer == 0x00030004 || buffer == 0x00030005);
	}
	#endif

	initialized = true;
}

void cardReadNormal(u8* dst, u32 src, u32 len) {
	/*nocashMessage("begin\n");

	dbg_hexa(dst);
	nocashMessage("\n");
	dbg_hexa(src);
	nocashMessage("\n");
	dbg_hexa(len);
	nocashMessage("\n");*/

	//nocashMessage("aaaaaaaaaa\n");
	fileRead((char*)dst, (ce9->apFixOverlaysCluster && src >= ce9->overlaysSrc && src < ndsHeader->arm7romOffset) ? &apFixOverlaysFile : &romFile, src, len);

	//nocashMessage("end\n");
}

void cardReadRAM(u8* dst, u32 src, u32 len) {
	// Copy directly
	if (src >= 0 && src < 0x160) {
		u32 newSrc = (u32)ndsHeader+src;
		tonccpy(dst, (u8*)newSrc, len);
	} else {
		u32 newSrc = ce9->romLocation+src;
		if (ndsHeader->unitCode == 3 && src >= ce9->arm9iromOffset) {
			newSrc -= ce9->arm9ibinarySize;
		}
		tonccpy(dst, (u8*)newSrc, len);
	}
}

bool isNotTcm(u32 address, u32 len) {
    u32 base = (getDtcmBase()>>12) << 12;
    return    // test data not in ITCM
    address > 0x02000000
    // test data not in DTCM
    && (address < base || address> base+0x4000)
    && (address+len < base || address+len> base+0x4000);
}

#ifndef GSDD
static int counter=0;
int cardReadPDash(u32* cacheStruct, u32 src, u8* dst, u32 len) {
	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

    cardStruct[0] = src;
    cardStruct[1] = (vu32)dst;
    cardStruct[2] = len;

	cardRead(cacheStruct, dst, src, len);

    counter++;
	return counter;
}
#else
void setBL(int arg1, int arg2) {
	*(u32*)arg1 = (((u32)(arg2 - arg1 - 8) >> 2) & 0xFFFFFF) | 0xEB000000;
}

void gsddFix(void) {
	const u32 gsddOverlayOffset = *(u32*)0x02FFF000;
	extern u32 gsdd_fix2;

	// Patch overlay 335 (DSProtect v2.01s)
	if (*(u32*)gsddOverlayOffset == 0xE163F679)
	{
		setBL(gsddOverlayOffset+0x115C, gsdd_fix2);
	} else // Patch overlay 334 (DSProtect v2.01)
	if (*(u32*)gsddOverlayOffset == 0xE544AA7C)
	{
		VoidFn decrypt = (VoidFn)(gsddOverlayOffset+0x1210);
		(*decrypt)();

		setBL(gsddOverlayOffset+0xA4C, gsdd_fix2);
	}
}

void gsddFix2(u32* code) {
	// Patch overlay 335 (DSProtect v2.01s) (Part 2)
	if (code[0] == 0xE92D4FF8 && code[1] == 0xE24DDF92 && code[2] == 0xE3A0BA07) {
		code[0x224/sizeof(u32)] += 0xE0000000; // beq -> b
		code[0x278/sizeof(u32)] = 0xE3A01000; // mov r1, #0
		code[0x27C/sizeof(u32)] = 0xE3A00000; // mov r0, #0
		code[0x284/sizeof(u32)] = 0xE1A00000; // nop
		code[0x288/sizeof(u32)] = 0xE1A00000; // nop
	} else // Patch overlay 334 (DSProtect v2.01) (Part 2)
	if (code[0] == 0xE92D4FF0 && code[1] == 0xE24DDF93 && code[2] == 0xE58D000C && code[3] == 0xE3A0BA07) {
		code[0x228/sizeof(u32)] += 0xE0000000; // beq -> b
		code[0x2C4/sizeof(u32)] = 0xE3A01000; // mov r1, #0
		code[0x2C8/sizeof(u32)] = 0xE3A00000; // mov r0, #0
		code[0x2D0/sizeof(u32)] = 0xE1A00000; // nop
		code[0x2D4/sizeof(u32)] = 0xE1A00000; // nop
	}
}

bool cardSave(void) {
	vu32* cardStruct = (vu32*)(ce9->cardStruct1);

	u32 src = cardStruct[0];
	u32 dst = cardStruct[1];
	u32 len = cardStruct[2];

	// volatile void (*finish)(void*) = (volatile void*)(cardStruct[ce9->cardSaveCmdPos+1]);
	// void *const arg = (void*)(cardStruct[ce9->cardSaveCmdPos+2]);
	volatile void (*finish)(void*) = (volatile void*)(cardStruct[-3]);
	void *const arg = (void*)(cardStruct[-2]);

	if (len == 0) {
		if (finish) {
			(*finish)(arg);
		}
		return false;
	}

	bool res = false;

	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();

	if (cardStruct[7] != 2)
	// if (ce9->cardSaveCmdPos ? (cardStruct[ce9->cardSaveCmdPos] != 2) : (dst >= 0x01FF8000))
	{
		// Read save
		if ((src % ce9->saveSize)+len > ce9->saveSize) {
			u32 len2 = len;
			u32 len3 = 0;
			while ((src % ce9->saveSize)+len2 > ce9->saveSize) {
				len2--;
				len3++;
			}
			fileRead((char*)dst, &savFile, (src % ce9->saveSize), len2);
			res = fileRead((char*)dst+len2, &savFile, ((src+len2) % ce9->saveSize), len3);
		} else {
			res = fileRead((char*)dst, &savFile, (src % ce9->saveSize), len);
		}
	} else {
		// Write save
		if ((dst % ce9->saveSize)+len > ce9->saveSize) {
			u32 len2 = len;
			u32 len3 = 0;
			while ((dst % ce9->saveSize)+len2 > ce9->saveSize) {
				len2--;
				len3++;
			}
			fileWrite((char*)src, &savFile, (dst % ce9->saveSize), len2);
			res = fileWrite((char*)src+len2, &savFile, ((dst+len2) % ce9->saveSize), len3);
		} else {
			res = fileWrite((char*)src, &savFile, (dst % ce9->saveSize), len);
		}
	}

	REG_EXMEMCNT = exmemcnt;
	if (finish) {
		(*finish)(arg);
	}
	return res;
}
#endif

void cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	if (!mpuSet) {
		/* if (region0FixNeeded) {
			region0Fix();
		} */
		if (ce9->valueBits & enableExceptionHandler) {
			setExceptionHandler2();
		}
		mpuSet = true;
	}

	const u16 exmemcnt = REG_EXMEMCNT;
	cardReadInProgress = true;

	setDeviceOwner();
	initialize();

	// cardReadCount++;

	#ifdef GSDD
	u32 src = src0;
	u8* dst = dst0;
	u32 len = len0;
	#else
	enableIPC_SYNC();

	vu32* cardStruct = (vu32*)(ce9->cardStruct0);

	u32 src = ((ce9->valueBits & isSdk5) ? src0 : cardStruct[0]);
	u8* dst = ((ce9->valueBits & isSdk5) ? dst0 : (u8*)(cardStruct[1]));
	u32 len = ((ce9->valueBits & isSdk5) ? len0 : cardStruct[2]);
	#endif

	// Simulate ROM mirroring
	while (src >= ce9->romPaddingSize) {
		src -= ce9->romPaddingSize;
	}

	if ((ce9->valueBits & cardReadFix) && src < 0x8000) {
		// Fix reads below 0x8000
		src = 0x8000 + (src & 0x1FF);
	}

	if ((ce9->valueBits & ROMinRAM) || (ce9->romPartSize > 0 && src >= ce9->romPartSrc && src < ce9->romPartSrc+ce9->romPartSize)) {
		cardReadRAM(dst, src, len);
	} else {
		cardReadNormal(dst, src, len);
		#ifndef GSDD
		if (ce9->valueBits & cacheFlushFlag) {
			cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
		}
		#endif
	}

	cardReadInProgress = false;
	REG_EXMEMCNT = exmemcnt;
}

bool nandRead(void* memory,void* flash,u32 len,u32 dma) {
	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();
	fileRead(memory, &savFile, (u32)flash, len);
	REG_EXMEMCNT = exmemcnt;
    return true;
}

bool nandWrite(void* memory,void* flash,u32 len,u32 dma) {
	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();
	fileWrite(memory, &savFile, (u32)flash, len);
	REG_EXMEMCNT = exmemcnt;
	return true;
}

#ifndef NODSIWARE
void ndmaCopy(int ndmaSlot, const void* src, void* dst, u32 len) {
	tonccpy(dst, src, len);
}

static bool sharedFontOpened = false;
static bool dsiSaveInited = false;
static bool dsiSaveExists = false;
static u32 dsiSavePerms = 0;
static s32 dsiSaveSeekPos = 0;
static s32 dsiSaveSize = 0;
static s32 dsiSaveResultCode = 0;

typedef struct dsiSaveInfo
{
	u32 attributes;
	u32 ctime[6];
	u32 mtime[6];
	u32 atime[6];
	u32 filesize;
	u32 id;
}
dsiSaveInfo;

static void dsiSaveInit(void) {
	if (dsiSaveInited) {
		return;
	}
	u32 existByte = 0;

	int oldIME = enterCriticalSection();
	const u16 exmemcnt = REG_EXMEMCNT;
	cardReadInProgress = true;
	setDeviceOwner();
	fileRead((char*)&dsiSaveSize, &savFile, ce9->saveSize-4, 4);
	fileRead((char*)&existByte, &savFile, ce9->saveSize-8, 4);
	cardReadInProgress = false;
	REG_EXMEMCNT = exmemcnt;
	leaveCriticalSection(oldIME);

	dsiSaveExists = (existByte != 0);
	dsiSaveInited = true;
}

u32 dsiSaveCheckExists(void) {
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		return 1;
	}

	dsiSaveInit();

	return dsiSaveExists ? 0 : 1;
}

u32 dsiSaveGetResultCode(const char* path) {
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		return 0xE;
	}

	dsiSaveInit();

	if (strcmp(path, "data") == 0) // Specific to EnjoyUp-developed games
	{
		return dsiSaveExists ? 8 : 0xE;
	}
	return dsiSaveResultCode;
}

bool dsiSaveCreate(const char* path, u32 permit) {
	dsiSaveSeekPos = 0;
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 0xE;
		return false;
	}

	dsiSaveInit();
	//if ((!dsiSaveExists && permit == 1) || (dsiSaveExists && permit == 2)) {
	//	return false;
	//}

	if (strcmp(path, "data") == 0) // Specific to EnjoyUp-developed games
	{
		return !dsiSaveExists;
	} else
	if (!dsiSaveExists) {
		u32 existByte = 1;

		int oldIME = enterCriticalSection();
		const u16 exmemcnt = REG_EXMEMCNT;
		cardReadInProgress = true;
		setDeviceOwner();
		fileWrite((char*)&existByte, &savFile, ce9->saveSize-8, 4);
		cardReadInProgress = false;
		REG_EXMEMCNT = exmemcnt;
		leaveCriticalSection(oldIME);

		dsiSaveExists = true;
		dsiSaveResultCode = 0;
		return true;
	}
	dsiSaveResultCode = 8;
	return false;
}

bool dsiSaveDelete(const char* path) {
	dsiSaveSeekPos = 0;
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 0xE;
		return false;
	}

	if (dsiSaveExists) {
		dsiSaveSize = 0;

		int oldIME = enterCriticalSection();
		const u16 exmemcnt = REG_EXMEMCNT;
		cardReadInProgress = true;
		setDeviceOwner();
		fileWrite((char*)&dsiSaveSize, &savFile, ce9->saveSize-4, 4);
		fileWrite((char*)&dsiSaveSize, &savFile, ce9->saveSize-8, 4);
		cardReadInProgress = false;
		REG_EXMEMCNT = exmemcnt;
		leaveCriticalSection(oldIME);

		dsiSaveExists = false;
		dsiSaveResultCode = 0;
		return true;
	}
	dsiSaveResultCode = 8;
	return false;
}

bool dsiSaveGetInfo(const char* path, dsiSaveInfo* info) {
	toncset(info, 0, sizeof(dsiSaveInfo));
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 0xE;
		return false;
	}

	dsiSaveInit();
	dsiSaveResultCode = 0;

	if (strcmp(path, "dataPub:") == 0 || strcmp(path, "dataPub:/") == 0
	 || strcmp(path, "dataPrv:") == 0 || strcmp(path, "dataPrv:/") == 0)
	{
		return true;
	} else if (!dsiSaveExists) {
		dsiSaveResultCode = 0xB;
		return false;
	}

	info->filesize = dsiSaveSize;
	return true;
}

u32 dsiSaveSetLength(void* ctx, s32 len) {
	dsiSaveSeekPos = 0;
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 1;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return 1;
	}

	if (len > ce9->saveSize-0x200) {
		len = ce9->saveSize-0x200;
	}

	dsiSaveSize = len;

	int oldIME = enterCriticalSection();
	const u16 exmemcnt = REG_EXMEMCNT;
	cardReadInProgress = true;
	setDeviceOwner();
	bool res = fileWrite((char*)&dsiSaveSize, &savFile, ce9->saveSize-4, 4);
	dsiSaveResultCode = res ? 0 : 1;
	toncset32(ctx+0x14, dsiSaveResultCode, 1);
	cardReadInProgress = false;
	REG_EXMEMCNT = exmemcnt;
	leaveCriticalSection(oldIME);

	return dsiSaveResultCode;
}

bool dsiSaveOpen(void* ctx, const char* path, u32 mode) {
	dsiSaveSeekPos = 0;
	if (strcmp(path, "nand:/<sharedFont>") == 0) {
		if (sharedFontFile.firstCluster == CLUSTER_FREE || sharedFontFile.firstCluster == CLUSTER_EOF) {
			dsiSaveResultCode = 0xE;
			toncset32(ctx+0x14, dsiSaveResultCode, 1);
			return false;
		}
		dsiSaveResultCode = 0;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		sharedFontOpened = true;
		return true;
	}
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 0xE;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return false;
	}

	dsiSaveInit();
	dsiSaveResultCode = dsiSaveExists ? 0 : 0xB;
	toncset32(ctx+0x14, dsiSaveResultCode, 1);

	dsiSavePerms = mode;
	return dsiSaveExists;
}

bool dsiSaveClose(void* ctx) {
	dsiSaveSeekPos = 0;
	if (sharedFontOpened) {
		sharedFontOpened = false;
		if (sharedFontFile.firstCluster == CLUSTER_FREE || sharedFontFile.firstCluster == CLUSTER_EOF) {
			dsiSaveResultCode = 0xE;
			toncset32(ctx+0x14, dsiSaveResultCode, 1);
			return false;
		}
		dsiSaveResultCode = 0;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return true;
	}
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 0xE;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return false;
	}
	//toncset(ctx, 0, 0x80);
	dsiSaveResultCode = dsiSaveExists ? 0 : 0xB;
	toncset32(ctx+0x14, dsiSaveResultCode, 1);
	return dsiSaveExists;
}

u32 dsiSaveGetLength(void* ctx) {
	dsiSaveSeekPos = 0;
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		return 0;
	}

	if (dsiSaveExists) {
		dsiSaveResultCode = 0;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
	}
	return dsiSaveSize;
}

u32 dsiSaveGetPosition(void* ctx) {
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		return 0;
	}

	return dsiSaveSeekPos;
}

bool dsiSaveSeek(void* ctx, s32 pos, u32 mode) {
	if (sharedFontOpened) {
		if (sharedFontFile.firstCluster == CLUSTER_FREE || sharedFontFile.firstCluster == CLUSTER_EOF) {
			dsiSaveResultCode = 0xE;
			return false;
		}
		dsiSaveSeekPos = pos;
		dsiSaveResultCode = 0;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return true;
	}
	if (savFile.firstCluster == CLUSTER_FREE || savFile.firstCluster == CLUSTER_EOF) {
		dsiSaveResultCode = 0xE;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return false;
	}
	if (!dsiSaveExists) {
		dsiSaveResultCode = 1;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return false;
	}
	dsiSaveSeekPos = pos;
	dsiSaveResultCode = 0;
	toncset32(ctx+0x14, dsiSaveResultCode, 1);
	return true;
}

s32 dsiSaveRead(void* ctx, void* dst, s32 len) {
	if (!sharedFontOpened) {
		if (dsiSavePerms == 2 || !dsiSaveExists) {
			dsiSaveResultCode = 1;
			toncset32(ctx+0x14, dsiSaveResultCode, 1);
			return -1; // Return if only write perms are set
		}

		if (dsiSaveSize == 0) {
			dsiSaveResultCode = 1;
			toncset32(ctx+0x14, dsiSaveResultCode, 1);
			return 0;
		}

		while (dsiSaveSeekPos+len > dsiSaveSize) {
			len--;
		}

		if (len == 0) {
			dsiSaveResultCode = 1;
			toncset32(ctx+0x14, dsiSaveResultCode, 1);
			return 0;
		}
	}

	int oldIME = enterCriticalSection();
	const u16 exmemcnt = REG_EXMEMCNT;
	cardReadInProgress = true;
	setDeviceOwner();
	bool res = false;
	if ((__myio_dldi.features & FEATURE_SLOT_GBA) && (u32)dst >= 0x08000000 && (u32)dst < 0x0A000000) {
		s32 bufLen = len;
		u32 dstAdd = 0;
		while (1) {
			u32 readLen = (bufLen > 0x600) ? 0x600 : len;

			res = fileRead((char*)0x027FF200, sharedFontOpened ? &sharedFontFile : &savFile, dsiSaveSeekPos+dstAdd, readLen);
			if (!res) {
				break;
			}
			tonccpy((char*)dst+dstAdd, (char*)0x027FF200, readLen);

			bufLen -= 0x600;
			dstAdd += 0x600;

			if (bufLen <= 0) {
				break;
			}
		}
	} else {
		res = fileRead(dst, sharedFontOpened ? &sharedFontFile : &savFile, dsiSaveSeekPos, len);
	}
	dsiSaveResultCode = res ? 0 : 1;
	toncset32(ctx+0x14, dsiSaveResultCode, 1);
	cardReadInProgress = false;
	REG_EXMEMCNT = exmemcnt;
	leaveCriticalSection(oldIME);
	if (res) {
		dsiSaveSeekPos += len;
		return len;
	}
	return -1;
}

s32 dsiSaveWrite(void* ctx, void* src, s32 len) {
	if (dsiSavePerms == 1 || !dsiSaveExists) {
		dsiSaveResultCode = 1;
		toncset32(ctx+0x14, dsiSaveResultCode, 1);
		return -1; // Return if only read perms are set
	}

	if (dsiSaveSeekPos >= ce9->saveSize-0x200) {
		return 0;
	}

	while (dsiSaveSeekPos+len > ce9->saveSize-0x200) {
		// Do not overwrite exist flag and save file size
		len--;
	}

	int oldIME = enterCriticalSection();
	const u16 exmemcnt = REG_EXMEMCNT;
	cardReadInProgress = true;
	setDeviceOwner();
	bool res = false;
	if ((__myio_dldi.features & FEATURE_SLOT_GBA) && (u32)src >= 0x08000000 && (u32)src < 0x0A000000) {
		s32 bufLen = len;
		u32 srcAdd = 0;
		while (1) {
			u32 readLen = (bufLen > 0x600) ? 0x600 : len;

			tonccpy((char*)0x027FF200, (char*)src+srcAdd, readLen);
			res = fileWrite((char*)0x027FF200, &savFile, dsiSaveSeekPos+srcAdd, readLen);
			if (!res) {
				break;
			}

			bufLen -= 0x600;
			srcAdd += 0x600;

			if (bufLen <= 0) {
				break;
			}
		}
	} else {
		res = fileWrite(src, &savFile, dsiSaveSeekPos, len);
	}
	dsiSaveResultCode = res ? 0 : 1;
	toncset32(ctx+0x14, dsiSaveResultCode, 1);
	cardReadInProgress = false;
	REG_EXMEMCNT = exmemcnt;
	leaveCriticalSection(oldIME);
	if (res) {
		if (dsiSaveSize < dsiSaveSeekPos+len) {
			dsiSaveSize = dsiSaveSeekPos+len;

			int oldIME = enterCriticalSection();
			u16 exmemcnt = REG_EXMEMCNT;
			setDeviceOwner();
			cardReadInProgress = true;
			fileWrite((char*)&dsiSaveSize, &savFile, ce9->saveSize-4, 4);
			cardReadInProgress = false;
			REG_EXMEMCNT = exmemcnt;
			leaveCriticalSection(oldIME);
		}
		dsiSaveSeekPos += len;
		return len;
	}
	return -1;
}
#endif


/*void reset(u32 param) {
	setDeviceOwner();
	u32 resetParams = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	if (ce9->valueBits & softResetMb) {
		*(u32*)resetParams = 0;
		*(u32*)(resetParams+8) = 0x44414F4C; // 'LOAD'
		fileWrite(ndsHeader, &pageFile, 0x2BFE00, 0x160);
		fileWrite((char*)ndsHeader->arm9destination, &pageFile, 0x14000, ndsHeader->arm9binarySize);
		fileWrite((char*)0x022C0000, &pageFile, 0x2C0000, ndsHeader->arm7binarySize);
	} else {
		*(u32*)resetParams = param;
	}
	fileWrite((char*)resetParams, srParamsFile, 0, 0x10);
	sharedAddr[3] = 0x52534554;
	while (1);
}*/

#ifndef NODSIWARE
void musicInit(void) {
	#ifndef FOTO
	if (musicInited || musicMagicStringChecked || ce9->musicCluster == 0) {
		return;
	}

	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();

	musicMagicStringChecked = true;
	u32 magic = 0;
	fileRead((char*)&magic, &musicsFile, 0, 4);
	if (magic != 0x4B43502E) {
		REG_EXMEMCNT = exmemcnt;
		return;
	}

	fileRead((char*)&musicFileCount, &musicsFile, 4, 4);

	REG_EXMEMCNT = exmemcnt;

	musicInited = true;
	#endif
}

void updateMusic(void) {
	#ifndef FOTO
	if (sharedAddr[2] == 0x5953554D && !cardReadInProgress) { // 'MUSY'
		const u16 exmemcnt = REG_EXMEMCNT;
		setDeviceOwner();

		const u16 currentLen = (musicPosRev > musicReadLen) ? musicReadLen : musicPosRev;
		fileRead((char*)(ce9->musicBuffer+(musicBufferNo*musicReadLen)), &musicsFile, musicPosInFile + musicPos, currentLen);
		musicPos += musicReadLen;
		if (musicPos > musicFileSize) {
			musicPos = musicLoopPos;
			u16 lastLenTemp = musicPosRev;
			u16 lastLen = 0;
			while (lastLenTemp < 0x2000) {
				lastLenTemp++;
				lastLen++;
			}
			fileRead((char*)(ce9->musicBuffer+(musicBufferNo*musicReadLen)+musicPosRev), &musicsFile, musicPosInFile + musicPos, lastLen);
			musicPos += lastLen;
			musicPosRev = musicFileSize - musicLoopPos - lastLen;
		} else {
			musicPosRev -= musicReadLen;
			if (musicPosRev == 0) {
				musicPos = musicLoopPos;
				musicPosRev = musicFileSize - musicLoopPos;
			}
		}

		musicBufferNo++;
		if (musicBufferNo == 2) musicBufferNo = 0;

		sharedAddr[2] = 0;
		REG_EXMEMCNT = exmemcnt;
	}
	#endif
}

#ifdef FOTO
void musicPlay(u32 r0, u32 r1)
#else
void musicPlay(int id)
#endif
{
	#ifdef FOTO
	const char* romTid = getRomTid(ndsHeader);
	if (strncmp(romTid, "DSY", 3) == 0 || strncmp(romTid, "KSY", 3) == 0) {
		// This is System FLAW
		if (ce9->musicCluster == 0 || ce9->musicsSize < 0x18004*2) {
			return;
		}

		static u32 r1Bak = 0;

		videoFrameDelay++;
		if (videoFrameDelay > videoFrameDelayMaxFlaw) videoFrameDelay = 0;

		videoFrameLoading = true;
		const u16 exmemcnt = REG_EXMEMCNT;
		setDeviceOwner();

		// r1 is the frame offset
		if (videoFrameDelay) {
			tonccpy((u16*)r1, (u16*)r1Bak, 0x18000);
		} else {
			fileRead((char*)r1, &musicsFile, videoPos, (256*192)*2);

			videoPos += 0x18004; // Seek to next frame
			if (videoPos >= ce9->musicsSize) videoPos = 0; // Loop back to first frame after last frame has been reached
		}

		r1Bak = r1;

		REG_EXMEMCNT = exmemcnt;
		videoFrameLoading = false;

		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
		return;
	}

	// Otherwise, this is Foto Showdown
	if (ce9->musicCluster == 0 || ce9->musicsSize < 0x18000) {
		toncset16((u16*)0x06008000, 0x8000, 256*192);
		return;
	}

	videoFrameLoading = true;
	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();

	fileRead((char*)0x06008000, &musicsFile, videoPos, (256*192)*2);

	REG_EXMEMCNT = exmemcnt;
	videoFrameLoading = false;
	#else
	musicInit();

	if (musicPlaying) {
		sharedAddr[2] = 0x5353554D; // 'MUSS'
		while (sharedAddr[2] == 0x5353554D);
		musicPlaying = false;
	}

	if (musicInited && id < musicFileCount) {
		musicBufferNo = 1;

		const u16 exmemcnt = REG_EXMEMCNT;
		setDeviceOwner();

		fileRead((char*)&musicPosInFile, &musicsFile, 0x10+(0x10*id), 4);
		fileRead((char*)&musicFileSize, &musicsFile, 0x14+(0x10*id), 4);
		fileRead((char*)&musicLoopPos, &musicsFile, 0x18+(0x10*id), 4);

		musicPos = musicReadLen;
		musicPosRev = musicFileSize - musicReadLen;

		fileRead((char*)ce9->musicBuffer, &musicsFile, musicPosInFile, musicReadLen);

		REG_EXMEMCNT = exmemcnt;
		sharedAddr[2] = 0x5053554D; // 'MUSP'
		while (sharedAddr[2] == 0x5053554D);
		musicPlaying = true;
	}
	#endif
}

#ifdef FOTO
u32 musicStopEffect(void)
#else
void musicStopEffect(int id)
#endif
{
	#ifdef FOTO
	const u32 ret = (ndsHeader->gameCode[3] == 'J') ? 0x020EEE68 : 0x020EE108;

	videoFrameDelay++;
	if (videoFrameDelay > videoFrameDelayMax) videoFrameDelay = 0;

	if (videoFrameDelay || ce9->musicCluster == 0 || ce9->musicsSize < 0x18004*2) {
		return ret;
	}

	videoFrameLoading = true;
	const u16 exmemcnt = REG_EXMEMCNT;
	setDeviceOwner();

	videoPos += 0x18004; // Seek to next frame
	if (videoPos >= ce9->musicsSize) videoPos = 0; // Loop back to first frame after last frame has been reached

	fileRead((char*)0x06008000, &musicsFile, videoPos, (256*192)*2);

	REG_EXMEMCNT = exmemcnt;
	videoFrameLoading = false;
	return ret;
	#else
	if (!musicPlaying) {
		return;
	}
	sharedAddr[2] = 0x5353554D; // 'MUSS'
	while (sharedAddr[2] == 0x5353554D);
	musicPlaying = false;
	#endif
}

void rumble(u32 arg) {
	sharedAddr[0] = ce9->rumbleFrames[0];
	sharedAddr[1] = ce9->rumbleForce[0];
	sharedAddr[3] = 0x424D5552; // 'RUMB'
}

void rumble2(u32 arg) {
	sharedAddr[0] = ce9->rumbleFrames[1];
	sharedAddr[1] = ce9->rumbleForce[1];
	sharedAddr[3] = 0x424D5552; // 'RUMB'
}
#endif

u32 myIrqEnable(u32 irq) {
	#ifdef GSDD
	return irq;
	#else
	int oldIME = enterCriticalSection();
	u16 exmemcnt = REG_EXMEMCNT;

	#ifdef DEBUG
	nocashMessage("myIrqEnable\n");
	#endif

	setDeviceOwner();
	initialize();

	if (isDSiWare && (ce9->valueBits & expansionPakFound)) {
		if (__myio_dldi.features & FEATURE_SLOT_NDS) {
			slot2MpuFix();
		}
		sysSetCartOwner (BUS_OWNER_ARM9);
		exmemcnt = REG_EXMEMCNT;
	}

	if (ce9->valueBits & enableExceptionHandler) {
		setExceptionHandler2();
	}

	/* if (unpatchedFuncs->mpuDataOffset) {
		region0FixNeeded = unpatchedFuncs->mpuInitRegionOldData == 0x4000033;
	} */

	hookIPC_SYNC();

	REG_EXMEMCNT = exmemcnt;

	u32 irq_before = REG_IE | IRQ_IPC_SYNC;
	irq |= IRQ_IPC_SYNC;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
	return irq_before;
	#endif
}
