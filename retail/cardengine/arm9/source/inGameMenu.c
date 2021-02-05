#include <nds/ndstypes.h>
#include <nds/interrupts.h>

#include "locations.h"
#include "cardengine.h"
#include "nds_header.h"

extern vu32* volatile sharedAddr;

void inGameMenu(void) {
	int oldIME = enterCriticalSection();
	while (1) {
		if (sharedAddr[4] == 0x54495845
		|| sharedAddr[4] == 0x57534352) break;
	}
	leaveCriticalSection(oldIME);
}