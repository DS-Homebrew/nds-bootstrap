@---------------------------------------------------------------------------------
@ DS processor selection
@---------------------------------------------------------------------------------
	.arch	armv5te
	.cpu	arm946e-s

	.text
	.arm
	
	.global myMemCached
	.type	myMemCached STT_FUNC
@---------------------------------------------------------------------------------
myMemCached:
@---------------------------------------------------------------------------------
	ldr	r1,=masks
	ldr	r1, [r1]
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
	ldr	r1,=masks
	ldr	r1, [r1]
	ldr	r2,[r1],#8
	and	r0,r0,r2
	ldr	r2,[r1]
	orr	r0,r0,r2
	bx	lr

	.data
	.align	2

dsmasks:
	.word	0x003fffff, 0x02000000, 0x02c00000
debugmasks:
	.word	0x007fffff, 0x02000000, 0x02800000
dsimasks:
	.word	0x00ffffff, 0x02000000, 0x0c000000

masks:	.word	dsimasks
