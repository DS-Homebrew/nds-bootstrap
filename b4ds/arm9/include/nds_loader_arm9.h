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
#include "load_crt0.h"

#ifdef __cplusplus
extern "C"
{
#endif

//#define LOAD_DEFAULT_NDS 0

void runNds(loadCrt0* loader, u32 loaderSize, u32 cluster, u32 saveCluster, u32 donorCluster, u32 apPatchCluster, u32 patchOffsetCacheCluster, configuration* conf);

#ifdef __cplusplus
}
#endif

#endif // NDS_LOADER_ARM9_H
