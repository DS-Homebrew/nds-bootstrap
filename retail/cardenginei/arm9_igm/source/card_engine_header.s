@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global igmText
	.global sharedAddr
	.global scfgExtBak
	.global scfgClkBak
	.align	4
	.arm

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#Text is placed here
igmText:
.space 0xA0C
.align 4

sharedAddr:
.word 0
scfgExtBak:
.word 0
scfgClkBak:
.hword 0
.align 4

card_engine_start:

@---------------------------------------------------------------------------------
igm_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r2-r11,lr}

	bl		inGameMenu

	ldmfd   sp!, {r2-r11,pc}
card_engine_end:
