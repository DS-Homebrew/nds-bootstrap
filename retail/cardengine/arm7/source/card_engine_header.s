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
.global moduleParams
.global saveCluster
.global language
.global languageAddr

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32


ce7 :
	.word	ce7
patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
moduleParams:
	.word	0x00000000
cardStruct:
	.word	0x00000000
language:
	.word	0x00000000
languageAddr:
	.word	0x00000000

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

patches:
.word	card_pull_out_arm9
.word	vblankHandler
.word   card_pull
.word   cacheFlushRef
.word   readCachedRef
.word   arm7Functions
.word   arm7FunctionsThumb

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
card_pull:
@---------------------------------------------------------------------------------
	bx lr

arm7Functions:
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
_blx_r3_stubthumb:
	bx	lr

eepromProtectThumbStub:
	push    {r14}
	push	{r1-r4}
	ldr	r3, =eepromProtect
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3    
	
eepromPageEraseThumbStub:
	push    {lr}
	push	{r1-r4}
	ldr	r3, =eepromPageErase
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3    

eepromPageVerifyThumbStub:
	push    {lr}
	push	{r1-r4}
	ldr	r3, =eepromPageVerify
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3
	
eepromPageWriteThumbStub:
	push    {lr}
	push	{r1-r4}
	ldr	r3, =eepromPageWrite
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3
	
eepromPageProgThumbStub:
	push    {lr}
	push	{r1-r4}
	ldr	r3, =eepromPageProg
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3

cardReadThumbStub:
	push    {lr}
	push	{r1-r4}
	ldr	r3, =cardRead
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3

eepromReadThumbStub:
	push    {lr}
	push	{r1-r4}
	ldr	r3, =eepromRead
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3
	
cardIdThumbStub:
	ldr r0, cardIdDataT
	bx      lr
.align	4
cardIdDataT:
.word  0xC2FF01C0

	.pool
