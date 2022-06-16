#include "nds_header.h"
#include "module_params.h"
#include <nds/memory.h> // tNDSHeader

const char* getRomTid(const tNDSHeader* ndsHeader) {
	//u32 ROM_TID = *(u32*)ndsHeader->gameCode;
	static char romTid[5];
	strncpy(romTid, ndsHeader->gameCode, 4);
	romTid[4] = '\0';
	return romTid;
}

#ifndef NO_CARDID
const u32 getChipId(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
    u32 cardid = 0xC2;
    u8 size = ndsHeader->deviceSize;
    
    //Devicecapacity         (Chipsize = 128KB SHL nn) (eg. 7 = 16MB)
    //Chip size (00h..7Fh: (N+1)Mbytes, F0h..FFh: (100h-N)*256Mbytes?) (eg. 0Fh = 16MB)
	u8 mb = 0;
    switch (size) {
		default:
			break;
		case 0x04:
			mb = 0x01;
			break;
		case 0x05:
			mb = 0x03;
			break;
		case 0x06:
			mb = 0x07;
			break;
		case 0x07:
			mb = 0x0F;
			break;
		case 0x08:
			mb = 0x1F;
			break;
		case 0x09:
			mb = 0x3F;
			break;
		case 0x0A:
			mb = 0x7F;
			break;
		case 0x0B:
			mb = 0xFF;
			break;
		case 0x0C:
			mb = 0xFE;
			break;
	}
    cardid |= mb << 8;

	//The Flag Bits in 3th byte can be
	//0   Maybe Infrared flag? (in case ROM does contain on-chip infrared stuff)
	if (ndsHeader->gameCode[0] == 'I') {
		cardid |= 0x01 << 16;
	}

    //The Flag Bits in 4th byte can be
    //6   DSi flag (0=NDS/3DS, 1=DSi)
    //7   Cart Protocol Variant (0=older/smaller carts, 1=newer/bigger carts)
    u8 unit = 0;
    if (ndsHeader->unitCode==0x02) {
		unit=0xC0;
	} else if (ndsHeader->gameCode[0] == 'I') {
		unit=0xE0;
	} else if (moduleParams->sdk_version > 0x4000000) {
		unit=0x80;
	}
    cardid |= unit << 24;

    //if (memcmp(ndsHeader->gameCode, "BO5", 3) == 0)  cardid = 0xE080FF80; // golden sun
    return cardid;
}
#endif