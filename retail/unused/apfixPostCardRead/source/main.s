@---------------------------------------------------------------------------------
	.align	4
	.arm
@---------------------------------------------------------------------------------
.ascii ".PCK"
.word 0 @ This is where the amount would be entered, but is unused due to having to change it manually, so a "END" string is placed at the end of the list to determine the amount
.space 0x8
@---------------------------------------------------------------------------------
.ascii "AZLE"
.hword 0xFFFF
.word stylSavUsaOverlayApFix
.word stylSavUsaOverlayApFix_end-stylSavUsaOverlayApFix
.hword 0

.ascii "AZLK"
.hword 0xFFFF
.word stylSavKorOverlayApFix
.word stylSavKorOverlayApFix_end-stylSavKorOverlayApFix
.hword 0

.ascii "AZLP"
.hword 0xFFFF
.word stylSavEurOverlayApFix
.word stylSavEurOverlayApFix_end-stylSavEurOverlayApFix
.hword 0

.ascii "B6ZE"
.hword 0xFFFF
.word mmzcOverlayApFix
.word mmzcOverlayApFix_end-mmzcOverlayApFix
.hword 0

.ascii "B6ZJ"
.hword 0xFFFF
.word mmzcOverlayApFix
.word mmzcOverlayApFix_end-mmzcOverlayApFix
.hword 0

.ascii "B6ZP"
.hword 0xFFFF
.word mmzcOverlayApFix
.word mmzcOverlayApFix_end-mmzcOverlayApFix
.hword 0

.ascii "BRJE"
.hword 0xFFFF
.word rdntHistEngOverlayApFix
.word rdntHistEngOverlayApFix_end-rdntHistEngOverlayApFix
.hword 0

.ascii "BRJJ"
.hword 0xFFFF
.word rdntHistJpnOverlayApFix
.word rdntHistJpnOverlayApFix_end-rdntHistJpnOverlayApFix
.hword 0

.ascii "VCDJ"
.hword 0xFFFF
.word solatoroboJpnOverlayApFix
.word solatoroboJpnOverlayApFix_end-solatoroboJpnOverlayApFix
.hword 0

.ascii "END"
.byte 0
@---------------------------------------------------------------------------------
stylSavUsaOverlayApFix: @ overlay9_156
	ldr r0, =0x02160F80
	ldrb r1, [r0, #0x59C]
	mov r2, #0x0C
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x6F7]
	mov r2, #0x10
	cmp r2, r1
	bxne lr
	mov r2, #0x36
	strb r2, [r0, #0x59C]
	mov r2, #0x11
	strb r2, [r0, #0x6F7]
	bx lr
.pool
stylSavUsaOverlayApFix_end:
@---------------------------------------------------------------------------------
stylSavKorOverlayApFix: @ overlay9_155
	ldr r0, =0x0216EF00
	ldrh r1, [r0, #0x9E]
	ldr r2, =0xE0AA
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x149]
	mov r2, #0xBF
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x14B]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldr r2, =0x39A9
	strh r2, [r0, #0x9E]
	mov r2, #0xBE
	strb r2, [r0, #0x149]
	mov r2, #0x36
	strb r2, [r0, #0x14B]
	bx lr
.pool
stylSavKorOverlayApFix_end:
@---------------------------------------------------------------------------------
stylSavEurOverlayApFix: @ overlay9_156
	ldr r0, =0x02161100
	ldrb r1, [r0, #0x599]
	mov r2, #0x0C
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x6F4]
	mov r2, #0x10
	cmp r2, r1
	bxne lr
	mov r2, #0x36
	strb r2, [r0, #0x599]
	mov r2, #0x11
	strb r2, [r0, #0x6F4]
	bx lr
.pool
stylSavEurOverlayApFix_end:
@---------------------------------------------------------------------------------
mmzcOverlayApFix: @ overlay9_8
	ldr r0, =0x0215A100
	ldrh r1, [r0, #0x9E]
	ldr r2, =0xE0AA
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x148]
	mov r2, #0xBF
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x14A]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldr r2, =0x39A9
	strh r2, [r0, #0x9E]
	mov r2, #0xBE
	strb r2, [r0, #0x148]
	mov r2, #0x36
	strb r2, [r0, #0x14A]
	bx lr
.pool
mmzcOverlayApFix_end:
@---------------------------------------------------------------------------------
rdntHistEngOverlayApFix: @ overlay9_0
	ldr r0, =0x02176A00
	ldrb r1, [r0, #0x6D]
	mov r2, #0x8F
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x6F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x10D]
	mov r2, #0x45
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x10F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x1AD]
	mov r2, #0x1A
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x1AF]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x24D]
	mov r2, #0x9C
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x24F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	mov r2, #0x8E
	strb r2, [r0, #0x6D]
	mov r2, #0x36
	strb r2, [r0, #0x6F]
	mov r2, #0x44
	strb r2, [r0, #0x10D]
	mov r2, #0x36
	strb r2, [r0, #0x10F]
	mov r2, #0x19
	strb r2, [r0, #0x1AD]
	mov r2, #0x36
	strb r2, [r0, #0x1AF]
	mov r2, #0x9B
	strb r2, [r0, #0x24D]
	mov r2, #0x36
	strb r2, [r0, #0x24F]
	bx lr
.pool
rdntHistEngOverlayApFix_end:
@---------------------------------------------------------------------------------
rdntHistJpnOverlayApFix: @ overlay9_0
	ldr r0, =0x02176E00
	ldrb r1, [r0, #0x6D]
	mov r2, #0x8F
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x6F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x10D]
	mov r2, #0x45
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x10F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x1AD]
	mov r2, #0x1A
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x1AF]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x24D]
	mov r2, #0x9C
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x24F]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	mov r2, #0x8E
	strb r2, [r0, #0x6D]
	mov r2, #0x36
	strb r2, [r0, #0x6F]
	mov r2, #0x44
	strb r2, [r0, #0x10D]
	mov r2, #0x36
	strb r2, [r0, #0x10F]
	mov r2, #0x19
	strb r2, [r0, #0x1AD]
	mov r2, #0x36
	strb r2, [r0, #0x1AF]
	mov r2, #0x9B
	strb r2, [r0, #0x24D]
	mov r2, #0x36
	strb r2, [r0, #0x24F]
	bx lr
.pool
rdntHistJpnOverlayApFix_end:
@---------------------------------------------------------------------------------
solatoroboJpnOverlayApFix: @ overlay9_4
	ldr r0, =0x021E39E0
	ldrb r1, [r0, #0x27A]
	mov r2, #0xB4
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x27C]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x319]
	mov r2, #0xE1
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #0x31B]
	mov r2, #0xE0
	cmp r2, r1
	bxne lr
	mov r2, #0xB3
	strb r2, [r0, #0x27A]
	mov r2, #0x36
	strb r2, [r0, #0x27C]
	mov r2, #0xE0
	strb r2, [r0, #0x319]
	mov r2, #0x36
	strb r2, [r0, #0x31B]
	bx lr
.pool
solatoroboJpnOverlayApFix_end:
