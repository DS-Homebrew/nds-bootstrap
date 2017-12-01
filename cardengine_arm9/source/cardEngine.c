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

#include <nds.h> 
#include <nds/fifomessages.h>
#include "cardEngine.h"

#include "databwlist.h"

static u32 ROM_LOCATION = 0x0C800000;
static u32 ROM_TID;
static u32 ARM9_LEN;
static u32 romSize;

#define _32KB_READ_SIZE 0x8000
#define _64KB_READ_SIZE 0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE 0x100000

#define REG_MBK_WRAM_CACHE_START	0x4004045
#define WRAM_CACHE_ADRESS_START 0x03708000
#define WRAM_CACHE_ADRESS_END 0x03778000
#define WRAM_CACHE_ADRESS_SIZE 0x78000
#define WRAM_CACHE_SLOTS 15

#define _128KB_CACHE_ADRESS_START 0x0C800000
#define _128KB_CACHE_ADRESS_SIZE 0x600000
#define _128KB_CACHE_SLOTS 0x30
#define _256KB_CACHE_ADRESS_START 0x0CE00000
#define _256KB_CACHE_ADRESS_SIZE 0x800000
#define _256KB_CACHE_SLOTS 0x20
#define _512KB_CACHE_ADRESS_START 0x0D600000
#define _512KB_CACHE_ADRESS_SIZE 0xA00000
#define _512KB_CACHE_SLOTS 0x14

#define only_CACHE_ADRESS_START 0x0C800000
#define only_CACHE_ADRESS_SIZE 0x1800000
#define only_128KB_CACHE_SLOTS 0xC0
#define only_192KB_CACHE_SLOTS 0x80
#define only_256KB_CACHE_SLOTS 0x60
#define only_512KB_CACHE_SLOTS 0x30
#define only_768KB_CACHE_SLOTS 0x20
#define only_1MB_CACHE_SLOTS 0x18

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 WRAM_cacheDescriptor [WRAM_CACHE_SLOTS];
static u32 WRAM_cacheCounter [WRAM_CACHE_SLOTS];
static u32 _128KB_cacheDescriptor [_128KB_CACHE_SLOTS] = {0xffffffff};
static u32 _128KB_cacheCounter [_128KB_CACHE_SLOTS];
static u32 _256KB_cacheDescriptor [_256KB_CACHE_SLOTS] = {0xffffffff};
static u32 _256KB_cacheCounter [_256KB_CACHE_SLOTS];
static u32 _512KB_cacheDescriptor [_512KB_CACHE_SLOTS] = {0xffffffff};
static u32 _512KB_cacheCounter [_512KB_CACHE_SLOTS];
static u32 only_cacheDescriptor [only_128KB_CACHE_SLOTS];
static u32 only_cacheCounter [only_128KB_CACHE_SLOTS];
static u32 WRAM_accessCounter = 0;
static u32 _128KB_accessCounter = 0;
static u32 _256KB_accessCounter = 0;
static u32 _512KB_accessCounter = 0;
static u32 only_accessCounter = 0;

static int selectedSize = -1;
static u32 only_cacheSlots = 0;
static u32 CACHE_READ_SIZE = _512KB_READ_SIZE;
static bool cacheSizeSet = false;
static bool dynamicCaching = false;

static bool flagsSet = false;
static int ROMinRAM = 0;
static int use28MB = 0;
static bool dsiWramUsed = false;

static u32 GAME_CACHE_ADRESS_START = 0x0C800000;
static u32 GAME_CACHE_SLOTS = 0;
static u32 GAME_READ_SIZE = _256KB_READ_SIZE;

u32 setDataBWlist[3] = {0x00000000, 0x00000000, 0x00000000};
u32 setDataBWlist_1[3] = {0x00000000, 0x00000000, 0x00000000};
u32 setDataBWlist_2[3] = {0x00000000, 0x00000000, 0x00000000};
int dataAmount = 0;

static bool whitelist = false;

void user_exception(void);

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	exceptionStack = (u32)0x23EFFFC ;
	EXCEPTION_VECTOR = enterException ;
	*exceptionC = user_exception;
}

int WRAM_allocateCacheSlot() {
	int slot = 0;
	int lowerCounter = WRAM_accessCounter;
	for(int i=0; i<WRAM_CACHE_SLOTS; i++) {
		if(WRAM_cacheCounter[i]<=lowerCounter) {
			lowerCounter = WRAM_cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int allocateCacheSlot() {
	int slot = 0;
	int lowerCounter = 0;
	if(selectedSize==0) {
		lowerCounter = _128KB_accessCounter;
		for(int i=0; i<_128KB_CACHE_SLOTS; i++) {
			if(_128KB_cacheCounter[i]<=lowerCounter) {
				lowerCounter = _128KB_cacheCounter[i];
				slot = i;
				if(!lowerCounter) break;
			}
		}
	} else if(selectedSize==1) {
		lowerCounter = _256KB_accessCounter;
		for(int i=0; i<_256KB_CACHE_SLOTS; i++) {
			if(_256KB_cacheCounter[i]<=lowerCounter) {
				lowerCounter = _256KB_cacheCounter[i];
				slot = i;
				if(!lowerCounter) break;
			}
		}
	} else if(selectedSize==2) {
		lowerCounter = _512KB_accessCounter;
		for(int i=0; i<_512KB_CACHE_SLOTS; i++) {
			if(_512KB_cacheCounter[i]<=lowerCounter) {
				lowerCounter = _512KB_cacheCounter[i];
				slot = i;
				if(!lowerCounter) break;
			}
		}
	} else {
		lowerCounter = only_accessCounter;
		for(int i=0; i<only_cacheSlots; i++) {
			if(only_cacheCounter[i]<=lowerCounter) {
				lowerCounter = only_cacheCounter[i];
				slot = i;
				if(!lowerCounter) break;
			}
		}
	}
	return slot;
}

int GAME_allocateCacheSlot() {
	int slot = 0;
	int lowerCounter = only_accessCounter;
	for(int i=0; i<GAME_CACHE_SLOTS; i++) {
		if(only_cacheCounter[i]<=lowerCounter) {
			lowerCounter = only_cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int WRAM_getSlotForSector(u32 sector) {
	for(int i=0; i<WRAM_CACHE_SLOTS; i++) {
		if(WRAM_cacheDescriptor[i]==sector) {
			return i;
		}
	}
	return -1;
}

int getSlotForSector(u32 sector) {
	if(selectedSize==0) {
		for(int i=0; i<_128KB_CACHE_SLOTS; i++) {
			if(_128KB_cacheDescriptor[i]==sector) {
				return i;
			}
		}
	} else if(selectedSize==1) {
		for(int i=0; i<_256KB_CACHE_SLOTS; i++) {
			if(_256KB_cacheDescriptor[i]==sector) {
				return i;
			}
		}
	} else if(selectedSize==2) {
		for(int i=0; i<_512KB_CACHE_SLOTS; i++) {
			if(_512KB_cacheDescriptor[i]==sector) {
				return i;
			}
		}
	} else {
		for(int i=0; i<only_cacheSlots; i++) {
			if(only_cacheDescriptor[i]==sector) {
				return i;
			}
		}
	}
	return -1;
}

int GAME_getSlotForSector(u32 sector) {
	for(int i=0; i<GAME_CACHE_SLOTS; i++) {
		if(only_cacheDescriptor[i]==sector) {
			return i;
		}
	}
	return -1;
}


vu8* WRAM_getCacheAddress(int slot) {
	return (vu32*)(WRAM_CACHE_ADRESS_START+slot*_32KB_READ_SIZE);
}

vu8* getCacheAddress(int slot) {
	if(selectedSize==0) {
		return (vu32*)(_128KB_CACHE_ADRESS_START+slot*_128KB_READ_SIZE);
	} else if(selectedSize==1) {
		return (vu32*)(_256KB_CACHE_ADRESS_START+slot*_256KB_READ_SIZE);
	} else if(selectedSize==2) {
		return (vu32*)(_512KB_CACHE_ADRESS_START+slot*_512KB_READ_SIZE);
	} else {
		return (vu32*)(only_CACHE_ADRESS_START+slot*CACHE_READ_SIZE);
	}
}

vu8* GAME_getCacheAddress(int slot) {
	return (vu32*)(GAME_CACHE_ADRESS_START+slot*GAME_READ_SIZE);
}

void transfertToArm7(int slot) {
	*((vu8*)(REG_MBK_WRAM_CACHE_START+slot)) |= 0x1;
}

void transfertToArm9(int slot) {
	*((vu8*)(REG_MBK_WRAM_CACHE_START+slot)) &= 0xFE;
}

void WRAM_updateDescriptor(int slot, u32 sector) {
	WRAM_cacheDescriptor[slot] = sector;
	WRAM_cacheCounter[slot] = WRAM_accessCounter;
}

void updateDescriptor(int slot, u32 sector) {
	if(selectedSize==0) {
		_128KB_cacheDescriptor[slot] = sector;
		_128KB_cacheCounter[slot] = _128KB_accessCounter;
	} else if(selectedSize==1) {
		_256KB_cacheDescriptor[slot] = sector;
		_256KB_cacheCounter[slot] = _256KB_accessCounter;
	} else if(selectedSize==2) {
		_512KB_cacheDescriptor[slot] = sector;
		_512KB_cacheCounter[slot] = _512KB_accessCounter;
	} else {
		only_cacheDescriptor[slot] = sector;
		only_cacheCounter[slot] = only_accessCounter;
	}
}

void accessCounterIncrease() {
	if(selectedSize==0) {
		_128KB_accessCounter++;
	} else if(selectedSize==1) {
		_256KB_accessCounter++;
	} else if(selectedSize==2) {
		_512KB_accessCounter++;
	} else {
		only_accessCounter++;
	}
}

int cardRead (u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");
	
	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;
	u32 commandRead;
	u32 src = cardStruct[0];
	if(src==0) {
		return 0;	// If ROM read location is 0, do not proceed.
	}
	u8* dst = (u8*) (cardStruct[1]);
	u32 len = cardStruct[2];

	u32 page = (src/512)*512;
	
	if(!flagsSet) {
		u32 tempNdsHeader[0x170>>2];

		// read directly at arm7 level
		commandRead = 0x025FFB08;

		sharedAddr[0] = tempNdsHeader;
		sharedAddr[1] = 0x170;
		sharedAddr[2] = 0;
		sharedAddr[3] = commandRead;

		IPC_SendSync(0xEE24);

		while(sharedAddr[3] != (vu32)0);

		ROM_TID = tempNdsHeader[0x00C>>2];
		u32 ROM_HEADERCRC = tempNdsHeader[0x15C>>2];

		// ExceptionHandler2 (red screen) blacklist
		if((ROM_TID & 0x00FFFFFF) != 0x4D5341	// SM64DS
		&& (ROM_TID & 0x00FFFFFF) != 0x443241	// NSMB
		&& (ROM_TID & 0x00FFFFFF) != 0x4D4441)	// AC:WW
		{
			setExceptionHandler2();
		}

		if((ROM_TID & 0x00FFFFFF) == 0x5A3642	// MegaMan Zero Collection
		|| (ROM_TID & 0x00FFFFFF) == 0x583642	// Rockman EXE: Operation Shooting Star
		|| (ROM_TID & 0x00FFFFFF) == 0x323343)	// Ace Attorney Investigations: Miles Edgeworth
		{
			dsiWramUsed = true;
		}

		ARM9_LEN = tempNdsHeader[0x02C>>2];
		// Check ROM size in ROM header...
		romSize = tempNdsHeader[0x080>>2];
		if((romSize & 0x0000000F) == 0x1
		|| (romSize & 0x0000000F) == 0x3
		|| (romSize & 0x0000000F) == 0x5
		|| (romSize & 0x0000000F) == 0x7
		|| (romSize & 0x0000000F) == 0x9
		|| (romSize & 0x0000000F) == 0xB
		|| (romSize & 0x0000000F) == 0xD
		|| (romSize & 0x0000000F) == 0xF)
		{
			romSize--;	// If ROM size is at an odd number, subtract 1 from it.
		}
		romSize -= 0x4000;
		romSize -= ARM9_LEN;

		// If ROM size is 0x01C00000 or below, then the ROM is in RAM.
		if((romSize <= 0x01C00000) && (ROM_TID != 0x45475241) && !dsiWramUsed) {
			if(romSize > 0x01800000 && romSize <= 0x01C00000) {
				use28MB = 1;
				ROM_LOCATION = 0x0E000000-romSize;
				if((ROM_TID & 0x00FFFFFF) == 0x324441	// Nintendogs - Chihuahua & Friends
				|| (ROM_TID & 0x00FFFFFF) == 0x334441	// Nintendogs - Lab & Friends
				|| (ROM_TID & 0x00FFFFFF) == 0x354441	// Nintendogs - Best Friends
				|| (ROM_TID & 0x00FFFFFF) == 0x474441)	// Nintendogs - Dachshund & Friends
				{
					ROM_LOCATION -= 0x20;	// Fix some games white-screening
				}
			}

			ROM_LOCATION -= 0x4000;
			ROM_LOCATION -= ARM9_LEN;

			ROMinRAM = 1;
		} else {
			if((ROM_TID == 0x4A575A41) && (ROM_HEADERCRC == 0x539FCF56)) {		// Sawaru - Made in Wario (J)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AZWJ0[i];

				GAME_CACHE_ADRESS_START = 0x0D680000;
				GAME_CACHE_SLOTS = 0x26;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x4A575A41) && (ROM_HEADERCRC == 0xE37BCF56)) {	// Sawaru - Made in Wario (J) (v02)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AZWJ2[i];

				GAME_CACHE_ADRESS_START = 0x0D680000;
				GAME_CACHE_SLOTS = 0x26;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45575A41) && (ROM_HEADERCRC == 0x7356CF56)) {	// WarioWare: Touched (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AZWE0[i];

				GAME_CACHE_ADRESS_START = 0x0D680000;
				GAME_CACHE_SLOTS = 0x26;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x50575A41) && (ROM_HEADERCRC == 0x8E8FCF56)) {	// WarioWare: Touched (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AZWP0[i];

				GAME_CACHE_ADRESS_START = 0x0D7C0000;
				GAME_CACHE_SLOTS = 0x21;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x43575A41) && (ROM_HEADERCRC == 0xE10BCF56)) {	// Momo Waliou Zhizao (C)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AZWC0[i];

				GAME_CACHE_ADRESS_START = 0x0D740000;
				GAME_CACHE_SLOTS = 0x1F;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x4B575A41) && (ROM_HEADERCRC == 0xB5C6CF56)) {	// Manjyeora! Made in Wario (KS)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AZWK0[i];

				GAME_CACHE_ADRESS_START = 0x0D5C0000;
				GAME_CACHE_SLOTS = 0x29;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x454D4441) && (ROM_HEADERCRC == 0xFEBBCF56)) {	// Animal Crossing: Wild World (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ADME0[i];

				GAME_CACHE_ADRESS_START = 0x0D240000;
				GAME_CACHE_SLOTS = 0x37;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x454D4441) && (ROM_HEADERCRC == 0x5744CF56)) {	// Animal Crossing: Wild World (U) (v01)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ADME1[i];

				GAME_CACHE_ADRESS_START = 0x0D240000;
				GAME_CACHE_SLOTS = 0x37;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45533241) && (ROM_HEADERCRC == 0xA860CF56)) {	// Dragon Ball Z: Supersonic Warriors 2 (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_A2SE0[i];

				GAME_CACHE_ADRESS_START = 0x0D180000;
				GAME_CACHE_SLOTS = 0x13;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45484241) && (ROM_HEADERCRC == 0x3AFCCF56)) {	// Resident Evil: Deadly Silence (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ABHE0[i];

				GAME_CACHE_ADRESS_START = 0x0D300000;
				GAME_CACHE_SLOTS = 0xD;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x455A5241) && (ROM_HEADERCRC == 0xDBFFCF56)) {	// MegaMan ZX (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ARZE0[i];

				GAME_CACHE_ADRESS_START = 0x0D020000;
				GAME_CACHE_SLOTS = 0x7F;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x505A5241) && (ROM_HEADERCRC == 0xBD7ECF56)) {	// MegaMan ZX (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ARZP0[i];

				GAME_CACHE_ADRESS_START = 0x0CFE0000;
				GAME_CACHE_SLOTS = 0x81;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45464641) && (ROM_HEADERCRC == 0x70A6CF56)) {	// Final Fantasy III (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AFFE0[i];

				GAME_CACHE_ADRESS_START = 0x0D000000;
				GAME_CACHE_SLOTS = 0x1F;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x50464641) && (ROM_HEADERCRC == 0x1AE7CF56)) {	// Final Fantasy III (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AFFP0[i];

				GAME_CACHE_ADRESS_START = 0x0D680000;
				GAME_CACHE_SLOTS = 0x15;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x454B4C41) && (ROM_HEADERCRC == 0xB8C7CF56)) {	// Lunar Knights (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ALKE0[i];

				GAME_CACHE_ADRESS_START = 0x0D540000;
				GAME_CACHE_SLOTS = 0x56;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x504B4C41) && (ROM_HEADERCRC == 0x5973CF56)) {	// Lunar Knights (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ALKP0[i];

				GAME_CACHE_ADRESS_START = 0x0D540000;
				GAME_CACHE_SLOTS = 0x56;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			/* } else if((ROM_TID == 0x50514D41) && (ROM_HEADERCRC == 0x9703CF56)) {	// Mario Vs Donkey Kong 2: March of the Minis (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_AMQP0[i];

				GAME_CACHE_ADRESS_START = 0x0D500000;
				GAME_CACHE_SLOTS = 0x2C;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			*/
			} else if((ROM_TID == 0x45424341) && (ROM_HEADERCRC == 0xF10BCF56)) {	// Castlevania: Portrait of Ruin (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ACBE0[i];

				GAME_CACHE_ADRESS_START = 0x0DB40000;
				GAME_CACHE_SLOTS = 0x13;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45414441) && (ROM_HEADERCRC == 0xCA37CF56)		// Pokemon Diamond (U)
					|| (ROM_TID == 0x45415041) && (ROM_HEADERCRC == 0xA80CCF56)) {	// Pokemon Pearl (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_ADAE0[i];

				GAME_CACHE_ADRESS_START = 0x0D1C0000;
				GAME_CACHE_SLOTS = 0x39;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45585A59) && (ROM_HEADERCRC == 0x3057CF56)) {	// MegaMan ZX Advent (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_YZXE0[i];

				GAME_CACHE_ADRESS_START = 0x0D3A0000;
				GAME_CACHE_SLOTS = 0x5D;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x50585A59) && (ROM_HEADERCRC == 0xB2ADCF56)) {	// MegaMan ZX Advent (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_YZXP0[i];

				GAME_CACHE_ADRESS_START = 0x0D3A0000;
				GAME_CACHE_SLOTS = 0x5D;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x45463541) && (ROM_HEADERCRC == 0x9EA0CF56)) {	// Professor Layton and the Curious Village (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_A5FE0[i];

				GAME_CACHE_ADRESS_START = 0x0DD40000;
				GAME_CACHE_SLOTS = 0xB;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			/* } else if((ROM_TID == 0x45344659) && (ROM_HEADERCRC == 0x2635CF56)) {	// Final Fantasy IV (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_YF4E0[i];

				GAME_CACHE_ADRESS_START = 0x0CE80000;
				GAME_CACHE_SLOTS = 0x23;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true;
			} else if((ROM_TID == 0x50344659) && (ROM_HEADERCRC == 0xDB3BCF56)) {	// Final Fantasy IV (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataWhitelist_YF4P0[i];

				GAME_CACHE_ADRESS_START = 0x0CD80000;
				GAME_CACHE_SLOTS = 0x25;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				whitelist = true; */
			} else if((ROM_TID == 0x45525241) && (ROM_HEADERCRC == 0xBE09CF56)) {	// Ridge Racer DS (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ARRE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D380000;
				GAME_CACHE_SLOTS = 0x19;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x454B5341) && (ROM_HEADERCRC == 0xB10BCF56)) {	// Lost in Blue (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ASKE0[i];

				ROM_LOCATION = 0x0C400000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DF40000;
				GAME_CACHE_SLOTS = 0x3;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x45393541) && (ROM_HEADERCRC == 0x5836CF56)) {	// Castlevania: Dawn of Sorrow (U) (Kiosk Demo)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_A59E0[i];

				ROM_LOCATION = 0x0C600000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0C400000;
				GAME_CACHE_SLOTS = 0x1;
				GAME_READ_SIZE = _1MB_READ_SIZE*2;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x45564341) && (ROM_HEADERCRC == 0x0399CF56)) {	// Castlevania: Dawn of Sorrow (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ACVE0[i];

				ROM_LOCATION = 0x0C600000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0C400000;
				GAME_CACHE_SLOTS = 0x1;
				GAME_READ_SIZE = _1MB_READ_SIZE*2;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x50564341) && (ROM_HEADERCRC == 0x7ACDCF56)) {	// Castlevania: Dawn of Sorrow (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ACVP0[i];

				ROM_LOCATION = 0x0C480000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0C400000;
				GAME_CACHE_SLOTS = 0x1;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x45594741) && (ROM_HEADERCRC == 0x9AD6CF56)) {	// Phoenix Wright: Ace Attorney (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AGYE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0CE80000;
				GAME_CACHE_SLOTS = 0x44;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45435341) && (ROM_HEADERCRC == 0xFB0CCF56)) {	// Sonic Rush (J)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ASCJ0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DEC0000;
				GAME_CACHE_SLOTS = 0x5;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45435341) && (ROM_HEADERCRC == 0xFB0CCF56)	// Sonic Rush (U)
					|| (ROM_TID == 0x50435341) && (ROM_HEADERCRC == 0x04C8CF56)) {	// Sonic Rush (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ASCE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DEC0000;
				GAME_CACHE_SLOTS = 0x5;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x454D5241) && (ROM_HEADERCRC == 0x089ECF56)) {	// Mario & Luigi: Partners in Time (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ARME0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA40000;
				GAME_CACHE_SLOTS = 0x17;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x504D5241) && (ROM_HEADERCRC == 0xD0BCCF56)) {	// Mario & Luigi: Partners in Time (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ARMP0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D000000;
				GAME_CACHE_SLOTS = 0x40;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x4A334241) && (ROM_HEADERCRC == 0x0C22CF56)) {	// Mario Basketball: 3 on 3 (J)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AB3J0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D900000;
				GAME_CACHE_SLOTS = 0x7;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45334241) && (ROM_HEADERCRC == 0xE6D9CF56)) {	// Mario Hoops 3 on 3 (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AB3E0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D900000;
				GAME_CACHE_SLOTS = 0x7;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x50334241) && (ROM_HEADERCRC == 0xB642CF56)) {	// Mario Slam Basketball (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[0] = dataBlacklist_AB3P0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DF00000;
				GAME_CACHE_SLOTS = 0x1;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45485041) && (ROM_HEADERCRC == 0xD376CF56)) {	// Pokemon Mystery Dungeon: Blue Rescue Team (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_APHE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D400000;
				GAME_CACHE_SLOTS = 0x18;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45475241) && (ROM_HEADERCRC == 0x5461CF56)) {	// Pokemon Ranger (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ARGE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA80000;
				GAME_CACHE_SLOTS = 0x2C;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45575941) && (ROM_HEADERCRC == 0xA300CF56)
					|| (ROM_TID == 0x45575941) && (ROM_HEADERCRC == 0xFA95CF56)) {	// Yoshi's Island DS (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AYWE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DEC0000;
				GAME_CACHE_SLOTS = 0x5;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x4A4E4441) && (ROM_HEADERCRC == 0x462DCF56)) {	// Digimon Story (J)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ADNJ0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DEC0000;
				GAME_CACHE_SLOTS = 0x5;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x454E4441) && (ROM_HEADERCRC == 0xAC46CF56)) {	// Digimon World DS (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_ADNE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA00000;
				GAME_CACHE_SLOTS = 0x18;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45574B41) && (ROM_HEADERCRC == 0xC8C3CF56)) {	// Kirby Squeak Squad (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AKWE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DBA0000;
				GAME_CACHE_SLOTS = 0x22;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x50574B41) && (ROM_HEADERCRC == 0x706CCF56)) {	// Kirby Mouse Attack (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AKWP0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DC00000;
				GAME_CACHE_SLOTS = 0x1F;
				GAME_READ_SIZE = _128KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x4A593341) && (ROM_HEADERCRC == 0x77E4CF56)) {	// Sonic Rush Adventure (J)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_A3YJ0[i];

				ROM_LOCATION = 0x0C700000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DE00000;
				GAME_CACHE_SLOTS = 0x4;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45593341) && (ROM_HEADERCRC == 0x7A5ACF56)	// Sonic Rush Adventure (U)
					|| (ROM_TID == 0x50593341) && (ROM_HEADERCRC == 0xB96BCF56)	// Sonic Rush Adventure (E)
					|| (ROM_TID == 0x50593341) && (ROM_HEADERCRC == 0xD1B2CF56)) {	// Sonic Rush Adventure (E) (v01)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_A3YE0[i];

				ROM_LOCATION = 0x0C700000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DE00000;
				GAME_CACHE_SLOTS = 0x4;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x4B593341) && (ROM_HEADERCRC == 0x3DF8CF56)) {	// Sonic Rush Adventure (KS)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_A3YK0[i];

				ROM_LOCATION = 0x0C700000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DC80000;
				GAME_CACHE_SLOTS = 0x7;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45575641) && (ROM_HEADERCRC == 0x1652CF56)	// Miami Nights - Singles in the City (U)
					|| (ROM_TID == 0x50575641) && (ROM_HEADERCRC == 0x8329CF56)) {	// Miami Nights - Singles in the City (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_AVWE0[i];

				ROM_LOCATION = 0x0C680000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0C400000;
				GAME_CACHE_SLOTS = 0x5;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x454F4359) && (ROM_HEADERCRC == 0x7591CF56)) {	// Call of Duty 4: Modern Warfare (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_YCOE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0CF00000;
				GAME_CACHE_SLOTS = 0x22;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45325759) && (ROM_HEADERCRC == 0xD1EBCF56)) {	// Advance Wars: Days of Ruin (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_YW2E0[i];

				ROM_LOCATION = 0x0C400000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA00000;
				GAME_CACHE_SLOTS = 0x6;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x50325759) && (ROM_HEADERCRC == 0x8CA3CF56)) {	// Advance Wars: Dark Conflict (E)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_YW2P0[i];

				ROM_LOCATION = 0x0C400000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA00000;
				GAME_CACHE_SLOTS = 0x6;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x45574B59) && (ROM_HEADERCRC == 0x317DCF56)) {	// Kirby Super Star Ultra (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_YKWE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA00000;
				GAME_CACHE_SLOTS = 0x6;
				GAME_READ_SIZE = _1MB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45434A43) && (ROM_HEADERCRC == 0xF0BACF56)) {	// My Japanese Coach: Learn a New Language (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_CJCE0[i];

				ROM_LOCATION = 0x0C400000;
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0DA00000;
				GAME_CACHE_SLOTS = 0x30;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
				use28MB = 1;
			} else if((ROM_TID == 0x454A4C43) && (ROM_HEADERCRC == 0xCE77CF56)
					|| (ROM_TID == 0x454A4C43) && (ROM_HEADERCRC == 0x8F73CF56)) {	// Mario & Luigi: Bowser's Inside Story (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_CLJE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D800000;
				GAME_CACHE_SLOTS = 0x20;
				GAME_READ_SIZE = _256KB_READ_SIZE;

				ROMinRAM = 2;
			} else if((ROM_TID == 0x45494B42) && (ROM_HEADERCRC == 0xE25BCF56)) {	// The Legend of Zelda: Spirit Tracks (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_BKIE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;

				GAME_CACHE_ADRESS_START = 0x0D980000;
				GAME_CACHE_SLOTS = 0xD;
				GAME_READ_SIZE = _512KB_READ_SIZE;

				ROMinRAM = 2;
			} /* else if((ROM_TID == 0x455A3642) && (ROM_HEADERCRC == 0x0026CF56)) {	// MegaMan Zero Collection (U)
				for(int i = 0; i < 3; i++)
					setDataBWlist[i] = dataBlacklist_B6ZE0[i];

				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;
				
				GAME_READ_SIZE = _32KB_READ_SIZE;

				ROMinRAM = 2;
				dsiWramUsed = true;
			} */
		}
		flagsSet = true;
	}

	#ifdef DEBUG
	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	IPC_SendSync(0xEE24);

	while(sharedAddr[3] != (vu32)0);
	// -------------------------------------*/
	#endif
	
	
	if(!dsiWramUsed) {
		if(!cacheSizeSet && ROMinRAM==0) {
			if((ROM_TID & 0x00FFFFFF) == 0x593341)	// Sonic Rush Adventure
			{
				CACHE_READ_SIZE = _1MB_READ_SIZE;
				only_cacheSlots = only_1MB_CACHE_SLOTS;
			} else if((ROM_TID & 0x00FFFFFF) == 0x414441	// PKMN Diamond
					|| (ROM_TID & 0x00FFFFFF) == 0x415041	// PKMN Pearl
					|| (ROM_TID & 0x00FFFFFF) == 0x555043	// PKMN Platinum
					|| (ROM_TID & 0x00FFFFFF) == 0x4B5049	// PKMN HG
					|| (ROM_TID & 0x00FFFFFF) == 0x475049	// PKMN SS
					|| (ROM_TID & 0x00FFFFFF) == 0x4D5241	// Mario & Luigi: Partners in Time
					|| (ROM_TID & 0x00FFFFFF) == 0x575941)	// Yoshi's Island DS
			{
				CACHE_READ_SIZE = _256KB_READ_SIZE;
				only_cacheSlots = only_256KB_CACHE_SLOTS;
			} else if((ROM_TID & 0x00FFFFFF) == 0x4B4C41)	// Lunar Knights
			{
				CACHE_READ_SIZE = _192KB_READ_SIZE;
				only_cacheSlots = only_192KB_CACHE_SLOTS;
			} else {
				dynamicCaching = true;
			}
			cacheSizeSet = true;
		}
		if(dynamicCaching) {
			selectedSize = 2;
			CACHE_READ_SIZE = _512KB_READ_SIZE;
			if(len <= _128KB_READ_SIZE) {
				selectedSize = 0;
				CACHE_READ_SIZE = _128KB_READ_SIZE;
			}
			if(len <= _256KB_READ_SIZE) {
				selectedSize = 1;
				CACHE_READ_SIZE = _256KB_READ_SIZE;
			}
		}
	} else {
		CACHE_READ_SIZE = _32KB_READ_SIZE;
	}
	
	if(ROMinRAM==0) {
		if(dsiWramUsed) WRAM_accessCounter++;
		else accessCounterIncrease();
	}

	u32 sector = (src/CACHE_READ_SIZE)*CACHE_READ_SIZE;

	if(ROMinRAM==0 && page == src && len > CACHE_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
		// read directly at arm7 level
		commandRead = 0x025FFB08;

		cacheFlush();

		sharedAddr[0] = dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		IPC_SendSync(0xEE24);

		while(sharedAddr[3] != (vu32)0);

	} else {
		// read via the main RAM cache
		while(len > 0) {
			if(ROMinRAM==0) {
				int slot = 0;
				vu8* buffer = 0;
				if(dsiWramUsed) {
					slot = WRAM_getSlotForSector(sector);
					buffer = WRAM_getCacheAddress(slot);
				} else {
					slot = getSlotForSector(sector);
					buffer = getCacheAddress(slot);
				}
				// read max CACHE_READ_SIZE via the main RAM cache
				if(slot==-1) {
					// send a command to the arm7 to fill the RAM cache
					commandRead = 0x025FFB08;

					if(dsiWramUsed) {
						slot = WRAM_allocateCacheSlot();
						
						buffer = WRAM_getCacheAddress(slot);
					} else {
						slot = allocateCacheSlot();
						
						buffer = getCacheAddress(slot);
					}

					if(!dsiWramUsed) REG_SCFG_EXT = 0x8300C000;

					if(needFlushDCCache) DC_FlushRange(buffer, CACHE_READ_SIZE);

					// transfer the WRAM-B cache to the arm7
					if(dsiWramUsed) transfertToArm7(slot);				

					// write the command
					sharedAddr[0] = buffer;
					sharedAddr[1] = CACHE_READ_SIZE;
					sharedAddr[2] = sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr[3] != (vu32)0);

					// transfer back the WRAM-B cache to the arm9
					if(dsiWramUsed) transfertToArm9(slot);

					if(!dsiWramUsed) REG_SCFG_EXT = 0x83000000;
				}

				if(dsiWramUsed) {
					WRAM_updateDescriptor(slot, sector);
				} else {
					updateDescriptor(slot, sector);
				}

				u32 len2=len;
				if((src - sector) + len2 > CACHE_READ_SIZE){
					len2 = sector - src + CACHE_READ_SIZE;
				}

				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = buffer+src-sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr[3] != (vu32)0);
					// -------------------------------------*/
					#endif

					// copy directly
					if(!dsiWramUsed) REG_SCFG_EXT = 0x8300C000;
					fastCopy32(buffer+(src-sector),dst,len2);
					if(!dsiWramUsed) REG_SCFG_EXT = 0x83000000;

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = buffer+page-sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr[3] != (vu32)0);
					// -------------------------------------*/
					#endif

					// read via the 512b ram cache
					if(!dsiWramUsed) REG_SCFG_EXT = 0x8300C000;
					fastCopy32(buffer+(page-sector), cacheBuffer, 512);
					if(!dsiWramUsed) REG_SCFG_EXT = 0x83000000;
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
					sector = (src/CACHE_READ_SIZE)*CACHE_READ_SIZE;
					if(dsiWramUsed) WRAM_accessCounter++;
					else accessCounterIncrease();
				}
			} else if (ROMinRAM==1) {
				// Prevent overwriting ROM in RAM
				if(dst > 0x02400000 && dst < 0x02800000) {
					if(use28MB==2) {
						return 0;	// Reject data from being loaded into debug 4MB area
					} else if(use28MB==1) {
						dst -= 0x00400000;
					}
				}

				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr[3] != (vu32)0);
					// -------------------------------------*/
					#endif

					// read ROM loaded into RAM
					REG_SCFG_EXT = 0x8300C000;
					fastCopy32(ROM_LOCATION+src,dst,len2);
					REG_SCFG_EXT = 0x83000000;

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr[3] != (vu32)0);
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					REG_SCFG_EXT = 0x8300C000;
					fastCopy32(ROM_LOCATION+page, cacheBuffer, 512);
					REG_SCFG_EXT = 0x83000000;
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			} else if (ROMinRAM==2) {
				if(dst > 0x02400000 && dst < 0x02800000) {
					if(use28MB==2) {
						return 0;	// Reject data from being loaded into debug 4MB area
					} else if(use28MB==1) {
						dst -= 0x00400000;
					}
				}

				if(whitelist && src >= setDataBWlist[0] && src < setDataBWlist[1]) {
					// if(src >= setDataBWlist[0] && src < setDataBWlist[1]) {
						u32 src2=src;
						src2 -= setDataBWlist[0];
						u32 page2=page;
						page2 -= setDataBWlist[0];

						u32 len2=len;
						if(len2 > 512) {
							len2 -= src%4;
							len2 -= len2 % 32;
						}

						if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
							#ifdef DEBUG
							// send a log command for debug purpose
							// -------------------------------------
							commandRead = 0x026ff800;

							sharedAddr[0] = dst;
							sharedAddr[1] = len2;
							sharedAddr[2] = ROM_LOCATION+src2;
							sharedAddr[3] = commandRead;

							IPC_SendSync(0xEE24);

							while(sharedAddr[3] != (vu32)0);
							// -------------------------------------
							#endif

							// read ROM loaded into RAM
							REG_SCFG_EXT = 0x8300C000;
							fastCopy32(ROM_LOCATION+src2,dst,len2);
							REG_SCFG_EXT = 0x83000000;

							// update cardi common
							cardStruct[0] = src + len2;
							cardStruct[1] = dst + len2;
							cardStruct[2] = len - len2;
						} else {
							#ifdef DEBUG
							// send a log command for debug purpose
							// -------------------------------------
							commandRead = 0x026ff800;

							sharedAddr[0] = page;
							sharedAddr[1] = len2;
							sharedAddr[2] = ROM_LOCATION+page2;
							sharedAddr[3] = commandRead;

							IPC_SendSync(0xEE24);

							while(sharedAddr[3] != (vu32)0);
							// -------------------------------------
							#endif

							// read via the 512b ram cache
							REG_SCFG_EXT = 0x8300C000;
							fastCopy32(ROM_LOCATION+page2, cacheBuffer, 512);
							REG_SCFG_EXT = 0x83000000;
							*cachePage = page;
							(*readCachedRef)(cacheStruct);
						}
						len = cardStruct[2];
						if(len>0) {
							src = cardStruct[0];
							dst = cardStruct[1];
							page = (src/512)*512;
						}
					// }
					/* if(dataAmount >= 1 && src >= setDataBWlist_1[0] && src < setDataBWlist_1[1]) {
						u32 src2=src;
						src2 -= setDataBWlist_1[0];
						src2 += setDataBWlist[2];
						u32 page2=page;
						page2 -= setDataBWlist_1[0];
						page2 += setDataBWlist[2];

						u32 len2=len;
						if(len2 > 512) {
							len2 -= src%4;
							len2 -= len2 % 32;
						}

						if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
							#ifdef DEBUG
							// send a log command for debug purpose
							// -------------------------------------
							commandRead = 0x026ff800;

							sharedAddr[0] = dst;
							sharedAddr[1] = len2;
							sharedAddr[2] = ROM_LOCATION+src2;
							sharedAddr[3] = commandRead;

							IPC_SendSync(0xEE24);

							while(sharedAddr[3] != (vu32)0);
							// -------------------------------------
							#endif

							// read ROM loaded into RAM
							REG_SCFG_EXT = 0x8300C000;
							fastCopy32(ROM_LOCATION+src2,dst,len2);
							REG_SCFG_EXT = 0x83000000;

							// update cardi common
							cardStruct[0] = src + len2;
							cardStruct[1] = dst + len2;
							cardStruct[2] = len - len2;
						} else {
							#ifdef DEBUG
							// send a log command for debug purpose
							// -------------------------------------
							commandRead = 0x026ff800;

							sharedAddr[0] = page;
							sharedAddr[1] = len2;
							sharedAddr[2] = ROM_LOCATION+page2;
							sharedAddr[3] = commandRead;

							IPC_SendSync(0xEE24);

							while(sharedAddr[3] != (vu32)0);
							// -------------------------------------
							#endif

							// read via the 512b ram cache
							REG_SCFG_EXT = 0x8300C000;
							fastCopy32(ROM_LOCATION+page2, cacheBuffer, 512);
							REG_SCFG_EXT = 0x83000000;
							*cachePage = page;
							(*readCachedRef)(cacheStruct);
						}
						len = cardStruct[2];
						if(len>0) {
							src = cardStruct[0];
							dst = cardStruct[1];
							page = (src/512)*512;
						}
					}
					if(dataAmount == 2 && src >= setDataBWlist_2[0] && src < setDataBWlist_2[1]) {
						u32 src2=src;
						src2 -= setDataBWlist_2[0];
						src2 += setDataBWlist[2];
						src2 += setDataBWlist_1[2];
						u32 page2=page;
						page2 -= setDataBWlist_2[0];
						page2 += setDataBWlist[2];
						page2 += setDataBWlist_1[2];

						u32 len2=len;
						if(len2 > 512) {
							len2 -= src%4;
							len2 -= len2 % 32;
						}

						if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
							#ifdef DEBUG
							// send a log command for debug purpose
							// -------------------------------------
							commandRead = 0x026ff800;

							sharedAddr[0] = dst;
							sharedAddr[1] = len2;
							sharedAddr[2] = ROM_LOCATION+src2;
							sharedAddr[3] = commandRead;

							IPC_SendSync(0xEE24);

							while(sharedAddr[3] != (vu32)0);
							// -------------------------------------
							#endif

							// read ROM loaded into RAM
							REG_SCFG_EXT = 0x8300C000;
							fastCopy32(ROM_LOCATION+src2,dst,len2);
							REG_SCFG_EXT = 0x83000000;

							// update cardi common
							cardStruct[0] = src + len2;
							cardStruct[1] = dst + len2;
							cardStruct[2] = len - len2;
						} else {
							#ifdef DEBUG
							// send a log command for debug purpose
							// -------------------------------------
							commandRead = 0x026ff800;

							sharedAddr[0] = page;
							sharedAddr[1] = len2;
							sharedAddr[2] = ROM_LOCATION+page2;
							sharedAddr[3] = commandRead;

							IPC_SendSync(0xEE24);

							while(sharedAddr[3] != (vu32)0);
							// -------------------------------------
							#endif

							// read via the 512b ram cache
							REG_SCFG_EXT = 0x8300C000;
							fastCopy32(ROM_LOCATION+page2, cacheBuffer, 512);
							REG_SCFG_EXT = 0x83000000;
							*cachePage = page;
							(*readCachedRef)(cacheStruct);
						}
						len = cardStruct[2];
						if(len>0) {
							src = cardStruct[0];
							dst = cardStruct[1];
							page = (src/512)*512;
						}
					} */
				} else if(!whitelist && src > 0 && src < setDataBWlist[0]) {
					u32 len2=len;
					if(len2 > 512) {
						len2 -= src%4;
						len2 -= len2 % 32;
					}

					if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
						#ifdef DEBUG
						// send a log command for debug purpose
						// -------------------------------------
						commandRead = 0x026ff800;

						sharedAddr[0] = dst;
						sharedAddr[1] = len2;
						sharedAddr[2] = ROM_LOCATION+src;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);
						// -------------------------------------*/
						#endif

						// read ROM loaded into RAM
						REG_SCFG_EXT = 0x8300C000;
						fastCopy32(ROM_LOCATION+src,dst,len2);
						REG_SCFG_EXT = 0x83000000;

						// update cardi common
						cardStruct[0] = src + len2;
						cardStruct[1] = dst + len2;
						cardStruct[2] = len - len2;
					} else {
						#ifdef DEBUG
						// send a log command for debug purpose
						// -------------------------------------
						commandRead = 0x026ff800;

						sharedAddr[0] = page;
						sharedAddr[1] = len2;
						sharedAddr[2] = ROM_LOCATION+page;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);
						// -------------------------------------
						#endif

						// read via the 512b ram cache
						REG_SCFG_EXT = 0x8300C000;
						fastCopy32(ROM_LOCATION+page, cacheBuffer, 512);
						REG_SCFG_EXT = 0x83000000;
						*cachePage = page;
						(*readCachedRef)(cacheStruct);
					}
					len = cardStruct[2];
					if(len>0) {
						src = cardStruct[0];
						dst = cardStruct[1];
						page = (src/512)*512;
					}
					sharedAddr[3] = 0;
				} else if(!whitelist && src >= setDataBWlist[1] && src < romSize) {
					u32 len2=len;
					if(len2 > 512) {
						len2 -= src%4;
						len2 -= len2 % 32;
					}

					if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
						#ifdef DEBUG
						// send a log command for debug purpose
						// -------------------------------------
						commandRead = 0x026ff800;

						sharedAddr[0] = dst;
						sharedAddr[1] = len2;
						sharedAddr[2] = ROM_LOCATION-setDataBWlist[2]+src;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);
						// -------------------------------------*/
						#endif

						// read ROM loaded into RAM
						REG_SCFG_EXT = 0x8300C000;
						fastCopy32(ROM_LOCATION-setDataBWlist[2]+src,dst,len2);
						REG_SCFG_EXT = 0x83000000;

						// update cardi common
						cardStruct[0] = src + len2;
						cardStruct[1] = dst + len2;
						cardStruct[2] = len - len2;
					} else {
						#ifdef DEBUG
						// send a log command for debug purpose
						// -------------------------------------
						commandRead = 0x026ff800;

						sharedAddr[0] = page;
						sharedAddr[1] = len2;
						sharedAddr[2] = ROM_LOCATION-setDataBWlist[2]+page;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);
						// -------------------------------------
						#endif

						// read via the 512b ram cache
						REG_SCFG_EXT = 0x8300C000;
						fastCopy32(ROM_LOCATION-setDataBWlist[2]+page, cacheBuffer, 512);
						REG_SCFG_EXT = 0x83000000;
						*cachePage = page;
						(*readCachedRef)(cacheStruct);
					}
					len = cardStruct[2];
					if(len>0) {
						src = cardStruct[0];
						dst = cardStruct[1];
						page = (src/512)*512;
					}
					sharedAddr[3] = 0;
				} else if(page == src && len > GAME_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
					if(dsiWramUsed) WRAM_accessCounter++;
					else only_accessCounter++;

					// read directly at arm7 level
					commandRead = 0x025FFB08;

					cacheFlush();

					sharedAddr[0] = dst;
					sharedAddr[1] = len;
					sharedAddr[2] = src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr[3] != (vu32)0);
				} else {
					if(dsiWramUsed) WRAM_accessCounter++;
					else only_accessCounter++;

					u32 sector = (src/GAME_READ_SIZE)*GAME_READ_SIZE;

					int slot = 0;
					vu8* buffer = 0;
					if(dsiWramUsed) {
						slot = WRAM_getSlotForSector(sector);
						buffer = WRAM_getCacheAddress(slot);
					} else {
						slot = GAME_getSlotForSector(sector);
						buffer = GAME_getCacheAddress(slot);
					}
					// read max CACHE_READ_SIZE via the main RAM cache
					if(slot==-1) {
						// send a command to the arm7 to fill the RAM cache
						commandRead = 0x025FFB08;

						if(dsiWramUsed) {
							slot = WRAM_allocateCacheSlot();
							
							buffer = WRAM_getCacheAddress(slot);
						} else {
							slot = GAME_allocateCacheSlot();
							
							buffer = GAME_getCacheAddress(slot);
						}

						if(!dsiWramUsed) REG_SCFG_EXT = 0x8300C000;

						if(needFlushDCCache) DC_FlushRange(buffer, GAME_READ_SIZE);

						// transfer the WRAM-B cache to the arm7
						if(dsiWramUsed) transfertToArm7(slot);				

						// write the command
						sharedAddr[0] = buffer;
						sharedAddr[1] = GAME_READ_SIZE;
						sharedAddr[2] = sector;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);

						// transfer back the WRAM-B cache to the arm9
						if(dsiWramUsed) transfertToArm9(slot);

						if(!dsiWramUsed) REG_SCFG_EXT = 0x83000000;
					}

					if(dsiWramUsed) {
						WRAM_updateDescriptor(slot, sector);
					} else {
						updateDescriptor(slot, sector);
					}

					u32 len2=len;
					if((src - sector) + len2 > GAME_READ_SIZE){
						len2 = sector - src + GAME_READ_SIZE;
					}

					if(len2 > 512) {
						len2 -= src%4;
						len2 -= len2 % 32;
					}

					if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
						#ifdef DEBUG
						// send a log command for debug purpose
						// -------------------------------------
						commandRead = 0x026ff800;

						sharedAddr[0] = dst;
						sharedAddr[1] = len2;
						sharedAddr[2] = buffer+src-sector;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);
						// -------------------------------------*/
						#endif

						// copy directly
						if(!dsiWramUsed) REG_SCFG_EXT = 0x8300C000;
						fastCopy32(buffer+(src-sector),dst,len2);
						if(!dsiWramUsed) REG_SCFG_EXT = 0x83000000;

						// update cardi common
						cardStruct[0] = src + len2;
						cardStruct[1] = dst + len2;
						cardStruct[2] = len - len2;
					} else {
						#ifdef DEBUG
						// send a log command for debug purpose
						// -------------------------------------
						commandRead = 0x026ff800;

						sharedAddr[0] = page;
						sharedAddr[1] = len2;
						sharedAddr[2] = buffer+page-sector;
						sharedAddr[3] = commandRead;

						IPC_SendSync(0xEE24);

						while(sharedAddr[3] != (vu32)0);
						// -------------------------------------*/
						#endif

						// read via the 512b ram cache
						if(!dsiWramUsed) REG_SCFG_EXT = 0x8300C000;
						fastCopy32(buffer+(page-sector), cacheBuffer, 512);
						if(!dsiWramUsed) REG_SCFG_EXT = 0x83000000;
						*cachePage = page;
						(*readCachedRef)(cacheStruct);
					}
					len = cardStruct[2];
					if(len>0) {
						src = cardStruct[0];
						dst = cardStruct[1];
						page = (src/512)*512;
						sector = (src/GAME_READ_SIZE)*GAME_READ_SIZE;
						if(!dsiWramUsed) WRAM_accessCounter++;
						else only_accessCounter++;
					}
				}
			}
		}
	}
	return 0;
}




