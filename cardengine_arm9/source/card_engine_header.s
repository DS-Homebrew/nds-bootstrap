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
	bl	_blx_r3_stub		@ jump to myIrqHandler
	
	@ exit after return
	b	exit
	
code_handler_start_fifo:
	push	{r0-r12} 
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

.global readCachedRef
patches:
.word	card_read_arm9
.word	card_pull_out_arm9
.word	0x0
.word	vblankHandler
.word	fifoHandler
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   readCachedRef
.word   0x0

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r4-r11,lr}
		
	@ get back the WRAM B & C to arm9
	ldr     r4,=0x4004044     
    ldr     r1,=0x8084888C	
	sub     r2, r4, #(0x4004044 - 0x4004048)
	ldr     r3,=0x9094989C
	str     r1,[r4]
	str     r3,[r2]
	sub     r4, r2, #(0x4004048 - 0x400404C)
	sub     r2, r4, #(0x400404C - 0x4004050)
	str     r1,[r4]
	str     r3,[r2]
	
	ldr		r3, =cardRead
	ldr     r1, =0xE92D4FF0
wait_for_wram_card_read:
	ldr     r2, [r3]
	cmp     r1, r2
	bne     wait_for_wram_card_read
	
	push    {lr}
	bl		_blx_r3_stub_card_read	
    pop  	{lr}

    ldmfd   sp!, {r4-r11,lr}
    bx      lr
_blx_r3_stub_card_read:
	bx	r3	
.pool	
cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
readCachedRef:
.word    0x00000000  
cacheRef:
.word    0x00000000  
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_id_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r4-r11,lr}

	ldr		r3, =cardId	
	push    {lr}
	bl		_blx_r3_stub_card_id
	pop  	{lr}
    
    ldmfd   sp!, {r4-r11,lr}
    bx      lr
_blx_r3_stub_card_id:
	bx	r3	
.pool	
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull_out_arm9:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r11,lr}
		
	@ get back the WRAM B & C to arm9
	ldr     r4,=0x4004044     
    ldr     r1,=0x8084888C	
	sub     r2, r4, #(0x4004044 - 0x4004048)
	ldr     r3,=0x9094989C
	str     r1,[r4]
	str     r3,[r2]
	sub     r4, r2, #(0x4004048 - 0x400404C)
	sub     r2, r4, #(0x400404C - 0x4004050)
	str     r1,[r4]
	str     r3,[r2]
	
	ldr		r3, =cardRead
	ldr     r1, =0xE92D4FF0
wait_for_wram_card_pull:
	ldr     r2, [r3]
	cmp     r1, r2
	bne     wait_for_wram_card_pull
	
	@ push    {lr}
	@ bl		_blx_r3_stub_card_pull	
    @ pop  	{lr}

    ldmfd   sp!, {r4-r11,lr}
    bx      lr
_blx_r3_stub_card_pull:
	@bx	r3		
.pool

.global cacheFlush
.type	cacheFlush STT_FUNC
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
	