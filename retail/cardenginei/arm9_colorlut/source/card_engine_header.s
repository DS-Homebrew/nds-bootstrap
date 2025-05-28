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
.word applyColorLutCastlevaniaDoSVideo

saveEarlyMobiclipFrameDst:
	mov r10, #0xC0
saveMobiclipFrameDst:
	adr r11, mobiclipFrameHeight
	str r10, [r11]
	ldr r1, [r0,#4]
	ldr r2, [r0,#8]
	adr r11, mobiclipFrameDst
	str r2, [r11]
	ldr r2, =0x03733800 @ new frame dst
	ldr r0, [r0]
	add lr, #4
	bx lr

applyColorLutMobiclip:
	mov r0, r2
	bl applyColorLutBitmap
	ldmfd sp!, {r4-r12,pc}

applyColorLutCastlevaniaDoSVideo:
	stmfd sp!, {r4-r5,lr}
	mov r4, #0xC0
	adr r5, mobiclipFrameHeight
	str r4, [r5]
	adr r5, mobiclipFrameDst
	str r2, [r5]
	mov r0, r1
	add r0, r3
	bl applyColorLutBitmap
	ldmfd sp!, {r4-r5,pc}
.global mobiclipFrameHeight
mobiclipFrameHeight:
.word 0
.global mobiclipFrameDst
mobiclipFrameDst:
.word 0
.pool

card_engine_end:
