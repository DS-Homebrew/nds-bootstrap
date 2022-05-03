#include <nds/ndstypes.h>

/*void region2Disable() {
	asm("MOV R0,#0\n\tmcr p15, 0, r0, C6,C2,0");
}*/

// Required for proper access to the extra DSi RAM
void debugRamMpuFix() {
	//asm("MOV R0,#0x4A\n\tmcr p15, 0, r0, C2,C0,0\nLDR R0,=#0x15111111\n\tmcr p15, 0, r0, C5,C0,2");
	asm("LDR R0,=#0x15111111\n\tmcr p15, 0, r0, C5,C0,2");
}

// Revert region 0 patch
void region0Fix() {
	asm("LDR R0,=#0x4000033\n\tmcr p15, 0, r0, C6,C0,0");
}
