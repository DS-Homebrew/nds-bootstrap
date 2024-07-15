#include <nds.h>
#include <string.h>
#include <stdio.h>

#define __itcm __attribute__((section(".itcm")))

void __itcm LZ77_Decompress(const u8* source, u8* destination) {
	u32 leng = (source[1] | (source[2] << 8) | (source[3] << 16));
	int Offs = 4;
	int dstoffs = 0;
	while (true)
	{
		u8 header = source[Offs++];
		for (int i = 0; i < 8; i++)
		{
			if ((header & 0x80) == 0) destination[dstoffs++] = source[Offs++];
			else
			{
				u8 a = source[Offs++];
				u8 b = source[Offs++];
				int offs = (((a & 0xF) << 8) | b) + 1;
				int length = (a >> 4) + 3;
				for (int j = 0; j < length; j++)
				{
					destination[dstoffs] = destination[dstoffs - offs];
					dstoffs++;
				}
			}
			if (dstoffs >= (int)leng) return;
			header <<= 1;
		}
	}
}

/* void LZ77_DecompressFromFile(u8* destination, FILE* file, const u32 leng) {
	int Offs = 4;
	fseek(file, Offs, SEEK_SET);
	int dstoffs = 0;
	while (true)
	{
		u8 header;
		fread(&header, 1, 1, file);
		for (int i = 0; i < 8; i++)
		{
			if ((header & 0x80) == 0)
			{
				u8 byte;
				fread(&byte, 1, 1, file);

				destination[dstoffs++] = byte;
			}
			else
			{
				u8 a[2];
				fread(a, 1, 2, file);
				int offs = (((a[0] & 0xF) << 8) | a[1]) + 1;
				int length = (a[0] >> 4) + 3;
				for (int j = 0; j < length; j++)
				{
					destination[dstoffs] = destination[dstoffs - offs];
					dstoffs++;
				}
			}
			if (dstoffs >= (int)leng) return;
			header <<= 1;
		}
	}
} */

u32 LZ77_GetLength(const u8* source) {
	return (source[1] | (source[2] << 8) | (source[3] << 16));
}
