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

ce9:
	.word	ce9
patches_offset:
	.word	patches
thumbPatches_offset:
	.word	thumbPatches
intr_ipc_orig_return:
	.word	0x00000000
fileCluster:
	.word	0x00000000
saveCluster:
	.word	0x00000000
romFatTableCache:
	.word	0x00000000
savFatTableCache:
	.word	0x00000000
ramDumpCluster:
	.word	0x00000000
srParamsCluster:
	.word	0x00000000
pageFileCluster:
	.word	0x00000000
cardStruct0:
	.word	0x00000000
valueBits:
	.word	0x00000000
overlaysSize:
	.word	0x00000000
ioverlaysSize:
	.word	0x00000000
irqTable:
	.word	0x00000000
romLocation:
	.word	0x00000000
rumbleFrames:
	.word	30

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

ipcSyncHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_ipc
	ldr 	r0,	intr_ipc_orig_return
	bx  	r0

code_handler_start_ipc:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerIPC
	bl	_blx_r3_stub		@ jump to myIrqHandler
	
	@ exit after return
	b	arm9exit

@---------------------------------------------------------------------------------
_blx_r3_stub:
@---------------------------------------------------------------------------------
	bx	r3

@---------------------------------------------------------------------------------

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

arm9exit:
	pop   	{r0-r12} 
	pop  	{lr}
	bx  lr

.pool

patches:
.word	card_read_arm9
.word	card_irq_enable
.word	card_pull_out_arm9
.word	card_id_arm9
.word	card_dma_arm9
.word   nand_read_arm9
.word   nand_write_arm9
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   terminateForPullOutRef
.word   reset_arm9
.word   rumble_arm9
needFlushDCCache:
.word   0x0
.word   pdash_read
.word	ipcSyncHandler
thumbPatches:
.word	thumb_card_read_arm9
.word	thumb_card_irq_enable
.word	thumb_card_pull_out_arm9
.word	thumb_card_id_arm9
.word	thumb_card_dma_arm9
.word   thumb_nand_read_arm9
.word   thumb_nand_write_arm9
.word	cardStructArm9
.word   thumb_card_pull
.word   cacheFlushRef
.word   terminateForPullOutRef
.word   thumb_reset_arm9

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r11,lr}

	ldr		r6, =cardRead
    
	bl		_blx_r6_stub_card_read

	ldmfd   sp!, {r4-r11,pc}
_blx_r6_stub_card_read:
	bx	r6
.pool
cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
terminateForPullOutRef:
.word    0x00000000  
cacheRef:
.word    0x00000000  
	.thumb
@---------------------------------------------------------------------------------
thumb_card_read_arm9:
@---------------------------------------------------------------------------------
	push	{r3-r7, lr}

	ldr		r6, =cardRead

	bl		_blx_r6_stub_thumb_card_read	

	pop	{r3-r7, pc}
_blx_r6_stub_thumb_card_read:
	bx	r6	
.pool
.align	4
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
card_pull:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_card_id_arm9:
@---------------------------------------------------------------------------------
	ldr r0, cardIdDataT
	bx      lr
.align	4
cardIdDataT:
.word  0xC2FF01C0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_dma_arm9:
@---------------------------------------------------------------------------------
	mov r0, #0
	bx      lr		
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_pull_out_arm9:
thumb_card_pull:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
nand_read_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r3-r9,lr}

	ldr		r6, =nandRead

	bl		_blx_r6_stub_nand_read	

	ldmfd   sp!, {r3-r9,pc}
_blx_r6_stub_nand_read:
	bx	r6	
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
nand_write_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r3-r9,lr}

	ldr		r6, =nandWrite

	bl		_blx_r6_stub_nand_write

	ldmfd   sp!, {r3-r9,pc}
_blx_r6_stub_nand_write:
	bx	r6	
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_nand_read_arm9:
@---------------------------------------------------------------------------------
    push	{r1-r7, lr}

	ldr		r6, =nandRead

	bl		_blx_r6_stub_thumb_nand_read	

	pop	{r1-r7, pc}
_blx_r6_stub_thumb_nand_read:
	bx	r6	
.pool
.align	4
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_nand_write_arm9:
@---------------------------------------------------------------------------------
    push	{r1-r7, lr}

	ldr		r6, =nandWrite

	bl		_blx_r6_stub_thumb_nand_write

	pop	{r1-r7, pc}
_blx_r6_stub_thumb_nand_write:
	bx	r6	
.pool
.align	4
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
card_irq_enable:
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

	.thumb
@---------------------------------------------------------------------------------
thumb_card_irq_enable:
@---------------------------------------------------------------------------------
    push	{r1-r7, lr}

	ldr	r3, =myIrqEnable

	bl	thumb_blx_r3_stub2
	pop	{r1-r7, pc}
	bx  lr
thumb_blx_r3_stub2:
	bx	r3
.pool
.align	4
@---------------------------------------------------------------------------------

	.arm
pdash_read:
    push	{r1-r11, lr}
    @mov     r0, r4 @DST
    @mov     r1, r5 @SRC
    @mov     r2, r6 @LEN
    @mov     r3, r10 @cardStruct
    add     r0, r0, #0x2C    
    ldr		r6, =cardReadPDash
	bl		_blx_r6_stub_pdash   
    pop	    {r1-r11, pc}
    bx      lr
_blx_r6_stub_pdash:
	bx	r6	
.pool

	.thumb   
@---------------------------------------------------------------------------------
thumb_reset_arm9:
@---------------------------------------------------------------------------------
    push	{r1-r7, lr}

	ldr	r3, =reset

	bl	thumb_blx_r3_stub3
	pop	{r1-r7, pc}
thumb_blx_r3_stub3:
	bx	r3
.pool
@---------------------------------------------------------------------------------


	.arm
@---------------------------------------------------------------------------------
reset_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r1-r11,lr}

	ldr		r6, =reset

	bl		_blx_r6_stub_reset

	ldmfd   sp!, {r1-r11,pc}
_blx_r6_stub_reset:
	bx	r6	
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
rumble_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r1-r11,lr}

	ldr		r6, =rumble
	bl		_blx_r6_stub_rumble
	nop

	ldmfd   sp!, {r1-r11,pc}
_blx_r6_stub_rumble:
	bx	r6	
.pool
@---------------------------------------------------------------------------------

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
