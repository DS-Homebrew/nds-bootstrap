#ifndef LZX_DECOMPRESS_H
#define LZX_DECOMPRESS_H

#ifdef __cplusplus
extern "C" {
#endif
bool LZX_DecodeFromFile(unsigned char *raw, FILE* file, unsigned int raw_len);

#ifdef __cplusplus
}
#endif
#endif /* DECOMPRESS_H */
