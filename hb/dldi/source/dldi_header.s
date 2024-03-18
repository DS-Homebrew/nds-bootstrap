#include <nds/arm9/dldi_asm.h>

@---------------------------------------------------------------------------------
	.section ".crt0","ax"
@---------------------------------------------------------------------------------
	.global _start
	.global dataStartOffset
	.global ioType
	.global word_command
	.global word_params
	.global words_msg
	.global dldi_bss_end
	.global allocated_space
	.align	4
	.arm

@---------------------------------------------------------------------------------
@ Driver patch file standard header -- 16 bytes
	.word	0xBF8DA5ED		@ Magic number to identify this region
	.asciz	" Chishm"		@ Identifying Magic string (8 bytes with null terminator)
	.byte	0x01			@ Version number
	.byte	DLDI_SIZE_8KB	@ 8KiB	@ Log [base-2] of the size of this driver in bytes.
	.byte	FIX_GOT | FIX_GLUE | FIX_BSS	@ Sections to fix
	allocated_space:
	.byte 	0x00			@ Space allocated in the application, not important here.
	
@---------------------------------------------------------------------------------
@ Text identifier - can be anything up to 47 chars + terminating null -- 48 bytes
	.align	4
	.asciz "DSI 3DS sd card"
	
@---------------------------------------------------------------------------------
@ Offsets to important sections within the data	-- 32 bytes
	.align	6
	dataStartOffset:
	.word   __text_start	@ data start
	.word   __data_end		@ data end
	.word	__glue_start	@ Interworking glue start	-- Needs address fixing
	.word	__glue_end		@ Interworking glue end
	.word   __got_start		@ GOT start					-- Needs address fixing
	.word   __got_end		@ GOT end
	.word   __bss_start		@ bss start					-- Needs setting to zero
	dldi_bss_end:
	.word   __bss_end		@ bss end

@---------------------------------------------------------------------------------
@ IO_INTERFACE data -- 32 bytes
	ioType:
	.ascii	"DSID"			@ ioType
	.word	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_SLOT_GBA
	.word	startup			@ 
	.word	isInserted		@ 
	.word	readSectors		@   Function pointers to standard device driver functions
	.word	writeSectors	@ 
	.word	clearStatus		@ 
	.word	shutdown		@ 
	
	word_command:
	.word	0x00000000
	word_params:
	.word	0x00000000
	words_msg:
	.word	0x00000000
	.word	0x00000000
	.word	0x00000000
	.word	0x00000000
	
@---------------------------------------------------------------------------------
_start:
@---------------------------------------------------------------------------------
	.align
	.pool
	.end
@---------------------------------------------------------------------------------
