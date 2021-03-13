@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#verText
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0

card_engine_start:

@---------------------------------------------------------------------------------
igm_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r11,lr}

	ldr		r6, =inGameMenu
    
	bl		_blx_r6_stub_igm

	ldmfd   sp!, {r4-r11,pc}
	bx      lr
_blx_r6_stub_igm:
	bx	r6
.pool
card_engine_end:
