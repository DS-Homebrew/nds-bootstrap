.align	4
.arm

.global slot2MpuFix
.type	slot2MpuFix STT_FUNC
slot2MpuFix:
    MOV R0,#0x4A
    mcr p15, 0, r0, C2,C0,0
    mcr p15, 0, r0, C2,C0,1
    MOV R0,#0xA
    mcr p15, 0, r0, C3,C0,0
    bx lr

// Revert region 0 patch
.global region0Fix
.type	region0Fix STT_FUNC
region0Fix:
	LDR R0,= 0x4000033
    mcr p15, 0, r0, C6,C0,0
    bx lr

.global sdk5MpuFix
.type	sdk5MpuFix STT_FUNC
sdk5MpuFix:
	LDR R0,= 0x2000031
    mcr p15, 0, r0, C6,C1,0
    bx lr

.global resetMpu
.type	resetMpu STT_FUNC
resetMpu:
	LDR R0,= 0x12078
    mcr p15, 0, r0, C1,C0,0
    bx lr

.pool
.end
