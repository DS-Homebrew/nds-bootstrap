#ifndef IPS_H
#define IPS_H

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, bool arm9Only, bool ROMinRAM, bool overlaysInRam, const bool usesCloneboot);

#endif // IPS_H
