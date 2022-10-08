@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global ce9
	.global ndsCodeStart
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
intr_vblank_orig_return:
	.word	0x00000000
intr_ipc_orig_return:
	.word	0x00000000
fileCluster:
	.word	0x00000000
saveCluster:
	.word	0x00000000
saveSize:
	.word	0x00000000
cardStruct0:
	.word	0x00000000
cacheStruct:
	.word	0x00000000
valueBits:
	.word	0x00000000
overlaysSize:
	.word	0x00000000
consoleModel:
	.word	0x00000000
irqTable:
	.word	0x00000000

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

ndsCodeStart:
	.thumb
	bx	pc
.align	4
	.arm
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	mov r10, #0
	mov r11, #0

	bx	r0

patches:
.word	card_read_arm9
.word	card_irq_enable
.word	card_pull_out_arm9
.word	card_id_arm9
.word	card_dma_arm9
.word	card_set_dma_arm9
.word   nand_read_arm9
.word   nand_write_arm9
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word	cardStructArm9
.word   waitSysCycles
.word	cart_read
.word   cacheFlushRef
.word   0x0 @cardEndReadDmaRef
.word   0x0 @sleepRef
.word	swi02
.word   reset_arm9
needFlushDCCache:
.word   0x0
.word   pdash_read
.word   vblankHandler
.word   ipcSyncHandler
thumbPatches:
.word	thumb_card_read_arm9
.word	thumb_card_irq_enable
.word	thumb_card_pull_out_arm9
.word	thumb_card_id_arm9
.word	thumb_card_dma_arm9
.word	thumb_card_set_dma_arm9
.word   thumb_nand_read_arm9
.word   thumb_nand_write_arm9
.word	cardStructArm9
.word   thumb_card_pull
.word	thumb_cart_read
.word   cacheFlushRef
thumbCardEndReadDmaRef:
.word   0x0 @cardEndReadDmaRef
thumbSleepRef:
.word   0x0 @sleepRef
.word   thumb_reset_arm9

	.thumb
@---------------------------------------------------------------------------------
swi02:
@---------------------------------------------------------------------------------
	swi	#0x02
	bx	lr
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r11,lr}

	ldr		r6, cardReadRef1
    ldr     r7, ce9location1
    add     r6, r6, r7
    
	bl		_blx_r6_stub_card_read

	ldmfd   sp!, {r4-r11,pc}
_blx_r6_stub_card_read:
	bx	r6
.pool
cardStructArm9:
.word    0x00000000 
cacheFlushRef:
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
_blx_r6_stub_thumb_card_read:
	bx	r6	
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
    stmfd   sp!, {r3-r9,lr}

	ldr		r6, cardReadRef4
    ldr     r7, ce9location4
    add     r6, r6, r7

	bl		_blx_r6_stub_card_read_dma	
    

	ldmfd   sp!, {r3-r9,pc}
_blx_r6_stub_card_read_dma:
	bx	r6	
ce9location4:
.word   ce9
cardReadRef4:
.word   cardReadDma-ce9 
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_set_dma_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r1-r11,lr}

	ldr		r6, cardReadRef14
    ldr     r7, ce9location14
    add     r6, r6, r7

	bl		_blx_r6_stub_card_set_dma	
    

	ldmfd   sp!, {r1-r11,pc}
_blx_r6_stub_card_set_dma:
	bx	r6	
ce9location14:
.word   ce9
cardReadRef14:
.word   cardSetDma-ce9 
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull_out_arm9:
@---------------------------------------------------------------------------------
	bx      lr
@	stmfd   sp!, {lr}
@	sub     sp, sp, #4
@	ldr		r6, cardPullOutRef
@    ldr     r7, ce9location5
@    add     r6, r6, r7
    
@	bl		_blx_r6_stub_card_pull_out

@	add     sp, sp, #4
@	ldmfd   sp!, {lr}
@	bx      lr
@_blx_r6_stub_card_pull_out:
@	bx	r6
@ce9location5:
@.word   ce9
@cardPullOutRef:
@.word   cardPullOut-ce9
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
waitCpuCycles:
@---------------------------------------------------------------------------------
	SUBS            R0, R0, #4
	BCS             waitCpuCycles
	BX              LR

@---------------------------------------------------------------------------------
waitSysCycles:
@---------------------------------------------------------------------------------
	STMFD           SP!, {R3,LR}
	MOV             R0, R0,LSL#2
	CMP             R0, #0x10
	LDMLSFD         SP!, {R3,PC}
	SUB             R0, R0, #0x10
	BL              waitCpuCycles
	LDMFD           SP!, {R3,PC}

@---------------------------------------------------------------------------------
cart_read:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r7,lr}

	ldr		r6, cardReadRefS2R
    ldr     r7, ce9locationS2R
    add     r6, r6, r7
    
	bl		_blx_r6_stub_slot2_read

	ldmfd   sp!, {r4-r7,pc}
_blx_r6_stub_slot2_read:
	bx	r6
ce9locationS2R:
.word   ce9
cardReadRefS2R:
.word   cartRead-ce9
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
    push	{r3-r7, lr}
    
	ldr		r6, cardReadRef7
    ldr     r7, ce9location7
    add     r6, r6, r7

	bl		_blx_r6_stub_thumb_card_read_dma	

    pop	{r3-r7, pc}
_blx_r6_stub_thumb_card_read_dma:
	bx	r6	
.align	4
ce9location7:
.word   ce9
cardReadRef7:
.word   cardReadDma-ce9 
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_set_dma_arm9:
@---------------------------------------------------------------------------------
    push	{r1-r7, lr}
    
	ldr		r6, cardReadRef15
    ldr     r7, ce9location15
    add     r6, r6, r7

	bl		_blx_r6_stub_thumb_card_set_dma	

    pop	{r1-r7, pc}
_blx_r6_stub_thumb_card_set_dma:
	bx	r6	
.align	4
ce9location15:
.word   ce9
cardReadRef15:
.word   cardSetDma-ce9
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
nand_read_arm9:
nand_write_arm9:
@---------------------------------------------------------------------------------
    mov r0, #0
	bx	lr
@---------------------------------------------------------------------------------

	.thumb    
@---------------------------------------------------------------------------------
thumb_nand_read_arm9:
thumb_nand_write_arm9:
@---------------------------------------------------------------------------------
    mov r0, #0
	bx	lr
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
card_irq_enable:
@---------------------------------------------------------------------------------
	push    {lr}
	push	{r1-r12}

	ldr		r3, cardReadRefIrq
    ldr     r4, ce9locationIrq
    add     r3, r3, r4

	bl	_blx_r3_stub2
	pop   	{r1-r12} 
	pop  	{lr}
	bx  lr
_blx_r3_stub2:
	bx	r3
ce9locationIrq:
.word   ce9
cardReadRefIrq:
.word   myIrqEnable-ce9 
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_card_irq_enable:
@---------------------------------------------------------------------------------
    push	{r1-r7, lr}

	ldr		r3, cardReadRefTIrq
    ldr     r4, ce9locationTIrq
    add     r3, r3, r4

	bl	thumb_blx_r3_stub2
	pop	{r1-r7, pc}
thumb_blx_r3_stub2:
	bx	r3
@.align	4
ce9locationTIrq:
.word   ce9
cardReadRefTIrq:
.word   myIrqEnable-ce9 
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_reset_arm9:
@---------------------------------------------------------------------------------
    push	{r2-r7, lr}

	ldr		r3, cardReadRefTReset
    ldr     r4, ce9locationTReset
    add     r3, r3, r4

	bl	thumb_blx_r3_stub3
	pop	{r2-r7, pc}
thumb_blx_r3_stub3:
	bx	r3
.align	4
ce9locationTReset:
.word   ce9
cardReadRefTReset:
.word   reset-ce9 
@---------------------------------------------------------------------------------

	.arm
pdash_read:
    push	{r1-r11, lr}
    @mov     r0, r4 @DST
    @mov     r1, r5 @SRC
    @mov     r2, r6 @LEN
    @mov     r3, r10 @cardStruct
    add     r0, r0, #0x2C    
    ldr		r6, cardReadRef12
    ldr     r7, ce9location12
    add     r6, r6, r7
	bl		_blx_r6_stub_pdash   
    pop	    {r1-r11, pc}
    bx      lr
_blx_r6_stub_pdash:
	bx	r6	
ce9location12:
.word   ce9
cardReadRef12:
.word   cardReadPDash-ce9 

	.thumb   
@---------------------------------------------------------------------------------
thumb_card_pull_out_arm9:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_pull:
@---------------------------------------------------------------------------------
	bx      lr

@---------------------------------------------------------------------------------
thumb_cart_read:
@---------------------------------------------------------------------------------
	push	{r4-r7, lr}

	ldr		r6, cardReadRefTS2R
    ldr     r7, ce9locationTS2R
    add     r6, r6, r7

	bl		_blx_r6_stub_thumb_slot2_read	

	POP		{r4-r7}
	POP		{r3}
	bx      r3
_blx_r6_stub_thumb_slot2_read:
	bx	r6	
.align	4
ce9locationTS2R:
.word   ce9
cardReadRefTS2R:
.word   cartRead-ce9
	.arm
    
vblankHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_vblank
	ldr 	r0,	intr_vblank_orig_return
	bx  	r0

ipcSyncHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_ipc
	ldr 	r0,	intr_ipc_orig_return
	bx  	r0
    
code_handler_start_vblank:
	push	{r0-r12} 
    ldr		r6, cardReadRef13V
    ldr     r7, ce9location13
    add     r6, r6, r7
	bl	_blx_r6_stub_start_ipc		@ jump to myIrqHandler
	
	@ exit after return
	b	arm9exit

code_handler_start_ipc:
	push	{r0-r12} 
    ldr		r6, cardReadRef13
    ldr     r7, ce9location13
    add     r6, r6, r7
	bl	_blx_r6_stub_start_ipc		@ jump to myIrqHandler
  
	@ exit after return
	b	arm9exit
_blx_r6_stub_start_ipc:
	bx	r6

arm9exit:
	pop   	{r0-r12} 
	pop  	{lr}
	bx  lr
    
ce9location13:
.word   ce9
cardReadRef13V:
.word   myIrqHandlerVBlank-ce9  
cardReadRef13:
.word   myIrqHandlerIPC-ce9  

@---------------------------------------------------------------------------------
reset_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r2-r11,lr}

	ldr		r6, cardReadRefRes
    ldr     r7, ce9locationRes
    add     r6, r6, r7

	bl		_blx_r6_stub_reset	
    

	ldmfd   sp!, {r2-r11,pc}
_blx_r6_stub_reset:
	bx	r6	
ce9locationRes:
.word   ce9
cardReadRefRes:
.word   reset-ce9 
@---------------------------------------------------------------------------------

.global callEndReadDmaThumb
.type	callEndReadDmaThumb STT_FUNC
callEndReadDmaThumb:
    push	{r1-r11, lr}
    ldr     r6, thumbCardEndReadDmaRef
    add     r6, #1
    bl		_blx_r6_stub_callEndReadDmaThumb
    pop	    {r1-r11, pc}
	bx      lr

.global callSleepThumb
.type	callSleepThumb STT_FUNC
callSleepThumb:
    push	{r1-r11, lr}
    ldr     r6, thumbSleepRef
    add     r6, #1
    bl		_blx_r6_stub_callEndReadDmaThumb
    pop	    {r1-r11, pc}
	bx      lr
_blx_r6_stub_callEndReadDmaThumb:
	bx	r6	
.pool

	.thumb
.global disableIrqMask
.type	disableIrqMask STT_FUNC
disableIrqMask:
    LDR             R7, =0x4000208
    MOV             R2, #0
    LDRH            R3, [R7]
    MVN             R1, R0
    STRH            R2, [R7]
    LDR             R0, [R7,#8]
    AND             R1, R0, R1
    STR             R1, [R7,#8]
    LDRH            R1, [R7]
    STRH            R3, [R7]
    BX              LR
.pool
    
	.arm
//---------------------------------------------------------------------------------
.global  getDtcmBase
.type	 getDtcmBase STT_FUNC
/*---------------------------------------------------------------------------------
	getDtcmBase
---------------------------------------------------------------------------------*/
getDtcmBase:
	mrc	p15, 0, r0, c9, c1, 0
	bx	lr


//---------------------------------------------------------------------------------
.global  IC_InvalidateAll
.type	 IC_InvalidateAll STT_FUNC
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
IC_InvalidateAll:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0
	bx	lr

//---------------------------------------------------------------------------------
.global IC_InvalidateRange
.type	IC_InvalidateRange STT_FUNC
/*---------------------------------------------------------------------------------
	Invalidate a range
---------------------------------------------------------------------------------*/
IC_InvalidateRange:
	add	r1, r1, r0
	bic	r0, r0, #CACHE_LINE_SIZE - 1
.invalidate:
	mcr	p15, 0, r0, c7, c5, 1
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	.invalidate
	bx	lr

@---------------------------------------------------------------------------------        
.global cacheFlush
.type	cacheFlush STT_FUNC
/*---------------------------------------------------------------------------------
	Flush dcache and icache
---------------------------------------------------------------------------------*/
cacheFlush:
	stmfd   sp!, {r0-r11,lr}

	@disable interrupt
	ldr r8,= 0x4000208
	ldr r11,[r8]
	mov r7, #0
	str r7, [r8]
//---------------------------------------------------------------------------------
// IC_InvalidateAll:
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
	mcr	p15, 0, r7, c7, c5, 0
//---------------------------------------------------------------------------------
//DC_FlushAll:
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
//DC_WaitWriteBufferEmpty:
//---------------------------------------------------------------------------------               
	MCR     p15, 0, R7,c7,c10, 4

	@restore interrupt
	str r11, [r8]

	ldmfd   sp!, {r0-r11,lr}
	bx      lr
	.pool

card_engine_end:
