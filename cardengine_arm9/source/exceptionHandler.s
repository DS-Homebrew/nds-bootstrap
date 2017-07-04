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
user_exception:
    mov r12, #0x04000000
    mov r11, #0x20000
    str r11, [r12]
    mov r11, #0x80
    strb r11, [r12, #0x240]
    mov r11, #0x6800000
    mov r10, #0x8000
    orr r10, r10, #0x1F
    mov r9, #(128 * 1024)
    //clear screen
1:
    strh r10, [r11], #2
    subs r9, #2
    bne 1b
	ldr r4, =0x02FFFD90
	ldr	r7,[r4,#0xC]
	ldr	r4,[r4,#8]
    mov r5, #8
    mov r6, #0x6800000
    mov r8, #0
2:
    mov r0, r4, lsr #28
    mov r1, r6
    bl put_dotrow
    mov r4, r4, lsl #4
    add r6, #2048
    subs r5, #1
    bne 2b

    cmp r8, #1
    beq .
    add r6, #4096
    mov r4, r7
    mov r5, #8
    mov r8, #1
    b 2b

    //r0 = count
    //r1 = at
put_dotrow:
    ldr r2,= 0xFFFF
    add r1, #2 //skip one pixel
1:
    subs r0, #1
    strgeh r2, [r1], #4
    bge 1b
    bx lr

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
