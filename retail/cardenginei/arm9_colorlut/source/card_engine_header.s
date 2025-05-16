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

card_engine_end:
