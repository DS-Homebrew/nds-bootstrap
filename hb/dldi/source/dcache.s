/*---------------------------------------------------------------------------------

  Copyright (C) 2005
  	Michael Noland (joat)
  	Jason Rogers (dovoto)
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

//---------------------------------------------------------------------------------
	.arm
//---------------------------------------------------------------------------------
BEGIN_ASM_FUNC DC_FlushRange
/*---------------------------------------------------------------------------------
	Clean and invalidate a range
---------------------------------------------------------------------------------*/
	add	r1, r1, r0
	bic	r0, r0, #(CACHE_LINE_SIZE - 1)
.flush:
	mcr	p15, 0, r0, c7, c14, 1		@ clean and flush address
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	.flush

drainWriteBuffer:
	mov     r0, #0
	mcr     p15, 0, r0, c7, c10, 4		@ drain write buffer
	bx	lr

//---------------------------------------------------------------------------------
BEGIN_ASM_FUNC DC_InvalidateAll
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
	mov	r0, #0
	mcr	p15, 0, r0, c7, c6, 0
	bx	lr

//---------------------------------------------------------------------------------
BEGIN_ASM_FUNC DC_InvalidateRange
/*---------------------------------------------------------------------------------
	Invalidate a range
---------------------------------------------------------------------------------*/
	add	r1, r1, r0
	tst	r0, #CACHE_LINE_SIZE - 1
	mcrne	p15, 0, r0, c7, c10, 1		@ clean D entry
	tst	r1, #CACHE_LINE_SIZE - 1
	mcrne	p15, 0, r1, c7, c10, 1		@ clean D entry
	bic	r0, r0, #CACHE_LINE_SIZE - 1
.invalidate:
	mcr	p15, 0, r0, c7, c6, 1
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	.invalidate
	bx	lr
