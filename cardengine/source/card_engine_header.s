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
.global sdmmc_engine_size
.global commandAddr
.global fileCluster


card_engine_size:
	.word	card_engine_end - card_engine_start
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
card_engine_irqHandler:
	ldr	r1, =intr_vcount_orig_return	@ user IRQ handler address
	cmp	r1, #0
	bne	call_handler
	bx  lr

card_engine_irqEnable:
    push    {lr}
	push	{r1-r12}
	ldr	r3, =myIrqEnable
	bl	_blx_r3_stub		@ jump to myIrqEnable	
	pop   	{r1-r12} 
	pop  	{lr}
	bx  lr

call_handler:
	push    {lr}
	adr 	lr, code_handler_start
	ldr		r2, [r1]
	ldr		r0, [r2]
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
@ my patch
@---------------------------------------------------------------------------------
myPatch:
	ldr    r1, =card_engine_start        @ my custom handler
	str    r2, [r1, #-8]		@ irqhandler
	str    pc, [r1, #-4]		@ irqsig
	b      got_handler
.pool	
got_handler:
	str	r0, [r12, #4]	@ IF Clear
@---------------------------------------------------------------------------------

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

exit:	
	pop   	{r0-r12} 
	pop  	{lr}
	bx  lr

.pool

card_engine_end:


