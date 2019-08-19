/*
	Noobish Noobsicle wrote this IPS patching code
	Adapted to C by RocketRobz
*/

#include <nds/ndstypes.h>
#include <nds/memory.h>
#include "tonccpy.h"

void applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte) {
	int ipson = 5;
	int totalrepeats = 0;
	u32 offset = 0;
	void* rombyte = 0;
	while (1) {
		offset = ipsbyte[ipson] * 0x10000 + ipsbyte[ipson + 1] * 0x100 + ipsbyte[ipson + 2];
		if (offset >= ndsHeader->arm9romOffset && offset < ndsHeader->arm9romOffset+ndsHeader->arm9binarySize) {
			rombyte = ndsHeader->arm9destination - 0x4000;
		}
		ipson++;
		ipson++;
		ipson++;
		if (ipsbyte[ipson] * 256 + ipsbyte[ipson + 1] == 0) {
			ipson++;
			ipson++;
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson++;
			ipson++;
			u8 repeatbyte[totalrepeats];
			for (int ontime = 0; ontime < totalrepeats; ontime++) {
				repeatbyte[ontime] = ipsbyte[ipson];
			}
			tonccpy(rombyte+offset, repeatbyte, totalrepeats);
			ipson++;
		} else {
			totalrepeats = ipsbyte[ipson] * 256 + ipsbyte[ipson + 1];
			ipson++;
			ipson++;
			tonccpy(rombyte+offset, ipsbyte+ipson, totalrepeats);
			ipson += totalrepeats;
		}
		if (ipsbyte[ipson] == 69 && ipsbyte[ipson + 1] == 79 && ipsbyte[ipson + 2] == 70) {
			break;
		}
	}
}