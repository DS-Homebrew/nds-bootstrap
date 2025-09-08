@---------------------------------------------------------------------------------
	.align	4
	.arm
	.global hgssEngOverlayApFix
@---------------------------------------------------------------------------------
hgssEngOverlayApFix:
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