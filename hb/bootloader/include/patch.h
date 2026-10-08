#ifndef PATCH_H
#define PATCH_H

#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader

#define patchOffsetCacheFileVersion 7	// Change when new functions are being patched, some offsets removed
										// the offset order changed, and/or the function signatures changed
typedef struct patchOffsetCacheContents {
    u16 ver;
    u16 type;
	u32 dldiOffset;
	u32 dldiChecked;
	u32* heapEndOffset;
	u32 heapEndChecked;
	u32* bootloaderOffset;
	u32 bootloaderChecked;
	u16* a9Swi00Offset;
	u32 a9Swi00Checked;
	u32* a9Swi0FARMOffset;
	u32 a9Swi0FARMChecked;
	u16* a9Swi0FOffset;
	u32 a9Swi0FChecked;
	u16* a9Swi12Offset;
	u32 a9Swi12Checked;
	u32* a7IrqHookOffset;
	u32* a7IrqHookAccelOffset;
	u16* a7Swi00Offset;
	u32 a7Swi00Checked;
} patchOffsetCacheContents;

extern u16 patchOffsetCacheFilePrevCrc;
extern u16 patchOffsetCacheFileNewCrc;

extern patchOffsetCacheContents patchOffsetCache;

extern void patchBinary(const tNDSHeader* ndsHeader);

#endif // PATCH_H
