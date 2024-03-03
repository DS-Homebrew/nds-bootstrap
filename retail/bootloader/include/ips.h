#ifndef IPS_H
#define IPS_H

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, bool arm9Only, bool ROMinRAM, const bool usesCloneboot);

#endif // IPS_H
