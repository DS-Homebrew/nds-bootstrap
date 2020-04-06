@   NitroHax -- Cheat tool for the Nintendo DS
@   Copyright (C) 2008  Michael "Chishm" Chisholm
@
@   This program is free software: you can redistribute it and/or modify
@   it under the terms of the GNU General Public License as published by
@   the Free Software Foundation, either version 3 of the License, or
@   (at your option) any later version.
@
@   This program is distributed in the hope that it will be useful,
@   but WITHOUT ANY WARRANTY; without even the implied warranty of
@   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@   GNU General Public License for more details.
@
@   You should have received a copy of the GNU General Public License
@   along with this program.  If not, see <http://www.gnu.org/licenses/>.

@ Clears ICache and Dcache, and resets the protection units
@ Originally written by Darkain, modified by Chishm

.arm
.global arm9_clearCache

arm9_clearCache:
	@ Clean and flush cache
	mov r1, #0                   
	outer_loop:                  
		mov r0, #0                  
		inner_loop:                 
			orr r2, r1, r0             
			mcr p15, 0, r2, c7, c14, 2 
			add r0, r0, #0x20          
			cmp r0, #0x400             
		bne inner_loop              
		add r1, r1, #0x40000000     
		cmp r1, #0x0                
	bne outer_loop               

	mov r3, #0                  
	mcr p15, 0, r3, c7, c5, 0		@ Flush ICache
	mcr p15, 0, r3, c7, c6, 0		@ Flush DCache
	mcr p15, 0, r3, c7, c10, 4		@ empty write buffer

	bx lr

