/*
	Noobish Noobsicle wrote this IPS patching code
	Adapted to C by RocketRobz
*/

#include <nds/ndstypes.h>
#include <nds/memory.h>
#include "nds_header.h"
#include "locations.h"
#include "my_fat.h"
#include "tonccpy.h"

extern u32 apFixOverlaysCluster;

extern bool extendedMemory;
extern bool dsDebugRam;
extern u32 romLocation;

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, bool arm9Only, bool ROMinRAM, const bool usesCloneboot) {
	if (ipsbyte[0] != 'P' && ipsbyte[1] != 'A' && ipsbyte[2] != 'T' && ipsbyte[3] != 'C' && ipsbyte[4] != 'H' && ipsbyte[5] != 0) {
		return false;
	}

	aFile apFixOverlaysFile;
	if (!ROMinRAM) {
		getFileFromCluster(&apFixOverlaysFile, apFixOverlaysCluster);
	}

	const u32 arm9romOffset = arm9Only ? 0x4000 : ndsHeader->arm9romOffset;

	int ipson = 5;
	int totalrepeats = 0;
	u32 offset = 0;
	void* rombyte = 0;
	while (1) {
		bool overlays = false;
		offset = ipsbyte[ipson] * 0x10000 + ipsbyte[ipson + 1] * 0x100 + ipsbyte[ipson + 2];
		if ((offset >= ndsHeader->arm9romOffset && offset < ndsHeader->arm9romOffset+ndsHeader->arm9binarySize) || arm9Only) {
			// ARM9 binary
			rombyte = ndsHeader->arm9destination - arm9romOffset;
		} else if (offset >= ndsHeader->arm7romOffset && offset < ndsHeader->arm7romOffset+ndsHeader->arm7binarySize) {
			// ARM7 binary
			rombyte = ndsHeader->arm7destination - ndsHeader->arm7romOffset;
		} else if (offset >= ndsHeader->arm9overlaySource && offset < ndsHeader->arm7romOffset) {
			// Overlays
			overlays = true;
			rombyte = (void*)romLocation;
			if (ROMinRAM) {
				if (usesCloneboot) {
					rombyte -= 0x8000;
				} else if (ndsHeader->arm9overlaySource == 0 || ndsHeader->arm9overlaySize == 0) {
					rombyte -= (ndsHeader->arm7romOffset + ndsHeader->arm7binarySize);
				} else if (ndsHeader->arm9overlaySource > ndsHeader->arm7romOffset) {
					rombyte -= (ndsHeader->arm9romOffset + ndsHeader->arm9binarySize);
				} else {
					rombyte -= ndsHeader->arm9overlaySource;
				}
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
			if (!overlays || ROMinRAM) {
				tonccpy(rombyte+offset, repeatbyte, totalrepeats);
			} else {
				fileWrite((char*)&repeatbyte, &apFixOverlaysFile, offset, totalrepeats);
			}
			ipson++;
		} else {
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson += 2;
			if (!overlays || ROMinRAM) {
				tonccpy(rombyte+offset, ipsbyte+ipson, totalrepeats);
			} else {
				fileWrite((char*)ipsbyte+ipson, &apFixOverlaysFile, offset, totalrepeats);
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