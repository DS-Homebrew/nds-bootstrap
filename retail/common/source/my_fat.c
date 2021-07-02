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

// Boot Sector - must be packed
typedef struct
{
	u8	jmpBoot[3];
	u8	OEMName[8];
	// BIOS Parameter Block
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
		}	fat16;
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
		}	fat32;
	}	extBlock;

	__PACKED	u16	bootSig;

}	__PACKED BOOT_SEC;

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
#endif

enum {FS_UNKNOWN, FS_FAT12, FS_FAT16, FS_FAT32}
#ifdef TWOCARD
discFileSystem[2];
#else
discFileSystem;
#endif

// Global sector buffer to save on stack space
unsigned char globalBuffer[BYTES_PER_SECTOR];

#define CLUSTER_CACHE      0x2700000 // Main RAM
#define CLUSTER_CACHE_SIZE 0x7FF80 // 512K

#ifndef B4DS
static u32* lastClusterCacheUsed = (u32*) CLUSTER_CACHE;
#else
u32* lastClusterCacheUsed = (u32*) CLUSTER_CACHE;
u32 clusterCache = CLUSTER_CACHE;
u32 clusterCacheSize = CLUSTER_CACHE_SIZE;
#endif


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
			#else
			sector = discFAT + (((cluster * 3) / 2) / BYTES_PER_SECTOR);
			#endif
			offset = ((cluster * 3) / 2) % BYTES_PER_SECTOR;
			CARD_ReadSector(sector, globalBuffer, 0, 0);
			nextCluster = ((u8*) globalBuffer)[offset];
			offset++;

			if (offset >= BYTES_PER_SECTOR) {
				offset = 0;
				sector++;
			}

			CARD_ReadSector(sector, globalBuffer, 0, 0);
			nextCluster |= (((u8*) globalBuffer)[offset]) << 8;

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

			CARD_ReadSector(sector, globalBuffer, 0, 0);
			// read the nextCluster value
			nextCluster = ((u16*)globalBuffer)[offset];

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

			CARD_ReadSector(sector, globalBuffer, 0, 0);
			// read the nextCluster value
			nextCluster = (((u32*)globalBuffer)[offset]) & 0x0FFFFFFF;

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
	CARD_ReadSector (bootSector,  bootSec, 0, 0);

	// Store required information about the file system
	#ifdef TWOCARD
	if (bootSec->sectorsPerFAT != 0)
	{
		discSecPerFAT[card2] = bootSec->sectorsPerFAT;
	}
	else
	{
		discSecPerFAT[card2] = bootSec->extBlock.fat32.sectorsPerFAT32;
	}

	if (bootSec->numSectorsSmall != 0)
	{
		discNumSec[card2] = bootSec->numSectorsSmall;
	}
	else
	{
		discNumSec[card2] = bootSec->numSectors;
	}

	discBytePerSec[card2] = BYTES_PER_SECTOR;	// Sector size is redefined to be 512 bytes
	discSecPerClus[card2] = bootSec->sectorsPerCluster * bootSec->bytesPerSector / BYTES_PER_SECTOR;
	discBytePerClus[card2] = discBytePerSec[card2] * discSecPerClus[card2];
	discFAT[card2] = bootSector + bootSec->reservedSectors;

	discRootDir[card2] = discFAT[card2] + (bootSec->numFATs * discSecPerFAT[card2]);
	discData[card2] = discRootDir[card2] + ((bootSec->rootEntries * sizeof(DIR_ENT)) / BYTES_PER_SECTOR);

	if ((discNumSec[card2] - discData[card2]) / bootSec->sectorsPerCluster < 4085)
	{
		discFileSystem[card2] = FS_FAT12;
	}
	else if ((discNumSec[card2] - discData[card2]) / bootSec->sectorsPerCluster < 65525)
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
	if (bootSec->sectorsPerFAT != 0)
	{
		discSecPerFAT = bootSec->sectorsPerFAT;
	}
	else
	{
		discSecPerFAT = bootSec->extBlock.fat32.sectorsPerFAT32;
	}

	if (bootSec->numSectorsSmall != 0)
	{
		discNumSec = bootSec->numSectorsSmall;
	}
	else
	{
		discNumSec = bootSec->numSectors;
	}

	discBytePerSec = BYTES_PER_SECTOR;	// Sector size is redefined to be 512 bytes
	discSecPerClus = bootSec->sectorsPerCluster * bootSec->bytesPerSector / BYTES_PER_SECTOR;
	discBytePerClus = discBytePerSec * discSecPerClus;
	discFAT = bootSector + bootSec->reservedSectors;

	discRootDir = discFAT + (bootSec->numFATs * discSecPerFAT);
	discData = discRootDir + ((bootSec->rootEntries * sizeof(DIR_ENT)) / BYTES_PER_SECTOR);

	if ((discNumSec - discData) / bootSec->sectorsPerCluster < 4085)
	{
		discFileSystem = FS_FAT12;
	}
	else if ((discNumSec - discData) / bootSec->sectorsPerCluster < 65525)
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
	#ifndef B4DS
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
	#ifndef B4DS
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
		return file;
	}

	char *ptr = (char*)bootName;
	while (*ptr != '.') ptr++;
	int namelen = ptr - bootName;

//	maxSectors = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? (discData - discRootDir) : discSecPerClus);
	// Scan Dir for correct entry
	#ifdef TWOCARD
	firstSector = discRootDir[card2];
	#else
	firstSector = discRootDir;
	#endif
	CARD_ReadSector (firstSector + wrkDirSector, globalBuffer, 0, 0);
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
			CARD_ReadSector (firstSector + wrkDirSector, globalBuffer, 0, 0);
		}
		dir = ((DIR_ENT*) globalBuffer)[wrkDirOffset];
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
	return file;
}

aFile getFileFromCluster (u32 cluster) {
	aFile file;
	file.firstCluster = cluster;
	file.currentCluster = file.firstCluster;
	file.currentOffset=0;
	file.fatTableCached=false;
	return file;
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

	if (file->firstCluster == CLUSTER_FREE || file->firstCluster == CLUSTER_EOF) 
	{
		return true;
	}

	if(startOffset<file->currentOffset) {
		file->currentOffset=0;
		file->currentCluster = file->firstCluster;
	}

	if(file->fatTableCached) {
    	#ifdef DEBUG
        nocashMessage("fat table cached");
        #endif
		context.clusterIndex = startOffset/discBytePerClus[0];
		file->currentCluster = file->fatTableCache[context.clusterIndex];
		file->currentOffset=context.clusterIndex*discBytePerClus[0];
	} else {
        #ifdef DEBUG
        nocashMessage("fatTable not cached");
        #endif
		if(startOffset<file->currentOffset) {
			file->currentOffset=0;
			file->currentCluster = file->firstCluster;
		}

		// Follow cluster list until desired one is found
		for (int chunks = (startOffset-file->currentOffset) / discBytePerClus[0]; chunks > 0; chunks--)
		{
			file->currentCluster = FAT_NextCluster (file->currentCluster, false);
			file->currentOffset+=discBytePerClus[0];
		}
	}

	// Calculate the sector and byte of the current position,
	// and store them
	context.curSect = (startOffset % discBytePerClus[0]) / BYTES_PER_SECTOR;
	context.curByte = startOffset % BYTES_PER_SECTOR;
    context.dataPos=0;
    beginBytes = (BYTES_PER_SECTOR < length + context.curByte ? (BYTES_PER_SECTOR - context.curByte) : length);

	// Load sector buffer for new position in file
	CARD_ReadSector( context.curSect + FAT_ClustToSect(file->currentCluster, false), buffer+context.dataPos, context.curByte, 0);
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
              if (context.curSect >= discSecPerClus[0])
    			{
                  context.clusterIndex+= context.curSect/discSecPerClus[0];
                  context.curSect = context.curSect % discSecPerClus[0];
                  context.file->currentCluster = context.file->fatTableCache[context.clusterIndex];
    				context.file->currentOffset+=discBytePerClus[0];
    			}

               // Calculate how many sectors to read (try to group several cluster at a time if there is no fragmentation)
              for(int tempClusterIndex=context.clusterIndex; sectorsToRead<=context.chunks; ) {   
                  if(context.file->fatTableCache[tempClusterIndex]+1 == context.file->fatTableCache[tempClusterIndex+1]) {
                      #ifdef DEBUG
                  	nocashMessage("contiguous read");
                  	#endif
                      // the 2 cluster are consecutive
                      sectorsToRead += discSecPerClus[0];
                      tempClusterIndex++;    
                  } else {
                      #ifdef DEBUG
                  	nocashMessage("non contiguous read");
                  	#endif
                      break;
                  }
              }

              if(!sectorsToRead) sectorsToRead = discSecPerClus[0] - context.curSect;  
              else sectorsToRead = sectorsToRead - context.curSect;

              if(context.chunks < sectorsToRead) {
  		    sectorsToRead = context.chunks;
              }

              #ifdef DEBUG
              dbg_hexa(context.curSect + FAT_ClustToSect(context.file->currentCluster, false));
              dbg_hexa(sectorsToRead);
              dbg_hexa(context.buffer + context.dataPos);
              #endif

              // Read the sectors
    			CARD_ReadSectorsNonBlocking(context.curSect + FAT_ClustToSect(context.file->currentCluster, false), sectorsToRead, context.buffer + context.dataPos, context.ndmaSlot);
    			context.chunks  -= sectorsToRead;
    			context.curSect += sectorsToRead;
    			context.dataPos += BYTES_PER_SECTOR * sectorsToRead;
              #ifdef DEBUG
              dbg_hexa(discSecPerClus[0]);
              dbg_hexa(context.curSect/discSecPerClus[0]);
              #endif
              context.clusterIndex+= context.curSect/discSecPerClus[0];
              context.curSect = context.curSect % discSecPerClus[0];
              context.file->currentCluster = context.file->fatTableCache[context.clusterIndex];
              context.cmd=0x33C12;
              return false;         
          } else {
              // Move to the next cluster if necessary
  			if (context.curSect >= discSecPerClus[0])
  			{
  				context.curSect = 0;
                  context.file->currentCluster = FAT_NextCluster (context.file->currentCluster, false);
  				context.file->currentOffset+=discBytePerClus[0];
  			}

              // Calculate how many sectors to read (read a maximum of discSecPerClus at a time)
      	    sectorsToRead = discSecPerClus[0] - context.curSect;
      	    if(context.chunks < sectorsToRead)
      		sectorsToRead = context.chunks;

              // Read the sectors
  			CARD_ReadSectorsNonBlocking(context.curSect + FAT_ClustToSect(context.file->currentCluster, false), sectorsToRead, context.buffer + context.dataPos, context.ndmaSlot);
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
      		if (context.curSect >= discSecPerClus[0])
      		{
      			if(context.file->fatTableCached) {
                        context.clusterIndex+= context.curSect/discSecPerClus[0];
                        context.curSect = context.curSect % discSecPerClus[0];
                        context.file->currentCluster = context.file->fatTableCache[context.clusterIndex]; 
                    } else {
                        context.curSect = 0;
                        context.file->currentCluster = FAT_NextCluster (context.file->currentCluster, false);
                    }
      			context.file->currentOffset+=discBytePerClus[0];
      		}

              #ifdef DEBUG
              dbg_hexa(context.curSect + FAT_ClustToSect(context.file->currentCluster, false));
              dbg_hexa(globalBuffer);
              #endif

            CARD_ReadSector( context.curSect + FAT_ClustToSect(context.file->currentCluster, false), globalBuffer, 0, 0);    
      		//CARD_ReadSector( context.curSect + FAT_ClustToSect(context.file->currentCluster, false), context.buffer+context.dataPos, 0, 512-(context.length-context.dataPos));

      		// Read in last partial chunk
              tonccpy(context.buffer+context.dataPos,globalBuffer+context.curByte,context.length-context.dataPos);
              
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
#ifdef TWOCARD
u32 fileRead (char* buffer, aFile file, u32 startOffset, u32 length, bool card2, int ndmaSlot)
#else
u32 fileRead (char* buffer, aFile file, u32 startOffset, u32 length, int ndmaSlot)
#endif
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

	if(startOffset<file.currentOffset) {
		file.currentOffset=0;
		file.currentCluster = file.firstCluster;
	}

	if(file.fatTableCached) {
    	#ifdef DEBUG
        nocashMessage("fat table cached");
        #endif
		#ifdef TWOCARD
		clusterIndex = startOffset/discBytePerClus[card2];
		file.currentCluster = file.fatTableCache[clusterIndex];
		file.currentOffset=clusterIndex*discBytePerClus[card2];
		#else
		clusterIndex = startOffset/discBytePerClus;
		file.currentCluster = file.fatTableCache[clusterIndex];
		file.currentOffset=clusterIndex*discBytePerClus;
		#endif
	} else {
        #ifdef DEBUG
        nocashMessage("fatTable not cached");
        #endif
		if(startOffset<file.currentOffset) {
			file.currentOffset=0;
			file.currentCluster = file.firstCluster;
		}

		#ifdef TWOCARD
		// Follow cluster list until desired one is found
		for (chunks = (startOffset-file.currentOffset) / discBytePerClus[card2]; chunks > 0; chunks--)
		{
			file.currentCluster = FAT_NextCluster (file.currentCluster, card2);
			file.currentOffset+=discBytePerClus[card2];
		}
		#else
		// Follow cluster list until desired one is found
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
	curSect = (startOffset % discBytePerClus[card2]) / BYTES_PER_SECTOR;
	#else
	curSect = (startOffset % discBytePerClus) / BYTES_PER_SECTOR;
	#endif
	curByte = startOffset % BYTES_PER_SECTOR;

	// Load sector buffer for new position in file
	#ifdef TWOCARD
	CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster, card2), globalBuffer, 0, 0);
	#else
	CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0, 0);
	#endif
	curSect++;

	// Number of bytes needed to read to align with a sector
	beginBytes = (BYTES_PER_SECTOR < length + curByte ? (BYTES_PER_SECTOR - curByte) : length);

	// Read first part from buffer, to align with sector boundary
    dataPos=0;
    tonccpy(buffer+dataPos,globalBuffer+curByte,beginBytes-dataPos);
    curByte+=beginBytes;
    dataPos+=beginBytes;

	// Read in all the 512 byte chunks of the file directly, saving time
	for ( chunks = ((int)length - beginBytes) / BYTES_PER_SECTOR; chunks > 0;)
	{
		int sectorsToRead=0;

		if(file.fatTableCached) {
          
              // Move to the next cluster if necessary
			  #ifdef TWOCARD
              if (curSect >= discSecPerClus[card2])
  			{
                  clusterIndex+= curSect/discSecPerClus[card2];
                  curSect = curSect % discSecPerClus[card2];
                  file.currentCluster = file.fatTableCache[clusterIndex];
  				file.currentOffset+=discBytePerClus[card2];
			}
				#else
              if (curSect >= discSecPerClus)
  			{
                  clusterIndex+= curSect/discSecPerClus;
                  curSect = curSect % discSecPerClus;
                  file.currentCluster = file.fatTableCache[clusterIndex];
  				file.currentOffset+=discBytePerClus;
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
                      sectorsToRead += discSecPerClus[card2];
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
              if(!sectorsToRead) sectorsToRead = discSecPerClus[card2] - curSect;
			  #else
              if(!sectorsToRead) sectorsToRead = discSecPerClus - curSect;
			  #endif
              else sectorsToRead = sectorsToRead - curSect;
              
              if(chunks < sectorsToRead) {
			    sectorsToRead = chunks;
              }
              
              #ifdef DEBUG
			  #ifdef TWOCARD
              dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster, card2));
			  #else
              dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster));
			  #endif
              dbg_hexa(sectorsToRead);
              dbg_hexa(buffer + dataPos);
              #endif
              
              // Read the sectors
			  #ifndef B4DS
			  #ifdef TWOCARD
    			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster, card2), sectorsToRead, buffer + dataPos, ndmaSlot);
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
              dbg_hexa(discSecPerClus[card2]);
              dbg_hexa(curSect/discSecPerClus[card2]);
			  #else
              
              clusterIndex+= curSect/discSecPerClus[card2];
              curSect = curSect % discSecPerClus[card2];
              #endif
              dbg_hexa(discSecPerClus);
              dbg_hexa(curSect/discSecPerClus);
              
              clusterIndex+= curSect/discSecPerClus;
              curSect = curSect % discSecPerClus;
              #endif
              file.currentCluster = file.fatTableCache[clusterIndex];         
          } else {
              // Move to the next cluster if necessary
			#ifdef TWOCARD
  			if (curSect >= discSecPerClus[card2])
  			{
  				curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster, card2);
  				file.currentOffset+=discBytePerClus[card2];
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
		    sectorsToRead = discSecPerClus[card2] - curSect;
		    if(chunks < sectorsToRead)
			sectorsToRead = chunks;
              
              // Read the sectors
  			CARD_ReadSectors(curSect + FAT_ClustToSect(file.currentCluster, card2), sectorsToRead, buffer + dataPos, ndmaSlot);
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
		curByte = 0;
		#ifdef TWOCARD
		if (curSect >= discSecPerClus[card2])
		{
			if(file.fatTableCached) {
                  clusterIndex+= curSect/discSecPerClus[card2];
                  curSect = curSect % discSecPerClus[card2];
                  file.currentCluster = file.fatTableCache[clusterIndex]; 
              } else {
                  curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster, card2);
              }
			file.currentOffset+=discBytePerClus[card2];
		}
		#else
		if (curSect >= discSecPerClus)
		{
			if(file.fatTableCached) {
                  clusterIndex+= curSect/discSecPerClus;
                  curSect = curSect % discSecPerClus;
                  file.currentCluster = file.fatTableCache[clusterIndex]; 
              } else {
                  curSect = 0;
                  file.currentCluster = FAT_NextCluster (file.currentCluster);
              }
			file.currentOffset+=discBytePerClus;
		}
		#endif
          
          #ifdef DEBUG
          dbg_hexa(curSect + FAT_ClustToSect(file.currentCluster));
          dbg_hexa(globalBuffer);
          #endif
          
		#ifdef TWOCARD
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster, card2), globalBuffer, 0, 0);
		#else
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0, 0);
		#endif

		// Read in last partial chunk
          tonccpy(buffer+dataPos,globalBuffer+curByte,length-dataPos);
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
#ifdef TWOCARD
u32 fileWrite (const char* buffer, aFile file, u32 startOffset, u32 length, bool card2, int ndmaSlot)
#else
u32 fileWrite (const char* buffer, aFile file, u32 startOffset, u32 length, int ndmaSlot)
#endif
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
		clusterIndex = startOffset/discBytePerClus[card2];
		file.currentCluster = file.fatTableCache[clusterIndex];
		file.currentOffset=clusterIndex*discBytePerClus[card2];
		#else
		clusterIndex = startOffset/discBytePerClus;
		file.currentCluster = file.fatTableCache[clusterIndex];
		file.currentOffset=clusterIndex*discBytePerClus;
		#endif
	} else {
		if(startOffset<file.currentOffset) {
			file.currentOffset=0;
			file.currentCluster = file.firstCluster;
		}

		#ifdef TWOCARD
		// Follow cluster list until desired one is found
		for (chunks = (startOffset-file.currentOffset) / discBytePerClus[card2]; chunks > 0; chunks--)
		{
			file.currentCluster = FAT_NextCluster (file.currentCluster, card2);
			file.currentOffset+=discBytePerClus[card2];
		}
		#else
		// Follow cluster list until desired one is found
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
	curSect = (startOffset % discBytePerClus[card2]) / BYTES_PER_SECTOR;
	#else
	curSect = (startOffset % discBytePerClus) / BYTES_PER_SECTOR;
	#endif
	curByte = startOffset % BYTES_PER_SECTOR;

	// Load sector buffer for new position in file
	#ifdef TWOCARD
	CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster, card2), globalBuffer, 0, 0);
	#else
	CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0, 0);
	#endif


	// Number of bytes needed to read to align with a sector
	beginBytes = (BYTES_PER_SECTOR < length + curByte ? (BYTES_PER_SECTOR - curByte) : length);

	// Read first part from buffer, to align with sector boundary
    dataPos=0;
    tonccpy(globalBuffer+curByte,buffer+dataPos,beginBytes-dataPos);
    curByte+=beginBytes;
    dataPos+=beginBytes;

	#ifndef B4DS
	#ifdef TWOCARD
	CARD_WriteSector(curSect + FAT_ClustToSect(file.currentCluster, card2), globalBuffer, ndmaSlot);
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
		if (curSect >= discSecPerClus[card2])
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = file.fatTableCache[clusterIndex]; 
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster, card2);
			    
            }
            file.currentOffset+=discBytePerClus[card2];
			curSect = 0;
		}

		// Calculate how many sectors to read (read a maximum of discSecPerClus at a time)
		sectorsToWrite = discSecPerClus[card2] - curSect;
		if(chunks < sectorsToWrite)
			sectorsToWrite = chunks;

		// Read the sectors
		#ifdef TWOCARD
		CARD_WriteSectors(curSect + FAT_ClustToSect(file.currentCluster, card2), sectorsToWrite, buffer + dataPos, ndmaSlot);
		#else
		CARD_WriteSectors(curSect + FAT_ClustToSect(file.currentCluster), sectorsToWrite, buffer + dataPos, ndmaSlot);
		#endif
		#else
		if (curSect >= discSecPerClus)
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = file.fatTableCache[clusterIndex]; 
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
		if (curSect >= discSecPerClus[card2])
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = file.fatTableCache[clusterIndex]; 
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster, card2);
            }
			curSect = 0;
			file.currentOffset+=discBytePerClus[card2];
		}
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster, card2), globalBuffer, 0, 0);

		// Read in last partial chunk
        tonccpy(globalBuffer+curByte,buffer+dataPos,length-dataPos);
        curByte+=length;
        dataPos+=length;

		#ifdef TWOCARD
		CARD_WriteSector( curSect + FAT_ClustToSect(file.currentCluster, card2), globalBuffer, ndmaSlot);
		#else
		CARD_WriteSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, ndmaSlot);
		#endif
		#else
		// Update the read buffer
		curByte = 0;
		if (curSect >= discSecPerClus)
		{
            if(file.fatTableCached) {
                clusterIndex++;
                file.currentCluster = file.fatTableCache[clusterIndex]; 
            } else {
                file.currentCluster = FAT_NextCluster (file.currentCluster);
            }
			curSect = 0;
			file.currentOffset+=discBytePerClus;
		}
		CARD_ReadSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0, 0);

		// Read in last partial chunk
        tonccpy(globalBuffer+curByte,buffer+dataPos,length-dataPos);
        curByte+=length;
        dataPos+=length;

		CARD_WriteSector( curSect + FAT_ClustToSect(file.currentCluster), globalBuffer, 0);
		#endif
	}

	return dataPos;
}

#ifndef B4DS
void buildFatTableCache (aFile * file, bool card2, int ndmaSlot)
#else
void buildFatTableCache (aFile * file)
#endif
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
#ifndef B4DS
		&& (u32)lastClusterCacheUsed<CLUSTER_CACHE+CLUSTER_CACHE_SIZE)
#else
		&& (u32)lastClusterCacheUsed<clusterCache+clusterCacheSize)
#endif
	{
		*lastClusterCacheUsed = file->currentCluster;
#ifdef TWOCARD
		file->currentOffset+=discBytePerClus[card2];
		file->currentCluster = FAT_NextCluster (file->currentCluster, card2);
#else
		file->currentOffset+=discBytePerClus;
		file->currentCluster = FAT_NextCluster (file->currentCluster);
#endif
		lastClusterCacheUsed++;
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
    }
    #endif

	file->currentOffset=0;
	file->currentCluster = file->firstCluster;
}