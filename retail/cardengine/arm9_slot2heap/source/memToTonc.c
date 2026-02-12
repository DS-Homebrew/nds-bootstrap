#include <nds/ndstypes.h>
#include "tonccpy.h"

void* memcpy(void* dst, const void* src, u32 len) {
	tonccpy(dst, src, len);
	return dst;
}

void* memset(void* dst, u32 src, u32 len) {
	toncset(dst, src, len);
	return dst;
}

char* strncpy(char* dst, const char* src, u32 len) {
	tonccpy(dst, src, len);
	return dst;
}
