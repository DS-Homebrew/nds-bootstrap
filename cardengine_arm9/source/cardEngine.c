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

static u32 ROM_LOCATION = 0x0C800000;
static u32 ROM_TID;

#define _32KB_READ_SIZE 0x8000
#define _64KB_READ_SIZE 0x10000
#define _128KB_READ_SIZE 0x20000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000

#define REG_MBK_WRAM_CACHE_START	0x4004045
#define WRAM_CACHE_ADRESS_START 0x03708000
#define WRAM_CACHE_ADRESS_END 0x03778000
#define WRAM_CACHE_ADRESS_SIZE 0x78000
#define WRAM_CACHE_SLOTS 15

#define _64KB_CACHE_ADRESS_START 0x0C800000
#define _64KB_CACHE_ADRESS_SIZE 0x200000
#define _64KB_CACHE_SLOTS 0x20
#define _128KB_CACHE_ADRESS_START 0x0CA00000
#define _128KB_CACHE_ADRESS_SIZE 0x400000
#define _128KB_CACHE_SLOTS 0x20
#define _256KB_CACHE_ADRESS_START 0x0CE00000
#define _256KB_CACHE_ADRESS_SIZE 0x800000
#define _256KB_CACHE_SLOTS 0x20
#define _512KB_CACHE_ADRESS_START 0x0D600000
#define _512KB_CACHE_ADRESS_SIZE 0xA00000
#define _512KB_CACHE_SLOTS 0x14

#define only_CACHE_ADRESS_START 0x0C800000
#define only_CACHE_ADRESS_SIZE 0x1800000
#define only_256KB_CACHE_SLOTS 0x60
#define only_512KB_CACHE_SLOTS 0x30

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr_ROMinRAM = (vu32*)0x02400008;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 WRAM_cacheDescriptor [WRAM_CACHE_SLOTS] = {0xffffffff};
static u32 WRAM_cacheCounter [WRAM_CACHE_SLOTS];
static u32 _64KB_cacheDescriptor [_64KB_CACHE_SLOTS] = {0xffffffff};
static u32 _64KB_cacheCounter [_64KB_CACHE_SLOTS];
static u32 _128KB_cacheDescriptor [_128KB_CACHE_SLOTS] = {0xffffffff};
static u32 _128KB_cacheCounter [_128KB_CACHE_SLOTS];
static u32 _256KB_cacheDescriptor [_256KB_CACHE_SLOTS] = {0xffffffff};
static u32 _256KB_cacheCounter [_256KB_CACHE_SLOTS];
static u32 _512KB_cacheDescriptor [_512KB_CACHE_SLOTS] = {0xffffffff};
static u32 _512KB_cacheCounter [_512KB_CACHE_SLOTS];
static u32 only_256KB_cacheDescriptor [only_256KB_CACHE_SLOTS] = {0xffffffff};
static u32 only_256KB_cacheCounter [only_256KB_CACHE_SLOTS];
static u32 only_512KB_cacheDescriptor [only_512KB_CACHE_SLOTS] = {0xffffffff};
static u32 only_512KB_cacheCounter [only_512KB_CACHE_SLOTS];
static u32 WRAM_accessCounter = 0;
static u32 _64KB_accessCounter = 0;
static u32 _128KB_accessCounter = 0;
static u32 _256KB_accessCounter = 0;
static u32 _512KB_accessCounter = 0;
static u32 only_accessCounter = 0;

static int selectedSize = 0;

static bool flagsSet = false;
static bool ROMinRAM = false;
static bool dsiWramUsed = false;

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
	switch(selectedSize) {
		case 0:
		default:
			break;
		case 1:
			lowerCounter = _64KB_accessCounter;
			for(int i=0; i<_64KB_CACHE_SLOTS; i++) {
				if(_64KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = _64KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			break;
		case 2:
			lowerCounter = _128KB_accessCounter;
			for(int i=0; i<_128KB_CACHE_SLOTS; i++) {
				if(_128KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = _128KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			break;
		case 3:
			lowerCounter = _256KB_accessCounter;
			for(int i=0; i<_256KB_CACHE_SLOTS; i++) {
				if(_256KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = _256KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			break;
		case 4:
			lowerCounter = _512KB_accessCounter;
			for(int i=0; i<_512KB_CACHE_SLOTS; i++) {
				if(_512KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = _512KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			break;
		case 13:
			lowerCounter = only_accessCounter;
			for(int i=0; i<only_256KB_CACHE_SLOTS; i++) {
				if(only_256KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = only_256KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			break;
		case 14:
			lowerCounter = only_accessCounter;
			for(int i=0; i<only_512KB_CACHE_SLOTS; i++) {
				if(only_512KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = only_512KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			break;
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
	switch(selectedSize) {
		case 0:
		default:
			break;
		case 1:
			for(int i=0; i<_64KB_CACHE_SLOTS; i++) {
				if(_64KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
			break;
		case 2:
			for(int i=0; i<_128KB_CACHE_SLOTS; i++) {
				if(_128KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
			break;
		case 3:
			for(int i=0; i<_256KB_CACHE_SLOTS; i++) {
				if(_256KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
			break;
		case 4:
			for(int i=0; i<_512KB_CACHE_SLOTS; i++) {
				if(_512KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
			break;
		case 13:
			for(int i=0; i<only_256KB_CACHE_SLOTS; i++) {
				if(only_256KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
			break;
		case 14:
			for(int i=0; i<only_512KB_CACHE_SLOTS; i++) {
				if(only_512KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
			break;
	}
	return -1;
}


vu8* WRAM_getCacheAddress(int slot) {
	return (vu32*)(WRAM_CACHE_ADRESS_START+slot*_32KB_READ_SIZE);
}

vu8* getCacheAddress(int slot) {
	switch(selectedSize) {
		case 0:
		default:
			break;
		case 1:
			return (vu32*)(_64KB_CACHE_ADRESS_START+slot*_64KB_READ_SIZE);
			break;
		case 2:
			return (vu32*)(_128KB_CACHE_ADRESS_START+slot*_128KB_READ_SIZE);
			break;
		case 3:
			return (vu32*)(_256KB_CACHE_ADRESS_START+slot*_256KB_READ_SIZE);
			break;
		case 4:
			return (vu32*)(_512KB_CACHE_ADRESS_START+slot*_512KB_READ_SIZE);
			break;
		case 13:
			return (vu32*)(only_CACHE_ADRESS_START+slot*_256KB_READ_SIZE);
			break;
		case 14:
			return (vu32*)(only_CACHE_ADRESS_START+slot*_512KB_READ_SIZE);
			break;
	}
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
	switch(selectedSize) {
		case 0:
		default:
			break;
		case 1:
			_64KB_cacheDescriptor[slot] = sector;
			_64KB_cacheCounter[slot] = _64KB_accessCounter;
			break;
		case 2:
			_128KB_cacheDescriptor[slot] = sector;
			_128KB_cacheCounter[slot] = _128KB_accessCounter;
			break;
		case 3:
			_256KB_cacheDescriptor[slot] = sector;
			_256KB_cacheCounter[slot] = _256KB_accessCounter;
			break;
		case 4:
			_512KB_cacheDescriptor[slot] = sector;
			_512KB_cacheCounter[slot] = _512KB_accessCounter;
			break;
		case 13:
			only_256KB_cacheDescriptor[slot] = sector;
			only_256KB_cacheCounter[slot] = only_accessCounter;
			break;
		case 14:
			only_512KB_cacheDescriptor[slot] = sector;
			only_512KB_cacheCounter[slot] = only_accessCounter;
			break;
	}
}

void accessCounterIncrease() {
	switch(selectedSize) {
		case 0:
		default:
			break;
		case 1:
			_64KB_accessCounter++;
			break;
		case 2:
			_128KB_accessCounter++;
			break;
		case 3:
			_256KB_accessCounter++;
			break;
		case 4:
			_512KB_accessCounter++;
			break;
		case 13:
		case 14:
			only_accessCounter++;
			break;
	}
}

int cardRead (u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");
	
	setExceptionHandler2();
	
	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;
	u32 commandRead;
	u32 src = cardStruct[0];
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
		
		if((ROM_TID & 0x00FFFFFF) == 0x5A3642	// MegaMan Zero Collection
		|| (ROM_TID & 0x00FFFFFF) == 0x494B42	// Zelda: Spirit Tracks
		|| (ROM_TID & 0x00FFFFFF) == 0x323343)	// Ace Attorney Investigations: Miles Edgeworth
		{
			dsiWramUsed = true;
		}

		u32 ARM9_LEN = tempNdsHeader[0x02C>>2];
		// Check ROM size in ROM header...
		u32 romSize = tempNdsHeader[0x080>>2];
		romSize -= 0x4000;
		romSize -= ARM9_LEN;
		
		if(romSize > 0x01800000 && romSize <= 0x01C00000) {
			ROM_LOCATION = 0x0E000000-romSize;
			if((ROM_TID & 0x00FFFFFF) == 0x474441) {
				ROM_LOCATION = 0x0DFFFFE0-romSize;	// Fix for Nintendogs - Dachshund & Friends
			}
		}

		// If ROM size is 0x01C00000 or below, then load the ROM into RAM.
		if(romSize <= 0x01C00000) {
			// read directly at arm7 level
			commandRead = 0x025FFB08;

			REG_SCFG_EXT = 0x8300C000;

			sharedAddr_ROMinRAM[0] = ROM_LOCATION;
			sharedAddr_ROMinRAM[1] = romSize;
			sharedAddr_ROMinRAM[2] = 0x4000+ARM9_LEN;
			sharedAddr_ROMinRAM[3] = commandRead;

			IPC_SendSync(0xEE24);

			while(sharedAddr_ROMinRAM[3] != (vu32)0);

			REG_SCFG_EXT = 0x83000000;

			ROM_LOCATION -= 0x4000;
			ROM_LOCATION -= ARM9_LEN;

			ROMinRAM = true;
		}
		flagsSet = true;
	}

	#ifdef DEBUG
	if(ROMinRAM) {
		REG_SCFG_EXT = 0x8300C000;

		// send a log command for debug purpose
		// -------------------------------------
		commandRead = 0x026ff800;

		sharedAddr_ROMinRAM[0] = dst;
		sharedAddr_ROMinRAM[1] = len;
		sharedAddr_ROMinRAM[2] = src;
		sharedAddr_ROMinRAM[3] = commandRead;

		IPC_SendSync(0xEE24);

		while(sharedAddr_ROMinRAM[3] != (vu32)0);
		// -------------------------------------*/

		REG_SCFG_EXT = 0x83000000;
	} else {
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
	}
	#endif
	
	
	selectedSize = 4;
	u32 CACHE_READ_SIZE = _512KB_READ_SIZE;
	if(!dsiWramUsed) {
		if((ROM_TID & 0x00FFFFFF) == 0x593341
		|| (ROM_TID & 0x00FFFFFF) == 0x414441	// Diamond
		|| (ROM_TID & 0x00FFFFFF) == 0x415041	// Pearl
		|| (ROM_TID & 0x00FFFFFF) == 0x555043	// Platinum
		|| (ROM_TID & 0x00FFFFFF) == 0x4B5049	// HG
		|| (ROM_TID & 0x00FFFFFF) == 0x475049)	// SS
		{
			selectedSize = 14;
		} else if((ROM_TID & 0x00FFFFFF) == 0x4D5241) {
			selectedSize = 13;
			CACHE_READ_SIZE = _256KB_READ_SIZE;
		} else {
			if(len <= _64KB_READ_SIZE) {
				selectedSize = 1;
				CACHE_READ_SIZE = _64KB_READ_SIZE;
			}
			if(len <= _128KB_READ_SIZE) {
				selectedSize = 2;
				CACHE_READ_SIZE = _128KB_READ_SIZE;
			}
			if(len <= _256KB_READ_SIZE) {
				selectedSize = 3;
				CACHE_READ_SIZE = _256KB_READ_SIZE;
			}
		}
	} else {
		selectedSize = 0;
		CACHE_READ_SIZE = _32KB_READ_SIZE;
	}
	
	if(!ROMinRAM) {
		if(dsiWramUsed) WRAM_accessCounter++;
		else accessCounterIncrease();
	}

	u32 sector = (src/CACHE_READ_SIZE)*CACHE_READ_SIZE;

	if(page == src && len > CACHE_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
		// read directly at arm7 level
		commandRead = 0x025FFB08;

		cacheFlush();

		if(ROMinRAM) {
			REG_SCFG_EXT = 0x8300C000;

			sharedAddr_ROMinRAM[0] = dst;
			sharedAddr_ROMinRAM[1] = len;
			sharedAddr_ROMinRAM[2] = src;
			sharedAddr_ROMinRAM[3] = commandRead;

			IPC_SendSync(0xEE24);

			while(sharedAddr_ROMinRAM[3] != (vu32)0);

			REG_SCFG_EXT = 0x83000000;
		} else {
			sharedAddr[0] = dst;
			sharedAddr[1] = len;
			sharedAddr[2] = src;
			sharedAddr[3] = commandRead;

			IPC_SendSync(0xEE24);

			while(sharedAddr[3] != (vu32)0);
		}
	} else {
		// Prevent writing above DS 4MB RAM area
		if(dst >= 0x02400000 && dst < 0x02800000) dst -= 0x00400000;
		// read via the main RAM cache
		while(len > 0) {
			if(!ROMinRAM) {
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
			} else {
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

					REG_SCFG_EXT = 0x8300C000;

					sharedAddr_ROMinRAM[0] = dst;
					sharedAddr_ROMinRAM[1] = len2;
					sharedAddr_ROMinRAM[2] = ROM_LOCATION+src;
					sharedAddr_ROMinRAM[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr_ROMinRAM[3] != (vu32)0);
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

					REG_SCFG_EXT = 0x8300C000;

					sharedAddr_ROMinRAM[0] = page;
					sharedAddr_ROMinRAM[1] = len2;
					sharedAddr_ROMinRAM[2] = ROM_LOCATION+page;
					sharedAddr_ROMinRAM[3] = commandRead;

					IPC_SendSync(0xEE24);

					while(sharedAddr_ROMinRAM[3] != (vu32)0);
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
			}
		}
	}
	return 0;
}




