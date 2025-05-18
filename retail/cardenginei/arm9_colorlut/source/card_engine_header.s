@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.align	4
	.arm

card_engine_start:

	b applyColorLut

.global flags
.global bankProcessSize

flags:
.hword 0
bankProcessSize:
.hword 0x2000

card_engine_end:
