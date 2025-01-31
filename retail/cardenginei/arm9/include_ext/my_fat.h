/*-----------------------------------------------------------------
 fat.h
 
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

#ifndef FAT_H
#define FAT_H

#include <nds/ndstypes.h>

#define CLUSTER_FREE	0x00000000
#define	CLUSTER_EOF		0x0FFFFFFF
#define CLUSTER_FIRST	0x00000002

#define fatCached BIT(0)
#define fatCompressed BIT(1)

typedef	struct
{
	u32	firstCluster;
	u32	currentCluster;
	u32 currentOffset;
	u32 fatTableSettings; // 0: Cached, 1: Compressed
	u32* fatTableCache;
	u32 fatTableCacheSize;
} aFile;

typedef	struct
{
	aFile * file;
    char* buffer;
	u32 startOffset;
	u32 length;
    int dataPos;
    int curSect;
    int curByte;
    int clusterIndex;
    int chunks;
    int cmd;
} readContext;

bool FAT_InitFiles(bool initCard);
void getBootFileCluster(aFile* file, const char* bootName);
void getFileFromCluster(aFile* file, u32 cluster);
u32 fileRead(char* buffer, aFile* file, u32 startOffset, u32 length);
bool fileReadNonBLocking(char* buffer, aFile* file, u32 startOffset, u32 length);
bool resumeFileRead();
u32 fileWrite(const char* buffer, aFile* file, u32 startOffset, u32 length);
u32 FAT_ClustToSect(u32 cluster);
void buildFatTableCache(aFile* file);

/* ROM Header Region Information Structure */

#endif // FAT_H
