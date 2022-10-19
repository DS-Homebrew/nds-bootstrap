#include <nds/ndstypes.h>

void slot2MpuFix() {
	asm("MOV R0,#0x4A\n\tmcr p15, 0, r0, C2,C0,0\n\tmcr p15, 0, r0, C2,C0,1\nMOV R0,#0xA\n\tmcr p15, 0, r0, C3,C0,0");
}

// Revert region 0 patch
void region0Fix() {
	asm("LDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0");
}

void sdk5MpuFix() {
	asm("LDR R0,=#0x2000031\n\tmcr p15, 0, r0, C6,C1,0");
}

void resetMpu() {
	asm("LDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0");
}
