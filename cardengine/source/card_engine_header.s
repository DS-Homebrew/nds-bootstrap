@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global card_engine_start
.global card_engine_start_sync
.global card_engine_end
.global staticCommand
.global staticCache
.global patches_offset
.global commandAddr
.global fileCluster


patches_offset:
	.word	patches
intr_vcount_orig_return:
	.word	0x00000000
intr_synch_orig_return:
	.word	0x00000000
commandAddr:
	.word	0x00000000
fileCluster:
	.word	0x00000000
staticCommand:
	.word	0x00000000
staticCache:
	.word	0x00000000
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start
	ldr 	r0,	intr_vcount_orig_return
	bx  	r0
	
code_handler_start:
	push	{r0-r12} 
	ldr	r3, =myIrqHandler
	bl	_blx_r3_stub		@ jump to myIrqHandler
  
  @ exit after return
	b	exit
	
@---------------------------------------------------------------------------------
_blx_r3_stub:
@---------------------------------------------------------------------------------
	bx	r3	
	
@---------------------------------------------------------------------------------

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

exit:	
	pop   	{r0-r12} 
	pop  	{lr}
	bx  lr

.pool

card_engine_end:

patches:
card_read_arm9:
	stmfd   sp!, {r4-r11,lr}
	sub     sp, sp, #4
	ldr		r10, =0x24
	add		r10, r10, r0	
	ldr		r5, =0x027FEE04
	ldr		r6, =0x02096320 @ debug area,
	str     r5, [r10] @ wordcommand area, 2096200 + 24 = 2096224
	str     r5, [r6] 
	str     r10, [r6,#4]
	add     sp, sp, #4
	ldmfd   sp!, {r4-r11,lr}
	bx      lr
	
.pool

