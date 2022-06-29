#include <nds/ndstypes.h>

#ifdef TWLSDK
/*void openDebugRam() {
	asm("LDR R0,=#0x8000035\n\tmcr p15, 0, r0, C6,C3,0");
}*/
#else
// Revert region 0 patch
void region0Fix() {
	asm("LDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0");
}

void mpuFix() {
	asm("LDR R0,=#0x2000031\n\tmcr p15, 0, r0, C6,C1,0");
}
#endif
void resetMpu() {
	asm("LDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0");
}
