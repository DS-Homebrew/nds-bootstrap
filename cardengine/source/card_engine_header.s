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
.global saveCluster

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

.global fastCopy32
.type	fastCopy32 STT_FUNC
@ r0 : src, r1 : dst, r2 : len
fastCopy32:
    stmfd   sp!, {r3-r11,lr}
	@ copy 512 bytes
	mov     r10, r0	
	mov     r9, r1	
	mov     r8, r2	
loop_fastCopy32:
	ldmia   r10!, {r0-r7}
	stmia   r9!,  {r0-r7}
	subs    r8, r8, #32  @ 4*8 bytes
	bgt     loop_fastCopy32
	ldmfd   sp!, {r3-r11,lr}
    bx      lr

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
.word   arm7Functions

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r0-r11,lr}
	str 	r0, cacheRef
	
begin:	
	@ registers used r0,r1,r2,r3,r5,r8,r11
    ldr     r3,=0x4000100     @IPC_SYNC & command value
    ldr     r8,=0x027FFB08    @shared area command			
    ldr     r4, cardStructArm9	
    ldr     r5, [R4]      @SRC
	ldr     r1, [R4,#0x8] @LEN
	ldr     r0, [R4,#0x4] @DST
	mov     r2, #0x2400	
	
	@page computation
	mov     r9, #0x200
	rsb     r10, r9, #0
	and     r11, r5, r10
	
	@ check for cmd2
	cmp     r11, r5
	bne     cmd1	
	cmp     r1, #1024
	blt     cmd1	
	sub     r7, r8, #(0x027FFB08 - 0x026FFB08) @below dtcm
	cmp     r0, r7
	bgt     cmd1
	sub     r7, r8, #(0x027FFB08 - 0x019FFB08) @above itcm
	cmp     r0, r7
	blt     cmd1
	ands    r10, r0, #3
	bne     cmd1
	
cmd2:
	sub r7, r8, #(0x027FFB08 - 0x025FFB08) @cmd2 marker
	@r0 dst, r1 len
	ldr r9, cacheFlushRef
	blx r9  			@ cache flush code
	b 	send_cmd

cmd1:	
	mov     R1, #0x200
	mov     r5, r11       @ current page	
    sub     r7, r8, #(0x027FFB08 - 0x027ff800) @cmd1 marker

send_cmd:
	@dst, len, src, marker
    stmia r8, {r0,r1,r5,r7}
    
    @sendIPCSync
    strh    r2, [r3,#0x80]

loop_wait:
    ldr r9, [r8,#12]
    cmp r9,#0
    bne loop_wait	

	@ check for cmd2
	cmp     r1, #0x200
	bne     exitfunc
	
	ldr 	r9, cacheRef
	add     r9,r9,#0x20	@ cache buffer
	mov     r10,r7	

	@ copy 512 bytes
	mov     r8, #512	
loop_copy:
	ldmia   r10!, {r0-r7}
	stmia   r9!,  {r0-r7}
	subs    r8, r8, #32  @ 4*8 bytes
	bgt     loop_copy

	ldr 	r0, cacheRef	
	str     r11, [r0, #8]	@ cache page
	
	ldr r9, readCachedRef
	blx r9  		
	
	cmp r0,#0	
	bne begin

exitfunc:	
    ldmfd   sp!, {r0-r11,lr}
    bx      lr

cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
readCachedRef:
.word    0x00000000  
cacheRef:
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
	
arm7Functions :
.word    eepromProtect 
.word    eepromPageErase 
.word    eepromPageVerify  
.word    eepromPageWrite  
.word    eepromPageProg  
.word    eepromRead  
.word    cardRead 
.word    cardId 
saveCluster:
.word    0x00000000 