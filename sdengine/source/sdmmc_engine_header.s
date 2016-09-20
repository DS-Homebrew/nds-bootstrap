@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global sdmmc_engine_start
.global sdmmc_engine_start_sync
.global sdmmc_engine_end
.global sdmmc_intr_orig_return_offset
.global sdmmc_intr_sync_orig_return_offset
.global sdmmc_engine_size


sdmmc_engine_size:
	.word	sdmmc_engine_end - sdmmc_engine_start

sdmmc_intr_orig_return_offset:
	.word	intr_orig_return - sdmmc_engine_start
	
sdmmc_intr_sync_orig_return_offset:
	.word	intr_sync_orig_return - sdmmc_engine_start
	
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
  
sdmmc_engine_start_sync:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start
	ldr 	r0,	intr_sync_orig_return
	bx  	r0
	
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

intr_sync_orig_return:
	.word	0x00000000

.pool

sdmmc_engine_end:


