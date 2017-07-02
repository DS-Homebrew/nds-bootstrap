@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global sdmmc_engine_start
.global sdmmc_engine_start_sync
.global sdmmc_engine_end
.global irqHandler
.global irqSig
.global sdmmc_engine_size
.global commandAddr


sdmmc_engine_size:
	.word	sdmmc_engine_end - sdmmc_engine_start
commandAddr:
	.word	0x00000000
irqHandler:
	.word	0x00000000
irqSig:
	.word	0x00000000

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

sdmmc_engine_start:
sdmmc_engine_irqHandler:
	ldr	r1, =irqHandler			@ user IRQ handler address
	cmp	r1, #0
	bne	call_handler
	bx  lr

sdmmc_engine_irqEnable:
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
	ldr    r1, =sdmmc_engine_start        @ my custom handler
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

sdmmc_engine_end:


