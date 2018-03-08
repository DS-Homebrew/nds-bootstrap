#include <nds.h>

void SCFGFifoCheck (void)
{
	if(fifoCheckValue32(FIFO_USER_06)) { REG_SCFG_CLK = 0x0181; }
}

