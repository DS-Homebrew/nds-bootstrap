@---------------------------------------------------------------------------------
	.global wordSrchStreamWavFile
	.align	4
	.arm

.global wordSrchStreamWavFileLen
wordSrchStreamWavFileLen:
.word wordSrchStreamWavFileEnd - wordSrchStreamWavFile

@---------------------------------------------------------------------------------
wordSrchStreamWavFile:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r4-r5,lr}

	mov r5, r2 @ len
	mov r4, r1 @ dst
	add r1, r0, #0x2C @ src
	mov r0, r9
	mov r2, #0
	ldr r12, wordSrch_fileSeek
	.word 0xE12FFF3C @ blx r12

	mov r0, r9
	mov r1, r4 @ dst
	mov r2, r5 @ len
	ldr r12, wordSrch_fileRead
	.word 0xE12FFF3C @ blx r12

	ldmfd   sp!, {r4-r5,pc}
@---------------------------------------------------------------------------------
wordSrch_fileSeek:
.word 0x02024940
wordSrch_fileRead:
.word 0x0202496C
@---------------------------------------------------------------------------------
wordSrchStreamWavFileEnd:
