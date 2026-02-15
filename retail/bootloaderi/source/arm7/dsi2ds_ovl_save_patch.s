@---------------------------------------------------------------------------------
	.global applyDSi2DSSaveCodeToOverlay
	.global applyDSi2DSThumbSaveCodeToOverlay
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
	ldr r4, dsi2dsSaveOverlayPatch
applyDSi2DSSaveCodeToOverlay_loop:
	ldr r0, [r4] @ offset
	add r4, #4
	ldr r1, [r4] @ code
	str r1, [r0]
	add r4, #4

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
.word 0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
applyDSi2DSThumbSaveCodeToOverlay:
@---------------------------------------------------------------------------------
	ldr r0, dsi2dsThumbSaveOverlayOffset
	ldr r1, dsi2dsThumbSaveOverlayCode
	ldr r0, [r0]
	cmp r0, r1
	bxne lr
	mov r2, #0
	ldr r3, dsi2dsThumbSaveOverlayPatchLines
	ldr r4, dsi2dsThumbSaveOverlayPatch
applyDSi2DSThumbSaveCodeToOverlay_loop:
	ldr r0, [r4] @ offset
	add r4, #4
	ldrh r1, [r4] @ code
	strh r1, [r0]
	add r4, #2
	add r0, #2
	ldrh r1, [r4]
	strh r1, [r0]
	add r4, #2

	add r2, #1
	cmp r2, r3
	bne applyDSi2DSThumbSaveCodeToOverlay_loop
	bx lr
dsi2dsThumbSaveOverlayOffset:
.word 0
dsi2dsThumbSaveOverlayCode:
.word 0
dsi2dsThumbSaveOverlayPatchLines:
.word 0
dsi2dsThumbSaveOverlayPatch:
.word 0
@---------------------------------------------------------------------------------
