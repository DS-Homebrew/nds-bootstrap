@---------------------------------------------------------------------------------
	.global hakokoroUnusedFontLoad
	.align	4
	.arm

@---------------------------------------------------------------------------------
hakokoroUnusedFontLoad:
@---------------------------------------------------------------------------------
	cmp r1, #0
	ldreq r2, hakokoroUnusedFont0
	beq hakokoroRunFontLoad
	cmp r1, #1
	ldreq r2, hakokoroUnusedFont1
	beq hakokoroRunFontLoad
	cmp r1, #2
	ldreq r2, hakokoroUnusedFont2
hakokoroRunFontLoad:
	ldr pc, hakokoroFontLoad
hakokoroUnusedFont0:
.word 0x020C96B0
hakokoroUnusedFont1:
.word 0x020CC5CC
hakokoroUnusedFont2:
.word 0x020CCE98
hakokoroFontLoad:
.word 0x02021550
@---------------------------------------------------------------------------------
