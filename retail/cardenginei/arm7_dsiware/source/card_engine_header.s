#include "locations.h"

@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global ce7
	.align	4
	.arm

.global card_engine_start
.global card_engine_start_sync
.global card_engine_end
.global cardStruct
.global patches_offset
.global cheatEngineAddr
.global moduleParams
.global fileCluster
.global saveCluster
.global patchOffsetCacheFileCluster
.global srParamsCluster
.global ramDumpCluster
.global screenshotCluster
.global pageFileCluster
.global manualCluster
.global valueBits
.global mainScreen
.global language
.global languageAddr
.global consoleModel
.global romRead_LED
.global dmaRomRead_LED
.global scfgRomBak
.global igmHotkey
.global ndsCodeStart
.global romLocation
.global romMapLines
.global romMap

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32


ce7 :
	.word	ce7
patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
intr_fifo_orig_return:
	.word	0x00000000
cheatEngineAddr:
	.word	0x00000000
moduleParams:
	.word	0x00000000
fileCluster:
	.word	0x00000000
patchOffsetCacheFileCluster:
	.word	0x00000000
srParamsCluster:
	.word	0x00000000
ramDumpCluster:
	.word	0x00000000
screenshotCluster:
	.word	0x00000000
pageFileCluster:
	.word	0x00000000
manualCluster:
	.word	0x00000000
cardStruct:
	.word	0x00000000
valueBits:
	.word	0x00000000
mainScreen:
	.word	0
languageAddr:
	.word	0x00000000
language:
	.byte	0
consoleModel:
	.byte	0
romRead_LED:
	.byte	0
dmaRomRead_LED:
	.byte	0
irqTable_offset:
	.word	irqTable
scfgRomBak:
	.hword	0
igmHotkey:
	.hword	0
romLocation:
	.word	0x00000000
romMapLines:
	.word	0x00000000
romMap:
	.word	0x00000000
	.word	0x00000000
	.word	0x00000000

	.word	0x00000000
	.word	0x00000000
	.word	0x00000000

	.word	0x00000000
	.word	0x00000000
	.word	0x00000000

	.word	0x00000000
	.word	0x00000000
	.word	0x00000000

	.word	0x00000000
	.word	0x00000000
	.word	0x00000000
.align	4

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

vblankHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_vblank
	ldr 	r0,	intr_vblank_orig_return
	bx  	r0

code_handler_start_vblank:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerVBlank
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
.word	0
.word	card_irq_enable_arm7
.word	thumb_card_irq_enable_arm7
.word	0
.word	vblankHandler
.word	0
.word   0
.word   arm7FunctionsDirect
.word   arm7Functions
.word   arm7FunctionsThumb
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0
.pool
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

	.thumb
@---------------------------------------------------------------------------------
thumb_card_irq_enable_arm7:
@---------------------------------------------------------------------------------
    push	{r4, lr}

	ldr		r3, =myIrqEnable

	bl	thumb_blx_r3_stub2
	pop	{r4}
	pop	{r3}
	bx  r3
thumb_blx_r3_stub2:
	bx	r3
.pool
.align	4
@---------------------------------------------------------------------------------

#ifdef CARDSAVE
arm7FunctionsDirect:
.word    eepromProtect
.word    eepromPageErase
.word    eepromPageVerify
.word    eepromPageWrite
.word    eepromPageProg
.word    eepromRead
.word    cardRead
.word    cardId

arm7Functions:
.word    eepromProtectStub
.word    eepromPageEraseStub
.word    eepromPageVerifyStub
.word    eepromPageWriteStub
.word    eepromPageProgStub
.word    eepromReadStub
.word    cardReadStub
.word    cardIdStub
saveCluster:
.word    0x00000000
saveSize:
.word    0x00000000

eepromProtectStub:
	stmfd   sp!, {r3-r11,lr}
	ldr	r4, =eepromProtect
	bl	_blx_r4_stub1
	ldmfd   sp!, {r3-r11,pc}
_blx_r4_stub1:
	bx	r4
.pool
eepromPageEraseStub:
	stmfd   sp!, {r3-r11,lr}
	ldr	r4, =eepromPageErase
	bl	_blx_r4_stub2
	ldmfd   sp!, {r3-r11,pc}
_blx_r4_stub2:
	bx	r4
.pool
eepromPageVerifyStub:
	stmfd   sp!, {r3-r11,lr}
	ldr	r4, =eepromPageVerify
	bl	_blx_r4_stub3
	ldmfd   sp!, {r3-r11,pc}
_blx_r4_stub3:
	bx	r4
.pool
eepromPageWriteStub:
	stmfd   sp!, {r4-r11,lr}
	ldr	r4, =eepromPageWrite
	bl	_blx_r4_stub4
	ldmfd   sp!, {r4-r11,pc}
_blx_r4_stub4:
	bx	r4
.pool
eepromPageProgStub:
	stmfd   sp!, {r4-r11,lr}
	ldr	r4, =eepromPageProg
	bl	_blx_r4_stub5
	ldmfd   sp!, {r4-r11,pc}
_blx_r4_stub5:
	bx	r4
.pool
cardReadStub:
	stmfd   sp!, {r4-r11,lr}
	ldr	r4, =cardRead
	bl	_blx_r4_stub6
	ldmfd   sp!, {r4-r11,pc}
_blx_r4_stub6:
	bx	r4
.pool
eepromReadStub:
	stmfd   sp!, {r4-r11,lr}
	ldr	r4, =eepromRead
	bl	_blx_r4_stub7
	ldmfd   sp!, {r4-r11,pc}
_blx_r4_stub7:
	bx	r4
.pool
cardIdStub:
	stmfd   sp!, {r4-r11,lr}
	ldr	r4, =cardId
	bl	_blx_r4_stub8
	ldmfd   sp!, {r4-r11,pc}
_blx_r4_stub8:
	bx	r4
.pool

arm7FunctionsThumb:
.word    eepromProtectThumbStub
.word    eepromPageEraseThumbStub
.word    eepromPageVerifyThumbStub
.word    eepromPageWriteThumbStub
.word    eepromPageProgThumbStub
.word    eepromReadThumbStub
.word    cardReadThumbStub
.word    cardIdThumbStub

.thumb
eepromProtectThumbStub:
	push	{r3-r7,lr}
	ldr	r4, =eepromProtect
	bl	_blx_r3_stubthumb1
	pop   	{r3-r7} 
	pop  	{r3}
	bx  r3    
_blx_r3_stubthumb1:
	bx	r4
.pool
eepromPageEraseThumbStub:
	push	{r3-r7,lr}
	ldr	r4, =eepromPageErase
	bl	_blx_r3_stubthumb2
	pop   	{r3-r7} 
	pop  	{r3}
	bx  r3    
_blx_r3_stubthumb2:
	bx	r4
.pool
eepromPageVerifyThumbStub:
	push	{r3-r7,lr}
	ldr	r4, =eepromPageVerify
	bl	_blx_r3_stubthumb3
	pop   	{r3-r7} 
	pop  	{r3}
	bx  r3
_blx_r3_stubthumb3:
	bx	r4
.pool
eepromPageWriteThumbStub:
	push	{r4-r7,lr}
	ldr	r4, =eepromPageWrite
	bl	_blx_r3_stubthumb4
	pop   	{r4-r7} 
	pop  	{r3}
	bx  r3
_blx_r3_stubthumb4:
	bx	r4
.pool
eepromPageProgThumbStub:
	push	{r4-r7,lr}
	ldr	r4, =eepromPageProg
	bl	_blx_r3_stubthumb5
	pop   	{r4-r7} 
	pop  	{r3}
	bx  r3
_blx_r3_stubthumb5:
	bx	r4
.pool
cardReadThumbStub:
	push	{r4-r6,lr}
	ldr	r4, =cardRead
	bl	_blx_r3_stubthumb6
	pop   	{r4-r6} 
	pop  	{r3}
	bx  r3
_blx_r3_stubthumb6:
	bx	r4
.pool
eepromReadThumbStub:
	push	{r4-r6,lr}
	ldr	r4, =eepromRead
	bl	_blx_r3_stubthumb7
	pop   	{r4-r6} 
	pop  	{r3}
	bx  r3
_blx_r3_stubthumb7:
	bx	r4
.pool
cardIdThumbStub:
	push	{r4-r6,lr}
	ldr	r4, =cardId
	bl	_blx_r3_stubthumb8
	pop   	{r4-r6} 
	pop  	{r3}
	bx  r3
_blx_r3_stubthumb8:
	bx	r4
.pool
#else
arm7FunctionsDirect:
arm7Functions:
arm7FunctionsThumb:
#endif
