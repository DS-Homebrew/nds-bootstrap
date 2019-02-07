/*-------------------------------------------------------------------------
Loading pong
--------------------------------------------------------------------------*/

#define ARM9
#undef ARM7

#include <nds/system.h>
#include <nds/arm9/video.h>
#include <nds/input.h>

#include "common.h"
#include "loading.h"

static bool drawnStuff = false;
static bool rightpaddle_control = false;

static int leftpaddle_yPos = 76;
static int rightpaddle_yPos = 76;
static int ball_xPos = 16;
static int ball_yPos = 72;
static int ball_ySpeed = 2;
static int ball_moveUD = false;	// false = up, true = down
static int ball_moveLR = true;	// false = left, true = right

static u16 pongPaddleColor;
static u16 pongBallColor;
static u16 bgColor;
static u16 errorColor;

void restartPong(void) {
	// Restart minigame
	for (int i = 0; i < 256*192; i++) {
		VRAM_A[i] = bgColor;
	}
	leftpaddle_yPos = 76;
	rightpaddle_yPos = 76;
	if (!ball_moveLR) {
		ball_xPos = 16;
		ball_moveLR = true;	// false = left, true = right
	} else {
		ball_xPos = 256-16;
		ball_moveLR = false;	// false = left, true = right
	}
	ball_yPos = 64;
	ball_ySpeed = 2;
	ball_moveUD = false;	// false = up, true = down
}

void arm9_pong(void) {
	if (!drawnStuff) {
		pongPaddleColor = arm9_darkTheme ? 0x2D6B : 0x5294;
		pongBallColor = arm9_darkTheme ? 0x14A5 : 0x6B5A;
		bgColor = arm9_darkTheme ? 0x0842 : 0x7FFF;

		//if (!darkTheme) {
			errorColor = 0x001B;
		//} else {
		//	errorColor = 0x001B;
		//}

		if (!arm9_swapLcds){
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A | POWER_SWAP_LCDS);
		} else {
			REG_POWERCNT = (u16)(POWER_LCD | POWER_2D_A);	
		}
		REG_DISPCNT = MODE_FB0;
		VRAM_A_CR = VRAM_ENABLE;

		// Draw white/dark BG
		for (int i = 0; i < 256*192; i++) {
			VRAM_A[i] = bgColor;
		}

		drawnStuff = true;
	}
	
	if (arm9_errorColor) pongPaddleColor = errorColor;
	if (arm9_errorColor) pongBallColor = errorColor;

	while (REG_VCOUNT!=191); // Fix speed
	
	if (!rightpaddle_control) {
		rightpaddle_yPos = ball_yPos-16;
		if (rightpaddle_yPos < 0) rightpaddle_yPos = 0;
		if (rightpaddle_yPos > 156) rightpaddle_yPos = 156;
	}

	// Draw ball (back)
	for (int y = ball_yPos-16; y <= ball_yPos+24; y++) {
		for (int k = ball_xPos-2; k <= ball_xPos+10; k++) {
			VRAM_A[y*256+k] = bgColor;
		}
	}

	// Draw left paddle (back)
	for (int y = leftpaddle_yPos-4; y <= leftpaddle_yPos+36; y++) {
		for (int k = 8; k <= 16; k++) {
			VRAM_A[y*256+k] = bgColor;
		}
	}

	// Draw right paddle (back)
	for (int y = rightpaddle_yPos-16; y <= rightpaddle_yPos+48; y++) {
		for (int k = 240; k <= 248; k++) {
			VRAM_A[y*256+k] = bgColor;
		}
	}

	// Draw ball
	for (int y = ball_yPos; y <= ball_yPos+8; y++) {
		for (int k = ball_xPos; k <= ball_xPos+8; k++) {
			VRAM_A[y*256+k] = pongBallColor;
		}
	}

	// Draw left paddle
	for (int y = leftpaddle_yPos; y <= leftpaddle_yPos+32; y++) {
		for (int k = 8; k <= 16; k++) {
			VRAM_A[y*256+k] = pongPaddleColor;
		}
	}

	// Draw right paddle
	for (int y = rightpaddle_yPos; y <= rightpaddle_yPos+32; y++) {
		for (int k = 240; k <= 248; k++) {
			VRAM_A[y*256+k] = pongPaddleColor;
		}
	}

	if (ball_moveUD==false) {
		ball_yPos -= ball_ySpeed;
		if (ball_yPos <= 0) ball_moveUD = true;
	} else if (ball_moveUD==true) {
		ball_yPos += ball_ySpeed;
		if (ball_yPos >= 184) ball_moveUD = false;
	}

	if (ball_moveLR==false) {
		ball_xPos -= 2;
		if ((ball_yPos > leftpaddle_yPos-8) && (ball_yPos < leftpaddle_yPos+32)) {
			if (ball_xPos == 16) {
				ball_moveLR = true;
				if (ball_ySpeed != 6) {
					ball_ySpeed++;	// Increase Y speed of ball
				}
			}
		} else if (ball_xPos <= 0) {
			restartPong();
		}
	} else if (ball_moveLR==true) {
		ball_xPos += 2;
		if ((ball_yPos > rightpaddle_yPos-8) && (ball_yPos < rightpaddle_yPos+32)) {
			if (ball_xPos == 256-24) {
				ball_moveLR = false;
				if (ball_ySpeed != 6) {
					ball_ySpeed++;	// Increase Y speed of ball
				}
			}
		} else if (ball_xPos >= 255) {
			restartPong();
		}
	}

	// Control left paddle
	if (0 == (REG_KEYINPUT & KEY_UP)) {
		leftpaddle_yPos -= 4;
		if (leftpaddle_yPos < 0) leftpaddle_yPos = 0;
	}
	if (0 == (REG_KEYINPUT & KEY_DOWN)) {
		leftpaddle_yPos += 4;
		if (leftpaddle_yPos > 156) leftpaddle_yPos = 156;
	}

	// Control right paddle
	/*if (0 == (REG_KEYINPUT & KEY_X) && rightpaddle_control) {
		rightpaddle_yPos -= 4;
		if (rightpaddle_yPos < 0) rightpaddle_yPos = 0;
	}
	if (0 == (REG_KEYINPUT & KEY_B) && rightpaddle_control) {
		rightpaddle_yPos += 4;
		if (rightpaddle_yPos > 156) rightpaddle_yPos = 156;
	}*/
	if (rightpaddle_control) {
		if (0 == (REG_KEYINPUT & KEY_B)) {
			rightpaddle_yPos += 4;
			if (rightpaddle_yPos > 156) rightpaddle_yPos = 156;
		} else {
			rightpaddle_yPos -= 4;
			if (rightpaddle_yPos < 0) rightpaddle_yPos = 0;
		}
	} else {
		if (0 == (REG_KEYINPUT & KEY_R)) {
			rightpaddle_control = true;
			restartPong();
		}
	}

}