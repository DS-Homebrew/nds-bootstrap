#include <nds.h>

void fifocheck (void)
{
	if(fifoCheckValue32(FIFO_USER_06)) {
		REG_SCFG_ROM = 0x703;
		if(fifoCheckValue32(FIFO_USER_07)) { REG_SCFG_CLK=0x0181; }
		REG_SCFG_EXT = 0x93A40000; // NAND/SD Access
		fifoSendValue32(FIFO_USER_06, 0);
	}
}

