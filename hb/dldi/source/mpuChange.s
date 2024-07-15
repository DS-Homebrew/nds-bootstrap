#include "asminc.h"

    .arm



BEGIN_ASM_FUNC unlockDSiWram
	mrc	p15,0,r0,c6,c0,0
	ldr r1, =0x04000033
	cmp r0, r1
	moveq	r0, #0x35
	mcreq	p15,0,r0,c6,c0,0
	bx lr
.pool