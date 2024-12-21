/*
	Noobish Noobsicle wrote this IPS patching code
	Adapted to C by RocketRobz
*/

#include <nds/ndstypes.h>
#include <nds/memory.h>
#include "nds_header.h"
#include "locations.h"
#include "tonccpy.h"

extern u8 gameOnFlashcard;
extern u8 consoleModel;
extern u8 _io_dldi_size;
extern bool dsiModeConfirmed;
extern bool sharedWramEnabled;
extern bool overlaysInRam;

extern bool scfgBios9i(void);

bool applyIpsPatch(const tNDSHeader* ndsHeader, u8* ipsbyte, const bool arm9Only, const bool laterSdk, const bool isSdk5, const bool ROMinRAM, const u32 cacheBlockSize) {
	if (ipsbyte[0] != 'P' && ipsbyte[1] != 'A' && ipsbyte[2] != 'T' && ipsbyte[3] != 'C' && ipsbyte[4] != 'H' && ipsbyte[5] != 0) {
		return false;
	}

	bool armPatched = false;
	const bool dsiBios = scfgBios9i();
	const u32 wramLocation = sharedWramEnabled ? 0x036F8000 : 0x03700000;

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
				extern u32 romMapLines;
				extern u32 romMap[5][3];
				int i = 0;
				for (i = 0; i < romMapLines; i++) {
					if (offset >= romMap[i][0] && (i == romMapLines-1 || offset < romMap[i+1][0])) {
						break;
					}
				}
				rombyte = (void*)(romMap[i][1]-romMap[i][0]);
			} else {
				rombyte = (void*)CACHE_ADRESS_START_DSIMODE;
				rombyte -= (ndsHeader->arm9overlaySource/cacheBlockSize)*cacheBlockSize;
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
					if (isSdk5 || (dsiBios && laterSdk)) {
						if ((u32)rombyteOffset == 0x0C7C4000) {
							rombyteOffset += (laterSdk ? 0x4000 : 0x24000);
						} else if (ndsHeader->unitCode == 0) {
							if ((u32)rombyteOffset == 0x0CFFC000) {
								rombyteOffset += 0x4000;
							} else if ((u32)rombyteOffset == 0x0C7D8000 && laterSdk) {
								rombyteOffset += 0x28000;
							} else if ((u32)rombyteOffset == 0x0C7F8000 && !laterSdk) {
								rombyteOffset += 0x8000;
							}
						} else {
							if ((u32)rombyteOffset == 0x0C7D8000) {
								rombyteOffset += 0x8000;
							} else if ((u32)rombyteOffset == 0x0C7FC000) {
								rombyteOffset += 0x4000;
							} else if ((u32)rombyteOffset == 0x0CFE0000) {
								rombyteOffset += 0x20000;
							}
						}
					} else if ((u32)rombyteOffset == 0x0CFFC000 && dsiBios) {
						rombyteOffset += 0x4000;
					} else if ((u32)rombyteOffset == 0x0C7C0000) {
						rombyteOffset += (laterSdk ? 0x8000 : 0x28000);
					} else if ((u32)rombyteOffset == 0x0C7D8000 && laterSdk) {
						rombyteOffset += 0x28000;
					} else if ((u32)rombyteOffset == 0x0C7F8000 && !laterSdk) {
						rombyteOffset += 0x8000;
					}
					if ((u32)rombyteOffset == (consoleModel > 0 ? 0x0E000000 : 0x0D000000)) {
						rombyteOffset = (u8*)wramLocation;
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
					if (isSdk5 || (dsiBios && laterSdk)) {
						if ((u32)rombyteOffset == 0x0C7C4000) {
							rombyteOffset += (laterSdk ? 0x4000 : 0x24000);
						} else if (ndsHeader->unitCode == 0) {
							if ((u32)rombyteOffset == 0x0CFFC000) {
								rombyteOffset += 0x4000;
							} else if ((u32)rombyteOffset == 0x0C7D8000 && laterSdk) {
								rombyteOffset += 0x28000;
							} else if ((u32)rombyteOffset == 0x0C7F8000 && !laterSdk) {
								rombyteOffset += 0x8000;
							}
						} else {
							if ((u32)rombyteOffset == 0x0C7D8000) {
								rombyteOffset += 0x8000;
							} else if ((u32)rombyteOffset == 0x0C7FC000) {
								rombyteOffset += 0x4000;
							} else if ((u32)rombyteOffset == 0x0CFE0000) {
								rombyteOffset += 0x20000;
							}
						}
					} else if ((u32)rombyteOffset == 0x0CFFC000 && dsiBios) {
						rombyteOffset += 0x4000;
					} else if ((u32)rombyteOffset == 0x0C7C0000) {
						rombyteOffset += (laterSdk ? 0x8000 : 0x28000);
					} else if ((u32)rombyteOffset == 0x0C7D8000 && laterSdk) {
						rombyteOffset += 0x28000;
					} else if ((u32)rombyteOffset == 0x0C7F8000 && !laterSdk) {
						rombyteOffset += 0x8000;
					}
					if ((u32)rombyteOffset == (consoleModel > 0 ? 0x0E000000 : 0x0D000000)) {
						rombyteOffset = (u8*)wramLocation;
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