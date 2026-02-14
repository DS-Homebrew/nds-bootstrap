@---------------------------------------------------------------------------------
	.global dqWarsHeapAlloc
	.global dqWarsHeapAlloc2
	.global dqWarsHeapFree
	.global dqWarsSlot2HeapAlloc
	.global dqWarsSlot2HeapFree
	.global dqWarsRestoreCpsrStateFunc
	.align	4
	.arm

.global dqWarsHeapAllocLen
dqWarsHeapAllocLen:
.word dqWarsHeapAllocEnd - dqWarsHeapAlloc

.global dqWarsHeapAlloc2Len
dqWarsHeapAlloc2Len:
.word dqWarsHeapAlloc2End - dqWarsHeapAlloc2

.global dqWarsHeapFreeLen
dqWarsHeapFreeLen:
.word dqWarsHeapFreeEnd - dqWarsHeapFree

@---------------------------------------------------------------------------------
dqWarsHeapAlloc:
@---------------------------------------------------------------------------------
	cmp r0, #0
	moveq r0, r8
	ldreq r3, dqWarsSlot2HeapAlloc
	.word 0x012FFF33 @ blxeq r3
	mov r6, r0
	mov r0, r7
	ldr r3, dqWarsRestoreCpsrStateFunc
	.word 0xE12FFF33 @ blx r3
	mov r0, r6
	ldmfd   sp!, {r3-r9,pc}
dqWarsHeapAllocEnd:
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dqWarsHeapAlloc2:
@---------------------------------------------------------------------------------
	cmp r0, #0
	moveq r0, r9
	ldreq r3, dqWarsSlot2HeapAlloc
	.word 0x012FFF33 @ blxeq r3
	mov r11, r0
	mov r0, r6
	ldr r3, dqWarsRestoreCpsrStateFunc
	.word 0xE12FFF33 @ blx r3
	mov r0, r11
	ldmfd   sp!, {r3-r11,pc}
dqWarsSlot2HeapAlloc: .word 0
dqWarsRestoreCpsrStateFunc: .word 0
dqWarsHeapAlloc2End:
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dqWarsHeapFree:
@---------------------------------------------------------------------------------
	cmp r1, #0x08000000
	blt dqWarsHeapFree_org
	mov r0, r1
	ldr r3, dqWarsSlot2HeapFree
	.word 0xE12FFF33 @ blx r3
	ldmfd   sp!, {r3-r7,pc}

dqWarsHeapFree_org:
	mov r7, r0
	ldr r4, [r7]

	ldr pc, =0x01FFA26C
dqWarsSlot2HeapFree: .word 0
.pool
dqWarsHeapFreeEnd:
@---------------------------------------------------------------------------------
