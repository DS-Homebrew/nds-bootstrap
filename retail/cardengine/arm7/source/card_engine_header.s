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
.global saveCluster
.global valueBits
.global mainScreen
.global language
.global languageAddr
.global igmHotkey
.global RumblePakType
.global ndsCodeStart

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32


ce7 :
	.word	ce7
patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
cheatEngineAddr:
	.word	0x00000000
moduleParams:
	.word	0x00000000
cardStruct:
	.word	0x00000000
valueBits:
	.word	0x00000000
mainScreen:
	.word	0x00000000
language:
	.word	0x00000000
languageAddr:
	.word	0x00000000
igmHotkey:
	.hword	0
RumblePakType:
	.byte	0
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
.word	card_pull_out_arm9
.word	card_irq_enable_arm7
.word	thumb_card_irq_enable_arm7
.word	vblankHandler
.word   j_twlGetPitchTable
.word   arm7FunctionsDirect
.word   arm7Functions
.word   arm7FunctionsThumb
.pool

@---------------------------------------------------------------------------------
j_twlGetPitchTable:
@---------------------------------------------------------------------------------
	ldr	r12, = twlGetPitchTable
	bx	r12
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
twlGetPitchTable:
@---------------------------------------------------------------------------------
	ldr	r1, =0x46A
	subs	r0, r0, r1
	swi	#0x1B0000
	lsls	r0, r0, #0x10
	lsrs	r0, r0, #0x10
	bx	lr
@---------------------------------------------------------------------------------

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

	.arm
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
.word    cardId
saveCluster:
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
cardId:
	ldr r0, cardIdData
	bx      lr
cardIdData:
.word  0xC2FF01C0

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
	ldr r0, cardIdDataT
	bx      lr
.align	4
cardIdDataT:
.word  0xC2FF01C0

	.pool
