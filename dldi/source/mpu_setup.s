@---------------------------------------------------------------------------------
@ DS processor selection
@---------------------------------------------------------------------------------
	.arch	armv5te
	.cpu	arm946e-s

	.text
	.arm

dsmasks:
	.word	0x00Cfffff, 0x02000000, 0x02400000

masks:	.word	dsmasks

	.global myMemCached
	.type	myMemCached STT_FUNC
@---------------------------------------------------------------------------------
myMemCached:
@---------------------------------------------------------------------------------
	ldr	r1,masks
	ldr	r2,[r1],#4
	and	r0,r0,r2
	ldr	r2,[r1]
	orr	r0,r0,r2
	bx	lr

	.global	myMemUncached
	.type	myMemUncached STT_FUNC
@---------------------------------------------------------------------------------
myMemUncached:
@---------------------------------------------------------------------------------
	ldr	r1,masks
	ldr	r2,[r1],#8
	and	r0,r0,r2
	ldr	r2,[r1]
	orr	r0,r0,r2
	bx	lr