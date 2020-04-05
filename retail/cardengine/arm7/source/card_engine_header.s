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
.global fileCluster
.global saveCluster
.global srParamsCluster
.global ramDumpCluster
.global gameOnFlashcard
.global saveOnFlashcard
.global language
.global dsiMode
.global dsiSD
.global extendedMemory
.global ROMinRAM
.global consoleModel
.global romRead_LED
.global dmaRomRead_LED
.global preciseVolumeControl

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
intr_ndma0_orig_return:
	.word	0x00000000
moduleParams:
	.word	0x00000000
fileCluster:
	.word	0x00000000
srParamsCluster:
	.word	0x00000000
ramDumpCluster:
	.word	0x00000000
cardStruct:
	.word	0x00000000
gameOnFlashcard:
	.word	0x00000000
saveOnFlashcard:
	.word	0x00000000
language:
	.word	0x00000000
dsiMode:
	.word	0x00000000
dsiSD:
	.word	0x00000000
extendedMemory:
	.word	0x00000000
ROMinRAM:
	.word	0x00000000
consoleModel:
	.word	0x00000000
romRead_LED:
	.word	0x00000000
dmaRomRead_LED:
	.word	0x00000000
preciseVolumeControl:
	.word	0x00000000
cheat_data_offset:
	.word	cheat_data
extraIrqTable_offset:
	.word	extraIrqTable

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

ndma0Handler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_ndma0
	ldr 	r0,	intr_ndma0_orig_return
	bx  	r0

code_handler_start_vblank:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerVBlank
	bl	_blx_r3_stub		@ jump to myIrqHandler
	
	@ exit after return
	b	exit

code_handler_start_fifo:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerFIFO
	bl	_blx_r3_stub		@ jump to myIrqHandler
  
  
	@ exit after return
	b	exit

code_handler_start_ndma0:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerNdma0
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

irqHandler:
                STMFD           SP!, {LR}
                MOV             R12, #0x4000000
                ADD             R12, R12, #0x210
                LDR             R1, [R12,#-8]
                CMP             R1, #0
                LDMEQFD         SP!, {PC}
                LDMIA           R12, {R1,R2}
                ANDS            R1, R1, R2
                BEQ             loc_2384378
                MOV             R3, #1
                MOV             R0, #0

loc_238435C:
                ANDS            R2, R1, R3,LSL R0
                ADDEQ           R0, R0, #1
                BEQ             loc_238435C
                STR             R2, [R12,#4]
                LDR             R1, irqTable
                LDR             R0, [R1,R0,LSL#2]
                B               loc_23843A8
@ ---------------------------------------------------------------------------

loc_2384378:
                ADD             R12, R12, #8
                LDMIA           R12, {R1,R2}
                ANDS            R1, R1, R2
                LDMEQFD         SP!, {PC}
                MOV             R3, #1
                MOV             R0, #0

loc_2384390:
                ANDS            R2, R1, R3,LSL R0
                ADDEQ           R0, R0, #1
                BEQ             loc_2384390
                STR             R2, [R12,#4]
                LDR             R1, =extraIrqTable
                LDR             R0, [R1,R0,LSL#2]

loc_23843A8:
                LDR             R2, =extraIrqTable0
                STMIA           R2, {R4-R11,SP}
                LDR             LR, irqRet
                BX              R0
@ End of function irqHandler

@ ---------------------------------------------------------------------------
.pool
irqTable:
	.word	0
irqRet:
	.word	0


extraIrqTable:
	.word	extraIrq_ret		@ GPIO18[0]
	.word	extraIrq_ret		@ GPIO18[1]
	.word	extraIrq_ret		@ GPIO18[2]
	.word	extraIrq_ret		@ Unused (0)
	.word	extraIrq_ret		@ GPIO33[0] unknown (related to "GPIO330" testpoint on mainboard?)
	.word	extraIrq_ret		@ GPIO33[1] Headphone connect (HP#SP) (static state)
	.word	extraIrq_ret		@ GPIO33[2] Powerbutton interrupt (short pulse upon key-down)
	.word	extraIrq_ret		@ GPIO33[3] sound enable output (ie. not a useful irq-input)
	.word	extraIrq_ret		@ SD/MMC Controller   ;-Onboard eMMC and External SD Slot
	.word	extraIrq_ret		@ SD Slot Data1 pin   ;-For SDIO hardware in External SD Slot
	.word	extraIrq_ret		@ SDIO Controller     ;\Atheros Wifi Unit
	.word	extraIrq_ret		@ SDIO Data1 pin      ;/
	.word	extraIrq_ret		@ AES interrupt
	.word	extraIrq_ret		@ I2C interrupt
	.word	extraIrq_ret		@ Microphone Extended interrupt
	.word	extraIrq_ret
extraIrqTable0:
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret
	.word	extraIrq_ret

extraIrq_ret:
	bx  lr

card_engine_end:

patches:
.word	card_pull_out_arm9
.word	card_irq_enable_arm7
.word	j_irqHandler
.word	vblankHandler
.word	fifoHandler
.word	ndma0Handler
.word   card_pull
.word   arm7Functions
.word   swi02
.word   j_twlGetPitchTable
.word   j_twlGetPitchTableThumb
.word   getPitchTableStub
.word   arm7FunctionsThumb
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
swi02:
@---------------------------------------------------------------------------------
	swi	#0x02
	bx	lr
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
j_irqHandler:
@---------------------------------------------------------------------------------
	ldr	r12, = irqHandler
	bx	r12
.pool
@---------------------------------------------------------------------------------

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

	.thumb
@---------------------------------------------------------------------------------
j_twlGetPitchTableThumb:
@---------------------------------------------------------------------------------
	push	{r4-r6, lr}
	ldr	r6, =twlGetPitchTableBranch
	bl	_blx_r6_stub
	pop             {r4-r6}
	pop             {r3}
	bx  r3
_blx_r6_stub:
	bx	r6
.pool

	.arm
@---------------------------------------------------------------------------------
twlGetPitchTableBranch:
@---------------------------------------------------------------------------------
sub_2386E60:                             @ CODE XREF: sub_2387560+2C4?p
                STMFD           SP!, {R4-R6,LR}
                MOV             R5, R0
                RSB             R0, R1, #0
                MOV             R4, #0
                B               loc_2386E7C
@ ---------------------------------------------------------------------------

loc_2386E74:                             @ CODE XREF: sub_2386E60+20?j
                SUB             R4, R4, #1
                ADD             R0, R0, #0x300

loc_2386E7C:                             @ CODE XREF: sub_2386E60+10?j
                CMP             R0, #0
                BLT             loc_2386E74
                B               loc_2386E90
@ ---------------------------------------------------------------------------

loc_2386E88:                             @ CODE XREF: sub_2386E60+34?j
                ADD             R4, R4, #1
                SUB             R0, R0, #0x300

loc_2386E90:                             @ CODE XREF: sub_2386E60+24?j
                CMP             R0, #0x300
                BGE             loc_2386E88
                BL              twlGetPitchTable
                ADDS            R3, R0, #0x10000
                MOV             R0, R5,ASR#31
                UMULL           R2, R1, R3, R5
                MOV             R12, #0
                MLA             R1, R3, R0, R1
                ADC             R3, R12, #0
                SUB             R0, R4, #0x10
                MLA             R1, R3, R5, R1
                CMP             R0, #0
                MOV             R4, #0x10000
                BGT             loc_2386EE8
                RSB             R3, R0, #0
                MOV             R4, R2,LSR R3
                RSB             R0, R3, #0x20
                ORR             R4, R4, R1,LSL R0
                SUB             R0, R3, #0x20
                MOV             R3, R1,LSR R3
                ORR             R4, R4, R1,LSR R0
                B               loc_2386F44
@ ---------------------------------------------------------------------------

loc_2386EE8:                             @ CODE XREF: sub_2386E60+64?j
                CMP             R0, #0x20
                BGE             loc_2386F3C
                RSB             R5, R0, #0x20
                SUB             LR, R12, #1
                MOV             R6, LR,LSL R5
                RSB             R3, R5, #0x20
                ORR             R6, R6, LR,LSR R3
                SUB             R3, R5, #0x20
                ORR             R6, R6, LR,LSL R3
                AND             R3, R1, R6
                AND             R6, R2, LR,LSL R5
                CMP             R3, R12
                CMPEQ           R6, R12
                SUBNE           R0, R4, #1
                BNE             loc_2386F74
                MOV             R3, R1,LSL R0
                ORR             R3, R3, R2,LSR R5
                SUB             R1, R0, #0x20
                MOV             R4, R2,LSL R0
                ORR             R3, R3, R2,LSL R1
                B               loc_2386F44
@ ---------------------------------------------------------------------------

loc_2386F3C:                             @ CODE XREF: sub_2386E60+8C?j
                SUB             R0, R4, #1
                B               loc_2386F74
@ ---------------------------------------------------------------------------

loc_2386F44:                             @ CODE XREF: sub_2386E60+84?j
                                        @ sub_2386E60+D8?j
                MOV             R0, #0x10
                CMP             R3, #0
                CMPEQ           R4, #0x10
                MOV             R1, #0
                MOVCC           R4, R0
                BCC             loc_2386F6C
                LDR             R0, =0xFFFF
                CMP             R3, R1
                CMPEQ           R4, R0
                MOVHI           R4, R0

loc_2386F6C:                             @ CODE XREF: sub_2386E60+F8?j
                MOV             R0, R4,LSL#16
                MOV             R0, R0,LSR#16

loc_2386F74:                             @ CODE XREF: sub_2386E60+C0?j
                                        @ sub_2386E60+E0?j
                LDMFD           SP!, {R4-R6,LR}
                BX              LR
@ End of function sub_2386E60
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
getPitchTableStub:
@---------------------------------------------------------------------------------
	nop
	nop
	nop
	nop
	nop
	nop
@---------------------------------------------------------------------------------

	.arm
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

@---------------------------------------------------------------------------------
card_pull:
@---------------------------------------------------------------------------------
	bx lr
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
	bx	r3

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
	push    {lr}
	push	{r1-r4}
	ldr	r3, =cardId
	bl	_blx_r3_stubthumb
	pop   	{r1-r4} 
	pop  	{r3}
	bx  r3

	.pool

	.arm
.global tryLockMutex
.type	tryLockMutex STT_FUNC
@ r0 : mutex adr
tryLockMutex:
	mov r1, r0   
	mov r2, #1
	swp r0,r2, [r1]
	cmp r0, r2
	beq trymutex_fail	
	mov r0, #1
	b mutex_exit	
trymutex_fail:
	mov r0, #0
mutex_exit:
	bx  lr

.global lockMutex
.type	lockMutex STT_FUNC
@ r0 : mutex adr
lockMutex:
  mov r1, r0    
  mov r2, #1
mutex_loop:
  swp r0,r2, [r1]
  cmp r0,r2
  beq mutex_loop    
  mov r0, #1	
  bx  lr



.global unlockMutex
.type	unlockMutex STT_FUNC
@ r0 : mutex adr
unlockMutex:  
	mov r1, #0
	str r1, [r0]
	bx  lr
