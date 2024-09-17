@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global ce9
	.global ndsCodeStart
	.balign	4
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
bootNdsCluster:
	.word	0x00000000
fileCluster:
	.word	0x00000000
saveCluster:
	.word	0x00000000
saveSize:
	.word	0x00000000
romFatTableCache:
	.word	0x00000000
savFatTableCache:
	.word	0x00000000
romFatTableCompressed:
	.byte	0x00
savFatTableCompressed:
	.byte	0x00
musicFatTableCompressed:
	.byte	0x00
cardSaveCmdPos:
	.byte	0x00
patchOffsetCacheFileCluster:
	.word	0x00000000
musicFatTableCache:
	.word	0x00000000
ramDumpCluster:
	.word	0x00000000
srParamsCluster:
	.word	0x00000000
screenshotCluster:
	.word	0x00000000
apFixOverlaysCluster:
	.word	0x00000000
musicCluster:
	.word	0x00000000
musicsSize:
	.word	0x00000000
musicBuffer:
	.word	0x00000000
pageFileCluster:
	.word	0x00000000
manualCluster:
	.word	0x00000000
sharedFontCluster:
	.word	0x00000000
cardStruct0:
	.word	0x00000000
cardStruct1:
	.word	0x00000000
valueBits:
	.word	0x00000000
mainScreen:
	.word	0x00000000
irqTable:
	.word	0x00000000
s2FlashcardId:
	.hword	0x0000
	.hword	0x0000 @ align
overlaysSrc:
	.word	0x00000000
overlaysSize:
	.word	0x00000000
ioverlaysSize:
	.word	0x00000000
arm9iromOffset:
	.word	0x00000000
arm9ibinarySize:
	.word	0x00000000
romPaddingSize:
	.word	0x00000000
romLocation:
	.word	0x00000000
rumbleFrames:
	.word	30
	.word	30
rumbleForce:
	.word	1
	.word	1
prepareScreenshotPtr:
	.word prepareScreenshot
saveScreenshotPtr:
	.word saveScreenshot
prepareManualPtr:
	.word prepareManual
readManualPtr:
	.word readManual
restorePreManualPtr:
	.word restorePreManual
saveMainScreenSettingPtr:
	.word saveMainScreenSetting

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

ipcSyncHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_ipc
	ldr 	pc,	intr_ipc_orig_return

code_handler_start_ipc:
	push	{r0-r12}
	bl		myIrqHandlerIPC @ jump to myIrqHandler
	pop   	{r0-r12,pc}

.pool

/* .thumb
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
.arm */

patches:
.word	card_read_arm9
.word	card_save_arm9
.word	card_irq_enable
.word	card_pull_out_arm9
.word	card_id_arm9
.word	card_dma_arm9
.word	card_set_dma_arm9
.word   nand_read_arm9
.word   nand_write_arm9
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   0x0 @cardEndReadDmaRef
.word   reset_arm9
needFlushDCCache:
.word   0x0
#ifdef GSDD
.word   0
.word   gsdd_fix
#else
.word   pdash_read
.word   0x0
#endif
.word	ipcSyncHandler
#ifndef NODSIWARE
.word   rumble_arm9
.word   rumble2_arm9
.word   ndmaCopy_arm
.word   dsiSaveCheckExists_arm
.word   dsiSaveGetResultCode_arm
.word   dsiSaveCreate_arm
.word   dsiSaveDelete_arm
.word   dsiSaveGetInfo_arm
.word   dsiSaveSetLength_arm
.word   dsiSaveOpen_arm
.word   dsiSaveOpenR_arm
.word   dsiSaveClose_arm
.word   dsiSaveGetLength_arm
.word   dsiSaveGetPosition_arm
.word   dsiSaveSeek_arm
.word   dsiSaveRead_arm
.word   dsiSaveWrite_arm
.word   musicPlay_arm
.word   musicStopEffect_arm
#endif
thumbPatches:
.word	thumb_card_read_arm9
#ifdef GSDD
.word   0
#else
.word   thumb_card_save_arm9
#endif
.word	thumb_card_irq_enable
.word	thumb_card_pull_out_arm9
.word	thumb_card_id_arm9
.word	thumb_card_dma_arm9
.word	thumb_card_set_dma_arm9
.word   thumb_nand_read_arm9
.word   thumb_nand_write_arm9
.word	cardStructArm9
.word   thumb_card_pull
.word   cacheFlushRef
thumbCardEndReadDmaRef:
.word   0x0 @cardEndReadDmaRef
.word   thumb_reset_arm9

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
	ldr	pc, =cardRead
.pool
@---------------------------------------------------------------------------------
card_save_arm9:
@---------------------------------------------------------------------------------
#ifdef GSDD
	ldr	pc, =cardSave
#else
	ldr	pc, =cardSaveA
#endif
.pool
cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
cacheRef:
.word    0x00000000  
	.thumb
@---------------------------------------------------------------------------------
thumb_card_read_arm9:
@---------------------------------------------------------------------------------
	push {r6, lr}
	ldr	r6, =cardRead
	blx	r6
	pop	{r6, pc}
.pool
.balign	4
#ifndef GSDD
@---------------------------------------------------------------------------------
thumb_card_save_arm9:
@---------------------------------------------------------------------------------
	push {r6, lr}
	ldr	r6, =cardSaveA
	blx	r6
	pop	{r6, pc}
.pool
.balign	4
#endif
	.arm
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_id_arm9:
@---------------------------------------------------------------------------------
	ldr r0, cardIdData
	bx lr
cardIdData:
.word  0xC2FF01C0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_dma_arm9:
@---------------------------------------------------------------------------------
#ifndef GSDD
	ldr		pc, =cardReadDma
.pool
#endif
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_set_dma_arm9:
@---------------------------------------------------------------------------------
#ifndef GSDD
	ldr		pc, =cardSetDma
.pool
#else
	mov r0, #0
	bx lr
#endif
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
cardIdDataT:
.word  0xC2FF01C0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_dma_arm9:
@---------------------------------------------------------------------------------
#ifndef GSDD
	push {r6, lr}
	ldr r6, =cardReadDma
	blx r6
	pop	{r6, pc}
.pool
.balign	4
#endif
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_set_dma_arm9:
@---------------------------------------------------------------------------------
#ifndef GSDD
	push {r6, lr}
	ldr r6, =cardSetDma
	blx r6
	pop {r6, pc}
.pool
.balign	4
#else
	mov r0, #0
	bx lr
#endif
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
    ldr pc,= nandRead
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
nand_write_arm9:
@---------------------------------------------------------------------------------
    ldr pc,= nandWrite
.pool
@---------------------------------------------------------------------------------

#ifndef NODSIWARE
@---------------------------------------------------------------------------------
ndmaCopy_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =ndmaCopy
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveCheckExists_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveCheckExists
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetResultCode_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetResultCode
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveCreate_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveCreate
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveDelete_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveDelete
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetInfo_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetInfo
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveSetLength_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveSetLength
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveOpen_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveOpen
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveOpenR_arm:
@---------------------------------------------------------------------------------
	mov r2, #1
	ldr	pc, =dsiSaveOpen
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveClose_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveClose
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetLength_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetLength
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetPosition_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetPosition
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveSeek_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveSeek
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveRead_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveRead
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveWrite_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveWrite
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
musicPlay_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =musicPlay
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
musicStopEffect_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =musicStopEffect
.pool
@---------------------------------------------------------------------------------
#endif

	.thumb
@---------------------------------------------------------------------------------
thumb_nand_read_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr		r6, =nandRead
    blx	r6

	pop	{r6, pc}
.pool
.balign	4
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_nand_write_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr		r6, =nandWrite
    blx	r6

	pop	{r6, pc}
.pool
.balign	4
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
card_irq_enable:
@---------------------------------------------------------------------------------
	ldr pc,= myIrqEnable
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_card_irq_enable:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr	r6, =myIrqEnable
    blx	r6

	pop	{r6, pc}
.pool
.balign	4
@---------------------------------------------------------------------------------

	.arm
#ifndef GSDD
pdash_read:
    push	{r1-r11, lr}
    @mov     r0, r4 @DST
    @mov     r1, r5 @SRC
    @mov     r2, r6 @LEN
    @mov     r3, r10 @cardStruct
    add     r0, r0, #0x2C    
    ldr		r6, =cardReadPDash
	blx		r6
    pop	    {r1-r11, pc}
.pool
#else
gsdd_fix:
	push {lr}
	bl gsddFix
	mov r0, #1
	pop {pc}

.global gsdd_fix2
gsdd_fix2: .word gsdd_fix2+4
	push {r0-r3, lr}
	mov r0, r6
	bl gsddFix2
	pop {r0-r3}
	sub r1, r4, #0x11
	pop {pc}
#endif

	.thumb   
@---------------------------------------------------------------------------------
thumb_reset_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr	r6, =reset
    blx	r6

	pop	{r6, pc}
.pool
@---------------------------------------------------------------------------------


	.arm
@---------------------------------------------------------------------------------
reset_arm9:
@---------------------------------------------------------------------------------
    ldr pc,= reset
.pool
@---------------------------------------------------------------------------------

.global callEndReadDmaThumb
.type	callEndReadDmaThumb STT_FUNC
callEndReadDmaThumb:
    push	{r1-r11, lr}
    ldr     r6, thumbCardEndReadDmaRef
    add     r6, #1
	blx		r6
    pop	    {r1-r11, pc}

.global disableIrqMask
.type	disableIrqMask STT_FUNC
disableIrqMask:
	PUSH {R7, LR}
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
	POP {R7, PC}

#ifndef NODSIWARE
@---------------------------------------------------------------------------------
rumble_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r1-r11,lr}

	ldr		r6, =rumble
    blx	r6
	nop

	ldmfd   sp!, {r1-r11,pc}
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
rumble2_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r1-r11,lr}

	ldr		r6, =rumble2
    blx	r6
	nop

	ldmfd   sp!, {r1-r11,pc}
.pool
@---------------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------------
.global  getDtcmBase
.type	 getDtcmBase STT_FUNC
/*---------------------------------------------------------------------------------
	getDtcmBase
---------------------------------------------------------------------------------*/
getDtcmBase:
	mrc	p15, 0, r0, c9, c1, 0
	bx	lr


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

	ldmfd   sp!, {r0-r11,pc}
	.pool
	

card_engine_end:
