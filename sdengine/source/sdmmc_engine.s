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

.arm

.global sdmmc_engine_start
.global sdmmc_engine_end
.global intr_orig_return_offset
.global sdmmc_engine_size


sdmmc_engine_size:
	.word	sdmmc_engine_end - sdmmc_engine_start

intr_orig_return_offset:
	.word	intr_orig_return - sdmmc_engine_start
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

sdmmc_engine_start:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start
	ldr 	r0,	intr_orig_return
	bx  	r0
  
code_handler_start:
	ldr	r3, =runSdMmcEngineCheck
	bl	_blx_r3_stub		@ jump to user code
  
  @ exit after return
	b	exit
  
@---------------------------------------------------------------------------------
_blx_r3_stub:
@---------------------------------------------------------------------------------
	bx	r3		

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

exit:	
	ldmia	sp!,	{r0-r12} 
	ldmia	sp!,	{lr}
	bx		lr

intr_orig_return:
	.word	0x00000000

.pool

sdmmc_engine_end:


