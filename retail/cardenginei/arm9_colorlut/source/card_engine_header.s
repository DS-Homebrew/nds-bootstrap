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

applyColorLutMobiclip:
	mov r0, r2
	sub r0, #0x18000
	bl applyColorLutBitmap
	ldmfd sp!, {r4-r12,pc}

card_engine_end:
