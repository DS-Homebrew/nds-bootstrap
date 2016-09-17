#include <nds.h>

void ntrcheck (void)
{
		if(fifoCheckValue32(FIFO_USER_03)) { 
			if(REG_SCFG_ROM == 0x501) {
				REG_SCFG_ROM = 0x703;
				REG_SCFG_CLK = 0x0181;
				REG_SCFG_EXT = 0x93A40000; // NAND/SD Access
				// REG_SCFG_EXT = 0x12A00000;
				// REG_SCFG_EXT = 0x92A00000;
			}
		}
}

