// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
//
// ABI:
//    __aeabi_memclr, __aeabi_memclr4, __aeabi_memclr8,
//    __aeabi_memset, __aeabi_memset4, __aeabi_memset8
// Standard:
//    memset
// Support:
//    __ndsabi_wordset4, __ndsabi_lwordset4, __ndsabi_memset1

#include "asminc.h"

#include "macros.inc"

    .syntax unified

    .arm


BEGIN_ASM_FUNC __aeabi_memclr

    mov     r2, #0
    b       __aeabi_memset


BEGIN_ASM_FUNC __aeabi_memclr8
BEGIN_ASM_FUNC __aeabi_memclr4

    mov     r2, #0
    b       __ndsabi_wordset4


BEGIN_ASM_FUNC __aeabi_memset

    @ < 8 bytes probably won't be aligned: go byte-by-byte
    cmp     r1, #8
    blt     __ndsabi_memset1

    @ Copy head to align to next word
    rsb     r3, r0, #4
    joaobapt_test r3
    strbmi  r2, [r0], #1
    submi   r1, r1, #1
    strbcs  r2, [r0], #1
    strbcs  r2, [r0], #1
    subcs   r1, r1, #2


BEGIN_ASM_FUNC_NO_SECTION __aeabi_memset8
BEGIN_ASM_FUNC_NO_SECTION __aeabi_memset4

    lsl     r2, r2, #24
    orr     r2, r2, r2, lsr #8
    orr     r2, r2, r2, lsr #16


BEGIN_ASM_FUNC_NO_SECTION __ndsabi_wordset4

    mov     r3, r2


BEGIN_ASM_FUNC_NO_SECTION __ndsabi_lwordset4

    @ 16 words is roughly the threshold when lwordset is slower
    cmp     r1, #64
    blt     .Lset_2_words

    @ 8 word set
    push    {r4-r9}
    mov     r4, r2
    mov     r5, r3
    mov     r6, r2
    mov     r7, r3
    mov     r8, r2
    mov     r9, r3

.Lset_8_words:
    subs    r1, r1, #32
    stmiage r0!, {r2-r9}
    bgt     .Lset_8_words
    pop     {r4-r9}
    bxeq    lr

    @ Fixup remaining
    add     r1, r1, #32
.Lset_2_words:
    subs    r1, r1, #8
    stmiage r0!, {r2-r3}
    bgt     .Lset_2_words
    bxeq    lr

    @ Test for remaining word
    adds    r1, r1, #4
    strge   r2, [r0], #4
    bxeq    lr

    @ Set tail
    joaobapt_test r1
    strhcs  r2, [r0], #2
    strbmi  r2, [r0], #1
    bx      lr


BEGIN_ASM_FUNC __ndsabi_memset1

    subs    r1, r1, #1
    strbge  r2, [r0], #1
    bgt     __ndsabi_memset1
    bx      lr


@BEGIN_ASM_FUNC abi_memset

@    mov     r3, r1
@    mov     r1, r2
@    mov     r2, r3
@    push    {r0, lr}
@    bl      __aeabi_memset
@    pop     {r0, lr}
@    bx      lr
