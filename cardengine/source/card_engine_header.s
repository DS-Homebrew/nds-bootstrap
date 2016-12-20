@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global card_engine_start
.global card_engine_start_sync
.global card_engine_end
.global cardStruct
.global cacheStruct
.global patches_offset
.global sdk_version
.global fileCluster


patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
intr_fifo_orig_return:
	.word	0x00000000
sdk_version:
	.word	0x00000000
fileCluster:
	.word	0x00000000
cardStruct:
	.word	0x00000000
cacheStruct:
	.word	0x00000000
	
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
.word	card_read_arm9
.word	card_pull_out_arm9
.word	card_init_pull_arm7
.word	vblankHandler
.word	fifoHandler
.word	cardStructArm9
card_read_arm9:
    stmfd   sp!, {r0-r11,lr}
    
	@ registers used r0,r1,r2,r3,r5,r6,r7,r8
    ldr     r3,=0x4000100     @IPC_SYNC & command value
    ldr     r8,=0x027FFB08    @shared area command
	@sub 	r0, r8, #(0x027FFB08 - 0x027ff800) @shared area data
    ldr     r4, cardStructArm9
    ldr     r5, [R4]      @SRC
    ldr     r1, [R4,#0x4] @DST
    ldr     r7, [R4,#0x8] @LEN
	@mov r2, #128          @len for swicopy
	
check_partial:
	cmp r7, #512    
    blt partial
    
chunck_loop:
    mov r4, #512
	@dst, marker, len, src
	stmia r8, {r1,r3,r4,r5}
    
    @sendIPCSync
    mov    r11, #0x2400
    strh    r11, [r3,#0x80]

chunck_loop_wait:
    ldr r9, [r8,#4]
    cmp r9,#2
    beq chunck_begin_copy
    cmp r9,#0
    bne chunck_loop_wait
chunck_begin_copy:
    mov r10, #512
    sub r0, r8, #(0x027FFB08 - 0x027ff800) @shared area data
chunck_loop_copy:
    ldrb r9, [r0], #1
    strb r9, [r1], #1
    subs r10, #1
    bgt chunck_loop_copy
@   b chunk_end_copy

@chunck_swi_copy: 
@    swi 0x0C0000
@    add r1, #512

chunk_end_copy:
    add  r5, #512
    subs r7, #512
	beq exitfunc
    b check_partial

partial:
    mov r4, r7
	@dst, marker, len, src
    stmia r8, {r1,r3,r4,r5}
    
    @sendIPCSync
    mov    r11, #0x2400
    strh    R11, [r3,#0x80]

partial_loop_wait:
    ldr r9, [r8,#4]
    cmp r9,#2
    beq partial_begin_copy
    cmp r9,#0
    bne partial_loop_wait
	
partial_begin_copy:
    sub r0, r8, #(0x027FFB08 - 0x027ff800) @shared area data
partial_loop_copy:
    ldrb r9, [r0], #1
    strb r9, [r1], #1
    subs r7, #1
    bgt partial_loop_copy

exitfunc:
    ldmfd   sp!, {r0-r11,lr}
    bx      lr

cardStructArm9:
.word    0x00000000    
.pool
card_pull_out_arm9:
	bx      lr
	@.pool
card_init_pull_arm7:
    push    {lr}
	push	{r1-r12}
	ldr	r3, =myIrqEnable
	bl	_blx_r3_stub2
	pop   	{r1-r12} 
	pop  	{lr}
	bx  lr
@---------------------------------------------------------------------------------
_blx_r3_stub2:
@---------------------------------------------------------------------------------
	bx	r3		
.pool
