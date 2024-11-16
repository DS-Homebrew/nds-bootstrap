.align	4
.arm

.global slot2MpuFixGbaDldi
.type slot2MpuFixGbaDldi, STT_FUNC
slot2MpuFixGbaDldi:
    mrc p15, 0, r0, c2, c0, 0
    bic r0, r0, #(1 << 3)
    mcr p15, 0, r0, c2, c0, 0
    mrc p15, 0, r0, c3, c0, 0
    bic r0, r0, #(1 << 3)
    mcr p15, 0, r0, c3, c0, 0
    mrc p15, 0, r0, c5, c0, 2
    bic r0, r0, #(0xF << 12)
    orr r0, r0, #(3 << 12)
    mcr p15, 0, r0, c5, c0, 2
    bx lr

.global slot2MpuFix
.type	slot2MpuFix STT_FUNC
slot2MpuFix:
	push {r0, lr}
    MOV R0,#0x4A
    mcr p15, 0, r0, C2,C0,0
    mcr p15, 0, r0, C2,C0,1
    MOV R0,#0xA
    mcr p15, 0, r0, C3,C0,0
	pop {r0, pc}

// Revert region 0 patch
@.global region0Fix
@.type	region0Fix STT_FUNC
@region0Fix:
@	LDR R0,= 0x4000033
@    mcr p15, 0, r0, C6,C0,0
@    bx lr

.global sdk5MpuFix
.type	sdk5MpuFix STT_FUNC
sdk5MpuFix:
	push {r0, lr}
	LDR R0,= 0x2000031
    mcr p15, 0, r0, C6,C1,0
	pop {r0, pc}

.global resetMpu
.type	resetMpu STT_FUNC
resetMpu:
	push {r0, lr}
	LDR R0,= 0x12078
    mcr p15, 0, r0, C1,C0,0
	pop {r0, pc}

.pool
.end
