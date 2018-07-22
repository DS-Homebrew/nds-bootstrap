#ifndef SAVE_PATCH_H
#define SAVE_PATCH_H

#include <nds/ndstypes.h>
#include "card_patcher.h"
#include "card_finder.h"
#include "debugToFile.h"

//
// Subroutine function signatures ARM7
//

u32 relocateStartSignature[1];
u32 nextFunctiontSignature[1];
u32 a7cardReadSignature[2];
u32 a7something2Signature[2];

/*
u32 a7JumpTableSignature[4]     = {0xE5950024, 0xE3500000, 0x13A00001, 0x03A00000};
u32 a7JumpTableSignatureV3_1[3] = {0xE92D4FF0, 0xE24DD004, 0xE59F91F8};
u32 a7JumpTableSignatureV3_2[3] = {0xE92D4FF0, 0xE24DD004, 0xE59F91D4};
u32 a7JumpTableSignatureV4_1[3] = {0xE92D41F0, 0xE59F4224, 0xE3A05000};
u32 a7JumpTableSignatureV4_2[3] = {0xE92D41F0, 0xE59F4200, 0xE3A05000};
*/

#endif // SAVE_PATCH_H