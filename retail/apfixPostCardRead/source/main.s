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

.ascii "BOEJ"
.hword 0xFFFF
.word ie3OgreOverlayApFix
.word ie3OgreOverlayApFix_end-ie3OgreOverlayApFix
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

.ascii "CSGJ"
.hword 0xFFFF
.word saga2OverlayApFix
.word saga2OverlayApFix_end-saga2OverlayApFix
.hword 0

.ascii "IPGD"
.hword 0xFFFF
.word hgssGerOverlayApFix
.word hgssGerOverlayApFix_end-hgssGerOverlayApFix
.hword 0

.ascii "IPGE"
.hword 0xFFFF
.word hgssEngOverlayApFix
.word hgssEngOverlayApFix_end-hgssEngOverlayApFix
.hword 0

.ascii "IPGF"
.hword 0xFFFF
.word ssFreOverlayApFix
.word ssFreOverlayApFix_end-ssFreOverlayApFix
.hword 0

.ascii "IPGI"
.hword 0xFFFF
.word hgssItaOverlayApFix
.word hgssItaOverlayApFix_end-hgssItaOverlayApFix
.hword 0

.ascii "IPGJ"
.hword 0xFFFF
.word hgssJpnOverlayApFix
.word hgssJpnOverlayApFix_end-hgssJpnOverlayApFix
.hword 0

.ascii "IPGK"
.hword 0xFFFF
.word ssKorOverlayApFix
.word ssKorOverlayApFix_end-ssKorOverlayApFix
.hword 0

.ascii "IPGS"
.hword 0xFFFF
.word ssSpaOverlayApFix
.word ssSpaOverlayApFix_end-ssSpaOverlayApFix
.hword 0

.ascii "IPKD"
.hword 0xFFFF
.word hgssGerOverlayApFix
.word hgssGerOverlayApFix_end-hgssGerOverlayApFix
.hword 0

.ascii "IPKE"
.hword 0xFFFF
.word hgssEngOverlayApFix
.word hgssEngOverlayApFix_end-hgssEngOverlayApFix
.hword 0

.ascii "IPKF"
.hword 0xFFFF
.word hgFreOverlayApFix
.word hgFreOverlayApFix_end-hgFreOverlayApFix
.hword 0

.ascii "IPKI"
.hword 0xFFFF
.word hgssItaOverlayApFix
.word hgssItaOverlayApFix_end-hgssItaOverlayApFix
.hword 0

.ascii "IPKJ"
.hword 0xFFFF
.word hgssJpnOverlayApFix
.word hgssJpnOverlayApFix_end-hgssJpnOverlayApFix
.hword 0

.ascii "IPKK"
.hword 0xFFFF
.word hgKorOverlayApFix
.word hgKorOverlayApFix_end-hgKorOverlayApFix
.hword 0

.ascii "IPKS"
.hword 0xFFFF
.word hgSpaOverlayApFix
.word hgSpaOverlayApFix_end-hgSpaOverlayApFix
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
ie3OgreOverlayApFix: @ overlay9_129
	ldr r0, =0x0212A9C0
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
ie3OgreOverlayApFix_end:
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
saga2OverlayApFix: @ overlay9_2
	ldr r0, =0x0213C9C0+0x22790
	ldr r1, [r0]
	ldr r2, =0xE28DB020
	cmp r2, r1
	bxne lr
	ldr r1, [r0, #4]
	ldr r2, =0xEA00000C
	cmp r2, r1
	bxne lr
	mov r2, #0x37
	strb r2, [r0, #4]
	bx lr
.pool
saga2OverlayApFix_end:
@---------------------------------------------------------------------------------
hgssGerOverlayApFix: @ overlay9_1
	ldr r0, =0x021E58E0+0x218
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssGerOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgssGerOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
hgssGerOverlayApFix_end:
@---------------------------------------------------------------------------------
hgssEngOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5900+0x219
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssEngOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgssEngOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
hgssEngOverlayApFix_end:
@---------------------------------------------------------------------------------
ssFreOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5920+0x218
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne ssFreOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
ssFreOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
ssFreOverlayApFix_end:
@---------------------------------------------------------------------------------
hgFreOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5920+0x217
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgFreOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgFreOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
hgFreOverlayApFix_end:
@---------------------------------------------------------------------------------
hgssItaOverlayApFix: @ overlay9_1
	ldr r0, =0x021E58A0+0x219
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssItaOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgssItaOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
hgssItaOverlayApFix_end:
@---------------------------------------------------------------------------------
ssSpaOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5940+0x218
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne ssSpaOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
ssSpaOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
ssSpaOverlayApFix_end:
@---------------------------------------------------------------------------------
hgSpaOverlayApFix: @ overlay9_1
	ldr r0, =0x021E5920+0x218
	ldrb r1, [r0]
	mov r2, #0x28
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgSpaOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgSpaOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
hgSpaOverlayApFix_end:
@---------------------------------------------------------------------------------
hgssJpnOverlayApFix: @ overlay9_1 + overlay9_122
	ldr r0, =0x021E4E40+0x21A
	ldrb r1, [r0]
	mov r2, #0x53
	cmp r2, r1
	bne hgssJpnOverlayApFix_check122
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgssJpnOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	b hgssJpnOverlayApFix_check122
hgssJpnOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bne hgssJpnOverlayApFix_check122
	mov r2, #0xE0
	strb r2, [r0, #3]
hgssJpnOverlayApFix_check122:
	ldr r0, =0x0225E3C0
	ldr r1, [r0, #0x584]
	ldr r2, =0x510CE58D
	cmp r2, r1
	bxne lr
	mov r2, #0x36
	strb r2, [r0, #0x586]
	ldr r1, [r0, #0x6DC]
	ldr r2, =0x427AE112
	cmp r2, r1
	bxne lr
	mov r2, #0x7B
	strb r2, [r0, #0x6DE]
	bx lr
.pool
hgssJpnOverlayApFix_end:
@---------------------------------------------------------------------------------
ssKorOverlayApFix: @ overlay9_1
	ldr r0, =0x021E6300+0x216
	ldrb r1, [r0]
	mov r2, #0x5B
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne ssKorOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
ssKorOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
ssKorOverlayApFix_end:
@---------------------------------------------------------------------------------
hgKorOverlayApFix: @ overlay9_1
	ldr r0, =0x021E6300+0x213
	ldrb r1, [r0]
	mov r2, #0x5B
	cmp r2, r1
	bxne lr
	ldrb r1, [r0, #2]
	mov r2, #0xD1
	cmp r2, r1
	bne hgKorOverlayApFix_check3
	mov r2, #0xE0
	strb r2, [r0, #2]
	bx lr
hgKorOverlayApFix_check3:
	ldrb r1, [r0, #3]
	cmp r2, r1
	bxne lr
	mov r2, #0xE0
	strb r2, [r0, #3]
	bx lr
.pool
hgKorOverlayApFix_end:
