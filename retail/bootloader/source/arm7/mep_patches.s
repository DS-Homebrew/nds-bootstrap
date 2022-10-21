@---------------------------------------------------------------------------------
	.global cch2HeapAlloc
	.global cch2HeapSetPatch
	.global fourSwHeapAlloc
	.global siezHeapAlloc
	.align	4
	.arm

.word 0x5050454D @ 'MEPP' string

cch2HeapAlloc:
	.word cch2HeapAllocFunc
cch2HeapSetPatch:
	.word cch2HeapSetPatchFunc
fourSwHeapAlloc:
	.word fourSwHeapAllocFunc
siezHeapAlloc:
	.word siezHeapAllocFunc

@---------------------------------------------------------------------------------
cch2OrgFunction: .word 0
cch2HeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r5-r6,lr}
	mov r5, #0

	ldr r6, =0x8D030 @ Size of fontGBK.bin
	cmp r0, r6
	moveq r0, #0x4000
	moveq r5, #1

	ldr	r6, cch2OrgFunction
	bl	_blx_cch2OrgFunction

	cmp r5, #1
	moveq r0, #0x09000000 @ Offset of fontGBK.bin

cch2HeapAlloc_return:
	ldmfd   sp!, {r5-r6,pc}
_blx_cch2OrgFunction:
	bx	r6
.pool

cch2HeapSetOrgFunc: .word 0
cch2HeapSetPatchFunc:
	stmfd   sp!, {r6,lr}

	cmp r3, #0x09000000
	ldmeqfd   sp!, {r6,pc}

	ldr	r6, cch2HeapSetOrgFunc
	bl	_blx_cch2HeapSetOrgFunc

	ldmfd   sp!, {r6,pc}
_blx_cch2HeapSetOrgFunc:
	bx	r6

@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
fourSwOrgFunction: .word 0
fourSwHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	@ldr r6, =0x45720 @ Size of subtask.cmp
	@cmp r0, r6
	@moveq r6, #0
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x6C24 @ Size of subtask_us_en.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x7090 @ Size of subtask_us_fr.cmp & subtask_eu_fr.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x71F4 @ Size of subtask_us_sp.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x6C70 @ Size of subtask_eu_en.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x6E40 @ Size of subtask_eu_gr.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x6E20 @ Size of subtask_eu_it.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x7260 @ Size of subtask_eu_sp.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	@ldr r6, =0x7468 @ Size of subtask_jp.cmp
	@cmp r0, r6
	@moveq r6, #4
	@beq fourSwHeapAlloc_cont
	ldr r6, =0x128F8 @ Size of pat.bin
	cmp r0, r6
	moveq r6, #0
	beq fourSwHeapAlloc_cont
	ldr r6, =0x1AFC7C @ Size of zeldat.bin
	cmp r0, r6
	moveq r6, #4
	beq fourSwHeapAlloc_cont
	ldr r6, =0x1086DC @ Size of zelmap.bin
	cmp r0, r6
	moveq r6, #8
	beq fourSwHeapAlloc_cont
	ldr r6, =0x20208 @ Size of us.kmsg
	cmp r0, r6
	moveq r6, #0xC
	beq fourSwHeapAlloc_cont
	ldr r6, =0x33310 @ Size of eu.kmsg
	cmp r0, r6
	moveq r6, #0xC
	beq fourSwHeapAlloc_cont
	ldr r6, =0xF638 @ Size of jp.kmsg
	cmp r0, r6
	moveq r6, #0xC
	beq fourSwHeapAlloc_cont
	ldr	r6, fourSwOrgFunction
	bl	_blx_fourSwOrgFunction
	b fourSwHeapAlloc_return

fourSwHeapAlloc_cont:
	ldr r0, =0x02019FA4+0x80 @ fourSwHeapAddr
	ldr r0, [r0, r6]

fourSwHeapAlloc_return:
	ldmfd   sp!, {r6,pc}
_blx_fourSwOrgFunction:
	bx	r6
fourSwHeapAddr:
@.word	0x09320000 @ Offset of subtask.cmp
@.word	0x09370000 @ Offset of subtask_??_??.cmp
.word	0x092C0000 @ Offset of pat.bin
.word	0x09000000 @ Offset of zeldat.bin
.word	0x091B0000 @ Offset of zelmap.bin
.word	0x092E0000 @ Offset of us/eu/jp.kmsg
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
siezHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	ldr r6, =0x1F1F24 @ Size of kr0000.ntfx
	cmp r1, r6
	moveq r0, #0x09000000 @ Offset of kr0000.ntfx
	beq siezHeapAlloc_return
	ldr r6, =0x1F876C @ Size of kr0100.ntfx
	cmp r1, r6
	moveq r0, #0x09200000 @ Offset of kr0100.ntfx
	beq siezHeapAlloc_return
	ldr r6, =0x020DB338
	bl	_blx_siezOrgFunction

siezHeapAlloc_return:
	ldmfd   sp!, {r6,pc}
_blx_siezOrgFunction:
	bx	r6
.pool
@---------------------------------------------------------------------------------
