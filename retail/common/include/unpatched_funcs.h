#ifndef UNPATCHED_FUNCS_ARM9_H
#define UNPATCHED_FUNCS_ARM9_H

#include <nds/ndstypes.h>
#include "module_params.h"

typedef struct unpatchedFunctions {
    u32 compressed_static_end;
    module_params_t* moduleParams;
	u32* mpuDataOffset;
	u32* mpuDataOffset2;
	u32 mpuInitRegionOldData;
	int mpuAccessOffset;
    u32 mpuOldInstrAccess;
    u32 mpuOldDataAccess;
    u32 mpuOldDataAccess2;
} __attribute__ ((__packed__)) unpatchedFunctions;

#endif // UNPATCHED_FUNCS_ARM9_H
