#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "module_params.h"

void decompressBinary(const tNDSHeader* ndsHeader, module_params_t* moduleParams, bool foundModuleParams);

#endif // DECOMPRESS_H
