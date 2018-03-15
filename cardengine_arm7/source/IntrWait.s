	.arch	armv5te
	.cpu	arm946e-s

	.text
	.arm
	.align


//---------------------------------------------------------------------------------
.global swiIntrWaitAux
.type	swiIntrWaitAux STT_FUNC 
swiIntrWaitAux:
//---------------------------------------------------------------------------------
	stmfd	sp!, {lr}
	mov r2, r1
	swi	#(4<<16)
	bx	lr



