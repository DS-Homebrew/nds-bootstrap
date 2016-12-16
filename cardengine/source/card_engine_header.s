@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global card_engine_start
.global card_engine_start_sync
.global card_engine_end
.global cardStruct
.global cacheStruct
.global patches_offset
.global commandAddr
.global fileCluster


patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
intr_fifo_orig_return:
	.word	0x00000000
commandAddr:
	.word	0x00000000
fileCluster:
	.word	0x00000000
cardStruct:
	.word	0x00000000
cacheStruct:
	.word	0x00000000
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

vblankHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_vblank
	ldr 	r0,	intr_vblank_orig_return
	bx  	r0

fifoHandler:	
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_fifo
	ldr 	r0,	intr_fifo_orig_return
	bx  	r0
	
code_handler_start_vblank:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerVBlank
	bl	_blx_r3_stub		@ jump to myIrqHandler
	
code_handler_start_fifo:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerFIFO
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
.word	card_read_arm9
.word	card_pull_out_arm9
.word	vblankHandler
.word	fifoHandler
.word	card_send_arm9
card_read_arm9:
	stmfd   sp!, {r4-r11,lr}
	sub     sp, sp, #4
	
	@ send the command via the wordcommand area. not yet working
	ldr		r10, =0x24
	add		r10, r10, r0	
	ldr 	r5, =0x027FEE04
	@ str     r5, [r10] @ wordcommand area, 2096200 + 24 = 2096224
	@ str     r5, [r6] 
	
	@ send the command via the debug area (may cause conflict)
	ldr     r5, =0x027FEE04
    ldr     r6, =0x02100000
    str     r5, [r6]	
	@ str     r10, [r6,#4]

	@ turn the screen blue
    ldr     r5, =0x027FEE04
    ldr     r6, =0x05000400
    str     r5, [r6] 
	
	@ call card send for fifo activation	
	@ ldr     r6, card_send_arm9
	@ ldr     r0, card_send_arm9
	@ ldr     r1, =0x0002
	@ bx  	r6
	
	add     sp, sp, #4
	ldmfd   sp!, {r4-r11,lr}
	bx      lr
card_send_arm9:
.word	0x00000000	
.pool
card_pull_out_arm9:
	bx      lr
