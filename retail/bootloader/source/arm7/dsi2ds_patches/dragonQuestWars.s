@---------------------------------------------------------------------------------
	.global dqWarsHeapAlloc
	.global dqWarsHeapFree
	.global dqWarsSlot2HeapAlloc
	.global dqWarsSlot2HeapFree
	.align	4
	.arm

.global dqWarsHeapAllocLen
dqWarsHeapAllocLen:
.word dqWarsHeapAllocEnd - dqWarsHeapAlloc

.global dqWarsHeapFreeLen
dqWarsHeapFreeLen:
.word dqWarsHeapFreeEnd - dqWarsHeapFree

@---------------------------------------------------------------------------------
dqWarsHeapAlloc:
@---------------------------------------------------------------------------------
	mov r0, r6
	cmp r0, #0
	moveq r0, r8
	ldreq r3, dqWarsSlot2HeapAlloc
	.word 0x012FFF33 @ blxeq r3

	ldmfd   sp!, {r3-r9,pc}
dqWarsSlot2HeapAlloc: .word 0
dqWarsHeapAllocEnd:
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
