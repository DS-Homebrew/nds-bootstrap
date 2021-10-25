/*----------------------------------------------------------------------------*/
/*--  lzss.c - LZSS coding for Nintendo GBA/DS                              --*/
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
#define CMD_DECODE    0x00       // decode
#define CMD_CODE_10   0x10       // LZSS magic number

#define LZS_NORMAL    0x00       // normal mode, (0)
#define LZS_FAST      0x80       // fast mode, (1 << 7)
#define LZS_BEST      0x40       // best mode, (1 << 6)

#define LZS_WRAM      0x00       // VRAM not compatible (LZS_WRAM | LZS_NORMAL)
#define LZS_VRAM      0x01       // VRAM compatible (LZS_VRAM | LZS_NORMAL)
#define LZS_WFAST     0x80       // LZS_WRAM fast (LZS_WRAM | LZS_FAST)
#define LZS_VFAST     0x81       // LZS_VRAM fast (LZS_VRAM | LZS_FAST)
#define LZS_WBEST     0x40       // LZS_WRAM best (LZS_WRAM | LZS_BEST)
#define LZS_VBEST     0x41       // LZS_VRAM best (LZS_VRAM | LZS_BEST)

#define LZS_SHIFT     1          // bits to shift
#define LZS_MASK      0x80       // bits to check:
                                 // ((((1 << LZS_SHIFT) - 1) << (8 - LZS_SHIFT)

#define LZS_THRESHOLD 2          // max number of bytes to not encode
#define LZS_N         0x1000     // max offset (1 << 12)
#define LZS_F         0x12       // max coded ((1 << 4) + LZS_THRESHOLD)
#define LZS_NIL       LZS_N      // index for root of binary search trees

#define RAW_MINIM     0x00000000 // empty file, 0 bytes
#define RAW_MAXIM     0x00FFFFFF // 3-bytes length, 16MB - 1

#define LZS_MINIM     0x00000004 // header only (empty RAW file)
#define LZS_MAXIM     0x01400000 // 0x01200003, padded to 20MB:
                                 // * header, 4
                                 // * length, RAW_MAXIM
                                 // * flags, (RAW_MAXIM + 7) / 8
                                 // 4 + 0x00FFFFFF + 0x00200000 + padding

/*----------------------------------------------------------------------------*/
unsigned char ring[LZS_N + LZS_F - 1];
int           dad[LZS_N + 1], lson[LZS_N + 1], rson[LZS_N + 1 + 256];
int           pos_ring, len_ring, lzs_vram;

/*----------------------------------------------------------------------------*/
#define BREAK(text) { printf(text); return; }
#define EXIT(text)  { printf(text); exit(-1); }

/*----------------------------------------------------------------------------*/
void  Title(void);
void  Usage(void);

unsigned char *Load(char *filename, unsigned int *length, int min, int max);
void  Save(char *filename, unsigned char *buffer, int length);
unsigned char *Memory(int length, int size);

void  LZS_Decode(char *filename);
void  LZS_Encode(char *filename, int mode);
unsigned char *LZS_Code(unsigned char *raw_buffer, int raw_len, unsigned int *new_len, int best);

unsigned char *LZS_Fast(unsigned char *raw_buffer, int raw_len, unsigned int *new_len);
void  LZS_InitTree(void);
void  LZS_InsertNode(int r);
void  LZS_DeleteNode(int p);

/*----------------------------------------------------------------------------*/
int main(int argc, char **argv) {
  int cmd, mode;
  int arg;

  Title();

  if (argc < 2) Usage();
  if      (!strcasecmp(argv[1], "-d"))   { cmd = CMD_DECODE; }
  else if (!strcasecmp(argv[1], "-evn")) { cmd = CMD_CODE_10; mode = LZS_VRAM; }
  else if (!strcasecmp(argv[1], "-ewn")) { cmd = CMD_CODE_10; mode = LZS_WRAM; }
  else if (!strcasecmp(argv[1], "-evf")) { cmd = CMD_CODE_10; mode = LZS_VFAST; }
  else if (!strcasecmp(argv[1], "-ewf")) { cmd = CMD_CODE_10; mode = LZS_WFAST; }
  else if (!strcasecmp(argv[1], "-evo")) { cmd = CMD_CODE_10; mode = LZS_VBEST; }
  else if (!strcasecmp(argv[1], "-ewo")) { cmd = CMD_CODE_10; mode = LZS_WBEST; }
  else                                  EXIT("Command not supported\n");
  if (argc < 3) EXIT("Filename not specified\n");

  switch (cmd) {
    case CMD_DECODE:
      for (arg = 2; arg < argc; arg++) LZS_Decode(argv[arg]);
      break;
    case CMD_CODE_10:
      for (arg = 2; arg < argc; arg++) LZS_Encode(argv[arg], mode);
      break;
    default:
      break;
  }

  printf("\nDone\n");

  return(0);
}

/*----------------------------------------------------------------------------*/
void Title(void) {
  printf(
    "\n"
    "LZSS - (c) CUE 2011\n"
    "LZSS coding for Nintendo GBA/DS\n"
    "\n"
  ); 
}

/*----------------------------------------------------------------------------*/
void Usage(void) {
  EXIT(
    "Usage: LZSS command filename [filename [...]]\n"
    "\n"
    "command:\n"
    "  -d ..... decode 'filename'\n"
    "  -evn ... encode 'filename', VRAM compatible, normal mode (LZ10)\n"
    "  -ewn ... encode 'filename', WRAM compatible, normal mode\n"
    "  -evf ... encode 'filename', VRAM compatible, fast mode\n"
    "  -ewf ... encode 'filename', WRAM compatible, fast mode\n"
    "  -evo ... encode 'filename', VRAM compatible, optimal mode (LZ-CUE)\n"
    "  -ewo ... encode 'filename', WRAM compatible, optimal mode (LZ-CUE)\n"
    "\n"
    "* multiple filenames and wildcards are permitted\n"
    "* the original file is overwritten with the new file\n"
  );
}

/*----------------------------------------------------------------------------*/
unsigned char *Load(char *filename, unsigned int *length, int min, int max) {
  FILE *fp;
  unsigned int fs;
  unsigned char *fb;

  if ((fp = fopen(filename, "rb")) == NULL) EXIT("\nFile open error\n");
  fseek(fp, 0, SEEK_END);
  fs = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if ((fs < min) || (fs > max)) EXIT("\nFile size error\n");
  fb = Memory(fs + 3, sizeof(char));
  if (fread(fb, 1, fs, fp) != fs) EXIT("\nFile read error\n");
  if (fclose(fp) == EOF) EXIT("\nFile close error\n");

  *length = fs;

  return(fb);
}

/*----------------------------------------------------------------------------*/
void Save(char *filename, unsigned char *buffer, int length) {
  FILE *fp;

  if ((fp = fopen(filename, "wb")) == NULL) EXIT("\nFile create error\n");
  if (fwrite(buffer, 1, length, fp) != length) EXIT("\nFile write error\n");
  if (fclose(fp) == EOF) EXIT("\nFile close error\n");
}

/*----------------------------------------------------------------------------*/
unsigned char *Memory(int length, int size) {
  unsigned char *fb;

  fb = (unsigned char *) calloc(length, size);
  if (fb == NULL) EXIT("\nMemory error\n");

  return(fb);
}

/*----------------------------------------------------------------------------*/
void LZS_Decode(char *filename) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   pak_len, raw_len, header, len, pos;
  unsigned char  flags, mask;

  printf("- decoding '%s'", filename);

  pak_buffer = Load(filename, &pak_len, LZS_MINIM, LZS_MAXIM);

  header = *pak_buffer;
  if (header != CMD_CODE_10) {
    free(pak_buffer);
    BREAK(", WARNING: file is not LZSS encoded!\n");
  }

  raw_len = *(unsigned int *)pak_buffer >> 8;
  raw_buffer = (unsigned char *) Memory(raw_len, sizeof(char));

  pak = pak_buffer + 4;
  raw = raw_buffer;
  pak_end = pak_buffer + pak_len;
  raw_end = raw_buffer + raw_len;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= LZS_SHIFT)) {
      if (pak == pak_end) break;
      flags = *pak++;
      mask = LZS_MASK;
    }

    if (!(flags & mask)) {
      if (pak == pak_end) break;
      *raw++ = *pak++;
    } else {
      if (pak + 1 >= pak_end) break;
      pos = *pak++;
      pos = (pos << 8) | *pak++;
      len = (pos >> 12) + LZS_THRESHOLD + 1;
      if (raw + len > raw_end) {
        printf(", WARNING: wrong decoded length!");
        len = raw_end - raw;
      }
      pos = (pos & 0xFFF) + 1;
      while (len--) {
        *raw = *(raw - pos);
        raw++;
      }
    }
  }

  raw_len = raw - raw_buffer;

  if (raw != raw_end) printf(", WARNING: unexpected end of encoded file!");

  Save(filename, raw_buffer, raw_len);

  free(raw_buffer);
  free(pak_buffer);

  printf("\n");
}

/*----------------------------------------------------------------------------*/
void LZS_Encode(char *filename, int mode) {
  unsigned char *raw_buffer, *pak_buffer, *new_buffer;
  unsigned int   raw_len, pak_len, new_len;

  lzs_vram = mode & 0xF;

  printf("- encoding '%s'", filename);

  raw_buffer = Load(filename, &raw_len, RAW_MINIM, RAW_MAXIM);

  pak_buffer = NULL;
  pak_len = LZS_MAXIM + 1;

  if (!(mode & LZS_FAST)) {
    mode = mode & LZS_BEST ? 1 : 0;
    new_buffer = LZS_Code(raw_buffer, raw_len, &new_len, mode);
  } else {
    new_buffer = LZS_Fast(raw_buffer, raw_len, &new_len);
  }
  if (new_len < pak_len) {
    if (pak_buffer != NULL) free(pak_buffer);
    pak_buffer = new_buffer;
    pak_len = new_len;
  }

  Save(filename, pak_buffer, pak_len);

  free(pak_buffer);
  free(raw_buffer);

  printf("\n");
}

/*----------------------------------------------------------------------------*/
unsigned char *LZS_Code(unsigned char *raw_buffer, int raw_len, unsigned int *new_len, int best) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *flg;
  unsigned int   pak_len, len, pos, len_best, pos_best;
  unsigned int   len_next, pos_next, len_post, pos_post;
  unsigned char  mask;

#define SEARCH(l,p) { \
  l = LZS_THRESHOLD;                                          \
                                                              \
  pos = raw - raw_buffer >= LZS_N ? LZS_N : raw - raw_buffer; \
  for ( ; pos > lzs_vram; pos--) {                            \
    for (len = 0; len < LZS_F; len++) {                       \
      if (raw + len == raw_end) break;                        \
      if (*(raw + len) != *(raw + len - pos)) break;          \
    }                                                         \
                                                              \
    if (len > l) {                                            \
      p = pos;                                                \
      if ((l = len) == LZS_F) break;                          \
    }                                                         \
  }                                                           \
}

  pak_len = 4 + raw_len + ((raw_len + 7) / 8);
  pak_buffer = (unsigned char *) Memory(pak_len, sizeof(char));

  *(unsigned int *)pak_buffer = CMD_CODE_10 | (raw_len << 8);

  pak = pak_buffer + 4;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= LZS_SHIFT)) {
      *(flg = pak++) = 0;
      mask = LZS_MASK;
    }

    SEARCH(len_best, pos_best);

    // LZ-CUE optimization start
    if (best) {
      if (len_best > LZS_THRESHOLD) {
        if (raw + len_best < raw_end) {
          raw += len_best;
          SEARCH(len_next, pos_next);
          raw -= len_best - 1;
          SEARCH(len_post, pos_post);
          raw--;

          if (len_next <= LZS_THRESHOLD) len_next = 1;
          if (len_post <= LZS_THRESHOLD) len_post = 1;

          if (len_best + len_next <= 1 + len_post) len_best = 1;
        }
      }
    }
    // LZ-CUE optimization end

    if (len_best > LZS_THRESHOLD) {
      raw += len_best;
      *flg |= mask;
      *pak++ = ((len_best - (LZS_THRESHOLD + 1)) << 4) | ((pos_best - 1) >> 8);
      *pak++ = (pos_best - 1) & 0xFF;
    } else {
      *pak++ = *raw++;
    }
  }

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

/*----------------------------------------------------------------------------*/
unsigned char *LZS_Fast(unsigned char *raw_buffer, int raw_len, unsigned int *new_len) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *flg;
  unsigned int   pak_len, len, r, s, len_tmp, i;
  unsigned char  mask; 

  pak_len = 4 + raw_len + ((raw_len + 7) / 8);
  pak_buffer = (unsigned char *) Memory(pak_len, sizeof(char));

  *(unsigned int *)pak_buffer = CMD_CODE_10 | (raw_len << 8);

  pak = pak_buffer + 4;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  LZS_InitTree();

  r = s = 0;

  len = raw_len < LZS_F ? raw_len : LZS_F;
  while (r < LZS_N - len) ring[r++] = 0;

  for (i = 0; i < len; i++) ring[r + i] = *raw++;

  LZS_InsertNode(r);

  mask = 0;

  while (len) {
    if (!(mask >>= LZS_SHIFT)) {
      *(flg = pak++) = 0;
      mask = LZS_MASK;
    }

    if (len_ring > len) len_ring = len;

    if (len_ring > LZS_THRESHOLD) {
      *flg |= mask;
      pos_ring = ((r - pos_ring) & (LZS_N - 1)) - 1;
      *pak++ = ((len_ring - LZS_THRESHOLD - 1) << 4) | (pos_ring >> 8);
      *pak++ = pos_ring & 0xFF;
    } else {
      len_ring = 1;
      *pak++ = ring[r];
    }

    len_tmp = len_ring;
    for (i = 0; i < len_tmp; i++) {
      if (raw == raw_end) break;
      LZS_DeleteNode(s);
      ring[s] = *raw++;
      if (s < LZS_F - 1) ring[s + LZS_N] = ring[s];
      s = (s + 1) & (LZS_N - 1);
      r = (r + 1) & (LZS_N - 1);
      LZS_InsertNode(r);
    }
    while (i++ < len_tmp) {
      LZS_DeleteNode(s);
      s = (s + 1) & (LZS_N - 1);
      r = (r + 1) & (LZS_N - 1);
      if (--len) LZS_InsertNode(r);
    }
  }

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

/*----------------------------------------------------------------------------*/
void LZS_InitTree(void) {
  int i;

  for (i = LZS_N + 1; i <= LZS_N + 256; i++)
    rson[i] = LZS_NIL;

  for (i = 0; i < LZS_N; i++)
    dad[i] = LZS_NIL;
}

/*----------------------------------------------------------------------------*/
void LZS_InsertNode(int r) {
  unsigned char *key;
  int            i, p, cmp, prev;

  prev = (r - 1) & (LZS_N - 1);

  cmp = 1;
  len_ring = 0;

  key = &ring[r];
  p = LZS_N + 1 + key[0];

  rson[r] = lson[r] = LZS_NIL;

  for ( ; ; ) {
    if (cmp >= 0) {
      if (rson[p] != LZS_NIL) p = rson[p];
      else                  { rson[p] = r; dad[r] = p; return; }
    } else {
      if (lson[p] != LZS_NIL) p = lson[p];
      else                  { lson[p] = r; dad[r] = p; return; }
    }

    for (i = 1; i < LZS_F; i++)
      if ((cmp = key[i] - ring[p + i])) break;

    if (i > len_ring) {
      if (!lzs_vram || (p != prev)) {
        pos_ring = p;
        if ((len_ring = i) == LZS_F) break;
      }
    }
  }

  dad[r] = dad[p]; lson[r] = lson[p]; rson[r] = rson[p];

  dad[lson[p]] = r; dad[rson[p]] = r;

  if (rson[dad[p]] == p) rson[dad[p]] = r;
  else                   lson[dad[p]] = r;

  dad[p] = LZS_NIL;
}

/*----------------------------------------------------------------------------*/
void LZS_DeleteNode(int p) {
  int q;
  
  if (dad[p] == LZS_NIL) return;

  if (rson[p] == LZS_NIL) {
    q = lson[p];
  } else if (lson[p] == LZS_NIL) {
    q = rson[p];
  } else {
    q = lson[p];
    if (rson[q] != LZS_NIL) {
      do {
        q = rson[q];
      } while (rson[q] != LZS_NIL);

      rson[dad[q]] = lson[q]; dad[lson[q]] = dad[q];
      lson[q]      = lson[p]; dad[lson[p]] = q;
    }

    rson[q] = rson[p]; dad[rson[p]] = q;
  }

  dad[q] = dad[p];

  if (rson[dad[p]] == p) rson[dad[p]] = q;
  else                   lson[dad[p]] = q;

  dad[p] = LZS_NIL;
}

/*----------------------------------------------------------------------------*/
/*--  EOF                                           Copyright (C) 2011 CUE  --*/
/*----------------------------------------------------------------------------*/
