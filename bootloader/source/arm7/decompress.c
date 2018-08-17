#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "nds_header.h"
#include "module_params.h"
#include "decompress.h"
#include "debug_file.h"

static void decompressLZ77Backwards(u8* addr, u32 size) {
	u32 len = *(u32*)(addr + size - 4) + size;

	//byte[] Result = new byte[len];
	//Array.Copy(Data, Result, Data.Length);

	u32 end = *(u32*)(addr + size - 8) & 0xFFFFFF;

	u8* result = addr;

	int Offs = (int)(size - (*(u32*)(addr + size - 8) >> 24));
	int dstoffs = (int)len;
	while (true) {
		u8 header = result[--Offs];
		for (int i = 0; i < 8; i++) {
			if ((header & 0x80) == 0) {
				result[--dstoffs] = result[--Offs];
			} else {
				u8 a = result[--Offs];
				u8 b = result[--Offs];
				int offs = (((a & 0xF) << 8) | b) + 2;//+ 1;
				int length = (a >> 4) + 2;
				do {
					result[dstoffs - 1] = result[dstoffs + offs];
					dstoffs--;
					length--;
				} while (length >= 0);
			}

			if (Offs <= size - end) {
				return;
			}

			header <<= 1;
		}
	}
}

static void ensureArm9Decompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams) {
	if (!moduleParams->compressed_static_end) {
		// Not compressed
		dbg_printf("This rom is not compressed\n");
		return;
	}
	dbg_printf("This rom is compressed\n");
	decompressLZ77Backwards((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
	moduleParams->compressed_static_end = 0;
}

void decompressBinary(const tNDSHeader* ndsHeader, module_params_t* moduleParams, bool foundModuleParams) {
	const char* romTid = getRomTid(ndsHeader);

	if (strcmp(romTid, "YQUJ") == 0 // Chrono Trigger (Japan)
	|| strcmp(romTid, "YQUE") == 0  // Chrono Trigger (USA)
	|| strcmp(romTid, "YQUP") == 0) // Chrono Trigger (Europe)
	{
		decompressLZ77Backwards((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
	}

	if (foundModuleParams) {
		*(vu32*)0x280000C = moduleParams->compressed_static_end; // from 'ensureArm9Decompressed'
	}
    ensureArm9Decompressed(ndsHeader, moduleParams);
}
