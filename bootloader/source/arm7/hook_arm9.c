#include <nds/ndstypes.h>
#include "hook.h"
#include "cardengine_header_arm9.h"

//extern bool sdk5;

extern u32 ROMinRAM;
u32 ROM_HEADERCRC = 0; //extern u32 ROM_HEADERCRC;
u32 ARM9_LEN = 0; //extern u32 ARM9_LEN;
u32 romSize = 0; //extern u32 romSize;
extern bool dsiModeConfirmed; // SDK 5
extern u32 enableExceptionHandler;
extern unsigned long consoleModel;
extern unsigned long asyncPrefetch;

void hookNdsRetailArm9(cardengineArm9* ce9) {
	ce9->rom_in_ram               = ROMinRAM;
	ce9->rom_headercrc            = ROM_HEADERCRC;
	ce9->arm9_len                 = ARM9_LEN;
	ce9->rom_size                 = romSize;
	ce9->dsi_mode                 = dsiModeConfirmed; // SDK 5
	ce9->enable_exception_handler = enableExceptionHandler;
	ce9->console_model            = consoleModel;
	ce9->async_prefetch           = asyncPrefetch;
}
