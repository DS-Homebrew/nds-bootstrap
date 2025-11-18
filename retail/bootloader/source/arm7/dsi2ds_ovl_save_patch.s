@---------------------------------------------------------------------------------
	.global applyDSi2DSSaveCodeToOverlay
	.align	4
	.arm

@---------------------------------------------------------------------------------
applyDSi2DSSaveCodeToOverlay:
@---------------------------------------------------------------------------------
	ldr r0, dsi2dsSaveOverlayOffset
	ldr r1, dsi2dsSaveOverlayCode
	ldr r0, [r0]
	cmp r0, r1
	bxne lr
	mov r2, #0
	ldr r3, dsi2dsSaveOverlayPatchLines
	adr r4, dsi2dsSaveOverlayPatch
	mov r5, #0
applyDSi2DSSaveCodeToOverlay_loop:
	ldr r0, [r4, r5] @ offset
	add r5, #4
	ldr r1, [r4, r5] @ code
	add r5, #4
	str r1, [r0]

	add r2, #1
	cmp r2, r3
	bne applyDSi2DSSaveCodeToOverlay_loop
	bx lr
dsi2dsSaveOverlayOffset:
.word 0
dsi2dsSaveOverlayCode:
.word 0
dsi2dsSaveOverlayPatchLines:
.word 0
dsi2dsSaveOverlayPatch:
@---------------------------------------------------------------------------------
