/*
	Copyright (C) 2008 somebody
	Copyright (C) 2009 yellow wood goblin
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "nds_header.h"
#include "module_params.h"
#include "unpatched_funcs.h"
#include "decompress.h"
#include "debug_file.h"
#include "locations.h"
#include "tonccpy.h"

#define BLZ_SHIFT     1          // bits to shift
#define BLZ_MASK      0x80       // bits to check:
                                 // ((((1 << BLZ_SHIFT) - 1) << (8 - BLZ_SHIFT)

#define BLZ_THRESHOLD 2          // max number of bytes to not encode

#define RAW_MAXIM     0x00FFFFFF // 3-bytes length, 16MB - 1

extern bool dsiModeConfirmed;

static unsigned char* encr_data = (unsigned char*)BLOWFISH_LOCATION;

/*static void decompressLZ77Backwards(u8* addr, u32 size) {
	u32 len = *(u32*)(addr + size - 4) + size;
	
	if(len == size) {
		size -= 12;
	}
		
	len = *(u32*)(addr + size - 4) + size;
	
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
}*/

void BLZ_Invert(char *buffer, int length) {
  char *bottom, ch;

  bottom = buffer + length - 1;

  while (buffer < bottom) {
    ch = *buffer;
    *buffer++ = *bottom;
    *bottom-- = ch;
  }
}

u32 iUncompressedSize = 0;
u32 iUncompressedSizei = 0;
static u32 iFixedAddr = 0;
static u32 iFixedData = 0;

static u32 decompressBinary(u8 *aMainMemory, u32 aCodeLength, u32 aMemOffset) {
	u8 *ADDR1 = NULL;
	u8 *ADDR1_END = NULL;
	u8 *ADDR2 = NULL;
	u8 *ADDR3 = NULL;

	u8 *pBuffer32 = (u8 *)(aMainMemory);
	u8 *pBuffer32End = (u8 *)(aMainMemory + aCodeLength);

	while (pBuffer32 < pBuffer32End) {
		if (0xDEC00621 == *(u32 *)pBuffer32 && 0x2106C0DE == *(u32 *)(pBuffer32 + 4)) {
			ADDR1 = (u8 *)(*(u32 *)(pBuffer32 - 8));
			iFixedAddr = (u32)(pBuffer32 - 8);
			iFixedData = *(u32 *)(pBuffer32 - 8);
			*(u32 *)(pBuffer32 - 8) = 0;
			break;
		}
		pBuffer32 += 4;
	}
	if (0 == ADDR1) {
		iFixedAddr = 0;
		return 0;
	}

	u32 A = *(u32 *)(ADDR1 + aMemOffset - 4);
	u32 B = *(u32 *)(ADDR1 + aMemOffset - 8);
	ADDR1_END = ADDR1 + A;
	ADDR2 = ADDR1 - (B >> 24);
	B &= ~0xff000000;
	ADDR3 = ADDR1 - B;
	u32 uncompressEnd = ((u32)ADDR1_END) - ((u32)aMainMemory);

	while (!(ADDR2 <= ADDR3)) {
		u32 marku8 = *(--ADDR2 + aMemOffset);
		//ADDR2-=1;
		int count = 8;
		while (true) {
			count--;
			if (count < 0)
				break;
			if (0 == (marku8 & 0x80)) {
				*(--ADDR1_END + aMemOffset) = *(--ADDR2 + aMemOffset);
			} else {
				int u8_r12 = *(--ADDR2 + aMemOffset);
				int u8_r7 = *(--ADDR2 + aMemOffset);
				u8_r7 |= (u8_r12 << 8);
				u8_r7 &= ~0xf000;
				u8_r7 += 2;
				u8_r12 += 0x20;
				do
				{
					u8 realu8 = *(ADDR1_END + aMemOffset + u8_r7);
					*(--ADDR1_END + aMemOffset) = realu8;
					u8_r12 -= 0x10;
				} while (u8_r12 >= 0);
			}
			marku8 <<= 1;
			if (ADDR2 <= ADDR3) {
				break;
			}
		}
	}
	return uncompressEnd;
}

static u32 decompressIBinary(unsigned char *pak_buffer, unsigned int pak_len) {
  unsigned char *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   raw_len, len, pos, inc_len, hdr_len, enc_len, dec_len;
  unsigned char  flags = 0, mask;

  inc_len = *(unsigned int *)(pak_buffer + pak_len - 4);
  if (!inc_len) {
    dbg_printf("WARNING: not coded binary!\n");
    enc_len = 0;
    dec_len = pak_len;
    pak_len = 0;
    raw_len = dec_len;
  } else {
    hdr_len = pak_buffer[pak_len - 5];
    if ((hdr_len < 0x08) || (hdr_len > 0x0B)) {
		dbg_printf("Bad header length\n");
		return pak_len;
	}
    if (pak_len <= hdr_len) {
		dbg_printf("Bad length\n");
		return pak_len;
	}
	enc_len = *(unsigned int *)(pak_buffer + pak_len - 8) & 0x00FFFFFF;
    dec_len = pak_len - enc_len;
    pak_len = enc_len - hdr_len;
    raw_len = dec_len + enc_len + inc_len;
    if (raw_len > RAW_MAXIM) {
		dbg_printf("Bad decoded length\n");
		return 0;
	}
  }

  raw_buffer = (unsigned char *)0x02800000;

  pak = pak_buffer;
  raw = raw_buffer;
  pak_end = pak_buffer + dec_len + pak_len;
  raw_end = raw_buffer + raw_len;

  for (len = 0; len < dec_len; len++) *raw++ = *pak++;

  BLZ_Invert((char *)pak_buffer + dec_len, pak_len);

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= BLZ_SHIFT)) {
      if (pak == pak_end) break;
      flags = *pak++;
      mask = BLZ_MASK;
    }

    if (!(flags & mask)) {
      if (pak == pak_end) break;
      *raw++ = *pak++;
    } else {
      if (pak + 1 >= pak_end) break;
      pos = *pak++ << 8;
      pos |= *pak++;
      len = (pos >> 12) + BLZ_THRESHOLD + 1;
      if (raw + len > raw_end) {
        //printf(", WARNING: wrong decoded length!");
        len = raw_end - raw;
      }
      pos = (pos & 0xFFF) + 3;
      while (len--) {
		*raw = *(raw - pos);
		++raw;
	  }
    }
  }

  BLZ_Invert((char *)raw_buffer + dec_len, raw_len - dec_len);

	tonccpy(pak_buffer, raw_buffer, raw_len);
	toncset(raw_buffer, 0, raw_len);

	return raw_len;
}

static bool a9Decompressed = false;
static bool a9iDecompressed = false;

void ensureBinaryDecompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams, ltd_module_params_t* ltdModuleParams, bool arm9iToo) {
	if (a9Decompressed) return;

	unpatchedFunctions* unpatchedFuncs = (unpatchedFunctions*)UNPATCHED_FUNCTION_LOCATION;

	if (moduleParams->compressed_static_end && ((moduleParams->compressed_static_end/512)*512 == ((((u32)ndsHeader->arm9destination)+ndsHeader->arm9binarySize)/512)*512 || (moduleParams->compressed_static_end/4)*4 == 0xDEC00621)) {
		// Compressed
		dbg_printf("arm9 is compressed\n");
		unpatchedFuncs->compressedFlagOffset = (u32*)((u32)moduleParams+0x14);
		unpatchedFuncs->compressed_static_end = moduleParams->compressed_static_end;
		//decompressLZ77Backwards((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
		iUncompressedSize = decompressBinary((u8*)ndsHeader->arm9destination, ndsHeader->arm9binarySize, 0);
		moduleParams->compressed_static_end = 0;
	} else {
		// Not compressed
		dbg_printf("arm9 is not compressed\n");
		iUncompressedSize = ndsHeader->arm9binarySize;
	}
	a9Decompressed = true;

	if (!arm9iToo || a9iDecompressed) return;
	if (ndsHeader->unitCode > 0 && dsiModeConfirmed) {
		if (ltdModuleParams->compressed_static_end) {
			// Compressed
			dbg_printf("arm9i is compressed\n");
			unpatchedFuncs->iCompressedFlagOffset = (u32*)((u32)ltdModuleParams+0xC);
			unpatchedFuncs->ltd_compressed_static_end = ltdModuleParams->compressed_static_end;
			u32 arm9ioffset = *(u32*)0x02FFE1C8;
			iUncompressedSizei = decompressIBinary((unsigned char*)arm9ioffset, *(unsigned int*)0x02FFE1CC);
			ltdModuleParams->compressed_static_end = 0;
		} else {
			// Not compressed
			dbg_printf("arm9i is not compressed\n");
			iUncompressedSizei = *(u32*)0x02FFE1CC;
		}
	}
	a9iDecompressed = true;
}

u32 card_hash[0x412];

u32 lookup(u32 *magic, u32 v)
{
	u32 a = (v >> 24) & 0xFF;
	u32 b = (v >> 16) & 0xFF;
	u32 c = (v >> 8) & 0xFF;
	u32 d = (v >> 0) & 0xFF;

	a = magic[a+18+0];
	b = magic[b+18+256];
	c = magic[c+18+512];
	d = magic[d+18+768];

	return d + (c ^ (b + a));
}

void encrypt(u32 *magic, u32 *arg1, u32 *arg2)
{
	u32 a,b,c;
	a = *arg1;
	b = *arg2;
	for (int i=0; i<16; i++)
	{
		c = magic[i] ^ a;
		a = b ^ lookup(magic, c);
		b = c;
	}
	*arg2 = a ^ magic[16];
	*arg1 = b ^ magic[17];
}

void decrypt(u32 *magic, u32 *arg1, u32 *arg2)
{
	u32 a,b,c;
	a = *arg1;
	b = *arg2;
	for (int i=17; i>1; i--)
	{
		c = magic[i] ^ a;
		a = b ^ lookup(magic, c);
		b = c;
	}
	*arg1 = b ^ magic[0];
	*arg2 = a ^ magic[1];
}

void update_hashtable(u32* magic, u8 arg1[8])
{
	for (int j=0;j<18;j++)
	{
		u32 r3=0;
		for (int i=0;i<4;i++)
		{
			r3 <<= 8;
			r3 |= arg1[(j*4 + i) & 7];
		}
		magic[j] ^= r3;
	}

	u32 tmp1 = 0;
	u32 tmp2 = 0;
	for (int i=0; i<18; i+=2)
	{
		encrypt(magic,&tmp1,&tmp2);
		magic[i+0] = tmp1;
		magic[i+1] = tmp2;
	}
	for (int i=0; i<0x400; i+=2)
	{
		encrypt(magic,&tmp1,&tmp2);
		magic[i+18+0] = tmp1;
		magic[i+18+1] = tmp2;
	}
}

u32 arg2[3];

void init2(u32 *magic, u32 a[3])
{
	encrypt(magic, a+2, a+1);
	encrypt(magic, a+1, a+0);
	update_hashtable(magic, (u8*)a);
}

void init1(u32 cardheader_gamecode)
{
	tonccpy(card_hash, encr_data, 4*(1024 + 18));
	arg2[0] = *(u32 *)&cardheader_gamecode;
	arg2[1] = (*(u32 *)&cardheader_gamecode) >> 1;
	arg2[2] = (*(u32 *)&cardheader_gamecode) << 1;
	init2(card_hash, arg2);
	init2(card_hash, arg2);
}

// ARM9 decryption check values
#define MAGIC30		0x72636E65
#define MAGIC34		0x6A624F79

/*
 * decrypt_arm9
 */
bool decrypt_arm9ntr(const tDSiHeader* dsiHeader)
{
	// Decrypt NDS secure area
	u32 *p = (u32*)dsiHeader->ndshdr.arm9destination;

	if (p[0] == 0 || (p[0] == 0xE7FFDEFF && p[1] == 0xE7FFDEFF)) {
		toncset(encr_data, 0, 0x1048);
		return false;
	}

	u32 cardheader_gamecode = *(u32*)dsiHeader->ndshdr.gameCode;

	init1(cardheader_gamecode);
	decrypt(card_hash, p+1, p);
	arg2[1] <<= 1;
	arg2[2] >>= 1;	
	init2(card_hash, arg2);
	decrypt(card_hash, p+1, p);

	if (p[0] == MAGIC30 && p[1] == MAGIC34)
	{
		*p++ = 0xE7FFDEFF;
		*p++ = 0xE7FFDEFF;
	}
	else p+=2;
	u32 size = 0x800 - 8;
	while (size > 0)
	{
		decrypt(card_hash, p+1, p);
		p += 2;
		size -= 8;
	}

	toncset(encr_data, 0, 0x1048);
	return true;
}

bool decrypt_arm9(const tDSiHeader* dsiHeader)
{
	bool result = decrypt_arm9ntr(dsiHeader);

	return result;
}