#ifndef CHEAT_PATCH_H
#define CHEAT_PATCH_H

#include <nds/memory.h> // tNDSHeader
#include "cardengine_header_arm7.h"

void cheatPatch(cardengineArm7* ce7, const tNDSHeader* ndsHeader);

#endif // CHEAT_PATCH_H
