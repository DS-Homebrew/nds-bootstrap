/*
	Noobish Noobsicle wrote this IPS patching code
	Adapted to C by RocketRobz
*/

#include <nds/ndstypes.h>
#include <nds/memory.h>
#include "nds_header.h"
#include "locations.h"
#include "tonccpy.h"

#define cacheBlockSize 0x4000

extern u8 consoleModel;
extern bool dsiModeConfirmed;
extern bool extendedMemoryConfirmed;
extern bool overlaysInRam;

extern u32 getRomLocation(const tNDSHeader* ndsHeader, const bool isSdk5);

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, const bool arm9Only, const bool isSdk5, const bool ROMinRAM, const bool usesCloneboot) {
	if (ipsbyte[0] != 'P' && ipsbyte[1] != 'A' && ipsbyte[2] != 'T' && ipsbyte[3] != 'C' && ipsbyte[4] != 'H' && ipsbyte[5] != 0) {
		return false;
	}

	bool armPatched = false;
	const u32 romLocation = getRomLocation(ndsHeader, isSdk5);

	int ipson = 5;
	int totalrepeats = 0;
	u32 offset = 0;
	void* rombyte = 0;
	while (1) {
		offset = ipsbyte[ipson] * 0x10000 + ipsbyte[ipson + 1] * 0x100 + ipsbyte[ipson + 2];
		if (offset >= ndsHeader->arm9romOffset && ((offset < ndsHeader->arm9romOffset+ndsHeader->arm9binarySize) || arm9Only)) {
			// ARM9 binary
			rombyte = ndsHeader->arm9destination - ndsHeader->arm9romOffset;
			armPatched = true;
		} else if (offset >= ndsHeader->arm7romOffset && offset < ndsHeader->arm7romOffset+ndsHeader->arm7binarySize) {
			// ARM7 binary
			rombyte = ndsHeader->arm7destination - ndsHeader->arm7romOffset;
			armPatched = true;
		} else if (offset >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && offset < ndsHeader->arm7romOffset) {
			// Overlays
			if (!overlaysInRam) {
				return armPatched;
			}
			if (ROMinRAM) {
				rombyte = (void*)romLocation;
				if (usesCloneboot) {
					rombyte -= 0x4000;
				} else if (ndsHeader->arm9overlaySource == 0 || ndsHeader->arm9overlaySize == 0) {
					rombyte -= ndsHeader->arm7romOffset;
					rombyte -= ndsHeader->arm7binarySize;
				} else {
					rombyte -= ndsHeader->arm9romOffset;
					rombyte -= ndsHeader->arm9binarySize;
				}
				if (ndsHeader->unitCode == 0 || !dsiModeConfirmed) {
					if (isSdk5) {
						if (romLocation < (ndsHeader->unitCode > 0 ? 0x0C7E0000 : 0x0C800000) && (u32)rombyte >= 0x0C7C4000) {
							rombyte += (ndsHeader->unitCode > 0 ? 0x1C000 : 0x3C000);
						} else if (ndsHeader->unitCode == 0) {
							if (romLocation < 0x0D000000 && (u32)rombyte >= 0x0CFFC000) {
								rombyte += 0x4000;
							}
						} else {
							if (romLocation < 0x0C800000 && (u32)rombyte >= 0x0C7FC000) {
								rombyte += 0x4000;
							} else if (romLocation < 0x0D000000 && (u32)rombyte >= 0x0CFE0000) {
								rombyte += 0x20000;
							}
						}
					} else if (romLocation < 0x0C800000 && (u32)rombyte >= 0x0C7C0000) {
						rombyte += 0x40000;
					}
				}
			} else {
				rombyte = (void*)CACHE_ADRESS_START_DSIMODE;
				rombyte -= ((ndsHeader->arm9romOffset+ndsHeader->arm9binarySize)/cacheBlockSize)*cacheBlockSize;
			}
		}
		ipson += 3;
		if (ipsbyte[ipson] * 256 + ipsbyte[ipson + 1] == 0) {
			ipson += 2;
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson += 2;
			u8 repeatbyte[totalrepeats];
			for (int ontime = 0; ontime < totalrepeats; ontime++) {
				repeatbyte[ontime] = ipsbyte[ipson];
			}
			// tonccpy(rombyte+offset, repeatbyte, totalrepeats);
			u8* rombyteOffset = (u8*)rombyte+offset;
			for (int i = 0; i < totalrepeats; i++) {
				*rombyteOffset = repeatbyte[i];
				rombyteOffset++;
				if (ROMinRAM && (ndsHeader->unitCode == 0 || !dsiModeConfirmed)) {
					if (isSdk5) {
						if ((u32)rombyteOffset == 0x0C7C4000) {
							rombyteOffset += (ndsHeader->unitCode > 0 ? 0x1C000 : 0x3C000);
						} else if (ndsHeader->unitCode == 0) {
							if ((u32)rombyteOffset == 0x0CFFC000) {
								rombyteOffset += 0x4000;
							}
						} else {
							if ((u32)rombyteOffset == 0x0C7FC000) {
								rombyteOffset += 0x4000;
							} else if ((u32)rombyteOffset == 0x0CFE0000) {
								rombyteOffset += 0x20000;
							}
						}
					} else if ((u32)rombyteOffset == 0x0C7C0000) {
						rombyteOffset += 0x40000;
					}
				}
			}
			ipson++;
		} else {
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson += 2;
			// tonccpy(rombyte+offset, ipsbyte+ipson, totalrepeats);
			u8* rombyteOffset = (u8*)rombyte+offset;
			for (int i = 0; i < totalrepeats; i++) {
				*rombyteOffset = ipsbyte[ipson+i];
				rombyteOffset++;
				if (ROMinRAM && (ndsHeader->unitCode == 0 || !dsiModeConfirmed)) {
					if (isSdk5) {
						if ((u32)rombyteOffset == 0x0C7C4000) {
							rombyteOffset += (ndsHeader->unitCode > 0 ? 0x1C000 : 0x3C000);
						} else if (ndsHeader->unitCode == 0) {
							if ((u32)rombyteOffset == 0x0CFFC000) {
								rombyteOffset += 0x4000;
							}
						} else {
							if ((u32)rombyteOffset == 0x0C7FC000) {
								rombyteOffset += 0x4000;
							} else if ((u32)rombyteOffset == 0x0CFE0000) {
								rombyteOffset += 0x20000;
							}
						}
					} else if ((u32)rombyteOffset == 0x0C7C0000) {
						rombyteOffset += 0x40000;
					}
				}
			}
			ipson += totalrepeats;
		}
		if (ipsbyte[ipson] == 69 && ipsbyte[ipson + 1] == 79 && ipsbyte[ipson + 2] == 70) {
			break;
		}
	}
	return true;
}

bool ipsHasOverlayPatch(const tNDSHeader* ndsHeader, u8* ipsbyte) {
	if (ipsbyte[0] != 'P' && ipsbyte[1] != 'A' && ipsbyte[2] != 'T' && ipsbyte[3] != 'C' && ipsbyte[4] != 'H' && ipsbyte[5] != 0) {
		return false;
	}

	int ipson = 5;
	int totalrepeats = 0;
	u32 offset = 0;
	while (1) {
		offset = ipsbyte[ipson] * 0x10000 + ipsbyte[ipson + 1] * 0x100 + ipsbyte[ipson + 2];
		if (offset >= ndsHeader->arm9romOffset+ndsHeader->arm9binarySize && offset < ndsHeader->arm7romOffset) {
			return true;
		}
		ipson += 3;
		if (ipsbyte[ipson] * 256 + ipsbyte[ipson + 1] == 0) {
			ipson += 2;
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson += 2;
			ipson++;
		} else {
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson += 2;
			ipson += totalrepeats;
		}
		if (ipsbyte[ipson] == 69 && ipsbyte[ipson + 1] == 79 && ipsbyte[ipson + 2] == 70) {
			break;
		}
	}
	return false;
}