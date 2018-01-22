#ifndef _DATABWLIST_H
#define _DATABWLIST_H

// ROM data include list.
// 1 = start of data address, 2 = end of data address, 3 = data size
u32 dataWhitelist_AZWJ0[3] = {0x000D9B98, 0x00F54000, 0x00E7A468};	// Sawaru - Made in Wario (J)

// ROM data exclude list.
// 1 = start of data address, 2 = end of data address, 3 = data size
u32 dataBlacklist_ARRE0[3] = {0x0096D400, 0x01AC4800, 0x01157400};	// Ridge Racer DS (U)

#endif // _DATABWLIST_H
