/*
	iointerface.c template

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. All derivative works must be clearly marked as such. Derivatives of this file
	 must have the author of the derivative indicated within the source.
  2. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//#define MAX_READ 53
#define cacheBlockSize 0x8000
#define defaultCacheSlots 0x80000/cacheBlockSize
#define BYTES_PER_READ 512
#define cacheBlockSectors (cacheBlockSize/BYTES_PER_READ)

#define REG_MBK_CACHE_START	0x4004044

#ifndef NULL
 #define NULL 0
#endif

#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/disc_io.h>
#include <nds/debug.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/dma.h>
#include <nds/ipc.h>
#include <nds/arm9/dldi.h>
#include "my_sdmmc.h"
#include "locations.h"
#include "aeabi.h"

void unlockDSiWram(void);

extern char ioType[4];
extern u32 dataStartOffset;
//extern vu32 word_command; // word_command_offset
//extern vu32 word_params; // word_command_offset+4
//extern u32* words_msg; // word_command_offset+8
u32 word_command_offset = 0;

extern u32 heapShrunk;
u32 cacheAddress = CACHE_ADRESS_START;
int cacheSlots = 16;
bool cacheEnabled = false;
u32 cacheDescriptor[defaultCacheSlots] = {0xFFFFFFFF};
int cacheCounter[defaultCacheSlots];
int cacheAllocated = 0;
int accessCounter = 0;

int allocateCacheSlot(void) {
	cacheAllocated++;
	if (cacheAllocated > cacheSlots) cacheAllocated = cacheSlots;

	int slot = 0;
	u32 lowerCounter = accessCounter;
	for (int i = 0; i < cacheAllocated; i++) {
		if (cacheCounter[i] <= lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if (!lowerCounter) {
				break;
			}
		}
	}
	return slot;
}

int getSlotForSector(sec_t sector) {
	for (int i = 0; i < cacheAllocated; i++) {
		if (cacheDescriptor[i] == sector) {
			return i;
		}
	}
	return -1;
}

vu8* getCacheAddress(int slot) {
	return (vu8*)(cacheAddress + slot*cacheBlockSize);
}

void updateDescriptor(int slot, sec_t sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

void transferToArm7(int slot) {
	*((vu8*)(REG_MBK_CACHE_START+slot)) |= 0x1;
}

void transferToArm9(int slot) {
	*((vu8*)(REG_MBK_CACHE_START+slot)) &= 0xFE;
}

// Use the dldi remaining space as temporary buffer : 28k usually available
u32* tmp_buf_addr = 0;
extern u32 dldi_bss_end;
extern vu8 allocated_space;

bool dsiMode = false;

static inline void sendValue32(u32 value32) {
	//nocashMessage("sendValue32");
	*(vu32*)(word_command_offset+4) = (vu32)value32;
	*(vu32*)word_command_offset = (vu32)0x027FEE04;
	IPC_SendSync(0xEE24);
}

static inline void sendMsg(int size, u8* msg) {
	//nocashMessage("sendMsg");
	*(vu32*)(word_command_offset+4) = (vu32)size;
	__aeabi_memcpy((u8*)word_command_offset+8, msg, size);
	*(vu32*)word_command_offset = (vu32)0x027FEE05;
	IPC_SendSync(0xEE24);
}

static inline void waitValue32() {
	//nocashMessage("waitValue32");
    //dbg_hexa(&word_command);
    //dbg_hexa(myMemUncached(&word_command));
	while(*(vu32*)word_command_offset != (vu32)0x027FEE08);
}

static inline u32 getValue32() {
	//nocashMessage("getValue32");
	return *(u32*)(word_command_offset+4);
}

/*void goodOldCopy32(u32* src, u32* dst, int size) {
	for(int i = 0 ; i<size/4; i++) {
		dst[i]=src[i];
	}
}*/

void extendedMemory(bool yes) {
	if (yes) {
		REG_SCFG_EXT += 0xC000;
	} else {
		REG_SCFG_EXT -= 0xC000;
	}
}

//---------------------------------------------------------------------------------
bool sd_Startup() {
//---------------------------------------------------------------------------------
	//nocashMessage("sdio_Startup");

	sendValue32(SDMMC_HAVE_SD);

	// waitValue32();
	int waitCycles = 0x100000;
	while(*(vu32*)word_command_offset != (vu32)0x027FEE08) {
		waitCycles--;
		if (waitCycles == 0) {
			return false;
		}
	}

	int result = getValue32();

	if(result==0) return false;

	sendValue32(SDMMC_SD_START);

	waitValue32();

	result = getValue32();

	return result == 0;
}

//---------------------------------------------------------------------------------
bool sd_ReadSectors(sec_t sector, sec_t numSectors,void* buffer) {
//---------------------------------------------------------------------------------
	//nocashMessage("sd_ReadSectors");
	FifoMessage msg;
	int result = 0;
	if (cacheEnabled) {
		accessCounter++;

		while(numSectors > 0) {
			const sec_t alignedSector = (sector/cacheBlockSectors)*cacheBlockSectors;

			int slot = getSlotForSector(alignedSector);
			vu8* cacheBuffer = getCacheAddress(slot);
			// Read max CACHE_READ_SIZE via the main RAM cache
			if (slot == -1) {
				slot = allocateCacheSlot();

				cacheBuffer = getCacheAddress(slot);

				msg.type = SDMMC_SD_READ_SECTORS;
				msg.sdParams.startsector = alignedSector;
				msg.sdParams.numsectors = cacheBlockSectors;
				msg.sdParams.buffer = (u32*)cacheBuffer;

				transferToArm7(15-slot);
				sendMsg(sizeof(msg), (u8*)&msg);
				waitValue32();
				transferToArm9(15-slot);

				result = getValue32();
			}
			updateDescriptor(slot, alignedSector);	

			sec_t len2 = numSectors;
			if ((sector - alignedSector) + numSectors > cacheBlockSectors) {
				len2 = alignedSector - sector + cacheBlockSectors;
			}

			__aeabi_memcpy(buffer, (u8*)cacheBuffer+((sector-alignedSector)*BYTES_PER_READ), len2*BYTES_PER_READ);

			for (u32 i = 0; i < len2; i++) {
				numSectors--;
				if (numSectors == 0) break;
			}
			if (numSectors > 0) {
				sector += len2;
				buffer += len2*BYTES_PER_READ;
				accessCounter++;
			}
		}
	} else {
		sec_t startsector, readsectors;

		int max_reads = ((1 << allocated_space) / 512) - 11;

		for(int numreads =0; numreads<numSectors; numreads+=max_reads) {
			startsector = sector+numreads;
			if(numSectors - numreads < max_reads) readsectors = numSectors - numreads ;
			else readsectors = max_reads;

			msg.type = SDMMC_SD_READ_SECTORS;
			msg.sdParams.startsector = startsector;
			msg.sdParams.numsectors = readsectors;
			msg.sdParams.buffer = tmp_buf_addr;

			sendMsg(sizeof(msg), (u8*)&msg);

			waitValue32();

			result = getValue32();

			__aeabi_memcpy(buffer+numreads*512, tmp_buf_addr, readsectors*512);
		}
	}

	return result == 0;
}

//---------------------------------------------------------------------------------
bool sd_WriteSectors(sec_t sector, sec_t numSectors,void* buffer) {
//---------------------------------------------------------------------------------
	//nocashMessage("sd_ReadSectors");
	FifoMessage msg;
	int result = 0;

	if (cacheEnabled) {
		accessCounter++;

		sec_t numSectorsBak = numSectors;

		while(numSectors > 0) {
			const sec_t alignedSector = (sector/cacheBlockSectors)*cacheBlockSectors;

			int slot = getSlotForSector(alignedSector);
			vu8* cacheBuffer = getCacheAddress(slot);
			// Read max CACHE_READ_SIZE via the main RAM cache
			if (slot == -1) {
				slot = allocateCacheSlot();

				cacheBuffer = getCacheAddress(slot);

				msg.type = SDMMC_SD_READ_SECTORS;
				msg.sdParams.startsector = alignedSector;
				msg.sdParams.numsectors = cacheBlockSectors;
				msg.sdParams.buffer = (u32*)cacheBuffer;

				transferToArm7(15-slot);
				sendMsg(sizeof(msg), (u8*)&msg);
				waitValue32();
				transferToArm9(15-slot);

				result = getValue32();
			}
			updateDescriptor(slot, alignedSector);	

			sec_t len2 = numSectors;
			if ((sector - alignedSector) + numSectors > cacheBlockSectors) {
				len2 = alignedSector - sector + cacheBlockSectors;
			}

			__aeabi_memcpy((u8*)cacheBuffer+((sector-alignedSector)*BYTES_PER_READ), buffer, len2*BYTES_PER_READ);

			for (u32 i = 0; i < len2; i++) {
				numSectors--;
				if (numSectors == 0) break;
			}
			if (numSectors > 0) {
				sector += len2;
				buffer += len2*BYTES_PER_READ;
				accessCounter++;
			}
		}

		numSectors = numSectorsBak;
	}

	sec_t startsector, readsectors;

	int max_reads = ((1 << allocated_space) / 512) - 11;

	for(int numreads =0; numreads<numSectors; numreads+=max_reads) {
		startsector = sector+numreads;
		if(numSectors - numreads < max_reads) readsectors = numSectors - numreads ;
		else readsectors = max_reads;

		msg.type = SDMMC_SD_WRITE_SECTORS;
		msg.sdParams.startsector = startsector;
		msg.sdParams.numsectors = readsectors;
		msg.sdParams.buffer = tmp_buf_addr;

		__aeabi_memcpy(tmp_buf_addr, buffer+numreads*512, readsectors*512);
		sendMsg(sizeof(msg), (u8*)&msg);

		waitValue32();

		result = getValue32();
	}

	return result == 0;
}

bool isArm7 = false;
bool ramDisk = false;


//---------------------------------------------------------------------------------
bool ramd_ReadSectors(u32 sector, u32 numSectors, void* buffer) {
//---------------------------------------------------------------------------------
	if (dsiMode) {
		__aeabi_memcpy(buffer, (void*)RAM_DISK_LOCATION_DSIMODE+(sector << 9), numSectors << 9);
	} else {
		if (buffer >= (void*)0x02C00000 && buffer < (void*)0x03000000) {
			buffer -= 0xC00000;		// Move out of RAM disk location
		} else if (buffer >= (void*)0x02800000 && buffer < (void*)0x02C00000) {
			buffer -= 0x800000;		// Move out of RAM disk location
		} else if (buffer >= (void*)0x02400000 && buffer < (void*)0x02800000) {
			buffer -= 0x400000;		// Move out of RAM disk location
		}

		if (!isArm7) extendedMemory(true);		// Enable extended memory mode to access RAM drive
		__aeabi_memcpy(buffer, (void*)RAM_DISK_LOCATION+(sector << 9), numSectors << 9);
		if (!isArm7) extendedMemory(false);	// Disable extended memory mode
	}
	return true;
}

//---------------------------------------------------------------------------------
bool ramd_WriteSectors(u32 sector, u32 numSectors, void* buffer) {
//---------------------------------------------------------------------------------
	if (dsiMode) {
		__aeabi_memcpy((void*)RAM_DISK_LOCATION_DSIMODE+(sector << 9), buffer, numSectors << 9);
	} else {
		if (buffer >= (void*)0x02C00000 && buffer < (void*)0x03000000) {
			buffer -= 0xC00000;		// Move out of RAM disk location
		} else if (buffer >= (void*)0x02800000 && buffer < (void*)0x02C00000) {
			buffer -= 0x800000;		// Move out of RAM disk location
		} else if (buffer >= (void*)0x02400000 && buffer < (void*)0x02800000) {
			buffer -= 0x400000;		// Move out of RAM disk location
		}

		if (!isArm7) extendedMemory(true);		// Enable extended memory mode to access RAM drive
		__aeabi_memcpy((void*)RAM_DISK_LOCATION+(sector << 9), buffer, numSectors << 9);
		if (!isArm7) extendedMemory(false);	// Disable extended memory mode
	}
	return true;
}


/*-----------------------------------------------------------------
startUp
Initialize the interface, geting it into an idle, ready state
returns true if successful, otherwise returns false
-----------------------------------------------------------------*/
bool startup(void) {
	//nocashMessage("startup");
	isArm7 = sdmmc_read16(REG_SDSTATUS0)!=0;
	ramDisk = (ioType[0] == 'R' && ioType[1] == 'A' && ioType[2] == 'M' && ioType[3] == 'D');
	if (REG_SCFG_EXT == 0x8307F100) {
		dsiMode = *(vu32*)((u32)NDS_HEADER_16MB+0xC) == *(vu32*)((u32)NDS_HEADER_16MB+0x0A00000C);
		//dsiMode = *(u16*)((u32)RAM_DISK_LOCATION_DSIMODE+0x1FE) == 0xAA55;
	}

	if (ramDisk) {
		return true;
	} else if (isArm7) {
		sdmmc_init();
		return SD_Init()==0;
	} else {
		unlockDSiWram(); // Unlock DSi WRAM and uncached RAM mirrors
		for (int i = 0; i < 16; i++) {
			transferToArm9(i);
		}
		*(vu32*)0x03700000 = 0x4253444E; // 'NDSB'
		if (*(vu32*)0x03700000 == 0x4253444E) {
			*(vu32*)0x03708000 = 0x77777777;
			cacheEnabled = (*(vu32*)0x03700000 == 0x4253444E); // DSi WRAM found, enable LRU cache
		}
		if (!cacheEnabled && heapShrunk) {
			cacheAddress = 0x02FE4000;
			cacheSlots = 3;
			cacheEnabled = true; // Enable LRU cache in Main RAM
			heapShrunk = 0;
		}

		const u32 mirrorOffset = (REG_SCFG_EXT == 0x8307F100) ? 0x0A000000 : 0xC00000;
		word_command_offset = dataStartOffset + 0x80 + mirrorOffset;
		tmp_buf_addr = (u32*)(dldi_bss_end + mirrorOffset);
		return sd_Startup();
	}
}

/*-----------------------------------------------------------------
isInserted
Is a card inserted?
return true if a card is inserted and usable
-----------------------------------------------------------------*/
bool isInserted (void) {
	//nocashMessage("isInserted");
	return true;
}


/*-----------------------------------------------------------------
clearStatus
Reset the card, clearing any status errors
return true if the card is idle and ready
-----------------------------------------------------------------*/
bool clearStatus (void) {
	//nocashMessage("clearStatus");
	return true;
}


/*-----------------------------------------------------------------
readSectors
Read "numSectors" 512-byte sized sectors from the card into "buffer",
starting at "sector".
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool readSectors (u32 sector, u32 numSectors, void* buffer) {
	//nocashMessage("readSectors");
	if (ramDisk) {
		return ramd_ReadSectors(sector,numSectors,buffer);
	} else if (isArm7) {
		return my_sdmmc_sdcard_readsectors(sector,numSectors,buffer,0)==0;
	} else {
		return sd_ReadSectors(sector,numSectors,buffer);
	}
}



/*-----------------------------------------------------------------
writeSectors
Write "numSectors" 512-byte sized sectors from "buffer" to the card,
starting at "sector".
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool writeSectors (u32 sector, u32 numSectors, void* buffer) {
	//nocashMessage("writeSectors");
	if (ramDisk) {
		return ramd_WriteSectors(sector,numSectors,buffer);
	} else if (isArm7) {
		return my_sdmmc_sdcard_writesectors(sector,numSectors,buffer,-1)==0;
	} else {
		return sd_WriteSectors(sector,numSectors,buffer);
	}
}

/*-----------------------------------------------------------------
shutdown
shutdown the card, performing any needed cleanup operations
Don't expect this function to be called before power off,
it is merely for disabling the card.
return true if the card is no longer active
-----------------------------------------------------------------*/
bool shutdown(void) {
	//nocashMessage("shutdown");
	return true;
}
