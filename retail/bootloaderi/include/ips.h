#ifndef IPS_H
#define IPS_H

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, bool arm9Only, bool isSdk5, bool ROMinRAM);
bool ipsHasOverlayPatch(const tNDSHeader* ndsHeader, u8* ipsbyte);

#endif // IPS_H
