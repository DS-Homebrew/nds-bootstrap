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
#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
#include <nds/dma.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/fifomessages.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "hex.h"
#include "nds_header.h"
#include "module_params.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define _32KB_READ_SIZE  0x8000
#define _64KB_READ_SIZE  0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE   0x100000

#define ICACHE_SIZE      0x2000      
#define DCACHE_SIZE      0x1000      
#define CACHE_LINE_SIZE  32

#define THRESHOLD_CACHE_FLUSH 0x500  

//extern void user_exception(void);

extern cardengineArm9* volatile ce9;

vu32* volatile sharedAddr = (vu32*)CARDENGINE_SHARED_ADDRESS;

/*static u32 cacheDescriptor[dev_CACHE_SLOTS_32KB] = {0xFFFFFFFF};
static u32 cacheCounter[dev_CACHE_SLOTS_32KB];*/
static u32* cacheDescriptor = (u32*)0x02710000;
static u32* cacheCounter = (u32*)0x02712000;
static u32 accessCounter = 0;

static tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEADER;
static u32 romLocation = ROM_LOCATION;
static u32 readSize = _32KB_READ_SIZE;
static u32 cacheAddress = CACHE_ADRESS_START;
static u16 cacheSlots = retail_CACHE_SLOTS_32KB;

static bool flagsSet = false;
static bool isDma = false;
static u8 dma = 0;

static int allocateCacheSlot(void) {
	int slot = 0;
	u32 lowerCounter = accessCounter;
	for (int i = 0; i < cacheSlots; i++) {
		if (*(cacheCounter+i) <= lowerCounter) {
			lowerCounter = *(cacheCounter+i);
			slot = i;
			if (!lowerCounter) {
				break;
			}
		}
	}
	return slot;
}

static int getSlotForSector(u32 sector) {
	for (int i = 0; i < cacheSlots; i++) {
		if (*(cacheDescriptor+i) == sector) {
			return i;
		}
	}
	return -1;
}

static vu8* getCacheAddress(int slot) {
	//return (vu32*)(cacheAddress + slot*readSize);
	return (vu8*)(cacheAddress + slot*readSize);
}

static void updateDescriptor(int slot, u32 sector) {
	*(cacheDescriptor+slot) = sector;
	*(cacheCounter+slot) = accessCounter;
}

static void sleep(u32 ms) {
    if(ce9->patches->sleepRef) {
        volatile void (*sleepRef)(u32) = ce9->patches->sleepRef;
        (*sleepRef)(ms);
    } else if(ce9->thumbPatches->sleepRef) {
        callSleepThumb(ms);
    }    
}

static void readCached(u32* cacheStruct) {
    if(*ce9->patches->readCachedRef) {
        volatile int (*readCachedRef)(u32*) = *ce9->patches->readCachedRef;
        (*readCachedRef)(cacheStruct);
    } else if(*ce9->thumbPatches->readCachedRef) {
        callReadCacheRefThumb(cacheStruct);
    }    
}

static void waitForArm7(void) {
    IPC_SendSync(0xEE24);
    int count = 0;
	while (sharedAddr[3] != (vu32)0) {
        count++;
        sleep(2);
        if(count==20000000 || ce9->patches->sleepRef || ce9->thumbPatches->sleepRef){
            IPC_SendSync(0xEE24);
            count=0;
        }
    }
}

/*static inline bool isGameLaggy(const tNDSHeader* ndsHeader) {
	const char* romTid = getRomTid(ndsHeader);
	//return (strncmp(romTid, "ASM", 3) == 0  // Super Mario 64 DS (fixes sound crackles, breaks Mario's Holiday)
	return (strncmp(romTid, "AP2", 3) == 0   // Metroid Prime Pinball
		|| strncmp(romTid, "ADM", 3) == 0   // Animal Crossing: Wild World (fixes some sound crackles)
		|| strncmp(romTid, "APT", 3) == 0   // Pokemon Trozei (slightly boosts load speed)
		|| strncmp(romTid, "A2D", 3) == 0   // New Super Mario Bros. (fixes sound crackles)
		|| strncmp(romTid, "ARZ", 3) == 0   // MegaMan ZX (slightly boosts load speed)
		|| strncmp(romTid, "AC9", 3) == 0   // Spider-Man: Battle for New York
		|| strncmp(romTid, "YZX", 3) == 0   // MegaMan ZX Advent (slightly boosts load speed)
		|| strncmp(romTid, "YCT", 3) == 0   // Contra 4 (slightly boosts load speed)
		|| strncmp(romTid, "YT7", 3) == 0   // SEGA Superstars Tennis (fixes some sound issues)
		|| strncmp(romTid, "CS5", 3) == 0   // Spider-Man: Web of Shadows
		|| strncmp(romTid, "YGX", 3) == 0   // Grand Theft Auto: Chinatown Wars
		|| strncmp(romTid, "CS3", 3) == 0   // Sonic & SEGA All-Stars Racing
		|| strncmp(romTid, "VSO", 3) == 0   // Sonic Classic Collection
		|| strncmp(romTid, "IPK", 3) == 0   // Pokemon HeartGold
		|| strncmp(romTid, "IPG", 3) == 0   // Pokemon SoulSilver
		|| strncmp(romTid, "B6Z", 3) == 0   // MegaMan Zero Collection (slightly boosts load speed)
		|| strncmp(romTid, "IRB", 3) == 0   // Pokemon Black
		|| strncmp(romTid, "IRA", 3) == 0   // Pokemon White
		|| strncmp(romTid, "IRE", 3) == 0   // Pokemon Black 2
		|| strncmp(romTid, "IRD", 3) == 0); // Pokemon White 2
}*/

static inline int cardReadNormal(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	u32 commandRead;
	u32 sector = (src/readSize)*readSize;

	accessCounter++;

	/*if (page == src && len > readSize && (u32)dst < 0x02700000 && (u32)dst > 0x02000000 && (u32)dst % 4 == 0) {
		// Read directly at ARM7 level
		commandRead = 0x025FFB08;

		//cacheFlush();

		//REG_IME = 0;

		sharedAddr[0] = (vu32)dst;
		sharedAddr[1] = len;
		sharedAddr[2] = src;
		sharedAddr[3] = commandRead;

		waitForArm7();

		//REG_IME = 1;

	} else {*/
		// Read via the main RAM cache
		while(len > 0) {
			int slot = getSlotForSector(sector);
			vu8* buffer = getCacheAddress(slot);
			// Read max CACHE_READ_SIZE via the main RAM cache
			if (slot == -1) {
				// Send a command to the ARM7 to fill the RAM cache
				commandRead = 0x025FFB08;

				slot = allocateCacheSlot();

				buffer = getCacheAddress(slot);

				//REG_IME = 0;

				// Write the command
				sharedAddr[0] = (vu32)buffer;
				sharedAddr[1] = readSize;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				waitForArm7();

				//REG_IME = 1;
			}

			updateDescriptor(slot, sector);	
	
			u32 len2 = len;
			if ((src - sector) + len2 > readSize) {
				len2 = sector - src + readSize;
			}

			if (len2 > 512) {
				len2 -= src % 4;
				len2 -= len2 % 32;
			}
            
            if (isDma) {
                // Copy via dma
  				dmaCopyWordsAsynch(dma, (u8*)buffer+(src-sector), dst, len2);
                while (dmaBusy(dma)) {
                    sleep(1);
                }        
  
  				// Update cardi common
  				cardStruct[0] = src + len2;
  				cardStruct[1] = (vu32)(dst + len2);
  				cardStruct[2] = len - len2; 
            }  else {
    			if ((*ce9->patches->readCachedRef == 0 && *ce9->thumbPatches->readCachedRef == 0) || (len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0)) {
    				#ifdef DEBUG
    				// Send a log command for debug purpose
    				// -------------------------------------
    				commandRead = 0x026ff800;
    
    				sharedAddr[0] = dst;
    				sharedAddr[1] = len2;
    				sharedAddr[2] = buffer+src-sector;
    				sharedAddr[3] = commandRead;
    
    				waitForArm7();
    				// -------------------------------------*/
    				#endif
    
    				// Copy directly
    				tonccpy(dst, (u8*)buffer+(src-sector), len2);
    
    				// Update cardi common
    				cardStruct[0] = src + len2;
    				cardStruct[1] = (vu32)(dst + len2);
    				cardStruct[2] = len - len2;
    			} else {
    				#ifdef DEBUG
    				// Send a log command for debug purpose
    				// -------------------------------------
    				commandRead = 0x026ff800;
    
    				sharedAddr[0] = page;
    				sharedAddr[1] = len2;
    				sharedAddr[2] = buffer+page-sector;
    				sharedAddr[3] = commandRead;
    
    				waitForArm7();
    				// -------------------------------------
    				#endif
    
    				// Read via the 512b ram cache
    				//copy8(buffer+(page-sector)+(src%512), dst, len2);
    				//cardStruct[0] = src + len2;
    				//cardStruct[1] = dst + len2;
    				//cardStruct[2] = len - len2;
    				tonccpy(cacheBuffer, (u8*)buffer+(page-sector), 512);
    				*cachePage = page;
    				
                    readCached(cacheStruct);
    			}
            }
			len = cardStruct[2];
			if (len > 0) {
				src = cardStruct[0];
				dst = (u8*)cardStruct[1];
				page = (src / 512) * 512;
				sector = (src / readSize) * readSize;
				accessCounter++;
			}
		}
	//}
	
	if(strncmp(getRomTid(ndsHeader), "CLJ", 3) == 0){
		cacheFlush(); //workaround for some weird data-cache issue in Bowser's Inside Story.
	}

	return 0;
}

static inline int cardReadRAM(vu32* volatile cardStruct, u32* cacheStruct, u8* dst, u32 src, u32 len, u32 page, u8* cacheBuffer, u32* cachePage) {
	//u32 commandRead;
	while (len > 0) {
		u32 len2 = len;
		if (len2 > 512) {
			len2 -= src % 4;
			len2 -= len2 % 32;
		}

		if (isDma) {
            // Copy via dma
  			dmaCopyWordsAsynch(dma, (u8*)(((ce9->dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src), dst, len);
            while (dmaBusy(dma)) {
                sleep(1);
            }        
  
  			// Update cardi common
  			cardStruct[0] = src + len;
  			cardStruct[1] = (vu32)(dst + len);
  			cardStruct[2] = len - len; 
		} else {
			if ((*ce9->patches->readCachedRef == 0 && *ce9->thumbPatches->readCachedRef == 0) || (len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0)) {
				#ifdef DEBUG
				// Send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = dst;
				sharedAddr[1] = len;
				sharedAddr[2] = (ce9->dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src;
				sharedAddr[3] = commandRead;

				waitForArm7();
				// -------------------------------------
				#endif

				// Copy directly
				tonccpy(dst, (u8*)(((ce9->dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+src),len);

				// Update cardi common
				cardStruct[0] = src + len;
				cardStruct[1] = (vu32)(dst + len);
				cardStruct[2] = len - len;
			} else {
				#ifdef DEBUG
				// Send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = page;
				sharedAddr[1] = len2;
				sharedAddr[2] = ((ce9->dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation)-0x4000-ndsHeader->arm9binarySize)+page;
				sharedAddr[3] = commandRead;

				waitForArm7();
				// -------------------------------------
				#endif

				// Read via the 512b ram cache
				tonccpy(cacheBuffer, (u8*)(((ce9->dsiMode ? dev_CACHE_ADRESS_START_SDK5 : romLocation) - 0x4000 - ndsHeader->arm9binarySize) + page), 512);
				*cachePage = page;
				readCached(cacheStruct);
			}
		}
		len = cardStruct[2];
		if (len > 0) {
			src = cardStruct[0];
			dst = (u8*)cardStruct[1];
			page = (src / 512) * 512;
		}
	}

	return 0;
}

//Currently used for NSMBDS romhacks
void __attribute__((target("arm"))) debug8mbMpuFix(){
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}

u32 cardReadDma() {
	vu32* volatile cardStruct = ce9->cardStruct0;
    
	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];
    dma = cardStruct[3]; // dma channel
	void* func = (void*)cardStruct[4]; // function to call back once read done
	void* arg  = (void*)cardStruct[5]; // arguments of the function above
    
    if(dma >= 0 
        && dma <= 3 
        //&& func != NULL
        && len > 0
        && !(((int)dst) & 31)
        // test data not in ITCM
        && dst > 0x02000000
        // test data not in DTCM
        && (dst < 0x27E0000 || dst > 0x27E4000) 
        // check 512 bytes page alignement 
        && !(((int)len) & 511)
        && !(((int)src) & 511)
        && (ce9->patches->sleepRef || ce9->thumbPatches->sleepRef) // so far dma is useless without sleep method available
        ) {
        isDma = true;
        
        /*if (len < THRESHOLD_CACHE_FLUSH) {
            int oldIME = enterCriticalSection();
            u32     dst2 = dst;
            u32     mod = (dst2 & (CACHE_LINE_SIZE - 1));
            if (mod)
            {
                dst2 -= mod;
                DC_StoreRange((void *)(dst2), CACHE_LINE_SIZE);
                DC_StoreRange((void *)(dst2 + len), CACHE_LINE_SIZE);
                len += CACHE_LINE_SIZE;
            }
            IC_InvalidateRange((void *)dst, len);
            DC_InvalidateRange((void *)dst2, len);
            DC_WaitWriteBufferEmpty();
            leaveCriticalSection(oldIME);   
        } else {*/ 
            // Note : cacheFlush disable / reenable irq
            cacheFlush();
        //}
    } else { 
        isDma = false;
        dma=0;
    }
    
    return 0;    
}

/*
TODO: return the correct ID

From gbatek 
Returns RAW unencrypted Chip ID (eg. C2h,0Fh,00h,00h), repeated every 4 bytes.
  1st byte - Manufacturer (eg. C2h=Macronix) (roughly based on JEDEC IDs)
  2nd byte - Chip size (00h..7Fh: (N+1)Mbytes, F0h..FFh: (100h-N)*256Mbytes?)
  3rd byte - Flags (see below)
  4th byte - Flags (see below)
The Flag Bits in 3th byte can be
  0   Maybe Infrared flag? (in case ROM does contain on-chip infrared stuff)
  1   Unknown (set in some 3DS carts)
  2-7 Zero
The Flag Bits in 4th byte can be
  0-2 Zero
  3   Seems to be NAND flag (0=ROM, 1=NAND) (observed in only ONE cartridge)
  4   3DS Flag (0=NDS/DSi, 1=3DS)
  5   Zero   ... set in ... DSi-exclusive games?
  6   DSi flag (0=NDS/3DS, 1=DSi)
  7   Cart Protocol Variant (0=older/smaller carts, 1=newer/bigger carts)

Existing/known ROM IDs are:
  C2h,07h,00h,00h NDS Macronix 8MB ROM  (eg. DS Vision)
  AEh,0Fh,00h,00h NDS Noname   16MB ROM (eg. Meine Tierarztpraxis)
  C2h,0Fh,00h,00h NDS Macronix 16MB ROM (eg. Metroid Demo)
  C2h,1Fh,00h,00h NDS Macronix 32MB ROM (eg. Over the Hedge)
  C2h,1Fh,00h,40h DSi Macronix 32MB ROM (eg. Art Academy, TWL-VAAV, SystemFlaw)
  80h,3Fh,01h,E0h ?            64MB ROM+Infrared (eg. Walk with Me, NTR-IMWP)
  AEh,3Fh,00h,E0h DSi Noname   64MB ROM (eg. de Blob 2, TWL-VD2V)
  C2h,3Fh,00h,00h NDS Macronix 64MB ROM (eg. Ultimate Spiderman)
  C2h,3Fh,00h,40h DSi Macronix 64MB ROM (eg. Crime Lab, NTR-VAOP)
  80h,7Fh,00h,80h NDS SanDisk  128MB ROM (DS Zelda, NTR-AZEP-0)
  80h,7Fh,01h,E0h ?            128MB ROM+Infrared? (P-letter Soul Silver, IPGE)
  C2h,7Fh,00h,80h NDS Macronix 128MB ROM (eg. Spirit Tracks, NTR-BKIP)
  C2h,7Fh,00h,C0h DSi Macronix 128MB ROM (eg. Cooking Coach/TWL-VCKE)
  ECh,7Fh,00h,88h NDS Samsung  128MB NAND (eg. Warioware D.I.Y.)
  ECh,7Fh,01h,88h NDS Samsung? 128MB NAND+What? (eg. Jam with the Band, UXBP)
  ECh,7Fh,00h,E8h DSi Samsung? 128MB NAND (eg. Face Training, USKV)
  80h,FFh,80h,E0h NDS          256MB ROM (Kingdom Hearts - Re-Coded, NTR-BK9P)
  C2h,FFh,01h,C0h DSi Macronix 256MB ROM+Infrared? (eg. P-Letter White)
  C2h,FFh,00h,80h NDS Macronix 256MB ROM (eg. Band Hero, NTR-BGHP)
  C2h,FEh,01h,C0h DSi Macronix 512MB ROM+Infrared? (eg. P-Letter White 2)
  C2h,FEh,00h,90h 3DS Macronix probably 512MB? ROM (eg. Sims 3)
  45h,FAh,00h,90h 3DS SunDisk? maybe... 1.5GB? ROM (eg. Starfox)
  C2h,F8h,00h,90h 3DS Macronix maybe... 2GB?   ROM (eg. Kid Icarus)
  C2h,7Fh,00h,90h 3DS Macronix 128MB ROM CTR-P-AENJ MMinna no Ennichi
  C2h,FFh,00h,90h 3DS Macronix 256MB ROM CTR-P-AFSJ Pro Yakyuu Famista 2011
  C2h,FEh,00h,90h 3DS Macronix 512MB ROM CTR-P-AFAJ Real 3D Bass FishingFishOn
  C2h,FAh,00h,90h 3DS Macronix 1GB ROM CTR-P-ASUJ Hana to Ikimono Rittai Zukan
  C2h,FAh,02h,90h 3DS Macronix 1GB ROM CTR-P-AGGW Luigis Mansion 2 ASiA CHT
  C2h,F8h,00h,90h 3DS Macronix 2GB ROM CTR-P-ACFJ Castlevania - Lords of Shadow
  C2h,F8h,02h,90h 3DS Macronix 2GB ROM CTR-P-AH4J Monster Hunter 4
  AEh,FAh,00h,90h 3DS          1GB ROM CTR-P-AGKJ Gyakuten Saiban 5
  AEh,FAh,00h,98h 3DS          1GB NAND CTR-P-EGDJ Tobidase Doubutsu no Mori
  45h,FAh,00h,90h 3DS          1GB ROM CTR-P-AFLJ Fantasy Life
  45h,F8h,00h,90h 3DS          2GB ROM CTR-P-AVHJ Senran Kagura Burst - Guren
  C2h,F0h,00h,90h 3DS Macronix 4GB ROM CTR-P-ABRJ Biohazard Revelations
  FFh,FFh,FFh,FFh None (no cartridge inserted)
*/
bool cardInitialized = false;
u32 cardId(void) {
	#ifdef DEBUG	
	dbg_printf("\ncardId\n");
	#endif
    
    u32 cardid = 0xC2;
    u8 size = ndsHeader->deviceSize;
    
    //Devicecapacity         (Chipsize = 128KB SHL nn) (eg. 7 = 16MB)
    //Chip size (00h..7Fh: (N+1)Mbytes, F0h..FFh: (100h-N)*256Mbytes?) (eg. 0Fh = 16MB)
	u8 mb = 0;
    switch (size) {
		default:
			break;
		case 0x04:
			mb = 0x01;
			break;
		case 0x05:
			mb = 0x03;
			break;
		case 0x06:
			mb = 0x07;
			break;
		case 0x07:
			mb = 0x0F;
			break;
		case 0x08:
			mb = 0x1F;
			break;
		case 0x09:
			mb = 0x3F;
			break;
		case 0x0A:
			mb = 0x7F;
			break;
		case 0x0B:
			mb = 0xFF;
			break;
		case 0x0C:
			mb = 0xFE;
			break;
	}
    cardid |= mb << 8;

	//The Flag Bits in 3th byte can be
	//0   Maybe Infrared flag? (in case ROM does contain on-chip infrared stuff)
	if (strncmp(getRomTid(ndsHeader), "I", 1) == 0) {
		cardid |= 0x01 << 16;
	}

    //The Flag Bits in 4th byte can be
    //6   DSi flag (0=NDS/3DS, 1=DSi)
    //7   Cart Protocol Variant (0=older/smaller carts, 1=newer/bigger carts)
    u8 unit = 0;
    if (ndsHeader->unitCode==0x02) {
		unit=0xC0;
	} else if (strncmp(getRomTid(ndsHeader), "I", 1) == 0) {
		unit=0xE0;
	} else if (ce9->moduleParams->sdk_version > 0x4000000) {
		unit=0x80;
	}
    cardid |= unit << 24;
    
    
    if (!cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0xE080FF80; // golden sun
    //if (!cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0x80FF80E0; // golden sun
    if (cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0xFF000000; // golden sun
    //if (cardInitialized && strncmp(getRomTid(ndsHeader), "BO5", 3) == 0)  cardid = 0x000000FF; // golden sun

    cardInitialized= true;
    
	return cardid;
}

int cardRead(u32* cacheStruct, u8* dst0, u32 src0, u32 len0) {
	//nocashMessage("\narm9 cardRead\n");
	if (!flagsSet) {
		if (ce9->dsiMode) {
			romLocation = ROM_SDK5_LOCATION;
			cacheAddress = retail_CACHE_ADRESS_START_SDK5;
		}

		//if (isGameLaggy(ndsHeader)) {
			if (ce9->consoleModel > 0) {
				if (ce9->dsiMode) {
					// SDK 5
					cacheAddress = dev_CACHE_ADRESS_START_SDK5;
				}
				cacheSlots = (ce9->dsiMode ? dev_CACHE_SLOTS_32KB_SDK5 : dev_CACHE_SLOTS_32KB);
			} else {
				cacheSlots = (ce9->dsiMode ? retail_CACHE_SLOTS_32KB_SDK5 : retail_CACHE_SLOTS_32KB);
			}
			//readSize = _32KB_READ_SIZE;
		/*} else if (ce9->consoleModel > 0) {
			if (ce9->dsiMode) {
				// SDK 5
				cacheAddress = dev_CACHE_ADRESS_START_SDK5;
			}
			cacheSlots = (ce9->dsiMode ? dev_CACHE_SLOTS_SDK5 : dev_CACHE_SLOTS);
		}*/

		const char* romTid = getRomTid(ndsHeader);
		if (strncmp(romTid, "UBR", 3) == 0) {
			cacheAddress = retail_CACHE_ADRESS_START_SDK5;
			cacheSlots = retail_CACHE_SLOTS_32KB_SDK5;
		} else {
			debug8mbMpuFix();
		}

		//ndsHeader->romSize += 0x1000;

		if (ce9->enableExceptionHandler && ce9==CARDENGINE_ARM9_LOCATION) {
			//exceptionStack = (u32)EXCEPTION_STACK_LOCATION;
			//setExceptionHandler(user_exception);
		}
		
		flagsSet = true;
	}

	vu32* volatile cardStruct = (vu32* volatile)ce9->cardStruct0;

	u32 src = cardStruct[0];
	u8* dst = (u8*)(cardStruct[1]);
	u32 len = cardStruct[2];

	u32 page = (src / 512) * 512;

	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;

	#ifdef DEBUG
	u32 commandRead;

	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	waitForArm7();
	// -------------------------------------*/
	#endif

	if (src == 0) {
		// If ROM read location is 0, do not proceed.
		return 0;
	}

	// Fix reads below 0x8000
	if (src <= 0x8000){
		src = 0x8000 + (src & 0x1FF);
	}
    
    int ret = ce9->ROMinRAM ? cardReadRAM(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage) : cardReadNormal(cardStruct, cacheStruct, dst, src, len, page, cacheBuffer, cachePage);

    isDma=false;
    dma=0;

	return ret; 
}

void cardPullOut(void) {
	if (!ce9->ROMinRAM && *(vu32*)(0x027FFB30) != 0) {
		/*volatile int (*terminateForPullOutRef)(u32*) = *ce9->patches->terminateForPullOutRef;
        (*terminateForPullOutRef);*/
		sharedAddr[3] = 0x5245424F;
		waitForArm7();
	}
}
