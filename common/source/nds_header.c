#include "nds_header.h"

const char* getRomTid(const tNDSHeader* ndsHeader) {
    //u32 ROM_TID = *(u32*)ndsHeader->gameCode;
	static char romTid[5];
    strncpy(romTid, ndsHeader->gameCode, 4);
    romTid[4] = '\0';
    return romTid;
}
