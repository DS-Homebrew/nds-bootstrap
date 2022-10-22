@---------------------------------------------------------------------------------
	.global mepHeapSetPatch
	.global cch2HeapAlloc
	@.global elePlHeapAlloc
	.global fourSwHeapAlloc
	@.global mvdk3HeapAlloc
	.global nintCdwnCalHeapAlloc
	.global siezHeapAlloc
	.align	4
	.arm

.word 0x5050454D @ 'MEPP' string

mepHeapSetPatch:
	.word mepHeapSetPatchFunc
cch2HeapAlloc:
	.word cch2HeapAllocFunc
@elePlHeapAlloc:
@	.word elePlHeapAllocFunc
fourSwHeapAlloc:
	.word fourSwHeapAllocFunc
@mvdk3HeapAlloc:
@	.word mvdk3HeapAllocFunc
nintCdwnCalHeapAlloc:
	.word nintCdwnCalHeapAllocFunc
siezHeapAlloc:
	.word siezHeapAllocFunc

@---------------------------------------------------------------------------------
mepHeapSetOrgFunc: .word 0
mepHeapSetPatchFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	cmp r3, #0x09000000
	ldmgefd   sp!, {r6,pc}

	ldr	r6, mepHeapSetOrgFunc
	bl	_blx_mepHeapSetOrgFunc

	ldmfd   sp!, {r6,pc}
_blx_mepHeapSetOrgFunc:
	bx	r6
@---------------------------------------------------------------------------------

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
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
@elePlOrgFunction: .word 0
@elePlHeapAllocFunc:
@---------------------------------------------------------------------------------
@	stmfd   sp!, {r3-r6,lr}

@	cmp r0, #0x100000 @ if this is a .sdat file
@	bgt elePlRunOrgFunction @ then run the original function

@	cmp r0, #0x1000 @ if file less than 8KB
@	blt elePlRunOrgFunction @ then run the original function

@	ldr r3, =0x02060000
@	cmp r10, r3 @ if filename is outside of arm9 binary
@	bgt elePlRunOrgFunction @ then run the original function

@	mov r6, #0
@elePlFilenameCheck:
@	ldr r3, =0x02000000 @ filename pointer list
@	ldr r5, [r3, r6]
@	cmp r5, r10
@	beq elePlUseOldHeapPtr
@	cmp r5, #0
@	streq r10, [r3, r6]
@	beq elePlLastHeapPtrUpdate
@	add r6, #4
@	b elePlFilenameCheck

@elePlLastHeapPtrUpdate:
@	ldr r3, =0x02003FFC @ last heap pointer
@	ldr r4, [r3]
@	cmp r4, #0
@	moveq r4, #0x09000000
@	addne r4, r0
@	str r4, [r3]

@ save heap ponter
@	ldr r3, =0x02001000 @ heap pointers
@	str r4, [r3, r6]
@	mov r0, r4
@	ldmfd   sp!, {r3-r6,pc}

@elePlUseOldHeapPtr:
@	ldr r3, =0x02001000 @ heap pointers
@	ldr r0, [r3, r6]
@	ldmfd   sp!, {r3-r6,pc}

@elePlRunOrgFunction:
@	ldr	r6, elePlOrgFunction
@	bl	_blx_elePlOrgFunction
@	ldmfd   sp!, {r3-r6,pc}
@_blx_elePlOrgFunction:
@	bx	r6
@.pool
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
@mvdk3HeapAllocFunc:
@---------------------------------------------------------------------------------
@	stmfd   sp!, {r5,lr}
@	ldr r5, =0x3456C @ Size of miniMario_anim.bin
@	cmp r6, r5
@	moveq r0, #0x09000000
@	ldmeqfd   sp!, {r6,pc}
@	ldr r5, =0x390B8 @ Size of miniPeach_anim.bin
@	cmp r6, r5
@	moveq r0, #0x09100000
@	ldmeqfd   sp!, {r6,pc}
@	ldr r5, =0x2FD2C @ Size of miniToad_anim.bin
@	cmp r6, r5
@	moveq r0, #0x09200000
@	ldmeqfd   sp!, {r6,pc}
@	@ldr r5, =0x49194 @ Size of miniDK_anim.bin
@	@cmp r6, r5
@	mov r0, #0x09300000
@	ldmfd   sp!, {r5,pc}
@.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
nintCdwnCalHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	ldr r6, =0x72C0 @ Size of Font.nftr
	cmp r0, r6
	moveq r0, #0x09000000
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x72B0 @ Size of FontCal.nftr
	cmp r0, r6
	ldreq r0, =0x09008000
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x67D4 @ Size of FontCal2.nftr
	cmp r0, r6
	ldreq r0, =0x09010000
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x7340 @ Size of FontInfo.nftr
	cmp r0, r6
	ldreq r0, =0x09018000
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x733C @ Size of FontLine.nftr
	cmp r0, r6
	ldreq r0, =0x09020000
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x7474 @ Size of FontPage.nftr
	cmp r0, r6
	ldreq r0, =0x09028000
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xAF44 @ Size of FontRed.nftr
	cmp r0, r6
	ldreq r0, =0x09030000
	ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x4452C @ Size of suraTWLFont8x16.nftr
	@cmp r0, r6
	ldr r0, =0x0903B000
	ldmfd   sp!, {r6,pc}
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
