#include <nds.h>

void SCFGFifoCheck (void)
{
	if(fifoCheckValue32(FIFO_USER_06)) {
		if(fifoCheckValue32(FIFO_USER_07)) { REG_SCFG_CLK = 0x0181; }
	fifoSendValue32(FIFO_USER_06, 0);
	}
}

