#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <nds/ndstypes.h>
#include <nds/memory.h> // tNDSHeader
#include "module_params.h"

void ensureBinaryDecompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams);
bool decrypt_arm9(const tDSiHeader* dsiHeader);

#endif // DECOMPRESS_H
