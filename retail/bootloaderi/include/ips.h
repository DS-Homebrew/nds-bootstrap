#ifndef IPS_H
#define IPS_H

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, const bool arm9Only, const bool isESdk2, const bool isSdk5, const bool ROMinRAM, const u32 cacheBlockSize);
bool ipsHasOverlayPatch(const tNDSHeader* ndsHeader, u8* ipsbyte);

#endif // IPS_H
