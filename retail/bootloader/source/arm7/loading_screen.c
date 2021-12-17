#include "common.h"
#include "loading_screen.h"

void clearScreen(void) {

	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);

	arm9_stateFlag = ARM9_SCRNCLR;

	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);
}

void esrbOutput(void) {
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);

	arm9_stateFlag = ARM9_DISPESRB;

	// Wait until the ARM9 is ready
	//while (arm9_stateFlag != ARM9_READY);
}

void pleaseWaitOutput(void) {
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);

	arm9_stateFlag = ARM9_DISPSCRN;

	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);
}

void errorOutput(void) {
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);

	arm9_stateFlag = ARM9_DISPERR;

	// Stop
	while (1);
}
