#include <nds/ndstypes.h>

// Revert region 0 patch
void region0Fix() {
	asm("LDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0");
}

void sdk5MpuFix() {
	asm("LDR R0,=#0x2000031\n\tmcr p15, 0, r0, C6,C1,0");
}

