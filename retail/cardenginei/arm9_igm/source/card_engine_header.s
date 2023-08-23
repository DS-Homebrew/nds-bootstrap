@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global igmText
	.global sharedAddr
	.global waitSysCyclesLocPtr
	.global scfgExtBak
	.global scfgClkBak
	.global codeJump
	.global codeJump1
	.global codeJumpWord
	.global changeMpu
	.global revertMpu
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
waitSysCyclesLocPtr:
.word 0
scfgExtBak:
.word 0x027FEFF8
scfgClkBak:
.word 0x027FEFFC
.align 4

card_engine_start:

@---------------------------------------------------------------------------------
igm_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r2-r11,lr}

	bl		inGameMenu

	ldmfd   sp!, {r2-r11,pc}

@---------------------------------------------------------------------------------
codeJump: codeJump1: .word codeJump+4
@---------------------------------------------------------------------------------
	ldr pc, codeJumpWord
codeJumpWord:
.word	0

@---------------------------------------------------------------------------------
changeMpu: .word changeMpu+4
@---------------------------------------------------------------------------------
	stmfd   sp!, {r0-r2,lr}

	ldr r1, =mpuBits
	ldr r2, =0x027FF017

	mrc	p15,0,r0,c6,c0,0
	str r0, [r1]
	mov	r0, #0x35
	mcr	p15,0,r0,c6,c0,0

	mrc	p15,0,r0,c6,c2,0
	str r0, [r1, #4]
	cmp r0, r2
	beq changeMpu3
	mov	r0, #0
	mcr	p15,0,r0,c6,c2,0

changeMpu3:
	mrc	p15,0,r0,c6,c3,0
	str r0, [r1, #8]
	mov	r0, #0x08000000
	add	r0, r0, #0x35
	mcr	p15,0,r0,c6,c3,0

	ldmfd   sp!, {r0-r2,pc}

@---------------------------------------------------------------------------------
revertMpu: .word revertMpu+4
@---------------------------------------------------------------------------------
	stmfd   sp!, {r0-r1,lr}

	ldr r1, =mpuBits

	ldr r0, [r1]
	mcr	p15,0,r0,c6,c0,0
	ldr r0, [r1, #4]
	mcr	p15,0,r0,c6,c2,0
	ldr r0, [r1, #8]
	mcr	p15,0,r0,c6,c3,0

	ldmfd   sp!, {r0-r1,pc}
.pool

@ MPU region backups
mpuBits:
.word	0 @ Region 0
.word	0 @ Region 2
.word	0 @ Region 3

card_engine_end:
