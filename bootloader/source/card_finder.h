#ifndef CARD_FINDER_H
#define CARD_FINDER_H

#include <nds/ndstypes.h>
#include "card_patcher.h"

extern bool cardReadFound;
extern bool usesThumb;
extern bool sdk5;

module_params_t* findModuleParams(const tNDSHeader* ndsHeader, u32 donorSdkVer);
u32 getOffset(u32* addr, size_t size, u32* find, size_t lenofFind, int direction);
u32 getOffsetThumb(u16* addr, size_t size, u16* find, size_t lenofFind, int direction);
u32 getCardReadEndOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32 getCardReadStartOffset(const tNDSHeader* ndsHeader, u32 cardReadEndOffset);
u32 getCardReadCachedEndOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32 getCardReadCachedStartOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams, u32 cardReadCachedEndOffset);
u32 getCardPullOutOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
//u32 getForceToPowerOffOffset(const tNDSHeader* ndsHeader);
u32 getCardIdEndOffset(const tNDSHeader* ndsHeader, u32 cardReadEndOffset);
u32 getCardIdStartOffset(const tNDSHeader* ndsHeader, u32 cardIdEndOffset);
u32 getCardReadDmaEndOffset(const tNDSHeader* ndsHeader);
u32 getCardReadDmaStartOffset(const tNDSHeader* ndsHeader, u32 cardReadDmaEndOffset);
u32* getMpuInitRegionSignature(u32 patchMpuRegion);
u32 getMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion);
u32 getMpuDataOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams, u32 mpuStartOffset, u32 patchMpuRegion);
u32 getMpuInitCacheOffset(const tNDSHeader* ndsHeader, u32 mpuStartOffset);
//u32 getArenaLowOffset(const tNDSHeader* ndsHeader);
u32 getRandomPatchOffset(const tNDSHeader* ndsHeader);
u32 getSwiHaltOffset(const tNDSHeader* ndsHeader);
u32 getSwi12Offset(const tNDSHeader* ndsHeader);
u32 getSwiGetPitchTableOffset(const tNDSHeader* ndsHeader);
u32 getSleepPatchOffset(const tNDSHeader* ndsHeader);
u32 getCardCheckPullOutOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
u32 getCardIrqEnableOffset(const tNDSHeader* ndsHeader, module_params_t* moduleParams);

#endif // CARD_FINDER_H