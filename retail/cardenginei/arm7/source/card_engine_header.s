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
.global screenshotCluster
.global pageFileCluster
.global manualCluster
.global valueBits
.global language
.global languageAddr
.global consoleModel
.global romRead_LED
.global dmaRomRead_LED
.global scfgRomBak
.global igmHotkey
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
.align	4

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

ndsCodeStart:
@	.thumb
@	bx	pc
@.align	4
@	.arm
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
                LDR             R1, dsiIrqTable
                LDR             R0, [R1,R0,LSL#2]

loc_23843A8:
                LDR             R2, dsiIrqRet
                STMIA           R2, {R4-R11,SP}
                LDR             LR, irqRet
                BX              R0
@ End of function irqHandler

@ ---------------------------------------------------------------------------
irqTable:
	.word	0
irqRet:
	.word	0
.global dsiIrqTable
dsiIrqTable:
	.word	extraIrqTable
.global dsiIrqRet
dsiIrqRet:
	.word	extraIrqRet
@.global extraIrqTable_offset
@extraIrqTable_offset:
@	.word	extraIrqTable
@.global extraIrqRet_offset
@extraIrqRet_offset:
@	.word	extraIrqRet


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
extraIrqRet:
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

extraIrq_ret:
	bx  lr

card_engine_end:

patches:
.word	card_pull_out_arm9
.word	card_irq_enable_arm7
.word	thumb_card_irq_enable_arm7
.word	j_irqHandler
.word	vblankHandler
.word	fifoHandler
.word	ndma0Handler
.word   card_pull
.word   arm7Functions
.word   arm7FunctionsThumb
.word   swi02
.word   swi24
.word   swi25
.word   swi26
.word   swi27
.word   j_twlGetPitchTable
.word   j_twlGetPitchTableThumb
.word   getPitchTableStub
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
swi02:
@---------------------------------------------------------------------------------
	swi	#0x02
	bx	lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
swi24:
@---------------------------------------------------------------------------------
	ldr	r3, =0x02F15871
	bx	r3
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
swi25:
@---------------------------------------------------------------------------------
	ldr	r3, =0x02F158AD
	bx	r3
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
swi26:
@---------------------------------------------------------------------------------
	ldr	r3, =0x02F15779
	bx	r3
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
swi27:
@---------------------------------------------------------------------------------
	ldr	r3, =0x02F12B15
	bx	r3
.pool
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
j_irqHandler:
@---------------------------------------------------------------------------------
	ldr	r12, =irqHandler
	bx	r12
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
j_twlGetPitchTable:
@---------------------------------------------------------------------------------
	ldr	r12, =twlGetPitchTable
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
