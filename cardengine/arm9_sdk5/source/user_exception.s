#include <nds/asminc.h>

	.text

	.arm

	
BEGIN_ASM_FUNC user_exception
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
	ldr r0, =exceptionRegisters
    ldr r4, [r0, #0x3C] //pc + 4
    ldr r7, [r0, #0x38] //lr
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