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
mainScreen:
	.word	0x00000000
consoleModel:
	.word	0x00000000
irqTable:
	.word	0x00000000

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

.thumb
ndsCodeStart:
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, r1
	mov r9, r1
	mov r10, r1
	mov r11, r1
	bx r0

.balign	4
patches:
.word   0x0
.word	card_irq_enable
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
.word   0x0
.word   0x0
.word   0x0
.word   reset_arm9
needFlushDCCache:
.word   0x0
.word   0x0
.word   0x0
.word   vblankHandler
.word   ipcSyncHandler
thumbPatches:
.word   0x0
.word	thumb_card_irq_enable
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
thumbCardEndReadDmaRef:
.word   0x0
thumbSleepRef:
.word   0x0
.word   thumb_reset_arm9

	.arm
@---------------------------------------------------------------------------------
card_irq_enable:
@---------------------------------------------------------------------------------
	ldr		pc, =myIrqEnable
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_card_irq_enable:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr		r6, =myIrqEnable
	blx		r6

	pop	{r6, pc}
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_reset_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr		r6, =reset
	blx		r6

	pop	{r6, pc}
.pool
.align	4
@---------------------------------------------------------------------------------


	.arm
vblankHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_vblank
	ldr 	pc,	intr_vblank_orig_return

ipcSyncHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_ipc
	ldr 	pc,	intr_ipc_orig_return

code_handler_start_vblank:
	push	{r0-r12}
	bl	myIrqHandlerVBlank
	pop   	{r0-r12,pc}

code_handler_start_ipc:
	push	{r0-r12}
	bl	myIrqHandlerIPC
	pop   	{r0-r12,pc}

.pool

@---------------------------------------------------------------------------------
reset_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r2-r11,lr}

	ldr		r6, =reset
	blx	r6

	ldmfd   sp!, {r2-r11,pc}
.pool
@---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
.global  getDtcmBase
.type	 getDtcmBase STT_FUNC
/*---------------------------------------------------------------------------------
	getDtcmBase
---------------------------------------------------------------------------------*/
getDtcmBase:
	mrc	p15, 0, r0, c9, c1, 0
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
// DC_FlushAll:
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
// DC_WaitWriteBufferEmpty:
//---------------------------------------------------------------------------------
	MCR     p15, 0, R7,c7,c10, 4

	@restore interrupt
	str r11, [r8]

	ldmfd   sp!, {r0-r11,lr}
	bx      lr
	.pool

card_engine_end:
