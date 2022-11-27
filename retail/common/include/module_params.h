#ifndef MODULE_PARAMS_H
#define MODULE_PARAMS_H

#include <nds/ndstypes.h>

typedef struct {
	u32* auto_load_list_offset;
	u32* auto_load_list_end;
	u32* auto_load_start;
	u32* static_bss_start;
	u32* static_bss_end; // arm9i binary gets placed there in DSi mode
	u32 compressed_static_end;
	u32 sdk_version;
	u32 nitro_code_be;
	u32 nitro_code_le;
} module_params_t;

typedef struct {
	u32* arm9i_end1;
	u32* arm9i_end2;
	u32* arm9i_offset;
	u32 compressed_static_end;
	u32 nitro_code_be;
	u32 nitro_code_le;
} ltd_module_params_t;

inline bool isSdk5(const module_params_t* moduleParams) {
	return (moduleParams->sdk_version > 0x5000000);
}

#endif // MODULE_PARAMS_H
