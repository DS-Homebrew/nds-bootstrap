@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global card_engine_start
.global card_engine_start_sync
.global card_engine_end
.global cardStruct0
.global cacheStruct
.global patches_offset
.global sdk_version
.global fileCluster
.global saveCluster
.global ROMinRAM
.global romSize
.global dsiMode
.global enableExceptionHandler
.global consoleModel
.global asyncPrefetch

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32


patches_offset:
	.word	patches
thumbPatches_offset:
	.word	thumbPatches
intr_fifo_orig_return:
	.word	0x00000000
sdk_version:
	.word	0x00000000
fileCluster:
	.word	0x00000000
cardStruct0:
	.word	0x00000000
cacheStruct:
	.word	0x00000000
ROMinRAM:
	.word	0x00000000
romSize:
	.word	0x00000000
dsiMode:
	.word	0x00000000
enableExceptionHandler:
	.word	0x00000000
consoleModel:
	.word	0x00000000
asyncPrefetch:
	.word	0x00000000

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

.global fastCopy32
.type	fastCopy32 STT_FUNC
@ r0 : src, r1 : dst, r2 : len
fastCopy32:
	stmfd   sp!, {r3-r11,lr}
	@ copy r2 bytes
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
.word	card_id_arm9
.word	card_dma_arm9
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   readCachedRef
.word   0x0
.global needFlushDCCache
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
.word   0x0

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r6,lr}

	ldr		r6, =cardRead
	bl		_blx_r3_stub_card_read

	ldmfd   sp!, {r4-r6,lr}
	bx      lr
_blx_r3_stub_card_read:
	bx	r6
.pool
cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
readCachedRef:
.word    0x00000000  
cacheRef:
.word    0x00000000  
	.thumb
@---------------------------------------------------------------------------------
thumb_card_read_arm9:
@---------------------------------------------------------------------------------
	push	{r3-r7, lr}

	ldr		r6, =cardRead

	bl		_blx_r3_stub_thumb_card_read	

	pop	{r3-r7, pc}
	bx      lr
_blx_r3_stub_thumb_card_read:
	bx	r6	
.pool	
	.arm
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_id_arm9:
@---------------------------------------------------------------------------------
	mov r0, #1
	bx      lr
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
	bx      lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull:
@---------------------------------------------------------------------------------
	bx      lr
	.thumb
@---------------------------------------------------------------------------------
thumb_card_id_arm9:
@---------------------------------------------------------------------------------
	mov r0, #1
	bx      lr		
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

.global DC_FlushRange
.type	DC_FlushRange STT_FUNC
DC_FlushRange:
	MOV             R12, #0
	ADD             R1, R1, R0
	BIC             R0, R0, #0x1F
loop_flush_range :
	MCR             p15, 0, R12,c7,c10, 4
	MCR             p15, 0, R0,c7,c14, 1
	ADD             R0, R0, #0x20
	CMP             R0, R1
	BLT             loop_flush_range
	BX              LR

.global tryLockMutex
.type	tryLockMutex STT_FUNC
tryLockMutex:
adr     r1, mutex    
mov r2, #1
mutex_loop:
	swp r0,r2, [r1]
	cmp r0, #1
	beq mutex_fail

mutex_success:
	mov r2, #1
	str r2, [r1]
	mov r0, #1
	b mutex_exit

mutex_fail:
	mov r0, #0

mutex_exit:
	bx  lr


.global unLockMutex
.type	unLockMutex STT_FUNC
unLockMutex:
	adr r1, mutex    
	mov r2, #0
	str r2, [r1]
	bx  lr

mutex:
.word    0x00000000
