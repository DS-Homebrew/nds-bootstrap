#ifndef FIND_H
#define FIND_H

#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "locations.h"
#include "module_params.h"

//extern int readType;

// COMMON
//u8* memsearch(const u8* start, u32 dataSize, const u8* find, u32 findSize);
u32* memsearch32(const u32* start, u32 dataSize, const u32* find, u32 findSize, bool forward);
u16* memsearch16(const u16* start, u32 dataSize, const u16* find, u32 findSize, bool forward);

inline u32* findOffset(const u32* start, u32 dataSize, const u32* find, u32 findLen) {
	return memsearch32(start, dataSize, find, findLen*sizeof(u32), true);
}
inline u32* findOffsetBackwards(const u32* start, u32 dataSize, const u32* find, u32 findLen) {
	return memsearch32(start, dataSize, find, findLen*sizeof(u32), false);
}
inline u16* findOffsetThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen) {
	return memsearch16(start, dataSize, find, findLen*sizeof(u16), true);
}
inline u16* findOffsetBackwardsThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen) {
	return memsearch16(start, dataSize, find, findLen*sizeof(u16), false);
}

// ARM9
u32* findModuleParamsOffset(const tNDSHeader* ndsHeader);
u32* findCardReadEndOffsetType0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadEndOffsetType1(const tNDSHeader* ndsHeader);
u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader);
u16* findCardReadEndOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams); // SDK 5
u16* findCardReadEndOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams); // SDK 5
u32* findCardReadStartOffsetType0(const u32* cardReadEndOffset);
u32* findCardReadStartOffsetType1(const u32* cardReadEndOffset);
u32* findCardReadStartOffset5(const module_params_t* moduleParams, const u32* cardReadEndOffset); // SDK 5
u16* findCardReadStartOffsetThumb(const u16* cardReadEndOffset);
u16* findCardReadStartOffsetThumb5Type0(const module_params_t* moduleParams, const u16* cardReadEndOffset); // SDK 5
u16* findCardReadStartOffsetThumb5Type1(const module_params_t* moduleParams, const u16* cardReadEndOffset); // SDK 5
u32* findCardReadCachedEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadCachedStartOffset(const module_params_t* moduleParams, const u32* cardReadCachedEndOffset);
u32* findCardPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader);
u16* findCardPullOutOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams); // SDK 5
u16* findCardPullOutOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams); // SDK 5
u32* findCardTerminateForPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
//u32* findForceToPowerOffOffset(const tNDSHeader* ndsHeader);
u32* findCardIdEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32* cardReadEndOffset);
u16* findCardIdEndOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u16* cardReadEndOffset);
u32* findCardIdStartOffset(const module_params_t* moduleParams, const u32* cardIdEndOffset);
u16* findCardIdStartOffsetThumb(const module_params_t* moduleParams, const u16* cardIdEndOffset);
u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader);
u16* findCardReadDmaEndOffsetThumb(const tNDSHeader* ndsHeader);
u32* findCardReadDmaStartOffset(const module_params_t* moduleParams, const u32* cardReadDmaEndOffset);
u16* findCardReadDmaStartOffsetThumb(const u16* cardReadDmaEndOffset);
const u32* getMpuInitRegionSignature(u32 patchMpuRegion);
u32* findMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion);
u32* findMpuDataOffset(const module_params_t* moduleParams, u32 patchMpuRegion, const u32* mpuStartOffset);
u32* findMpuInitCacheOffset(const u32* mpuStartOffset);
u32* findHeapPointerOffset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader);
u32* findRandomPatchOffset(const tNDSHeader* ndsHeader);
u32* findRandomPatchOffset5First(const tNDSHeader* ndsHeader, const module_params_t* moduleParams); // SDK 5
u32* findRandomPatchOffset5Second(const tNDSHeader* ndsHeader, const module_params_t* moduleParams); // SDK 5
u32* findOperaRamOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);

// ARM7
u32* findSwi12Offset(const tNDSHeader* ndsHeader);
u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findSleepPatchOffset(const tNDSHeader* ndsHeader);
u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader);
u32* findRamClearOffset(const tNDSHeader* ndsHeader);
u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);

#endif // FIND_H
