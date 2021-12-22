/*-------------------------------------------------------------------------
Loading regular
--------------------------------------------------------------------------*/

#define ARM9
#undef ARM7

#include <nds/system.h>
#include <nds/arm9/video.h>

#include "locations.h"
#include "common.h"
#include "loading.h"

bool arm9_macroMode = false;

void arm9_esrbScreen(void) {
	dmaCopy((u16*)IMAGES_LOCATION, VRAM_A, 0x18000);

	REG_POWERCNT = arm9_macroMode ? (POWER_ALL_2D) : (POWER_ALL_2D | POWER_SWAP_LCDS);
	REG_DISPCNT = MODE_FB0;
}

void arm9_pleaseWaitText(void) {
	dmaCopy((u16*)(IMAGES_LOCATION+0x18000), VRAM_A, 0x18000);

	REG_POWERCNT = (u16)POWER_ALL_2D;
	REG_DISPCNT = MODE_FB0;
}

void arm9_errorText(void) {
	dmaCopy((u16*)(IMAGES_LOCATION+0x30000), VRAM_A, 0x18000);

	REG_POWERCNT =(u16) POWER_ALL_2D;
	REG_DISPCNT = MODE_FB0;
}
