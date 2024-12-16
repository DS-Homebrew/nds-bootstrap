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
u32* memsearch32_2(const u32* start, u32 dataSize, const u32* find, const u32* find2, u32 findSize, bool forward);
u32* memsearch32_3(const u32* start, u32 dataSize, const u32* find, const u32* find2, const u32* find3, u32 findSize, bool forward);
u16* memsearch16(const u16* start, u32 dataSize, const u16* find, u32 findSize, bool forward);
u16* memsearch16_4(const u16* start, u32 dataSize, const u16* find, const u16* find2, const u16* find3, const u16* find4, u32 findSize, bool forward);

static inline u32* findOffset(const u32* start, u32 dataSize, const u32* find, u32 findLen) {
	return memsearch32(start, dataSize, find, findLen*sizeof(u32), true);
}
static inline u32* findOffsetBackwards(const u32* start, u32 dataSize, const u32* find, u32 findLen) {
	return memsearch32(start, dataSize, find, findLen*sizeof(u32), false);
}
static inline u32* findOffsetBackwards2(const u32* start, u32 dataSize, const u32* find, const u32* find2, u32 findLen) {
	return memsearch32_2(start, dataSize, find, find2, findLen*sizeof(u32), false);
}
static inline u32* findOffsetBackwards3(const u32* start, u32 dataSize, const u32* find, const u32* find2, const u32* find3, u32 findLen) {
	return memsearch32_3(start, dataSize, find, find2, find3, findLen*sizeof(u32), false);
}
static inline u16* findOffsetThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen) {
	return memsearch16(start, dataSize, find, findLen*sizeof(u16), true);
}
static inline u16* findOffsetBackwardsThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen) {
	return memsearch16(start, dataSize, find, findLen*sizeof(u16), false);
}
static inline u16* findOffsetBackwardsThumb4(const u16* start, u32 dataSize, const u16* find, const u16* find2, const u16* find3, const u16* find4, u32 findLen) {
	return memsearch16_4(start, dataSize, find, find2, find3, find4, findLen*sizeof(u16), false);
}

// ARM9
u32* a9_findSwi12Offset(const tNDSHeader* ndsHeader);
u32* findModuleParamsOffset(const tNDSHeader* ndsHeader);
u32* findLtdModuleParamsOffset(const tNDSHeader* ndsHeader);
u32* findDsiModeCheckOffset(const tNDSHeader* ndsHeader);
u32* findDsiModeCheck2Offset(const u32* dsiModeCheckOffset, bool usesThumb);
u32* findCardHashInitOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u16* findCardHashInitOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadEndOffsetType0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset);
u32* findCardReadEndOffsetType1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset);
u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader, u32 startOffset);
u16* findCardReadEndOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset); // SDK 5
u16* findCardReadEndOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset); // SDK 5
u32* findCardReadStartOffsetType0(const module_params_t* moduleParams, const u32* cardReadEndOffset);
u32* findCardReadStartOffsetType1(const u32* cardReadEndOffset);
u32* findCardReadStartOffset5(const module_params_t* moduleParams, const u32* cardReadEndOffset); // SDK 5
u32* findCardReadCheckOffsetMvDK4(u32 startOffset);
u16* findCardReadStartOffsetThumb(const u16* cardReadEndOffset);
u16* findCardReadStartOffsetThumb5Type0(const module_params_t* moduleParams, const u16* cardReadEndOffset); // SDK 5
u16* findCardReadStartOffsetThumb5Type1(const module_params_t* moduleParams, const u16* cardReadEndOffset); // SDK 5
u32* findCardReadCachedEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardReadCachedStartOffset(const module_params_t* moduleParams, const u32* cardReadCachedEndOffset);
// u32* findCardRomInitOffset(const u32* cardReadEndOffset);
// u16* findCardRomInitOffsetThumb(const u16* cardReadEndOffset);
//u32* findCardReadHashOffset(void);
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
//u32* findCardRefreshOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb);
u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u16* findCardReadDmaEndOffsetThumb(const tNDSHeader* ndsHeader);
u32* findCardReadDmaStartOffset(const module_params_t* moduleParams, const u32* cardReadDmaEndOffset);
u16* findCardReadDmaStartOffsetThumb(const u16* cardReadDmaEndOffset);
u32* findGbaSlotInitOffset(const tNDSHeader* ndsHeader);
u16* findGbaSlotInitOffsetThumb(const tNDSHeader* ndsHeader);
u32* a9FindCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumb);
const u32* getMpuInitRegionSignature(u32 patchMpuRegion);
u32* findMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion);
u32* findMpuDataOffset(const module_params_t* moduleParams, u32 patchMpuRegion, const u32* mpuStartOffset);
u32* findMpuDataOffsetAlt(const tNDSHeader* ndsHeader);
u32* findMpuChange(const tNDSHeader* ndsHeader);
u32* findMpuInitTwlEnd(const u32* heapPointer2Offset);
u32* findHeapPointerOffset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader);
u32* findHeapPointer2Offset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader);
u32* findRandomPatchOffset(const tNDSHeader* ndsHeader);
u32* findRandomPatchOffset5Second(const tNDSHeader* ndsHeader); // SDK 5
//u32* findOperaRamOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findFileIoOpenOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findFileIoCloseOffset(const u32* fileIoOpenOffset);
u16* findFileIoCloseOffsetThumb(const u16* fileIoOpenOffset);
u32* findFileIoSeekOffset(const u32* fileIoCloseOffset, const module_params_t* moduleParams);
u16* findFileIoSeekOffsetThumb(const u16* fileIoCloseOffset);
u32* findFileIoReadOffset(const u32* fileIoSeekOffset, const module_params_t* moduleParams);
u16* findFileIoReadOffsetThumb(const u16* fileIoSeekOffset, const module_params_t* moduleParams);
u32* findCartInfoInitConstantOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb);
u32* findCartExistOffset(const tNDSHeader* ndsHeader, bool usesThumb);
u32* findCartReadOffset(const tNDSHeader* ndsHeader, bool usesThumb);
u32* findWaitCpuCyclesOffset(const tNDSHeader* ndsHeader);
u32* findWaitSysCyclesOffset(const tNDSHeader* ndsHeader);
u32* findSleepOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, u32* usesThumbPtr);
u32* findCardEndReadDma(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, const u32* cardReadDmaEndOffset, u32* offsetDmaHandler);
u32* findCardSetDma(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb);
u32* findSrlStartOffset9(const tNDSHeader* ndsHeader);
u32* findResetOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const bool softResetMb);
u32* findNandTmpJumpFuncOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findMbkWramBOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u16* findMbkWramBOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findMbkWramBOffsetBoth(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumb);
u32* findSharedFontPathOffset(const tNDSHeader* ndsHeader);

// ARM7
bool a7GetReloc(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findSwiHaltOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u16* findSwiHaltThumbOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* a7_findSwi12Offset(const tNDSHeader* ndsHeader);
u16* a7_findSwi24Offset(void);
u16* a7_findSwi25Offset(void);
u16* a7_findSwi26Offset(void);
u16* a7_findSwi27Offset(void);
u32* a7_findScfgExtOffset(const tNDSHeader* ndsHeader);
u16* findSwiGetPitchTableThumbBranchOffset(const tNDSHeader* ndsHeader);
u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
//u32* findUserDataAddrOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findSleepPatchOffset(const tNDSHeader* ndsHeader);
u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader);
u32* findSleepInputWriteOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findRamClearOffset(const tNDSHeader* ndsHeader);
u32* findRamClearIOffset(const tNDSHeader* ndsHeader);
u32* findRamClearI2Offset(const u32* ramClearIOffset);
u32* findPostBootOffset(const tNDSHeader* ndsHeader);
u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findSrlStartOffset7(const tNDSHeader* ndsHeader);
//u32* findA7iStartOffset(void);
u32* findSdCardResetOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams);
u32* findSdCardFuncsOffset(const tNDSHeader* ndsHeader);
u32* findAutoPowerOffOffset(const tNDSHeader* ndsHeader);

#endif // FIND_H
