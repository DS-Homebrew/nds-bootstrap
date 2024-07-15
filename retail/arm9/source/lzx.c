/*----------------------------------------------------------------------------*/
/*--  lzx.c - LZ eXtended coding for Nintendo GBA/DS                        --*/
/*--  Copyright (C) 2011 CUE                                                --*/
/*--                                                                        --*/
/*--  This program is free software: you can redistribute it and/or modify  --*/
/*--  it under the terms of the GNU General Public License as published by  --*/
/*--  the Free Software Foundation, either version 3 of the License, or     --*/
/*--  (at your option) any later version.                                   --*/
/*--                                                                        --*/
/*--  This program is distributed in the hope that it will be useful,       --*/
/*--  but WITHOUT ANY WARRANTY; without even the implied warranty of        --*/
/*--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          --*/
/*--  GNU General Public License for more details.                          --*/
/*--                                                                        --*/
/*--  You should have received a copy of the GNU General Public License     --*/
/*--  along with this program. If not, see <http://www.gnu.org/licenses/>.  --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#include <nds/ndstypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
#define CMD_DECODE    0x00       // decode
#define CMD_CODE_11   0x11       // LZX big endian magic number
#define CMD_CODE_40   0x40       // LZX low endian magic number

#define LZX_WRAM      0x00       // VRAM file not compatible (0)
#define LZX_VRAM      0x01       // VRAM file compatible (1)

#define LZX_SHIFT     1          // bits to shift
#define LZX_MASK      0x80       // first bit to check
                                 // ((((1 << LZX_SHIFT) - 1) << (8 - LZX_SHIFT)

#define LZX_THRESHOLD 2          // max number of bytes to not encode
#define LZX_N         0x1000     // max offset (1 << 12)
#define LZX_F         0x10       // max coded (1 << 4)
#define LZX_F1        0x110      // max coded ((1 << 4) + (1 << 8))
#define LZX_F2        0x10110    // max coded ((1 << 4) + (1 << 8) + (1 << 16))

#define RAW_MINIM     0x00000000 // empty file, 0 bytes
#define RAW_MAXIM     0x00FFFFFF // 3-bytes length, 16MB - 1

#define LZX_MINIM     0x00000004 // header only (empty RAW file)
#define LZX_MAXIM     0x01400000 // 0x01200006, padded to 20MB:
                                 // * header, 4
                                 // * length, RAW_MAXIM
                                 // * flags, (RAW_MAXIM + 7) / 8
                                 // * 3 (flag + 2 end-bytes)
                                 // 4 + 0x00FFFFFF + 0x00200000 + 3 + padding

/*----------------------------------------------------------------------------*/
bool LZX_DecodeFromFile(unsigned char *raw, FILE* file, unsigned int raw_len) {
  unsigned char  header, pak, *raw_end;
  unsigned int   len, pos, threshold, tmp;
  unsigned char  flags, mask;

  fseek(file, 0, SEEK_SET);
  fread(&header, 1, 1, file);
  if ((header != CMD_CODE_11) && ((header != CMD_CODE_40))){
    return false;
  }

  fseek(file, 4, SEEK_SET);
  raw_end = raw + raw_len;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= LZX_SHIFT)) {
      if (!fread(&pak, 1, 1, file)) break;
      flags = pak;
      if (header == CMD_CODE_40) flags = -flags;
      mask = LZX_MASK;
    }

    if (!(flags & mask)) {
      if (!fread(&pak, 1, 1, file)) break;
      *raw++ = pak;
    } else {
      if (header == CMD_CODE_11) {
        if (!fread(&pak, 1, 1, file)) break;
        pos = pak;
        if (!fread(&pak, 1, 1, file)) break;
        pos = (pos << 8) | pak;

        tmp = pos >> 12;
        if (tmp < LZX_THRESHOLD) {
          pos &= 0xFFF;
          if (!fread(&pak, 1, 1, file)) break;
          pos = (pos << 8) | pak;
          threshold = LZX_F;
          if (tmp) {
            if (!fread(&pak, 1, 1, file)) break;
            pos = (pos << 8) | pak;
            threshold = LZX_F1;
          }
        } else {
          threshold = 0;
        }

        len = (pos >> 12) + threshold + 1;
        pos = (pos & 0xFFF) + 1;
      } else {
        if (!fread(&pak, 1, 1, file)) break;
        pos = pak;
        if (!fread(&pak, 1, 1, file)) break;
        pos |= pak << 8;

        tmp = pos & 0xF;
        if (tmp < LZX_THRESHOLD) {
          if (!fread(&pak, 1, 1, file)) break;
          len = pak;
          threshold = LZX_F;
          if (tmp) {
            if (!fread(&pak, 1, 1, file)) break;
            len = (pak << 8) | len;
            threshold = LZX_F1;
          }
        } else {
          len = tmp;
          threshold = 0;
        }

        len += threshold;
        pos >>= 4;
      }

      if (raw + len > raw_end) {
        len = raw_end - raw;
      }

      while (len--) *raw++ = *(raw - pos);
    }
  }
  return true;
}

/*----------------------------------------------------------------------------*/
/*--  EOF                                           Copyright (C) 2011 CUE  --*/
/*----------------------------------------------------------------------------*/
