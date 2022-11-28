@---------------------------------------------------------------------------------
	.global copyBackCe9
	.global copyBackCe9OrgFunc
	.align	4
	.arm

@.word	0x39394543
copyBackCe9:
	.word	copyBackCe9Func

@---------------------------------------------------------------------------------
copyBackCe9Func:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r0-r4,lr}

	ldr r0, =0x02370000 @ src
	ldr r1, =0x023F9C00 @ dst
	mov r2, #0x3400 @ len
	mov r3, r0
	mov r4, r2
	bl cpuCopy32

	mov r0, #0
	mov r1, r3
	mov r2, r4
	bl cpuClear32

	ldr r0, =0x02378000 @ src, Check if FAT table exists in main RAM
	ldr r1, [r0]
	cmp r1, #0
	beq copyBackCe9Func_cont
	ldr r1, =0x023E8000 @ dst
	mov r2, #0x8000 @ len
	mov r3, r0
	mov r4, r2
	bl cpuCopy32

	mov r0, #0
	mov r1, r3
	mov r2, r4
	bl cpuClear32

copyBackCe9Func_cont:
	ldr r4, copyBackCe9OrgFunc
	cmp r4, #0
	ldmeqfd   sp!, {r0-r4,pc}
	bl copyBackCe9_blx_r4

	ldmfd   sp!, {r0-r4,pc}
copyBackCe9_blx_r4:
	bx	r4
copyBackCe9OrgFunc:
.word	0
.pool
@---------------------------------------------------------------------------------

cpuCopy32:
	ADD		R12, R1, R2

cpuCopy32_loop:
	CMP		R1, R12
	LDMLTIA	R0!, {R2}
	STMLTIA	R1!, {R2}
	BLT		cpuCopy32_loop
	BX	LR

cpuClear32:
	ADD		R12, R1, R2

cpuClear32_loop:
	CMP	R1, R12
	STMLTIA	R1!, {R0}
	BLT		cpuClear32_loop
	BX		LR
