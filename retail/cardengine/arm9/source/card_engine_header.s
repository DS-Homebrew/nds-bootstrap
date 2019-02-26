@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global ce9
	.align	4
	.arm

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

ce9 :
	.word	ce9
patches_offset:
	.word	patches
thumbPatches_offset:
	.word	thumbPatches
intr_fifo_orig_return:
	.word	0x00000000
moduleParams:
	.word	0x00000000
fileCluster:
	.word	0x00000000
cardStruct0:
	.word	0x00000000
cacheStruct:
	.word	0x00000000
ROMinRAM:
	.word	0x00000000
dsiMode:
	.word	0x00000000
enableExceptionHandler:
	.word	0x00000000
consoleModel:
	.word	0x00000000

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

patches:
.word	card_read_arm9
.word	card_pull_out_arm9
.word	0x0
.word	card_id_arm9
.word	card_dma_arm9
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   readCachedRef
.word   terminateForPullOutRef
needFlushDCCache:
.word   0x0
thumbPatches:
.word	thumb_card_read_arm9
.word	thumb_card_pull_out_arm9
.word	0x0
.word	thumb_card_id_arm9
.word	thumb_card_dma_arm9
.word	cardStructArm9
.word   thumb_card_pull
.word   cacheFlushRef
.word   readCachedRef
.word   terminateForPullOutRef

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r11,lr}

	ldr		r6, cardReadRef1
    ldr     r7, ce9location1
    add     r6, r6, r7
    
	bl		_blx_r6_stub_card_read

	ldmfd   sp!, {r4-r11,pc}
	bx      lr
_blx_r6_stub_card_read:
	bx	r6
.pool
cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
readCachedRef:
.word    0x00000000  
terminateForPullOutRef:
.word    0x00000000  
cacheRef:
.word    0x00000000
ce9location1:
.word   ce9
cardReadRef1:
.word   cardRead-ce9 	  
	.thumb
@---------------------------------------------------------------------------------
thumb_card_read_arm9:
@---------------------------------------------------------------------------------
	push	{r3-r7, lr}

	ldr		r6, cardReadRef2
    ldr     r7, ce9location2
    add     r6, r6, r7

	bl		_blx_r6_stub_thumb_card_read	

	pop	{r3-r7, pc}
	bx      lr
_blx_r6_stub_thumb_card_read:
	bx	r6	
.pool
.align	4
ce9location2:
.word   ce9
cardReadRef2:
.word   cardRead-ce9 	
	.arm
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_id_arm9:
@---------------------------------------------------------------------------------
	ldr r0, cardIdData 
	bx      lr
cardIdData:
.word  0xC2FF01C0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_dma_arm9:
@---------------------------------------------------------------------------------
	mov r0, #0
	bx      lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull_out_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {lr}
	sub     sp, sp, #4
	ldr		r6, cardPullOutRef
    ldr     r7, ce9location3
    add     r6, r6, r7
    
	bl		_blx_r6_stub_card_pull_out

	add     sp, sp, #4
	ldmfd   sp!, {lr}
	bx      lr
_blx_r6_stub_card_pull_out:
	bx	r6
.pool
.align	4
ce9location3:
.word   ce9
cardPullOutRef:
.word   cardPullOut-ce9 	
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull:
@---------------------------------------------------------------------------------
	bx      lr
	.thumb
@---------------------------------------------------------------------------------
thumb_card_id_arm9:
@---------------------------------------------------------------------------------
	ldr r0, cardIdDataT 
	bx      lr
cardIdDataT:
.word  1	
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_dma_arm9:
@---------------------------------------------------------------------------------
	mov r0, #0
	bx      lr		
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_pull_out_arm9:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_pull:
@---------------------------------------------------------------------------------
	bx      lr
	.arm
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

card_engine_end:
