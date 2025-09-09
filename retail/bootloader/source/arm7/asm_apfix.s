@---------------------------------------------------------------------------------
	.align	4
	.arm
	.global ie3OgreOverlayApFix
	.global hgssJpnOverlayApFix
	.global hgssIntOverlayApFix
	.global hgssKorOverlayApFix
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
hgssJpnOverlayApFix: @ overlay9_1 + overlay9_122
	ldr r0, =0x021E4E40+0x21A
	ldrb r1, [r0]
	mov r2, #0x53
	cmp r2, r1
	bne hgssJpnOverlayApFix_check122
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssJpnOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	b hgssJpnOverlayApFix_check122
hgssJpnOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bne hgssJpnOverlayApFix_check122
	mov r2, #0xE0
	strb r2, [r0, #3]
hgssJpnOverlayApFix_check122:
	ldr r0, =0x0225E3C0
	ldr r1, [r0, #0x584]
	ldr r2, =0x510CE58D
	cmp r2, r1
	bxne lr
	mov r2, #0x36
	strb r2, [r0, #0x586]
	ldr r1, [r0, #0x6DC]
	ldr r2, =0x427AE112
	cmp r2, r1
	bxne lr
	mov r2, #0x7B
	strb r2, [r0, #0x6DE]
	bx lr
.pool
@---------------------------------------------------------------------------------
hgssIntOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5900+0x219 @ Offset varies by language
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssIntOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgssIntOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
@---------------------------------------------------------------------------------
hgssKorOverlayApFix: @ overlay9_1
	ldr r0, =0x021E6300+0x213 @ add 3 for SS
	ldrb r1, [r0]
	mov r2, #0x5B
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssKorOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgssKorOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
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