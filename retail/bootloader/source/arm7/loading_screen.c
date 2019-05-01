#include "common.h"
#include "loading_screen.h"

void errorOutput(void) {
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);

	arm9_stateFlag = ARM9_DISPERR;

	// Stop
	while (1);
}
