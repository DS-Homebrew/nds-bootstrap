@---------------------------------------------------------------------------------
	.align	4
	.arm
	.global hgssEngOverlayApFix
	.global saga2OverlayApFix
@---------------------------------------------------------------------------------
hgssEngOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5900+0x219
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
.pool
@---------------------------------------------------------------------------------
saga2OverlayApFix: @ overlay9_2
	ldr r0, =0x0213C9C0+0x22790
	ldr r1, [r0]
	ldr r2, =0xE28DB020
	cmp r2, r1
	bxne lr
	ldr r1, [r0, #4]
	ldr r2, =0xEA00000C
	cmp r2, r1
	bxne lr
	mov r2, #0x37
	strb r2, [r0, #4]
	bx lr
.pool