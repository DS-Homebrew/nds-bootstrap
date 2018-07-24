#ifndef CARD_FINDER_H
#define CARD_FINDER_H

#include <nds/ndstypes.h>
#include "card_patcher.h"

//extern bool sdk5;
//extern int readType;

module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer);
u32* findOffset(u32* addr, size_t size, u32* find, size_t lenofFind, int direction);
u16* findOffsetThumb(u16* addr, size_t size, u16* find, size_t lenofFind, int direction);
u32* findCardReadEndOffset0(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32* findCardReadEndOffset1(const tNDSHeader* ndsHeader);
u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32* findCardReadStartOffset0(u32* cardReadEndOffset);
u32* findCardReadStartOffset1(u32* cardReadEndOffset);
u16* findCardReadStartOffsetThumb(u16* cardReadEndOffset);
u32* findCardReadCachedEndOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32* findCardReadCachedStartOffset(module_params_t* moduleParams, u32* cardReadCachedEndOffset);
u32* findCardPullOutOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
//u32* findForceToPowerOffOffset(const tNDSHeader* ndsHeader);
u32* findCardIdEndOffset(const tNDSHeader* ndsHeader, u32* cardReadEndOffset);
u16* findCardIdEndOffsetThumb(const tNDSHeader* ndsHeader, u16* cardReadEndOffset);
u32* findCardIdStartOffset(u32* cardIdEndOffset);
u16* findCardIdStartOffsetThumb(u16* cardIdEndOffset);
u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader);
u16* findCardReadDmaEndOffsetThumb(const tNDSHeader* ndsHeader);
u32* findCardReadDmaStartOffset(u32* cardReadDmaEndOffset);
u16* findCardReadDmaStartOffsetThumb(u16* cardReadDmaEndOffset);
u32* getMpuInitRegionSignature(u32 patchMpuRegion);
u32* findMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion);
u32* findMpuDataOffset(module_params_t* moduleParams, u32 patchMpuRegion, u32* mpuStartOffset);
u32* findMpuInitCacheOffset(u32* mpuStartOffset);
//u32* findArenaLowOffset(const tNDSHeader* ndsHeader);
u32* findRandomPatchOffset(const tNDSHeader* ndsHeader);
u32* findSwiHaltOffset(const tNDSHeader* ndsHeader);
u32* findSwi12Offset(const tNDSHeader* ndsHeader);
u32* findSwiGetPitchTableOffset(const tNDSHeader* ndsHeader);
u32* findSleepPatchOffset(const tNDSHeader* ndsHeader);
u16* findSleepPatchOffsetThumb(const tNDSHeader* ndsHeader);
u32* findCardCheckPullOutOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32* findCardIrqEnableOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);

#endif // CARD_FINDER_H