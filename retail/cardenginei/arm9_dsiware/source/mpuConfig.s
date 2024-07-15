.align	4
.arm

.global resetMpu
.type	resetMpu STT_FUNC
resetMpu:
	LDR R0,= 0x12078
    mcr p15, 0, r0, C1,C0,0
    bx lr

.pool
.end
