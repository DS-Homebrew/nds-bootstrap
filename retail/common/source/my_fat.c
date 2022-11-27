/*-----------------------------------------------------------------
 fat.c
 
 NDS MP
 GBAMP NDS Firmware Hack Version 2.12
 An NDS aware firmware patch for the GBA Movie Player.
 By Michael Chisholm (Chishm)
 
 Filesystem code based on GBAMP_CF.c by Chishm (me).
 
 License:
    Copyright (C) 2005  Michael "Chishm" Chisholm

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    If you use this code, please give due credit and email me about your
    project at chishm@hotmail.com
------------------------------------------------------------------*/
   
#include "tonccpy.h"
#include "my_fat.h"
#include "card.h"
#include "debug_file.h"

#define nextClusterBufferCount 8

//#define memcpy __builtin_memcpy


//---------------------------------------------------------------
// FAT constants

#define FILE_LAST 0x00
#define FILE_FREE 0xE5

#define ATTRIB_ARCH	0x20
#define ATTRIB_DIR	0x10
#define ATTRIB_LFN	0x0F
#define ATTRIB_VOL	0x08
#define ATTRIB_HID	0x02
#define ATTRIB_SYS	0x04
#define ATTRIB_RO	0x01

#define FAT16_ROOT_DIR_CLUSTER 0x00

// File Constants
#ifndef EOF
#define EOF -1
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif


//-----------------------------------------------------------------
// FAT constants
#define CLUSTER_EOF_16	0xFFFF

#define ATTRIB_ARCH	0x20
#define ATTRIB_DIR	0x10
#define ATTRIB_LFN	0x0F
#define ATTRIB_VOL	0x08
#define ATTRIB_HID	0x02
#define ATTRIB_SYS	0x04
#define ATTRIB_RO	0x01

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Data Structures

#define __PACKED __attribute__ ((__packed__))

// BIOS Parameter Block
typedef struct {

	u16	bytesPerSector;
	u8	sectorsPerCluster;
	u16	reservedSectors;
	u8	numFATs;
	u16	rootEntries;
	u16	numSectorsSmall;
	u8	mediaDesc;
	u16	sectorsPerFAT;
	u16	sectorsPerTrk;
	u16	numHeads;
	u32	numHiddenSectors;
	u32	numSectors;
} __PACKED BIOS_BPB;

// Boot Sector - must be packed
typedef struct
{
	u8	jmpBoot[3];
	u8	OEMName[8];
	BIOS_BPB bpb;
	union	// Different types of extended BIOS Parameter Block for FAT16 and FAT32
	{
		struct  
		{
			// Ext BIOS Parameter Block for FAT16
			u8	driveNumber;
			u8	reserved1;
			u8	extBootSig;
			u32	volumeID;
			u8	volumeLabel[11];
			u8	fileSysType[8];
			// Bootcode
			u8	bootCode[448];
		}	__PACKED fat16;
		struct  
		{
			// FAT32 extended block
			u32	sectorsPerFAT32;
			u16	extFlags;
			u16	fsVer;
			u32	rootClus;
			u16	fsInfo;
			u16	bkBootSec;
			u8	reserved[12];
			// Ext BIOS Parameter Block for FAT16
			u8	driveNumber;
			u8	reserved1;
			u8	extBootSig;
			u32	volumeID;
			u8	volumeLabel[11];
			u8	fileSysType[8];
			// Bootcode
			u8	bootCode[420];
		}	__PACKED fat32;
	}	__PACKED extBlock;

	__PACKED	u16	bootSig;

}	__PACKED BOOT_SEC;

_Static_assert(sizeof(BOOT_SEC) == 512);

// Directory entry - must be packed
typedef struct
{
	u8	name[8];
	u8	ext[3];
	u8	attrib;
	u8	reserved;
	u8	cTime_ms;
	u16	cTime;
	u16	cDate;
	u16	aDate;
	u16	startClusterHigh;
	u16	mTime;
	u16	mDate;
	u16	startCluster;
	u32	fileSize;
}	__PACKED DIR_ENT;

// File information - no need to pack
typedef struct
{
	u32 firstCluster;
	u32 length;
	u32 curPos;
	u32 curClus;			// Current cluster to read from
	int curSect;			// Current sector within cluster
	int curByte;			// Current byte within sector
	char readBuffer[512];	// Buffer used for unaligned reads
	u32 appClus;			// Cluster to append to
	int appSect;			// Sector within cluster for appending
	int appByte;			// Byte within sector for appending
	bool read;	// Can read from file
	bool write;	// Can write to file
	bool append;// Can append to file
	bool inUse;	// This file is open
	u32 dirEntSector;	// The sector where the directory entry is stored
	int dirEntOffset;	// The offset within the directory sector
}	FAT_FILE;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Global Variables

// _VARS_IN_RAM variables are stored in the largest section of WRAM 
// available: IWRAM on NDS ARM7, EWRAM on NDS ARM9 and GBA

// Locations on card
#ifdef TWOCARD
int discRootDir[2];
int discRootDirClus[2];
int discFAT[2];
int discSecPerFAT[2];
int discNumSec[2];
int discData[2];
int discBytePerSec[2];
int discSecPerClus[2];
int discBytePerClus[2];
int prevNextClust[2] = {0};
int prevFirstClust[2] = {-1};
int prevSect[2] = {-1};
int prevClust[2] = {-1};
#else
int discRootDir;
int discRootDirClus;
int discFAT;
int discSecPerFAT;
int discNumSec;
int discData;
int discBytePerSec;
int discSecPerClus;
int discBytePerClus;
#ifdef MORECLUSTERBUFFERS
int fatAccessCounter = 0;
int prevNextClust[nextClusterBufferCount] = {0};
#else
int prevNextClust = 0;
#endif
int prevFirstClust = -1;
int prevSect = -1;
int prevClust = -1;
#endif

enum {FS_UNKNOWN, FS_FAT12, FS_FAT16, FS_FAT32}
#ifdef TWOCARD
discFileSystem[2];
#else
discFileSystem;
#endif

// Global sector buffer to save on stack space
#ifdef TWOCARD
unsigned char nextClusterBuffer[2][BYTES_PER_SECTOR];
unsigned char lastGlobalBuffer[2][BYTES_PER_SECTOR];
unsigned char globalBuffer[2][BYTES_PER_SECTOR];
#else
#ifdef MORECLUSTERBUFFERS
unsigned char nextClusterBuffer[nextClusterBufferCount][BYTES_PER_SECTOR];
#else
unsigned char nextClusterBuffer[BYTES_PER_SECTOR];
#endif
unsigned char lastGlobalBuffer[BYTES_PER_SECTOR];
unsigned char globalBuffer[BYTES_PER_SECTOR];
#endif

#define CLUSTER_CACHE      0x2700000 // Main RAM
#define CLUSTER_CACHE_SIZE 0x80000 // 512K

u32* lastClusterCacheUsed = (u32*) CLUSTER_CACHE;
u32 clusterCache = CLUSTER_CACHE;
#ifndef B4DS
u32 currentClusterCacheSize = 0;
#endif
u32 clusterCacheSize = CLUSTER_CACHE_SIZE;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//FAT routines

#ifdef TWOCARD
u32 FAT_ClustToSect (u32 cluster, bool card2) {
	return (((cluster-2) * discSecPerClus[card2]) + discData[card2]);
}
#else
u32 FAT_ClustToSect (u32 cluster) {
	return (((cluster-2) * discSecPerClus) + discData);
}

#ifdef MORECLUSTERBUFFERS
int FAT_ReadNextClusterCache(u32 sector)
{
	int curSector = 0;
	for (int i = 0; i < nextClusterBufferCount; i++) {
		if (prevNextClust[i] == sector) {
			return i;
		}
	}

	curSector = fatAccessCounter;
	CARD_ReadSector(sector, nextClusterBuffer[curSector], 0, 0);
	prevNextClust[curSector] = sector;
	fatAccessCounter++;
	if (fatAccessCounter == nextClusterBufferCount) fatAccessCounter = 0;

	return curSector;
}
#endif
#endif

/*-----------------------------------------------------------------
FAT_NextCluster
Internal function - gets the cluster linked from input cluster
-----------------------------------------------------------------*/
#ifdef TWOCARD
u32 FAT_NextCluster(u32 cluster, bool card2)
#else
u32 FAT_NextCluster(u32 cluster)
#endif
{
	u32 nextCluster = CLUSTER_FREE;
	u32 sector;
	int offset;
#ifdef MORECLUSTERBUFFERS
	int curSector;
#endif


#ifdef TWOCARD
	switch (discFileSystem[card2]) 
#else
	switch (discFileSystem) 
#endif
	{
		case FS_UNKNOWN:
			nextCluster = CLUSTER_FREE;
			break;

		case FS_FAT12:
			#ifdef TWOCARD
			sector = discFAT[card2] + (((cluster * 3) / 2) / BYTES_PER_SECTOR);
			offset = ((cluster * 3) / 2) % BYTES_PER_SECTOR;
			if (prevNextClust[card2] != sector) {
				CARD_ReadSector(sector, nextClusterBuffer[card2], 0, 0, card2);
				prevNextClust[card2] = sector;
			}
			nextCluster = ((u8*) nextClusterBuffer[card2])[offset];
			#else
			sector = discFAT + (((cluster * 3) / 2) / BYTES_PER_SECTOR);
			offset = ((cluster * 3) / 2) % BYTES_PER_SECTOR;
			#ifdef MORECLUSTERBUFFERS
			curSector = FAT_ReadNextClusterCache(sector);
			// read the nextCluster value
			nextCluster = ((u8*) nextClusterBuffer[curSector])[offset];
			#else
			if (prevNextClust != sector) {
				CARD_ReadSector(sector, nextClusterBuffer, 0, 0);
				prevNextClust = sector;
			}
			// read the nextCluster value
			nextCluster = ((u8*) nextClusterBuffer)[offset];
			#endif
			#endif
			offset++;

			if (offset >= BYTES_PER_SECTOR) {
				offset = 0;
				sector++;
			}

			#ifdef TWOCARD
			if (prevNextClust[card2] != sector) {
				CARD_ReadSector(sector, nextClusterBuffer[card2], 0, 0, card2);
				prevNextClust[card2] = sector;
			}
			nextCluster |= (((u8*) nextClusterBuffer[card2])[offset]) << 8;
			#else
			#ifdef MORECLUSTERBUFFERS
			curSector = FAT_ReadNextClusterCache(sector);
			// read the nextCluster value
			nextCluster |= (((u8*) nextClusterBuffer[curSector])[offset]) << 8;
			#else
			if (prevNextClust != sector) {
				CARD_ReadSector(sector, nextClusterBuffer, 0, 0);
				prevNextClust = sector;
			}
			// read the nextCluster value
			nextCluster |= (((u8*) nextClusterBuffer)[offset]) << 8;
			#endif
			#endif

			if (cluster & 0x01) {
				nextCluster = nextCluster >> 4;
			} else 	{
				nextCluster &= 0x0FFF;
			}

			break;

		case FS_FAT16:
			#ifdef TWOCARD
			sector = discFAT[card2] + ((cluster << 1) / BYTES_PER_SECTOR);
			#else
			sector = discFAT + ((cluster << 1) / BYTES_PER_SECTOR);
			#endif
			offset = cluster % (BYTES_PER_SECTOR >> 1);

			#ifdef TWOCARD
			if (prevNextClust[card2] != sector) {
				CARD_ReadSector(sector, nextClusterBuffer[card2], 0, 0, card2);
				prevNextClust[card2] = sector;
			}
			// read the nextCluster value
			nextCluster = ((u16*)nextClusterBuffer[card2])[offset];
			#else
			#ifdef MORECLUSTERBUFFERS
			curSector = FAT_ReadNextClusterCache(sector);
			// read the nextCluster value
			nextCluster = ((u16*)nextClusterBuffer[curSector])[offset];
			#else
			if (prevNextClust != sector) {
				CARD_ReadSector(sector, nextClusterBuffer, 0, 0);
				prevNextClust = sector;
			}
			// read the nextCluster value
			nextCluster = ((u16*)nextClusterBuffer)[offset];
			#endif
			#endif

			if (nextCluster >= 0xFFF7)
			{
				nextCluster = CLUSTER_EOF;
			}
			break;

		case FS_FAT32:
			#ifdef TWOCARD
			sector = discFAT[card2] + ((cluster << 2) / BYTES_PER_SECTOR);
			#else
			sector = discFAT + ((cluster << 2) / BYTES_PER_SECTOR);
			#endif
			offset = cluster % (BYTES_PER_SECTOR >> 2);

			#ifdef TWOCARD
			if (prevNextClust[card2] != sector) {
				CARD_ReadSector(sector, nextClusterBuffer[card2], 0, 0, card2);
				prevNextClust[card2] = sector;
			}
			// read the nextCluster value
			nextCluster = (((u32*)nextClusterBuffer[card2])[offset]) & 0x0FFFFFFF;
			#else
			#ifdef MORECLUSTERBUFFERS
			curSector = FAT_ReadNextClusterCache(sector);
			// read the nextCluster value
			nextCluster = (((u32*)nextClusterBuffer[curSector])[offset]) & 0x0FFFFFFF;
			#else
			if (prevNextClust != sector) {
				CARD_ReadSector(sector, nextClusterBuffer, 0, 0);
				prevNextClust = sector;
			}
			// read the nextCluster value
			nextCluster = (((u32*)nextClusterBuffer)[offset]) & 0x0FFFFFFF;
			#endif
			#endif

			if (nextCluster >= 0x0FFFFFF7)
			{
				nextCluster = CLUSTER_EOF;
			}
			break;

		default:
			nextCluster = CLUSTER_FREE;
			break;
	}

	return nextCluster;
}

/*-----------------------------------------------------------------
ucase
Returns the uppercase version of the given char
char IN: a character
char return OUT: uppercase version of character
-----------------------------------------------------------------*/
char ucase (char character)
{
	if ((character > 0x60) && (character < 0x7B))
		character = character - 0x20;
	return (character);
}

/*-----------------------------------------------------------------
FAT_InitFiles
Reads the FAT information from the CF card.
You need to call this before reading any files.
bool return OUT: true if successful.
-----------------------------------------------------------------*/
#ifndef B4DS
#ifdef TWOCARD
bool FAT_InitFiles (bool initCard, bool card2, int ndmaSlot)
#else
bool FAT_InitFiles (bool initCard, int ndmaSlot)
#endif
#else
bool FAT_InitFiles (bool initCard)
#endif
{
	int i;
	int bootSector;
	BOOT_SEC* bootSec;

	#ifdef TWOCARD
	if (initCard && !CARD_StartUp(card2))
	{
		#ifdef DEBUG
		nocashMessage("!CARD_StartUp(card2)");
		#endif
		return (false);
	}

	// Read first sector of card
	if (!CARD_ReadSector (0, globalBuffer[card2], 0, 0, card2))
	{
		#ifdef DEBUG
		nocashMessage("!CARD_ReadSector (0, globalBuffer[card2])");
		#endif
		return false;
	}
	// Check if there is a FAT string, which indicates this is a boot sector
	if ((globalBuffer[card2][0x36] == 'F') && (globalBuffer[card2][0x37] == 'A') && (globalBuffer[card2][0x38] == 'T'))
	{
		bootSector = 0;
	}
	// Check for FAT32
	else if ((globalBuffer[card2][0x52] == 'F') && (globalBuffer[card2][0x53] == 'A') && (globalBuffer[card2][0x54] == 'T'))
	{
		bootSector = 0;
	}
	else	// This is an MBR
	{
		// Find first valid partition from MBR
		// First check for an active partition
		for (i=0x1BE; (i < 0x1FE) && (globalBuffer[card2][i] != 0x80); i+= 0x10);
		// If it didn't find an active partition, search for any valid partition
		if (i == 0x1FE) 
			for (i=0x1BE; (i < 0x1FE) && (globalBuffer[card2][i+0x04] == 0x00); i+= 0x10);

		// Go to first valid partition
		if ( i != 0x1FE)	// Make sure it found a partition
		{
			bootSector = globalBuffer[card2][0x8 + i] + (globalBuffer[card2][0x9 + i] << 8) + (globalBuffer[card2][0xA + i] << 16) + ((globalBuffer[card2][0xB + i] << 24) & 0x0F);
		} else {
			bootSector = 0;	// No partition found, assume this is a MBR free disk
		}
	}

	// Read in boot sector
	bootSec = (BOOT_SEC*) globalBuffer[card2];
	CARD_ReadSector (bootSector, bootSec, 0, 0, card2);

	// Store required information about the file system
	if (bootSec->bpb.sectorsPerFAT != 0)
	{
		discSecPerFAT[card2] = bootSec->bpb.sectorsPerFAT;
	}
	else
	{
		discSecPerFAT[card2] = bootSec->extBlock.fat32.sectorsPerFAT32;
	}

	if (bootSec->bpb.numSectorsSmall != 0)
	{
		discNumSec[card2] = bootSec->bpb.numSectorsSmall;
	}
	else
	{
		discNumSec[card2] = bootSec->bpb.numSectors;
	}

	discBytePerSec[card2] = BYTES_PER_SECTOR;	// Sector size is redefined to be 512 bytes
	discSecPerClus[card2] = bootSec->bpb.sectorsPerCluster * bootSec->bpb.bytesPerSector / BYTES_PER_SECTOR;
	discBytePerClus[card2] = discBytePerSec[card2] * discSecPerClus[card2];
	discFAT[card2] = bootSector + bootSec->bpb.reservedSectors;

	discRootDir[card2] = discFAT[card2] + (bootSec->bpb.numFATs * discSecPerFAT[card2]);
	discData[card2] = discRootDir[card2] + ((bootSec->bpb.rootEntries * sizeof(DIR_ENT)) / BYTES_PER_SECTOR);

	if ((discNumSec[card2] - discData[card2]) / bootSec->bpb.sectorsPerCluster < 4085)
	{
		discFileSystem[card2] = FS_FAT12;
	}
	else if ((discNumSec[card2] - discData[card2]) / bootSec->bpb.sectorsPerCluster < 65525)
	{
		discFileSystem[card2] = FS_FAT16;
	}
	else
	{
		discFileSystem[card2] = FS_FAT32;
	}

	if (discFileSystem[card2] != FS_FAT32)
	{
		discRootDirClus[card2] = FAT16_ROOT_DIR_CLUSTER;
	}
	else	// Set up for the FAT32 way
	{
		discRootDirClus[card2] = bootSec->extBlock.fat32.rootClus;
		// Check if FAT mirroring is enabled
		if (!(bootSec->extBlock.fat32.extFlags & 0x80))
		{
			// Use the active FAT
			discFAT[card2] = discFAT[card2] + ( discSecPerFAT[card2] * (bootSec->extBlock.fat32.extFlags & 0x0F));
		}
	}
	#else
	if (initCard && !CARD_StartUp())
	{
		#ifdef DEBUG
		nocashMessage("!CARD_StartUp()");
		#endif
		return (false);
	}

	// Read first sector of card
	if (!CARD_ReadSector (0, globalBuffer, 0, 0))
	{
		#ifdef DEBUG
		nocashMessage("!CARD_ReadSector (0, globalBuffer)");
		#endif
		return false;
	}
	// Check if there is a FAT string, which indicates this is a boot sector
	if ((globalBuffer[0x36] == 'F') && (globalBuffer[0x37] == 'A') && (globalBuffer[0x38] == 'T'))
	{
		bootSector = 0;
	}
	// Check for FAT32
	else if ((globalBuffer[0x52] == 'F') && (globalBuffer[0x53] == 'A') && (globalBuffer[0x54] == 'T'))
	{
		bootSector = 0;
	}
	else	// This is an MBR
	{
		// Find first valid partition from MBR
		// First check for an active partition
		for (i=0x1BE; (i < 0x1FE) && (globalBuffer[i] != 0x80); i+= 0x10);
		// If it didn't find an active partition, search for any valid partition
		if (i == 0x1FE) 
			for (i=0x1BE; (i < 0x1FE) && (globalBuffer[i+0x04] == 0x00); i+= 0x10);

		// Go to first valid partition
		if ( i != 0x1FE)	// Make sure it found a partition
		{
			bootSector = globalBuffer[0x8 + i] + (globalBuffer[0x9 + i] << 8) + (globalBuffer[0xA + i] << 16) + ((globalBuffer[0xB + i] << 24) & 0x0F);
		} else {
			bootSector = 0;	// No partition found, assume this is a MBR free disk
		}
	}

	// Read in boot sector
	bootSec = (BOOT_SEC*) globalBuffer;
	CARD_ReadSector (bootSector, bootSec, 0, 0);

	// Store required information about the file system
	if (bootSec->bpb.sectorsPerFAT != 0)
	{
		discSecPerFAT = bootSec->bpb.sectorsPerFAT;
	}
	else
	{
		discSecPerFAT = bootSec->extBlock.fat32.sectorsPerFAT32;
	}

	if (bootSec->bpb.numSectorsSmall != 0)
	{
		discNumSec = bootSec->bpb.numSectorsSmall;
	}
	else
	{
		discNumSec = bootSec->bpb.numSectors;
	}

	discBytePerSec = BYTES_PER_SECTOR;	// Sector size is redefined to be 512 bytes
	discSecPerClus = bootSec->bpb.sectorsPerCluster * bootSec->bpb.bytesPerSector / BYTES_PER_SECTOR;
	discBytePerClus = discBytePerSec * discSecPerClus;
	discFAT = bootSector + bootSec->bpb.reservedSectors;

	discRootDir = discFAT + (bootSec->bpb.numFATs * discSecPerFAT);
	discData = discRootDir + ((bootSec->bpb.rootEntries * sizeof(DIR_ENT)) / BYTES_PER_SECTOR);

	if ((discNumSec - discData) / bootSec->bpb.sectorsPerCluster < 4085)
	{
		discFileSystem = FS_FAT12;
	}
	else if ((discNumSec - discData) / bootSec->bpb.sectorsPerCluster < 65525)
	{
		discFileSystem = FS_FAT16;
	}
	else
	{
		discFileSystem = FS_FAT32;
	}

	if (discFileSystem != FS_FAT32)
	{
		discRootDirClus = FAT16_ROOT_DIR_CLUSTER;
	}
	else	// Set up for the FAT32 way
	{
		discRootDirClus = bootSec->extBlock.fat32.rootClus;
		// Check if FAT mirroring is enabled
		if (!(bootSec->extBlock.fat32.extFlags & 0x80))
		{
			// Use the active FAT
			discFAT = discFAT + ( discSecPerFAT * (bootSec->extBlock.fat32.extFlags & 0x0F));
		}
	}
	#endif

	#ifdef DEBUG
	nocashMessage("FAT_InitFiles OK");
	#endif

	return (true);
}


/*-----------------------------------------------------------------
getBootFileCluster
-----------------------------------------------------------------*/
#ifdef TWOCARD
aFile getBootFileCluster (const char* bootName, bool card2)
#else
aFile getBootFileCluster (const char* bootName)
#endif
{
	DIR_ENT dir;
	int firstSector = 0;
	bool notFound = false;
	bool found = false;
//	int maxSectors;
	#ifdef TWOCARD
	u32 wrkDirCluster = discRootDirClus[card2];
	#else
	u32 wrkDirCluster = discRootDirClus;
	#endif
	u32 wrkDirSector = 0;
	int wrkDirOffset = 0;
	int nameOffset;
	aFile file;

	dir.startCluster = CLUSTER_FREE; // default to no file found
	dir.startClusterHigh = CLUSTER_FREE;


	// Check if fat has been initialised
	#ifdef TWOCARD
	if (discBytePerSec[card2] == 0)
	#else
	if (discBytePerSec == 0)
	#endif
	{
		#ifdef DEBUG
		nocashMessage("getBootFileCluster  fat not initialised");
		#endif

		file.firstCluster = CLUSTER_FREE;
		file.currentCluster = file.firstCluster;
		file.currentOffset=0;
		#ifdef TWOCARD
		file.card2 = card2;
		#endif
		return file;
	}

	char *ptr = (char*)bootName;
	while (*ptr != '.') ptr++;
	int namelen = ptr - bootName;

//	maxSectors = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? (discData - discRootDir) : discSecPerClus);
	// Scan Dir for correct entry
	#ifdef TWOCARD
	firstSector = discRootDir[card2];
	prevSect[card2] = -1;
	CARD_ReadSector (firstSector + wrkDirSector, globalBuffer[card2], 0, 0, card2);
	#else
	firstSector = discRootDir;
	prevSect = -1;
	CARD_ReadSector (firstSector + wrkDirSector, globalBuffer, 0, 0);
	#endif
	found = false;
	notFound = false;
	wrkDirOffset = -1;	// Start at entry zero, Compensating for increment
	while (!found && !notFound) {
		wrkDirOffset++;
		if (wrkDirOffset == BYTES_PER_SECTOR / sizeof (DIR_ENT))
		{
			wrkDirOffset = 0;
			wrkDirSector++;
			#ifdef TWOCARD
			if ((wrkDirSector == discSecPerClus[card2]) && (wrkDirCluster != FAT16_ROOT_DIR_CLUSTER))
			#else
			if ((wrkDirSector == discSecPerClus) && (wrkDirCluster != FAT16_ROOT_DIR_CLUSTER))
			#endif
			{
				wrkDirSector = 0;
				#ifdef TWOCARD
				wrkDirCluster = FAT_NextCluster(wrkDirCluster, card2);
				#else
				wrkDirCluster = FAT_NextCluster(wrkDirCluster);
				#endif
				if (wrkDirCluster == CLUSTER_EOF)
				{
					notFound = true;
				}
				#ifdef TWOCARD
				firstSector = FAT_ClustToSect(wrkDirCluster, card2);
				#else
				firstSector = FAT_ClustToSect(wrkDirCluster);
				#endif
			}
			else if ((wrkDirCluster == FAT16_ROOT_DIR_CLUSTER) && (wrkDirSector == (discData - discRootDir)))
			{
				notFound = true;	// Got to end of root dir
			}
			#ifdef TWOCARD
			CARD_ReadSector (firstSector + wrkDirSector, globalBuffer[card2], 0, 0, card2);
			#else
			CARD_ReadSector (firstSector + wrkDirSector, globalBuffer, 0, 0);
			#endif
		}
		#ifdef TWOCARD
		dir = ((DIR_ENT*) globalBuffer[card2])[wrkDirOffset];
		#else
		dir = ((DIR_ENT*) globalBuffer)[wrkDirOffset];
		#endif
		found = true;
		if ((dir.attrib & ATTRIB_DIR) || (dir.attrib & ATTRIB_VOL))
		{
			found = false;
		}
		if(namelen<8 && dir.name[namelen]!=0x20) found = false;
		for (nameOffset = 0; nameOffset < namelen && found; nameOffset++)
		{
			if (ucase(dir.name[nameOffset]) != bootName[nameOffset])
				found = false;
		}
		for (nameOffset = 0; nameOffset < 3 && found; nameOffset++)
		{
			if (ucase(dir.ext[nameOffset]) != bootName[nameOffset+namelen+1])
				found = false;
		}
		if (dir.name[0] == FILE_LAST)
		{
			notFound = true;
		}
	} 

	// If no file is found, return CLUSTER_FREE
	if (notFound)
	{
		#ifdef DEBUG
		nocashMessage("getBootFileCluster  notFound");
		#endif

		file.firstCluster = CLUSTER_FREE;
		file.currentCluster = file.firstCluster;
		file.currentOffset=0;
		file.fatTableCached=false;

		return file;
	}

	#ifdef DEBUG
	nocashMessage("getBootFileCluster  found");
	#endif

	file.firstCluster = (dir.startCluster | (dir.startClusterHigh << 16));
	file.currentCluster = file.firstCluster;
	file.currentOffset=0;
	file.fatTableCached=false;
	#ifdef TWOCARD
	file.card2 = card2;
	#endif
	return file;
}

#ifdef TWOCARD
aFile getFileFromCluster (u32 cluster, bool card2)
#else
aFile getFileFromCluster (u32 cluster)
#endif
{
	aFile file;
	file.firstCluster = cluster;
	file.currentCluster = file.firstCluster;
	file.currentOffset=0;
	file.fatTableCached=false;
	#ifdef TWOCARD
	file.card2 = card2;
	#endif
	return file;
}

u32 getCachedCluster(aFile * file, int clusterIndex)
{
	if (file->fatTableCompressed) {
		int posSub = 0;
		for (int c = 0; c <= (int)file->fatTableCacheSize/4; c+=2) {
			const u32 cluster = file->fatTableCache[c];
			const u32 cachePlusIndex = cluster+(clusterIndex - posSub);
			if (cachePlusIndex >= cluster && cachePlusIndex < (cluster + file->fatTableCache[c+1])) {
				return cachePlusIndex;
			}
			posSub += file->fatTableCache[c+1];
		}
	}
	return file->fatTableCache[clusterIndex];
}

#ifndef B4DS
#ifndef _NO_SDMMC
static readContext context;

/*-----------------------------------------------------------------
fileReadNonBLocking(buffer, cluster, startOffset, length)
-----------------------------------------------------------------*/
bool fileReadNonBLocking (char* buffer, aFile * file, u32 startOffset, u32 length, int ndmaSlot)
{
	#ifdef DEBUG
	nocashMessage("fileRead");
    dbg_hexa(buffer);   
    dbg_hexa(startOffset);
    dbg_hexa(length);
	#endif

	context.dataPos = 0;
    context.file = file;
    context.buffer = buffer;
    context.length = length;
    context.ndmaSlot = ndmaSlot;

	int beginBytes;

    context.clusterIndex = 0;

	if (file->firstCluster == CLUSTER_FREE || file->firstCluster == CLUSTER_EOF
	#ifdef TWOCARD
		|| file->card2
	#endif
	) {
		return true;
	}

	if(file->fatTableCached) {
    	#ifdef DEBUG
        nocashMessage("fat table cached");
        #endif
		#ifdef TWOCARD
		context.clusterIndex = startOffset/discBytePerClus[0];
		file->currentOffset=context.clusterIndex*discBytePerClus[0];
		#else
		context.clusterIndex = startOffset/discBytePerClus;
		file->currentOffset=context.clusterIndex*discBytePerClus;
		#endif
		file->currentCluster = getCachedCluster(file, context.clusterIndex);
	} else {
        #ifdef DEBUG
        nocashMessage("fatTable not cached");
        #endif
		if(startOffset<file->currentOffset) {
			file->currentOffset=0;
			file->currentCluster = file->firstCluster;
		}

		// Follow cluster list until desired one is found
		#ifdef TWOCARD
		for (int chunks = (startOffset-file->currentOffset) / discBytePerClus[0]; chunks > 0; chunks--)
		{
			file->currentCluster = FAT_NextCluster (file->currentCluster, false);
			file->currentOffset+=discBytePerClus[0];
		}
		#else
		for (int chunks = (startOffset-file->currentOffset) / discBytePerClus; chunks > 0; chunks--)
		{
			file->currentCluster = FAT_NextCluster (file->currentCluster);
			file->currentOffset+=discBytePerClus;
		}
		#endif
	}

	// Calculate the sector and byte of the current position,
	// and store them
	#ifdef TWOCARD
	context.curSect = (startOffset % discBytePerClus[0]) / BYTES_PER_SECTOR;
	#else
	context.curSect = (startOffset % discBytePerClus) / BYTES_PER_SECTOR;
	#endif
	context.curByte = startOffset % BYTES_PER_SECTOR;
    context.dataPos=0;
    beginBytes = (BYTES_PER_SECTOR < length + context.curByte ? (BYTES_PER_SECTOR - context.curByte) : length);

	// Load sector buffer for new position in file
	#ifdef TWOCARD
	CARD_ReadSector( context.curSect + FAT_ClustToSect(file->currentCluster, false), buffer+context.dataPos, context.curByte, 0, false);
	#else
	CARD_ReadSector( context.curSect + FAT_ClustToSect(file->currentCluster), buffer+context.dataPos, context.curByte, 0);
	#endif
	context.curSect++;

    context.curByte+=beginBytes;
    context.dataPos+=beginBytes;

    context.chunks = ((int)length - beginBytes) / BYTES_PER_SECTOR;
    context.cmd=0;

    return false;
}

bool resumeFileRead()
{
    if(context.cmd == 0 || CARD_CheckCommand(context.cmd, context.ndmaSlot))
    {
        if(context.chunks>0)
        {
    		int sectorsToRead=0;

    		if(context.file->fatTableCached) {
              // Move to the next cluster if necessary
			  #ifdef TWOCARD
              if (context.curSect >= discSecPerClus[0])
    			{
				  context.clusterIndex+= context.curSect/discSecPerClus[0];
				  context.curSect = context.curSect % discSecPerClus[0];
				  context.file->currentCluster = context.file->fatTableCache[context.clusterIndex];
				  context.file->currentOffset+=discBytePerClus[0];
    			}
			  #else
              if (context.curSect >= discSecPerClus)
    			{
				  context.clusterIndex+= context.curSect/discSecPerClus;
				  context.curSect = context.curSect % discSecPerClus;
				  context.file->currentCluster = context.file->fatTableCache[context.clusterIndex];
				  context.file->currentOffset+=discBytePerClus;
    			}
			  #endif

               // Calculate how many sectors to read (try to group several cluster at a time if there is no fragmentation)
              for(int tempClusterIndex=context.clusterIndex; sectorsToRead<=context.chunks; ) {   
                  if(context.file->fatTableCache[tempClusterIndex]+1 == context.file->fatTableCache[tempClusterIndex+1]) {
                      #ifdef DEBUG
                  	nocashMessage("contiguous read");
                  	#endif
                      // the 2 cluster are consecutive
					  #ifdef TWOCARD
                      sectorsToRead += discSecPerClus[0];
					  #else
                      sectorsToRead += discSecPerClus;
					  #endif
                      tempClusterIndex++;    
                  } else {
                      #ifdef DEBUG
                  	nocashMessage("non contiguous read");
                  	#endif
                      break;
                  }
              }

			  #ifdef TWOCARD
              if(!sectorsToRead) sectorsToRead = discSecPerClus[0] - context.curSect;
			  #else
              if(!sectorsToRead) sectorsToRead = discSecPerClus - context.curSect;
			  #endif
              else sectorsToRead = sectorsToRead - context.curSect;

              if(context.chunks < sectorsToRead) {
  		    sectorsToRead = context.chunks;
              }

              #ifdef DEBUG
			  #ifdef TWOCARD
              dbg_hexa(context.curSect + FAT_ClustToSect(context.file->currentCluster, false));
			  #else
              dbg_hexa(context.curSect + FAT_ClustToSect(context.file->currentCluster));
			  #endif
              dbg_hexa(sectorsToRead);
              dbg_hexa(context.buffer + context.dataPos);
              #endif

              // Read the sectors
				#ifdef TWOCARD
    			CARD_ReadSectorsNonBlocking(context.curSect + FAT_ClustToSect(context.file->currentCluster, false), sectorsToRead, context.buffer + context.dataPos, context.ndmaSlot);
				#else
    			CARD_ReadSectorsNonBlocking(context.curSect + FAT_ClustToSect(context.file->currentCluster), sectorsToRead, context.buffer + context.dataPos, context.ndmaSlot);
				#endif
    			context.chunks  -= sectorsToRead;
    			context.curSect += sectorsToRead;
    			context.dataPos += BYTES_PER_SECTOR * sectorsToRead;
              #ifdef DEBUG
			  #ifdef TWOCARD
              dbg_hexa(discSecPerClus[0]);
              dbg_hexa(context.curSect/discSecPerClus[0]);
			  #else
              dbg_hexa(discSecPerClus);
              dbg_hexa(context.curSect/discSecPerClus);
			  #endif
              #endif
			  #ifdef TWOCARD
              context.clusterIndex+= context.curSect/discSecPerClus[0];
              context.curSect = context.curSect % discSecPerClus[0];
			  #else
              context.clusterIndex+= context.curSect/discSecPerClus;
              context.curSect = context.curSect % discSecPerClus;
			  #endif
			  context.file->currentCluster = getCachedCluster(context.file, context.clusterIndex);
              context.cmd=0x33C12;
              return false;         
          } else {
              // Move to the next cluster if necessary
			#ifdef TWOCARD
  			if (context.curSect >= discSecPerClus[0])
  			{
  				context.curSect = 0;
                  context.file->currentCluster = FAT_NextCluster (context.file->currentCluster, false);
  				context.file->currentOffset+=discBytePerClus[0];
  			}
			#else
  			if (context.curSect >= discSecPerClus)
  			{
  				context.curSect = 0;
                  context.file->currentCluster = FAT_NextCluster (context.file->currentCluster);
  				context.file->currentOffset+=discBytePerClus;
  			}
			#endif

              // Calculate how many sectors to read (read a maximum of discSecPerClus at a time)
 			#ifdef TWOCARD
     	    sectorsToRead = discSecPerClus[0] - context.curSect;
			#else
     	    sectorsToRead = discSecPerClus - context.curSect;
			#endif
      	    if(context.chunks < sectorsToRead)
      		sectorsToRead = context.chunks;

              // Read the sectors
 			#ifdef TWOCARD
  			CARD_ReadSectorsNonBlocking(context.curSect + FAT_ClustToSect(context.file->currentCluster, false), sectorsToRead, context.buffer + context.dataPos, context.ndmaSlot);
			#else
  			CARD_ReadSectorsNonBlocking(context.curSect + FAT_ClustToSect(context.file->currentCluster), sectorsToRead, context.buffer + context.dataPos, context.ndmaSlot);
			#endif
  			context.chunks  -= sectorsToRead;
  			context.curSect += sectorsToRead;
  			context.dataPos += BYTES_PER_SECTOR * sectorsToRead;
            context.cmd=0x33C12;
            return false;         
          }			
  	}
      else
      {
          // Take care of any bytes left over before end of read
      	if (context.dataPos < context.length)
      	{
              #ifdef DEBUG
            	nocashMessage("non aligned read, data is missing");
              if(context.length-context.dataPos>BYTES_PER_SECTOR) {
                  nocashMessage("error: unread sector are missing");
              }
              #endif

      		// Update the read buffer
      		context.curByte = 0;
			#ifdef TWOCARD
      		if (context.curSect >= discSecPerClus[0])
      		{
      			if(context.file->fatTableCached) {
                        context.clusterIndex+= context.curSect/discSecPerClus[0];
                        context.curSect = context.curSect % discSecPerClus[0];
						context.file->currentCluster = getCachedCluster(context.file, context.clusterIndex);
                    } else {
                        context.curSect = 0;
                        context.file->currentCluster = FAT_NextCluster (context.file->currentCluster, false);
                    }
      			context.file->currentOffset+=discBytePerClus[0];
      		}
			#else
      		if (context.curSect >= discSecPerClus)
      		{
      			if(context.file->fatTableCached) {
                        context.clusterIndex+= context.curSect/discSecPerClus;
                        context.curSect = context.curSect % discSecPerClus;
						context.file->currentCluster = getCachedCluster(context.file, context.clusterIndex);
                    } else {
                        context.curSect = 0;
                        context.file->currentCluster = FAT_NextCluster (context.file->currentCluster);
                    }
      			context.file->currentOffset+=discBytePerClus;
      		}
			#endif

              #ifdef DEBUG
			  #ifdef TWOCARD
              dbg_hexa(context.curSect + FAT_ClustToSect(context.file->currentCluster, false));
              dbg_hexa(lastGlobalBuffer[0]);
			  #else
              dbg_hexa(context.curSect + FAT_ClustToSect(context.file->currentCluster));
              dbg_hexa(lastGlobalBuffer);
			  #endif
              #endif

			#ifdef TWOCARD
            CARD_ReadSector( context.curSect + FAT_ClustToSect(context.file->currentCluster, false), lastGlobalBuffer[0], 0, 0, false);    
      		//CARD_ReadSector( context.curSect + FAT_ClustToSect(context.file->currentCluster, false), context.buffer+context.dataPos, 0, 512-(context.length-context.dataPos), false);
			#else
            CARD_ReadSector( context.curSect + FAT_ClustToSect(context.file->currentCluster), lastGlobalBuffer, 0, 0);    
      		//CARD_ReadSector( context.curSect + FAT_ClustToSect(context.file->currentCluster, false), context.buffer+context.dataPos, 0, 512-(context.length-context.dataPos));
			#endif

      		// Read in last partial chunk
			  #ifdef TWOCARD
              tonccpy(context.buffer+context.dataPos,lastGlobalBuffer[0]+context.curByte,context.length-context.dataPos);
			  #else
              tonccpy(context.buffer+context.dataPos,lastGlobalBuffer+context.curByte,context.length-context.dataPos);
			  #endif

              context.curByte+=context.length;
              context.dataPos+=context.length;
      	}
          return true;
      }

    }  
	return false;
}
#endif
#endif

/*-----------------------------------------------------------------
fileRead(buffer, cluster, startOffset, length)
-----------------------------------------------------------------*/
#ifndef B4DS
u32 fileRead (char* buffer, aFile file, u32 startOffset, u32 length, int ndmaSlot)
#else
u32 fileRead (char* buffer, aFile file, u32 startOffset, u32 length)
#endif
{
	#ifdef DEBUG
	nocashMessage("fileRead");
    dbg_hexa(buffer);   
    dbg_hexa(startOffset);
    dbg_hexa(length);
	#endif

	int curByte;
	int curSect;

	int dataPos = 0;
	int chunks;
	int beginBytes;
    
    u32 clusterIndex = 0;

	if (file.firstCluster == CLUSTER_FREE || file.firstCluster == CLUSTER_EOF) 
	{
		return 0;
	}

	if (file.fatTableCached) {
    	#ifdef DEBUG
        nocashMessage("fat table cached");
        #endif
		#ifdef TWOCARD
		clusterIndex = startOffset/discBytePerClus[file.card2];
		file.currentOffset=clusterIndex*discBytePerClus[file.card2];
		#else
		clusterIndex = startOffset/discBytePerClus;
		file.currentOffset=clusterIndex*discBytePerClus;
		#endif
		file.currentCluster = getCachedCluster(&file, clusterIndex);
	} else {
        #ifdef DEBUG
        nocashMessage("fatTable not cached");
        #endif
		if(startOffset<file.currentOffset) {
			file.currentOffset=0;
			file.currentCluster = file.firstCluster;
		}

		// Follow cluster list until desired one is found
		#ifdef TWOCARD
		for (chunks = (startOffset-file.currentOffset) / discBytePerClus[file.card2]; chunks > 0; chunks--)
		{
			file.currentCluster = FAT_NextCluster (file.currentCluster, file.card2);
			file.currentOffset+=discBytePerClus[file.card2];
		}
		#else
		for (chunks = (startOffset-file.currentOffset) / discBytePerClus; chunks > 0; chunks--)
		{
			file.currentCluster = FAT_NextCluster (file.currentCluster);
			file.currentOffset+=discBytePerClus;
		}
		#endif
	}

	// Calculate the sector and byte of the current position,
	// and store them
	#ifdef TWOCARD
	curSect = (startOffset % discBytePerClus[file.card2]) / BYTES_PER_SECTOR;
	#else
	curSect = (startOffset % discBytePerClus) / BYTES_PER_SECTOR;
	#endif
	curByte = startOffset % BYTES_PER_SECTOR;

	// Load sector buffer for new position in file
	#ifdef TWOCARD
	if (prevFirstClust[file.card2] != file.firstCluster || prevSect[file.card2] != curSect || prevClust[file.card2] != file.currentCluster) {
		prevFirstClust[file.card2] = file.firstCluster;
		CARD_ReadSectors( curSect + FAT_ClustToSect(file.currentCluster, file.card2), 1, globalBuffer[file.card2], ndmaSlot, file.card2);
		prevSect[file.card2] = curSect;
		prevClust[file.card2] = file.currentCluster;
	}
	#else
	if (prevFirstClust != file.firstCluster || prevSect != curSect || prevClust != file.currentCluster) {
		prevFirstClust = file.firstCluster;
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0, 0);
		prevSect = curSect;
		prevClust = file.currentCluster;
	}
	#endif
	curSect++;

	// Number of bytes needed to read to align with a sector
	beginBytes = (BYTES_PER_SECTOR < length + curByte ? (BYTES_PER_SECTOR - curByte) : length);

	// Read first part from buffer, to align with sector boundary
    dataPos=0;
	#ifdef TWOCARD
    tonccpy(buffer,globalBuffer[file.card2]+curByte,beginBytes);
	#else
    tonccpy(buffer,globalBuffer+curByte,beginBytes);
	#endif
    curByte+=beginBytes;
    dataPos+=beginBytes;

	// Read in all the 512 byte chunks of the file directly, saving time
	for ( chunks = ((int)length - beginBytes) / BYTES_PER_SECTOR; chunks > 0;)
	{
		int sectorsToRead=0;

		if(file.fatTableCached) {
          
              // Move to the next cluster if necessary
			  #ifdef TWOCARD
              if (curSect >= discSecPerClus[file.card2])
  			{
                  clusterIndex+= curSect/discSecPerClus[file.card2];
                  curSect = curSect % discSecPerClus[file.card2];
  				file.currentOffset+=discBytePerClus[file.card2];
				file.currentCluster = getCachedCluster(&file, clusterIndex);
			}
				#else
              if (curSect >= discSecPerClus)
  			{
                  clusterIndex+= curSect/discSecPerClus;
                  curSect = curSect % discSecPerClus;
  				file.currentOffset+=discBytePerClus;
				file.currentCluster = getCachedCluster(&file, clusterIndex);
  			}
			  #endif

               // Calculate how many sectors to read (try to group several cluster at a time if there is no fragmentation)
              for(int tempClusterIndex=clusterIndex; sectorsToRead<=chunks; ) {   
                  if(file.fatTableCache[tempClusterIndex]+1 == file.fatTableCache[tempClusterIndex+1]) {
                      #ifdef DEBUG
                  	nocashMessage("contiguous read");
                  	#endif
                      // the 2 cluster are consecutive
					#ifdef TWOCARD
                      sectorsToRead += discSecPerClus[file.card2];
					#else
                      sectorsToRead += discSecPerClus;
					#endif
                      tempClusterIndex++;    
                  } else {
                      #ifdef DEBUG
                  	nocashMessage("non contiguous read");
                  	#endif
                      break;
                  }
              }

			  #ifdef TWOCARD
              if(!sectorsToRead) sectorsToRead = discSecPerClus[file.card2] - curSect;
			  #else
              if(!sectorsToRead) sectorsToRead = discSecPerClus - curSect;
			  #endif
              else sectorsToRead = sectorsToRead - curSect;
              
              if(chunks < sectorsToRead) {
			    sectorsToRead = chunks;
              }

              #ifdef DEBUG
			  #ifdef TWOCARD
              dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster, file.card2));
			  #else
              dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster));
			  #endif
              dbg_hexa(sectorsToRead);
              dbg_hexa(buffer + dataPos);
              #endif

              // Read the sectors
			  #ifndef B4DS
			  #ifdef TWOCARD
    			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster, file.card2), sectorsToRead, buffer + dataPos, ndmaSlot, file.card2);
			  #else
    			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToRead, buffer + dataPos, ndmaSlot);
			  #endif
			  #else
    			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToRead, buffer + dataPos, 0);
			  #endif
    			chunks  -= sectorsToRead;
    			curSect += sectorsToRead;
    			dataPos += BYTES_PER_SECTOR * sectorsToRead;

              #ifdef DEBUG
			  #ifdef TWOCARD
              dbg_hexa(discSecPerClus[file.card2]);
              dbg_hexa(curSect/discSecPerClus[file.card2]);
			  #else
              dbg_hexa(discSecPerClus);
              dbg_hexa(curSect/discSecPerClus);
              #endif
              #endif

			  #ifdef TWOCARD
              clusterIndex+= curSect/discSecPerClus[file.card2];
              curSect = curSect % discSecPerClus[file.card2];
			  #else
              clusterIndex+= curSect/discSecPerClus;
              curSect = curSect % discSecPerClus;
              #endif
				file.currentCluster = getCachedCluster(&file, clusterIndex);
          } else {
              // Move to the next cluster if necessary
			#ifdef TWOCARD
  			if (curSect >= discSecPerClus[file.card2])
  			{
  				curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster, file.card2);
  				file.currentOffset+=discBytePerClus[file.card2];
  			}
			#else
  			if (curSect >= discSecPerClus)
  			{
  				curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster);
  				file.currentOffset+=discBytePerClus;
  			}
			#endif

              // Calculate how many sectors to read (read a maximum of discSecPerClus at a time)
			#ifndef B4DS
			#ifdef TWOCARD
		    sectorsToRead = discSecPerClus[file.card2] - curSect;
		    if(chunks < sectorsToRead)
			sectorsToRead = chunks;

              // Read the sectors
  			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster, file.card2), sectorsToRead, buffer + dataPos, ndmaSlot, file.card2);
			#else
		    sectorsToRead = discSecPerClus - curSect;
		    if(chunks < sectorsToRead)
			sectorsToRead = chunks;

              // Read the sectors
  			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToRead, buffer + dataPos, ndmaSlot);
			#endif
			#else
		    sectorsToRead = discSecPerClus - curSect;
		    if(chunks < sectorsToRead)
			sectorsToRead = chunks;
              
              // Read the sectors
  			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToRead, buffer + dataPos, 0);
			#endif
  			chunks  -= sectorsToRead;
  			curSect += sectorsToRead;
  			dataPos += BYTES_PER_SECTOR * sectorsToRead;
          }			
	}

	// Take care of any bytes left over before end of read
	if (dataPos < length)
	{
          #ifdef DEBUG
        	nocashMessage("non aligned read, data is missing");
          if(length-dataPos>BYTES_PER_SECTOR) {
              nocashMessage("error: unread sector are missing");
          }
          #endif

		// Update the read buffer
		#ifdef TWOCARD
		if (curSect >= discSecPerClus[file.card2])
		{
			if(file.fatTableCached) {
                  clusterIndex+= curSect/discSecPerClus[file.card2];
                  curSect = curSect % discSecPerClus[file.card2];
				file.currentCluster = getCachedCluster(&file, clusterIndex);
              } else {
                  curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster, file.card2);
              }
			file.currentOffset+=discBytePerClus[file.card2];
		}
		#else
		if (curSect >= discSecPerClus)
		{
			if(file.fatTableCached) {
                  clusterIndex+= curSect/discSecPerClus;
                  curSect = curSect % discSecPerClus;
				file.currentCluster = getCachedCluster(&file, clusterIndex);
              } else {
                  curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster);
              }
			file.currentOffset+=discBytePerClus;
		}
		#endif

          #ifdef DEBUG
		  #ifdef TWOCARD
          dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster, file.card2));
          dbg_hexa(lastGlobalBuffer[file.card2]);
		  #else
          dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster));
		  #endif
          dbg_hexa(lastGlobalBuffer);
          #endif

		#ifdef TWOCARD
		CARD_ReadSectors( curSect + FAT_ClustToSect(file.currentCluster, file.card2), 1, lastGlobalBuffer[file.card2], ndmaSlot, file.card2);
		#else
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), lastGlobalBuffer, 0, 0);
		#endif

		// Read in last partial chunk
		  #ifdef TWOCARD
          tonccpy(buffer+dataPos,lastGlobalBuffer[file.card2],length-dataPos);
		  #else
          tonccpy(buffer+dataPos,lastGlobalBuffer,length-dataPos);
		  #endif
          curByte+=length;
          dataPos+=length;
	}

      #ifdef DEBUG
      nocashMessage("fileRead completed");
      nocashMessage("");
      #endif

	return dataPos;
	
}

/*-----------------------------------------------------------------
fileWrite(buffer, cluster, startOffset, length)
-----------------------------------------------------------------*/
#ifndef B4DS
u32 fileWrite (const char* buffer, aFile file, u32 startOffset, u32 length, int ndmaSlot)
#else
u32 fileWrite (const char* buffer, aFile file, u32 startOffset, u32 length)
#endif
{
	#ifdef DEBUG
	nocashMessage("fileWrite");
	#endif

	int curByte;
	int curSect;

	int dataPos = 0;
	int chunks;
	int beginBytes;
    u32 clusterIndex = 0;

	if (file.firstCluster == CLUSTER_FREE || file.firstCluster == CLUSTER_EOF) 
	{
		#ifdef DEBUG
		nocashMessage("CLUSTER_FREE or CLUSTER_EOF");
		#endif
		return 0;
	}

	if(file.fatTableCached) {
		#ifdef TWOCARD
		clusterIndex = startOffset/discBytePerClus[file.card2];
		file.currentOffset=clusterIndex*discBytePerClus[file.card2];
		#else
		clusterIndex = startOffset/discBytePerClus;
		file.currentOffset=clusterIndex*discBytePerClus;
		#endif
		file.currentCluster = getCachedCluster(&file, clusterIndex);
	} else {
		if(startOffset<file.currentOffset) {
			file.currentOffset=0;
			file.currentCluster = file.firstCluster;
		}

		// Follow cluster list until desired one is found
		#ifdef TWOCARD
		for (chunks = (startOffset-file.currentOffset) / discBytePerClus[file.card2]; chunks > 0; chunks--)
		{
			file.currentCluster = FAT_NextCluster (file.currentCluster, file.card2);
			file.currentOffset+=discBytePerClus[file.card2];
		}
		#else
		for (chunks = (startOffset-file.currentOffset) / discBytePerClus; chunks > 0; chunks--)
		{
			file.currentCluster = FAT_NextCluster (file.currentCluster);
			file.currentOffset+=discBytePerClus;
		}
		#endif
	}

	// Calculate the sector and byte of the current position,
	// and store them
	#ifdef TWOCARD
	curSect = (startOffset % discBytePerClus[file.card2]) / BYTES_PER_SECTOR;
	#else
	curSect = (startOffset % discBytePerClus) / BYTES_PER_SECTOR;
	#endif
	curByte = startOffset % BYTES_PER_SECTOR;

	// Load sector buffer for new position in file
	#ifdef TWOCARD
	if (prevFirstClust[file.card2] != file.firstCluster || prevSect[file.card2] != curSect || prevClust[file.card2] != file.currentCluster) {
		prevFirstClust[file.card2] = file.firstCluster;
		CARD_ReadSectors( curSect + FAT_ClustToSect(file.currentCluster, file.card2), 1, globalBuffer[file.card2], ndmaSlot, file.card2);
		prevSect[file.card2] = curSect;
		prevClust[file.card2] = file.currentCluster;
	}
	#else
	if (prevFirstClust != file.firstCluster || prevSect != curSect || prevClust != file.currentCluster) {
		prevFirstClust = file.firstCluster;
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0, 0);
		prevSect = curSect;
		prevClust = file.currentCluster;
	}
	#endif


	// Number of bytes needed to read to align with a sector
	beginBytes = (BYTES_PER_SECTOR < length + curByte ? (BYTES_PER_SECTOR - curByte) : length);

	// Read first part from buffer, to align with sector boundary
    dataPos=0;
	#ifdef TWOCARD
    tonccpy(globalBuffer[file.card2]+curByte,buffer,beginBytes);
	#else
    tonccpy(globalBuffer+curByte,buffer,beginBytes);
	#endif
    curByte+=beginBytes;
    dataPos+=beginBytes;

	#ifndef B4DS
	#ifdef TWOCARD
	CARD_WriteSector(curSect + FAT_ClustToSect(file.currentCluster, file.card2), globalBuffer[file.card2], ndmaSlot, file.card2);
	#else
	CARD_WriteSector(curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, ndmaSlot);
	#endif
	#else
	CARD_WriteSector(curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0);
	#endif

	curSect++;

	// Read in all the 512 byte chunks of the file directly, saving time
	for ( chunks = ((int)length - beginBytes) / BYTES_PER_SECTOR; chunks > 0;)
	{
		int sectorsToWrite;

		// Move to the next cluster if necessary
		#ifndef B4DS
		#ifdef TWOCARD
		if (curSect >= discSecPerClus[file.card2])
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = getCachedCluster(&file, clusterIndex);
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster, file.card2);
			    
            }
            file.currentOffset+=discBytePerClus[file.card2];
			curSect = 0;
		}
		#else
		if (curSect >= discSecPerClus)
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = getCachedCluster(&file, clusterIndex); 
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster);
			    
            }
            file.currentOffset+=discBytePerClus;
			curSect = 0;
		}
		#endif

		// Calculate how many sectors to read (read a maximum of discSecPerClus at a time)
		#ifdef TWOCARD
		sectorsToWrite = discSecPerClus[file.card2] - curSect;
		#else
		sectorsToWrite = discSecPerClus - curSect;
		#endif
		if(chunks < sectorsToWrite)
			sectorsToWrite = chunks;

		// Read the sectors
		#ifdef TWOCARD
		CARD_WriteSectors(curSect + FAT_ClustToSect(file.currentCluster, file.card2), sectorsToWrite, buffer + dataPos, ndmaSlot, file.card2);
		#else
		CARD_WriteSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToWrite, buffer + dataPos, ndmaSlot);
		#endif
		#else
		if (curSect >= discSecPerClus)
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = getCachedCluster(&file, clusterIndex); 
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster);
			    
            }
            file.currentOffset+=discBytePerClus;
			curSect = 0;
		}

		// Calculate how many sectors to read (read a maximum of discSecPerClus at a time)
		sectorsToWrite = discSecPerClus - curSect;
		if(chunks < sectorsToWrite)
			sectorsToWrite = chunks;

		// Read the sectors
		CARD_WriteSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToWrite, buffer + dataPos, 0);
		#endif

		chunks  -= sectorsToWrite;
		curSect += sectorsToWrite;
		dataPos += BYTES_PER_SECTOR * sectorsToWrite;
	}

	// Take care of any bytes left over before end of read
	if (dataPos < length)
	{
		#ifndef B4DS
		// Update the read buffer
		curByte = 0;
		#ifdef TWOCARD
		if (curSect >= discSecPerClus[file.card2])
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = getCachedCluster(&file, clusterIndex);
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster, file.card2);
            }
			curSect = 0;
			file.currentOffset+=discBytePerClus[file.card2];
		}
		CARD_ReadSectors( curSect + FAT_ClustToSect(file.currentCluster, file.card2), 1, lastGlobalBuffer[file.card2], ndmaSlot, file.card2);
		#else
		if (curSect >= discSecPerClus)
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = getCachedCluster(&file, clusterIndex);
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster);
            }
			curSect = 0;
			file.currentOffset+=discBytePerClus;
		}
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), lastGlobalBuffer, 0, 0);
		#endif

		// Read in last partial chunk
		#ifdef TWOCARD
        tonccpy(lastGlobalBuffer[file.card2]+curByte,buffer+dataPos,length-dataPos);
		#else
        tonccpy(lastGlobalBuffer+curByte,buffer+dataPos,length-dataPos);
		#endif
        curByte+=length;
        dataPos+=length;

		#ifdef TWOCARD
		CARD_WriteSector( curSect + FAT_ClustToSect(file.currentCluster, file.card2), lastGlobalBuffer[file.card2], ndmaSlot, file.card2);
		#else
		CARD_WriteSector( curSect + FAT_ClustToSect(file.currentCluster), lastGlobalBuffer, ndmaSlot);
		#endif
		#else
		// Update the read buffer
		if (curSect >= discSecPerClus)
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = getCachedCluster(&file, clusterIndex);
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster);
            }
			curSect = 0;
			file.currentOffset+=discBytePerClus;
		}
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), lastGlobalBuffer, 0, 0);

		// Read in last partial chunk
        tonccpy(lastGlobalBuffer,buffer+dataPos,length-dataPos);
        curByte+=length;
        dataPos+=length;

		CARD_WriteSector( curSect + FAT_ClustToSect(file.currentCluster), lastGlobalBuffer, 0);
		#endif
	}

	return dataPos;
}

void buildFatTableCache (aFile * file)
{
	if (file->fatTableCached) return;

    #ifdef DEBUG
	nocashMessage("buildFatTableCache");
    #endif
    
	file->currentOffset=0;
	file->currentCluster = file->firstCluster;

	file->fatTableCache = lastClusterCacheUsed;

	// Follow cluster list until desired one is found
	while (file->currentCluster != CLUSTER_EOF && file->firstCluster != CLUSTER_FREE 
		&& (u32)lastClusterCacheUsed<clusterCache+clusterCacheSize)
	{
		*lastClusterCacheUsed = file->currentCluster;
#ifdef TWOCARD
		file->currentOffset+=discBytePerClus[file->card2];
		file->currentCluster = FAT_NextCluster (file->currentCluster, file->card2);
#else
		file->currentOffset+=discBytePerClus;
		file->currentCluster = FAT_NextCluster (file->currentCluster);
#endif
		lastClusterCacheUsed++;
#ifndef B4DS
		currentClusterCacheSize += 4;
#endif
		file->fatTableCacheSize += 4;
	}

	if(file->currentCluster == CLUSTER_EOF) {
        #ifdef DEBUG
        nocashMessage("fat table cached");
        #endif
		file->fatTableCached = true;
	}
    #ifdef DEBUG 
    else {
      nocashMessage("fat table not cached");
	  file->fatTableCacheSize = 0;
    }
    #endif

	file->currentOffset=0;
	file->currentCluster = file->firstCluster;
}

void buildFatTableCacheCompressed (aFile * file)
{
	if (file->fatTableCached) return;

    #ifdef DEBUG
	nocashMessage("buildFatTableCacheCompressed");
    #endif
    
	file->currentOffset=0;
	file->currentCluster = file->firstCluster;

	file->fatTableCache = lastClusterCacheUsed;

	// Follow cluster list until desired one is found
	while (file->currentCluster != CLUSTER_EOF && file->firstCluster != CLUSTER_FREE 
		&& (u32)lastClusterCacheUsed<clusterCache+clusterCacheSize)
	{
		u32 clusterNext = file->currentCluster;
		int clusterCount = 1; // Adjacent cluster count

		*lastClusterCacheUsed = file->currentCluster;
		while (file->currentCluster != CLUSTER_EOF && file->firstCluster != CLUSTER_FREE) {
			#ifdef TWOCARD
			file->currentOffset+=discBytePerClus[file->card2];
			file->currentCluster = FAT_NextCluster (file->currentCluster, file->card2);
			#else
			file->currentOffset+=discBytePerClus;
			file->currentCluster = FAT_NextCluster (file->currentCluster);
			#endif
			clusterNext++;
			if (file->currentCluster == clusterNext) {
				clusterCount++;
			} else {
				break;
			}
		}
		lastClusterCacheUsed++;
		#ifndef B4DS
		currentClusterCacheSize += 4;
		#endif
		file->fatTableCacheSize += 4;

		*lastClusterCacheUsed = clusterCount;
		lastClusterCacheUsed++;
		#ifndef B4DS
		currentClusterCacheSize += 4;
		#endif
		file->fatTableCacheSize += 4;
	}

	if(file->currentCluster == CLUSTER_EOF) {
        #ifdef DEBUG
        nocashMessage("fat table cached and compressed");
        #endif
		file->fatTableCached = true;
		file->fatTableCompressed = true;
	}
    #ifdef DEBUG 
    else {
      nocashMessage("fat table not cached and compressed");
	  file->fatTableCacheSize = 0;
    }
    #endif

	file->currentOffset=0;
	file->currentCluster = file->firstCluster;
}