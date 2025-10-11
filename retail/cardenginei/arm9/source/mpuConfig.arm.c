#include <nds/ndstypes.h>

/*void region2Disable() {
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}*/

#ifdef TWLSDK
void setLowVectors(void) {
	asm("LDR R12,=#0x5507D\n\tmcr p15, 0, r12, C1,C0,0\nMOV R12,#0x31\n\tmcr p15, 0, r12, C6,C5,0");
}
#else
// Required for proper access to the extra DSi RAM
void debugRamMpuFix() {
	//asm("MOV R0,#0x4A\n\tmcr p15, 0, r0, C2,C0,0\nLDR R0,=#0x15111111\n\tmcr p15, 0, r0, C5,C0,2");
	asm("PUSH {R0}\nLDR R0,=#0x15111111\n\tmcr p15, 0, r0, C5,C0,2\nPOP {R0}");
}

// Revert region 0 patch
/* void region0Fix() {
	asm("PUSH {R0}\nLDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0\nPOP {R0}");
} */
#endif

void resetMpu(void) {
	asm("PUSH {R0}\nLDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0\nPOP {R0}");
}
