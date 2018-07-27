#ifndef CARD_FINDER_H
#define CARD_FINDER_H

#include <nds/ndstypes.h>
#include "card_patcher.h"

//extern bool sdk5;
//extern int readType;

// COMMON
//u8* memsearch(const u8* start, u32 dataSize, const u8* find, u32 findSize);
u32* memsearch32(const u32* start, u32 dataSize, const u32* find, u32 findSize, bool forward);
u16* memsearch16(const u16* start, u32 dataSize, const u16* find, u32 findSize, bool forward);

inline u32* findOffset(const u32* start, u32 dataSize, const u32* find, u32 findLen) {
	u32* debug = (u32*)0x037D0000;
	debug[3] = (u32)(start + dataSize);
	//return (u32*)memsearch((u8*)start, dataSize, (u8*)find, findLen*sizeof(u32));
	return memsearch32(start, dataSize, find, findLen*sizeof(u32), true);
}
inline u32* findOffsetBackwards(const u32* start, u32 dataSize, const u32* find, u32 findLen) {
	//return findOffset((u32*)((u32)start - dataSize), dataSize, find, findLen);
	//return memsearch32(start - dataSize/sizeof(u32), dataSize, find, findLen*sizeof(u32));
	//return findOffset(start - dataSize/sizeof(u32), dataSize, find, findLen);
	return memsearch32(start, dataSize, find, findLen*sizeof(u32), false);
}
inline u16* findOffsetThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen) {
	//return (u16*)memsearch((u8*)start, dataSize, (u8*)find, findLen*sizeof(u16));
	return memsearch16(start, dataSize, find, findLen*sizeof(u16), true);
}
inline u16* findOffsetBackwardsThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen) {
	//return findOffsetThumb((u16*)((u32)start - dataSize), dataSize, find, findLen);
	//return memsearch16(start - dataSize/sizeof(u16), dataSize, find, findLen*sizeof(u16));
	//return findOffsetThumb(start - dataSize/sizeof(u16), dataSize, find, findLen);
	return memsearch16(start, dataSize, find, findLen*sizeof(u16), false);
}

// ARM9
module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer);
u32* findCardReadEndOffset0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadEndOffset1(const tNDSHeader* ndsHeader);
u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadStartOffset0(const u32* cardReadEndOffset);
u32* findCardReadStartOffset1(const u32* cardReadEndOffset);
u16* findCardReadStartOffsetThumb(const u16* cardReadEndOffset);
u32* findCardReadCachedEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadCachedStartOffset(const module_params_t* moduleParams, const u32* cardReadCachedEndOffset);
u32* findCardPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
//u32* findForceToPowerOffOffset(const tNDSHeader* ndsHeader);
u32* findCardIdEndOffset(const tNDSHeader* ndsHeader, const u32* cardReadEndOffset);
u16* findCardIdEndOffsetThumb(const tNDSHeader* ndsHeader, const u16* cardReadEndOffset);
u32* findCardIdStartOffset(const u32* cardIdEndOffset);
u16* findCardIdStartOffsetThumb(const u16* cardIdEndOffset);
u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader);
u16* findCardReadDmaEndOffsetThumb(const tNDSHeader* ndsHeader);
u32* findCardReadDmaStartOffset(const u32* cardReadDmaEndOffset);
u16* findCardReadDmaStartOffsetThumb(const u16* cardReadDmaEndOffset);
u32* getMpuInitRegionSignature(u32 patchMpuRegion);
u32* findMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion);
u32* findMpuDataOffset(const module_params_t* moduleParams, u32 patchMpuRegion, const u32* mpuStartOffset);
u32* findMpuInitCacheOffset(const u32* mpuStartOffset);
//u32* findArenaLowOffset(const tNDSHeader* ndsHeader);
u32* findRandomPatchOffset(const tNDSHeader* ndsHeader);

// ARM7
u32* findSwi12Offset(const tNDSHeader* ndsHeader);
u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader);
u32* findSwiHaltOffset(const tNDSHeader* ndsHeader);
u32* findSleepPatchOffset(const tNDSHeader* ndsHeader);
u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader);
u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);

#endif // CARD_FINDER_H