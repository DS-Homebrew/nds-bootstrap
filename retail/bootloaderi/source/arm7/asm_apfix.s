@---------------------------------------------------------------------------------
	.align	4
	.arm
	.global ie3OgreOverlayApFix
	.global hgssEngOverlayApFix
	.global saga2OverlayApFix
@---------------------------------------------------------------------------------
ie3OgreOverlayApFix: @ overlay9_129
	ldr r0, =0x0212A9C0
	ldrb r1, [r0, #0x6D]
	mov r2, #0x8F
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x6F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x10D]
	mov r2, #0x45
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x10F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x1AD]
	mov r2, #0x1A
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x1AF]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x24D]
	mov r2, #0x9C
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x24F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	mov r2, #0x8E
	strb r2, [r0, #0x6D]
	mov r2, #0x36
	strb r2, [r0, #0x6F]
	mov r2, #0x44
	strb r2, [r0, #0x10D]
	mov r2, #0x36
	strb r2, [r0, #0x10F]
	mov r2, #0x19
	strb r2, [r0, #0x1AD]
	mov r2, #0x36
	strb r2, [r0, #0x1AF]
	mov r2, #0x9B
	strb r2, [r0, #0x24D]
	mov r2, #0x36
	strb r2, [r0, #0x24F]
	bx lr
.pool
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