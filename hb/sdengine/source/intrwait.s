/*---------------------------------------------------------------------------------

  Copyright (C) 2006 - 2016
    Dave Murphy (WinterMute)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you
     must not claim that you wrote the original software. If you use
     this software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and
     must not be misrepresented as being the original software.
  3. This notice may not be removed or altered from any source
     distribution.

---------------------------------------------------------------------------------*/
#include <nds/asminc.h>

	.arm
	.cpu arm7tdmi
	.text
@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC swiWaitForVBlank
@---------------------------------------------------------------------------------
	mov	r0, #1
	mov	r1, #1
	mov	r2, #0
	nop
@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC swiIntrWait
@---------------------------------------------------------------------------------

	stmfd	sp!, {lr}
	cmp	r0, #0
	blne	testirq

wait_irq:
	swi	#(6<<16)
	bl	testirq
	beq	wait_irq
	ldmfd	sp!, {lr}
	bx	lr

testirq:
	mov	r12, #0x4000000
	strb	r12, [r12,#0x208]
	ldr	r3, [r12,#-8]
	ands	r0, r1,	r3
	eorne	r3, r3,	r0
	strne	r3, [r12,#-8]
	mov	r0, #1
	strb	r0, [r12,#0x208]
	bx	lr
