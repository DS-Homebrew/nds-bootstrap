#ifndef LZ77_DECOMPRESS_H
#define LZ77_DECOMPRESS_H

#ifdef __cplusplus
extern "C" {
#endif
void LZ77_Decompress(const u8* source, u8* destination);
void LZ77_DecompressFromFile(u8* destination, FILE* file, const u32 leng);
u32 LZ77_GetLength(const u8* source);

#ifdef __cplusplus
}
#endif
#endif /* DECOMPRESS_H */
