@---------------------------------------------------------------------------------
	.global fourSwHeapAlloc
	.global fourSwHeapAllocExp
	.global fourSwHeapAddr
	.global fourSwLoadFromZeldatFile
	.global fourSwLoadFromZelmapFile
	.align	4
	.arm

.global fourSwHeapAllocLen
fourSwHeapAllocLen:
.word fourSwHeapAllocEnd - fourSwHeapAlloc

.global fourSwHeapAllocExpLen
fourSwHeapAllocExpLen:
.word fourSwHeapAllocExpEnd - fourSwHeapAllocExp

.global fourSwLoadFromZeldatFileLen
fourSwLoadFromZeldatFileLen:
.word fourSwLoadFromZeldatFileEnd - fourSwLoadFromZeldatFile

.global fourSwLoadFromZelmapFileLen
fourSwLoadFromZelmapFileLen:
.word fourSwLoadFromZelmapFileEnd - fourSwLoadFromZelmapFile

@---------------------------------------------------------------------------------
fourSwOrgFunction: .word 0
fourSwHeapAlloc:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4,lr}

	ldr r4, =0x1AFC7C @ Size of zeldat.bin
	cmp r0, r4
	ldreq r0, =0xE3710
	beq fourSwHeapAlloc_runOrgFunction

	ldr r4, =0x1086DC @ Size of zelmap.bin
	cmp r0, r4
	ldreq r0, =0x1858

fourSwHeapAlloc_runOrgFunction:
	str r0, [r6]
	ldr	r4, fourSwOrgFunction
	.word 0xE12FFF34 @ blx r4
	ldmfd   sp!, {r4,pc}
.pool
fourSwHeapAllocEnd:
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
fourSwOrgFunctionExp: .word 0
fourSwHeapAllocExp:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4,lr}

	ldr r4, =0x1AFC7C @ Size of zeldat.bin
	cmp r0, r4
	ldreq r0, fourSwHeapAddr_zeldat
	ldmeqfd   sp!, {r4,pc}

	ldr r4, =0xFA30 @ Size of zeldat_us_en.bin / zeldat_eu_en.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xF89C @ Size of zeldat_us_fr.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xF898 @ Size of zeldat_us_sp.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xF8AC @ Size of zeldat_eu_fr.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xF788 @ Size of zeldat_eu_gr.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xF9F8 @ Size of zeldat_eu_it.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xF888 @ Size of zeldat_eu_sp.bin
	cmp r0, r4
	beq fourSwHeapAllocExp_isZeldatLang

	ldr r4, =0xFDC0 @ Size of zeldat_jp.bin
	cmp r0, r4
fourSwHeapAllocExp_isZeldatLang:
	ldreq r0, fourSwHeapAddr_zeldat_lang
	ldmeqfd   sp!, {r4,pc}

	ldr r4, =0x1086DC @ Size of zelmap.bin
	cmp r0, r4
	ldreq r0, fourSwHeapAddr_zelmap
	ldmeqfd   sp!, {r4,pc}

	ldr r4, =0x20208 @ Size of us.kmsg
	cmp r0, r4
	beq fourSwHeapAllocExp_isKmsg

	ldr r4, =0x33310 @ Size of eu.kmsg
	cmp r0, r4
	beq fourSwHeapAllocExp_isKmsg

	ldr r4, =0xF638 @ Size of jp.kmsg
	cmp r0, r4
fourSwHeapAllocExp_isKmsg:
	ldreq r0, fourSwHeapAddr_kmsg
	ldmeqfd   sp!, {r4,pc}

	ldr r4, =0x1008 @ Size of font_ltn.nftr
	cmp r0, r4
	beq fourSwHeapAllocExp_isFont

	ldr r4, =0x15100 @ Size of font_jp.nftr
	cmp r0, r4
fourSwHeapAllocExp_isFont:
	ldreq r0, fourSwHeapAddr_font
	ldmeqfd   sp!, {r4,pc}

	ldr	r4, fourSwOrgFunctionExp
	.word 0xE12FFF34 @ blx r4
	ldmfd   sp!, {r4,pc}
fourSwHeapAddr:
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
fourSwHeapAllocExpEnd:
@---------------------------------------------------------------------------------

@.word 0x444C455A @ 'ZELD'
@---------------------------------------------------------------------------------
fourSwLoadFromZeldatFile:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r7,lr}

	ldr r4, =0xE3710
	cmp r3, r4
	ldmltfd   sp!, {r4-r7,pc}

	mov r4, r3 @ src
	mov r5, r2 @ len
	mov r6, r2 @ len (no compress bit)
	tst r6, #0x80000000
	subne r6, #0x80000000

	ldr r0, fourSwLoadFromZeldatFile_lruSrcCache
	cmp r0, #0
	bne fourSwLoadFromZeldatFile_lruCacheIsSetUp
	ldr r7, =0x6A0
	mov r0, r7
	mov r1, #0x20
	ldr r12, fourSw_calloc
	.word 0xE12FFF3C @ blx r12 (workaround as this file is compiled for arm7)

	adr r1, fourSwLoadFromZeldatFile_lruSrcCache
	str r0, [r1]

	mov r0, r7
	mov r1, #0x20
	ldr r12, fourSw_calloc
	.word 0xE12FFF3C @ blx r12

	adr r1, fourSwLoadFromZeldatFile_lruDstCache
	str r0, [r1]

fourSwLoadFromZeldatFile_lruCacheIsSetUp:
	mov r7, #0x4000
	cmp r6, r7
	bgt fourSwLoadFromZeldatFile_freeLastUsedData
	ldr r2, fourSwLoadFromZeldatFile_lruCacheSlotsAllocated
	cmp r2, #0
	beq fourSwLoadFromZeldatFile_lruCacheSearchDone
	mov r1, #0
	ldr r3, fourSwLoadFromZeldatFile_lruSrcCache
fourSwLoadFromZeldatFile_lruCacheSearchLoop:
	ldr r0, [r3, r1]
	cmp r0, r4
	ldreq r3, fourSwLoadFromZeldatFile_lruDstCache
	ldreq r7, [r3, r1]
	beq fourSwLoadFromZeldatFile_done

	add r1, #4
	cmp r1, r2
	blt fourSwLoadFromZeldatFile_lruCacheSearchLoop
	b fourSwLoadFromZeldatFile_lruCacheSearchDone

fourSwLoadFromZeldatFile_freeLastUsedData:
	ldr r0, fourSwLoadFromZeldatFile_lastUsedData
	cmp r0, #0
	beq fourSwLoadFromZeldatFile_lruCacheSearchDone
	ldr r12, fourSw_free
	.word 0xE12FFF3C @ blx r12

fourSwLoadFromZeldatFile_lruCacheSearchDone:
	sub sp, #0x48

	mov r0, sp
	ldr r12, fourSw_fileInit
	.word 0xE12FFF3C @ blx r12

	ldr r1, fourSw_zeldatBin
	ldr r12, fourSw_fileOpen
	.word 0xE12FFF3C @ blx r12

	mov r0, r6
	mov r1, #0x20
	ldr r12, fourSw_calloc
	.word 0xE12FFF3C @ blx r12
	cmp r6, r7
	mov r7, r0
	bgt fourSwLoadFromZeldatFile_setLastUsedData

	ldr r2, fourSwLoadFromZeldatFile_lruCacheSlotsAllocated
	ldr r1, fourSwLoadFromZeldatFile_lruSrcCache
	str r4, [r1, r2]
	ldr r1, fourSwLoadFromZeldatFile_lruDstCache
	str r7, [r1, r2]
	add r2, #4
	adr r1, fourSwLoadFromZeldatFile_lruCacheSlotsAllocated
	str r2, [r1]
	b fourSwLoadFromZeldatFile_cont

fourSwLoadFromZeldatFile_setLastUsedData:
	adr r0, fourSwLoadFromZeldatFile_lastUsedData
	str r7, [r0]

fourSwLoadFromZeldatFile_cont:
	mov r0, sp
	mov r1, r4
	ldr r12, fourSw_fileSeek
	.word 0xE12FFF3C @ blx r12

	mov r0, sp
	mov r1, r7
	mov r2, r6
	ldr r12, fourSw_fileRead
	.word 0xE12FFF3C @ blx r12

	mov r0, sp
	ldr r12, fourSw_fileClose
	.word 0xE12FFF3C @ blx r12

	add sp, #0x48

fourSwLoadFromZeldatFile_done:
	mov r0, r7
	mov r2, r5
	mov r3, r4
	ldmfd   sp!, {r4-r7,lr}
	mov r1, sp
	bx lr
fourSwLoadFromZeldatFile_lruSrcCache:
.word 0
fourSwLoadFromZeldatFile_lruDstCache:
.word 0
fourSwLoadFromZeldatFile_lruCacheSlotsAllocated:
.word 0
fourSwLoadFromZeldatFile_lastUsedData:
.word 0
fourSwLoadFromZeldatFileEnd:
@---------------------------------------------------------------------------------
fourSwLoadFromZelmapFile:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r1-r6,lr}

	tst r1, #1
	adr r1, fourSwLoadFromZelmapFile_loaded
	moveq r3, #0
	streq r3, [r1]
	adreq r1, fourSwLoadFromZelmapFile_savedSrc
	streq r0, [r1]
	addeq r0, r2
	ldmeqfd   sp!, {r1-r6,pc}

	ldr r1, [r1]
	cmp r1, #1
	ldmeqfd   sp!, {r1-r6,pc}

	ldr r4, fourSwLoadFromZelmapFile_savedSrc @ src
	mov r5, r0 @ len
	mov r6, r0 @ len (no compress bit)
	tst r6, #0x80000000
	subne r6, #0x80000000

fourSwLoadFromZelmapFile_freeLastUsedData:
	ldr r0, fourSwLoadFromZelmapFile_lastUsedData
	cmp r0, #0
	beq fourSwLoadFromZelmapFile_lruCacheSearchDone
	ldr r12, fourSw_free
	.word 0xE12FFF3C @ blx r12

fourSwLoadFromZelmapFile_lruCacheSearchDone:
	sub sp, #0x48

	mov r0, sp
	ldr r12, fourSw_fileInit
	.word 0xE12FFF3C @ blx r12

	ldr r1, fourSw_zelmapBin
	ldr r12, fourSw_fileOpen
	.word 0xE12FFF3C @ blx r12

	mov r0, r6
	mov r1, #0x20
	ldr r12, fourSw_calloc
	.word 0xE12FFF3C @ blx r12
	mov r7, r0

fourSwLoadFromZelmapFile_setLastUsedData:
	adr r0, fourSwLoadFromZelmapFile_lastUsedData
	str r7, [r0]

fourSwLoadFromZelmapFile_cont:
	mov r0, sp
	mov r1, r4
	ldr r12, fourSw_fileSeek
	.word 0xE12FFF3C @ blx r12

	mov r0, sp
	mov r1, r7
	mov r2, r6
	ldr r12, fourSw_fileRead
	.word 0xE12FFF3C @ blx r12

	mov r0, sp
	ldr r12, fourSw_fileClose
	.word 0xE12FFF3C @ blx r12

	add sp, #0x48

fourSwLoadFromZelmapFile_done:
	adr r1, fourSwLoadFromZelmapFile_loaded
	mov r0, #1
	str r0, [r1]

	mov r0, r5
	ldmfd   sp!, {r1-r6,pc}
fourSwLoadFromZelmapFile_loaded:
.word 0
fourSwLoadFromZelmapFile_savedSrc:
.word 0
fourSwLoadFromZelmapFile_lastUsedData:
.word 0
.pool
@---------------------------------------------------------------------------------
fourSw_fileInit:
.word 0x020102D4
fourSw_fileClose:
.word 0x02010BA0
fourSw_fileSeek:
.word 0x02010C38
fourSw_fileRead:
.word 0x02010C84
fourSw_fileOpen:
.word 0x02010EDC
.global fourSw_calloc
fourSw_calloc:
.word 0x0208E144
.global fourSw_free
fourSw_free:
.word 0x0208E168
.global fourSw_zeldatBin
fourSw_zeldatBin:
.word 0x0211ABE8
.global fourSw_zelmapBin
fourSw_zelmapBin:
.word 0x0211ABF4
@---------------------------------------------------------------------------------
fourSwLoadFromZelmapFileEnd:
