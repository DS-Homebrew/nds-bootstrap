#ifndef PATCH_H
#define PATCH_H

#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader

typedef struct patchOffsetCacheContents {
    u16 ver;
    u16 type;
	u32 dldiOffset;
	u32 dldiChecked;
	u32* wordCommandOffset;
	u32* bootloaderOffset;
	u32 bootloaderChecked;
	u32* a7IrqHookOffset;
	u32* a7IrqHookAccelOffset;
} patchOffsetCacheContents;

extern u16 patchOffsetCacheFileVersion;
extern patchOffsetCacheContents patchOffsetCache;
extern bool patchOffsetCacheChanged;
extern void rsetPatchCache(const tNDSHeader* ndsHeader);

extern void patchBinary(const tNDSHeader* ndsHeader);

#endif // PATCH_H
