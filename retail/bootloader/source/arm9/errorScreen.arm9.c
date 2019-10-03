/*-------------------------------------------------------------------------
Loading regular
--------------------------------------------------------------------------*/

#define ARM9
#undef ARM7

#include <nds/system.h>
#include <nds/arm9/video.h>

#include "common.h"
#include "loading.h"

void arm9_pleaseWaitText(void) {

	VRAM_A_CR = VRAM_ENABLE;

	dmaCopy((u16*)0x2350000, VRAM_A, 0x18000);

	REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
	REG_DISPCNT = MODE_FB0;
}

void arm9_errorText(void) {
	VRAM_A_CR = VRAM_ENABLE;

	dmaCopy((u16*)0x2368000, VRAM_A, 0x18000);

	REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
	REG_DISPCNT = MODE_FB0;
}
