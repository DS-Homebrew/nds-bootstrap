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

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/
#include <nds/asminc.h>

	.text
	.align 4

	.thumb

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC swiDelay
@---------------------------------------------------------------------------------
	swi	0x03
	bx	lr

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC swiSleep
@---------------------------------------------------------------------------------
	swi	0x07
	bx	lr

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC swiDivMod
@---------------------------------------------------------------------------------
	push	{r2, r3}
	swi	0x09
	pop	{r2, r3}
	str	r0, [r2]
	str	r1, [r3]
	bx	lr
