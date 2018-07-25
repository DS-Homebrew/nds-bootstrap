#include <string.h> // memcmp
#include <stddef.h> // NULL
#include <limits.h>
#include "card_finder.h"

// (memcmp is slower)
//#define memcmp __builtin_memcmp

#define TABLE_SIZE (UCHAR_MAX + 1) // 256

extern inline u32* findOffset(const u32* start, u32 dataSize, const u32* find, u32 findLen);
extern inline u32* findOffsetBackwards(const u32* start, u32 dataSize, const u32* find, u32 findLen);
extern inline u16* findOffsetThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen);
extern inline u16* findOffsetBackwardsThumb(const u16* start, u32 dataSize, const u16* find, u32 findLen);

/*
*   Look for @find and return the position of it.
*   Boyer-Moore Horspool algorithm
*/
u8* memsearch(const u8 *start, u32 dataSize, const u8 *find, u32 findSize) {
    u32 table[TABLE_SIZE];

    // Preprocessing
    for (u32 i = 0; i < TABLE_SIZE; ++i) {
        table[i] = findSize;
    }
    for (u32 i = 0; i < findSize - 1; ++i) {
        table[find[i]] = findSize - i - 1;
    }

    // Searching
    u32 j = 0;
    while (j <= dataSize - findSize) {
        u8 c = start[j + findSize - 1];
        if (find[findSize - 1] == c && memcmp(find, start + j, findSize - 1) == 0) {
            return (u8*)start + j;
        }
        j += table[c];
    }

    return NULL;
}

/*
*   Quick Search algorithm
*/
/*u8* memsearch(const u8 *start, u32 dataSize, const u8 *find, u32 findSize) {
    u32 table[TABLE_SIZE];

    // Preprocessing
    for (u32 i = 0; i < TABLE_SIZE; ++i) {
        table[i] = findSize + 1;
    }
    for (u32 i = 0; i < findSize; ++i) {
        table[find[i]] = findSize - i;
    }

    // Searching
    u32 j = 0;
    while (j <= dataSize - findSize) {
        if (memcmp(find, start + j, findSize) == 0) {
            return (u8*)start + j;
        }
        j += table[start[j + findSize]];
    }

    return NULL;
}*/
