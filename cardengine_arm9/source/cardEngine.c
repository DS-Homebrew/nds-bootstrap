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

#define _32KB_READ_SIZE 0x8000
#define _64KB_READ_SIZE 0x10000
#define _128KB_READ_SIZE 0x20000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000

#define _32KB_CACHE_ADRESS_START 0x0C400000
#define _32KB_CACHE_ADRESS_SIZE 0x80000
#define _32KB_CACHE_SLOTS 0x10
#define _64KB_CACHE_ADRESS_START 0x0C480000
#define _64KB_CACHE_ADRESS_SIZE 0x80000
#define _64KB_CACHE_SLOTS 0x8
#define _128KB_CACHE_ADRESS_START 0x0C500000
#define _128KB_CACHE_ADRESS_SIZE 0x300000
#define _128KB_CACHE_SLOTS 0x15
#define _256KB_CACHE_ADRESS_START 0x0C800000
#define _256KB_CACHE_ADRESS_SIZE 0x400000
#define _256KB_CACHE_SLOTS 0x10
#define _512KB_CACHE_ADRESS_START 0x0CC00000
#define _512KB_CACHE_ADRESS_SIZE 0x400000
#define _512KB_CACHE_SLOTS 0x8

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 _32KB_cacheDescriptor [_32KB_CACHE_SLOTS];
static u32 _32KB_cacheCounter [_32KB_CACHE_SLOTS];
static u32 _64KB_cacheDescriptor [_64KB_CACHE_SLOTS];
static u32 _64KB_cacheCounter [_64KB_CACHE_SLOTS];
static u32 _128KB_cacheDescriptor [_128KB_CACHE_SLOTS];
static u32 _128KB_cacheCounter [_128KB_CACHE_SLOTS];
static u32 _256KB_cacheDescriptor [_256KB_CACHE_SLOTS];
static u32 _256KB_cacheCounter [_256KB_CACHE_SLOTS];
static u32 _512KB_cacheDescriptor [_512KB_CACHE_SLOTS];
static u32 _512KB_cacheCounter [_512KB_CACHE_SLOTS];
static u32 _32KB_accessCounter = 0;
static u32 _64KB_accessCounter = 0;
static u32 _128KB_accessCounter = 0;
static u32 _256KB_accessCounter = 0;
static u32 _512KB_accessCounter = 0;

static int selectedSize = 0;

void user_exception(void);

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	exceptionStack = (u32)0x23EFFFC ;
	EXCEPTION_VECTOR = enterException ;
	*exceptionC = user_exception;
}

int allocateCacheSlot() {
	int slot = 0;
	int lowerCounter = 0;
	switch(selectedSize) {
		case 0:
		default:
			lowerCounter = _32KB_accessCounter;
			for(int i=0; i<_32KB_CACHE_SLOTS; i++) {
				if(_32KB_cacheCounter[i]<=lowerCounter) {
					lowerCounter = _32KB_cacheCounter[i];
					slot = i;
					if(!lowerCounter) break;
				}
			}
			return slot;
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
			return slot;
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
			return slot;
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
			return slot;
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
			return slot;
			break;
	}
}

int getSlotForSector(u32 sector) {
	switch(selectedSize) {
		case 0:
		default:
			for(int i=0; i<_32KB_CACHE_SLOTS; i++) {
				if(_32KB_cacheDescriptor[i]==sector) {
					return i;
				}
			}
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
	}
	return -1;
}


vu8* getCacheAddress(int slot) {
	switch(selectedSize) {
		case 0:
		default:
			return (vu32*)(_32KB_CACHE_ADRESS_START+slot*_32KB_READ_SIZE);
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
	}
}

void updateDescriptor(int slot, u32 sector) {
	switch(selectedSize) {
		case 0:
		default:
			_32KB_cacheDescriptor[slot] = sector;
			_32KB_cacheCounter[slot] = _32KB_accessCounter;
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
	}
}

void accessCounterIncrease() {
	switch(selectedSize) {
		case 0:
		default:
			_32KB_accessCounter++;
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
	
	
	selectedSize = 4;
	u32 CACHE_READ_SIZE = _512KB_READ_SIZE;
	if(len <= _32KB_READ_SIZE) {
		selectedSize = 0;
		CACHE_READ_SIZE = _32KB_READ_SIZE;
	}
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
	
	accessCounterIncrease();

	u32 sector = (src/CACHE_READ_SIZE)*CACHE_READ_SIZE;

	if(page == src && len > CACHE_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
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
		if(dst >= 0x02400000 && dst < 0x02800000) dst -= 0x00400000;	// Prevent writing above DS 4MB RAM area
		// read via the WRAM cache
		while(len > 0) {
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			// read max CACHE_READ_SIZE via the main RAM cache
			if(slot==-1) {
				// send a command to the arm7 to fill the RAM cache
				commandRead = 0x025FFB08;

				slot = allocateCacheSlot();
				
				buffer = getCacheAddress(slot);

				REG_SCFG_EXT = 0x83008000;

				if(needFlushDCCache) DC_FlushRange(buffer, CACHE_READ_SIZE);

				// write the command
				sharedAddr[0] = buffer;
				sharedAddr[1] = CACHE_READ_SIZE;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				while(sharedAddr[3] != (vu32)0);

				REG_SCFG_EXT = 0x83000000;
			}

			updateDescriptor(slot, sector);

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
				REG_SCFG_EXT = 0x83008000;
				fastCopy32(buffer+(src-sector),dst,len2);
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
				sharedAddr[2] = buffer+page-sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				while(sharedAddr[3] != (vu32)0);
				// -------------------------------------*/
				#endif

				// read via the 512b ram cache
				REG_SCFG_EXT = 0x83008000;
				fastCopy32(buffer+(page-sector), cacheBuffer, 512);
				REG_SCFG_EXT = 0x83000000;
				*cachePage = page;
				(*readCachedRef)(cacheStruct);
			}
			len = cardStruct[2];
			if(len>0) {
				src = cardStruct[0];
				dst = cardStruct[1];
				page = (src/512)*512;
				sector = (src/CACHE_READ_SIZE)*CACHE_READ_SIZE;
				accessCounterIncrease();
			}
		}
	}
	return 0;
}




