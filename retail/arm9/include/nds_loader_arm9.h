/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2010
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy

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

------------------------------------------------------------------*/

#ifndef NDS_LOADER_ARM9_H
#define NDS_LOADER_ARM9_H

#include "configuration.h"

#ifdef __cplusplus
extern "C"
{
#endif

//#define LOAD_DEFAULT_NDS 0

int runNds(u32 cluster, u32 saveCluster, u32 donorTwlCluster, /* u32 gbaCluster, u32 gbaSavCluster, */ u32 wideCheatCluster, u32 apPatchCluster, u32 apPatchPostCardReadCluster, u32 dsi2dsSavePatchCluster, u32 cheatCluster, u32 patchOffsetCacheCluster, u32 ramDumpCluster, u32 srParamsCluster, u32 screenshotCluster, u32 apFixOverlaysCluster, u32 musicCluster, u32 pageFileCluster, u32 manualCluster, u32 sharedFontCluster, configuration* conf);

#ifdef __cplusplus
}
#endif

#endif // NDS_LOADER_ARM9_H
