#ifndef PATCH_H
#define PATCH_H

#include <nds/ndstypes.h>

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

#endif // PATCH_H
