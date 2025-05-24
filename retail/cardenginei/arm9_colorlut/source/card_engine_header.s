@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.align	4
	.arm

card_engine_start:

	b applyColorLut

.global flags
flags:
.word 0

.word saveMobiclipFrameDst
.word applyColorLutMobiclip

saveMobiclipFrameDst:
	ldr r1, [r0,#4]
	ldr r2, [r0,#8]
	adr r11, mobiclipFrameDst
	str r2, [r11]
	ldr r2, =0x03733800 @ new frame dst
	ldr r0, [r0]
	add lr, #4
	bx lr
.global mobiclipFrameDst
mobiclipFrameDst:
.word 0
.pool

applyColorLutMobiclip:
	mov r0, r2
	sub r0, #0x18000
	bl applyColorLutBitmap
	ldmfd sp!, {r4-r12,pc}

card_engine_end:
