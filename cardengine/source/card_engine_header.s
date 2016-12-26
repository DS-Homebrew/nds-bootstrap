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
.global sdk_version
.global fileCluster

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32


patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
intr_fifo_orig_return:
	.word	0x00000000
sdk_version:
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
	
	@ exit after return
	b	exit
	
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
.word	card_irq_enable_arm7
.word	vblankHandler
.word	fifoHandler
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   readCachedRef

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r0-r11,lr}
	
begin:
	
	@ registers used r0,r1,r2,r3,r5,r6,r7,r8
    ldr     r3,=0x4000100     @IPC_SYNC & command value
    ldr     r8,=0x027FFB08    @shared area command
	str 	r0,[r8,#16]
    ldr     r4, cardStructArm9
    ldr     r5, [R4]      @SRC
	ldr     r0, [R4,#0x4] @DST
    ldr     r1, [R4,#0x8] @LEN
	mov     r2, #0x2400
	
	@sub r7, r8, #(0x027FFB08 - 0x026FFB08) @below dtcm
	@cmp r0, r7
	@bgt check_partial
	@b check_partial @deactivate cmd2 optimization
cmd2:
	@sub r7, r8, #(0x027FFB08 - 0x025FFB08) @cmd2 marker
	@r0 dst, r1 len
	@ldr r9, cacheFlushRef
	@blx r9  			@ cache flush code
	@b partial_cmd2

check_partial:
	
	mov r0, r5
	
	MOV     R1, #0x200
	RSB     R10, R1, #0
	AND     R11, R5, R10
	mov     r5, r11       @ current page	
	


	
partial:
    sub r7, r8, #(0x027FFB08 - 0x027ff800) @shared area data
partial_cmd2:
	@dst, len, src, marker
    stmia r8, {r0,r1,r5,r7}
    
    @sendIPCSync
    strh    r2, [r3,#0x80]

partial_loop_wait:
    ldr r9, [r8,#12]
    cmp r9,#0
    bne partial_loop_wait	

partial_loop_copy:

	@ldr     r8,=0x027FFB08    @shared area command
	
	ldr 	r9,[r8,#16]	
	MOV     R10, #0
	str     r10, [r9, #8]	@ clear cache page
	add     r9,r9,#0x20	@ cache buffer
	mov     r10,r7
	

	@ copy 512 bytes
	MOV     R12, #512
loop_copy:
	ldmia r10!, {r0-r7}
	stmia r9!, {r0-r7}
	subs r12, r12, #32  @ 4*8 bytes
	bgt loop_copy

	ldr 	r0,[r8,#16]		
	str r11, [r0, #8]	@ cache page
	
	ldr r9, readCachedRef
	blx r9  		
	
	@ldr     r4, =0x05000000
	@mov     r6, #31
	@str     r6, [r4]
	
	cmp r0,#0	
	beq exitfunc
	
	ldr 	r0,[r8,#16]	
	b begin

exitfunc:
    ldmfd   sp!, {r0-r11,lr}
    bx      lr

cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
readCachedRef:
.word    0x00000000  
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull_out_arm9:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------
	
@---------------------------------------------------------------------------------
card_irq_enable_arm7:
@---------------------------------------------------------------------------------
    push    {lr}
	push	{r1-r12}
	ldr	r3, =myIrqEnable
	bl	_blx_r3_stub2
	pop   	{r1-r12} 
	pop  	{lr}
	bx  lr
_blx_r3_stub2:
	bx	r3		
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull:
@---------------------------------------------------------------------------------
	bx lr
cacheFlush:
    stmfd   sp!, {r0-r11,lr}
	
	@disable interrupt	
	ldr r8,= 0x4000208
	ldr r11,[r8]
	mov r7, #0
	str r7, [r8]		
	
//---------------------------------------------------------------------------------
IC_InvalidateAll:
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
	mcr	p15, 0, r7, c7, c5, 0
		
//---------------------------------------------------------------------------------
DC_FlushAll:
/*---------------------------------------------------------------------------------
	Clean and invalidate a range
---------------------------------------------------------------------------------*/
	mov	r1, #0
outer_loop:
	mov	r0, #0
inner_loop:
	orr	r2, r1, r0			@ generate segment and line address
	mcr p15, 0, r7, c7, c10, 4
	mcr	p15, 0, r2, c7, c14, 2		@ clean and flush the line
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, #DCACHE_SIZE/4
	bne	inner_loop
	add	r1, r1, #0x40000000
	cmp	r1, #0
	bne	outer_loop
	
//---------------------------------------------------------------------------------	
DC_WaitWriteBufferEmpty:
//---------------------------------------------------------------------------------               
    MCR     p15, 0, R7,c7,c10, 4
	
	@restore interrupt
	str r11, [r8]
	
    ldmfd   sp!, {r0-r11,lr}
    bx      lr
	.pool