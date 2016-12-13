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
intr_vblank_orig_return:
	.word	0x00000000
intr_fifo_orig_return:
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

vblankHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start
	ldr 	r0,	intr_vblank_orig_return
	bx  	r0

fifoHandler:	
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start
	ldr 	r0,	intr_fifo_orig_return
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
.word	card_read_arm9
.word	card_pull_out_arm9
.word	vblankHandler
.word	fifoHandler
card_read_arm9:
	stmfd   sp!, {r4-r11,lr}
	sub     sp, sp, #4
	ldr		r10, =0x24
	add		r10, r10, r0	
	ldr 	r5, =0x027FEE04
	ldr		r6, =0x02496320 @ debug area,
	str     r5, [r10] @ wordcommand area, 2096200 + 24 = 2096224
	str     r5, [r6] 
	str     r10, [r6,#4]
	
	@REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE;
	ldr     r4, =0x04000000
	ldr     r6, =0x10100
	str     r6, [r4]
	
	@VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
	ldr     r4, =0x04000240
	mov     r6, #0x81
	str     r6, [r4]
	
	@BG_PALETTE[0] = RGB15(31, 0, 0)
	ldr     r4, =0x05000000
	mov     r6, #31
	str     r6, [r4]
	
	0x0002
	
	add     sp, sp, #4
	ldmfd   sp!, {r4-r11,lr}
	bx      lr	
.pool
card_pull_out_arm9:
	bx      lr
