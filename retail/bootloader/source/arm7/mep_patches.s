@---------------------------------------------------------------------------------
	@.global lrStoreTestCode
	.global mepHeapSetPatch
	.global twlFontHeapAlloc
	.global twlFontHeapAllocSize
	.global twlFontHeapAllocNoMep
	.global cch2HeapAlloc
	.global cch2HeapAddrPtr
	@.global elementalistsHeapAlloc
	.global fourSwHeapAlloc
	.global fourSwHeapAddrPtr
	@.global gate18HeapAlloc
	@.global gate18HeapAddrPtr
	.global goGoKokopoloHeapAlloc
	@.global goGoKokopoloHeapAddrPtr
	.global metalTorrentSndLoad
	@.global mvdk3HeapAlloc
@	.global myLtlRestHeapAlloc
@	.global myLtlRestHeapAddrPtr
	.global nintCdwnCalHeapAlloc
	.global nintCdwnCalHeapAddrPtr
	.global nintendojiHeapAlloc
	.global nintendojiHeapAddrPtr
	.global rmtRacersHeapAlloc
	.global rmtRacersHeapAddrPtr
	.global siezHeapAlloc
	.global siezHeapAddrPtr
	.align	4
	.arm

.word 0x5050454D @ 'MEPP' string

@lrStoreTestCode:
@	.word lrStoreTest
mepHeapSetPatch:
	.word mepHeapSetPatchFunc
twlFontHeapAlloc:
	.word twlFontHeapPtr
twlFontHeapAllocSize:
	.word twlFontHeapAllocFunc_end-twlFontHeapPtr
twlFontHeapAllocNoMep:
	.word twlFontHeapAllocFuncNoMep
cch2HeapAlloc:
	.word cch2HeapAllocFunc
cch2HeapAddrPtr:
	.word cch2HeapAddr
@elementalistsHeapAlloc:
@	.word elementalistsHeapAllocFunc
fourSwHeapAlloc:
	.word fourSwHeapAllocFunc
fourSwHeapAddrPtr:
	.word fourSwHeapAddr
@gate18HeapAlloc:
@	.word gate18HeapAllocFunc
@gate18HeapAddrPtr:
@	.word gate18HeapAddr
goGoKokopoloHeapAlloc:
	.word goGoKokopoloHeapAllocFunc
@goGoKokopoloHeapAddrPtr:
@	.word goGoKokopoloHeapAddr
metalTorrentSndLoad:
	.word metalTorrentSndLoadFunc
@mvdk3HeapAlloc:
@	.word mvdk3HeapAllocFunc
@myLtlRestHeapAlloc:
@	.word myLtlRestHeapAllocFunc
@myLtlRestHeapAddrPtr:
@	.word myLtlRestHeapAddr
nintCdwnCalHeapAlloc:
	.word nintCdwnCalHeapAllocFunc
nintCdwnCalHeapAddrPtr:
	.word nintCdwnCalHeapAddr
@mvdk3HeapAlloc:
nintendojiHeapAlloc:
	.word nintendojiHeapAllocFunc
nintendojiHeapAddrPtr:
	.word nintendojiHeapAddr
rmtRacersHeapAlloc:
	.word rmtRacersHeapAllocFunc
rmtRacersHeapAddrPtr:
	.word rmtRacersHeapAddr
siezHeapAlloc:
	.word siezHeapAllocFunc
siezHeapAddrPtr:
	.word siezHeapAddr

@---------------------------------------------------------------------------------
@lrStoreTest:
@---------------------------------------------------------------------------------
@	ldr r12, =0x02000000
@	str lr, [r12]
@	bx lr
@.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
mepHeapSetOrgFunc: .word 0
mepHeapSetPatchFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	cmp r3, #0x08000000
	ldmgefd   sp!, {r6,pc}

	ldr	r6, mepHeapSetOrgFunc
	bl	_blx_mepHeapSetOrgFunc

	ldmfd   sp!, {r6,pc}
_blx_mepHeapSetOrgFunc:
	bx	r6
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
twlFontHeapPtr: .word 0x09000000
twlFontHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r3-r6,lr}

	mov r6, #0
	cmp r1, #0x1000
	movlt r1, r0
	blt twlFontFilenameCheck
	cmp r1, #0x02000000
	blt twlFontFilenameCheck
	cmpge r1, #0x02800000
	movlt r1, r0
twlFontFilenameCheck:
	ldr r3, =0x02FFF100 @ filesize pointer list
	ldr r5, [r3, r6]
	cmp r5, r1
	beq twlFontUseOldHeapPtr
	cmp r5, #0
	streq r1, [r3, r6]
	beq twlFontLastHeapPtrUpdate
	add r6, #4
	b twlFontFilenameCheck

twlFontLastHeapPtrUpdate:
	ldr r3, =0x02FFF1FC @ last heap pointer
	ldr r4, [r3]
	cmp r4, #0
	ldreq r4, twlFontHeapPtr
	beq twlFontLastHeapPtrStr
	add r4, r1
	add r4, #0x40000
twlFontLastHeapPtrStr:
	str r4, [r3]

@ save heap ponter
	ldr r3, =0x02FFF180 @ heap pointers
	str r4, [r3, r6]
	mov r0, r4
	ldmfd   sp!, {r3-r6,pc}

twlFontUseOldHeapPtr:
	ldr r3, =0x02FFF180 @ heap pointers
	ldr r0, [r3, r6]
	ldmfd   sp!, {r3-r6,pc}
.pool
@---------------------------------------------------------------------------------
twlFontHeapAllocFunc_end:

@---------------------------------------------------------------------------------
twlFontHeapAllocFuncNoMep:
@---------------------------------------------------------------------------------
	ldr r0, =0x02300000
	bx	lr
.pool
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
	ldreq r0, cch2HeapAddr

cch2HeapAlloc_return:
	ldmfd   sp!, {r5-r6,pc}
_blx_cch2OrgFunction:
	bx	r6
cch2HeapAddr:
.word	0x09000000 @ Offset of fontGBK.bin
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
@elementalistsOrgFunction: .word 0
@elementalistsHeapAllocFunc:
@---------------------------------------------------------------------------------
@	stmfd   sp!, {r3-r7,lr}

@	mov r7, #0
@elementalistsFilenameCheck:
@	ldr r3, =0x02000000 @ filename pointer list
@	ldr r5, [r3, r7]
@	cmp r5, r6
@	beq elementalistsUseOldHeapPtr
@	cmp r5, #0
@	streq r6, [r3, r7]
@	beq elementalistsLastHeapPtrUpdate
@	add r7, #4
@	b elementalistsFilenameCheck

@elementalistsLastHeapPtrUpdate:
@	ldr r3, =0x02003FFC @ last heap pointer
@	ldr r4, [r3]
@	cmp r4, #0
@	moveq r4, #0x09000000
@	beq elementalistsLastHeapPtrStr
@elementalistsAlign:
@	add r4, #0xC000
@	cmp r4, r1
@	blt elementalistsAlign
@elementalistsLastHeapPtrStr:
@	str r4, [r3]

@ save heap ponter
@	ldr r3, =0x02001000 @ heap pointers
@	str r4, [r3, r7]
@	mov r0, r4
@	ldmfd   sp!, {r3-r7,pc}

@elementalistsUseOldHeapPtr:
@	ldr r3, =0x02001000 @ heap pointers
@	ldr r0, [r3, r7]
@	ldmfd   sp!, {r3-r7,pc}
@.pool
@---------------------------------------------------------------------------------
@ old functions
@elementalistsAlign:
@	sub r1, #4
@	cmp r1, #4
@	bgt elementalistsAlign
@	beq elementalistsLastHeapPtrStr

@	cmp r1, #3
@	addeq r4, #1
@	beq elementalistsLastHeapPtrStr
@	cmp r1, #2
@	addeq r4, #2
@	beq elementalistsLastHeapPtrStr
@	cmp r1, #1
@	addeq r4, #3

@elementalistsRunOrgFunction:
@	ldr	r7, elementalistsOrgFunction
@	bl	_blx_elementalistsOrgFunction
@	ldmfd   sp!, {r3-r7,pc}
@_blx_elementalistsOrgFunction:
@	bx	r7
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
fourSwOrgFunction: .word 0
fourSwHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	@ldr r6, =0x45720 @ Size of subtask.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x6C24 @ Size of subtask_us_en.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x7090 @ Size of subtask_us_fr.cmp & subtask_eu_fr.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x71F4 @ Size of subtask_us_sp.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x6C70 @ Size of subtask_eu_en.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x6E40 @ Size of subtask_eu_gr.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x6E20 @ Size of subtask_eu_it.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x7260 @ Size of subtask_eu_sp.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x7468 @ Size of subtask_jp.cmp
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_subtask_lang
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x128F8 @ Size of pat.bin
	@cmp r0, r6
	@ldreq r0, fourSwHeapAddr_pat
	@ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x1AFC7C @ Size of zeldat.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xFA30 @ Size of zeldat_us_en.bin / zeldat_eu_en.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF89C @ Size of zeldat_us_fr.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF898 @ Size of zeldat_us_sp.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF8AC @ Size of zeldat_eu_fr.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF788 @ Size of zeldat_eu_gr.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF9F8 @ Size of zeldat_eu_it.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF888 @ Size of zeldat_eu_sp.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xFDC0 @ Size of zeldat_jp.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x1086DC @ Size of zelmap.bin
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_zelmap
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x20208 @ Size of us.kmsg
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_kmsg
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x33310 @ Size of eu.kmsg
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_kmsg
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xF638 @ Size of jp.kmsg
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_kmsg
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x1008 @ Size of font_ltn.nftr
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_font
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x15100 @ Size of font_jp.nftr
	cmp r0, r6
	ldreq r0, fourSwHeapAddr_font
	ldmeqfd   sp!, {r6,pc}

	ldr	r6, fourSwOrgFunction
	bl	_blx_fourSwOrgFunction
	ldmfd   sp!, {r6,pc}
_blx_fourSwOrgFunction:
	bx	r6
fourSwHeapAddr:
fourSwHeapAddr_subtask:
@.word	0x09320000 @ Offset of subtask.cmp
fourSwHeapAddr_subtask_lang:
@.word	0x09370000 @ Offset of subtask_??_??.cmp
fourSwHeapAddr_pat:
@.word	0x092C0000 @ Offset of pat.bin
fourSwHeapAddr_zeldat:
.word	0x09000000 @ Offset of zeldat.bin
fourSwHeapAddr_zeldat_lang:
.word	0x091B0000 @ Offset of zeldat_??_??.bin
fourSwHeapAddr_zelmap:
.word	0x091C0000 @ Offset of zelmap.bin
fourSwHeapAddr_kmsg:
.word	0x092D0000 @ Offset of us/eu/jp.kmsg
fourSwHeapAddr_font:
.word	0x09308000 @ Offset of font_??.nftr
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
@gate18OrgFunction: .word 0
@gate18HeapAllocFunc:
@---------------------------------------------------------------------------------
@	stmfd   sp!, {r6,lr}

@	ldr r6, =0x8D030 @ Size of fontGBK.bin
@	cmp r1, r6
@	ldreq r0, gate18HeapAddr
@	ldmeqfd   sp!, {r6,pc}

@	ldr	r6, gate18OrgFunction
@	bl	_blx_gate18OrgFunction

@	ldmfd   sp!, {r6,pc}
@_blx_gate18OrgFunction:
@	bx	r6
@gate18HeapAddr:
@.word	0x09000000 @ Offset of fontGBK.bin
@.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
goGoKokopoloHeapAllocFunc:
@---------------------------------------------------------------------------------
	ldr r6, =0x11200
	bx lr
.pool

@	stmfd   sp!, {r6-r7,lr}
@	mov r7, #0

@	ldr r6, goGoKokopoloHeapAddr
@	cmp r9, #0
@	moveq r0, r6
@	ldmeqfd   sp!, {r6-r7,pc}

@goGoKokopoloHeapAllocLoop:
@	add r6, r0
@	add r7, #1
@	cmp r9, r7
@	bne goGoKokopoloHeapAllocLoop

@	mov r0, r6
@	ldmfd   sp!, {r6-r7,pc}
@goGoKokopoloHeapAddr:
@.word	0x09000000
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
metalTorrentSndLoadOrgFunc: .word 0
metalTorrentSndLoadFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	cmp r2, #2
	ldmeqfd   sp!, {r6,pc}

	ldr r6, metalTorrentSndLoadOrgFunc
	bl	_blx_metalTorrentSndLoadOrgFunc

	ldmfd   sp!, {r6,pc}
_blx_metalTorrentSndLoadOrgFunc:
	bx	r6
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
@mvdk3HeapAllocFunc:
@---------------------------------------------------------------------------------
@	stmfd   sp!, {r5,lr}
@	ldr r5, =0x3456C @ Size of miniMario_anim.bin
@	cmp r6, r5
@	moveq r0, #0x09000000
@	ldmeqfd   sp!, {r5,pc}
@	ldr r5, =0x390B8 @ Size of miniPeach_anim.bin
@	cmp r6, r5
@	moveq r0, #0x09100000
@	ldmeqfd   sp!, {r5,pc}
@	ldr r5, =0x2FD2C @ Size of miniToad_anim.bin
@	cmp r6, r5
@	moveq r0, #0x09200000
@	ldmeqfd   sp!, {r5,pc}
@	@ldr r5, =0x49194 @ Size of miniDK_anim.bin
@	@cmp r6, r5
@	mov r0, #0x09300000
@	ldmfd   sp!, {r5,pc}
@.pool
@---------------------------------------------------------------------------------
@	.thumb
@---------------------------------------------------------------------------------
@myLtlRestHeapAllocFunc:
@---------------------------------------------------------------------------------
@	push {r6,lr}

@	@ldr r6, =0x1C404C @ Modified size of textures.dat (Original: 0x1C7BD0)
@	@cmp r0, r6
@	@bne myLtlRestHeapAllocFunc_return
@	ldr r0, myLtlRestHeapAddr
@	@pop {r6,pc}

@myLtlRestHeapAllocFunc_return:
@	pop {r6,pc}
@.align 4
@myLtlRestHeapAddr:
@.word	0x09080000
@.pool
@---------------------------------------------------------------------------------
@	.arm
@---------------------------------------------------------------------------------
nintCdwnCalHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	ldr r6, =0x72C0 @ Size of Font.nftr
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x72B0 @ Size of FontCal.nftr
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr+4
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x67D4 @ Size of FontCal2.nftr
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr+8
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x7340 @ Size of FontInfo.nftr
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr+0xC
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x733C @ Size of FontLine.nftr
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr+0x10
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x7474 @ Size of FontPage.nftr
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr+0x14
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0xAF44 @ Size of FontRed.nftr (USA/EUR)
	cmp r0, r6
	ldreq r0, nintCdwnCalHeapAddr+0x18
	ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x4AE8 @ Size of FontRed.nftr (JAP)
	@cmp r0, r6
	@ldreq r0, nintCdwnCalHeapAddr+0x18
	@ldmeqfd   sp!, {r6,pc}

	@ldr r6, =0x4452C @ Size of suraTWLFont8x16.nftr
	@cmp r0, r6
	ldr r0, nintCdwnCalHeapAddr+0x1C
	ldmfd   sp!, {r6,pc}
nintCdwnCalHeapAddr:
.word	0x09000000
.word	0x09008000
.word	0x09010000
.word	0x09018000
.word	0x09020000
.word	0x09028000
.word	0x09030000
.word	0x0903B000
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
nintendojiHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	cmp r1, #0x500000 @ Size of fileHeap
	ldreq r0, nintendojiHeapAddr
	ldmeqfd   sp!, {r6,pc}

	ldr r6, =0x02022E14
	bl	_blx_nintendojiOrgFunction

	ldmfd   sp!, {r6,pc}
_blx_nintendojiOrgFunction:
	bx	r6
nintendojiHeapAddr:
.word	0x09100000
.pool
@---------------------------------------------------------------------------------
	.thumb
@---------------------------------------------------------------------------------
rmtRacersHeapAllocFunc:
@---------------------------------------------------------------------------------
	push {r6,lr}

rmtRacersAllocTextures:
	ldr r6, =0x13A160 @ Modified size of textures.dat (Original: 0x13ACCC)
	cmp r0, r6
	bne rmtRacersAllocGui
	ldr r0, rmtRacersHeapAddr
	pop {r6,pc}

rmtRacersAllocGui:
	ldr r6, =0x111484 @ Modified size of gui.dat (Original: 0x112088)
	cmp r0, r6
	bne rmtRacersAllocGameDat
	ldr r0, rmtRacersHeapAddr+4
	pop {r6,pc}

rmtRacersAllocGameDat:	@ game.dat
	ldr r0, rmtRacersHeapAddr+8
	pop {r6,pc}
.align 4
rmtRacersHeapAddr:
.word	0x09000000
.word	0x09140000
.word	0x09260000
.pool
@---------------------------------------------------------------------------------
	.arm
@---------------------------------------------------------------------------------
siezHeapAllocFunc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r6,lr}

	ldr r6, =0x1F1F24 @ Size of kr0000.ntfx
	cmp r1, r6
	ldreq r0, siezHeapAddr
	beq siezHeapAlloc_return
	ldr r6, =0x1F876C @ Size of kr0100.ntfx
	cmp r1, r6
	ldreq r0, siezHeapAddr+4
	beq siezHeapAlloc_return
	ldr r6, =0x020DB338
	bl	_blx_siezOrgFunction

siezHeapAlloc_return:
	ldmfd   sp!, {r6,pc}
_blx_siezOrgFunction:
	bx	r6
siezHeapAddr:
.word	0x09000000 @ Offset of kr0000.ntfx
.word	0x09200000 @ Offset of kr0100.ntfx
.pool
@---------------------------------------------------------------------------------
