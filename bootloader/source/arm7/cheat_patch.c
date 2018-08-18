#include <string.h>
#include <nds/ndstypes.h>
#include <nds/debug.h>
#include "nds_header.h"
#include "cheat_engine.h"
#include "cheat_patch.h"

/*static const u32 gameName[] = {
    // Description of cheat section
	0x00000000, 0x00000000,
	0x00000000, 0x00000000
};*/

static bool findString(const char* one, const char** many) {
    u32 len = sizeof(many) / sizeof(char*);
    for (u32 i = 0; i < len; ++i) {
        if (strcmp(one, many[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool findRomTid(const tNDSHeader* ndsHeader, const char** romTids) {
    return findString(getRomTid(ndsHeader), romTids);
}

static void defaultPatch(cardengineArm7* ce7, const tNDSHeader* ndsHeader, const char** romTids, const u32* patch_cheat_data, u32 patch_cheat_data_size) {
    if (!findRomTid(ndsHeader, romTids)) {
        return;
    }
    
    u32* ce7_cheat_data = getCheatData(ce7);
    u32 patch_cheat_data_len = patch_cheat_data_size/sizeof(u32);
    if (checkCheatDataLen(ce7->cheat_data_len + patch_cheat_data_len)) {
        memcpy(ce7_cheat_data + ce7->cheat_data_len, patch_cheat_data, patch_cheat_data_size);
		ce7->cheat_data_len += patch_cheat_data_len;
    } else {
        nocashMessage("CHEAT_DATA size limit reached, cheat patch ignored!\n");
    }
}

/*static void patchGameName(cardengineArm7* ce7, const tNDSHeader* ndsHeader) {
    const char* romTids[] = { "AAAA" };
	return defaultPatch(ce7, ndsHeader, romTids, gameName, sizeof(gameName));
}*/

void cheatPatch(cardengineArm7* ce7, const tNDSHeader* ndsHeader) {
	// General patches

    // Specific patches
    //patchGameName(ce7, ndsHeader);
}
