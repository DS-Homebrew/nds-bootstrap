/*-----------------------------------------------------------------

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

#ifndef CARD_H
#define CARD_H

#include "my_disc_io.h"
#include "my_sdmmc.h"

// Export interface
extern NEW_DISC_INTERFACE __myio_dsisd;

static inline bool CARD_StartUp(void) {
	return __myio_dsisd.startup();
}

static inline bool CARD_IsInserted(void) {
	return __myio_dsisd.isInserted();
}

static inline bool CARD_ReadSector(u32 sector, void *buffer, u32 startOffset, u32 endOffset) {
	return __myio_dsisd.readSector(sector, buffer, startOffset, endOffset);
}

static inline bool CARD_ReadSectors(u32 sector, int count, void *buffer) {
	return __myio_dsisd.readSectors(sector, count, buffer);
}

static inline int CARD_ReadSectorsNonBlocking(u32 sector, int count, void *buffer) {
	return __myio_dsisd.readSectorsNonBlocking(sector, count, buffer);
}

static inline int CARD_CheckCommand(int cmd) {
	return __myio_dsisd.checkCommand(cmd);
}

static inline bool CARD_WriteSector(u32 sector, const void *buffer) {
	return __myio_dsisd.writeSectors(sector, 1, buffer);
}

static inline bool CARD_WriteSectors(u32 sector, int count, const void *buffer) {
	return __myio_dsisd.writeSectors(sector, count, buffer);
}

#endif // CARD_H
