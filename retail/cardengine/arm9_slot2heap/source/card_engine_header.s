@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.align	4
	.arm

card_engine_start:

	b MyMalloc
	b MyFree

.global memory
.global memorySize
memory:
.word 0x09000000
memorySize:
.word 0x0077C000

card_engine_end:
