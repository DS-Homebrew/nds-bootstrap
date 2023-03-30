//#include <stddef.h>
#include <nds/system.h>
#include "nds_header.h"
#include "module_params.h"
#include "cardengine_header_arm7.h"
#include "cardengine_header_arm9.h"
#include "patch.h"
#include "common.h"
#include "locations.h"
#include "tonccpy.h"
#include "loading_screen.h"
#include "debug_file.h"

extern bool useSharedFont;
extern u8 valueBits3;
#define twlSharedFont (valueBits3 & BIT(3))
#define chnSharedFont (valueBits3 & BIT(4))
#define korSharedFont (valueBits3 & BIT(5))

#define nopT 0x46C0

static inline void doubleNopT(u32 addr) {
	*(u16*)(addr)   = nopT;
	*(u16*)(addr+2) = nopT;
}

static void patchMsg16(u32 addr, const char* msg) {
	u8* dst = (u8*)addr;
	for (int i = 0; i <= strlen(msg); i++) {
		dst[i*2] = msg[i];
	}
}

void patchDSiModeToDSMode(cardengineArm9* ce9, const tNDSHeader* ndsHeader) {
	extern bool ce9Alt;
	extern bool expansionPakFound;
	extern u16 s2FlashcardId;
	extern u32 donorFileTwlCluster;	// SDK5 (TWL)
	extern u32 fatTableAddr;
	const char* romTid = getRomTid(ndsHeader);
	// const char* dataPub = "dataPub:";
	const char* dataPrv = "dataPrv:";
	const char* dsiRequiredMsg = "A Nintendo DSi is required to use this feature.";
	extern u32 relocateBssPart(const tNDSHeader* ndsHeader, u32 bssEnd, u32 bssPartStart, u32 bssPartEnd, u32 newPartStart);
	extern void patchHiHeapDSiWareThumb(u32 addr, u32 newCodeAddr, u32 heapEnd);
	extern void patchInitDSiWare(u32 addr, u32 heapEnd);
	extern void patchVolumeGetDSiWare(u32 addr);
	extern void patchUserSettingsReadDSiWare(u32 addr);
	extern void patchTwlFontLoad(u32 heapAllocAddr, u32 newCodeAddr);

	const u32 heapEndRetail = ce9Alt ? 0x023E0000 : ((fatTableAddr < 0x023C0000 || fatTableAddr >= CARDENGINE_ARM9_LOCATION_DLDI) ? CARDENGINE_ARM9_LOCATION_DLDI : fatTableAddr);
	const u32 heapEnd = extendedMemory2 ? (((u32)ndsHeader->arm9destination >= 0x02004000) ? CARDENGINE_ARM9_LOCATION_DLDI_EXTMEM : 0x02700000) : heapEndRetail;
	const u32 heapEnd8MBHack = extendedMemory2 ? heapEnd : heapEndRetail+0x400000; // extendedMemory2 ? #0x27B0000 : #0x27E0000 (mirrors to 0x23E0000 on retail DS units)
	const u32 heapEndExceed = extendedMemory2 ? heapEnd+0x800000 : heapEndRetail+0xC00000; // extendedMemory2 ? #0x2FB0000 (mirrors to 0x27B0000 on debug DS units) : #0x2FE0000 (mirrors to 0x23E0000 on retail DS units)
	const bool debugOrMep = (extendedMemory2 || expansionPakFound);
	const bool largeS2RAM = (expansionPakFound && (s2FlashcardId != 0)); // 16MB or more
	if (donorFileTwlCluster == CLUSTER_FREE) {
		return;
	}

	const u32* dsiSaveCheckExists = ce9->patches->dsiSaveCheckExists;
	const u32* dsiSaveGetResultCode = ce9->patches->dsiSaveGetResultCode;
	const u32* dsiSaveCreate = ce9->patches->dsiSaveCreate;
	const u32* dsiSaveDelete = ce9->patches->dsiSaveDelete;
	const u32* dsiSaveGetInfo = ce9->patches->dsiSaveGetInfo;
	const u32* dsiSaveSetLength = ce9->patches->dsiSaveSetLength;
	const u32* dsiSaveOpen = ce9->patches->dsiSaveOpen;
	const u32* dsiSaveOpenR = ce9->patches->dsiSaveOpenR;
	const u32* dsiSaveClose = ce9->patches->dsiSaveClose;
	const u32* dsiSaveGetLength = ce9->patches->dsiSaveGetLength;
	const u32* dsiSaveGetPosition = ce9->patches->dsiSaveGetPosition;
	const u32* dsiSaveSeek = ce9->patches->dsiSaveSeek;
	const u32* dsiSaveRead = ce9->patches->dsiSaveRead;
	const u32* dsiSaveWrite = ce9->patches->dsiSaveWrite;

	const bool twlFontFound = twlSharedFont;
	//const bool chnFontFound = chnSharedFont;
	//const bool korFontFound = korSharedFont;

	// Patch DSi-Exclusives to run in DS mode

	// Nintendo DSi XL Demo Video (USA)
	// Requires 8MB of RAM
	if (strcmp(romTid, "DMEE") == 0 && extendedMemory2) {
		// *(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x02008DD8 = 0xE1A00000; // nop
		*(u32*)0x02008EF4 = 0xE1A00000; // nop
		*(u32*)0x02008F08 = 0xE1A00000; // nop
		*(u32*)0x0200BC58 = 0xE1A00000; // nop
		patchInitDSiWare(0x0200EF68, heapEnd);
		*(u32*)0x020107FC = 0xE1A00000; // nop
		*(u32*)0x02010800 = 0xE1A00000; // nop
		*(u32*)0x02010804 = 0xE1A00000; // nop
		*(u32*)0x02010808 = 0xE1A00000; // nop
	}

	// Nintendo DSi XL Demo Video: Volume 2 (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "DMDE") == 0 && extendedMemory2) {
		// *(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x02008E04 = 0xE1A00000; // nop
		*(u32*)0x02008F14 = 0xE1A00000; // nop
		*(u32*)0x02008F28 = 0xE1A00000; // nop
		*(u32*)0x0200BB3C = 0xE1A00000; // nop
		patchInitDSiWare(0x0200ECF4, heapEnd);
	}

	// NOE Movie Player: Volume 1 (Europe)
	else if (strcmp(romTid, "DMPP") == 0) {
		*(u32*)0x02009ED8 = 0xE1A00000; // nop
		*(u32*)0x0200D0D8 = 0xE1A00000; // nop
		patchInitDSiWare(0x020105A4, heapEnd);
	}

	// Picture Perfect Hair Salon (USA)
	// Hair Salon (Europe/Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "DHSE") == 0 || strcmp(romTid, "DHSV") == 0) && extendedMemory2) {
		// *(u32*)0x02004B9C = 0x0200002F;
		/* if (!extendedMemory2) {
			*(u32*)0x020050D4 = 0xE3A00901; // mov r0, #0x4000
			*(u32*)0x02024550 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02024554 = 0xE12FFF1E; // bx lr
			*(u32*)0x020245B4 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020245B8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202480C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02024810 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202483C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02024840 = 0xE12FFF1E; // bx lr
			*(u32*)0x0203F1F4 = 0xE1A00000; // nop
			*(u32*)0x0203F590 = 0xE1A00000; // nop
		} */
		*(u32*)0x02005108 = 0xE1A00000; // nop
		*(u32*)0x0200517C = 0xE1A00000; // nop
		*(u32*)0x02005190 = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x0200F2B0 = 0xE1A00000; // nop
		*(u32*)0x0200F3DC = 0xE1A00000; // nop
		*(u32*)0x0200F3F0 = 0xE1A00000; // nop
		*(u32*)0x02012840 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019980, heapEnd);
		*(u32*)0x02019CF0 = *(u32*)0x02004FD0;
		*(u32*)0x02020B10 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020B14 = 0xE12FFF1E; // bx lr
	}

	// Patch DSiWare to run in DS mode

#ifndef LOADERTWO
	// 1st Class Poker & BlackJack (USA)
	else if (strcmp(romTid, "KYPE") == 0) {
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
		*(u32*)0x0204A92C = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// 1st Class Poker & BlackJack (Europe)
	else if (strcmp(romTid, "KYPP") == 0) {
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
		*(u32*)0x0204A920 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// GO Series: 10 Second Run (USA)
	// GO Series: 10 Second Run (Europe)
	else if (strcmp(romTid, "KJUE") == 0 || strcmp(romTid, "KJUP") == 0) {
		*(u32*)0x020150FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201588C = 0xE1A00000; // nop
		*(u32*)0x0201589C = 0xE1A00000; // nop
		*(u32*)0x020158A8 = 0xE1A00000; // nop
		*(u32*)0x020158B4 = 0xE1A00000; // nop
		*(u32*)0x02015948 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015968 = 0xE1A00000; // nop
		*(u32*)0x02015970 = 0xE1A00000; // nop
		*(u32*)0x02015980 = 0xE1A00000; // nop
		*(u32*)0x02015A60 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02015A98 = 0xE1A00000; // nop
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
		*(u32*)0x02018B4C = 0xE1A00000; // nop
		*(u32*)0x020193E0 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
		*(u32*)0x02019D20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02030A88 = 0xE1A00000; // nop
		//tonccpy((u32*)0x02031660, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02034224 = 0xE1A00000; // nop
		patchInitDSiWare(0x02039C40, heapEnd);
		// *(u32*)0x02039FCC = 0x02115860;
		patchUserSettingsReadDSiWare(0x0203B3CC);
		*(u32*)0x0203B7D4 = 0xE1A00000; // nop
		*(u32*)0x0203B7D8 = 0xE1A00000; // nop
		*(u32*)0x0203B7DC = 0xE1A00000; // nop
		*(u32*)0x0203B7E0 = 0xE1A00000; // nop
	}

	// 10 Byou Sou (Japan)
	else if (strcmp(romTid, "KJUJ") == 0) {
		*(u32*)0x02014628 = 0xE12FFF1E; // bx lr
		*(u32*)0x02014DC0 = 0xE1A00000; // nop
		*(u32*)0x02014DD0 = 0xE1A00000; // nop
		*(u32*)0x02014DDC = 0xE1A00000; // nop
		*(u32*)0x02014DE8 = 0xE1A00000; // nop
		*(u32*)0x02014E7C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02014E9C = 0xE1A00000; // nop
		*(u32*)0x02014EA4 = 0xE1A00000; // nop
		*(u32*)0x02014EB4 = 0xE1A00000; // nop
		*(u32*)0x02014F94 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02014FCC = 0xE1A00000; // nop
		*(u32*)0x02018080 = 0xE1A00000; // nop
		*(u32*)0x02018914 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
		*(u32*)0x02019254 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202FFBC = 0xE1A00000; // nop
		*(u32*)0x02033758 = 0xE1A00000; // nop
		patchInitDSiWare(0x02039174, heapEnd);
		patchUserSettingsReadDSiWare(0x0203A900);
		*(u32*)0x0203AD08 = 0xE1A00000; // nop
		*(u32*)0x0203AD0C = 0xE1A00000; // nop
		*(u32*)0x0203AD10 = 0xE1A00000; // nop
		*(u32*)0x0203AD14 = 0xE1A00000; // nop
	}

	// 101 Pinball World (USA)
	else if (strcmp(romTid, "KIIE") == 0) {
		*(u32*)0x0204F46C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02093EB0 = 0xE1A00000; // nop
		*(u32*)0x02097220 = 0xE1A00000; // nop
		patchInitDSiWare(0x0209D104, heapEnd);
		*(u32*)0x0209D490 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0209E8F0);
		*(u32*)0x0209ED24 = 0xE1A00000; // nop
		*(u32*)0x0209ED28 = 0xE1A00000; // nop
		*(u32*)0x0209ED2C = 0xE1A00000; // nop
		*(u32*)0x0209ED30 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020A9BF4 = 0xE12FFF1E; // bx lr (Disable music)
			*(u32*)0x020A9AF8 = 0xE12FFF1E; // bx lr (Disable sounds)
		}
		/* *(u32*)0x020B7B0C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7B30 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7B54 = 0xE3A00001; // mov r0, #1 */
		setBL(0x020C6788, (u32)dsiSaveOpen);
		setBL(0x020C67B8, (u32)dsiSaveRead);
		setBL(0x020C67C0, (u32)dsiSaveClose);
		setBL(0x020C6890, (u32)dsiSaveOpen);
		setBL(0x020C68C0, (u32)dsiSaveRead);
		setBL(0x020C68C8, (u32)dsiSaveClose);
		*(u32*)0x020C69B4 = 0xE1A00000; // nop
		*(u32*)0x020C69CC = 0xE1A00000; // nop
		setBL(0x020C6A28, (u32)dsiSaveOpen);
		setBL(0x020C6A58, (u32)dsiSaveRead);
		setBL(0x020C6A60, (u32)dsiSaveClose);
		setBL(0x020C6AEC, (u32)dsiSaveOpen);
		setBL(0x020C6B18, (u32)dsiSaveRead);
		setBL(0x020C6B20, (u32)dsiSaveClose);
		*(u32*)0x020C6B8C = 0xE1A00000; // nop
		*(u32*)0x020C6B9C = 0xE1A00000; // nop
		setBL(0x020C6BF0, (u32)dsiSaveOpen);
		setBL(0x020C6C04, (u32)dsiSaveClose);
		*(u32*)0x020C6C78 = 0xE1A00000; // nop
		*(u32*)0x020C6C90 = 0xE1A00000; // nop
		setBL(0x020C6D04, (u32)dsiSaveOpen);
		setBL(0x020C6D34, (u32)dsiSaveRead);
		setBL(0x020C6D3C, (u32)dsiSaveClose);
		setBL(0x020C6DAC, (u32)dsiSaveOpen);
		setBL(0x020C6DD8, (u32)dsiSaveRead);
		setBL(0x020C6DE0, (u32)dsiSaveClose);
		setBL(0x020C6E80, (u32)dsiSaveOpen);
		setBL(0x020C6EB0, (u32)dsiSaveRead);
		setBL(0x020C6EB8, (u32)dsiSaveClose);
		setBL(0x020C6F28, (u32)dsiSaveOpen);
		setBL(0x020C6F54, (u32)dsiSaveRead);
		setBL(0x020C6F5C, (u32)dsiSaveClose);
		*(u32*)0x020C6FBC = 0xE1A00000; // nop
		*(u32*)0x020C6FCC = 0xE1A00000; // nop
		setBL(0x020C7038, (u32)dsiSaveOpen);
		setBL(0x020C704C, (u32)dsiSaveClose);
		setBL(0x020C7060, (u32)dsiSaveCreate);
		setBL(0x020C707C, (u32)dsiSaveOpen);
		*(u32*)0x020C708C = 0xE1A00000; // nop
		setBL(0x020C7098, (u32)dsiSaveClose);
		setBL(0x020C70A0, (u32)dsiSaveDelete);
		setBL(0x020C70B8, (u32)dsiSaveCreate);
		setBL(0x020C70C8, (u32)dsiSaveOpen);
		setBL(0x020C70E4, (u32)dsiSaveSetLength);
		setBL(0x020C70F4, (u32)dsiSaveWrite);
		setBL(0x020C70FC, (u32)dsiSaveClose);
	}

	// 101 Pinball World (Europe)
	else if (strcmp(romTid, "KIIP") == 0) {
		*(u32*)0x02049488 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0208DC80 = 0xE1A00000; // nop
		*(u32*)0x02090FF0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02096ED4, heapEnd);
		*(u32*)0x02097260 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020986C0);
		*(u32*)0x02098AF4 = 0xE1A00000; // nop
		*(u32*)0x02098AF8 = 0xE1A00000; // nop
		*(u32*)0x02098AFC = 0xE1A00000; // nop
		*(u32*)0x02098B00 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020A2AAC = 0xE12FFF1E; // bx lr (Disable music)
			*(u32*)0x020A29B0 = 0xE12FFF1E; // bx lr (Disable sounds)
		}
		/* *(u32*)0x020B0888 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B08AC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B08D0 = 0xE3A00001; // mov r0, #1 */
		setBL(0x020BC4D0, (u32)dsiSaveOpen);
		setBL(0x020BC500, (u32)dsiSaveRead);
		setBL(0x020BC508, (u32)dsiSaveClose);
		setBL(0x020BC5D8, (u32)dsiSaveOpen);
		setBL(0x020BC608, (u32)dsiSaveRead);
		setBL(0x020BC610, (u32)dsiSaveClose);
		*(u32*)0x020BC6FC = 0xE1A00000; // nop
		*(u32*)0x020BC714 = 0xE1A00000; // nop
		setBL(0x020BC770, (u32)dsiSaveOpen);
		setBL(0x020BC7A0, (u32)dsiSaveRead);
		setBL(0x020BC7A8, (u32)dsiSaveClose);
		setBL(0x020BC834, (u32)dsiSaveOpen);
		setBL(0x020BC860, (u32)dsiSaveRead);
		setBL(0x020BC868, (u32)dsiSaveClose);
		*(u32*)0x020BC8D4 = 0xE1A00000; // nop
		*(u32*)0x020BC8E4 = 0xE1A00000; // nop
		setBL(0x020BC938, (u32)dsiSaveOpen);
		setBL(0x020BC94C, (u32)dsiSaveClose);
		setBL(0x020BC9D0, (u32)dsiSaveOpen);
		setBL(0x020BC9E4, (u32)dsiSaveClose);
		setBL(0x020BC9F8, (u32)dsiSaveCreate);
		setBL(0x020BCA14, (u32)dsiSaveOpen);
		*(u32*)0x020BCA24 = 0xE1A00000; // nop
		setBL(0x020BCA30, (u32)dsiSaveClose);
		setBL(0x020BCA38, (u32)dsiSaveDelete);
		setBL(0x020BCA50, (u32)dsiSaveCreate);
		setBL(0x020BCA60, (u32)dsiSaveOpen);
		setBL(0x020BCA7C, (u32)dsiSaveSetLength);
		setBL(0x020BCA8C, (u32)dsiSaveWrite);
		setBL(0x020BCA94, (u32)dsiSaveClose);
	}

	// 18th Gate (USA)
	// 18th Gate (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KXOE") == 0 || strcmp(romTid, "KXOV") == 0) && extendedMemory2) {
		// extern u32* mepHeapSetPatch;
		// extern u32* gate18HeapAlloc;
		// extern u32* gate18HeapAddrPtr;

		*(u32*)0x02005090 = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop
		*(u32*)0x0200D278 = 0xE1A00000; // nop
		*(u32*)0x020105CC = 0xE1A00000; // nop
		patchInitDSiWare(0x020163C0, heapEnd);
		// *(u32*)0x0201674C = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020178D0);
		if (romTid[3] == 'E') {
			/* if (!extendedMemory2) {
				if (s2FlashcardId == 0x5A45) {
					gate18HeapAddrPtr[0] -= 0x01000000;
				}
				tonccpy((u32*)0x0200E844, mepHeapSetPatch, 0x70);
				tonccpy((u32*)0x02017E18, gate18HeapAlloc, 0xBC);

				*(u32*)0x0200E840 = (u32)getOffsetFromBL((u32*)0x02020740);
				setBL(0x02020740, 0x0200E844);
				*(u32*)0x02017E14 = (u32)getOffsetFromBL((u32*)0x020D1D04);
				setBL(0x020D1D04, 0x02017E18);
			}
			if (!extendedMemory2) {
				*(u32*)0x020CB630 = 0xE3A01812; // mov r1, #0x120000
			} */
			setBL(0x020D172C, (u32)dsiSaveGetInfo);
			setBL(0x020D1740, (u32)dsiSaveOpen);
			setBL(0x020D1754, (u32)dsiSaveCreate);
			setBL(0x020D1764, (u32)dsiSaveOpen);
			setBL(0x020D1774, (u32)dsiSaveGetResultCode);
			*(u32*)0x020D1784 = 0xE1A00000; // nop
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
		} else {
			/* if (!extendedMemory2) {
				*(u32*)0x020996FC = 0xE3A01812; // mov r1, #0x120000
			} */
			setBL(0x0209F7F8, (u32)dsiSaveGetInfo);
			setBL(0x0209F80C, (u32)dsiSaveOpen);
			setBL(0x0209F820, (u32)dsiSaveCreate);
			setBL(0x0209F830, (u32)dsiSaveOpen);
			setBL(0x0209F840, (u32)dsiSaveGetResultCode);
			*(u32*)0x0209F850 = 0xE1A00000; // nop
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
	}

	// 1001 Crystal Mazes Collection (USA)
	else if (strcmp(romTid, "KOKE") == 0) {
		*(u32*)0x0200C294 = 0xE1A00000; // nop
		*(u32*)0x0200C29C = 0xE1A00000; // nop
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
		*(u32*)0x0200CA50 = 0xE1A00000; // nop
		setBL(0x0200CA5C, (u32)dsiSaveClose);
		setBL(0x0200CA64, (u32)dsiSaveDelete);
		setBL(0x0200CA78, (u32)dsiSaveCreate);
		setBL(0x0200CA88, (u32)dsiSaveOpen);
		setBL(0x0200CA98, (u32)dsiSaveGetResultCode);
		setBL(0x0200CAB0, (u32)dsiSaveSetLength);
		setBL(0x0200CAC0, (u32)dsiSaveWrite);
		setBL(0x0200CAC8, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)0x0200E250 = 0xE12FFF1E; // bx lr (Disable loading .xm music files)
		}
		*(u32*)0x0200EF88 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020312C8 = 0xE1A00000; // nop
		*(u32*)0x02034528 = 0xE1A00000; // nop
		patchInitDSiWare(0x02039C18, heapEndExceed);
		if (!extendedMemory2) {
			*(u32*)0x02039FA4 = 0x020B26A0;
		}
		patchUserSettingsReadDSiWare(0x0203B258);
	}

	// 1001 Crystal Mazes Collection (Europe)
	else if (strcmp(romTid, "KOKP") == 0) {
		*(u32*)0x0200C22C = 0xE1A00000; // nop
		*(u32*)0x0200C234 = 0xE1A00000; // nop
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
		*(u32*)0x0200CA1C = 0xE1A00000; // nop
		setBL(0x0200CA28, (u32)dsiSaveClose);
		setBL(0x0200CA30, (u32)dsiSaveDelete);
		setBL(0x0200CA48, (u32)dsiSaveCreate);
		setBL(0x0200CA58, (u32)dsiSaveOpen);
		setBL(0x0200CA68, (u32)dsiSaveGetResultCode);
		setBL(0x0200CA84, (u32)dsiSaveSetLength);
		setBL(0x0200CA94, (u32)dsiSaveWrite);
		setBL(0x0200CA9C, (u32)dsiSaveClose);
		*(u32*)0x0200CAAC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		if (!extendedMemory2) {
			*(u32*)0x0200E08C = 0xE12FFF1E; // bx lr (Disable loading .xm music files)
		}
		*(u32*)0x02030584 = 0xE1A00000; // nop
		*(u32*)0x020337E4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02038ED4, heapEndExceed);
		if (!extendedMemory2) {
			*(u32*)0x02039260 = 0x020B14E0;
		}
		patchUserSettingsReadDSiWare(0x0203A514);
	}

	// 1950s Lawn Mower Kids (USA)
	// 1950s Lawn Mower Kids (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "K95E") == 0 || strcmp(romTid, "K95V") == 0) {
		*(u32*)0x02019608 = 0xE1A00000; // nop
		*(u32*)0x02015A08 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F5CC, heapEnd);
		patchUserSettingsReadDSiWare(0x02020DD8);
		doubleNopT(0x020471B0);
		*(u16*)0x0204713C = 0x4770; // bx lr
		*(u16*)0x0204715C = 0x4770; // bx lr
	}

	// 200 Vmaja: Charen Ji Supirittsu (Japan)
	else if (strcmp(romTid, "KVMJ") == 0) {
		*(u32*)0x020053EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02014E60 = 0xE1A00000; // nop
		*(u32*)0x020192F4 = 0xE1A00000; // nop
		*(u32*)0x02025DE4 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x02025DFC, heapEnd);
		*(u32*)0x0202EA3C = 0xE3A00001; // mov r0, #1
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

		toncset((char*)0x020A05F4, 0, 9); // Redirect otherPrv to dataPrv
		tonccpy((char*)0x020A05F4, dataPrv, strlen(dataPrv));
		toncset((char*)0x020A0608, 0, 9);
		tonccpy((char*)0x020A0608, dataPrv, strlen(dataPrv));
	}

	// 21 Blackjack (USA)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KBJE") == 0 && debugOrMep) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020053C4, 0x02034634);
			}
		} else {
			*(u32*)0x02005104 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x02005170 = 0xE1A00000; // nop
		*(u32*)0x02005190 = 0xE1A00000; // nop
		*(u32*)0x02005554 = 0xE1A00000; // nop
		*(u32*)0x02005584 = 0xE1A00000; // nop
		*(u32*)0x02005590 = 0xE1A00000; // nop
		*(u32*)0x020055CC = 0xE1A00000; // nop
		*(u32*)0x020055D8 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02016C60 = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
		}
		*(u32*)0x0202AE1C = 0xE1A00000; // nop
		*(u32*)0x0202DF80 = 0xE1A00000; // nop
		patchInitDSiWare(0x02032C04, heapEnd);
		patchUserSettingsReadDSiWare(0x020340E0);
	}

	// 21 Blackjack (Europe)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KBJP") == 0 && debugOrMep) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020053C4, 0x02034934);
			}
		} else {
			*(u32*)0x0200511C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x02005188 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x02005724 = 0xE1A00000; // nop
		*(u32*)0x02005754 = 0xE1A00000; // nop
		*(u32*)0x02005760 = 0xE1A00000; // nop
		*(u32*)0x0200579C = 0xE1A00000; // nop
		*(u32*)0x020057A8 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02016EBC = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
		}
		*(u32*)0x0202B078 = 0xE1A00000; // nop
		*(u32*)0x0202E264 = 0xE1A00000; // nop
		patchInitDSiWare(0x02032F04, heapEnd);
		patchUserSettingsReadDSiWare(0x020343E0);
	}

	// 24/7 Solitaire (USA)
	else if (strcmp(romTid, "K4IE") == 0) {
		*(u32*)0x02008918 = 0xE1A00000; // nop
		*(u32*)0x0200B6C4 = 0xE3A00001; // mov r0, #1
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
		*(u32*)0x020359EC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020709F8 = 0xE1A00000; // nop
		*(u32*)0x02073E8C = 0xE1A00000; // nop
		patchInitDSiWare(0x0207C560, heapEnd);
		patchUserSettingsReadDSiWare(0x0207DB44);
	}

	// 24/7 Solitaire (Europe)
	else if (strcmp(romTid, "K4IP") == 0) {
		*(u32*)0x020088C0 = 0xE1A00000; // nop
		*(u32*)0x0200B558 = 0xE3A00001; // mov r0, #1
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
		*(u32*)0x02035880 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020705D4 = 0xE1A00000; // nop
		*(u32*)0x02073A68 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207C13C, heapEnd);
		patchUserSettingsReadDSiWare(0x0207D720);
	}

	// Soritea Korekusho (Japan)
	else if (strcmp(romTid, "K4IJ") == 0) {
		*(u32*)0x020088B8 = 0xE1A00000; // nop
		*(u32*)0x0200B650 = 0xE3A00001; // mov r0, #1
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
		*(u32*)0x02035654 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02070108 = 0xE1A00000; // nop
		*(u32*)0x02073630 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207BD44, heapEnd);
		patchUserSettingsReadDSiWare(0x0207D338);
	}

	// 2Puzzle It: Fantasy (Europe)
	// Unknown bug making it not boot
	/* else if (strcmp(romTid, "K2PP") == 0) {
		*(u32*)0x0201F098 = 0xE1A00000; // nop
		*(u32*)0x02022560 = 0xE1A00000; // nop
		patchInitDSiWare(0x02026EB4, heapEnd);
		patchUserSettingsReadDSiWare(0x02028770);
		if (!extendedMemory2) {
			*(u32*)0x02027240 = 0x02163AE0;
			*(u32*)0x020837D0 = 0x270900;
		}
	} */

	// 3D Mahjong (USA)
	else if (strcmp(romTid, "KMJE") == 0) {
		*(u32*)0x020089C0 = 0xE1A00000; // nop
		*(u32*)0x020093D8 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02009DC4 = 0xE3A00001; // mov r0, #1
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
		*(u32*)0x0205F8E0 = 0xE1A00000; // nop
		*(u32*)0x02062D74 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206B048, heapEnd);
		patchUserSettingsReadDSiWare(0x0206C62C);
	}

	// 3D Mahjong (Europe)
	else if (strcmp(romTid, "KMJP") == 0) {
		*(u32*)0x02008964 = 0xE1A00000; // nop
		*(u32*)0x0200937C = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02009D68 = 0xE3A00001; // mov r0, #1
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
		*(u32*)0x0205F488 = 0xE1A00000; // nop
		*(u32*)0x0206291C = 0xE1A00000; // nop
		patchInitDSiWare(0x0206A920, heapEnd);
		patchUserSettingsReadDSiWare(0x0206BF04);
	}

	// 3D Twist Match (USA)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "K3CE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200F614 = 0xE1A00000; // nop
		*(u32*)0x02012AE0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A188, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B7A4);
		*(u32*)0x0203C264 = 0xE1A00000; // nop
		*(u32*)0x0203DC14 = 0xE1A00000; // nop
		*(u32*)0x0203FE8C = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203FEE8, 0x0201BCE8);
		}
		*(u32*)0x0203FF70 = 0xE1A00000; // nop
		*(u32*)0x02040014 = 0xE1A00000; // nop
		*(u32*)0x020400D4 = 0xE1A00000; // nop
		*(u32*)0x020532B0 = 0xE1A00000; // nop
		*(u32*)0x02054970 = 0xE1A00000; // nop
	}

	// 3D Twist Match (Europe)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "K3UP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200F610 = 0xE1A00000; // nop
		*(u32*)0x02012ADC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A184, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B7A0);
		*(u32*)0x0203C260 = 0xE1A00000; // nop
		*(u32*)0x0203DC10 = 0xE1A00000; // nop
		*(u32*)0x0203FE18 = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203FE74, 0x0201BCE4);
		}
		*(u32*)0x0203FEFC = 0xE1A00000; // nop
		*(u32*)0x0203FFA0 = 0xE1A00000; // nop
		*(u32*)0x02040044 = 0xE1A00000; // nop
		*(u32*)0x02053210 = 0xE1A00000; // nop
		*(u32*)0x02054928 = 0xE1A00000; // nop
	}

	// 3 Heroes: Crystal Soul (USA)
	// 3 Heroes: Crystal Soul (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "K3YE") == 0 || strcmp(romTid, "K3YV") == 0) && extendedMemory2) {
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x0200B494 = 0xE1A00000; // nop
		*(u32*)0x0200E7E8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02013DB4, heapEnd);
		patchUserSettingsReadDSiWare(0x020152C4);
		if (romTid[3] == 'E') {
			setBL(0x02027A28, (u32)dsiSaveGetInfo);
			setBL(0x02027A3C, (u32)dsiSaveOpen);
			setBL(0x02027A50, (u32)dsiSaveCreate);
			setBL(0x02027A60, (u32)dsiSaveOpen);
			setBL(0x02027A70, (u32)dsiSaveGetResultCode);
			*(u32*)0x02027A80 = 0xE1A00000; // nop
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
		} else {
			setBL(0x020314EC, (u32)dsiSaveGetInfo);
			setBL(0x02031500, (u32)dsiSaveOpen);
			setBL(0x02031514, (u32)dsiSaveCreate);
			setBL(0x02031524, (u32)dsiSaveOpen);
			setBL(0x02031534, (u32)dsiSaveGetResultCode);
			*(u32*)0x02031544 = 0xE1A00000; // nop
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
	}

	// 3 Heroes: Crystal Soul (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K3YJ") == 0 && extendedMemory2) {
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x0200B494 = 0xE1A00000; // nop
		*(u32*)0x0200E7E8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02013FE8, heapEnd);
		patchUserSettingsReadDSiWare(0x020154F8);
		setBL(0x0203DDD8, (u32)dsiSaveGetInfo);
		setBL(0x0203DDEC, (u32)dsiSaveOpen);
		setBL(0x0203DE00, (u32)dsiSaveCreate);
		setBL(0x0203DE10, (u32)dsiSaveOpen);
		setBL(0x0203DE20, (u32)dsiSaveGetResultCode);
		*(u32*)0x0203DE30 = 0xE1A00000; // nop
		setBL(0x0203DE3C, (u32)dsiSaveCreate);
		setBL(0x0203DE4C, (u32)dsiSaveOpen);
		setBL(0x0203DE78, (u32)dsiSaveSeek);
		setBL(0x0203DE90, (u32)dsiSaveWrite);
		setBL(0x0203DEA0, (u32)dsiSaveSeek);
		setBL(0x0203DEB0, (u32)dsiSaveWrite);
		setBL(0x0203DEC0, (u32)dsiSaveSeek);
		setBL(0x0203DED0, (u32)dsiSaveWrite);
		setBL(0x0203DF3C, (u32)dsiSaveSeek);
		setBL(0x0203DF4C, (u32)dsiSaveWrite);
		setBL(0x0203DF54, (u32)dsiSaveClose);
		setBL(0x0203DFA0, (u32)dsiSaveOpen);
		setBL(0x0203DFDC, (u32)dsiSaveSeek);
		setBL(0x0203DFF0, (u32)dsiSaveRead);
		setBL(0x0203E018, (u32)dsiSaveClose);
		setBL(0x0203E090, (u32)dsiSaveOpen);
		setBL(0x0203E0A4, (u32)dsiSaveSeek);
		setBL(0x0203E0B4, (u32)dsiSaveWrite);
		setBL(0x0203E0BC, (u32)dsiSaveClose);
	}

	// 3 Punten Katou Itsu: Bakumatsu Kuizu He (Japan)
	// `dataPub:/user.bin` is read when exists, but only `dataPub:/common.bin` is created and written, leaving `dataPub:/user.bin` unused (pretty sure)
	else if (strcmp(romTid, "K3BJ") == 0) {
		*(u32*)0x0200D6B8 = 0xE1A00000; // nop
		*(u32*)0x02011340 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017E78, heapEnd);
		*(u32*)0x02018204 -= 0x30000;
		*(u32*)0x020249E4 = 0xE1A00000; // nop
		*(u32*)0x02024A2C = 0xE3A00603; // mov r0, 0x300000
		*(u32*)0x02024DCC -= 0x220000;
		*(u32*)0x0202FAC8 -= 0x220000;
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
	else if (strcmp(romTid, "KHGJ") == 0 || strcmp(romTid, "K24J") == 0) {
		*(u32*)0x0200D7CC = 0xE1A00000; // nop
		*(u32*)0x02011448 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017F64, heapEnd);
		*(u32*)0x020182F0 -= 0x30000;
		*(u32*)0x020251E0 = 0xE1A00000; // nop
		*(u32*)0x02025228 = 0xE3A00603; // mov r0, 0x300000
		*(u32*)0x020255C8 -= 0x220000;
		*(u32*)0x020301F4 -= 0x220000;
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
	else if (strcmp(romTid, "K5RJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020104A0 = 0xE1A00000; // nop
		*(u32*)0x02014100 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C190, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D868);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203CD28, 0x0201DDE8);
		}
		setBL(0x02041D54, (u32)dsiSaveOpen);
		setBL(0x02041D6C, (u32)dsiSaveGetLength);
		setBL(0x02041D98, (u32)dsiSaveRead);
		setBL(0x02041DA0, (u32)dsiSaveClose);
		setBL(0x02041DDC, (u32)dsiSaveCreate);
		setBL(0x02041DEC, (u32)dsiSaveOpen);
		setBL(0x02041DFC, (u32)dsiSaveGetResultCode);
		*(u32*)0x02041E08 = 0xE1A00000; // nop
		setBL(0x02041E20, (u32)dsiSaveSetLength);
		setBL(0x02041E30, (u32)dsiSaveWrite);
		setBL(0x02041E38, (u32)dsiSaveClose);
		*(u32*)0x02086F54 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
	}

	// 4 Elements (USA)
	else if (strcmp(romTid, "K7AE") == 0) {
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
		*(u32*)0x0207C298 = 0xE3A05703; // mov r5, #0xC0000
		*(u32*)0x0207C760 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02087F64 = 0xE3A00602; // mov r0, #0x200000
		*(u32*)0x0208A6FC = 0xE1A00000; // nop
		*(u32*)0x0208DC40 = 0xE1A00000; // nop
		patchInitDSiWare(0x020A7A00, heapEnd);
		*(u32*)0x020A7D8C = 0x020F4FA0;
		patchUserSettingsReadDSiWare(0x020A90E8);
		*(u32*)0x020ADFA0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020ADFA4 = 0xE12FFF1E; // bx lr
	}

	// 4 Elements (Europe, Australia)
	else if (strcmp(romTid, "K7AV") == 0) {
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
		*(u32*)0x0207C2E0 = 0xE3A05703; // mov r5, #0xC0000
		*(u32*)0x0207C7A8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02087FE4 = 0xE3A00602; // mov r0, #0x200000
		*(u32*)0x0208A77C = 0xE1A00000; // nop
		*(u32*)0x0208DCC0 = 0xE1A00000; // nop
		patchInitDSiWare(0x020A7A80, heapEnd);
		*(u32*)0x020A7E0C = 0x020F5020;
		patchUserSettingsReadDSiWare(0x020A9168);
		*(u32*)0x020AE020 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020AE024 = 0xE12FFF1E; // bx lr
	}

	// 4 Travellers: Play French (USA)
	else if (strcmp(romTid, "KTFE") == 0) {
		useSharedFont = twlFontFound;
		if (!twlFontFound) {
			*(u32*)0x02005320 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02012E80 = 0xE1A00000; // nop
		*(u32*)0x02016070 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AF40, heapEnd);
		*(u32*)0x0203954C = 0xE1A00000; // nop
		*(u32*)0x02039550 = 0xE1A00000; // nop
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
		*(u32*)0x02050B44 = 0xE1A00000; // nop
		*(u32*)0x02050B4C = 0xE1A00000; // nop
		setBL(0x02050B58, (u32)dsiSaveCreate);
		setBL(0x02050B64, (u32)dsiSaveCreate);
		*(u32*)0x02050B6C = 0xE1A00000; // nop
		*(u32*)0x02050B84 = 0xE1A00000; // nop
		setBL(0x02050B94, (u32)dsiSaveOpen);
		*(u32*)0x02050BF0 = 0xE1A00000; // nop
		setBL(0x02050C10, (u32)dsiSaveSeek);
		setBL(0x02050C24, (u32)dsiSaveWrite);
		setBL(0x02050C38, (u32)dsiSaveWrite);
		setBL(0x02050C40, (u32)dsiSaveClose);
		setBL(0x02050C50, (u32)dsiSaveOpen);
		setBL(0x02050C88, (u32)dsiSaveSeek);
		setBL(0x02050C98, (u32)dsiSaveWrite);
		setBL(0x02050CAC, (u32)dsiSaveWrite);
		setBL(0x02050CB8, (u32)dsiSaveClose);
		*(u32*)0x02050CC0 = 0xE1A00000; // nop
		*(u32*)0x02050CD8 = 0xE1A00000; // nop
		*(u32*)0x02052854 = 0xE1A00000; // nop
		*(u32*)0x02052864 = 0xE1A00000; // nop
		setBL(0x02052874, (u32)dsiSaveCreate);
		setBL(0x02052880, (u32)dsiSaveCreate);
		*(u32*)0x020528B4 = 0xE1A00000; // nop
		*(u32*)0x020528C8 = 0xE1A00000; // nop
	}

	// 4 Travellers: Play French (Europe)
	// 4 Travellers: Play French (Australia)
	else if (strcmp(romTid, "KTFP") == 0 || strcmp(romTid, "KTFU") == 0) {
		useSharedFont = twlFontFound;
		if (!twlFontFound) {
			*(u32*)0x020052DC = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02012E34 = 0xE1A00000; // nop
		*(u32*)0x02016024 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AEF4, heapEnd);
		if (romTid[3] == 'P') {
			*(u32*)0x02025248 = 0xE1A00000; // nop
			*(u32*)0x0202524C = 0xE1A00000; // nop
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
			*(u32*)0x02040570 = 0xE1A00000; // nop
			*(u32*)0x02040578 = 0xE1A00000; // nop
			setBL(0x02040584, (u32)dsiSaveCreate);
			setBL(0x02040590, (u32)dsiSaveCreate);
			*(u32*)0x02040598 = 0xE1A00000; // nop
			*(u32*)0x020405B0 = 0xE1A00000; // nop
			setBL(0x020405C0, (u32)dsiSaveOpen);
			*(u32*)0x0204061C = 0xE1A00000; // nop
			setBL(0x0204063C, (u32)dsiSaveSeek);
			setBL(0x02040650, (u32)dsiSaveWrite);
			setBL(0x02040664, (u32)dsiSaveWrite);
			setBL(0x0204066C, (u32)dsiSaveClose);
			setBL(0x0204067C, (u32)dsiSaveOpen);
			setBL(0x020406B4, (u32)dsiSaveSeek);
			setBL(0x020406C4, (u32)dsiSaveWrite);
			setBL(0x020406D8, (u32)dsiSaveWrite);
			setBL(0x020406E4, (u32)dsiSaveClose);
			*(u32*)0x020406EC = 0xE1A00000; // nop
			*(u32*)0x02040704 = 0xE1A00000; // nop
			*(u32*)0x02042280 = 0xE1A00000; // nop
			*(u32*)0x02042290 = 0xE1A00000; // nop
			setBL(0x020422A0, (u32)dsiSaveCreate);
			setBL(0x020422AC, (u32)dsiSaveCreate);
			*(u32*)0x020422E0 = 0xE1A00000; // nop
			*(u32*)0x020422F4 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020394A4 = 0xE1A00000; // nop
			*(u32*)0x020394A8 = 0xE1A00000; // nop
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
			*(u32*)0x02050DF0 = 0xE1A00000; // nop
			*(u32*)0x02050DF8 = 0xE1A00000; // nop
			setBL(0x02050E04, (u32)dsiSaveCreate);
			setBL(0x02050E10, (u32)dsiSaveCreate);
			*(u32*)0x02050E18 = 0xE1A00000; // nop
			*(u32*)0x02050E30 = 0xE1A00000; // nop
			setBL(0x02050E40, (u32)dsiSaveOpen);
			*(u32*)0x02050E9C = 0xE1A00000; // nop
			setBL(0x02050EBC, (u32)dsiSaveSeek);
			setBL(0x02050ED0, (u32)dsiSaveWrite);
			setBL(0x02050EE4, (u32)dsiSaveWrite);
			setBL(0x02050EEC, (u32)dsiSaveClose);
			setBL(0x02050EFC, (u32)dsiSaveOpen);
			setBL(0x02050F34, (u32)dsiSaveSeek);
			setBL(0x02050F44, (u32)dsiSaveWrite);
			setBL(0x02050F58, (u32)dsiSaveWrite);
			setBL(0x02050F64, (u32)dsiSaveClose);
			*(u32*)0x02050F6C = 0xE1A00000; // nop
			*(u32*)0x02050F84 = 0xE1A00000; // nop
			*(u32*)0x02052B00 = 0xE1A00000; // nop
			*(u32*)0x02052B10 = 0xE1A00000; // nop
			setBL(0x02052B20, (u32)dsiSaveCreate);
			setBL(0x02052B2C, (u32)dsiSaveCreate);
			*(u32*)0x02052B60 = 0xE1A00000; // nop
			*(u32*)0x02052B74 = 0xE1A00000; // nop
		}
	}

	// 4 Travellers: Play Spanish (USA)
	else if (strcmp(romTid, "KTSE") == 0) {
		useSharedFont = twlFontFound;
		if (!twlFontFound) {
			*(u32*)0x02004CC0 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02012D70 = 0xE1A00000; // nop
		*(u32*)0x02015F54 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AE08, heapEnd);
		*(u32*)0x02025094 = 0xE1A00000; // nop
		*(u32*)0x02025098 = 0xE1A00000; // nop
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
		*(u32*)0x02051164 = 0xE1A00000; // nop
		*(u32*)0x0205116C = 0xE1A00000; // nop
		setBL(0x02051178, (u32)dsiSaveCreate);
		setBL(0x02051184, (u32)dsiSaveCreate);
		*(u32*)0x0205118C = 0xE1A00000; // nop
		*(u32*)0x020511A4 = 0xE1A00000; // nop
		setBL(0x020511B4, (u32)dsiSaveOpen);
		*(u32*)0x02051210 = 0xE1A00000; // nop
		setBL(0x02051230, (u32)dsiSaveSeek);
		setBL(0x02051244, (u32)dsiSaveWrite);
		setBL(0x02051258, (u32)dsiSaveWrite);
		setBL(0x02051260, (u32)dsiSaveClose);
		setBL(0x02051270, (u32)dsiSaveOpen);
		setBL(0x020512A8, (u32)dsiSaveSeek);
		setBL(0x020512B8, (u32)dsiSaveWrite);
		setBL(0x020512CC, (u32)dsiSaveWrite);
		setBL(0x020512D8, (u32)dsiSaveClose);
		*(u32*)0x020512E0 = 0xE1A00000; // nop
		*(u32*)0x020512F8 = 0xE1A00000; // nop
		*(u32*)0x02052E60 = 0xE1A00000; // nop
		*(u32*)0x02052E70 = 0xE1A00000; // nop
		setBL(0x02052E80, (u32)dsiSaveCreate);
		setBL(0x02052E8C, (u32)dsiSaveCreate);
		*(u32*)0x02052EC0 = 0xE1A00000; // nop
		*(u32*)0x02052ED4 = 0xE1A00000; // nop
	}

	// 4 Travellers: Play Spanish (Europe)
	// 4 Travellers: Play Spanish (Australia)
	else if (strcmp(romTid, "KTSP") == 0 || strcmp(romTid, "KTSU") == 0) {
		u32 offsetChange = (romTid[3] == 'U') ? 0x4C : 0;

		useSharedFont = twlFontFound;
		if (!twlFontFound) {
			*(u32*)0x02004C7C = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02012D24 = 0xE1A00000; // nop
		*(u32*)0x02015F08 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201ADBC, heapEnd);
		*(u32*)0x02025038 = 0xE1A00000; // nop
		*(u32*)0x0202503C = 0xE1A00000; // nop
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
		*(u32*)(0x0205147C-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02051484-offsetChange) = 0xE1A00000; // nop
		setBL(0x02051490-offsetChange, (u32)dsiSaveCreate);
		setBL(0x0205149C-offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x020514A4-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020514BC-offsetChange) = 0xE1A00000; // nop
		setBL(0x020514CC-offsetChange, (u32)dsiSaveOpen);
		*(u32*)(0x02051528-offsetChange) = 0xE1A00000; // nop
		setBL(0x02051548-offsetChange, (u32)dsiSaveSeek);
		setBL(0x0205155C-offsetChange, (u32)dsiSaveWrite);
		setBL(0x02051570-offsetChange, (u32)dsiSaveWrite);
		setBL(0x02051578-offsetChange, (u32)dsiSaveClose);
		setBL(0x02051588-offsetChange, (u32)dsiSaveOpen);
		setBL(0x020515C0-offsetChange, (u32)dsiSaveSeek);
		setBL(0x020515D0-offsetChange, (u32)dsiSaveWrite);
		setBL(0x020515E4-offsetChange, (u32)dsiSaveWrite);
		setBL(0x020515F0-offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020515F8-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02051610-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0205318C-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0205319C-offsetChange) = 0xE1A00000; // nop
		setBL(0x020531AC-offsetChange, (u32)dsiSaveCreate);
		setBL(0x020531B8-offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x020531EC-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02053200-offsetChange) = 0xE1A00000; // nop
	}

	// 40-in-1: Explosive Megamix (USA)
	else if (strcmp(romTid, "K45E") == 0) {
		*(u32*)0x0200DFB8 = 0xE1A00000; // nop
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
		*(u32*)0x020FD244 = 0xE1A00000; // nop
		tonccpy((u32*)0x020FDDC8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x021008F8 = 0xE1A00000; // nop
		patchInitDSiWare(0x021059B0, heapEnd);
		patchUserSettingsReadDSiWare(0x021070C0);
		*(u32*)0x021070DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x021070E0 = 0xE12FFF1E; // bx lr
		*(u32*)0x021070E8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x021070EC = 0xE12FFF1E; // bx lr
		*(u32*)0x021074E0 = 0xE1A00000; // nop
		*(u32*)0x021074E4 = 0xE1A00000; // nop
		*(u32*)0x021074E8 = 0xE1A00000; // nop
		*(u32*)0x021074EC = 0xE1A00000; // nop
		*(u32*)0x0210BEC0 = 0xE3A00003; // mov r0, #3
	}

	// 40-in-1: Explosive Megamix (Europe)
	else if (strcmp(romTid, "K45P") == 0) {
		*(u32*)0x0200DF68 = 0xE1A00000; // nop
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
		*(u32*)0x020FC8B4 = 0xE1A00000; // nop
		tonccpy((u32*)0x020FD438, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020FFF68 = 0xE1A00000; // nop
		patchInitDSiWare(0x02105020, heapEnd);
		patchUserSettingsReadDSiWare(0x02106730);
		*(u32*)0x0210674C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02106750 = 0xE12FFF1E; // bx lr
		*(u32*)0x02106758 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0210675C = 0xE12FFF1E; // bx lr
		*(u32*)0x02106B50 = 0xE1A00000; // nop
		*(u32*)0x02106B54 = 0xE1A00000; // nop
		*(u32*)0x02106B58 = 0xE1A00000; // nop
		*(u32*)0x02106B5C = 0xE1A00000; // nop
		*(u32*)0x0210B530 = 0xE3A00003; // mov r0, #3
	}

	// 5 in 1 Mahjong (USA)
	// 5 in 1 Mahjong (Europe)
	else if (strcmp(romTid, "KRJE") == 0 || strcmp(romTid, "KRJP") == 0) {
		*(u32*)0x02012514 = 0xE1A00000; // nop
		tonccpy((u32*)0x02013098, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02015FD8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CBE4, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E2C4);
		*(u32*)0x020235D0 = 0xE1A00000; // nop
		setBL(0x02030C18, (u32)dsiSaveOpen);
		setBL(0x02030C28, (u32)dsiSaveClose);
		*(u32*)0x02030CB8 = 0xE1A00000; // nop
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
		*(u32*)0x02056970 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// 5 in 1 Solitaire (USA)
	// 5 in 1 Solitaire (Europe)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if ((strcmp(romTid, "K5IE") == 0 || strcmp(romTid, "K5IP") == 0) && debugOrMep) {
		u16 offsetChange = (romTid[3] == 'E') ? 0 : 0x170;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad((romTid[3] == 'E') ? 0x020056DC : 0x02005658, 0x020455D0+offsetChange);
			} */
		} else {
			*(u32*)0x02005098 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		if (romTid[3] == 'E') {
			*(u32*)0x02005244 = 0xE1A00000; // nop
			*(u32*)0x02005260 = 0xE1A00000; // nop
			*(u32*)0x02005B38 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020051C4 = 0xE1A00000; // nop
			*(u32*)0x020051DC = 0xE1A00000; // nop
			*(u32*)0x020056B0 = 0xE1A00000; // nop
			*(u32*)0x020056DC = 0xE1A00000; // nop
			*(u32*)0x020056E8 = 0xE1A00000; // nop
			*(u32*)0x02005720 = 0xE1A00000; // nop
			*(u32*)0x0200572C = 0xE1A00000; // nop
			*(u32*)0x02005AF0 = 0xE1A00000; // nop
		}
		if (!extendedMemory2) {
			*(u32*)(0x0202914C+offsetChange) = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
		}
		*(u32*)(0x0203BD2C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203EE88+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x02043AF0+offsetChange, heapEnd);
		if (!extendedMemory2) {
			*(u32*)(0x02043E7C+offsetChange) = *(u32*)0x02004FD0;
		}
		patchUserSettingsReadDSiWare(0x0204508C+offsetChange);
	}

	// 505 Tangram (USA)
	else if (strcmp(romTid, "K2OE") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		/*if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x0200E860, 0x02067044);
			}
		} else {*/
			*(u32*)0x0200E8C4 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		// }
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
		*(u32*)0x02059E8C = 0xE1A00000; // nop
		*(u32*)0x0205D32C = 0xE1A00000; // nop
		*(u32*)0x0205F120 = 0xE1A00000; // nop
		*(u32*)0x0205F124 = 0xE1A00000; // nop
		patchInitDSiWare(0x020654E0, heapEnd);
		patchUserSettingsReadDSiWare(0x02066AD4);
	}

	// 505 Tangram (Europe)
	else if (strcmp(romTid, "K2OP") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		/*if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x0200E674, 0x02066BAC);
			}
		} else {*/
			*(u32*)0x0200E6DC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		// }
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
		*(u32*)0x020599F4 = 0xE1A00000; // nop
		*(u32*)0x0205CE94 = 0xE1A00000; // nop
		*(u32*)0x0205EC88 = 0xE1A00000; // nop
		*(u32*)0x0205EC8C = 0xE1A00000; // nop
		patchInitDSiWare(0x02065048, heapEnd);
		patchUserSettingsReadDSiWare(0x0206663C);
	}

	// 505 Tangram (Japan)
	else if (strcmp(romTid, "K2OJ") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		/*if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x0200E000, 0x020665AC);
			}
		} else {*/
			*(u32*)0x0200E064 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		// }
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
		*(u32*)0x02059334 = 0xE1A00000; // nop
		*(u32*)0x0205C85C = 0xE1A00000; // nop
		*(u32*)0x0205E680 = 0xE1A00000; // nop
		*(u32*)0x0205E684 = 0xE1A00000; // nop
		patchInitDSiWare(0x02064A48, heapEnd);
		patchUserSettingsReadDSiWare(0x0206603C);
	}

	// 7 Card Games (USA)
	else if (strcmp(romTid, "K7CE") == 0) {
		*(u32*)0x02012574 = 0xE1A00000; // nop
		tonccpy((u32*)0x020130EC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02015FA4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CB3C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E1F8);
		setBL(0x02034820, (u32)dsiSaveOpen);
		setBL(0x02034830, (u32)dsiSaveClose);
		*(u32*)0x020348A8 = 0xE1A00000; // nop
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
		*(u32*)0x020417CC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02041B88 = 0xE1A00000; // nop
	}

	// 7 Wonders II (USA)
	// Opening Scores menu causes a crash for some reason
	else if (strcmp(romTid, "K7WE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020549CC = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x020549DC = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x020549F8 = 0xE1A00000; // nop
		setBL(0x020572A4, (u32)dsiSaveOpen);
		setBL(0x020572BC, (u32)dsiSaveClose);
		*(u32*)0x02057748 = (u32)dsiSaveRead;
		setBL(0x02057754, (u32)dsiSaveSeek);
		setBL(0x020579CC, (u32)dsiSaveOpen);
		*(u32*)0x02057A34 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x02057A3C, (u32)dsiSaveClose);
		*(u32*)0x02057C40 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x02057C48, (u32)dsiSaveClose);
		*(u32*)0x02057E50 = (u32)dsiSaveWrite;
		setBL(0x02057E80, (u32)dsiSaveCreate);
		setBL(0x02057E94, (u32)dsiSaveOpen);
		setBL(0x02057ED0, (u32)dsiSaveWrite);
		setBL(0x02057EF8, (u32)dsiSaveClose);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0205D80C, 0x020A1B54);
			}
		} else {
			*(u32*)0x0205DA34 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x02097D0C = 0xE1A00000; // nop
		*(u32*)0x0209B2C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x020A0008, heapEnd);
		patchUserSettingsReadDSiWare(0x020A1600);
	}

	// 90's Pool (USA)
	// Audio does not play on retail consoles
	else if (strcmp(romTid, "KXPE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201B538 = 0xE1A00000; // nop
		*(u32*)0x0201F868 = 0xE1A00000; // nop
		patchInitDSiWare(0x02024E3C, heapEnd);
		patchUserSettingsReadDSiWare(0x02026420);
		setBL(0x02035444, (u32)dsiSaveCreate);
		setBL(0x02035454, (u32)dsiSaveOpen);
		setBL(0x02035470, (u32)dsiSaveGetResultCode);
		setBL(0x02035494, (u32)dsiSaveSeek);
		setBL(0x020354AC, (u32)dsiSaveGetResultCode);
		setBL(0x020354D0, (u32)dsiSaveWrite);
		setBL(0x020354F0, (u32)dsiSaveClose);
		setBL(0x020354F8, (u32)dsiSaveGetResultCode);
		setBL(0x02035514, (u32)dsiSaveGetResultCode);
		setBL(0x02035550, (u32)dsiSaveOpenR);
		setBL(0x02035560, (u32)dsiSaveGetLength);
		setBL(0x02035594, (u32)dsiSaveRead);
		setBL(0x020355AC, (u32)dsiSaveClose);
		setBL(0x020355B8, (u32)dsiSaveGetResultCode);
		if (!extendedMemory2) {
			*(u32*)0x0203D544 = 0xE12FFF1E; // bx lr
		}
	}

	// 90's Pool (Europe)
	// Audio does not play on retail consoles
	else if (strcmp(romTid, "KXPP") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x02010F08 = 0xE1A00000; // nop
		*(u32*)0x02015238 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A80C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201BDF0);
		setBL(0x0202AE14, (u32)dsiSaveCreate);
		setBL(0x0202AE24, (u32)dsiSaveOpen);
		setBL(0x0202AE40, (u32)dsiSaveGetResultCode);
		setBL(0x0202AE64, (u32)dsiSaveSeek);
		setBL(0x0202AE7C, (u32)dsiSaveGetResultCode);
		setBL(0x0202AEA0, (u32)dsiSaveWrite);
		setBL(0x0202AEC0, (u32)dsiSaveClose);
		setBL(0x0202AEC8, (u32)dsiSaveGetResultCode);
		setBL(0x0202AEE4, (u32)dsiSaveGetResultCode);
		setBL(0x0202AF20, (u32)dsiSaveOpenR);
		setBL(0x0202AF30, (u32)dsiSaveGetLength);
		setBL(0x0202AF64, (u32)dsiSaveRead);
		setBL(0x0202AF7C, (u32)dsiSaveClose);
		setBL(0x0202AF88, (u32)dsiSaveGetResultCode);
		if (!extendedMemory2) {
			*(u32*)0x02032F10 = 0xE12FFF1E; // bx lr
		}
	}

	// 99Bullets (USA)
	else if (strcmp(romTid, "K99E") == 0) {
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02005144 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005160 = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x02011E1C = 0xE1A00000; // nop
		*(u32*)0x02011E50 = 0xE1A00000; // nop
		*(u32*)0x02013E8C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02013EA8 = 0xE1A00000; // nop
		*(u32*)0x02013EDC = 0xE1A00000; // nop
		*(u32*)0x02013EE4 = 0xE1A00000; // nop
		*(u32*)0x02013F58 = 0xE1A00000; // nop
		*(u32*)0x02013F5C = 0xE1A00000; // nop
		*(u32*)0x02013F60 = 0xE1A00000; // nop
		*(u32*)0x02013FE4 = 0xE1A00000; // nop
		*(u32*)0x02013FE8 = 0xE1A00000; // nop
		*(u32*)0x02013FEC = 0xE1A00000; // nop
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
		*(u32*)0x0204F5B0 = 0xE1A00000; // nop
		*(u32*)0x02053278 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206A010, heapEnd);
	}

	// 99Bullets (Europe)
	else if (strcmp(romTid, "K99P") == 0) {
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x0200512C = 0xE1A00000; // nop
		*(u32*)0x02005140 = 0xE1A00000; // nop
		*(u32*)0x02005144 = 0xE1A00000; // nop
		*(u32*)0x02005148 = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02010EAC = 0xE1A00000; // nop
		*(u32*)0x02010EE0 = 0xE1A00000; // nop
		*(u32*)0x02012F1C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012F38 = 0xE1A00000; // nop
		*(u32*)0x02012F6C = 0xE1A00000; // nop
		*(u32*)0x02012F74 = 0xE1A00000; // nop
		*(u32*)0x02012FE8 = 0xE1A00000; // nop
		*(u32*)0x02012FEC = 0xE1A00000; // nop
		*(u32*)0x02012FF0 = 0xE1A00000; // nop
		*(u32*)0x02013074 = 0xE1A00000; // nop
		*(u32*)0x02013078 = 0xE1A00000; // nop
		*(u32*)0x0201307C = 0xE1A00000; // nop
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
		*(u32*)0x0205FCF4 = 0xE1A00000; // nop
		*(u32*)0x0206488C = 0xE1A00000; // nop
		patchInitDSiWare(0x0206CFBC, heapEnd);
	}

	// 99Bullets (Japan)
	else if (strcmp(romTid, "K99J") == 0) {
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02005144 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005160 = 0xE1A00000; // nop
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x02010F2C = 0xE1A00000; // nop
		*(u32*)0x02010F60 = 0xE1A00000; // nop
		setBL(0x02012E48, (u32)dsiSaveCreate);
		*(u32*)0x02012E5C = 0xE1A00000; // nop
		*(u32*)0x02012E68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012EF8 = 0xE1A00000; // nop
		*(u32*)0x02012EFC = 0xE1A00000; // nop
		*(u32*)0x02012F00 = 0xE1A00000; // nop
		setBL(0x02012F0C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012F28 = 0xE1A00000; // nop
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
		*(u32*)0x02063618 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206BD64, heapEnd);
		*(u32*)0x020726F0 = 0xE1A00000; // nop
	}

	// 99Moves (USA)
	// 99Moves (Europe)
	else if (strcmp(romTid, "K9WE") == 0 || strcmp(romTid, "K9WP") == 0) {
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02010CE8 = 0xE1A00000; // nop
		*(u32*)0x02010D1C = 0xE1A00000; // nop
		setBL(0x02012BD4, (u32)dsiSaveCreate);
		*(u32*)0x02012BE8 = 0xE1A00000; // nop
		*(u32*)0x02012BF4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012C84 = 0xE1A00000; // nop
		*(u32*)0x02012C88 = 0xE1A00000; // nop
		*(u32*)0x02012C8C = 0xE1A00000; // nop
		setBL(0x02012C98, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012CB4 = 0xE1A00000; // nop
		*(u32*)0x02012CC0 = 0xE3A00001; // mov r0, #1
		if (romTid[3] == 'E') {
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
			*(u32*)0x02061478 = 0xE1A00000; // nop
			patchInitDSiWare(0x02069B80, heapEnd);
			*(u32*)0x0207050C = 0xE1A00000; // nop
		} else {
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
			*(u32*)0x020614C8 = 0xE1A00000; // nop
			patchInitDSiWare(0x02069BD0, heapEnd);
			*(u32*)0x0207055C = 0xE1A00000; // nop
		}
	}

	// 99Seconds (USA)
	// 99Seconds (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KXTE") == 0 || strcmp(romTid, "KXTP") == 0) && extendedMemory2) {
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02010CE4 = 0xE1A00000; // nop
		*(u32*)0x02010D18 = 0xE1A00000; // nop
		setBL(0x02011918, (u32)dsiSaveCreate);
		*(u32*)0x0201192C = 0xE1A00000; // nop
		*(u32*)0x02011938 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020119CC = 0xE1A00000; // nop
		*(u32*)0x020119D0 = 0xE1A00000; // nop
		*(u32*)0x020119D4 = 0xE1A00000; // nop
		setBL(0x020119E0, (u32)dsiSaveGetResultCode);
		*(u32*)0x020119FC = 0xE1A00000; // nop
		*(u32*)0x02011A08 = 0xE3A00001; // mov r0, #1
		if (romTid[3] == 'E') {
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
			*(u32*)0x02061590 = 0xE1A00000; // nop
			patchInitDSiWare(0x02069CC4, heapEnd);
			*(u32*)0x02070668 = 0xE1A00000; // nop
		} else {
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
			*(u32*)0x020615E0 = 0xE1A00000; // nop
			patchInitDSiWare(0x02069E14, heapEnd);
			*(u32*)0x020706B8 = 0xE1A00000; // nop
		}
	}

	// Aa! Nikaku Dori (Japan)
	else if (strcmp(romTid, "K2KJ") == 0) {
		useSharedFont = twlFontFound;
		if (!useSharedFont) {
			*(u32*)0x020050D4 = 0xE1A00000; // nop
		}
		*(u32*)0x0200D6EC = 0xE1A00000; // nop
		*(u32*)0x02010B44 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016CB0, heapEnd);
		patchUserSettingsReadDSiWare(0x02018140);
		*(u32*)0x0201D5C8 = 0xE1A00000; // nop
		*(u32*)0x0201D5D0 = 0xE1A00000; // nop
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
		*(u32*)0x02028F74 = 0xE1A00000; // nop
		setBL(0x02029340, (u32)dsiSaveGetResultCode);
		*(u32*)0x02029374 = 0xE1A00000; // nop
	}

	// Absolute Baseball (USA)
	// Audio doesn't play on retail consoles
	// Extra fixes required for it to work on real hardware
	/*else if (strcmp(romTid, "KE9E") == 0) {
		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x0200F890 = 0xE1A00000; // nop
		*(u32*)0x02013454 = 0xE1A00000; // nop
		*(u32*)0x02018230 = 0xE1A00000; // nop
		*(u32*)0x02019FDC = 0xE1A00000; // nop
		*(u32*)0x02019FE0 = 0xE1A00000; // nop
		*(u32*)0x02019FEC = 0xE1A00000; // nop
		*(u32*)0x0201A14C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201A1A8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B42C);
		if (!extendedMemory2) {
			*(u32*)0x0201D0FC = 0xE12FFF1E; // bx lr
			*(u32*)0x0201D124 = 0xE12FFF1E; // bx lr
			*(u32*)0x02038E5C = 0xE1A00000; // nop
		}
		*(u32*)0x0205FAD0 = 0xE1A00000; // nop
		*(u32*)0x02072554 = 0xE3A00001; // mov r0, #0
	}*/

	// Absolute BrickBuster (USA)
	// Crashes after starting a game mode
	// Requires 8MB of RAM
	/* else if (strcmp(romTid, "K6QE") == 0) {
		*(u32*)0x020053E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02055B74 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02055B78 = 0xE12FFF1E; // bx lr
		*(u32*)0x02055C48 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02055C4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0205C8DC = 0xE1A00000; // nop
		*(u32*)0x02060D94 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206D9CC, heapEnd);
		patchUserSettingsReadDSiWare(0x0206EE64);
		*(u32*)0x0206EEEC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206EEF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206EEF8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0206EEFC = 0xE12FFF1E; // bx lr
	} */

	// Abyss (USA)
	// Abyss (Europe)
	else if (strcmp(romTid, "KXGE") == 0 || strcmp(romTid, "KXGP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x50;

		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02010CE8 = 0xE1A00000; // nop
		*(u32*)0x02010D1C = 0xE1A00000; // nop
		setBL(0x02012BBC, (u32)dsiSaveCreate);
		*(u32*)0x02012BD0 = 0xE1A00000; // nop
		*(u32*)0x02012BDC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012C6C = 0xE1A00000; // nop
		*(u32*)0x02012C70 = 0xE1A00000; // nop
		*(u32*)0x02012C74 = 0xE1A00000; // nop
		setBL(0x02012C80, (u32)dsiSaveGetResultCode);
		*(u32*)0x02012C9C = 0xE1A00000; // nop
		*(u32*)0x02012CA8 = 0xE3A00001; // mov r0, #1
		setBL(0x020318E0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020318F8+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02031908+offsetChange, (u32)dsiSaveSeek);
		setBL(0x02031918+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02031920+offsetChange, (u32)dsiSaveClose);
		setBL(0x02031990+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020319A8+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x020319BC+offsetChange, (u32)dsiSaveSeek);
		setBL(0x020319CC+offsetChange, (u32)dsiSaveRead);
		setBL(0x020319D4+offsetChange, (u32)dsiSaveClose);
		setBL(0x02031A4C+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02031A78+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02031AB4+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02031AC4+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020618A4+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x02069F94+offsetChange, heapEnd);
		*(u32*)(0x0206A320+offsetChange) -= 0x30000;
		*(u32*)(0x02070920+offsetChange) = 0xE1A00000; // nop
	}

	// Ace Mathician (USA)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KQKE") == 0) {
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x02005128 = 0xE1A00000; // nop
		*(u32*)0x0200512C = 0xE1A00000; // nop
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200513C = 0xE1A00000; // nop
		*(u32*)0x0200DFD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02010084 = 0xE1A00000; // nop
		*(u32*)0x02030344 = 0xE1A00000; // nop
		*(u32*)0x020335B0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02038154, heapEnd);
		*(u32*)0x02039B74 = 0xE1A00000; // nop
		*(u32*)0x02039B78 = 0xE1A00000; // nop
		*(u32*)0x02039B7C = 0xE1A00000; // nop
		*(u32*)0x02039B80 = 0xE1A00000; // nop
	}

	// Ace Mathician (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KQKV") == 0) {
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x02005128 = 0xE1A00000; // nop
		*(u32*)0x0200512C = 0xE1A00000; // nop
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200513C = 0xE1A00000; // nop
		*(u32*)0x0200DFD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02030354 = 0xE1A00000; // nop
		*(u32*)0x020335C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02038164, heapEnd);
		*(u32*)0x02039B84 = 0xE1A00000; // nop
		*(u32*)0x02039B88 = 0xE1A00000; // nop
		*(u32*)0x02039B8C = 0xE1A00000; // nop
		*(u32*)0x02039B90 = 0xE1A00000; // nop
	}

	// Advanced Circuits (USA)
	// Advanced Circuits (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strncmp(romTid, "KAC", 3) == 0) {
		*(u32*)0x02011298 = 0xE1A00000; // nop
		*(u32*)0x02014738 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A1BC, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B758);
		*(u32*)0x0202CDA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D490 = 0xE1A00000; // nop
		if (romTid[3] == 'E') {
			*(u32*)0x02053F30 = 0xE1A00000; // nop
			*(u32*)0x02053F90 = 0xE1A00000; // nop
			*(u32*)0x02054920 = 0xE12FFF1E; // bx lr
		} else if (romTid[3] == 'V') {
			*(u32*)0x02053F58 = 0xE1A00000; // nop
			*(u32*)0x02053FB8 = 0xE1A00000; // nop
			*(u32*)0x020548C0 = 0xE12FFF1E; // bx lr
		}
	}

	// Ah! Heaven (USA)
	// Ah! Heaven (Europe)
	else if (strcmp(romTid, "K5HE") == 0 || strcmp(romTid, "K5HP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200F778 = 0xE1A00000; // nop
		//tonccpy((u32*)0x020102FC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02012A58 = 0xE1A00000; // nop
		patchInitDSiWare(0x02018098, heapEnd);
		patchUserSettingsReadDSiWare(0x02019538);
		/* setBL(0x0201E3C0, (u32)dsiSaveCreate);
		setBL(0x0201E3E8, (u32)dsiSaveOpen);
		*(u32*)0x0201E41C = 0xE1A00000; // nop
		setBL(0x0201E428, (u32)dsiSaveCreate);
		setBL(0x0201E438, (u32)dsiSaveOpen);
		setBL(0x0201E468, (u32)dsiSaveOpen);
		setBL(0x0201E540, (u32)dsiSaveRead);
		setBL(0x0201E560, (u32)dsiSaveSeek);
		setBL(0x0201E578, (u32)dsiSaveWrite);
		setBL(0x0201E588, (u32)dsiSaveSeek); */
		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x0201DDC8, 0x02019A7C);
			}
		} else {
			*(u32*)0x0201FD04 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02029C68 = 0xE12FFF1E; // bx lr
		*(u32*)0x02029D14 = 0xE12FFF1E; // bx lr
		/* setBL(0x02029CFC, (u32)dsiSaveClose);
		setBL(0x02029DB8, (u32)dsiSaveClose);
		setBL(0x02029DCC, (u32)dsiSaveClose); */
	}

	// AiRace: Tunnel (USA)
	// Requires 8MB of RAM
	// Crashes after selecting a stage due to weird bug
	/*else if (strcmp(romTid, "KATE") == 0 && extendedMemory2) {
		*(u16*)0x0202A3D2 = nopT;
		*(u16*)0x0202A3D4 = nopT;
		*(u16*)0x0202A59C = nopT;
		*(u16*)0x0202A59E = nopT;
		*(u32*)0x02032AF0 = 0xE1A00000; // nop
		*(u16*)0x02042042 = nopT;
		*(u16*)0x02042044 = nopT;
		*(u16*)0x02042048 = nopT;
		*(u16*)0x0204204A = nopT;
		*(u32*)0x020420F4 = 0xE1A00000; // nop
		*(u32*)0x02048AC0 = 0xE1A00000; // nop
		*(u32*)0x0204C1C8 = 0xE1A00000; // nop
		*(u32*)0x02056798 = 0xE1A00000; // nop
		*(u32*)0x02058628 = 0xE1A00000; // nop
		*(u32*)0x0205862C = 0xE1A00000; // nop
		*(u32*)0x02058638 = 0xE1A00000; // nop
		*(u32*)0x02058798 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020587F4, heapEnd); // mov r0, #0x2700000
		patchUserSettingsReadDSiWare(0x02059B68);
	}*/

	// Alien Puzzle Adventure (USA)
	// Alien Puzzle Adventure (Europe, Australia)
	else if (strcmp(romTid, "KP7E") == 0 || strcmp(romTid, "KP7V") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02005090 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020053B0 = 0xE1A00000; // nop
		*(u32*)0x020053B4 = 0xE1A00000; // nop
		*(u32*)0x020053B8 = 0xE1A00000; // nop
		*(u32*)0x020053BC = 0xE1A00000; // nop
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
		if (romTid[3] == 'E') {
			if (useSharedFont) {
				*(u32*)0x02025338 = 0xE3A05703; // mov r5, 0xC0000
				if (!extendedMemory2) {
					patchTwlFontLoad(0x02025448, 0x02049D80);
				}
			} else {
				*(u32*)0x02017864 = 0xE1A00000; // nop (Skip Manual screen)
			}
			*(u32*)0x0203F1EC = 0xE1A00000; // nop
			tonccpy((u32*)0x0203FD80, dsiSaveGetResultCode, 0xC);
			*(u32*)0x020428B8 = 0xE1A00000; // nop
			patchInitDSiWare(0x02048244, heapEnd);
			patchUserSettingsReadDSiWare(0x0204982C);
		} else {
			if (useSharedFont) {
				*(u32*)0x0202526C = 0xE3A05703; // mov r5, 0xC0000
				if (!extendedMemory2) {
					patchTwlFontLoad(0x0202537C, 0x02049CD4);
				}
			} else {
				*(u32*)0x020177AC = 0xE1A00000; // nop (Skip Manual screen)
			}
			*(u32*)0x0203F140 = 0xE1A00000; // nop
			tonccpy((u32*)0x0203FCD4, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0204280C = 0xE1A00000; // nop
			patchInitDSiWare(0x02048198, heapEnd);
			patchUserSettingsReadDSiWare(0x02049780);
		}
	}

	// G.G. Series: All Breaker (USA)
	// G.G. Series: All Breaker (Japan)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "K27E") == 0 || strcmp(romTid, "K27J") == 0) && extendedMemory2) {
		*(u32*)0x0200D71C = 0xE1A00000; // nop
		*(u32*)0x0204E880 = 0xE1A00000; // nop
		*(u32*)0x02052814 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205A9F8, heapEnd);
		patchUserSettingsReadDSiWare(0x0205C1B4);
	}

	// All-Star Air Hockey (USA)
	else if (strcmp(romTid, "KAOE") == 0) {
		// useSharedFont = twlFontFound;
		setBL(0x020056F8, (u32)dsiSaveOpen);
		setBL(0x0200570C, (u32)dsiSaveGetLength);
		setBL(0x02005720, (u32)dsiSaveRead);
		setBL(0x02005730, (u32)dsiSaveClose);
		*(u32*)0x020057D0 = 0xE1A00000; // nop
		setBL(0x020057E0, (u32)dsiSaveCreate);
		setBL(0x020057FC, (u32)dsiSaveOpen);
		setBL(0x02005820, (u32)dsiSaveSetLength);
		setBL(0x02005830, (u32)dsiSaveWrite);
		setBL(0x02005838, (u32)dsiSaveClose);
		setBL(0x020058E0, (u32)dsiSaveOpen);
		setBL(0x020058F8, (u32)dsiSaveSetLength);
		setBL(0x02005908, (u32)dsiSaveWrite);
		setBL(0x02005914, (u32)dsiSaveClose);
		// if (!useSharedFont) {
			setB(0x020104DC, 0x020105D4); // Skip Manual screen
		// }
		*(u32*)0x020272D8 = 0xE1A00000; // nop
		*(u32*)0x0202A784 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203002C, heapEnd);
		patchUserSettingsReadDSiWare(0x02031634);
	}

	// AlphaBounce (USA)
	// Does not boot
	/*else if (strcmp(romTid, "KALE") == 0) {
		*(u32*)0x020187B8 = 0xE1A00000; // nop
		*(u32*)0x0201BC4C = 0xE1A00000; // nop
		*(u32*)0x0201EA34 = 0xE1A00000; // nop
		*(u32*)0x0201EAE8 = 0xE1A00000; // nop
		*(u32*)0x0201EC08 = 0xE1A00000; // nop
		*(u32*)0x0201EC84 = 0xE1A00000; // nop
		*(u32*)0x0201ED08 = 0xE1A00000; // nop
		*(u32*)0x0201FBC8 = 0xE1A00000; // nop
		*(u32*)0x0201FC38 = 0xE1A00000; // nop
		*(u32*)0x0201FD4C = 0xE1A00000; // nop
		*(u32*)0x0201FDB4 = 0xE1A00000; // nop
		*(u32*)0x0201FE34 = 0xE1A00000; // nop
		*(u32*)0x0201FE98 = 0xE1A00000; // nop
		*(u32*)0x0201FF50 = 0xE1A00000; // nop
		*(u32*)0x0201FFC0 = 0xE1A00000; // nop
		*(u32*)0x020226A0 = 0xE1A00000; // nop
		*(u32*)0x02024DD4 = 0xE1A00000; // nop
		*(u32*)0x02024DD8 = 0xE1A00000; // nop
		*(u32*)0x02024DE4 = 0xE1A00000; // nop
		*(u32*)0x02024F44 = 0xE1A00000; // nop
		*(u32*)0x02024FA0 = 0xE3A0078F; // mov r0, #0x23E0000
		*(u32*)0x02024FC4 = 0xE3500001; // cmp r0, #1
		*(u32*)0x02024FCC = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x020299E0 = 0xE1A00000; // nop
		*(u32*)0x020B0600 = 0xE1A00000; // nop
		*(u32*)0x020B0604 = 0xE1A00000; // nop
		*(u32*)0x020B060C = 0xE1A00000; // nop
	}*/

	// Amakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF2J") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200D658 = 0xE1A00000; // nop
		*(u32*)0x02010BD8 = 0xE1A00000; // nop
		*(u32*)0x02017A20 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x02017A38, heapEnd);
		patchUserSettingsReadDSiWare(0x02018ED4);
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
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203E620, 0x02019458);
			*(u32*)0x0203E6E0 = 0xE1A00000; // nop
		}
	}

	// Animal Boxing (USA)
	// Animal Boxing (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KAXE") == 0 || strcmp(romTid, "KAXP") == 0) && extendedMemory2) {
		useSharedFont = twlFontFound;
		*(u32*)0x0201DCE4 = 0xE1A00000; // nop
		*(u32*)0x02021764 = 0xE1A00000; // nop
		patchInitDSiWare(0x02027B98, heapEnd);
		patchUserSettingsReadDSiWare(0x02029080);
		*(u32*)0x0202909C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020290A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020290A8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020290AC = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
			setBL(0x020D1F18, (u32)dsiSaveOpen);
			setBL(0x020D1F28, (u32)dsiSaveClose);
			*(u32*)0x020D1F3C = 0xE1A00000; // nop
			setBL(0x020D1F4C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x020D1F64, (u32)dsiSaveOpen);
			setBL(0x020D1F88, (u32)dsiSaveSetLength);
			setBL(0x020D1F9C, (u32)dsiSaveSeek);
			setBL(0x020D1FDC, (u32)dsiSaveWrite);
			setBL(0x020D1FE4, (u32)dsiSaveClose);
			setBL(0x020D20C0, (u32)dsiSaveOpen);
			setBL(0x020D2144, (u32)dsiSaveClose);
			setBL(0x020D218C, (u32)dsiSaveSeek);
			setBL(0x020D219C, (u32)dsiSaveWrite);
			setBL(0x020D21D0, (u32)dsiSaveSeek);
			setBL(0x020D21E0, (u32)dsiSaveRead);
			if (!useSharedFont) {
				*(u32*)0x020D3F38 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
			*(u32*)0x020D4070 = 0xE12FFF1E; // bx lr
		} else {
			setBL(0x020D1F5C, (u32)dsiSaveOpen);
			setBL(0x020D1F6C, (u32)dsiSaveClose);
			*(u32*)0x020D1F80 = 0xE1A00000; // nop
			setBL(0x020D1F90, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x020D1FA8, (u32)dsiSaveOpen);
			setBL(0x020D1FCC, (u32)dsiSaveSetLength);
			setBL(0x020D1FE0, (u32)dsiSaveSeek);
			setBL(0x020D2020, (u32)dsiSaveWrite);
			setBL(0x020D2028, (u32)dsiSaveClose);
			setBL(0x020D2104, (u32)dsiSaveOpen);
			setBL(0x020D2188, (u32)dsiSaveClose);
			setBL(0x020D21D0, (u32)dsiSaveSeek);
			setBL(0x020D21E0, (u32)dsiSaveWrite);
			setBL(0x020D2214, (u32)dsiSaveSeek);
			setBL(0x020D2224, (u32)dsiSaveRead);
			if (!useSharedFont) {
				*(u32*)0x020D3F7C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
			*(u32*)0x020D40B4 = 0xE12FFF1E; // bx lr
		}
	}

	// Animal Puzzle Adventure (USA)
	else if (strcmp(romTid, "KPCE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0201A54C, 0x02038CB4);
		}
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
		*(u32*)0x02026964 = 0xE1A00000; // nop
		setBL(0x02026988, (u32)dsiSaveSetLength);
		setBL(0x020269D8, (u32)dsiSaveWrite);
		setBL(0x020269E0, (u32)dsiSaveClose);
		*(u32*)0x0202D6EC = 0xE1A00000; // nop
		tonccpy((u32*)0x0202E838, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020312B4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02036E94, heapEnd);
		patchUserSettingsReadDSiWare(0x02038330);
	}

	// Animal Puzzle Adventure (Europe, Australia)
	else if (strcmp(romTid, "KPCV") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02019610, 0x020379A4);
		}
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
		*(u32*)0x02025A28 = 0xE1A00000; // nop
		setBL(0x02025A4C, (u32)dsiSaveSetLength);
		setBL(0x02025A9C, (u32)dsiSaveWrite);
		setBL(0x02025AA4, (u32)dsiSaveClose);
		*(u32*)0x0202C7B0 = 0xE1A00000; // nop
		tonccpy((u32*)0x0202D8FC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02030378 = 0xE1A00000; // nop
		patchInitDSiWare(0x02035F68, heapEnd);
		patchUserSettingsReadDSiWare(0x02037404);
	}

	// O Tegaru Pazuru Shirizu: Chiria no Doubutsu Goya (Japan)
	else if (strcmp(romTid, "KPCJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200E894 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F9E0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201245C = 0xE1A00000; // nop
		patchInitDSiWare(0x02018024, heapEnd);
		patchUserSettingsReadDSiWare(0x020194B0);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02026FB8, 0x02019D64);
		}
		setBL(0x02032EA4, (u32)dsiSaveOpen);
		setBL(0x02032EB8, (u32)dsiSaveGetLength);
		setBL(0x02032EC8, (u32)dsiSaveRead);
		setBL(0x02032ED0, (u32)dsiSaveClose);
		*(u32*)0x02032F30 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02032F70 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02032FFC, (u32)dsiSaveGetInfo);
		setBL(0x02033008, (u32)dsiSaveCreate);
		setBL(0x02033018, (u32)dsiSaveOpen);
		setBL(0x02033048, (u32)dsiSaveSetLength);
		setBL(0x02033074, (u32)dsiSaveWrite);
		setBL(0x0203307C, (u32)dsiSaveClose);
		setBL(0x020331F0, (u32)dsiSaveGetInfo);
		setBL(0x02033224, (u32)dsiSaveCreate);
		setBL(0x02033234, (u32)dsiSaveOpen);
		setBL(0x02033260, (u32)dsiSaveSetLength);
		setBL(0x020332B0, (u32)dsiSaveWrite);
		setBL(0x020332B8, (u32)dsiSaveClose);
	}

	// Anne's Doll Studio: Antique Collection (USA)
	// Anne's Doll Studio: Antique Collection (Europe)
	// Atorie Decora Doll: Antique (Japan)
	// Anne's Doll Studio: Princess Collection (USA)
	// Anne's Doll Studio: Princess Collection (Europe)
	else if (strcmp(romTid, "KY8E") == 0 || strcmp(romTid, "KY8P") == 0 || strcmp(romTid, "KY8J") == 0
		   || strcmp(romTid, "K2SE") == 0 || strcmp(romTid, "K2SP") == 0) {
		/*if (!extendedMemory2) {
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				u32* addr = (u32*)ndsHeader->arm9destination;
				if (addr[i] >= 0x022D0000 && addr[i] < 0x02360000) {
					addr[i] -= 0x200000;
				}
			}
		}*/

		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02011374 = 0xE1A00000; // nop
		*(u32*)0x020151A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BFB4, heapEnd);
		/*if (!extendedMemory2) {
			*(u32*)0x0201C340 -= 0x240000;
		}*/
		patchUserSettingsReadDSiWare(0x0201D654);
		setBL(0x0202A164, (u32)dsiSaveGetResultCode);
		setBL(0x0202A288, (u32)dsiSaveOpen);
		setBL(0x0202A2BC, (u32)dsiSaveRead);
		setBL(0x0202A2E4, (u32)dsiSaveClose);
		setBL(0x0202A344, (u32)dsiSaveOpen);
		setBL(0x0202A38C, (u32)dsiSaveWrite);
		setBL(0x0202A3AC, (u32)dsiSaveClose);
		setBL(0x0202A3F0, (u32)dsiSaveCreate);
		setBL(0x0202A44C, (u32)dsiSaveDelete);
		*(u32*)0x0202C608 = 0xE1A00000; // nop
		if (strncmp(romTid, "KY8", 3) == 0) {
			*(u32*)0x0202F97C = 0xE1A00000; // nop
			*(u32*)0x0202F998 = 0xE1A00000; // nop
			*(u32*)0x02030018 = 0xE12FFF1E; // bx lr
			*(u32*)0x020310D0 = 0xE1A00000; // nop
			if (romTid[3] == 'E') {
				*(u32*)0x0203B89C = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAFC = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BB00 = 0xE12FFF1E; // bx lr
			} else if (romTid[3] == 'P') {
				*(u32*)0x0203B844 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAA4 = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BAA8 = 0xE12FFF1E; // bx lr
			} else {
				*(u32*)0x0203B848 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
				*(u32*)0x0203BAA8 = 0xE3A00000; // mov r0, #0 (Skip free space check)
				*(u32*)0x0203BAAC = 0xE12FFF1E; // bx lr
			}
		} else {
			*(u32*)0x0202F978 = 0xE1A00000; // nop
			*(u32*)0x0202F994 = 0xE1A00000; // nop
			*(u32*)0x02030014 = 0xE12FFF1E; // bx lr
			*(u32*)0x020310CC = 0xE1A00000; // nop
			*(u32*)0x0203B678 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x0203B8D8 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x0203B8DC = 0xE12FFF1E; // bx lr
		}
	}

	// Anne's Doll Studio: Gothic Collection (USA)
	// Nae Mamdaero Peurinseseu 2: Yureop-Pyeon (Korea)
	else if (strcmp(romTid, "K54E") == 0 || strcmp(romTid, "KLQK") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0xD0;
		u8 offsetChange2 = (romTid[3] == 'E') ? 0 : 0xD4;
		u8 offsetChange3 = (romTid[3] == 'E') ? 0 : 0xD8;
		u16 offsetChange4 = (romTid[3] == 'E') ? 0 : 0x148;

		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02011270 = 0xE1A00000; // nop
		*(u32*)0x0201509C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BEB0, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D550);
		*(u32*)(0x02033850-offsetChange) = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)(0x02033AB0-offsetChange) = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)(0x02033AB4-offsetChange) = 0xE12FFF1E; // bx lr
		setBL(0x02035614-offsetChange2, (u32)dsiSaveGetResultCode);
		setBL(0x02035738-offsetChange2, (u32)dsiSaveOpen);
		setBL(0x0203576C-offsetChange2, (u32)dsiSaveRead);
		setBL(0x02035794-offsetChange2, (u32)dsiSaveClose);
		setBL(0x020357F4-offsetChange2, (u32)dsiSaveOpen);
		setBL(0x0203583C-offsetChange2, (u32)dsiSaveWrite);
		setBL(0x0203585C-offsetChange2, (u32)dsiSaveClose);
		setBL(0x020358A0-offsetChange2, (u32)dsiSaveCreate);
		setBL(0x020358FC-offsetChange2, (u32)dsiSaveDelete);
		*(u32*)(0x02037AC4-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203AE68-offsetChange3) = 0xE1A00000; // nop
		*(u32*)(0x0203AE84-offsetChange3) = 0xE1A00000; // nop
		*(u32*)(0x0203B69C-offsetChange3) = 0xE12FFF1E; // bx lr
		*(u32*)(0x0203C7C0-offsetChange4) = 0xE1A00000; // nop
	}

	// Anne's Doll Studio: Lolita Collection (USA)
	// Anne's Doll Studio: Lolita Collection (Europe)
	else if (strcmp(romTid, "KLQE") == 0 || strcmp(romTid, "KLQP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x54;

		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020112A0 = 0xE1A00000; // nop
		*(u32*)0x020150CC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BEE0, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D580);
		*(u32*)0x020337B0 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)0x02033A10 = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)0x02033A14 = 0xE12FFF1E; // bx lr
		setBL(0x020355C4-offsetChange, (u32)dsiSaveGetResultCode);
		setBL(0x020356E8-offsetChange, (u32)dsiSaveOpen);
		setBL(0x0203571C-offsetChange, (u32)dsiSaveRead);
		setBL(0x02035744-offsetChange, (u32)dsiSaveClose);
		setBL(0x020357A4-offsetChange, (u32)dsiSaveOpen);
		setBL(0x020357EC-offsetChange, (u32)dsiSaveWrite);
		setBL(0x0203580C-offsetChange, (u32)dsiSaveClose);
		setBL(0x02035850-offsetChange, (u32)dsiSaveCreate);
		setBL(0x020358AC-offsetChange, (u32)dsiSaveDelete);
		*(u32*)(0x02037A74-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203AE0C-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203AE28-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203B640-offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x0203C6F4-offsetChange) = 0xE1A00000; // nop
	}

	// Atorie Decora Doll: Gothic (Japan)
	// Atorie Decora Doll: Lolita (Japan)
	else if (strcmp(romTid, "K54J") == 0 || strcmp(romTid, "KLQJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020111A0 = 0xE1A00000; // nop
		*(u32*)0x02014F44 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BD3C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D3DC);
		if (strncmp(romTid, "K54", 3) == 0) {
			*(u32*)0x02032A58 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x02032CB8 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x02032CBC = 0xE12FFF1E; // bx lr
			setBL(0x020347FC, (u32)dsiSaveGetResultCode);
			setBL(0x02034920, (u32)dsiSaveOpen);
			setBL(0x02034954, (u32)dsiSaveRead);
			setBL(0x0203497C, (u32)dsiSaveClose);
			setBL(0x020349DC, (u32)dsiSaveOpen);
			setBL(0x02034A24, (u32)dsiSaveWrite);
			setBL(0x02034A44, (u32)dsiSaveClose);
			setBL(0x02034A88, (u32)dsiSaveCreate);
			setBL(0x02034AE4, (u32)dsiSaveDelete);
			*(u32*)0x02036CAC = 0xE1A00000; // nop
			*(u32*)0x0203A050 = 0xE1A00000; // nop
			*(u32*)0x0203A06C = 0xE1A00000; // nop
			*(u32*)0x0203A884 = 0xE12FFF1E; // bx lr
			*(u32*)0x0203B938 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020329C4 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x02032C24 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x02032C28 = 0xE12FFF1E; // bx lr
			setBL(0x02034764, (u32)dsiSaveGetResultCode);
			setBL(0x02034888, (u32)dsiSaveOpen);
			setBL(0x020348BC, (u32)dsiSaveRead);
			setBL(0x020348E4, (u32)dsiSaveClose);
			setBL(0x02034944, (u32)dsiSaveOpen);
			setBL(0x0203498C, (u32)dsiSaveWrite);
			setBL(0x020349AC, (u32)dsiSaveClose);
			setBL(0x020349F0, (u32)dsiSaveCreate);
			setBL(0x02034A4C, (u32)dsiSaveDelete);
			*(u32*)0x02036C14 = 0xE1A00000; // nop
			*(u32*)0x02039FB4 = 0xE1A00000; // nop
			*(u32*)0x02039FD0 = 0xE1A00000; // nop
			*(u32*)0x0203A7E8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0203B89C = 0xE1A00000; // nop
		}
	}

	// Anne's Doll Studio: Tokyo Collection (USA)
	// Atorie Decora Doll: Princess (Japan)
	else if (strcmp(romTid, "KSQE") == 0 || strcmp(romTid, "K2SJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02011344 = 0xE1A00000; // nop
		*(u32*)0x02015170 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BF84, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D624);
		if (romTid[3] == 'E') {
			setBL(0x02027F34, (u32)dsiSaveGetResultCode);
			setBL(0x02028058, (u32)dsiSaveOpen);
			setBL(0x0202808C, (u32)dsiSaveRead);
			setBL(0x020280B4, (u32)dsiSaveClose);
			setBL(0x02028114, (u32)dsiSaveOpen);
			setBL(0x0202815C, (u32)dsiSaveWrite);
			setBL(0x0202817C, (u32)dsiSaveClose);
			setBL(0x020281C0, (u32)dsiSaveCreate);
			setBL(0x0202821C, (u32)dsiSaveDelete);
			*(u32*)0x0202A3E4 = 0xE1A00000; // nop
			*(u32*)0x0202D760 = 0xE1A00000; // nop
			*(u32*)0x0202D77C = 0xE1A00000; // nop
			*(u32*)0x0202DF84 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202F05C = 0xE1A00000; // nop
			*(u32*)0x0203A534 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x0203A794 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x0203A798 = 0xE12FFF1E; // bx lr
		} else {
			setBL(0x0202A134, (u32)dsiSaveGetResultCode);
			setBL(0x0202A258, (u32)dsiSaveOpen);
			setBL(0x0202A28C, (u32)dsiSaveRead);
			setBL(0x0202A2B4, (u32)dsiSaveClose);
			setBL(0x0202A314, (u32)dsiSaveOpen);
			setBL(0x0202A35C, (u32)dsiSaveWrite);
			setBL(0x0202A37C, (u32)dsiSaveClose);
			setBL(0x0202A3C0, (u32)dsiSaveCreate);
			setBL(0x0202A41C, (u32)dsiSaveDelete);
			*(u32*)0x0202C5D8 = 0xE1A00000; // nop
			*(u32*)0x0202F950 = 0xE1A00000; // nop
			*(u32*)0x0202F96C = 0xE1A00000; // nop
			*(u32*)0x0202FFEC = 0xE12FFF1E; // bx lr
			*(u32*)0x020310A4 = 0xE1A00000; // nop
			*(u32*)0x0203B650 = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
			*(u32*)0x0203B8B0 = 0xE3A00000; // mov r0, #0 (Skip free space check)
			*(u32*)0x0203B8B4 = 0xE12FFF1E; // bx lr
		}
	}

	// Atorie Decora Doll (Japan)
	// Nae Mamdaero Peurinseseu (Korea)
	else if (strcmp(romTid, "KDUJ") == 0 || strcmp(romTid, "KDUK") == 0) {
		u8 offsetChangeInit0 = (romTid[3] == 'J') ? 0 : 0x5C;
		u8 offsetChangeInit1 = (romTid[3] == 'J') ? 0 : 0x68;
		u8 offsetChangeInit = (romTid[3] == 'J') ? 0 : 0x84;
		u8 offsetChange = (romTid[3] == 'J') ? 0 : 0x94;
		u8 offsetChange2 = (romTid[3] == 'J') ? 0 : 0xA4;
		u16 offsetChange3 = (romTid[3] == 'J') ? 0 : 0x16C;
		u16 offsetChange4 = (romTid[3] == 'J') ? 0 : 0x1C0;

		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x0200509C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)(0x02011218+offsetChangeInit0) = 0xE1A00000; // nop
		*(u32*)(0x02014FB0+offsetChangeInit1) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BD8C+offsetChangeInit, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D41C+offsetChangeInit);
		setBL(0x020270E4+offsetChange, (u32)dsiSaveGetResultCode);
		setBL(0x02027208+offsetChange2, (u32)dsiSaveOpen);
		setBL(0x0202723C+offsetChange2, (u32)dsiSaveRead);
		setBL(0x02027264+offsetChange2, (u32)dsiSaveClose);
		setBL(0x020272C4+offsetChange2, (u32)dsiSaveOpen);
		setBL(0x0202730C+offsetChange2, (u32)dsiSaveWrite);
		setBL(0x0202732C+offsetChange2, (u32)dsiSaveClose);
		setBL(0x02027370+offsetChange2, (u32)dsiSaveCreate);
		setBL(0x020273CC+offsetChange2, (u32)dsiSaveDelete);
		*(u32*)(0x02029594+offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x0202C848+offsetChange3) = 0xE1A00000; // nop
		*(u32*)(0x0202C864+offsetChange3) = 0xE1A00000; // nop
		*(u32*)(0x0202D06C+offsetChange3) = 0xE12FFF1E; // bx lr
		*(u32*)(0x0202E0D4+offsetChange3) = 0xE1A00000; // nop
		*(u32*)(0x02039428+offsetChange4) = 0xE3A00000; // mov r0, #0 (Skip pit.bin check)
		*(u32*)(0x02039688+offsetChange4) = 0xE3A00000; // mov r0, #0 (Skip free space check)
		*(u32*)(0x0203968C+offsetChange4) = 0xE12FFF1E; // bx lr
	}

	// Anonymous Notes 1: From The Abyss (USA & Europe)
	// Anonymous Notes 2: From The Abyss (USA & Europe)
	else if ((strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0)
	  && romTid[3] != 'J') {
		useSharedFont = (twlFontFound && debugOrMep);
		const u32 newCodeAddr = 0x0201A4B0;
		*(u32*)0x0200E74C = 0xE1A00000; // nop
		*(u32*)0x02011C48 = 0xE1A00000; // nop
		patchInitDSiWare(0x02018854, heapEnd);
		patchUserSettingsReadDSiWare(0x02019F18);
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
		*(u32*)0x02024510 = 0xE1A00000; // nop
		*(u32*)0x02024530 = 0xE1A00000; // nop
		*(u32*)0x0202457C = 0xE1A00000; // nop
		if (ndsHeader->gameCode[2] == 'I') {
			if (romTid[3] == 'E') {
				*(u32*)0x0209E2CC = 0xE1A00000; // nop
				*(u32*)0x0209E2E0 = 0xE1A00000; // nop
				*(u32*)0x0209E2F4 = 0xE1A00000; // nop
				*(u32*)0x020CFB78 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D02A0, newCodeAddr);
					}
				} else {
					*(u32*)0x020CFCD0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			} else {
				*(u32*)0x0209E0DC = 0xE1A00000; // nop
				*(u32*)0x0209E0F0 = 0xE1A00000; // nop
				*(u32*)0x0209E104 = 0xE1A00000; // nop
				*(u32*)0x020CF988 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D00F4, newCodeAddr);
					}
				} else {
					*(u32*)0x020CFAE0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			}
		} else if (ndsHeader->gameCode[2] == '2') {
			if (romTid[3] == 'E') {
				*(u32*)0x0209E300 = 0xE1A00000; // nop
				*(u32*)0x0209E314 = 0xE1A00000; // nop
				*(u32*)0x0209E328 = 0xE1A00000; // nop
				*(u32*)0x020D071C = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D0E44, newCodeAddr);
					}
				} else {
					*(u32*)0x020D0874 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			} else {
				*(u32*)0x0209E0EC = 0xE1A00000; // nop
				*(u32*)0x0209E100 = 0xE1A00000; // nop
				*(u32*)0x0209E114 = 0xE1A00000; // nop
				*(u32*)0x020D0508 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D0C74, newCodeAddr);
					}
				} else {
					*(u32*)0x020D0660 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			}
		}
	}

	// Anonymous Notes 1: From The Abyss (Japan)
	// Anonymous Notes 2: From The Abyss (Japan)
	else if (strncmp(romTid, "KVI", 3) == 0 || strncmp(romTid, "KV2", 3) == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		const u32 newCodeAddr = 0x0201A840;
		*(u32*)0x0200506C = 0xE1A00000; // nop
		*(u32*)0x0200E6DC = 0xE1A00000; // nop
		*(u32*)0x02011C9C = 0xE1A00000; // nop
		patchInitDSiWare(0x02018BBC, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A278);
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
		*(u32*)0x02024B0C = 0xE1A00000; // nop
		*(u32*)0x02024B2C = 0xE1A00000; // nop
		*(u32*)0x02024B78 = 0xE1A00000; // nop
		if (ndsHeader->gameCode[2] == 'I') {
			*(u32*)0x0209E0A4 = 0xE1A00000; // nop
			*(u32*)0x0209E0B8 = 0xE1A00000; // nop
			*(u32*)0x0209E0CC = 0xE1A00000; // nop
			if (useSharedFont) {
				if (!extendedMemory2 && expansionPakFound) {
					patchTwlFontLoad(0x020CFCF0, newCodeAddr);
				}
			} else {
				*(u32*)0x020CF970 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		} else if (ndsHeader->gameCode[2] == '2') {
			*(u32*)0x0209E0D0 = 0xE1A00000; // nop
			*(u32*)0x0209E0E4 = 0xE1A00000; // nop
			*(u32*)0x0209E0F8 = 0xE1A00000; // nop
			if (useSharedFont) {
				if (!extendedMemory2 && expansionPakFound) {
					patchTwlFontLoad(0x020D0930, newCodeAddr);
				}
			} else {
				*(u32*)0x020D050C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
		}
	}

	// Anonymous Notes 3: From The Abyss (USA & Japan)
	// Anonymous Notes 4: From The Abyss (USA & Japan)
	else if (strncmp(romTid, "KV3", 3) == 0 || strncmp(romTid, "KV4", 3) == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		const u32 newCodeAddr = 0x0201A4DC;
		*(u32*)0x02005084 = 0xE1A00000; // nop
		*(u32*)0x0200E778 = 0xE1A00000; // nop
		*(u32*)0x02011C74 = 0xE1A00000; // nop
		patchInitDSiWare(0x02018880, heapEnd);
		patchUserSettingsReadDSiWare(0x02019F44);
		*(u32*)0x02019F60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02019F64 = 0xE12FFF1E; // bx lr
		*(u32*)0x02019F6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02019F70 = 0xE12FFF1E; // bx lr
		if (ndsHeader->gameCode[2] == '3') {
			if (romTid[3] == 'E') {
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
				*(u32*)0x0202453C = 0xE1A00000; // nop
				*(u32*)0x0202455C = 0xE1A00000; // nop
				*(u32*)0x020245A8 = 0xE1A00000; // nop
				*(u32*)0x0209E250 = 0xE1A00000; // nop
				*(u32*)0x0209E264 = 0xE1A00000; // nop
				*(u32*)0x0209E278 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D06F0, newCodeAddr);
					}
				} else {
					*(u32*)0x020D0120 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
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
				*(u32*)0x02024560 = 0xE1A00000; // nop
				*(u32*)0x02024580 = 0xE1A00000; // nop
				*(u32*)0x020245CC = 0xE1A00000; // nop
				*(u32*)0x0209DB18 = 0xE1A00000; // nop
				*(u32*)0x0209DB2C = 0xE1A00000; // nop
				*(u32*)0x0209DB40 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020CFC2C, newCodeAddr);
					}
				} else {
					*(u32*)0x020CF8AC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			}
		} else if (ndsHeader->gameCode[2] == '4') {
			if (romTid[3] == 'E') {
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
				*(u32*)0x0202454C = 0xE1A00000; // nop
				*(u32*)0x0202456C = 0xE1A00000; // nop
				*(u32*)0x020245B8 = 0xE1A00000; // nop
				*(u32*)0x0209F72C = 0xE1A00000; // nop
				*(u32*)0x0209F740 = 0xE1A00000; // nop
				*(u32*)0x0209F754 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D1594, newCodeAddr);
					}
				} else {
					*(u32*)0x020D0FFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
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
				*(u32*)0x02024570 = 0xE1A00000; // nop
				*(u32*)0x02024590 = 0xE1A00000; // nop
				*(u32*)0x020245DC = 0xE1A00000; // nop
				*(u32*)0x0209EDEC = 0xE1A00000; // nop
				*(u32*)0x0209EE00 = 0xE1A00000; // nop
				*(u32*)0x0209EE14 = 0xE1A00000; // nop
				if (useSharedFont) {
					if (!extendedMemory2 && expansionPakFound) {
						patchTwlFontLoad(0x020D0930, newCodeAddr);
					}
				} else {
					*(u32*)0x020D0590 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				}
			}
		}
	}

	// Antipole (USA)
	// Does not boot due to lack of memory
	/*else if (strcmp(romTid, "KJHE") == 0) {
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x020333F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02035704 = 0xE1A00000; // nop
		*(u32*)0x020357E0 = 0xE1A00000; // nop
		*(u32*)0x0203BAD8 = 0xE1A00000; // nop
		*(u32*)0x0203D9E4 = 0xE1A00000; // nop
		*(u32*)0x02056BD0 = 0xE1A00000; // nop
		*(u32*)0x020577A4 = 0xE1A00000; // nop
		*(u32*)0x020581AC = 0xE1A00000; // nop
		*(u32*)0x02058260 = 0xE1A00000; // nop
		*(u32*)0x02058300 = 0xE1A00000; // nop
		*(u32*)0x02058380 = 0xE1A00000; // nop
		*(u32*)0x020583FC = 0xE1A00000; // nop
		*(u32*)0x02058480 = 0xE1A00000; // nop
		*(u32*)0x02058988 = 0xE1A00000; // nop
		*(u32*)0x02058A44 = 0xE1A00000; // nop
		*(u32*)0x02058AF0 = 0xE1A00000; // nop
		*(u32*)0x02058B84 = 0xE1A00000; // nop
		*(u32*)0x02058C18 = 0xE1A00000; // nop
		*(u32*)0x02058CAC = 0xE1A00000; // nop
		*(u32*)0x02058D40 = 0xE1A00000; // nop
		*(u32*)0x02058DD4 = 0xE1A00000; // nop
		*(u32*)0x02058E68 = 0xE1A00000; // nop
		*(u32*)0x02058EFC = 0xE1A00000; // nop
		*(u32*)0x02059020 = 0xE1A00000; // nop
		*(u32*)0x02059084 = 0xE1A00000; // nop
		*(u32*)0x0205914C = 0xE1A00000; // nop
		*(u32*)0x020591BC = 0xE1A00000; // nop
		*(u32*)0x02059248 = 0xE1A00000; // nop
		*(u32*)0x020592B8 = 0xE1A00000; // nop
		*(u32*)0x02059340 = 0xE1A00000; // nop
		*(u32*)0x020593B0 = 0xE1A00000; // nop
		*(u32*)0x020594C4 = 0xE1A00000; // nop
		*(u32*)0x0205952C = 0xE1A00000; // nop
		*(u32*)0x020595AC = 0xE1A00000; // nop
		*(u32*)0x02059610 = 0xE1A00000; // nop
		*(u32*)0x020596C8 = 0xE1A00000; // nop
		*(u32*)0x02059738 = 0xE1A00000; // nop
		*(u32*)0x0205D874 = 0xE1A00000; // nop
		*(u32*)0x0205D878 = 0xE1A00000; // nop
		*(u32*)0x0205D884 = 0xE1A00000; // nop
		*(u32*)0x0205DA40 = 0xE3A0078F; // mov r0, #0x23E0000
		*(u32*)0x0205DA64 = 0xE3500001; // cmp r0, #1
		*(u32*)0x0205DA6C = 0x13A00627; // movne r0, #0x2700000
		*(u32*)0x0205F090 = 0xE1A00000; // nop
		*(u32*)0x0205F094 = 0xE1A00000; // nop
		*(u32*)0x0205F098 = 0xE1A00000; // nop
		*(u32*)0x0205F09C = 0xE1A00000; // nop
		*(u32*)0x02061984 = 0xE1A00000; // nop
	}*/

	// Anyohaseyo!: Kankokugo Wado Pazuru (Japan)
	else if (strcmp(romTid, "KL8J") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200BEB4 = 0xE1A00000; // nop
		*(u32*)0x0200F4E0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02015108, heapEnd);
		patchUserSettingsReadDSiWare(0x02016868);
		*(u32*)0x02023B7C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02023F1C = 0xE1A00000; // nop
		setBL(0x02024128, (u32)dsiSaveClose);
		*(u32*)0x0202418C = 0xE1A00000; // nop
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
		/* if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02023BE8, 0x02016DAC);
			}
		} else { */
			*(u32*)0x020321BC = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x020398E4 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		// }
	}

	// Art Style: AQUIA (USA)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAE") == 0) {
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		// *(u32*)0x0203BB4C = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203BB50 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203BC18 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203BC1C = 0xE12FFF1E; // bx lr
		setBL(0x0203BBE4, (u32)dsiSaveOpen);
		setBL(0x0203BC08, (u32)dsiSaveClose);
		*(u32*)0x0203BC2C = 0xE1A00000; // nop
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
		*(u32*)0x02051E00 = 0xE1A00000; // nop
		*(u32*)0x02054CD8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02054CDC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020583BC = 0xE1A00000; // nop
		patchInitDSiWare(0x020649BC, heapEnd);
		patchUserSettingsReadDSiWare(0x020660BC);

		/* *(u32*)0x02064B9C = generateA7Instr(0x02064B9C, 0x020665C4); // bl 0x020665C4
		{
			*(u32*)0x020665C4 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020665C8 = 0xE3A01402; // mov r1, #0x2000000
			*(u32*)0x020665CC = 0xE3A0202A; // mov r2, #0x2A
			*(u32*)0x020665D0 = generateA7Instr(0x020665D0, 0x02065214); // bl 0x02065214
			*(u32*)0x020665D4 = 0xE59F100C; // ldr r1, =0x27FF000
			*(u32*)0x020665D8 = 0xE3A00002; // mov r0, #2
			*(u32*)0x020665DC = 0xE3A02016; // mov r2, #0x16
			*(u32*)0x020665E0 = generateA7Instr(0x020665E0, 0x02065214); // bl 0x02065214
			*(u32*)0x020665E4 = 0xE8BD8008; // LDMFD SP!, {R3,PC}
			*(u32*)0x020665E8 = 0x027FF000;
		} */
	}

	// Art Style: AQUITE (Europe, Australia)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAV") == 0) {
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		// *(u32*)0x0203BC5C = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203BC60 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203BD28 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203BD2C = 0xE12FFF1E; // bx lr
		setBL(0x0203BCF4, (u32)dsiSaveOpen);
		setBL(0x0203BD18, (u32)dsiSaveClose);
		*(u32*)0x0203BD3C = 0xE1A00000; // nop
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
		*(u32*)0x02051F10 = 0xE1A00000; // nop
		*(u32*)0x02054DE8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02054DEC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020584CC = 0xE1A00000; // nop
		patchInitDSiWare(0x02064ACC, heapEnd);
		patchUserSettingsReadDSiWare(0x020661CC);
	}

	// Art Style: AQUARIO (Japan)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KAAJ") == 0) {
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020051B8 = 0xE1A00000; // nop
		// *(u32*)0x0203E250 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203E254 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0203E324 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203E328 = 0xE12FFF1E; // bx lr
		setBL(0x0203E2F0, (u32)dsiSaveOpen);
		setBL(0x0203E314, (u32)dsiSaveClose);
		*(u32*)0x0203E334 = 0xE1A00000; // nop
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
		*(u32*)0x0205446C = 0xE1A00000; // nop
		*(u32*)0x02057344 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02057348 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0205AA28 = 0xE1A00000; // nop
		patchInitDSiWare(0x02067028, heapEnd);
	}

	// Everyday Soccer (USA)
	// DS Download Play requires 8MB of RAM
	else if (strcmp(romTid, "KAZE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x0205864C, 0x0207C314);
			}
		} else {
			*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		// *(u32*)0x0200DC9C = 0xE1A00000; // nop
		*(u32*)0x02059E0C = 0xE1A00000; // nop
		*(u32*)0x02059E14 = 0xE1A00000; // nop
		setBL(0x02059E20, (u32)dsiSaveCreate);
		setBL(0x02059E3C, (u32)dsiSaveOpen);
		setBL(0x02059E68, (u32)dsiSaveSetLength);
		setBL(0x02059E84, (u32)dsiSaveWrite);
		setBL(0x02059E90, (u32)dsiSaveClose);
		*(u32*)0x02059EB0 = 0xE1A00000; // nop
		*(u32*)0x02059EB8 = 0xE1A00000; // nop
		setBL(0x02059F2C, (u32)dsiSaveOpen);
		setBL(0x02059F9C, (u32)dsiSaveRead);
		setBL(0x02059FA8, (u32)dsiSaveClose);
		*(u32*)0x02068414 = 0xE1A00000; // nop
		*(u32*)0x0206CA14 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207A3E4, heapEnd8MBHack);
		*(u32*)0x0207A770 = 0x02299500;
		patchUserSettingsReadDSiWare(0x0207BC98);
		*(u32*)0x0207BCB4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207BCB8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BCC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207BCC4 = 0xE12FFF1E; // bx lr
	}

	// ARC Style: Everyday Football (Europe, Australia)
	// DS Download Play requires 8MB of RAM
	else if (strcmp(romTid, "KAZV") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02058720, 0x0207C3E8);
			}
		} else {
			*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		// *(u32*)0x0200DD70 = 0xE1A00000; // nop
		*(u32*)0x02059EE0 = 0xE1A00000; // nop
		*(u32*)0x02059EE8 = 0xE1A00000; // nop
		setBL(0x02059EF4, (u32)dsiSaveCreate);
		setBL(0x02059F10, (u32)dsiSaveOpen);
		setBL(0x02059F3C, (u32)dsiSaveSetLength);
		setBL(0x02059F58, (u32)dsiSaveWrite);
		setBL(0x02059F64, (u32)dsiSaveClose);
		*(u32*)0x02059F84 = 0xE1A00000; // nop
		*(u32*)0x02059F8C = 0xE1A00000; // nop
		setBL(0x0205A000, (u32)dsiSaveOpen);
		setBL(0x0205A070, (u32)dsiSaveRead);
		setBL(0x0205A07C, (u32)dsiSaveClose);
		*(u32*)0x020684E8 = 0xE1A00000; // nop
		*(u32*)0x0206CAE8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207A4B8, heapEnd8MBHack);
		*(u32*)0x0207A844 = 0x02299500;
		patchUserSettingsReadDSiWare(0x0207BD6C);
		*(u32*)0x0207BD88 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207BD8C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BD94 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207BD98 = 0xE12FFF1E; // bx lr
	}

	// ARC Style: Soccer! (Japan)
	// ARC Style: Soccer! (Korea)
	else if (strcmp(romTid, "KAZJ") == 0 || strcmp(romTid, "KAZK") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02058658, 0x0207C2C8);
			}
		} else {
			*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		// *(u32*)0x0200DD70 = 0xE1A00000; // nop
		*(u32*)0x02059DF0 = 0xE1A00000; // nop
		*(u32*)0x02059DF8 = 0xE1A00000; // nop
		setBL(0x02059E04, (u32)dsiSaveCreate);
		setBL(0x02059E20, (u32)dsiSaveOpen);
		setBL(0x02059E4C, (u32)dsiSaveSetLength);
		setBL(0x02059E68, (u32)dsiSaveWrite);
		setBL(0x02059E74, (u32)dsiSaveClose);
		*(u32*)0x02059E88 = 0xE1A00000; // nop
		*(u32*)0x02059E90 = 0xE1A00000; // nop
		setBL(0x02059F04, (u32)dsiSaveOpen);
		setBL(0x02059F74, (u32)dsiSaveRead);
		setBL(0x02059F80, (u32)dsiSaveClose);
		*(u32*)0x020683C8 = 0xE1A00000; // nop
		*(u32*)0x0206C9C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207A398, heapEnd8MBHack);
		*(u32*)0x0207A724 = 0x022993E0;
		patchUserSettingsReadDSiWare(0x0207BC4C);
		*(u32*)0x0207BC68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207BC6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207BC74 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207BC78 = 0xE12FFF1E; // bx lr
	}

	// Arcade Bowling (USA)
	else if (strcmp(romTid, "K4BE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200B774 = 0xE1A00000; // nop
		*(u32*)0x0200F3FC = 0xE1A00000; // nop
		patchInitDSiWare(0x02014DA8, heapEnd);
		patchUserSettingsReadDSiWare(0x020163A4);
		setBL(0x0201F8EC, (u32)dsiSaveOpen);
		setBL(0x0201F900, (u32)dsiSaveGetLength);
		setBL(0x0201F914, (u32)dsiSaveRead);
		setBL(0x0201F924, (u32)dsiSaveClose);
		*(u32*)0x0201F9C4 = 0xE1A00000; // nop
		setBL(0x0201F9D4, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0201F9F0, (u32)dsiSaveOpen);
		setBL(0x0201FA08, (u32)dsiSaveSetLength);
		setBL(0x0201FA18, (u32)dsiSaveWrite);
		setBL(0x0201FA20, (u32)dsiSaveClose);
		setBL(0x0201FAA0, (u32)dsiSaveOpen);
		setBL(0x0201FAB8, (u32)dsiSaveSetLength);
		setBL(0x0201FAC8, (u32)dsiSaveWrite);
		setBL(0x0201FAD4, (u32)dsiSaveClose);
		if (!useSharedFont) {
			setB(0x0202036C, 0x02020A78); // Skip Manual screen
		}
	}

	// Arcade Hoops Basketball (USA)
	else if (strcmp(romTid, "KSAE") == 0) {
		setBL(0x020066D0, (u32)dsiSaveOpen);
		setBL(0x020066E4, (u32)dsiSaveGetLength);
		setBL(0x020066F8, (u32)dsiSaveRead);
		setBL(0x02006708, (u32)dsiSaveClose);
		*(u32*)0x020067A8 = 0xE1A00000; // nop
		setBL(0x020067B8, (u32)dsiSaveCreate);
		setBL(0x020067D4, (u32)dsiSaveOpen);
		setBL(0x020067EC, (u32)dsiSaveSetLength);
		setBL(0x020067FC, (u32)dsiSaveWrite);
		setBL(0x02006804, (u32)dsiSaveClose);
		setBL(0x02006890, (u32)dsiSaveOpen);
		setBL(0x020068A8, (u32)dsiSaveSetLength);
		setBL(0x020068B8, (u32)dsiSaveWrite);
		setBL(0x020068C4, (u32)dsiSaveClose);
		setB(0x02007064, 0x02007168); // Skip Manual screen
		*(u32*)0x020249B4 = 0xE1A00000; // nop
		*(u32*)0x02027FAC = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E894, heapEnd);
		patchUserSettingsReadDSiWare(0x0202FE9C);
	}

	// Armada (USA)
	else if (strcmp(romTid, "KRDE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02010FB0 = 0xE1A00000; // nop
		*(u32*)0x02014074 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B380, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C940);
		if (!extendedMemory2) {
			*(u32*)0x02021724 = 0xE1A00000; // nop (Disable playing ingame_main.wav.adpcm)
		}
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
		*(u32*)0x020424FC = 0xE3A00000; // mov r0, #?
		*(u32*)0x020424FC += twlFontFound ? 3 : 1;
	}

	// Army Defender (USA)
	// Army Defender (Europe)
	else if (strncmp(romTid, "KAY", 3) == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200523C = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0201FDD8, 0x0204F5DC);
		}
		setBL(0x02020A28, (u32)dsiSaveCreate);
		setBL(0x02020A38, (u32)dsiSaveOpen);
		setBL(0x02020A8C, (u32)dsiSaveWrite);
		setBL(0x02020A94, (u32)dsiSaveClose);
		setBL(0x02020ADC, (u32)dsiSaveOpen);
		setBL(0x02020B08, (u32)dsiSaveGetLength);
		setBL(0x02020B18, (u32)dsiSaveRead);
		setBL(0x02020B20, (u32)dsiSaveClose);
		*(u32*)0x020426BC = 0xE1A00000; // nop
		tonccpy((u32*)0x02043360, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02045CC4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204CE40, heapEnd);
		patchUserSettingsReadDSiWare(0x0204E3E0);
	}

	// Around the World in 80 Days (USA)
	// Around the World in 80 Days (Europe, Australia)
	else if (strcmp(romTid, "K7BE") == 0 || strcmp(romTid, "K7BV") == 0) {
		*(u32*)0x0201A0DC = 0xE1A00000; // nop
		*(u32*)0x0201D620 = 0xE1A00000; // nop
		patchInitDSiWare(0x020241CC, heapEnd);
		*(u32*)0x02024558 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020258E0);
		*(u32*)0x0202ADF8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202ADFC = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
			setBL(0x02033E68, (u32)dsiSaveGetLength);
			setBL(0x02033E7C, (u32)dsiSaveSetLength);
			setBL(0x02033E90, (u32)dsiSaveSeek);
			setBL(0x02033EA0, (u32)dsiSaveWrite);
			setBL(0x02033EA8, (u32)dsiSaveClose);
			setBL(0x020342B0, (u32)dsiSaveGetLength);
			setBL(0x020342C4, (u32)dsiSaveSetLength);
			setBL(0x020342D8, (u32)dsiSaveSeek);
			setBL(0x020342E8, (u32)dsiSaveRead);
			setBL(0x020342F0, (u32)dsiSaveClose);
			setBL(0x0203438C, (u32)dsiSaveOpen);
			setBL(0x020343E0, (u32)dsiSaveCreate);
			setBL(0x02034400, (u32)dsiSaveGetResultCode);
			*(u32*)0x02034438 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020362BC = 0xE3A05702; // mov r5, #0x80000
			*(u32*)0x02036784 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02099C38 = 0xE3A00709; // mov r0, #0x240000
		} else {
			setBL(0x02033E40, (u32)dsiSaveGetLength);
			setBL(0x02033E54, (u32)dsiSaveSetLength);
			setBL(0x02033E68, (u32)dsiSaveSeek);
			setBL(0x02033E78, (u32)dsiSaveWrite);
			setBL(0x02033E80, (u32)dsiSaveClose);
			setBL(0x02034288, (u32)dsiSaveGetLength);
			setBL(0x0203429C, (u32)dsiSaveSetLength);
			setBL(0x020342B0, (u32)dsiSaveSeek);
			setBL(0x020342C0, (u32)dsiSaveRead);
			setBL(0x020342C8, (u32)dsiSaveClose);
			setBL(0x02034364, (u32)dsiSaveOpen);
			setBL(0x020343B8, (u32)dsiSaveCreate);
			setBL(0x020343D8, (u32)dsiSaveGetResultCode);
			*(u32*)0x02034410 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02036294 = 0xE3A05702; // mov r5, #0x80000
			*(u32*)0x0203675C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020990AC = 0xE3A00709; // mov r0, #0x240000
		}
	}

	// Arrow of Laputa (Japan)
	else if (strcmp(romTid, "KYAJ") == 0) {
		setBL(0x02018F5C, (u32)dsiSaveCreate);
		*(u32*)0x02018F7C = 0xE1A00000; // nop
		setBL(0x02018F88, (u32)dsiSaveCreate);
		setBL(0x02018FCC, (u32)dsiSaveOpen);
		setBL(0x02018FF8, (u32)dsiSaveSetLength);
		setBL(0x02019018, (u32)dsiSaveWrite);
		setBL(0x02019028, (u32)dsiSaveWrite);
		setBL(0x02019038, (u32)dsiSaveWrite);
		setBL(0x02019048, (u32)dsiSaveWrite);
		setBL(0x02019058, (u32)dsiSaveWrite);
		setBL(0x02019068, (u32)dsiSaveWrite);
		setBL(0x0201907C, (u32)dsiSaveWrite);
		setBL(0x02019090, (u32)dsiSaveWrite);
		setBL(0x020190A0, (u32)dsiSaveWrite);
		setBL(0x020190B0, (u32)dsiSaveWrite);
		setBL(0x020190C4, (u32)dsiSaveWrite);
		setBL(0x020190CC, (u32)dsiSaveClose);
		setBL(0x02019110, (u32)dsiSaveOpen);
		*(u32*)0x0201912C = 0xE1A00000; // nop
		setBL(0x0201913C, (u32)dsiSaveGetLength);
		setBL(0x02019150, (u32)dsiSaveRead);
		setBL(0x02019164, (u32)dsiSaveRead);
		setBL(0x0201918C, (u32)dsiSaveRead);
		setBL(0x0201919C, (u32)dsiSaveRead);
		setBL(0x020191AC, (u32)dsiSaveRead);
		setBL(0x020191BC, (u32)dsiSaveRead);
		setBL(0x020191D0, (u32)dsiSaveRead);
		setBL(0x020191E0, (u32)dsiSaveRead);
		setBL(0x020191F0, (u32)dsiSaveRead);
		setBL(0x02019200, (u32)dsiSaveRead);
		setBL(0x02019214, (u32)dsiSaveRead);
		setBL(0x0201921C, (u32)dsiSaveClose);
		/* if (!extendedMemory2 && expansionPakFound) {
			// Relocate object(?) heap to Memory Expansion Pak
			// Not working correctly, as it crashes past the title screen
			if (s2FlashcardId == 0x5A45) {
				*(u32*)0x02028688 = 0xE3A00408; // mov r0, #0x08000000
			} else {
				*(u32*)0x02028688 = 0xE3A00409; // mov r0, #0x09000000
			}
		} */
		*(u32*)0x02037120 = 0xE1A00000; // nop
		tonccpy((u32*)0x02038D64, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0203B68C = 0xE1A00000; // nop
		patchInitDSiWare(0x02042834, heapEnd);
		*(u32*)0x02042BC0 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02043CE8);
		*(u32*)0x0204C978 = 0xE3A02702; // mov r2, #0x80000
		if (!extendedMemory2) {
			// if (!expansionPakFound) {
				*(u32*)0x0204CEF8 = 0xE12FFF1E; // bx lr (Disable audio)
			// }
			// *(u32*)0x0204CF88 -= 0x20000; // Shrink sound heap from 0xE2400 to 0xC2400
		}
	}

	// Artillery: Knights vs. Orcs (Europe)
	else if (strcmp(romTid, "K9ZP") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x02018B48 = 0xE1A00000; // nop
		*(u32*)0x0201C798 = 0xE1A00000; // nop
		patchInitDSiWare(0x020236DC, heapEnd);
		patchUserSettingsReadDSiWare(0x02024CF4);
		*(u32*)0x0204603C = 0xE1A00000; // nop
		*(u32*)0x02046050 = 0xE1A00000; // nop
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

		// Skip Manual screen (Not working)
		// *(u32*)0x02073568 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// *(u32*)0x02073614 = 0xE1A00000; // nop
		// *(u32*)0x0207361C = 0xE1A00000; // nop
		// *(u32*)0x02073628 = 0xE1A00000; // nop
	}

	// Aru Seishun no Monogatari: Kouenji Joshi Sakka (Japan)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KQJJ") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08000000 : 0x09000000;

		*(u32*)0x020050C8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02005110 = 0xE1A00000; // nop (Show white screen instead of manual screen)
		*(u32*)0x0200C184 = 0xE1A00000; // nop
		*(u32*)0x0200F86C = 0xE1A00000; // nop
		patchInitDSiWare(0x02015BC4, extendedMemory2 ? heapEnd : mepAddr+0x700000);
		*(u32*)0x02015F50 = extendedMemory2 ? *(u32*)0x02004FE8 : mepAddr;
		patchUserSettingsReadDSiWare(0x0201730C);
		setBL(0x02024EC0, (u32)dsiSaveGetResultCode);
		*(u32*)0x02024FA8 = 0xE1A00000; // nop
		setBL(0x02024FE0, (u32)dsiSaveOpen);
		setBL(0x02025018, (u32)dsiSaveRead);
		setBL(0x02025040, (u32)dsiSaveClose);
		setBL(0x020250A4, (u32)dsiSaveOpen);
		setBL(0x020250F0, (u32)dsiSaveWrite);
		setBL(0x02025110, (u32)dsiSaveClose);
		setBL(0x02025158, (u32)dsiSaveCreate);
		setBL(0x020251B4, (u32)dsiSaveDelete);
		if (!extendedMemory2) {
			*(u32*)0x0202B604 = 0xE3A00000; // mov r0, #0 (Disable MobiClip playback)
		}
	}

	// Asphalt 4: Elite Racing (USA)
	// Does not boot: White screens, crash cause unknown
	/* else if (strcmp(romTid, "KA4E") == 0) {
		*(u32*)0x020050E0 = 0xE1A00000; // nop
		*(u32*)0x0200517C = 0xE1A00000; // nop
		*(u32*)0x02031E08 = 0xE1A00000; // nop
		*(u32*)0x0204FA6C = 0xE12FFF1E; // bx lr
		*(u32*)0x02052C64 = 0xE3A00000; // mov r0, #0 (Disable audio)
		*(u32*)0x0207A0B0 = 0xE3A00601; // mov r0, #0x100000 (Shrink heap for bottom screen data)
		*(u32*)0x0207ACB8 = 0xE1A00000; // nop
		*(u32*)0x0208FCC4 = 0xE1A00000; // nop
		*(u32*)0x02093E04 = 0xE1A00000; // nop
		patchInitDSiWare(0x0209A558, heapEnd);
		*(u32*)0x0209A8C8 = *(u32*)0x02004FC0 - 0x200000;
		// *(u32*)0x0209A8C8 = 0x09000000;
		patchUserSettingsReadDSiWare(0x0209BCF4);
		*(u32*)0x020A17E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020A17EC = 0xE12FFF1E; // bx lr
	} */

	// G.G. Series: Assault Buster (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KABE") == 0 && extendedMemory2) {
		*(u32*)0x0200D83C = 0xE1A00000; // nop
		*(u32*)0x0204F59C = 0xE1A00000; // nop
		*(u32*)0x02053530 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205B790, heapEnd);
		patchUserSettingsReadDSiWare(0x0205CF4C);
	}

	// G.G. Series: Assault Buster (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KABJ") == 0 && extendedMemory2) {
		*(u32*)0x0200A29C = 0xE1A00000; // nop
		*(u32*)0x020427A0 = 0xE1A00000; // nop
		*(u32*)0x020466A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204E638, heapEnd);
		patchUserSettingsReadDSiWare(0x0204FDE4);
	}

	// Astro (USA)
	else if (strcmp(romTid, "K7DE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02017498 = 0xE1A00000; // nop
		*(u32*)0x0201AE24 = 0xE1A00000; // nop
		patchInitDSiWare(0x02022FAC, heapEnd);
		setBL(0x02048AA8, (u32)dsiSaveOpenR);
		setBL(0x02048AC8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02048CC0, (u32)dsiSaveOpen);
		setBL(0x02048CD4, (u32)dsiSaveGetResultCode);
		*(u32*)0x02048CF0 = 0xE1A00000; // nop
		setBL(0x02048600, (u32)dsiSaveOpen);
		setBL(0x0204861C, (u32)dsiSaveWrite);
		setBL(0x02048628, (u32)dsiSaveClose);
		setBL(0x0204867C, (u32)dsiSaveOpen);
		setBL(0x02048690, (u32)dsiSaveGetLength);
		setBL(0x020486A4, (u32)dsiSaveRead);
		setBL(0x020486B0, (u32)dsiSaveClose);
	}

	// Atama o Yoku Suru Anzan DS: Zou no Hana Fuusen (Japan)
	else if (strcmp(romTid, "KZ3J") == 0) {
		*(u32*)0x02006FD4 = 0xE1A00000; // nop
		*(u32*)0x0200AA08 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0200AC10, (u32)dsiSaveCreate);
		setBL(0x0200AC4C, (u32)dsiSaveOpen);
		setBL(0x0200AC84, (u32)dsiSaveSetLength);
		setBL(0x0200AC94, (u32)dsiSaveWrite);
		setBL(0x0200ACAC, (u32)dsiSaveClose);
		setBL(0x0200AD34, (u32)dsiSaveOpen);
		setBL(0x0200AD6C, (u32)dsiSaveSetLength);
		setBL(0x0200AD7C, (u32)dsiSaveWrite);
		setBL(0x0200AD94, (u32)dsiSaveClose);
		setBL(0x0200AE14, (u32)dsiSaveOpen);
		setBL(0x0200AE4C, (u32)dsiSaveRead);
		setBL(0x0200AE60, (u32)dsiSaveClose);
		setBL(0x0200AEB0, (u32)dsiSaveDelete);
		setBL(0x0200AF1C, (u32)dsiSaveGetInfo);
		*(u32*)0x0200AF60 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc & dsiSaveFreeSpaceAvailable)
		*(u32*)0x0200AF64 = 0xE12FFF1E; // bx lr
		*(u32*)0x02031CFC = 0xE1A00000; // nop
		tonccpy((u32*)0x02032874, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02035A64 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203D044, heapEnd);
		patchUserSettingsReadDSiWare(0x0203E510);

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200B2C0;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// ATV Fever (USA)
	else if (strcmp(romTid, "KVUE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020151E8 = 0xE1A00000; // nop
		*(u32*)0x0201906C = 0xE1A00000; // nop
		patchInitDSiWare(0x02020B48, heapEnd);
		*(u32*)0x0202AD04 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202AD08 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x02088A74 = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02095E54, 0x020227F0);
		}
	}

	// ATV Quad Kings (USA)
	else if (strcmp(romTid, "K9UE") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x02015288 = 0xE1A00000; // nop
		*(u32*)0x0201910C = 0xE1A00000; // nop
		patchInitDSiWare(0x02020BE8, heapEnd);
		patchUserSettingsReadDSiWare(0x0202237C);
		*(u32*)0x0202AE28 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202AE2C = 0xE12FFF1E; // bx lr
		setBL(0x0205D1D8, (u32)dsiSaveOpenR);
		*(u32*)0x0205D1FC = (u32)dsiSaveCreate; // dsiSaveCreateAuto
		setBL(0x0205D228, (u32)dsiSaveOpen);
		setBL(0x0205D23C, (u32)dsiSaveGetResultCode);
		*(u32*)0x0205D24C = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KSRE") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005164 = 0xE1A00000; // nop
		*(u32*)0x020104A0 = 0xE1A00000; // nop
		*(u32*)0x02010508 = 0xE1A00000; // nop
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
		*(u32*)0x0203F500 = 0xE1A00000; // nop
		*(u32*)0x02042F10 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204B1F0, heapEnd);
		patchUserSettingsReadDSiWare(0x0204C52C);
	}

	// Aura-Aura Climber (Europe, Australia)
	else if (strcmp(romTid, "KSRV") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005164 = 0xE1A00000; // nop
		*(u32*)0x0201066C = 0xE1A00000; // nop
		*(u32*)0x020106D4 = 0xE1A00000; // nop
		*(u32*)0x020265A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203F580 = 0xE1A00000; // nop
		*(u32*)0x02042F90 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204B270, heapEnd);
		patchUserSettingsReadDSiWare(0x0204C5AC);
	}

	// Sukai Janpa Soru (Japan)
	else if (strcmp(romTid, "KSRJ") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02005164 = 0xE1A00000; // nop
		*(u32*)0x02010498 = 0xE1A00000; // nop
		*(u32*)0x02010500 = 0xE1A00000; // nop
		*(u32*)0x02026848 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203F4B0 = 0xE1A00000; // nop
		*(u32*)0x02042EC0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204B1A0, heapEnd);
		patchUserSettingsReadDSiWare(0x0204C4DC);
	}

	// Ball Fighter (USA)
	else if (strcmp(romTid, "KBOE") == 0) {
		setBL(0x0200C710, (u32)dsiSaveOpen);
		setBL(0x0200C724, (u32)dsiSaveClose);
		setBL(0x0200C744, (u32)dsiSaveCreate);
		setBL(0x0200C75C, (u32)dsiSaveOpen);
		setBL(0x0200C774, (u32)dsiSaveClose);
		setBL(0x0200C77C, (u32)dsiSaveDelete);
		setBL(0x0200C904, (u32)dsiSaveOpen);
		setBL(0x0200C91C, (u32)dsiSaveGetLength);
		setBL(0x0200C940, (u32)dsiSaveRead);
		setBL(0x0200C948, (u32)dsiSaveClose);
		*(u32*)0x0200C9AC = 0xE1A00000; // nop
		*(u32*)0x0200C9B4 = 0xE1A00000; // nop
		setBL(0x0200C9D0, (u32)dsiSaveOpen);
		setBL(0x0200C9E4, (u32)dsiSaveClose);
		setBL(0x0200C9F8, (u32)dsiSaveCreate);
		setBL(0x0200CA10, (u32)dsiSaveOpen);
		*(u32*)0x0200CA20 = 0xE1A00000; // nop
		setBL(0x0200CA2C, (u32)dsiSaveClose);
		setBL(0x0200CA34, (u32)dsiSaveDelete);
		setBL(0x0200CA48, (u32)dsiSaveCreate);
		setBL(0x0200CA58, (u32)dsiSaveOpen);
		setBL(0x0200CA68, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200CA70 = 0xE1A00000; // nop
		*(u32*)0x0200CA84 = 0xE1A00000; // nop
		setBL(0x0200CA9C, (u32)dsiSaveSetLength);
		setBL(0x0200CAAC, (u32)dsiSaveWrite);
		setBL(0x0200CAB4, (u32)dsiSaveClose);
		*(u32*)0x0200CABC = 0xE1A00000; // nop
		*(u32*)0x0200CAD0 = 0xE1A00000; // nop
		*(u32*)0x0200CAEC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x0203A6DC = 0xE1A00000; // nop
		*(u32*)0x0203DA9C = 0xE1A00000; // nop
		patchInitDSiWare(0x02043430, heapEndExceed);
		patchUserSettingsReadDSiWare(0x02044A74);
	}

	// Ball Fighter (Europe)
	else if (strcmp(romTid, "KBOP") == 0) {
		*(u32*)0x0200C288 = 0xE1A00000; // nop
		*(u32*)0x0200C290 = 0xE1A00000; // nop
		setBL(0x0200C770, (u32)dsiSaveOpen);
		setBL(0x0200C784, (u32)dsiSaveClose);
		setBL(0x0200C7A4, (u32)dsiSaveCreate);
		setBL(0x0200C7BC, (u32)dsiSaveOpen);
		setBL(0x0200C7D4, (u32)dsiSaveClose);
		setBL(0x0200C7DC, (u32)dsiSaveDelete);
		setBL(0x0200C964, (u32)dsiSaveOpen);
		setBL(0x0200C97C, (u32)dsiSaveGetLength);
		setBL(0x0200C9A0, (u32)dsiSaveRead);
		setBL(0x0200C9A8, (u32)dsiSaveClose);
		setBL(0x0200C9E4, (u32)dsiSaveOpen);
		setBL(0x0200C9F8, (u32)dsiSaveClose);
		setBL(0x0200CA0C, (u32)dsiSaveCreate);
		setBL(0x0200CA24, (u32)dsiSaveOpen);
		*(u32*)0x0200CA34 = 0xE1A00000; // nop
		setBL(0x0200CA40, (u32)dsiSaveClose);
		setBL(0x0200CA48, (u32)dsiSaveDelete);
		setBL(0x0200CA5C, (u32)dsiSaveCreate);
		setBL(0x0200CA6C, (u32)dsiSaveOpen);
		setBL(0x0200CA7C, (u32)dsiSaveGetResultCode);
		setBL(0x0200CA94, (u32)dsiSaveSetLength);
		setBL(0x0200CAA4, (u32)dsiSaveWrite);
		setBL(0x0200CAAC, (u32)dsiSaveClose);
		*(u32*)0x0200CABC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x0203A82C = 0xE1A00000; // nop
		*(u32*)0x0203DA8C = 0xE1A00000; // nop
		patchInitDSiWare(0x0204317C, heapEndExceed);
		patchUserSettingsReadDSiWare(0x020447BC);
	}

	// Art Style: BASE 10 (USA)
	else if (strcmp(romTid, "KADE") == 0) {
		*getOffsetFromBL((u32*)0x020074A8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074BC) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074D8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074E8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x02007500) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020075C4) = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D25C = 0xEB00007C; // bl 0x0202D454 (Skip Manual screen)
		// *(u32*)0x0202D2EC = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0202D314 = 0xE3A00000; // mov r0, #0
		setBL(0x0203A248, (u32)dsiSaveOpen);
		setBL(0x0203A26C, (u32)dsiSaveClose);
		*(u32*)0x0203A288 = 0xE3A00000; // mov r0, #0
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
		*(u32*)0x02056724 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02059F88 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064FC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020668F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020668FC = 0xE12FFF1E; // bx lr
		*(u32*)0x02066E50 = 0xE1A00000; // nop
		*(u32*)0x02066E54 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066E6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066FB4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067050 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067080 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02067154 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067184 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0206865C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020686B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068A98 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02070068 = 0xE3A00000; // mov r0, #0
	}

	// Art Style: CODE (Europe, Australia)
	else if (strcmp(romTid, "KADV") == 0) {
		*getOffsetFromBL((u32*)0x020074A8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074BC) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074D8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074E8) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x02007500) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020075C4) = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D288 = 0xEB00007C; // bl 0x0202D480 (Skip Manual screen)
		setBL(0x0203A2D8, (u32)dsiSaveOpen);
		setBL(0x0203A2FC, (u32)dsiSaveClose);
		*(u32*)0x0203A318 = 0xE3A00000; // mov r0, #0
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
		*(u32*)0x020567B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205A018 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02065050 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066988 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206698C = 0xE12FFF1E; // bx lr
		*(u32*)0x02066EE0 = 0xE1A00000; // nop
		*(u32*)0x02066EE4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02066EFC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067044 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020670E0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067110 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020671E4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067214 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020686EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068740 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068B28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020700F8 = 0xE3A00000; // mov r0, #0
	}

	// Art Style: DECODE (Japan)
	else if (strcmp(romTid, "KADJ") == 0) {
		*getOffsetFromBL((u32*)0x020074B0) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074C4) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074E0) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020074F0) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x02007508) = 0xE12FFF1E; // bx lr
		*getOffsetFromBL((u32*)0x020075C8) = 0xE12FFF1E; // bx lr
		*(u32*)0x0202E2AC = 0xEB000071; // bl 0x0202E478 (Skip Manual screen)
		setBL(0x0203B108, (u32)dsiSaveOpen);
		setBL(0x0203B12C, (u32)dsiSaveClose);
		*(u32*)0x0203B148 = 0xE3A00000; // mov r0, #0
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
		*(u32*)0x020575B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205AE14 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067784 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02067788 = 0xE12FFF1E; // bx lr
		*(u32*)0x02067CDC = 0xE1A00000; // nop
		*(u32*)0x02067CE0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067CF8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067E40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067EDC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067F0C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02067FE0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02068010 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02069480 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020694C0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02070E08 = 0xE3A00000; // mov r0, #0
	}

	// Beauty Academy (Europe)
	else if (strcmp(romTid, "K8BP") == 0) {
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02018B48 = 0xE1A00000; // nop
		*(u32*)0x0201C8C4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02023664, heapEnd);
		patchUserSettingsReadDSiWare(0x02024DC0);
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

		// Skip Manual screen (Not working)
		// *(u32*)0x02092FDC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// *(u32*)0x02093070 = 0xE1A00000; // nop
		// *(u32*)0x02093078 = 0xE1A00000; // nop
		// *(u32*)0x02093084 = 0xE1A00000; // nop
	}

	// Bejeweled Twist (USA)
	else if (strcmp(romTid, "KBEE") == 0) {
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
		doubleNopT(0x02036904);
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
		doubleNopT(0x020945FA);
		doubleNopT(0x02097682);
		doubleNopT(0x0209B390);
		doubleNopT(0x0209CA52);
		doubleNopT(0x0209CA56);
		doubleNopT(0x0209CA62);
		doubleNopT(0x0209CB46);
		patchHiHeapDSiWareThumb(0x0209CB84, 0x0209A2F0, heapEnd);
		*(u32*)0x0209CC5C = 0x0210E1C0;
		patchUserSettingsReadDSiWare(0x0209D80E);
		*(u16*)0x0209D828 = 0x2001; // movs r0, #1
		*(u16*)0x0209D82A = 0x4770; // bx lr
		*(u16*)0x0209D834 = 0x2000; // movs r0, #0
		*(u16*)0x0209D836 = 0x4770; // bx lr
	}

	// Bejeweled Twist (Europe, Australia)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBEV") == 0 && extendedMemory2) {
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

		doubleNopT(0x0200910C);
		doubleNopT(0x020091F2);
		doubleNopT(0x0203601E); // dsiSaveCreateDirAuto
		setBLThumb(0x02036026, dsiSaveCreateT); // dsiSaveCreateAuto
		setBLThumb(0x02036030, dsiSaveOpenT);
		//setBLThumb(0x0203603A, dsiSaveGetResultCodeT);
		doubleNopT(0x02036044);
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
		doubleNopT(0x0209324E);
		doubleNopT(0x02096246);
		doubleNopT(0x02099F74);
		doubleNopT(0x0209B63E);
		doubleNopT(0x0209B642);
		doubleNopT(0x0209B64E);
		doubleNopT(0x0209B732);
		patchHiHeapDSiWareThumb(0x0209B770, 0x02098EC0, heapEnd); // movs r0, #0x2700000
		*(u32*)0x0209B848 = 0x0212B7E0;
		patchUserSettingsReadDSiWare(0x0209C366);
		*(u16*)0x0209C384 = 0x2001; // movs r0, #1
		*(u16*)0x0209C386 = 0x4770; // bx lr
		*(u16*)0x0209C38C = 0x2000; // movs r0, #0
		*(u16*)0x0209C38E = 0x4770; // bx lr
	}

	// Big Bass Arcade (USA)
	// Crashes later on retail consoles
	else if (strcmp(romTid, "K9GE") == 0) {
		*(u32*)0x02005120 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200D83C = 0xE1A00000; // nop
		tonccpy((u32*)0x0200E3C0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02010DDC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201865C, heapEnd);
		*(u32*)0x020189E8 = 0x022F2220;
		patchUserSettingsReadDSiWare(0x02019AFC);
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

	// Bird & Beans (USA)
	// Difficult to get working
	/*else if (strcmp(romTid, "KP6E") == 0) {
		doubleNopT(0x0200509E);
		doubleNopT(0x020050A2);
		doubleNopT(0x020050AC);
		doubleNopT(0x02005254);
		*(u16*)0x0205F52 = 0x2001; // movs r0, #1
		*(u16*)0x0205F54 = nopT;
		doubleNopT(0x02005F74);
		*(u16*)0x0205F90 = 0x2001; // movs r0, #1
		*(u16*)0x0205F92 = nopT;
		doubleNopT(0x0200DC4E);
		//doubleNopT(0x0200EDCE);
		//doubleNopT(0x0200EDDE);
		doubleNopT(0x02020A9E);
		doubleNopT(0x02022EFA);
		doubleNopT(0x0202640C);
		doubleNopT(0x02027B7A);
		doubleNopT(0x02027B7E);
		doubleNopT(0x02027B8A);
		doubleNopT(0x02027C6E);
		patchHiHeapDSiWareThumb(0x02027CAC, 0x020256D8, heapEnd); // movs r0, #0x23E0000
		*(u16*)0x02028A30 = 0xBD10; // POP {R4,PC}
		doubleNopT(0x02028D26);
		doubleNopT(0x02028D2A);
		doubleNopT(0x02028D2E);
		doubleNopT(0x0202AD82);
	}*/

	// Tori to Mame (Japan)
	else if (strcmp(romTid, "KP6J") == 0) {
		// useSharedFont = twlFontFound;
		*(u32*)0x0200D17C = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		*(u32*)0x02017694 = 0xE3A00001; // mov r0, #1 (Enable TWL soft-reset function)
		/* if (useSharedFont) {
			*(u16*)0x0200796C = 0x2001; // movs r0, #1
			*(u16*)0x02007970 = 0x2001; // movs r0, #1
			*(u16*)0x0200797C = 0x2001; // movs r0, #1
			*(u16*)0x02007980 = 0x2001; // movs r0, #1
			*(u16*)0x0200798A = 0x2001; // movs r0, #1
			*(u16*)0x02007990 = 0x2001; // movs r0, #1
			*(u16*)0x02007994 = 0x2001; // movs r0, #1
			*(u32*)0x02015928 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020217C8 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020217F0 = 0xE3A00001; // mov r0, #1
		} else { */
			*(u32*)0x020217B0 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
			*(u32*)0x02021954 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		// }
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
		*(u32*)0x020302A0 = 0xE12FFF1E; // bx lr (Hide volume icon in pause menu)
	}

	// BlayzBloo: Super Melee Brawlers Battle Royale (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBZE") == 0 && extendedMemory2) {
		*(u32*)0x0206B93C = 0xE1A00000; // nop
		*(u32*)0x0206F438 = 0xE1A00000; // nop
		patchInitDSiWare(0x02077594, heapEnd);
		patchUserSettingsReadDSiWare(0x02078A40);
		*(u32*)0x02078A5C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02078A60 = 0xE12FFF1E; // bx lr
		*(u32*)0x02078A68 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02078A6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207D764 = 0xE3A00003; // mov r0, #3
	}

	// BlayzBloo: Batoru x Batoru (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBZJ") == 0 && extendedMemory2) {
		*(u32*)0x0206B810 = 0xE1A00000; // nop
		*(u32*)0x0206F30C = 0xE1A00000; // nop
		patchInitDSiWare(0x02077468, heapEnd);
		patchUserSettingsReadDSiWare(0x02078914);
		*(u32*)0x02078930 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02078934 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207893C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02078940 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207D628 = 0xE3A00003; // mov r0, #3
	}

	// BlayzBloo: Baeteul x Baeteul (Korea)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KBZK") == 0 && extendedMemory2) {
		*(u32*)0x0206B718 = 0xE1A00000; // nop
		*(u32*)0x0206F214 = 0xE1A00000; // nop
		patchInitDSiWare(0x02077370, heapEnd);
		patchUserSettingsReadDSiWare(0x0207881C);
		*(u32*)0x02078838 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207883C = 0xE12FFF1E; // bx lr
		*(u32*)0x02078844 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02078848 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207D540 = 0xE3A00003; // mov r0, #3
	}

	// Blockado: Puzzle Island (USA)
	// Locks up on black screens (Cause unknown)
	// Requires 8MB of RAM
	/*else if (strcmp(romTid, "KZ4E") == 0 && extendedMemory2) {
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

		*(u32*)0x02015040 = 0xE1A00000; // nop
		*(u32*)0x020186D0 = 0xE1A00000; // nop
		*(u32*)0x0201C5CC = 0xE1A00000; // nop
		patchInitDSiWare(0x02024F50, heapEnd);
		patchUserSettingsReadDSiWare(0x020264C0);
		*(u32*)0x02026C50 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02026C54 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202B608 = 0xE1A00000; // nop
		*(u16*)0x0202DFB8 = 0x4770; // bx lr (Skip NFTR font rendering)
		*(u16*)0x0202E1F0 = 0x4770; // bx lr (Skip NFTR font rendering)
		*(u16*)0x0202E504 = 0x4770; // bx lr (Skip NFTR font rendering)
		*(u16*)0x0202F928 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x0203CD5C);
		*(u32*)0x0203DAE4 = 0xE12FFF1E; // bx lr
		*(u16*)0x0204B0C0 = 0x2001; // movs r0, #1 (dsiSaveGetArcSrc)
		*(u16*)0x0204B0C2 = nopT;
		setBLThumb(0x0204B0E6, dsiSaveGetInfoT);
		doubleNopT(0x0204B12E);
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
	}*/

	// Bloons (USA)
	/*else if (strcmp(romTid, "KBLE") == 0) {
		*(u32*)0x0206A808 = 0xE1A00000; // nop
		*(u32*)0x0206A810 = 0xE1A00000; // nop
		*(u32*)0x0206A860 = 0xE12FFF1E; // bx lr
		setBL(0x0206AC38, (u32)dsiSaveOpenR);
		setBL(0x0206ACFC, (u32)dsiSaveGetLength);
		setBL(0x0206AD68, (u32)dsiSaveClose);
		setBL(0x0206B040, (u32)dsiSaveSeek);
		setBL(0x0206B0A0, (u32)dsiSaveRead);
		setBL(0x0206B104, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0206B1BC, (u32)dsiSaveWrite);
		setBL(0x0206B228, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0206B318, (u32)dsiSaveCreate);
		setBL(0x0206B3C0, (u32)dsiSaveOpen);
		setBL(0x0206B45C, (u32)dsiSaveSetLength);
		setBL(0x0206B50C, (u32)dsiSaveOpen);
		setBL(0x0206B5C8, (u32)dsiSaveOpen);
		setBL(0x0206B668, (u32)dsiSaveSeek);
		setBL(0x0206B6A8, (u32)dsiSaveOpen);
		setBL(0x0206B748, (u32)dsiSaveSeek);
		setBL(0x0206B784, (u32)dsiSaveOpen);
		setBL(0x0206B824, (u32)dsiSaveSeek);
		setBL(0x0206B840, (u32)dsiSaveGetLength);
		setBL(0x0206B874, (u32)dsiSaveOpen);
		setBL(0x0206B914, (u32)dsiSaveSeek);
		setBL(0x0206B930, (u32)dsiSaveGetLength);
		setBL(0x0206BF34, (u32)dsiSaveDelete);
		*(u32*)0x0206C060 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0206C078 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0206C09C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207569C = 0xE1A00000; // nop
		tonccpy((u32*)0x020771C0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02079EB0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02080EC0, heapEnd);
		*(u32*)0x0208124C = 0x020E0380;
		patchUserSettingsReadDSiWare(0x020820F4);
	}*/

	// Bloons TD (USA)
	// Bloons TD (Europe)
	// A weird bug is preventing save support
	// Audio is disabled on retail consoles
	else if (strcmp(romTid, "KLNE") == 0 || strcmp(romTid, "KLNP") == 0) {
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050E0 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop (Work around save-related crash)
		*(u32*)0x02014304 = 0xE1A00000; // nop
		//tonccpy((u32*)0x02014E88, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02017C38 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D718, heapEnd);
		*(u32*)0x0201DAA4 -= 0x30000;
		if (!extendedMemory2) {
			*(u32*)0x0201DAA4 -= 0x1B3740;
		}
		patchUserSettingsReadDSiWare(0x0201ED00);
		if (romTid[3] == 'E') {
			/* *(u32*)0x0205EDAC = 0xE3A00000; // mov r0, #0
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
			if (!extendedMemory2) {
				*(u32*)0x0205FDA0 = 0x4000; // New sound heap size (Disables audio)
			}
		} else {
			if (!extendedMemory2) {
				*(u32*)0x0205FDC0 = 0x4000; // New sound heap size (Disables audio)
			}
		}
	}

	// Bloons TD 4 (USA)
	// Requires 8MB of RAM
	// Audio is disabled
	else if (strcmp(romTid, "KUVE") == 0 && extendedMemory2) {
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x0201C584 = 0xE1A00000; // nop
		*(u32*)0x0201FF40 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025A08, heapEnd);
		*(u32*)0x02025D94 = 0x0225F380;
		patchUserSettingsReadDSiWare(0x0202700C);
		*(u32*)0x020AE2DC = 0x4000; // New sound heap size (Disables audio)
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
	// Requires 8MB of RAM
	// Audio is disabled
	else if (strcmp(romTid, "KUVP") == 0 && extendedMemory2) {
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x0201C5B4 = 0xE1A00000; // nop
		*(u32*)0x0201FF70 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025A38, heapEnd);
		*(u32*)0x02025DC4 = 0x0225F7E0;
		patchUserSettingsReadDSiWare(0x0202703C);
		*(u32*)0x020AE74C = 0x4000; // New sound heap size (Disables audio)
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

	// Boardwalk Ball Toss (USA)
	else if (strcmp(romTid, "KA5E") == 0) {
		*(u32*)0x020052C4 = 0xE1A00000; // nop
		*(u32*)0x020052CC = 0xE1A00000; // nop
		*(u32*)0x0200E250 = 0xE1A00000; // nop
		*(u32*)0x02011DA8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017088, heapEnd);
		patchUserSettingsReadDSiWare(0x02018690);
		setBL(0x02023BD4, (u32)dsiSaveOpen);
		setBL(0x02023BF4, (u32)dsiSaveGetLength);
		setBL(0x02023C08, (u32)dsiSaveRead);
		setBL(0x02023C20, (u32)dsiSaveClose);
		*(u32*)0x02023CFC = 0xE1A00000; // nop
		setBL(0x02023D0C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02023D2C, (u32)dsiSaveOpen);
		setBL(0x02023D4C, (u32)dsiSaveSetLength);
		setBL(0x02023D60, (u32)dsiSaveWrite);
		setBL(0x02023D6C, (u32)dsiSaveClose);
		*(u32*)0x02023DA0 = 0xE1A00000; // nop
		setBL(0x02023E54, (u32)dsiSaveOpen);
		setBL(0x02023E74, (u32)dsiSaveSetLength);
		setBL(0x02023E88, (u32)dsiSaveWrite);
		setBL(0x02023E94, (u32)dsiSaveClose);
		setB(0x02025308, 0x02025400); // Skip Manual screen
	}

	// Bomberman Blitz (USA)
	// Bomberman Blitz (Europe, Australia)
	// Itsudemo Bomberman (Japan)
	else if (strncmp(romTid, "KBB", 3) == 0) {
		*(u32*)0x02008988 = 0xE1A00000; // nop
		tonccpy((u32*)0x02009670, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200C280 = 0xE1A00000; // nop
		patchInitDSiWare(0x02014638, heapEnd);
		patchUserSettingsReadDSiWare(0x02015B60);
		*(u32*)0x02015B88 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015B8C = 0xE12FFF1E; // bx lr
		*(u32*)0x02015B94 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015B98 = 0xE12FFF1E; // bx lr
		*(u32*)0x02015BB8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015BBC = 0xE12FFF1E; // bx lr
		*(u32*)0x02015BCC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02015BD0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02015BDC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02015BE0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201B8BC = 0xE3A00003; // mov r0, #3
		if (romTid[3] == 'E') {
			*(u32*)0x0204351C = 0xE1A00000; // nop
			// *(u32*)0x02043528 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			// *(u32*)0x020437AC = 0xE3A00001; // mov r0, #1
			// *(u32*)0x020437B0 = 0xE12FFF1E; // bx lr
			*(u32*)0x020437B4 = 0xE1A00000; // nop
			*(u32*)0x020437D0 = 0xE1A00000; // nop
			*(u32*)0x020437D8 = 0xE1A00000; // nop
			*(u32*)0x020437F0 = 0xE1A00000; // nop
			*(u32*)0x02043814 = 0xE1A00000; // nop
			setBL(0x02043950, (u32)dsiSaveOpen);
			setBL(0x020439D0, (u32)dsiSaveCreate);
			setBL(0x02043A5C, (u32)dsiSaveWrite);
			setBL(0x02043A70, (u32)dsiSaveClose);
			setBL(0x02043AE8, (u32)dsiSaveClose);
			setBL(0x02046394, (u32)dsiSaveOpen);
			setBL(0x02046428, (u32)dsiSaveRead);
			setBL(0x0204649C, (u32)dsiSaveClose);
			*(u32*)0x02085158 = 0xE3A00000; // mov r0, #0
			setBL(0x0208523C, 0x020867FC);
			setBL(0x020852F0, 0x02086930);
			setBL(0x020853A8, 0x0208699C);
			setBL(0x02085624, 0x02086AA4);
			setBL(0x02085704, 0x02086B54);
			setBL(0x02085844, 0x02086BC0);
			setBL(0x02085974, 0x02086D6C);
			*(u32*)0x02085D40 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02085D70 = 0xE3A00000; // mov r0, #0
			setBL(0x020864E0, 0x02086CA8);
			*(u32*)0x0208DF10 = 0xE1A00000; // nop
			*(u32*)0x0208DF18 = 0xE3A00001; // mov r0, #1
			setB(0x0208EF04, 0x0208EFEC);
			*(u32*)0x0208EFEC = 0xE1A00000; // nop
			*(u32*)0x0208EFF0 = 0xE1A00000; // nop
			*(u32*)0x0208EFF8 = 0xE1A00000; // nop
			*(u32*)0x0208EFFC = 0xE1A00000; // nop
			*(u32*)0x0208F000 = 0xE1A00000; // nop
			*(u32*)0x0208F004 = 0xE1A00000; // nop
			setB(0x0208F828, 0x0208F8C8);
			*(u32*)0x0208FA34 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FA38 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FA90 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FA94 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FB94 = 0xE1A00000; // nop
			setB(0x02090938, 0x02090AE4);
			*(u32*)0x020911B8 = 0xE1A00000; // nop
			*(u32*)0x020911BC = 0xE1A00000; // nop
			*(u32*)0x020911C0 = 0xE1A00000; // nop
			*(u32*)0x020911C4 = 0xE1A00000; // nop
			*(u32*)0x020911C8 = 0xE1A00000; // nop
			*(u32*)0x020911CC = 0xE1A00000; // nop
			*(u32*)0x020911D0 = 0xE1A00000; // nop
			*(u32*)0x020911D4 = 0xE1A00000; // nop
			*(u32*)0x020911D8 = 0xE1A00000; // nop
			setB(0x02092560, 0x0209257C);
			setB(0x020927CC, 0x020927F4);
			*(u32*)0x020927F4 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
			*(u32*)0x020927F8 += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
			*(u32*)0x020927FC = 0xE3A00000; // mov r0, #0
			*(u32*)0x02092800 = 0xE5850030; // str r0, [r5,#0x30]
			*(u32*)0x02092804 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
			*(u32*)0x020AAFD0 = 0xE3A02C07; // mov r2, #0x700
			*(u32*)0x020AAFF0 = 0xE2840B01; // add r0, r4, #0x400
			*(u32*)0x020AAFF8 = 0xE1A00004; // mov r0, r4
			*(u32*)0x020AB000 = 0xE1A00000; // nop
			*(u32*)0x020AB004 = 0xE1A00000; // nop
			*(u32*)0x020AB008 = 0xE1A00000; // nop
			*(u32*)0x020AB00C = 0xE1A00000; // nop
			*(u32*)0x020AB010 = 0xE1A00000; // nop
			*(u32*)0x020AB024 = 0xE2841B01; // add r1, r4, #0x400
		} else if (romTid[3] == 'V') {
			*(u32*)0x020435E8 = 0xE1A00000; // nop
			// *(u32*)0x020435F4 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			// *(u32*)0x02043878 = 0xE3A00001; // mov r0, #1
			// *(u32*)0x0204387C = 0xE12FFF1E; // bx lr
			*(u32*)0x02043880 = 0xE1A00000; // nop
			*(u32*)0x0204389C = 0xE1A00000; // nop
			*(u32*)0x020438A4 = 0xE1A00000; // nop
			*(u32*)0x020438BC = 0xE1A00000; // nop
			*(u32*)0x020438E0 = 0xE1A00000; // nop
			setBL(0x02043A1C, (u32)dsiSaveOpen);
			setBL(0x02043A9C, (u32)dsiSaveCreate);
			setBL(0x02043B28, (u32)dsiSaveWrite);
			setBL(0x02043B28, (u32)dsiSaveClose);
			setBL(0x02043BB4, (u32)dsiSaveClose);
			setBL(0x02046460, (u32)dsiSaveOpen);
			setBL(0x020464F4, (u32)dsiSaveRead);
			setBL(0x02046568, (u32)dsiSaveClose);
			*(u32*)0x02085254 = 0xE3A00000; // mov r0, #0
			setBL(0x02085338, 0x020868F8);
			setBL(0x020853EC, 0x02086A2C);
			setBL(0x020854A4, 0x02086A98);
			setBL(0x02085720, 0x02086BA0);
			setBL(0x02085800, 0x02086C50);
			setBL(0x02085940, 0x02086CBC);
			setBL(0x02085A70, 0x02086E68);
			*(u32*)0x02085E3C = 0xE3A00001; // mov r0, #1
			*(u32*)0x02085E6C = 0xE3A00000; // mov r0, #0
			setBL(0x020865DC, 0x02086DA4);
			*(u32*)0x0208E00C = 0xE1A00000; // nop
			*(u32*)0x0208E014 = 0xE3A00001; // mov r0, #1
			setB(0x0208F000, 0x0208F0E8);
			*(u32*)0x0208F0E8 = 0xE1A00000; // nop
			*(u32*)0x0208F0EC = 0xE1A00000; // nop
			*(u32*)0x0208F0F4 = 0xE1A00000; // nop
			*(u32*)0x0208F0F8 = 0xE1A00000; // nop
			*(u32*)0x0208F0FC = 0xE1A00000; // nop
			*(u32*)0x0208F100 = 0xE1A00000; // nop
			setB(0x0208F924, 0x0208F9C4);
			*(u32*)0x0208FB30 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FB34 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FB8C = 0xE3A00001; // mov r0, #1
			*(u32*)0x0208FB90 = 0xE12FFF1E; // bx lr
			*(u32*)0x0208FC90 = 0xE1A00000; // nop
			setB(0x02090A34, 0x02090BE0);
			*(u32*)0x020912B4 = 0xE1A00000; // nop
			*(u32*)0x020912B8 = 0xE1A00000; // nop
			*(u32*)0x020912BC = 0xE1A00000; // nop
			*(u32*)0x020912C0 = 0xE1A00000; // nop
			*(u32*)0x020912C4 = 0xE1A00000; // nop
			*(u32*)0x020912C8 = 0xE1A00000; // nop
			*(u32*)0x020912CC = 0xE1A00000; // nop
			*(u32*)0x020912D0 = 0xE1A00000; // nop
			*(u32*)0x020912D4 = 0xE1A00000; // nop
			setB(0x0209265C, 0x02092678);
			setB(0x020928C8, 0x020928F0);
			*(u32*)0x020928F0 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
			*(u32*)0x020928F4 += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
			*(u32*)0x020928F8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020928FC = 0xE5850030; // str r0, [r5,#0x30]
			*(u32*)0x02092900 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
			*(u32*)0x020AB0CC = 0xE3A02C07; // mov r2, #0x700
			*(u32*)0x020AB0EC = 0xE2840B01; // add r0, r4, #0x400
			*(u32*)0x020AB0F4 = 0xE1A00004; // mov r0, r4
			*(u32*)0x020AB0FC = 0xE1A00000; // nop
			*(u32*)0x020AB100 = 0xE1A00000; // nop
			*(u32*)0x020AB104 = 0xE1A00000; // nop
			*(u32*)0x020AB108 = 0xE1A00000; // nop
			*(u32*)0x020AB10C = 0xE1A00000; // nop
			*(u32*)0x020AB120 = 0xE2841B01; // add r1, r4, #0x400
		} else if (romTid[3] == 'J') {
			*(u32*)0x02043248 = 0xE1A00000; // nop
			// *(u32*)0x02043254 = 0xE3A00000; // mov r0, #0 (Skip WiFi error screen)
			// *(u32*)0x020434D8 = 0xE3A00001; // mov r0, #1
			// *(u32*)0x020434DC = 0xE12FFF1E; // bx lr
			*(u32*)0x020434E0 = 0xE1A00000; // nop
			*(u32*)0x020434FC = 0xE1A00000; // nop
			*(u32*)0x02043504 = 0xE1A00000; // nop
			*(u32*)0x0204351C = 0xE1A00000; // nop
			*(u32*)0x02043540 = 0xE1A00000; // nop
			setBL(0x0204367C, (u32)dsiSaveOpen);
			setBL(0x020436FC, (u32)dsiSaveCreate);
			setBL(0x02043788, (u32)dsiSaveWrite);
			setBL(0x0204379C, (u32)dsiSaveClose);
			setBL(0x02043814, (u32)dsiSaveClose);
			setBL(0x020460C0, (u32)dsiSaveOpen);
			setBL(0x02046154, (u32)dsiSaveRead);
			setBL(0x020461C8, (u32)dsiSaveClose);
			*(u32*)0x0207EEFC = 0xE3A00000; // mov r0, #0
			setBL(0x0207EFE0, 0x020805A0);
			setBL(0x0207F094, 0x020806D4);
			setBL(0x0207F14C, 0x02080740);
			setBL(0x0207F3C8, 0x02080848);
			setBL(0x0207F4A8, 0x020808F8);
			setBL(0x0207F5E8, 0x02080964);
			setBL(0x0207F718, 0x02080B10);
			*(u32*)0x0207FAE4 = 0xE3A00001; // mov r0, #1
			*(u32*)0x0207FB14 = 0xE3A00000; // mov r0, #0
			setBL(0x02080284, 0x02080A4C);
			*(u32*)0x02087CB4 = 0xE1A00000; // nop
			*(u32*)0x02087CBC = 0xE3A00001; // mov r0, #1
			setB(0x02088CA8, 0x02088D90);
			*(u32*)0x02088D90 = 0xE1A00000; // nop
			*(u32*)0x02088D94 = 0xE1A00000; // nop
			*(u32*)0x02088D9C = 0xE1A00000; // nop
			*(u32*)0x02088DA0 = 0xE1A00000; // nop
			*(u32*)0x02088DA4 = 0xE1A00000; // nop
			*(u32*)0x02088DA8 = 0xE1A00000; // nop
			setB(0x020895CC, 0x0208966C);
			*(u32*)0x020897D8 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020897DC = 0xE12FFF1E; // bx lr
			*(u32*)0x02089834 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02089838 = 0xE12FFF1E; // bx lr
			*(u32*)0x02089938 = 0xE1A00000; // nop
			setB(0x0208A6DC, 0x0208A888);
			*(u32*)0x0208AF5C = 0xE1A00000; // nop
			*(u32*)0x0208AF60 = 0xE1A00000; // nop
			*(u32*)0x0208AF64 = 0xE1A00000; // nop
			*(u32*)0x0208AF68 = 0xE1A00000; // nop
			*(u32*)0x0208AF6C = 0xE1A00000; // nop
			*(u32*)0x0208AF70 = 0xE1A00000; // nop
			*(u32*)0x0208AF74 = 0xE1A00000; // nop
			*(u32*)0x0208AF78 = 0xE1A00000; // nop
			*(u32*)0x0208AF7C = 0xE1A00000; // nop
			setB(0x0208C304, 0x0208C320);
			setB(0x0208C570, 0x0208C598);
			*(u32*)0x0208C598 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
			*(u32*)0x0208C59C += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
			*(u32*)0x0208C5A0 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0208C5A4 = 0xE5850030; // str r0, [r5,#0x30]
			*(u32*)0x0208C5A8 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
			*(u32*)0x020A4D74 = 0xE3A02C07; // mov r2, #0x700
			*(u32*)0x020A4D94 = 0xE2840B01; // add r0, r4, #0x400
			*(u32*)0x020A4D9C = 0xE1A00004; // mov r0, r4
			*(u32*)0x020A4DA4 = 0xE1A00000; // nop
			*(u32*)0x020A4DA8 = 0xE1A00000; // nop
			*(u32*)0x020A4DAC = 0xE1A00000; // nop
			*(u32*)0x020A4DB0 = 0xE1A00000; // nop
			*(u32*)0x020A4DB4 = 0xE1A00000; // nop
			*(u32*)0x020A4DC8 = 0xE2841B01; // add r1, r4, #0x400
		}
	}

	// Bookstore Dream (USA)
	// Bookstore Dream (Europe, Australia)
	// Bookstore Dream (Japan)
	else if (strncmp(romTid, "KQV", 3) == 0) {
		u32 offsetChange = (romTid[3] == 'E') ? 0 : 4;

		*(u32*)(0x0200FF00-offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x02010A84-offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x02013254-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x020189F4-offsetChange, heapEnd);
		*(u32*)(0x02018D80-offsetChange) = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02019E94-offsetChange);
		if (romTid[3] == 'E') {
			setBL(0x02052C48, (u32)dsiSaveGetInfo);
			setBL(0x02052C5C, (u32)dsiSaveOpen);
			setBL(0x02052C70, (u32)dsiSaveCreate);
			setBL(0x02052C80, (u32)dsiSaveOpen);
			*(u32*)0x02052CA0 = 0xE1A00000; // nop
			setBL(0x02052CAC, (u32)dsiSaveCreate);
			setBL(0x02052CBC, (u32)dsiSaveOpen);
			setBL(0x02052D04, (u32)dsiSaveSeek);
			setBL(0x02052D14, (u32)dsiSaveWrite);
			setBL(0x02052D1C, (u32)dsiSaveClose);
			setBL(0x02052D74, (u32)dsiSaveOpen);
			setBL(0x02053364, (u32)dsiSaveSeek);
			setBL(0x02053374, (u32)dsiSaveRead);
			setBL(0x020533A0, (u32)dsiSaveClose);
			setBL(0x02053468, (u32)dsiSaveOpen);
			setBL(0x02053484, (u32)dsiSaveCreate);
			setBL(0x02053494, (u32)dsiSaveOpen);
			*(u32*)0x020534B4 = 0xE1A00000; // nop
			setBL(0x020534C0, (u32)dsiSaveCreate);
			setBL(0x020534D0, (u32)dsiSaveOpen);
			setBL(0x020534E8, (u32)dsiSaveSeek);
			setBL(0x02053500, (u32)dsiSaveWrite);
			setBL(0x02053508, (u32)dsiSaveClose);
		} else if (romTid[3] == 'V') {
			setBL(0x02052F54, (u32)dsiSaveGetInfo);
			setBL(0x02052F68, (u32)dsiSaveOpen);
			setBL(0x02052F7C, (u32)dsiSaveCreate);
			setBL(0x02052F8C, (u32)dsiSaveOpen);
			*(u32*)0x02052FAC = 0xE1A00000; // nop
			setBL(0x02052FB8, (u32)dsiSaveCreate);
			setBL(0x02052FC8, (u32)dsiSaveOpen);
			setBL(0x02053010, (u32)dsiSaveSeek);
			setBL(0x02053020, (u32)dsiSaveWrite);
			setBL(0x02053028, (u32)dsiSaveClose);
			setBL(0x02053080, (u32)dsiSaveOpen);
			setBL(0x02053670, (u32)dsiSaveSeek);
			setBL(0x02053680, (u32)dsiSaveRead);
			setBL(0x020536AC, (u32)dsiSaveClose);
			setBL(0x02053774, (u32)dsiSaveOpen);
			setBL(0x02053790, (u32)dsiSaveCreate);
			setBL(0x020537A0, (u32)dsiSaveOpen);
			*(u32*)0x020537C0 = 0xE1A00000; // nop
			setBL(0x020537CC, (u32)dsiSaveCreate);
			setBL(0x020537DC, (u32)dsiSaveOpen);
			setBL(0x020537F4, (u32)dsiSaveSeek);
			setBL(0x0205380C, (u32)dsiSaveWrite);
			setBL(0x02053814, (u32)dsiSaveClose);
		} else {
			setBL(0x02053344, (u32)dsiSaveGetInfo);
			setBL(0x02053358, (u32)dsiSaveOpen);
			setBL(0x0205336C, (u32)dsiSaveCreate);
			setBL(0x0205337C, (u32)dsiSaveOpen);
			*(u32*)0x0205339C = 0xE1A00000; // nop
			setBL(0x020533A8, (u32)dsiSaveCreate);
			setBL(0x020533B8, (u32)dsiSaveOpen);
			setBL(0x02053400, (u32)dsiSaveSeek);
			setBL(0x02053410, (u32)dsiSaveWrite);
			setBL(0x02053418, (u32)dsiSaveClose);
			setBL(0x02053470, (u32)dsiSaveOpen);
			setBL(0x02053A60, (u32)dsiSaveSeek);
			setBL(0x02053A70, (u32)dsiSaveRead);
			setBL(0x02053A9C, (u32)dsiSaveClose);
			setBL(0x02053B64, (u32)dsiSaveOpen);
			setBL(0x02053B80, (u32)dsiSaveCreate);
			setBL(0x02053B90, (u32)dsiSaveOpen);
			*(u32*)0x02053BB0 = 0xE1A00000; // nop
			setBL(0x02053BBC, (u32)dsiSaveCreate);
			setBL(0x02053BCC, (u32)dsiSaveOpen);
			setBL(0x02053BE4, (u32)dsiSaveSeek);
			setBL(0x02053BFC, (u32)dsiSaveWrite);
			setBL(0x02053C04, (u32)dsiSaveClose);
		}
	}

	// Bookworm (USA)
	// Saving is not supported due to using more than one file in filesystem
	/*else if (strcmp(romTid, "KBKE") == 0) {
		if (!extendedMemory2) {
			*(u32*)0x02017DE4 = 0x8C000;
		}
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
		*(u32*)0x02065B14 = 0xE1A00000; // nop
		*(u32*)0x020699BC = 0xE1A00000; // nop
		patchInitDSiWare(0x020703F8, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x02070784 = 0x020DE060;
		}
		patchUserSettingsReadDSiWare(0x02071E6C);
		*(u32*)0x0207574C = 0xE1A00000; // nop
	}*/

	// Boom Boom Squaries (USA)
	else if (strcmp(romTid, "KBME") == 0) {
		*(u32*)0x02005150 = 0xE1A00000; // nop (Disable NFTR font loading from TWLNAND)
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop
		// Somehow detects save data as corrupted
		/* setBL(0x02007454, (u32)dsiSaveOpen);
		*(u32*)0x020074AC = 0xE1A00000; // nop
		setBL(0x020074E8, (u32)dsiSaveRead);
		setBL(0x020074F8, (u32)dsiSaveClose);
		setBL(0x02007508, (u32)dsiSaveGetLength);
		setBL(0x0200751C, (u32)dsiSaveRead);
		setBL(0x02007524, (u32)dsiSaveClose);
		setBL(0x020075F8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0200760C, (u32)dsiSaveOpen);
		*(u32*)0x02007640 = 0xE1A00000; // nop
		setBL(0x02007678, (u32)dsiSaveSetLength);
		setBL(0x02007688, (u32)dsiSaveWrite);
		setBL(0x02007698, (u32)dsiSaveWrite);
		setBL(0x020076A0, (u32)dsiSaveClose); */
		*(u32*)0x0202CE74 = 0xE1A00000; // nop
		// tonccpy((u32*)0x0202D9F8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02030830 = 0xE1A00000; // nop
		patchInitDSiWare(0x02035BE8, heapEnd);
		*(u32*)0x02035F74 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02037088);
	}

	// Boom Boom Squaries (Europe, Australia)
	else if (strcmp(romTid, "KBMV") == 0) {
		*(u32*)0x02005150 = 0xE1A00000; // nop (Disable NFTR font loading from TWLNAND)
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop
		// Somehow detects save data as corrupted
		/* setBL(0x020073BC, (u32)dsiSaveOpen);
		*(u32*)0x02007414 = 0xE1A00000; // nop
		setBL(0x02007450, (u32)dsiSaveRead);
		setBL(0x02007460, (u32)dsiSaveClose);
		setBL(0x02007470, (u32)dsiSaveGetLength);
		setBL(0x02007484, (u32)dsiSaveRead);
		setBL(0x0200748C, (u32)dsiSaveClose);
		setBL(0x02007560, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02007574, (u32)dsiSaveOpen);
		*(u32*)0x020075A8 = 0xE1A00000; // nop
		setBL(0x020075E0, (u32)dsiSaveSetLength);
		setBL(0x020075F0, (u32)dsiSaveWrite);
		setBL(0x02007600, (u32)dsiSaveWrite);
		setBL(0x02007608, (u32)dsiSaveClose); */
		*(u32*)0x0202CDDC = 0xE1A00000; // nop
		// tonccpy((u32*)0x0202D960, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02030798 = 0xE1A00000; // nop
		patchInitDSiWare(0x02035B50, heapEnd);
		*(u32*)0x02035EDC = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02036FF0);
	}

	// Boom Boom Squaries (Japan)
	else if (strcmp(romTid, "KBMJ") == 0) {
		*(u32*)0x02005150 = 0xE1A00000; // nop (Disable NFTR font loading from TWLNAND)
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop
		// Somehow detects save data as corrupted
		/* setBL(0x020074E8, (u32)dsiSaveOpen);
		*(u32*)0x02007540 = 0xE1A00000; // nop
		setBL(0x0200757C, (u32)dsiSaveRead);
		setBL(0x0200758C, (u32)dsiSaveClose);
		setBL(0x0200759C, (u32)dsiSaveGetLength);
		setBL(0x020075B0, (u32)dsiSaveRead);
		setBL(0x020075B8, (u32)dsiSaveClose);
		setBL(0x0200768C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x020076A0, (u32)dsiSaveOpen);
		*(u32*)0x020076D4 = 0xE1A00000; // nop
		setBL(0x0200770C, (u32)dsiSaveSetLength);
		setBL(0x0200771C, (u32)dsiSaveWrite);
		setBL(0x0200772C, (u32)dsiSaveWrite);
		setBL(0x02007734, (u32)dsiSaveClose); */
		*(u32*)0x0202F178 = 0xE1A00000; // nop
		// tonccpy((u32*)0x0202FCFC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02032B34 = 0xE1A00000; // nop
		patchInitDSiWare(0x02037EEC, heapEnd);
		*(u32*)0x02038278 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0203938C);
	}

	// Bounce & Break (USA)
	else if (strcmp(romTid, "KZEE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200DC8C = 0xE1A00000; // nop
		*(u32*)0x02011430 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016820, heapEnd);
		setBL(0x020489B0, (u32)dsiSaveOpen);
		setBL(0x02048A04, (u32)dsiSaveGetLength);
		setBL(0x02048A14, (u32)dsiSaveRead);
		setBL(0x02048A1C, (u32)dsiSaveClose);
		*(u32*)0x02048A34 = 0xE12FFF1E; // bx lr
		setBL(0x02048A6C, (u32)dsiSaveOpen);
		*(u32*)0x02048A84 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02048A94 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x02048AB0, (u32)dsiSaveCreate);
		setBL(0x02048AC4, (u32)dsiSaveOpen);
		setBL(0x02048AD4, (u32)dsiSaveGetResultCode);
		setBL(0x02048B18, (u32)dsiSaveSetLength);
		setBL(0x02048B28, (u32)dsiSaveWrite);
		setBL(0x02048B30, (u32)dsiSaveClose);
	}

	// Bounce & Break (Europe)
	else if (strcmp(romTid, "KZEP") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200DC44 = 0xE1A00000; // nop
		*(u32*)0x0201153C = 0xE1A00000; // nop
		patchInitDSiWare(0x02016C08, heapEnd);
		setBL(0x02049138, (u32)dsiSaveOpen);
		setBL(0x0204918C, (u32)dsiSaveGetLength);
		setBL(0x0204919C, (u32)dsiSaveRead);
		setBL(0x020491A4, (u32)dsiSaveClose);
		*(u32*)0x020491BC = 0xE12FFF1E; // bx lr
		setBL(0x020491F4, (u32)dsiSaveOpen);
		*(u32*)0x0204920C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0204921C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x02049238, (u32)dsiSaveCreate);
		setBL(0x0204924C, (u32)dsiSaveOpen);
		setBL(0x0204925C, (u32)dsiSaveGetResultCode);
		setBL(0x020492A0, (u32)dsiSaveSetLength);
		setBL(0x020492B0, (u32)dsiSaveWrite);
		setBL(0x020492B8, (u32)dsiSaveClose);
	}

	// Box Pusher (USA)
	else if (strcmp(romTid, "KQBE") == 0) {
		setBL(0x02041510, (u32)dsiSaveOpen);
		setBL(0x02041584, (u32)dsiSaveGetLength);
		setBL(0x02041598, (u32)dsiSaveClose);
		setBL(0x020415B8, (u32)dsiSaveSeek);
		setBL(0x020415D0, (u32)dsiSaveRead);
		setBL(0x020415E4, (u32)dsiSaveClose);
		setBL(0x02041638, (u32)dsiSaveClose);
		*(u32*)0x02041688 = 0xE1A00000; // nop
		setBL(0x020416E8, (u32)dsiSaveCreate);
		setBL(0x0204173C, (u32)dsiSaveOpen);
		setBL(0x020417A4, (u32)dsiSaveSetLength);
		setBL(0x020417BC, (u32)dsiSaveClose);
		setBL(0x02041810, (u32)dsiSaveGetLength);
		setBL(0x02041824, (u32)dsiSaveClose);
		setBL(0x02041844, (u32)dsiSaveSeek);
		setBL(0x0204185C, (u32)dsiSaveWrite);
		setBL(0x02041870, (u32)dsiSaveClose);
		setBL(0x020418BC, (u32)dsiSaveClose);
		*(u32*)0x0207F6DC = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Box Pusher (Europe)
	else if (strcmp(romTid, "KQBP") == 0) {
		setBL(0x02041B20, (u32)dsiSaveOpen);
		setBL(0x02041B94, (u32)dsiSaveGetLength);
		setBL(0x02041BA8, (u32)dsiSaveClose);
		setBL(0x02041BC8, (u32)dsiSaveSeek);
		setBL(0x02041BE0, (u32)dsiSaveRead);
		setBL(0x02041BF4, (u32)dsiSaveClose);
		setBL(0x02041C48, (u32)dsiSaveClose);
		*(u32*)0x02041C98 = 0xE1A00000; // nop
		setBL(0x02041CF8, (u32)dsiSaveCreate);
		setBL(0x02041D4C, (u32)dsiSaveOpen);
		setBL(0x02041DB4, (u32)dsiSaveSetLength);
		setBL(0x02041DCC, (u32)dsiSaveClose);
		setBL(0x02041E20, (u32)dsiSaveGetLength);
		setBL(0x02041E34, (u32)dsiSaveClose);
		setBL(0x02041E54, (u32)dsiSaveSeek);
		setBL(0x02041E6C, (u32)dsiSaveWrite);
		setBL(0x02041E80, (u32)dsiSaveClose);
		setBL(0x02041ECC, (u32)dsiSaveClose);
		*(u32*)0x0207FCEC = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Art Style: BOXLIFE (USA)
	else if (strcmp(romTid, "KAHE") == 0) {
		*(u32*)0x0202FBD0 = 0xE1A00000; // nop
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
		*(u32*)0x02035DC0 = 0xE1A00000; // nop
		*(u32*)0x02035DD0 = 0xE1A00000; // nop
		*(u32*)0x02035DE0 = 0xE1A00000; // nop
		*(u32*)0x02036060 = 0xE1A00000; // nop
		*(u32*)0x0203606C = 0xE1A00000; // nop
		*(u32*)0x02036088 = 0xE1A00000; // nop
		*(u32*)0x02055990 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02055994 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02058F84 = 0xE1A00000; // nop
		patchInitDSiWare(0x02062C8C, heapEnd);
		patchUserSettingsReadDSiWare(0x02064224);
	}

	// Art Style: BOXLIFE (Europe, Australia)
	else if (strcmp(romTid, "KAHV") == 0) {
		*(u32*)0x0202FB18 = 0xE1A00000; // nop
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
		*(u32*)0x02035A08 = 0xE1A00000; // nop
		*(u32*)0x02035A18 = 0xE1A00000; // nop
		*(u32*)0x02035A28 = 0xE1A00000; // nop
		*(u32*)0x02035CA8 = 0xE1A00000; // nop
		*(u32*)0x02035CB4 = 0xE1A00000; // nop
		*(u32*)0x02035CD0 = 0xE1A00000; // nop
		*(u32*)0x02055694 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02055698 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02058C88 = 0xE1A00000; // nop
		patchInitDSiWare(0x02062990, heapEnd);
		patchUserSettingsReadDSiWare(0x02063F28);
	}

	// Art Style: Hacolife (Japan)
	else if (strcmp(romTid, "KAHJ") == 0) {
		*(u32*)0x0202F148 = 0xE1A00000; // nop
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
		*(u32*)0x02034D54 = 0xE1A00000; // nop
		*(u32*)0x02034D64 = 0xE1A00000; // nop
		*(u32*)0x02034D74 = 0xE1A00000; // nop
		*(u32*)0x02034FF4 = 0xE1A00000; // nop
		*(u32*)0x02035000 = 0xE1A00000; // nop
		*(u32*)0x0203501C = 0xE1A00000; // nop
		*(u32*)0x02054C10 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02054C14 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020583A8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02062838, heapEnd);
		patchUserSettingsReadDSiWare(0x02063DE4);
	}

	// Bridge (USA)
	// Bridge (Europe)
	else if (strcmp(romTid, "K9FE") == 0 || strcmp(romTid, "K9FP") == 0) {
		u32 offsetChange = (romTid[3] == 'E') ? 0 : 4;
		setBL(0x02010450-offsetChange, (u32)dsiSaveOpen);
		setBL(0x020104C4-offsetChange, (u32)dsiSaveGetLength);
		setBL(0x020104D8-offsetChange, (u32)dsiSaveClose);
		setBL(0x020104F8-offsetChange, (u32)dsiSaveSeek);
		setBL(0x02010510-offsetChange, (u32)dsiSaveRead);
		setBL(0x02010524-offsetChange, (u32)dsiSaveClose);
		setBL(0x02010578-offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020105C8-offsetChange) = 0xE1A00000; // nop
		setBL(0x02010628-offsetChange, (u32)dsiSaveCreate);
		setBL(0x0201067C-offsetChange, (u32)dsiSaveOpen);
		setBL(0x020106E4-offsetChange, (u32)dsiSaveSetLength);
		setBL(0x020106FC-offsetChange, (u32)dsiSaveClose);
		setBL(0x02010750-offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02010764-offsetChange, (u32)dsiSaveClose);
		setBL(0x02010784-offsetChange, (u32)dsiSaveSeek);
		setBL(0x0201079C-offsetChange, (u32)dsiSaveWrite);
		setBL(0x020107B0-offsetChange, (u32)dsiSaveClose);
		setBL(0x020107FC-offsetChange, (u32)dsiSaveClose);
		*(u32*)0x020490DC = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Bugs'N'Balls (USA)
	// Bugs'N'Balls (Europe)
	else if (strncmp(romTid, "KKQ", 3) == 0) {
		u32* saveFuncOffsets[22] = {NULL};

		useSharedFont = twlFontFound;
		*(u32*)0x0201B334 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201BEB8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201E7B0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02024CE0, heapEnd);
		patchUserSettingsReadDSiWare(0x0202637C);
		if (romTid[3] == 'E') {
			*(u32*)0x0205A438 = 0xE1A00000;
			*(u32*)0x0205A478 = 0xE1A00000;
			*(u32*)0x0205A550 = 0xE1A00000;
			*(u32*)0x0205A558 = 0xE1A00000;
			*(u32*)0x0205A588 = 0xE1A00000;
			*(u32*)0x0205A598 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205A600 = 0xE1A00000;
			*(u32*)0x0205A608 = 0xE1A00000;
			*(u32*)0x0205A740 = 0xE1A00000;
			*(u32*)0x0205A804 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205A83C = 0xE1A00000;
			*(u32*)0x0205A844 = 0xE1A00000;
			*(u32*)0x0205A8F4 = 0xE1A00000;
			*(u32*)0x0205A8FC = 0xE1A00000;
			*(u32*)0x0205ABBC = 0xE1A00000;
			*(u32*)0x0205ACEC = 0xE1A00000;
			*(u32*)0x0205ACF4 = 0xE1A00000;
			*(u32*)0x0205AEB0 = 0xE1A00000;
			*(u32*)0x0205AED8 = 0xE1A00000;
			*(u32*)0x0205AEE0 = 0xE1A00000;
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
		} else if (romTid[3] == 'P') {
			*(u32*)0x0205B920 = 0xE1A00000;
			*(u32*)0x0205B960 = 0xE1A00000;
			*(u32*)0x0205BA38 = 0xE1A00000;
			*(u32*)0x0205BA40 = 0xE1A00000;
			*(u32*)0x0205BB6C = 0xE1A00000;
			*(u32*)0x0205BB7C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205BBE4 = 0xE1A00000;
			*(u32*)0x0205BBEC = 0xE1A00000;
			*(u32*)0x0205BC78 = 0xE1A00000;
			*(u32*)0x0205BCE4 = 0xE1A00000;
			*(u32*)0x0205BCEC = 0xE1A00000;
			*(u32*)0x0205BD78 = 0xE1A00000;
			*(u32*)0x0205BE3C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0205BE74 = 0xE1A00000;
			*(u32*)0x0205BE7C = 0xE1A00000;
			*(u32*)0x0205BF2C = 0xE1A00000;
			*(u32*)0x0205BF34 = 0xE1A00000;
			*(u32*)0x0205C16C = 0xE1A00000;
			*(u32*)0x0205C29C = 0xE1A00000;
			*(u32*)0x0205C2A4 = 0xE1A00000;
			*(u32*)0x0205C460 = 0xE1A00000;
			*(u32*)0x0205C488 = 0xE1A00000;
			*(u32*)0x0205C490 = 0xE1A00000;
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

		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad((romTid[3] == 'E') ? 0x0209695C : 0x020994D8, 0x020268D0);
		}
	}

	// Cake Ninja (USA)
	else if (strcmp(romTid, "K2JE") == 0) {
		*(u32*)0x02008C4C = 0xE1A00000; // nop
		*(u32*)0x02008DE4 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Make BG static to cut down RAM usage
			setB(0x02008400, 0x02008664);
			*(u32*)0x0200DCC0 = 0xE1A00000; // nop
			*(u32*)0x0200DCC4 = 0xE1A00000; // nop
			*(u32*)0x0200DCC8 = 0xE1A00000; // nop
			*(u32*)0x0200DCCC = 0xE1A00000; // nop
		}
		setBL(0x0202CDF0, (u32)dsiSaveOpen);
		setBL(0x0202CE48, (u32)dsiSaveCreate);
		setBL(0x0202CE7C, (u32)dsiSaveOpen);
		setBL(0x0202CE90, (u32)dsiSaveSetLength);
		setBL(0x0202CEA0, (u32)dsiSaveGetLength);
		setBL(0x0202CEA8, (u32)dsiSaveClose);
		setBL(0x0202CEE0, (u32)dsiSaveSetLength);
		setBL(0x0202CEF0, (u32)dsiSaveGetLength);
		setBL(0x0202CEF8, (u32)dsiSaveClose);
		*(u32*)0x0202D028 = 0xE1A00000; // nop
		setBL(0x0202D100, (u32)dsiSaveOpen);
		setBL(0x0202D128, (u32)dsiSaveSeek);
		setBL(0x0202D13C, (u32)dsiSaveRead);
		setBL(0x0202D154, (u32)dsiSaveClose);
		setBL(0x0202D21C, (u32)dsiSaveOpen);
		setBL(0x0202D244, (u32)dsiSaveSeek);
		setBL(0x0202D258, (u32)dsiSaveWrite);
		setBL(0x0202D264, (u32)dsiSaveClose);
		*(u32*)0x02057938 = 0xE1A00000; // nop
		tonccpy((u32*)0x020584CC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B0FC = 0xE1A00000; // nop
		patchInitDSiWare(0x02062C24, heapEnd);
		patchUserSettingsReadDSiWare(0x02064424);
		*(u32*)0x02064440 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02064444 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206444C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064450 = 0xE12FFF1E; // bx lr
	}

	// Cake Ninja (Europe)
	else if (strcmp(romTid, "K2JP") == 0) {
		*(u32*)0x02008C4C = 0xE1A00000; // nop
		*(u32*)0x02008ED4 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Make BG static to cut down RAM usage
			setB(0x02008400, 0x02008664);
			*(u32*)0x0200DD4C = 0xE1A00000; // nop
			*(u32*)0x0200DD50 = 0xE1A00000; // nop
			*(u32*)0x0200DD54 = 0xE1A00000; // nop
			*(u32*)0x0200DD58 = 0xE1A00000; // nop
		}
		setBL(0x0202CEC8, (u32)dsiSaveOpen);
		setBL(0x0202CF20, (u32)dsiSaveCreate);
		setBL(0x0202CF54, (u32)dsiSaveOpen);
		setBL(0x0202CF68, (u32)dsiSaveSetLength);
		setBL(0x0202CF78, (u32)dsiSaveGetLength);
		setBL(0x0202CF80, (u32)dsiSaveClose);
		setBL(0x0202CFB8, (u32)dsiSaveSetLength);
		setBL(0x0202CFC8, (u32)dsiSaveGetLength);
		setBL(0x0202CFD0, (u32)dsiSaveClose);
		*(u32*)0x0202D100 = 0xE1A00000; // nop
		setBL(0x0202D1D8, (u32)dsiSaveOpen);
		setBL(0x0202D200, (u32)dsiSaveSeek);
		setBL(0x0202D214, (u32)dsiSaveRead);
		setBL(0x0202D22C, (u32)dsiSaveClose);
		setBL(0x0202D2F4, (u32)dsiSaveOpen);
		setBL(0x0202D31C, (u32)dsiSaveSeek);
		setBL(0x0202D330, (u32)dsiSaveWrite);
		setBL(0x0202D33C, (u32)dsiSaveClose);
		*(u32*)0x02057A10 = 0xE1A00000; // nop
		tonccpy((u32*)0x020585A4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B1D4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02062CFC, heapEnd);
		patchUserSettingsReadDSiWare(0x020644FC);
		*(u32*)0x02064518 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206451C = 0xE12FFF1E; // bx lr
		*(u32*)0x02064524 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064528 = 0xE12FFF1E; // bx lr
	}

	// Cake Ninja 2 (USA)
	else if (strcmp(romTid, "K2NE") == 0) {
		*(u32*)0x02008880 = 0xE1A00000; // nop
		*(u32*)0x020089F4 = 0xE1A00000; // nop
		setBL(0x0204C918, (u32)dsiSaveOpen);
		setBL(0x0204C970, (u32)dsiSaveCreate);
		setBL(0x0204C9A4, (u32)dsiSaveOpen);
		setBL(0x0204C9B8, (u32)dsiSaveSetLength);
		setBL(0x0204C9C8, (u32)dsiSaveGetLength);
		setBL(0x0204C9D0, (u32)dsiSaveClose);
		setBL(0x0204CA08, (u32)dsiSaveSetLength);
		setBL(0x0204CA18, (u32)dsiSaveGetLength);
		setBL(0x0204CA20, (u32)dsiSaveClose);
		*(u32*)0x0204CB50 = 0xE1A00000; // nop
		setBL(0x0204CC28, (u32)dsiSaveOpen);
		setBL(0x0204CC50, (u32)dsiSaveSeek);
		setBL(0x0204CC64, (u32)dsiSaveRead);
		setBL(0x0204CC7C, (u32)dsiSaveClose);
		setBL(0x0204CD44, (u32)dsiSaveOpen);
		setBL(0x0204CD6C, (u32)dsiSaveSeek);
		setBL(0x0204CD80, (u32)dsiSaveWrite);
		setBL(0x0204CD8C, (u32)dsiSaveClose);
		*(u32*)0x020774AC = 0xE1A00000; // nop
		tonccpy((u32*)0x02078040, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0207AC70 = 0xE1A00000; // nop
		patchInitDSiWare(0x020827C4, heapEnd);
		*(u32*)0x02082B50 = 0x0210AFE0;
		patchUserSettingsReadDSiWare(0x02083FC4);
		*(u32*)0x02083FE0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02083FE4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02083FEC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02083FF0 = 0xE12FFF1E; // bx lr
	}

	// Cake Ninja 2 (Europe)
	else if (strcmp(romTid, "K2NP") == 0) {
		*(u32*)0x02008880 = 0xE1A00000; // nop
		*(u32*)0x02008A88 = 0xE1A00000; // nop
		setBL(0x0204C974, (u32)dsiSaveOpen);
		setBL(0x0204C9CC, (u32)dsiSaveCreate);
		setBL(0x0204CA00, (u32)dsiSaveOpen);
		setBL(0x0204CA14, (u32)dsiSaveSetLength);
		setBL(0x0204CA24, (u32)dsiSaveGetLength);
		setBL(0x0204CA2C, (u32)dsiSaveClose);
		setBL(0x0204CA64, (u32)dsiSaveSetLength);
		setBL(0x0204CA74, (u32)dsiSaveGetLength);
		setBL(0x0204CA7C, (u32)dsiSaveClose);
		*(u32*)0x0204CBAC = 0xE1A00000; // nop
		setBL(0x0204CC84, (u32)dsiSaveOpen);
		setBL(0x0204CCAC, (u32)dsiSaveSeek);
		setBL(0x0204CCC0, (u32)dsiSaveRead);
		setBL(0x0204CCD8, (u32)dsiSaveClose);
		setBL(0x0204CDA0, (u32)dsiSaveOpen);
		setBL(0x0204CDC8, (u32)dsiSaveSeek);
		setBL(0x0204CDDC, (u32)dsiSaveWrite);
		setBL(0x0204CDE8, (u32)dsiSaveClose);
		*(u32*)0x02077508 = 0xE1A00000; // nop
		tonccpy((u32*)0x0207809C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0207ACCC = 0xE1A00000; // nop
		patchInitDSiWare(0x02082820, heapEnd);
		*(u32*)0x02082BAC = 0x0210B040;
		patchUserSettingsReadDSiWare(0x02084020);
		*(u32*)0x0208403C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02084040 = 0xE12FFF1E; // bx lr
		*(u32*)0x02084048 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0208404C = 0xE12FFF1E; // bx lr
	}

	// Cake Ninja: XMAS (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYNE") == 0 && extendedMemory2) {
		*(u32*)0x0200846C = 0xE1A00000; // nop
		*(u32*)0x02008604 = 0xE1A00000; // nop
		setBL(0x0202571C, (u32)dsiSaveOpen);
		setBL(0x02025774, (u32)dsiSaveCreate);
		setBL(0x020257A8, (u32)dsiSaveOpen);
		setBL(0x020257BC, (u32)dsiSaveSetLength);
		setBL(0x020257CC, (u32)dsiSaveGetLength);
		setBL(0x020257D4, (u32)dsiSaveClose);
		setBL(0x0202580C, (u32)dsiSaveSetLength);
		setBL(0x0202581C, (u32)dsiSaveGetLength);
		setBL(0x02025824, (u32)dsiSaveClose);
		*(u32*)0x02025954 = 0xE1A00000; // nop
		setBL(0x02025A2C, (u32)dsiSaveOpen);
		setBL(0x02025A54, (u32)dsiSaveSeek);
		setBL(0x02025A68, (u32)dsiSaveRead);
		setBL(0x02025A80, (u32)dsiSaveClose);
		setBL(0x02025B48, (u32)dsiSaveOpen);
		setBL(0x02025B70, (u32)dsiSaveSeek);
		setBL(0x02025B84, (u32)dsiSaveWrite);
		setBL(0x02025B90, (u32)dsiSaveClose);
		*(u32*)0x02050348 = 0xE1A00000; // nop
		tonccpy((u32*)0x02050EDC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02053B0C = 0xE1A00000; // nop
		patchInitDSiWare(0x0205B760, heapEnd);
		patchUserSettingsReadDSiWare(0x0205CF60);
		*(u32*)0x0205CF7C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205CF80 = 0xE12FFF1E; // bx lr
		*(u32*)0x0205CF88 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205CF8C = 0xE12FFF1E; // bx lr
	}

	// Cake Ninja: XMAS (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYNP") == 0 && extendedMemory2) {
		*(u32*)0x0200846C = 0xE1A00000; // nop
		*(u32*)0x020086F4 = 0xE1A00000; // nop
		setBL(0x020257A8, (u32)dsiSaveOpen);
		setBL(0x02025800, (u32)dsiSaveCreate);
		setBL(0x02025834, (u32)dsiSaveOpen);
		setBL(0x02025848, (u32)dsiSaveSetLength);
		setBL(0x02025858, (u32)dsiSaveGetLength);
		setBL(0x02025860, (u32)dsiSaveClose);
		setBL(0x02025898, (u32)dsiSaveSetLength);
		setBL(0x020258A8, (u32)dsiSaveGetLength);
		setBL(0x020258B0, (u32)dsiSaveClose);
		*(u32*)0x020259E0 = 0xE1A00000; // nop
		setBL(0x02025AB8, (u32)dsiSaveOpen);
		setBL(0x02025AE0, (u32)dsiSaveSeek);
		setBL(0x02025AF4, (u32)dsiSaveRead);
		setBL(0x02025B0C, (u32)dsiSaveClose);
		setBL(0x02025BD4, (u32)dsiSaveOpen);
		setBL(0x02025BFC, (u32)dsiSaveSeek);
		setBL(0x02025C10, (u32)dsiSaveWrite);
		setBL(0x02025C1C, (u32)dsiSaveClose);
		*(u32*)0x020503D4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02050F68, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02053B98 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205B7EC, heapEnd);
		patchUserSettingsReadDSiWare(0x0205CFEC);
		*(u32*)0x0205D008 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205D00C = 0xE12FFF1E; // bx lr
		*(u32*)0x0205D014 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205D018 = 0xE12FFF1E; // bx lr
	}

	// Calculator (USA)
	else if (strcmp(romTid, "KCYE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201F6A8 = 0xE1A00000; // nop
		*(u32*)0x020239D8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02028FAC, heapEnd);
		patchUserSettingsReadDSiWare(0x0202A590);
		*(u32*)0x0203ED20 = 0xE1A00000; // nop
		*(u32*)0x0204D1FC = 0xE12FFF1E; // bx lr
	}

	// Calculator (Europe, Australia)
	else if (strcmp(romTid, "KCYV") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201503C = 0xE1A00000; // nop
		*(u32*)0x0201937C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E950, heapEnd);
		patchUserSettingsReadDSiWare(0x0201FF34);
		*(u32*)0x020346D0 = 0xE1A00000; // nop
		*(u32*)0x02042B6C = 0xE12FFF1E; // bx lr
	}

	// Candle Route (USA)
	// Candle Route (Europe)
	// Requires 8MB of RAM
	/* else if ((strcmp(romTid, "K9YE") == 0 || strcmp(romTid, "K9YP") == 0) && extendedMemory2) {
		*(u32*)0x020175F4 = 0xE1A00000; // nop
		*(u32*)0x0201ADD4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02021C14, heapEnd);
		patchUserSettingsReadDSiWare(0x02023108);
		*(u32*)0x0207DEB8 = 0xE1A00000; // nop
		*(u32*)0x0207DEBC = 0xE1A00000; // nop
		*(u32*)0x0207DEC0 = 0xE1A00000; // nop
		if (romTid[3] == 'E') {
			*(u32*)0x020AE44C = 0xE1A00000; // nop

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020AE76C;
				offset[i] = 0xE1A00000; // nop
			}
		} else if (romTid[3] == 'P') {
			*(u32*)0x020AE4F0 = 0xE1A00000; // nop

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020AE810;
				offset[i] = 0xE1A00000; // nop
			}
		}
	} */

	// GO Series: Captain Sub (USA)
	// GO Series: Captain Sub (Europe)
	// Otakara Hanta: Submarine Kid no Bouken (Japan)
	else if (strncmp(romTid, "K3N", 3) == 0) {
		u8 offsetChange = (romTid[3] != 'J') ? 0 : 0xE4;
		u8 offsetChangeM = (romTid[3] != 'J') ? 0 : 0xB4;

		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005538 = 0xE1A00000; // nop
		*(u32*)0x0200A22C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0200AA38, (u32)dsiSaveOpen);
		setBL(0x0200AA70, (u32)dsiSaveRead);
		setBL(0x0200AA90, (u32)dsiSaveClose);
		setBL(0x0200AB28, (u32)dsiSaveCreate);
		setBL(0x0200AB68, (u32)dsiSaveOpen);
		setBL(0x0200ABA0, (u32)dsiSaveSetLength);
		setBL(0x0200ABB8, (u32)dsiSaveWrite);
		setBL(0x0200ABDC, (u32)dsiSaveClose);
		setBL(0x0200AC6C, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B550 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)(0x0200F1B8+offsetChangeM) = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		if (romTid[3] != 'J') {
			*(u32*)0x0200F3A4 = 0xE1A00000; // nop
			*(u32*)0x0203862C = 0xE1A00000; // nop
		}
		*(u32*)(0x0204D2D8-offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x0205144C-offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x02051FD0-offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x020549AC-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0205BA1C-offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x0205CEF8-offsetChange);
		*(u32*)(0x0205D32C-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0205D330-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0205D334-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0205D338-offsetChange) = 0xE1A00000; // nop

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)(0x0200F2F4+offsetChangeM);
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Castle Conqueror (USA)
	else if (strcmp(romTid, "KCNE") == 0) {
		patchInitDSiWare(0x0201D82C, heapEnd);
		*(u32*)0x0201DBB8 -= 0x30000;
		patchUserSettingsReadDSiWare(0x0201ECBC);
		*(u32*)0x02024740 = 0xE1A00000; // nop
		tonccpy((u32*)0x020252B8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02027A00 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0203D44C = 0x2C00C8; // Shrink sound heap from 0x3000C8
		}
		// *(u32*)0x0204AFD8 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0204B084 = 0xE12FFF1E; // bx lr
		// *(u32*)0x0204B44C = 0xE12FFF1E; // bx lr
		setBL(0x0204AFF4, (u32)dsiSaveCreate);
		setBL(0x0204B014, (u32)dsiSaveOpen);
		*(u32*)0x0204B034 = 0xE1A00000; // nop
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
		*(u32*)0x0204B4D8 = 0xE1A00000; // nop
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
	// Crashes on some file function after company logo
	/*else if (strncmp(romTid, "KCN", 3) == 0) {
		*(u32*)0x02016340 = 0xE1A00000; // nop
		tonccpy((u32*)0x02016EC4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02019694 = 0xE1A00000; // nop
		*(u32*)0x0201D330 = 0xE1A00000; // nop
		*(u32*)0x0201F0CC = 0xE1A00000; // nop
		*(u32*)0x0201F0D0 = 0xE1A00000; // nop
		*(u32*)0x0201F0DC = 0xE1A00000; // nop
		*(u32*)0x0201F23C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201F298, heapEnd);
		*(u32*)0x0201F3CC -= 0x30000;
		patchUserSettingsReadDSiWare(0x020204E0);
		if (romTid[3] == 'E') { // English
			if (!extendedMemory2) {
				*(u32*)0x0202B3F8 = 0x2C00D0; // Shrink sound heap from 0x3000D0
			}
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
			*(u32*)0x0203A9C0 = 0xE1A00000; // nop
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
			*(u32*)0x0203BC2C = 0xE1A00000; // nop
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
		} else if (romTid[3] == 'D') { // German
			if (!extendedMemory2) {
				*(u32*)0x0202CE80 = 0x2C00D0; // Shrink sound heap from 0x3000D0
			}
		} else if (romTid[3] == 'F') { // French
			if (!extendedMemory2) {
				*(u32*)0x0202A80 = 0x2C00D0; // Shrink sound heap from 0x3000D0
			}
		} else if (romTid[3] == 'I') { // Italian
			if (!extendedMemory2) {
				*(u32*)0x0202CE84 = 0x2C00D0; // Shrink sound heap from 0x3000D0
			}
		} else if (romTid[3] == 'S') { // Spanish
			if (!extendedMemory2) {
				*(u32*)0x0202BB90 = 0x2C00D0; // Shrink sound heap from 0x3000D0
			}
		}
	}*/

	// Castle Conqueror: Against (USA)
	// Castle Conqueror: Against (Europe, Australia)
	else if (strcmp(romTid, "KQOE") == 0 || strcmp(romTid, "KQOV") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = (romTid[3] == 'E') ? 0x020ADE18 : 0x020ADFB8;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xA8000, bssEnd, sdatOffset);
		// }

		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x68;

		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02015B2C = 0xE1A00000; // nop
		*(u32*)(0x02018DA4+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201EBB0+offsetChange, heapEnd);
		*(u32*)(0x0201EF3C+offsetChange) = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020200E4+offsetChange);
		if (romTid[3] == 'E') {
			// *(u32*)0x0206ACC4 = 0xE12FFF1E; // bx lr
			// *(u32*)0x0206AEC8 = 0xE12FFF1E; // bx lr

			*(u32*)0x0206ACB4 = 0xE1A00000; // nop
			setBL(0x0206ADCC, (u32)dsiSaveOpen);
			setBL(0x0206ADE4, (u32)dsiSaveCreate);
			*(u32*)0x0206ADF4 = 0xE1A00000; // nop
			setBL(0x0206AE00, (u32)dsiSaveCreate);
			setBL(0x0206AE1C, (u32)dsiSaveClose);
			setBL(0x0206AE38, (u32)dsiSaveOpen);
			setBL(0x0206AE6C, (u32)dsiSaveWrite);
			setBL(0x0206AE98, (u32)dsiSaveClose);
			setBL(0x0206AEE8, (u32)dsiSaveOpen);
			setBL(0x0206AF4C, (u32)dsiSaveRead);
			setBL(0x0206AF78, (u32)dsiSaveClose);
			*(u32*)0x0206B0EC = 0xE1A00000; // nop
		} else {
			// *(u32*)0x02041BF8 = 0xE12FFF1E; // bx lr
			// *(u32*)0x02041DFC = 0xE12FFF1E; // bx lr

			*(u32*)0x02041BE8 = 0xE1A00000; // nop
			setBL(0x02041D00, (u32)dsiSaveOpen);
			setBL(0x02041D18, (u32)dsiSaveCreate);
			*(u32*)0x02041D28 = 0xE1A00000; // nop
			setBL(0x02041D34, (u32)dsiSaveCreate);
			setBL(0x02041D50, (u32)dsiSaveClose);
			setBL(0x02041D6C, (u32)dsiSaveOpen);
			setBL(0x02041DA0, (u32)dsiSaveWrite);
			setBL(0x02041DCC, (u32)dsiSaveClose);
			setBL(0x02041E28, (u32)dsiSaveOpen);
			setBL(0x02041E38, (u32)dsiSaveGetResultCode);
			setBL(0x02041EA4, (u32)dsiSaveOpen);
			setBL(0x02041EBC, (u32)dsiSaveCreate);
			*(u32*)0x02041ECC = 0xE1A00000; // nop
			setBL(0x02041ED8, (u32)dsiSaveCreate);
			setBL(0x02041EE8, (u32)dsiSaveOpen);
			setBL(0x02041EFC, (u32)dsiSaveWrite);
			setBL(0x02041F04, (u32)dsiSaveClose);
			setBL(0x02041F4C, (u32)dsiSaveRead);
			setBL(0x02041F78, (u32)dsiSaveClose);
			*(u32*)0x02042100 = 0xE1A00000; // nop
		}
	}

	// Castle Conqueror: Heroes (USA)
	// Castle Conqueror: Heroes (Japan)
	else if (strcmp(romTid, "KC5E") == 0 || strcmp(romTid, "KC5J") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = (romTid[3] == 'E') ? 0x020A561C : 0x020BDF60;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xA0000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02017744 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201831C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201AB7C = 0xE1A00000; // nop
		patchInitDSiWare(0x02020544, heapEnd);
		*(u32*)0x020208D0 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02021A68);
		if (romTid[3] == 'E') {
			setBL(0x02065C8C, (u32)dsiSaveGetInfo);
			setBL(0x02065CA0, (u32)dsiSaveOpen);
			setBL(0x02065CB4, (u32)dsiSaveCreate);
			setBL(0x02065CC4, (u32)dsiSaveOpen);
			*(u32*)0x02065CE4 = 0xE1A00000; // nop
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
		} else {
			setBL(0x02026F94, (u32)dsiSaveGetInfo);
			setBL(0x02026FA8, (u32)dsiSaveOpen);
			setBL(0x02026FC0, (u32)dsiSaveCreate);
			setBL(0x02026FD0, (u32)dsiSaveOpen);
			*(u32*)0x02026FF0 = 0xE1A00000; // nop
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
	}

	// Castle Conqueror: Heroes (Europe, Australia)
	else if (strcmp(romTid, "KC5V") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x020A5BDC;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xA0000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02017670 = 0xE1A00000; // nop
		tonccpy((u32*)0x02018248, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201AAA8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020470, heapEnd);
		*(u32*)0x020207FC = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02021994);
		setBL(0x020660FC, (u32)dsiSaveGetInfo);
		setBL(0x02066110, (u32)dsiSaveOpen);
		setBL(0x02066128, (u32)dsiSaveCreate);
		setBL(0x02066138, (u32)dsiSaveOpen);
		*(u32*)0x02066158 = 0xE1A00000; // nop
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

	// Castle Conqueror: Heroes 2 (USA)
	// Castle Conqueror: Heroes 2 (Europe, Australia)
	// Castle Conqueror: Heroes 2 (Japan)
	else if (strncmp(romTid, "KXC", 3) == 0) {
		/* extern u32* mepHeapSetPatch;
		extern u32* cch2HeapAlloc;
		extern u32* cch2HeapAddrPtr; */

		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x02094AA8;
			if (romTid[3] == 'V') {
				sdatOffset = 0x02093848;
			} else if (romTid[3] == 'J') {
				sdatOffset = 0x0209124C;
			}

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x98000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02013170 = 0xE1A00000; // nop
		tonccpy((u32*)0x02013CF4, dsiSaveGetResultCode, 0xC);
		/* if (!extendedMemory2) {
			if (s2FlashcardId == 0x5A45) {
				cch2HeapAddrPtr[0] -= 0x01000000;
			}
			tonccpy((u32*)0x0201473C, mepHeapSetPatch, 0x70);
			tonccpy((u32*)0x0201D810, cch2HeapAlloc, 0xBC);
		} */
		*(u32*)0x020164C4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BE28, heapEnd);
		*(u32*)0x0201C1B4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201D2C8);
		if (romTid[3] == 'E') {
			/* if (!extendedMemory2) {
				*(u32*)0x02014738 = (u32)getOffsetFromBL((u32*)0x02026A80);
				setBL(0x02026A80, 0x0201473C);
				*(u32*)0x0201D80C = (u32)getOffsetFromBL((u32*)0x020719F0);
				setBL(0x020719F0, 0x0201D810);
			} */

			setBL(0x02035478, (u32)dsiSaveGetInfo);
			setBL(0x0203548C, (u32)dsiSaveOpen);
			setBL(0x020354A4, (u32)dsiSaveCreate);
			setBL(0x020354B4, (u32)dsiSaveOpen);
			*(u32*)0x020354D4 = 0xE1A00000; // nop
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
		} else if (romTid[3] == 'V') {
			/* if (!extendedMemory2) {
				*(u32*)0x02014738 = (u32)getOffsetFromBL((u32*)0x02026A80);
				setBL(0x02026A80, 0x0201473C);
				*(u32*)0x0201D80C = (u32)getOffsetFromBL((u32*)0x0203749C);
				setBL(0x0203749C, 0x0201D810);
			} */

			setBL(0x0206A44C, (u32)dsiSaveGetInfo);
			setBL(0x0206A460, (u32)dsiSaveOpen);
			setBL(0x0206A478, (u32)dsiSaveCreate);
			setBL(0x0206A488, (u32)dsiSaveOpen);
			*(u32*)0x0206A4A8 = 0xE1A00000; // nop
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
		} else {
			/* if (!extendedMemory2) {
				*(u32*)0x02014738 = (u32)getOffsetFromBL((u32*)0x0206082C);
				setBL(0x0206082C, 0x0201473C);
				*(u32*)0x0201D80C = (u32)getOffsetFromBL((u32*)0x0205DF58);
				setBL(0x0205DF58, 0x0201D810);
			} */

			setBL(0x02026EAC, (u32)dsiSaveGetInfo);
			setBL(0x02026EC0, (u32)dsiSaveOpen);
			setBL(0x02026ED8, (u32)dsiSaveCreate);
			setBL(0x02026EE8, (u32)dsiSaveOpen);
			*(u32*)0x02026F08 = 0xE1A00000; // nop
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
	}

	// Castle Conqueror: Revolution (USA)
	// Castle Conqueror: Revolution (Europe, Australia)
	else if (strcmp(romTid, "KQNE") == 0 || strcmp(romTid, "KQNV") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = (romTid[3] == 'E') ? 0x020BB4A4 : 0x20AE4CC;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xD0000, bssEnd, sdatOffset);
		// }

		u16 offsetChange = (romTid[3] == 'E') ? 0 : 0x2A04;

		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02015B4C = 0xE1A00000; // nop
		*(u32*)0x02018E2C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201EC38, heapEnd);
		*(u32*)0x0201EFC4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020201DC);
		// *(u32*)(0x020642A4+offsetChange) = 0xE12FFF1E; // bx lr
		// *(u32*)(0x020644BC+offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x02064294+offsetChange) = 0xE1A00000; // nop
		setBL(0x020643BC+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020643D4+offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x020643E4+offsetChange) = 0xE1A00000; // nop
		setBL(0x020643F0+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0206440C+offsetChange, (u32)dsiSaveClose);
		setBL(0x02064428+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0206445C+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02064488+offsetChange, (u32)dsiSaveClose);
		setBL(0x020644E8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020644F8+offsetChange, (u32)dsiSaveGetResultCode);
		setBL(0x02064564+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0206457C+offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x0206458C+offsetChange) = 0xE1A00000; // nop
		setBL(0x02064598+offsetChange, (u32)dsiSaveCreate);
		setBL(0x020645A8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020645BC+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020645C4+offsetChange, (u32)dsiSaveClose);
		setBL(0x0206460C+offsetChange, (u32)dsiSaveRead);
		setBL(0x02064638+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020647CC+offsetChange) = 0xE1A00000; // nop
	}

	// Ose Ose! Kyassuru Hirozu: Kaku Meihen (Japan)
	else if (strcmp(romTid, "KQNJ") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x020B52C8;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xD0000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02005104 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02015B58 = 0xE1A00000; // nop
		*(u32*)0x02018F48 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F04C, heapEnd);
		*(u32*)0x0201F3D8 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020205F0);
		*(u32*)0x0207974C = 0xE1A00000; // nop
		setBL(0x02079874, (u32)dsiSaveOpen);
		setBL(0x0207988C, (u32)dsiSaveCreate);
		*(u32*)0x0207989C = 0xE1A00000; // nop
		setBL(0x020798A8, (u32)dsiSaveCreate);
		setBL(0x020798C4, (u32)dsiSaveClose);
		setBL(0x020798E0, (u32)dsiSaveOpen);
		setBL(0x02079914, (u32)dsiSaveWrite);
		setBL(0x02079940, (u32)dsiSaveClose);
		setBL(0x020799A0, (u32)dsiSaveOpen);
		setBL(0x020799B0, (u32)dsiSaveGetResultCode);
		setBL(0x02079A1C, (u32)dsiSaveOpen);
		setBL(0x02079A34, (u32)dsiSaveCreate);
		*(u32*)0x02079A44 = 0xE1A00000; // nop
		setBL(0x02079A50, (u32)dsiSaveCreate);
		setBL(0x02079A60, (u32)dsiSaveOpen);
		setBL(0x02079A74, (u32)dsiSaveWrite);
		setBL(0x02079A7C, (u32)dsiSaveClose);
		setBL(0x02079AC4, (u32)dsiSaveRead);
		setBL(0x02079AF0, (u32)dsiSaveClose);
		*(u32*)0x02079C84 = 0xE1A00000; // nop
	}

	// Cat Frenzy (USA)
	// Cat Frenzy (Europe)
	// May require 8MB of RAM
	/* else if (strcmp(romTid, "KVXE") == 0 || strcmp(romTid, "KVXP") == 0) {
		*(u32*)0x020050EC = 0xE1A00000; // nop
		*(u32*)0x020176F4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02018278, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201AED4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02021B28, heapEnd);
		*(u32*)0x02021EB4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0202301C);
		*(u32*)0x02027F58 = 0xE1A00000; // nop
		*(u32*)0x02027F60 = 0xE1A00000; // nop
		setBL(0x020293A0, (u32)dsiSaveGetInfo);
		setBL(0x020293D4, (u32)dsiSaveCreate);
		setBL(0x020293FC, (u32)dsiSaveOpen);
		setBL(0x02029424, (u32)dsiSaveSetLength);
		setBL(0x0202943C, (u32)dsiSaveWrite);
		setBL(0x02029444, (u32)dsiSaveClose);
		setBL(0x020294A8, (u32)dsiSaveOpen);
		setBL(0x020294D0, (u32)dsiSaveSetLength);
		setBL(0x02029554, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x020295AC, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x020295DC, (u32)dsiSaveClose);
		*(u32*)0x020296D0 = 0xE1A00000; // nop
	} */

	// Cave Story (USA)
	else if (strcmp(romTid, "KCVE") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200526C = 0xE1A00000; // nop
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
		*(u32*)0x02005B88 = 0xE1A00000; // nop
		setBL(0x02005BAC, (u32)dsiSaveOpen);
		setBL(0x02005BD8, (u32)dsiSaveOpen);
		setBL(0x02005BF4, (u32)dsiSaveGetLength);
		setBL(0x02005C08, (u32)dsiSaveClose);
		setBL(0x02005C28, (u32)dsiSaveSeek);
		setBL(0x02005C38, (u32)dsiSaveWrite);
		setBL(0x02005C40, (u32)dsiSaveClose);
		*(u32*)0x02005C54 = 0xE1A00000; // nop
		*(u32*)0x02005C60 = 0xE1A00000; // nop
		*(u32*)0x02005C64 = 0xE1A00000; // nop
		/* if (useSharedFont) {
			if (expansionPakFound) {
				patchTwlFontLoad(0x020616F0, 0x0207DCAC);
			}
		} else { */
			*(u32*)0x0200A12C = 0xE1A00000; // nop (Skip Manual screen)
		//}
		*(u32*)0x0207342C = 0xE1A00000; // nop
		tonccpy((u32*)0x02073FA4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0207654C = 0xE1A00000; // nop
		patchInitDSiWare(0x0207C180, heapEnd);
		patchUserSettingsReadDSiWare(0x0207D758);
	}

	// Chess Challenge! (USA)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KCTE") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		// useSharedFont = twlFontFound;
		// if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x02022594, 0x02067BDC);
			} */
		// } else {
			*(u32*)0x02005104 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// }
		*(u32*)0x0200553C = 0xE1A00000; // nop
		*(u32*)0x0200556C = 0xE1A00000; // nop
		*(u32*)0x02022ADC = 0xE1A00000; // nop
		*(u32*)0x02022B0C = 0xE1A00000; // nop
		*(u32*)0x02022B18 = 0xE1A00000; // nop
		*(u32*)0x02022B54 = 0xE1A00000; // nop
		*(u32*)0x02022B60 = 0xE1A00000; // nop
		*(u32*)0x020475C4 = 0xE12FFF1E; // bx lr
		if (!extendedMemory2) {
			// *(u32*)0x02047F9C = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
			*(u32*)0x02047FE8 = 0xE3A0078A; // mov r0, #0x02280000
		}
		*(u32*)0x0205E150 = 0xE1A00000; // nop
		*(u32*)0x020612A8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206604C, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		*(u32*)0x020663D8 = extendedMemory2 ? *(u32*)0x02004FD0 : mepAddr;
		patchUserSettingsReadDSiWare(0x02067680);
		*(u32*)0x02067AA0 = 0xE1A00000; // nop
		*(u32*)0x02067AA4 = 0xE1A00000; // nop
		*(u32*)0x02067AA8 = 0xE1A00000; // nop
		*(u32*)0x02067AAC = 0xE1A00000; // nop
		*(u32*)0x0206C710 = 0xE3A00003; // mov r0, #3
		*(u32*)0x0206D138 = 0xE3A00000; // mov r0, #0 (Disable wireless communications to save battery power)
		*(u32*)0x0206D13C = 0xE12FFF1E; // bx lr
	}

	// Chess Challenge! (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KCTV") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		// useSharedFont = twlFontFound;
		// if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x02022430, 0x02067BF0);
			} */
		// } else {
			*(u32*)0x02005104 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// }
		*(u32*)0x0200553C = 0xE1A00000; // nop
		*(u32*)0x0200556C = 0xE1A00000; // nop
		*(u32*)0x02022978 = 0xE1A00000; // nop
		*(u32*)0x020229A8 = 0xE1A00000; // nop
		*(u32*)0x020229B4 = 0xE1A00000; // nop
		*(u32*)0x020229F0 = 0xE1A00000; // nop
		*(u32*)0x020229FC = 0xE1A00000; // nop
		*(u32*)0x020475A0 = 0xE12FFF1E; // bx lr
		if (!extendedMemory2) {
			// *(u32*)0x02047F78 = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
			*(u32*)0x02047FC4 = 0xE3A0078A; // mov r0, #0x02280000
		}
		*(u32*)0x0205E12C = 0xE1A00000; // nop
		*(u32*)0x02061290 = 0xE1A00000; // nop
		patchInitDSiWare(0x02066050, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		*(u32*)0x020663DC = extendedMemory2 ? *(u32*)0x02004FD0 : mepAddr;
		patchUserSettingsReadDSiWare(0x02067694);
		*(u32*)0x02067A5C = 0xE1A00000; // nop
		*(u32*)0x02067A60 = 0xE1A00000; // nop
		*(u32*)0x02067A64 = 0xE1A00000; // nop
		*(u32*)0x02067A68 = 0xE1A00000; // nop
		*(u32*)0x0206C724 = 0xE3A00003; // mov r0, #3
		*(u32*)0x0206D14C = 0xE3A00000; // mov r0, #0 (Disable wireless communications to save battery power)
		*(u32*)0x0206D150 = 0xE12FFF1E; // bx lr
	}

	// Chotto DS Bun ga Kuzenshuu: Sekai no Bungaku 20 (Japan)
	else if (strcmp(romTid, "KBGJ") == 0) {
		*(u32*)0x02005B24 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02021328 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02021DD0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02021F08 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0204D30C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0204D310 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02050A50 = 0xE1A00000; // nop
		patchInitDSiWare(0x02058A10, heapEnd);
		*(u32*)0x02058D8C = *(u32*)0x02004FBC;
		patchUserSettingsReadDSiWare(0x02059F74);
	}

	// Christmas Wonderland (USA)
	else if (strcmp(romTid, "KXWE") == 0) {
		*(u32*)0x0200E1A4 = 0xE1A00000; // nop
		*(u32*)0x02011670 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016394, heapEnd);
		patchUserSettingsReadDSiWare(0x02017844);
		if (!extendedMemory2) {
			*(u32*)0x0201C9F8 = 0xE3A05901; // mov r5, #0x4000 (Disable music)
			*(u32*)0x0201CA90 = 0xE1A00000; // nop (Disable sound effects)
		}
		*(u32*)0x0201D440 = 0xE1A00000; // nop
		*(u32*)0x0201D454 = 0xE1A00000; // nop
		setBL(0x0203D60C, (u32)dsiSaveOpen);
		setBL(0x0203D71C, (u32)dsiSaveRead);
		setBL(0x0203D724, (u32)dsiSaveClose);
		*(u32*)0x0203D75C = 0xE1A00000; // nop
		setBL(0x0203D918, (u32)dsiSaveOpen);
		setBL(0x0203D940, (u32)dsiSaveCreate);
		setBL(0x0203D950, (u32)dsiSaveOpen);
		setBL(0x0203D96C, (u32)dsiSaveSetLength);
		setBL(0x0203D9A4, (u32)dsiSaveSeek);
		setBL(0x0203D9B4, (u32)dsiSaveWrite);
		setBL(0x0203D9BC, (u32)dsiSaveClose);
		*(u32*)0x0203DA2C = 0xE1A00000; // nop
	}

	// Christmas Wonderland (Europe)
	else if (strcmp(romTid, "KXWP") == 0) {
		*(u32*)0x0200E0D0 = 0xE1A00000; // nop
		*(u32*)0x0201159C = 0xE1A00000; // nop
		patchInitDSiWare(0x020162C0, heapEnd);
		patchUserSettingsReadDSiWare(0x02017770);
		if (!extendedMemory2) {
			*(u32*)0x0201C924 = 0xE3A05901; // mov r5, #0x4000 (Disable music)
			*(u32*)0x0201C9BC = 0xE1A00000; // nop (Disable sound effects)
		}
		*(u32*)0x0201D36C = 0xE1A00000; // nop
		*(u32*)0x0201D380 = 0xE1A00000; // nop
		setBL(0x0203D4E8, (u32)dsiSaveOpen);
		setBL(0x0203D5F8, (u32)dsiSaveRead);
		setBL(0x0203D600, (u32)dsiSaveClose);
		*(u32*)0x0203D638 = 0xE1A00000; // nop
		setBL(0x0203D7F4, (u32)dsiSaveOpen);
		setBL(0x0203D81C, (u32)dsiSaveCreate);
		setBL(0x0203D82C, (u32)dsiSaveOpen);
		setBL(0x0203D848, (u32)dsiSaveSetLength);
		setBL(0x0203D880, (u32)dsiSaveSeek);
		setBL(0x0203D890, (u32)dsiSaveWrite);
		setBL(0x0203D898, (u32)dsiSaveClose);
		*(u32*)0x0203D908 = 0xE1A00000; // nop
	}

	// Christmas Wonderland 2 (USA)
	// Christmas Wonderland 2 (Europe)
	else if (strcmp(romTid, "K2WE") == 0 || strcmp(romTid, "K2WP") == 0) {
		u8 offsetChange58 = (romTid[3] == 'E') ? 0 : 0x58;
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x70;

		*(u32*)0x0200E100 = 0xE1A00000; // nop
		*(u32*)0x020115CC = 0xE1A00000; // nop
		patchInitDSiWare(0x02016388, heapEnd);
		patchUserSettingsReadDSiWare(0x02017838);
		if (!extendedMemory2) {
			*(u32*)(0x02034A60+offsetChange58) = 0xE3A05901; // mov r5, #0x4000 (Disable music)
		}
		*(u32*)(0x02035C90+offsetChange58) = 0xE1A00000; // nop
		*(u32*)(0x02035CA4+offsetChange58) = 0xE1A00000; // nop
		setBL(0x020375F4+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02037708+offsetChange, (u32)dsiSaveRead);
		setBL(0x02037710+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02037748+offsetChange) = 0xE1A00000; // nop
		setBL(0x02037904+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0203792C+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0203793C+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02037958+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x02037990+offsetChange, (u32)dsiSaveSeek);
		setBL(0x020379A0+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020379A8+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02037A1C+offsetChange) = 0xE1A00000; // nop
	}

	// Chronicles of Vampires: Awakening (USA)
	// Chronicles of Vampires: Origins (USA)
	else if (strcmp(romTid, "KVVE") == 0 || strcmp(romTid, "KVWE") == 0) {
		*(u32*)0x0207E194 = 0xE1A00000; // nop
		*(u32*)0x02081504 = 0xE1A00000; // nop
		patchInitDSiWare(0x020873E8, heapEnd);
		*(u32*)0x02087774 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02088BD4);
		*(u32*)0x020A14A0 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x020ACFC4, (u32)dsiSaveOpen);
		setBL(0x020ACFF4, (u32)dsiSaveRead);
		setBL(0x020ACFFC, (u32)dsiSaveClose);
		setBL(0x020AD0CC, (u32)dsiSaveOpen);
		setBL(0x020AD0FC, (u32)dsiSaveRead);
		setBL(0x020AD104, (u32)dsiSaveClose);
		*(u32*)0x020AD1F0 = 0xE1A00000; // nop
		*(u32*)0x020AD208 = 0xE1A00000; // nop
		setBL(0x020AD264, (u32)dsiSaveOpen);
		setBL(0x020AD294, (u32)dsiSaveRead);
		setBL(0x020AD29C, (u32)dsiSaveClose);
		setBL(0x020AD328, (u32)dsiSaveOpen);
		setBL(0x020AD354, (u32)dsiSaveRead);
		setBL(0x020AD35C, (u32)dsiSaveClose);
		*(u32*)0x020AD3C8 = 0xE1A00000; // nop
		*(u32*)0x020AD3D8 = 0xE1A00000; // nop
		setBL(0x020AD42C, (u32)dsiSaveOpen);
		setBL(0x020AD440, (u32)dsiSaveClose);
		setBL(0x020AD4A8, (u32)dsiSaveOpen);
		setBL(0x020AD4BC, (u32)dsiSaveClose);
		setBL(0x020AD4D0, (u32)dsiSaveCreate);
		setBL(0x020AD4EC, (u32)dsiSaveOpen);
		*(u32*)0x020AD4FC = 0xE1A00000; // nop
		setBL(0x020AD508, (u32)dsiSaveClose);
		setBL(0x020AD510, (u32)dsiSaveDelete);
		setBL(0x020AD528, (u32)dsiSaveCreate);
		setBL(0x020AD538, (u32)dsiSaveOpen);
		setBL(0x020AD554, (u32)dsiSaveSetLength);
		setBL(0x020AD564, (u32)dsiSaveWrite);
		setBL(0x020AD56C, (u32)dsiSaveClose);
	}

	// Chronos Twins: One Hero in Two Times (USA)
	// Overlay-related crash
	/*else if (strcmp(romTid, "K9TE") == 0) {
		*(u32*)0x0200B7AC = 0xE1A00000; // nop
		*(u32*)0x0200F400 = 0xE1A00000; // nop
		*(u32*)0x02013488 = 0xE1A00000; // nop
		*(u32*)0x0201526C = 0xE1A00000; // nop
		*(u32*)0x02015270 = 0xE1A00000; // nop
		*(u32*)0x0201527C = 0xE1A00000; // nop
		*(u32*)0x020153C0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201541C, heapEnd); // mov r0, #0x23E0000
	}*/

	// Chuck E. Cheese's Alien Defense Force (USA)
	else if (strcmp(romTid, "KUQE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200BB6C = 0xE1A00000; // nop
		*(u32*)0x0200F008 = 0xE1A00000; // nop
		patchInitDSiWare(0x02014064, heapEnd);
		patchUserSettingsReadDSiWare(0x02015504);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201B808, 0x02015A84);
			}
		} else {
			*(u32*)0x0201B9E4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 4; i++) {
				u32* offset = (u32*)0x0202D43C;
				offset[i] = 0xE1A00000; // nop
			}
		}
		setBL(0x0201BBA4, (u32)dsiSaveCreate);
		setBL(0x0201BBB4, (u32)dsiSaveOpen);
		setBL(0x0201BBD0, (u32)dsiSaveSeek);
		setBL(0x0201BBE0, (u32)dsiSaveWrite);
		setBL(0x0201BBE8, (u32)dsiSaveClose);
		setBL(0x0201BD10, (u32)dsiSaveOpenR);
		setBL(0x0201BD28, (u32)dsiSaveSeek);
		setBL(0x0201BD38, (u32)dsiSaveRead);
		setBL(0x0201BD40, (u32)dsiSaveClose);
		*(u32*)0x0201BD60 = 0xE1A00000; // nop
	}

	// Chuck E. Cheese's Arcade Room (USA)
	else if (strcmp(romTid, "KUCE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02013978 = 0xE1A00000; // nop
		*(u32*)0x02016E14 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BFC0, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D460);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02045814, 0x0201D9E0);
			}
		} else {
			*(u32*)0x02032550 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x020459F0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		setBL(0x02045BAC, (u32)dsiSaveCreate);
		setBL(0x02045BBC, (u32)dsiSaveOpen);
		setBL(0x02045BD8, (u32)dsiSaveSeek);
		setBL(0x02045BE8, (u32)dsiSaveWrite);
		setBL(0x02045BF0, (u32)dsiSaveClose);
		setBL(0x02045D1C, (u32)dsiSaveOpenR);
		setBL(0x02045D34, (u32)dsiSaveSeek);
		setBL(0x02045D44, (u32)dsiSaveRead);
		setBL(0x02045D4C, (u32)dsiSaveClose);
		*(u32*)0x02045D6C = 0xE1A00000; // nop
	}

	// Chuugaku Eijukugo: Kiho 150 Go Master (Japan)
	// Koukou Eitango: Kiho 400 Go Master (Japan)
	else if (strcmp(romTid, "KJCJ") == 0 || strcmp(romTid, "KEKJ") == 0) {
		u8 offsetChange1 = (strncmp(romTid, "KJC", 3) == 0) ? 0 : 0x48;
		u16 offsetChange2 = (strncmp(romTid, "KJC", 3) == 0) ? 0 : 0x114;
		u16 offsetChange3 = (strncmp(romTid, "KJC", 3) == 0) ? 0 : 0x110;

		*(u32*)0x02012C6C = 0xE1A00000; // nop
		*(u32*)0x02016330 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C2DC, heapEnd);
		// *(u32*)0x0201C64C = *(u32*)0x020013C0;
		patchUserSettingsReadDSiWare(0x0201DA44);
		setBL(0x0202CAB8-offsetChange1, (u32)dsiSaveCreate);
		*(u32*)(0x0204F20C-offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x0204F230-offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x020643A4-offsetChange2) = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)(0x02064580-offsetChange3) = 0xE3A00000; // mov r0, #0
		setBL(0x02076DD0-offsetChange3, (u32)dsiSaveOpen);
		setBL(0x02076DEC-offsetChange3, (u32)dsiSaveSeek);
		setBL(0x02076E00-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076E18-offsetChange3, (u32)dsiSaveRead);
		setBL(0x02076E28-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076E34-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076E68-offsetChange3, (u32)dsiSaveOpen);
		setBL(0x02076E80-offsetChange3, (u32)dsiSaveSeek);
		setBL(0x02076E98-offsetChange3, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02076EC8-offsetChange3, (u32)dsiSaveOpen);
		setBL(0x02076EE0-offsetChange3, (u32)dsiSaveSetLength);
		setBL(0x02076EF0-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076F04-offsetChange3, (u32)dsiSaveSeek);
		setBL(0x02076F18-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076F30-offsetChange3, (u32)dsiSaveWrite);
		setBL(0x02076F40-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076F4C-offsetChange3, (u32)dsiSaveClose);
		setBL(0x02076F80-offsetChange3, (u32)dsiSaveOpen);
		setBL(0x02076F94-offsetChange3, (u32)dsiSaveSetLength);
		setBL(0x02076FAC-offsetChange3, (u32)dsiSaveSeek);
		setBL(0x02076FC4-offsetChange3, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)(0x02077124-offsetChange3) = 0xE12FFF1E; // bx lr
	}

	// Chuugaku Eitango: Kiho 400 Go Master (Japan)
	else if (strcmp(romTid, "KETJ") == 0) {
		*(u32*)0x02016988 = 0xE1A00000; // nop
		*(u32*)0x02019EF0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FB58, heapEnd);
		patchUserSettingsReadDSiWare(0x020212B8);
		*(u32*)0x02026440 = 0xE1A00000; // nop
		*(u32*)0x02026464 = 0xE1A00000; // nop
		*(u32*)0x0202933C = 0xE12FFF1E; // bx lr
		*(u32*)0x0202C9FC = 0xE3A00000; // mov r0, #0
		setBL(0x02030B24, (u32)dsiSaveCreate);
		setBL(0x020312A8, (u32)dsiSaveClose);
		setBL(0x02031364, (u32)dsiSaveOpen);
		setBL(0x02031380, (u32)dsiSaveSeek);
		setBL(0x02031394, (u32)dsiSaveClose);
		setBL(0x020313AC, (u32)dsiSaveRead);
		setBL(0x020313BC, (u32)dsiSaveClose);
		setBL(0x020313C8, (u32)dsiSaveClose);
		setBL(0x020313FC, (u32)dsiSaveOpen);
		setBL(0x02031414, (u32)dsiSaveSeek);
		setBL(0x0203142C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0203145C, (u32)dsiSaveOpen);
		setBL(0x02031474, (u32)dsiSaveSetLength);
		setBL(0x02031484, (u32)dsiSaveClose);
		setBL(0x02031498, (u32)dsiSaveSeek);
		setBL(0x020314AC, (u32)dsiSaveClose);
		setBL(0x020314C4, (u32)dsiSaveWrite);
		setBL(0x020314D4, (u32)dsiSaveClose);
		setBL(0x020314E0, (u32)dsiSaveClose);
		setBL(0x02031514, (u32)dsiSaveOpen);
		setBL(0x02031528, (u32)dsiSaveSetLength);
		setBL(0x02031540, (u32)dsiSaveSeek);
		setBL(0x02031558, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x02058BB4 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x0206FE0C = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
	}

	// Chuuga Kukihon' Eitango: Wado Pazuru (Japan)
	else if (strcmp(romTid, "KWPJ") == 0) {
		*(u32*)0x0200817C = 0xE1A00000; // nop
		*(u32*)0x0200B89C = 0xE1A00000; // nop
		patchInitDSiWare(0x02011724, heapEnd);
		patchUserSettingsReadDSiWare(0x02012E8C);
		*(u32*)0x02020428 = 0xE1A00000; // nop
		setBL(0x02020634, (u32)dsiSaveClose);
		*(u32*)0x02020698 = 0xE1A00000; // nop
		setBL(0x02020788, (u32)dsiSaveClose);
		setBL(0x0202092C, (u32)dsiSaveOpen);
		setBL(0x02020954, (u32)dsiSaveSeek);
		setBL(0x02020970, (u32)dsiSaveClose);
		setBL(0x02020988, (u32)dsiSaveRead);
		setBL(0x020209A8, (u32)dsiSaveClose);
		setBL(0x020209B8, (u32)dsiSaveClose);
		setBL(0x020209F4, (u32)dsiSaveOpen);
		setBL(0x02020A0C, (u32)dsiSaveSeek);
		setBL(0x02020A24, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02020A58, (u32)dsiSaveOpen);
		setBL(0x02020A78, (u32)dsiSaveSetLength);
		setBL(0x02020A88, (u32)dsiSaveClose);
		setBL(0x02020AA4, (u32)dsiSaveSeek);
		setBL(0x02020AC0, (u32)dsiSaveClose);
		setBL(0x02020AD8, (u32)dsiSaveWrite);
		setBL(0x02020AFC, (u32)dsiSaveClose);
		setBL(0x02020B08, (u32)dsiSaveClose);
		setBL(0x02020B48, (u32)dsiSaveOpen);
		setBL(0x02020B5C, (u32)dsiSaveSetLength);
		setBL(0x02020B74, (u32)dsiSaveSeek);
		setBL(0x02020B8C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02020BE4, (u32)dsiSaveCreate);
		setBL(0x02020BEC, (u32)dsiSaveGetResultCode);
		*(u32*)0x02032D10 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		*(u32*)0x0203A864 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x020450D0 = 0xE3A00000; // mov r0, #0
	}

	// Gibonyeongdaneo: Wodeu Peojeul (Korea)
	else if (strcmp(romTid, "KWPK") == 0) {
		*(u32*)0x0200BEB4 = 0xE1A00000; // nop
		*(u32*)0x0200F4E0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02015108, heapEnd);
		patchUserSettingsReadDSiWare(0x02016868);
		*(u32*)0x020235A8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02023948 = 0xE1A00000; // nop
		setBL(0x02023B54, (u32)dsiSaveClose);
		*(u32*)0x02023BB8 = 0xE1A00000; // nop
		setBL(0x02023CA8, (u32)dsiSaveClose);
		setBL(0x02023E4C, (u32)dsiSaveOpen);
		setBL(0x02023E74, (u32)dsiSaveSeek);
		setBL(0x02023E90, (u32)dsiSaveClose);
		setBL(0x02023EA8, (u32)dsiSaveRead);
		setBL(0x02023EC8, (u32)dsiSaveClose);
		setBL(0x02023ED8, (u32)dsiSaveClose);
		setBL(0x02023F14, (u32)dsiSaveOpen);
		setBL(0x02023F2C, (u32)dsiSaveSeek);
		setBL(0x02023F44, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02023F78, (u32)dsiSaveOpen);
		setBL(0x02023F98, (u32)dsiSaveSetLength);
		setBL(0x02023FA8, (u32)dsiSaveClose);
		setBL(0x02023FC4, (u32)dsiSaveSeek);
		setBL(0x02023FE0, (u32)dsiSaveClose);
		setBL(0x02023FF8, (u32)dsiSaveWrite);
		setBL(0x0202401C, (u32)dsiSaveClose);
		setBL(0x02024028, (u32)dsiSaveClose);
		setBL(0x02024068, (u32)dsiSaveOpen);
		setBL(0x0202407C, (u32)dsiSaveSetLength);
		setBL(0x02024094, (u32)dsiSaveSeek);
		setBL(0x020240AC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02024104, (u32)dsiSaveCreate);
		setBL(0x0202410C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02031C74 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x02039390 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
	}

	// Chuukara! Dairoujou (Japan)
	else if (strcmp(romTid, "KQLJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200DDFC = 0xE1A00000; // nop
		*(u32*)0x020115C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201DB0C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201F008);
		*(u32*)0x0201F024 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F028 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F030 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201F034 = 0xE12FFF1E; // bx lr
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
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02047734, 0x0201F574);
			*(u32*)0x020477F4 = 0xE1A00000; // nop
		}
	}

	// Clash of Elementalists (USA)
	// Clash of Elementalists (Europe)
	// Requires Memory Expansion Pak
	// Crashes when stage starts
	/*else if ((strcmp(romTid, "KVLE") == 0 || strcmp(romTid, "KVLP") == 0) && expansionPakFound) {
		extern u32* mepHeapSetPatch;
		extern u32* elementalistsHeapAlloc;

		tonccpy((u32*)0x02002004, mepHeapSetPatch, 0x1C);

		*(u32*)0x0200C038 = 0xE1A00000; // nop
		*(u32*)0x0200C160 = 0xE1A00000; // nop
		*(u32*)0x0200C174 = 0xE1A00000; // nop
		*(u32*)0x0200F3C4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201717C, heapEndExceed);
		*(u32*)0x02017508 -= 0x30000;
		patchUserSettingsReadDSiWare(0x0201875C);
		*(u32*)0x02018778 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201877C = 0xE12FFF1E; // bx lr
		*(u32*)0x02018784 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02018788 = 0xE12FFF1E; // bx lr
		tonccpy((u32*)0x02018C50, elementalistsHeapAlloc, 0xC0);
		if (romTid[3] == 'E') {
			*(u32*)0x0202627C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			setBL(0x02027524, 0x02018C50);
			*(u32*)0x02028B6C = 0xE1A00000; // nop
			*(u32*)0x02028B70 = 0xE1A00000; // nop
			*(u32*)0x02028B88 = 0xE1A00000; // nop
			*(u32*)0x02028B90 = 0xE1A00000; // nop
			*(u32*)0x02028BB0 = 0xE1A00000; // nop
			*(u32*)0x0202B8CC = 0xE1A00000; // nop
			*(u32*)0x0202B8E8 = 0xE1A00000; // nop
			*(u32*)0x02002000 = (u32)getOffsetFromBL((u32*)0x02040A5C);
			*(u32*)0x020409AC = 0xE1A00000; // nop
			setBL(0x02040A5C, 0x02002004);
		} else {
			*(u32*)0x02028C58 = 0xE1A00000; // nop
			*(u32*)0x02028C5C = 0xE1A00000; // nop
			*(u32*)0x02028C74 = 0xE1A00000; // nop
			*(u32*)0x02028C7C = 0xE1A00000; // nop
			*(u32*)0x02028C9C = 0xE1A00000; // nop
			*(u32*)0x0202BAC8 = 0xE1A00000; // nop
			*(u32*)0x0202BAE4 = 0xE1A00000; // nop
		}
	}*/

	// Clubhouse Games Express: Card Classics (USA, Australia)
	else if (strcmp(romTid, "KTRT") == 0) {
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
		*(u32*)0x0205FE18 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		// *(u32*)0x020720F4 = 0xE3A00001; // mov r0, #1 (Enable TWL soft-reset function)
	}

	// A Little Bit of... All-Time Classics: Card Classics (Europe)
	else if (strcmp(romTid, "KTRP") == 0) {
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
		*(u32*)0x0205FD10 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Chotto Asobi Taizen: Jikkuri Toranpu (Japan)
	else if (strcmp(romTid, "KTRJ") == 0) {
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
		*(u32*)0x02060EC4 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Yixia Xia Ming Liu Daquan: Zhiyong Shuangquan (China)
	else if (strcmp(romTid, "KTRC") == 0) {
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
		*(u32*)0x0205EDBC = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Clubhouse Games Express: Card Classics (Korea)
	else if (strcmp(romTid, "KTRK") == 0) {
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
		*(u32*)0x02061700 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Clubhouse Games Express: Family Favorites (USA, Australia)
	else if (strcmp(romTid, "KTCT") == 0) {
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
		*(u32*)0x020639B0 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// A Little Bit of... All-Time Classics: Family Games (Europe)
	else if (strcmp(romTid, "KTPP") == 0) {
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
		*(u32*)0x020638F8 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Chotto Asobi Taizen: Otegaru Toranpu (Japan)
	else if (strcmp(romTid, "KTPJ") == 0) {
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
		*(u32*)0x020600F8 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Yixia Xia Ming Liu Daquan: Qingsong Xiuxian (China)
	else if (strcmp(romTid, "KTPC") == 0) {
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
		*(u32*)0x02060F48 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Clubhouse Games Express: Family Favorites (Korea)
	else if (strcmp(romTid, "KTPK") == 0) {
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
		*(u32*)0x0205F6C4 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Clubhouse Games Express: Strategy Pack (USA, Australia)
	else if (strcmp(romTid, "KTDT") == 0) {
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
		*(u32*)0x02063838 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// A Little Bit of... All-Time Classics: Strategy Games (Europe)
	else if (strcmp(romTid, "KTBP") == 0) {
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
		*(u32*)0x02063774 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Chotto Asobi Taizen: Onajimi Teburu (Japan)
	else if (strcmp(romTid, "KTBJ") == 0) {
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
		*(u32*)0x020604B4 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Yixia Xia Ming Liu Daquan: Jingdian Zhongwen (China)
	else if (strcmp(romTid, "KTBC") == 0) {
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
		*(u32*)0x0205EC38 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Clubhouse Games Express: Strategy Pack (Korea)
	else if (strcmp(romTid, "KTBK") == 0) {
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
		*(u32*)0x02060E30 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Color Commando (USA)
	// Color Commando (Europe) (Rev 0)
	// Color Commando (Europe) (Rev 1)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KXFE") == 0 || strcmp(romTid, "KXFP") == 0) {
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x0200AD6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0200CF04 = 0xE1A00000; // nop
		*(u32*)0x0202D958 = 0xE1A00000; // nop
		*(u32*)0x02030BC4 = 0xE1A00000; // nop
		if (romTid[3] == 'P' && ndsHeader->romversion == 1) {
			patchInitDSiWare(0x02035694, heapEnd);
			patchUserSettingsReadDSiWare(0x02036C9C);
		} else {
			patchInitDSiWare(0x02035768, heapEnd);
			patchUserSettingsReadDSiWare(0x02036D70);
		}
	}

	// Commando: Steel Disaster (USA)
	// Commando: Steel Disaster (Europe)
	else if (strcmp(romTid, "KC7E") == 0 || strcmp(romTid, "KC7P") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x30;
		u8 offsetChangeS = (romTid[3] == 'E') ? 0 : 0x1C;

		*(u32*)(0x0200C394-offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0200CF18-offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0200F8C8-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201481C-offsetChange, heapEnd);
		*(u32*)(0x02014BA8-offsetChange) = *(u32*)0x02004FE8;
		setBL(0x02065368+offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x0206537C+offsetChangeS, (u32)dsiSaveOpen);
		*(u32*)(0x020653BC+offsetChangeS) = 0xE1A00000; // nop
		setBL(0x020653D4+offsetChangeS, (u32)dsiSaveSetLength);
		setBL(0x020653E4+offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x020653F4+offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x020653FC+offsetChangeS, (u32)dsiSaveClose);
		setBL(0x0206543C+offsetChangeS, (u32)dsiSaveOpen);
		*(u32*)(0x02065478+offsetChangeS) = 0xE1A00000; // nop
		setBL(0x0206548C+offsetChangeS, (u32)dsiSaveGetLength);
		setBL(0x020654B8+offsetChangeS, (u32)dsiSaveRead);
		*(u32*)(0x020654EC+offsetChangeS) = 0xE1A00000; // nop
		setBL(0x02065500+offsetChangeS, (u32)dsiSaveClose);
	}

	// Coropata (Japan)
	else if (strcmp(romTid, "K56J") == 0) {
		*(u32*)0x020110B8 = 0xE1A00000; // nop
		tonccpy((u32*)0x02011C3C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02014610 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AFC4, heapEnd);
		*(u32*)0x0201B350 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201C618);
		*(u32*)0x020699A8 = 0xE1A00000; // nop (Soft-lock instead of displaying Manual screen)
		setBL(0x0206E1E4, (u32)dsiSaveOpen);
		setBL(0x0206E1F4, (u32)dsiSaveGetLength);
		setBL(0x0206E204, (u32)dsiSaveClose);
		setBL(0x0206E20C, (u32)dsiSaveDelete);
		setBL(0x0206E238, (u32)dsiSaveClose);
		setBL(0x0206E28C, (u32)dsiSaveCreate);
		setBL(0x0206E29C, (u32)dsiSaveOpen);
		setBL(0x0206E2C8, (u32)dsiSaveSetLength);
		setBL(0x0206E2E4, (u32)dsiSaveClose);
		setBL(0x0206E318, (u32)dsiSaveWrite);
		setBL(0x0206E334, (u32)dsiSaveClose);
		setBL(0x0206E348, (u32)dsiSaveClose);
		*(u32*)0x0206E618 = 0xE1A00000; // nop
		setBL(0x0206E9DC, (u32)dsiSaveOpen);
		setBL(0x0206E9FC, (u32)dsiSaveSeek);
		setBL(0x0206EA0C, (u32)dsiSaveRead);
		setBL(0x0206EA14, (u32)dsiSaveClose);
		setBL(0x0206EA54, (u32)dsiSaveOpen);
		setBL(0x0206EA74, (u32)dsiSaveSeek);
		setBL(0x0206EA84, (u32)dsiSaveWrite);
		setBL(0x0206EA8C, (u32)dsiSaveClose);
		*(u32*)0x0206FB78 = 0x61A80;
	}

	// Crash-Course Domo (USA)
	else if (strcmp(romTid, "KDCE") == 0) {
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
		doubleNopT(0x0200E1E6);
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
		*(u16*)0x02015418 = nopT;
		doubleNopT(0x02023C72);
		doubleNopT(0x02025EC2);
		doubleNopT(0x02028B44);
		doubleNopT(0x0202A0FE);
		doubleNopT(0x0202A102);
		doubleNopT(0x0202A10E);
		doubleNopT(0x0202A1F2);
		patchHiHeapDSiWareThumb(0x0202A230, 0x02024B7C, heapEnd); // movs r0, #0x23E0000
		doubleNopT(0x0202B2B6);
		*(u16*)0x0202B2BA = nopT;
		*(u16*)0x0202B2BC = nopT;
		doubleNopT(0x0202B2BE);
	}

	// Crazy Golf (USA)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KZGE") == 0) {
		*(u32*)0x0200B760 = 0xE12FFF1E; // bx lr
		/* setBL(0x0200B7C4, (u32)dsiSaveCreate);
		setBL(0x0200B7D4, (u32)dsiSaveOpen);
		setBL(0x0200B83C, (u32)dsiSaveWrite);
		setBL(0x0200B84C, (u32)dsiSaveClose);
		setBL(0x0200B864, (u32)dsiSaveWrite);
		setBL(0x0200B870, (u32)dsiSaveClose);
		setBL(0x0200B8C4, (u32)dsiSaveGetInfo);
		setBL(0x0200B940, (u32)dsiSaveOpen);
		setBL(0x0200B9A0, (u32)dsiSaveGetLength);
		setBL(0x0200B9C0, (u32)dsiSaveRead);
		setBL(0x0200B9D0, (u32)dsiSaveClose);
		setBL(0x0200B9E8, (u32)dsiSaveRead);
		setBL(0x0200B9F4, (u32)dsiSaveClose); */
		*(u32*)0x0200BA60 = 0xE1A00000; // nop
		*(u32*)0x02028390 = 0xE1A00000; // nop
		*(u32*)0x0202C110 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203264C, heapEnd);
		patchUserSettingsReadDSiWare(0x02033E48);
		*(u32*)0x02038B9C = 0xE3A00003; // mov r0, #3
		*(u32*)0x020395C4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020395C8 = 0xE12FFF1E; // bx lr
	}

	// Crazy Golf (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KZGV") == 0) {
		*(u32*)0x0200B348 = 0xE12FFF1E; // bx lr
		/* setBL(0x0200B3AC, (u32)dsiSaveCreate);
		setBL(0x0200B3BC, (u32)dsiSaveOpen);
		setBL(0x0200B424, (u32)dsiSaveWrite);
		setBL(0x0200B434, (u32)dsiSaveClose);
		setBL(0x0200B44C, (u32)dsiSaveWrite);
		setBL(0x0200B458, (u32)dsiSaveClose);
		setBL(0x0200B4AC, (u32)dsiSaveGetInfo);
		setBL(0x0200B528, (u32)dsiSaveOpen);
		setBL(0x0200B588, (u32)dsiSaveGetLength);
		setBL(0x0200B5A8, (u32)dsiSaveRead);
		setBL(0x0200B5B8, (u32)dsiSaveClose);
		setBL(0x0200B5D0, (u32)dsiSaveRead);
		setBL(0x0200B5DC, (u32)dsiSaveClose); */
		*(u32*)0x0200B648 = 0xE1A00000; // nop
		*(u32*)0x02027E48 = 0xE1A00000; // nop
		*(u32*)0x0202B710 = 0xE1A00000; // nop
		patchInitDSiWare(0x02031938, heapEnd);
		patchUserSettingsReadDSiWare(0x02033134);
		*(u32*)0x02037E54 = 0xE3A00003; // mov r0, #3
		*(u32*)0x0203887C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02038880 = 0xE12FFF1E; // bx lr
	}

	// Crazy Pinball (USA)
	// Saving not supported due to using more than one file in filesystem
	/* else if (strcmp(romTid, "KCIE") == 0) {
		*(u32*)0x02013F78 = 0xE1A00000; // nop
		*(u32*)0x02013FC0 = 0xE1A00000; // nop
		*(u32*)0x02013FC8 = 0xE1A00000; // nop
		*(u32*)0x0201405C = 0xE1A00000; // nop
		*(u32*)0x020142A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020145A8 = 0xE1A00000; // nop
		*(u32*)0x0204EEF4 = 0xE1A00000; // nop
		*(u32*)0x020527BC = 0xE1A00000; // nop
		patchInitDSiWare(0x02058630, heapEnd);
		*(u32*)0x020589BC -= 0x30000;
		patchUserSettingsReadDSiWare(0x02059DD0);
	} */

	// Crazy Pinball (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	/* else if (strcmp(romTid, "KCIV") == 0) {
		*(u32*)0x02014348 = 0xE12FFF1E; // bx lr
		*(u32*)0x020144C0 = 0xE1A00000; // nop
		*(u32*)0x0204EE0C = 0xE1A00000; // nop
		*(u32*)0x020526D4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02058548, heapEnd);
		*(u32*)0x020588D4 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02059CE8);
	} */

	// Crazy Sudoku (USA)
	// Crazy Sudoku (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KCRE") == 0 || strcmp(romTid, "KCRV") == 0) {
		u16 offsetChange = (romTid[3] == 'E') ? 0 : 0x200;

		if (romTid[3] == 'E') {
			*(u32*)0x0200926C = 0xE12FFF1E; // bx lr
			*(u32*)0x0200956C = 0xE1A00000; // nop
		} else {
			*(u32*)0x020091A4 = 0xE12FFF1E; // bx lr
			*(u32*)0x020094A4 = 0xE1A00000; // nop
		}
		*(u32*)(0x02023758-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020274D8-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0202D8A0-offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x0202EEE4-offsetChange);
	}

	// Crystal Adventure (USA)
	// Crystal Adventure (Europe)
	else if (strcmp(romTid, "KXDE") == 0 || strcmp(romTid, "KXDP") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x0206D988;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x140000, bssEnd, sdatOffset);
		// }

		*(u32*)0x0200C278 = 0xE1A00000; // nop
		*(u32*)0x0200F5CC = 0xE1A00000; // nop
		patchInitDSiWare(0x020149D4, heapEnd);
		*(u32*)0x02014D60 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02015E90);
		setBL(0x0201E5C0, (u32)dsiSaveGetInfo);
		setBL(0x0201E5D4, (u32)dsiSaveOpen);
		setBL(0x0201E5E8, (u32)dsiSaveCreate);
		setBL(0x0201E5F8, (u32)dsiSaveOpen);
		setBL(0x0201E608, (u32)dsiSaveGetResultCode);
		*(u32*)0x0201E61C = 0xE1A00000; // nop
		setBL(0x0201E628, (u32)dsiSaveCreate);
		setBL(0x0201E638, (u32)dsiSaveOpen);
		setBL(0x0201E8C8, (u32)dsiSaveSeek);
		setBL(0x0201E8DC, (u32)dsiSaveWrite);
		setBL(0x0201E8EC, (u32)dsiSaveSeek);
		setBL(0x0201E8FC, (u32)dsiSaveWrite);
		setBL(0x0201E90C, (u32)dsiSaveSeek);
		setBL(0x0201E91C, (u32)dsiSaveWrite);
		setBL(0x0201E9A8, (u32)dsiSaveSeek);
		setBL(0x0201E9B8, (u32)dsiSaveWrite);
		setBL(0x0201E9C8, (u32)dsiSaveClose);
		setBL(0x0201EA34, (u32)dsiSaveOpen);
		setBL(0x0201EA5C, (u32)dsiSaveSeek);
		setBL(0x0201EA6C, (u32)dsiSaveRead);
		setBL(0x0201EAAC, (u32)dsiSaveClose);
		setBL(0x0201ED7C, (u32)dsiSaveOpen);
		setBL(0x0201ED90, (u32)dsiSaveSeek);
		setBL(0x0201EDA0, (u32)dsiSaveWrite);
		setBL(0x0201EDA8, (u32)dsiSaveClose);
		*(u32*)0x020282C0 = 0xE1A00000; // nop
		*(u32*)0x02028398 = 0xE1A00000; // nop
	}

	// Crystal Caverns of Amon-Ra (USA)
	// Crystal Caverns of Amon-Ra (Europe)
	else if (strcmp(romTid, "KQQE") == 0 || strcmp(romTid, "KQQP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0xB0;

		if (romTid[3] == 'E') {
			*(u32*)0x02070BBC = 0xE1A00000; // nop
			*(u32*)0x02073EA4 = 0xE1A00000; // nop
			patchInitDSiWare(0x02079D6C, heapEnd);
			*(u32*)0x0207A0F8 = *(u32*)0x02004FD0;
			patchUserSettingsReadDSiWare(0x0207B558);
			*(u32*)0x0207B98C = 0xE1A00000; // nop
			*(u32*)0x0207B990 = 0xE1A00000; // nop
			*(u32*)0x0207B994 = 0xE1A00000; // nop
			*(u32*)0x0207B998 = 0xE1A00000; // nop
		} else {
			*(u32*)0x02070BC8 = 0xE1A00000; // nop
			*(u32*)0x02073F38 = 0xE1A00000; // nop
			patchInitDSiWare(0x02079E1C, heapEnd);
			*(u32*)0x0207A1A8 = *(u32*)0x02004FE8;
			patchUserSettingsReadDSiWare(0x0207B608);
			*(u32*)0x0207BA3C = 0xE1A00000; // nop
			*(u32*)0x0207BA40 = 0xE1A00000; // nop
			*(u32*)0x0207BA44 = 0xE1A00000; // nop
			*(u32*)0x0207BA48 = 0xE1A00000; // nop
		}
		*(u32*)(0x02093700+offsetChange) = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0209F2D8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F308+offsetChange, (u32)dsiSaveRead);
		setBL(0x0209F310+offsetChange, (u32)dsiSaveClose);
		setBL(0x0209F3E0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F410+offsetChange, (u32)dsiSaveRead);
		setBL(0x0209F418+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0209F504+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0209F51C+offsetChange) = 0xE1A00000; // nop
		setBL(0x0209F578+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F5A8+offsetChange, (u32)dsiSaveRead);
		setBL(0x0209F5B0+offsetChange, (u32)dsiSaveClose);
		setBL(0x0209F63C+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F668+offsetChange, (u32)dsiSaveRead);
		setBL(0x0209F670+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0209F6DC+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0209F6EC+offsetChange) = 0xE1A00000; // nop
		setBL(0x0209F740+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F754+offsetChange, (u32)dsiSaveClose);
		setBL(0x0209F7D8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F7EC+offsetChange, (u32)dsiSaveClose);
		setBL(0x0209F800+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0209F81C+offsetChange, (u32)dsiSaveOpen);
		*(u32*)(0x0209F82C+offsetChange) = 0xE1A00000; // nop
		setBL(0x0209F838+offsetChange, (u32)dsiSaveClose);
		setBL(0x0209F840+offsetChange, (u32)dsiSaveDelete);
		setBL(0x0209F858+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0209F868+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0209F884+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x0209F894+offsetChange, (u32)dsiSaveWrite);
		setBL(0x0209F89C+offsetChange, (u32)dsiSaveClose);
	}

	// CuteWitch! runner (USA)
	// CuteWitch! runner (Europe)
	// Stage music doesn't play on retail consoles
	else if (strncmp(romTid, "K32", 3) == 0) {
		u32* saveFuncOffsets[22] = {NULL};

		// useSharedFont = twlFontFound;
		*(u32*)0x0201B8CC = 0xE1A00000; // nop
		tonccpy((u32*)0x0201C450, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201ED48 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025278, heapEnd);
		*(u32*)0x02025604 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02026978);
		if (romTid[3] == 'E') {
			*(u32*)0x020620D8 = 0xE1A00000; // nop
			*(u32*)0x02062118 = 0xE1A00000; // nop
			*(u32*)0x020621FC = 0xE1A00000; // nop
			*(u32*)0x02062204 = 0xE1A00000; // nop
			*(u32*)0x02062318 = 0xE1A00000; // nop
			*(u32*)0x02062328 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02062394 = 0xE1A00000; // nop
			*(u32*)0x0206239C = 0xE1A00000; // nop
			*(u32*)0x02062428 = 0xE1A00000; // nop
			*(u32*)0x0206249C = 0xE1A00000; // nop
			*(u32*)0x020624A4 = 0xE1A00000; // nop
			*(u32*)0x02062530 = 0xE1A00000; // nop
			*(u32*)0x02062600 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02062638 = 0xE1A00000; // nop
			*(u32*)0x0206263C = 0xE1A00000; // nop
			*(u32*)0x02062640 = 0xE1A00000; // nop
			*(u32*)0x020626F4 = 0xE1A00000; // nop
			*(u32*)0x020626FC = 0xE1A00000; // nop
			*(u32*)0x02062928 = 0xE1A00000; // nop
			*(u32*)0x02062A40 = 0xE1A00000; // nop
			*(u32*)0x02062A48 = 0xE1A00000; // nop
			*(u32*)0x02062C0C = 0xE1A00000; // nop
			*(u32*)0x02062C34 = 0xE1A00000; // nop
			*(u32*)0x02062C38 = 0xE1A00000; // nop
			*(u32*)0x02062C3C = 0xE1A00000; // nop
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
		} else if (romTid[3] == 'P') {
			*(u32*)0x02093B14 = 0xE1A00000; // nop
			*(u32*)0x02093B54 = 0xE1A00000; // nop
			*(u32*)0x02093C38 = 0xE1A00000; // nop
			*(u32*)0x02093C40 = 0xE1A00000; // nop
			*(u32*)0x02093D54 = 0xE1A00000; // nop
			*(u32*)0x02093D64 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02093DD0 = 0xE1A00000; // nop
			*(u32*)0x02093DD8 = 0xE1A00000; // nop
			*(u32*)0x02093E64 = 0xE1A00000; // nop
			*(u32*)0x02093ED8 = 0xE1A00000; // nop
			*(u32*)0x02093EE0 = 0xE1A00000; // nop
			*(u32*)0x02093F6C = 0xE1A00000; // nop
			*(u32*)0x0209403C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02094074 = 0xE1A00000; // nop
			*(u32*)0x02094078 = 0xE1A00000; // nop
			*(u32*)0x0209407C = 0xE1A00000; // nop
			*(u32*)0x02094130 = 0xE1A00000; // nop
			*(u32*)0x02094138 = 0xE1A00000; // nop
			*(u32*)0x02094364 = 0xE1A00000; // nop
			*(u32*)0x0209447C = 0xE1A00000; // nop
			*(u32*)0x02094480 = 0xE1A00000; // nop
			*(u32*)0x02094484 = 0xE1A00000; // nop
			*(u32*)0x02094648 = 0xE1A00000; // nop
			*(u32*)0x02094670 = 0xE1A00000; // nop
			*(u32*)0x02094674 = 0xE1A00000; // nop
			*(u32*)0x02094678 = 0xE1A00000; // nop
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

		/* if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad((romTid[3] == 'E') ? 0x02076B5C : 0x0203C09C, 0x02026ECC);
		} */
	}

	// Dairojo! Samurai Defenders (USA)
	else if (strcmp(romTid, "KF3E") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200DDB4 = 0xE1A00000; // nop
		*(u32*)0x02011580 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201DAC4, heapEnd);
		patchUserSettingsReadDSiWare(0x0201EFC0);
		*(u32*)0x0201EFDC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EFE0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EFE8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EFEC = 0xE12FFF1E; // bx lr
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
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02047AB4, 0x0201F52C);
			*(u32*)0x02047B74 = 0xE1A00000; // nop
		}
	}

	// Karakuchi! Dairoujou (Japan)
	else if (strcmp(romTid, "KF3J") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200DD9C = 0xE1A00000; // nop
		*(u32*)0x020114D4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D9E0, heapEnd);
		patchUserSettingsReadDSiWare(0x0201EECC);
		*(u32*)0x0201EEE8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EEEC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EEF4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EEF8 = 0xE12FFF1E; // bx lr
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
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02047418, 0x0201F438);
			*(u32*)0x020474D8 = 0xE1A00000; // nop
		}
	}

	// Dancing Academy (Europe)
	else if (strcmp(romTid, "KINP") == 0) {
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050EC = 0xE1A00000; // nop
		*(u32*)0x02013474 = 0xE1A00000; // nop
		*(u32*)0x020171F0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E004, heapEnd);
		patchUserSettingsReadDSiWare(0x0201F7D0);
		setBL(0x0208D228, (u32)dsiSaveCreate);
		setBL(0x0208D238, (u32)dsiSaveOpen);
		setBL(0x0208D248, (u32)dsiSaveGetResultCode);
		setBL(0x0208D284, (u32)dsiSaveSetLength);
		setBL(0x0208D294, (u32)dsiSaveWrite);
		setBL(0x0208D29C, (u32)dsiSaveClose);
		setBL(0x0208D2D8, (u32)dsiSaveOpen);
		setBL(0x0208D2E8, (u32)dsiSaveGetResultCode);
		setBL(0x0208D300, (u32)dsiSaveGetLength);
		setBL(0x0208D310, (u32)dsiSaveRead);
		setBL(0x0208D318, (u32)dsiSaveClose);
		setBL(0x0208D350, (u32)dsiSaveOpen);
		setBL(0x0208D360, (u32)dsiSaveGetResultCode);
		setBL(0x0208D378, (u32)dsiSaveClose);

		// Skip Manual screen (Not working)
		// *(u32*)0x0208CFD8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// *(u32*)0x0208D06C = 0xE1A00000; // nop
		// *(u32*)0x0208D074 = 0xE1A00000; // nop
		// *(u32*)0x0208D080 = 0xE1A00000; // nop
	}

	// Dark Void Zero (USA)
	// Dark Void Zero (Europe, Australia)
	else if (strcmp(romTid, "KDVE") == 0 || strcmp(romTid, "KDVV") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02018A3C = 0xE1A00000; // nop
		*(u32*)0x02018A4C = 0xE1A00000; // nop
		tonccpy((u32*)0x02043DDC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02046DD8 = 0xE1A00000; // nop
		*(u32*)0x0204EE80 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204EE8C, heapEnd);
		patchUserSettingsReadDSiWare(0x0205052C);
		*(u32*)0x02052BD4 = 0xE1A00000; // nop
		*(u32*)0x02052C00 = 0xE1A00000; // nop
		*(u32*)0x02058A24 = 0xE1A00000; // nop
		*(u32*)0x02059C44 = 0xE1A00000; // nop
		/* if (useSharedFont && !extendedMemory2 && expansionPakFound) {
			patchTwlFontLoad(0x0206D254, 0x02050B78);
			*(u32*)0x0206D258 = 0xE1A00000; // nop
			*(u32*)0x0206D25C = 0xE1A00000; // nop
			*(u32*)0x0206D260 = 0xE1A00000; // nop
		} */
		*(u16*)0x020851A4 = nopT;
		*(u16*)0x020851A6 = nopT;
		*(u32*)0x020891BC = 0xE1A00000; // nop
		// *(u32*)0x0208AE4C = 0xE12FFF1E; // bx lr
		// *(u32*)0x0208B008 = 0xE12FFF1E; // bx lr
		*(u32*)0x0208AE6C = 0xE1A00000; // nop
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
		*(u32*)0x0208AFD0 = 0xE1A00000; // nop
		*(u32*)0x0208AFD8 = 0xE1A00000; // nop
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

	// Decathlon 2012 (USA)
	// Audio does not play
	else if (strcmp(romTid, "KUIE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201CA88 = 0xE1A00000; // nop
		*(u32*)0x02020DB8 = 0xE1A00000; // nop
		patchInitDSiWare(0x020263EC, heapEnd);
		patchUserSettingsReadDSiWare(0x020279D0);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0204FCA4, 0x02027F14);
			*(u32*)0x0204FCF0 = 0xE1A00000; // nop
		}
		setBL(0x02057B48, (u32)dsiSaveCreate);
		setBL(0x02057B58, (u32)dsiSaveOpen);
		setBL(0x02057B74, (u32)dsiSaveGetResultCode);
		setBL(0x02057B98, (u32)dsiSaveSeek);
		setBL(0x02057BB0, (u32)dsiSaveGetResultCode);
		setBL(0x02057BD4, (u32)dsiSaveWrite);
		setBL(0x02057BF4, (u32)dsiSaveClose);
		setBL(0x02057BFC, (u32)dsiSaveGetResultCode);
		setBL(0x02057C18, (u32)dsiSaveGetResultCode);
		setBL(0x02057C54, (u32)dsiSaveOpenR);
		setBL(0x02057C64, (u32)dsiSaveGetLength);
		setBL(0x02057C98, (u32)dsiSaveRead);
		setBL(0x02057CB0, (u32)dsiSaveClose);
		setBL(0x02057CBC, (u32)dsiSaveGetResultCode);
		setBL(0x0205C47C, 0x02057D18);
		*(u32*)0x020626F8 = 0xE12FFF1E; // bx lr
	}

	// Decathlon 2012 (Europe)
	// Audio does not play
	else if (strcmp(romTid, "KUIP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201242C = 0xE1A00000; // nop
		*(u32*)0x0201675C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BD90, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D374);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02045648, 0x0201D8B8);
			*(u32*)0x02045694 = 0xE1A00000; // nop
		}
		setBL(0x0204D4EC, (u32)dsiSaveCreate);
		setBL(0x0204D4FC, (u32)dsiSaveOpen);
		setBL(0x0204D518, (u32)dsiSaveGetResultCode);
		setBL(0x0204D53C, (u32)dsiSaveSeek);
		setBL(0x0204D554, (u32)dsiSaveGetResultCode);
		setBL(0x0204D578, (u32)dsiSaveWrite);
		setBL(0x0204D598, (u32)dsiSaveClose);
		setBL(0x0204D5A0, (u32)dsiSaveGetResultCode);
		setBL(0x0204D5BC, (u32)dsiSaveGetResultCode);
		setBL(0x0204D5F8, (u32)dsiSaveOpenR);
		setBL(0x0204D608, (u32)dsiSaveGetLength);
		setBL(0x0204D63C, (u32)dsiSaveRead);
		setBL(0x0204D654, (u32)dsiSaveClose);
		setBL(0x0204D660, (u32)dsiSaveGetResultCode);
		setBL(0x02051E20, 0x0204D6BC);
		*(u32*)0x0205809C = 0xE12FFF1E; // bx lr
	}

	// Deep Sea Creatures (USA)
	else if (strcmp(romTid, "K6BE") == 0) {
		*(u32*)0x02013174 = 0xE1A00000; // nop
		*(u32*)0x0201652C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D050, heapEnd);
		*(u32*)0x0201D3DC = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201E7B0);
		*(u32*)0x02023988 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202398C = 0xE12FFF1E; // bx lr
		*(u32*)0x0204D388 = 0xE1A023A9; // mov r2, r9, lsr #7
		*(u32*)0x0204D398 = 0xE3A01601; // mov r1, #0x100000
		*(u32*)0x0204D3A8 = 0xE1A00000; // nop
		setBL(0x020514B8, (u32)dsiSaveOpen);
		*(u32*)0x020514CC = 0xE1A00000; // nop
		setBL(0x020514D8, (u32)dsiSaveCreate);
		setBL(0x020514E8, (u32)dsiSaveOpen);
		setBL(0x0205150C, (u32)dsiSaveSeek);
		setBL(0x020516DC, (u32)dsiSaveSeek);
		setBL(0x020516F0, (u32)dsiSaveRead);
		setBL(0x020517BC, (u32)dsiSaveSeek);
		setBL(0x020517CC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x020517DC, (u32)dsiSaveSeek);
		setBL(0x020517F8, (u32)dsiSaveSeek);
		setBL(0x02051808, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02051818, (u32)dsiSaveSeek);
		setBL(0x020518C0, (u32)dsiSaveSeek);
		setBL(0x020518D0, (u32)dsiSaveRead);
		setBL(0x02051904, (u32)dsiSaveSeek);
		setBL(0x02051914, (u32)dsiSaveWrite);
		*(u32*)0x02072778 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02072780 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// GO Series: Defense Wars (USA)
	// GO Series: Defence Wars (Europe)
	// Uchi Makure!: Touch Pen Wars (Japan)
	else if (strncmp(romTid, "KWT", 3) == 0) {
		*(u32*)0x0200722C = 0xE1A00000; // nop
		*(u32*)0x0200B350 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		*(u32*)0x0200D344 = 0xE1A00000; // nop
		if (romTid[3] != 'J') {
			*(u32*)0x02049F68 = 0xE1A00000; // nop
			tonccpy((u32*)0x0204AAEC, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0204DC94 = 0xE1A00000; // nop
			patchInitDSiWare(0x02055548, heapEnd);
			patchUserSettingsReadDSiWare(0x02056A24);
		} else {
			*(u32*)0x02049D70 = 0xE1A00000; // nop
			tonccpy((u32*)0x0204A8E8, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0204DA90 = 0xE1A00000; // nop
			patchInitDSiWare(0x02055328, heapEnd);
			patchUserSettingsReadDSiWare(0x020567F4);
		}

		// Manual screen
		/* *(u32*)0x0200CB0C = 0xE1A00000; // nop
		*(u32*)0x0200CB10 = 0xE1A00000; // nop
		*(u32*)0x0200CB34 = 0xE1A00000; // nop
		*(u32*)0x0200CB50 = 0xE1A00000; // nop
		*(u32*)0x0200CB80 = 0xE1A00000; // nop */

		// Skip
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200CC98;
			offset[i] = 0xE1A00000; // nop
		}

		/*for (int i = 0; i < 10; i++) {
			u32* offset = (u32*)0x0203B3A0;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x0203CAD4 = 0xEB006798; // bl 0x205693C
		for (int i = 0; i < 6; i++) {
			u32* offset = (u32*)0x0203CD60;
			offset[i] = 0xE1A00000; // nop
		}*/
	}

	// Dekisugi Tingle Pack (Japan)
	else if (strcmp(romTid, "KCPJ") == 0) {
		*(u32*)0x0200506C = 0xE1A00000; // nop
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		*(u32*)0x020052A8 = 0xE1A00000; // nop
		*(u32*)0x020055F8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200DC74 = 0xE1A00000; // nop (Disable photo file loading)
		*(u32*)0x0200ED30 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020226DC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0202271C = 0xE1A00000; // nop
		*(u32*)0x02023564 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02023590 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020235D8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02023684 = 0xE3A00000; // mov r0, #0

		// *(u32*)0x020255F4 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x020255F8 = 0xE12FFF1E; // bx lr
		setBL(0x02025604, (u32)dsiSaveCreate);
		for (int i = 0; i < 10; i++) { // Disable creating "save2.bin"
			u32* offset = (u32*)0x02025620;
			offset[i] = 0xE1A00000; // nop
		}
		setBL(0x020256C4, 0x021AFE1C);
		setBL(0x020256E0, 0x021AFD84);
		setBL(0x0202571C, 0x021AFD84);
		setBL(0x0202572C, 0x021AFEE4);
		setBL(0x02025748, 0x021AFD84);
		setBL(0x02025764, 0x021AFE1C);
		setBL(0x02025780, 0x021AFD84);
		setBL(0x020257B4, 0x021AFD84);
		setBL(0x020257C4, 0x021AFEE4);
		setBL(0x020257E0, 0x021AFD84);
		setBL(0x02025830, 0x021AFD84);
		setBL(0x02025874, 0x021AFE1C);
		setBL(0x02025890, 0x021AFD84);
		setBL(0x020258B4, 0x021AFE8C);
		setBL(0x020258D0, 0x021AFD84);
		setBL(0x020258E0, 0x021AFEE4);
		setBL(0x020258FC, 0x021AFD84);
		setBL(0x02025944, 0x021AFD84);
		setBL(0x0202595C, 0x021AFE8C);
		setBL(0x02025988, 0x021AFEE4);
		setBL(0x020259A4, 0x021AFD84);
		setBL(0x020259C8, 0x021AFD84);
		setBL(0x020259F4, 0x021AFD84);

		codeCopy((u32*)0x021AFF30, (u32*)0x02031F30, 0x2C);
		codeCopy((u32*)0x021AFF78, (u32*)0x02031F78, 0x2C);
		codeCopy((u32*)0x021AFFEC, (u32*)0x02031FEC, 0x24);
		codeCopy((u32*)0x021AFD84, (u32*)0x02037484, 0x48);
		codeCopy((u32*)0x021AFE1C, (u32*)0x0203751C, 0x70);
		codeCopy((u32*)0x021AFE8C, (u32*)0x0203758C, 0x58);
		codeCopy((u32*)0x021AFEE4, (u32*)0x020375E4, 0x20);
		setBL(0x021AFF44, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x021AFF8C, (u32)dsiSaveOpen);
		setBL(0x021AFFF8, (u32)dsiSaveClose);
		setBL(0x021AFDBC, 0x021AFFEC);
		setBL(0x021AFE68, 0x021AFF78);
		setBL(0x021AFEA4, 0x021AFF30);
		setBL(0x021AFEF4, 0x021AFFEC);

		setBL(0x0203764C, (u32)dsiSaveWrite);

		// *(u32*)0x02038818 = 0xE3A00000; // mov r0, #0 (Brings up some kind of debug menu on boot)
		// *(u32*)0x0203881C = 0xE12FFF1E; // bx lr
		*(u32*)0x0202666C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02051748 = 0xE2640601; // rsb r0,r4,0x100000 (Shrink sound heap from 0x180000)
		}
		*(u32*)0x0207357C = 0xE1A00000; // nop
		*(u32*)0x02077788 = 0xE1A00000; // nop
		patchInitDSiWare(0x02081708, heapEnd);
		*(u32*)0x02081A78 = 0x021B0DA0;
		patchUserSettingsReadDSiWare(0x02082D20);
		*(u32*)0x0208317C = 0xE1A00000; // nop
		*(u32*)0x02083180 = 0xE1A00000; // nop
		*(u32*)0x02083184 = 0xE1A00000; // nop
		*(u32*)0x02083188 = 0xE1A00000; // nop
	}

	// Delbo (USA)
	// Difficult to get working for some reason
	/* else if (strcmp(romTid, "KDBE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02005128 = 0xE1A00000; // nop
		*(u32*)0x0200EA14 = 0xE1A00000; // nop
		*(u32*)0x02012804 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201967C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AEBC);
		*(u32*)0x0202D770 = 0x14E000; // Shrink sound heap from 0x2EE000
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0202EB0C, 0x0201B43C);
			}
		} else {
			*(u32*)0x02005124 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		setBL(0x02038668, (u32)dsiSaveOpen);
		setBL(0x0203869C, (u32)dsiSaveClose);
		setBL(0x020386CC, (u32)dsiSaveGetLength);
		setBL(0x020386F4, (u32)dsiSaveRead);
		setBL(0x020386FC, (u32)dsiSaveClose);
		setBL(0x02038730, (u32)dsiSaveClose);
		*(u32*)0x0203877C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02038824 = 0xE1A00000; // nop
		*(u32*)0x02038834 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		*(u32*)0x02038954 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02038A70 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02038C60 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		setBL(0x02038D00, (u32)dsiSaveDelete);
		*(u32*)0x02038D10 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x02038D20 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		setBL(0x02038DB0, (u32)dsiSaveCreate);
		setBL(0x02038DC0, (u32)dsiSaveOpen);
		setBL(0x02038E04, (u32)dsiSaveClose);
		setBL(0x02038E20, (u32)dsiSaveSetLength);
		setBL(0x02038E30, (u32)dsiSaveWrite);
		setBL(0x02038E38, (u32)dsiSaveClose);
	} */

	// Devil Band: Rock the Underworld (USA)
	// Devil Band: Rock the Underworld (Europe, Australia)
	// Devil Band: Rock the Underworld (Japan)
	// Requires 8MB of RAM
	else if (strncmp(romTid, "KN2", 3) == 0 && extendedMemory2) {
		*(u32*)0x02005088 = 0xE1A00000; // nop (Disable reading save data)
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		if (romTid[3] != 'J') {
			*(u32*)0x0200FC0C = 0xE1A00000; // nop
			*(u32*)0x02012E84 = 0xE1A00000; // nop
			patchInitDSiWare(0x02018FA8, heapEnd);
			patchUserSettingsReadDSiWare(0x0201A4B8);
		} else {
			*(u32*)0x0200FC08 = 0xE1A00000; // nop
			*(u32*)0x02012E80 = 0xE1A00000; // nop
			patchInitDSiWare(0x02018FA4, heapEnd);
			patchUserSettingsReadDSiWare(0x0201A4B4);
		}
	}

	// Disney Fireworks (USA)
	// Locks up on ESRB screen
	// Requires 8MB of RAM
	/* else if (strcmp(romTid, "KDSE") == 0 && extendedMemory2) {
		*(u32*)0x0201763C = 0xE1A00000; // nop (Disable .sdat loading)
		*(u32*)0x02024C3C = 0xE1A00000; // nop
		*(u32*)0x02076CF4 = 0xE1A00000; // nop
		*(u32*)0x0207B974 = 0xE1A00000; // nop
		patchInitDSiWare(0x02084034, heapEnd);
		*(u32*)0x020843C0 = 0x020D82C0;
		patchUserSettingsReadDSiWare(0x02085568);
		*(u32*)0x020745D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020745DC = 0xE12FFF1E; // bx lr
	} */

	// Divergent Shift (USA)
	// Divergent Shift (Europe, Australia)
	else if (strcmp(romTid, "KRFE") == 0 || strcmp(romTid, "KRFV") == 0) {
		*(u32*)0x02011694 = 0xE1A00000; // nop
		*(u32*)0x02014AE0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A0BC, heapEnd);
		*(u32*)0x0201A448 -= 0x39000;
		patchUserSettingsReadDSiWare(0x0201B6D4);
		*(u32*)0x020391F0 = 0xE2406901; // sub r6, r0, #0x4000
		*(u32*)0x0203BC18 = 0xE1A00000; // nop
		*(u32*)0x02049808 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204B3E4 = 0xE3A00701; // mov r0, #0x40000 (Shrink sdat-specific sound heap from 0x200000)
		setBL(0x0204B7CC, (u32)dsiSaveOpen);
		*(u32*)0x0204B81C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0204B844 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0204B864, (u32)dsiSaveCreate);
		setBL(0x0204B88C, (u32)dsiSaveOpen);
		setBL(0x0204B8B8, (u32)dsiSaveWrite);
		*(u32*)0x0204B8C4 = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0204B8CC, (u32)dsiSaveClose);
		setBL(0x0204B914, (u32)dsiSaveOpen);
		setBL(0x0204B970, (u32)dsiSaveRead);
		setBL(0x0204B97C, (u32)dsiSaveClose);
	}

	// Don't Cross the Line (USA)
	/* else if (strcmp(romTid, "KMLE") == 0) {
		*(u32*)0x0200B438 = 0xE1A00000; // nop
		*(u32*)0x0200F5B4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02015AF8, heapEnd);
		// *(u32*)0x02015E84 -= 0x39000;
		patchUserSettingsReadDSiWare(0x020171D0);
		*(u32*)0x0201A934 = 0xE1A00000; // nop
		// *(u32*)0x0201C5BC = extendedMemory2 ? 0x500000 : 0x2C0000;
	} */

	// DotMan (USA)
	else if (strcmp(romTid, "KHEE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020226E4, 0x020186A0);
			}
		} else {
			*(u32*)0x02005358 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x02022600;
				offset[i] = 0xE1A00000; // nop
			}
		}
		*(u32*)0x0200E038 = 0xE1A00000; // nop
		*(u32*)0x0201158C = 0xE1A00000; // nop
		patchInitDSiWare(0x02016C80, heapEnd);
		patchUserSettingsReadDSiWare(0x02018120);
		*(u32*)0x0201D1A0 = 0xE1A00000; // nop
	}

	// DotMan (Europe)
	else if (strcmp(romTid, "KHEP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020227CC, 0x02018780);
			}
		} else {
			*(u32*)0x02005370 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020226DC;
				offset[i] = 0xE1A00000; // nop
			}
		}
		*(u32*)0x0200E074 = 0xE1A00000; // nop
		*(u32*)0x02011650 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016D60, heapEnd);
		patchUserSettingsReadDSiWare(0x02018200);
		*(u32*)0x0201D280 = 0xE1A00000; // nop
	}

	// DotMan (Japan)
	else if (strcmp(romTid, "KHEJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02022570, 0x0201867C);
			}
		} else {
			*(u32*)0x02005358 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x0202248C;
				offset[i] = 0xE1A00000; // nop
			}
		}
		*(u32*)0x0200E014 = 0xE1A00000; // nop
		*(u32*)0x02011568 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016C5C, heapEnd);
		patchUserSettingsReadDSiWare(0x020180FC);
		*(u32*)0x0201D080 = 0xE1A00000; // nop
	}

	// Dr. Mario Express (USA)
	// A Little Bit of... Dr. Mario (Europe, Australia)
	else if (strcmp(romTid, "KD9E") == 0 || strcmp(romTid, "KD9V") == 0) {
		// useSharedFont = twlFontFound;
		*(u32*)0x020103C4 = 0xE3A00000; // mov r0, #0
		tonccpy((u32*)0x02011160, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02013A08 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02019DF4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201B724 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201B728 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201BC0C = 0xE1A00000; // nop
		*(u32*)0x0201BC10 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BC28 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BD70 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BD88 = 0xE1A00000; // nop (Leave MPU region 1 untouched)
		*(u32*)0x0201BE0C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BE3C = 0xE3A00000; // mov r0, #0
		// if (!useSharedFont) {
			*(u32*)0x0201BF10 = 0xE3A00000; // mov r0, #0
		// }
		*(u32*)0x0201BF40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201CF08 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D2A8 = 0xE3A00000; // mov r0, #0
		// if (!useSharedFont) {
			*(u32*)0x020248C4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02025CD4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		// }
		*(u32*)0x0203D228 = 0xE3A00000; // mov r0, #0 (Skip saving to "back.dat")
		// *(u32*)0x0203D488 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0203D48C = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
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
			*(u32*)0x0206F430 = 0xE3A00000; // mov r0, #0
			/* if (useSharedFont) {
				if (!extendedMemory2 && expansionPakFound) {
					patchTwlFontLoad(0x020741C8, 0x02018590);
				}
			} else { */
				*(u32*)0x0207347C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x020736DC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x0207401C = 0xE3A00000; // mov r0, #0
				*(u32*)0x02074054 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
			// }
		} else {
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
			*(u32*)0x0206F320 = 0xE3A00000; // mov r0, #0
			// if (!useSharedFont) {
				*(u32*)0x0207336C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x020735CC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
				*(u32*)0x02073F0C = 0xE3A00000; // mov r0, #0
				*(u32*)0x02073F44 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
			// }
		}
	}

	// Chotto Dr. Mario (Japan)
	else if (strcmp(romTid, "KD9J") == 0) {
		// useSharedFont = twlFontFound;
		*(u32*)0x020052B0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02010B08 = 0xE3A00000; // mov r0, #0
		tonccpy((u32*)0x020118A4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02013E58 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201A244 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201BB74 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201BB78 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201C05C = 0xE1A00000; // nop
		*(u32*)0x0201C060 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C078 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C1C0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C1D8 = 0xE1A00000; // nop (Leave MPU region 1 untouched)
		*(u32*)0x0201C25C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C28C = 0xE3A00000; // mov r0, #0
		// if (!useSharedFont) {
			*(u32*)0x0201C360 = 0xE3A00000; // mov r0, #0
		// }
		*(u32*)0x0201C390 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D358 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201D6F8 = 0xE3A00000; // mov r0, #0
		// if (!useSharedFont) {
			*(u32*)0x02024CF4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02026104 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202D3B4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202D644 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0202DFB8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202DFF0 = 0xE1A00000; // nop (Skip NFTR file loading from TWLNAND)
		// }
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

	// Dragon's Lair (USA)
	else if (strcmp(romTid, "KDLE") == 0) {
		// *(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop (Skip Manual screen)
		// *(u32*)0x02012064 = 0xE1A00000; // nop
		// *(u32*)0x02012068 = 0xE1A00000; // nop
		/*for (int i = 0; i < 5; i++) {
			u32* offset1 = (u32*)0x020132C0;
			u32* offset2 = (u32*)0x020135FC;
			u32* offset3 = (u32*)0x02013A44;
			u32* offset4 = (u32*)0x02014DA8;
			u32* offset5 = (u32*)0x02016134;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
			offset3[i] = 0xE1A00000; // nop
			offset4[i] = 0xE1A00000; // nop
			offset5[i] = 0xE1A00000; // nop
		}*/
		*(u32*)0x0201B894 = 0xE1A00000; // nop
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
		*(u32*)0x0201BC18 = 0xE1A00000; // nop
		*(u32*)0x0202FACC = 0xE1A00000; // nop
		*(u32*)0x0202FC00 = 0xE1A00000; // nop
		*(u32*)0x0202FC14 = 0xE1A00000; // nop
		*(u32*)0x02033044 = 0xE1A00000; // nop
		patchInitDSiWare(0x020387DC, heapEnd);
		*(u32*)0x02038B4C = 0x02084600;
		patchUserSettingsReadDSiWare(0x0203A0D0);
		*(u32*)0x0203A53C = 0xE1A00000; // nop
		*(u32*)0x0203A540 = 0xE1A00000; // nop
		*(u32*)0x0203A544 = 0xE1A00000; // nop
		*(u32*)0x0203A548 = 0xE1A00000; // nop
	}

	// Dragon's Lair (Europe, Australia)
	else if (strcmp(romTid, "KDLV") == 0) {
		// *(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020051E4 = 0xE1A00000; // nop (Skip Manual screen)
		// *(u32*)0x0201205C = 0xE1A00000; // nop
		// *(u32*)0x02012060 = 0xE1A00000; // nop
		/*for (int i = 0; i < 5; i++) {
			u32* offset1 = (u32*)0x020132B4;
			u32* offset2 = (u32*)0x020135F0;
			u32* offset3 = (u32*)0x02013A38;
			u32* offset4 = (u32*)0x02014D9C;
			u32* offset5 = (u32*)0x02016128;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
			offset3[i] = 0xE1A00000; // nop
			offset4[i] = 0xE1A00000; // nop
			offset5[i] = 0xE1A00000; // nop
		}*/
		*(u32*)0x0201B888 = 0xE1A00000; // nop
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
		*(u32*)0x0201BC0C = 0xE1A00000; // nop
		*(u32*)0x0202FAC0 = 0xE1A00000; // nop
		*(u32*)0x0202FBF4 = 0xE1A00000; // nop
		*(u32*)0x0202FC08 = 0xE1A00000; // nop
		*(u32*)0x02033038 = 0xE1A00000; // nop
		patchInitDSiWare(0x020387D0, heapEnd);
		*(u32*)0x02038B40 = 0x02084600;
		patchUserSettingsReadDSiWare(0x0203A0C4);
		*(u32*)0x0203A530 = 0xE1A00000; // nop
		*(u32*)0x0203A534 = 0xE1A00000; // nop
		*(u32*)0x0203A538 = 0xE1A00000; // nop
		*(u32*)0x0203A53C = 0xE1A00000; // nop
	}

	// Dragon's Lair II: Time Warp (USA)
	else if (strcmp(romTid, "KLYE") == 0) {
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		// *(u32*)0x020171CC = 0xE1A00000; // nop
		// *(u32*)0x020171D0 = 0xE1A00000; // nop
		*(u32*)0x0201FFEC = 0xE1A00000; // nop
		setBL(0x02020034, (u32)dsiSaveOpen);
		setBL(0x0202004C, (u32)dsiSaveRead);
		setBL(0x02020074, (u32)dsiSaveClose);
		*(u32*)0x020200D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020104 = 0xE1A00000; // nop
		setBL(0x02020110, (u32)dsiSaveCreate);
		setBL(0x02020140, (u32)dsiSaveOpen);
		setBL(0x02020170, (u32)dsiSaveWrite);
		setBL(0x02020198, (u32)dsiSaveClose);
		*(u32*)0x020201CC = 0xE1A00000; // nop
		*(u32*)0x020201D8 = 0xE1A00000; // nop
		*(u32*)0x02020238 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020264 = 0xE1A00000; // nop
		setBL(0x02020274, (u32)dsiSaveOpen);
		setBL(0x020202B0, (u32)dsiSaveSeek);
		setBL(0x020202E0, (u32)dsiSaveWrite);
		setBL(0x02020308, (u32)dsiSaveClose);
		*(u32*)0x0202033C = 0xE1A00000; // nop
		*(u32*)0x02020348 = 0xE1A00000; // nop
		setBL(0x02020374, (u32)dsiSaveGetResultCode);
		setBL(0x020203A4, (u32)dsiSaveClose);
		setBL(0x020203BC, (u32)dsiSaveClose);
		*(u32*)0x020203C8 = 0xE1A00000; // nop
		*(u32*)0x02033EE0 = 0xE1A00000; // nop
		*(u32*)0x02037300 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203C834, heapEnd);
		*(u32*)0x0203CBC0 = 0x02089260;
		patchUserSettingsReadDSiWare(0x0203E13C);
	}

	// Dragon's Lair II: Time Warp (Europe, Australia)
	// Crashes on company logos (Loads large garbage data?)
	else if (strcmp(romTid, "KLYV") == 0) {
		*(u32*)0x020050EC = 0xE1A00000; // nop
		*(u32*)0x020051E0 = 0xE1A00000; // nop (Skip Manual screen)
		// *(u32*)0x020171E8 = 0xE1A00000; // nop
		// *(u32*)0x020171EC = 0xE1A00000; // nop
		*(u32*)0x02020004 = 0xE1A00000; // nop
		setBL(0x0202004C, (u32)dsiSaveOpen);
		setBL(0x02020064, (u32)dsiSaveRead);
		setBL(0x0202008C, (u32)dsiSaveClose);
		*(u32*)0x020200F0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202011C = 0xE1A00000; // nop
		setBL(0x02020128, (u32)dsiSaveCreate);
		setBL(0x02020158, (u32)dsiSaveOpen);
		setBL(0x02020188, (u32)dsiSaveWrite);
		setBL(0x020201B0, (u32)dsiSaveClose);
		*(u32*)0x020201E4 = 0xE1A00000; // nop
		*(u32*)0x020201F0 = 0xE1A00000; // nop
		*(u32*)0x02020250 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202027C = 0xE1A00000; // nop
		setBL(0x0202028C, (u32)dsiSaveOpen);
		setBL(0x020202C8, (u32)dsiSaveSeek);
		setBL(0x020202F8, (u32)dsiSaveWrite);
		setBL(0x02020320, (u32)dsiSaveClose);
		*(u32*)0x02020354 = 0xE1A00000; // nop
		*(u32*)0x02020360 = 0xE1A00000; // nop
		setBL(0x0202038C, (u32)dsiSaveGetResultCode);
		setBL(0x020203BC, (u32)dsiSaveClose);
		setBL(0x020203D4, (u32)dsiSaveClose);
		*(u32*)0x020203DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020410 = 0xE1A00000; // nop
		*(u32*)0x02020418 = 0xE1A00000; // nop
		setBL(0x02020424, (u32)dsiSaveCreate);
		*(u32*)0x02020438 = 0xE1A00000; // nop
		*(u32*)0x02020444 = 0xE1A00000; // nop
		*(u32*)0x02033F64 = 0xE1A00000; // nop
		*(u32*)0x02036AE8 = 0xE1A00000; // nop
		*(u32*)0x0203740C = 0xE1A00000; // nop
		patchInitDSiWare(0x0203C95C, heapEnd);
		*(u32*)0x0203CCE8 = 0x020894E0;
		patchUserSettingsReadDSiWare(0x0203E264);
	}

	// Dragon Quest Wars (USA)
	// DSi save function patching not needed
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDQE") == 0 && extendedMemory2) {
		*(u32*)0x0201F208 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020A82EC = 0xE1A00000; // nop
		*(u32*)0x020AC120 = 0xE1A00000; // nop
		patchInitDSiWare(0x020B6640, heapEnd);
		// *(u32*)0x020B69B0 = 0x022A83C0;
		patchUserSettingsReadDSiWare(0x020B7BF0);
		*(u32*)0x020B7C18 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7C1C = 0xE12FFF1E; // bx lr
		*(u32*)0x020B7C6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B7C70 = 0xE12FFF1E; // bx lr
	}

	// Dragon Quest Wars (Europe, Australia)
	// DSi save function patching not needed
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDQV") == 0 && extendedMemory2) {
		*(u32*)0x0201F250 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020A8334 = 0xE1A00000; // nop
		*(u32*)0x020AC168 = 0xE1A00000; // nop
		patchInitDSiWare(0x020B6688, heapEnd);
		patchUserSettingsReadDSiWare(0x020B7C38);
		*(u32*)0x020B7C60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B7C64 = 0xE12FFF1E; // bx lr
		*(u32*)0x020B7CB4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B7CB8 = 0xE12FFF1E; // bx lr
	}

	// Dragon Quest Wars (Japan)
	// DSi save function patching not needed
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDQJ") == 0 && extendedMemory2) {
		*(u32*)0x0201EF84 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020A7CAC = 0xE1A00000; // nop
		*(u32*)0x020ABAE0 = 0xE1A00000; // nop
		patchInitDSiWare(0x020B6000, heapEnd);
		patchUserSettingsReadDSiWare(0x020B75B0);
		*(u32*)0x020B75D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B75DC = 0xE12FFF1E; // bx lr
		*(u32*)0x020B762C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B7630 = 0xE12FFF1E; // bx lr
	}

	// Dreamwalker (USA)
	else if (strcmp(romTid, "K9EE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02029610 = 0xE1A00000; // nop
		setBL(0x0202963C, (u32)dsiSaveOpen);
		setBL(0x02029654, (u32)dsiSaveRead);
		setBL(0x0202967C, (u32)dsiSaveClose);
		*(u32*)0x020296E0 = 0xE1A00000; // nop
		*(u32*)0x020296E8 = 0xE1A00000; // nop
		setBL(0x020296F4, (u32)dsiSaveCreate);
		setBL(0x02029724, (u32)dsiSaveOpen);
		setBL(0x02029754, (u32)dsiSaveWrite);
		setBL(0x0202977C, (u32)dsiSaveClose);
		*(u32*)0x020297B0 = 0xE1A00000; // nop
		*(u32*)0x020297B8 = 0xE1A00000; // nop
		*(u32*)0x0202981C = 0xE1A00000; // nop
		*(u32*)0x0202982C = 0xE1A00000; // nop
		setBL(0x0202983C, (u32)dsiSaveOpen);
		setBL(0x02029878, (u32)dsiSaveSeek);
		setBL(0x020298A8, (u32)dsiSaveWrite);
		setBL(0x020298D0, (u32)dsiSaveClose);
		*(u32*)0x02029904 = 0xE1A00000; // nop
		*(u32*)0x0202990C = 0xE1A00000; // nop
		setBL(0x02029938, (u32)dsiSaveGetResultCode);
		setBL(0x0202996C, (u32)dsiSaveClose);
		setBL(0x02029984, (u32)dsiSaveClose);
		*(u32*)0x02029990 = 0xE1A00000; // nop
		setBL(0x020299DC, (u32)dsiSaveSeek);
		setBL(0x020299F0, (u32)dsiSaveWrite);
		*(u32*)0x0207DB84 = 0xE1A00000; // nop
		*(u32*)0x02080F88 = 0xE1A00000; // nop
		patchInitDSiWare(0x02087404, heapEnd);
		patchUserSettingsReadDSiWare(0x02088988);
	}

	// DS WiFi Settings
	else if (strcmp(romTid, "B88A") == 0) {
		const u16* branchCode = generateA7InstrThumb(0x020051F4, (int)ce9->thumbPatches->reset_arm9);

		*(u16*)0x020051F4 = branchCode[0];
		*(u16*)0x020051F6 = branchCode[1];
		*(u32*)0x02005224 = 0xFFFFFFFF;
		*(u16*)0x0202FAEE = nopT;
		*(u16*)0x0202FAF0 = nopT;
		*(u16*)0x02031B3E = nopT;
		*(u16*)0x02031B40 = nopT;
		*(u16*)0x020344A0 = nopT;
		*(u16*)0x020344A2 = nopT;
		*(u16*)0x02036532 = nopT;
		*(u16*)0x02036534 = nopT;
		*(u16*)0x02036536 = nopT;
		*(u16*)0x02036538 = nopT;
		*(u16*)0x02036542 = nopT;
		*(u16*)0x02036544 = nopT;
		*(u16*)0x02036626 = nopT;
		*(u16*)0x02036628 = nopT;
		*(u16*)0x020374D8 = 0x2001; // movs r0, #1
		*(u16*)0x020374DA = 0x4770; // bx lr
		*(u16*)0x02037510 = 0x2000; // movs r0, #0
		*(u16*)0x02037512 = 0x4770; // bx lr
		*(u16*)0x0203B490 = 0x2003; // movs r0, #3
		*(u16*)0x0203B492 = 0x4770; // bx lr
	}

	// GO Series: Earth Saver (USA)
	else if (strcmp(romTid, "KB8E") == 0) {
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005530 = 0xE1A00000; // nop
		//if (!twlFontFound) {
			*(u32*)0x0200A3D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200B800 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x02014AB0 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
			*(u32*)0x02047E4C = 0xE12FFF1E; // bx lr

			// Skip Manual screen, Part 2
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x02014BEC;
				offset[i] = 0xE1A00000; // nop
			}
		//}
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
		*(u32*)0x02036398 = 0xE1A00000; // nop
		*(u32*)0x0204BFE8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0204CB6C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204F548 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205619C, heapEnd);
		patchUserSettingsReadDSiWare(0x02057678);
		*(u32*)0x02057AAC = 0xE1A00000; // nop
		*(u32*)0x02057AB0 = 0xE1A00000; // nop
		*(u32*)0x02057AB4 = 0xE1A00000; // nop
		*(u32*)0x02057AB8 = 0xE1A00000; // nop
		/* *(u16*)0x0205C54C = 0x2001; // movs r0, #1
		*(u16*)0x0205C54E = 0x4770; // bx lr
		*(u16*)0x0205C56C = 0x2001; // movs r0, #1
		*(u16*)0x0205C56E = 0x4770; // bx lr
		*(u16*)0x0205C5E4 = 0x2001; // movs r0, #1
		*(u16*)0x0205C5E8 = 0x4770; // bx lr
		*(u16*)0x0205C604 = 0x2001; // movs r0, #1
		*(u16*)0x0205C608 = 0x4770; // bx lr */
	}

	// GO Series: Earth Saver (Europe)
	else if (strcmp(romTid, "KB8P") == 0) {
		*(u32*)0x02005234 = 0xE1A00000; // nop
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
		*(u32*)0x02036394 = 0xE1A00000; // nop
		*(u32*)0x02047D50 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204BEEC = 0xE1A00000; // nop
		tonccpy((u32*)0x0204CA70, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204F44C = 0xE1A00000; // nop
		patchInitDSiWare(0x020560A0, heapEnd);
		patchUserSettingsReadDSiWare(0x0205757C);
		*(u32*)0x020579B0 = 0xE1A00000; // nop
		*(u32*)0x020579B4 = 0xE1A00000; // nop
		*(u32*)0x020579B8 = 0xE1A00000; // nop
		*(u32*)0x020579BC = 0xE1A00000; // nop

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014AF0;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Earth Saver: Inseki Bakuha Dai Sakuse (Japan)
	else if (strcmp(romTid, "KB9J") == 0) {
		*(u32*)0x0200521C = 0xE1A00000; // nop
		setBL(0x02009CF4, (u32)dsiSaveOpen);
		setBL(0x02009D2C, (u32)dsiSaveRead);
		setBL(0x02009D4C, (u32)dsiSaveClose);
		setBL(0x02009DE4, (u32)dsiSaveCreate);
		setBL(0x02009E24, (u32)dsiSaveOpen);
		setBL(0x02009E5C, (u32)dsiSaveSetLength);
		setBL(0x02009E74, (u32)dsiSaveWrite);
		setBL(0x02009E98, (u32)dsiSaveClose);
		setBL(0x02009F28, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B9C4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200A7CC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0200FF44 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		*(u32*)0x0203092C = 0xE12FFF1E; // bx lr
		*(u32*)0x02034AD4 = 0xE1A00000; // nop
		tonccpy((u32*)0x0203564C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02037FA0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203EBBC, heapEnd);
		patchUserSettingsReadDSiWare(0x02040088);
		*(u32*)0x020404BC = 0xE1A00000; // nop
		*(u32*)0x020404C0 = 0xE1A00000; // nop
		*(u32*)0x020404C4 = 0xE1A00000; // nop
		*(u32*)0x020404C8 = 0xE1A00000; // nop

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02010080;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Earth Saver Plus: Inseki Bakuha Dai Sakuse (Japan)
	else if (strcmp(romTid, "KB8J") == 0) {
		*(u32*)0x0200521C = 0xE1A00000; // nop
		*(u32*)0x0200A038 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0200A84C, (u32)dsiSaveOpen);
		setBL(0x0200A888, (u32)dsiSaveRead);
		setBL(0x0200A8A8, (u32)dsiSaveClose);
		setBL(0x0200A944, (u32)dsiSaveCreate);
		setBL(0x0200A984, (u32)dsiSaveOpen);
		setBL(0x0200A9BC, (u32)dsiSaveSetLength);
		setBL(0x0200A9D8, (u32)dsiSaveWrite);
		setBL(0x0200A9FC, (u32)dsiSaveClose);
		setBL(0x0200AA90, (u32)dsiSaveGetInfo);
		*(u32*)0x0200B438 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02014708 = 0xE12FFF1E; // bx lr (Skip Manual screen, Part 1)
		*(u32*)0x020360E8 = 0xE1A00000; // nop
		*(u32*)0x02047A88 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204BC24 = 0xE1A00000; // nop
		tonccpy((u32*)0x0204C79C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204F0F0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02055D0C, heapEnd);
		patchUserSettingsReadDSiWare(0x020571D8);
		*(u32*)0x0205760C = 0xE1A00000; // nop
		*(u32*)0x02057610 = 0xE1A00000; // nop
		*(u32*)0x02057614 = 0xE1A00000; // nop
		*(u32*)0x02057618 = 0xE1A00000; // nop

		// Skip Manual screen, Part 2
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x02014844;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Easter Eggztravaganza (USA)
	// Easter Eggztravaganza (Europe)
	else if (strcmp(romTid, "K2EE") == 0 || strcmp(romTid, "K2EP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x18;

		*(u32*)0x0200E0FC = 0xE1A00000; // nop
		*(u32*)0x020115C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016384, heapEnd);
		*(u32*)0x02016710 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02017834);
		*(u32*)0x0203490C = 0xE3A05925; // mov r5, #0x94000
		*(u32*)0x02035B3C = 0xE1A00000; // nop
		*(u32*)0x02035B50 = 0xE1A00000; // nop
		setBL(0x020374A0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020375B4+offsetChange, (u32)dsiSaveRead);
		setBL(0x020375BC+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020375F4+offsetChange) = 0xE1A00000; // nop
		setBL(0x020377B0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020377D8+offsetChange, (u32)dsiSaveCreate);
		setBL(0x020377E8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02037804+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x0203783C+offsetChange, (u32)dsiSaveSeek);
		setBL(0x0203784C+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02037854+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020378C8+offsetChange) = 0xE1A00000; // nop
	}

	// EJ Puzzles: Hooked (USA)
	// Music does not play
	else if (strcmp(romTid, "KHWE") == 0) {
		*(u32*)0x020243F8 = 0xE1A00000; // nop
		setBL(0x0202442C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		*(u32*)0x02024488 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x020244A0 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x020244C8, (u32)dsiSaveOpen);
		setBL(0x02024524, (u32)dsiSaveSetLength);
		setBL(0x02024564, (u32)dsiSaveClose);
		setBL(0x02024584, (u32)dsiSaveSeek);
		setBL(0x02024598, (u32)dsiSaveWrite);
		setBL(0x020245DC, (u32)dsiSaveClose);
		setBL(0x020245F4, (u32)dsiSaveClose);
		setBL(0x020246E8, (u32)dsiSaveOpen);
		setBL(0x02024778, (u32)dsiSaveSeek);
		setBL(0x0202478C, (u32)dsiSaveWrite);
		setBL(0x020247C8, (u32)dsiSaveClose);
		setBL(0x020247E0, (u32)dsiSaveClose);
		setBL(0x020248B8, (u32)dsiSaveOpen);
		setBL(0x0202492C, (u32)dsiSaveSeek);
		setBL(0x0202493C, (u32)dsiSaveRead);
		setBL(0x02024978, (u32)dsiSaveClose);
		setBL(0x020249A4, (u32)dsiSaveClose);
		*(u32*)0x02020FBC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02024858 = 0xE1A00000; // nop
		*(u32*)0x02024864 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0202487C = 0xE1A00000; // nop
		*(u32*)0x02024A60 = 0xE1A00000; // nop
		*(u32*)0x02024A6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02024A84 = 0xE1A00000; // nop
		*(u32*)0x020259EC = 0xE3A02602; // mov r2, #0x200000
		*(u32*)0x02025A4C = 0xE3A02702; // mov r2, #0x80000
		*(u32*)0x02026264 = 0xE1A00000; // nop (Disable loading "Sounds/Menu.pcm")
		*(u32*)0x0202627C = 0xE1A00000; // nop (Disable loading "Sounds/Game.pcm")
		*(u32*)0x020262E0 = 0xE1A00000; // nop
		*(u32*)0x02027750 = 0xE1A00000; // nop
		*(u32*)0x0202B290 = 0xE1A00000; // nop
		patchInitDSiWare(0x02031CB0, heapEnd);
		*(u32*)0x0203203C = *(u32*)0x02004FD0;
	}

	// Electroplankton: Beatnes (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEIE") == 0 && extendedMemory2) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x020124F4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012500 = 0xE1A00000; // nop
		*(u32*)0x0203A1F4 = 0xE1A00000; // nop
		*(u32*)0x0203D59C = 0xE1A00000; // nop
		patchInitDSiWare(0x02043F50, heapEnd);
		patchUserSettingsReadDSiWare(0x02045388);
		*(u32*)0x020457BC = 0xE1A00000; // nop
		*(u32*)0x020457C0 = 0xE1A00000; // nop
		*(u32*)0x020457C4 = 0xE1A00000; // nop
		*(u32*)0x020457C8 = 0xE1A00000; // nop
	}

	// Electroplankton: Beatnes (Europe, Australia)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEIV") == 0 && extendedMemory2) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011FD8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011FE4 = 0xE1A00000; // nop
		*(u32*)0x02039CF4 = 0xE1A00000; // nop
		*(u32*)0x0203D09C = 0xE1A00000; // nop
		patchInitDSiWare(0x02043A50, heapEnd);
		patchUserSettingsReadDSiWare(0x02044E88);
		*(u32*)0x020452BC = 0xE1A00000; // nop
		*(u32*)0x020452C0 = 0xE1A00000; // nop
		*(u32*)0x020452C4 = 0xE1A00000; // nop
		*(u32*)0x020452C8 = 0xE1A00000; // nop
	}

	// Electroplankton: Beatnes (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEIJ") == 0 && extendedMemory2) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02011CE8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011CF4 = 0xE1A00000; // nop
		*(u32*)0x02039D34 = 0xE1A00000; // nop
		*(u32*)0x0203D09C = 0xE1A00000; // nop
		patchInitDSiWare(0x02043DD8, heapEnd);
		patchUserSettingsReadDSiWare(0x0204521C);
		*(u32*)0x02045678 = 0xE1A00000; // nop
		*(u32*)0x0204567C = 0xE1A00000; // nop
		*(u32*)0x02045680 = 0xE1A00000; // nop
		*(u32*)0x02045684 = 0xE1A00000; // nop
	}

	// Electroplankton: Hanenbow (USA)
	else if (strcmp(romTid, "KEBE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02012210 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201221C = 0xE1A00000; // nop
		*(u32*)0x0203CD94 = 0xE1A00000; // nop
		*(u32*)0x0204013C = 0xE1A00000; // nop
		patchInitDSiWare(0x02046CB0, heapEnd);
		*(u32*)0x0204703C = 0x020AD860;
		patchUserSettingsReadDSiWare(0x020480E8);
		*(u32*)0x0204851C = 0xE1A00000; // nop
		*(u32*)0x02048520 = 0xE1A00000; // nop
		*(u32*)0x02048524 = 0xE1A00000; // nop
		*(u32*)0x02048528 = 0xE1A00000; // nop
	}

	// Electroplankton: Hanenbow (Europe, Australia)
	else if (strcmp(romTid, "KEBV") == 0) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011CF4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011D00 = 0xE1A00000; // nop
		*(u32*)0x0203C894 = 0xE1A00000; // nop
		*(u32*)0x0203FC3C = 0xE1A00000; // nop
		patchInitDSiWare(0x020467B0, heapEnd);
		*(u32*)0x02046B3C = 0x020AD1A0;
		patchUserSettingsReadDSiWare(0x02047BE8);
		*(u32*)0x0204801C = 0xE1A00000; // nop
		*(u32*)0x02048020 = 0xE1A00000; // nop
		*(u32*)0x02048024 = 0xE1A00000; // nop
		*(u32*)0x02048028 = 0xE1A00000; // nop
	}

	// Electroplankton: Hanenbow (Japan)
	else if (strcmp(romTid, "KEBJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02011A04 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011A10 = 0xE1A00000; // nop
		*(u32*)0x0203C8AC = 0xE1A00000; // nop
		*(u32*)0x0203FD48 = 0xE1A00000; // nop
		patchInitDSiWare(0x02046B30, heapEnd);
		*(u32*)0x02046EA0 = 0x020AC440;
		patchUserSettingsReadDSiWare(0x02047F74);
		*(u32*)0x020483D0 = 0xE1A00000; // nop
		*(u32*)0x020483D4 = 0xE1A00000; // nop
		*(u32*)0x020483D8 = 0xE1A00000; // nop
		*(u32*)0x020483DC = 0xE1A00000; // nop
	}

	// Electroplankton: Lumiloop (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEGE") == 0 && extendedMemory2) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02012214 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012220 = 0xE1A00000; // nop
		*(u32*)0x0203A034 = 0xE1A00000; // nop
		*(u32*)0x0203D3DC = 0xE1A00000; // nop
		patchInitDSiWare(0x02043D80, heapEnd);
		patchUserSettingsReadDSiWare(0x020451B8);
		*(u32*)0x020455EC = 0xE1A00000; // nop
		*(u32*)0x020455F0 = 0xE1A00000; // nop
		*(u32*)0x020455F4 = 0xE1A00000; // nop
		*(u32*)0x020455F8 = 0xE1A00000; // nop
	}

	// Electroplankton: Lumiloop (Europe, Australia)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEGV") == 0 && extendedMemory2) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011CF8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011D04 = 0xE1A00000; // nop
		*(u32*)0x02039B34 = 0xE1A00000; // nop
		*(u32*)0x0203CEDC = 0xE1A00000; // nop
		patchInitDSiWare(0x02043880, heapEnd);
		patchUserSettingsReadDSiWare(0x02044CB8);
		*(u32*)0x020450EC = 0xE1A00000; // nop
		*(u32*)0x020450F0 = 0xE1A00000; // nop
		*(u32*)0x020450F4 = 0xE1A00000; // nop
		*(u32*)0x020450F8 = 0xE1A00000; // nop
	}

	// Electroplankton: Lumiloop (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEGJ") == 0 && extendedMemory2) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02011A08 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011A14 = 0xE1A00000; // nop
		*(u32*)0x02039B60 = 0xE1A00000; // nop
		*(u32*)0x0203CFFC = 0xE1A00000; // nop
		patchInitDSiWare(0x02043BF4, heapEnd);
		patchUserSettingsReadDSiWare(0x02045038);
		*(u32*)0x02045494 = 0xE1A00000; // nop
		*(u32*)0x02045498 = 0xE1A00000; // nop
		*(u32*)0x0204549C = 0xE1A00000; // nop
		*(u32*)0x020454A0 = 0xE1A00000; // nop
	}

	// Electroplankton: Luminarrow (USA)
	else if (strcmp(romTid, "KECE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x0201233C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012348 = 0xE1A00000; // nop
		*(u32*)0x0203B3C0 = 0xE1A00000; // nop
		*(u32*)0x0203E768 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204510C, heapEnd);
		*(u32*)0x02045498 = 0x020A93C0;
		patchUserSettingsReadDSiWare(0x02046544);
		*(u32*)0x02046978 = 0xE1A00000; // nop
		*(u32*)0x0204697C = 0xE1A00000; // nop
		*(u32*)0x02046980 = 0xE1A00000; // nop
		*(u32*)0x02046984 = 0xE1A00000; // nop
	}

	// Electroplankton: Luminarrow (Europe, Australia)
	else if (strcmp(romTid, "KECV") == 0) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011E20 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011E2C = 0xE1A00000; // nop
		*(u32*)0x0203AEC0 = 0xE1A00000; // nop
		*(u32*)0x0203E268 = 0xE1A00000; // nop
		patchInitDSiWare(0x02044C0C, heapEnd);
		*(u32*)0x02044F98 = 0x020A8D00;
		patchUserSettingsReadDSiWare(0x02046044);
		*(u32*)0x02046478 = 0xE1A00000; // nop
		*(u32*)0x0204647C = 0xE1A00000; // nop
		*(u32*)0x02046480 = 0xE1A00000; // nop
		*(u32*)0x02046484 = 0xE1A00000; // nop
	}

	// Electroplankton: Luminarrow (Japan)
	else if (strcmp(romTid, "KECJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02011B30 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011B3C = 0xE1A00000; // nop
		*(u32*)0x0203AEF4 = 0xE1A00000; // nop
		*(u32*)0x0203E390 = 0xE1A00000; // nop
		patchInitDSiWare(0x02044F88, heapEnd);
		*(u32*)0x020452F8 = 0x020A7FC0;
		patchUserSettingsReadDSiWare(0x020463CC);
		*(u32*)0x02046828 = 0xE1A00000; // nop
		*(u32*)0x0204682C = 0xE1A00000; // nop
		*(u32*)0x02046830 = 0xE1A00000; // nop
		*(u32*)0x02046834 = 0xE1A00000; // nop
	}

	// Electroplankton: Marine-Crystals (USA)
	else if (strcmp(romTid, "KEHE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02012128 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012134 = 0xE1A00000; // nop
		*(u32*)0x0203A4C8 = 0xE1A00000; // nop
		*(u32*)0x0203D870 = 0xE1A00000; // nop
		patchInitDSiWare(0x02044214, heapEnd);
		*(u32*)0x020445A0 = 0x020A7F80;
		patchUserSettingsReadDSiWare(0x0204564C);
		*(u32*)0x02045A80 = 0xE1A00000; // nop
		*(u32*)0x02045A84 = 0xE1A00000; // nop
		*(u32*)0x02045A88 = 0xE1A00000; // nop
		*(u32*)0x02045A8C = 0xE1A00000; // nop
	}

	// Electroplankton: Marine-Crystals (Europe, Australia)
	else if (strcmp(romTid, "KEHV") == 0) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011C0C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011C18 = 0xE1A00000; // nop
		*(u32*)0x02039FC8 = 0xE1A00000; // nop
		*(u32*)0x0203D370 = 0xE1A00000; // nop
		patchInitDSiWare(0x02043D14, heapEnd);
		*(u32*)0x020440A0 = 0x020A78A0;
		patchUserSettingsReadDSiWare(0x0204514C);
		*(u32*)0x02045580 = 0xE1A00000; // nop
		*(u32*)0x02045584 = 0xE1A00000; // nop
		*(u32*)0x02045588 = 0xE1A00000; // nop
		*(u32*)0x0204558C = 0xE1A00000; // nop
	}

	// Electroplankton: Marine-Crystals (Japan)
	else if (strcmp(romTid, "KEHJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x0201191C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011928 = 0xE1A00000; // nop
		*(u32*)0x02039FF8 = 0xE1A00000; // nop
		*(u32*)0x0203D494 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204408C, heapEnd);
		*(u32*)0x020443FC = 0x020A6B80;
		patchUserSettingsReadDSiWare(0x020454D0);
		*(u32*)0x0204592C = 0xE1A00000; // nop
		*(u32*)0x02045930 = 0xE1A00000; // nop
		*(u32*)0x02045934 = 0xE1A00000; // nop
		*(u32*)0x02045938 = 0xE1A00000; // nop
	}

	// Electroplankton: Nanocarp (USA)
	else if (strcmp(romTid, "KEFE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02013DA0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02013DAC = 0xE1A00000; // nop
		*(u32*)0x0203C54C = 0xE1A00000; // nop
		*(u32*)0x0203F8F4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02046328, heapEnd);
		*(u32*)0x020466B4 = 0x020AF0C0;
		patchUserSettingsReadDSiWare(0x02047760);
		*(u32*)0x02047B94 = 0xE1A00000; // nop
		*(u32*)0x02047B98 = 0xE1A00000; // nop
		*(u32*)0x02047B9C = 0xE1A00000; // nop
		*(u32*)0x02047BA0 = 0xE1A00000; // nop
	}

	// Electroplankton: Nanocarp (Europe, Australia)
	else if (strcmp(romTid, "KEFV") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02013898 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020138A4 = 0xE1A00000; // nop
		*(u32*)0x0203C060 = 0xE1A00000; // nop
		*(u32*)0x0203F408 = 0xE1A00000; // nop
		patchInitDSiWare(0x02045E3C, heapEnd);
		*(u32*)0x020461C8 = 0x020AEB80;
		patchUserSettingsReadDSiWare(0x02047274);
		*(u32*)0x020476A8 = 0xE1A00000; // nop
		*(u32*)0x020476AC = 0xE1A00000; // nop
		*(u32*)0x020476B0 = 0xE1A00000; // nop
		*(u32*)0x020476B4 = 0xE1A00000; // nop
	}

	// Electroplankton: Nanocarp (Japan)
	else if (strcmp(romTid, "KEFJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02013594 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020135A0 = 0xE1A00000; // nop
		*(u32*)0x0203C058 = 0xE1A00000; // nop
		*(u32*)0x0203F4F4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204617C, heapEnd);
		*(u32*)0x020464EC = 0x020ADCA0;
		patchUserSettingsReadDSiWare(0x020475C0);
		*(u32*)0x02047A1C = 0xE1A00000; // nop
		*(u32*)0x02047A20 = 0xE1A00000; // nop
		*(u32*)0x02047A24 = 0xE1A00000; // nop
		*(u32*)0x02047A28 = 0xE1A00000; // nop
	}

	// Electroplankton: Rec-Rec (USA)
	else if (strcmp(romTid, "KEEE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x02012884 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012890 = 0xE1A00000; // nop
		*(u32*)0x0203AA30 = 0xE1A00000; // nop
		*(u32*)0x0203DDD8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204477C, heapEnd);
		*(u32*)0x02044B08 = 0x020AA2C0;
		patchUserSettingsReadDSiWare(0x02045BB4);
		*(u32*)0x02045FE8 = 0xE1A00000; // nop
		*(u32*)0x02045FEC = 0xE1A00000; // nop
		*(u32*)0x02045FF0 = 0xE1A00000; // nop
		*(u32*)0x02045FF4 = 0xE1A00000; // nop
	}

	// Electroplankton: Rec-Rec (Europe, Australia)
	else if (strcmp(romTid, "KEEV") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x0201237C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012388 = 0xE1A00000; // nop
		*(u32*)0x0203A544 = 0xE1A00000; // nop
		*(u32*)0x0203D8EC = 0xE1A00000; // nop
		patchInitDSiWare(0x02044290, heapEnd);
		*(u32*)0x0204461C = 0x020A9D80;
		patchUserSettingsReadDSiWare(0x020456C8);
		*(u32*)0x02045AFC = 0xE1A00000; // nop
		*(u32*)0x02045B00 = 0xE1A00000; // nop
		*(u32*)0x02045B04 = 0xE1A00000; // nop
		*(u32*)0x02045B08 = 0xE1A00000; // nop
	}

	// Electroplankton: Rec-Rec (Japan)
	else if (strcmp(romTid, "KEEJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02012078 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012084 = 0xE1A00000; // nop
		*(u32*)0x0203A580 = 0xE1A00000; // nop
		*(u32*)0x0203DA1C = 0xE1A00000; // nop
		patchInitDSiWare(0x02044614, heapEnd);
		*(u32*)0x02044984 = 0x020A8EC0;
		patchUserSettingsReadDSiWare(0x02045A58);
		*(u32*)0x02045EB4 = 0xE1A00000; // nop
		*(u32*)0x02045EB8 = 0xE1A00000; // nop
		*(u32*)0x02045EBC = 0xE1A00000; // nop
		*(u32*)0x02045EC0 = 0xE1A00000; // nop
	}

	// Electroplankton: Sun-Animalcule (USA)
	else if (strcmp(romTid, "KEDE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x020121C0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020121CC = 0xE1A00000; // nop
		*(u32*)0x0203965C = 0xE1A00000; // nop
		*(u32*)0x0203CA04 = 0xE1A00000; // nop
		patchInitDSiWare(0x020433A8, heapEnd);
		*(u32*)0x02043734 = 0x020A7160;
		patchUserSettingsReadDSiWare(0x020447E0);
		*(u32*)0x02044C14 = 0xE1A00000; // nop
		*(u32*)0x02044C18 = 0xE1A00000; // nop
		*(u32*)0x02044C1C = 0xE1A00000; // nop
		*(u32*)0x02044C20 = 0xE1A00000; // nop
	}

	// Electroplankton: Sun-Animalcule (Europe, Australia)
	else if (strcmp(romTid, "KEDV") == 0) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011CA4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011CB0 = 0xE1A00000; // nop
		*(u32*)0x0203915C = 0xE1A00000; // nop
		*(u32*)0x0203C504 = 0xE1A00000; // nop
		patchInitDSiWare(0x02042EA8, heapEnd);
		*(u32*)0x02043234 = 0x020A6A80;
		patchUserSettingsReadDSiWare(0x020442E0);
		*(u32*)0x02044714 = 0xE1A00000; // nop
		*(u32*)0x02044718 = 0xE1A00000; // nop
		*(u32*)0x0204471C = 0xE1A00000; // nop
		*(u32*)0x02044720 = 0xE1A00000; // nop
	}

	// Electroplankton: Sun-Animalcule (Japan)
	else if (strcmp(romTid, "KEDJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x020119B4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020119C0 = 0xE1A00000; // nop
		*(u32*)0x0203919C = 0xE1A00000; // nop
		*(u32*)0x0203C638 = 0xE1A00000; // nop
		patchInitDSiWare(0x02043230, heapEnd);
		*(u32*)0x020435A0 = 0x020A5D60;
		patchUserSettingsReadDSiWare(0x02044674);
		*(u32*)0x02044AD0 = 0xE1A00000; // nop
		*(u32*)0x02044AD4 = 0xE1A00000; // nop
		*(u32*)0x02044AD8 = 0xE1A00000; // nop
		*(u32*)0x02044ADC = 0xE1A00000; // nop
	}

	// Electroplankton: Trapy (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEAE") == 0 && extendedMemory2) {
		//extern u32* mepHeapSetPatch;
		//extern u32* elePlHeapAlloc;

		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		/*if (!extendedMemory2) {
			*(u32*)0x02046374 = (u32)getOffsetFromBL((u32*)0x02005508);
			tonccpy((u32*)0x02046378, elePlHeapAlloc, 0xBC);

			*(u32*)0x02003000 = (u32)getOffsetFromBL((u32*)0x020331FC);
			tonccpy((u32*)0x02003004, mepHeapSetPatch, 0x1C);

			setBL(0x02005508, 0x02046378);
			setBL(0x020331FC, 0x02003004);
		}*/
		*(u32*)0x020123F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012404 = 0xE1A00000; // nop
		*(u32*)0x0203AAE8 = 0xE1A00000; // nop
		*(u32*)0x0203DE90 = 0xE1A00000; // nop
		patchInitDSiWare(0x020449CC, /*extendedMemory2 ?*/ 0x02700000 /*: heapEnd*/);
		patchUserSettingsReadDSiWare(0x02045E04);
		*(u32*)0x02046238 = 0xE1A00000; // nop
		*(u32*)0x0204623C = 0xE1A00000; // nop
		*(u32*)0x02046240 = 0xE1A00000; // nop
		*(u32*)0x02046244 = 0xE1A00000; // nop
	}

	// Electroplankton: Trapy (Europe, Australia)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEAV") == 0 && extendedMemory2) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005238 = 0xE1A00000; // nop
		*(u32*)0x0200524C = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		*(u32*)0x02011EDC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011EE8 = 0xE1A00000; // nop
		*(u32*)0x0203A5E8 = 0xE1A00000; // nop
		*(u32*)0x0203D990 = 0xE1A00000; // nop
		patchInitDSiWare(0x020444CC, heapEnd);
		patchUserSettingsReadDSiWare(0x02045904);
		*(u32*)0x02045D38 = 0xE1A00000; // nop
		*(u32*)0x02045D3C = 0xE1A00000; // nop
		*(u32*)0x02045D40 = 0xE1A00000; // nop
		*(u32*)0x02045D44 = 0xE1A00000; // nop
	}

	// Electroplankton: Trapy (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KEAJ") == 0 && extendedMemory2) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02011C24 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011C30 = 0xE1A00000; // nop
		*(u32*)0x0203A61C = 0xE1A00000; // nop
		*(u32*)0x0203DAB8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02044868, heapEnd);
		patchUserSettingsReadDSiWare(0x02045CAC);
		*(u32*)0x02046108 = 0xE1A00000; // nop
		*(u32*)0x0204610C = 0xE1A00000; // nop
		*(u32*)0x02046110 = 0xE1A00000; // nop
		*(u32*)0x02046114 = 0xE1A00000; // nop
	}

	// Electroplankton: Varvoice (USA)
	else if (strcmp(romTid, "KEJE") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x020137C8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020137D4 = 0xE1A00000; // nop
		*(u32*)0x0203C498 = 0xE1A00000; // nop
		*(u32*)0x0203F840 = 0xE1A00000; // nop
		patchInitDSiWare(0x02046220, heapEnd);
		*(u32*)0x020465AC = 0x020AA720;
		patchUserSettingsReadDSiWare(0x02047658);
		*(u32*)0x02047A8C = 0xE1A00000; // nop
		*(u32*)0x02047A90 = 0xE1A00000; // nop
		*(u32*)0x02047A94 = 0xE1A00000; // nop
		*(u32*)0x02047A98 = 0xE1A00000; // nop
	}

	// Electroplankton: Varvoice (Europe, Australia)
	else if (strcmp(romTid, "KEJV") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x020050C8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005244 = 0xE1A00000; // nop
		*(u32*)0x02005258 = 0xE1A00000; // nop
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x0201330C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02013318 = 0xE1A00000; // nop
		*(u32*)0x0203BFF8 = 0xE1A00000; // nop
		*(u32*)0x0203F3A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02045D80, heapEnd);
		*(u32*)0x0204610C = 0x020AA280;
		patchUserSettingsReadDSiWare(0x020471B8);
		*(u32*)0x020475EC = 0xE1A00000; // nop
		*(u32*)0x020475F0 = 0xE1A00000; // nop
		*(u32*)0x020475F4 = 0xE1A00000; // nop
		*(u32*)0x020475F8 = 0xE1A00000; // nop
	}

	// Electroplankton: Varvoice (Japan)
	else if (strcmp(romTid, "KEJJ") == 0) {
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050B8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x0200525C = 0xE1A00000; // nop
		*(u32*)0x02012F20 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02012F2C = 0xE1A00000; // nop
		*(u32*)0x0203BF04 = 0xE1A00000; // nop
		*(u32*)0x0203F3A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02045FD4, heapEnd);
		*(u32*)0x02046344 = 0x020A9220;
		patchUserSettingsReadDSiWare(0x02047418);
		*(u32*)0x02047874 = 0xE1A00000; // nop
		*(u32*)0x02047878 = 0xE1A00000; // nop
		*(u32*)0x0204787C = 0xE1A00000; // nop
		*(u32*)0x02047880 = 0xE1A00000; // nop
	}

	// Escape Trick: The Secret of Rock City Prison (USA)
	// Requires 8MB of RAM(?)
	/* else if (strcmp(romTid, "K5QE") == 0) {
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020050EC = 0xE1A00000; // nop
		*(u32*)0x020052B0 = 0xE1A00000; // nop
		setBL(0x02010FD8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02011028, (u32)dsiSaveOpen);
		setBL(0x020110BC, (u32)dsiSaveGetLength);
		setBL(0x020110D8, (u32)dsiSaveRead);
		setBL(0x020110E0, (u32)dsiSaveClose);
		setBL(0x02011124, (u32)dsiSaveOpen);
		setBL(0x020111A4, (u32)dsiSaveGetLength);
		setBL(0x020111B4, (u32)dsiSaveSeek);
		setBL(0x02011230, (u32)dsiSaveClose);
		setBL(0x02011258, (u32)dsiSaveWrite);
		setBL(0x02011264, (u32)dsiSaveClose);
		*(u32*)0x0205CE84 = 0xE1A00000; // nop
		tonccpy((u32*)0x0205DA08, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020609EC = 0xE1A00000; // nop
		patchInitDSiWare(0x020679D4, heapEndExceed);
		patchUserSettingsReadDSiWare(0x02068EB0);
	} */

	// Fall in the Dark (Japan)
	// A bit hard/confusing to add save support
	else if (strcmp(romTid, "K4EJ") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02010284 = 0xE1A00000; // nop
		*(u32*)0x02013894 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019000, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A5C0);
		*(u32*)0x02022CA0 = 0xE12FFF1E; // bx lr
		if (!twlFontFound) {
			*(u32*)0x0203EE0C = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Famicom Wars DS: Ushinawareta Hikari (Japan)
	// DSi save function patching not needed 
	else if (strcmp(romTid, "Z2EJ") == 0) {
		*(u32*)0x02015BC4 = 0xE1A00000; // nop
		*(u32*)0x020197B8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202162C, heapEnd);
		*(u32*)0x020219B8 = 0x02257500;
		*(u32*)0x02022C1C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C28 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C2C = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C4C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C50 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02022C64 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022C70 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02022C74 = 0xE12FFF1E; // bx lr
	}

	// Farm Frenzy (USA)
	else if (strcmp(romTid, "KFKE") == 0) {
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x0200E938 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F4CC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011EE4 = 0xE1A00000; // nop
		patchInitDSiWare(0x020178F4, heapEnd);
		patchUserSettingsReadDSiWare(0x02019090);
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
		*(u32*)0x02041460 = 0xE12FFF1E; // bx lr
		*(u32*)0x02041480 = 0xE12FFF1E; // bx lr
	}

	// Fashion Tycoon (USA)
	// Saving not supported due to some weirdness with the code going on
	else if (strcmp(romTid, "KU7E") == 0) {
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
		doubleNopT(0x0203633E);
		// *(u16*)0x0203728C = 0x2000; // movs r0, #0 (dsiSaveOpenDir)
		// *(u16*)0x0203728E = 0x4770; // bx lr
		// *(u16*)0x020372D8 = 0x2000; // movs r0, #0 (dsiSaveCloseDir)
		// *(u16*)0x020372DA = 0x4770; // bx lr
		doubleNopT(0x02038386);
		doubleNopT(0x0203AC10);
		doubleNopT(0x0203C232);
		doubleNopT(0x0203C236);
		doubleNopT(0x0203C242);
		doubleNopT(0x0203C326);
		patchHiHeapDSiWareThumb(0x0203C364, 0x0203A144, heapEnd); // movs r0, #0x23E0000
		patchUserSettingsReadDSiWare(0x0203D182);
	}

	// Fieldrunners (USA)
	// Fieldrunners (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KFDE") == 0 || strcmp(romTid, "KFDV") == 0) {
		if (!extendedMemory2) {
			// Disable audio
			*(u32*)0x0200F52C = 0xE12FFF1E; // bx lr
			*(u32*)0x020106E0 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x0205828C = 0xE1A00000; // nop
		*(u32*)0x0205B838 = 0xE1A00000; // nop
		patchInitDSiWare(0x02061654, heapEnd);
		*(u32*)0x020619C4 = *(u32*)0x02004FC0;
		patchUserSettingsReadDSiWare(0x02062CF0);
		*(u32*)0x02063280 = 0xE1A00000; // nop
		*(u32*)0x02063284 = 0xE1A00000; // nop
		*(u32*)0x02063288 = 0xE1A00000; // nop
		*(u32*)0x0206328C = 0xE1A00000; // nop
	}

	// Fire Panic (USA)
	// Fire Panic (Europe, Australia)
	else if (strcmp(romTid, "KF8E") == 0 || strcmp(romTid, "KF8V") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0xD4;
		u16 offsetChangeS = (romTid[3] == 'E') ? 0 : 0x208;

		*(u32*)(0x02013690-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0201697C-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C5E0-offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x0201DD3C-offsetChange);
		*(u32*)(0x02052A30-offsetChangeS) = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		setBL(0x02052A50-offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x02052A68-offsetChangeS, (u32)dsiSaveSetLength);
		setBL(0x02052AA4-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x02052ACC-offsetChangeS, (u32)dsiSaveSeek);
		setBL(0x02052AEC-offsetChangeS, (u32)dsiSaveClose);
		setBL(0x02052B14-offsetChangeS, (u32)dsiSaveRead);
		setBL(0x02052B60-offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x02052BB0-offsetChangeS, (u32)dsiSaveSeek);
		*(u32*)(0x02052BC8-offsetChangeS) = 0xE1A00000; // nop
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02067754-offsetChangeS, 0x0201E290-offsetChange);
				*(u32*)(0x020678C4-offsetChangeS) = 0xE1A00000; // nop
			}
		} else {
			*(u32*)(0x020677C0-offsetChangeS) = 0xE3A00000; // mov r0, #0 (Lockup when trying to manual screen)
		}
	}

	// Fizz (USA)
	else if (strcmp(romTid, "KZZE") == 0) {
		*(u32*)0x020106E8 = 0xE1A00000; // nop
		tonccpy((u32*)0x02011260, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201391C = 0xE1A00000; // nop
		patchInitDSiWare(0x02018BD8, heapEnd8MBHack);
		*(u32*)0x02018F64 = 0x0213B440;
		patchUserSettingsReadDSiWare(0x0201A174);
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

	// Flametail (USA)
	// Cannot run properly without save patches: Shows "save data is corrupt" message
	/* else if (strcmp(romTid, "K4KE") == 0 && extendedMemory2 && twlFontFound) {
		useSharedFont = true;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x02016530 = 0xE1A00000; // nop
		*(u32*)0x02019C58 = 0xE1A00000; // nop
		*(u32*)0x0201A088 = 0xE1A00000; // nop
		*(u32*)0x0201A08C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F904, heapEnd);
		patchUserSettingsReadDSiWare(0x02020FAC);
		setB(0x0203CE54, 0x0203CE88); // Skip DSi WRAM init
		*(u32*)0x0203CF18 = 0xE1A00000; // nop (Disable audio)
		*(u32*)0x02055C60 = 0xE3A00000; // mov r0, #0
	} */

	// Flashlight (USA)
	else if (strcmp(romTid, "KFSE") == 0) {
		if (twlFontFound) {
			useSharedFont = true;
		} else {
			*(u32*)0x02005134 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0201A200 = 0xE1A00000; // nop
		*(u32*)0x0201D1F0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02021EF0, heapEnd);
		patchUserSettingsReadDSiWare(0x020233AC);
	}

	// Flashlight (Europe)
	else if (strcmp(romTid, "KFSP") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0201A10C = 0xE1A00000; // nop
		*(u32*)0x0201D0FC = 0xE1A00000; // nop
		patchInitDSiWare(0x02021DFC, heapEnd);
		patchUserSettingsReadDSiWare(0x020232B8);
	}

	// Flip the Core (USA)
	// Flip the Core (Europe)
	else if (strcmp(romTid, "KKRE") == 0 || strcmp(romTid, "KKRP") == 0) {
		const u32 dsiSaveCreateT = 0x0209B380;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x0209B390;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x0209B3A0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveGetLengthT = 0x0209B3B0;
		*(u16*)dsiSaveGetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetLengthT + 4), dsiSaveGetLength, 0xC);

		const u32 dsiSaveSeekT = 0x0209B3C0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveSetLengthT = 0x0209B408;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveReadAsyncT = 0x0209B5F4;
		*(u16*)dsiSaveReadAsyncT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadAsyncT + 4), dsiSaveRead, 0xC);

		/* const u32 dsiSaveWriteT = 0x0209B63C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC); */

		const u32 dsiSaveWriteAsyncT = 0x0209B66C;
		*(u16*)dsiSaveWriteAsyncT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteAsyncT + 4), dsiSaveWrite, 0xC);

		doubleNopT(0x02015390);
		doubleNopT(0x020157A0);
		*(u16*)0x0204A2E8 = 0x4770; // bx lr (Skip NAND error checking)
		setBLThumb(0x0204A2FC, dsiSaveCloseT);
		setBLThumb(0x0204A336, dsiSaveOpenT);
		*(u16*)0x0204A366 = 0x2001; // movs r0, #1 (dsiSaveGetArcSrc)
		*(u16*)0x0204A368 = nopT;
		setBLThumb(0x0204A39C, dsiSaveOpenT);
		*(u16*)0x0204A3C4 = 0x2001; // movs r0, #1 (dsiSaveFlush)
		*(u16*)0x0204A3C6 = nopT;
		setBLThumb(0x0204A3D4, dsiSaveGetLengthT);
		setBLThumb(0x0204A42C, dsiSaveSeekT);
		setBLThumb(0x0204A478, dsiSaveSeekT);
		*(u16*)0x02053154 = 0x2301; // movs r3, #1
		doubleNopT(0x0205F696);
		doubleNopT(0x0205F69C);

		*(u16*)0x0209A3A2 += 0x1000; // beq -> b
		doubleNopT(0x0209A4E6);
		doubleNopT(0x0209C876);
		doubleNopT(0x020A09AC);
		doubleNopT(0x020A1FD6);
		doubleNopT(0x020A1FDA);
		doubleNopT(0x020A1FE6);
		doubleNopT(0x020A20CA);
		patchHiHeapDSiWareThumb(0x020A2108, 0x0209FD38, heapEnd);
		patchUserSettingsReadDSiWare(0x020A2F2A);
		doubleNopT(0x020A31F8);
		*(u16*)0x020A31FC = nopT;
		*(u16*)0x020A31FE = nopT;
		doubleNopT(0x020A3200);
		*(u16*)0x020D86B6 = nopT;
		*(u16*)0x020D86B8 = nopT;
	}

	// Flipper (USA)
	// Music will not play on retail consoles
	else if (strcmp(romTid, "KFPE") == 0) {
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x0200DE64 = 0xE1A00000; // nop
		*(u32*)0x02031F9C = 0xE1A00000; // nop
		*(u32*)0x020351FC = 0xE1A00000; // nop
		patchInitDSiWare(0x0203A1EC, heapEnd);
		patchUserSettingsReadDSiWare(0x0203B7E4);
	}

	// Flipper (Europe)
	// Music will not play on retail consoles
	else if (strcmp(romTid, "KFPP") == 0) {
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x0200DECC = 0xE1A00000; // nop
		*(u32*)0x02032008 = 0xE1A00000; // nop
		*(u32*)0x02035268 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203A258, heapEnd);
		patchUserSettingsReadDSiWare(0x0203B8C0);
	}

	// Flipper 2: Flush the Goldfish (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KKNE") == 0 && extendedMemory2) {
		*(u32*)0x020051EC = 0xE1A00000; // nop
		*(u32*)0x02005208 = 0xE1A00000; // nop
		*(u32*)0x02005220 = 0xE1A00000; // nop
		*(u32*)0x02005224 = 0xE1A00000; // nop
		*(u32*)0x02005228 = 0xE1A00000; // nop
		*(u32*)0x0200522C = 0xE1A00000; // nop
		*(u32*)0x02005230 = 0xE1A00000; // nop
		*(u32*)0x02005234 = 0xE1A00000; // nop
		*(u32*)0x02008160 = 0xE1A00000; // nop
		*(u32*)0x0203A338 = 0xE1A00000; // nop
		*(u32*)0x0203D5A4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02042180, heapEnd);
		patchUserSettingsReadDSiWare(0x020437F8);
	}

	// Flips: The Bubonic Builders (USA)
	// Flips: The Bubonic Builders (Europe, Australia)
	// Flips: Silent But Deadly (USA)
	// Flips: Silent But Deadly (Europe, Australia)
	// Flips: Terror in Cubicle Four (USA)
	// Flips: Terror in Cubicle Four (Europe, Australia)
	else if (strcmp(romTid, "KFUE") == 0 || strcmp(romTid, "KFUV") == 0
		   || strcmp(romTid, "KF4E") == 0 || strcmp(romTid, "KF4V") == 0
		   || strcmp(romTid, "KF9E") == 0 || strcmp(romTid, "KF9V") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0xC4;
		u8 offsetChange2 = (romTid[3] == 'E') ? 0 : 0xD0;
		u8 offsetChange3 = (romTid[3] == 'E') ? 0 : 0x30;
		u8 offsetChangeS = (romTid[3] == 'E') ? 0 : 0x20;

		*(u32*)0x02015604 = 0xE1A00000; // nop
		*(u32*)0x02019390 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025E8C, heapEnd);
		*(u32*)0x02026218 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02027578);
		*(u32*)0x0202CED0 = extendedMemory2 ? 0xE3A00001 : 0xE3A00000; // mov r0, #extendedMemory2 ? 1 : 0
		*(u32*)0x0202CED4 = 0xE12FFF1E; // bx lr
		*(u32*)(0x02036A7C+offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x02040394+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020403AC+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02040424+offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x02040434+offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x02040764+offsetChange) = 0xE3A00000; // mov r0, #0
		setBL(0x02044210-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x02044220-offsetChangeS, (u32)dsiSaveGetLength);
		setBL(0x02044234-offsetChangeS, (u32)dsiSaveSetLength);
		*(u32*)(0x0204428C-offsetChangeS) = 0xE3A00000; // mov r0, #0 (dsiSaveGetArcSrc)
		*(u32*)(0x020442D4-offsetChangeS) = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)(0x02044308-offsetChangeS) = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0204431C-offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x0204432C-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x0204433C-offsetChangeS, (u32)dsiSaveGetResultCode);
		setBL(0x0204435C-offsetChangeS, (u32)dsiSaveSetLength);
		setBL(0x02044378-offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x02044380-offsetChangeS, (u32)dsiSaveClose);
		*(u32*)(0x020443D0-offsetChangeS) = 0xE1A00000; // nop
		*(u32*)(0x020443E0-offsetChangeS) = 0xE3A00000; // mov r0, #0
		*(u32*)(0x020443F8-offsetChangeS) = 0xE1A00000; // nop
		if (strcmp(romTid, "KF4V") != 0 && strcmp(romTid, "KF9V") != 0) {
			setBL(0x0204471C-offsetChangeS, (u32)dsiSaveOpen);
			setBL(0x0204472C-offsetChangeS, (u32)dsiSaveGetResultCode);
			setBL(0x02044750-offsetChangeS, (u32)dsiSaveSeek);
			setBL(0x02044774-offsetChangeS, (u32)dsiSaveRead);
			setBL(0x0204477C-offsetChangeS, (u32)dsiSaveClose);
			setBL(0x0204483C-offsetChangeS, (u32)dsiSaveOpen);
			setBL(0x0204484C-offsetChangeS, (u32)dsiSaveGetResultCode);
			setBL(0x02044870-offsetChangeS, (u32)dsiSaveSeek);
			setBL(0x02044890-offsetChangeS, (u32)dsiSaveWrite);
			setBL(0x02044898-offsetChangeS, (u32)dsiSaveClose);
		} else {
			setBL(0x02044748, (u32)dsiSaveOpen);
			setBL(0x02044758, (u32)dsiSaveGetResultCode);
			setBL(0x0204477C, (u32)dsiSaveSeek);
			setBL(0x020447A0, (u32)dsiSaveRead);
			setBL(0x020447A8, (u32)dsiSaveClose);
			setBL(0x02044868, (u32)dsiSaveOpen);
			setBL(0x02044878, (u32)dsiSaveGetResultCode);
			setBL(0x0204489C, (u32)dsiSaveSeek);
			setBL(0x020448BC, (u32)dsiSaveWrite);
			setBL(0x020448C4, (u32)dsiSaveClose);
		}
		if (strncmp(romTid, "KFU", 3) == 0) {
			patchMsg16(0x020D879C-offsetChange3, dsiRequiredMsg);
		} else if (strncmp(romTid, "KF4", 3) == 0) {
			patchMsg16(0x020D8784-offsetChange3, dsiRequiredMsg);
		} else {
			patchMsg16(0x020D87A8-offsetChange3, dsiRequiredMsg);
		}
	}

	// Flips: The Enchanted Wood (USA)
	// Flips: The Enchanted Wood (Europe, Australia)
	// Flips: The Folk of the Faraway Tree (USA)
	// Flips: The Folk of the Faraway Tree (Europe, Australia)
	// Flips: The Magic Faraway Tree (USA)
	// Flips: The Magic Faraway Tree (Europe, Australia)
	else if (strcmp(romTid, "KFFE") == 0 || strcmp(romTid, "KFFV") == 0
		  || strcmp(romTid, "KF6E") == 0 || strcmp(romTid, "KF6V") == 0
		  || strcmp(romTid, "KFTE") == 0 || strcmp(romTid, "KFTV") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x2C;
		u8 offsetChange3 = (romTid[3] == 'E') ? 0 : 0x40;
		u8 offsetChangeS = (romTid[3] == 'E') ? 0 : 0x44;

		*(u32*)0x02015670 = 0xE1A00000; // nop
		*(u32*)0x020193FC = 0xE1A00000; // nop
		patchInitDSiWare(0x02025F28, heapEnd);
		*(u32*)0x020262B4 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02027614);
		*(u32*)0x0202CF6C = extendedMemory2 ? 0xE3A00001 : 0xE3A00000; // mov r0, #extendedMemory2 ? 1 : 0
		*(u32*)0x0202CF70 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203B23C = 0xE1A00000; // nop
		*(u32*)(0x020449A8-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020449C0-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02044A38-offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x02044A48-offsetChange) = 0xE12FFF1E; // bx lr
		*(u32*)(0x02044D9C-offsetChange) = 0xE3A00000; // mov r0, #0
		setBL(0x020487A4-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x020487B4-offsetChangeS, (u32)dsiSaveGetLength);
		setBL(0x020487C8-offsetChangeS, (u32)dsiSaveSetLength);
		*(u32*)(0x02048820-offsetChangeS) = 0xE3A00000; // mov r0, #0 (dsiSaveGetArcSrc)
		*(u32*)(0x02048868-offsetChangeS) = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)(0x0204889C-offsetChangeS) = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x020488B0-offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x020488C0-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x020488D0-offsetChangeS, (u32)dsiSaveGetResultCode);
		setBL(0x020488F0-offsetChangeS, (u32)dsiSaveSetLength);
		setBL(0x0204890C-offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x02048914-offsetChangeS, (u32)dsiSaveClose);
		setBL(0x020492D8-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x020492E8-offsetChangeS, (u32)dsiSaveGetResultCode);
		setBL(0x0204930C-offsetChangeS, (u32)dsiSaveSeek);
		setBL(0x02049330-offsetChangeS, (u32)dsiSaveRead);
		setBL(0x02049338-offsetChangeS, (u32)dsiSaveClose);
		setBL(0x020493F8-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x02049408-offsetChangeS, (u32)dsiSaveGetResultCode);
		setBL(0x0204942C-offsetChangeS, (u32)dsiSaveSeek);
		setBL(0x0204944C-offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x02049454-offsetChangeS, (u32)dsiSaveClose);
		if (strncmp(romTid, "KFF", 3) == 0) {
			patchMsg16(0x020DDAD0-offsetChange3, dsiRequiredMsg);
		} else if (strncmp(romTid, "KFT", 3) == 0) {
			patchMsg16(0x020DDADC-offsetChange3, dsiRequiredMsg);
		} else {
			patchMsg16(0x020DDAEC-offsetChange3, dsiRequiredMsg);
		}
	}

	// Flips: More Bloody Horowitz (USA)
	// Flips: More Bloody Horowitz (Europe, Australia)
	else if (strcmp(romTid, "KFHE") == 0 || strcmp(romTid, "KFHV") == 0) {
		*(u32*)0x02014768 = 0xE1A00000; // nop
		*(u32*)0x020184B0 = 0xE1A00000; // nop
		patchInitDSiWare(0x020246CC, heapEnd);
		*(u32*)0x02024A58 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02025D7C);
		*(u32*)0x0202B4EC = extendedMemory2 ? 0xE3A00001 : 0xE3A00000; // mov r0, #extendedMemory2 ? 1 : 0
		*(u32*)0x0202B4F0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020307AC = 0xE1A00000; // nop
		*(u32*)0x02039828 = 0xE1A00000; // nop
		*(u32*)0x02039840 = 0xE1A00000; // nop
		*(u32*)0x020398B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020398C4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02039C14 = 0xE3A00000; // mov r0, #0
		setBL(0x0203DA40, (u32)dsiSaveOpen);
		setBL(0x0203DA50, (u32)dsiSaveGetLength);
		setBL(0x0203DA64, (u32)dsiSaveSetLength);
		*(u32*)0x0203DA8C = 0xE3A00000; // mov r0, #0 (dsiSaveGetArcSrc)
		*(u32*)0x0203DAD4 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0203DB08 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0203DB1C, (u32)dsiSaveCreate);
		setBL(0x0203DB2C, (u32)dsiSaveOpen);
		setBL(0x0203DB3C, (u32)dsiSaveGetResultCode);
		setBL(0x0203DB5C, (u32)dsiSaveSetLength);
		setBL(0x0203DB88, (u32)dsiSaveWrite);
		setBL(0x0203DB90, (u32)dsiSaveClose);
		setBL(0x0203DEF4, (u32)dsiSaveOpen);
		setBL(0x0203DF04, (u32)dsiSaveGetResultCode);
		setBL(0x0203DF28, (u32)dsiSaveSeek);
		setBL(0x0203DF4C, (u32)dsiSaveRead);
		setBL(0x0203DF54, (u32)dsiSaveClose);
		setBL(0x0203DFD8, (u32)dsiSaveOpen);
		setBL(0x0203DFE8, (u32)dsiSaveGetResultCode);
		setBL(0x0203E00C, (u32)dsiSaveSeek);
		setBL(0x0203E02C, (u32)dsiSaveWrite);
		setBL(0x0203E034, (u32)dsiSaveClose);
	}

	// Frenzic (USA)
	// Frenzic (Europe)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KFOE") == 0 || strcmp(romTid, "KFOP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x7C;

		if (romTid[3] == 'E') {
			*(u16*)0x0202A644 += 0x1000; // beq -> b
		} else {
			*(u16*)0x0202A710 += 0x1000; // beq -> b
		}
		if (!extendedMemory2) {
			*(u16*)(0x0206E8DC+offsetChange) = 0x4770; // bx lr (Disable loading exception textures)
		}
		*(u16*)(0x0206F160+offsetChange) = 0x4770; // bx lr

		doubleNopT(0x02075356+offsetChange);
		doubleNopT(0x02077C62+offsetChange);
		doubleNopT(0x0207B31C+offsetChange);
		doubleNopT(0x02087BD6+offsetChange);
		doubleNopT(0x02087BDA+offsetChange);
		doubleNopT(0x02087BE6+offsetChange);
		doubleNopT(0x02087CCA+offsetChange);
		patchHiHeapDSiWareThumb(0x02087D08+offsetChange, 0x0207A0EC+offsetChange, heapEnd);
		*(u32*)(0x02087DE0+offsetChange) = *(u32*)0x02004FDC;
		patchUserSettingsReadDSiWare(0x02088A36+offsetChange);
	}

	// Frogger Returns (USA)
	else if (strcmp(romTid, "KFGE") == 0) {
		*(u32*)0x020117D4 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201234C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020152F0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BFB4, heapEnd8MBHack);
		*(u32*)0x020381BC = 0xE1A00000; // nop
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

	// Fuuu! Dairoujou Kai (Japan)
	else if (strcmp(romTid, "K6JJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200DDFC = 0xE1A00000; // nop
		*(u32*)0x020115C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201DB18, heapEnd);
		patchUserSettingsReadDSiWare(0x0201F014);
		*(u32*)0x0201F030 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F034 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F03C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201F040 = 0xE12FFF1E; // bx lr
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203A2DC, 0x0201F580);
			*(u32*)0x0203A39C = 0xE1A00000; // nop
		}
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

	// Gaia's Moon (USA)
	// Gaia's Moon (Europe)
	else if (strcmp(romTid, "KKGE") == 0 || strcmp(romTid, "KKGP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x4C;

		*(u32*)0x020050F4 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x02005140 = 0xE1A00000; // nop
		*(u32*)0x02005178 = 0xE1A00000; // nop
		*(u32*)0x02005180 = 0xE1A00000; // nop
		*(u32*)0x020110F8 = 0xE1A00000; // nop
		*(u32*)0x0201112C = 0xE1A00000; // nop
		setBL(0x02011E88, (u32)dsiSaveCreate);
		*(u32*)0x02011E9C = 0xE1A00000; // nop
		*(u32*)0x02011EA8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011F3C = 0xE1A00000; // nop
		*(u32*)0x02011F40 = 0xE1A00000; // nop
		*(u32*)0x02011F44 = 0xE1A00000; // nop
		setBL(0x02011F50, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011F6C = 0xE1A00000; // nop
		*(u32*)0x02011F78 = 0xE3A00001; // mov r0, #1
		setBL(0x020301E8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02030200+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02030210+offsetChange, (u32)dsiSaveSeek);
		setBL(0x02030220+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02030228+offsetChange, (u32)dsiSaveClose);
		setBL(0x02030298+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020302B0+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x020302C4+offsetChange, (u32)dsiSaveSeek);
		setBL(0x020302D4+offsetChange, (u32)dsiSaveRead);
		setBL(0x020302DC+offsetChange, (u32)dsiSaveClose);
		setBL(0x02030354+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02030380+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020303BC+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020303CC+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02060760+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x02068E94+offsetChange, heapEnd);
		*(u32*)(0x02069220+offsetChange) -= 0x30000;
		*(u32*)(0x0206F820+offsetChange) = 0xE1A00000; // nop
	}

	// Gaia's Moon (Japan)
	else if (strcmp(romTid, "KKGJ") == 0) {
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02010CB0 = 0xE1A00000; // nop
		*(u32*)0x02010CE4 = 0xE1A00000; // nop
		setBL(0x02011A40, (u32)dsiSaveCreate);
		*(u32*)0x02011A54 = 0xE1A00000; // nop
		*(u32*)0x02011A60 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011AF4 = 0xE1A00000; // nop
		*(u32*)0x02011AF8 = 0xE1A00000; // nop
		*(u32*)0x02011AFC = 0xE1A00000; // nop
		setBL(0x02011B08, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011B24 = 0xE1A00000; // nop
		*(u32*)0x02011B30 = 0xE3A00001; // mov r0, #1
		setBL(0x0202FD8C, (u32)dsiSaveOpen);
		setBL(0x0202FDA4, (u32)dsiSaveGetLength);
		setBL(0x0202FDB4, (u32)dsiSaveSeek);
		setBL(0x0202FDC4, (u32)dsiSaveWrite);
		setBL(0x0202FDCC, (u32)dsiSaveClose);
		setBL(0x0202FE3C, (u32)dsiSaveOpen);
		setBL(0x0202FE54, (u32)dsiSaveGetLength);
		setBL(0x0202FE68, (u32)dsiSaveSeek);
		setBL(0x0202FE78, (u32)dsiSaveRead);
		setBL(0x0202FE80, (u32)dsiSaveClose);
		setBL(0x0202FEF8, (u32)dsiSaveCreate);
		setBL(0x0202FF24, (u32)dsiSaveOpen);
		setBL(0x0202FF60, (u32)dsiSaveWrite);
		setBL(0x0202FF70, (u32)dsiSaveClose);
		*(u32*)0x0205FDD8 = 0xE1A00000; // nop
		patchInitDSiWare(0x020684E0, heapEnd);
		*(u32*)0x0206886C -= 0x30000;
		*(u32*)0x0206EE6C = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KGBO") == 0 || strcmp(romTid, "KGHO") == 0 || strcmp(romTid, "KGJO") == 0 || strcmp(romTid, "KGMO") == 0 || strcmp(romTid, "KGVO") == 0) {
		*(u32*)0x0201007C = 0xE1A00000; // nop
		*(u32*)0x020138C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019620, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AB3C);
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
	else if (strcmp(romTid, "KGBJ") == 0 || strcmp(romTid, "KGHJ") == 0 || strcmp(romTid, "KGJJ") == 0 || strcmp(romTid, "KGMJ") == 0 || strcmp(romTid, "KGVJ") == 0) {
		*(u32*)0x02010024 = 0xE1A00000; // nop
		*(u32*)0x02013804 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019474, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A98C);
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
	else if (strcmp(romTid, "KGCO") == 0) {
		*(u32*)0x02011B84 = 0xE1A00000; // nop
		*(u32*)0x020153C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B318, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C834);
		*(u32*)0x0202F0FC = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Chef (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGCJ") == 0) {
		*(u32*)0x02011B2C = 0xE1A00000; // nop
		*(u32*)0x0201530C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B16C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C684);
		*(u32*)0x0202EE9C = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Donkey Kong Jr. (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGDO") == 0) {
		*(u32*)0x0201007C = 0xE1A00000; // nop
		*(u32*)0x020138C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201989C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201ADB8);
		*(u32*)0x0202D860 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Donkey Kong Jr. (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGDJ") == 0) {
		*(u32*)0x02010024 = 0xE1A00000; // nop
		*(u32*)0x02013804 = 0xE1A00000; // nop
		patchInitDSiWare(0x020196F0, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AC08);
		*(u32*)0x0202D600 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Flagman (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGGO") == 0) {
		*(u32*)0x020104C8 = 0xE1A00000; // nop
		*(u32*)0x02013D0C = 0xE1A00000; // nop
		patchInitDSiWare(0x02019A6C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AF88);
		*(u32*)0x0202D520 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Flagman (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGGJ") == 0) {
		*(u32*)0x02010470 = 0xE1A00000; // nop
		*(u32*)0x02013C50 = 0xE1A00000; // nop
		patchInitDSiWare(0x020198C0, heapEnd);
		patchUserSettingsReadDSiWare(0x0201ADD8);
		*(u32*)0x0202D2C0 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Mario's Cement Factory (USA, Europe)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGFO") == 0) {
		*(u32*)0x02011B84 = 0xE1A00000; // nop
		*(u32*)0x020153C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B3A4, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C8C0);
		*(u32*)0x0202F188 = 0xE12FFF1E; // bx lr
	}

	// Game & Watch: Mario's Cement Factory (Japan)
	// Softlocks after 3 misses or exiting gameplay
	else if (strcmp(romTid, "KGFJ") == 0) {
		*(u32*)0x02011B2C = 0xE1A00000; // nop
		*(u32*)0x02015250 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B1F8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C710);
		*(u32*)0x0202EF28 = 0xE12FFF1E; // bx lr
	}

	// Ginsei Tsume-Shougi (Japan)
	// A bit hard/confusing to add save support
	else if (strcmp(romTid, "K2MJ") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x02011288 = 0xE1A00000; // nop
		*(u32*)0x02014E1C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A260, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B830);
		if (!useSharedFont) {
			*(u32*)0x02082050 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Globulos Party (USA)
	// Globulos Party (Europe)
	else if (strncmp(romTid, "KGS", 3) == 0) {
		*(u32*)0x0201ABFC = 0xE1A00000; // nop
		*(u32*)0x0201AC04 = 0xE1A00000; // nop
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
		*(u32*)0x0205D3B8 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Punito 20 no Asobiba (Japan)
	else if (strcmp(romTid, "KU2J") == 0) {
		*(u32*)0x0201AB3C = 0xE1A00000; // nop
		*(u32*)0x0201AB44 = 0xE1A00000; // nop
		setBL(0x02040804, (u32)dsiSaveOpen);
		setBL(0x0204083C, (u32)dsiSaveClose);
		setBL(0x02040898, (u32)dsiSaveCreate);
		setBL(0x02040928, (u32)dsiSaveDelete);
		setBL(0x020409F4, (u32)dsiSaveSeek);
		setBL(0x02040A14, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02040A28, (u32)dsiSaveRead);
		setBL(0x02040A44, (u32)dsiSaveClose);
		setBL(0x02040A60, (u32)dsiSaveClose);
		setBL(0x02040B18, (u32)dsiSaveSeek);
		setBL(0x02040B38, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02040B4C, (u32)dsiSaveWrite);
		setBL(0x02040B68, (u32)dsiSaveClose);
		setBL(0x02040B84, (u32)dsiSaveClose);
		tonccpy((u32*)0x02058E14, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B318 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Glory Days: Tactical Defense (USA)
	// Glory Days: Tactical Defense (Europe)
	else if (strcmp(romTid, "KGKE") == 0 || strcmp(romTid, "KGKP") == 0) {
		// *(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x0200B488 = 0xE1A00000; // nop
		*(u32*)0x0200E7A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02018F08, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A404);
		*(u32*)0x0201FB24 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201FB28 = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
			for (int i = 0; i < 12; i++) {
				u32* offset = (u32*)0x0206710C;
				offset[i] = 0xE1A00000; // nop
			}
			*(u32*)0x020671B4 = 0xE1A00000; // nop
			for (int i = 0; i < 10; i++) {
				u32* offset = (u32*)0x02075514;
				offset[i] = 0xE1A00000; // nop
			}
		} else {
			for (int i = 0; i < 12; i++) {
				u32* offset = (u32*)0x02067264;
				offset[i] = 0xE1A00000; // nop
			}
			*(u32*)0x0206730C = 0xE1A00000; // nop
			for (int i = 0; i < 10; i++) {
				u32* offset = (u32*)0x0207566C;
				offset[i] = 0xE1A00000; // nop
			}
		}
	}

	// Go Fetch! (USA)
	else if (strcmp(romTid, "KGXE") == 0) {
		*(u32*)0x02010CA0 = 0xE1A00000; // nop
		tonccpy((u32*)0x02011824, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020143C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BB38, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D014);
		setBL(0x0203BD8C, (u32)dsiSaveOpen);
		setBL(0x0203BDA4, (u32)dsiSaveCreate);
		setBL(0x0203BDBC, (u32)dsiSaveOpen);
		setBL(0x0203BDDC, (u32)dsiSaveWrite);
		setBL(0x0203BDEC, (u32)dsiSaveClose);
		setBL(0x0203BE08, (u32)dsiSaveClose);
		setBL(0x0203BE44, (u32)dsiSaveOpen);
		setBL(0x0203BE64, (u32)dsiSaveRead);
		setBL(0x0203BE74, (u32)dsiSaveClose);
		setBL(0x0203BE90, (u32)dsiSaveClose);
		setBL(0x0203BF40, (u32)dsiSaveCreate);
		setBL(0x0203BF50, (u32)dsiSaveOpen);
		setBL(0x0203BF7C, (u32)dsiSaveClose);
		setBL(0x0203BFA8, (u32)dsiSaveCreate);
		setBL(0x0203BFB8, (u32)dsiSaveOpen);
		setBL(0x0203BFE4, (u32)dsiSaveClose);
	}

	// Go Fetch! (Japan)
	else if (strcmp(romTid, "KGXJ") == 0) {
		*(u32*)0x02010CA0 = 0xE1A00000; // nop
		tonccpy((u32*)0x02011824, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02014A6C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C4F8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D9D4);
		setBL(0x0203CAE8, (u32)dsiSaveOpen);
		setBL(0x0203CB00, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0203CB18, (u32)dsiSaveOpen);
		setBL(0x0203CB38, (u32)dsiSaveWrite);
		setBL(0x0203CB48, (u32)dsiSaveClose);
		setBL(0x0203CB58, (u32)dsiSaveClose);
		setBL(0x0203CB98, (u32)dsiSaveOpen);
		setBL(0x0203CBBC, (u32)dsiSaveRead);
		setBL(0x0203CBCC, (u32)dsiSaveClose);
		setBL(0x0203CBDC, (u32)dsiSaveClose);
		setBL(0x0203CCBC, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0203CCCC, (u32)dsiSaveOpen);
		setBL(0x0203CCF8, (u32)dsiSaveClose);
		setBL(0x0203CD30, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0203CD40, (u32)dsiSaveOpen);
		setBL(0x0203CD6C, (u32)dsiSaveClose);
	}

	// Go Fetch! 2 (USA)
	// Go Fetch! 2 (Japan)
	else if (strcmp(romTid, "KKFE") == 0 || strcmp(romTid, "KKFJ") == 0) {
		*(u32*)0x020187A8 = 0xE1A00000; // nop
		tonccpy((u32*)0x02019338, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201C634 = 0xE1A00000; // nop
		patchInitDSiWare(0x02028FDC, heapEnd);
		patchUserSettingsReadDSiWare(0x0202A68C);
		*(u32*)0x0203262C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02032630 = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
			*(u32*)0x0203B09C = 0xE1A00000; // nop
			*(u32*)0x0203B0B4 = 0xE1A00000; // nop
			setBL(0x02040240, (u32)dsiSaveOpen);
			setBL(0x02040258, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02040270, (u32)dsiSaveOpen);
			setBL(0x02040290, (u32)dsiSaveWrite);
			setBL(0x020402A0, (u32)dsiSaveClose);
			setBL(0x020402B0, (u32)dsiSaveClose);
			setBL(0x020402F0, (u32)dsiSaveOpen);
			setBL(0x02040314, (u32)dsiSaveRead);
			setBL(0x02040324, (u32)dsiSaveClose);
			setBL(0x02040334, (u32)dsiSaveClose);
			setBL(0x02040414, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02040424, (u32)dsiSaveOpen);
			setBL(0x02040450, (u32)dsiSaveClose);
			setBL(0x02040488, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02040498, (u32)dsiSaveOpen);
			setBL(0x020404C4, (u32)dsiSaveClose);
		} else {
			*(u32*)0x02058154 = 0xE1A00000; // nop
			*(u32*)0x0205816C = 0xE1A00000; // nop
			setBL(0x0205DD14, (u32)dsiSaveOpen);
			setBL(0x0205DD2C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0205DD44, (u32)dsiSaveOpen);
			setBL(0x0205DD64, (u32)dsiSaveWrite);
			setBL(0x0205DD74, (u32)dsiSaveClose);
			setBL(0x0205DD84, (u32)dsiSaveClose);
			setBL(0x0205DDC4, (u32)dsiSaveOpen);
			setBL(0x0205DDE8, (u32)dsiSaveRead);
			setBL(0x0205DDF8, (u32)dsiSaveClose);
			setBL(0x0205DE08, (u32)dsiSaveClose);
			setBL(0x0205DEE8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0205DEF8, (u32)dsiSaveOpen);
			setBL(0x0205DF24, (u32)dsiSaveClose);
			setBL(0x0205DF5C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0205DF6C, (u32)dsiSaveOpen);
			setBL(0x0205DF98, (u32)dsiSaveClose);
		}
	}

	// Go! Go! Island Rescue! (USA)
	// Go! Go! Island Rescue! (Europe, Australia)
	else if (strcmp(romTid, "KGQE") == 0 || strcmp(romTid, "KGQV") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x98;
		u8 offsetChangeS = (romTid[3] == 'E') ? 0 : 0x14;
		u8 offsetChangeS2 = (romTid[3] == 'E') ? 0 : 0x1C;
		u8 offsetChangeS3 = (romTid[3] == 'E') ? 0 : 0x24;
		u16 offsetChange2 = (romTid[3] == 'E') ? 0 : 0x2AC;

		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x02005090 = 0xE1A00000; // nop
		*(u32*)(0x02014CE0-offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x02015874-offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x020183AC-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D9C0-offsetChange, heapEnd);
		setBL(0x0202AD60-offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x0202AD74-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x0202AD9C-offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x0202ADB4-offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x0202ADD8-offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x0202ADE8-offsetChangeS, (u32)dsiSaveClose);
		setBL(0x0202AE68-offsetChangeS2, (u32)dsiSaveWrite);
		setBL(0x0202AE70-offsetChangeS2, (u32)dsiSaveClose);
		setBL(0x0202AEDC-offsetChangeS3, (u32)dsiSaveOpen);
		*(u32*)(0x0202AEA4-offsetChangeS3) = 0xE12FFF1E; // bx lr
		*(u32*)(0x0202AEE8-offsetChangeS3) = 0xE1A00000; // nop
		*(u32*)(0x0202AEF4-offsetChangeS3) = 0xE3A00003; // mov r0, #3
		setBL(0x0202B09C-offsetChangeS3, (u32)dsiSaveGetLength);
		setBL(0x0202B0BC-offsetChangeS3, (u32)dsiSaveRead);
		setBL(0x0202B0C4-offsetChangeS3, (u32)dsiSaveClose);
		if (useSharedFont) {
			if (extendedMemory2) {
				*(u32*)(0x0206A870+offsetChange2) = 0xE3A05603; // mov r5, #0x300000
			} else {
				*(u32*)(0x0206A870+offsetChange2) = 0xE3A05601; // mov r5, #0x100000
				patchTwlFontLoad(0x0206A980+offsetChange2, 0x0201F534-offsetChange);
			}
		} else {
			*(u32*)(0x0205BA38+offsetChange2) = 0xE3A00000; // mov r0, #0 (Skip Manual screen, and instead display the unused help menu)
		}
	}

	// Go! Go! Kokopolo (USA)
	// Go! Go! Kokopolo (Europe)
	else if (strcmp(romTid, "K3GE") == 0 || strcmp(romTid, "K3GP") == 0) {
		// extern u32* goGoKokopoloHeapAddrPtr;
		const u32 readCodeCopy = 0x02013CF4;

		/* if (!extendedMemory2 && s2FlashcardId == 0x5A45) {
			*goGoKokopoloHeapAddrPtr -= 0x01000000;
		} */

		if (!extendedMemory2) {
			extern u32* goGoKokopoloHeapAlloc;
			tonccpy((u32*)0x0201DB58, (u32*)goGoKokopoloHeapAlloc, 0xC);
		}

		*(u32*)0x02012738 = 0xE1A00000; // nop
		*(u32*)0x02015C98 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BF88, heapEnd);
		*(u32*)0x0201C314 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201D604);
		*(u32*)0x02022AD8 = 0xE1A00000; // nop
		*(u32*)0x02022ADC = 0xE1A00000; // nop
		*(u32*)0x02022B0C = 0xE1A00000; // nop
		if (ndsHeader->romversion == 0) {
			if (romTid[3] == 'E') {
				*(u32*)0x02042F40 = 0xE1A00000; // nop
				*(u32*)0x02042F5C = 0xE1A00000; // nop

				codeCopy((u32*)readCodeCopy, (u32*)0x020BCB48, 0x70);

				*(u32*)0x020431B0 = 0xE1A00000; // nop
				setBL(0x02043290, readCodeCopy);
				setBL(0x02043328, readCodeCopy);

				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);

				*(u32*)0x0208EB74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

				if (!extendedMemory2) {
					setBL(0x020B5EA4, 0x0201DB58);
				}

				setBL(0x020BCCF4, (u32)dsiSaveCreate);
				setBL(0x020BCD04, (u32)dsiSaveOpen);
				setBL(0x020BCD18, (u32)dsiSaveSetLength);
				setBL(0x020BCD28, (u32)dsiSaveWrite);
				setBL(0x020BCD30, (u32)dsiSaveClose);

				*(u32*)0x020BE934 = 0xE3A00000; // mov r0, #0
			} else {
				*(u32*)0x02042F9C = 0xE1A00000; // nop
				*(u32*)0x02042FB8 = 0xE1A00000; // nop

				codeCopy((u32*)readCodeCopy, (u32*)0x020BCC6C, 0x70);

				*(u32*)0x0204320C = 0xE1A00000; // nop
				setBL(0x020432EC, readCodeCopy);
				setBL(0x02043384, readCodeCopy);

				setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
				setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
				setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
				setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);

				*(u32*)0x0208EC38 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

				if (!extendedMemory2) {
					setBL(0x020B5FAC, 0x0201DB58);
				}

				setBL(0x020BCE18, (u32)dsiSaveCreate);
				setBL(0x020BCE28, (u32)dsiSaveOpen);
				setBL(0x020BCE3C, (u32)dsiSaveSetLength);
				setBL(0x020BCE4C, (u32)dsiSaveWrite);
				setBL(0x020BCE54, (u32)dsiSaveClose);

				*(u32*)0x020BEA68 = 0xE3A00000; // mov r0, #0
			}
		} else {
			*(u32*)0x02042F9C = 0xE1A00000; // nop
			*(u32*)0x02042FB8 = 0xE1A00000; // nop

			codeCopy((u32*)readCodeCopy, (u32*)0x020BCDA4, 0x70);

			*(u32*)0x0204320C = 0xE1A00000; // nop
			setBL(0x020432EC, readCodeCopy);
			setBL(0x02043384, readCodeCopy);

			setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
			setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
			setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
			setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);

			*(u32*)0x0208EE9C = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

			if (!extendedMemory2) {
				setBL(0x020B60E4, 0x0201DB58);
			}

			setBL(0x020BCF50, (u32)dsiSaveCreate);
			setBL(0x020BCF60, (u32)dsiSaveOpen);
			setBL(0x020BCF74, (u32)dsiSaveSetLength);
			setBL(0x020BCF84, (u32)dsiSaveWrite);
			setBL(0x020BCF8C, (u32)dsiSaveClose);

			*(u32*)0x020BEBA0 = 0xE3A00000; // mov r0, #0
		}
	}

	// Go! Go! Kokopolo (Japan)
	else if (strcmp(romTid, "K3GJ") == 0) {
		if (!extendedMemory2) {
			extern u32* goGoKokopoloHeapAlloc;
			tonccpy((u32*)0x0201DB88, (u32*)goGoKokopoloHeapAlloc, 0xC);
		}

		*(u32*)0x02012768 = 0xE1A00000; // nop
		*(u32*)0x02015CC8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BFB8, heapEnd);
		*(u32*)0x0201C344 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201D634);
		*(u32*)0x02022B08 = 0xE1A00000; // nop
		*(u32*)0x02022B0C = 0xE1A00000; // nop
		*(u32*)0x02022B3C = 0xE1A00000; // nop

		*(u32*)0x02042EFC = 0xE1A00000; // nop
		*(u32*)0x02042F18 = 0xE1A00000; // nop

		const u32 readCodeCopy = 0x02013D24;
		codeCopy((u32*)readCodeCopy, (u32*)0x020BCD90, 0x70);

		*(u32*)0x0204316C = 0xE1A00000; // nop
		setBL(0x0204324C, readCodeCopy);
		setBL(0x020432E4, readCodeCopy);

		setBL(readCodeCopy+0x24, (u32)dsiSaveOpenR);
		setBL(readCodeCopy+0x34, (u32)dsiSaveGetLength);
		setBL(readCodeCopy+0x44, (u32)dsiSaveRead);
		setBL(readCodeCopy+0x4C, (u32)dsiSaveClose);

		*(u32*)0x0208EC74 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)

		if (!extendedMemory2) {
			setBL(0x020B60D0, 0x0201DB88);
		}

		setBL(0x020BCF3C, (u32)dsiSaveCreate);
		setBL(0x020BCF4C, (u32)dsiSaveOpen);
		setBL(0x020BCF60, (u32)dsiSaveSetLength);
		setBL(0x020BCF70, (u32)dsiSaveWrite);
		setBL(0x020BCF78, (u32)dsiSaveClose);

		*(u32*)0x020BEB8C = 0xE3A00000; // mov r0, #0
	}

	// Goooooal America (USA)
	// Audio does not play
	else if (strcmp(romTid, "K9AE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201CE24 = 0xE1A00000; // nop
		*(u32*)0x02021154 = 0xE1A00000; // nop
		patchInitDSiWare(0x02026950, heapEnd);
		patchUserSettingsReadDSiWare(0x02027F34);
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
		*(u32*)0x02053380 = 0xE12FFF1E; // bx lr
	}

	// Goooooal Europa 2012 (Europe)
	// Audio does not play
	else if (strcmp(romTid, "K9AP") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x020127C8 = 0xE1A00000; // nop
		*(u32*)0x02016AF8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C2F4, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D8D8);
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
		*(u32*)0x02048CDC = 0xE12FFF1E; // bx lr
	}

	// Gold Fever (USA)
	// Requires more than 8MB of RAM
	/*else if (strcmp(romTid, "KG7E") == 0) {
		*(u32*)0x02013E80 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02014008 = 0xE1A00000; // nop
		*(u32*)0x020142E0 = 0xE1A00000; // nop
		*(u32*)0x02023AA4 = 0xE1A00000; // nop
		*(u32*)0x02031854 = 0xE1A00000; // nop
		*(u32*)0x02032698 = 0xE1A00000; // nop
		*(u32*)0x02032A04 = 0xE1A00000; // nop
		*(u32*)0x020392C0 = 0xE1A00000; // nop
		*(u32*)0x0203B2F8 = 0xE1A00000; // nop
		*(u32*)0x0203E7B4 = 0xE1A00000; // nop
		*(u32*)0x02041FD8 = 0xE1A00000; // nop
		*(u32*)0x02043E5C = 0xE1A00000; // nop
		*(u32*)0x02043E60 = 0xE1A00000; // nop
		*(u32*)0x02043E6C = 0xE1A00000; // nop
		*(u32*)0x02043FCC = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02044028, heapEnd); // mov r0, #0x23E0000
		patchUserSettingsReadDSiWare(0x0204546C);
		*(u32*)0x02048314 = 0xE1A00000; // nop
	}*/

	// Hachiwandaiba DS: Naru Zouku Ha Samishougi (Japan)
	else if (strcmp(romTid, "K83J") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02010124 = 0xE1A00000; // nop
		*(u32*)0x02013464 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201855C, heapEnd);
		patchUserSettingsReadDSiWare(0x02019B0C);
		if (!twlFontFound) {
			*(u32*)0x02043198 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Halloween Trick or Treat (USA)
	else if (strcmp(romTid, "KZHE") == 0) {
		*(u32*)0x0200E0D0 = 0xE1A00000; // nop
		*(u32*)0x0201159C = 0xE1A00000; // nop
		patchInitDSiWare(0x020162C0, heapEnd);
		patchUserSettingsReadDSiWare(0x02017770);
		if (!extendedMemory2) {
			*(u32*)0x0201C924 = 0xE3A05901; // mov r5, #0x4000 (Disable music)
		}
		*(u32*)0x0201D36C = 0xE1A00000; // nop
		*(u32*)0x0201D380 = 0xE1A00000; // nop
		setBL(0x0203D11C, (u32)dsiSaveOpen);
		setBL(0x0203D190, (u32)dsiSaveRead);
		setBL(0x0203D198, (u32)dsiSaveClose);
		*(u32*)0x0203D1D0 = 0xE1A00000; // nop
		setBL(0x0203D364, (u32)dsiSaveOpen);
		setBL(0x0203D38C, (u32)dsiSaveCreate);
		setBL(0x0203D39C, (u32)dsiSaveOpen);
		setBL(0x0203D3B8, (u32)dsiSaveSetLength);
		setBL(0x0203D3F0, (u32)dsiSaveSeek);
		setBL(0x0203D400, (u32)dsiSaveWrite);
		setBL(0x0203D408, (u32)dsiSaveClose);
		*(u32*)0x0203D478 = 0xE1A00000; // nop
	}

	// Halloween Trick or Treat (Europe)
	else if (strcmp(romTid, "KZHP") == 0) {
		*(u32*)0x0200E1A4 = 0xE1A00000; // nop
		*(u32*)0x02011670 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016394, heapEnd);
		patchUserSettingsReadDSiWare(0x02017844);
		if (!extendedMemory2) {
			*(u32*)0x0201C9F8 = 0xE3A05901; // mov r5, #0x4000 (Disable music)
		}
		*(u32*)0x0201D440 = 0xE1A00000; // nop
		*(u32*)0x0201D454 = 0xE1A00000; // nop
		setBL(0x0203D1E4, (u32)dsiSaveOpen);
		setBL(0x0203D258, (u32)dsiSaveRead);
		setBL(0x0203D260, (u32)dsiSaveClose);
		*(u32*)0x0203D298 = 0xE1A00000; // nop
		setBL(0x0203D42C, (u32)dsiSaveOpen);
		setBL(0x0203D454, (u32)dsiSaveCreate);
		setBL(0x0203D464, (u32)dsiSaveOpen);
		setBL(0x0203D480, (u32)dsiSaveSetLength);
		setBL(0x0203D4B8, (u32)dsiSaveSeek);
		setBL(0x0203D4C8, (u32)dsiSaveWrite);
		setBL(0x0203D4D0, (u32)dsiSaveClose);
		*(u32*)0x0203D540 = 0xE1A00000; // nop
	}

	// Handy Hockey (Japan)
	else if (strcmp(romTid, "KHOJ") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200B534 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200C1D4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200F0D4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CA9C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E0E8);
		*(u32*)0x0201E110 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E114 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E11C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E120 = 0xE12FFF1E; // bx lr
		*(u32*)0x02031440 = 0xE1A00000; // nop (dsiSaveCreateDir)
		*(u32*)0x020314AC = 0xE3A00001; // mov r0, #1 (dsiSaveCreateDirAuto)
		setBL(0x0203150C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02031530, (u32)dsiSaveOpen);
		*(u32*)0x0203156C = 0xE1A00000; // nop
		setBL(0x02031580, (u32)dsiSaveSetLength);
		setBL(0x02031590, (u32)dsiSaveWrite);
		setBL(0x02031598, (u32)dsiSaveClose);
		setBL(0x0203162C, (u32)dsiSaveOpen);
		*(u32*)0x02031660 = 0xE1A00000; // nop
		setBL(0x02031674, (u32)dsiSaveGetLength);
		setBL(0x02031694, (u32)dsiSaveClose);
		setBL(0x020316B0, (u32)dsiSaveRead);
		setBL(0x020316B8, (u32)dsiSaveClose);
	}

	// Handy Mahjong (Japan)
	else if (strcmp(romTid, "KHMJ") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200D5EC = 0xE1A00000; // nop
		*(u32*)0x0201095C = 0xE1A00000; // nop
		*(u32*)0x0201DDD0 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x0201DDE8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201F424);
		*(u32*)0x0201F44C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F450 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F458 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201F45C = 0xE12FFF1E; // bx lr
	}

	// Hard-Hat Domo (USA)
	else if (strcmp(romTid, "KDHE") == 0) {
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
		doubleNopT(0x02024C7E);
		doubleNopT(0x02022A2E);
		doubleNopT(0x02027900);
		doubleNopT(0x02028EBA);
		doubleNopT(0x02028EBE);
		doubleNopT(0x02028ECA);
		doubleNopT(0x02028FAE);
		patchHiHeapDSiWareThumb(0x02028FEC, 0x02023938, heapEnd); // movs r0, #0x23E0000
		doubleNopT(0x0202A076);
		*(u16*)0x0202A078 = nopT;
		*(u16*)0x0202A07A = nopT;
		doubleNopT(0x0202A07E);
	}

	// Hearts Spades Euchre (USA)
	else if (strcmp(romTid, "KHQE") == 0) {
		setBL(0x020107B4, (u32)dsiSaveOpen);
		setBL(0x02010828, (u32)dsiSaveGetLength);
		setBL(0x0201083C, (u32)dsiSaveClose);
		setBL(0x0201085C, (u32)dsiSaveSeek);
		setBL(0x02010874, (u32)dsiSaveRead);
		setBL(0x02010888, (u32)dsiSaveClose);
		setBL(0x020108DC, (u32)dsiSaveClose);
		*(u32*)0x0201092C = 0xE1A00000; // nop
		setBL(0x0201098C, (u32)dsiSaveCreate);
		setBL(0x020109E0, (u32)dsiSaveOpen);
		setBL(0x02010A48, (u32)dsiSaveSetLength);
		setBL(0x02010A60, (u32)dsiSaveClose);
		setBL(0x02010AB4, (u32)dsiSaveGetLength);
		setBL(0x02010AC8, (u32)dsiSaveClose);
		setBL(0x02010AE8, (u32)dsiSaveSeek);
		setBL(0x02010B00, (u32)dsiSaveWrite);
		setBL(0x02010B14, (u32)dsiSaveClose);
		setBL(0x02010B60, (u32)dsiSaveClose);
		*(u32*)0x02049760 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Hearts Spades Euchre (Europe)
	else if (strcmp(romTid, "KHQP") == 0) {
		setBL(0x02010770, (u32)dsiSaveOpen);
		setBL(0x020107E4, (u32)dsiSaveGetLength);
		setBL(0x020107F8, (u32)dsiSaveClose);
		setBL(0x02010818, (u32)dsiSaveSeek);
		setBL(0x02010830, (u32)dsiSaveRead);
		setBL(0x02010844, (u32)dsiSaveClose);
		setBL(0x02010898, (u32)dsiSaveClose);
		*(u32*)0x020108E8 = 0xE1A00000; // nop
		setBL(0x02010948, (u32)dsiSaveCreate);
		setBL(0x0201099C, (u32)dsiSaveOpen);
		setBL(0x02010A04, (u32)dsiSaveSetLength);
		setBL(0x02010A1C, (u32)dsiSaveClose);
		setBL(0x02010A70, (u32)dsiSaveGetLength);
		setBL(0x02010A84, (u32)dsiSaveClose);
		setBL(0x02010AA4, (u32)dsiSaveSeek);
		setBL(0x02010ABC, (u32)dsiSaveWrite);
		setBL(0x02010AD0, (u32)dsiSaveClose);
		setBL(0x02010B1C, (u32)dsiSaveClose);
		*(u32*)0x02049748 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Heathcliff: Spot On (USA)
	else if (strcmp(romTid, "K6SE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0201615C = 0xE1A00000; // nop
		*(u32*)0x02019EDC = 0xE1A00000; // nop
		patchInitDSiWare(0x02020F3C, heapEnd);
		*(u32*)0x02027DE4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02027DE8 = 0xE12FFF1E; // bx lr
		setBL(0x0204BF68, (u32)dsiSaveOpenR);
		setBL(0x0204BF88, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0204BFC0, (u32)dsiSaveOpen);
		setBL(0x0204BFD4, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204BFEC = 0xE1A00000; // nop
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

	// Hell's Kitchen VS (USA)
	else if (strcmp(romTid, "KHLE") == 0) {
		*(u32*)0x020A661C = 0xE1A00000; // nop
		*(u32*)0x020A9B00 = 0xE1A00000; // nop
		patchInitDSiWare(0x020B0C5C, heapEnd);
		*(u32*)0x020B21F4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B21F8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020B2200 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B2204 = 0xE12FFF1E; // bx lr
	}

	// Hell's Kitchen VS (Europe, Australia)
	else if (strcmp(romTid, "KHLV") == 0) {
		*(u32*)0x020A65A4 = 0xE1A00000; // nop
		*(u32*)0x020A9A88 = 0xE1A00000; // nop
		patchInitDSiWare(0x020B0BE4, heapEnd);
		*(u32*)0x020B217C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B2180 = 0xE12FFF1E; // bx lr
		*(u32*)0x020B2188 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B218C = 0xE12FFF1E; // bx lr
	}

	// Hellokids: Vol. 1: Coloring and Painting! (USA)
	// Loops in some code during white screens
	/*else if (strcmp(romTid, "KKIE") == 0) {
		*(u32*)0x0200B888 = 0xE3A02001; // mov r2, #1
		*(u32*)0x02028700 = 0xE1A00000; // nop
		*(u32*)0x0202F7F4 = 0xE1A00000; // nop
		*(u32*)0x02034310 = 0xE1A00000; // nop
		*(u32*)0x020361A8 = 0xE1A00000; // nop
		*(u32*)0x020361AC = 0xE1A00000; // nop
		*(u32*)0x020361B8 = 0xE1A00000; // nop
		*(u32*)0x02036318 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02036374, heapEnd); // mov r0, #0x23E0000
		patchUserSettingsReadDSiWare(0x02037868);
		*(u32*)0x02039C48 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02039C80 = 0xE1A00000; // nop
		*(u32*)0x0203B674 = 0xE1A00000; // nop
	}*/

	// High Stakes Texas Hold'em (USA)
	// High Stakes Texas Hold'em (Europe, Australia)
	else if (strcmp(romTid, "KTXE") == 0 || strcmp(romTid, "KTXV") == 0) {
		*(u32*)0x0200E1A0 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200EE34, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011F20 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AF98, heapEnd);
		// *(u32*)0x0201B308 = *(u32*)0x02004FC0;
		*(u32*)0x0201B308 -= 0x30000;
		patchUserSettingsReadDSiWare(0x0201C824);
		*(u32*)0x0201C84C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201C850 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201C858 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201C85C = 0xE12FFF1E; // bx lr
		setBL(0x0203A774, (u32)dsiSaveGetInfo);
		*(u32*)0x0203A7B4 = 0xE1A00000; // nop
		setBL(0x0203A7E0, (u32)dsiSaveCreate);
		setBL(0x0203A820, (u32)dsiSaveOpen);
		setBL(0x0203A850, (u32)dsiSaveSetLength);
		setBL(0x0203A860, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0203A8A0, (u32)dsiSaveClose);
		setBL(0x0203A96C, (u32)dsiSaveOpen);
		setBL(0x0203A9D0, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0203AA0C, (u32)dsiSaveClose);
		if (romTid[3] == 'E') {
			*(u32*)0x02064E50 = 0x280000;
			*(u32*)0x02064E54 = 0x4000;
		} else {
			*(u32*)0x02064E70 = 0x280000;
			*(u32*)0x02064E74 = 0x4000;
		}
	}

	// Hints Hunter (USA)
	else if (strcmp(romTid, "KHIE") == 0) {
		*(u32*)0x0201672C = 0xE1A00000; // nop
		*(u32*)0x020199EC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F340, heapEnd);
		*(u32*)0x0201F6CC = 0x02096EE0;
		patchUserSettingsReadDSiWare(0x020207D0);
		setBL(0x020472CC, (u32)dsiSaveOpen);
		setBL(0x020472E0, (u32)dsiSaveCreate);
		setBL(0x020472E8, (u32)dsiSaveGetResultCode);
		setBL(0x02047354, (u32)dsiSaveOpen);
		setBL(0x02047364, (u32)dsiSaveGetResultCode);
		*(u32*)0x02047374 = 0xE1A00000; // nop
		setBL(0x02047380, (u32)dsiSaveCreate);
		setBL(0x02047390, (u32)dsiSaveOpen);
		setBL(0x020473B8, (u32)dsiSaveWrite);
		setBL(0x020473C8, (u32)dsiSaveWrite);
		setBL(0x020473D8, (u32)dsiSaveWrite);
		setBL(0x02047410, (u32)dsiSaveWrite);
		setBL(0x02047418, (u32)dsiSaveClose);
		setBL(0x02047558, (u32)dsiSaveGetInfo);
		setBL(0x02047568, (u32)dsiSaveOpen);
		setBL(0x02047588, (u32)dsiSaveSeek);
		setBL(0x02047598, (u32)dsiSaveWrite);
		setBL(0x020475A0, (u32)dsiSaveClose);
		setBL(0x020475E4, (u32)dsiSaveOpen);
		setBL(0x020475F4, (u32)dsiSaveSeek);
		setBL(0x02047604, (u32)dsiSaveRead);
		setBL(0x0204760C, (u32)dsiSaveClose);
		*(u32*)0x020494FC = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Hints Hunter (Europe)
	else if (strcmp(romTid, "KHIP") == 0) {
		*(u32*)0x020050A0 = 0xE1A00000; // nop
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x020167B0 = 0xE1A00000; // nop
		*(u32*)0x02019B04 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F490, heapEnd);
		*(u32*)0x0201F81C = 0x0209B4E0;
		patchUserSettingsReadDSiWare(0x02020930);
		setBL(0x02044060, (u32)dsiSaveOpen);
		setBL(0x02044074, (u32)dsiSaveGetResultCode);
		setBL(0x02044098, (u32)dsiSaveCreate);
		setBL(0x020440A0, (u32)dsiSaveGetResultCode);
		setBL(0x0204414C, (u32)dsiSaveOpen);
		setBL(0x0204415C, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204416C = 0xE1A00000; // nop
		setBL(0x02044178, (u32)dsiSaveCreate);
		setBL(0x02044188, (u32)dsiSaveOpen);
		setBL(0x020441B0, (u32)dsiSaveWrite);
		setBL(0x020441C0, (u32)dsiSaveWrite);
		setBL(0x020441D4, (u32)dsiSaveWrite);
		setBL(0x02044220, (u32)dsiSaveWrite);
		setBL(0x0204422C, (u32)dsiSaveClose);
		*(u32*)0x02044268 = 0xE1A00000; // nop
		*(u32*)0x02044288 = 0xE1A00000; // nop
		setBL(0x020443E4, (u32)dsiSaveGetInfo);
		setBL(0x020443F4, (u32)dsiSaveOpen);
		setBL(0x02044414, (u32)dsiSaveSeek);
		setBL(0x02044424, (u32)dsiSaveWrite);
		setBL(0x0204442C, (u32)dsiSaveClose);
		setBL(0x02044470, (u32)dsiSaveOpen);
		setBL(0x02044480, (u32)dsiSaveSeek);
		setBL(0x02044490, (u32)dsiSaveRead);
		setBL(0x02044498, (u32)dsiSaveClose);
		*(u32*)0x02046834 = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Ichi Moudaji!: Neko King (Japan)
	// Either this uses more than one save file in filesystem, or saving is somehow just bugged
	/* if (strcmp(romTid, "KNEJ") == 0) {
		*(u32*)0x02003398 = 0xE1A00000; // nop
		*(u32*)0x02011F90 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x02012210, (u32)dsiSaveCreate);
		setBL(0x0201224C, (u32)dsiSaveOpen);
		setBL(0x02012284, (u32)dsiSaveSetLength);
		setBL(0x02012294, (u32)dsiSaveWrite);
		setBL(0x020122AC, (u32)dsiSaveClose);
		setBL(0x02012328, (u32)dsiSaveOpen);
		setBL(0x02012360, (u32)dsiSaveSetLength);
		setBL(0x02012370, (u32)dsiSaveWrite);
		setBL(0x02012388, (u32)dsiSaveClose);
		setBL(0x020123FC, (u32)dsiSaveOpen);
		setBL(0x02012434, (u32)dsiSaveRead);
		setBL(0x02012448, (u32)dsiSaveClose);
		setBL(0x0201248C, (u32)dsiSaveDelete);
		setBL(0x0200124F8, (u32)dsiSaveGetInfo);
		*(u32*)0x0201253C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc & dsiSaveFreeSpaceAvailable)
		*(u32*)0x02012540 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202BAD0 = 0xE1A00000; // nop
		tonccpy((u32*)0x0202C764, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0202F998 = 0xE1A00000; // nop
		*(u32*)0x020373A8 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020373C0, heapEnd);
		patchUserSettingsReadDSiWare(0x020388A8);

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0201289C;
			offset[i] = 0xE1A00000; // nop
		}
	} */

	// Invasion of the Alien Blobs (USA)
	// Branches to DSi code in ITCM?
	/*else if (strcmp(romTid, "KBTE") == 0) {
		*(u32*)0x0201BF84 = 0xE1A00000; // nop
		*(u32*)0x0201BF94 = 0xE1A00000; // nop
		*(u32*)0x0201BFB0 = 0xE1A00000; // nop
		*(u32*)0x02021214 = 0xE1A00000; // nop
		*(u32*)0x020224EC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020224F0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02023498 = 0xE1A00000; // nop
		*(u32*)0x0203C79C = 0xE1A00000; // nop
		*(u32*)0x020404A8 = 0xE1A00000; // nop
		*(u32*)0x02046608 = 0xE1A00000; // nop
		*(u32*)0x02048444 = 0xE1A00000; // nop
		*(u32*)0x02048448 = 0xE1A00000; // nop
		*(u32*)0x02048454 = 0xE1A00000; // nop
		*(u32*)0x020485B4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02048610, heapEnd8MBHack);
		patchUserSettingsReadDSiWare(0x02049894);
	}*/

	// Ivy the Kiwi? mini (USA)
	// GO Series: Ivy the Kiwi? mini (Europe, Australia)
	else if (strcmp(romTid, "KIKX") == 0 || strcmp(romTid, "KIKV") == 0) {
		u8 offsetChange = (romTid[3] == 'X') ? 0 : 0x48;
		u16 offsetChangeS = (romTid[3] == 'X') ? 0 : 0x344;

		*(u32*)0x020124D4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02013058, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02015BAC = 0xE1A00000; // nop
		patchInitDSiWare(0x02025468, heapEnd);
		patchUserSettingsReadDSiWare(0x02026990);
		*(u32*)0x0202C1A8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202C1AC = 0xE12FFF1E; // bx lr
		*(u32*)(0x02064EC8+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02064EDC+offsetChange) = 0xE1A00000; // nop
		setBL(0x020B0AB8+offsetChangeS, (u32)dsiSaveCreate);
		*(u32*)(0x020B0D94+offsetChangeS) = 0xE1A00000; // nop
		setBL(0x020B1078+offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x020B1088+offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x020B10A8+offsetChangeS, (u32)dsiSaveSetLength);
		setBL(0x020B10E8+offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x020B10F0+offsetChangeS, (u32)dsiSaveClose);
		*(u32*)(0x020B113C+offsetChangeS) = 0xE1A00000; // nop
		setBL(0x020B11D8+offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x020B1200+offsetChangeS, (u32)dsiSaveRead);
		setBL(0x020B1208+offsetChangeS, (u32)dsiSaveClose);
		*(u32*)(0x020B128C+offsetChangeS) = 0xE1A00000; // nop
		setBL(0x020B1324+offsetChangeS, (u32)dsiSaveCreate);
		setBL(0x020B1334+offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x020B135C+offsetChangeS, (u32)dsiSaveSetLength);
		setBL(0x020B13A4+offsetChangeS, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x020B13E8+offsetChangeS, (u32)dsiSaveClose);
		setBL(0x020B1448+offsetChangeS, (u32)dsiSaveClose);
		setBL(0x020B1464+offsetChangeS, (u32)dsiSaveClose);
		*(u32*)(0x020B1470+offsetChangeS) = 0xE1A00000; // nop
	}

	// Jazzy Billiards (USA)
	// Saving not supported due to code taking place in the overlays
	else if (strcmp(romTid, "K9BE") == 0) {
		if (!extendedMemory2) {
			*(u32*)0x020070CC = 0xE3A06816; // mov r6, #0x160000
		}
		*(u32*)0x020326A8 = 0xE1A00000; // nop
		*(u32*)0x020364BC = 0xE1A00000; // nop
		patchInitDSiWare(0x0203F588, heapEnd);
		*(u32*)0x0203F8F8 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02040AC8);
		*(u32*)0x02046694 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02046698 = 0xE12FFF1E; // bx lr
	}

	// Jazzy Billiards (Europe, Australia)
	// Jazzy Billiards (Korea)
	// Saving not supported due to code taking place in the overlays
	else if (strcmp(romTid, "K9BV") == 0 || strcmp(romTid, "K9BK") == 0) {
		if (!extendedMemory2) {
			*(u32*)0x020070DC = 0xE3A06816; // mov r6, #0x160000
		}
		*(u32*)0x020326E4 = 0xE1A00000; // nop
		*(u32*)0x020363A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203ECFC, heapEnd);
		*(u32*)0x0203F088 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02040298);
		*(u32*)0x02045C90 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02045C94 = 0xE12FFF1E; // bx lr
	}

	// Jazzy Billiards (Japan)
	// Saving not supported due to code taking place in the overlays
	else if (strcmp(romTid, "K9BJ") == 0) {
		if (!extendedMemory2) {
			*(u32*)0x020070C8 = 0xE3A06816; // mov r6, #0x160000
		}
		*(u32*)0x020326A4 = 0xE1A00000; // nop
		*(u32*)0x020364B8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203F584, heapEnd);
		*(u32*)0x0203F8F4 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02040AC4);
		*(u32*)0x02046690 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02046694 = 0xE12FFF1E; // bx lr
	}

	// JellyCar 2 (USA)
	else if (strcmp(romTid, "KJYE") == 0) {
		*(u32*)0x02006334 = 0xE1A00000; // nop
		*(u32*)0x0200634C = 0xE1A00000; // nop
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
		*(u32*)0x020B4694 = 0xE1A00000; // nop
		*(u32*)0x020B7F60 = 0xE1A00000; // nop
		patchInitDSiWare(0x020BE070, heapEnd);
		*(u32*)0x020BE3FC = 0x0213D220;
		patchUserSettingsReadDSiWare(0x020BF7D0);
	}

	// Jewel Adventures (USA)
	else if (strcmp(romTid, "KYJE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02019D44 = 0xE1A00000; // nop
		*(u32*)0x0201D904 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025A3C, heapEnd);
		setBL(0x0203F3A4, (u32)dsiSaveOpen);
		setBL(0x0203F3C0, (u32)dsiSaveWrite);
		setBL(0x0203F3CC, (u32)dsiSaveClose);
		setBL(0x0203F420, (u32)dsiSaveOpen);
		setBL(0x0203F434, (u32)dsiSaveGetLength);
		setBL(0x0203F448, (u32)dsiSaveRead);
		setBL(0x0203F454, (u32)dsiSaveClose);
		setBL(0x0206FFE8, (u32)dsiSaveOpenR);
		setBL(0x02070044, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02070128, (u32)dsiSaveOpen);
		setBL(0x0207013C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02070154 = 0xE1A00000; // nop
	}

	// Jewel Keepers: Easter Island (USA)
	// Jewel Keepers: Easter Island (Europe)
	else if (strcmp(romTid, "KJBE") == 0 || strcmp(romTid, "KJBP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x20;
		*(u32*)(0x020061D4-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020061E8-offsetChange) = 0xE3A00001; // mov r0, #1
		*(u32*)(0x02006808-offsetChange) = 0xE3A00000; // mov r0, #0
		*(u32*)(0x02006824-offsetChange) = 0xE3A00000; // mov r0, #0
		setBL(0x0200B318-offsetChange, 0x0201EBB0-offsetChange);
		*(u32*)(0x0201E7B8-offsetChange) = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)(0x0201E7E8-offsetChange) = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)(0x0201E860-offsetChange) = 0xE1A00000; // nop (dsiSaveGetArcSrc)
		*(u32*)(0x0201E8A4-offsetChange) = 0xE1A00000; // nop
		setBL(0x0201E8E0-offsetChange, (u32)dsiSaveCreate);
		setBL(0x0201E8F0-offsetChange, (u32)dsiSaveOpen);
		*(u32*)(0x0201E910-offsetChange) = 0xE1A00000; // nop
		setBL(0x0201E928-offsetChange, (u32)dsiSaveSetLength);
		setBL(0x0201E938-offsetChange, (u32)dsiSaveWrite);
		setBL(0x0201E940-offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0201E950-offsetChange) = 0xE1A00000; // nop
		setBL(0x0201EC08-offsetChange, (u32)dsiSaveOpen);
		setBL(0x0201EC38-offsetChange, (u32)dsiSaveRead);
		setBL(0x0201EC40-offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02031EC4-offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x02032A48-offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x02035578-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0203B404-offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x0203CA58-offsetChange);
		*(u32*)(0x0203CE60-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203CE64-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203CE68-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0203CE6C-offsetChange) = 0xE1A00000; // nop
	}

	// Jewel Legends: Tree of Life (USA)
	else if (strcmp(romTid, "KUKE") == 0) {
		*(u32*)0x02010848 = 0xE1A00000; // nop
		setBL(0x0201E25C, (u32)dsiSaveOpen);
		setBL(0x0201E26C, (u32)dsiSaveClose);
		*(u32*)0x0201E2E4 = 0xE1A00000; // nop
		setBL(0x0201E6D8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0201E6FC, (u32)dsiSaveOpen);
		setBL(0x0201E710, (u32)dsiSaveSetLength);
		setBL(0x0201E720, (u32)dsiSaveClose);
		setBL(0x0201E7A4, (u32)dsiSaveOpen);
		setBL(0x0201E834, (u32)dsiSaveClose);
		setBL(0x0201E8BC, (u32)dsiSaveSeek);
		setBL(0x0201E8D4, (u32)dsiSaveRead);
		setBL(0x0201E95C, (u32)dsiSaveSeek);
		setBL(0x0201E974, (u32)dsiSaveWrite);
		*(u32*)0x0203E324 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02044FD8 = 0xE1A00000; // nop
		tonccpy((u32*)0x02046AB4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020499F4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205050C, heapEnd);
		patchUserSettingsReadDSiWare(0x02051BA0);
	}

	// Jewel Quest 4: Heritage (USA)
	// Jewel Quest 4: Heritage (Europe)
	// Weird crash when saving
	/* else if (strcmp(romTid, "K43E") == 0 || strcmp(romTid, "K43P") == 0) {
		const u32 dsiSaveCreateT = 0x020B18B0;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x020B18C0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x020B18D0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveGetLengthT = 0x020B18E0;
		*(u16*)dsiSaveGetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetLengthT + 4), dsiSaveGetLength, 0xC);

		const u32 dsiSaveSeekT = 0x020B18F0;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveSetLengthT = 0x020B1938;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveReadAsyncT = 0x020B1B24;
		*(u16*)dsiSaveReadAsyncT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadAsyncT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteAsyncT = 0x020B1B9C;
		*(u16*)dsiSaveWriteAsyncT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteAsyncT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x0206C40C = 0x4770; // bx lr (Skip NAND error checking)
		setBLThumb(0x0206C460, dsiSaveCloseT);
		setBLThumb(0x0206C49C, dsiSaveOpenT);
		doubleNopT(0x0206C4B4);
		*(u16*)0x0206C4D8 = 0x2001; // movs r0, #1 (dsiSaveGetArcSrc)
		*(u16*)0x0206C4DA = nopT;
		setBLThumb(0x0206C50E, dsiSaveOpenT);
		setBLThumb(0x0206C546, dsiSaveGetLengthT);
		*(u16*)0x0206C536 = 0x2001; // movs r0, #1 (dsiSaveFlush)
		*(u16*)0x0206C538 = nopT;
		setBLThumb(0x0206C59C, dsiSaveSeekT);
		setBLThumb(0x0206C5E8, dsiSaveSeekT);
		*(u16*)0x020711B4 = 0x2301; // movs r3, #1
		doubleNopT(0x0207ABC6);
		doubleNopT(0x0207ABCC);

		*(u16*)0x020B08D2 += 0x1000; // beq -> b
		doubleNopT(0x020B0A16);
		doubleNopT(0x020B2DA6);
		doubleNopT(0x020B5E70);
		doubleNopT(0x020B74E2);
		doubleNopT(0x020B74E6);
		doubleNopT(0x020B74F2);
		doubleNopT(0x020B75D6);
		patchHiHeapDSiWareThumb(0x020B7614, 0x020B5274, heapEnd);
		*(u32*)0x020B76EC = *(u32*)0x02004FF4;
		patchUserSettingsReadDSiWare(0x020B848A);
		doubleNopT(0x020B8758);
		*(u16*)0x020B875C = nopT;
		*(u16*)0x020B875E = nopT;
		doubleNopT(0x020B8760);
		*(u16*)0x02122B22 = nopT;
		*(u16*)0x02122B24 = nopT;
	} */

	// Jinia Supasonaru: Eiwa Rakubiki Jiten (Japan)
	// Saving not supported due to using more than one file in filesystem
	// Requires Slot-2 RAM expansion up to 16MB or more (Standard Memory Expansion Pak is not enough)
	else if (strcmp(romTid, "KD3J") == 0 && largeS2RAM) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x02040178 = 0xE3A00001; // mov r0, #1 (Hide battery icon)
		*(u32*)0x020401F0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205CE84 = 0xE1A00000; // nop
		*(u32*)0x0205E7DC = 0xE1A00000; // nop
		*(u32*)0x0205E7E4 = 0xE1A00000; // nop
		if (!useSharedFont) {
			*(u32*)0x0205EE08 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x0206E828 = (s2FlashcardId == 0x5A45) ? 0xE3A00408 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08000000 : #0x09000000
		*(u32*)0x020A6914 = 0xE1A00000; // nop
		*(u32*)0x020A9390 = 0xE1A00000; // nop
		*(u32*)0x020AD834 = 0xE1A00000; // nop
		*(u32*)0x020B2E48 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020B2E60, heapEnd);
		patchUserSettingsReadDSiWare(0x020B46C4);
		*(u32*)0x020BB044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BB048 = 0xE12FFF1E; // bx lr
	}

	// Jinia Supasonaru: Waei Rakubiki Jiten (Japan)
	// Saving not supported due to using more than one file in filesystem
	// Requires Memory Expansion Pak
	else if (strcmp(romTid, "KD5J") == 0 && expansionPakFound) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x02040214 = 0xE3A00001; // mov r0, #1 (Hide battery icon)
		*(u32*)0x0204028C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205CD14 = 0xE1A00000; // nop
		*(u32*)0x0205E66C = 0xE1A00000; // nop
		*(u32*)0x0205E674 = 0xE1A00000; // nop
		if (!useSharedFont) {
			*(u32*)0x0205EC98 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x0206E6B8 = (s2FlashcardId == 0x5A45) ? 0xE3A00408 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08000000 : #0x09000000
		*(u32*)0x020A67A4 = 0xE1A00000; // nop
		*(u32*)0x020A8F3C = 0xE1A00000; // nop
		*(u32*)0x020AD3E0 = 0xE1A00000; // nop
		*(u32*)0x020B29F4 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020B2A0C, heapEnd);
		patchUserSettingsReadDSiWare(0x020B4270);
		*(u32*)0x020BABF0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BABF4 = 0xE12FFF1E; // bx lr
	}

	// Jump Trials (USA)
	// Does not work on real hardware
	/*else if (strcmp(romTid, "KJPE") == 0) {
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x0201BD6C = 0xE1A00000; // nop
		*(u32*)0x0201BD9C = 0xE1A00000; // nop
		*(u32*)0x0201BDBC = 0xE1A00000; // nop
		*(u32*)0x0201E5D0 = 0xE1A00000; // nop
		*(u32*)0x0201E5E0 = 0xE1A00000; // nop
		*(u32*)0x0201E61C = 0xE1A00000; // nop
		*(u32*)0x0201E88C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E890 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201EA40 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201EA44 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203AA5C = 0xE1A00000; // nop
		*(u32*)0x0203ED64 = 0xE1A00000; // nop
		*(u32*)0x02045AFC = 0xE1A00000; // nop
		*(u32*)0x02047938 = 0xE1A00000; // nop
		*(u32*)0x0204793C = 0xE1A00000; // nop
		*(u32*)0x02047948 = 0xE1A00000; // nop
		*(u32*)0x02047AA8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02047B04, heapEnd8MBHack);
		patchUserSettingsReadDSiWare(0x02048D88);
	}*/

	// Jump Trials Extreme (USA)
	// Does not work on real hardware
	/*else if (strcmp(romTid, "KZCE") == 0) {
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x0201EAD0 = 0xE1A00000; // nop
		*(u32*)0x0201EB00 = 0xE1A00000; // nop
		*(u32*)0x0201EB20 = 0xE1A00000; // nop
		*(u32*)0x02021334 = 0xE1A00000; // nop
		*(u32*)0x02021344 = 0xE1A00000; // nop
		*(u32*)0x02021380 = 0xE1A00000; // nop
		*(u32*)0x020215F0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020215F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020217A4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020217A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203D7C0 = 0xE1A00000; // nop
		*(u32*)0x02041AC8 = 0xE1A00000; // nop
		*(u32*)0x020488C0 = 0xE1A00000; // nop
		*(u32*)0x0204A6FC = 0xE1A00000; // nop
		*(u32*)0x0204A700 = 0xE1A00000; // nop
		*(u32*)0x0204A70C = 0xE1A00000; // nop
		*(u32*)0x0204A86C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204A8C8, heapEnd8MBHack);
		patchUserSettingsReadDSiWare(0x0204BB4C);
	}*/

	// Just SING! 80's (USA)
	// Just SING! 80's (Europe)
	else if (strcmp(romTid, "KJFE") == 0 || strcmp(romTid, "KJFP") == 0) {
		tonccpy((u32*)0x02070968, dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x02070974;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02070984;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveGetLengthT = 0x02070994;
		*(u16*)dsiSaveGetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetLengthT + 4), dsiSaveGetLength, 0xC);

		const u32 dsiSaveSeekT = 0x020709A4;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		tonccpy((u32*)0x02070A58, dsiSaveSetLength, 0xC);

		tonccpy((u32*)0x02070DC4, dsiSaveRead, 0xC); // dsiSaveReadAsync

		tonccpy((u32*)0x02070E90, dsiSaveWrite, 0xC); // dsiSaveWriteAsync

		doubleNopT(0x0200B7EA);
		doubleNopT(0x0200B9B8);
		*(u16*)0x0200B9C4 = 0x2000; // movs r0, #0
		*(u16*)0x0200B9C6 = nopT;
		*(u16*)0x0200B9D4 = 0x2000; // movs r0, #0
		*(u16*)0x0200B9D6 = nopT;
		doubleNopT(0x02013864);
		doubleNopT(0x0201C5B0);
		*(u32*)0x02021AC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02026460 = 0xE1A00000; // nop
		*(u16*)0x02026EAC = 0x2000; // movs r0, #0
		*(u16*)0x02026EAE = nopT;
		doubleNopT(0x0202BFA6);
		*(u32*)0x02037CAC = 0x4770; // bx lr (Skip NAND error checking)
		setBLThumb(0x02037D00, dsiSaveCloseT);
		setBLThumb(0x02037D3A, dsiSaveOpenT);
		*(u16*)0x02037D6A = 0x2001; // movs r0, #1 (dsiSaveGetArcSrc)
		*(u16*)0x02037D6C = nopT;
		setBLThumb(0x02037DA0, dsiSaveOpenT);
		*(u16*)0x02037DC8 = 0x2001; // movs r0, #1 (dsiSaveFlush)
		*(u16*)0x02037DCA = nopT;
		setBLThumb(0x02037DD8, dsiSaveGetLengthT);
		setBLThumb(0x02037E30, dsiSaveSeekT);
		setBLThumb(0x02037E7C, dsiSaveSeekT);
		*(u16*)0x020382B4 = nopT;
		*(u16*)0x02038354 = nopT;
		*(u16*)0x020410E4 = 0x2301; // movs r3, #1
		doubleNopT(0x0204C206);
		doubleNopT(0x0204C20C);
		*(u16*)0x0204C9E4 = 0x2000; // movs r0, #0
		*(u32*)0x0206F170 += 0x60000000; // bhi -> b
		*(u32*)0x0206F2FC = 0xE1A00000; // nop
		*(u32*)0x02072CB0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02079E50, heapEnd);
		*(u32*)0x0207A1DC = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0207B4F4);
		*(u32*)0x0207B8FC = 0xE1A00000; // nop
		*(u32*)0x0207B900 = 0xE1A00000; // nop
		*(u32*)0x0207B904 = 0xE1A00000; // nop
		*(u32*)0x0207B908 = 0xE1A00000; // nop
		*(u32*)0x020B06BC = 0xE1A00000; // nop
		*(u32*)0x020B06C0 = 0xE1A00000; // nop
	}

	// Just SING! Christmas Songs (USA)
	// Just SING! Christmas Songs (Europe)
	// Difficult to get working properly without a good debugger (Crashes after selecting a song on hardware, crashes before title screen on NO$GBA)
	/* else if (strcmp(romTid, "K4CE") == 0 || strcmp(romTid, "K4CP") == 0) {
		doubleNopT(0x02008A74);
		doubleNopT(0x02008F04);
		*(u16*)0x02008F12 = 0x2000; // movs r0, #0
		*(u16*)0x02008F14 = nopT;
		*(u16*)0x02008F22 = 0x2000; // movs r0, #0
		*(u16*)0x02008F24 = nopT;
		*(u16*)0x02008F32 = 0x2000; // movs r0, #0
		*(u16*)0x02008F34 = nopT;
		*(u16*)0x02008F42 = 0x2000; // movs r0, #0
		*(u16*)0x02008F44 = nopT;
		doubleNopT(0x02008F4C);
		doubleNopT(0x02008F52);
		doubleNopT(0x02008F58);
		doubleNopT(0x02008F64);
		*(u32*)0x02027A04 = 0xE3A00000; // mov r0, #0
		doubleNopT(0x0202AB80);
		doubleNopT(0x0202AB88);
		doubleNopT(0x0202AB92);
		doubleNopT(0x0202ABDA);
		*(u16*)0x0202AE3A += 0x1000; // bne -> b
		*(u16*)0x0202AE5E += 0x1000; // bne -> b
		doubleNopT(0x0202AEA0);
		doubleNopT(0x0202AEA8);
		*(u32*)0x0202AF54 += 0xD0000000; // bne -> b
		*(u32*)0x0202AF8C += 0xD0000000; // LDMNEFD -> LDMFD
		doubleNopT(0x02033D8E);
		setBL(0x020428B0, (u32)dsiSaveOpen);
		*(u32*)0x02042934 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x02042944, (u32)dsiSaveGetResultCode);
		setBL(0x0204299C, (u32)dsiSaveClose);
		setBL(0x020429BC, (u32)dsiSaveCreate);
		setBL(0x02042A3C, (u32)dsiSaveClose);
		setBL(0x02042A5C, (u32)dsiSaveOpen);
		setBL(0x02042ADC, (u32)dsiSaveClose);
		setBL(0x02042AF8, (u32)dsiSaveSetLength);
		setBL(0x02042B68, (u32)dsiSaveClose);
		*(u32*)0x02042B7C = 0xE3A00001; // mov r0, #1 (dsiSaveFlush)
		setBL(0x02042BDC, (u32)dsiSaveClose);
		setBL(0x02042BF0, (u32)dsiSaveGetLength);
		setBL(0x02042C54, (u32)dsiSaveSeek);
		setBL(0x02042CB8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02042D50, (u32)dsiSaveSeek);
		setBL(0x02042DB4, (u32)dsiSaveRead); // dsiSaveReadAsync
		*(u32*)0x0205063C = 0xE12FFF1E; // bx lr (Skip NAND error checking)
		*(u16*)0x0205096C = 0x2201; // movs r2, #1
		doubleNopT(0x020618EC);
		doubleNopT(0x020618F2);
		*(u32*)0x02092170 = 0xE1A00000; // nop
		*(u32*)0x02095968 = 0xE1A00000; // nop
		patchInitDSiWare(0x0209CC28, heapEnd);
		patchUserSettingsReadDSiWare(0x0209E2CC);
	} */

	// A Kappa's Trail (USA)
	// Requires 8MB of RAM
	// Crashes after ESRB screen
	/* else if (strcmp(romTid, "KPAE") == 0 && extendedMemory2) {
		*(u32*)0x020194A8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201A020, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201CFE4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02023EB0, heapEnd);
		*(u32*)0x0202423C = (*(u32*)0x02004FD0)+0x50000;
		patchUserSettingsReadDSiWare(0x020253D0);
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
		*(u32*)0x020321A4 = 0xE1A00000; // nop
		*(u32*)0x020321BC = 0xE12FFF1E; // bx lr
		*(u32*)0x020321F8 = 0xE12FFF1E; // bx lr
	} */

	// Katamukusho (Japan)
	// Crashes after title screen fades in
	/* else if (strcmp(romTid, "K69J") == 0) {
		*(u32*)0x02011138 = 0xE1A00000; // nop
		tonccpy((u32*)0x02011DCC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020147D8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CA60, heapEnd);
		*(u32*)0x0201CDD0 = *(u32*)0x02004FC0;
		patchUserSettingsReadDSiWare(0x0201DF58);
		*(u32*)0x0201E370 = 0x77777777;
		*(u32*)0x0202345C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02023460 = 0xE12FFF1E; // bx lr
		*(u32*)0x02051B08 = 0xE1A00000; // nop
		*(u32*)0x02051B58 = 0xE1A00000; // nop
		*(u32*)0x02051B64 = 0xE1A00000; // nop
		*(u32*)0x02051B98 = 0xE1A00000; // nop
		*(u32*)0x02053F8C = 0xE3A00000; // mov r0, #0
		setBL(0x0206AED8, (u32)dsiSaveOpen);
		setBL(0x0206AEF0, (u32)dsiSaveGetLength);
		setBL(0x0206AF18, (u32)dsiSaveRead);
		setBL(0x0206AF3C, (u32)dsiSaveClose);
		setBL(0x0206AF80, (u32)dsiSaveCreate);
		setBL(0x0206AF90, (u32)dsiSaveOpen);
		setBL(0x0206AFB0, (u32)dsiSaveSetLength);
		setBL(0x0206AFD4, (u32)dsiSaveWrite);
		setBL(0x0206AFEC, (u32)dsiSaveClose);
		*(u32*)0x0206B0E8 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0206B164 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		*(u32*)0x0206B514 = 0xE1A00000; // nop
		*(u32*)0x0206B718 = 0xE1A00000; // nop
	} */

	// Kazu De Asobu: Mahoujin To Imeji Kei-san (Japan)
	else if (strcmp(romTid, "K3HJ") == 0) {
		*(u32*)0x0200F4A8 = 0xE1A00000; // nop
		setBL(0x0200F6B0, (u32)dsiSaveClose);
		*(u32*)0x0200F714 = 0xE1A00000; // nop
		setBL(0x0200F7F4, (u32)dsiSaveClose);
		setBL(0x0200F994, (u32)dsiSaveOpen);
		setBL(0x0200F9BC, (u32)dsiSaveSeek);
		setBL(0x0200F9D8, (u32)dsiSaveClose);
		setBL(0x0200F9F0, (u32)dsiSaveRead);
		setBL(0x0200FA10, (u32)dsiSaveClose);
		setBL(0x0200FA20, (u32)dsiSaveClose);
		setBL(0x0200FA5C, (u32)dsiSaveOpen);
		setBL(0x0200FA74, (u32)dsiSaveSeek);
		setBL(0x0200FA8C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0200FAC0, (u32)dsiSaveOpen);
		setBL(0x0200FAE0, (u32)dsiSaveSetLength);
		setBL(0x0200FAF0, (u32)dsiSaveClose);
		setBL(0x0200FB0C, (u32)dsiSaveSeek);
		setBL(0x0200FB28, (u32)dsiSaveClose);
		setBL(0x0200FB40, (u32)dsiSaveWrite);
		setBL(0x0200FB64, (u32)dsiSaveClose);
		setBL(0x0200FB70, (u32)dsiSaveClose);
		setBL(0x0200FBB0, (u32)dsiSaveOpen);
		setBL(0x0200FBC4, (u32)dsiSaveSetLength);
		setBL(0x0200FBDC, (u32)dsiSaveSeek);
		setBL(0x0200FBF4, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0200FC4C, (u32)dsiSaveCreate);
		setBL(0x0200FC54, (u32)dsiSaveGetResultCode);
		*(u32*)0x020215D0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02021924 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x0202DA30 = 0xE1A00000; // nop
		*(u32*)0x0202DE78 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		*(u32*)0x02038CCC = 0xE1A00000; // nop
		patchInitDSiWare(0x0203EBAC, heapEnd);
		patchUserSettingsReadDSiWare(0x02040314);
	}

	// Sutjarang Nolja!: Maejikseukweeowa Imijigyesan (Korea)
	else if (strcmp(romTid, "K3HK") == 0) {
		*(u32*)0x0200C4E8 = 0xE1A00000; // nop
		*(u32*)0x0200FB14 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201577C, heapEnd);
		patchUserSettingsReadDSiWare(0x02016EDC);
		*(u32*)0x020237DC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02023B7C = 0xE1A00000; // nop
		setBL(0x02023D84, (u32)dsiSaveClose);
		*(u32*)0x02023DE8 = 0xE1A00000; // nop
		setBL(0x02023EC8, (u32)dsiSaveClose);
		setBL(0x02024068, (u32)dsiSaveOpen);
		setBL(0x02024090, (u32)dsiSaveSeek);
		setBL(0x020240AC, (u32)dsiSaveClose);
		setBL(0x020240C4, (u32)dsiSaveRead);
		setBL(0x020240E4, (u32)dsiSaveClose);
		setBL(0x020240F4, (u32)dsiSaveClose);
		setBL(0x02024130, (u32)dsiSaveOpen);
		setBL(0x02024148, (u32)dsiSaveSeek);
		setBL(0x02024160, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02024194, (u32)dsiSaveOpen);
		setBL(0x020241B4, (u32)dsiSaveSetLength);
		setBL(0x020241C4, (u32)dsiSaveClose);
		setBL(0x020241E0, (u32)dsiSaveSeek);
		setBL(0x020241FC, (u32)dsiSaveClose);
		setBL(0x02024214, (u32)dsiSaveWrite);
		setBL(0x02024238, (u32)dsiSaveClose);
		setBL(0x02024244, (u32)dsiSaveClose);
		setBL(0x02024284, (u32)dsiSaveOpen);
		setBL(0x02024298, (u32)dsiSaveSetLength);
		setBL(0x020242B0, (u32)dsiSaveSeek);
		setBL(0x020242C8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x02024320, (u32)dsiSaveCreate);
		setBL(0x02024328, (u32)dsiSaveGetResultCode);
		*(u32*)0x0202F7EC = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x0203CD38 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
	}

	// Keibadou Uma no Suke 2012 (Japan)
	else if (strcmp(romTid, "KUXJ") == 0) {
		*(u32*)0x02005098 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200512C = 0xE1A00000; // nop (Show white screen instead of manual screen)
		*(u32*)0x0200C634 = 0xE1A00000; // nop
		*(u32*)0x0200FBD0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016E7C, heapEnd);
		*(u32*)0x02017208 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02018494);
		setBL(0x0202CBF0, (u32)dsiSaveGetResultCode);
		*(u32*)0x0202CC80 = 0xE1A00000; // nop
		setBL(0x0202CC9C, (u32)dsiSaveOpen);
		setBL(0x0202CCD0, (u32)dsiSaveRead);
		setBL(0x0202CCF8, (u32)dsiSaveClose);
		*(u32*)0x0202CD1C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0202CD34 = 0xE1A00000; // nop
		setBL(0x0202CD64, (u32)dsiSaveCreate);
		*(u32*)0x0202CDC4 = 0xE1A00000; // nop
		setBL(0x0202CDF8, (u32)dsiSaveOpen);
		setBL(0x0202CE3C, (u32)dsiSaveWrite);
		setBL(0x0202CE5C, (u32)dsiSaveClose);
		*(u32*)0x0202CE84 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0202CE9C = 0xE1A00000; // nop
		*(u32*)0x0202CED0 = 0xE3A00000; // mov r0, #0
	}

	// Keisan 100 Renda (Japan)
	else if (strcmp(romTid, "K3DJ") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200B698 = 0xE1A00000; // nop
		*(u32*)0x0200EBE4 = 0xE1A00000; // nop
		patchInitDSiWare(0x020143A8, heapEnd);
		patchUserSettingsReadDSiWare(0x02015844);
		*(u32*)0x0201B6DC = 0xE3A00000; // mov r0, #0
		setBL(0x0201B768, (u32)dsiSaveOpen);
		setBL(0x0201B77C, (u32)dsiSaveGetLength);
		setBL(0x0201B7A0, (u32)dsiSaveRead);
		setBL(0x0201B7AC, (u32)dsiSaveClose);
		setBL(0x0201B868, (u32)dsiSaveCreate);
		setBL(0x0201B87C, (u32)dsiSaveOpen);
		setBL(0x0201B890, (u32)dsiSaveSetLength);
		setBL(0x0201B8A8, (u32)dsiSaveWrite);
		setBL(0x0201B8B4, (u32)dsiSaveClose);
		*(u32*)0x0201BC40 = 0xE1A00000; // nop
		/* if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0202EF60, 0x02016058);
		} */
	}

	// Kemonomix (Japan)
	else if (strcmp(romTid, "KMXJ") == 0) {
		const u32 openCodeCopy = 0x02004010;
		const u32 readCodeCopy = openCodeCopy+0x6C;
		const u32 closeCodeCopy = readCodeCopy+0x4C;
		const u32 getLengthCodeCopy = closeCodeCopy+0x60;

		*(u32*)0x0200548C = 0xE1A00000; // nop
		*(u32*)0x02005600 = 0xE1A00000; // nop
		*(u32*)0x020058A8 = 0xE1A00000; // nop
		codeCopy((u32*)openCodeCopy, (u32*)0x0200C0E8, 0x6C);
		codeCopy((u32*)readCodeCopy, (u32*)0x0200C1E4, 0x4C);
		codeCopy((u32*)closeCodeCopy, (u32*)0x0200C230, 0x60);
		codeCopy((u32*)getLengthCodeCopy, (u32*)0x0200C290, 0x18);
		setBL(0x0200C128, (u32)dsiSaveOpen);
		setBL(0x0200C168, (u32)dsiSaveCreate);
		setBL(0x0200C1A0, (u32)dsiSaveGetInfo);
		setBL(0x0200C1DC, (u32)dsiSaveWrite);
		setBL(0x0200C1FC, (u32)dsiSaveRead);
		setBL(0x0200C274, (u32)dsiSaveClose);
		*(u32*)0x0200C2A4 = (u32)dsiSaveGetLength;
		setBL(0x0200C2E8, getLengthCodeCopy);
		setBL(0x0200C30C, readCodeCopy);
		setBL(0x0200C31C, closeCodeCopy);
		setBL(0x0200C360, openCodeCopy);
		setBL(0x0200C380, getLengthCodeCopy);
		setBL(0x0200C3A4, readCodeCopy);
		setBL(0x0200C3B4, closeCodeCopy);
		*(u32*)0x0200E7F8 = 0xE1A00000; // nop
		*(u32*)0x0200E804 = 0xE1A00000; // nop
		*(u32*)0x0200E814 = 0xE1A00000; // nop
		*(u32*)0x02057A2C = 0xE1A00000; // nop
		tonccpy((u32*)0x020585A4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205B650 = 0xE1A00000; // nop
		patchInitDSiWare(0x02061C68, heapEnd);
		patchUserSettingsReadDSiWare(0x020633E8);
		doubleNopT(0x0206826E);
		doubleNopT(0x02068292);
		doubleNopT(0x020682AA);
		doubleNopT(0x020682CA);
		doubleNopT(0x020682E6);
		doubleNopT(0x02068306);
		doubleNopT(0x02068322);
		doubleNopT(0x0206834A);
	}

	// Kokoro no Herusumeta: Kokoron (Japan)
	// TWLFontTable.dat required
	else if (strcmp(romTid, "K9CJ") == 0 && twlFontFound) {
		useSharedFont = twlFontFound;
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x02005E68 = 0xE1A00000; // nop
		*(u32*)0x02005FB4 = 0xE1A00000; // nop
		setBL(0x0200F820, (u32)dsiSaveOpen);
		setBL(0x0200F840, (u32)dsiSaveGetLength);
		setBL(0x0200F858, (u32)dsiSaveRead);
		setBL(0x0200F868, (u32)dsiSaveRead);
		setBL(0x0200F8B0, (u32)dsiSaveClose);
		setBL(0x0200F8C8, (u32)dsiSaveClose);
		setBL(0x0200F910, (u32)dsiSaveOpen);
		setBL(0x0200F938, (u32)dsiSaveSetLength);
		setBL(0x0200F980, (u32)dsiSaveWrite);
		setBL(0x0200F990, (u32)dsiSaveWrite);
		setBL(0x0200F998, (u32)dsiSaveClose);
		setBL(0x0200F9C0, (u32)dsiSaveCreate);
		*(u32*)0x02026F84 = 0xE1A00000; // nop
		*(u32*)0x0202A4E8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202F2B4, heapEnd);
		patchUserSettingsReadDSiWare(0x02030754);
	}

	// Koneko no ie: Kiri Shima Keto-San Biki no Koneko (Japan)
	else if (strcmp(romTid, "KONJ") == 0) {
		setBL(0x02005F00, (u32)dsiSaveOpen);
		setBL(0x02005F1C, (u32)dsiSaveCreate);
		*(u32*)0x02005F6C = 0xE3A00001; // mov r0, #1
		setBL(0x02005F94, (u32)dsiSaveCreate);
		setBL(0x02005FB0, (u32)dsiSaveOpen);
		setBL(0x02005FFC, (u32)dsiSaveWrite);
		setBL(0x0200600C, (u32)dsiSaveClose);
		setBL(0x02006068, (u32)dsiSaveOpen);
		setBL(0x020060BC, (u32)dsiSaveSeek);
		setBL(0x020060CC, (u32)dsiSaveRead);
		setBL(0x020060DC, (u32)dsiSaveClose);
		setBL(0x02006140, (u32)dsiSaveOpen);
		setBL(0x02006194, (u32)dsiSaveRead);
		setBL(0x020061C4, (u32)dsiSaveClose);
		setBL(0x020061E0, (u32)dsiSaveSeek);
		setBL(0x020061F0, (u32)dsiSaveWrite);
		setBL(0x02006220, (u32)dsiSaveSeek);
		setBL(0x02006230, (u32)dsiSaveWrite);
		setBL(0x0200624C, (u32)dsiSaveClose);
		*(u32*)0x0202AC54 = 0xE1A00000; // nop
		tonccpy((u32*)0x0202B8E8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0202E834 = 0xE1A00000; // nop
		patchInitDSiWare(0x020353AC, heapEnd);
	}

	// Koukou Eijukugo: Kiho 200 Go Master (Japan)
	else if (strcmp(romTid, "KJKJ") == 0) {
		*(u32*)0x0201686C = 0xE1A00000; // nop
		*(u32*)0x02019F30 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FEDC, heapEnd);
		patchUserSettingsReadDSiWare(0x02021644);
		setBL(0x02030520, (u32)dsiSaveCreate);
		setBL(0x02030BB4, (u32)dsiSaveClose);
		*(u32*)0x02052D14 = 0xE1A00000; // nop
		*(u32*)0x02052D38 = 0xE1A00000; // nop
		*(u32*)0x02067EB0 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x0206808C = 0xE3A00000; // mov r0, #0
		*(u32*)0x020683EC = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x0207A8DC, (u32)dsiSaveOpen);
		setBL(0x0207A8F8, (u32)dsiSaveSeek);
		setBL(0x0207A90C, (u32)dsiSaveClose);
		setBL(0x0207A924, (u32)dsiSaveRead);
		setBL(0x0207A934, (u32)dsiSaveClose);
		setBL(0x0207A940, (u32)dsiSaveClose);
		setBL(0x0207A974, (u32)dsiSaveOpen);
		setBL(0x0207A98C, (u32)dsiSaveSeek);
		setBL(0x0207A9A4, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0207A9D4, (u32)dsiSaveOpen);
		setBL(0x0207A9EC, (u32)dsiSaveSetLength);
		setBL(0x0207A9FC, (u32)dsiSaveClose);
		setBL(0x0207AA10, (u32)dsiSaveSeek);
		setBL(0x0207AA24, (u32)dsiSaveClose);
		setBL(0x0207AA3C, (u32)dsiSaveWrite);
		setBL(0x0207AA4C, (u32)dsiSaveClose);
		setBL(0x0207AA58, (u32)dsiSaveClose);
		setBL(0x0207AA8C, (u32)dsiSaveOpen);
		setBL(0x0207AAA0, (u32)dsiSaveSetLength);
		setBL(0x0207AAB8, (u32)dsiSaveSeek);
		setBL(0x0207AAD0, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x0207AC30 = 0xE12FFF1E; // bx lr
	}

	// Kuizu Ongaku Nojika (Japan)
	/* else if (strcmp(romTid, "KVFJ") == 0) {
		// useSharedFont = twlFontFound;
		*(u32*)0x02012BA0 = 0xE1A00000; // nop
		*(u32*)0x0201617C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CB3C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E0F8);
		*(u32*)0x0201E114 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E118 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E120 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E124 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E144 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E148 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E158 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E15C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E168 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E16C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E17C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E180 = 0xE12FFF1E; // bx lr
		*(u32*)0x02069118 = 0xE1A00000; // nop
		*(u32*)0x0206913C = 0xE1A00000; // nop
		*(u32*)0x02069144 = 0xE1A00000; // nop
		*(u32*)0x02069158 = 0xE1A00000; // nop
		*(u32*)0x0206916C = 0xE1A00000; // nop
		*(u32*)0x02069174 = 0xE1A00000; // nop
		*(u32*)0x0206917C = 0xE1A00000; // nop
		*(u32*)0x02069194 = 0xE1A00000; // nop
		*(u32*)0x020691BC = 0xE1A00000; // nop
		*(u32*)0x020691E4 = 0xE1A00000; // nop
		*(u32*)0x020691EC = 0xE1A00000; // nop
		setBL(0x020691F4, (u32)dsiSaveWrite);
		*(u32*)0x020691FC = 0xE1A00000; // nop
		*(u32*)0x02069204 = 0xE1A00000; // nop
		*(u32*)0x02069214 = 0xE1A00000; // nop
		setBL(0x0206924C, (u32)dsiSaveRead);
		setBL(0x020693C4, (u32)dsiSaveGetInfo);
		setBL(0x02069434, (u32)dsiSaveGetInfo);
		*(u32*)0x02069540 = 0xE1A00000; // nop (dsiSaveCreateDir)
		*(u32*)0x0206954C = 0xE3A0000B; // mov r0, #0xB (Result code of dsiSaveCreateDir)
		setBL(0x0206959C, (u32)dsiSaveCreate);
		setBL(0x020695CC, (u32)dsiSaveOpen);
		setBL(0x020695F0, (u32)dsiSaveSetLength);
		setBL(0x02069610, (u32)dsiSaveClose);
		setBL(0x02069870, (u32)dsiSaveOpen);
		setBL(0x02069920, (u32)dsiSaveClose);
		setBL(0x02069AB8, (u32)dsiSaveOpen);
		setBL(0x02069B84, (u32)dsiSaveClose);
		setBL(0x02069C48, (u32)dsiSaveGetResultCode);
		*(u32*)0x0209276C = 0xE3A00001; // mov r0, #1
	} */

	// Kung Fu Dragon (USA)
	// Kung Fu Dragon (Europe)
	else if (strcmp(romTid, "KT9E") == 0 || strcmp(romTid, "KT9P") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x020051D4 = 0xE1A00000; // nop
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201F3E4, 0x02018F3C);
			}
		} else {
			*(u32*)0x02005310 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x0201D8EC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200E8F4 = 0xE1A00000; // nop
		*(u32*)0x02011D90 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201751C, heapEnd);
		patchUserSettingsReadDSiWare(0x020189BC);
		setBL(0x0201DA28, (u32)dsiSaveOpen);
		setBL(0x0201DA50, (u32)dsiSaveRead);
		setBL(0x0201DA60, (u32)dsiSaveRead);
		setBL(0x0201DA68, (u32)dsiSaveClose);
		setBL(0x0201DD24, (u32)dsiSaveOpen);
		setBL(0x0201DE6C, (u32)dsiSaveWrite);
		setBL(0x0201DE74, (u32)dsiSaveClose);
		*(u32*)0x02024994 = 0xE1A00000; // nop
		setBL(0x020249A0, (u32)dsiSaveCreate);
		*(u32*)0x02024A4C = 0xE1A00000; // nop
		setBL(0x02024A58, (u32)dsiSaveCreate);
	}

	// Akushon Gemu: Tobeyo!! Dorago! (Japan)
	else if (strcmp(romTid, "KT9J") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x020051D8 = 0xE1A00000; // nop
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201F3B8, 0x02018F10);
			}
		} else {
			*(u32*)0x020052F0 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x0201D8C0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200E8C8 = 0xE1A00000; // nop
		*(u32*)0x02011D64 = 0xE1A00000; // nop
		patchInitDSiWare(0x020174F0, heapEnd);
		patchUserSettingsReadDSiWare(0x02018990);
		setBL(0x0201D9FC, (u32)dsiSaveOpen);
		setBL(0x0201DA24, (u32)dsiSaveRead);
		setBL(0x0201DA34, (u32)dsiSaveRead);
		setBL(0x0201DA3C, (u32)dsiSaveClose);
		setBL(0x0201DCF8, (u32)dsiSaveOpen);
		setBL(0x0201DE40, (u32)dsiSaveWrite);
		setBL(0x0201DE48, (u32)dsiSaveClose);
		*(u32*)0x020248BC = 0xE1A00000; // nop
		setBL(0x020248C8, (u32)dsiSaveCreate);
		*(u32*)0x02024974 = 0xE1A00000; // nop
		setBL(0x02024980, (u32)dsiSaveCreate);
	}

	// Kyara Pasha!: Hello Kitty (Japan)
	// Shows Japanese error screen (Possibly related to camera)
	/*else if (strcmp(romTid, "KYKJ") == 0) {
		*(u32*)0x020051E4 -= 4;
		*(u32*)0x020051EC = 0xE1A00000; // nop
		*(u32*)0x020052CC = 0xE1A00000; // nop
		*(u32*)0x0201EA20 = 0xE1A00000; // nop
		*(u32*)0x0201EA24 = 0xE1A00000; // nop
		*(u32*)0x0201EA3C = 0xE8BD80F8; // LDMFD SP!, {R3-R7,PC}
		*(u32*)0x0203F004 = 0xE1A00000; // nop
		*(u32*)0x02046130 = 0xE1A00000; // nop
		*(u32*)0x0204B7C4 = 0xE1A00000; // nop
		*(u32*)0x0204DE30 = 0xE1A00000; // nop
		*(u32*)0x0204DE34 = 0xE1A00000; // nop
		*(u32*)0x0204DE40 = 0xE1A00000; // nop
		*(u32*)0x0204DFA0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0204DFFC, heapEnd); // mov r0, #0x23E0000
		patchUserSettingsReadDSiWare(0x0204F350);
	}*/

	// Kyou Hanan no hi Hyakka: Hyakkajiten Maipedea Yori (Japan)
	// Crashes at unknown location
	/*else if (strcmp(romTid, "K47J") == 0 && extendedMemory2) {
		*(u32*)0x0200E240 = 0xE1A00000; // nop
		*(u32*)0x02011720 = 0xE1A00000; // nop
		*(u32*)0x02015FD4 = 0xE1A00000; // nop
		*(u32*)0x02017DC4 = 0xE1A00000; // nop
		*(u32*)0x02017DC8 = 0xE1A00000; // nop
		*(u32*)0x02017DD4 = 0xE1A00000; // nop
		*(u32*)0x02017F18 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02017F74, heapEndExceed);
		patchUserSettingsReadDSiWare(0x02019214);
		*(u32*)0x02020C78 = 0xE1A00000; // nop
		*(u32*)0x02020C90 = 0xE1A00000; // nop
		setBL(0x02020CD4, (u32)dsiSaveOpen);
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
		*(u32*)0x02020EE4 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x020291AC = 0xE1A00000; // nop
		*(u32*)0x0202F3F0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}*/

	// The Legend of Zelda: Four Swords: Anniversary Edition (USA)
	// The Legend of Zelda: Four Swords: Anniversary Edition (Europe, Australia)
	// Zelda no Densetsu: 4-tsu no Tsurugi: 25th Kinen Edition (Japan)
	// Requires either 8MB of RAM or Memory Expansion Pak
	// Audio is disabled on retail consoles
	else if (strncmp(romTid, "KQ9", 3) == 0 && debugOrMep) {
		extern u32* fourSwHeapAlloc;
		extern u32* fourSwHeapAddrPtr;
		//u32* getLengthFunc = (u32*)0;

		*(u32*)0x020051CC = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F860, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02012AAC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201853C, heapEnd);
		*(u32*)0x020188C8 = *(u32*)0x02004FF4;
		patchUserSettingsReadDSiWare(0x0201994C);
		*(u32*)0x02019968 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201996C = 0xE12FFF1E; // bx lr
		*(u32*)0x02019974 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02019978 = 0xE12FFF1E; // bx lr
		if (!extendedMemory2) {
			if (s2FlashcardId == 0x5A45) {
				for (int i = 0; i < 4; i++) {
					fourSwHeapAddrPtr[i] -= 0x01000000;
				}
			}
			tonccpy((u32*)0x02019FA4, fourSwHeapAlloc, 0xC0);
		}
		*(u32*)0x02021F40 = 0xE1A00000; // nop (Fix glitched stage graphics)
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
		if (romTid[3] == 'E') {
			*(u32*)0x02082A58 = 0xE1A00000; // nop
			if (!extendedMemory2) {
				//getLengthFunc = getOffsetFromBL((u32*)0x0208C7B0);
				//setBL(0x0208C7B0, 0x02010300);

				*(u32*)0x0208CDC0 = 0xE3A00000; // mov r0, #0 (Skip .wave file loading)
				*(u32*)0x0208CDC4 = 0xE1A00000; // nop
				*(u32*)0x0208CDCC = 0xE1A00000; // nop
				*(u32*)0x0208CDD8 = 0xE1A00000; // nop
				*(u32*)0x0208CDE0 = 0xE1A00000; // nop
				*(u32*)0x0208CDEC = 0xE1A00000; // nop
				*(u32*)0x0208CDFC = 0xE3A00000; // mov r0, #0
				*(u32*)0x02019FA0 = (u32)getOffsetFromBL((u32*)0x0208D9D8);
				setBL(0x0208D9D8, 0x02019FA4);

				// Disable sound
				*(u32*)0x0208CF38 = 0xE1A00000; // nop
				*(u32*)0x0208D038 = 0xE1A00000; // nop
				*(u32*)0x020C04EC = 0xE12FFF1E; // bx lr
				*(u32*)0x020C05CC = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C05D0 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0630 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0634 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0694 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0698 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C06F8 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C06FC = 0xE12FFF1E; // bx lr
				*(u32*)0x020C075C = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0760 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C07C0 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C07C4 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0824 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0828 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C26DC = 0xE12FFF1E; // bx lr
			}
			*(u32*)0x020A44F4 = 0xE1A00000; // nop
			*(u32*)0x020A44F8 = 0xE1A00000; // nop
			*(u32*)0x020A44FC = 0xE1A00000; // nop
			*(u32*)0x020A467C = 0xE1A00000; // nop
			*(u32*)0x020C7CC4 = 0xE3A00000; // mov r0, #0
		} else if (romTid[3] == 'V') {
			*(u32*)0x02082A78 = 0xE1A00000; // nop
			if (!extendedMemory2) {
				//getLengthFunc = getOffsetFromBL((u32*)0x0208C7D0);
				//setBL(0x0208C7D0, 0x02010300);

				*(u32*)0x0208CDE0 = 0xE3A00000; // mov r0, #0 (Skip .wave file loading)
				*(u32*)0x0208CDE4 = 0xE1A00000; // nop
				*(u32*)0x0208CDEC = 0xE1A00000; // nop
				*(u32*)0x0208CDF8 = 0xE1A00000; // nop
				*(u32*)0x0208CE00 = 0xE1A00000; // nop
				*(u32*)0x0208CE0C = 0xE1A00000; // nop
				*(u32*)0x0208CE1C = 0xE3A00000; // mov r0, #0
				*(u32*)0x02019FA0 = (u32)getOffsetFromBL((u32*)0x0208D9F8);
				setBL(0x0208D9F8, 0x02019FA4);

				// Disable sound
				*(u32*)0x0208CF58 = 0xE1A00000; // nop
				*(u32*)0x0208D058 = 0xE1A00000; // nop
				*(u32*)0x020C050C = 0xE12FFF1E; // bx lr
				*(u32*)0x020C05EC = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C05F0 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0650 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0654 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C06B4 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C06B8 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0718 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C071C = 0xE12FFF1E; // bx lr
				*(u32*)0x020C077C = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0780 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C07E0 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C07E4 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0844 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0848 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C26FC = 0xE12FFF1E; // bx lr
			}
			*(u32*)0x020A4514 = 0xE1A00000; // nop
			*(u32*)0x020A4518 = 0xE1A00000; // nop
			*(u32*)0x020A451C = 0xE1A00000; // nop
			*(u32*)0x020A469C = 0xE1A00000; // nop
			*(u32*)0x020C7CE4 = 0xE3A00000; // mov r0, #0
		} else {
			*(u32*)0x02082A14 = 0xE1A00000; // nop
			if (!extendedMemory2) {
				//getLengthFunc = getOffsetFromBL((u32*)0x0208C76C);
				//setBL(0x0208C76C, 0x02010300);

				*(u32*)0x0208CD7C = 0xE3A00000; // mov r0, #0 (Skip .wave file loading)
				*(u32*)0x0208CD80 = 0xE1A00000; // nop
				*(u32*)0x0208CD88 = 0xE1A00000; // nop
				*(u32*)0x0208CD94 = 0xE1A00000; // nop
				*(u32*)0x0208CD9C = 0xE1A00000; // nop
				*(u32*)0x0208CDA8 = 0xE1A00000; // nop
				*(u32*)0x0208CDB8 = 0xE3A00000; // mov r0, #0
				*(u32*)0x02019FA0 = (u32)getOffsetFromBL((u32*)0x0208D994);
				setBL(0x0208D994, 0x02019FA4);

				// Disable sound
				*(u32*)0x0208CEF4 = 0xE1A00000; // nop
				*(u32*)0x0208CFF4 = 0xE1A00000; // nop
				*(u32*)0x020C0528 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0608 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C060C = 0xE12FFF1E; // bx lr
				*(u32*)0x020C066C = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0670 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C06D0 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C06D4 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0734 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0738 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0798 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C079C = 0xE12FFF1E; // bx lr
				*(u32*)0x020C07FC = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0800 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C0860 = 0xE3A00000; // mov r0, #0
				*(u32*)0x020C0864 = 0xE12FFF1E; // bx lr
				*(u32*)0x020C2718 = 0xE12FFF1E; // bx lr
			}
			*(u32*)0x020A44B0 = 0xE1A00000; // nop
			*(u32*)0x020A44B4 = 0xE1A00000; // nop
			*(u32*)0x020A44B8 = 0xE1A00000; // nop
			*(u32*)0x020A4638 = 0xE1A00000; // nop
			*(u32*)0x020C7D00 = 0xE3A00000; // mov r0, #0
		}

		/*if (!extendedMemory2) {
			*(u32*)0x02010300 = 0xE92D4038; // stmfd sp!, {r3-r5,lr}
			setBL(0x02010304, (u32)getLengthFunc);
			*(u32*)0x02010308 = 0xE59F3008; // ldr r3, =0x180140 (.sdat filesize)
			*(u32*)0x0201030C = 0xE1530000; // cmp r3, r0
			*(u32*)0x02010310 = 0x059F0004; // ldreq r0, =0xE03E0 (.sdat filesize without music samples)
			*(u32*)0x02010314 = 0xE8BD8038; // ldmfd sp!, {r3-r5,pc}
			*(u32*)0x02010318 = 0x180140;
			*(u32*)0x0201031C = 0xE03E0;
		}*/
	}

	// Legendary Wars: T-Rex Rumble (USA)
	// Legendary Wars: T-Rex Rumble (Europe, Australia)
	else if (strcmp(romTid, "KLDE") == 0 || strcmp(romTid, "KLDV") == 0) {
		*(u32*)0x0200518C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02005D64 = 0x4000; // Shrink unknown heap from 0x177000
		}
		*(u32*)0x0201B6C8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201C24C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201F188 = 0xE1A00000; // nop
		patchInitDSiWare(0x020269E0, heapEnd);
		*(u32*)0x02026D6C -= 0x30000;
		patchUserSettingsReadDSiWare(0x02028098);
		if (romTid[3] == 'E') {
			setBL(0x0205DB2C, (u32)dsiSaveOpen);
			setBL(0x0205DBA8, (u32)dsiSaveCreate);
			setBL(0x0205DBE4, (u32)dsiSaveGetLength);
			setBL(0x0205DBF8, (u32)dsiSaveRead);
			setBL(0x0205DC04, (u32)dsiSaveClose);
			setBL(0x0205DC74, (u32)dsiSaveOpen);
			setBL(0x0205DCF4, (u32)dsiSaveCreate);
			setBL(0x0205DD28, (u32)dsiSaveOpen);
			setBL(0x0205DD60, (u32)dsiSaveSetLength);
			setBL(0x0205DD70, (u32)dsiSaveWrite);
			setBL(0x0205DD7C, (u32)dsiSaveClose);
			*(u32*)0x0205DDB0 = 0xE3A0000B; // mov r0, #0xB
		} else {
			setBL(0x0205DB90, (u32)dsiSaveOpen);
			setBL(0x0205DC0C, (u32)dsiSaveCreate);
			setBL(0x0205DC48, (u32)dsiSaveGetLength);
			setBL(0x0205DC5C, (u32)dsiSaveRead);
			setBL(0x0205DC68, (u32)dsiSaveClose);
			setBL(0x0205DCD8, (u32)dsiSaveOpen);
			setBL(0x0205DD58, (u32)dsiSaveCreate);
			setBL(0x0205DD8C, (u32)dsiSaveOpen);
			setBL(0x0205DDC4, (u32)dsiSaveSetLength);
			setBL(0x0205DDD4, (u32)dsiSaveWrite);
			setBL(0x0205DDE0, (u32)dsiSaveClose);
			*(u32*)0x0205DE14 = 0xE3A0000B; // mov r0, #0xB
		}
	}

	// ARC Style: Jurassic War (Japan)
	else if (strcmp(romTid, "KLDJ") == 0) {
		*(u32*)0x0200518C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02005CA8 = 0x4000; // Shrink unknown heap from 0x177000
		}
		*(u32*)0x0201B61C = 0xE1A00000; // nop
		tonccpy((u32*)0x0201C1A0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201F0DC = 0xE1A00000; // nop
		patchInitDSiWare(0x02026934, heapEnd);
		*(u32*)0x02026CC0 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02027FEC);
		setBL(0x0205DCCC, (u32)dsiSaveOpen);
		setBL(0x0205DD48, (u32)dsiSaveCreate);
		setBL(0x0205DD84, (u32)dsiSaveGetLength);
		setBL(0x0205DD98, (u32)dsiSaveRead);
		setBL(0x0205DDA4, (u32)dsiSaveClose);
		setBL(0x0205DE14, (u32)dsiSaveOpen);
		setBL(0x0205DE94, (u32)dsiSaveCreate);
		setBL(0x0205DEC8, (u32)dsiSaveOpen);
		setBL(0x0205DF00, (u32)dsiSaveSetLength);
		setBL(0x0205DF10, (u32)dsiSaveWrite);
		setBL(0x0205DF1C, (u32)dsiSaveClose);
		*(u32*)0x0205DF50 = 0xE3A0000B; // mov r0, #0xB
	}

	// Legends of Exidia (USA)
	// Requires more than 8MB of RAM
	/*else if (strcmp(romTid, "KLEE") == 0) {
		*(u32*)0x020050F4 = 0xE1A00000; // nop
		*(u32*)0x0201473C = 0xE1A00000; // nop
		*(u32*)0x02017E2C = 0xE1A00000; // nop
		*(u32*)0x0201C794 = 0xE1A00000; // nop
		*(u32*)0x0201E624 = 0xE1A00000; // nop
		*(u32*)0x0201E628 = 0xE1A00000; // nop
		*(u32*)0x0201E634 = 0xE1A00000; // nop
		*(u32*)0x0201E794 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201E7F0, heapEnd); // mov r0, #0x2700000
		patchUserSettingsReadDSiWare(0x0201FDA8);
		*(u32*)0x02026E94 = 0xE1A00000; // nop
	}*/

	// Letter Challenge (USA)
	else if (strcmp(romTid, "K5CE") == 0) {
		*(u32*)0x02017458 = 0xE1A00000; // nop
		*(u32*)0x0201BB14 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020F18, heapEnd);
		*(u32*)0x020212A4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02022610);
		setBL(0x020274EC, (u32)dsiSaveOpen);
		*(u32*)0x02027528 = 0xE1A00000; // nop
		setBL(0x02027540, (u32)dsiSaveGetLength);
		setBL(0x02027550, (u32)dsiSaveRead);
		setBL(0x02027558, (u32)dsiSaveClose);
		setBL(0x020275A8, (u32)dsiSaveOpen);
		*(u32*)0x020275C0 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x020275D0 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x020275EC, (u32)dsiSaveCreate);
		setBL(0x02027600, (u32)dsiSaveOpen);
		setBL(0x02027610, (u32)dsiSaveGetResultCode);
		setBL(0x0202764C, (u32)dsiSaveSetLength);
		setBL(0x0202765C, (u32)dsiSaveWrite);
		setBL(0x02027664, (u32)dsiSaveClose);
		*(u16*)0x0204E788 = 0x03BF; // lsls r7, r7, #0xE
		*(u16*)0x0204E796 = 0x2602; // movs r6, #2
	}

	// Libera Wing (Europe)
	// Black screens
	/*else if (strcmp(romTid, "KLWP") == 0) {
		*(u32*)0x02016138 = 0xE1A00000; // nop
		*(u32*)0x020195A8 = 0xE1A00000; // nop
		*(u32*)0x0201ED70 = 0xE1A00000; // nop
		*(u32*)0x02020B90 = 0xE1A00000; // nop
		*(u32*)0x02020B94 = 0xE1A00000; // nop
		*(u32*)0x02020BA0 = 0xE1A00000; // nop
		*(u32*)0x02020D00 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02020D5C, heapEndExceed);
		*(u32*)0x02020E90 = 0x020D7F40;
		patchUserSettingsReadDSiWare(0x020221CC);
		*(u32*)0x02044668 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204585C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02045860 = 0xE12FFF1E; // bx lr
		*(u32*)0x020563F8 = 0xE1A00000; // nop
	}*/

	// Link 'n' Launch (USA)
	// Link 'n' Launch (Europe, Australia)
	else if (strcmp(romTid, "KPTE") == 0 || strcmp(romTid, "KPTV") == 0) {
		useSharedFont = (romTid[3] == 'E') ? twlFontFound : (twlFontFound && debugOrMep);
		u8 offsetChange1 = (romTid[3] == 'E') ? 0 : 0xC0;
		u8 offsetChange2 = (romTid[3] == 'E') ? 0 : 0x8;
		u8 offsetChange3 = (romTid[3] == 'E') ? 0 : 0x40;
		u16 offsetChange4 = (romTid[3] == 'E') ? 0 : 0x36C;
		u16 offsetChange5 = (romTid[3] == 'E') ? 0 : 0x3AC;
		u16 offsetChange6 = (romTid[3] == 'E') ? 0 : 0x348;

		setBL(0x02005700, (u32)dsiSaveOpen);
		setBL(0x02005798, (u32)dsiSaveClose);
		setBL(0x02005828, (u32)dsiSaveRead);
		setBL(0x020058AC, (u32)dsiSaveWrite);
		setBL(0x020058E0, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02005934, (u32)dsiSaveDelete);
		setBL(0x02005990, (u32)dsiSaveSetLength);
		*(u32*)0x020059C4 = 0xE1A00000; // nop
		*(u32*)0x020137F8 = 0xE1A00000; // nop
		*(u32*)0x02013828 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)(0x02014770+offsetChange1) = 0xE3A03601; // mov r3, #0x100000 (Shrink sound heap from 0x280000: Disables music)
		}
		if (useSharedFont) {
			if (!extendedMemory2 && romTid[3] == 'V') {
				patchTwlFontLoad(0x0201778C+offsetChange2, 0x02070EC4-offsetChange6);
			}
		} else {
			*(u32*)(0x02017674+offsetChange2) = 0xE3A00000; // mov r0, #0
			*(u32*)(0x02017764+offsetChange2) = 0xE3A00000; // mov r0, #0
			*(u32*)(0x020177A8+offsetChange2) = 0xE3A00000; // mov r0, #0
			*(u32*)(0x02019384+offsetChange3) = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)(0x0203E068+offsetChange4) = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		*(u32*)(0x02051324+offsetChange5) = 0xE12FFF1E; // bx lr (Soft-lock instead of displaying Manual screen)
		*(u32*)(0x020663C8-offsetChange6) = 0xE1A00000; // nop
		tonccpy((u32*)(0x02066F94-offsetChange6), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0206A004-offsetChange6) = 0xE1A00000; // nop
		patchInitDSiWare(0x0206F2E8-offsetChange6, heapEnd);
		*(u32*)(0x0206F674-offsetChange6) -= 0x30000;
		patchUserSettingsReadDSiWare(0x02070930-offsetChange6);
	}

	// Panel Renketsu: 3-Fun Rocket (Japan)
	else if (strcmp(romTid, "KPTJ") == 0) {
		useSharedFont = twlFontFound;
		setBL(0x0200553C, (u32)dsiSaveOpen);
		setBL(0x020055D4, (u32)dsiSaveClose);
		setBL(0x02005664, (u32)dsiSaveRead);
		setBL(0x020056E8, (u32)dsiSaveWrite);
		setBL(0x0200571C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02005770, (u32)dsiSaveDelete);
		setBL(0x020057CC, (u32)dsiSaveSetLength);
		*(u32*)0x02005800 = 0xE1A00000; // nop
		*(u32*)0x02013610 = 0xE1A00000; // nop
		*(u32*)0x02013640 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02014568 = 0xE3A03601; // mov r3, #0x100000 (Shrink sound heap from 0x280000: Disables music)
		}
		if (!useSharedFont) {
			*(u32*)0x02017370 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02017460 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020174A4 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02019064 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
			*(u32*)0x0203DAB0 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		}
		*(u32*)0x02050CC4 = 0xE12FFF1E; // bx lr (Soft-lock instead of displaying Manual screen)
		*(u32*)0x02065A9C = 0xE1A00000; // nop
		tonccpy((u32*)0x02066F94, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02069814 = 0xE1A00000; // nop
		*(u32*)0x0206EDD4 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x0206EDEC, heapEnd);
		*(u32*)0x0206F15C -= 0x30000;
		patchUserSettingsReadDSiWare(0x0207043C);
	}

	// Little Red Riding Hood's Zombie BBQ (USA)
	else if (strcmp(romTid, "KZBE") == 0 && extendedMemory2) {
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x02005124 = 0xE1A00000; // nop
		*(u32*)0x02005194 = 0xE1A00000; // nop
		*(u32*)0x02005198 = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		*(u32*)0x0200BFE0 = 0xE1A00000; // nop
		*(u32*)0x0200FDB4 = 0xE1A00000; // nop
		patchInitDSiWare(0x020190DC, heapEnd);
		*(u32*)0x0201F65C = 0xE1A00000; // nop
		*(u32*)0x0202178C = 0xE1A00000; // nop
		*(u32*)0x02021790 = 0xE1A00000; // nop
		*(u32*)0x02021794 = 0xE1A00000; // nop
		*(u32*)0x02026224 = 0xE1A00000; // nop
		*(u32*)0x02026BFC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02026C18 = 0xE1A00000; // nop
		*(u32*)0x02026CB0 = 0xE1A00000; // nop
		*(u32*)0x02026CB4 = 0xE1A00000; // nop
		*(u32*)0x02026CB8 = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KZBP") == 0 && extendedMemory2) {
		*(u32*)0x02017550 = 0xE1A00000; // nop
		*(u32*)0x0201B324 = 0xE1A00000; // nop
		patchInitDSiWare(0x02024640, heapEnd);
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
	else if (strcmp(romTid, "KLPE") == 0 || strcmp(romTid, "KLPV") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200506C = 0xE1A00000; // nop
		*(u32*)0x02005088 = 0xE1A00000; // nop
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0205F024, 0x02022C58);
			}
		} else {
			*(u32*)0x0200509C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			// Skip Manual screen
			for (int i = 0; i < 11; i++) {
				u32* offset = (u32*)0x020055BC;
				offset[i] = 0xE1A00000; // nop
			}
		}
		*(u32*)0x02014D4C = 0xE1A00000; // nop
		tonccpy((u32*)0x020159F0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02018354 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020724, heapEnd);
		patchUserSettingsReadDSiWare(0x02021C1C);
		setBL(0x0205DE18, (u32)dsiSaveOpen);
		setBL(0x0205DE70, (u32)dsiSaveRead);
		setBL(0x0205DF24, (u32)dsiSaveCreate);
		setBL(0x0205DF34, (u32)dsiSaveOpen);
		setBL(0x0205DF7C, (u32)dsiSaveSetLength);
		setBL(0x0205DF8C, (u32)dsiSaveWrite);
		setBL(0x0205DF94, (u32)dsiSaveClose);
	}

	// Little Twin Stars (Japan)
	// Locks up(?) after confirming age and left/right hand
	/*else if (strcmp(romTid, "KQ3J") == 0) {
		*(u32*)0x020050DC = 0xE1A00000; // nop
		*(u32*)0x020050F0 = 0xE1A00000; // nop
		*(u32*)0x02005200 = 0xE1A00000; // nop
		*(u32*)0x0200523C = 0xE1A00000; // nop
		*(u32*)0x020162C4 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x0203DEF4 = 0xE1A00000; // nop
		*(u32*)0x020417D0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02047F08, heapEnd);
		*(u32*)0x02048294 = 0x0221A980;
		patchUserSettingsReadDSiWare(0x020495B0);
	}*/

	// Lola's Alphabet Train (USA)
	else if (strcmp(romTid, "KLKE") == 0) {
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x02024D24 = 0xE1A00000; // nop
		*(u32*)0x02028FAC = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E7B0, heapEnd);
		patchUserSettingsReadDSiWare(0x0202FDB4);
	}

	// Lola's Alphabet Train (Europe)
	else if (strcmp(romTid, "KLKP") == 0) {
		*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x02024CE4 = 0xE1A00000; // nop
		*(u32*)0x02028F6C = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E770, heapEnd);
		patchUserSettingsReadDSiWare(0x0202FD74);
	}

	// Lola's Fruit Shop Sudoku (USA)
	else if (strcmp(romTid, "KOFE") == 0) {
		*(u32*)0x02005108 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200BAD4 = 0xE1A00000; // nop
		*(u32*)0x0200EB58 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201377C, heapEnd);
		*(u32*)0x02013B08 = 0x021BC260;
		patchUserSettingsReadDSiWare(0x02014C48);
		*(u32*)0x0201CF70 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Lola's Fruit Shop Sudoku (Europe)
	else if (strcmp(romTid, "KOFP") == 0) {
		*(u32*)0x02005108 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200BB04 = 0xE1A00000; // nop
		*(u32*)0x0200EB88 = 0xE1A00000; // nop
		patchInitDSiWare(0x020137AC, heapEnd);
		*(u32*)0x02013B38 = 0x021BC1A0;
		patchUserSettingsReadDSiWare(0x02014C78);
		*(u32*)0x0201CFCC = 0xE1A00000; // nop (Skip Manual screen)
	}
#else
	// Maestro! Green Groove (USA)
	// Maestro! Green Groove (Europe, Australia)
	// Does not save due to unknown cause
	else if (strcmp(romTid, "KMUE") == 0 || strcmp(romTid, "KM6V") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x020137E4 = 0xE1A00000; // nop
		*(u32*)0x02016C1C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E704, heapEnd);
		*(u32*)0x02029F34 = 0xE3A00001; // mov r0, #1
		if (romTid[3] == 'E') {
			*(u32*)0x020B9AE4 = 0xE1A00000; // nop
			*(u32*)0x020B9B00 = 0xE1A00000; // nop
			*(u32*)0x020BC568 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020BC904 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020BC908 = 0xE12FFF1E; // bx lr
			/* setBL(0x020BC7BC, (u32)dsiSaveOpenR);
			setBL(0x020BC7D0, (u32)dsiSaveCreate);
			setBL(0x020BC7DC, (u32)dsiSaveClose);
			setBL(0x020BC7F0, (u32)dsiSaveOpen);
			*(u32*)0x020BC824 = 0xE1A00000; // nop
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

			if (useSharedFont && !extendedMemory2) {
				patchTwlFontLoad(0x020C3230, 0x02020064);
				if (expansionPakFound) {
					*(u32*)0x020C3298 = 0xE1A00000; // nop
				}
			}
		} else {
			*(u32*)0x020B9FBC = 0xE1A00000; // nop
			*(u32*)0x020B9FD8 = 0xE1A00000; // nop
			*(u32*)0x020BCA40 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020BCDDC = 0xE3A00001; // mov r0, #1
			*(u32*)0x020BCDE0 = 0xE12FFF1E; // bx lr
			/* setBL(0x020BCC94, (u32)dsiSaveOpenR);
			setBL(0x020BCCA8, (u32)dsiSaveCreate);
			setBL(0x020BCCB4, (u32)dsiSaveClose);
			setBL(0x020BCCC8, (u32)dsiSaveOpen);
			*(u32*)0x020BCCFC = 0xE1A00000; // nop
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

			if (useSharedFont && !extendedMemory2) {
				patchTwlFontLoad(0x020C3708, 0x02020064);
				if (expansionPakFound) {
					*(u32*)0x020C3770 = 0xE1A00000; // nop
				}
			}
		}
	}

	// Magical Diary: Secrets Sharing (USA)
	// Unable to save data due to RAM limitations
	/* else if (strcmp(romTid, "K73E") == 0) {
		*(u32*)0x0201A17C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0201A22C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x0201A6A0, (u32)dsiSaveClose);
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
		*(u32*)0x0201AF84 = 0xE1A00000; // nop
		setBL(0x0201AF90, (u32)dsiSaveCreate);
		setBL(0x0201AFB4, (u32)dsiSaveClose);
		setBL(0x0201B058, (u32)dsiSaveOpen);
		setBL(0x0201B06C, (u32)dsiSaveCreate);
		setBL(0x0201B090, (u32)dsiSaveClose);
		setBL(0x0201B0E4, (u32)dsiSaveOpen);
		setBL(0x0201B0F4, (u32)dsiSaveRead);
		setBL(0x0201B118, (u32)dsiSaveClose);
		setBL(0x0201B12C, (u32)dsiSaveClose);
		*(u32*)0x020764FC = 0xE1A00000; // nop
		*(u32*)0x0207650C = 0xE1A00000; // nop
		*(u32*)0x0208AFFC = 0xE1A00000; // nop
		*(u32*)0x0208B124 = 0xE1A00000; // nop
		*(u32*)0x0208B138 = 0xE1A00000; // nop
		*(u32*)0x0208E6DC = 0xE1A00000; // nop
		patchInitDSiWare(0x02096C54, heapEnd);
		*(u32*)0x02096FE0 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0209831C);
		*(u32*)0x02098338 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0209833C = 0xE12FFF1E; // bx lr
		*(u32*)0x02098344 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02098348 = 0xE12FFF1E; // bx lr
	} */

	// Tomodachi Tsukurou!: Mahou no Koukan Nikki (Japan)
	// Unable to save data due to RAM limitations
	/* else if (strcmp(romTid, "K85J") == 0 && extendedMemory2) {
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
		*(u32*)0x0201B150 = 0xE1A00000; // nop
		setBL(0x0201B15C, (u32)dsiSaveCreate);
		setBL(0x0201B178, (u32)dsiSaveClose);
		setBL(0x0201B1F0, (u32)dsiSaveOpen);
		setBL(0x0201B204, (u32)dsiSaveCreate);
		setBL(0x0201B21C, (u32)dsiSaveClose);
		setBL(0x0201B270, (u32)dsiSaveOpen);
		setBL(0x0201B280, (u32)dsiSaveRead);
		setBL(0x0201B2A0, (u32)dsiSaveClose);
		*(u32*)0x02078BE4 = 0xE1A00000; // nop
		*(u32*)0x02078BF4 = 0xE1A00000; // nop
		*(u32*)0x020A6620 = 0xE1A00000; // nop
		*(u32*)0x020A9D64 = 0xE1A00000; // nop
		*(u32*)0x020B01A8 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020B23E0, heapEnd); // mov r0, #0x2700000
		patchUserSettingsReadDSiWare(0x020B37CC);
		*(u32*)0x020B37E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020B37EC = 0xE12FFF1E; // bx lr
		*(u32*)0x020B37F4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020B37F8 = 0xE12FFF1E; // bx lr
	} */

	// Magical Drop Yurutto (Japan)
	/*else if (strcmp(romTid, "KMAJ") == 0) {
		*(u32*)0x02005D08 = 0xE12FFF1E; // bx lr
		*(u32*)0x02019350 = 0xE1A00000; // nop
		*(u32*)0x02019360 = 0xE1A00000; // nop
		*(u32*)0x0201936C = 0xE1A00000; // nop
		*(u32*)0x02019378 = 0xE1A00000; // nop
		*(u32*)0x0201940C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201942C = 0xE1A00000; // nop
		*(u32*)0x0201943C = 0xE1A00000; // nop
		// Save patch code causes weird crash
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
		*(u32*)0x020199FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201C160 = 0xE1A00000; // nop
		*(u32*)0x0201C89C = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
		*(u32*)0x0202DA1C = 0xE12FFF1E; // bx lr
		*(u32*)0x0202E65C = 0xE12FFF1E; // bx lr
		*(u32*)0x020343E8 = 0xE1A00000; // nop
		tonccpy((u32*)0x020350D0, dsiSaveGetResultCode, 0xC);
		tonccpy((u32*)0x02035C7C, dsiSaveGetInfo, 0xC);
		*(u32*)0x02037D30 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203D968, heapEnd);
		*(u32*)0x0203DE70 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		patchUserSettingsReadDSiWare(0x0203F0F0);
		*(u32*)0x0203F468 = 0xE1A00000; // nop
		*(u32*)0x0203F46C = 0xE1A00000; // nop
		*(u32*)0x0203F470 = 0xE1A00000; // nop
		*(u32*)0x0203F474 = 0xE1A00000; // nop
	}*/

	// Magical Whip (USA)
	else if (strcmp(romTid, "KWME") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200E5D4 = 0xE1A00000; // nop
		*(u32*)0x02011A18 = 0xE1A00000; // nop
		patchInitDSiWare(0x020172B8, heapEnd);
		patchUserSettingsReadDSiWare(0x02018758);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02030914, 0x02018CD8);
			}
		} else {
			*(u32*)0x0201D4F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02030288 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02026E94 = 0xE1A00000; // nop
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
		*(u32*)0x0203FB18 = 0xE1A00000; // nop
		setBL(0x0203FB34, (u32)dsiSaveCreate);
	}

	// Magical Whip (Europe)
	else if (strcmp(romTid, "KWMP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200E610 = 0xE1A00000; // nop
		*(u32*)0x02011ADC = 0xE1A00000; // nop
		patchInitDSiWare(0x02017398, heapEnd);
		patchUserSettingsReadDSiWare(0x02018838);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020309F4, 0x02018DB8);
			}
		} else {
			*(u32*)0x0201D5D8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02030368 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x02026F74 = 0xE1A00000; // nop
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
		*(u32*)0x0203FBF8 = 0xE1A00000; // nop
		setBL(0x0203FC14, (u32)dsiSaveCreate);
	}

	// Magnetic Joe (USA)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KJOE") == 0) {
		*(u32*)0x02013344 = 0xE1A00000; // nop
		*(u32*)0x0201705C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CEBC, heapEnd);
		*(u32*)0x0201D248 = 0x020C5360;
		*(u32*)0x0201E658 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E65C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201E664 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201E668 = 0xE12FFF1E; // bx lr
		*(u32*)0x02027058 = 0xE1A00000; // nop
		*(u32*)0x02027328 = 0xE1A00000; // nop
		*(u32*)0x02036964 = 0xE1A00000; // nop
		*(u32*)0x02036980 = 0xE1A00000; // nop
		*(u32*)0x020369F8 = 0xE1A00000; // nop
		*(u32*)0x02036A30 = 0xE1A00000; // nop (Skip
		*(u32*)0x02036A34 = 0xE1A00000; // nop  Manual screen)
		*(u32*)0x02036AAC = 0xE1A00000; // nop
		*(u32*)0x0204E5AC = 0xE3A00002; // mov r0, #2
		*(u32*)0x0204E62C = 0xE1A00000; // nop
		*(u32*)0x0205DAAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205DAB0 = 0xE12FFF1E; // bx lr
	}

	// Make Up & Style (USA)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KYLE") == 0 && debugOrMep) {
		if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FD0;

			*(u32*)0x02004FD0 = bssEnd - relocateBssPart(ndsHeader, bssEnd, 0x0213CA60, 0x022F4A60, (s2FlashcardId == 0x5A45) ? 0x08D3CA60 : 0x0913CA60);
			setB(0x02020930, 0x02020984); // Skip FMV playback due to a weird bug
		}

		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x020052B8 = 0xE1A00000; // nop
		*(u32*)0x020052BC = 0xE1A00000; // nop
		*(u32*)0x020052E0 = 0xE1A00000; // nop
		*(u32*)0x020052F4 = 0xE1A00000; // nop
		*(u32*)0x02005324 = 0xE1A00000; // nop
		*(u32*)0x02005348 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0200534C = 0xE1A00000; // nop
		setBL(0x0202A34C, (u32)dsiSaveOpen);
		setBL(0x0202A360, (u32)dsiSaveCreate);
		setBL(0x0202A384, (u32)dsiSaveOpen);
		setBL(0x0202A398, (u32)dsiSaveSetLength);
		setBL(0x0202A3A4, (u32)dsiSaveClose);
		setBL(0x0202A3C4, (u32)dsiSaveSetLength);
		setBL(0x0202A3CC, (u32)dsiSaveClose);
		*(u32*)0x0202A4E8 = 0xE1A00000; // nop
		setBL(0x0202A5C4, (u32)dsiSaveOpen);
		setBL(0x0202A5EC, (u32)dsiSaveSeek);
		setBL(0x0202A600, (u32)dsiSaveRead);
		setBL(0x0202A60C, (u32)dsiSaveClose);
		setBL(0x0202A6D4, (u32)dsiSaveOpen);
		setBL(0x0202A6FC, (u32)dsiSaveSeek);
		setBL(0x0202A710, (u32)dsiSaveWrite);
		setBL(0x0202A71C, (u32)dsiSaveClose);
		*(u32*)0x020558D4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02056468, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02059040 = 0xE1A00000; // nop
		patchInitDSiWare(0x02060784, heapEnd);
		*(u32*)0x02060B10 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02061F74);
		*(u32*)0x0206237C = 0xE1A00000; // nop
		*(u32*)0x02062380 = 0xE1A00000; // nop
		*(u32*)0x02062384 = 0xE1A00000; // nop
		*(u32*)0x02062388 = 0xE1A00000; // nop
	}

	// Make Up & Style (Europe)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KYLP") == 0 && debugOrMep) {
		if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, 0x0213CC40, 0x022F4C40, (s2FlashcardId == 0x5A45) ? 0x08D3CC40 : 0x0913CC40);
			setB(0x02020A68, 0x02020ABC); // Skip FMV playback due to a weird bug
		}

		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x020052D0 = 0xE1A00000; // nop
		*(u32*)0x020052D4 = 0xE1A00000; // nop
		*(u32*)0x020052F8 = 0xE1A00000; // nop
		*(u32*)0x0200530C = 0xE1A00000; // nop
		*(u32*)0x0200533C = 0xE1A00000; // nop
		*(u32*)0x02005360 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x02005364 = 0xE1A00000; // nop
		setBL(0x0202A484, (u32)dsiSaveOpen);
		setBL(0x0202A498, (u32)dsiSaveCreate);
		setBL(0x0202A4BC, (u32)dsiSaveOpen);
		setBL(0x0202A4D0, (u32)dsiSaveSetLength);
		setBL(0x0202A4DC, (u32)dsiSaveClose);
		setBL(0x0202A4FC, (u32)dsiSaveSetLength);
		setBL(0x0202A504, (u32)dsiSaveClose);
		*(u32*)0x0202A620 = 0xE1A00000; // nop
		setBL(0x0202A6FC, (u32)dsiSaveOpen);
		setBL(0x0202A724, (u32)dsiSaveSeek);
		setBL(0x0202A738, (u32)dsiSaveRead);
		setBL(0x0202A744, (u32)dsiSaveClose);
		setBL(0x0202A80C, (u32)dsiSaveOpen);
		setBL(0x0202A834, (u32)dsiSaveSeek);
		setBL(0x0202A848, (u32)dsiSaveWrite);
		setBL(0x0202A854, (u32)dsiSaveClose);
		*(u32*)0x02055A0C = 0xE1A00000; // nop
		tonccpy((u32*)0x020565A0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02059200 = 0xE1A00000; // nop
		patchInitDSiWare(0x02060968, heapEnd);
		*(u32*)0x02060CF4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02062158);
		*(u32*)0x02062560 = 0xE1A00000; // nop
		*(u32*)0x02062564 = 0xE1A00000; // nop
		*(u32*)0x02062568 = 0xE1A00000; // nop
		*(u32*)0x0206256C = 0xE1A00000; // nop
	}

	// Mario Calculator (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KWFE") == 0 && extendedMemory2) {
		doubleNopT(0x0200504E);
		*(u16*)0x02010E64 = 0x7047; // bx lr
		*(u16*)0x020346F8 = 0xB003; // ADD SP, SP, #0xC
		*(u16*)0x020346FA = 0xBD78; // POP {R3-R6,PC}
		doubleNopT(0x020369D4);
		doubleNopT(0x0203B08C);
		doubleNopT(0x0203C5DA);
		doubleNopT(0x0203C5DE);
		doubleNopT(0x0203C5EA);
		doubleNopT(0x0203C6D6);
		patchHiHeapDSiWareThumb(0x0203C714, 0x02039A40, heapEnd);
		// *(u32*)0x0203C7EC = 0x02090140;
		*(u16*)0x020474DA = nopT;
		*(u16*)0x020474E6 = nopT;
		*(u16*)0x020474F0 = nopT;
	}

	// Mario vs. Donkey Kong: Minis March Again! (USA)
	// Save code too advanced to patch, preventing support
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDME") == 0 && extendedMemory2) {
		//extern u32* mvdk3HeapAlloc;

		*(u32*)0x02005180 = 0xE3A01702; // mov r1, #0x80000
		/*if (!extendedMemory2) {
			*(u32*)0x02005188 = 0xE1A00000; // nop (Disable loading sound data)
		}*/
		*(u32*)0x02005190 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		*(u32*)0x0202E6F8 = 0xE1A00000; // nop
		*(u32*)0x0202E788 = 0xE1A00000; // nop
		/*if (!extendedMemory2) {
			tonccpy((u32*)0x0206AAC8, mvdk3HeapAlloc, 0x1D8);
			setBL(0x02033288, 0x0206AAC8);
		}*/
		*(u32*)0x020612B8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020612BC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02064F80 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206F720, heapEndExceed);
		patchUserSettingsReadDSiWare(0x02070E5C);
		*(u32*)0x02071390 = 0xE1A00000; // nop
		*(u32*)0x02071394 = 0xE1A00000; // nop
		*(u32*)0x02071398 = 0xE1A00000; // nop
		*(u32*)0x0207139C = 0xE1A00000; // nop
	}

	// Mario vs. Donkey Kong: Minis March Again! (Europe, Australia)
	// Save code too advanced to patch, preventing support
	// Requires 8MB of RAM
	// Does not boot
	else if (strcmp(romTid, "KDMV") == 0 && extendedMemory2) {
		*(u32*)0x0200518C = 0xE3A01702; // mov r1, #0x80000
		/*if (!extendedMemory2) {
			*(u32*)0x02005194 = 0xE1A00000; // nop (Disable loading sound data)
		}*/
		*(u32*)0x0200519C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0202C6D0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202C6D4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202EF08 = 0xE1A00000; // nop
		*(u32*)0x0202EF98 = 0xE1A00000; // nop
		*(u32*)0x02033850 = 0xE1A00000; // nop
		*(u32*)0x02033858 = 0xE1A00000; // nop
		*(u32*)0x020617B4 = 0xE1A00000; // nop
		*(u32*)0x0206534C = 0xE1A00000; // nop
		patchInitDSiWare(0x0206E490, heapEndExceed);
		patchUserSettingsReadDSiWare(0x0206FBAC);
		*(u32*)0x020700AC = 0xE1A00000; // nop
		*(u32*)0x020700B0 = 0xE1A00000; // nop
		*(u32*)0x020700B4 = 0xE1A00000; // nop
		*(u32*)0x020700B8 = 0xE1A00000; // nop
	}

	// Mario vs. Donkey Kong: Mini Mini Sai Koushin! (Japan)
	// Save code too advanced to patch, preventing support
	// Requires 8MB of RAM
	// Does not boot
	else if (strcmp(romTid, "KDMJ") == 0 && extendedMemory2) {
		*(u32*)0x0200518C = 0xE3A01702; // mov r1, #0x80000
		/*if (!extendedMemory2) {
			*(u32*)0x02005194 = 0xE1A00000; // nop (Disable loading sound data)
		}*/
		*(u32*)0x0200519C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0202C6B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202C6B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202EEE8 = 0xE1A00000; // nop
		*(u32*)0x0202EF78 = 0xE1A00000; // nop
		*(u32*)0x02033890 = 0xE1A00000; // nop
		*(u32*)0x02033898 = 0xE1A00000; // nop
		*(u32*)0x020617F4 = 0xE1A00000; // nop
		*(u32*)0x0206538C = 0xE1A00000; // nop
		patchInitDSiWare(0x0206E4D0, heapEndExceed);
		patchUserSettingsReadDSiWare(0x0206FBEC);
		*(u32*)0x020700EC = 0xE1A00000; // nop
		*(u32*)0x020700F0 = 0xE1A00000; // nop
		*(u32*)0x020700F4 = 0xE1A00000; // nop
		*(u32*)0x020700F8 = 0xE1A00000; // nop
	}

	// Master of Illusion Express: Deep Psyche (USA, Australia)
	// A Little Bit of... Magic Made Fun: Deep Psyche (Europe)
	else if (strcmp(romTid, "KM9T") == 0 || strcmp(romTid, "KM9P") == 0) {
		u8 offsetChangeM = (romTid[3] == 'T') ? 0 : 0xB0;
		u16 offsetChange = (romTid[3] == 'T') ? 0 : 0x304;
		u16 offsetChangeInit0 = (romTid[3] == 'T') ? 0 : 0x31C;
		u16 offsetChangeInit = (romTid[3] == 'T') ? 0 : 0x344;

		*(u32*)(0x02005878+offsetChangeM) = 0xE1A00000; // nop (Skip Manual screen)
		if (romTid[3] == 'T') {
			*(u32*)0x02006B40 = 0xE1A00000; // nop
			*(u32*)0x02006B68 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006BCC = 0xE1A00000; // nop
			*(u32*)0x02006BD8 = 0xE1A00000; // nop
			setBL(0x02006D0C, (u32)dsiSaveOpen);
			setBL(0x02006D24, (u32)dsiSaveRead);
			setBL(0x02006DC8, (u32)dsiSaveClose);
			*(u32*)0x02006DE4 = 0xE1A00000; // nop
			setBL(0x02006E20, (u32)dsiSaveCreate);
			setBL(0x02006E90, (u32)dsiSaveOpen);
			setBL(0x02006EC0, (u32)dsiSaveSetLength);
			setBL(0x02006ED0, (u32)dsiSaveWrite);
			setBL(0x02006ED8, (u32)dsiSaveClose);
		} else {
			*(u32*)0x02006D90 = 0xE1A00000; // nop
			*(u32*)0x02006DB8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006E60 = 0xE1A00000; // nop
			*(u32*)0x02006E6C = 0xE1A00000; // nop
			setBL(0x02006FE4, (u32)dsiSaveOpen);
			setBL(0x02006FFC, (u32)dsiSaveRead);
			setBL(0x020070C8, (u32)dsiSaveClose);
			*(u32*)0x020070E8 = 0xE1A00000; // nop
			setBL(0x02007124, (u32)dsiSaveCreate);
			setBL(0x02007194, (u32)dsiSaveOpen);
			setBL(0x020071C4, (u32)dsiSaveSetLength);
			setBL(0x020071D4, (u32)dsiSaveWrite);
			setBL(0x020071DC, (u32)dsiSaveClose);
		}
		*(u32*)(0x0200B058+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B064+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B078+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B698+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B69C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B6A8+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020394A0+offsetChangeInit0) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0203A10C+offsetChangeInit), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0203C8EC+offsetChangeInit) = 0xE1A00000; // nop
		patchInitDSiWare(0x02042150+offsetChangeInit, heapEnd);
	}

	// Chotto Majikku Taizen: Osoroshii Suuji (Japan)
	else if (strcmp(romTid, "KM9J") == 0) {
		*(u32*)0x02005874 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02006B3C = 0xE1A00000; // nop
		*(u32*)0x02006B64 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02006BC8 = 0xE1A00000; // nop
		*(u32*)0x02006BD4 = 0xE1A00000; // nop
		setBL(0x02006D14, (u32)dsiSaveOpen);
		setBL(0x02006D2C, (u32)dsiSaveRead);
		setBL(0x02006DE0, (u32)dsiSaveClose);
		*(u32*)0x02006DFC = 0xE1A00000; // nop
		setBL(0x02006E38, (u32)dsiSaveCreate);
		setBL(0x02006EA8, (u32)dsiSaveOpen);
		setBL(0x02006EE0, (u32)dsiSaveSetLength);
		setBL(0x02006EF4, (u32)dsiSaveWrite);
		setBL(0x02006EFC, (u32)dsiSaveClose);
		*(u32*)0x0200B244 = 0xE1A00000; // nop
		*(u32*)0x0200B250 = 0xE1A00000; // nop
		*(u32*)0x0200B258 = 0xE1A00000; // nop
		*(u32*)0x0200B888 = 0xE1A00000; // nop
		*(u32*)0x0200B88C = 0xE1A00000; // nop
		*(u32*)0x0200B89C = 0xE1A00000; // nop
		*(u32*)0x0204995C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02049960 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0204A6DC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204CFEC = 0xE1A00000; // nop
		patchInitDSiWare(0x0205906C, heapEnd);
	}

	// Master of Illusion Express: Funny Face (USA, Australia)
	// A Little Bit of... Magic Made Fun: Funny Face (Europe)
	else if (strcmp(romTid, "KMFT") == 0 || strcmp(romTid, "KMFP") == 0 /* || strcmp(romTid, "KMFX") == 0 */) {
		u8 offsetChangeM = (romTid[3] == 'T') ? 0 : 0x50;
		u16 offsetChange = (romTid[3] == 'T') ? 0 : 0x2A0;
		u16 offsetChangeInit0 = (romTid[3] == 'T') ? 0 : 0x46C;
		u16 offsetChangeInit = (romTid[3] == 'T') ? 0 : 0x494;
		/* if (romTid[3] == 'X') {
			offsetChangeM = 0x54;
			offsetChange = 0x2F0;
			offsetChangeInit0 = 0x5DC;
			offsetChangeInit = 0x604;
		} */

		*(u32*)(0x0200584C+offsetChangeM) = 0xE1A00000; // nop (Skip Manual screen)
		if (romTid[3] == 'T') {
			*(u32*)0x02006B64 = 0xE1A00000; // nop
			*(u32*)0x02006B8C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006BF0 = 0xE1A00000; // nop
			*(u32*)0x02006BFC = 0xE1A00000; // nop
			setBL(0x02006D30, (u32)dsiSaveOpen);
			setBL(0x02006D48, (u32)dsiSaveRead);
			setBL(0x02006DEC, (u32)dsiSaveClose);
			*(u32*)0x02006E08 = 0xE1A00000; // nop
			setBL(0x02006E44, (u32)dsiSaveCreate);
			setBL(0x02006EB4, (u32)dsiSaveOpen);
			setBL(0x02006EE4, (u32)dsiSaveSetLength);
			setBL(0x02006EF4, (u32)dsiSaveWrite);
			setBL(0x02006EFC, (u32)dsiSaveClose);
		} else if (romTid[3] == 'P') {
			*(u32*)0x02006D50 = 0xE1A00000; // nop
			*(u32*)0x02006D78 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006E20 = 0xE1A00000; // nop
			*(u32*)0x02006E2C = 0xE1A00000; // nop
			setBL(0x02006FA4, (u32)dsiSaveOpen);
			setBL(0x02006FBC, (u32)dsiSaveRead);
			setBL(0x02007088, (u32)dsiSaveClose);
			*(u32*)0x020070A8 = 0xE1A00000; // nop
			setBL(0x020070E4, (u32)dsiSaveCreate);
			setBL(0x02007154, (u32)dsiSaveOpen);
			setBL(0x02007184, (u32)dsiSaveSetLength);
			setBL(0x02007194, (u32)dsiSaveWrite);
			setBL(0x0200719C, (u32)dsiSaveClose);
		} /* else {
			*(u32*)0x02006D58 = 0xE1A00000; // nop
			*(u32*)0x02006D80 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006E58 = 0xE1A00000; // nop
			*(u32*)0x02006E64 = 0xE1A00000; // nop
			setBL(0x02006FDC, (u32)dsiSaveOpen);
			setBL(0x02006FF4, (u32)dsiSaveRead);
			setBL(0x020070D8, (u32)dsiSaveClose);
			*(u32*)0x020070F8 = 0xE1A00000; // nop
			setBL(0x02007134, (u32)dsiSaveCreate);
			setBL(0x020071A4, (u32)dsiSaveOpen);
			setBL(0x020071D4, (u32)dsiSaveSetLength);
			setBL(0x020071E4, (u32)dsiSaveWrite);
			setBL(0x020071EC, (u32)dsiSaveClose);
		} */
		*(u32*)(0x0200B220+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B22C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B240+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B87C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B880+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B88C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0202EF6C+offsetChangeInit0) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0202FBD8+offsetChangeInit), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x020323B8+offsetChangeInit) = 0xE1A00000; // nop
		patchInitDSiWare(0x02037B70+offsetChangeInit, heapEnd);
	}

	// Chotto Majikku Taizen: Funi Fuisu (Japan)
	else if (strcmp(romTid, "KMFJ") == 0) {
		*(u32*)0x0200588C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02006BA4 = 0xE1A00000; // nop
		*(u32*)0x02006BCC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02006C30 = 0xE1A00000; // nop
		*(u32*)0x02006C3C = 0xE1A00000; // nop
		setBL(0x02006D7C, (u32)dsiSaveOpen);
		setBL(0x02006D94, (u32)dsiSaveRead);
		setBL(0x02006E48, (u32)dsiSaveClose);
		*(u32*)0x02006E64 = 0xE1A00000; // nop
		setBL(0x02006EA0, (u32)dsiSaveCreate);
		setBL(0x02006F10, (u32)dsiSaveOpen);
		setBL(0x02006F48, (u32)dsiSaveSetLength);
		setBL(0x02006F5C, (u32)dsiSaveWrite);
		setBL(0x02006F64, (u32)dsiSaveClose);
		*(u32*)0x0200B400 = 0xE1A00000; // nop
		*(u32*)0x0200B40C = 0xE1A00000; // nop
		*(u32*)0x0200B414 = 0xE1A00000; // nop
		*(u32*)0x0200BA60 = 0xE1A00000; // nop
		*(u32*)0x0200BA64 = 0xE1A00000; // nop
		*(u32*)0x0200BA74 = 0xE1A00000; // nop
		*(u32*)0x0204BB78 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0204BB7C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0204C8F8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204F208 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205B1DC, heapEnd);
	}

	// Master of Illusion Express: Matchmaker (USA, Australia)
	// A Little Bit of... Magic Made Fun: Matchmaker (Europe)
	else if (strcmp(romTid, "KMDT") == 0 || strcmp(romTid, "KMDP") == 0) {
		u8 offsetChangeM = (romTid[3] == 'T') ? 0 : 0xE4;
		u16 offsetChange = (romTid[3] == 'T') ? 0 : 0x2F4;
		u16 offsetChangeInit = (romTid[3] == 'T') ? 0 : 0x4B8;

		*(u32*)0x02005140 = 0xE1A00000; // nop (Skip Camera, Part 1)
		*(u32*)(0x0200586C+offsetChangeM) = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02006C18+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02006C30+offsetChange, (u32)dsiSaveRead);
		setBL(0x02006CB0+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02006CCC+offsetChange) = 0xE1A00000; // nop
		setBL(0x02006D08+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02006D78+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02006DA8+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x02006DB8+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02006DC0+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0200AF80+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200AF8C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200AFA0+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B604+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B608+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B614+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B978+offsetChange) = 0xE3A00000; // mov r0, #0 (Skip Camera, Part 2)
		*(u32*)(0x0202D3EC+offsetChangeInit) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0202E080+offsetChangeInit), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x02030B2C+offsetChangeInit) = 0xE1A00000; // nop
		patchInitDSiWare(0x02036304+offsetChangeInit, heapEnd);
	}

	// Chotto Majikku Taizen: Deto Uranai (Japan)
	else if (strcmp(romTid, "KMDJ") == 0) {
		*(u32*)0x0200511C = 0xE1A00000; // nop (Skip Camera, Part 1)
		*(u32*)0x020057D0 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02006B68, (u32)dsiSaveOpen);
		setBL(0x02006B80, (u32)dsiSaveRead);
		setBL(0x02006C00, (u32)dsiSaveClose);
		*(u32*)0x02006C1C = 0xE1A00000; // nop
		setBL(0x02006C58, (u32)dsiSaveCreate);
		setBL(0x02006CC8, (u32)dsiSaveOpen);
		setBL(0x02006CF8, (u32)dsiSaveSetLength);
		setBL(0x02006D08, (u32)dsiSaveWrite);
		setBL(0x02006D10, (u32)dsiSaveClose);
		*(u32*)0x0200AED8 = 0xE1A00000; // nop
		*(u32*)0x0200AEE4 = 0xE1A00000; // nop
		*(u32*)0x0200AEEC = 0xE1A00000; // nop
		*(u32*)0x0200B548 = 0xE1A00000; // nop
		*(u32*)0x0200B54C = 0xE1A00000; // nop
		*(u32*)0x0200B558 = 0xE1A00000; // nop
		*(u32*)0x0200B8BC = 0xE3A00000; // mov r0, #0 (Skip Camera, Part 2)
		*(u32*)0x0202D28C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0202D290 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0202E000, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020309A8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02037988, heapEnd);
	}

	// Master of Illusion Express: Mind Probe (USA, Australia)
	// A Little Bit of... Magic Made Fun: Mind Probe (Europe)
	else if (strcmp(romTid, "KMIT") == 0 || strcmp(romTid, "KMIP") == 0) {
		u8 offsetChangeM = (romTid[3] == 'T') ? 0 : 0x60;
		u16 offsetChange = (romTid[3] == 'T') ? 0 : 0x270;
		u16 offsetChangeInit = (romTid[3] == 'T') ? 0 : 0x368;

		*(u32*)(0x02005814+offsetChangeM) = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02006B94+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02006BAC+offsetChange, (u32)dsiSaveRead);
		setBL(0x02006C2C+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02006C48+offsetChange) = 0xE1A00000; // nop
		setBL(0x02006C84+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02006CF4+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02006D24+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x02006D34+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02006D3C+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0200AEFC+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200AF08+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200AF1C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B580+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B584+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B590+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0202CCC4+offsetChangeInit) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0202D958+offsetChangeInit), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0203019C+offsetChangeInit) = 0xE1A00000; // nop
		patchInitDSiWare(0x02035890+offsetChangeInit, heapEnd);
	}

	// Chotto Majikku Taizen: Suki Kirai Hakkenki (Japan)
	else if (strcmp(romTid, "KMIJ") == 0) {
		*(u32*)0x02005780 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02006AEC, (u32)dsiSaveOpen);
		setBL(0x02006B04, (u32)dsiSaveRead);
		setBL(0x02006B84, (u32)dsiSaveClose);
		*(u32*)0x02006BA0 = 0xE1A00000; // nop
		setBL(0x02006BDC, (u32)dsiSaveCreate);
		setBL(0x02006C4C, (u32)dsiSaveOpen);
		setBL(0x02006C7C, (u32)dsiSaveSetLength);
		setBL(0x02006C8C, (u32)dsiSaveWrite);
		setBL(0x02006C94, (u32)dsiSaveClose);
		*(u32*)0x0200AE5C = 0xE1A00000; // nop
		*(u32*)0x0200AE68 = 0xE1A00000; // nop
		*(u32*)0x0200AE70 = 0xE1A00000; // nop
		*(u32*)0x0200B4CC = 0xE1A00000; // nop
		*(u32*)0x0200B4D0 = 0xE1A00000; // nop
		*(u32*)0x0200B4DC = 0xE1A00000; // nop
		*(u32*)0x0202CB4C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0202CB50 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0202D8C0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020300A4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02036D24, heapEnd);
	}

	// Master of Illusion Express: Shuffle Games (USA, Australia)
	// A Little Bit of... Magic Made Fun: Shuffle Games (Europe)
	else if (strcmp(romTid, "KMST") == 0 || strcmp(romTid, "KMSP") == 0) {
		u8 offsetChangeM = (romTid[3] == 'T') ? 0 : 0x50;
		u16 offsetChange = (romTid[3] == 'T') ? 0 : 0x2A0;
		u16 offsetChangeInit0 = (romTid[3] == 'T') ? 0 : 0x318;
		u16 offsetChangeInit = (romTid[3] == 'T') ? 0 : 0x340;

		*(u32*)(0x0200584C+offsetChangeM) = 0xE1A00000; // nop (Skip Manual screen)
		if (romTid[3] == 'T') {
			*(u32*)0x02006B08 = 0xE1A00000; // nop
			*(u32*)0x02006B30 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006B94 = 0xE1A00000; // nop
			*(u32*)0x02006BA0 = 0xE1A00000; // nop
			setBL(0x02006CD4, (u32)dsiSaveOpen);
			setBL(0x02006CEC, (u32)dsiSaveRead);
			setBL(0x02006D90, (u32)dsiSaveClose);
			*(u32*)0x02006DAC = 0xE1A00000; // nop
			setBL(0x02006DE8, (u32)dsiSaveCreate);
			setBL(0x02006E58, (u32)dsiSaveOpen);
			setBL(0x02006E88, (u32)dsiSaveSetLength);
			setBL(0x02006E98, (u32)dsiSaveWrite);
			setBL(0x02006EA0, (u32)dsiSaveClose);
		} else {
			*(u32*)0x02006CF4 = 0xE1A00000; // nop
			*(u32*)0x02006D1C = 0xE3A00000; // mov r0, #0
			*(u32*)0x02006DC4 = 0xE1A00000; // nop
			*(u32*)0x02006DD0 = 0xE1A00000; // nop
			setBL(0x02006F48, (u32)dsiSaveOpen);
			setBL(0x02006F60, (u32)dsiSaveRead);
			setBL(0x0200702C, (u32)dsiSaveClose);
			*(u32*)0x0200704C = 0xE1A00000; // nop
			setBL(0x02007088, (u32)dsiSaveCreate);
			setBL(0x020070F8, (u32)dsiSaveOpen);
			setBL(0x02007128, (u32)dsiSaveSetLength);
			setBL(0x02007138, (u32)dsiSaveWrite);
			setBL(0x02007140, (u32)dsiSaveClose);
		}
		*(u32*)(0x0200B020+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B02C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B040+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B660+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B664+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0200B670+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0202CB80+offsetChangeInit0) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0202D7EC+offsetChangeInit), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0202FFCC+offsetChangeInit) = 0xE1A00000; // nop
		patchInitDSiWare(0x020355DC+offsetChangeInit, heapEnd);
	}

	// Chotto Majikku Taizen: 3ttsu no Shaffuru Gemu (Japan)
	else if (strcmp(romTid, "KMSJ") == 0) {
		*(u32*)0x0200588C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02006B48 = 0xE1A00000; // nop
		*(u32*)0x02006B70 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02006BD4 = 0xE1A00000; // nop
		*(u32*)0x02006BE0 = 0xE1A00000; // nop
		setBL(0x02006D20, (u32)dsiSaveOpen);
		setBL(0x02006D38, (u32)dsiSaveRead);
		setBL(0x02006DEC, (u32)dsiSaveClose);
		*(u32*)0x02006E08 = 0xE1A00000; // nop
		setBL(0x02006E44, (u32)dsiSaveCreate);
		setBL(0x02006EB4, (u32)dsiSaveOpen);
		setBL(0x02006EEC, (u32)dsiSaveSetLength);
		setBL(0x02006F00, (u32)dsiSaveWrite);
		setBL(0x02006F08, (u32)dsiSaveClose);
		*(u32*)0x0200B250 = 0xE1A00000; // nop
		*(u32*)0x0200B25C = 0xE1A00000; // nop
		*(u32*)0x0200B264 = 0xE1A00000; // nop
		*(u32*)0x0200B894 = 0xE1A00000; // nop
		*(u32*)0x0200B898 = 0xE1A00000; // nop
		*(u32*)0x0200B8A8 = 0xE1A00000; // nop
		*(u32*)0x020496C8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020496CC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0204A448, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0204CD58 = 0xE1A00000; // nop
		patchInitDSiWare(0x02058B84, heapEnd);
	}

	// Match Up! (USA)
	// Match Up! (Europe)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if ((strcmp(romTid, "KUPE") == 0 || strcmp(romTid, "KUPP") == 0) && debugOrMep) {
		u16 offsetChange = (romTid[3] == 'E') ? 0 : 0x190;

		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad((romTid[3] == 'E') ? 0x02005350 : 0x02005338, 0x0202FA88+offsetChange);
			}
		} else {
			*(u32*)0x02005090 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x02005148 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)(0x02011664+offsetChange) = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
		}
		*(u32*)(0x0202622C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02029390+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E068+offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x0202F544+offsetChange);
	}

	// Mega Words (USA)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KWKE") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x020053A0, 0x0203A620);
			} */
		} else {
			*(u32*)0x02005094 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200518C = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020058B8 = 0xE1A00000; // nop
		*(u32*)0x020058E4 = 0xE1A00000; // nop
		*(u32*)0x020058F0 = 0xE1A00000; // nop
		*(u32*)0x02005928 = 0xE1A00000; // nop
		*(u32*)0x02005934 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// *(u32*)0x0201D0C0 = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
			*(u32*)0x0201D108 = 0xE3A00622; // mov r0, #0x02200000
		}
		*(u32*)0x02030D44 = 0xE1A00000; // nop
		*(u32*)0x02033E9C = 0xE1A00000; // nop
		patchInitDSiWare(0x02038B04, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x02038E90 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x0203A0DC);
		*(u32*)0x0203A4E4 = 0xE1A00000; // nop
		*(u32*)0x0203A4E8 = 0xE1A00000; // nop
		*(u32*)0x0203A4EC = 0xE1A00000; // nop
		*(u32*)0x0203A4F0 = 0xE1A00000; // nop
	}

	// Mega Words (Europe)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KWKP") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x0200551C, 0x020456E8);
			} */
		} else {
			*(u32*)0x02005104 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020052FC = 0xE1A00000; // nop
		*(u32*)0x0200531C = 0xE1A00000; // nop
		*(u32*)0x02005AC4 = 0xE1A00000; // nop
		*(u32*)0x02005AF4 = 0xE1A00000; // nop
		*(u32*)0x02005B00 = 0xE1A00000; // nop
		*(u32*)0x02005B3C = 0xE1A00000; // nop
		*(u32*)0x02005B48 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// *(u32*)0x02028150 = (s2FlashcardId == 0x5A45) ? 0xE3A00522 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08800000 : #0x09000000
			*(u32*)0x02028198 = 0xE3A00622; // mov r0, #0x02200000
		}
		*(u32*)0x0203BDD4 = 0xE1A00000; // nop
		*(u32*)0x0203EF38 = 0xE1A00000; // nop
		patchInitDSiWare(0x02043BBC, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x02043F48 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x020451A4);
		*(u32*)0x020455AC = 0xE1A00000; // nop
		*(u32*)0x020455B0 = 0xE1A00000; // nop
		*(u32*)0x020455B4 = 0xE1A00000; // nop
		*(u32*)0x020455B8 = 0xE1A00000; // nop
	}

	// Mehr Kreuzwortratsel: Welt Edition (Germany)
	else if (strcmp(romTid, "KMKD") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x0207EB14;
			u32 sdatOffset = 0x020CB610;

			*(u32*)0x0207EB14 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x170000, bssEnd, sdatOffset);
		// }

		*(u32*)0x0200EDA4 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F91C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02012334 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017AD4, heapEnd);
		*(u32*)0x02017E60 = *(u32*)0x0207EB14;
		patchUserSettingsReadDSiWare(0x02018FD4);
		*(u32*)0x02029258 = 0xE1A00000; // nop (Do not load Manual screen)
		*(u32*)0x0202B0A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0202C75C, (u32)dsiSaveOpen);
		setBL(0x0202C774, (u32)dsiSaveSeek);
		setBL(0x0202C784, (u32)dsiSaveWrite);
		setBL(0x0202C78C, (u32)dsiSaveClose);
		setBL(0x0202DA50, (u32)dsiSaveOpen);
		setBL(0x0202DA68, (u32)dsiSaveSeek);
		setBL(0x0202DA78, (u32)dsiSaveWrite);
		setBL(0x0202DA80, (u32)dsiSaveClose);
		setBL(0x0202DB1C, (u32)dsiSaveOpen);
		setBL(0x0202DB34, (u32)dsiSaveSeek);
		setBL(0x0202DB44, (u32)dsiSaveWrite);
		setBL(0x0202DCD0, (u32)dsiSaveOpen);
		setBL(0x0202DCE8, (u32)dsiSaveSeek);
		setBL(0x0202DCF8, (u32)dsiSaveWrite);
		setBL(0x0202DD00, (u32)dsiSaveClose);
		setBL(0x0202E090, (u32)dsiSaveOpen);
		setBL(0x0202E0A8, (u32)dsiSaveSeek);
		setBL(0x0202E0B8, (u32)dsiSaveWrite);
		setBL(0x0202E0C0, (u32)dsiSaveClose);
		setBL(0x0202E5CC, (u32)dsiSaveOpen);
		setBL(0x0202E5E4, (u32)dsiSaveSeek);
		setBL(0x0202E5F4, (u32)dsiSaveWrite);
		setBL(0x0202E5FC, (u32)dsiSaveClose);
		setBL(0x0202E86C, (u32)dsiSaveOpen);
		setBL(0x0202E884, (u32)dsiSaveSeek);
		setBL(0x0202E884, (u32)dsiSaveWrite);
		setBL(0x0202E89C, (u32)dsiSaveClose);
		*(u32*)0x0202E91C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0202E92C = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0202E93C, (u32)dsiSaveOpen);
		setBL(0x0202E954, (u32)dsiSaveSeek);
		setBL(0x0202E964, (u32)dsiSaveRead);
		setBL(0x0202E96C, (u32)dsiSaveClose);
		setBL(0x0202E9A8, (u32)dsiSaveClose);
		setBL(0x0202E9E8, (u32)dsiSaveCreate);
		setBL(0x0202EA00, (u32)dsiSaveOpen);
		setBL(0x0202EA38, (u32)dsiSaveSeek);
		setBL(0x0202EA48, (u32)dsiSaveWrite);
		setBL(0x0202EA50, (u32)dsiSaveClose);
		setBL(0x0202EA6C, (u32)dsiSaveClose);
		setBL(0x0202EACC, (u32)dsiSaveClose);
		*(u32*)0x0202F03C = 0xE1A00000; // nop
		*(u32*)0x0202F04C = 0xE1A00000; // nop (dsiSaveCreateDir)
		setBL(0x0202F058, (u32)dsiSaveCreate);
		setBL(0x0202F068, (u32)dsiSaveOpen);
		setBL(0x0202F094, (u32)dsiSaveSeek);
		setBL(0x0202F0A4, (u32)dsiSaveWrite);
		setBL(0x0202F0AC, (u32)dsiSaveClose);
	}

	// Meikyou Kokugo: Rakubiki Jiten (Japan)
	// Saving not supported due to using more than one file in filesystem
	// Requires Slot-2 RAM expansion up to 16MB or more (Standard Memory Expansion Pak is not enough)
	else if (strcmp(romTid, "KD4J") == 0 && largeS2RAM) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x02040284 = 0xE3A00001; // mov r0, #1 (Hide battery icon)
		*(u32*)0x020402FC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0205CD28 = 0xE1A00000; // nop
		*(u32*)0x0205E680 = 0xE1A00000; // nop
		*(u32*)0x0205E688 = 0xE1A00000; // nop
		if (!useSharedFont) {
			*(u32*)0x0205ECAC = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x0206E260 = (s2FlashcardId == 0x5A45) ? 0xE3A00408 : 0xE3A00409; // mov r0, (s2FlashcardId == 0x5A45) ? #0x08000000 : #0x09000000
		*(u32*)0x020A6664 = 0xE1A00000; // nop
		*(u32*)0x020A8DFC = 0xE1A00000; // nop
		*(u32*)0x020AD2A0 = 0xE1A00000; // nop
		*(u32*)0x020B28B4 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020B28CC, heapEnd);
		patchUserSettingsReadDSiWare(0x020B4130);
		*(u32*)0x020BABB0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020BABB4 = 0xE12FFF1E; // bx lr
	}

	// Metal Torrent (USA)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59E") == 0) {
		*(u32*)0x020136D4 = 0xE1A00000; // nop
		*(u32*)0x02017CD4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02017CF4 = 0xE1A00000; // nop
		/* *(u32*)0x0201DBAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DBB0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DC58 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DC5C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DCA0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DCA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DD20 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DD24 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DD68 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DD6C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DDB8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DDBC = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DE80 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DE84 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DF48 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DF4C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201DFB4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201DFB8 = 0xE12FFF1E; // bx lr */
		/*setBL(0x0201E198, (u32)dsiSaveCreate);
		setBL(0x0201E374, (u32)dsiSaveOpen);
		setBL(0x0201E4F4, (u32)dsiSaveClose);
		setBL(0x0201E628, (u32)dsiSaveSeek);
		setBL(0x0201E638, (u32)dsiSaveRead);
		setBL(0x0201E7D0, (u32)dsiSaveSeek);
		setBL(0x0201E7E0, (u32)dsiSaveWrite);
		setBL(0x0201E970, (u32)dsiSaveOpenR);
		setBL(0x0201EA18, (u32)dsiSaveClose);*/
		/* *(u32*)0x0201E0EC = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E2B0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E494 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E58C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201EA5C = 0xE3A00001; // mov r0, #1 */
		// *(u32*)0x0201EA90 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02045F88 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		// *(u32*)0x0204DF68 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x0204DF6C = 0xE12FFF1E; // bx lr
		// *(u32*)0x020552DC = 0xE3A00000; // mov r0, #0
		// tonccpy((u32*)0x02057F8C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0205A50C = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		*(u32*)0x02063AA8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02063AAC = 0xE12FFF1E; // bx lr
		*(u32*)0x020DD1A0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DDB00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDD60 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDF00 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DE0A4 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		if (!extendedMemory2) {
			extern u32* metalTorrentSndLoad;

			*(u32*)0x02041F50 = 0xE3A00000; // mov r0, #0 (Disable SSEQ playback)
			*(u32*)0x02041F54 = 0xE12FFF1E; // bx lr

			*(u32*)0x0204DD50 = 0x020FCA20;
			tonccpy((u32*)0x0204DD54, metalTorrentSndLoad, 0x1C);

			*(u32*)0x020F98BC = 0xE3A00901; // mov r0, #0x4000
			setBL(0x020FC094, 0x0204DD54);
		}
	}

	// Metal Torrent (Europe, Australia)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59V") == 0) {
		*(u32*)0x020136EC = 0xE1A00000; // nop
		*(u32*)0x02017CEC = 0xE12FFF1E; // bx lr
		*(u32*)0x02017D0C = 0xE1A00000; // nop
		// *(u32*)0x0201EAA8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02045FA0 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x0205A524 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		*(u32*)0x02063AC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02063AC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DCFB8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DD918 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDB78 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDD18 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020DDEBC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		if (!extendedMemory2) {
			extern u32* metalTorrentSndLoad;

			*(u32*)0x02041F68 = 0xE3A00000; // mov r0, #0 (Disable SSEQ playback)
			*(u32*)0x02041F6C = 0xE12FFF1E; // bx lr

			*(u32*)0x0204DD68 = 0x020FC838;
			tonccpy((u32*)0x0204DD6C, metalTorrentSndLoad, 0x1C);

			*(u32*)0x020F96D4 = 0xE3A00901; // mov r0, #0x4000
			setBL(0x020FBEAC, 0x0204DD6C);
		}
	}

	// A Mujou Setsuna (Japan)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "K59J") == 0) {
		*(u32*)0x020215CC = 0xE1A00000; // nop
		*(u32*)0x02025C50 = 0xE12FFF1E; // bx lr
		*(u32*)0x02025C70 = 0xE1A00000; // nop
		// *(u32*)0x0202BE90 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02041E10 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02042684 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02042930 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02042AD0 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02042C74 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x02089E08 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
		*(u32*)0x0209E3B4 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		*(u32*)0x020A7E44 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020A7E48 = 0xE12FFF1E; // bx lr
		if (!extendedMemory2) {
			extern u32* metalTorrentSndLoad;

			*(u32*)0x02085D60 = 0xE3A00000; // mov r0, #0 (Disable SSEQ playback)
			*(u32*)0x02085D64 = 0xE12FFF1E; // bx lr

			*(u32*)0x02091944 = 0x0205A3B8;
			tonccpy((u32*)0x02091948, metalTorrentSndLoad, 0x1C);

			*(u32*)0x0206AFA0 = 0xE3A00901; // mov r0, #0x4000
			setBL(0x02059A2C, 0x02091948);
		}
	}

	// Mighty Flip Champs! (USA)
	else if (strcmp(romTid, "KMGE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02014BDC;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x0201A38C;

		setBL(0x0200B048, (u32)dsiSaveCreate);
		setBL(0x0200B070, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200B080 = 0xE1A00000; // nop
		setBL(0x0200B090, (u32)dsiSaveCreate);
		setBL(0x0200B0E8, (u32)dsiSaveOpen);
		*(u32*)0x0200B100 = 0xE1A00000; // nop
		setBL(0x0200B114, (u32)dsiSaveOpen);
		setBL(0x0200B124, (u32)dsiSaveRead);
		setBL(0x0200B12C, (u32)dsiSaveClose);
		setBL(0x0200B388, (u32)dsiSaveCreate);
		setBL(0x0200B39C, (u32)dsiSaveOpen);
		setBL(0x0200B5A4, (u32)dsiSaveSetLength);
		setBL(0x0200B5B4, (u32)dsiSaveWrite);
		setBL(0x0200B5BC, (u32)dsiSaveClose);

		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				*(u32*)0x0200FC3C = 0xE3A00703; // mov r0, #0xC0000
				*(u32*)0x0200FC5C = 0xE3A02703; // mov r2, #0xC0000
				*(u32*)0x0200FC68 = 0xE3A01703; // mov r1, #0xC0000

				patchTwlFontLoad(0x0200FD00, 0x0205A324);
			}
		} else {
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

		setBL(0x02014BDC, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		setBL(0x0201A38C, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
		*(u32*)0x0204D3C4 = 0xE1A00000; // nop
		*(u32*)0x02051124 = 0xE1A00000; // nop
		patchInitDSiWare(0x02058530, heapEnd);
		patchUserSettingsReadDSiWare(0x020599A0);
	}

	// Mighty Flip Champs! (Europe, Australia)
	else if (strcmp(romTid, "KMGV") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201528C;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x0201AA44;

		setBL(0x0200B350, (u32)dsiSaveCreate);
		setBL(0x0200B378, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200B388 = 0xE1A00000; // nop
		setBL(0x0200B398, (u32)dsiSaveCreate);
		setBL(0x0200B3F0, (u32)dsiSaveOpen);
		*(u32*)0x0200B408 = 0xE1A00000; // nop
		setBL(0x0200B41C, (u32)dsiSaveOpen);
		setBL(0x0200B42C, (u32)dsiSaveRead);
		setBL(0x0200B434, (u32)dsiSaveClose);
		setBL(0x0200B690, (u32)dsiSaveCreate);
		setBL(0x0200B6A4, (u32)dsiSaveOpen);
		setBL(0x0200B8AC, (u32)dsiSaveSetLength);
		setBL(0x0200B8BC, (u32)dsiSaveWrite);
		setBL(0x0200B8C4, (u32)dsiSaveClose);

		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				*(u32*)0x02010138 = 0xE3A00703; // mov r0, #0xC0000
				*(u32*)0x02010158 = 0xE3A02703; // mov r2, #0xC0000
				*(u32*)0x02010164 = 0xE3A01703; // mov r1, #0xC0000

				patchTwlFontLoad(0x020101FC, 0x02059D68);
			}
		} else {
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

		setBL(0x0201528C, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		setBL(0x0201AA44, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
		*(u32*)0x0204D504 = 0xE1A00000; // nop
		*(u32*)0x02050F30 = 0xE1A00000; // nop
		*(u32*)0x02058340 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x02058358, heapEnd);
		patchUserSettingsReadDSiWare(0x020597C8);
	}

	// Mighty Flip Champs! (Japan)
	else if (strcmp(romTid, "KMGJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		ce9->rumbleFrames[0] = 30;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 2;
		ce9->rumbleForce[1] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02014718;
		ce9->patches->rumble_arm9[1][3] = *(u32*)0x02019C54;

		setBL(0x0200B134, (u32)dsiSaveCreate);
		setBL(0x0200B158, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200B168 = 0xE1A00000; // nop
		setBL(0x0200B174, (u32)dsiSaveCreate);
		setBL(0x0200B1D4, (u32)dsiSaveOpen);
		*(u32*)0x0200B1EC = 0xE1A00000; // nop
		setBL(0x0200B1FC, (u32)dsiSaveOpen);
		setBL(0x0200B210, (u32)dsiSaveRead);
		setBL(0x0200B218, (u32)dsiSaveClose);
		setBL(0x0200B478, (u32)dsiSaveCreate);
		setBL(0x0200B488, (u32)dsiSaveOpen);
		setBL(0x0200B694, (u32)dsiSaveSetLength);
		setBL(0x0200B6A4, (u32)dsiSaveWrite);
		setBL(0x0200B6AC, (u32)dsiSaveClose);

		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				*(u32*)0x0200FAA4 = 0xE3A06703; // mov r6, #0xC0000

				patchTwlFontLoad(0x0200FB68, 0x02057688);
			}
		} else {
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

		setBL(0x02014718, (int)ce9->patches->rumble_arm9[0]); // Make tick sounds when player gets shocked
		setBL(0x02019C54, (int)ce9->patches->rumble_arm9[1]); // Rumble when flip slam effect plays
		*(u32*)0x0204B538 = 0xE1A00000; // nop
		*(u32*)0x0204EEB4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02055CA8, heapEnd);
		patchUserSettingsReadDSiWare(0x02057118);
	}

	// Mighty Milky Way (USA)
	// Mighty Milky Way (Europe)
	// Mighty Milky Way (Japan)
	// Music doesn't play on retail consoles
	else if (strncmp(romTid, "KWY", 3) == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);

		ce9->rumbleFrames[0] = 10;
		ce9->rumbleFrames[1] = 30;
		ce9->rumbleForce[0] = 1;
		ce9->rumbleForce[1] = 1;

		*(u32*)0x0200545C = 0xE1A00000; // nop
		setBL(0x0200547C, (u32)dsiSaveCreate);
		setBL(0x020054A0, (u32)dsiSaveGetResultCode);
		*(u32*)0x020054B0 = 0xE1A00000; // nop
		setBL(0x020054BC, (u32)dsiSaveCreate);
		*(u32*)0x020054C8 = 0xE1A00000; // nop
		*(u32*)0x020054CC = 0xE1A00000; // nop
		*(u32*)0x020054D0 = 0xE1A00000; // nop
		*(u32*)0x020054D4 = 0xE1A00000; // nop
		*(u32*)0x020054DC = 0xE1A00000; // nop
		setBL(0x02005534, (u32)dsiSaveOpen);
		*(u32*)0x0200554C = 0xE1A00000; // nop
		setBL(0x0200555C, (u32)dsiSaveOpen);
		setBL(0x02005570, (u32)dsiSaveRead);
		setBL(0x02005578, (u32)dsiSaveClose);
		*(u32*)0x020057C0 = 0xE1A00000; // nop
		setBL(0x020057E4, (u32)dsiSaveCreate);
		setBL(0x020057F4, (u32)dsiSaveOpen);
		setBL(0x020059FC, (u32)dsiSaveSetLength);
		setBL(0x02005A0C, (u32)dsiSaveWrite);
		setBL(0x02005A14, (u32)dsiSaveClose);
		*(u32*)0x02005A20 = 0xE1A00000; // nop
		*(u32*)0x02005A24 = 0xE1A00000; // nop
		*(u32*)0x02005A28 = 0xE1A00000; // nop
		*(u32*)0x02005A2C = 0xE1A00000; // nop
		*(u32*)0x02005A38 = 0xE1A00000; // nop
		*(u32*)0x02005A3C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x020072CC = 0xE3A00901; // mov r0, #0x4000 (Shrink sound heap from 1MB to 16KB: Disables music)
			tonccpy((u32*)0x02008528, ce9->patches->musicPlay, 0xC);
			tonccpy((u32*)0x02008570, ce9->patches->musicStopEffect, 0xC);
		}
		if (romTid[3] == 'J') {
			ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201D008;
			ce9->patches->rumble_arm9[1][3] = *(u32*)0x020275F8;
			if (!extendedMemory2) {
				// Replace title music
				*(u32*)0x02012930 = 0xE3A0000A; // mov r0, #0xA
				setBL(0x02012934, 0x02008528);
				*(u32*)0x02012938 = 0xE1A00000; // nop
				*(u32*)0x0201293C = 0xE1A00000; // nop
				*(u32*)0x02012940 = 0xE1A00000; // nop
				*(u32*)0x02012944 = 0xE1A00000; // nop
				*(u32*)0x02012964 = 0xE1A00000; // nop
				*(u32*)0x02012980 = 0xE1A00000; // nop
				*(u32*)0x02012DC4 = 0xE1A00000; // nop
				*(u32*)0x02012DE0 = 0xE1A00000; // nop
			}
			// Skip Manual screen
			if (!useSharedFont) {
				*(u32*)0x02013694 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				*(u32*)0x02013760 = 0xE1A00000; // nop
				*(u32*)0x02013768 = 0xE1A00000; // nop
				*(u32*)0x02013774 = 0xE1A00000; // nop
				*(u32*)0x02013E58 = 0xE3A06901; // mov r6, #0x4000
			}
			if (!extendedMemory2) {
				*(u32*)0x02013870 = 0xE3A00901; // mov r0, #0x4000 (Shrink sound heap from 1MB to 16KB: Disables music)
			}

			setBL(0x0201D008, (int)ce9->patches->rumble_arm9[0]); // Rumble when Luna gets shocked
			setBL(0x020275F8, (int)ce9->patches->rumble_arm9[1]); // Rumble when planet is destroyed
			*(u32*)0x02064FB0 = 0xE1A00000; // nop
			*(u32*)0x02068924 = 0xE1A00000; // nop
			patchInitDSiWare(0x0206EC5C, heapEnd);
			if (!extendedMemory2) {
				*(u32*)0x0206EFE8 = *(u32*)0x02004FE8;
			}
			patchUserSettingsReadDSiWare(0x020700CC);
			*(u32*)0x02070500 = 0xE1A00000; // nop
			*(u32*)0x02070504 = 0xE1A00000; // nop
			*(u32*)0x02070508 = 0xE1A00000; // nop
			*(u32*)0x0207050C = 0xE1A00000; // nop
			*(u32*)0x02070518 = 0xE1A00000; // nop (Enable error exception screen)
		} else {
			ce9->patches->rumble_arm9[0][3] = *(u32*)0x0201CFB0;
			ce9->patches->rumble_arm9[1][3] = *(u32*)0x0202750C;
			if (!extendedMemory2) {
				// Replace title music
				*(u32*)0x020128E4 = 0xE3A0000A; // mov r0, #0xA
				setBL(0x020128E8, 0x02008528);
				*(u32*)0x020128EC = 0xE1A00000; // nop
				*(u32*)0x020128F0 = 0xE1A00000; // nop
				*(u32*)0x020128F4 = 0xE1A00000; // nop
				*(u32*)0x020128F8 = 0xE1A00000; // nop
				*(u32*)0x02012918 = 0xE1A00000; // nop
				*(u32*)0x02012934 = 0xE1A00000; // nop
				*(u32*)0x02012D7C = 0xE1A00000; // nop
				*(u32*)0x02012D98 = 0xE1A00000; // nop
			}
			// Skip Manual screen
			if (!useSharedFont) {
				*(u32*)0x02013648 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
				*(u32*)0x02013714 = 0xE1A00000; // nop
				*(u32*)0x0201371C = 0xE1A00000; // nop
				*(u32*)0x02013728 = 0xE1A00000; // nop
				*(u32*)0x02013E04 = 0xE3A06901; // mov r6, #0x4000
			}
			if (!extendedMemory2) {
				*(u32*)0x02013824 = 0xE3A00901; // mov r0, #0x4000 (Shrink sound heap from 1MB to 16KB: Disables music)
			}

			setBL(0x0201CFB0, (int)ce9->patches->rumble_arm9[0]); // Rumble when Luna gets shocked
			setBL(0x0202750C, (int)ce9->patches->rumble_arm9[1]); // Rumble when planet is destroyed
			*(u32*)0x02064E34 = 0xE1A00000; // nop
			*(u32*)0x020687A8 = 0xE1A00000; // nop
			patchInitDSiWare(0x0206EAE0, heapEnd);
			if (!extendedMemory2) {
				*(u32*)0x0206EE6C = *(u32*)0x02004FE8;
			}
			patchUserSettingsReadDSiWare(0x0206FF50);
			*(u32*)0x02070384 = 0xE1A00000; // nop
			*(u32*)0x02070388 = 0xE1A00000; // nop
			*(u32*)0x0207038C = 0xE1A00000; // nop
			*(u32*)0x02070390 = 0xE1A00000; // nop
			*(u32*)0x0207039C = 0xE1A00000; // nop (Enable error exception screen)
		}
	}

	// Missy Mila Twisted Tales (Europe)
	else if (strcmp(romTid, "KM7P") == 0) {
		*(u32*)0x020151B8 = 0xE1A00000; // nop
		*(u32*)0x0201874C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201DC84, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x0201E010 = 0x02095240;
		}
		patchUserSettingsReadDSiWare(0x0201F350);
	}

	// Mixed Messages (USA)
	// Mixed Messages (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KMME") == 0 || strcmp(romTid, "KMMV") == 0) && extendedMemory2) {
		*(u32*)0x020036D8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020036DC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02006E74 = 0xE1A00000; // nop
		patchInitDSiWare(0x0200B870, heapEnd);
		patchUserSettingsReadDSiWare(0x0200CD44);
		*(u32*)0x0202A65C = 0xE1A00000; // nop
		*(u32*)0x02031A40 = 0xE3A00008; // mov r0, #8
		*(u32*)0x02031A44 = 0xE12FFF1E; // bx lr
		*(u32*)0x020337FC = 0xE1A00000; // nop
		*(u32*)0x02033B00 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Model Academy (Europe)
	else if (strcmp(romTid, "K8MP") == 0) {
		*(u32*)0x02005098 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02011A0C = 0xE1A00000; // nop
		*(u32*)0x02015788 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C8FC, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E058);
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

		// Skip Manual screen (Not working)
		// *(u32*)0x020B2800 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// *(u32*)0x020B2894 = 0xE1A00000; // nop
		// *(u32*)0x020B289C = 0xE1A00000; // nop
		// *(u32*)0x020B28A8 = 0xE1A00000; // nop
	}

	// Monster Buster Club (USA)
	else if (strcmp(romTid, "KXBE") == 0) {
		/* *(u32*)0x0207F058 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F05C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F138 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F13C = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F4F8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F4FC = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F63C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F640 = 0xE12FFF1E; // bx lr */
		*(u32*)0x0207F098 = 0xE1A00000; // nop
		*(u32*)0x0207F0B8 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x0207F17C = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0207F224 = 0xE1A00000; // nop
		setBL(0x0207F244, (u32)dsiSaveCreate);
		setBL(0x0207F254, (u32)dsiSaveOpen);
		*(u32*)0x0207F278 = 0xE1A00000; // nop
		setBL(0x0207F298, (u32)dsiSaveSetLength);
		setBL(0x0207F2B4, (u32)dsiSaveWrite);
		setBL(0x0207F2C8, (u32)dsiSaveClose);
		*(u32*)0x0207F2F0 = 0xE1A00000; // nop
		setBL(0x0207F494, (u32)dsiSaveOpen);
		setBL(0x0207F4C4, (u32)dsiSaveRead);
		setBL(0x0207F4D8, (u32)dsiSaveClose);
		*(u32*)0x0207F530 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F560 = 0xE3A00001; // mov r0, #1
		setBL(0x0207F584, 0x0207F5C4);
		*(u32*)0x0207F5E8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F654 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02094318 = 0xE1A00000; // nop
		tonccpy((u32*)0x02094EAC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02097954 = 0xE1A00000; // nop
		patchInitDSiWare(0x0209C858, heapEnd);
		patchUserSettingsReadDSiWare(0x0209DEA8);
		*(u32*)0x0209E2B0 = 0xE1A00000; // nop
		*(u32*)0x0209E2B4 = 0xE1A00000; // nop
		*(u32*)0x0209E2B8 = 0xE1A00000; // nop
		*(u32*)0x0209E2BC = 0xE1A00000; // nop
	}

	// Monster Buster Club (Europe)
	else if (strcmp(romTid, "KXBP") == 0) {
		/* *(u32*)0x0207EF64 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207EF68 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F044 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F048 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F414 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F418 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207F558 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F55C = 0xE12FFF1E; // bx lr */
		*(u32*)0x0207EFA4 = 0xE1A00000; // nop
		*(u32*)0x0207EFC4 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x0207F084 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x0207F12C = 0xE1A00000; // nop
		setBL(0x0207F14C, (u32)dsiSaveCreate);
		setBL(0x0207F15C, (u32)dsiSaveOpen);
		*(u32*)0x0207F180 = 0xE1A00000; // nop
		setBL(0x0207F1A0, (u32)dsiSaveSetLength);
		setBL(0x0207F1BC, (u32)dsiSaveWrite);
		setBL(0x0207F1D0, (u32)dsiSaveClose);
		*(u32*)0x0207F1F8 = 0xE1A00000; // nop
		setBL(0x0207F3AC, (u32)dsiSaveOpen);
		setBL(0x0207F3E0, (u32)dsiSaveRead);
		setBL(0x0207F3F4, (u32)dsiSaveClose);
		*(u32*)0x0207F44C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F47C = 0xE3A00001; // mov r0, #1
		setBL(0x0207F4A0, 0x0207F4E0);
		*(u32*)0x0207F504 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207F584 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0209424C = 0xE1A00000; // nop
		tonccpy((u32*)0x02094DE0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02097888 = 0xE1A00000; // nop
		patchInitDSiWare(0x0209C78C, heapEnd);
		patchUserSettingsReadDSiWare(0x0209DDDC);
		*(u32*)0x0209E1E4 = 0xE1A00000; // nop
		*(u32*)0x0209E1E8 = 0xE1A00000; // nop
		*(u32*)0x0209E1EC = 0xE1A00000; // nop
		*(u32*)0x0209E1F0 = 0xE1A00000; // nop
	}

	// Motto Me de Unou o Kitaeru: DS Sokudoku Jutsu Light (Japan)
	else if (strcmp(romTid, "K9SJ") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200E824 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F3A8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011D58 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016F60, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x020172EC = 0x0207AE60;
		}
		patchUserSettingsReadDSiWare(0x02018304);
		if (useSharedFont) {
			if (!extendedMemory2) {
				if (expansionPakFound) {
					*(u32*)0x02049B6C = 0xE1A00000; // nop
				}
				patchTwlFontLoad(0x02049C38, 0x02018858);
			}
		} else {
			*(u32*)0x0203F378 = 0xE1A00000; // nop (Skip Manual screen)
		}
		setBL(0x02047350, (u32)dsiSaveOpen);
		*(u32*)0x0204738C = 0xE1A00000; // nop
		setBL(0x0204739C, (u32)dsiSaveCreate);
		setBL(0x020473D0, (u32)dsiSaveGetLength);
		setBL(0x020473E0, (u32)dsiSaveRead);
		setBL(0x020473E8, (u32)dsiSaveClose);
		setBL(0x02047480, (u32)dsiSaveOpen);
		*(u32*)0x020474C0 = 0xE1A00000; // nop
		setBL(0x020474CC, (u32)dsiSaveCreate);
		setBL(0x02047504, (u32)dsiSaveOpen);
		setBL(0x02047548, (u32)dsiSaveSetLength);
		setBL(0x02047558, (u32)dsiSaveClose);
		setBL(0x02047578, (u32)dsiSaveWrite);
		setBL(0x02047580, (u32)dsiSaveClose);
	}

	// Mr. Brain (Japan)
	else if (strcmp(romTid, "KMBJ") == 0) {
		*(u32*)0x020054EC = 0xE1A00000; // nop
		*(u32*)0x02005504 = 0xE1A00000; // nop
		*(u32*)0x020057A0 = 0xE1A00000; // nop
		setBL(0x02005A40, (u32)dsiSaveOpen);
		setBL(0x02005A5C, (u32)dsiSaveGetLength);
		setBL(0x02005A78, (u32)dsiSaveRead);
		setBL(0x02005AA0, (u32)dsiSaveClose);
		setBL(0x02005AF8, (u32)dsiSaveCreate);
		setBL(0x02005B08, (u32)dsiSaveOpen);
		setBL(0x02005B34, (u32)dsiSaveSetLength);
		setBL(0x02005B54, (u32)dsiSaveWrite);
		setBL(0x02005B6C, (u32)dsiSaveClose);
		*(u32*)0x0200648C = 0xE1A00000; // nop
		*(u32*)0x0200949C = 0xE1A00000; // nop
		*(u32*)0x02025AB4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02026748, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02029804 = 0xE1A00000; // nop
		patchInitDSiWare(0x02032570, heapEndExceed);
		patchUserSettingsReadDSiWare(0x02033AC4);
	}

	// Mr. Driller: Drill Till You Drop (USA)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRE") == 0) {
		*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x02032410, (u32)dsiSaveOpen);
		setBL(0x02032428, (u32)dsiSaveClose);
		*(u32*)0x02032450 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02032470 = 0xE1A00000; // nop
		setBL(0x020324A8, (u32)dsiSaveCreate);
		setBL(0x020324CC, (u32)dsiSaveGetResultCode);
		setBL(0x0203277C, (u32)dsiSaveOpen);
		setBL(0x02032790, (u32)dsiSaveSeek);
		setBL(0x020327A0, (u32)dsiSaveRead);
		setBL(0x020327C0, (u32)dsiSaveClose);
		setBL(0x02032858, (u32)dsiSaveOpen);
		setBL(0x02032888, (u32)dsiSaveWrite);
		setBL(0x02032948, (u32)dsiSaveClose);
		*(u32*)0x02034DA4 = 0xE1A00000; // nop
		*(u32*)0x02036DA4 = 0xE1A00000; // nop
		*(u32*)0x0203AAB4 = 0xE1A00000; // nop
		*(u32*)0x0203AB34 = 0xE1A00000; // nop
		*(u32*)0x0203B1F4 = 0xE1A00000; // nop
		*(u32*)0x0203BC4C = 0xE1A00000; // nop
		*(u32*)0x0203BCE8 = 0xE1A00000; // nop
		*(u32*)0x0203BD9C = 0xE1A00000; // nop
		*(u32*)0x0203BE50 = 0xE1A00000; // nop
		*(u32*)0x0203BEF0 = 0xE1A00000; // nop
		*(u32*)0x0203BF70 = 0xE1A00000; // nop
		*(u32*)0x0203BFEC = 0xE1A00000; // nop
		*(u32*)0x0203C070 = 0xE1A00000; // nop
		*(u32*)0x0203C110 = 0xE1A00000; // nop
		*(u32*)0x0203C1CC = 0xE1A00000; // nop
		*(u32*)0x0203C308 = 0xE1A00000; // nop
		*(u32*)0x0203C36C = 0xE1A00000; // nop
		*(u32*)0x0203C434 = 0xE1A00000; // nop
		*(u32*)0x0203C4A4 = 0xE1A00000; // nop
		*(u32*)0x0203C530 = 0xE1A00000; // nop
		*(u32*)0x0203C5A0 = 0xE1A00000; // nop
		*(u32*)0x0203C628 = 0xE1A00000; // nop
		*(u32*)0x0203C698 = 0xE1A00000; // nop
		*(u32*)0x0203C7AC = 0xE1A00000; // nop
		*(u32*)0x0203C814 = 0xE1A00000; // nop
		*(u32*)0x0203C894 = 0xE1A00000; // nop
		*(u32*)0x0203C8F8 = 0xE1A00000; // nop
		*(u32*)0x0203C9B0 = 0xE1A00000; // nop
		*(u32*)0x0203CA20 = 0xE1A00000; // nop
		patchInitDSiWare(0x02040048, heapEnd);
		patchUserSettingsReadDSiWare(0x020417B8);
	}

	// Mr. Driller: Drill Till You Drop (Europe, Australia)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRV") == 0) {
		*(u32*)0x0201FEA0 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202009C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202030C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x02032160, (u32)dsiSaveOpen);
		setBL(0x02032178, (u32)dsiSaveClose);
		*(u32*)0x020321A0 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc) 
		*(u32*)0x020321C0 = 0xE1A00000; // nop
		setBL(0x020321F8, (u32)dsiSaveCreate);
		setBL(0x0203221C, (u32)dsiSaveGetResultCode);
		setBL(0x020324CC, (u32)dsiSaveOpen);
		setBL(0x020324E0, (u32)dsiSaveSeek);
		setBL(0x020324F0, (u32)dsiSaveRead);
		setBL(0x02032510, (u32)dsiSaveClose);
		setBL(0x020325A8, (u32)dsiSaveOpen);
		setBL(0x020325D8, (u32)dsiSaveWrite);
		setBL(0x02032698, (u32)dsiSaveClose);
		*(u32*)0x02034AF4 = 0xE1A00000; // nop
		*(u32*)0x02036B10 = 0xE1A00000; // nop
		*(u32*)0x0203A820 = 0xE1A00000; // nop
		*(u32*)0x0203A8A0 = 0xE1A00000; // nop
		*(u32*)0x0203AF60 = 0xE1A00000; // nop
		*(u32*)0x0203B9B8 = 0xE1A00000; // nop
		*(u32*)0x0203BA54 = 0xE1A00000; // nop
		*(u32*)0x0203BB08 = 0xE1A00000; // nop
		*(u32*)0x0203BBBC = 0xE1A00000; // nop
		*(u32*)0x0203BC5C = 0xE1A00000; // nop
		*(u32*)0x0203BCDC = 0xE1A00000; // nop
		*(u32*)0x0203BD58 = 0xE1A00000; // nop
		*(u32*)0x0203BDDC = 0xE1A00000; // nop
		*(u32*)0x0203BE7C = 0xE1A00000; // nop
		*(u32*)0x0203BF38 = 0xE1A00000; // nop
		*(u32*)0x0203C074 = 0xE1A00000; // nop
		*(u32*)0x0203C0D8 = 0xE1A00000; // nop
		*(u32*)0x0203C1A0 = 0xE1A00000; // nop
		*(u32*)0x0203C210 = 0xE1A00000; // nop
		*(u32*)0x0203C29C = 0xE1A00000; // nop
		*(u32*)0x0203C30C = 0xE1A00000; // nop
		*(u32*)0x0203C394 = 0xE1A00000; // nop
		*(u32*)0x0203C404 = 0xE1A00000; // nop
		*(u32*)0x0203C518 = 0xE1A00000; // nop
		*(u32*)0x0203C580 = 0xE1A00000; // nop
		*(u32*)0x0203C600 = 0xE1A00000; // nop
		*(u32*)0x0203C664 = 0xE1A00000; // nop
		*(u32*)0x0203C71C = 0xE1A00000; // nop
		*(u32*)0x0203C78C = 0xE1A00000; // nop
		patchInitDSiWare(0x0203FDB4, heapEnd);
		patchUserSettingsReadDSiWare(0x02041524);
	}

	// Sakutto Hamareru Hori Hori Action: Mr. Driller (Japan)
	// Saving not working due to weird code layout
	else if (strcmp(romTid, "KDRJ") == 0) {
		*(u32*)0x0201FE10 = 0xE1A00000; // nop (Disable NFTR font loading)
		*(u32*)0x0202000C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x0202027C = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		setBL(0x0203204C, (u32)dsiSaveOpen);
		setBL(0x02032064, (u32)dsiSaveClose);
		*(u32*)0x0203208C = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x020320AC = 0xE1A00000; // nop
		setBL(0x020320E4, (u32)dsiSaveCreate);
		setBL(0x02032108, (u32)dsiSaveGetResultCode);
		setBL(0x020323B8, (u32)dsiSaveOpen);
		setBL(0x020323CC, (u32)dsiSaveSeek);
		setBL(0x020323DC, (u32)dsiSaveRead);
		setBL(0x020323FC, (u32)dsiSaveClose);
		setBL(0x02032494, (u32)dsiSaveOpen);
		setBL(0x020324C4, (u32)dsiSaveWrite);
		setBL(0x02032584, (u32)dsiSaveClose);
		*(u32*)0x020349DC = 0xE1A00000; // nop
		*(u32*)0x0203697C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02036980 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203A8C8 = 0xE1A00000; // nop
		*(u32*)0x0203A950 = 0xE1A00000; // nop
		*(u32*)0x0203AFAC = 0xE1A00000; // nop
		*(u32*)0x0203DA98 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203DAF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203DB74 = 0xE12FFF1E; // bx lr
		patchInitDSiWare(0x020416B4, heapEnd);
		patchUserSettingsReadDSiWare(0x02042E48);
	}

	// Music on: Acoustic Guitar (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KG6E") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02007988 += 0xD0000000; // bne -> b
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x02012A88 = 0xE1A00000; // nop
				*(u32*)0x02012A8C = 0xE1A00000; // nop
				*(u32*)0x02012A90 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02008864 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008A10 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008A24 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200A3FC = 0xE1A00000; // nop
		*(u32*)0x0200A44C = 0xE1A00000; // nop
		*(u32*)0x0200A46C = 0xE1A00000; // nop
		*(u32*)0x0200A47C = 0xE1A00000; // nop
		*(u32*)0x0200A4A8 = 0xE1A00000; // nop

		*(u32*)0x0201C654 = 0xE1A00000; // nop
		*(u32*)0x0201FB94 = 0xE1A00000; // nop
		*(u32*)0x0201FFBC = 0xE1A00000; // nop
		*(u32*)0x0201FFC0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02024778, heapEnd);
		patchUserSettingsReadDSiWare(0x02025E00);
	}

	// Music on: Acoustic Guitar (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KG6V") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02007988 += 0xD0000000; // bne -> b
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x02012A0C = 0xE1A00000; // nop
				*(u32*)0x02012A10 = 0xE1A00000; // nop
				*(u32*)0x02012A14 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02008864 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008A34 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008A48 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200A3FC = 0xE1A00000; // nop
		*(u32*)0x0200A44C = 0xE1A00000; // nop
		*(u32*)0x0200A46C = 0xE1A00000; // nop
		*(u32*)0x0200A47C = 0xE1A00000; // nop
		*(u32*)0x0200A4A8 = 0xE1A00000; // nop

		*(u32*)0x0201C5D8 = 0xE1A00000; // nop
		*(u32*)0x0201FB18 = 0xE1A00000; // nop
		*(u32*)0x0201FF40 = 0xE1A00000; // nop
		*(u32*)0x0201FF44 = 0xE1A00000; // nop
		patchInitDSiWare(0x020246FC, heapEnd);
		patchUserSettingsReadDSiWare(0x02025D84);
	}

	// Music on: Drums (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KQDE") == 0) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x02017978 = 0xE1A00000; // nop
				*(u32*)0x0201797C = 0xE1A00000; // nop
				*(u32*)0x02017980 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x0200A158 = 0xE12FFF1E; // bx lr
			*(u32*)0x0200A304 = 0xE12FFF1E; // bx lr
			*(u32*)0x0200A318 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200BD58 = 0xE1A00000; // nop
		*(u32*)0x0200BDA8 = 0xE1A00000; // nop
		*(u32*)0x0200BDC8 = 0xE1A00000; // nop
		*(u32*)0x0200BDD8 = 0xE1A00000; // nop
		*(u32*)0x0200BE04 = 0xE1A00000; // nop

		*(u32*)0x02021C18 = 0xE1A00000; // nop
		*(u32*)0x020253C4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202A2B8, heapEnd);
		patchUserSettingsReadDSiWare(0x0202B940);
	}

	// Music on: Drums (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KQDV") == 0) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x020178C8 = 0xE1A00000; // nop
				*(u32*)0x020178CC = 0xE1A00000; // nop
				*(u32*)0x020178D0 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x0200A158 = 0xE12FFF1E; // bx lr
			*(u32*)0x0200A328 = 0xE12FFF1E; // bx lr
			*(u32*)0x0200A33C = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200BD7C = 0xE1A00000; // nop
		*(u32*)0x0200BDCC = 0xE1A00000; // nop
		*(u32*)0x0200BDEC = 0xE1A00000; // nop
		*(u32*)0x0200BDFC = 0xE1A00000; // nop
		*(u32*)0x0200BE28 = 0xE1A00000; // nop

		*(u32*)0x02021B68 = 0xE1A00000; // nop
		*(u32*)0x02025314 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202A208, heapEnd);
		patchUserSettingsReadDSiWare(0x0202B890);
	}

	// Anata no Raku Raku: Doramu Mashin (Japan)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KQDJ") == 0) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0201754C = 0xE1A00000; // nop
				*(u32*)0x02017550 = 0xE1A00000; // nop
				*(u32*)0x02017554 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x0200DE78 = 0xE12FFF1E; // bx lr
			*(u32*)0x0200E024 = 0xE12FFF1E; // bx lr
			*(u32*)0x0200E038 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200F318 = 0xE1A00000; // nop
		*(u32*)0x0200F368 = 0xE1A00000; // nop
		*(u32*)0x0200F388 = 0xE1A00000; // nop
		*(u32*)0x0200F398 = 0xE1A00000; // nop
		*(u32*)0x0200F3C4 = 0xE1A00000; // nop

		*(u32*)0x02021964 = 0xE1A00000; // nop
		*(u32*)0x020251A4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202A208, heapEnd);
		patchUserSettingsReadDSiWare(0x0202B8A0);
	}

	// Music on: Electric Guitar (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KIEE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02008B64 += 0xD0000000; // bne -> b
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0201517C = 0xE1A00000; // nop
				*(u32*)0x02015180 = 0xE1A00000; // nop
				*(u32*)0x02015184 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02009B20 = 0xE12FFF1E; // bx lr
			*(u32*)0x02009CCC = 0xE12FFF1E; // bx lr
			*(u32*)0x02009CE0 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200B648 = 0xE1A00000; // nop
		*(u32*)0x0200B698 = 0xE1A00000; // nop
		*(u32*)0x0200B6B8 = 0xE1A00000; // nop
		*(u32*)0x0200B6C8 = 0xE1A00000; // nop
		*(u32*)0x0200B6F4 = 0xE1A00000; // nop

		*(u32*)0x0201F518 = 0xE1A00000; // nop
		*(u32*)0x02022A58 = 0xE1A00000; // nop
		*(u32*)0x02023014 = 0xE1A00000; // nop
		*(u32*)0x02023018 = 0xE1A00000; // nop
		patchInitDSiWare(0x020277D0, heapEnd);
		patchUserSettingsReadDSiWare(0x02028E58);
	}

	// Music on: Electric Guitar (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KIEV") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02008B64 += 0xD0000000; // bne -> b
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x02015124 = 0xE1A00000; // nop
				*(u32*)0x02015128 = 0xE1A00000; // nop
				*(u32*)0x0201512C = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02009B20 = 0xE12FFF1E; // bx lr
			*(u32*)0x02009CF0 = 0xE12FFF1E; // bx lr
			*(u32*)0x02009D04 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200B66C = 0xE1A00000; // nop
		*(u32*)0x0200B6BC = 0xE1A00000; // nop
		*(u32*)0x0200B6DC = 0xE1A00000; // nop
		*(u32*)0x0200B6EC = 0xE1A00000; // nop
		*(u32*)0x0200B718 = 0xE1A00000; // nop

		*(u32*)0x0201F4C0 = 0xE1A00000; // nop
		*(u32*)0x02022A00 = 0xE1A00000; // nop
		*(u32*)0x02022FBC = 0xE1A00000; // nop
		*(u32*)0x02022FC0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02027778, heapEnd);
		patchUserSettingsReadDSiWare(0x02028E00);
	}

	// Anata no Rakuraku: Erekutorikku Gita (Japan)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KIEJ") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02008D94 += 0xD0000000; // bne -> b
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x02015304 = 0xE1A00000; // nop
				*(u32*)0x02015308 = 0xE1A00000; // nop
				*(u32*)0x0201530C = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02009CC4 = 0xE12FFF1E; // bx lr
			*(u32*)0x02009E70 = 0xE12FFF1E; // bx lr
			*(u32*)0x02009E84 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x0200B80C = 0xE1A00000; // nop
		*(u32*)0x0200B85C = 0xE1A00000; // nop
		*(u32*)0x0200B87C = 0xE1A00000; // nop
		*(u32*)0x0200B88C = 0xE1A00000; // nop
		*(u32*)0x0200B8B8 = 0xE1A00000; // nop

		*(u32*)0x0201F6A0 = 0xE1A00000; // nop
		*(u32*)0x02022C74 = 0xE1A00000; // nop
		*(u32*)0x02023258 = 0xE1A00000; // nop
		*(u32*)0x0202325C = 0xE1A00000; // nop
		patchInitDSiWare(0x02027B5C, heapEnd);
		patchUserSettingsReadDSiWare(0x020291F4);
	}

	// Music on: Electronic Keyboard (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KK7E") == 0) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0200DE80 = 0xE1A00000; // nop
				*(u32*)0x0200DE84 = 0xE1A00000; // nop
				*(u32*)0x0200DE88 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02008850 = 0xE12FFF1E; // bx lr
			*(u32*)0x020089D0 = 0xE12FFF1E; // bx lr
			*(u32*)0x020089E4 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x02007F68 = 0xE1A00000; // nop
		*(u32*)0x02007FC4 = 0xE1A00000; // nop
		*(u32*)0x02007FF8 = 0xE1A00000; // nop
		*(u32*)0x02008004 = 0xE1A00000; // nop
		*(u32*)0x02008038 = 0xE1A00000; // nop

		*(u32*)0x02017CDC = 0xE1A00000; // nop
		*(u32*)0x0201B1BC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FCF0, heapEnd);
		patchUserSettingsReadDSiWare(0x02021314);
	}

	// Music on: Electronic Keyboard (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KK7V") == 0) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0200DE08 = 0xE1A00000; // nop
				*(u32*)0x0200DE0C = 0xE1A00000; // nop
				*(u32*)0x0200DE10 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02008850 = 0xE12FFF1E; // bx lr
			*(u32*)0x020089D0 = 0xE12FFF1E; // bx lr
			*(u32*)0x020089E4 = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x02007F68 = 0xE1A00000; // nop
		*(u32*)0x02007FC4 = 0xE1A00000; // nop
		*(u32*)0x02007FF8 = 0xE1A00000; // nop
		*(u32*)0x02008004 = 0xE1A00000; // nop
		*(u32*)0x02008038 = 0xE1A00000; // nop

		*(u32*)0x02017C74 = 0xE1A00000; // nop
		*(u32*)0x0201B154 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FC88, heapEnd);
		patchUserSettingsReadDSiWare(0x020212AC);
	}

	// Anata no Rakuraku: Erekutoronikku Kibodo (Japan)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KK7J") == 0) {
		useSharedFont = twlFontFound;
		if (useSharedFont) {
			if (!extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0200E210 = 0xE1A00000; // nop
				*(u32*)0x0200E214 = 0xE1A00000; // nop
				*(u32*)0x0200E218 = 0xE1A00000; // nop
			}
		} else {
			// Skip Manual screen
			*(u32*)0x02008BF8 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008D78 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008D8C = 0xE12FFF1E; // bx lr
		}

		*(u32*)0x02008080 = 0xE1A00000; // nop
		*(u32*)0x020080DC = 0xE1A00000; // nop
		*(u32*)0x02008110 = 0xE1A00000; // nop
		*(u32*)0x0200811C = 0xE1A00000; // nop
		*(u32*)0x02008150 = 0xE1A00000; // nop

		*(u32*)0x02017EF8 = 0xE1A00000; // nop
		*(u32*)0x0201B3D8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020038, heapEnd);
		patchUserSettingsReadDSiWare(0x0202165C);
	}

	// Music on: Learning Piano (USA)
	// Music on: Learning Piano (Europe)
	// Saving is difficult to implement
	else if (strcmp(romTid, "K88E") == 0 || strcmp(romTid, "K88P") == 0) {
		*(u32*)0x0200ADF8 += 0xD0000000; // bne -> b

		// Skip Manual screen
		*(u32*)0x0200C91C = 0xE1A00000; // nop
		*(u32*)0x0200C920 = 0xE1A00000; // nop
		*(u32*)0x0200C924 = 0xE1A00000; // nop

		*(u32*)0x0200DD88 = 0xE1A00000; // nop
		*(u32*)0x0200DDD8 = 0xE1A00000; // nop
		*(u32*)0x0200DDF8 = 0xE1A00000; // nop
		*(u32*)0x0200DE08 = 0xE1A00000; // nop
		*(u32*)0x0200DE34 = 0xE1A00000; // nop
		if (romTid[3] == 'E') {
			*(u32*)0x0201D684 = 0xE1A00000; // nop
			*(u32*)0x02020B58 = 0xE1A00000; // nop
			*(u32*)0x02021058 = 0xE1A00000; // nop
			*(u32*)0x0202105C = 0xE1A00000; // nop
			patchInitDSiWare(0x02025BA0, heapEnd);
			patchUserSettingsReadDSiWare(0x02027204);
		} else {
			*(u32*)0x0201D5A0 = 0xE1A00000; // nop
			*(u32*)0x02020A74 = 0xE1A00000; // nop
			*(u32*)0x02020F74 = 0xE1A00000; // nop
			*(u32*)0x02020F78 = 0xE1A00000; // nop
			patchInitDSiWare(0x02025ABC, heapEnd);
			patchUserSettingsReadDSiWare(0x02027120);
		}
	}

	// Music on: Learning Piano Vol. 2 (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KI7E") == 0) {
		*(u32*)0x0200AEBC += 0xD0000000; // bne -> b

		// Skip Manual screen
		*(u32*)0x0200C9F8 = 0xE1A00000; // nop
		*(u32*)0x0200C9FC = 0xE1A00000; // nop
		*(u32*)0x0200CA00 = 0xE1A00000; // nop

		*(u32*)0x0200DE64 = 0xE1A00000; // nop
		*(u32*)0x0200DEB4 = 0xE1A00000; // nop
		*(u32*)0x0200DED4 = 0xE1A00000; // nop
		*(u32*)0x0200DEE4 = 0xE1A00000; // nop
		*(u32*)0x0200DF10 = 0xE1A00000; // nop
		*(u32*)0x0201CD5C = 0xE1A00000; // nop
		*(u32*)0x020202C4 = 0xE1A00000; // nop
		*(u32*)0x020207EC = 0xE1A00000; // nop
		*(u32*)0x020207F0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025358, heapEnd);
		patchUserSettingsReadDSiWare(0x020269CC);
	}

	// Music on: Learning Piano Vol. 2 (Europe)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KI7P") == 0) {
		*(u32*)0x0200B090 += 0xD0000000; // bne -> b

		// Skip Manual screen
		*(u32*)0x0200CC40 = 0xE1A00000; // nop
		*(u32*)0x0200CC44 = 0xE1A00000; // nop
		*(u32*)0x0200CC48 = 0xE1A00000; // nop

		*(u32*)0x0200E0AC = 0xE1A00000; // nop
		*(u32*)0x0200E0FC = 0xE1A00000; // nop
		*(u32*)0x0200E11C = 0xE1A00000; // nop
		*(u32*)0x0200E12C = 0xE1A00000; // nop
		*(u32*)0x0200E158 = 0xE1A00000; // nop
		*(u32*)0x0201CFFC = 0xE1A00000; // nop
		*(u32*)0x02020564 = 0xE1A00000; // nop
		*(u32*)0x02020A8C = 0xE1A00000; // nop
		*(u32*)0x02020A90 = 0xE1A00000; // nop
		patchInitDSiWare(0x020255F8, heapEnd);
		patchUserSettingsReadDSiWare(0x020269CC);
	}

	// Music on: Playing Piano (USA)
	// Music on: Playing Piano (Europe)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KICE") == 0 || strcmp(romTid, "KICP") == 0) {
		*(u32*)0x0200AFCC += 0xD0000000; // bne -> b
		if (romTid[3] == 'E') {
			// Skip Manual screen
			*(u32*)0x0200CC58 = 0xE1A00000; // nop
			*(u32*)0x0200CC5C = 0xE1A00000; // nop
			*(u32*)0x0200CC60 = 0xE1A00000; // nop

			*(u32*)0x0200E08C = 0xE1A00000; // nop
			*(u32*)0x0200E0DC = 0xE1A00000; // nop
			*(u32*)0x0200E0FC = 0xE1A00000; // nop
			*(u32*)0x0200E10C = 0xE1A00000; // nop
			*(u32*)0x0200E138 = 0xE1A00000; // nop
			*(u32*)0x0201D9F4 = 0xE1A00000; // nop
			*(u32*)0x02020EC8 = 0xE1A00000; // nop
			*(u32*)0x020213C8 = 0xE1A00000; // nop
			*(u32*)0x020213CC = 0xE1A00000; // nop
			patchInitDSiWare(0x02025F10, heapEnd);
			patchUserSettingsReadDSiWare(0x02027574);
		} else {
			// Skip Manual screen
			*(u32*)0x0200CC7C = 0xE1A00000; // nop
			*(u32*)0x0200CC80 = 0xE1A00000; // nop
			*(u32*)0x0200CC84 = 0xE1A00000; // nop

			*(u32*)0x0200E0B0 = 0xE1A00000; // nop
			*(u32*)0x0200E100 = 0xE1A00000; // nop
			*(u32*)0x0200E120 = 0xE1A00000; // nop
			*(u32*)0x0200E130 = 0xE1A00000; // nop
			*(u32*)0x0200E15C = 0xE1A00000; // nop
			*(u32*)0x0201D964 = 0xE1A00000; // nop
			*(u32*)0x02020E38 = 0xE1A00000; // nop
			*(u32*)0x02021338 = 0xE1A00000; // nop
			*(u32*)0x0202133C = 0xE1A00000; // nop
			patchInitDSiWare(0x02025E80, heapEnd);
			patchUserSettingsReadDSiWare(0x020274E4);
		}
	}

	// Music on: Retro Keyboard (USA)
	// Music on: Retro Keyboard (Europe, Australia)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KRHE") == 0 || strcmp(romTid, "KRHV") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200833C = 0xE1A00000; // nop
		*(u32*)0x02008398 = 0xE1A00000; // nop
		*(u32*)0x020083CC = 0xE1A00000; // nop
		*(u32*)0x020083D8 = 0xE1A00000; // nop
		*(u32*)0x0200840C = 0xE1A00000; // nop

		if (!useSharedFont) {
			// Skip Manual screen
			*(u32*)0x02008CD0 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008E50 = 0xE12FFF1E; // bx lr
			*(u32*)0x02008E64 = 0xE12FFF1E; // bx lr
		}

		if (romTid[3] == 'E') {
			if (useSharedFont && !extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0200E314 = 0xE1A00000; // nop
				*(u32*)0x0200E318 = 0xE1A00000; // nop
				*(u32*)0x0200E31C = 0xE1A00000; // nop
			}
			*(u32*)0x02018180 = 0xE1A00000; // nop
			*(u32*)0x0201B660 = 0xE1A00000; // nop
			patchInitDSiWare(0x02020194, heapEnd);
			patchUserSettingsReadDSiWare(0x020217B8);
		} else {
			if (useSharedFont && !extendedMemory2) {
				// Skip Manual screen when past title screen
				*(u32*)0x0200E29C = 0xE1A00000; // nop
				*(u32*)0x0200E2A0 = 0xE1A00000; // nop
				*(u32*)0x0200E2A4 = 0xE1A00000; // nop
			}
			*(u32*)0x02018114 = 0xE1A00000; // nop
			*(u32*)0x0201B5F4 = 0xE1A00000; // nop
			patchInitDSiWare(0x02020128, heapEnd);
			patchUserSettingsReadDSiWare(0x0202174C);
		}
	}

	// My Aquarium: Seven Oceans (USA)
	// My Aquarium: Seven Oceans (Europe)
	// Kiwami Birei Akuariumu: Sekai no Sakana to Kujiratachi (Japan)
	else if (strcmp(romTid, "K7ZE") == 0 || strcmp(romTid, "K9RP") == 0 || strcmp(romTid, "K9RJ") == 0) {
		u8 offsetChange = (romTid[3] == 'J') ? 0x20 : 0;

		if (romTid[3] != 'J') {
			*(u32*)0x02005094 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020051B0 = 0xE1A00000; // nop
		}
		*(u32*)(0x0200FE24-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0201309C-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201ABFC-offsetChange, heapEnd);
		*(u32*)(0x0201AF88-offsetChange) = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201C1A0-offsetChange);
		*(u32*)(0x02021A88-offsetChange) = 0xE3A00001; // mov r0, #1
		*(u32*)(0x02021A8C-offsetChange) = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
			setBL(0x0203DC00, (u32)dsiSaveOpen);
			*(u32*)0x0203DC10 = 0xE1A00000; // nop
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
			*(u32*)0x0206DA88 = 0xE3A01001; // mov r1, #1 (Skip Manual screen)
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
		} else if (romTid[3] == 'P') {
			setBL(0x0203DC4C, (u32)dsiSaveOpen);
			*(u32*)0x0203DC5C = 0xE1A00000; // nop
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
			*(u32*)0x0206DAA0 = 0xE3A01001; // mov r1, #1 (Skip Manual screen)
		} else {
			setBL(0x0203D7BC, (u32)dsiSaveOpen);
			*(u32*)0x0203D7CC = 0xE1A00000; // nop
			setBL(0x0203D7D8, (u32)dsiSaveCreate);
			setBL(0x0203D804, (u32)dsiSaveOpen);
			setBL(0x0203D81C, (u32)dsiSaveRead);
			setBL(0x0203D838, (u32)dsiSaveClose);
			setBL(0x0203D850, (u32)dsiSaveClose);
			setBL(0x0203D8FC, (u32)dsiSaveOpen);
			setBL(0x0203D914, (u32)dsiSaveClose);
			setBL(0x0203D950, (u32)dsiSaveSeek);
			setBL(0x0203D964, (u32)dsiSaveRead);
			setBL(0x0203D98C, (u32)dsiSaveClose);
			setBL(0x0203D9CC, (u32)dsiSaveClose);
			setBL(0x0203DA20, (u32)dsiSaveClose);
			setBL(0x0203DA58, (u32)dsiSaveClose);
			setBL(0x0203DB68, (u32)dsiSaveOpen);
			setBL(0x0203DB94, (u32)dsiSaveWrite);
			setBL(0x0203DB9C, (u32)dsiSaveClose);
			setBL(0x020443C0, 0x02011364); // Branch to fixed code
			*(u32*)0x0206BE58 = 0xE3A01001; // mov r1, #1 (Skip Manual screen)
			*(u32*)0x0206C0C0 = 0xE1A00000; // nop
			*(u32*)0x0206C0D0 = 0xE1A00000; // nop

			// Fixed code with added branch to save write code
			*(u32*)0x02011364 = 0xE92D4003; // STMFD SP!, {R0-R1,LR}
			*(u32*)0x02011368 = 0xE5901008; // ldr r1, [r0,#8]
			*(u32*)0x0201136C = 0xE5910004; // ldr r0, [r1,#4]
			*(u32*)0x02011370 = 0xE3500005; // cmp r0, #5
			*(u32*)0x02011374 = 0x13700001; // cmpne r0, #1
			*(u32*)0x02011378 = 0x03E00001; // moveq r0, #0xFFFFFFFE
			*(u32*)0x0201137C = 0x05810004; // streq r0, [r1,#4]
			*(u32*)0x02011380 = 0x18BD8003; // LDMNEFD SP!, {R0-R1,PC}
			setBL(0x02011384, 0x0206C0A8);
			*(u32*)0x02011388 = 0xE8BD8003; // LDMFD SP!, {R0-R1,PC}
		}
	}

	// My Farm (USA)
	// My Exotic Farm (USA)
	// My Exotic Farm (Europe, Australia)
	else if (strcmp(romTid, "KMRE") == 0 || strcmp(romTid, "KMVE") == 0 || strcmp(romTid, "KMVV") == 0) {
		*(u32*)0x02011B64 = 0xE1A00000; // nop
		tonccpy((u32*)0x020126DC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02014FBC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B740, heapEnd);
		patchUserSettingsReadDSiWare(0x0201CD38);
		*(u32*)0x0201CD54 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201CD58 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201CD60 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201CD64 = 0xE12FFF1E; // bx lr
		if (strcmp(romTid, "KMRE") == 0) {
			*(u32*)0x020579FC = 0xE1A00000; // nop
		} else {
			*(u32*)0x02057A00 = 0xE1A00000; // nop
		}
		if (strcmp(romTid, "KMRE") == 0) {
			*(u32*)0x0205D0BC = 0xE1A00000; // nop
			*(u32*)0x0205D0DC = 0xE1A00000; // nop
			*(u32*)0x0205D0E8 = 0xE1A00000; // nop
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
			*(u32*)0x0207A990 = 0xE1A00000; // nop
			*(u32*)0x0207AAA8 = 0xE1A00000; // nop
			*(u32*)0x0207EEB4 = 0xE1A00000; // nop
		} else if (strcmp(romTid, "KMVE") == 0) {
			*(u32*)0x0205D0C0 = 0xE1A00000; // nop
			*(u32*)0x0205D0E0 = 0xE1A00000; // nop
			*(u32*)0x0205D0EC = 0xE1A00000; // nop
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
			*(u32*)0x0207A998 = 0xE1A00000; // nop
			*(u32*)0x0207AAB0 = 0xE1A00000; // nop
			*(u32*)0x0207EEBC = 0xE1A00000; // nop
		} else if (strcmp(romTid, "KMVV") == 0) {
			*(u32*)0x0205D068 = 0xE1A00000; // nop
			*(u32*)0x0205D088 = 0xE1A00000; // nop
			*(u32*)0x0205D094 = 0xE1A00000; // nop
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
			*(u32*)0x0207A970 = 0xE1A00000; // nop
			*(u32*)0x0207AA88 = 0xE1A00000; // nop
			*(u32*)0x0207EE94 = 0xE1A00000; // nop
		}
	}

	// My Farm (Europe, Australia)
	else if (strcmp(romTid, "KMRV") == 0) {
		*(u32*)0x02011A90 = 0xE1A00000; // nop
		tonccpy((u32*)0x02012608, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02014EE8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B66C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201CC64);
		*(u32*)0x0201CC80 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201CC84 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201CC8C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201CC90 = 0xE12FFF1E; // bx lr
		*(u32*)0x02057928 = 0xE1A00000; // nop
		*(u32*)0x0205CF90 = 0xE1A00000; // nop
		*(u32*)0x0205CFB0 = 0xE1A00000; // nop
		*(u32*)0x0205CFBC = 0xE1A00000; // nop
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
		*(u32*)0x0207A894 = 0xE1A00000; // nop
		*(u32*)0x0207A9AC = 0xE1A00000; // nop
		*(u32*)0x0207EDB8 = 0xE1A00000; // nop
	}

	// My Asian Farm (USA)
	// My Asian Farm (Europe)
	// My Australian Farm (USA)
	// My Australian Farm (Europe)
	else if (strcmp(romTid, "KL3E") == 0 || strcmp(romTid, "KL3P") == 0 || strcmp(romTid, "KL4E") == 0 || strcmp(romTid, "KL4P") == 0) {
		*(u32*)0x02011B88 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201270C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02015074 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B830, heapEnd);
		patchUserSettingsReadDSiWare(0x0201CE38);
		*(u32*)0x0201CE54 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201CE58 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201CE60 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201CE64 = 0xE12FFF1E; // bx lr
		*(u32*)0x0205421C = 0xE1A00000; // nop
		if (romTid[3] == 'E') {
			*(u32*)0x02059C64 = 0xE1A00000; // nop
			*(u32*)0x02059C84 = 0xE1A00000; // nop
			*(u32*)0x02059C90 = 0xE1A00000; // nop
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
			*(u32*)0x02077D40 = 0xE1A00000; // nop
			*(u32*)0x02077E58 = 0xE1A00000; // nop
			*(u32*)0x0207C244 = 0xE1A00000; // nop
		} else {
			*(u32*)0x02059C80 = 0xE1A00000; // nop
			*(u32*)0x02059CA0 = 0xE1A00000; // nop
			*(u32*)0x02059CAC = 0xE1A00000; // nop
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
			*(u32*)0x02077D8C = 0xE1A00000; // nop
			*(u32*)0x02077EA4 = 0xE1A00000; // nop
			*(u32*)0x0207C290 = 0xE1A00000; // nop
		}
	}

	// My Little Restaurant (USA)
	// Requires either 8MB of RAM or Memory Expansion Pak
	// Audio does not play on retail consoles
	// Crashes after intro finishes due to weird bug
	/*else if (strcmp(romTid, "KLTE") == 0 && debugOrMep) {
		extern u16* rmtRacersHeapAlloc;

		doubleNopT(0x0200FA6C);
		doubleNopT(0x0200FA72);
		*(u32*)0x0200FB34 = 0xE1A00000; // nop
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
		*(u32*)0x02023864 = 0xE1A00000; // nop
		*(u32*)0x02026C18 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202D0A8, heapEnd);
		*(u32*)0x0202D434 -= 0x30000;
		patchUserSettingsReadDSiWare(0x0202E5A0);
		*(u32*)0x02041468 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Disable audio
			*(u32*)0x020186DC = 0xE1A00000; // bx lr
			*(u32*)0x02041B18 = 0xE1A00000; // nop

			tonccpy((u32*)0x0202EBA4, rmtRacersHeapAlloc, 0xC0);
			setBLThumb(0x0205406C, 0x0202EBA4);
		}
		*(u32*)0x02041BD0 = 0xE12FFF1E; // bx lr
	}*/

	// Nandoku 500 Kanji: Wado Pazuru (Japan)
	else if (strcmp(romTid, "KJWJ") == 0) {
		*(u32*)0x02011E00 = 0xE1A00000; // nop
		setBL(0x0201200C, (u32)dsiSaveClose);
		*(u32*)0x02012070 = 0xE1A00000; // nop
		setBL(0x02012150, (u32)dsiSaveClose);
		setBL(0x020122EC, (u32)dsiSaveOpen);
		setBL(0x02012314, (u32)dsiSaveSeek);
		setBL(0x02012330, (u32)dsiSaveClose);
		setBL(0x02012348, (u32)dsiSaveRead);
		setBL(0x02012368, (u32)dsiSaveClose);
		setBL(0x02012378, (u32)dsiSaveClose);
		setBL(0x020123B4, (u32)dsiSaveOpen);
		setBL(0x020123CC, (u32)dsiSaveSeek);
		setBL(0x020123E4, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02012418, (u32)dsiSaveOpen);
		setBL(0x02012438, (u32)dsiSaveSetLength);
		setBL(0x02012448, (u32)dsiSaveClose);
		setBL(0x02012464, (u32)dsiSaveSeek);
		setBL(0x02012480, (u32)dsiSaveClose);
		setBL(0x02012498, (u32)dsiSaveWrite);
		setBL(0x020124BC, (u32)dsiSaveClose);
		setBL(0x020124C8, (u32)dsiSaveClose);
		setBL(0x02012504, (u32)dsiSaveOpen);
		setBL(0x02012518, (u32)dsiSaveSetLength);
		setBL(0x02012530, (u32)dsiSaveSeek);
		setBL(0x02012548, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201259C, (u32)dsiSaveCreate);
		setBL(0x020125A4, (u32)dsiSaveGetResultCode);
		*(u32*)0x02021FDC = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		*(u32*)0x020299C8 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x02033EE0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02036918 = 0xE1A00000; // nop
		*(u32*)0x02039F44 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203FB88, heapEnd);
		patchUserSettingsReadDSiWare(0x020412E8);
	}

	// Jeulgeoun Ilboneo Wodeupeojeul (Korea)
	else if (strcmp(romTid, "KJWK") == 0) {
		*(u32*)0x0200BEB4 = 0xE1A00000; // nop
		*(u32*)0x0200F4E0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02015108, heapEnd);
		patchUserSettingsReadDSiWare(0x02016868);
		*(u32*)0x02023B84 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02023F24 = 0xE1A00000; // nop
		setBL(0x02024130, (u32)dsiSaveClose);
		*(u32*)0x02024194 = 0xE1A00000; // nop
		setBL(0x02024274, (u32)dsiSaveClose);
		setBL(0x02024410, (u32)dsiSaveOpen);
		setBL(0x02024438, (u32)dsiSaveSeek);
		setBL(0x02024454, (u32)dsiSaveClose);
		setBL(0x0202446C, (u32)dsiSaveRead);
		setBL(0x0202448C, (u32)dsiSaveClose);
		setBL(0x0202449C, (u32)dsiSaveClose);
		setBL(0x020244D8, (u32)dsiSaveOpen);
		setBL(0x020244F0, (u32)dsiSaveSeek);
		setBL(0x02024508, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0202453C, (u32)dsiSaveOpen);
		setBL(0x0202455C, (u32)dsiSaveSetLength);
		setBL(0x0202456C, (u32)dsiSaveClose);
		setBL(0x02024588, (u32)dsiSaveSeek);
		setBL(0x020245A4, (u32)dsiSaveClose);
		setBL(0x020245BC, (u32)dsiSaveWrite);
		setBL(0x020245E0, (u32)dsiSaveClose);
		setBL(0x020245EC, (u32)dsiSaveClose);
		setBL(0x02024628, (u32)dsiSaveOpen);
		setBL(0x0202463C, (u32)dsiSaveSetLength);
		setBL(0x02024654, (u32)dsiSaveSeek);
		setBL(0x0202466C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x020246C0, (u32)dsiSaveCreate);
		setBL(0x020246C8, (u32)dsiSaveGetResultCode);
		*(u32*)0x020321C4 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x020398EC = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
	}

	// Nazo no Mini Game (Japan)
	else if (strcmp(romTid, "KN3J") == 0) {
		*(u32*)0x020118F0 = 0xE1A00000; // nop
		*(u32*)0x020152AC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AE04, heapEnd);
		*(u32*)0x0201B190 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201C2E0);
		if (!extendedMemory2) {
			// Disable music
			*(u32*)0x020213A4 = 0xE3A0080F; // mov r0, #0xF0000
			*(u32*)0x02023E28 = 0xE3A0180F; // mov r1, #0xF0000
		}
		*(u32*)0x020214BC = 0xE1A00000; // nop
		*(u32*)0x020355E8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0203FBB0, (u32)dsiSaveOpen);
		setBL(0x0203FBD4, (u32)dsiSaveGetLength);
		setBL(0x0203FBF8, (u32)dsiSaveRead);
		setBL(0x0203FC00, (u32)dsiSaveClose);
		setBL(0x0203FC44, (u32)dsiSaveCreate);
		setBL(0x0203FC54, (u32)dsiSaveOpen);
		setBL(0x0203FC64, (u32)dsiSaveGetResultCode);
		setBL(0x0203FC84, (u32)dsiSaveSetLength);
		setBL(0x0203FC94, (u32)dsiSaveWrite);
		setBL(0x0203FC9C, (u32)dsiSaveClose);
		*(u32*)0x0203FCDC = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0203FD04 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x0203FD2C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0203FD40 = 0xE3A00001; // mov r0, #1
		setBL(0x0203FD84, (u32)dsiSaveCheckExists);
		*(u32*)0x0203FDB8 = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		*(u32*)0x0203FDC8 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x0203FE20 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0203FE3C = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x0203FE64 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0203FE78 = 0xE3A00001; // mov r0, #1
		setBL(0x0203FEB4, (u32)dsiSaveDelete);
		*(u32*)0x0203FEC8 = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		*(u32*)0x0203FED8 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x02040290 = 0xE1A00000; // nop
	}

	// Need for Speed: Nitro-X (USA)
	// Need for Speed: Nitro-X (Europe, Australia)
	// Requires 8MB of RAM
	else if (strncmp(romTid, "KNP", 3) == 0 && extendedMemory2) {
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

		doubleNopT(0x0200511E);
		doubleNopT(0x02005124);
		doubleNopT(0x02005272);
		*(u32*)0x0201BA24 = 0xE1A00000; // nop
		//tonccpy((u32*)0x0201C5A8, dsiSaveGetResultCode, 0xC);
		//tonccpy((u32*)0x0201D178, dsiSaveSetLength, 0xC);
		*(u32*)0x0201F744 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202EC54, heapEnd);
		patchUserSettingsReadDSiWare(0x02030430);
		*(u32*)0x0203044C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02030450 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203046C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02030470 = 0xE12FFF1E; // bx lr
		*(u16*)0x020EBFC4 = 0x4770; // bx lr
		/*setBLThumb(0x020EBFDC, dsiSaveOpenT);
		setBLThumb(0x020EBFEA, dsiSaveCloseT);
		setBLThumb(0x020EBFF6, dsiSaveGetInfoT);
		doubleNopT(0x020EC00A);
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
		doubleNopT(0x020EC106);
		setBLThumb(0x020EC118, dsiSaveCreateT);
		setBLThumb(0x020EC126, dsiSaveOpenT);
		setBLThumb(0x020EC136, dsiSaveCloseT);
		setBLThumb(0x020EC14C, dsiSaveOpenT);
		setBLThumb(0x020EC162, dsiSaveSeekT);
		setBLThumb(0x020EC176, dsiSaveReadT);
		setBLThumb(0x020EC17E, dsiSaveCloseT);*/
	}

	// Neko Neko Bakery: Pan de Pazurunya! (Japan)
	else if (strcmp(romTid, "K9NJ") == 0) {
		*(u32*)0x020050C0 = 0xE1A00000; // nop
		*(u32*)0x0200B75C = 0xE1A00000; // nop
		*(u32*)0x0200EB6C = 0xE1A00000; // nop
		patchInitDSiWare(0x020139E0, heapEnd);
		patchUserSettingsReadDSiWare(0x02014EBC);
		if (!extendedMemory2) {
			*(u32*)0x0201A89C = 0xE3A00B65; // mov r0, #0x19400 (Do not pre-load streamed music files, disable them instead)
		}
		setBL(0x0203E844, (u32)dsiSaveOpen);
		setBL(0x0203E858, (u32)dsiSaveGetLength);
		setBL(0x0203E880, (u32)dsiSaveSeek);
		setBL(0x0203E890, (u32)dsiSaveRead);
		setBL(0x0203E89C, (u32)dsiSaveClose);
		setBL(0x0203E95C, (u32)dsiSaveCreate);
		setBL(0x0203E970, (u32)dsiSaveOpen);
		setBL(0x0203E988, (u32)dsiSaveSeek);
		setBL(0x0203E998, (u32)dsiSaveWrite);
		setBL(0x0203E9A4, (u32)dsiSaveClose);
	}

	// Neko Reversi (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KNVJ") == 0 && extendedMemory2) {
		*(u32*)0x02010A04 = 0xE1A00000; // nop
		*(u32*)0x020140A8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019874, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AE44);
		*(u32*)0x0203FCA4 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Nintendo Countdown Calendar (USA)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KAUE") == 0 && debugOrMep) {
		extern u32* nintCdwnCalHeapAlloc;
		extern u32* nintCdwnCalHeapAddrPtr;

		// useSharedFont = twlFontFound;
		setBL(0x02012480, (u32)dsiSaveGetLength);
		setBL(0x020124C0, (u32)dsiSaveRead);
		setBL(0x0201253C, (u32)dsiSaveWrite);
		setBL(0x02012B20, (u32)dsiSaveOpen);
		setBL(0x02012BA0, (u32)dsiSaveClose);
		setBL(0x02012F40, (u32)dsiSaveCreate);
		setBL(0x02012F50, (u32)dsiSaveOpen);
		setBL(0x02012F64, (u32)dsiSaveSetLength);
		setBL(0x02012FAC, (u32)dsiSaveClose);
		*(u32*)0x02013124 = 0xE1A00000; // nop
		*(u32*)0x020148AC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020148BC = 0xE1A00000; // nop
		*(u32*)0x020148D0 = 0xE1A00000; // nop
		*(u32*)0x0204F008 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			if (s2FlashcardId == 0x5A45) {
				for (int i = 0; i < 8; i++) {
					nintCdwnCalHeapAddrPtr[i] -= 0x800000;
				}
			}
			tonccpy((u32*)0x020917D8, nintCdwnCalHeapAlloc, 0xC0);
			setBL(0x0205AB70, 0x020917D8);
			/* if (twlFontFound) {
				patchTwlFontLoad(0x0205AC48, 0x02092324);
			} */
		}
		// if (!twlFontFound) {
			*(u32*)0x0205C1B4 = 0xE1A00000; // nop
		// }
		*(u32*)0x020863A8 = 0xE1A00000; // nop
		tonccpy((u32*)0x02086F2C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02089BB0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0208FCB8, heapEnd);
		*(u32*)0x02090044 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02091268);
	}

	// Nintendo Countdown Calendar (Europe, Australia)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KAUV") == 0 && debugOrMep) {
		extern u32* nintCdwnCalHeapAlloc;
		extern u32* nintCdwnCalHeapAddrPtr;

		setBL(0x020124DC, (u32)dsiSaveGetLength);
		setBL(0x0201251C, (u32)dsiSaveRead);
		setBL(0x02012598, (u32)dsiSaveWrite);
		setBL(0x02012B7C, (u32)dsiSaveOpen);
		setBL(0x02012BFC, (u32)dsiSaveClose);
		setBL(0x02012F9C, (u32)dsiSaveCreate);
		setBL(0x02012FAC, (u32)dsiSaveOpen);
		setBL(0x02012FC0, (u32)dsiSaveSetLength);
		setBL(0x02013008, (u32)dsiSaveClose);
		*(u32*)0x02013180 = 0xE1A00000; // nop
		*(u32*)0x02014908 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02014918 = 0xE1A00000; // nop
		*(u32*)0x0201492C = 0xE1A00000; // nop
		*(u32*)0x0204F214 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			if (s2FlashcardId == 0x5A45) {
				for (int i = 0; i < 8; i++) {
					nintCdwnCalHeapAddrPtr[i] -= 0x800000;
				}
			}
			tonccpy((u32*)0x02091A20, nintCdwnCalHeapAlloc, 0xC0);
			setBL(0x0205ADA8, 0x02091A20);
		}
		*(u32*)0x0205C3EC = 0xE1A00000; // nop
		*(u32*)0x020865E0 = 0xE1A00000; // nop
		tonccpy((u32*)0x02087164, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02089DE8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0208FEF0, heapEnd);
		*(u32*)0x0209027C = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x020914A0);
	}

	// Atonannichi Kazoeru: Nintendo DSi Calendar (Japan)
	// Requires either 8MB of RAM or Memory Expansion Pak
	/* else if (strcmp(romTid, "KAUJ") == 0 && debugOrMep) {
		extern u32* nintCdwnCalHeapAlloc;
		extern u32* nintCdwnCalHeapAddrPtr;

		useSharedFont = twlFontFound;
		setBL(0x02014EAC, (u32)dsiSaveGetLength);
		setBL(0x02014EEC, (u32)dsiSaveRead);
		setBL(0x02014F68, (u32)dsiSaveWrite);
		setBL(0x0201554C, (u32)dsiSaveOpen);
		setBL(0x020155CC, (u32)dsiSaveClose);
		setBL(0x0201596C, (u32)dsiSaveCreate);
		setBL(0x0201597C, (u32)dsiSaveOpen);
		setBL(0x02015990, (u32)dsiSaveSetLength);
		setBL(0x020159D8, (u32)dsiSaveClose);
		*(u32*)0x02015B50 = 0xE1A00000; // nop
		*(u32*)0x02016014 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02016024 = 0xE1A00000; // nop
		*(u32*)0x02016038 = 0xE1A00000; // nop
		*(u32*)0x0204CD9C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			if (s2FlashcardId == 0x5A45) {
				for (int i = 0; i < 8; i++) {
					nintCdwnCalHeapAddrPtr[i] -= 0x800000;
				}
			}
			tonccpy((u32*)0x0208A268, nintCdwnCalHeapAlloc, 0xC0);
			setBL(0x02058404, 0x0208A268);
		}
		if (!twlFontFound) {
			*(u32*)0x020597A4 = 0xE1A00000; // nop
		}
		*(u32*)0x0207EE70 = 0xE1A00000; // nop
		tonccpy((u32*)0x0207F9E8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0208266C = 0xE1A00000; // nop
		patchInitDSiWare(0x02088758, heapEnd);
		// *(u32*)0x02088AE4 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02089CF8);
	} */

	// Nintendo DSi + Internet (Japan)
	// Nintendo DSi + Internet (USA)
	else if (strcmp(romTid, "K2DJ") == 0 || strcmp(romTid, "K2DE") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x0200599C = 0xE1A00000; // nop
		*(u32*)0x020059A8 = 0xE1A00000; // nop
		*(u32*)0x020059B8 = 0xE1A00000; // nop
		*(u32*)0x020059C4 = 0xE1A00000; // nop
		*(u32*)0x0200AB2C = 0xE1A00000; // nop
		*(u32*)0x0200DB70 = 0xE1A00000; // nop
		*(u32*)0x0200FED4 = 0xE1A00000; // nop
		*(u32*)0x02011B0C = 0xE1A00000; // nop
		patchInitDSiWare(0x02011B18, heapEnd);
	}

	// Nintendo DSi + Internet (Europe)
	else if (strcmp(romTid, "K2DP") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x020059AC = 0xE1A00000; // nop
		*(u32*)0x020059B8 = 0xE1A00000; // nop
		*(u32*)0x020059C8 = 0xE1A00000; // nop
		*(u32*)0x020059D4 = 0xE1A00000; // nop
		*(u32*)0x0200ADE0 = 0xE1A00000; // nop
		*(u32*)0x0200DE24 = 0xE1A00000; // nop
		patchInitDSiWare(0x02011DBC, heapEnd);
	}

	// Nintendo DSi + Internet (Australia)
	else if (strcmp(romTid, "K2DU") == 0) {
		*(u32*)0x020050B8 = 0xE1A00000; // nop
		*(u32*)0x020059AC = 0xE1A00000; // nop
		*(u32*)0x020059B8 = 0xE1A00000; // nop
		*(u32*)0x020059C8 = 0xE1A00000; // nop
		*(u32*)0x020059D4 = 0xE1A00000; // nop
		*(u32*)0x0200AB70 = 0xE1A00000; // nop
		*(u32*)0x0200DBB4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02011B4C, heapEnd);
	}

	// Nintendo DSi Metronome (USA)
	// Saving not supported due to using more than one file in filesystem
	/*else if (strcmp(romTid, "KMTE") == 0) {
		*(u32*)0x0203C75C = 0xE1A00000; // nop
		*(u32*)0x0203FFB8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02045FD0, heapEnd);
		patchUserSettingsReadDSiWare(0x020475B8);
	}*/

	// Nintendogs (China)
	// Requires more than 8MB of RAM?
	/*else if (strcmp(romTid, "KDOC") == 0 && extendedMemory2) {
		*(u32*)0x0202A4FC = 0xE1A00000; // nop
		*(u32*)0x0202A524 = 0xE1A00000; // nop
		setBL(0x0202A6EC, (u32)dsiSaveSeek);
		setBL(0x0202A6FC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x0202A788 = 0xE1A00000; // nop
		*(u32*)0x0202A7E0 = 0xE1A00000; // nop
		setBL(0x0202A8C8, (u32)dsiSaveSeek);
		setBL(0x0202A8D8, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202A95C, (u32)dsiSaveSeek);
		setBL(0x0202A96C, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x0202A9F8 = 0xE1A00000; // nop
		*(u32*)0x0202AA50 = 0xE1A00000; // nop
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
		*(u32*)0x0202BCC0 = 0xE1A00000; // nop
		*(u32*)0x0202BD60 = 0xE1A00000; // nop
		*(u32*)0x0202BF0C = 0xE1A00000; // nop (dsiSaveFlush)
		setBL(0x0202BF14, (u32)dsiSaveClose);
		*(u32*)0x0202C400 = 0xE1A00000; // nop
		*(u32*)0x0202C408 = 0xE1A00000; // nop
		*(u32*)0x020FD6AC = 0xE1A00000; // nop
		*(u32*)0x021024D8 = 0xE1A00000; // nop
		*(u32*)0x0210A34C = 0xE1A00000; // nop
		*(u32*)0x0210C1D0 = 0xE1A00000; // nop
		*(u32*)0x0210C1D4 = 0xE1A00000; // nop
		*(u32*)0x0210C1E0 = 0xE1A00000; // nop
		*(u32*)0x0210C324 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0210C380, heapEnd); // mov r0, #0x23E0000
		*(u32*)0x0210D590 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0210D594 = 0xE12FFF1E; // bx lr
		*(u32*)0x0210D59C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0210D5A0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0210D824 = 0xE3A00001; // mov r0, #1
	}*/

	// Nintendoji (Japan)
	// Due to our save implementation, save data is stored in both slots
	// Audio does not play
	// Crashes when going downstairs (confirmed on retail consoles)
	else if (strcmp(romTid, "K9KJ") == 0 && debugOrMep) {
		extern u32* nintendojiHeapAlloc;
		extern u32* nintendojiHeapAddrPtr;
		if (expansionPakFound) {
			if (s2FlashcardId == 0x5A45) {
				nintendojiHeapAddrPtr[0] -= 0x01000000;
			}
			*(u32*)0x0201D1B0 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201D1D8 = 0xE12FFF1E; // bx lr
			tonccpy((u32*)0x0201C038, nintendojiHeapAlloc, 0xC0);
			setBL(0x02022FDC, 0x0201C038);
			*(u32*)0x0203F98C = 0xE1A00000; // nop
			*(u32*)0x020FAB1C = 0xE1A00000; // nop
			*(u32*)0x020FAB20 = 0xE3A06000; // mov r6, #0
			*(u32*)0x020FABB0 = 0xE1A00000; // nop
			*(u32*)0x020FABB4 = 0xE1A00000; // nop
			*(u32*)0x020FD8EC = 0xE1A00000; // nop
			*(u32*)0x020FD8F0 = 0xE3A01000; // mov r1, #0
			*(u32*)0x020FDBA8 = 0xE3A02000; // mov r2, #0
		} else {
			*(u32*)0x02005160 = 0xE3A01601; // mov r1, #0x100000
		}
		*(u32*)0x020051C0 = 0xE1A00000; // nop
		*(u32*)0x020051C4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop
		*(u32*)0x020051CC = 0xE1A00000; // nop
		*(u32*)0x02005538 = 0xE1A00000; // nop
		*(u32*)0x0200554C = 0xE1A00000; // nop
		*(u32*)0x02010FCC = 0xE1A00000; // nop
		*(u32*)0x020141B0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A96C, heapEnd);
		*(u32*)0x0201ACF8 = 0x021BFDE0;
		*(u32*)0x0202033C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020340 = 0xE12FFF1E; // bx lr
		*(u32*)0x0209EEB8 = 0xE1A00000; // nop
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
		*(u32*)0x0209F780 = 0xE1A00000; // nop
		setBL(0x0209F790, (u32)dsiSaveClose);
		*(u32*)0x0209F794 = 0xE1A00000; // nop
		setBL(0x0209F7AC, (u32)dsiSaveClose);
		setBL(0x0209F7BC, (u32)dsiSaveClose);

		// Disable sound
		*(u32*)0x02032AB8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032ABC = 0xE12FFF1E; // bx lr
		*(u32*)0x02032B1C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032B20 = 0xE12FFF1E; // bx lr
		*(u32*)0x02032B80 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032B84 = 0xE12FFF1E; // bx lr
		*(u32*)0x02032BE4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032BE8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02032C48 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032C4C = 0xE12FFF1E; // bx lr
		*(u32*)0x02032CAC = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032CB0 = 0xE12FFF1E; // bx lr
		*(u32*)0x02032D10 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032D14 = 0xE12FFF1E; // bx lr
		*(u32*)0x02032D74 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032D78 = 0xE12FFF1E; // bx lr
		*(u32*)0x02032DD8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032DDC = 0xE12FFF1E; // bx lr
		*(u32*)0x02032E00 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02032E04 = 0xE12FFF1E; // bx lr
		*(u32*)0x020FF42C = 0xE12FFF1E; // bx lr
	}

	// Noroi no Game: Chi (Japan)
	// Noroi no Game: Oku (Japan)
	else if (strcmp(romTid, "KJIJ") == 0 || strcmp(romTid, "KG9J") == 0) {
		u8 offsetChange = (strncmp(romTid, "KJI", 3) == 0) ? 0 : 4;
		u8 offsetChange2 = (strncmp(romTid, "KJI", 3) == 0) ? 0 : 0x10;

		*(u32*)0x020050A4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0200C000 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200CC94, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200F5F8 = 0xE1A00000; // nop
		*(u32*)0x02015AAC = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x02015AC4, heapEnd);
		patchUserSettingsReadDSiWare(0x02016FC4);
		setBL(0x02030DAC+offsetChange, (u32)dsiSaveOpen);
		*(u32*)(0x02030DDC+offsetChange) = 0xE1A00000; // nop
		setBL(0x02030DF0+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02030E00+offsetChange, (u32)dsiSaveRead);
		setBL(0x02030E08+offsetChange, (u32)dsiSaveClose);
		setBL(0x02030E44+offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x02030E84+offsetChange) = 0xE1A00000; // nop
		setBL(0x02030E90+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02030EC0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02030EE0+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x02030EF0+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02030EF8+offsetChange, (u32)dsiSaveClose);

		// Skip Manual screen
		*(u32*)(0x020414D4+offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x020414DC+offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x020414E8+offsetChange2) = 0xE1A00000; // nop
	}

	// Number Battle
	else if (strcmp(romTid, "KSUE") == 0) {
		*(u32*)0x02005330 = 0xE1A00000; // nop
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
		*(u32*)0x0200CB10 = 0xE1A00000; // nop
		*(u32*)0x0200CB20 = 0xE1A00000; // nop
		*(u32*)0x0200CBF8 = 0xE1A00000; // nop
		*(u32*)0x02021C20 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x020225C0 = 0xE1A00000; // nop
		*(u32*)0x020225CC = 0xE1A00000; // nop
		*(u32*)0x02022620 = 0xE1A00000; // nop
		*(u32*)0x02065870 = 0xE1A00000; // nop
		*(u32*)0x02065874 = 0xE1A00000; // nop
		*(u32*)0x0206588C = 0xE1A00000; // nop
		*(u32*)0x020A9EAC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020ACE4C = 0xE3A02C07; // mov r2, #0x700
		*(u32*)0x020ACE6C = 0xE2840B01; // add r0, r4, #0x400
		*(u32*)0x020ACE74 = 0xE1A00004; // mov r0, r4
		*(u32*)0x020ACE7C = 0xE1A00000; // nop
		*(u32*)0x020ACE80 = 0xE1A00000; // nop
		*(u32*)0x020ACE84 = 0xE1A00000; // nop
		*(u32*)0x020ACE88 = 0xE1A00000; // nop
		*(u32*)0x020ACE8C = 0xE1A00000; // nop
		*(u32*)0x020ACEA0 = 0xE2841B01; // add r1, r4, #0x400
		*(u32*)0x020CE79C = 0xE1A00000; // nop
		*(u32*)0x020D2A3C = 0xE1A00000; // nop
		patchInitDSiWare(0x020DD334, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x020DD6A4 = 0x0234F020;
		}
		patchUserSettingsReadDSiWare(0x020DEA44);
		*(u32*)0x020DEA6C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEA70 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEA78 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEA7C = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEA9C = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEAA0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEAB0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020DEAB4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEAC0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020DEAC4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020DEF00 = 0xE1A00000; // nop
		*(u32*)0x020DEF04 = 0xE1A00000; // nop
		*(u32*)0x020DEF08 = 0xE1A00000; // nop
		*(u32*)0x020DEF0C = 0xE1A00000; // nop
		*(u32*)0x020E4774 = 0xE3A00003; // mov r0, #3
	}

	// Odekake! Earth Seeker (Japan)
	// Black screens after company logos
	// Seemingly not possible to fix the cause? (Fails to read or write save)
	/*else if (strcmp(romTid, "KA7J") == 0) {
		*(u32*)0x020107E8 = 0xE1A00000; // nop
		*(u32*)0x02014350 = 0xE1A00000; // nop
		*(u32*)0x02018864 = 0xE1A00000; // nop
		*(u32*)0x0201A6B0 = 0xE1A00000; // nop
		*(u32*)0x0201A6B4 = 0xE1A00000; // nop
		*(u32*)0x0201A6C0 = 0xE1A00000; // nop
		*(u32*)0x0201A820 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201A87C, heapEnd); // mov r0, #0x23E0000
		patchUserSettingsReadDSiWare(0x0201BD30);
		*(u32*)0x0201C4A4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201C4A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02022DA4 = 0xE1A00000; // nop
		*(u32*)0x0202B070 = 0xE1A00000; // nop
		*(u32*)0x0202B078 = 0xE1A00000; // nop
		*(u32*)0x0202B080 = 0xE1A00000; // nop
		// *(u32*)0x02036694 = 0xE12FFF1E; // bx lr
		*(u32*)0x020366AC = 0xE3A00000; // mov r0, #0
		// *(u32*)0x020366C4 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x020366E4 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x02036708 = 0xE3A00001; // mov r0, #1
		// *(u32*)0x02036738 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02036C94 = 0xE1A00000; // nop
		*(u32*)0x0205D920 = 0xE1A00000; // nop
	}*/

	// Orion's Odyssey (USA)
	// Due to our save implementation, save data is stored in both slots
	// Crashes later on retail consoles
	else if (strcmp(romTid, "K6TE") == 0) {
		*(u32*)0x02011FAC = 0xE1A00000; // nop
		*(u32*)0x02015790 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201ABF0, heapEnd);
		*(u32*)0x0201AF7C = 0x02113EA0;
		patchUserSettingsReadDSiWare(0x0201C1F4);
		*(u32*)0x02020B68 = 0xE1A00000; // nop
		*(u32*)0x02020B80 = 0xE1A00000; // nop
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
		*(u32*)0x020296B0 = 0xE1A00000; // nop
	}

	// Oscar in Toyland (USA)
	// Oscar in Toyland (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KOTE") == 0 || strcmp(romTid, "KOTP") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200D550 = 0xE1A00000; // nop
		*(u32*)0x02010A9C = 0xE1A00000; // nop
		patchInitDSiWare(0x02016510, heapEnd);
		patchUserSettingsReadDSiWare(0x02017B20);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02055A38, 0x02018334);
		}
		setBL(0x020599A8, (u32)dsiSaveOpen);
		setBL(0x020599CC, (u32)dsiSaveGetLength);
		setBL(0x020599F8, (u32)dsiSaveRead);
		setBL(0x02059A00, (u32)dsiSaveClose);
		setBL(0x02059A40, (u32)dsiSaveCreate);
		setBL(0x02059A50, (u32)dsiSaveOpen);
		setBL(0x02059A78, (u32)dsiSaveSetLength);
		setBL(0x02059A8C, (u32)dsiSaveWrite);
		setBL(0x02059A94, (u32)dsiSaveClose);
	}

	// Oscar in Toyland 2 (USA)
	// Oscar in Toyland 2 (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KOYE") == 0 || strcmp(romTid, "KOYP") == 0) {
		u8 offsetChangeN = (romTid[3] == 'E') ? 0 : 4;
		u16 offsetChange = (romTid[3] == 'E') ? 0 : 0x120;
		u16 offsetChange2 = (romTid[3] == 'E') ? 0 : 0x16C;

		*(u32*)0x020053C0 = 0xFE000; // Shrink sound heap from 0x133333
		*(u32*)0x0200F728 = 0xE1A00000; // nop
		*(u32*)0x02012BF4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201872C, heapEnd);
		*(u32*)0x02018AB8 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02019BDC);
		*(u32*)(0x020353F4-offsetChangeN) = 0xE1A00000; // nop
		*(u32*)(0x020354D8-offsetChangeN) = 0xE1A00000; // nop
		*(u32*)(0x0203557C-offsetChangeN) = 0xE1A00000; // nop
		*(u32*)(0x0203563C-offsetChangeN) = 0xE1A00000; // nop
		*(u32*)(0x02043F18+offsetChange) = 0xE1A00000; // nop
		setBL(0x02043FC0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02043FD0+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0204411C+offsetChange) = 0xE3A00000; // mov r0, #0
		*(u32*)(0x02044120+offsetChange) = 0xE12FFF1E; // bx lr
		setBL(0x020443FC+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02044418+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02044434+offsetChange, (u32)dsiSaveRead);
		setBL(0x0204443C+offsetChange, (u32)dsiSaveClose);
		setBL(0x02044490+offsetChange, (u32)dsiSaveCreate);
		setBL(0x020444A0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020444DC+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x020444EC+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020444F4+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02044B20+offsetChange2) = 0xE3A00000; // mov r0, #0
		*(u32*)(0x02044C3C+offsetChange2) = 0xE1A00000; // nop
	}

	// Otegaru Pazuru Shirizu: Yurito Fushigina Meikyuu (Japan)
	else if (strcmp(romTid, "KTVJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200527C = 0xE1A00000; // nop
		*(u32*)0x02005284 = 0xE1A00000; // nop
		*(u32*)0x020052EC = 0xE1A00000; // nop
		*(u32*)0x0200F380 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200FF04, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020129D4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017548, heapEnd);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201CF74, 0x02018EB8);
			}
		} else {
			*(u32*)0x0202E35C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		setBL(0x020352B0, (u32)dsiSaveOpen);
		setBL(0x020352D4, (u32)dsiSaveGetLength);
		setBL(0x020352E4, (u32)dsiSaveRead);
		setBL(0x020352EC, (u32)dsiSaveClose);
		*(u32*)0x02035350 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02035394 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02035424, (u32)dsiSaveGetInfo);
		setBL(0x02035430, (u32)dsiSaveCreate);
		setBL(0x02035440, (u32)dsiSaveOpen);
		setBL(0x0203547C, (u32)dsiSaveSetLength);
		setBL(0x020354A8, (u32)dsiSaveWrite);
		setBL(0x020354B0, (u32)dsiSaveClose);
		setBL(0x02035630, (u32)dsiSaveGetInfo);
		setBL(0x0203566C, (u32)dsiSaveCreate);
		setBL(0x0203567C, (u32)dsiSaveOpen);
		*(u32*)0x0203569C = 0xE1A00000; // nop
		setBL(0x020356E4, (u32)dsiSaveSetLength);
		setBL(0x02035734, (u32)dsiSaveWrite);
		setBL(0x0203573C, (u32)dsiSaveClose);
	}

	// Othello (Japan)
	else if (strcmp(romTid, "KOLJ") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200B300 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200BF94, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200E7AC = 0xE1A00000; // nop
		patchInitDSiWare(0x02013F6C, heapEnd);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0202A6EC, 0x02015D14);
		}
		setBL(0x02038B98, (u32)dsiSaveGetInfo);
		setBL(0x02038C0C, (u32)dsiSaveGetInfo);
		*(u32*)0x02038C60 = 0xE1A00000; // nop
		setBL(0x02038C70, (u32)dsiSaveCreate);
		setBL(0x02038C84, (u32)dsiSaveOpen);
		setBL(0x02038CB0, (u32)dsiSaveSetLength);
		setBL(0x02038D08, (u32)dsiSaveWrite);
		setBL(0x02038D10, (u32)dsiSaveClose);
		*(u32*)0x02038D7C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02038DC4 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02038E0C, (u32)dsiSaveOpen);
		setBL(0x02038E2C, (u32)dsiSaveGetLength);
		setBL(0x02038E40, (u32)dsiSaveRead);
		setBL(0x02038E48, (u32)dsiSaveClose);
		setBL(0x020390A0, (u32)dsiSaveGetInfo);
		setBL(0x020390AC, (u32)dsiSaveCreate);
		setBL(0x020390BC, (u32)dsiSaveOpen);
		setBL(0x020390EC, (u32)dsiSaveSetLength);
		setBL(0x02039118, (u32)dsiSaveWrite);
		setBL(0x02039120, (u32)dsiSaveClose);
	}

	// Otona no Nihonshi Pazuru (Japan)
	// Otona no Sekaishi Pazuru (Japan)
	else if (strcmp(romTid, "KL7J") == 0 || strcmp(romTid, "KL6J") == 0) {
		u8 offsetChange1 = (romTid[2] == '7') ? 0 : 0xA0;
		u8 offsetChange2 = (romTid[2] == '7') ? 0 : 0xA4;
		u8 offsetChange3 = (romTid[2] == '7') ? 0 : 0x9C;
		u8 offsetChange4 = (romTid[2] == '7') ? 0 : 0x98;
		u8 offsetChange5 = (romTid[2] == '7') ? 0 : 0x94;
		u8 offsetChange6 = (romTid[2] == '7') ? 0 : 0xB8;

		*(u32*)(0x02011ED4+offsetChange1) = 0xE1A00000; // nop
		setBL(0x020120DC+offsetChange2, (u32)dsiSaveClose);
		*(u32*)(0x02012140+offsetChange2) = 0xE1A00000; // nop
		setBL(0x02012224+offsetChange1, (u32)dsiSaveClose);
		setBL(0x020123C4+offsetChange3, (u32)dsiSaveOpen);
		setBL(0x020123EC+offsetChange3, (u32)dsiSaveSeek);
		setBL(0x02012408+offsetChange3, (u32)dsiSaveClose);
		setBL(0x02012420+offsetChange3, (u32)dsiSaveRead);
		setBL(0x02012440+offsetChange3, (u32)dsiSaveClose);
		setBL(0x02012450+offsetChange3, (u32)dsiSaveClose);
		setBL(0x0201248C+offsetChange3, (u32)dsiSaveOpen);
		setBL(0x020124A4+offsetChange3, (u32)dsiSaveSeek);
		setBL(0x020124BC+offsetChange3, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x020124F0+offsetChange3, (u32)dsiSaveOpen);
		setBL(0x02012510+offsetChange3, (u32)dsiSaveSetLength);
		setBL(0x02012520+offsetChange3, (u32)dsiSaveClose);
		setBL(0x0201253C+offsetChange3, (u32)dsiSaveSeek);
		setBL(0x02012558+offsetChange3, (u32)dsiSaveClose);
		setBL(0x02012570+offsetChange3, (u32)dsiSaveWrite);
		setBL(0x02012594+offsetChange3, (u32)dsiSaveClose);
		setBL(0x020125A0+offsetChange3, (u32)dsiSaveClose);
		setBL(0x020125E0+offsetChange4, (u32)dsiSaveOpen);
		setBL(0x020125F4+offsetChange4, (u32)dsiSaveSetLength);
		setBL(0x0201260C+offsetChange4, (u32)dsiSaveSeek);
		setBL(0x02012624+offsetChange4, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0201267C+offsetChange5, (u32)dsiSaveCreate);
		setBL(0x02012684+offsetChange5, (u32)dsiSaveGetResultCode);
		*(u32*)(0x02022238+offsetChange6) = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		*(u32*)(0x02029C24+offsetChange6) = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)(0x0203413C+offsetChange6) = 0xE3A00000; // mov r0, #0
		*(u32*)(0x02036B74+offsetChange6) = 0xE1A00000; // nop
		*(u32*)(0x0203A1A0+offsetChange6) = 0xE1A00000; // nop
		patchInitDSiWare(0x0203FDE4+offsetChange6, heapEnd);
		patchUserSettingsReadDSiWare(0x02041544+offsetChange6);
	}

	// Otona no Tame no: Kei-san Training DS (Japan)
	else if (strcmp(romTid, "K3TJ") == 0) {
		*(u32*)0x02008808 = 0xE1A00000; // nop
		*(u32*)0x0200BE84 = 0xE1A00000; // nop
		patchInitDSiWare(0x02011E30, heapEnd);
		patchUserSettingsReadDSiWare(0x02013598);
		*(u32*)0x02018790 = 0xE1A00000; // nop
		*(u32*)0x020187A8 = 0xE1A00000; // nop
		*(u32*)0x02033418 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x02033604 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02036A8C = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x0204599C, (u32)dsiSaveCreate);
		setBL(0x0204611C, (u32)dsiSaveClose);
		setBL(0x020462AC, (u32)dsiSaveOpen);
		setBL(0x020462C8, (u32)dsiSaveSeek);
		setBL(0x020462DC, (u32)dsiSaveClose);
		setBL(0x020462F4, (u32)dsiSaveRead);
		setBL(0x02046304, (u32)dsiSaveClose);
		setBL(0x02046310, (u32)dsiSaveClose);
		setBL(0x02046344, (u32)dsiSaveOpen);
		setBL(0x0204635C, (u32)dsiSaveSeek);
		setBL(0x02046374, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x020463A4, (u32)dsiSaveOpen);
		setBL(0x020463BC, (u32)dsiSaveSetLength);
		setBL(0x020463CC, (u32)dsiSaveClose);
		setBL(0x020463E0, (u32)dsiSaveSeek);
		setBL(0x020463F4, (u32)dsiSaveClose);
		setBL(0x0204640C, (u32)dsiSaveWrite);
		setBL(0x0204641C, (u32)dsiSaveClose);
		setBL(0x02046428, (u32)dsiSaveClose);
		setBL(0x0204645C, (u32)dsiSaveOpen);
		setBL(0x02046470, (u32)dsiSaveSetLength);
		setBL(0x02046488, (u32)dsiSaveSeek);
		setBL(0x020464A0, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x0204652C = 0xE12FFF1E; // bx lr
	}

	// Sun-Ganui Gyesan!: Dunoehoejeoni Ppallajineun Gyesan Training (Korea)
	else if (strcmp(romTid, "K3TK") == 0) {
		*(u32*)0x0200C530 = 0xE1A00000; // nop
		*(u32*)0x0200FA50 = 0xE1A00000; // nop
		patchInitDSiWare(0x020156B8, heapEnd);
		patchUserSettingsReadDSiWare(0x02016E18);
		*(u32*)0x0201CC94 = 0xE12FFF1E; // bx lr
		*(u32*)0x02024C6C = 0xE3A00000; // mov r0, #0
		setBL(0x02024E08, (u32)dsiSaveCreate);
		setBL(0x02025588, (u32)dsiSaveClose);
		setBL(0x02025644, (u32)dsiSaveOpen);
		setBL(0x02025660, (u32)dsiSaveSeek);
		setBL(0x02025674, (u32)dsiSaveClose);
		setBL(0x0202568C, (u32)dsiSaveRead);
		setBL(0x0202569C, (u32)dsiSaveClose);
		setBL(0x020256A8, (u32)dsiSaveClose);
		setBL(0x020256DC, (u32)dsiSaveOpen);
		setBL(0x020256F4, (u32)dsiSaveSeek);
		setBL(0x0202570C, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x0202573C, (u32)dsiSaveOpen);
		setBL(0x02025754, (u32)dsiSaveSetLength);
		setBL(0x02025764, (u32)dsiSaveClose);
		setBL(0x02025778, (u32)dsiSaveSeek);
		setBL(0x0202578C, (u32)dsiSaveClose);
		setBL(0x020257A4, (u32)dsiSaveWrite);
		setBL(0x020257B4, (u32)dsiSaveClose);
		setBL(0x020257C0, (u32)dsiSaveClose);
		setBL(0x020257F4, (u32)dsiSaveOpen);
		setBL(0x02025808, (u32)dsiSaveSetLength);
		setBL(0x02025820, (u32)dsiSaveSeek);
		setBL(0x02025838, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x02031BB8 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x02036F8C = 0xE1A00000; // nop
		*(u32*)0x02036FA4 = 0xE1A00000; // nop
		*(u32*)0x0203A530 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
	}

	// Otona no Tame no: Renjuku Kanji (Japan)
	else if (strcmp(romTid, "KJ9J") == 0) {
		*(u32*)0x0200E618 = 0xE1A00000; // nop
		*(u32*)0x02011B38 = 0xE1A00000; // nop
		patchInitDSiWare(0x020177B4, heapEnd);
		patchUserSettingsReadDSiWare(0x02018F14);
		*(u32*)0x02048D60 = 0xE1A00000; // nop
		*(u32*)0x02048D74 = 0xE1A00000; // nop
		*(u32*)0x02049A5C = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
		*(u32*)0x02049C48 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020532B0 = 0xE3A00000; // mov r0, #0 (Skip Manual screen, Part 2)
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		setBL(0x0202A37C, (u32)dsiSaveClose);
		setBL(0x02064EC8, (u32)dsiSaveOpen);
		setBL(0x02064EE4, (u32)dsiSaveSeek);
		setBL(0x02064EF8, (u32)dsiSaveClose);
		setBL(0x02064F10, (u32)dsiSaveRead);
		setBL(0x02064F20, (u32)dsiSaveClose);
		setBL(0x02064F2C, (u32)dsiSaveClose);
		setBL(0x02064F60, (u32)dsiSaveOpen);
		setBL(0x02064F78, (u32)dsiSaveSeek);
		setBL(0x02064F90, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(0x02064FC0, (u32)dsiSaveOpen);
		setBL(0x02064FD8, (u32)dsiSaveSetLength);
		setBL(0x02064FE8, (u32)dsiSaveClose);
		setBL(0x02064FFC, (u32)dsiSaveSeek);
		setBL(0x02065010, (u32)dsiSaveClose);
		setBL(0x02065028, (u32)dsiSaveWrite);
		setBL(0x02065038, (u32)dsiSaveClose);
		setBL(0x02065044, (u32)dsiSaveClose);
		setBL(0x02065078, (u32)dsiSaveOpen);
		setBL(0x0206508C, (u32)dsiSaveSetLength);
		setBL(0x020650A4, (u32)dsiSaveSeek);
		setBL(0x020650BC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		*(u32*)0x02065148 = 0xE12FFF1E; // bx lr
	}

	// Panewa! (Japan)
	// Requires more than 16MB of RAM
	/* else if (strcmp(romTid, "KPWJ") == 0) {
		*(u32*)0x0200E2C0 = 0xE1A00000; // nop
		*(u32*)0x02012594 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A5EC, heapEnd);
		*(u32*)0x0201A95C -= 0x30000;
		patchUserSettingsReadDSiWare(0x0201BCCC);
		*(u32*)0x02021118 -= 0x2F;
		*(u32*)0x02021128 -= 0x2F;
		*(u32*)0x02021148 -= 0x2F;
		*(u32*)0x02021184 += 0xE0000000; // beq -> b
		setBL(0x020351EC, (u32)dsiSaveCreate);
		setBL(0x020351FC, (u32)dsiSaveOpen);
		setBL(0x0203521C, (u32)dsiSaveSetLength);
		setBL(0x0203523C, (u32)dsiSaveWrite);
		setBL(0x0203525C, (u32)dsiSaveClose);
		*(u32*)0x0203529C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x020352D8 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x02035300 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02035314 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02035364 = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		*(u32*)0x020353A8 = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		setBL(0x020354A8, (u32)dsiSaveOpen);
		setBL(0x020354C4, (u32)dsiSaveGetLength);
		setBL(0x020354FC, (u32)dsiSaveRead);
		setBL(0x02035514, (u32)dsiSaveClose);
	} */

	// Kami Hikouki (Japan)
	// Saving not supported due to using more than one file
	else if (strcmp(romTid, "KAMJ") == 0) {
		// useSharedFont = twlFontFound;
		*(u32*)0x0200D908 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		*(u32*)0x02017D38 = 0xE3A00001; // mov r0, #1 (Enable TWL soft-reset function)
		/* if (useSharedFont) {
			*(u32*)0x02015FCC = 0xE3A00001; // mov r0, #1
			*(u32*)0x02021E60 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02021E9C = 0xE3A00001; // mov r0, #1
		} else { */
			*(u32*)0x02021E48 = 0xE12FFF1E; // bx lr (Disable NFTR font loading)
			*(u32*)0x02021FEC = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		// }
		*(u32*)0x02030298 = 0xE12FFF1E; // bx lr (Hide volume icon in pause menu)
	}

	// Paul's Monster Adventure (USA)
	else if (strcmp(romTid, "KP9E") == 0) {
		*(u32*)0x02013828 = 0xE1A00000; // nop
		tonccpy((u32*)0x020143AC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02016F48 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E654, heapEnd);
		patchUserSettingsReadDSiWare(0x0201FB30);
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

	// Aru to Harapeko Monsuta (Japan)
	else if (strcmp(romTid, "KP9J") == 0) {
		*(u32*)0x02013794 = 0xE1A00000; // nop
		tonccpy((u32*)0x02014318, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02016E2C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E514, heapEnd);
		patchUserSettingsReadDSiWare(0x0201F9F0);
		setBL(0x020475B8, (u32)dsiSaveOpen);
		setBL(0x020475D8, (u32)dsiSaveWrite);
		setBL(0x020475E8, (u32)dsiSaveClose);
		setBL(0x02047604, (u32)dsiSaveClose);
		setBL(0x02047640, (u32)dsiSaveOpen);
		setBL(0x02047660, (u32)dsiSaveRead);
		setBL(0x02047670, (u32)dsiSaveClose);
		setBL(0x0204768C, (u32)dsiSaveClose);
		setBL(0x0204773C, (u32)dsiSaveCreate);
		setBL(0x0204774C, (u32)dsiSaveOpen);
		setBL(0x02047778, (u32)dsiSaveClose);
		setBL(0x020477A4, (u32)dsiSaveCreate);
		setBL(0x020477B4, (u32)dsiSaveOpen);
		setBL(0x020477E0, (u32)dsiSaveClose);
	}

	// Paul's Shooting Adventure (USA)
	else if (strcmp(romTid, "KPJE") == 0) {
		*(u32*)0x0200FF94 = 0xE1A00000; // nop
		tonccpy((u32*)0x02010B18, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201362C = 0xE1A00000; // nop
		patchInitDSiWare(0x02019C28, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B104);
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

	// Adventure Kid: Poru no Bouken (Japan)
	else if (strcmp(romTid, "KPJJ") == 0) {
		*(u32*)0x0200FF74 = 0xE1A00000; // nop
		tonccpy((u32*)0x02010AF8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201360C = 0xE1A00000; // nop
		patchInitDSiWare(0x02019C08, heapEnd);
		patchUserSettingsReadDSiWare(0x0201B0E4);
		setBL(0x02037328, (u32)dsiSaveOpen);
		setBL(0x02037340, (u32)dsiSaveCreate);
		setBL(0x02037358, (u32)dsiSaveOpen);
		setBL(0x02037378, (u32)dsiSaveWrite);
		setBL(0x02037388, (u32)dsiSaveClose);
		setBL(0x020373A4, (u32)dsiSaveClose);
		setBL(0x020373E0, (u32)dsiSaveOpen);
		setBL(0x02037400, (u32)dsiSaveRead);
		setBL(0x02037410, (u32)dsiSaveClose);
		setBL(0x0203742C, (u32)dsiSaveClose);
		setBL(0x020374DC, (u32)dsiSaveCreate);
		setBL(0x020374EC, (u32)dsiSaveOpen);
		setBL(0x02037518, (u32)dsiSaveClose);
		setBL(0x02037544, (u32)dsiSaveCreate);
		setBL(0x02037554, (u32)dsiSaveOpen);
		setBL(0x02037580, (u32)dsiSaveClose);
	}

	// Paul's Shooting Adventure 2 (USA)
	// Adventure Kid 2: Poru no Dai Bouken (Japan)
	else if (strcmp(romTid, "KUSE") == 0 || strcmp(romTid, "KUSJ") == 0) {
		*(u32*)0x02016008 = 0xE1A00000; // nop
		tonccpy((u32*)0x02016B8C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02019E58 = 0xE1A00000; // nop
		patchInitDSiWare(0x020217E8, heapEnd);
		patchUserSettingsReadDSiWare(0x02022E98);
		*(u32*)0x02028634 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02028638 = 0xE12FFF1E; // bx lr
		if (romTid[3] == 'E') {
			*(u32*)0x0202A968 = 0xE1A00000; // nop
			*(u32*)0x0202A980 = 0xE1A00000; // nop
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
		} else {
			*(u32*)0x0202C7AC = 0xE1A00000; // nop
			*(u32*)0x0202C7C4 = 0xE1A00000; // nop
			setBL(0x02030C88, (u32)dsiSaveOpen);
			setBL(0x02030CA0, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02030CB8, (u32)dsiSaveOpen);
			setBL(0x02030CD8, (u32)dsiSaveWrite);
			setBL(0x02030CE8, (u32)dsiSaveClose);
			setBL(0x02030CF8, (u32)dsiSaveClose);
			setBL(0x02030D38, (u32)dsiSaveOpen);
			setBL(0x02030D5C, (u32)dsiSaveRead);
			setBL(0x02030D6C, (u32)dsiSaveClose);
			setBL(0x02030D7C, (u32)dsiSaveClose);
			setBL(0x02030E5C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02030E6C, (u32)dsiSaveOpen);
			setBL(0x02030E98, (u32)dsiSaveClose);
			setBL(0x02030ED0, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02030EE0, (u32)dsiSaveOpen);
			setBL(0x02030F0C, (u32)dsiSaveClose);
		}
	}

	// Peg Solitaire (USA)
	else if (strcmp(romTid, "KP8E") == 0) {
		*(u32*)0x02012FA4 = 0xE1A00000; // nop
		*(u32*)0x02016348 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BC88, heapEnd);
		*(u32*)0x0201C014 = 0x022D5D40;
		patchUserSettingsReadDSiWare(0x0201D118);
		*(u32*)0x02037928 = 0xE1A00000; // nop
		*(u32*)0x02037930 = 0xE1A00000; // nop
		*(u32*)0x0203793C = 0xE1A00000; // nop
		*(u32*)0x02037944 = 0xE1A00000; // nop
		*(u32*)0x02037954 = 0xE1A00000; // nop
		*(u32*)0x0203DDA8 = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x0204616C, (u32)dsiSaveOpen);
		setBL(0x02046184, (u32)dsiSaveCreate);
		setBL(0x0204618C, (u32)dsiSaveGetResultCode);
		setBL(0x020461A0, (u32)dsiSaveOpen);
		setBL(0x020461B0, (u32)dsiSaveGetResultCode);
		*(u32*)0x020461C0 = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KP8P") == 0) {
		*(u32*)0x02012FBC = 0xE1A00000; // nop
		*(u32*)0x020163F4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201BD6C, heapEnd);
		*(u32*)0x0201C0F8 = 0x022D60C0;
		patchUserSettingsReadDSiWare(0x0201D20C);
		*(u32*)0x02037800 = 0xE1A00000; // nop
		*(u32*)0x02037808 = 0xE1A00000; // nop
		*(u32*)0x02037814 = 0xE1A00000; // nop
		*(u32*)0x0203781C = 0xE1A00000; // nop
		*(u32*)0x0203782C = 0xE1A00000; // nop
		*(u32*)0x0203E03C = 0xE1A00000; // nop (Skip Manual screen)
		setBL(0x02046400, (u32)dsiSaveOpen);
		setBL(0x02046410, (u32)dsiSaveGetResultCode);
		setBL(0x0204642C, (u32)dsiSaveCreate);
		setBL(0x02046444, (u32)dsiSaveOpen);
		setBL(0x02046454, (u32)dsiSaveGetResultCode);
		*(u32*)0x02046464 = 0xE1A00000; // nop
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

	// Petit Computer (USA)
	// Does not boot (black screens, seems to rely on code from DSi binaries)
	/*else if (strcmp(romTid, "KNAE") == 0) {
		*(u32*)0x0200523C = 0xE1A00000; // nop
		*(u32*)0x0200EB90 = 0xE1A00000; // nop
		*(u32*)0x0200EB98 = 0xE1A00000; // nop
		*(u32*)0x0200EBB4 = 0xE1A00000; // nop
		*(u32*)0x0200EBBC = 0xE1A00000; // nop
		*(u32*)0x0200EBF0 = 0xE1A00000; // nop
		*(u32*)0x0200EC08 = 0xE1A00000; // nop
		*(u32*)0x0200EC30 = 0xE1A00000; // nop
		*(u32*)0x02086F5C = 0xE1A00000; // nop
		*(u32*)0x0208B504 = 0xE1A00000; // nop
		*(u32*)0x0208BCBC = 0xE1A00000; // nop
		*(u32*)0x0208BCD8 = 0xE1A00000; // nop
		*(u32*)0x0208BF4C = 0xE1A00000; // nop
		*(u32*)0x0208C288 = 0xE1A00000; // nop
		*(u32*)0x0208C2A0 = 0xE1A00000; // nop
		*(u32*)0x0208C8D8 = 0xE1A00000; // nop
		*(u32*)0x0208C974 = 0xE1A00000; // nop
		*(u32*)0x0208CA28 = 0xE1A00000; // nop
		*(u32*)0x0208CADC = 0xE1A00000; // nop
		*(u32*)0x0208CB7C = 0xE1A00000; // nop
		*(u32*)0x0208CBFC = 0xE1A00000; // nop
		*(u32*)0x0208CC78 = 0xE1A00000; // nop
		*(u32*)0x0208CCFC = 0xE1A00000; // nop
		*(u32*)0x0208CD9C = 0xE1A00000; // nop
		*(u32*)0x0208CE58 = 0xE1A00000; // nop
		*(u32*)0x0208CF14 = 0xE1A00000; // nop
		*(u32*)0x0208CFD0 = 0xE1A00000; // nop
		*(u32*)0x0208D08C = 0xE1A00000; // nop
		*(u32*)0x0208D148 = 0xE1A00000; // nop
		*(u32*)0x0208D204 = 0xE1A00000; // nop
		*(u32*)0x0208D2C0 = 0xE1A00000; // nop
		*(u32*)0x0208D36C = 0xE1A00000; // nop
		*(u32*)0x0208D400 = 0xE1A00000; // nop
		*(u32*)0x0208D494 = 0xE1A00000; // nop
		*(u32*)0x0208D528 = 0xE1A00000; // nop
		*(u32*)0x0208D5BC = 0xE1A00000; // nop
		*(u32*)0x0208D650 = 0xE1A00000; // nop
		*(u32*)0x0208D6E4 = 0xE1A00000; // nop
		*(u32*)0x0208D778 = 0xE1A00000; // nop
		*(u32*)0x0208D89C = 0xE1A00000; // nop
		*(u32*)0x0208D900 = 0xE1A00000; // nop
		*(u32*)0x0208D9C8 = 0xE1A00000; // nop
		*(u32*)0x0208DA38 = 0xE1A00000; // nop
		*(u32*)0x0208DAC4 = 0xE1A00000; // nop
		*(u32*)0x0208DB34 = 0xE1A00000; // nop
		*(u32*)0x0208DBBC = 0xE1A00000; // nop
		*(u32*)0x0208DC2C = 0xE1A00000; // nop
		*(u32*)0x0208DD40 = 0xE1A00000; // nop
		*(u32*)0x0208DE28 = 0xE1A00000; // nop
		*(u32*)0x0208DE8C = 0xE1A00000; // nop
		*(u32*)0x0208DF44 = 0xE1A00000; // nop
		*(u32*)0x0208DFB4 = 0xE1A00000; // nop
		*(u32*)0x02090994 = 0xE1A00000; // nop
		*(u32*)0x02092A74 = 0xE1A00000; // nop
		*(u32*)0x02092A78 = 0xE1A00000; // nop
		*(u32*)0x02092A84 = 0xE1A00000; // nop
		*(u32*)0x02092BE4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x02092C40, heapEnd); // mov r0, #0x23E0000
	}*/

	// Petz Catz: Family (USA)
	// Unsure if it requires 8MB of RAM
	/*else if (strcmp(romTid, "KP5E") == 0) {
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

		doubleNopT(0x020191A8);
		doubleNopT(0x0201A4E4);
		doubleNopT(0x0201A4E8);
		doubleNopT(0x0201A4FC);
		doubleNopT(0x0201A50E);
		doubleNopT(0x0201A526);
		doubleNopT(0x02031234);
		setBLThumb(0x0203810A, dsiSaveOpenT);
		setBLThumb(0x02038152, dsiSaveCloseT);
		setBLThumb(0x02038176, dsiSaveGetInfoT);
		setBLThumb(0x0203819C, dsiSaveCreateT);
		setBLThumb(0x020381BE, dsiSaveSeekT);
		setBLThumb(0x020381D4, dsiSaveReadT);
		setBLThumb(0x020381F6, dsiSaveSeekT);
		setBLThumb(0x0203820C, dsiSaveWriteT);
		if (!extendedMemory2) {
			*(u32*)0x02089404 = 0xE3A00000; // mov r0, #0
		}
		*(u32*)0x020A5B04 = 0xE1A00000; // nop
		tonccpy((u32*)0x020A67EC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020A9B78 = 0xE1A00000; // nop
		*(u32*)0x020B046C = 0xE1A00000; // nop
		*(u32*)0x020B244C = 0xE1A00000; // nop
		*(u32*)0x020B2450 = 0xE1A00000; // nop
		*(u32*)0x020B245C = 0xE1A00000; // nop
		*(u32*)0x020B25A0 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020B25FC, heapEnd); // mov r0, #0x23E0000
		*(u32*)0x020B2730 = 0x021CD200;
		patchUserSettingsReadDSiWare(0x020B3A34);
		if (!extendedMemory2) {
			*(u32*)0x020CEF70 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CEF74 = 0xE12FFF1E; // bx lr
			*(u32*)0x020CEFD4 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CEFD8 = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF038 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF03C = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF09C = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF0A0 = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF100 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF104 = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF164 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF168 = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF1C8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF1CC = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF22C = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF230 = 0xE12FFF1E; // bx lr
			*(u32*)0x020CF290 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020CF294 = 0xE12FFF1E; // bx lr
			*(u32*)0x020D0254 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020D0258 = 0xE12FFF1E; // bx lr
		}
	}*/

	// Phantasy Star 0 Mini (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KPSJ") == 0 && extendedMemory2) {
		*(u32*)0x02007FA8 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02007FAC = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0200CC88 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202D950, heapEnd);
		// *(u32*)0x0202DCCC = 0x0218B0A0;
		patchUserSettingsReadDSiWare(0x0202EE48);
		*(u32*)0x0202EE70 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202EE74 = 0xE12FFF1E; // bx lr
		*(u16*)0x0209F778 = nopT;
		*(u16*)0x0209F77A = nopT;
	}

	// GO Series: Picdun (USA)
	// GO Series: Picdun (Europe)
	else if (strcmp(romTid, "KPQE") == 0 || strcmp(romTid, "KPQP") == 0) {
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020050E4 = 0xE1A00000; // nop
		*(u32*)0x020052A4 = 0xE1A00000; // nop
		*(u32*)0x0200A5C4 = 0xE1A00000; // nop
		// *(u32*)0x0200A66C = 0xE3A00000; // mov r0, #0
		// *(u32*)0x0200A670 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x020642CC = 0xE1A00000; // nop
		tonccpy((u32*)0x02064E50, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02067E34 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206ECC8, heapEndExceed);
		patchUserSettingsReadDSiWare(0x020701A4);
	}

	// Danjo RPG: Picudan (Japan)
	else if (strcmp(romTid, "KPQJ") == 0) {
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x0200528C = 0xE1A00000; // nop
		*(u32*)0x020097D8 = 0xE1A00000; // nop
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
		*(u32*)0x02061C34 = 0xE1A00000; // nop
		tonccpy((u32*)0x020627B8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02065714 = 0xE1A00000; // nop
		patchInitDSiWare(0x0206C584, heapEndExceed);
		patchUserSettingsReadDSiWare(0x0206DA60);
	}

	// Art Style: PiCTOBiTS (USA)
	// Art Style: PiCOPiCT (Europe, Australia)
	else if (strcmp(romTid, "KAPE") == 0 || strcmp(romTid, "KAPV") == 0) {
		*(u32*)0x0200518C = 0xE1A00000; // nop
		*(u32*)0x02005198 = 0xE1A00000; // nop
		*(u32*)0x0200519C = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
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
		if (romTid[3] == 'E') {
			*(u32*)0x020395E0 = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x020395E4 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x0203CC10 = 0xE1A00000; // nop
			patchInitDSiWare(0x02043218, heapEnd);
			patchUserSettingsReadDSiWare(0x02044804);
		} else if (romTid[3] == 'V') {
			*(u32*)0x02039658 = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x0203965C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x0203CC88 = 0xE1A00000; // nop
			patchInitDSiWare(0x02043290, heapEnd);
			patchUserSettingsReadDSiWare(0x0204487C);
		}
	}

	// Art Style: PiCOPiCT (Japan)
	else if (strcmp(romTid, "KAPJ") == 0) {
		*(u32*)0x02005194 = 0xE1A00000; // nop
		*(u32*)0x020051A0 = 0xE1A00000; // nop
		*(u32*)0x020051A4 = 0xE1A00000; // nop
		*(u32*)0x020051A8 = 0xE1A00000; // nop
		*(u32*)0x020051B0 = 0xE1A00000; // nop
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
		*(u32*)0x0203990C = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02039910 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0203CF3C = 0xE1A00000; // nop
		patchInitDSiWare(0x02043528, heapEnd);
		patchUserSettingsReadDSiWare(0x0204487C);
	}

	// PictureBook Games: The Royal Bluff (USA)
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KE3E") == 0) {
		*(u32*)0x0200509C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Disable audio
			*(u32*)0x02005084 = 0xE1A00000; // nop
			*(u32*)0x0206AC04 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206AC08 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206AC68 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206AC6C = 0xE12FFF1E; // bx lr
			*(u32*)0x0206ACCC = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206ACD0 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206AD30 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206AD34 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206AD94 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206AD98 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206ADF8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206ADFC = 0xE12FFF1E; // bx lr
			*(u32*)0x0206AE5C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206AE60 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206AEC0 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206AEC4 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x020050A4 = 0xE3A00002; // mov r0, #2
		*(u32*)0x0201013C = 0xE1A00000; // nop
		*(u32*)0x020136B8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02022304, heapEnd);
		*(u32*)0x02022674 = 0x0217A140;
		patchUserSettingsReadDSiWare(0x02023AD0);
		*(u32*)0x020293FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02029400 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202F1F0 = 0xE3A00001; // mov r0, #1
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
	// Audio doesn't play on retail consoles
	else if (strcmp(romTid, "KE3V") == 0 || strcmp(romTid, "KE3P") == 0) {
		*(u32*)0x0200509C = 0xE1A00000; // nop
		if (!extendedMemory2) {
			// Disable audio
			*(u32*)0x02005084 = 0xE1A00000; // nop
			*(u32*)0x0206B094 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B098 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B0F8 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B0FC = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B15C = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B160 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B1C0 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B1C4 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B224 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B228 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B288 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B28C = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B2EC = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B2F0 = 0xE12FFF1E; // bx lr
			*(u32*)0x0206B350 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0206B354 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x020050A4 = 0xE3A00002; // mov r0, #2
		*(u32*)0x02010150 = 0xE1A00000; // nop
		*(u32*)0x020136CC = 0xE1A00000; // nop
		patchInitDSiWare(0x02022318, heapEnd);
		*(u32*)0x02022688 = 0x0217FD00;
		patchUserSettingsReadDSiWare(0x02023AE4);
		*(u32*)0x02029410 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02029414 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202EEE0 = 0xE3A00001; // mov r0, #1
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
		u8 offsetChange = (romTid[3] == 'V') ? 0 : 4;
		*(u32*)(0x0205F550+offsetChange) = 0xE3A00001; // mov r0, #1
		*(u32*)(0x0205F564+offsetChange) = 0xE3A00000; // mov r0, #0

		// Change help button (No blank file found to hide it)
		//const char* lp0 = "t_1p.dt0";
		//tonccpy((char*)0x0211ABC6, lp0, strlen(lp0)+1);
		//tonccpy((char*)0x0211ABF0, lp0, strlen(lp0)+1);
	}

	// Picture Perfect: Pocket Stylist (USA)
	// Audio doesn't play on retail consoles
	// Requires 8MB of RAM for full usage
	else if (strcmp(romTid, "KHRE") == 0) {
		/*if (!extendedMemory2) {
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				u32* addr = (u32*)0x02004000;
				if (addr[i] >= 0x02294000 && addr[i] < 0x022B0000) {
					addr[i] -= 0x120000;
				}
			}
		}*/

		if (!extendedMemory2) {
			*(u32*)0x020050E8 = 0xE3A00901; // mov r0, #0x4000
			*(u32*)0x0202F034 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202F038 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202F098 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202F09C = 0xE12FFF1E; // bx lr
			*(u32*)0x0205837C = 0xE1A00000; // nop
			*(u32*)0x02058608 = 0xE1A00000; // nop
		}
		*(u32*)0x0200511C = 0xE1A00000; // nop
		for (int i = 0; i < 7; i++) {
			u32* offset = (u32*)0x02005194;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x020051B4 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200CF44 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200D14C = 0xE1A00000; // nop
		*(u32*)0x0201300C = 0xE1A00000; // nop
		*(u32*)0x02013144 = 0xE1A00000; // nop
		*(u32*)0x02013158 = 0xE1A00000; // nop
		*(u32*)0x02016F98 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F054, heapEnd);
		*(u32*)0x0201F3E0 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02020614);
		*(u32*)0x02025884 = 0xE1A00000; // nop
		*(u32*)0x02044080 = 0xE1A00000; // nop
		*(u32*)0x020440A0 = 0xE1A00000; // nop
		*(u32*)0x0204D7CC = 0xE1A00000; // nop
		*(u32*)0x0204E638 = 0xE1A00000; // nop
		*(u32*)0x0204E644 = 0xE1A00000; // nop
		*(u32*)0x0204E654 = 0xE1A00000; // nop
		*(u32*)0x0204E660 = 0xE1A00000; // nop
		*(u32*)0x0205C7A8 = 0xE1A00000; // nop
	}

	// Hair Salon: Pocket Stylist (Europe, Australia)
	// Audio doesn't play on retail consoles
	// Requires 8MB of RAM for full usage
	else if (strcmp(romTid, "KHRV") == 0) {
		/*if (!extendedMemory2) {
			for (u32 i = 0; i < ndsHeader->arm9binarySize/4; i++) {
				u32* addr = (u32*)0x02004000;
				if (addr[i] >= 0x02294000 && addr[i] < 0x022B0000) {
					addr[i] -= 0x120000;
				}
			}
		}*/

		if (!extendedMemory2) {
			*(u32*)0x020050D0 = 0xE3A00901; // mov r0, #0x4000
			*(u32*)0x0202EE50 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202EE54 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202EEB4 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0202EEB8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0205837C = 0xE1A00000; // nop
			*(u32*)0x02058708 = 0xE1A00000; // nop
		}
		*(u32*)0x02005104 = 0xE1A00000; // nop
		for (int i = 0; i < 7; i++) {
			u32* offset = (u32*)0x0200517C;
			offset[i] = 0xE1A00000; // nop
		}
		*(u32*)0x02005198 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200CE6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0200D064 = 0xE1A00000; // nop
		*(u32*)0x02013168 = 0xE1A00000; // nop
		*(u32*)0x020132AC = 0xE1A00000; // nop
		*(u32*)0x020132C0 = 0xE1A00000; // nop
		*(u32*)0x0201718C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F570, heapEnd);
		*(u32*)0x0201F8E0 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02020B34);
		*(u32*)0x02026080 = 0xE1A00000; // nop
		*(u32*)0x02044130 = 0xE1A00000; // nop
		*(u32*)0x02044150 = 0xE1A00000; // nop
		*(u32*)0x0204D7CC = 0xE1A00000; // nop
		*(u32*)0x020571C4 = 0xE1A00000; // nop
		*(u32*)0x020571D0 = 0xE1A00000; // nop
		*(u32*)0x020571E0 = 0xE1A00000; // nop
		*(u32*)0x020571EC = 0xE1A00000; // nop
		*(u32*)0x0205C898 = 0xE1A00000; // nop
	}

	// GO Series: Pinball Attack! (USA)
	// GO Series: Pinball Attack! (Europe)
	else if (strcmp(romTid, "KPYE") == 0 || strcmp(romTid, "KPYP") == 0) {
		// Skip Manual screen (Crashes)
		*(u32*)0x02040264 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0204030C = 0xE1A00000; // nop
		*(u32*)0x02040318 = 0xE1A00000; // nop
		*(u32*)0x02040330 = 0xE1A00000; // nop

		setBL(0x02044A18, (u32)dsiSaveCreate);
		setBL(0x02044A28, (u32)dsiSaveGetResultCode);
		*(u32*)0x02044A3C = 0xE1A00000; // nop
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
		*(u32*)0x02044BA4 = 0xE1A00000; // nop
		setBL(0x02044BB8, (u32)dsiSaveGetLength);
		setBL(0x02044BD0, (u32)dsiSaveRead);
		setBL(0x02044BE4, (u32)dsiSaveRead);
		setBL(0x02044C10, (u32)dsiSaveRead);
		setBL(0x02044C20, (u32)dsiSaveRead);
		setBL(0x02044C30, (u32)dsiSaveRead);
		setBL(0x02044C40, (u32)dsiSaveRead);
		setBL(0x02044C48, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)0x02045128 = 0xE3A02702; // mov r2, #0x80000
		}

		// Disable NFTR loading from TWLNAND
		*(u32*)0x02045264 = 0xE1A00000; // nop
		*(u32*)0x02045268 = 0xE1A00000; // nop 
		*(u32*)0x02045270 = 0xE1A00000; // nop
		*(u32*)0x0204527C = 0xE1A00000; // nop
		*(u32*)0x02045290 = 0xE1A00000; // nop
		*(u32*)0x0204529C = 0xE1A00000; // nop

		if (!extendedMemory2) {
			*(u32*)0x02045810 = 0xC2400; // Shrink sound heap from 0xE2400 by 128KB
		}
		*(u32*)0x020658E0 = 0xE1A00000; // nop
		*(u32*)0x02069E4C = 0xE1A00000; // nop
		patchInitDSiWare(0x02071674, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x02071A00 -= 0x30000;
		}
		patchUserSettingsReadDSiWare(0x02072B28);
	}

	// Pinball Attack! (Japan)
	else if (strcmp(romTid, "KPYJ") == 0) {
		// Skip Manual screen (Crashes)
		*(u32*)0x0203FCEC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x0203FD34 = 0xE1A00000; // nop
		*(u32*)0x0203FD3C = 0xE1A00000; // nop
		*(u32*)0x0203FD50 = 0xE1A00000; // nop

		setBL(0x02044410, (u32)dsiSaveCreate);
		setBL(0x02044420, (u32)dsiSaveGetResultCode);
		*(u32*)0x02044434 = 0xE1A00000; // nop
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
		*(u32*)0x0204458C = 0xE1A00000; // nop
		setBL(0x020445A0, (u32)dsiSaveGetLength);
		setBL(0x020445B8, (u32)dsiSaveRead);
		setBL(0x020445CC, (u32)dsiSaveRead);
		setBL(0x020445F8, (u32)dsiSaveRead);
		setBL(0x02044608, (u32)dsiSaveRead);
		setBL(0x02044618, (u32)dsiSaveRead);
		setBL(0x02044620, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)0x020449FC = 0xE3A02702; // mov r2, #0x80000
			*(u32*)0x020450AC = 0xC2400; // Shrink sound heap from 0xE2400 by 128KB
		}
		*(u32*)0x02064F3C = 0xE1A00000; // nop
		*(u32*)0x020696C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02071344, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x020716B4 -= 0x30000;
		}
		patchUserSettingsReadDSiWare(0x020727F4);
	}

	// Pinball Pulse: The Ancients Beckon (USA)
	// Incomplete/broken patch
	/*else if (strcmp(romTid, "KZPE") == 0) {
		*(u32*)0x02004988 = 0xE1A00000; // nop
		*(u32*)0x020050B4 = 0xE1A00000; // nop
		*(u32*)0x02008240 = 0xE12FFF1E; // bx lr
		for (int i = 0; i < 19; i++) {
			u32* offset1 = (u16*)0x02013614;
			u32* offset2 = (u16*)0x020136C4;
			offset1[i] = 0xE1A00000; // nop
			offset2[i] = 0xE1A00000; // nop
		}
		*(u32*)0x02013D2C = 0xE1A00000; // nop
		*(u32*)0x0202C6E0 = 0xE1A00000; // nop
		*(u32*)0x02049058 = 0xE1A00000; // nop
		*(u32*)0x020655AC = 0xE1A00000; // nop
		*(u32*)0x02067530 = 0xE1A00000; // nop
		*(u32*)0x0206ABA4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02087214 = 0xE1A00000; // nop
		*(u32*)0x0208A6E8 = 0xE1A00000; // nop
		*(u32*)0x0208DFCC = 0xE1A00000; // nop
		*(u32*)0x0208FE18 = 0xE1A00000; // nop
		*(u32*)0x0208FE1C = 0xE1A00000; // nop
		*(u32*)0x0208FE28 = 0xE1A00000; // nop
		*(u32*)0x0208FF6C = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0208FFC8, heapEnd); // mov r0, #0x23E0000
		*(u32*)0x0209147C = 0xE1A00000; // nop
		*(u32*)0x02091480 = 0xE1A00000; // nop
		*(u32*)0x02091484 = 0xE1A00000; // nop
		*(u32*)0x02091488 = 0xE1A00000; // nop
	}*/

	// Pirates Assault (USA)
	else if (strcmp(romTid, "KXAE") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x0207B3B8;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xF0000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02005090 = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop
		*(u32*)0x0200D1F8 = 0xE1A00000; // nop
		*(u32*)0x0201054C = 0xE1A00000; // nop
		patchInitDSiWare(0x020162E8, heapEnd);
		*(u32*)0x02016674 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020177F8);
		setBL(0x0204E8A4, (u32)dsiSaveGetInfo);
		setBL(0x0204E8B8, (u32)dsiSaveOpen);
		setBL(0x0204E8CC, (u32)dsiSaveCreate);
		setBL(0x0204E8DC, (u32)dsiSaveOpen);
		setBL(0x0204E8EC, (u32)dsiSaveGetResultCode);
		*(u32*)0x0204E8FC = 0xE1A00000; // nop
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
		*(u32*)0x0204EEB4 = 0xE1A00000; // nop
	}

	// Pirates Assault (Europe, Australia)
	else if (strcmp(romTid, "KXAV") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x0207EE9C;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xF0000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02005090 = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop
		*(u32*)0x0200D878 = 0xE1A00000; // nop
		*(u32*)0x02010BCC = 0xE1A00000; // nop
		patchInitDSiWare(0x02016968, heapEnd);
		*(u32*)0x02016CF4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02017E78);
		setBL(0x02052368, (u32)dsiSaveGetInfo);
		setBL(0x0205237C, (u32)dsiSaveOpen);
		setBL(0x02052390, (u32)dsiSaveCreate);
		setBL(0x020523A0, (u32)dsiSaveOpen);
		setBL(0x020523B0, (u32)dsiSaveGetResultCode);
		*(u32*)0x020523C0 = 0xE1A00000; // nop
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
		*(u32*)0x02052948 = 0xE1A00000; // nop
	}

	// Pirates Assault (Japan)
	else if (strcmp(romTid, "KXAJ") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x0207CFAC;

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0xF0000, bssEnd, sdatOffset);
		// }

		*(u32*)0x02005090 = 0xE1A00000; // nop
		*(u32*)0x020050A4 = 0xE1A00000; // nop
		*(u32*)0x0200D878 = 0xE1A00000; // nop
		*(u32*)0x02010BCC = 0xE1A00000; // nop
		patchInitDSiWare(0x02016C60, heapEnd);
		*(u32*)0x02016FEC = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02018170);
		setBL(0x02020BB8, (u32)dsiSaveGetInfo);
		setBL(0x02020BCC, (u32)dsiSaveOpen);
		setBL(0x02020BE0, (u32)dsiSaveCreate);
		setBL(0x02020BF0, (u32)dsiSaveOpen);
		setBL(0x02020C00, (u32)dsiSaveGetResultCode);
		*(u32*)0x02020C10 = 0xE1A00000; // nop
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
		*(u32*)0x020211C8 = 0xE1A00000; // nop
	}

	// Plants vs. Zombies (USA)
	else if (strcmp(romTid, "KZLE") == 0) {
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
		*(u32*)0x020C2B84 = 0xE1A00000; // nop
		*(u32*)0x020C2B8C = 0xE1A00000; // nop
		*(u32*)0x020C2BE0 = 0xE1A00000; // nop
		*(u32*)0x020C2BF4 = 0xE1A00000; // nop
		*(u32*)0x020C2BFC = 0xE1A00000; // nop
		*(u32*)0x020F71F4 = 0xE1A00000; // nop
		*(u32*)0x020F71F8 = 0xE1A00000; // nop
		*(u32*)0x020F7204 = 0xE1A00000; // nop
		*(u32*)0x020F7280 = 0xE1A00000; // nop
		*(u32*)0x020F7294 = 0xE1A00000; // nop
		tonccpy((u32*)0x020F8B24, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020FBF88 = 0xE1A00000; // nop
		patchInitDSiWare(0x02108684, heapEnd);
		*(u32*)0x02108A10 = 0x0226BE80;
		patchUserSettingsReadDSiWare(0x02109BCC);
		*(u32*)0x0211547C = 0xE1A00000; // nop
	}

	// Plants vs. Zombies (Europe, Australia)
	else if (strcmp(romTid, "KZLV") == 0) {
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
		*(u32*)0x020C3DE8 = 0xE1A00000; // nop
		*(u32*)0x020C3DF0 = 0xE1A00000; // nop
		*(u32*)0x020C3E44 = 0xE1A00000; // nop
		*(u32*)0x020C3E58 = 0xE1A00000; // nop
		*(u32*)0x020C3E60 = 0xE1A00000; // nop
		*(u32*)0x020F8E64 = 0xE1A00000; // nop
		*(u32*)0x020F8E68 = 0xE1A00000; // nop
		*(u32*)0x020F8E74 = 0xE1A00000; // nop
		*(u32*)0x020F8EF0 = 0xE1A00000; // nop
		*(u32*)0x020F8F04 = 0xE1A00000; // nop
		tonccpy((u32*)0x020FA794, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020FDBF8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0210A6DC, heapEnd);
		*(u32*)0x0210AA68 = 0x0226F8E0;
		patchUserSettingsReadDSiWare(0x0210BC24);
		*(u32*)0x021174D4 = 0xE1A00000; // nop
	}

	// PlayLearn Chinese (USA)
	// PlayLearn Spanish (USA)
	else if (strcmp(romTid, "KFXE") == 0 || strcmp(romTid, "KFQE") == 0) {
		*(u32*)0x02005190 = 0xE1A00000; // nop
		tonccpy((u32*)0x02014C94, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020176AC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E128, heapEnd);
		*(u32*)0x0201E4B4 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x0201F8C4);
		setBL(0x0202CAA4, (u32)dsiSaveCreate);
		setBL(0x0202CAC0, (u32)dsiSaveOpen);
		setBL(0x0202CAF0, (u32)dsiSaveWrite);
		setBL(0x0202CAF8, (u32)dsiSaveClose);
		*(u32*)0x0202CB88 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0202CB98 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0202CBAC, (u32)dsiSaveOpen);
		setBL(0x0202CBC8, (u32)dsiSaveClose);
		*(u32*)0x0202CC68 = (u32)dsiSaveSeek;
		*(u32*)0x0202CC84 = (u32)dsiSaveSeek;
		setBL(0x0202CC9C, (u32)dsiSaveOpen);
		setBL(0x0202CCC4, (u32)dsiSaveClose);
		setBL(0x0202CCEC, (u32)dsiSaveWrite);
		setBL(0x0202CD2C, (u32)dsiSaveRead);
		u32 offset = (strncmp(romTid, "KFX", 3) == 0) ? 0x02055684 : 0x02055670;
		*(u32*)(offset+0x38) = 0xE1A00000; // nop
		*(u32*)(offset+0x254) = 0xE1A00000; // nop
		*(u32*)(offset+0x264) = 0xE1A00000; // nop
		*(u32*)(offset+0x2AC) = 0xE1A00000; // nop
		*(u32*)(offset+0x2B4) = 0xE1A00000; // nop
		doubleNopT(strncmp(romTid, "KFX", 3) == 0 ? 0x0206D4A2 : 0x0206D48E);
	}

	// Pocket Pack: Strategy Games (Europe)
	else if (strcmp(romTid, "KSGP") == 0) {
		const u32 newOpenCodeLoc = 0x02068D90;
		const u32 newReadCodeLoc = newOpenCodeLoc+0x20;

		codeCopy((u32*)newOpenCodeLoc, (u32*)0x02064E08, 0x20);
		setBL(newOpenCodeLoc+8, (u32)dsiSaveOpenR);
		codeCopy((u32*)newReadCodeLoc, (u32*)0x02064F04, 0x20);
		setBL(newReadCodeLoc+8, (u32)dsiSaveRead);

		*(u32*)0x0203CEE8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203CF00 = 0xE3A00003; // mov r0, #3
		*(u32*)0x0203CF5C += 0xE0000000; // beq -> b
		setBL(0x0203CFD8, newReadCodeLoc);
		setBL(0x0203CFF4, (u32)dsiSaveClose);
		*(u32*)0x0203D034 = 0xE1A00000; // nop
		setBL(0x0203D138, (u32)dsiSaveClose);
		*(u32*)0x0203D174 = 0xE1A00000; // nop
		setBL(0x02064E30, (u32)dsiSaveOpen);
		setBL(0x02064E6C, newOpenCodeLoc);
		setBL(0x02064E80, (u32)dsiSaveCreate);
		setBL(0x02064E94, (u32)dsiSaveClose);
		setBL(0x02064ECC, (u32)dsiSaveSetLength);
		setBL(0x02064EEC, (u32)dsiSaveWrite);
		*(u32*)0x0206ABC8 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
		*(u32*)0x0208D108 = 0x02FFFE1C;
	}

	// Pocket Pack: Words & Numbers (Europe)
	else if (strcmp(romTid, "KWNP") == 0) {
		const u32 newOpenCodeLoc = 0x0206A49C;
		const u32 newReadCodeLoc = newOpenCodeLoc+0x20;

		codeCopy((u32*)newOpenCodeLoc, (u32*)0x020624CC, 0x20);
		setBL(newOpenCodeLoc+8, (u32)dsiSaveOpenR);
		codeCopy((u32*)newReadCodeLoc, (u32*)0x020625C8, 0x20);
		setBL(newReadCodeLoc+8, (u32)dsiSaveRead);

		*(u32*)0x0203CDF0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203CE08 = 0xE3A00003; // mov r0, #3
		*(u32*)0x0203CE64 += 0xE0000000; // beq -> b
		setBL(0x0203CEE0, newReadCodeLoc);
		setBL(0x0203CEFC, (u32)dsiSaveClose);
		*(u32*)0x0203CF3C = 0xE1A00000; // nop
		setBL(0x0203D040, (u32)dsiSaveClose);
		*(u32*)0x0203D07C = 0xE1A00000; // nop
		setBL(0x020624F4, (u32)dsiSaveOpen);
		setBL(0x02062530, newOpenCodeLoc);
		setBL(0x02062544, (u32)dsiSaveCreate);
		setBL(0x02062558, (u32)dsiSaveClose);
		setBL(0x02062590, (u32)dsiSaveSetLength);
		setBL(0x020625B0, (u32)dsiSaveWrite);
		*(u32*)0x02065A74 = 0x02FFFE1C;
		*(u32*)0x0206C2D4 = 0xE3A00001; // mov r0, #1 (Enable NitroFS reads)
	}

	// Pomjong (Japan)
	else if (strcmp(romTid, "KPMJ") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		const u32 newReadCodeLoc = 0x020123B0;
		const u32 newCloseCodeLoc = 0x020127B4;

		codeCopy((u32*)newReadCodeLoc, (u32*)0x0202AC78, 0x54);
		setBL(newReadCodeLoc+0x30, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(newReadCodeLoc+0x38, (u32)dsiSaveRead);
		codeCopy((u32*)newCloseCodeLoc, (u32*)0x0202AD20, 0x60);
		setBL(newCloseCodeLoc+0x4C, (u32)dsiSaveClose);

		*(u32*)0x02010D44 = 0xE1A00000; // nop
		*(u32*)0x020143F8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201ED44, heapEnd);
		patchUserSettingsReadDSiWare(0x020203B0);
		*(u32*)0x02025CA4 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02025CA8 = 0xE12FFF1E; // bx lr
		setBL(0x0202ACFC, (u32)dsiSaveWrite); // dsiSaveWriteAsync
		setBL(0x0202AD04, (u32)dsiSaveWrite);
		*(u32*)0x0202AF20 = 0xE1A00000; // nop
		*(u32*)0x0202B09C = 0xE1A00000; // nop
		*(u32*)0x0202B0B4 = 0xE1A00000; // nop
		*(u32*)0x0202B0EC = 0xE1A00000; // nop
		/* if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0202B238, 0x02020948);
			}
		} else { */
			*(u32*)0x0202B118 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// }
		setBL(0x02085F28, newCloseCodeLoc);
		setBL(0x02085F38, (u32)dsiSaveOpen);
		setBL(0x02085FB4, (u32)dsiSaveCreate);
		setBL(0x02085FFC, (u32)dsiSaveGetResultCode);
		setBL(0x020861F4, newCloseCodeLoc);
		setBL(0x02086214, newCloseCodeLoc);
		setBL(0x020862E0, newReadCodeLoc);
		setBL(0x020862F0, newCloseCodeLoc);
		setBL(0x02086310, newCloseCodeLoc);
		*(u32*)0x020863B0 = 0xE1A00000; // nop
	}

	// Pop+ Solo (USA)
	// Pop+ Solo (Europe, Australia)
	else if (strcmp(romTid, "KPIE") == 0 || strcmp(romTid, "KPIV") == 0) {
		*(u32*)0x0200517C = 0xE1A00000; // nop
		*(u32*)0x02005184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02019EBC = 0xE3A00000; // mov r0, #0
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
		if (romTid[3] == 'E') {
			if (!extendedMemory2) {
				*(u32*)0x02026FD0 = 0xE3A02601; // mov r2, #0x100000
				*(u32*)0x02026FE0 = 0xE3A01601; // mov r1, #0x100000
			}
			*(u32*)0x0204CF04 = 0xE1A00000; // nop
			*(u32*)0x02050940 = 0xE1A00000; // nop
			patchInitDSiWare(0x0205706C, heapEnd);
			patchUserSettingsReadDSiWare(0x02058688);
		} else {
			if (!extendedMemory2) {
				*(u32*)0x02026F84 = 0xE3A02601; // mov r2, #0x100000
				*(u32*)0x02026F94 = 0xE3A01601; // mov r1, #0x100000
			}
			*(u32*)0x0204CEB8 = 0xE1A00000; // nop
			*(u32*)0x020508F4 = 0xE1A00000; // nop
			patchInitDSiWare(0x02057020, heapEnd);
			patchUserSettingsReadDSiWare(0x0205863C);
		}
	}

	// GO Series: Portable Shrine Wars (USA)
	// GO Series: Portable Shrine Wars (Europe)
	// Omiko Shiuzu (Japan)
	else if (strncmp(romTid, "KOQ", 3) == 0) {
		*(u32*)0x020073E0 = 0xE1A00000; // nop
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
		*(u32*)0x0200E004 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		if (romTid[3] != 'J') {
			*(u32*)0x0204E1B8 = 0xE1A00000; // nop
			tonccpy((u32*)0x0204ED3C, dsiSaveGetResultCode, 0xC);
			*(u32*)0x02051F6C = 0xE1A00000; // nop
			patchInitDSiWare(0x020599E0, heapEnd);
			patchUserSettingsReadDSiWare(0x0205AEBC);
		} else {
			*(u32*)0x0204E02C = 0xE1A00000; // nop
			tonccpy((u32*)0x0204EBB0, dsiSaveGetResultCode, 0xC);
			*(u32*)0x02051DE0 = 0xE1A00000; // nop
			patchInitDSiWare(0x02059854, heapEnd);
			patchUserSettingsReadDSiWare(0x0205AD30);
		}

		// Skip Manual screen
		for (int i = 0; i < 11; i++) {
			u32* offset = (u32*)0x0200DE78;
			offset[i] = 0xE1A00000; // nop
		}
	}

	// Art Style: precipice (USA)
	// Art Style: KUBOS (Europe, Australia)
	// Art Style: nalaku (Japan)
	else if (strncmp(romTid, "KAK", 3) == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
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
		if (romTid[3] == 'E' || romTid[3] == 'V') {
			*(u32*)0x0200B370 = 0xE1A00000; // nop
			*(u32*)0x0200B37C = 0xE1A00000; // nop
			*(u32*)0x0200B3AC = 0xE1A00000; // nop
			*(u32*)0x0200B3B0 = 0xE1A00000; // nop
			*(u32*)0x0200B3B8 = 0xE1A00000; // nop
		} else {
			*(u32*)0x0200A654 = 0xE1A00000; // nop
			*(u32*)0x0200A660 = 0xE1A00000; // nop
			*(u32*)0x0200A690 = 0xE1A00000; // nop
			*(u32*)0x0200A694 = 0xE1A00000; // nop
			*(u32*)0x0200A69C = 0xE1A00000; // nop
		}
		if (romTid[3] == 'E') {
			/* if (useSharedFont && !extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02017E9C, 0x02049F70);
				setBL(0x02017EAC, 0x02049F74);
				setBL(0x02017EC0, 0x02049F74);
			} */
			*(u32*)0x0203B6AC = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x0203B6B0 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x0203EAC0 = 0xE1A00000; // nop
			patchInitDSiWare(0x02047980, heapEnd);
			patchUserSettingsReadDSiWare(0x02048F2C);
		} else if (romTid[3] == 'V') {
			*(u32*)0x0203B6A4 = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x0203B6A8 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x0203EAB8 = 0xE1A00000; // nop
			patchInitDSiWare(0x02047978, heapEnd);
			patchUserSettingsReadDSiWare(0x02048F24);
		} else {
			*(u32*)0x0203A528 = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x0203A52C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x0203D93C = 0xE1A00000; // nop
			patchInitDSiWare(0x020467E0, heapEnd);
			patchUserSettingsReadDSiWare(0x02047D8C);
		}
	}

	// Prehistorik Man (USA)
	else if (strcmp(romTid, "KPHE") == 0) {
		*(u32*)0x0200BC78 = 0xE1A00000; // nop
		*(u32*)0x0200F41C = 0xE1A00000; // nop
		patchInitDSiWare(0x02015378, heapEnd);
		patchUserSettingsReadDSiWare(0x02016960);
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
	else if (strcmp(romTid, "KPHV") == 0) {
		*(u32*)0x0200BC80 = 0xE1A00000; // nop
		*(u32*)0x0200F430 = 0xE1A00000; // nop
		patchInitDSiWare(0x020153A8, heapEnd);
		patchUserSettingsReadDSiWare(0x020169A0);
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

	// Zekkyou Genshiji: Samu no Daibouken (Japan)
	else if (strcmp(romTid, "KPHJ") == 0) {
		setB(0x02038A68, 0x02038CFC); // Skip Manual screen
		setBL(0x020487DC, (u32)dsiSaveOpen);
		setBL(0x020487F8, (u32)dsiSaveGetLength);
		setBL(0x0204880C, (u32)dsiSaveRead);
		setBL(0x02048814, (u32)dsiSaveClose);
		setBL(0x02048854, (u32)dsiSaveGetInfo);
		setBL(0x02048864, (u32)dsiSaveGetResultCode);
		*(u32*)0x02048888 = 0xE1A00000; // nop
		setBL(0x020488B0, (u32)dsiSaveCreate);
		setBL(0x020488B8, (u32)dsiSaveGetResultCode);
		*(u32*)0x020488DC = 0xE1A00000; // nop
		setBL(0x0204890C, (u32)dsiSaveOpen);
		setBL(0x0204891C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02048934 = 0xE1A00000; // nop
		setBL(0x02048960, (u32)dsiSaveSetLength);
		setBL(0x02048970, (u32)dsiSaveWrite);
		setBL(0x02048968, (u32)dsiSaveClose);
		patchInitDSiWare(0x0205F024, heapEnd);
		patchUserSettingsReadDSiWare(0x0206061C);
		*(u32*)0x0206416C = 0xE1A00000; // nop
		*(u32*)0x02068FA0 = 0xE1A00000; // nop
	}

	// The Price Is Right (Europe, Australia)
	else if (strcmp(romTid, "KPRV") == 0) {
		*(u32*)0x02005498 = 0xE1A00000; // nop
		*(u32*)0x020054B0 = 0xE1A00000; // nop
		*(u32*)0x02005940 = 0xE1A00000; // nop
		*(u32*)0x020E577C = 0xE1A00000; // nop
		*(u32*)0x020E8D40 = 0xE1A00000; // nop
		patchInitDSiWare(0x020F0088, heapEnd);
		*(u32*)0x020F0414 = *(u32*)0x02004FC0;
		setBL(0x02088D38, (u32)dsiSaveOpen);
		setBL(0x02088D4C, (u32)dsiSaveGetLength);
		setBL(0x02088D60, (u32)dsiSaveClose);
		setBL(0x02088D68, (u32)dsiSaveDelete);
		setBL(0x02088D80, (u32)dsiSaveCreate);
		*(u32*)0x02088D98 = 0xE1A00000; // nop
		setBL(0x02088DA4, (u32)dsiSaveCreate);
		setBL(0x02088DC8, (u32)dsiSaveOpen);
		setBL(0x02088E08, (u32)dsiSaveWrite);
		setBL(0x02088E28, (u32)dsiSaveClose);
		setBL(0x02088E50, (u32)dsiSaveClose);
		setBL(0x02088E94, (u32)dsiSaveOpen);
		setBL(0x02088EB8, (u32)dsiSaveSeek);
		setBL(0x02088EDC, (u32)dsiSaveRead);
		setBL(0x02088EE8, (u32)dsiSaveClose);
		setBL(0x02088F38, (u32)dsiSaveOpen);
		setBL(0x02088F5C, (u32)dsiSaveSeek);
		setBL(0x02088F80, (u32)dsiSaveWrite);
		setBL(0x02088F8C, (u32)dsiSaveGetLength);
		setBL(0x02088F98, (u32)dsiSaveClose);
		setBL(0x02088FE0, (u32)dsiSaveOpen);
		setBL(0x02088FF0, (u32)dsiSaveClose);
	}

	// Primrose (USA)
	// Primrose (Europe)
	else if (strcmp(romTid, "K2RE") == 0 || strcmp(romTid, "K2RP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x2C;
		const u32 newLoadCode = 0x0203869C+offsetChange;

		codeCopy((u32*)newLoadCode, (u32*)(0x02019F6C+offsetChange), 0xE0);
		setBL(newLoadCode+0x50, (u32)dsiSaveOpenR);
		setBL(newLoadCode+0x68, (u32)dsiSaveGetLength);
		setBL(newLoadCode+0xA4, (u32)dsiSaveRead);
		setBL(newLoadCode+0xD0, (u32)dsiSaveClose);

		setBL(0x020053F0, newLoadCode);
		*(u32*)0x02005410 = 0xE1A00000; // nop
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02011980, 0x0204099C+offsetChange);
			}
		} else {
			*(u32*)0x020118CC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		setBL(0x0201A06C+offsetChange, (u32)dsiSaveOpenR);
		setBL(0x0201A084+offsetChange, (u32)dsiSaveClose);
		setBL(0x0201A0FC+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0201A114+offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x0201A148+offsetChange) = 0xE1A00000; // nop (dsiSaveCreateDir)
		setBL(0x0201A154+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0201A184+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0201A1A8+offsetChange, (u32)dsiSaveWrite);
		setBL(0x0201A1B0+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x02037128+offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x02037CA0+offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0203A508+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0203EF8C+offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x02040458+offsetChange);
	}

	// Pro-Jumper! Chimaki's Hot Spring Tour! Guilty Gear Tangent!? (USA)
	// ARC Style: Furo Jump!! Girutegia Gaiden! (Japan)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KFVE") == 0 || strcmp(romTid, "KFVJ") == 0) && extendedMemory2) {
		const u32 newFunc = 0x02066100;

		/* if (!extendedMemory2) {
			*(u32*)0x02005260 = 0xE1A00000; // nop
			if (s2FlashcardId == 0x5A45) {
				*(u32*)0x02005264 = 0xE3A00408; // mov r0, #0x08000000
			} else {
				*(u32*)0x02005264 = 0xE3A00409; // mov r0, #0x09000000
			}
			*(u32*)0x02005268 = 0xE3A01716; // mov r1, #0x580000
		} */
		*(u32*)0x0200D6C8 = 0xE1A00000; // nop
		setBL(0x0200D728, newFunc);
		*(u32*)0x0200D764 = 0xE1A00000; // nop
		codeCopy((u32*)newFunc, (u32*)0x0200D878, 0xC0);
		setBL(newFunc+0x28, (u32)dsiSaveOpen);
		setBL(newFunc+0x40, (u32)dsiSaveGetLength);
		setBL(newFunc+0x5C, (u32)dsiSaveRead);
		setBL(newFunc+0x8C, (u32)dsiSaveClose);
		setBL(0x0200D960, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0200D970, (u32)dsiSaveOpen);
		setBL(0x0200D990, (u32)dsiSaveWrite);
		setBL(0x0200D9A8, (u32)dsiSaveWrite);
		*(u32*)0x02064A94 = 0xE1A00000; // nop
		tonccpy((u32*)0x02065618, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02068D78 = 0xE1A00000; // nop
		patchInitDSiWare(0x02070CE4, heapEnd);
		patchUserSettingsReadDSiWare(0x02072394);
		*(u32*)0x02077C78 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02077C7C = 0xE12FFF1E; // bx lr
	}

	// Pro-Putt Domo (USA)
	else if (strcmp(romTid, "KDPE") == 0) {
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
		doubleNopT(0x0201096A);
		*(u16*)0x020109AC = 0x2001; // movs r0, #1 (dsiSaveGetInfo)
		*(u16*)0x020109AE = 0x4770; // bx lr
		setBLThumb(0x02010A12, dsiSaveCreateT);
		setBLThumb(0x02010A28, dsiSaveOpenT);
		setBLThumb(0x02010A44, dsiSaveSetLengthT);
		setBLThumb(0x02010A58, dsiSaveWriteT);
		setBLThumb(0x02010A6A, dsiSaveCloseT);
		*(u16*)0x02010A90 = 0x4778; // bx pc
		tonccpy((u32*)0x02010A94, dsiSaveGetLength, 0xC);
		setBLThumb(0x02010AC0, dsiSaveOpenT);
		setBLThumb(0x02010AE6, dsiSaveCloseT);
		setBLThumb(0x02010AF8, dsiSaveReadT);
		setBLThumb(0x02010AFE, dsiSaveCloseT);
		setBLThumb(0x02010B12, dsiSaveDeleteT);
		*(u16*)0x020179F4 = 0x4770; // bx lr (Disable NFTR loading from TWLNAND)
		doubleNopT(0x02026262);
		doubleNopT(0x020284B2);
		doubleNopT(0x0202B2D4);
		doubleNopT(0x0202C88E);
		doubleNopT(0x0202C892);
		doubleNopT(0x0202C89E);
		doubleNopT(0x0202C982);
		patchHiHeapDSiWareThumb(0x0202C9C0, 0x0202716C, heapEnd); // movs r0, #0x23E0000
		doubleNopT(0x0202DA1E);
		*(u16*)0x0202DA22 = nopT;
		*(u16*)0x0202DA24 = nopT;
		doubleNopT(0x0202DA26);
	}

	// Publisher Dream (USA)
	// Publisher Dream (Europe)
	// Publisher Dream (Japan)
	else if (strncmp(romTid, "KXU", 3) == 0) {
		if (!extendedMemory2) {
			// Disable audio (Part 1)
			u32 bssEnd = *(u32*)0x02004FE8;
			u32 sdatOffset = 0x0209377C;
			if (romTid[3] == 'P') {
				sdatOffset = 0x0209371C;
			} else if (romTid[3] == 'J') {
				sdatOffset = 0x02093B7C;
			}

			*(u32*)0x02004FE8 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x200000, bssEnd, sdatOffset);
		}

		s16 offsetChange = (romTid[3] == 'E') ? 0 : -0x68;
		s16 offsetChangeA = (romTid[3] == 'E') ? 0 : -0x68;
		if (romTid[3] == 'J') {
			offsetChange += 0x500;
			offsetChangeA -= 0x120;
		}

		*(u32*)0x02010140 = 0xE1A00000; // nop
		tonccpy((u32*)0x02010CC4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02013494 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019570, heapEnd);
		*(u32*)0x020198FC = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201AA10);
		if (!extendedMemory2) {
			// Disable audio (Part 2)
			*(u32*)0x0201C568 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0201C56C = 0xE12FFF1E; // bx lr
			*(u32*)0x0201C590 = 0xE3A00000; // mov r0, #0
			*(u32*)0x0201C594 = 0xE12FFF1E; // bx lr

			*(u32*)0x0201F6B4 = 0xE1A00000; // nop

			u32* offset = getOffsetFromBL((u32*)0x0201F9C0);
			offset[0] = 0xE3A00000; // mov r0, #0
			offset[1] = 0xE12FFF1E; // bx lr

			*(u32*)(0x02064DCC+offsetChangeA) = 0xE3A00000; // mov r0, #0
			*(u32*)(0x02064DD0+offsetChangeA) = 0xE12FFF1E; // bx lr
			*(u32*)(0x02064E30+offsetChangeA) = 0xE3A00000; // mov r0, #0
			*(u32*)(0x02064E34+offsetChangeA) = 0xE12FFF1E; // bx lr
		}
		setBL(0x0206F764+offsetChange, (u32)dsiSaveGetInfo);
		setBL(0x0206F778+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0206F78C+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0206F79C+offsetChange, (u32)dsiSaveOpen);
		*(u32*)(0x0206F7BC+offsetChange) = 0xE1A00000; // nop
		setBL(0x0206F7C8+offsetChange, (u32)dsiSaveCreate);
		setBL(0x0206F7D8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0206F820+offsetChange, (u32)dsiSaveSeek);
		setBL(0x0206F830+offsetChange, (u32)dsiSaveWrite);
		setBL(0x0206F838+offsetChange, (u32)dsiSaveClose);
		setBL(0x0206F884+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0206FD00+offsetChange, (u32)dsiSaveSeek);
		setBL(0x0206FD10+offsetChange, (u32)dsiSaveRead);
		setBL(0x0206FD3C+offsetChange, (u32)dsiSaveClose);
		setBL(0x02070358+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02070374+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02070384+offsetChange, (u32)dsiSaveOpen);
		*(u32*)(0x020703A4+offsetChange) = 0xE1A00000; // nop
		setBL(0x020703B0+offsetChange, (u32)dsiSaveCreate);
		setBL(0x020703C0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020703D8+offsetChange, (u32)dsiSaveSeek);
		setBL(0x020703F0+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020703F8+offsetChange, (u32)dsiSaveClose);
	}

	// Pucca: Noodle Rush (Europe)
	else if (strcmp(romTid, "KNUP") == 0) {
		const u32 dsiSaveCreateT = 0x02058F8C;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveSetLengthT = 0x02058F9C;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02058FAC;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02058FBC;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveSeekT = 0x02058FCC;
		*(u16*)dsiSaveSeekT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSeekT + 4), dsiSaveSeek, 0xC);

		const u32 dsiSaveReadT = 0x02058FDC;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02058FEC;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		setBLThumb(0x020086EE, dsiSaveOpenT);
		setBLThumb(0x0200870A, dsiSaveSeekT);
		setBLThumb(0x0200871C, dsiSaveCloseT);
		setBLThumb(0x0200872A, dsiSaveReadT);
		setBLThumb(0x0200873C, dsiSaveCloseT);
		setBLThumb(0x02008746, dsiSaveCloseT);
		*(u32*)0x0200876C = 0x4770; // bx lr
		doubleNopT(0x020087A0);
		setBLThumb(0x020087AE, dsiSaveCreateT);
		setBLThumb(0x020087C2, dsiSaveOpenT);
		setBLThumb(0x020087E6, dsiSaveSetLengthT);
		setBLThumb(0x020087F6, dsiSaveCloseT);
		setBLThumb(0x0200880C, dsiSaveSeekT);
		setBLThumb(0x0200881E, dsiSaveCloseT);
		setBLThumb(0x02008830, dsiSaveWriteT);
		setBLThumb(0x02008842, dsiSaveCloseT);
		setBLThumb(0x02008850, dsiSaveCloseT);
		*(u16*)0x0200AF94 = 0x2000; // movs r0, #0
		*(u32*)0x02057A74 = 0xE1A00000; // nop
		*(u32*)0x0205AE94 = 0xE1A00000; // nop
		patchInitDSiWare(0x02060910, heapEnd);
		patchUserSettingsReadDSiWare(0x02062040);
		*(u32*)0x020677DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020677E0 = 0xE12FFF1E; // bx lr
	}

	// Puffins: Let's Fish! (USA)
	// Puffins: Let's Fish! (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KLFE") == 0 || strcmp(romTid, "KLFP") == 0) {
		u8 offsetChange = (romTid[3] == 'P') ? 8 : 0;

		*(u32*)0x020195B4 = 0xE12FFF1E; // bx lr
		*(u32*)0x02019608 = 0xE12FFF1E; // bx lr
		setBL(0x0202BA80-offsetChange, (u32)dsiSaveGetLength);
		setBL(0x0202BB78-offsetChange, (u32)dsiSaveRead);
		setBL(0x0202BC68-offsetChange, (u32)dsiSaveSeek);
		setBL(0x0202BD00-offsetChange, (u32)dsiSaveWrite);
		setBL(0x0202BDBC-offsetChange, (u32)dsiSaveClose);
		setBL(0x0202BE08-offsetChange, (u32)dsiSaveOpen);
		setBL(0x0202BE68-offsetChange, (u32)dsiSaveCreate);
		*(u32*)(0x0202BED4-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0204AFD0-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02069374-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0206C6B4-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x02074794-offsetChange, heapEnd8MBHack);
		*(u32*)(0x02074A80-offsetChange) -= 0x30000;
		patchUserSettingsReadDSiWare(0x02075DA8-offsetChange);
	}

	// Puffins: Let's Race! (USA)
	// Puffins: Let's Race! (Europe)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KLRE") == 0 || strcmp(romTid, "KLRP") == 0) {
		setBL(0x020289D0, (u32)dsiSaveGetLength);
		setBL(0x02028AC8, (u32)dsiSaveRead);
		setBL(0x02028BB8, (u32)dsiSaveSeek);
		setBL(0x02028C50, (u32)dsiSaveWrite);
		setBL(0x02028D0C, (u32)dsiSaveClose);
		setBL(0x02028D58, (u32)dsiSaveOpen);
		setBL(0x02028DB8, (u32)dsiSaveCreate);
		*(u32*)0x02028E24 = 0xE1A00000; // nop
		*(u32*)0x0202F128 = 0xE1A00000; // nop
		*(u32*)0x020733CC = 0xE1A00000; // nop
		*(u32*)0x02076668 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207EC3C, heapEnd8MBHack);
		*(u32*)0x0207EFC8 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02080334);
		*(u32*)0x02085B84 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02085B88 = 0xE12FFF1E; // bx lr
	}

	// Puffins: Let's Roll! (USA)
	// Due to our save implementation, save data is stored in all 3 slots
	// Requires more than 8MB of RAM?
	/*else if (strcmp(romTid, "KL2E") == 0) {
		*(u32*)0x0204BF1C = (u32)dsiSaveGetLength;
		setBL(0x0204BFA0, (u32)dsiSaveRead);
		setBL(0x0204C074, (u32)dsiSaveSeek);
		setBL(0x0204C10C, (u32)dsiSaveWrite);
		setBL(0x0204C1B4, (u32)dsiSaveClose);
		setBL(0x0204C1E8, (u32)dsiSaveOpen);
		setBL(0x0204C230, (u32)dsiSaveCreate);
		*(u32*)0x0204C63C = 0xE1A00000; // nop
		*(u32*)0x020716A0 = 0xE1A00000; // nop
		*(u32*)0x02074884 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207C640, heapEnd8MBHack);
		*(u32*)0x0207C9CC -= 0x30000;
		patchUserSettingsReadDSiWare(0x0207DC54);
	}*/

	// Puzzle League: Express (USA)
	// Some code seems to make save reading fail, preventing support
	else if (strcmp(romTid, "KPNE") == 0) {
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02023614 = 0xE3A00001; // mov r0, #1 (Hide volume icon in gameplay)
		*(u32*)0x0203604C = 0xE1A00000; // nop
		*(u32*)0x0205663C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02056640 = 0xE12FFF1E; // bx lr
		*(u32*)0x02056A28 = 0xE12FFF1E; // bx lr
		/* setBL(0x02057A80, (u32)dsiSaveOpen);
		setBL(0x02057A98, (u32)dsiSaveRead);
		setBL(0x02057AB0, (u32)dsiSaveClose);
		setBL(0x02057B34, (u32)dsiSaveOpen);
		setBL(0x02057B4C, (u32)dsiSaveWrite);
		setBL(0x02057B64, (u32)dsiSaveClose);
		setBL(0x02057BB8, 0x02057C54);
		// *(u32*)0x02057BC4 = 0xE3A00000; // mov r0, #0 (dsiSaveOpenDir)
		// *(u32*)0x02057BFC = 0xE1A00000; // nop (dsiSaveCloseDir)
		// *(u32*)0x02057C08 = 0xE3A00000; // mov r0, #0
		// *(u32*)0x02057C38 = 0xE3A00001; // mov r0, #1 (dsiSaveCreateDirAuto)
		setBL(0x02057C84, (u32)dsiSaveOpen);
		setBL(0x02057C94, (u32)dsiSaveGetLength);
		setBL(0x02057CA8, (u32)dsiSaveSetLength);
		setBL(0x02057CC4, (u32)dsiSaveClose);
		setBL(0x02057CF8, (u32)dsiSaveCreate);
		setBL(0x02057D10, (u32)dsiSaveOpen);
		setBL(0x02057D24, (u32)dsiSaveSetLength);
		setBL(0x02057D40, (u32)dsiSaveClose); */
		*(u32*)0x02064DD0 = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020ACF54 = 0xE1A00000; // nop
		//tonccpy((u32*)0x020ADBF4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020B1334 = 0xE1A00000; // nop
		patchInitDSiWare(0x020BFF78, heapEnd8MBHack);
		patchUserSettingsReadDSiWare(0x020C1668);
	}

	// A Little Bit of... Puzzle League (Europe, Australia)
	else if (strcmp(romTid, "KPNV") == 0) {
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x02005094 = 0xE1A00000; // nop
		*(u32*)0x020050D0 = 0xE1A00000; // nop
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x02023634 = 0xE3A00001; // mov r0, #1 (Hide volume icon in gameplay)
		*(u32*)0x02036FA0 = 0xE1A00000; // nop
		*(u32*)0x020575FC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02057600 = 0xE12FFF1E; // bx lr
		*(u32*)0x020579E8 = 0xE12FFF1E; // bx lr
		*(u32*)0x02065DE4 = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020AE94C = 0xE1A00000; // nop
		*(u32*)0x020BD2DC = 0xE1A00000; // nop
		patchInitDSiWare(0x020C1970, heapEnd8MBHack);
		patchUserSettingsReadDSiWare(0x020C3060);
	}

	// Chotto Panel de Pon (Japan)
	else if (strcmp(romTid, "KPNJ") == 0) {
		*(u32*)0x02005068 = 0xE1A00000; // nop
		*(u32*)0x02005070 = 0xE1A00000; // nop
		*(u32*)0x020050AC = 0xE1A00000; // nop
		*(u32*)0x020050B0 = 0xE1A00000; // nop
		*(u32*)0x020050C4 = 0xE1A00000; // nop
		*(u32*)0x02023404 = 0xE3A00001; // mov r0, #1 (Hide volume icon in gameplay)
		*(u32*)0x02035BB8 = 0xE1A00000; // nop
		*(u32*)0x02056128 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0205612C = 0xE12FFF1E; // bx lr
		*(u32*)0x02056514 = 0xE12FFF1E; // bx lr
		*(u32*)0x02064FEC = 0xE3A00001; // mov r0, #1 (Hide volume icon in menu)
		*(u32*)0x020ADCCC = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x020ADCD0 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x020B21BC = 0xE1A00000; // nop
		patchInitDSiWare(0x020C2934, heapEnd8MBHack);
		patchUserSettingsReadDSiWare(0x020C4030);
	}

	// Puzzle Rocks (USA)
	// Audio does not play
	else if (strcmp(romTid, "KPLE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x020182D8 = 0xE1A00000; // nop
		*(u32*)0x0201C608 = 0xE1A00000; // nop
		patchInitDSiWare(0x02021BDC, heapEnd);
		patchUserSettingsReadDSiWare(0x020231C0);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02042A20, 0x02023704);
			*(u32*)0x02042A6C = 0xE1A00000; // nop
		}
		setBL(0x02047E20, (u32)dsiSaveCreate);
		setBL(0x02047E30, (u32)dsiSaveOpen);
		setBL(0x02047E4C, (u32)dsiSaveGetResultCode);
		setBL(0x02047E74, (u32)dsiSaveSeek);
		setBL(0x02047E8C, (u32)dsiSaveGetResultCode);
		setBL(0x02047EB4, (u32)dsiSaveWrite);
		setBL(0x02047EBC, (u32)dsiSaveClose);
		setBL(0x02047EC4, (u32)dsiSaveGetResultCode);
		setBL(0x02047EE4, (u32)dsiSaveGetResultCode);
		setBL(0x02047F24, (u32)dsiSaveOpenR);
		setBL(0x02047F34, (u32)dsiSaveGetLength);
		setBL(0x02047F68, (u32)dsiSaveRead);
		setBL(0x02047F80, (u32)dsiSaveClose);
		setBL(0x02047F88, (u32)dsiSaveGetResultCode);
		setBL(0x02047F98, (u32)dsiSaveGetResultCode);
		setBL(0x02047FAC, (u32)dsiSaveGetResultCode);
		setBL(0x02047FC0, (u32)dsiSaveGetResultCode);
		*(u32*)0x02051F0C = 0xE12FFF1E; // bx lr
	}

	// Puzzle Rocks (Europe)
	// Audio does not play
	else if (strcmp(romTid, "KPLP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0200DC7C = 0xE1A00000; // nop
		*(u32*)0x02011FAC = 0xE1A00000; // nop
		patchInitDSiWare(0x02017580, heapEnd);
		patchUserSettingsReadDSiWare(0x02018B64);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x020383C4, 0x020190A8);
			*(u32*)0x02038410 = 0xE1A00000; // nop
		}
		setBL(0x0203D7C4, (u32)dsiSaveCreate);
		setBL(0x0203D7D4, (u32)dsiSaveOpen);
		setBL(0x0203D7F0, (u32)dsiSaveGetResultCode);
		setBL(0x0203D818, (u32)dsiSaveSeek);
		setBL(0x0203D830, (u32)dsiSaveGetResultCode);
		setBL(0x0203D858, (u32)dsiSaveWrite);
		setBL(0x0203D860, (u32)dsiSaveClose);
		setBL(0x0203D868, (u32)dsiSaveGetResultCode);
		setBL(0x0203D888, (u32)dsiSaveGetResultCode);
		setBL(0x0203D8C8, (u32)dsiSaveOpenR);
		setBL(0x0203D8D8, (u32)dsiSaveGetLength);
		setBL(0x0203D90C, (u32)dsiSaveRead);
		setBL(0x0203D924, (u32)dsiSaveClose);
		setBL(0x0203D92C, (u32)dsiSaveGetResultCode);
		setBL(0x0203D93C, (u32)dsiSaveGetResultCode);
		setBL(0x0203D950, (u32)dsiSaveGetResultCode);
		setBL(0x0203D964, (u32)dsiSaveGetResultCode);
		*(u32*)0x020478B0 = 0xE12FFF1E; // bx lr
	}

	// Puzzler Brain Games (USA)
	else if (strcmp(romTid, "KYEE") == 0) {
		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x020050F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020051BC = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02014F50 = 0xE1A00000; // nop
		tonccpy((u32*)0x02015AD4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02018134 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D25C, heapEnd);
		*(u32*)0x0201D5E8 = 0x020F7520;
		patchUserSettingsReadDSiWare(0x0201E738);
		setBL(0x02026984, (u32)dsiSaveOpen);
		setBL(0x020269CC, (u32)dsiSaveGetLength);
		setBL(0x02026A14, (u32)dsiSaveRead);
		setBL(0x02026A30, (u32)dsiSaveClose);
		*(u32*)0x02026AF0 = 0xE1A00000; // nop
		setBL(0x02026B24, (u32)dsiSaveCreate);
		setBL(0x02026B34, (u32)dsiSaveOpen);
		setBL(0x02026B80, (u32)dsiSaveSetLength);
		setBL(0x02026B94, (u32)dsiSaveWrite);
		setBL(0x02026BA8, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)0x0208AA08 = 0xE3A0170B; // mov r1, #0x2C0000
		}
	}

	// Puzzler World 2013 (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYGE") == 0 && extendedMemory2) {
		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x020050F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020051BC = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x020148A8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201542C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02017A8C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201CD94, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E270);
		setBL(0x0202663C, (u32)dsiSaveOpen);
		setBL(0x02026684, (u32)dsiSaveGetLength);
		setBL(0x020266CC, (u32)dsiSaveRead);
		setBL(0x020266E8, (u32)dsiSaveClose);
		*(u32*)0x020267A8 = 0xE1A00000; // nop
		setBL(0x020267DC, (u32)dsiSaveCreate);
		setBL(0x020267EC, (u32)dsiSaveOpen);
		setBL(0x02026838, (u32)dsiSaveSetLength);
		setBL(0x0202684C, (u32)dsiSaveWrite);
		setBL(0x02026860, (u32)dsiSaveClose);
	}

	// Puzzler World XL (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KUOE") == 0 && extendedMemory2) {
		*(u32*)0x02005088 = 0xE1A00000; // nop
		*(u32*)0x020050F8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020051BC = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02014F4C = 0xE1A00000; // nop
		tonccpy((u32*)0x02015AD0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02018130 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D258, heapEnd);
		//*(u32*)0x0201D5E4 = 0x02133AC0;
		patchUserSettingsReadDSiWare(0x0201E374);
		setBL(0x0202693C, (u32)dsiSaveOpen);
		setBL(0x02026984, (u32)dsiSaveGetLength);
		setBL(0x020269CC, (u32)dsiSaveRead);
		setBL(0x020269E8, (u32)dsiSaveClose);
		*(u32*)0x02026AA8 = 0xE1A00000; // nop
		setBL(0x02026ADC, (u32)dsiSaveCreate);
		setBL(0x02026AEC, (u32)dsiSaveOpen);
		setBL(0x02026B38, (u32)dsiSaveSetLength);
		setBL(0x02026B4C, (u32)dsiSaveWrite);
		setBL(0x02026B60, (u32)dsiSaveClose);
	}

	// Puzzle to Go: Baby Animals (Europe)
	else if (strcmp(romTid, "KBYP") == 0) {
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02005134 = 0xE1A00000; // nop
		*(u32*)0x020052E4 = 0xE1A00000; // nop
		*(u32*)0x020139E0 = 0xE1A00000; // nop
		*(u32*)0x0201759C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201ECF0, heapEnd);
		patchUserSettingsReadDSiWare(0x02020304);
		*(u32*)0x02025444 = 0xE1A00000; // nop
		*(u32*)0x0202551C = 0xE1A00000; // nop

		// Skip Manual screen
		*(u32*)0x02032AFC = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02032B80 = 0xE1A00000; // nop
		*(u32*)0x02032B88 = 0xE1A00000; // nop
		*(u32*)0x02032B94 = 0xE1A00000; // nop

		*(u32*)0x02032C0C = 0xE1A00000; // nop
		*(u32*)0x02034D08 = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KPUP") == 0 || strcmp(romTid, "KPDP") == 0) {
		*(u32*)0x020138D4 = 0xE1A00000; // nop
		*(u32*)0x020175C8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201F050, heapEnd);
		patchUserSettingsReadDSiWare(0x02020694);
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
	else if (strcmp(romTid, "KBXP") == 0 || strcmp(romTid, "KB3P") == 0) {
		*(u32*)0x02005130 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x020052FC = 0xE1A00000; // nop
		*(u32*)0x020139F8 = 0xE1A00000; // nop
		*(u32*)0x02017648 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201EDE0, heapEnd);
		patchUserSettingsReadDSiWare(0x020203F8);
		*(u32*)0x02025538 = 0xE1A00000; // nop
		*(u32*)0x02025610 = 0xE1A00000; // nop

		// Skip Manual screen
		*(u32*)0x02032BF0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02032C74 = 0xE1A00000; // nop
		*(u32*)0x02032C7C = 0xE1A00000; // nop
		*(u32*)0x02032C88 = 0xE1A00000; // nop

		*(u32*)0x02032D00 = 0xE1A00000; // nop
		*(u32*)0x02034DFC = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KUME") == 0 || strcmp(romTid, "KUMP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02010300 = 0xE1A00000; // nop
		*(u32*)0x020139A4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02019170, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A740);
		// *(u32*)0x0203EA70 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020330EC, 0x0201ACB0);
			}
		} else {
			*(u32*)0x02040240 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Anaume Pazuru Gemu Q (Japan)
	// A bit hard/confusing to add save support
	else if (strcmp(romTid, "KUMJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020102DC = 0xE1A00000; // nop
		*(u32*)0x02013980 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201914C, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A71C);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020330C8, 0x0201AC8C);
			}
		} else {
			*(u32*)0x02040460 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// QuickPick Farmer (USA)
	// QuickPick Farmer (Europe)
	else if (strcmp(romTid, "K9PE") == 0 || strcmp(romTid, "K9PP") == 0) {
		*(u32*)0x0200D850 = 0xE1A00000; // nop
		*(u32*)0x02011458 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016ED4, heapEnd);
		*(u32*)0x02017260 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02018610);
		*(u32*)0x0201FE80 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0201FE9C = 0xE3A01A2F; // mov r1, #0x2F000 (Shrink small heap from 0xAF000)
			*(u32*)0x0202819C = 0x1B4800; // Shrink large heap from 0x1C4800
		}
		*(u32*)0x0201FFD8 = 0xE1A00000; // nop
		*(u32*)0x0201FFEC = 0xE1A00000; // nop
		setBL(0x0206CF08, (u32)dsiSaveOpen);
		setBL(0x0206CF3C, (u32)dsiSaveGetLength);
		setBL(0x0206CF7C, (u32)dsiSaveRead);
		setBL(0x0206CFA0, (u32)dsiSaveClose);
		*(u32*)0x0206D014 = 0xE1A00000; // nop
		setBL(0x0206D020, (u32)dsiSaveCreate);
		setBL(0x0206D030, (u32)dsiSaveOpen);
		setBL(0x0206D04C, (u32)dsiSaveGetResultCode);
		setBL(0x0206D07C, (u32)dsiSaveSetLength);
		setBL(0x0206D0A8, (u32)dsiSaveWrite);
		setBL(0x0206D0CC, (u32)dsiSaveClose);
		setBL(0x0206D308, (u32)dsiSaveDelete);
	}

	// Rabi Laby (USA)
	// Rabi Laby (Europe)
	else if (strcmp(romTid, "KLBE") == 0 || strcmp(romTid, "KLBP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201DDF0, 0x02018120);
			}
		} else {
			*(u32*)0x020051C8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x020053A8 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x0200DC38 = 0xE1A00000; // nop
		*(u32*)0x020110D4 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016700, heapEnd);
		patchUserSettingsReadDSiWare(0x02017BA0);
		setBL(0x0201DEF8, (u32)dsiSaveOpen);
		setBL(0x0201DF1C, (u32)dsiSaveRead);
		setBL(0x0201DF2C, (u32)dsiSaveRead);
		setBL(0x0201DF34, (u32)dsiSaveClose);
		setBL(0x0201E1E4, (u32)dsiSaveOpen);
		setBL(0x0201E354, (u32)dsiSaveWrite);
		setBL(0x0201E35C, (u32)dsiSaveClose);
		if (romTid[3] == 'E') {
			*(u32*)0x0202663C = 0xE1A00000; // nop
			setBL(0x02026648, (u32)dsiSaveCreate);
			*(u32*)0x020266F8 = 0xE1A00000; // nop
			setBL(0x02026704, (u32)dsiSaveCreate);
		} else {
			*(u32*)0x020266CC = 0xE1A00000; // nop
			setBL(0x020266D8, (u32)dsiSaveCreate);
			*(u32*)0x02026788 = 0xE1A00000; // nop
			setBL(0x02026794, (u32)dsiSaveCreate);
		}
	}

	// Akushon Pazuru: Rabi x Rabi (Japan)
	else if (strcmp(romTid, "KLBJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201DC78, 0x02017FF0);
			}
		} else {
			*(u32*)0x02005190 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x02005360 = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x0200DBAC = 0xE1A00000; // nop
		*(u32*)0x02010FC0 = 0xE1A00000; // nop
		patchInitDSiWare(0x020165D0, heapEnd);
		patchUserSettingsReadDSiWare(0x02017A70);
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
		*(u32*)0x02026BD0 = 0xE1A00000; // nop
		setBL(0x02026BDC, (u32)dsiSaveCreate);
		*(u32*)0x02026CB4 = 0xE1A00000; // nop
		setBL(0x02026CC0, (u32)dsiSaveCreate);
	}

	// Rabi Laby 2 (USA)
	// Rabi Laby 2 (Europe)
	// Akushon Pazuru: Rabi x Rabi Episodo 2 (Japan)
	// Saving does not work properly
	else if (strncmp(romTid, "KLV", 3) == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02035658, 0x02017FCC);
			}
		} else {
			*(u32*)0x020051E8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0200540C = 0xE1A00000; // nop (Skip Manual screen)
		}
		*(u32*)0x0200DAE4 = 0xE1A00000; // nop
		*(u32*)0x02010F80 = 0xE1A00000; // nop
		patchInitDSiWare(0x020165AC, heapEnd);
		patchUserSettingsReadDSiWare(0x02017A4C);
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
		*(u32*)0x02035BF8 = 0xE1A00000; // nop
		*(u32*)0x02035CA8 = 0xE1A00000; // nop
	}

	// Real Crimes: Jack the Ripper (USA)
	else if (strcmp(romTid, "KRCE") == 0) {
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

		*(u32*)0x020103D8 = 0xE1A00000; // nop
		*(u32*)0x02013974 = 0xE1A00000; // nop
		patchInitDSiWare(0x020198E8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AFB8);
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
		*(u16*)0x020265B8 = nopT;
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
	else if (strcmp(romTid, "KRCV") == 0) {
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

		*(u32*)0x020103DC = 0xE1A00000; // nop
		*(u32*)0x02013978 = 0xE1A00000; // nop
		patchInitDSiWare(0x020198EC, heapEnd);
		patchUserSettingsReadDSiWare(0x0201AFBC);
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
		*(u16*)0x02026598 = nopT;
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
	else if (strcmp(romTid, "KLXJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020336CC, 0x0201AE24);
			}
		} else {
			*(u32*)0x02005254 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200E0F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201025C = 0xE1A00000; // nop
		*(u32*)0x020134D4 = 0xE1A00000; // nop
		*(u32*)0x02013DF8 = 0xE1A00000; // nop
		*(u32*)0x02013DFC = 0xE1A00000; // nop
		patchInitDSiWare(0x020191EC, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A89C);
		*(u32*)0x0202001C = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020020 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202D134 = 0xE1A00000; // nop
		*(u32*)0x0202E338 = 0xE1A00000; // nop
		*(u32*)0x0202E528 = 0xE1A00000; // nop
		*(u32*)0x0202E560 = 0xE1A00000; // nop
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
	// Remote Racers (Europe, Australia)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if ((strcmp(romTid, "KQRE") == 0 || strcmp(romTid, "KQRV") == 0) && debugOrMep) {
		extern u16* rmtRacersHeapAlloc;
		extern u16* rmtRacersHeapAddrPtr;

		*(u32*)0x020197F0 = 0xE1A00000; // nop
		*(u32*)0x0201CDA0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02023BC4, heapEnd);
		patchUserSettingsReadDSiWare(0x020250BC);
		if (!extendedMemory2) {
			if (s2FlashcardId == 0x5A45) {
				for (int i = 0; i < 3; i++) {
					rmtRacersHeapAddrPtr[i] -= 0x800000;
				}
			}
			tonccpy((u32*)0x020256C0, rmtRacersHeapAlloc, 0xC0);
			setBLThumb(0x02082484, 0x020256C0);
		}
		if (romTid[3] == 'E') {
			*(u32*)0x0208DCBC = 0xE1A00000; // nop
			*(u32*)0x0208DCC4 = 0xE1A00000; // nop
			*(u32*)0x0208DDB8 = 0xE1A00000; // nop
			*(u32*)0x0208E3A0 = 0xE1A00000; // nop
			*(u32*)0x0208E56C = 0xE1A00000; // nop
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
		} else {
			*(u32*)0x0208DC98 = 0xE1A00000; // nop
			*(u32*)0x0208DCA0 = 0xE1A00000; // nop
			*(u32*)0x0208DD94 = 0xE1A00000; // nop
			*(u32*)0x0208E37C = 0xE1A00000; // nop
			*(u32*)0x0208E548 = 0xE1A00000; // nop
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
	}

	// Renjuku Kanji: Shougaku 1 Nensei (Japan)
	// Renjuku Kanji: Shougaku 2 Nensei (Japan)
	// Renjuku Kanji: Shougaku 3 Nensei (Japan)
	// Renjuku Kanji: Shougaku 4 Nensei (Japan)
	// Renjuku Kanji: Shougaku 5 Nensei (Japan)
	// Renjuku Kanji: Shougaku 6 Nensei (Japan)
	// Renjuku Kanji: Chuugakusei (Japan)
	else if (strcmp(romTid, "KJZJ") == 0 || strcmp(romTid, "KJ2J") == 0 || strcmp(romTid, "KJ3J") == 0 || strcmp(romTid, "KJ4J") == 0
		   || strcmp(romTid, "KJ5J") == 0 || strcmp(romTid, "KJ6J") == 0 || strcmp(romTid, "KJ8J") == 0) {
		*(u32*)0x0200E618 = 0xE1A00000; // nop
		*(u32*)0x02011B38 = 0xE1A00000; // nop
		patchInitDSiWare(0x020177B4, heapEnd);
		patchUserSettingsReadDSiWare(0x02018F14);
		setBL(0x02029C0C, (u32)dsiSaveCreate);
		setBL(0x0202A37C, (u32)dsiSaveClose);
		if (strncmp(romTid, "KJZ", 3) == 0) {
			*(u32*)0x02048D68 = 0xE1A00000; // nop
			*(u32*)0x02048D7C = 0xE1A00000; // nop
			*(u32*)0x02049A64 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049C50 = 0xE3A00000; // mov r0, #0
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
		} else if (strncmp(romTid, "KJ2", 3) == 0 || strncmp(romTid, "KJ3", 3) == 0) {
			*(u32*)0x02048D50 = 0xE1A00000; // nop
			*(u32*)0x02048D64 = 0xE1A00000; // nop
			*(u32*)0x02049A4C = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049C38 = 0xE3A00000; // mov r0, #0
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
		} else if (strncmp(romTid, "KJ8", 3) != 0) {
			*(u32*)0x02048D50 = 0xE1A00000; // nop
			*(u32*)0x02048D64 = 0xE1A00000; // nop
			*(u32*)0x02049AE8 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049CD4 = 0xE3A00000; // mov r0, #0
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
		} else {
			*(u32*)0x02048D54 = 0xE1A00000; // nop
			*(u32*)0x02048D68 = 0xE1A00000; // nop
			*(u32*)0x02049A68 = 0xE3A00002; // mov r0, #2 (Skip Manual screen, Part 1)
			*(u32*)0x02049C54 = 0xE3A00000; // mov r0, #0
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
	}

	// Robot Rescue (USA)
	else if (strcmp(romTid, "KRTE") == 0) {
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
		*(u32*)0x0200C814 = 0xE1A00000; // nop
		*(u32*)0x0200C81C = 0xE1A00000; // nop
		setBL(0x0200C838, (u32)dsiSaveOpen);
		setBL(0x0200C84C, (u32)dsiSaveClose);
		setBL(0x0200C860, (u32)dsiSaveCreate);
		setBL(0x0200C878, (u32)dsiSaveOpen);
		*(u32*)0x0200C888 = 0xE1A00000; // nop
		setBL(0x0200C894, (u32)dsiSaveClose);
		setBL(0x0200C89C, (u32)dsiSaveDelete);
		setBL(0x0200C8B0, (u32)dsiSaveCreate);
		setBL(0x0200C8C0, (u32)dsiSaveOpen);
		setBL(0x0200C8D0, (u32)dsiSaveGetResultCode);
		*(u32*)0x0200C8D8 = 0xE1A00000; // nop
		*(u32*)0x0200C8EC = 0xE1A00000; // nop
		setBL(0x0200C904, (u32)dsiSaveSetLength);
		setBL(0x0200C914, (u32)dsiSaveWrite);
		setBL(0x0200C91C, (u32)dsiSaveClose);
		*(u32*)0x0200C924 = 0xE1A00000; // nop
		*(u32*)0x0200C938 = 0xE1A00000; // nop
		*(u32*)0x020108A4 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202A484 = 0xE1A00000; // nop
		*(u32*)0x0202D844 = 0xE1A00000; // nop
		patchInitDSiWare(0x020331D8, heapEndExceed);
		*(u32*)0x02033548 = 0x020A41C0;
		patchUserSettingsReadDSiWare(0x0203481C);
	}

	// Robot Rescue (Europe, Australia)
	else if (strcmp(romTid, "KRTV") == 0) {
		*(u32*)0x0200C084 = 0xE1A00000; // nop
		*(u32*)0x0200C08C = 0xE1A00000; // nop
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
		setBL(0x0200C8B0, (u32)dsiSaveGetResultCode);
		setBL(0x0200C8CC, (u32)dsiSaveSetLength);
		setBL(0x0200C8DC, (u32)dsiSaveWrite);
		setBL(0x0200C8E4, (u32)dsiSaveClose);
		*(u32*)0x02010C30 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202A56C = 0xE1A00000; // nop
		*(u32*)0x0202D7CC = 0xE1A00000; // nop
		patchInitDSiWare(0x02032EBC, heapEndExceed);
		*(u32*)0x02033248 = 0x020A0160;
		patchUserSettingsReadDSiWare(0x020344FC);
	}

	// ARC Style: Robot Rescue (Japan)
	else if (strcmp(romTid, "KRTJ") == 0) {
		*(u32*)0x0200F218 = 0xE1A00000; // nop
		*(u32*)0x0200F220 = 0xE1A00000; // nop
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
		*(u32*)0x0200F98C = 0xE1A00000; // nop
		setBL(0x0200F998, (u32)dsiSaveClose);
		setBL(0x0200F9A0, (u32)dsiSaveDelete);
		setBL(0x0200F9B8, (u32)dsiSaveCreate);
		setBL(0x0200F9C8, (u32)dsiSaveOpen);
		setBL(0x0200F9D8, (u32)dsiSaveGetResultCode);
		setBL(0x0200F9F4, (u32)dsiSaveSetLength);
		setBL(0x0200FA04, (u32)dsiSaveWrite);
		setBL(0x0200FA0C, (u32)dsiSaveClose);
		*(u32*)0x02013BC8 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x0202D3E0 = 0xE1A00000; // nop
		*(u32*)0x02030640 = 0xE1A00000; // nop
		patchInitDSiWare(0x02035D30, heapEndExceed);
		*(u32*)0x020360BC = 0x020A6580;
		patchUserSettingsReadDSiWare(0x02037370);
	}

	// Robot Rescue 2 (USA)
	// Robot Rescue 2 (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KRRE") == 0 || strcmp(romTid, "KRRP") == 0) && extendedMemory2) {
		*(u32*)0x0200C0AC = 0xE1A00000; // nop
		*(u32*)0x0200C0B4 = 0xE1A00000; // nop
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
		setBL(0x0200C8D8, (u32)dsiSaveGetResultCode);
		setBL(0x0200C8F4, (u32)dsiSaveSetLength);
		setBL(0x0200C904, (u32)dsiSaveWrite);
		setBL(0x0200C90C, (u32)dsiSaveClose);
		if (romTid[3] == 'E') {
			*(u32*)0x02010888 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x02033384 = 0xE1A00000; // nop
			*(u32*)0x02036678 = 0xE1A00000; // nop
			patchInitDSiWare(0x0203BDA8, heapEndExceed);
			*(u32*)0x0203C134 = 0x020BEAA0;
			patchUserSettingsReadDSiWare(0x0203D3F8);
		} else if (romTid[3] == 'P') {
			*(u32*)0x02010BE4 = 0xE1A00000; // nop (Skip Manual screen)
			*(u32*)0x02033760 = 0xE1A00000; // nop
			*(u32*)0x02036A54 = 0xE1A00000; // nop
			patchInitDSiWare(0x0203C184, heapEndExceed);
			*(u32*)0x0203C510 = 0x020BF400;
			patchUserSettingsReadDSiWare(0x0203D7D4);
		}
	}

	// Rock-n-Roll Domo (USA)
	else if (strcmp(romTid, "KD6E") == 0) {
		const u32 dsiSaveCreateT = 0x02025C20;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveDeleteT = 0x02025C30;
		*(u16*)dsiSaveDeleteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveDeleteT + 4), dsiSaveDelete, 0xC);

		const u32 dsiSaveSetLengthT = 0x02025C40;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveOpenT = 0x02025C50;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x02025C60;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveReadT = 0x02025C70;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x02025C80;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		*(u16*)0x02010164 = 0x2001; // movs r0, #1
		*(u16*)0x02010166 = 0x4770; // bx lr
		doubleNopT(0x0201041A);
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
		doubleNopT(0x02024D86);
		doubleNopT(0x02026FD6);
		doubleNopT(0x02029C58);
		doubleNopT(0x0202B212);
		doubleNopT(0x0202B216);
		doubleNopT(0x0202B222);
		doubleNopT(0x0202B306);
		patchHiHeapDSiWareThumb(0x0202B344, 0x02025C90, heapEnd); // movs r0, #0x23E0000
		doubleNopT(0x0202C3A2);
		*(u16*)0x0202C3A6 = nopT;
		*(u16*)0x0202C3A8 = nopT;
		doubleNopT(0x0202C3AA);
	}

	// Roller Angels (USA)
	// Roller Angels: Pashatto Dai Sakusen (Japan)
	else if (strcmp(romTid, "KRLE") == 0 || strcmp(romTid, "KRLJ") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 4;

		*(u32*)(0x02013560+offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x020140E4+offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0201732C+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D08C+offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x0201E7B0+offsetChange);
		if (romTid[3] == 'E') {
			*(u32*)0x0202E77C = 0xE1A00000; // nop
			*(u32*)0x0202E794 = 0xE1A00000; // nop
			setBL(0x020338F4, (u32)dsiSaveOpen);
			setBL(0x0203390C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02033924, (u32)dsiSaveOpen);
			setBL(0x02033944, (u32)dsiSaveWrite);
			setBL(0x02033954, (u32)dsiSaveClose);
			setBL(0x02033964, (u32)dsiSaveClose);
			setBL(0x020339A4, (u32)dsiSaveOpen);
			setBL(0x020339C8, (u32)dsiSaveRead);
			setBL(0x020339D8, (u32)dsiSaveClose);
			setBL(0x020339E8, (u32)dsiSaveClose);
			setBL(0x02033AC8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02033AD8, (u32)dsiSaveOpen);
			setBL(0x02033B04, (u32)dsiSaveClose);
			setBL(0x02033B3C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02033B4C, (u32)dsiSaveOpen);
			setBL(0x02033B78, (u32)dsiSaveClose);
			setB(0x02039C44, 0x02039DF8); // Skip reading photo files
		} else {
			setB(0x020393A8, 0x0203955C); // Skip reading photo files
			setBL(0x0204AD90, (u32)dsiSaveOpen);
			setBL(0x0204ADA8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0204ADC0, (u32)dsiSaveOpen);
			setBL(0x0204ADE0, (u32)dsiSaveWrite);
			setBL(0x0204ADF0, (u32)dsiSaveClose);
			setBL(0x0204AE00, (u32)dsiSaveClose);
			setBL(0x0204AE40, (u32)dsiSaveOpen);
			setBL(0x0204AE64, (u32)dsiSaveRead);
			setBL(0x0204AE74, (u32)dsiSaveClose);
			setBL(0x0204AE84, (u32)dsiSaveClose);
			setBL(0x0204AF64, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0204AF74, (u32)dsiSaveOpen);
			setBL(0x0204AFA0, (u32)dsiSaveClose);
			setBL(0x0204AFD8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0204AFE8, (u32)dsiSaveOpen);
			setBL(0x0204B014, (u32)dsiSaveClose);
		}
	}

	// RPG Dashutsu Game (Japan)
	else if (strcmp(romTid, "KRPJ") == 0) {
		*(u32*)0x020050BC = 0xE1A00000; // nop
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020052B0 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0200594C = 0x191000; // Shrink heap from 0x1D1000
			*(u32*)0x0200598C = 0x191000; // Shrink heap from 0x1D1000
		}
		setBL(0x0200EC8C, (u32)dsiSaveOpen);
		setBL(0x0200ECF4, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0200ED40, (u32)dsiSaveGetLength);
		setBL(0x0200ED5C, (u32)dsiSaveRead);
		setBL(0x0200ED64, (u32)dsiSaveClose);
		setBL(0x0200EDA8, (u32)dsiSaveOpen);
		setBL(0x0200EE28, (u32)dsiSaveGetLength);
		setBL(0x0200EE38, (u32)dsiSaveSeek);
		setBL(0x0200EEB4, (u32)dsiSaveClose);
		setBL(0x0200EEDC, (u32)dsiSaveWrite);
		setBL(0x0200EEE8, (u32)dsiSaveClose);
		*(u32*)0x0203C4CC = 0xE1A00000; // nop
		tonccpy((u32*)0x0203D050, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02040034 = 0xE1A00000; // nop
		patchInitDSiWare(0x02045D64, heapEnd);
		*(u32*)0x020460F0 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02047240);
	}

	// Saikyou Ginsei Shougi (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KG4J") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		*(u32*)0x0201125C = 0xE1A00000; // nop
		*(u32*)0x02014F20 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FAB4, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x0201FE24 = *(u32*)0x02004FC0;
		}
		patchUserSettingsReadDSiWare(0x020210A4);
		*(u32*)0x020210CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020210D0 = 0xE12FFF1E; // bx lr
		*(u32*)0x020210D8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020210DC = 0xE12FFF1E; // bx lr
		if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x02037E44, 0x020212C0);
			} */
		} else {
			*(u32*)0x020455E0 = 0xE1A00000; // nop (Skip Manual screen)
		}
	}

	// Sakurai Miho No Kouno: Megami Serapi Uranai (Japan)
	else if (strcmp(romTid, "K3PJ") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		// if (!useSharedFont) {
			*(u32*)0x020050B8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// }
		*(u32*)0x0200B434 = 0xE1A00000; // nop
		*(u32*)0x0200E6AC = 0xE1A00000; // nop
		patchInitDSiWare(0x020153BC, heapEnd);
		patchUserSettingsReadDSiWare(0x020169A4);
		setBL(0x020224A0, (u32)dsiSaveCreate);
		setBL(0x020224B0, (u32)dsiSaveOpen);
		setBL(0x020224E0, (u32)dsiSaveWrite);
		setBL(0x020224F8, (u32)dsiSaveClose);
		setBL(0x02022570, (u32)dsiSaveOpen);
		setBL(0x02022580, (u32)dsiSaveGetLength);
		setBL(0x0202259C, (u32)dsiSaveRead);
		setBL(0x020225D4, (u32)dsiSaveClose);
		/* if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0201CAA4, 0x02016F14);
		} */
	}

	// Save the Turtles (USA)
	// Save the Turtles (Europe, Australia)
	// Due to our save implementation, save data is stored in all 3 slots
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "K7TE") == 0 || strcmp(romTid, "K7TV") == 0) && extendedMemory2) {
		useSharedFont = twlFontFound;
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x60;
		u16 offsetChangeN = (romTid[3] == 'E') ? 0 : 0x150;
		u16 offsetChange2 = (romTid[3] == 'E') ? 0 : 0x13C;
		const u32 newLoadCode = 0x0204FF1C-offsetChange2;

		codeCopy((u32*)newLoadCode, (u32*)(0x02031698-offsetChange2), 0xE0);
		setBL(newLoadCode+0x50, (u32)dsiSaveOpenR);
		setBL(newLoadCode+0x68, (u32)dsiSaveGetLength);
		setBL(newLoadCode+0xA4, (u32)dsiSaveRead);
		setBL(newLoadCode+0xD0, (u32)dsiSaveClose);

		setBL(0x020272C0-offsetChangeN, newLoadCode);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0201E474-offsetChange, 0x0205819C-offsetChange2);
			}
		} else {
			*(u32*)(0x0201E3E4-offsetChange) = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		setBL(0x02031798-offsetChange2, (u32)dsiSaveOpenR);
		setBL(0x020317B0-offsetChange2, (u32)dsiSaveClose);
		setBL(0x02031828-offsetChange2, (u32)dsiSaveOpen);
		setBL(0x02031840-offsetChange2, (u32)dsiSaveCreate);
		*(u32*)(0x02031874-offsetChange2) = 0xE1A00000; // nop (dsiSaveCreateDir)
		setBL(0x02031880-offsetChange2, (u32)dsiSaveCreate);
		setBL(0x020318B0-offsetChange2, (u32)dsiSaveOpen);
		setBL(0x020318D4-offsetChange2, (u32)dsiSaveWrite);
		setBL(0x020318DC-offsetChange2, (u32)dsiSaveClose);
		// if (!extendedMemory2) {
			// Disable audio
			*(u32*)(0x02032EA0-offsetChange2) = 0xE12FFF1E; // bx lr
			*(u32*)(0x020330B8-offsetChange2) = 0xE12FFF1E; // bx lr
			*(u32*)(0x0203319C-offsetChange2) = 0xE12FFF1E; // bx lr
		// }
		*(u32*)(0x0204E9A8-offsetChange2) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0204F520-offsetChange2), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x02051D88-offsetChange2) = 0xE1A00000; // nop
		patchInitDSiWare(0x02056868-offsetChange2, heapEnd);
		patchUserSettingsReadDSiWare(0x02057D24-offsetChange2);
	}

	// Sea Battle (USA)
	// Sea Battle (Europe)
	else if (strcmp(romTid, "KRWE") == 0 || strcmp(romTid, "KRWP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0xBC;

		useSharedFont = (twlFontFound && debugOrMep);
		if (!useSharedFont) {
			*(u32*)0x02005248 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200E834 = 0xE1A00000; // nop
		*(u32*)0x02011AAC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201790C, heapEnd);
		patchUserSettingsReadDSiWare(0x02018FBC);
		*(u32*)0x0201E73C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201E740 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202B430 = 0xE1A00000; // nop
		setBL(0x02030F00+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02030F10+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02030F3C+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02030F4C+offsetChange, (u32)dsiSaveClose);
		setBL(0x02030F68+offsetChange, (u32)dsiSaveClose);
		setBL(0x02030FD4+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02030FE4+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02030FFC+offsetChange, (u32)dsiSaveRead);
		setBL(0x02031040+offsetChange, (u32)dsiSaveClose);
		setBL(0x0203105C+offsetChange, (u32)dsiSaveClose);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02031158+offsetChange, 0x02019548);
		}
	}

	// Kaisan Gemu: Radar (Japan)
	else if (strcmp(romTid, "KRWJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (!useSharedFont) {
			*(u32*)0x02005248 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200E2F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201045C = 0xE1A00000; // nop
		*(u32*)0x0201364C = 0xE1A00000; // nop
		*(u32*)0x02014D9C = 0xE1A00000; // nop
		*(u32*)0x02014DA0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A864, heapEnd);
		patchUserSettingsReadDSiWare(0x0201BF14);
		*(u32*)0x02021694 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02021698 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202E884 = 0xE1A00000; // nop
		*(u32*)0x0202ECF8 = 0xE1A00000; // nop
		*(u32*)0x02032464 = 0xE1A00000; // nop
		*(u32*)0x020324BC = 0xE1A00000; // nop
		*(u32*)0x02032510 = 0xE1A00000; // nop
		*(u32*)0x02032700 = 0xE1A00000; // nop
		*(u32*)0x02032738 = 0xE1A00000; // nop
		*(u32*)0x02032788 = 0xE1A00000; // nop
		*(u32*)0x020327C4 = 0xE1A00000; // nop
		setBL(0x0203784C, (u32)dsiSaveCreate);
		setBL(0x0203785C, (u32)dsiSaveOpen);
		setBL(0x02037888, (u32)dsiSaveWrite);
		setBL(0x02037898, (u32)dsiSaveClose);
		setBL(0x020378B4, (u32)dsiSaveClose);
		setBL(0x02037920, (u32)dsiSaveOpen);
		setBL(0x02037930, (u32)dsiSaveGetLength);
		setBL(0x02037948, (u32)dsiSaveRead);
		setBL(0x0203798C, (u32)dsiSaveClose);
		setBL(0x020379A8, (u32)dsiSaveClose);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02037AA4, 0x0201C49C);
		}
	}

	// The Seller (USA)
	else if (strcmp(romTid, "KLLE") == 0) {
		*(u32*)0x02014B1C = 0xE1A00000; // nop
		*(u32*)0x02017DDC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D654, heapEnd);
		*(u32*)0x0201D9E0 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x0201EAE4);
		setBL(0x0202D1F4, (u32)dsiSaveOpen);
		setBL(0x0202D204, (u32)dsiSaveWrite);
		setBL(0x0202D20C, (u32)dsiSaveClose);
		setBL(0x0202D26C, (u32)dsiSaveGetInfo);
		setBL(0x0202D280, (u32)dsiSaveOpen);
		setBL(0x0202D2B4, (u32)dsiSaveCreate);
		setBL(0x0202D2C4, (u32)dsiSaveOpen);
		setBL(0x0202D2D4, (u32)dsiSaveGetResultCode);
		*(u32*)0x0202D2E4 = 0xE1A00000; // nop
		setBL(0x0202D2F0, (u32)dsiSaveCreate);
		setBL(0x0202D300, (u32)dsiSaveOpen);
		setBL(0x0202D310, (u32)dsiSaveWrite);
		setBL(0x0202D39C, (u32)dsiSaveSeek);
		setBL(0x0202D3AC, (u32)dsiSaveWrite);
		setBL(0x0202D3B4, (u32)dsiSaveClose);
		setBL(0x0202D3F8, (u32)dsiSaveOpen);
		setBL(0x0202D43C, (u32)dsiSaveRead);
		setBL(0x0202D4A8, (u32)dsiSaveClose);
		*(u32*)0x020312A0 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// The Seller (Europe)
	else if (strcmp(romTid, "KLLP") == 0) {
		*(u32*)0x02014B34 = 0xE1A00000; // nop
		*(u32*)0x02017E88 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201D738, heapEnd);
		*(u32*)0x0201DAC4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x0201EBD8);
		setBL(0x0202D2E4, (u32)dsiSaveOpen);
		setBL(0x0202D2F8, (u32)dsiSaveCreate);
		setBL(0x0202D308, (u32)dsiSaveOpen);
		setBL(0x0202D318, (u32)dsiSaveGetResultCode);
		*(u32*)0x0202D328 = 0xE1A00000; // nop
		setBL(0x0202D334, (u32)dsiSaveCreate);
		setBL(0x0202D344, (u32)dsiSaveOpen);
		setBL(0x0202D358, (u32)dsiSaveWrite);
		setBL(0x0202D360, (u32)dsiSaveClose);
		setBL(0x0202D3C0, (u32)dsiSaveGetInfo);
		setBL(0x0202D3D4, (u32)dsiSaveOpen);
		setBL(0x0202D404, (u32)dsiSaveCreate);
		setBL(0x0202D414, (u32)dsiSaveOpen);
		setBL(0x0202D424, (u32)dsiSaveGetResultCode);
		*(u32*)0x0202D434 = 0xE1A00000; // nop
		setBL(0x0202D440, (u32)dsiSaveCreate);
		setBL(0x0202D450, (u32)dsiSaveOpen);
		setBL(0x0202D460, (u32)dsiSaveWrite);
		setBL(0x0202D4E4, (u32)dsiSaveSeek);
		setBL(0x0202D4F4, (u32)dsiSaveWrite);
		setBL(0x0202D4FC, (u32)dsiSaveClose);
		setBL(0x0202D544, (u32)dsiSaveOpen);
		setBL(0x0202D554, (u32)dsiSaveGetResultCode);
		setBL(0x0202D580, (u32)dsiSaveGetLength);
		setBL(0x0202D5D0, (u32)dsiSaveRead);
		setBL(0x0202D630, (u32)dsiSaveClose);
		*(u32*)0x02031428 = 0xE1A00000; // nop (Skip Manual screen)
	}

	// Shantae: Risky's Revenge (USA)
	// Requires 8MB of RAM, crashes after first battle with 4MB of RAM, but can get past with a save file
	// BGM is disabled to stay within RAM limitations
	else if (strcmp(romTid, "KS3E") == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleForce[0] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x02026F68;

		// Skip Manual screen
		/* *(u32*)0x02016130 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x020161C8 = 0xE1A00000; // nop
		*(u32*)0x020161D0 = 0xE1A00000; // nop
		*(u32*)0x020161DC = 0xE1A00000; // nop
		*(u32*)0x020166C8 = 0xE3A06901; // mov r6, #0x4000 */

		// Hide help button
		*(u32*)0x02016688 = 0xE1A00000; // nop

		if (!extendedMemory2) {
			// Disable pre-load function
			/* *(u32*)0x0201FBA0 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FD3C = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FDA8 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FE14 = 0xE12FFF1E; // bx lr */
			// *(u32*)0x020AB800 = 0xE1A00000; // nop
			*(u32*)0x020BCE44 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x0201FC20 = 0xE12FFF1E; // bx lr (Disable loading sdat file)
		tonccpy((u32*)0x0201FC40, ce9->patches->musicPlay, 0xC);
		tonccpy((u32*)0x0201FC78, ce9->patches->musicStopEffect, 0xC);
		setBL(0x02026F68, (int)ce9->patches->rumble_arm9[0]); // Rumble when hair is whipped
		setBL(0x0209201C, (u32)dsiSaveCreate);
		setBL(0x02092040, (u32)dsiSaveGetResultCode);
		*(u32*)0x02092050 = 0xE1A00000; // nop
		setBL(0x0209205C, (u32)dsiSaveCreate);
		*(u32*)0x02092078 = 0xE3A00000; // mov r0, #0
		setBL(0x0209291C, (u32)dsiSaveOpen);
		*(u32*)0x02092934 = 0xE1A00000; // nop
		setBL(0x02092944, (u32)dsiSaveOpen);
		setBL(0x02092958, (u32)dsiSaveRead);
		setBL(0x02092960, (u32)dsiSaveClose);
		*(u32*)0x02092BA8 = 0xE1A00000; // nop
		setBL(0x02092BCC, (u32)dsiSaveCreate);
		setBL(0x02092BDC, (u32)dsiSaveOpen);
		setBL(0x02092DE4, (u32)dsiSaveSetLength);
		setBL(0x02092DF4, (u32)dsiSaveWrite);
		setBL(0x02092DFC, (u32)dsiSaveClose);
		*(u32*)0x02092E08 = 0xE1A00000; // nop
		*(u32*)0x02092E0C = 0xE1A00000; // nop
		*(u32*)0x02092E10 = 0xE1A00000; // nop
		*(u32*)0x02092E14 = 0xE1A00000; // nop
		*(u32*)0x02092E20 = 0xE1A00000; // nop
		*(u32*)0x02092E24 = 0xE1A00000; // nop
		*(u32*)0x020DE420 = 0xE1A00000; // nop
		*(u32*)0x020DE548 = 0xE1A00000; // nop
		*(u32*)0x020DE55C = 0xE1A00000; // nop
		*(u32*)0x020E20C4 = 0xE1A00000; // nop
		patchInitDSiWare(0x020E7ED8, heapEnd);
		*(u32*)0x020E8264 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x020E9348);
		*(u32*)0x020E977C = 0xE1A00000; // nop
		*(u32*)0x020E9780 = 0xE1A00000; // nop
		*(u32*)0x020E9784 = 0xE1A00000; // nop
		*(u32*)0x020E9788 = 0xE1A00000; // nop
		*(u32*)0x020E9794 = 0xE1A00000; // nop (Enable error exception screen)
	}

	// Shantae: Risky's Revenge (Europe)
	// Requires 8MB of RAM, crashes after first battle with 4MB of RAM, but can get past with a save file
	// BGM is disabled to stay within RAM limitations
	else if (strcmp(romTid, "KS3P") == 0) {
		ce9->rumbleFrames[0] = 10;
		ce9->rumbleForce[0] = 1;
		ce9->patches->rumble_arm9[0][3] = *(u32*)0x020271E0;

		// Skip Manual screen
		/* *(u32*)0x020163B0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02016448 = 0xE1A00000; // nop
		*(u32*)0x02016450 = 0xE1A00000; // nop
		*(u32*)0x0201645C = 0xE1A00000; // nop
		*(u32*)0x02016940 = 0xE3A06901; // mov r6, #0x4000 */

		// Hide help button
		*(u32*)0x02016904 = 0xE1A00000; // nop

		if (!extendedMemory2) {
			// Disable pre-load function
			/* *(u32*)0x0201FE18 = 0xE12FFF1E; // bx lr
			*(u32*)0x0201FFB4 = 0xE12FFF1E; // bx lr
			*(u32*)0x02020020 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202008C = 0xE12FFF1E; // bx lr */
			// *(u32*)0x020ABBF0 = 0xE1A00000; // nop
			*(u32*)0x020BD234 = 0xE12FFF1E; // bx lr
		}
		*(u32*)0x0201FE98 = 0xE12FFF1E; // bx lr (Disable loading sdat file)
		tonccpy((u32*)0x0201FEB8, ce9->patches->musicPlay, 0xC);
		tonccpy((u32*)0x0201FEF0, ce9->patches->musicStopEffect, 0xC);
		setBL(0x020271E0, (int)ce9->patches->rumble_arm9[0]); // Rumble when hair is whipped
		setBL(0x020922A0, (u32)dsiSaveCreate);
		setBL(0x020922C4, (u32)dsiSaveGetResultCode);
		*(u32*)0x020922D4 = 0xE1A00000; // nop
		setBL(0x020922E0, (u32)dsiSaveCreate);
		*(u32*)0x020922FC = 0xE3A00000; // mov r0, #0
		setBL(0x02092D4C, (u32)dsiSaveOpen);
		*(u32*)0x02092D64 = 0xE1A00000; // nop
		setBL(0x02092D74, (u32)dsiSaveOpen);
		setBL(0x02092D88, (u32)dsiSaveRead);
		setBL(0x02092D90, (u32)dsiSaveClose);
		*(u32*)0x02092FD8 = 0xE1A00000; // nop
		setBL(0x02092FFC, (u32)dsiSaveCreate);
		setBL(0x0209300C, (u32)dsiSaveOpen);
		setBL(0x02093214, (u32)dsiSaveSetLength);
		setBL(0x02093224, (u32)dsiSaveWrite);
		setBL(0x0209322C, (u32)dsiSaveClose);
		*(u32*)0x02093238 = 0xE1A00000; // nop
		*(u32*)0x0209323C = 0xE1A00000; // nop
		*(u32*)0x02093240 = 0xE1A00000; // nop
		*(u32*)0x02093244 = 0xE1A00000; // nop
		*(u32*)0x020DE810 = 0xE1A00000; // nop
		*(u32*)0x020DE938 = 0xE1A00000; // nop
		*(u32*)0x020DE94C = 0xE1A00000; // nop
		*(u32*)0x020E253C = 0xE1A00000; // nop
		patchInitDSiWare(0x020E8374, heapEnd);
		*(u32*)0x020E8700 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020E97E4);
		*(u32*)0x020E9C18 = 0xE1A00000; // nop
		*(u32*)0x020E9C1C = 0xE1A00000; // nop
		*(u32*)0x020E9C20 = 0xE1A00000; // nop
		*(u32*)0x020E9C24 = 0xE1A00000; // nop
		*(u32*)0x020E9C30 = 0xE1A00000; // nop (Enable error exception screen)
	}

	// Shawn Johnson Gymnastics (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KSJE") == 0 && extendedMemory2) {
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x02005090 = 0xE1A00000; // nop
		setB(0x02005380, 0x020053D4); // Disable NFTR loading from TWLNAND
		*(u32*)0x02005400 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005468 = 0xE3A00000; // mov r0, #0
		*(u32*)0x02005594 = 0xE1A00000; // nop
		*(u32*)0x02019840 = 0xE1A00000; // nop
		*(u32*)0x0201D4E8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02023C6C, heapEnd);
		*(u32*)0x02023FDC = 0x02116740;
		patchUserSettingsReadDSiWare(0x02025274);
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
		*(u32*)0x02091A28 = 0xE3A00000; // mov r0, #0
	}

	// Kakitori Rekishi: Shouga Kusei (01) (Japan)
	// Chiri Kuizu: Shouga Kusei (02) (Japan)
	// Koumin Kuizu: Shouga Kusei (03) (Japan)
	// Rika Kuizu Shouga Kusei: Seibutsu Chigaku He (04) (Japan)
	// Jukugo Kuizu (05) (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KK5J") == 0 || strcmp(romTid, "KZ9J") == 0 || strcmp(romTid, "KK3J") == 0 || strcmp(romTid, "K48J") == 0 || strcmp(romTid, "K49J") == 0) {
		u8 offsetChange0 = (strcmp(romTid, "KZ9J") == 0 || strcmp(romTid, "KK3J") == 0 || strcmp(romTid, "K48J") == 0) ? 0 : 0x1C;
		u8 offsetChange = 0;
		u8 offsetChange2 = 0;
		if (strcmp(romTid, "KK5J") == 0) {
			offsetChange = 0x18;
			offsetChange2 = 0x70;
		} else if (strcmp(romTid, "K49J") == 0) {
			offsetChange = 0x1C;
			offsetChange2 = 0x1C;
		}

		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)(0x0200E148+offsetChange0) = 0xE1A00000; // nop
		*(u32*)(0x02011A80+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x020175BC+offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x02018A98+offsetChange);
		/* if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x020371A0+offsetChange2, 0x02019008+offsetChange);
			}
		} else { */
			*(u32*)(0x0201DD98+offsetChange) = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			*(u32*)(0x02037338+offsetChange2) = 0xE1A00000; // nop
			*(u32*)(0x02037340+offsetChange2) = 0xE1A00000; // nop
			*(u32*)(0x0203734C+offsetChange2) = 0xE1A00000; // nop
		// }
		*(u32*)(0x0201DD9C+offsetChange) = 0xE1A00000; // nop
		/* setBL(0x0204173C+offsetChange2, (u32)dsiSaveOpen);
		setBL(0x02041760+offsetChange2, (u32)dsiSaveGetLength);
		setBL(0x02041784+offsetChange2, (u32)dsiSaveRead);
		setBL(0x0204178C+offsetChange2, (u32)dsiSaveClose);
		setBL(0x020417D0+offsetChange2, (u32)dsiSaveCreate);
		setBL(0x020417E0+offsetChange2, (u32)dsiSaveOpen);
		setBL(0x020417F0+offsetChange2, (u32)dsiSaveGetResultCode);
		setBL(0x02041810+offsetChange2, (u32)dsiSaveSetLength);
		setBL(0x02041820+offsetChange2, (u32)dsiSaveWrite);
		setBL(0x02041828+offsetChange2, (u32)dsiSaveClose);
		*(u32*)(0x02041868+offsetChange2) = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)(0x02041890+offsetChange2) = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)(0x020418B8+offsetChange2) = 0xE3A00001; // mov r0, #1
		*(u32*)(0x020418CC+offsetChange2) = 0xE3A00001; // mov r0, #1
		*(u32*)(0x02041910+offsetChange2) = 0xE3A00000; // mov r0, #0
		// *(u32*)(0x02041948+offsetChange2) = 0xE1A00000; // nop
		*(u32*)(0x02041960+offsetChange2) = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		*(u32*)(0x02041970+offsetChange2) = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)(0x020419D0+offsetChange2) = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)(0x020419EC+offsetChange2) = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)(0x02041A14+offsetChange2) = 0xE3A00001; // mov r0, #1
		*(u32*)(0x02041A28+offsetChange2) = 0xE3A00001; // mov r0, #1
		setBL(0x02041A64+offsetChange2, (u32)dsiSaveDelete);
		*(u32*)(0x02041A78+offsetChange2) = 0xE3A00000; // mov r0, #0 (dsiSaveReadDir)
		*(u32*)(0x02041A88+offsetChange2) = 0xE1A00000; // nop (dsiSaveCloseDir) */
	}

	// Simple DS Series Vol. 1: The Misshitsukara no Dasshutsu (Japan)
	// Requires more than 8MB of RAM(?)
	/*else if (strcmp(romTid, "KM4J") == 0) {
		*(u32*)0x020050A8 = 0xE1A00000; // nop
		*(u32*)0x0200F91C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
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
		*(u32*)0x0205E43C = 0xE1A00000; // nop
		*(u32*)0x0205E570 = 0xE1A00000; // nop
		*(u32*)0x0205E584 = 0xE1A00000; // nop
		tonccpy((u32*)0x0205F0D0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0206204C = 0xE1A00000; // nop
		*(u32*)0x020678CC = 0xE1A00000; // nop
		*(u32*)0x0206973C = 0xE1A00000; // nop
		*(u32*)0x02069740 = 0xE1A00000; // nop
		*(u32*)0x0206974C = 0xE1A00000; // nop
		*(u32*)0x02069890 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020698EC, heapEndExceed);
		patchUserSettingsReadDSiWare(0x0206AB8C);
		*(u32*)0x0206AFE8 = 0xE1A00000; // nop
		*(u32*)0x0206AFEC = 0xE1A00000; // nop
		*(u32*)0x0206AFF0 = 0xE1A00000; // nop
		*(u32*)0x0206AFF4 = 0xE1A00000; // nop
	}*/

	// Simply Mahjong (USA)
	else if (strcmp(romTid, "K4JE") == 0) {
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
		*(u16*)0x0201FB08 = 0x2201; // movs r2, #1
		doubleNopT(0x0202C9AA);
		doubleNopT(0x0202C9B0);
		*(u32*)0x02054A84 = 0xE1A00000; // nop
		*(u32*)0x020581E8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205E3DC, heapEnd);
		patchUserSettingsReadDSiWare(0x0205FA24);
		*(u32*)0x0205FE2C = 0xE1A00000; // nop
		*(u32*)0x0205FE30 = 0xE1A00000; // nop
		*(u32*)0x0205FE34 = 0xE1A00000; // nop
		*(u32*)0x0205FE38 = 0xE1A00000; // nop
	}

	// Simply Mahjong (Europe)
	else if (strcmp(romTid, "K4JP") == 0) {
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
		*(u16*)0x0201FAF8 = 0x2201; // movs r2, #1
		doubleNopT(0x0202C99A);
		doubleNopT(0x0202C9A0);
		*(u32*)0x02054A34 = 0xE1A00000; // nop
		*(u32*)0x02058198 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205E38C, heapEnd);
		patchUserSettingsReadDSiWare(0x0205F9D4);
		*(u32*)0x0205FDDC = 0xE1A00000; // nop
		*(u32*)0x0205FDE0 = 0xE1A00000; // nop
		*(u32*)0x0205FDE4 = 0xE1A00000; // nop
		*(u32*)0x0205FDE8 = 0xE1A00000; // nop
	}

	// Simply Minesweeper (USA)
	// Simply Minesweeper (Europe)
	else if (strcmp(romTid, "KM3E") == 0 || strcmp(romTid, "KM3P") == 0) {
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
		*(u16*)0x0201DA44 = 0x2201; // movs r2, #1
		doubleNopT(0x0202AD46);
		doubleNopT(0x0202AD4C);
		*(u32*)0x0203622C = 0xE1A00000; // nop
		if (romTid[3] == 'E') {
			*(u32*)0x02052CE8 = 0xE1A00000; // nop
			*(u32*)0x0205644C = 0xE1A00000; // nop
			patchInitDSiWare(0x0205C614, heapEnd);
			patchUserSettingsReadDSiWare(0x0205DC90);
			*(u32*)0x0205E098 = 0xE1A00000; // nop
			*(u32*)0x0205E09C = 0xE1A00000; // nop
			*(u32*)0x0205E0A0 = 0xE1A00000; // nop
			*(u32*)0x0205E0A4 = 0xE1A00000; // nop
		} else {
			*(u32*)0x02052CB4 = 0xE1A00000; // nop
			*(u32*)0x02056418 = 0xE1A00000; // nop
			patchInitDSiWare(0x0205C5E0, heapEnd);
			patchUserSettingsReadDSiWare(0x0205DC5C);
			*(u32*)0x0205E064 = 0xE1A00000; // nop
			*(u32*)0x0205E068 = 0xE1A00000; // nop
			*(u32*)0x0205E06C = 0xE1A00000; // nop
			*(u32*)0x0205E070 = 0xE1A00000; // nop
		}
	}

	// Simply Solitaire (USA)
	// Simply Solitaire (Europe)
	else if (strcmp(romTid, "K4LE") == 0 || strcmp(romTid, "K4LP") == 0) {
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
		if (romTid[3] == 'E') {
			*(u16*)0x0201EBD8 = 0x2201; // movs r2, #1
			doubleNopT(0x0202C1A6);
			doubleNopT(0x0202C1AC);
			*(u32*)0x020376C4 = 0xE1A00000; // nop
			*(u32*)0x02054364 = 0xE1A00000; // nop
			*(u32*)0x02057AC8 = 0xE1A00000; // nop
			patchInitDSiWare(0x0205DCD0, heapEnd);
			patchUserSettingsReadDSiWare(0x0205F34C);
			*(u32*)0x0205F754 = 0xE1A00000; // nop
			*(u32*)0x0205F758 = 0xE1A00000; // nop
			*(u32*)0x0205F75C = 0xE1A00000; // nop
			*(u32*)0x0205F760 = 0xE1A00000; // nop
		} else {
			*(u16*)0x0201EBD4 = 0x2201; // movs r2, #1
			doubleNopT(0x0202C186);
			doubleNopT(0x0202C18C);
			*(u32*)0x020542F0 = 0xE1A00000; // nop
			*(u32*)0x02057A54 = 0xE1A00000; // nop
			patchInitDSiWare(0x0205DC5C, heapEnd);
			patchUserSettingsReadDSiWare(0x0205F2B0);
			*(u32*)0x0205F6B8 = 0xE1A00000; // nop
			*(u32*)0x0205F6BC = 0xE1A00000; // nop
			*(u32*)0x0205F6C0 = 0xE1A00000; // nop
			*(u32*)0x0205F6C4 = 0xE1A00000; // nop
		}
	}

	// Simply Sudoku (Europe)
	else if (strcmp(romTid, "KS4P") == 0) {
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
		*(u16*)0x0201F0D0 = 0x2201; // movs r2, #1
		doubleNopT(0x0202C696);
		doubleNopT(0x0202C69C);
		*(u32*)0x02054880 = 0xE1A00000; // nop
		*(u32*)0x02057FE4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0205E1E8, heapEnd);
		patchUserSettingsReadDSiWare(0x0205F83C);
		*(u32*)0x0205FC44 = 0xE1A00000; // nop
		*(u32*)0x0205FC48 = 0xE1A00000; // nop
		*(u32*)0x0205FC4C = 0xE1A00000; // nop
		*(u32*)0x0205FC50 = 0xE1A00000; // nop
	}

	// Slingo Supreme (USA)
	// Save patch does not work (only bypasses)
	else if (strcmp(romTid, "K3SE") == 0) {
		*(u32*)0x0200D490 = 0xE1A00000; // nop
		*(u32*)0x02011048 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201661C, heapEnd);
		*(u32*)0x020169A8 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02017D3C);
		*(u32*)0x02024E28 = 0xE12FFF1E; // bx lr
		// *(u32*)0x02024E50 = 0xE1A00000; // nop
		*(u32*)0x02024E8C = 0xE12FFF1E; // bx lr
		// *(u32*)0x02024EB4 = 0xE1A00000; // nop
		*(u32*)0x02024EF0 = 0xE12FFF1E; // bx lr
		/* *(u32*)0x02024F3C = 0xE1A00000; // nop
		setBL(0x02024F58, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0202501C, (u32)dsiSaveOpen);
		setBL(0x0202505C, (u32)dsiSaveSeek);
		setBL(0x0202507C, (u32)dsiSaveWrite);
		setBL(0x02025094, (u32)dsiSaveClose);
		setBL(0x020250C8, (u32)dsiSaveOpen);
		setBL(0x020250D8, (u32)dsiSaveGetLength);
		setBL(0x020250F4, (u32)dsiSaveSeek);
		setBL(0x02025114, (u32)dsiSaveSeek);
		setBL(0x02025138, (u32)dsiSaveWrite);
		setBL(0x0202516C, (u32)dsiSaveWrite);
		setBL(0x020251CC, (u32)dsiSaveSeek);
		setBL(0x020251EC, (u32)dsiSaveWrite);
		setBL(0x0202534C, (u32)dsiSaveSeek);
		setBL(0x02025370, (u32)dsiSaveWrite);
		*(u32*)0x020253C8 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		// setBL(0x020253DC, (u32)dsiSaveGetResultCode);
		setBL(0x02025460, (u32)dsiSaveOpen);
		setBL(0x02025478, (u32)dsiSaveSetLength);
		setBL(0x02025488, (u32)dsiSaveRead);
		setBL(0x0202552C, (u32)dsiSaveClose); */
		*(u32*)0x02046910 = 0xE3A00000; // mov r0, #0
	}

	// Smart Girl's Playhouse Mini (USA)
	else if (strcmp(romTid, "K2FE") == 0) {
		*(u32*)0x02005530 = 0xE1A00000; // nop
		*(u32*)0x02005548 = 0xE1A00000; // nop
		*(u32*)0x0200E44C = 0xE1A00000; // nop
		*(u32*)0x020118E8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02016944, heapEnd);
		*(u32*)0x02016CD0 = 0x02114D80;
		patchUserSettingsReadDSiWare(0x02017DE4);
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
		*(u32*)0x0202EA74 = 0xE1A00000; // nop
	}

	// Snakenoid Deluxe (USA)
	// Audio does not play, except for in the opening video
	else if (strcmp(romTid, "K4NE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020050A4 = 0xE1A00000; // nop
		*(u32*)0x020186C8 = 0xE1A00000; // nop
		*(u32*)0x0201CB54 = 0xE1A00000; // nop
		patchInitDSiWare(0x02022454, heapEnd);
		patchUserSettingsReadDSiWare(0x02023C10);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203FBCC, 0x02024154);
			*(u32*)0x0203FC18 = 0xE1A00000; // nop
		}
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
		*(u32*)0x020565A8 = 0xE1A00000; // nop
		*(u32*)0x02059DAC = 0xE12FFF1E; // bx lr
	}

	// Snakenoid (Europe)
	// Audio does not play, except for in the opening video
	else if (strcmp(romTid, "K4NP") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0200E014 = 0xE1A00000; // nop
		*(u32*)0x020123EC = 0xE1A00000; // nop
		patchInitDSiWare(0x02017CA4, heapEnd);
		patchUserSettingsReadDSiWare(0x02019450);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02033608, 0x02019C10);
			*(u32*)0x02033654 = 0xE1A00000; // nop
		}
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
		*(u32*)0x020490E0 = 0xE1A00000; // nop
		*(u32*)0x0204C8F8 = 0xE12FFF1E; // bx lr
	}

	// Snapdots (USA, Australia)
	else if (strcmp(romTid, "KTYT") == 0) {
		useSharedFont = twlFontFound;
		const u32 newCodeAddr = 0x02010128;

		if (!twlFontFound) {
			*(u32*)0x020052B0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0201F368 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
		*(u32*)0x02005064 = 0xE1A00000; // nop
		*(u32*)0x0200EB54 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F72C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011F90 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017790, heapEnd);
		patchUserSettingsReadDSiWare(0x02018C80);
		codeCopy((u32*)newCodeAddr, (u32*)0x0201E594, 0xF0);
		setBL(newCodeAddr+0x34, (u32)dsiSaveOpen);
		setBL(newCodeAddr+0x58, (u32)dsiSaveGetLength);
		setBL(newCodeAddr+0x6C, (u32)dsiSaveClose);
		setBL(newCodeAddr+0xA4, (u32)dsiSaveRead);
		setBL(newCodeAddr+0xD4, (u32)dsiSaveClose);
		*(u32*)0x0201E6B4 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		setBL(0x0201E6F4, newCodeAddr);
		*(u32*)0x0201E700 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0201E744, (u32)dsiSaveCreate);
		setBL(0x0201E77C, (u32)dsiSaveOpen);
		setBL(0x0201E7AC, (u32)dsiSaveSetLength);
		setBL(0x0201E7DC, (u32)dsiSaveWrite);
		setBL(0x0201E7EC, (u32)dsiSaveClose);
		*(u32*)0x0202D940 = 0xE1A00000; // nop
		*(u32*)0x0202DAEC = 0xE1A00000; // nop
		setBL(0x0202DD8C, (u32)dsiSaveDelete);
		setBL(0x0202DE34, (u32)dsiSaveDelete);
		setBL(0x0202DF14, (u32)dsiSaveDelete);
	}

	// Kaiten' Irasuto Pazuru: Guru Guru Logic (Japan)
	else if (strcmp(romTid, "KTYJ") == 0) {
		useSharedFont = twlFontFound;
		const u32 newCodeAddr = 0x0201007C;

		if (!twlFontFound) {
			*(u32*)0x02005294 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			*(u32*)0x0201EB20 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
		*(u32*)0x02005064 = 0xE1A00000; // nop
		*(u32*)0x0200EAB4 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F680, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011EE4 = 0xE1A00000; // nop
		patchInitDSiWare(0x020176C8, heapEnd);
		patchUserSettingsReadDSiWare(0x02018BA8);
		codeCopy((u32*)newCodeAddr, (u32*)0x0201DD4C, 0xF0);
		setBL(newCodeAddr+0x34, (u32)dsiSaveOpen);
		setBL(newCodeAddr+0x58, (u32)dsiSaveGetLength);
		setBL(newCodeAddr+0x6C, (u32)dsiSaveClose);
		setBL(newCodeAddr+0xA4, (u32)dsiSaveRead);
		setBL(newCodeAddr+0xD4, (u32)dsiSaveClose);
		*(u32*)0x0201DE6C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		setBL(0x0201DEAC, newCodeAddr);
		*(u32*)0x0201DEB8 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0201DEFC, (u32)dsiSaveCreate);
		setBL(0x0201DF34, (u32)dsiSaveOpen);
		setBL(0x0201DF64, (u32)dsiSaveSetLength);
		setBL(0x0201DF94, (u32)dsiSaveWrite);
		setBL(0x0201DFA4, (u32)dsiSaveClose);
		*(u32*)0x0202C65C = 0xE1A00000; // nop
		*(u32*)0x0202C808 = 0xE1A00000; // nop
		setBL(0x0202CA8C, (u32)dsiSaveDelete);
		setBL(0x0202CB34, (u32)dsiSaveDelete);
		setBL(0x0202CC14, (u32)dsiSaveDelete);
	}

	// SnowBoard Xtreme (USA)
	// SnowBoard Xtreme (Europe)
	else if (strcmp(romTid, "KX5E") == 0 || strcmp(romTid, "KX5P") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x50;

		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02010AB8 = 0xE1A00000; // nop
		*(u32*)0x02010AEC = 0xE1A00000; // nop
		setBL(0x02011B70, (u32)dsiSaveCreate);
		*(u32*)0x02011B84 = 0xE1A00000; // nop
		*(u32*)0x02011B90 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011C10 = 0xE1A00000; // nop
		*(u32*)0x02011C14 = 0xE1A00000; // nop
		*(u32*)0x02011C18 = 0xE1A00000; // nop
		setBL(0x02011C24, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011C3C = 0xE1A00000; // nop
		*(u32*)0x02011C48 = 0xE3A00001; // mov r0, #1
		setBL(0x020313AC+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020313C4+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x020313D4+offsetChange, (u32)dsiSaveSeek);
		setBL(0x020313E4+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020313EC+offsetChange, (u32)dsiSaveClose);
		setBL(0x0203145C+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02031474+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02031488+offsetChange, (u32)dsiSaveSeek);
		setBL(0x02031498+offsetChange, (u32)dsiSaveRead);
		setBL(0x020314A0+offsetChange, (u32)dsiSaveClose);
		setBL(0x02031518+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02031544+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02031580+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02031590+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x0206377C+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0206BE9C+offsetChange, heapEnd);
		*(u32*)(0x0206C228+offsetChange) -= 0x30000;
		*(u32*)(0x02072840+offsetChange) = 0xE1A00000; // nop
	}

	// Sokomania (USA)
	else if (strcmp(romTid, "KSOE") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		u32 newCode = *(u32*)0x02004FD0;
		if (extendedMemory2) {
			newCode += 0x10000;
		}

		*(u32*)0x02005064 = 0xE1A00000; // nop
		setBL(0x0201857C, (u32)dsiSaveCreate);
		setBL(0x0201858C, (u32)dsiSaveOpen);
		setBL(0x020185A8, (u32)dsiSaveGetResultCode);
		setBL(0x020185D0, (u32)dsiSaveSetLength);
		setBL(0x020185F8, (u32)dsiSaveWrite);
		setBL(0x02018600, (u32)dsiSaveClose);
		setBL(0x02018608, (u32)dsiSaveGetResultCode);

		codeCopy((u32*)newCode, (u32*)0x02018634, 0x188);
		setBL(newCode+0x2C, (u32)dsiSaveOpenR);
		setBL(newCode+0x38, (u32)dsiSaveGetResultCode);
		setBL(newCode+0x50, (u32)dsiSaveGetLength);
		setBL(newCode+0x74, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(newCode+0xA4, (u32)dsiSaveRead);
		setBL(newCode+0xBC, (u32)dsiSaveGetResultCode);
		setBL(newCode+0xCC, (u32)dsiSaveClose);
		setBL(0x02015294, newCode);

		*(u32*)0x020326D4 = 0xE1A00000; // nop
		*(u32*)0x02036A58 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203C04C, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x0203C3D8 = (*(u32*)0x02004FD0) + 0x188;
		}
		patchUserSettingsReadDSiWare(0x0203D620);
	}

	// Sokomania (Europe)
	else if (strcmp(romTid, "KSOP") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		u32 newCode = *(u32*)0x02004FC0;
		if (extendedMemory2) {
			newCode += 0x10000;
		}

		setBL(0x02018568, (u32)dsiSaveCreate);
		setBL(0x02018578, (u32)dsiSaveOpen);
		setBL(0x02018594, (u32)dsiSaveGetResultCode);
		setBL(0x020185BC, (u32)dsiSaveSetLength);
		setBL(0x020185E4, (u32)dsiSaveWrite);
		setBL(0x020185EC, (u32)dsiSaveClose);
		setBL(0x020185F4, (u32)dsiSaveGetResultCode);

		codeCopy((u32*)newCode, (u32*)0x02018620, 0x188);
		setBL(newCode+0x2C, (u32)dsiSaveOpenR);
		setBL(newCode+0x38, (u32)dsiSaveGetResultCode);
		setBL(newCode+0x50, (u32)dsiSaveGetLength);
		setBL(newCode+0x74, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(newCode+0xA4, (u32)dsiSaveRead);
		setBL(newCode+0xBC, (u32)dsiSaveGetResultCode);
		setBL(newCode+0xCC, (u32)dsiSaveClose);
		setBL(0x02015280, newCode);

		*(u32*)0x02032644 = 0xE1A00000; // nop
		*(u32*)0x020369A8 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203BF8C, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x0203C318 = (*(u32*)0x02004FC0) + 0x188;
		}
		patchUserSettingsReadDSiWare(0x0203D560);
	}

	// Sokomania 2: Cool Job (USA)
	// Audio does not play on retail consoles
	else if (strcmp(romTid, "KSVE") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		const u32 newCode = (*(u32*)0x02004FE8)+0x10000;

		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0201A81C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FE2C, heapEnd);
		patchUserSettingsReadDSiWare(0x02021410);

		setBL(0x02030480, (u32)dsiSaveCreate);
		setBL(0x02030490, (u32)dsiSaveOpen);
		setBL(0x020304AC, (u32)dsiSaveGetResultCode);
		setBL(0x020304D4, (u32)dsiSaveSetLength);
		setBL(0x020304FC, (u32)dsiSaveWrite);
		setBL(0x02030504, (u32)dsiSaveClose);
		setBL(0x0203050C, (u32)dsiSaveGetResultCode);

		codeCopy((u32*)newCode, (u32*)0x02030538, 0x188);
		setBL(newCode+0x2C, (u32)dsiSaveOpenR);
		setBL(newCode+0x38, (u32)dsiSaveGetResultCode);
		setBL(newCode+0x50, (u32)dsiSaveGetLength);
		setBL(newCode+0x74, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(newCode+0xA4, (u32)dsiSaveRead);
		setBL(newCode+0xBC, (u32)dsiSaveGetResultCode);
		setBL(newCode+0xCC, (u32)dsiSaveClose);
		setBL(0x0204BA50, newCode);

		if (!extendedMemory2) {
			*(u32*)0x0203C414 = 0xE12FFF1E; // bx lr
		}

		/* if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0204F2C0, 0x02021954);
			*(u32*)0x0204F31C = 0xE1A00000; // nop
		} */

		*(u32*)0x02050CB0 = 0xE1A00000; // nop
	}

	// Sokomania 2: Cool Job (Europe)
	// Audio does not play on retail consoles
	else if (strcmp(romTid, "KSVP") == 0) {
		useSharedFont = (twlFontFound && extendedMemory2);
		const u32 newCode = (*(u32*)0x02004FE8)+0x10000;

		*(u32*)0x0200507C = 0xE1A00000; // nop
		*(u32*)0x0200DDF0 = 0xE1A00000; // nop
		*(u32*)0x02012208 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017834, heapEnd);
		patchUserSettingsReadDSiWare(0x02018E18);

		setBL(0x02027E88, (u32)dsiSaveCreate);
		setBL(0x02027E98, (u32)dsiSaveOpen);
		setBL(0x02027EB4, (u32)dsiSaveGetResultCode);
		setBL(0x02027EDC, (u32)dsiSaveSetLength);
		setBL(0x02027F04, (u32)dsiSaveWrite);
		setBL(0x02027F0C, (u32)dsiSaveClose);
		setBL(0x02027F14, (u32)dsiSaveGetResultCode);

		codeCopy((u32*)newCode, (u32*)0x02027F40, 0x188);
		setBL(newCode+0x2C, (u32)dsiSaveOpenR);
		setBL(newCode+0x38, (u32)dsiSaveGetResultCode);
		setBL(newCode+0x50, (u32)dsiSaveGetLength);
		setBL(newCode+0x74, (u32)dsiSaveRead); // dsiSaveReadAsync
		setBL(newCode+0xA4, (u32)dsiSaveRead);
		setBL(newCode+0xBC, (u32)dsiSaveGetResultCode);
		setBL(newCode+0xCC, (u32)dsiSaveClose);
		setBL(0x0204DAB4, newCode);

		if (!extendedMemory2) {
			*(u32*)0x02033E1C = 0xE12FFF1E; // bx lr
		}
	}

	// Sokuren Keisa: Shougaku 1 Nensei (Japan)
	// Sokuren Keisa: Shougaku 2 Nensei (Japan)
	// Sokuren Keisa: Shougaku 3 Nensei (Japan)
	// Sokuren Keisa: Shougaku 4 Nensei (Japan)
	// Sokuren Keisa: Shougaku 5 Nensei (Japan)
	// Sokuren Keisa: Shougaku 6 Nensei (Japan)
	// Sokuren Keisa: Nanmon-Hen (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KL9J") == 0 || strcmp(romTid, "KH2J") == 0 || strcmp(romTid, "KH3J") == 0 || strcmp(romTid, "KH4J") == 0
		   || strcmp(romTid, "KO5J") == 0 || strcmp(romTid, "KO6J") == 0 || strcmp(romTid, "KO7J") == 0) {
		*(u32*)0x0200E148 = 0xE1A00000; // nop
		*(u32*)0x02011A7C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201796C, heapEnd);
		*(u32*)0x02017CF8 -= 0x30000;
		patchUserSettingsReadDSiWare(0x02018E48);
		if (strncmp(romTid, "KL9", 3) == 0 || strncmp(romTid, "KH2", 3) == 0) {
			*(u32*)0x0201FAE4 = 0xE1A00000; // nop
			*(u32*)0x02027C98 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			*(u32*)0x02027EBC = 0xE1A00000; // nop
			*(u32*)0x02027EC4 = 0xE1A00000; // nop
			*(u32*)0x02027ED0 = 0xE1A00000; // nop
		} else {
			*(u32*)0x0201FB20 = 0xE1A00000; // nop
			*(u32*)0x02027CD8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			*(u32*)0x02027EFC = 0xE1A00000; // nop
			*(u32*)0x02027F04 = 0xE1A00000; // nop
			*(u32*)0x02027F10 = 0xE1A00000; // nop
		}
	}

	// Sora Kake Girl: Shojo Shooting (Japan)
	else if (strcmp(romTid, "KU4J") == 0) {
		*(u32*)0x0200B544 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200C0BC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200EEBC = 0xE1A00000; // nop
		patchInitDSiWare(0x02016C6C, heapEnd);
		*(u32*)0x02016FF8 = *(u32*)0x02004FD0;
		*(u32*)0x020492BC = 0xE1A00000; // nop (dsiSaveCreateDir)
		*(u32*)0x020492C4 = 0xE3A00008; // mov r0, #8 (Result code of dsiSaveCreateDir)
		*(u32*)0x020493A4 = 0xE3A00001; // mov r0, #1 (dsiSaveCreateDirAuto)
		setBL(0x0204947C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x02049504, (u32)dsiSaveOpen);
		setBL(0x020495A0, (u32)dsiSaveWrite);
		setBL(0x020495AC, (u32)dsiSaveClose);
		setBL(0x02049658, (u32)dsiSaveOpen);
		setBL(0x020496E0, (u32)dsiSaveGetLength);
		setBL(0x020496FC, (u32)dsiSaveClose);
		setBL(0x02049714, (u32)dsiSaveRead);
		setBL(0x02049720, (u32)dsiSaveClose);
		*(u32*)0x0204988C = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
	}

	// Soul of Darkness (USA)
	// Does not boot: Black screens
	/*else if (strcmp(romTid, "KSKE") == 0) {
		*(u32*)0x020050F4 = 0xE1A00000; // nop
		*(u32*)0x0204CD4C = 0xE1A00000; // nop
		*(u32*)0x0204CDB8 = 0xE1A00000; // nop
		*(u32*)0x02088804 = 0xE1A00000; // nop
		*(u32*)0x0208BF00 = 0xE1A00000; // nop
		*(u32*)0x02090884 = 0xE1A00000; // nop
		*(u32*)0x02092714 = 0xE1A00000; // nop
		*(u32*)0x02092718 = 0xE1A00000; // nop
		*(u32*)0x02092724 = 0xE1A00000; // nop
		*(u32*)0x02092884 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x020928E0, heapEnd); // mov r0, #0x23E0000
	}*/

	// Space Ace (USA)
	else if (strcmp(romTid, "KA6E") == 0) {
		// *(u32*)0x02004B9C = 0x0200002F;
		*(u32*)0x020050D4 = 0xE1A00000; // nop
		*(u32*)0x020051C8 = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02005DD0 = 0xE1A00000; // nop
		// *(u32*)0x02016458 = 0xE1A00000; // nop
		// *(u32*)0x0201645C = 0xE1A00000; // nop
		*(u32*)0x0201F874 = 0xE1A00000; // nop
		setBL(0x0201F8BC, (u32)dsiSaveOpen);
		setBL(0x0201F8D4, (u32)dsiSaveRead);
		setBL(0x0201F8FC, (u32)dsiSaveClose);
		*(u32*)0x0201F960 = 0xE1A00000; // nop
		*(u32*)0x0201F978 = 0xE1A00000; // nop
		*(u32*)0x0201F984 = 0xE1A00000; // nop
		*(u32*)0x0201F98C = 0xE1A00000; // nop
		setBL(0x0201F998, (u32)dsiSaveCreate);
		setBL(0x0201F9C8, (u32)dsiSaveOpen);
		setBL(0x0201F9F8, (u32)dsiSaveWrite);
		setBL(0x0201FA20, (u32)dsiSaveClose);
		*(u32*)0x0201FA54 = 0xE1A00000; // nop
		*(u32*)0x0201FA60 = 0xE1A00000; // nop
		*(u32*)0x0201FAC0 = 0xE1A00000; // nop
		*(u32*)0x0201FAD8 = 0xE1A00000; // nop
		*(u32*)0x0201FAE4 = 0xE1A00000; // nop
		*(u32*)0x0201FAEC = 0xE1A00000; // nop
		setBL(0x0201FAFC, (u32)dsiSaveOpen);
		setBL(0x0201FB38, (u32)dsiSaveSeek);
		setBL(0x0201FB68, (u32)dsiSaveWrite);
		setBL(0x0201FB90, (u32)dsiSaveClose);
		*(u32*)0x0201FBC4 = 0xE1A00000; // nop
		*(u32*)0x0201FBD0 = 0xE1A00000; // nop
		setBL(0x0201FBFC, (u32)dsiSaveGetResultCode);
		setBL(0x0201FC2C, (u32)dsiSaveClose);
		setBL(0x0201FC44, (u32)dsiSaveClose);
		*(u32*)0x0201FC50 = 0xE1A00000; // nop
		*(u32*)0x02033768 = 0xE1A00000; // nop
		*(u32*)0x02033890 = 0xE1A00000; // nop
		*(u32*)0x020338A4 = 0xE1A00000; // nop
		*(u32*)0x02036B88 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203C07C, heapEnd);
		*(u32*)0x0203C408 = 0x02088940;
		patchUserSettingsReadDSiWare(0x0203D984);
		*(u32*)0x0203DDC8 = 0xE1A00000; // nop
		*(u32*)0x0203DDCC = 0xE1A00000; // nop
		*(u32*)0x0203DDD0 = 0xE1A00000; // nop
		*(u32*)0x0203DDD4 = 0xE1A00000; // nop
		// *(u32*)0x0203E9B4 = 0xE1A00000; // nop (Forgot what this does)
	}

	// Space Invaders Extreme Z (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KEVJ") == 0) {
		extern u32* siezHeapAlloc;
		extern u32* siezHeapAddrPtr;

		*(u32*)0x02017904 = 0xE1A00000; // nop
		if (!extendedMemory2 && expansionPakFound) {
			if (s2FlashcardId == 0x5A45) {
				for (int i = 0; i < 2; i++) {
					siezHeapAddrPtr[i] -= 0x01000000;
				}
			}
			tonccpy((u32*)0x020192F4, siezHeapAlloc, 0x50);
		}
		*(u32*)0x0201B794 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202254C, heapEnd);
		*(u32*)0x020228D8 = 0x0213CC60;
		patchUserSettingsReadDSiWare(0x02023A9C);
		if (!extendedMemory2) {
			if (expansionPakFound) {
				setBL(0x020DA69C, 0x020192F4);
			} else {
				// Disable .ntfx file loading: Hides bottom screen background during gameplay
				*(u32*)0x0203DFE0 = 0xE3A00000; // mov r0, #0
				*(u32*)0x0203F3E4 = 0xE1A00000; // nop
				*(u32*)0x0203F434 = 0xE12FFF1E; // bx lr
				*(u32*)0x0203FD5C = 0xE12FFF1E; // bx lr

				// Same as above, but causes slowdown and graphical glitches
				//toncset32((u32*)0x02121C90, 0, 1);
				//toncset32((u32*)0x02121C94, 0, 1);
			}
		}
		*(u32*)0x020E3E24 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020E3E4C = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E3E50 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E43A4 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E43A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E4624 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E4628 = 0xE12FFF1E; // bx lr
		*(u32*)0x020E4854 = 0xE3A00005; // mov r0, #5
		*(u32*)0x020E4858 = 0xE12FFF1E; // bx lr
	}

	// Spin Six (USA)
	// Spin Six (Europe, Australia)
	else if (strcmp(romTid, "KQ6E") == 0 || strcmp(romTid, "KQ6V") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02013150 = 0xE1A00000; // nop
		*(u32*)0x0201317C = 0xE1A00000; // nop
		/* if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02012F14, 0x02073BAC);
			}
		} else { */
			*(u32*)0x02013184 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// }
		if (romTid[3] == 'E') {
			setBL(0x020137B8, (u32)dsiSaveOpen);
			*(u32*)0x02013804 = 0xE1A00000; // nop
			*(u32*)0x02013818 = 0xE1A00000; // nop
			setBL(0x0201382C, (u32)dsiSaveGetLength);
			setBL(0x0201383C, (u32)dsiSaveGetLength);
			setBL(0x02013854, (u32)dsiSaveRead);
			setBL(0x0201385C, (u32)dsiSaveClose);
			setBL(0x020138D4, (u32)dsiSaveCreate);
			setBL(0x0201390C, (u32)dsiSaveOpen);
			setBL(0x0201393C, (u32)dsiSaveSetLength);
			setBL(0x02013960, (u32)dsiSaveWrite);
			setBL(0x02013968, (u32)dsiSaveClose);
			*(u32*)0x02013978 = 0xE1A00000; // nop
			*(u32*)0x020139B4 = 0xE1A00000; // nop
			patchVolumeGetDSiWare(0x02024D5C);
			*(u32*)0x02067804 = 0xE1A00000; // nop
			tonccpy((u32*)0x02068398, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0206AC78 = 0xE1A00000; // nop
			patchInitDSiWare(0x02071F38, heapEnd);
			patchUserSettingsReadDSiWare(0x02073598);
		} else {
			setBL(0x020137C4, (u32)dsiSaveOpen);
			*(u32*)0x02013810 = 0xE1A00000; // nop
			*(u32*)0x02013824 = 0xE1A00000; // nop
			setBL(0x02013838, (u32)dsiSaveGetLength);
			setBL(0x02013848, (u32)dsiSaveGetLength);
			setBL(0x02013860, (u32)dsiSaveRead);
			setBL(0x02013868, (u32)dsiSaveClose);
			setBL(0x020138E0, (u32)dsiSaveCreate);
			setBL(0x02013918, (u32)dsiSaveOpen);
			setBL(0x02013948, (u32)dsiSaveSetLength);
			setBL(0x0201396C, (u32)dsiSaveWrite);
			setBL(0x02013974, (u32)dsiSaveClose);
			*(u32*)0x02013984 = 0xE1A00000; // nop
			*(u32*)0x020139C0 = 0xE1A00000; // nop
			patchVolumeGetDSiWare(0x02024D68);
			*(u32*)0x02067810 = 0xE1A00000; // nop
			tonccpy((u32*)0x020683A4, dsiSaveGetResultCode, 0xC);
			*(u32*)0x0206AC84 = 0xE1A00000; // nop
			patchInitDSiWare(0x02071F44, heapEnd);
			patchUserSettingsReadDSiWare(0x020735A4);
		}
	}

	// Kuru Kuru Akushon: Kuru Pachi 6 (Japan)
	else if (strcmp(romTid, "KQ6J") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02013730 = 0xE1A00000; // nop
		*(u32*)0x02013758 = 0xE1A00000; // nop
		/* if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x020134FC, 0x02076F4C);
			}
		} else { */
			*(u32*)0x02013760 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		// }
		setBL(0x02013D40, (u32)dsiSaveOpen);
		*(u32*)0x02013D84 = 0xE1A00000; // nop
		setBL(0x02013DA4, (u32)dsiSaveGetLength);
		setBL(0x02013DB4, (u32)dsiSaveGetLength);
		setBL(0x02013DC8, (u32)dsiSaveRead);
		setBL(0x02013DD0, (u32)dsiSaveClose);
		setBL(0x02013E44, (u32)dsiSaveCreate);
		setBL(0x02013E74, (u32)dsiSaveOpen);
		setBL(0x02013EAC, (u32)dsiSaveSetLength);
		setBL(0x02013ECC, (u32)dsiSaveWrite);
		setBL(0x02013ED4, (u32)dsiSaveClose);
		patchVolumeGetDSiWare(0x02025004);
		*(u32*)0x02068508 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0206850C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0206928C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0206BB48 = 0xE1A00000; // nop
		patchInitDSiWare(0x020747FC, heapEnd);
		patchUserSettingsReadDSiWare(0x02075E60);
		*(u32*)0x02079B74 = 0xE12FFF1E; // bx lr
	}

	// Spot It! Challenge (USA)
	else if (strcmp(romTid, "KITE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02039254, 0x02019DE4);
			}
		} else {
			*(u32*)0x020050F4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200D970 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200E4E8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02010E7C = 0xE1A00000; // nop
		patchInitDSiWare(0x02018410, heapEnd);
		patchUserSettingsReadDSiWare(0x020198A0);
		setBL(0x020394A4, (u32)dsiSaveCreate);
		setBL(0x020394C0, (u32)dsiSaveOpen);
		setBL(0x020394D4, (u32)dsiSaveSetLength);
		setBL(0x02039500, (u32)dsiSaveClose);
		setBL(0x020395B0, (u32)dsiSaveDelete);
		*(u32*)0x020395F8 = 0xE1A00000; // nop
		setBL(0x0203965C, (u32)dsiSaveOpen);
		setBL(0x02039674, (u32)dsiSaveSeek);
		setBL(0x02039688, (u32)dsiSaveRead);
		setBL(0x02039698, (u32)dsiSaveClose);
		setBL(0x02039780, (u32)dsiSaveOpen);
		setBL(0x02039798, (u32)dsiSaveSeek);
		setBL(0x020397AC, (u32)dsiSaveWrite);
		setBL(0x020397BC, (u32)dsiSaveClose);
	}

	// Spot It! Challenge: Mean Machines (USA)
	else if (strcmp(romTid, "K2UE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020050CC = 0xE1A00000; // nop
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0203A290, 0x0201A460);
			}
		} else {
			*(u32*)0x02005110 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200DEB8 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200EA3C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011458 = 0xE1A00000; // nop
		patchInitDSiWare(0x02018A7C, heapEnd);
		patchUserSettingsReadDSiWare(0x02019F1C);
		*(u32*)0x0203A3A4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0203A3E0 = 0xE12FFF1E; // bx lr
		setBL(0x0203A564, (u32)dsiSaveCreate);
		setBL(0x0203A580, (u32)dsiSaveOpen);
		setBL(0x0203A594, (u32)dsiSaveSetLength);
		setBL(0x0203A5C0, (u32)dsiSaveClose);
		setBL(0x0203A670, (u32)dsiSaveDelete);
		*(u32*)0x0203A6B8 = 0xE1A00000; // nop
		setBL(0x0203A71C, (u32)dsiSaveOpen);
		setBL(0x0203A734, (u32)dsiSaveSeek);
		setBL(0x0203A748, (u32)dsiSaveRead);
		setBL(0x0203A758, (u32)dsiSaveClose);
		setBL(0x0203A844, (u32)dsiSaveOpen);
		setBL(0x0203A85C, (u32)dsiSaveSeek);
		setBL(0x0203A870, (u32)dsiSaveWrite);
		setBL(0x0203A880, (u32)dsiSaveClose);
	}

	// Spot the Difference (USA)
	// Spot the Difference (Europe)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KYSE") == 0 || strcmp(romTid, "KYSP") == 0) && extendedMemory2) {
		// const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08000000 : 0x09000000;
		if (romTid[3] == 'E') {
			*(u32*)0x02007170 = 0xE1A00000; // nop
			*(u32*)0x0200A7E8 = 0xE1A00000; // nop
			patchInitDSiWare(0x0200F9C4, heapEnd);
			*(u32*)0x0200FD50 = *(u32*)0x02004FD0;
			patchUserSettingsReadDSiWare(0x02011020);

			/* *(u16*)0x0200CF5C = 0x4800; // movs r0, #0x????????
			*(u16*)0x0200CF5E = 0x4770; // bx lr
			*(u32*)0x0200CF60 = mepAddr; */
		} else {
			*(u32*)0x0200DECC = 0xE1A00000; // nop
			*(u32*)0x02011544 = 0xE1A00000; // nop
			patchInitDSiWare(0x02016720, heapEnd);
			*(u32*)0x02016AAC = *(u32*)0x02004FD0;
			patchUserSettingsReadDSiWare(0x02017D7C);

			/* *(u16*)0x02013CB8 = 0x4800; // movs r0, #0x????????
			*(u16*)0x02013CBA = 0x4770; // bx lr
			*(u32*)0x02013CBC = mepAddr; */
		}
		setBL(0x02030634, (u32)dsiSaveOpen);
		*(u32*)0x02030674 = 0xE1A00000; // nop
		setBL(0x02030694, (u32)dsiSaveGetLength);
		setBL(0x020306A4, (u32)dsiSaveRead);
		setBL(0x020306AC, (u32)dsiSaveClose);
		*(u32*)0x020306D0 = 0xE3A00000; // mov r0, #0
		setBL(0x020306FC, (u32)dsiSaveOpen);
		*(u32*)0x02030714 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02030724 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x02030740, (u32)dsiSaveCreate);
		setBL(0x0203074C, (u32)dsiSaveClose);
		setBL(0x02030760, (u32)dsiSaveOpen);
		setBL(0x02030770, (u32)dsiSaveGetResultCode);
		*(u32*)0x02030794 = 0xE1A00000; // nop
		setBL(0x020307B8, (u32)dsiSaveSetLength);
		setBL(0x020307C8, (u32)dsiSaveWrite);
		setBL(0x020307D0, (u32)dsiSaveClose);
		/* if (!extendedMemory2 && expansionPakFound) {
			setBLThumb(0x02037AE0, (romTid[3] == 'E') ? 0x0200CF5C : 0x02013CB8);
		} */
	}

	// Atamu IQ Panic (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KYSJ") == 0 && extendedMemory2) {
		*(u32*)0x02007254 = 0xE1A00000; // nop
		*(u32*)0x0200A960 = 0xE1A00000; // nop
		patchInitDSiWare(0x0200FB74, heapEnd);
		*(u32*)0x0200FF00 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020111E0);
		setBL(0x020309B4, (u32)dsiSaveOpen);
		*(u32*)0x020309F4 = 0xE1A00000; // nop
		setBL(0x02030A14, (u32)dsiSaveGetLength);
		setBL(0x02030A24, (u32)dsiSaveRead);
		setBL(0x02030A2C, (u32)dsiSaveClose);
		*(u32*)0x02030A50 = 0xE3A00000; // mov r0, #0
		setBL(0x02030A7C, (u32)dsiSaveOpen);
		*(u32*)0x02030A94 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x02030AA4 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x02030AC0, (u32)dsiSaveCreate);
		setBL(0x02030ACC, (u32)dsiSaveClose);
		setBL(0x02030AE0, (u32)dsiSaveOpen);
		setBL(0x02030AF0, (u32)dsiSaveGetResultCode);
		*(u32*)0x02030B1C = 0xE1A00000; // nop
		setBL(0x02030B40, (u32)dsiSaveSetLength);
		setBL(0x02030B50, (u32)dsiSaveWrite);
		setBL(0x02030B58, (u32)dsiSaveClose);
		*(u32*)0x02037ED6 = 0x2602; // movs r6, #2
	}

	// Spotto! (USA)
	// Requires more than 8MB of RAM
	/* else if (strcmp(romTid, "KSPE") == 0) {
		*(u32*)0x02022AB4 = 0xE1A00000; // nop
		*(u32*)0x02026038 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E054, heapEnd);
		*(u32*)0x0202E3E0 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x0202F530);
		// *(u32*)0x0205504C = 0x00866402; // addeq r6, r6, r2, lsl #8
		*(u32*)0x020558F4 = 0xE1A00000; // nop
		*(u32*)0x020558FC = 0xE1A00000; // nop
		*(u32*)0x02055FAC = 0xE3A00000; // mov r0, #0 (Disable NFTR loading from TWLNAND)
		setBL(0x0205BBB8, (u32)dsiSaveOpen);
		setBL(0x0205BBD4, (u32)dsiSaveGetLength);
		setBL(0x0205BBF4, (u32)dsiSaveRead);
		setBL(0x0205BC0C, (u32)dsiSaveClose);
		setBL(0x0205BC2C, (u32)dsiSaveClose);
		setBL(0x0205BD08, (u32)dsiSaveCreate);
		setBL(0x0205BD20, (u32)dsiSaveOpen);
		setBL(0x0205BD44, (u32)dsiSaveWrite);
		setBL(0x0205BD5C, (u32)dsiSaveClose);
		setBL(0x0205BD64, (u32)dsiSaveClose);
	} */

	// SteamWorld Tower Defense (USA)
	// SteamWorld Tower Defense (Europe, Australia)
	// Soft-locks in save code
	/* else if (strcmp(romTid, "KSWE") == 0 || strcmp(romTid, "KSWV") == 0) {
		setBL(0x02005140, (u32)dsiSaveGetInfo);
		*(u32*)0x0200523C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02005270 = 0xE1A00000; // nop (dsiSaveCloseDir)
		*(u32*)0x02005330 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		*(u32*)0x02005348 += 0xD0000000; // bne -> b
		setBL(0x020053E0, (u32)dsiSaveCreate);
		setBL(0x02005450, (u32)dsiSaveOpen);
		setBL(0x020054B4, (u32)dsiSaveSetLength);
		setBL(0x02005508, (u32)dsiSaveClose);
		setBL(0x020068D8, (u32)dsiSaveOpen);
		setBL(0x02006930, (u32)dsiSaveOpen);
		setBL(0x02006954, (u32)dsiSaveWrite);
		setBL(0x02006968, (u32)dsiSaveClose);
		setBL(0x020069D8, (u32)dsiSaveOpen);
		setBL(0x02006A0C, (u32)dsiSaveRead);
		setBL(0x02006AAC, (u32)dsiSaveRead);
		setBL(0x0200704C, (u32)dsiSaveClose);
		setBL(0x02007070, (u32)dsiSaveClose);
		*(u32*)0x02007124 = 0xE1A00000; // nop
		setBL(0x0200718C, (u32)dsiSaveOpen);
		setBL(0x020071A4, (u32)dsiSaveClose);
		setBL(0x020071E8, (u32)dsiSaveSeek);
		setBL(0x020071F8, (u32)dsiSaveWrite);
		*(u32*)0x0204BA50 = 0xE1A00000; // nop
		*(u32*)0x020DB868 = 0xE1A00000; // nop
		tonccpy((u32*)0x020DC3E0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020DF270 = 0xE1A00000; // nop
		patchInitDSiWare(0x020E558C, heapEnd);
		*(u32*)0x020E5918 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x020E6B28);
	} */

	// Successfully Learning: English, Year 2 (Europe)
	// Successfully Learning: English, Year 3 (Europe)
	// Successfully Learning: English, Year 4 (Europe)
	// Successfully Learning: English, Year 5 (Europe)
	else if (strcmp(romTid, "KEUP") == 0 || strcmp(romTid, "KEZP") == 0 || strcmp(romTid, "KE6P") == 0 || strcmp(romTid, "KE7P") == 0) {
		u8 offsetChange = (romTid[2] == 'U' || romTid[2] == '6') ? 0 : 4;

		*(u32*)0x020174D0 = 0xE1A00000; // nop
		*(u32*)0x0201B0A0 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020AFC, heapEnd);
		*(u32*)0x02020E88 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02022174);
		setBL(0x020C1E34+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020C1E8C+offsetChange, (u32)dsiSaveClose);
		setBL(0x020C1EB4+offsetChange, (u32)dsiSaveRead);
		setBL(0x020C1ED0+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020C1F0C+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020C1F1C+offsetChange, (u32)dsiSaveClose);
		setBL(0x020C1F40+offsetChange, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x020C1F5C+offsetChange, (u32)dsiSaveDelete);
		setBL(0x020C1F78+offsetChange, (u32)dsiSaveGetLength);
		*(u32*)(0x020C3C6C+offsetChange) = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Successfully Learning: German, Year 2 (Europe)
	// Successfully Learning: German, Year 3 (Europe)
	// Successfully Learning: German, Year 4 (Europe)
	// Successfully Learning: German, Year 5 (Europe)
	else if (strcmp(romTid, "KHUP") == 0 || strcmp(romTid, "KHVP") == 0 || strcmp(romTid, "KHYP") == 0 || strcmp(romTid, "KHZP") == 0) {
		u8 offsetChange = (romTid[2] == 'U' || romTid[2] == 'Y') ? 0 : 4;

		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02005120 = 0xE1A00000; // nop
		*(u32*)0x02016F0C = 0xE1A00000; // nop
		*(u32*)0x0201AADC = 0xE1A00000; // nop
		patchInitDSiWare(0x02020538, heapEnd);
		*(u32*)0x020208C4 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02021BB0);
		setBL(0x0208D668+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0208D6C0+offsetChange, (u32)dsiSaveClose);
		setBL(0x0208D6E8+offsetChange, (u32)dsiSaveRead);
		setBL(0x0208D704+offsetChange, (u32)dsiSaveWrite);
		setBL(0x0208D740+offsetChange, (u32)dsiSaveOpen);
		setBL(0x0208D750+offsetChange, (u32)dsiSaveClose);
		setBL(0x0208D774+offsetChange, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x0208D790+offsetChange, (u32)dsiSaveDelete);
		setBL(0x0208D7AC+offsetChange, (u32)dsiSaveGetLength);
		*(u32*)(0x0208F4CC+offsetChange) = 0xE12FFF1E; // bx lr (Skip Manual screen)
	}

	// Successfully Learning: Mathematics, Year 2 (Europe)
	// Successfully Learning: Mathematics, Year 3 (Europe)
	// Successfully Learning: Mathematics, Year 4 (Europe)
	// Successfully Learning: Mathematics, Year 5 (Europe)
	else if (strcmp(romTid, "KKUP") == 0 || strcmp(romTid, "KKVP") == 0 || strcmp(romTid, "KKWP") == 0 || strcmp(romTid, "KKXP") == 0) {
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x02005120 = 0xE1A00000; // nop
		*(u32*)0x02017538 = 0xE1A00000; // nop
		*(u32*)0x0201B108 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020B64, heapEnd);
		*(u32*)0x02020EF0 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x020221DC);
		if (romTid[2] == 'U') {
			setBL(0x020C7B90, (u32)dsiSaveOpen);
			setBL(0x020C7BE8, (u32)dsiSaveClose);
			setBL(0x020C7C10, (u32)dsiSaveRead);
			setBL(0x020C7C2C, (u32)dsiSaveWrite);
			setBL(0x020C7C68, (u32)dsiSaveOpen);
			setBL(0x020C7C78, (u32)dsiSaveClose);
			setBL(0x020C7C9C, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x020C7CB8, (u32)dsiSaveDelete);
			setBL(0x020C7CD4, (u32)dsiSaveGetLength);
			*(u32*)0x020C99F4 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		} else {
			u8 offsetChange = (romTid[2] == 'X') ? 0 : 4;
			setBL(0x0212CA9C+offsetChange, (u32)dsiSaveOpen);
			setBL(0x0212CAF4+offsetChange, (u32)dsiSaveClose);
			setBL(0x0212CB1C+offsetChange, (u32)dsiSaveRead);
			setBL(0x0212CB38+offsetChange, (u32)dsiSaveWrite);
			setBL(0x0212CB74+offsetChange, (u32)dsiSaveOpen);
			setBL(0x0212CB84+offsetChange, (u32)dsiSaveClose);
			setBL(0x0212CBA8+offsetChange, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0212CBC4+offsetChange, (u32)dsiSaveDelete);
			setBL(0x0212CBE8+offsetChange, (u32)dsiSaveGetLength);
			*(u32*)(0x0212E900+offsetChange) = 0xE12FFF1E; // bx lr (Skip Manual screen)
		}
	}

	// Sudoku (USA)
	// Sudoku (USA) (Rev 1)
	else if (strcmp(romTid, "K4DE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (ndsHeader->romversion == 1) {
			if (useSharedFont) {
				if (!extendedMemory2 && expansionPakFound) {
					patchTwlFontLoad(0x02020200, 0x020C5348);
				}
			} else {
				*(u32*)0x0200698C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
			// *(u32*)0x0203701C = 0xE3A00001; // mov r0, #1
			// *(u32*)0x02037020 = 0xE12FFF1E; // bx lr
			setBL(0x02037560, (u32)dsiSaveOpen);
			*(u32*)0x02037580 = 0xE1A00000; // nop
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
			*(u32*)0x020B7CA4 = 0xE1A00000; // nop
			*(u32*)0x020BB4B8 = 0xE1A00000; // nop
			patchInitDSiWare(0x020C2A8C, heapEnd);
			patchUserSettingsReadDSiWare(0x020C40E4);
		} else {
			if (useSharedFont) {
				if (!extendedMemory2 && expansionPakFound) {
					patchTwlFontLoad(0x020208D0, 0x020ADBA4);
				}
			} else {
				*(u32*)0x0200695C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
			}
			// *(u32*)0x0203609C = 0xE3A00001; // mov r0, #1
			// *(u32*)0x020360A0 = 0xE12FFF1E; // bx lr
			setBL(0x020364A4, (u32)dsiSaveOpen);
			*(u32*)0x020364C0 = 0xE1A00000; // nop
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
			*(u32*)0x020A0670 = 0xE1A00000; // nop
			*(u32*)0x020A3E84 = 0xE1A00000; // nop
			patchInitDSiWare(0x020AB458, heapEnd);
			patchUserSettingsReadDSiWare(0x020ACAB0);
		}
	}

	// Sudoku (Europe, Australia) (Rev 1)
	else if (strcmp(romTid, "K4DV") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x0202024C, 0x020C5394);
			}
		} else {
			*(u32*)0x0200698C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
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
		*(u32*)0x020B7CF0 = 0xE1A00000; // nop
		*(u32*)0x020BB504 = 0xE1A00000; // nop
		patchInitDSiWare(0x020C2AD8, heapEnd);
		patchUserSettingsReadDSiWare(0x020C4130);
	}

	// Sudoku 4Pockets (USA)
	// Sudoku 4Pockets (Europe)
	else if (strcmp(romTid, "K4FE") == 0 || strcmp(romTid, "K4FP") == 0) {
		*(u32*)0x02004C4C = 0xE1A00000; // nop (Skip Manual screen)
		*(u32*)0x02013030 = 0xE1A00000; // nop
		*(u32*)0x0201680C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B500, heapEnd);
		*(u32*)0x0202E888 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202E88C = 0xE12FFF1E; // bx lr
	}

	// Sudoku & Kakuro: Welt Edition (Germany)
	else if (strcmp(romTid, "KWUD") == 0) {
		*(u32*)0x02012064 = 0xE1A00000; // nop
		tonccpy((u32*)0x02012BDC, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020155F4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B058, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C694);
		setBL(0x0202680C, (u32)dsiSaveCreate);
		setBL(0x02026828, (u32)dsiSaveOpen);
		setBL(0x0202688C, (u32)dsiSaveWrite);
		setBL(0x02026894, (u32)dsiSaveClose);
		*(u32*)0x020268C8 = 0xE1A00000; // nop
		*(u32*)0x02026930 = 0xE1A00000; // nop
		*(u32*)0x020269C0 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x020269D0 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x020269E0, (u32)dsiSaveOpen);
		setBL(0x02026BFC, (u32)dsiSaveOpen);
		setBL(0x02026C14, (u32)dsiSaveSeek);
		setBL(0x02026C48, (u32)dsiSaveWrite);
		setBL(0x02026C50, (u32)dsiSaveClose);
		*(u32*)0x02026C84 = 0xE1A00000; // nop
		setBL(0x02026CF0, (u32)dsiSaveOpen);
		setBL(0x02026D08, (u32)dsiSaveSeek);
		setBL(0x02026D1C, (u32)dsiSaveRead);
		setBL(0x02026D24, (u32)dsiSaveClose);
		*(u32*)0x02026D58 = 0xE1A00000; // nop
		*(u32*)0x0203CDF8 = 0xE1A00000; // nop (Do not load Manual screen)
	}

	// Sudoku Challenge! (USA)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KSCE") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x02005458, 0x020391EC);
			} */
		} else {
			*(u32*)0x0200509C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x02005248 = 0xE1A00000; // nop
		*(u32*)0x02005260 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x0201C3B0 = 0xE3A00623; // mov r0, #0x02300000
		}
		*(u32*)0x0202F298 = 0xE1A00000; // nop
		*(u32*)0x0203255C = 0xE1A00000; // nop
		patchInitDSiWare(0x020376D8, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x02037A48 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x02038C78);
		*(u32*)0x020390A8 = 0xE1A00000; // nop
		*(u32*)0x020390AC = 0xE1A00000; // nop
		*(u32*)0x020390B0 = 0xE1A00000; // nop
		*(u32*)0x020390B4 = 0xE1A00000; // nop
	}

	// Sudoku Challenge! (Europe)
	// Saving not supported due to using more than one file in filesystem
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KSCP") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (useSharedFont) {
			/* if (!extendedMemory2) {
				patchTwlFontLoad(0x0200541C, 0x020425B8);
			} */
		} else {
			*(u32*)0x0200510C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200526C = 0xE1A00000; // nop
		*(u32*)0x0200528C = 0xE1A00000; // nop
		*(u32*)0x02005498 = 0xE1A00000; // nop
		*(u32*)0x020054D4 = 0xE1A00000; // nop
		*(u32*)0x020054E0 = 0xE1A00000; // nop
		*(u32*)0x02005524 = 0xE1A00000; // nop
		*(u32*)0x02005530 = 0xE1A00000; // nop
		if (!extendedMemory2) {
			*(u32*)0x02025D24 = 0xE3A00623; // mov r0, #0x02300000
		}
		*(u32*)0x02038AD8 = 0xE1A00000; // nop
		*(u32*)0x0203BC34 = 0xE1A00000; // nop
		patchInitDSiWare(0x02040AD8, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x02040E64 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x02042074);
		*(u32*)0x0204247C = 0xE1A00000; // nop
		*(u32*)0x02042480 = 0xE1A00000; // nop
		*(u32*)0x02042484 = 0xE1A00000; // nop
		*(u32*)0x02042488 = 0xE1A00000; // nop
	}

	// Super Swap (USA)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K4WE") == 0 && extendedMemory2) {
		*(u8*)0x020050C8 = 0x60;
		*(u32*)0x02005138 = 0x60BA00;
		*(u32*)0x0200C378 = 0xE1A00000; // nop
		*(u32*)0x0200C380 = 0xE1A00000; // nop
		setBL(0x0200C86C, (u32)dsiSaveOpen);
		setBL(0x0200C884, (u32)dsiSaveClose);
		setBL(0x0200C8A4, (u32)dsiSaveCreate);
		setBL(0x0200C8BC, (u32)dsiSaveOpen);
		setBL(0x0200C8D4, (u32)dsiSaveClose);
		setBL(0x0200C8DC, (u32)dsiSaveDelete);
		setBL(0x0200C9DC, (u32)dsiSaveOpen);
		setBL(0x0200C9F4, (u32)dsiSaveGetLength);
		setBL(0x0200CA18, (u32)dsiSaveRead);
		setBL(0x0200CA20, (u32)dsiSaveClose);
		setBL(0x0200CA5C, (u32)dsiSaveOpen);
		setBL(0x0200CA70, (u32)dsiSaveClose);
		setBL(0x0200CA84, (u32)dsiSaveCreate);
		setBL(0x0200CA9C, (u32)dsiSaveOpen);
		*(u32*)0x0200CAAC = 0xE1A00000; // nop
		setBL(0x0200CAB8, (u32)dsiSaveClose);
		setBL(0x0200CAC0, (u32)dsiSaveDelete);
		setBL(0x0200CAD4, (u32)dsiSaveCreate);
		setBL(0x0200CAE4, (u32)dsiSaveOpen);
		setBL(0x0200CAF4, (u32)dsiSaveGetResultCode);
		setBL(0x0200CB0C, (u32)dsiSaveSetLength);
		setBL(0x0200CB1C, (u32)dsiSaveWrite);
		setBL(0x0200CB24, (u32)dsiSaveClose);
		*(u32*)0x0200EBD8 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02042980 = 0xE1A00000; // nop
		*(u32*)0x02045BE0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204B2D0, heapEnd);
		*(u32*)0x0204B65C = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x0204C910);
		*(u32*)0x0204CD44 = 0xE1A00000; // nop
		*(u32*)0x0204CD48 = 0xE1A00000; // nop
		*(u32*)0x0204CD4C = 0xE1A00000; // nop
		*(u32*)0x0204CD50 = 0xE1A00000; // nop
	}

	// Super Swap (Europe)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "K4WP") == 0 && extendedMemory2) {
		*(u8*)0x020050CC = 0x60;
		*(u32*)0x0200513C = 0x60BA00;
		*(u32*)0x0200C304 = 0xE1A00000; // nop
		*(u32*)0x0200C30C = 0xE1A00000; // nop
		setBL(0x0200C7EC, (u32)dsiSaveOpen);
		setBL(0x0200C800, (u32)dsiSaveClose);
		setBL(0x0200C820, (u32)dsiSaveCreate);
		setBL(0x0200C83C, (u32)dsiSaveOpen);
		setBL(0x0200C854, (u32)dsiSaveClose);
		setBL(0x0200C85C, (u32)dsiSaveDelete);
		setBL(0x0200C968, (u32)dsiSaveOpen);
		setBL(0x0200C980, (u32)dsiSaveGetLength);
		setBL(0x0200C9A4, (u32)dsiSaveRead);
		setBL(0x0200C9AC, (u32)dsiSaveClose);
		setBL(0x0200C9F0, (u32)dsiSaveOpen);
		setBL(0x0200CA04, (u32)dsiSaveClose);
		setBL(0x0200CA18, (u32)dsiSaveCreate);
		setBL(0x0200CA34, (u32)dsiSaveOpen);
		*(u32*)0x0200CA44 = 0xE1A00000; // nop
		setBL(0x0200CA50, (u32)dsiSaveClose);
		setBL(0x0200CA58, (u32)dsiSaveDelete);
		setBL(0x0200CA70, (u32)dsiSaveCreate);
		setBL(0x0200CA80, (u32)dsiSaveOpen);
		setBL(0x0200CA90, (u32)dsiSaveGetResultCode);
		setBL(0x0200CAAC, (u32)dsiSaveSetLength);
		setBL(0x0200CABC, (u32)dsiSaveWrite);
		setBL(0x0200CAC4, (u32)dsiSaveClose);
		*(u32*)0x0200E9FC = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x020416D4 = 0xE1A00000; // nop
		*(u32*)0x02044934 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204A024, heapEnd);
		*(u32*)0x0204A3B0 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x0204B664);
		*(u32*)0x0204BA98 = 0xE1A00000; // nop
		*(u32*)0x0204BA9C = 0xE1A00000; // nop
		*(u32*)0x0204BAA0 = 0xE1A00000; // nop
		*(u32*)0x0204BAA4 = 0xE1A00000; // nop
	}

	// Super Yum Yum: Puzzle Adventures (USA)
	// Super Yum Yum: Puzzle Adventures (Europe, Australia)
	// Due to our save implementation, save data is stored in all 4 slots
	// Requires 8MB of RAM
 	else if ((strcmp(romTid, "K4PE") == 0 || strcmp(romTid, "K4PV") == 0) && extendedMemory2) {
		u16 offsetChange = (romTid[3] == 'E') ? 0 : 0x228;
		const u32 newCode = 0x02018518;

		codeCopy((u32*)newCode, (u32*)0x02028E90, 0xB8);
		setBL(newCode+0x2C, (u32)dsiSaveOpenR);
		setBL(newCode+0x3C, (u32)dsiSaveGetLength);
		setBL(newCode+0x6C, (u32)dsiSaveRead);
		setBL(newCode+0x9C, (u32)dsiSaveClose);

		// if (!extendedMemory2) {
		// 	*(u32*)0x020050C8 = 0xE1A00000; // nop (Disable audio)
		// }
		*(u32*)0x02016F5C = 0xE1A00000; // nop
		tonccpy((u32*)0x02017AD4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201AC44 = 0xE1A00000; // nop
		patchInitDSiWare(0x02020844, heapEnd);
		*(u32*)0x02020BD0 = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x02021F9C);
		*(u32*)0x0202870C = 0xE2851602; // add r1, r5, #0x200000
		*(u32*)(0x02062A54+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02062A5C+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x02062B34+offsetChange) = 0xE3A00003; // mov r0, #3
		setBL(0x02062B58+offsetChange, newCode);
		*(u32*)(0x02062BE4+offsetChange) = 0xE3A00003; // mov r0, #3
		*(u32*)(0x02062C28+offsetChange) = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)(0x02062CDC+offsetChange) = 0xE3A00003; // mov r0, #3
		*(u32*)(0x02062D3C+offsetChange) = 0xE3A00001; // mov r0, #1 (Branch to dsiSaveOpenDir)
		*(u32*)(0x02062E7C+offsetChange) = 0xE3A00001; // mov r0, #1 (Branch to dsiSaveCreateDirAuto)
		setBL(0x02062EA0+offsetChange, (u32)dsiSaveCreate);
		setBL(0x02062F08+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02062F70+offsetChange, (u32)dsiSaveSetLength);
		setBL(0x02062F88+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02062F98+offsetChange, (u32)dsiSaveClose);
	}

	// Surfacer+ (USA)
	// Surfacer+ (Europe)
	else if (strcmp(romTid, "KOWE") == 0 || strcmp(romTid, "KOWP") == 0) {
		const u32 dsiSaveGetResultCodeT = 0x0201AB18;
		*(u16*)dsiSaveGetResultCodeT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetResultCodeT + 4), dsiSaveGetResultCode, 0xC);

		const u32 dsiSaveCreateT = 0x0201B1A0;
		*(u16*)dsiSaveCreateT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCreateT + 4), dsiSaveCreate, 0xC);

		const u32 dsiSaveOpenT = 0x0201B1B0;
		*(u16*)dsiSaveOpenT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveOpenT + 4), dsiSaveOpen, 0xC);

		const u32 dsiSaveCloseT = 0x0201B1C0;
		*(u16*)dsiSaveCloseT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveCloseT + 4), dsiSaveClose, 0xC);

		const u32 dsiSaveGetLengthT = 0x0201B1D0;
		*(u16*)dsiSaveGetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveGetLengthT + 4), dsiSaveGetLength, 0xC);

		const u32 dsiSaveSetLengthT = 0x0201B2F4;
		*(u16*)dsiSaveSetLengthT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveSetLengthT + 4), dsiSaveSetLength, 0xC);

		const u32 dsiSaveReadT = 0x0201B1E0;
		*(u16*)dsiSaveReadT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveReadT + 4), dsiSaveRead, 0xC);

		const u32 dsiSaveWriteT = 0x0201B49C;
		*(u16*)dsiSaveWriteT = 0x4778; // bx pc
		tonccpy((u32*)(dsiSaveWriteT + 4), dsiSaveWrite, 0xC);

		doubleNopT(0x02004A3A);
		doubleNopT(0x02004A48);
		doubleNopT(0x02004A74);
		doubleNopT(0x02006BB8);
		doubleNopT(0x02006E9E);
		*(u16*)0x02006EB2 = 0x2000; // movs r0, #0 (dsiSaveGetArcSrc)
		*(u16*)0x02006EB4 = nopT;
		doubleNopT(0x02006ED4);
		doubleNopT(0x02006EDC);
		doubleNopT(0x02006EFE);
		doubleNopT(0x02006F06);
		doubleNopT(0x02006F18);
		doubleNopT(0x02006F20);
		setBLThumb(0x02006FE0, dsiSaveOpenT);
		setBLThumb(0x02006FFA, dsiSaveGetLengthT);
		setBLThumb(0x02007012, dsiSaveReadT);
		setBLThumb(0x02007018, dsiSaveCloseT);
		setBLThumb(0x0200704C, dsiSaveOpenT);
		setBLThumb(0x02007078, dsiSaveCloseT);
		*(u16*)0x020125E4 = 0x4770; // bx lr (Show white screen instead of manual screen)

		doubleNopT(0x0201763E);
		doubleNopT(0x02017642);
		doubleNopT(0x0201764E);
		doubleNopT(0x02017732);
		patchHiHeapDSiWareThumb(0x02017770, 0x02018960, heapEnd);
		patchUserSettingsReadDSiWare(0x02018502);
		doubleNopT(0x0201942C);
		doubleNopT(0x0201C616);
		doubleNopT(0x0201EA66);
	}

	// Sutanoberuzu: Kono Hareta Sora no Shita de (Japan)
	// Sutanoberuzu: Shirogane no Torikago (Japan)
	// Requires either 8MB of RAM or Memory Expansion Pak
	// K97J: Music does not play on retail consoles
	else if ((strcmp(romTid, "K97J") == 0 || strcmp(romTid, "K98J") == 0) && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08000000 : 0x09000000;

		*(u32*)0x020050C8 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		*(u32*)0x02005110 = 0xE1A00000; // nop (Show white screen instead of manual screen)
		*(u32*)0x0200C184 = 0xE1A00000; // nop
		*(u32*)0x0200F86C = 0xE1A00000; // nop
		patchInitDSiWare(0x02015BC4, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		*(u32*)0x02015F50 = extendedMemory2 ? *(u32*)0x02004FE8 : mepAddr;
		patchUserSettingsReadDSiWare(0x0201730C);
		setBL(0x02024EC0, (u32)dsiSaveGetResultCode);
		*(u32*)0x02024FA8 = 0xE1A00000; // nop
		*(u32*)0x02024FCC = 0xE1A00000; // nop
		setBL(0x02024FEC, (u32)dsiSaveOpen);
		*(u32*)0x02025008 = 0xE1A00000; // nop
		setBL(0x0202502C, (u32)dsiSaveRead);
		*(u32*)0x02025048 = 0xE1A00000; // nop
		setBL(0x0202505C, (u32)dsiSaveClose);
		*(u32*)0x02025078 = 0xE1A00000; // nop
		*(u32*)0x02025088 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020250A0 = 0xE1A00000; // nop
		*(u32*)0x020250C8 = 0xE1A00000; // nop
		*(u32*)0x020250E8 = 0xE1A00000; // nop
		setBL(0x02025104, (u32)dsiSaveOpen);
		*(u32*)0x02025130 = 0xE1A00000; // nop
		setBL(0x02025158, (u32)dsiSaveWrite);
		setBL(0x02025178, (u32)dsiSaveClose);
		*(u32*)0x02025198 = 0xE1A00000; // nop
		*(u32*)0x020251A8 = 0xE3A00000; // mov r0, #0
		*(u32*)0x020251C0 = 0xE1A00000; // nop
		setBL(0x020251E8, (u32)dsiSaveCreate);
		setBL(0x02025244, (u32)dsiSaveDelete);
		if (!extendedMemory2) {
			if (romTid[2] == '7') {
				*(u32*)0x0202B7DC = 0xE3A00000; // mov r0, #0 (Disable MobiClip playback)
				*(u32*)0x0202E56C = 0xE3A00622; // mov r0, #0x02200000
			} else {
				*(u32*)0x0202B7AC = 0xE3A00000; // mov r0, #0 (Disable MobiClip playback)
				*(u32*)0x0202E2CC = 0xE3A00622; // mov r0, #0x02200000
				*(u32*)0x0202E2DC = 0xE3A00623; // mov r0, #0x02300000
			}
		}
	}

	// Tales to Enjoy!: Little Red Riding Hood (USA)
	// Tales to Enjoy!: Puss in Boots (USA)
	// Tales to Enjoy!: The Three Little Pigs (USA)
	// Tales to Enjoy!: The Ugly Duckling (USA)
	else if (strcmp(romTid, "KZUE") == 0 || strcmp(romTid, "KZVE") == 0 || strcmp(romTid, "KZ7E") == 0 || strcmp(romTid, "KZ8E") == 0) {
		*(u32*)0x02006F60 = 0xE1A00000; // nop
		*(u32*)0x0200A80C = 0xE1A00000; // nop
		patchInitDSiWare(0x0200FE40, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x020101CC -= 0x30000;
		}
		patchUserSettingsReadDSiWare(0x02011470);
		setBL(0x0204D500, (u32)dsiSaveOpen);
		*(u32*)0x0204D540 = 0xE1A00000; // nop
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
		*(u32*)0x0204D660 = 0xE1A00000; // nop
		setBL(0x0204D684, (u32)dsiSaveSetLength);
		setBL(0x0204D694, (u32)dsiSaveWrite);
		setBL(0x0204D69C, (u32)dsiSaveClose);
	}

	// Tangrams (USA)
	else if (strcmp(romTid, "KYYE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02010A2C = 0xE1A00000; // nop
		*(u32*)0x02014720 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A04C, heapEnd);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0201FE10, 0x0201BB4C);
		}
		*(u32*)0x0201FF54 = 0xE1A00000; // nop
		*(u32*)0x0201FF6C = 0xE1A00000; // nop
		*(u32*)0x020397C0 = 0xE1A00000; // nop
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
	else if (strcmp(romTid, "KJTJ") == 0) {
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
		*(u32*)0x0202D3A4 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0202D3A8 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		tonccpy((u32*)0x0202E118, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02031180 = 0xE1A00000; // nop
		patchInitDSiWare(0x0203958C, heapEnd);
	}

	// Tantei Jinguuji Saburou: Akenaiyoru ni (Japan)
	// Tantei Jinguuji Saburou: Kadannoitte (Japan)
	// Tantei Jinguuji Saburou: Rensa Suru Noroi (Japan)
	// Tantei Jinguuji Saburou: Nakiko no Shouzou (Japan)
	else if (strcmp(romTid, "KJAJ") == 0 || strcmp(romTid, "KJQJ") == 0 || strcmp(romTid, "KJLJ") == 0 || strcmp(romTid, "KJ7J") == 0) {
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
			*(u32*)0x0202D49C = 0xE1A00000; // nop
			tonccpy((u32*)0x0202E130, dsiSaveGetResultCode, 0xC);
			*(u32*)0x020311D8 = 0xE1A00000; // nop
			patchInitDSiWare(0x02038034, heapEnd);
			*(u32*)0x0203996C = 0xE1A00000; // nop
			*(u32*)0x02039970 = 0xE1A00000; // nop
			*(u32*)0x02039974 = 0xE1A00000; // nop
			*(u32*)0x02039978 = 0xE1A00000; // nop
		} else if (strncmp(romTid, "KJ7", 3) != 0) {
			*(u32*)0x0202D4B4 = 0xE1A00000; // nop
			tonccpy((u32*)0x0202E148, dsiSaveGetResultCode, 0xC);
			*(u32*)0x020311F0 = 0xE1A00000; // nop
			patchInitDSiWare(0x0203804C, heapEnd);
			*(u32*)0x02039984 = 0xE1A00000; // nop
			*(u32*)0x02039988 = 0xE1A00000; // nop
			*(u32*)0x0203998C = 0xE1A00000; // nop
			*(u32*)0x02039990 = 0xE1A00000; // nop
		} else {
			*(u32*)0x0202D51C = 0xE1A00000; // nop
			tonccpy((u32*)0x0202E1B0, dsiSaveGetResultCode, 0xC);
			*(u32*)0x02031258 = 0xE1A00000; // nop
			patchInitDSiWare(0x020380B4, heapEnd);
			*(u32*)0x020399EC = 0xE1A00000; // nop
			*(u32*)0x020399F0 = 0xE1A00000; // nop
			*(u32*)0x020399F4 = 0xE1A00000; // nop
			*(u32*)0x020399F8 = 0xE1A00000; // nop
		}
	}

	// Telegraph Crosswords (USA)
	// Telegraph Crosswords (Europe)
	else if (strcmp(romTid, "KXQE") == 0 || strcmp(romTid, "KXQP") == 0) {
		// if (!extendedMemory2) {
			u32 bssEnd = *(u32*)0x0207EF20;
			u32 sdatOffset = 0x020CBFB4;

			*(u32*)0x0207EF20 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x140000, bssEnd, sdatOffset);
		// }

		*(u32*)0x0200EDF0 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200F968, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02012380 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017B20, heapEnd);
		*(u32*)0x02017EAC = *(u32*)0x0207EF20;
		patchUserSettingsReadDSiWare(0x02019020);
		*(u32*)0x02029144 = 0xE1A00000; // nop (Do not load Manual screen)
		*(u32*)0x0202B008 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		setBL(0x0202C6C0, (u32)dsiSaveOpen);
		setBL(0x0202C6D8, (u32)dsiSaveSeek);
		setBL(0x0202C6E8, (u32)dsiSaveWrite);
		setBL(0x0202C6F0, (u32)dsiSaveClose);
		setBL(0x0202D9B4, (u32)dsiSaveOpen);
		setBL(0x0202D9CC, (u32)dsiSaveSeek);
		setBL(0x0202D9DC, (u32)dsiSaveWrite);
		setBL(0x0202D9E4, (u32)dsiSaveClose);
		setBL(0x0202DA80, (u32)dsiSaveOpen);
		setBL(0x0202DA98, (u32)dsiSaveSeek);
		setBL(0x0202DAA8, (u32)dsiSaveWrite);
		setBL(0x0202DC34, (u32)dsiSaveOpen);
		setBL(0x0202DC4C, (u32)dsiSaveSeek);
		setBL(0x0202DC5C, (u32)dsiSaveWrite);
		setBL(0x0202DC64, (u32)dsiSaveClose);
		setBL(0x0202DFF4, (u32)dsiSaveOpen);
		setBL(0x0202E00C, (u32)dsiSaveSeek);
		setBL(0x0202E01C, (u32)dsiSaveWrite);
		setBL(0x0202E024, (u32)dsiSaveClose);
		setBL(0x0202E53C, (u32)dsiSaveOpen);
		setBL(0x0202E554, (u32)dsiSaveSeek);
		setBL(0x0202E564, (u32)dsiSaveWrite);
		setBL(0x0202E56C, (u32)dsiSaveClose);
		setBL(0x0202E704, (u32)dsiSaveOpen);
		setBL(0x0202E71C, (u32)dsiSaveSeek);
		setBL(0x0202E72C, (u32)dsiSaveWrite);
		setBL(0x0202E734, (u32)dsiSaveClose);
		setBL(0x0202E990, (u32)dsiSaveOpen);
		setBL(0x0202E9A8, (u32)dsiSaveSeek);
		setBL(0x0202E9B8, (u32)dsiSaveWrite);
		setBL(0x0202E9C0, (u32)dsiSaveClose);
		*(u32*)0x0202EA3C = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0202EA4C = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x0202EA5C, (u32)dsiSaveOpen);
		setBL(0x0202EA74, (u32)dsiSaveSeek);
		setBL(0x0202EA84, (u32)dsiSaveRead);
		setBL(0x0202EA8C, (u32)dsiSaveClose);
		setBL(0x0202EAC8, (u32)dsiSaveClose);
		setBL(0x0202EB08, (u32)dsiSaveCreate);
		setBL(0x0202EB20, (u32)dsiSaveOpen);
		setBL(0x0202EB58, (u32)dsiSaveSeek);
		setBL(0x0202EB68, (u32)dsiSaveWrite);
		setBL(0x0202EB70, (u32)dsiSaveClose);
		setBL(0x0202EB8C, (u32)dsiSaveClose);
		setBL(0x0202EBEC, (u32)dsiSaveClose);
		*(u32*)0x0202F158 = 0xE1A00000; // nop
		*(u32*)0x0202F168 = 0xE1A00000; // nop (dsiSaveCreateDir)
		setBL(0x0202F174, (u32)dsiSaveCreate);
		setBL(0x0202F184, (u32)dsiSaveOpen);
		setBL(0x0202F1B0, (u32)dsiSaveSeek);
		setBL(0x0202F1C0, (u32)dsiSaveWrite);
		setBL(0x0202F1C8, (u32)dsiSaveClose);
	}

	// Telegraph Sudoku & Kakuro (USA)
	// Telegraph Sudoku & Kakuro (Europe)
	else if (strcmp(romTid, "KXLE") == 0 || strcmp(romTid, "KXLP") == 0) {
		*(u32*)0x02011FF4 = 0xE1A00000; // nop
		tonccpy((u32*)0x02012B6C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02015584 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201AFE8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C624);
		setBL(0x0202679C, (u32)dsiSaveCreate);
		setBL(0x020267B8, (u32)dsiSaveOpen);
		setBL(0x0202681C, (u32)dsiSaveWrite);
		setBL(0x02026824, (u32)dsiSaveClose);
		*(u32*)0x02026858 = 0xE1A00000; // nop
		*(u32*)0x020268C0 = 0xE1A00000; // nop
		*(u32*)0x02026950 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02026960 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02026970, (u32)dsiSaveOpen);
		setBL(0x02026B8C, (u32)dsiSaveOpen);
		setBL(0x02026BA4, (u32)dsiSaveSeek);
		setBL(0x02026BD8, (u32)dsiSaveWrite);
		setBL(0x02026BE0, (u32)dsiSaveClose);
		*(u32*)0x02026C14 = 0xE1A00000; // nop
		setBL(0x02026C80, (u32)dsiSaveOpen);
		setBL(0x02026C98, (u32)dsiSaveSeek);
		setBL(0x02026CAC, (u32)dsiSaveRead);
		setBL(0x02026CB4, (u32)dsiSaveClose);
		*(u32*)0x02026CE8 = 0xE1A00000; // nop
		if (romTid[3] == 'E') {
			*(u32*)0x0203CB28 = 0xE1A00000; // nop (Do not load Manual screen)
		} else {
			*(u32*)0x0203CB04 = 0xE1A00000; // nop (Do not load Manual screen)
		}
	}

	// Oshiete Darling (Japan)
	else if (strcmp(romTid, "KOSJ") == 0) {
		*(u32*)0x0200B3F4 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200BF6C, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200E894 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201412C, heapEnd);
		patchUserSettingsReadDSiWare(0x02015704);
		*(u32*)0x020192FC = 0xE1A00000; // nop
		setBL(0x0201BA8C, (u32)dsiSaveCreate);
		setBL(0x0201BACC, (u32)dsiSaveOpen);
		setBL(0x0201BAFC, (u32)dsiSaveSetLength);
		setBL(0x0201BB20, (u32)dsiSaveWrite);
		setBL(0x0201BB50, (u32)dsiSaveClose);
		setBL(0x0201BB9C, (u32)dsiSaveOpen);
		setBL(0x0201BBC4, (u32)dsiSaveGetLength);
		setBL(0x0201BBD8, (u32)dsiSaveRead);
		setBL(0x0201BC08, (u32)dsiSaveClose);
		// Display red blank battery icon
		*(u32*)0x02029A9C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02029AB0 = 0xE3A00001; // mov r0, #1
	}

	// Gareuchyeojwodalling (Korea)
	// ENG banner text: Tell me Darling
	else if (strcmp(romTid, "KOSK") == 0) {
		*(u32*)0x0200B40C = 0xE1A00000; // nop
		tonccpy((u32*)0x0200BF90, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0200E940 = 0xE1A00000; // nop
		patchInitDSiWare(0x02014210, heapEnd);
		patchUserSettingsReadDSiWare(0x020157F8);
		*(u32*)0x020193F0 = 0xE1A00000; // nop
		setBL(0x0201BB60, (u32)dsiSaveCreate);
		setBL(0x0201BBA0, (u32)dsiSaveOpen);
		setBL(0x0201BBD0, (u32)dsiSaveSetLength);
		setBL(0x0201BBF4, (u32)dsiSaveWrite);
		setBL(0x0201BC24, (u32)dsiSaveClose);
		setBL(0x0201BC70, (u32)dsiSaveOpen);
		setBL(0x0201BC98, (u32)dsiSaveGetLength);
		setBL(0x0201BCAC, (u32)dsiSaveRead);
		setBL(0x0201BCDC, (u32)dsiSaveClose);
		// Display red blank battery icon
		*(u32*)0x02029F3C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02029F50 = 0xE3A00001; // mov r0, #1
	}

	// Tetris Party Live (USA)
	// Tetris Party Live (Europe, Australia)
	else if (strcmp(romTid, "KTEE") == 0 || strcmp(romTid, "KTEV") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02005168 = 0xE1A00000; // nop
		*(u32*)0x02005170 = 0xE1A00000; // nop
		*(u32*)0x02005180 = 0xE1A00000; // nop
		*(u32*)0x020052C0 = 0xE1A00000; // nop
		*(u32*)0x02014828 = 0xE1A00000; // nop
		*(u32*)0x02017C50 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201E234, heapEnd);
		patchUserSettingsReadDSiWare(0x0201F82C);
		*(u32*)0x0201F848 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F84C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F854 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F858 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F878 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F87C = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F88C = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201F890 = 0xE12FFF1E; // bx lr
		*(u32*)0x0201F89C = 0xE3A00000; // mov r0, #0
		*(u32*)0x0201F8A0 = 0xE12FFF1E; // bx lr
		// if (!useSharedFont) {
			*(u32*)0x02054C30 = 0xE1A00000; // nop (Skip Manual screen)
		// }
		if (romTid[3] == 'E') {
			/* if (useSharedFont && !extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02059574, 0x0201FDD0);
			} */
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
			*(u32*)0x0205AAEC = 0xE1A00000; // nop
			*(u32*)0x0205B330 = 0xE1A00000; // nop
			*(u32*)0x0205B39C = 0xE3A00000; // mov r0, #0
		} else {
			/* if (useSharedFont && !extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02059560, 0x0201FDD0);
			} */
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
			*(u32*)0x0205AAD8 = 0xE1A00000; // nop
			*(u32*)0x0205B31C = 0xE1A00000; // nop
			*(u32*)0x0205B388 = 0xE3A00000; // mov r0, #0
		}
		*(u32*)0x0207AD18 = 0xE1A00000; // nop
		*(u32*)0x0207AD20 = 0xE3A00001; // mov r0, #1
		setB(0x0207BCD0, 0x0207BDA0);
		*(u32*)0x0207BDA0 = 0xE1A00000; // nop
		*(u32*)0x0207BDA4 = 0xE1A00000; // nop
		*(u32*)0x0207BDAC = 0xE1A00000; // nop
		*(u32*)0x0207BDB0 = 0xE1A00000; // nop
		*(u32*)0x0207BDB4 = 0xE1A00000; // nop
		*(u32*)0x0207BDB8 = 0xE1A00000; // nop
		setB(0x0207C52C, 0x0207C5D0);
		*(u32*)0x0207C734 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207C738 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207C790 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0207C794 = 0xE12FFF1E; // bx lr
		*(u32*)0x0207C850 = 0xE1A00000; // nop
		*(u32*)0x0207C854 = 0xE1A00000; // nop
		setB(0x0207D4B8, 0x0207D65C);
		*(u32*)0x0207DCC8 = 0xE1A00000; // nop
		*(u32*)0x0207DCCC = 0xE1A00000; // nop
		*(u32*)0x0207DCD0 = 0xE1A00000; // nop
		*(u32*)0x0207DCD4 = 0xE3A01001; // mov r1, #1
		setB(0x0207F01C, 0x0207F038);
		setB(0x0207F27C, 0x0207F2A4);
		*(u32*)0x0207F2A4 += 0xB0000000; // movcc r0, #0x240 -> mov r0, #0x240
		*(u32*)0x0207F2A8 += 0xB0000000; // strcc r0, [r5,#0x2C] -> str r0, [r5,#0x2C]
		*(u32*)0x0207F2AC = 0xE3A00000; // mov r0, #0
		*(u32*)0x0207F2B0 = 0xE5850030; // str r0, [r5,#0x30]
		*(u32*)0x0207F2B4 = 0xE8BD8078; // LDMFD SP!, {R4-R6,PC}
		*(u32*)0x0209A674 = 0xE3A02C07; // mov r2, #0x700
		*(u32*)0x0209A694 = 0xE2840B01; // add r0, r4, #0x400
		*(u32*)0x0209A69C = 0xE1A00004; // mov r0, r4
		*(u32*)0x0209A6A4 = 0xE1A00000; // nop
		*(u32*)0x0209A6A8 = 0xE1A00000; // nop
		*(u32*)0x0209A6AC = 0xE1A00000; // nop
		*(u32*)0x0209A6B0 = 0xE1A00000; // nop
		*(u32*)0x0209A6B4 = 0xE1A00000; // nop
		*(u32*)0x0209A6C8 = 0xE2841B01; // add r1, r4, #0x400
		*(u32*)0x020A3200 = 0xE3A00000; // mov r0, #0
		setBL(0x020A32DC, 0x020A4830);
		setBL(0x020A338C, 0x020A4968);
		setBL(0x020A3440, 0x020A49D4);
		setBL(0x020A36AC, 0x020A4ADC);
		setBL(0x020A3784, 0x020A4B88);
		setBL(0x020A38B8, 0x020A4BF4);
		setBL(0x020A39E4, 0x020A4D94);
		*(u32*)0x020A3D94 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020A3DC4 = 0xE3A00000; // mov r0, #0
		setBL(0x020A451C, 0x020A4CDC);
	}

	// Thorium Wars (USA)
	else if (strcmp(romTid, "KTWE") == 0) {
		if (!extendedMemory2) {
			// Disable audio
			u32 bssEnd = *(u32*)0x020013C0;
			u32 sdatOffset = 0x0212BE54;

			*(u32*)0x020013C0 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x1FC000, bssEnd, sdatOffset);
			*(u32*)0x02082868 = 0xE3A01901; // mov r1, #0x4000
		}

		*(u32*)0x0200C9EC = 0xE1A00000; // nop
		tonccpy((u32*)0x0200D680, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201041C = 0xE1A00000; // nop
		patchInitDSiWare(0x02017BD4, heapEnd);
		*(u32*)0x02017F44 = *(u32*)0x020013C0;
		patchUserSettingsReadDSiWare(0x02019370);
		*(u32*)0x0201ED30 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0201ED34 = 0xE12FFF1E; // bx lr
		if (!extendedMemory2) {
			// Disable MobiClip playback
			*(u32*)0x02032BFC = 0xE3A00000; // mov r0, #0
			*(u32*)0x02041698 = 0xE12FFF1E; // bx lr
			setBL(0x020856B8, 0x02041724); // Display non-video title logo on start
		}
		setBL(0x0207C824, (u32)dsiSaveCreate);
		setBL(0x0207C840, (u32)dsiSaveOpen);
		setBL(0x0207C854, (u32)dsiSaveSetLength);
		setBL(0x0207C880, (u32)dsiSaveClose);
		setBL(0x0207C92C, (u32)dsiSaveDelete);
		*(u32*)0x0207C974 = 0xE1A00000; // nop
		setBL(0x0207C9D8, (u32)dsiSaveOpen);
		setBL(0x0207C9F0, (u32)dsiSaveSeek);
		setBL(0x0207CA00, (u32)dsiSaveRead);
		setBL(0x0207CA10, (u32)dsiSaveClose);
		setBL(0x0207CAF4, (u32)dsiSaveOpen);
		setBL(0x0207CB0C, (u32)dsiSaveSeek);
		setBL(0x0207CB1C, (u32)dsiSaveWrite);
		setBL(0x0207CB2C, (u32)dsiSaveClose);
		*(u32*)0x02085678 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Thorium Wars (Europe)
	else if (strcmp(romTid, "KTWP") == 0) {
		if (!extendedMemory2) {
			// Disable audio
			u32 bssEnd = *(u32*)0x02004FD0;
			u32 sdatOffset = 0x0213F040;

			*(u32*)0x02004FD0 = bssEnd - relocateBssPart(ndsHeader, bssEnd, sdatOffset+0x1FC000, bssEnd, sdatOffset);
			*(u32*)0x0208C300 = 0xE3A01901; // mov r1, #0x4000
		}

		*(u32*)0x02010650 = 0xE1A00000; // nop
		tonccpy((u32*)0x020111C8, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02013F28 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B050, heapEnd);
		*(u32*)0x0201B3DC = *(u32*)0x02004FD0;
		patchUserSettingsReadDSiWare(0x0201C820);
		*(u32*)0x020220C0 = 0xE3A00001; // mov r0, #1
		*(u32*)0x020220C4 = 0xE12FFF1E; // bx lr
		if (!extendedMemory2) {
			// Disable MobiClip playback
			*(u32*)0x02036C70 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02045720 = 0xE12FFF1E; // bx lr
			setBL(0x0208F144, 0x020457AC); // Display non-video title logo on start
		}
		setBL(0x02084A78, (u32)dsiSaveCreate);
		setBL(0x02084A94, (u32)dsiSaveOpen);
		setBL(0x02084AA8, (u32)dsiSaveSetLength);
		setBL(0x02084AD4, (u32)dsiSaveClose);
		setBL(0x02084B80, (u32)dsiSaveDelete);
		*(u32*)0x02084BC8 = 0xE1A00000; // nop
		setBL(0x02084C2C, (u32)dsiSaveOpen);
		setBL(0x02084C44, (u32)dsiSaveSeek);
		setBL(0x02084C54, (u32)dsiSaveRead);
		setBL(0x02084C64, (u32)dsiSaveClose);
		setBL(0x02084D48, (u32)dsiSaveOpen);
		setBL(0x02084D60, (u32)dsiSaveSeek);
		setBL(0x02084D70, (u32)dsiSaveWrite);
		setBL(0x02084D80, (u32)dsiSaveClose);
		*(u32*)0x0208F104 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
	}

	// Topoloco (USA)
	// Topoloco (Europe)
	// Requires 8MB of RAM
	/*else if (strncmp(romTid, "KT5", 3) == 0 && extendedMemory2) {
		*(u32*)0x02014F78 = 0xE1A00000; // nop
		*(u32*)0x02018808 = 0xE1A00000; // nop
		*(u32*)0x0201C4AC = 0xE1A00000; // nop
		*(u32*)0x0201E334 = 0xE1A00000; // nop
		*(u32*)0x0201E338 = 0xE1A00000; // nop
		*(u32*)0x0201E344 = 0xE1A00000; // nop
		*(u32*)0x0201E4A4 = 0xE1A00000; // nop
		patchHiHeapDSiWare(0x0201E500, heapEnd); // mov r0, #0x2700000
		patchUserSettingsReadDSiWare(0x0201F8A4);
		*(u32*)0x020240D4 = 0xE1A00000; // nop
		*(u32*)0x020241D8 = 0xE1A00000; // nop
		*(u32*)0x02024200 = 0xE1A00000; // nop
		*(u32*)0x020516A4 = 0xE1A00000; // nop
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

	// Touch Solitaire (USA)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KSLE") == 0) {
		// if (!extendedMemory2) {
			*(u16*)0x0200D6D8 = 0x054C; // lsls r4, r1, #0x15
		// }
		*(u16*)0x0200D6F4 = 0x0424; // lsls r4, r4, #0x10
		doubleNopT(0x0200D78A);
		doubleNopT(0x0200D90A);
		doubleNopT(0x0200D916);
		doubleNopT(0x0200DA26);
		doubleNopT(0x0200E15A); // Hide volume icon
		doubleNopT(0x0201AD4A);
		doubleNopT(0x0201D28A);
		doubleNopT(0x02020F94);
		doubleNopT(0x02022552);
		doubleNopT(0x02022556);
		doubleNopT(0x02022562);
		doubleNopT(0x02022646);
		patchHiHeapDSiWareThumb(0x02022684, 0x0201FC7C, heapEnd);
		*(u32*)0x0202275C = 0x02059840;
		patchUserSettingsReadDSiWare(0x020233DE);
		doubleNopT(0x020236CC);
		*(u16*)0x020236D0 = nopT;
		*(u16*)0x020236D2 = nopT;
		doubleNopT(0x020236D4);
		*(u16*)0x02025690 = 0x2001; // mov r0, #1
		*(u16*)0x02025692 = 0x4770; // bx lr
		*(u16*)0x02025698 = 0x2000; // mov r0, #0
		*(u16*)0x0202569A = 0x4770; // bx lr
	}

	// 2-in-1 Solitaire (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KSLV") == 0) {
		// if (!extendedMemory2) {
			*(u16*)0x0200DC74 = 0x054C; // lsls r4, r1, #0x15
		// }
		*(u16*)0x0200DC90 = 0x0424; // lsls r4, r4, #0x10
		doubleNopT(0x0200DD26);
		doubleNopT(0x0200DEA6);
		doubleNopT(0x0200DEB2);
		doubleNopT(0x0200DFC2);
		doubleNopT(0x0200E6F6); // Hide volume icon
		doubleNopT(0x0201B62E);
		doubleNopT(0x0201DBEE);
		doubleNopT(0x02021958);
		*(u32*)0x02022ED6 = 0x2001; // mov r0, #1
		*(u16*)0x02022ED8 = nopT;
		doubleNopT(0x02022F4E);
		doubleNopT(0x02022F52);
		doubleNopT(0x02022F5E);
		doubleNopT(0x02023042);
		patchHiHeapDSiWareThumb(0x02023080, 0x0202063C, heapEnd);
		*(u32*)0x02023158 = 0x020592C0;
		patchUserSettingsReadDSiWare(0x02023E04);
		doubleNopT(0x020240FA);
		*(u16*)0x020240FE = nopT;
		*(u16*)0x02024100 = nopT;
		doubleNopT(0x02024102);
		*(u16*)0x02026190 = 0x2001; // mov r0, #1
		*(u16*)0x02026192 = 0x4770; // bx lr
		*(u16*)0x02026198 = 0x2000; // mov r0, #0
		*(u16*)0x0202619A = 0x4770; // bx lr
	}

	// Solitaire DSi (Japan)
	// Saving not supported due to using more than one file in filesystem
	// Does not boot due to save patch not implemented
	/* else if (strcmp(romTid, "KSLJ") == 0) {
		// if (!extendedMemory2) {
			*(u16*)0x0200D4F8 = 0x054C; // lsls r4, r1, #0x15
		// }
		*(u16*)0x0200D514 = 0x0424; // lsls r4, r4, #0x10
		doubleNopT(0x0200D5AA);
		doubleNopT(0x0200DF6A); // Hide volume icon
		*(u16*)0x0201AE60 = 0xB003; // ADD SP, SP, #0xC
		*(u16*)0x0201AE62 = 0xBD78; // POP {R3-R6,PC}
		doubleNopT(0x0201D474);
		doubleNopT(0x02022268);
		doubleNopT(0x0202375E);
		doubleNopT(0x02023762);
		doubleNopT(0x0202376E);
		doubleNopT(0x0202385A);
		patchHiHeapDSiWareThumb(0x02023898, 0x020206A4, heapEnd);
		*(u32*)0x02023970 = 0x02058280;
		patchUserSettingsReadDSiWare(0x0202461C);
		doubleNopT(0x02024912);
		*(u16*)0x02024916 = nopT;
		*(u16*)0x02024918 = nopT;
		doubleNopT(0x0202491A);
	} */

	// The Tower DS: Classic (Japan)
	// The Tower DS: Hotel (Japan)
	// The Tower DS: Shopping Santa (Japan)
	else if (strcmp(romTid, "KWWJ") == 0 || strcmp(romTid, "KWVJ") == 0 || strcmp(romTid, "KW4J") == 0) {
		u8 offsetChange = (romTid[2] == 'W') ? 0 : 0x48;
		u8 offsetChangeS = (romTid[2] == 'W') ? 0 : 0x60;

		if (romTid[2] == 'W') {
			*(u32*)0x02001618 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		} else {
			*(u32*)0x020014A0 = 0xE1A00000; // nop
			*(u32*)0x0200163C = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)(0x0200B734+offsetChange) = 0xE1A00000; // nop
		tonccpy((u32*)(0x0200C3C8+offsetChange), dsiSaveGetResultCode, 0xC);
		*(u32*)(0x0200F1D4+offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020154AC+offsetChange) = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020154C4+offsetChange, heapEnd);
		patchUserSettingsReadDSiWare(0x02016AF4+offsetChange);
		setBL(0x0201C5B4+offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x0201C5E4+offsetChangeS, (u32)dsiSaveGetPosition);
		setBL(0x0201C5F8+offsetChangeS, (u32)dsiSaveWrite);
		setBL(0x0201C608+offsetChangeS, (u32)dsiSaveClose);
		setBL(0x0201C680+offsetChangeS, (u32)dsiSaveOpen);
		setBL(0x0201C6AC+offsetChangeS, (u32)dsiSaveGetLength);
		setBL(0x0201C6CC+offsetChangeS, (u32)dsiSaveClose);
		setBL(0x0201C704+offsetChangeS, (u32)dsiSaveSeek);
		setBL(0x0201C714+offsetChangeS, (u32)dsiSaveRead);
		setBL(0x0201C72C+offsetChangeS, (u32)dsiSaveClose);
		setBL(0x0201C7C0+offsetChangeS, (u32)dsiSaveCreate);
	}

	// Trajectile (USA)
	// Reflect Missile (Europe, Australia)
	// Requires 8MB of RAM
	else if ((strcmp(romTid, "KDZE") == 0 || strcmp(romTid, "KDZV") == 0) && extendedMemory2) {
		*(u32*)0x0204086C = 0xE1A00000; // nop
		*(u32*)0x020408B8 = 0xE1A00000; // nop
		*(u32*)0x020408BC = 0xE1A00000; // nop
		doubleNopT(0x02040D24);
		doubleNopT(0x020B4B40);
		doubleNopT(0x020B9216);
		toncset16((u16*)0x020B9298, nopT, 0x4A/sizeof(u16)); // Do not use DSi WRAM
		*(u32*)0x020C7510 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020C7528, 0x027B0000);
		*(u32*)0x020C7898 -= 0x30000;
		patchUserSettingsReadDSiWare(0x020C8DDC);
		*(u32*)0x020CC3DC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020CC3E0 = 0xE12FFF1E; // bx lr
		doubleNopT(0x020CF4BA);
		doubleNopT(0x020D515E);
		doubleNopT(0x020DA0E6);
		doubleNopT(0x020DA10A);
		doubleNopT(0x020DA122);
		doubleNopT(0x020DA142);
		doubleNopT(0x020DA15E);
		doubleNopT(0x020DA17E);
		doubleNopT(0x020DA19A);
		doubleNopT(0x020DA1BA);
		doubleNopT(0x020DA1DA);
		doubleNopT(0x020DA1F6);
		doubleNopT(0x020DA21E);
		*(u32*)0x02104154 = 0x1E0000; // Shrink sound heap from 0x300000
	}

	// Reflect Missile (Japan)
	// Requires 8MB of RAM
	else if (strcmp(romTid, "KDZJ") == 0 && extendedMemory2) {
		*(u32*)0x0204055C = 0xE1A00000; // nop
		*(u32*)0x020405A8 = 0xE1A00000; // nop
		*(u32*)0x020405AC = 0xE1A00000; // nop
		doubleNopT(0x02040A14);
		doubleNopT(0x020B4830);
		doubleNopT(0x020B8F06);
		toncset16((u16*)0x020B8F88, nopT, 0x4A/sizeof(u16)); // Do not use DSi WRAM
		*(u32*)0x020C7200 = 0xE3A00001; // mov r0, #1
		patchInitDSiWare(0x020C7218, 0x027B0000);
		*(u32*)0x020C7588 -= 0x30000;
		patchUserSettingsReadDSiWare(0x020C8ACC);
		*(u32*)0x020CC0CC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020CC0D0 = 0xE12FFF1E; // bx lr
		doubleNopT(0x020CF1AA);
		doubleNopT(0x020D4E4E);
		doubleNopT(0x020D9DD6);
		doubleNopT(0x020D9DFA);
		doubleNopT(0x020D9E12);
		doubleNopT(0x020D9E32);
		doubleNopT(0x020D9E4E);
		doubleNopT(0x020D9E6E);
		doubleNopT(0x020D9E8A);
		doubleNopT(0x020D9EAA);
		doubleNopT(0x020D9ECA);
		doubleNopT(0x020D9EE6);
		doubleNopT(0x020D9F0E);
		*(u32*)0x02103DF8 = 0x1E0000; // Shrink sound heap from 0x300000
	}

	// Trollboarder (USA)
	// Trollboarder (Europe)
	else if (strcmp(romTid, "KB7E") == 0 || strcmp(romTid, "KB7P") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200FA58 = 0xE1A00000; // nop
		*(u32*)0x02013164 = 0xE1A00000; // nop
		patchInitDSiWare(0x020191E8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201A6D8);
		*(u32*)0x0201F444 = 0xE3A00000; // mov r0, #0
		setBL(0x0201F4A8, (u32)dsiSaveOpen);
		*(u32*)0x0201F4E0 = 0xE1A00000; // nop
		setBL(0x0201F4FC, (u32)dsiSaveGetLength);
		setBL(0x0201F50C, (u32)dsiSaveRead);
		setBL(0x0201F514, (u32)dsiSaveClose);
		setBL(0x0201F550, (u32)dsiSaveOpen);
		*(u32*)0x0201F568 = 0xE3A00001; // mov r0, #1 (dsiSaveGetArcSrc)
		*(u32*)0x0201F578 = 0xE3A00001; // mov r0, #1 (dsiSaveFreeSpaceAvailable)
		setBL(0x0201F594, (u32)dsiSaveCreate);
		setBL(0x0201F5A0, (u32)dsiSaveClose);
		setBL(0x0201F5B0, (u32)dsiSaveOpen);
		setBL(0x0201F5C0, (u32)dsiSaveGetResultCode);
		*(u32*)0x0201F5E4 = 0xE1A00000; // nop
		setBL(0x0201F604, (u32)dsiSaveSetLength);
		setBL(0x0201F614, (u32)dsiSaveWrite);
		setBL(0x0201F61C, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)((romTid[3] == 'E') ? 0x02029070 : 0x02029090) = 0xE1A00000; // nop (Disable audio)
		}
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad((romTid[3] == 'E') ? 0x02046994 : 0x020469B0, 0x0201AC1C);
		}
	}

	// True Swing Golf Express (USA)
	// A Little Bit of... Nintendo Touch Golf (Europe, Australia)
	else if (strcmp(romTid, "K72E") == 0 || strcmp(romTid, "K72V") == 0) {
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
		if (romTid[3] == 'E') {
			if (extendedMemory2) {
				*(u32*)0x0202BBD4 = 0xE3A0581E; // mov r5, #0x1E0000 (Shrink sound heap from 0x300000)
			} else {
				*(u32*)0x0202BBD4 = 0xE3A05901; // mov r5, #0x4000 (Disable audio)
			}
			*(u32*)0x02063214 = 0xE1A00000; // nop
			*(u32*)0x02066E74 = 0xE1A00000; // nop
			patchInitDSiWare(0x02074948, heapEnd);
			*(u32*)0x02074CD4 -= 0x2F0000;
			patchUserSettingsReadDSiWare(0x020760C8);
			*(u32*)0x020760E4 = 0xE3A00001; // mov r0, #1
			*(u32*)0x020760E8 = 0xE12FFF1E; // bx lr
			*(u32*)0x020760F0 = 0xE3A00000; // mov r0, #0
			*(u32*)0x020760F4 = 0xE12FFF1E; // bx lr
		} else {
			if (extendedMemory2) {
				*(u32*)0x0202BA40 = 0xE3A0581E; // mov r5, #0x1E0000 (Shrink sound heap from 0x300000)
			} else {
				*(u32*)0x0202BA40 = 0xE3A05901; // mov r5, #0x4000 (Disable audio)
			}
			*(u32*)0x02062FA8 = 0xE1A00000; // nop
			*(u32*)0x02066C08 = 0xE1A00000; // nop
			patchInitDSiWare(0x020746DC, heapEnd);
			*(u32*)0x02074A68 -= 0x2F0000;
			patchUserSettingsReadDSiWare(0x02075E5C);
			*(u32*)0x02075E78 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02075E7C = 0xE12FFF1E; // bx lr
			*(u32*)0x02075E84 = 0xE3A00000; // mov r0, #0
			*(u32*)0x02075E88 = 0xE12FFF1E; // bx lr
		}
	}

	// Turn: The Lost Artifact (USA)
	// Saving is difficult to implement
	else if (strcmp(romTid, "KTIE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x020051F4 = 0xE1A00000; // nop
		*(u32*)0x020128A8 = 0xE1A00000; // nop
		*(u32*)0x02015B98 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201C0D8, heapEnd);
		patchUserSettingsReadDSiWare(0x0201D754);
		*(u32*)0x020227BC = 0xE3A00001; // mov r0, #1
		*(u32*)0x020227C0 = 0xE12FFF1E; // bx lr
		*(u32*)0x0202BE5C = 0xE1A00000; // nop
		*(u32*)0x0202BE64 = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02049E68, 0x0201DCC0);
			*(u32*)0x02049EB0 = 0xE3A04601; // mov r4, #0x100000
		}
	}

	// Ubongo (USA)
	else if (strcmp(romTid, "KUBE") == 0) {
		*(u32*)0x02012A24 = 0xE1A00000; // nop
		*(u32*)0x02012A94 = 0xE1A00000; // nop
		*(u32*)0x02012AB0 = 0xE1A00000; // nop
		*(u32*)0x02012AB8 = 0xE1A00000; // nop
		*(u32*)0x02013278 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02013294 = 0xE1A00000; // nop
		*(u32*)0x020132C8 = 0xE1A00000; // nop
		*(u32*)0x020132D0 = 0xE1A00000; // nop
		*(u32*)0x02013344 = 0xE1A00000; // nop
		*(u32*)0x0201334C = 0xE1A00000; // nop
		*(u32*)0x020133D0 = 0xE1A00000; // nop
		*(u32*)0x020133D8 = 0xE1A00000; // nop
		setBL(0x02021498, (u32)dsiSaveOpen);
		setBL(0x020214B0, (u32)dsiSaveGetLength);
		setBL(0x020214C0, (u32)dsiSaveSeek);
		setBL(0x020214D0, (u32)dsiSaveWrite);
		setBL(0x020214D8, (u32)dsiSaveClose);
		setBL(0x02021548, (u32)dsiSaveOpen);
		setBL(0x02021560, (u32)dsiSaveGetLength);
		setBL(0x02021574, (u32)dsiSaveSeek);
		setBL(0x02021584, (u32)dsiSaveRead);
		setBL(0x0202158C, (u32)dsiSaveClose);
		setBL(0x02021604, (u32)dsiSaveCreate);
		setBL(0x02021630, (u32)dsiSaveOpen);
		setBL(0x0202166C, (u32)dsiSaveWrite);
		setBL(0x0202167C, (u32)dsiSaveClose);
		doubleNopT(0x020216B2);
		doubleNopT(0x020216D6);
		doubleNopT(0x020216EE);
		doubleNopT(0x0202170E);
		doubleNopT(0x0202172A);
		doubleNopT(0x0202174A);
		doubleNopT(0x02021766);
		doubleNopT(0x0202178E);
		*(u32*)0x0203D054 = 0xE1A00000; // nop
		*(u32*)0x02040E9C = 0xE1A00000; // nop
		*(u32*)0x0204F538 = 0xE1A00000; // nop
		*(u32*)0x0204F578 = 0xE1A00000; // nop
		patchInitDSiWare(0x02061D98, heapEnd);
		*(u32*)0x02064998 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0206499C = 0xE12FFF1E; // bx lr
		*(u32*)0x020649AC = 0xE3A00000; // mov r0, #0
		*(u32*)0x020649B0 = 0xE12FFF1E; // bx lr
	}

	// Ubongo (Europe)
	else if (strcmp(romTid, "KUBP") == 0) {
		*(u32*)0x02012A24 = 0xE1A00000; // nop
		*(u32*)0x02012A94 = 0xE1A00000; // nop
		*(u32*)0x02012AB0 = 0xE1A00000; // nop
		*(u32*)0x02012AB8 = 0xE1A00000; // nop
		*(u32*)0x020134EC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02013508 = 0xE1A00000; // nop
		*(u32*)0x0201353C = 0xE1A00000; // nop
		*(u32*)0x02013544 = 0xE1A00000; // nop
		*(u32*)0x020135B8 = 0xE1A00000; // nop
		*(u32*)0x020135C0 = 0xE1A00000; // nop
		*(u32*)0x02013644 = 0xE1A00000; // nop
		*(u32*)0x0201364C = 0xE1A00000; // nop
		setBL(0x02021758, (u32)dsiSaveOpen);
		setBL(0x02021770, (u32)dsiSaveGetLength);
		setBL(0x02021780, (u32)dsiSaveSeek);
		setBL(0x02021790, (u32)dsiSaveWrite);
		setBL(0x02021798, (u32)dsiSaveClose);
		setBL(0x02021808, (u32)dsiSaveOpen);
		setBL(0x02021820, (u32)dsiSaveGetLength);
		setBL(0x02021834, (u32)dsiSaveSeek);
		setBL(0x02021844, (u32)dsiSaveRead);
		setBL(0x0202184C, (u32)dsiSaveClose);
		setBL(0x020218C4, (u32)dsiSaveCreate);
		setBL(0x020218F0, (u32)dsiSaveOpen);
		setBL(0x0202192C, (u32)dsiSaveWrite);
		setBL(0x0202193C, (u32)dsiSaveClose);
		doubleNopT(0x02021972);
		doubleNopT(0x02021996);
		doubleNopT(0x020219AE);
		doubleNopT(0x020219CE);
		doubleNopT(0x020219EA);
		doubleNopT(0x02021A0A);
		doubleNopT(0x02021A26);
		doubleNopT(0x02021A4E);
		*(u32*)0x0203D314 = 0xE1A00000; // nop
		*(u32*)0x0204115C = 0xE1A00000; // nop
		*(u32*)0x0204F7F8 = 0xE1A00000; // nop
		*(u32*)0x0204F838 = 0xE1A00000; // nop
		patchInitDSiWare(0x02062058, heapEnd);
		*(u32*)0x02064C58 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02064C5C = 0xE12FFF1E; // bx lr
		*(u32*)0x02064C6C = 0xE3A00000; // mov r0, #0
		*(u32*)0x02064C70 = 0xE12FFF1E; // bx lr
	}

	// Uchi Makure!: Touch the Chameleon (Japan)
	else if (strcmp(romTid, "KKMJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200E5F8 = 0xE1A00000; // nop
		*(u32*)0x02012258 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201A410, heapEnd);
		patchUserSettingsReadDSiWare(0x0201BAE8);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0203AAD0, 0x0201C068);
		}
		setBL(0x0203FB98, (u32)dsiSaveOpen);
		setBL(0x0203FBB0, (u32)dsiSaveGetLength);
		setBL(0x0203FBDC, (u32)dsiSaveRead);
		setBL(0x0203FBE4, (u32)dsiSaveClose);
		setBL(0x0203FC20, (u32)dsiSaveCreate);
		setBL(0x0203FC30, (u32)dsiSaveOpen);
		setBL(0x0203FC40, (u32)dsiSaveGetResultCode);
		*(u32*)0x0203FC4C = 0xE1A00000; // nop
		setBL(0x0203FC64, (u32)dsiSaveSetLength);
		setBL(0x0203FC74, (u32)dsiSaveWrite);
		setBL(0x0203FC7C, (u32)dsiSaveClose);
		*(u32*)0x0203FD08 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x0203FD20 = 0xE1A00000; // nop (dsiSaveCloseDir)
	}

	// Unou to Sanougaren Sasuru: Uranoura (Japan)
	// Unable to save data
	else if (strcmp(romTid, "K6PJ") == 0) {
		*(u32*)0x02006E84 = 0xE12FFF1E; // bx lr (Skip NFTR font rendering)
		*(u32*)0x020092D4 = 0xE3A00000; // mov r0, #0 (Disable NFTR loading from TWLNAND)
		for (int i = 0; i < 11; i++) { // Skip Manual screen
			u32* offset = (u32*)0x0200A608;
			offset[i] = 0xE1A00000; // nop
		}
		/* setBL(0x02020B50, (u32)dsiSaveOpen);
		*(u32*)0x02020B88 = 0xE1A00000; // nop
		setBL(0x02020B94, (u32)dsiSaveGetLength);
		setBL(0x02020BB4, (u32)dsiSaveRead);
		*(u32*)0x02020BDC = 0xE1A00000; // nop
		setBL(0x02020BE4, (u32)dsiSaveClose);
		*(u32*)0x02020C40 = 0xE1A00000; // nop
		*(u32*)0x02020C50 = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02020C7C = 0xE1A00000; // nop
		*(u32*)0x02020C98 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x02020CE0 = 0xE1A00000; // nop
		*(u32*)0x02020CEC = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020D00 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020D94 = 0xE1A00000; // nop
		*(u32*)0x02020D9C = 0xE3A00001; // mov r0, #1 (dsiSaveCloseDir)
		*(u32*)0x02020DA0 = 0xE1A00000; // nop
		*(u32*)0x02020DEC = 0xE3A00001; // mov r0, #1 (dsiSaveOpenDir)
		*(u32*)0x02020E18 = 0xE1A00000; // nop
		*(u32*)0x02020E30 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x02020E78 = 0xE1A00000; // nop
		*(u32*)0x02020E84 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02020E98 = 0xE3A00001; // mov r0, #1
		setBL(0x02020ED4, (u32)dsiSaveDelete);
		*(u32*)0x02020EFC = 0xE1A00000; // nop
		*(u32*)0x02020F08 = 0xE3A00001; // mov r0, #1 (dsiSaveReadDir)
		*(u32*)0x02020F18 = 0xE1A00000; // nop (dsiSaveCloseDir)
		setBL(0x02020F58, (u32)dsiSaveCreate);
		*(u32*)0x02020F80 = 0xE1A00000; // nop
		setBL(0x02020F90, (u32)dsiSaveOpen);
		*(u32*)0x02020FBC = 0xE1A00000; // nop
		setBL(0x02020FE8, (u32)dsiSaveSetLength);
		setBL(0x02020FF8, (u32)dsiSaveWrite);
		*(u32*)0x02021020 = 0xE1A00000; // nop
		setBL(0x02021028, (u32)dsiSaveClose); */
		*(u32*)0x02038BE0 = 0xE1A00000; // nop
		// tonccpy((u32*)0x02039874, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0203CC30 = 0xE1A00000; // nop
		patchInitDSiWare(0x0204489C, heapEnd);
		patchUserSettingsReadDSiWare(0x02045D78);
	}

	// Viking Invasion (USA)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KVKE") == 0) {
		*(u32*)0x02011ECC = 0xE1A00000; // nop
		*(u32*)0x0201547C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B434, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C9D4);
		*(u32*)0x02035F64 = 0xE1A00000; // nop
		*(u32*)0x02036DFC = 0xE12FFF1E; // bx lr
		*(u32*)0x0206695C = 0xE12FFF1E; // bx lr
		*(u32*)0x02066D3C = 0xE12FFF1E; // bx lr
		*(u32*)0x02066F50 = 0xE1A00000; // nop
	}

	// Viking Invasion (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KVKV") == 0) {
		*(u32*)0x02011EB0 = 0xE1A00000; // nop
		*(u32*)0x02015460 = 0xE1A00000; // nop
		patchInitDSiWare(0x0201B418, heapEnd);
		patchUserSettingsReadDSiWare(0x0201C9B8);
		*(u32*)0x02035ECC = 0xE1A00000; // nop
		*(u32*)0x02036CD4 = 0xE12FFF1E; // bx lr
		*(u32*)0x020668DC = 0xE12FFF1E; // bx lr
		*(u32*)0x02066CBC = 0xE12FFF1E; // bx lr
		*(u32*)0x02066ED0 = 0xE1A00000; // nop
	}

	// VT Tennis (USA)
	else if (strcmp(romTid, "KVTE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200509C = 0xE1A00000; // nop
		*(u32*)0x020058EC = 0xE1A00000; // nop
		*(u32*)0x0201AA9C = 0xE1A00000; // nop
		tonccpy((u32*)0x0201B634, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201E184 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202657C, heapEnd);
		patchUserSettingsReadDSiWare(0x02027A48);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0209AB60, 0x02027F8C);
				if (expansionPakFound) {
					*(u32*)0x0209ABAC = 0xE1A00000; // nop
				}
			}
		} else {
			*(u32*)0x0205C000 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		}
		setBL(0x0209C1B0, (u32)dsiSaveCreate);
		setBL(0x0209C1C0, (u32)dsiSaveOpen);
		setBL(0x0209C1E8, (u32)dsiSaveSetLength);
		setBL(0x0209C1F8, (u32)dsiSaveWrite);
		setBL(0x0209C200, (u32)dsiSaveClose);
		*(u32*)0x0209C21C = 0xE1A00000; // nop
		setBL(0x0209C284, (u32)dsiSaveOpen);
		setBL(0x0209C2B0, (u32)dsiSaveRead);
		setBL(0x0209C2BC, (u32)dsiSaveClose);
	}

	// VT Tennis (Europe, Australia)
	else if (strcmp(romTid, "KVTV") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x02005084 = 0xE1A00000; // nop
		*(u32*)0x020057D0 = 0xE1A00000; // nop
		*(u32*)0x0201A168 = 0xE1A00000; // nop
		tonccpy((u32*)0x0201AD00, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201D850 = 0xE1A00000; // nop
		patchInitDSiWare(0x02025C48, heapEnd);
		patchUserSettingsReadDSiWare(0x02027114);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x0209AB60, 0x02027F8C);
				if (expansionPakFound) {
					*(u32*)0x02099B1C = 0xE1A00000; // nop
				}
			}
		} else {
			*(u32*)0x0205B314 = 0xE3A00000; // mov r0, #0 (Skip Manual screen)
		}
		setBL(0x0209B120, (u32)dsiSaveCreate);
		setBL(0x0209B130, (u32)dsiSaveOpen);
		setBL(0x0209B158, (u32)dsiSaveSetLength);
		setBL(0x0209B168, (u32)dsiSaveWrite);
		setBL(0x0209B170, (u32)dsiSaveClose);
		*(u32*)0x0209B18C = 0xE1A00000; // nop
		setBL(0x0209B1F4, (u32)dsiSaveOpen);
		setBL(0x0209B220, (u32)dsiSaveRead);
		setBL(0x0209B22C, (u32)dsiSaveClose);
	}

	// Wakugumi: Monochrome Puzzle (Europe, Australia)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KK4V") == 0) {
		// useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x02005A38 = 0xE1A00000; // nop
		*(u32*)0x0204F240 = 0xE3A00000; // mov r0, #0
		/* if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02050020, 0x02072F28);
			}
		} else { */
			*(u32*)0x02050114 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		// }
		*(u32*)0x020668F8 = 0xE1A00000; // nop
		*(u32*)0x0206A538 = 0xE1A00000; // nop
		patchInitDSiWare(0x02070778, heapEnd);
		patchUserSettingsReadDSiWare(0x02071D84);
	}

	// Kakonde Ke Shite: Wakugumi Nojika (Japan)
	// Saving not supported due to using more than one file in filesystem
	else if (strcmp(romTid, "KK4J") == 0) {
		*(u32*)0x02006618 = 0xE1A00000; // nop
		*(u32*)0x02057A84 = 0xE12FFF1E; // bx lr (Skip Manual screen)
		*(u32*)0x02078870 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x02078874 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x0207C530 = 0xE1A00000; // nop
		patchInitDSiWare(0x02083DD4, heapEnd);
		patchUserSettingsReadDSiWare(0x020853EC);
	}

	// WarioWare: Touched! DL (USA, Australia)
	// The sound loading code has been reworked to instead load the SDAT file all at once, so sound is disabled in order for the game to boot within RAM limitations
	else if (strcmp(romTid, "Z2AT") == 0) {
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x02009294 = 0xE1A00000; // nop
		*(u32*)0x020092A8 = 0xE1A00000; // nop
		*(u32*)0x0200937C = 0xE3A00000; // mov r0, #0 (Skip loading SDAT file)
		*(u32*)0x02009380 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x0200BF3C = 0xE1A00000; // nop
		setBL(0x0200BF7C, (u32)dsiSaveCreate);
		setBL(0x0200BF90, (u32)dsiSaveOpen);
		setBL(0x0200BFA4, (u32)dsiSaveSetLength);
		setBL(0x0200BFAC, (u32)dsiSaveClose);
		*(u32*)0x020689F0 = 0xE3A00000; // mov r0, #0 (Skip playing sound file)
		*(u32*)0x020689F4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206AA90 = 0xE1A00000; // nop
		*(u32*)0x0206E500 = 0xE1A00000; // nop
		patchInitDSiWare(0x02075858, heapEnd);
	}

	// WarioWare: Touched! DL (Europe)
	// The sound loading code has been reworked to instead load the SDAT file all at once, so sound is disabled in order for the game to boot within RAM limitations
	else if (strcmp(romTid, "Z2AP") == 0) {
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x020092F4 = 0xE1A00000; // nop
		*(u32*)0x02009308 = 0xE1A00000; // nop
		*(u32*)0x020093DC = 0xE3A00000; // mov r0, #0 (Skip loading SDAT file)
		*(u32*)0x020093E0 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x0200BF9C = 0xE1A00000; // nop
		setBL(0x0200BFDC, (u32)dsiSaveCreate);
		setBL(0x0200BFF0, (u32)dsiSaveOpen);
		setBL(0x0200C004, (u32)dsiSaveSetLength);
		setBL(0x0200C00C, (u32)dsiSaveClose);
		*(u32*)0x0206A9A4 = 0xE3A00000; // mov r0, #0 (Skip playing sound file)
		*(u32*)0x0206A9A8 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206CA44 = 0xE1A00000; // nop
		*(u32*)0x020704B4 = 0xE1A00000; // nop
		patchInitDSiWare(0x0207780C, heapEnd);
	}

	// Sawaru Made in Wario DL (Japan)
	// The sound loading code has been reworked to instead load the SDAT file all at once, so sound is disabled in order for the game to boot within RAM limitations
	else if (strcmp(romTid, "Z2AJ") == 0) {
		*(u32*)0x020050C8 = 0xE1A00000; // nop
		*(u32*)0x020050CC = 0xE1A00000; // nop
		*(u32*)0x02009290 = 0xE1A00000; // nop
		*(u32*)0x020092A4 = 0xE1A00000; // nop
		*(u32*)0x02009378 = 0xE3A00000; // mov r0, #0 (Skip loading SDAT file)
		*(u32*)0x0200937C = 0xE12FFF1E; // bx lr
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
		*(u32*)0x0200BF38 = 0xE1A00000; // nop
		setBL(0x0200BF78, (u32)dsiSaveCreate);
		setBL(0x0200BF8C, (u32)dsiSaveOpen);
		setBL(0x0200BFA0, (u32)dsiSaveSetLength);
		setBL(0x0200BFA8, (u32)dsiSaveClose);
		*(u32*)0x02068970 = 0xE3A00000; // mov r0, #0 (Skip playing sound file)
		*(u32*)0x02068974 = 0xE12FFF1E; // bx lr
		*(u32*)0x0206AA10 = 0xE1A00000; // nop
		*(u32*)0x0206E480 = 0xE1A00000; // nop
		patchInitDSiWare(0x020757D8, heapEnd);
	}

	// Whack-A-Friend (USA)
	// Pashatto Bashitto: Whack a Friend (Japan)
	// DSi Camera is disabled due to DS mode limitations
	else if (strcmp(romTid, "KWQE") == 0 || strcmp(romTid, "KWQJ") == 0) {
		*(u32*)0x020145B0 = 0xE1A00000; // nop
		tonccpy((u32*)0x02015134, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201837C = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FD04, heapEnd);
		patchUserSettingsReadDSiWare(0x02021428);
		if (romTid[3] == 'E') {
			for (int i = 0; i < 8; i++) {
				u32* offset = (u32*)0x02040E3C;
				offset[i] = 0xE1A00000;
			}
			*(u32*)0x02040E60 = 0xE3A01001; // mov r1, #1
			*(u32*)0x02041E50 = 0xE1A00000; // nop (Disable custom photo support)
			*(u32*)0x0204802C = 0xE1A00000; // nop
			*(u32*)0x02048044 = 0xE1A00000; // nop
			setBL(0x0204B4EC, (u32)dsiSaveOpen);
			setBL(0x0204B504, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0204B51C, (u32)dsiSaveOpen);
			setBL(0x0204B53C, (u32)dsiSaveWrite);
			setBL(0x0204B54C, (u32)dsiSaveClose);
			setBL(0x0204B55C, (u32)dsiSaveClose);
			setBL(0x0204B59C, (u32)dsiSaveOpen);
			setBL(0x0204B5C0, (u32)dsiSaveRead);
			setBL(0x0204B5D0, (u32)dsiSaveClose);
			setBL(0x0204B5E0, (u32)dsiSaveClose);
			setBL(0x0204B6C0, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0204B6D0, (u32)dsiSaveOpen);
			setBL(0x0204B6FC, (u32)dsiSaveClose);
			setBL(0x0204B734, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x0204B744, (u32)dsiSaveOpen);
			setBL(0x0204B770, (u32)dsiSaveClose);
		} else {
			for (int i = 0; i < 8; i++) {
				u32* offset = (u32*)0x02026A50;
				offset[i] = 0xE1A00000;
			}
			*(u32*)0x02026A74 = 0xE3A01001; // mov r1, #1
			*(u32*)0x0202CD70 = 0xE1A00000; // nop (Disable custom photo support)
			*(u32*)0x0202E0C0 = 0xE1A00000; // nop
			*(u32*)0x0202E0D8 = 0xE1A00000; // nop
			setBL(0x02031580, (u32)dsiSaveOpen);
			setBL(0x02031598, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x020315B0, (u32)dsiSaveOpen);
			setBL(0x020315D0, (u32)dsiSaveWrite);
			setBL(0x020315E0, (u32)dsiSaveClose);
			setBL(0x020315F0, (u32)dsiSaveClose);
			setBL(0x02031630, (u32)dsiSaveOpen);
			setBL(0x02031654, (u32)dsiSaveRead);
			setBL(0x02031664, (u32)dsiSaveClose);
			setBL(0x02031674, (u32)dsiSaveClose);
			setBL(0x02031754, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x02031764, (u32)dsiSaveOpen);
			setBL(0x02031790, (u32)dsiSaveClose);
			setBL(0x020317C8, (u32)dsiSaveCreate); // dsiSaveCreateAuto
			setBL(0x020317D8, (u32)dsiSaveOpen);
			setBL(0x02031804, (u32)dsiSaveClose);
		}
	}

	// White-Water Domo (USA)
	else if (strcmp(romTid, "KDWE") == 0) {
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
		doubleNopT(0x0200CBC6);
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
		doubleNopT(0x020223BE);
		doubleNopT(0x0202460E);
		doubleNopT(0x020272C4);
		doubleNopT(0x0202887E);
		doubleNopT(0x02028882);
		doubleNopT(0x0202888E);
		doubleNopT(0x02028972);
		patchHiHeapDSiWareThumb(0x020289B0, 0x020232C8, heapEnd); // movs r0, #0x23E0000
		doubleNopT(0x02029A36);
		*(u16*)0x02029A3A = nopT;
		*(u16*)0x02029A3C = nopT;
		doubleNopT(0x02029A3E);
	}

	// Wonderful Sports: Bowling (Japan)
	// Music does not play on retail consoles
	else if (strcmp(romTid, "KBSJ") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200C1F0 = 0xE1A00000; // nop
		*(u32*)0x0200F634 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017404, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x02017790 = 0x0219B920;
		}
		patchUserSettingsReadDSiWare(0x02018A58);
		setBL(0x02027AF4, (u32)dsiSaveOpen);
		setBL(0x02027B04, (u32)dsiSaveGetLength);
		setBL(0x02027B1C, (u32)dsiSaveRead);
		setBL(0x02027B6C, (u32)dsiSaveClose);
		setBL(0x02027BFC, (u32)dsiSaveCreate);
		setBL(0x02027C0C, (u32)dsiSaveOpen);
		setBL(0x02027C20, (u32)dsiSaveSetLength);
		setBL(0x02027C30, (u32)dsiSaveWrite);
		setBL(0x02027C38, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			// Disable music
			*(u32*)0x020283E0 = 0xE3A01702; // mov r1, #0x80000 (Shrink sound heap)
			*(u32*)0x0202B634 = 0xE12FFF1E; // bx lr
			*(u32*)0x0202BFE8 = 0xE12FFF1E; // bx lr
		}

		if (useSharedFont) {
			if (!extendedMemory2 && expansionPakFound) {
				patchTwlFontLoad(0x02028610, 0x02018FC8);
			}
		} else {
			*(u32*)0x02005084 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)

			// Skip Manual screen
			*(u32*)0x02029AF0 = 0xE1A00000; // nop
			*(u32*)0x02029AF8 = 0xE1A00000; // nop

			// Skip NFTR font rendering
			*(u32*)0x02028C50 = 0xE3A00001; // mov r0, #1
			*(u32*)0x02033D88 = 0xE12FFF1E; // bx lr
		}
	}

	// Word Searcher (USA)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KWSE") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (!useSharedFont) {
			*(u32*)0x020050D4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		setBL(0x020060FC, (u32)dsiSaveOpen);
		setBL(0x0200610C, (u32)dsiSaveClose);
		setBL(0x02006120, (u32)dsiSaveCreate);
		setBL(0x02006130, (u32)dsiSaveOpen);
		setBL(0x02006140, (u32)dsiSaveGetResultCode);
		setBL(0x02006148, (u32)dsiSaveClose);
		setBL(0x02008748, (u32)dsiSaveOpen);
		setBL(0x02008758, (u32)dsiSaveClose);
		setBL(0x0200899C, (u32)dsiSaveWrite);
		setBL(0x020089A4, (u32)dsiSaveClose);
		setBL(0x02008A7C, (u32)dsiSaveOpen);
		*(u32*)0x02008A98 = 0xE1A00000; // nop
		setBL(0x02008AB4, (u32)dsiSaveGetLength);
		setBL(0x02008AC8, (u32)dsiSaveClose);
		setBL(0x02008AE8, (u32)dsiSaveRead);
		setBL(0x02008AF0, (u32)dsiSaveClose);
		setBL(0x02008B04, (u32)dsiSaveDelete);
		setBL(0x0200B900, (u32)dsiSaveOpen);
		*(u32*)0x0200B91C = 0xE1A00000; // nop
		setBL(0x0200B938, (u32)dsiSaveGetLength);
		setBL(0x0200B94C, (u32)dsiSaveClose);
		setBL(0x0200B964, (u32)dsiSaveRead);
		setBL(0x0200B96C, (u32)dsiSaveClose);
		setBL(0x0200B980, (u32)dsiSaveDelete);
		*(u32*)0x02023070 = 0xE1A00000; // nop
		*(u32*)0x02026400 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202B1F4, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x0202B564 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x0202C698);
		*(u32*)0x0202CAC8 = 0xE1A00000; // nop
		*(u32*)0x0202CACC = 0xE1A00000; // nop
		*(u32*)0x0202CAD0 = 0xE1A00000; // nop
		*(u32*)0x0202CAD4 = 0xE1A00000; // nop
	}

	// Word Searcher (Europe)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KWSP") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (!useSharedFont) {
			*(u32*)0x020050D4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		setBL(0x020056AC, (u32)dsiSaveOpen);
		setBL(0x020056BC, (u32)dsiSaveClose);
		setBL(0x020056D0, (u32)dsiSaveCreate);
		setBL(0x020056E0, (u32)dsiSaveOpen);
		setBL(0x020056F0, (u32)dsiSaveGetResultCode);
		setBL(0x020056F8, (u32)dsiSaveClose);
		setBL(0x020081C0, (u32)dsiSaveOpen);
		setBL(0x020081D0, (u32)dsiSaveClose);
		setBL(0x02008414, (u32)dsiSaveWrite);
		setBL(0x0200841C, (u32)dsiSaveClose);
		setBL(0x020084F4, (u32)dsiSaveOpen);
		*(u32*)0x02008510 = 0xE1A00000; // nop
		setBL(0x0200852C, (u32)dsiSaveGetLength);
		setBL(0x02008540, (u32)dsiSaveClose);
		setBL(0x02008560, (u32)dsiSaveRead);
		setBL(0x02008568, (u32)dsiSaveClose);
		setBL(0x0200857C, (u32)dsiSaveDelete);
		setBL(0x0200B55C, (u32)dsiSaveOpen);
		*(u32*)0x0200B578 = 0xE1A00000; // nop
		setBL(0x0200B594, (u32)dsiSaveGetLength);
		setBL(0x0200B5A8, (u32)dsiSaveClose);
		setBL(0x0200B5C0, (u32)dsiSaveRead);
		setBL(0x0200B5C8, (u32)dsiSaveClose);
		setBL(0x0200B5DC, (u32)dsiSaveDelete);
		if (!extendedMemory2) {
			*(u32*)0x020119EC = 0xE3A0078A; // mov r0, #0x02280000
		}
		*(u32*)0x0202537C = 0xE1A00000; // nop
		*(u32*)0x020285C0 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202D114, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x0202D4A0 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x0202E6C0);
		*(u32*)0x0202EAC8 = 0xE1A00000; // nop
		*(u32*)0x0202EACC = 0xE1A00000; // nop
		*(u32*)0x0202EAD0 = 0xE1A00000; // nop
		*(u32*)0x0202EAD4 = 0xE1A00000; // nop
	}

	// Word Searcher II (USA)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if (strcmp(romTid, "KWRE") == 0 && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (!useSharedFont) {
			*(u32*)0x020050D4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		setBL(0x02005AF4, (u32)dsiSaveOpen);
		setBL(0x02005B04, (u32)dsiSaveClose);
		setBL(0x02005B18, (u32)dsiSaveCreate);
		setBL(0x02005B28, (u32)dsiSaveOpen);
		setBL(0x02005B38, (u32)dsiSaveGetResultCode);
		setBL(0x02005B48, (u32)dsiSaveClose);
		*(u32*)0x02005A08 = 0xE1A00000; // nop
		setBL(0x02005A20, (u32)dsiSaveOpen);
		*(u32*)0x02005A3C = 0xE1A00000; // nop
		*(u32*)0x02005A48 = 0xE1A00000; // nop
		setBL(0x02005A64, (u32)dsiSaveWrite);
		setBL(0x02005A6C, (u32)dsiSaveClose);
		*(u32*)0x02005A80 = 0xE1A00000; // nop
		*(u32*)0x02005A8C = 0xE1A00000; // nop
		setBL(0x02007DD4, (u32)dsiSaveOpen);
		setBL(0x02007E10, (u32)dsiSaveGetLength);
		setBL(0x02007E24, (u32)dsiSaveClose);
		setBL(0x02007E44, (u32)dsiSaveRead);
		setBL(0x02007E4C, (u32)dsiSaveClose);
		*(u32*)0x0200AA9C = 0xE1A00000; // nop
		setBL(0x0200AF80, (u32)dsiSaveOpen);
		setBL(0x0200AFAC, (u32)dsiSaveGetLength);
		setBL(0x0200AFC0, (u32)dsiSaveClose);
		setBL(0x0200AFDC, (u32)dsiSaveRead);
		setBL(0x0200AFE4, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)0x02012BA8 = 0xE3A0078A; // mov r0, #0x02280000
		}
		*(u32*)0x02026668 = 0xE1A00000; // nop
		*(u32*)0x02029840 = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E394, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)0x0202E720 = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x0202F940);
		*(u32*)0x0202FD48 = 0xE1A00000; // nop
		*(u32*)0x0202FD4C = 0xE1A00000; // nop
		*(u32*)0x0202FD50 = 0xE1A00000; // nop
		*(u32*)0x0202FD54 = 0xE1A00000; // nop
	}

	// Word Searcher III (USA)
	// Word Searcher IV (USA)
	// Requires either 8MB of RAM or Memory Expansion Pak
	else if ((strcmp(romTid, "KW6E") == 0 || strcmp(romTid, "KW8E") == 0) && debugOrMep) {
		const u32 mepAddr = (s2FlashcardId == 0x5A45) ? 0x08800000 : 0x09000000;
		u8 offsetChange = (romTid[2] == '6') ? 0 : 8;

		useSharedFont = (twlFontFound && extendedMemory2);
		if (!useSharedFont) {
			*(u32*)0x020050D4 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x020050FC = 0xE1A00000; // nop
		setBL(0x02005B1C, (u32)dsiSaveOpen);
		setBL(0x02005B2C, (u32)dsiSaveClose);
		setBL(0x02005B40, (u32)dsiSaveCreate);
		setBL(0x02005B50, (u32)dsiSaveOpen);
		setBL(0x02005B60, (u32)dsiSaveGetResultCode);
		setBL(0x02005B70, (u32)dsiSaveClose);
		*(u32*)0x02005A30 = 0xE1A00000; // nop
		setBL(0x02005A48, (u32)dsiSaveOpen);
		*(u32*)0x02005A64 = 0xE1A00000; // nop
		*(u32*)0x02005A70 = 0xE1A00000; // nop
		setBL(0x02005A8C, (u32)dsiSaveWrite);
		setBL(0x02005A94, (u32)dsiSaveClose);
		*(u32*)0x02005AA8 = 0xE1A00000; // nop
		*(u32*)0x02005AB4 = 0xE1A00000; // nop
		setBL(0x02007D70, (u32)dsiSaveOpen);
		setBL(0x02007DAC, (u32)dsiSaveGetLength);
		setBL(0x02007DC0, (u32)dsiSaveClose);
		setBL(0x02007DE0, (u32)dsiSaveRead);
		setBL(0x02007DE8, (u32)dsiSaveClose);
		*(u32*)0x0200AA38 = 0xE1A00000; // nop
		setBL(0x0200AF1C, (u32)dsiSaveOpen);
		setBL(0x0200AF48, (u32)dsiSaveGetLength);
		setBL(0x0200AF5C, (u32)dsiSaveClose);
		setBL(0x0200AF78, (u32)dsiSaveRead);
		setBL(0x0200AF80, (u32)dsiSaveClose);
		if (!extendedMemory2) {
			*(u32*)(0x02012B40-offsetChange) = 0xE3A0078A; // mov r0, #0x02280000
		}
		*(u32*)(0x02026600-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x020297D8-offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0202E32C-offsetChange, extendedMemory2 ? heapEnd : mepAddr+0x77C000);
		if (!extendedMemory2) {
			*(u32*)(0x0202E6B8-offsetChange) = mepAddr;
		}
		patchUserSettingsReadDSiWare(0x0202F8D8-offsetChange);
		*(u32*)(0x0202FCE0-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0202FCE4-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0202FCE8-offsetChange) = 0xE1A00000; // nop
		*(u32*)(0x0202FCEC-offsetChange) = 0xE1A00000; // nop
	}

	// WordJong Arcade (USA)
	// Save patch does not work (only bypasses)
	else if (strcmp(romTid, "K2AE") == 0) {
		*(u32*)0x0200D490 = 0xE1A00000; // nop
		*(u32*)0x02011048 = 0xE1A00000; // nop
		patchInitDSiWare(0x020165E4, heapEnd);
		*(u32*)0x02016970 = *(u32*)0x02004FE8;
		patchUserSettingsReadDSiWare(0x02017D04);
		*(u32*)0x020294E0 = 0xE3A00000; // mov r0, #0
		*(u32*)0x0204C4D4 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204C538 = 0xE12FFF1E; // bx lr
		*(u32*)0x0204C59C = 0xE12FFF1E; // bx lr
		*(u32*)0x02079C48 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
	}

	// Working Dawgs: A-maze-ing Pipes (USA)
	else if (strcmp(romTid, "KYWE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0200DB1C = 0xE1A00000; // nop
		tonccpy((u32*)0x0200E6A0, dsiSaveGetResultCode, 0xC);
		*(u32*)0x020110BC = 0xE1A00000; // nop
		patchInitDSiWare(0x02017830, heapEnd);
		setBL(0x02031D38, (u32)dsiSaveCreate);
		setBL(0x02031D54, (u32)dsiSaveOpen);
		setBL(0x02031D68, (u32)dsiSaveSetLength);
		setBL(0x02031D94, (u32)dsiSaveClose);
		setBL(0x02031E44, (u32)dsiSaveDelete);
		*(u32*)0x02031E8C = 0xE1A00000; // nop
		setBL(0x02031EF0, (u32)dsiSaveOpen);
		setBL(0x02031F08, (u32)dsiSaveSeek);
		setBL(0x02031F1C, (u32)dsiSaveRead);
		setBL(0x02031F2C, (u32)dsiSaveClose);
		*(u32*)0x02031FF0 = 0xE1A00000; // nop
		setBL(0x02032018, (u32)dsiSaveOpen);
		setBL(0x02032030, (u32)dsiSaveSeek);
		setBL(0x02032044, (u32)dsiSaveWrite);
		setBL(0x02032054, (u32)dsiSaveClose);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02032D6C, 0x02019190);
			}
		} else {
			*(u32*)0x02032BE8 = 0xE12FFF1E; // bx lr (Disable NFTR loading from TWLNAND)
		}
	}

	// Working Dawgs: Rivet Retriever (USA)
	else if (strcmp(romTid, "KU3E") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		if (useSharedFont) {
			if (!extendedMemory2) {
				patchTwlFontLoad(0x02033018, 0x020194B0);
			}
		} else {
			*(u32*)0x020055A0 = 0xE1A00000; // nop (Disable NFTR loading from TWLNAND)
		}
		*(u32*)0x0200DD60 = 0xE1A00000; // nop
		tonccpy((u32*)0x0200E8E4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02011300 = 0xE1A00000; // nop
		patchInitDSiWare(0x02017ACC, heapEnd);
		patchUserSettingsReadDSiWare(0x02018F6C);
		setBL(0x02031FDC, (u32)dsiSaveCreate);
		setBL(0x02031FF8, (u32)dsiSaveOpen);
		setBL(0x0203200C, (u32)dsiSaveSetLength);
		setBL(0x02032038, (u32)dsiSaveClose);
		setBL(0x020320E4, (u32)dsiSaveDelete);
		*(u32*)0x0203212C = 0xE1A00000; // nop
		setBL(0x02032190, (u32)dsiSaveOpen);
		setBL(0x020321A8, (u32)dsiSaveSeek);
		setBL(0x020321B8, (u32)dsiSaveRead);
		setBL(0x020321C8, (u32)dsiSaveClose);
		setBL(0x020322AC, (u32)dsiSaveOpen);
		setBL(0x020322C4, (u32)dsiSaveSeek);
		setBL(0x020322D4, (u32)dsiSaveWrite);
		setBL(0x020322E4, (u32)dsiSaveClose);
	}

	// Yummy Yummy Cooking Jam (USA)
	// Music is disabled
	else if (strcmp(romTid, "KYUE") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200508C = 0xE1A00000; // nop
		*(u32*)0x0201CD5C = 0xE1A00000; // nop
		tonccpy((u32*)0x0201DA10, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02020374 = 0xE1A00000; // nop
		patchInitDSiWare(0x02026E84, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x020271F4 = 0x020E5C00;
		}
		patchUserSettingsReadDSiWare(0x02028360);
		*(u32*)0x020639A4 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x02069F94 = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x020829B4, 0x02029298);
			if (expansionPakFound) {
				*(u32*)0x02082A00 = 0xE1A00000; // nop
			}
		}
	}

	// Yummy Yummy Cooking Jam (Europe, Australia)
	// Music is disabled
	else if (strcmp(romTid, "KYUV") == 0) {
		useSharedFont = twlFontFound;
		*(u32*)0x0200148C = 0xE1A00000; // nop
		*(u32*)0x02019130 = 0xE1A00000; // nop
		tonccpy((u32*)0x02019DE4, dsiSaveGetResultCode, 0xC);
		*(u32*)0x0201C748 = 0xE1A00000; // nop
		patchInitDSiWare(0x02023258, heapEnd);
		if (!extendedMemory2) {
			*(u32*)0x020235C8 = 0x020E1FC0;
		}
		patchUserSettingsReadDSiWare(0x02024734);
		*(u32*)0x0205FD48 = 0xE12FFF1E; // bx lr
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
		*(u32*)0x02066338 = 0xE1A00000; // nop
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x0207ED58, 0x0202566C);
			if (expansionPakFound) {
				*(u32*)0x0207EDA4 = 0xE1A00000; // nop
			}
		}
	}

	// Art Style: ZENGAGE (USA)
	// Art Style: NEMREM (Europe, Australia)
	else if (strcmp(romTid, "KASE") == 0 || strcmp(romTid, "KASV") == 0) {
		*(u32*)0x0200E000 = 0xE1A00000; // nop
		*(u32*)0x0200E080 = 0xE1A00000; // nop
		*(u32*)0x0200E1E8 = 0xE1A00000; // nop
		*(u32*)0x0200E290 = 0xE1A00000; // nop
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
		if (romTid[3] == 'E') {
			*(u32*)0x0201CAAC = 0xE1A00000; // nop
			*(u32*)0x0201CAB0 = 0xE1A00000; // nop
			*(u32*)0x0201CAC0 = 0xE1A00000; // nop
			*(u32*)0x0201CE74 = 0xE1A00000; // nop
			*(u32*)0x0201CE80 = 0xE1A00000; // nop
			*(u32*)0x0201CEA8 = 0xE1A00000; // nop
			*(u32*)0x0201D474 = 0xE1A00000; // nop
			*(u32*)0x0201D48C = 0xE1A00000; // nop
			*(u32*)0x0201D538 = 0xE1A00000; // nop
			*(u32*)0x02035228 = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x0203522C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x02038820 = 0xE1A00000; // nop
			patchInitDSiWare(0x02040DF0, heapEnd);
			patchUserSettingsReadDSiWare(0x02042388);
		} else if (romTid[3] == 'V') {
			*(u32*)0x0201C7A0 = 0xE1A00000; // nop
			*(u32*)0x0201C7A4 = 0xE1A00000; // nop
			*(u32*)0x0201C7B4 = 0xE1A00000; // nop
			*(u32*)0x0201CB5C = 0xE1A00000; // nop
			*(u32*)0x0201CB68 = 0xE1A00000; // nop
			*(u32*)0x0201CB90 = 0xE1A00000; // nop
			*(u32*)0x0201D164 = 0xE1A00000; // nop
			*(u32*)0x0201D17C = 0xE1A00000; // nop
			*(u32*)0x0201D228 = 0xE1A00000; // nop
			*(u32*)0x02034CBC = 0xE28DD00C; // ADD   SP, SP, #0xC
			*(u32*)0x02034CC0 = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
			*(u32*)0x020382B4 = 0xE1A00000; // nop
			patchInitDSiWare(0x02040884, heapEnd);
			patchUserSettingsReadDSiWare(0x02041E1C);
		}
	}

	// Art Style: SOMNIUM (Japan)
	else if (strcmp(romTid, "KASJ") == 0) {
		*(u32*)0x0200E280 = 0xE1A00000; // nop
		*(u32*)0x0200E300 = 0xE1A00000; // nop
		*(u32*)0x0200E480 = 0xE1A00000; // nop
		*(u32*)0x0200E534 = 0xE1A00000; // nop
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
		*(u32*)0x0201D190 = 0xE1A00000; // nop
		*(u32*)0x0201D194 = 0xE1A00000; // nop
		*(u32*)0x0201D1A4 = 0xE1A00000; // nop
		*(u32*)0x0201D554 = 0xE1A00000; // nop
		*(u32*)0x0201D560 = 0xE1A00000; // nop
		*(u32*)0x0201D588 = 0xE1A00000; // nop
		*(u32*)0x0201DAAC = 0xE1A00000; // nop
		*(u32*)0x0201DAC4 = 0xE1A00000; // nop
		*(u32*)0x0201DB70 = 0xE1A00000; // nop
		*(u32*)0x02035688 = 0xE28DD00C; // ADD   SP, SP, #0xC
		*(u32*)0x0203568C = 0xE8BD8078; // LDMFD SP!, {R3-R6,PC}
		*(u32*)0x02038E24 = 0xE1A00000; // nop
		patchInitDSiWare(0x020417A4, heapEnd);
		patchUserSettingsReadDSiWare(0x02042D50);
	}

	// Zombie Blaster (USA)
	else if (strcmp(romTid, "K7KE") == 0) {
		useSharedFont = (twlFontFound && debugOrMep);
		*(u32*)0x0201A710 = 0xE1A00000; // nop
		*(u32*)0x0201E2D0 = 0xE1A00000; // nop
		patchInitDSiWare(0x020267A0, heapEnd);
		*(u32*)0x0202D5D8 = 0xE3A00001; // mov r0, #1
		*(u32*)0x0202D5DC = 0xE12FFF1E; // bx lr
		setBL(0x02065568, (u32)dsiSaveOpenR);
		setBL(0x02065584, (u32)dsiSaveCreate); // dsiSaveCreateAuto
		setBL(0x020655C0, (u32)dsiSaveOpen);
		setBL(0x020655D4, (u32)dsiSaveGetResultCode);
		*(u32*)0x020655EC = 0xE1A00000; // nop
		setBL(0x02067068, (u32)dsiSaveOpen);
		setBL(0x02067084, (u32)dsiSaveWrite);
		setBL(0x02067090, (u32)dsiSaveClose);
		setBL(0x020670E4, (u32)dsiSaveOpen);
		setBL(0x020670F8, (u32)dsiSaveGetLength);
		setBL(0x0206710C, (u32)dsiSaveRead);
		setBL(0x02067118, (u32)dsiSaveClose);
		if (useSharedFont && !extendedMemory2) {
			patchTwlFontLoad(0x02090D50, 0x02028244);
		}
	}

	// Zombie Skape (USA)
	// Zombie Skape (Europe)
	else if (strcmp(romTid, "KZYE") == 0 || strcmp(romTid, "KZYP") == 0) {
		u8 offsetChange = (romTid[3] == 'E') ? 0 : 0x4C;

		*(u32*)0x020050E8 = 0xE1A00000; // nop
		*(u32*)0x0200510C = 0xE1A00000; // nop
		*(u32*)0x02005110 = 0xE1A00000; // nop
		*(u32*)0x02005114 = 0xE1A00000; // nop
		*(u32*)0x02005118 = 0xE1A00000; // nop
		*(u32*)0x0200511C = 0xE1A00000; // nop
		*(u32*)0x02005138 = 0xE1A00000; // nop
		*(u32*)0x0200514C = 0xE1A00000; // nop
		*(u32*)0x02005150 = 0xE1A00000; // nop
		*(u32*)0x02005154 = 0xE1A00000; // nop
		*(u32*)0x02005158 = 0xE1A00000; // nop
		*(u32*)0x0200515C = 0xE1A00000; // nop
		*(u32*)0x02010AB8 = 0xE1A00000; // nop
		*(u32*)0x02010AEC = 0xE1A00000; // nop
		setBL(0x02011D58, (u32)dsiSaveCreate);
		*(u32*)0x02011D6C = 0xE1A00000; // nop
		*(u32*)0x02011D78 = 0xE3A00001; // mov r0, #1
		*(u32*)0x02011DF8 = 0xE1A00000; // nop
		*(u32*)0x02011DFC = 0xE1A00000; // nop
		*(u32*)0x02011E00 = 0xE1A00000; // nop
		setBL(0x02011E0C, (u32)dsiSaveGetResultCode);
		*(u32*)0x02011E24 = 0xE1A00000; // nop
		*(u32*)0x02011E30 = 0xE3A00001; // mov r0, #1
		setBL(0x02032118+offsetChange, (u32)dsiSaveOpen);
		setBL(0x02032130+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x02032140+offsetChange, (u32)dsiSaveSeek);
		setBL(0x02032150+offsetChange, (u32)dsiSaveWrite);
		setBL(0x02032158+offsetChange, (u32)dsiSaveClose);
		setBL(0x020321C8+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020321E0+offsetChange, (u32)dsiSaveGetLength);
		setBL(0x020321F4+offsetChange, (u32)dsiSaveSeek);
		setBL(0x02032204+offsetChange, (u32)dsiSaveRead);
		setBL(0x0203220C+offsetChange, (u32)dsiSaveClose);
		setBL(0x02032284+offsetChange, (u32)dsiSaveCreate);
		setBL(0x020322B0+offsetChange, (u32)dsiSaveOpen);
		setBL(0x020322EC+offsetChange, (u32)dsiSaveWrite);
		setBL(0x020322FC+offsetChange, (u32)dsiSaveClose);
		*(u32*)(0x020641BC+offsetChange) = 0xE1A00000; // nop
		patchInitDSiWare(0x0206C8DC+offsetChange, heapEnd);
		*(u32*)(0x0206CC68+offsetChange) -= 0x30000;
		*(u32*)(0x02073280+offsetChange) = 0xE1A00000; // nop
	}

	// Zoonies: Escape from Makatu (USA)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KZSE") == 0) {
		*(u32*)0x0200C674 = 0xE1A00000; // nop
		*(u32*)0x0200C688 = 0xE1A00000; // nop
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
		*(u32*)0x0206F7BC = 0xE1A00000; // nop
		*(u32*)0x02072B08 = 0xE1A00000; // nop
		patchInitDSiWare(0x02079BFC, heapEnd);
		patchUserSettingsReadDSiWare(0x0207B09C);
	}

	// Zoonies: Escape from Makatu (Europe, Australia)
	// Due to our save implementation, save data is stored in all 3 slots
	else if (strcmp(romTid, "KZSV") == 0) {
		*(u32*)0x0200C658 = 0xE1A00000; // nop
		*(u32*)0x0200C66C = 0xE1A00000; // nop
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
		*(u32*)0x0206F79C = 0xE1A00000; // nop
		*(u32*)0x02072AE8 = 0xE1A00000; // nop
		patchInitDSiWare(0x02079BDC, heapEnd);
		patchUserSettingsReadDSiWare(0x0207B07C);
	}

	// Zuma's Revenge! (USA)
	// Zuma's Revenge! (Europe, Australia)
	else if (strcmp(romTid, "KZTE") == 0 || strcmp(romTid, "KZTV") == 0) {
		*(u32*)0x0200519C = 0xE1A00000; // nop
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
		*(u32*)0x02015334 = 0xE1A00000; // nop
		tonccpy((u32*)0x02016E10, dsiSaveGetResultCode, 0xC);
		*(u32*)0x02019CEC = 0xE1A00000; // nop
		patchInitDSiWare(0x0201FF04, heapEnd);
		*(u32*)0x02020290 -= 0x30000;
		patchUserSettingsReadDSiWare(0x020213C0);
		if (romTid[3] == 'E') {
			*(u32*)0x02081270 = 0xE1A00000; // nop
			*(u32*)0x02081278 = 0xE1A00000; // nop
			*(u32*)0x02081324 = 0xE1A00000; // nop
			*(u32*)0x02081338 = 0xE1A00000; // nop
			*(u32*)0x02081340 = 0xE1A00000; // nop
			*(u32*)0x02081418 = 0xE1A00000; // nop
			*(u32*)0x0208142C = 0xE1A00000; // nop
			*(u32*)0x02081434 = 0xE1A00000; // nop
			*(u32*)0x02081620 = 0xE1A00000; // nop
			*(u32*)0x02081628 = 0xE1A00000; // nop
			*(u32*)0x020816C8 = 0xE1A00000; // nop
			*(u32*)0x020816DC = 0xE1A00000; // nop
			*(u32*)0x020816E4 = 0xE1A00000; // nop
			*(u32*)0x02081770 = 0xE1A00000; // nop
			*(u32*)0x02081774 = 0xE1A00000; // nop
			*(u32*)0x02081790 = 0xE1A00000; // nop
			*(u32*)0x020817A4 = 0xE1A00000; // nop
			*(u32*)0x020817AC = 0xE1A00000; // nop
			*(u32*)0x0208180C = 0xE1A00000; // nop
			*(u32*)0x02081814 = 0xE1A00000; // nop
			*(u32*)0x02081850 = 0xE1A00000; // nop
			*(u32*)0x02081864 = 0xE1A00000; // nop
			*(u32*)0x0208186C = 0xE1A00000; // nop
			*(u32*)0x02081990 = 0xE1A00000; // nop
			*(u32*)0x02081998 = 0xE1A00000; // nop
			*(u32*)0x02081AF0 = 0xE1A00000; // nop
			*(u32*)0x02081B04 = 0xE1A00000; // nop
			*(u32*)0x02081B0C = 0xE1A00000; // nop
			*(u32*)0x02081B84 = 0xE1A00000; // nop
			*(u32*)0x02081B8C = 0xE1A00000; // nop
			*(u32*)0x02081D3C = 0xE1A00000; // nop
			*(u32*)0x02081D50 = 0xE1A00000; // nop
			*(u32*)0x02081D58 = 0xE1A00000; // nop
		} else {
			*(u32*)0x020812D8 = 0xE1A00000; // nop
			*(u32*)0x020812E0 = 0xE1A00000; // nop
			*(u32*)0x0208138C = 0xE1A00000; // nop
			*(u32*)0x020813A0 = 0xE1A00000; // nop
			*(u32*)0x020813A8 = 0xE1A00000; // nop
			*(u32*)0x02081480 = 0xE1A00000; // nop
			*(u32*)0x02081494 = 0xE1A00000; // nop
			*(u32*)0x0208149C = 0xE1A00000; // nop
			*(u32*)0x02081688 = 0xE1A00000; // nop
			*(u32*)0x02081690 = 0xE1A00000; // nop
			*(u32*)0x02081730 = 0xE1A00000; // nop
			*(u32*)0x02081744 = 0xE1A00000; // nop
			*(u32*)0x0208174C = 0xE1A00000; // nop
			*(u32*)0x020817D8 = 0xE1A00000; // nop
			*(u32*)0x020817E0 = 0xE1A00000; // nop
			*(u32*)0x020817F8 = 0xE1A00000; // nop
			*(u32*)0x0208180C = 0xE1A00000; // nop
			*(u32*)0x02081814 = 0xE1A00000; // nop
			*(u32*)0x02081874 = 0xE1A00000; // nop
			*(u32*)0x0208187C = 0xE1A00000; // nop
			*(u32*)0x020818B8 = 0xE1A00000; // nop
			*(u32*)0x020818CC = 0xE1A00000; // nop
			*(u32*)0x020818D4 = 0xE1A00000; // nop
			*(u32*)0x020819F8 = 0xE1A00000; // nop
			*(u32*)0x02081A00 = 0xE1A00000; // nop
			*(u32*)0x02081B58 = 0xE1A00000; // nop
			*(u32*)0x02081B6C = 0xE1A00000; // nop
			*(u32*)0x02081B74 = 0xE1A00000; // nop
			*(u32*)0x02081BEC = 0xE1A00000; // nop
			*(u32*)0x02081BF4 = 0xE1A00000; // nop
			*(u32*)0x02081DA4 = 0xE1A00000; // nop
			*(u32*)0x02081DB8 = 0xE1A00000; // nop
			*(u32*)0x02081DC0 = 0xE1A00000; // nop
		}
	}
#endif
}
