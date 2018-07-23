//#include <string.h>
//#include <stdlib.h>
//#include <nds/system.h>
//#include <nds/memory.h>
#include "card_finder.h"
#include "debugToFile.h"

// Look for @find and return the position of it.
u32* findOffset(u32* addr, size_t size, u32* find, size_t lenofFind, int direction) {
	u32* debug = (u32*)0x037D0000;
	u32* end = addr + size/sizeof(u32);
	debug[3] = (u32)end;
	for (; addr != end; addr += direction) {
		bool found = true;

		for (int i = 0; i < lenofFind; i++) {
			if (addr[i] != find[i]) {
				found = false;
				break;
			}
		}

		if (found) {
			return addr;
		}
	}

	return NULL;
}

u16* findOffsetThumb(u16* addr, size_t size, u16* find, size_t lenofFind, int direction) {
	for (u16* end = addr + size/sizeof(u16); addr != end; addr += direction) {
		bool found = true;

		for (int i = 0; i < lenofFind; i++) {
			if (addr[i] != find[i]) {
				found = false;
				break;
			}
		}

		if (found) {
			return addr;
		}
	}

	return NULL;
}
