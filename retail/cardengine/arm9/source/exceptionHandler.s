/*---------------------------------------------------------------------------------

  Copyright (C) 2005
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

	.text

	.arm

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC getCPSR
@---------------------------------------------------------------------------------
	mrs	r0,cpsr
	bx	r14

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC enterException
@---------------------------------------------------------------------------------
	// store context
#ifndef NODSIWARE
    push {r12}
#endif
	adr	r12, exceptionRegisters
	stmia	r12,{r0-r11}

#ifndef NODSIWARE
	// bios exception stack
	ldr 	r0, =0x027FFD90

	// grab stored r12 from bios exception stack
	adr	r1, reg12
	ldr r0, [r0, #4]
	str r0, [r1]

    push {lr}
	bl newSlot2Access
    pop {lr}
	cmp r0, #1
    bne enterException_cont

	// bios exception stack
	ldr 	r0, =0x027FFD90

	// store r12 to bios exception stack
	adr	r1, reg12
	ldr r1, [r1]
	str r1, [r0, #4]

	adr	r12, exceptionRegisters
	ldmia	r12,{r0-r11}

    pop {r12}
	bx lr

enterException_cont:
    pop {r12}
	adr	r12, exceptionRegisters
#endif

	str	r13,[r12,#oldStack - exceptionRegisters]
	// assign a stack
	adr	r13, exceptionStack
	ldr	r13,[r13]

	// renable MPU
	mrc	p15,0,r0,c1,c0,0
	orr	r0,r0,#1
	mcr	p15,0,r0,c1,c0,0

	// bios exception stack
	ldr 	r0, =0x027FFD90

	// grab r15 from bios exception stack
	ldr	r2,[r0,#8]
	str	r2,[r12,#reg15 - exceptionRegisters]

	// grab stored r12 and SPSR from bios exception stack
	ldmia	r0,{r2,r12}


	// grab banked registers from correct processor mode
	mrs	r3,cpsr
	bic	r4,r3,#0x1F
	and	r2,r2,#0x1F

	// Check for user mode & use system mode instead
	cmp	r2, #0x10
	moveq	r2, #0x1F

	orr	r4,r4,r2
	msr	cpsr,r4
	ldr	r0,=reg12
	stmia	r0,{r12-r14}
	msr	cpsr,r3

	// Get C function & call it
	adr	r12, exceptionC
	ldr	r12,[r12]
	blxne	r12

	// restore registers
	adr	r12, exceptionRegisters
	ldmia	r12,{r0-r11}
	ldr	r13,[r12,#oldStack - exceptionRegisters]

	// return through bios
	mov	pc,lr

@---------------------------------------------------------------------------------
	.global exceptionC
@---------------------------------------------------------------------------------
exceptionC:
@---------------------------------------------------------------------------------
	.word	0x00000000
@---------------------------------------------------------------------------------
	.global exceptionStack
@---------------------------------------------------------------------------------
exceptionStack:
@---------------------------------------------------------------------------------
	.word	0x00000000
@---------------------------------------------------------------------------------
	.global exceptionRegisters
@---------------------------------------------------------------------------------
exceptionRegisters:
@---------------------------------------------------------------------------------
	.space	12 * 4
reg12:		.word	0
reg13:		.word	0
reg14:		.word	0
reg15:		.word	0
oldStack:	.word	0