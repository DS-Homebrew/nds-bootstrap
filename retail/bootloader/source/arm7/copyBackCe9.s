@---------------------------------------------------------------------------------
	.global copyBackCe9
	.align	4
	.arm

@.word	0x39394543
copyBackCe9:
	.word	copyBackCe9Func

@---------------------------------------------------------------------------------
copyBackCe9Func:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r0-r2,lr}

	ldr r0, =0x02378000 @ src
	ldr r1, =0x023F0000 @ dst
	mov r2, #0x6000 @ len

	ADD		R12, R1, R2

copyLoop:
	CMP		R1, R12
	LDMLTIA	R0!, {R2}
	STMLTIA	R1!, {R2}
	BLT		copyLoop

	ldr r0, =0x023D8000 @ src, Check if FAT table exists in main RAM
	ldr r1, [r0]
	cmp r1, #0
	beq copyBackCe9Func_return
	ldr r1, =0x023E8000 @ dst
	mov r2, #0x8000 @ len

	ADD		R12, R1, R2

copyLoop2:
	CMP		R1, R12
	LDMLTIA	R0!, {R2}
	STMLTIA	R1!, {R2}
	BLT		copyLoop2

copyBackCe9Func_return:
	ldmfd   sp!, {r0-r2,pc}
.pool
@---------------------------------------------------------------------------------
