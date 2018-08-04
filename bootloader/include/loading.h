#ifndef LOADING_H
#define LOADING_H

/*-------------------------------------------------------------------------
Declarations of loading screen functions
--------------------------------------------------------------------------*/
void arm9_regularLoadingScreen(void); // Regular
void arm9_loadingCircle(void);        // Regular
void arm9_errorText(void);            // Regular
void arm9_pong(void);                 // Pong
void arm9_ttt(void);                  // Tic-Tac-Toe

extern volatile bool arm9_animateLoadingCircle;

// Regular
extern volatile int arm9_loadBarLength;
extern volatile bool arm9_animateLoadingCircle;
extern bool displayScreen; //extern static bool displayScreen;

// Pong & Tic-Tac-Toe
extern volatile bool arm9_errorColor;

#endif // LOADING_H
