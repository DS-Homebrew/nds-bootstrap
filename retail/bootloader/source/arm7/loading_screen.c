#include "common.h"
#include "loading_screen.h"

extern u32 loadingScreen;
extern u32 darkTheme;
extern u32 swapLcds;
extern u32 loadingFrames;
extern u32 loadingFps;
extern u32 loadingBar;
extern u32 loadingBarYpos;

void errorOutput(void) {
	if (loadingScreen > 0) {
		// Wait until the ARM9 is ready
		while (arm9_stateFlag != ARM9_READY);
		// Set the error code, then tell ARM9 to display it
		arm9_errorColor = true;
	}
	// Stop
	while (1);
}

void debugOutput(void) {
	if (loadingScreen > 0) {
		// Wait until the ARM9 is ready
		while (arm9_stateFlag != ARM9_READY);
		// Set the error code, then tell ARM9 to display it
		arm9_screenMode = loadingScreen - 1;
		arm9_darkTheme = darkTheme;
		arm9_swapLcds = swapLcds;
		arm9_loadingFrames = loadingFrames;
		arm9_loadingFps = loadingFps;
		arm9_loadingBar = loadingBar;
		arm9_loadingBarYpos = loadingBarYpos;
		arm9_stateFlag = ARM9_DISPERR;
		// Wait for completion
		while (arm9_stateFlag != ARM9_READY);
	}
}

void increaseLoadBarLength(void) {
	arm9_loadBarLength++;
	if (loadingScreen == 1) {
		debugOutput(); // Let the loading bar finish before ROM starts
	}
}

void fadeOut(void) {
	fadeType = false;
	while (screenBrightness != 25);	// Wait for screen to fade out
}
