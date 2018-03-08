#ifndef _DATABWLIST_H
#define _DATABWLIST_H

// ROM data include list.
// 1 = start of data address, 2 = end of data address, 3 = data size
u32 dataWhitelist_A5TE0[3] = {0x01369CA0, 0x01440800, 0x000D6C00};	// MegaMan Battle Network 5: Double Team DS (U)

// ROM data exclude list.
// 1 = start of data address, 2 = end of data address, 3 = data size
//u32 dataBlacklist_AYIE0[3] = {0x000C0000, 0x00853200, 0x00793200};	// Yoshi Touch & Go (U)
//u32 dataBlacklist_AWRE0[3] = {0x002EE800, 0x014F6000, 0x01207800};	// Advance Wars: Dual Strike (U)

#endif // _DATABWLIST_H
