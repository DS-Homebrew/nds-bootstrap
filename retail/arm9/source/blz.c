#include <nds/ndstypes.h>
#include "tonccpy.h"

u32 decompressLZ77Backwards(u8* addr, u32 size) {
	u32 len = *(u32*)(addr + size - 4) + size;

	if(len == size) {
		size -= 12;
	}

	len = *(u32*)(addr + size - 4) + size;

	u32 end = *(u32*)(addr + size - 8) & 0xFFFFFF;

	u8* result = addr;

	int Offs = (int)(size - (*(u32*)(addr + size - 8) >> 24));
	int dstoffs = (int)len;
	while (true) {
		u8 header = result[--Offs];
		for (int i = 0; i < 8; i++) {
			if ((header & 0x80) == 0) {
				toncset(result + (--dstoffs), result[--Offs], 1);
			} else {
				u8 a = result[--Offs];
				u8 b = result[--Offs];
				int offs = (((a & 0xF) << 8) | b) + 2;//+ 1;
				int length = (a >> 4) + 2;
				do {
					toncset(result + (dstoffs - 1), result[dstoffs + offs], 1);
					dstoffs--;
					length--;
				} while (length >= 0);
			}

			if (Offs <= size - end) {
				return len;
			}

			header <<= 1;
		}
	}

	return len;
}