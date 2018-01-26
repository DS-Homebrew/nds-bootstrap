/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "hook.h"
#include "common.h"
#include "cardengine_arm7_bin.h"
#include "fat.h"

extern unsigned long cheat_engine_size;
extern unsigned long intr_orig_return_offset;

extern const u8 cheat_engine_start[]; 

static const u32 handlerStartSig[5] = {
	0xe92d4000, 	// push {lr}
	0xe3a0c301, 	// mov  ip, #0x4000000
	0xe28cce21,		// add  ip, ip, #0x210
	0xe51c1008,		// ldr	r1, [ip, #-8]
	0xe3510000		// cmp	r1, #0
};

static const u32 handlerEndSig[4] = {
	0xe59f1008, 	// ldr  r1, [pc, #8]	(IRQ Vector table address)
	0xe7910100,		// ldr  r0, [r1, r0, lsl #2]
	0xe59fe004,		// ldr  lr, [pc, #4]	(IRQ return address)
	0xe12fff10		// bx   r0
};

// interruptDispatcher.s jump_intr:
static const u32 homebrewSig[5] = {
	0xE5921000, // ldr    r1, [r2]        @ user IRQ handler address
	0xE3510000, // cmp    r1, #0
	0x1A000001, // bne    got_handler
	0xE1A01000, // mov    r1, r0
	0xEAFFFFF6  // b    no_handler
};

// interruptDispatcher.s jump_intr:
//patch
static const u32 homebrewSigPatched[5] = {
	0xE59F1008, // ldr    r1, =0x3900010   @ my custom handler
	0xE5012008, // str    r2, [r1,#-8]     @ irqhandler
	0xE501F004, // str    r0, [r1,#-4]     @ irqsig 
	0xEA000000, // b      got_handler
	0x03780010  // DCD 	  0x03780010       
};
 
// accelerator patch for IPC_SYNC v2007
static const u32 homebrewAccelSig2007[4] = {
	0x2401B510   , // .
	               // MOVS    R4, #1
	0xD0064220   , // .
				// .
	0x881A4B10   , // ...
	0x430A2108   , // ...
};

static const u32 homebrewAccelSig2007Patched[4] = {
	0x47104A00   , // LDR     R2, =0x03780020
	               // BX      R2
	0x03780020   , // 
				   // 
	0x881A4B10   , // ...
	0x430A2108   , // ...
};

// accelerator patch for IPC_SYNC v2010 (libnds 1.4.8)
static const u32 homebrewAccelSig2010[4] = {
	0x07C3B500   , // .
	               // MOVS    R4, #1
	0x4B13D506   , // .
				// .
	0x22088819   , // ...
	0x0412430A   , // ...
};

static const u32 homebrewAccelSig2010Patched[4] = {
	0x47104A00   , // LDR     R2, =0x03780020
	               // BX      R2
	0x03780020   , // 
				   // 
	0x22088819   , // ...
	0x0412430A   , // ...
};

static const int MAX_HANDLER_SIZE = 50;

static u32* hookInterruptHandlerHomebrew (u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);

	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewSig[0]) && 
			(addr[1] == homebrewSig[1]) && 
			(addr[2] == homebrewSig[2]) && 
			(addr[3] == homebrewSig[3]) && 
			(addr[4] == homebrewSig[4])) 
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	// patch the program
	addr[0] = homebrewSigPatched[0];
	addr[1] = homebrewSigPatched[1];
	addr[2] = homebrewSigPatched[2];
	addr[3] = homebrewSigPatched[3];
	addr[4] = homebrewSigPatched[4];

	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}

static u32* hookAccelIPCHomebrew2007 (u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);

	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewAccelSig2007[0]) && 
			(addr[1] == homebrewAccelSig2007[1]) && 
			(addr[2] == homebrewAccelSig2007[2]) && 
			(addr[3] == homebrewAccelSig2007[3])) 
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	// patch the program
	addr[0] = homebrewAccelSig2007Patched[0];
	addr[1] = homebrewAccelSig2007Patched[1];
	addr[2] = homebrewAccelSig2007Patched[2];
	addr[3] = homebrewAccelSig2007Patched[3];

	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}


static u32* hookAccelIPCHomebrew2010 (u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);

	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewAccelSig2010[0]) && 
			(addr[1] == homebrewAccelSig2010[1]) && 
			(addr[2] == homebrewAccelSig2010[2]) && 
			(addr[3] == homebrewAccelSig2010[3])) 
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	// patch the program
	addr[0] = homebrewAccelSig2010Patched[0];
	addr[1] = homebrewAccelSig2010Patched[1];
	addr[2] = homebrewAccelSig2010Patched[2];
	addr[3] = homebrewAccelSig2010Patched[3];

	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}

static u32* hookInterruptHandler (u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);
	int i;

	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == handlerStartSig[0]) && 
			(addr[1] == handlerStartSig[1]) && 
			(addr[2] == handlerStartSig[2]) && 
			(addr[3] == handlerStartSig[3]) && 
			(addr[4] == handlerStartSig[4])) 
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	// Find the end of the handler
	for (i = 0; i < MAX_HANDLER_SIZE; i++) {
		if ((addr[i+0] == handlerEndSig[0]) && 
			(addr[i+1] == handlerEndSig[1]) && 
			(addr[i+2] == handlerEndSig[2]) && 
			(addr[i+3] == handlerEndSig[3])) 
		{
			break;
		}
	}

	if (i >= MAX_HANDLER_SIZE) {
		return NULL;
	}

	// Now find the IRQ vector table
	// Make addr point to the vector table address pointer within the IRQ handler
	addr = addr + i + sizeof(handlerEndSig)/sizeof(handlerEndSig[0]);

	// Use relative and absolute addresses to find the location of the table in RAM
	u32 tableAddr = addr[0];
	u32 returnAddr = addr[1];
	u32* actualReturnAddr = addr + 2;
	u32* actualTableAddr = actualReturnAddr + (tableAddr - returnAddr)/sizeof(u32);

	// The first entry in the table is for the Vblank handler, which is what we want
	return actualTableAddr;
	// 2     LCD V-Counter Match
}


int hookNdsRetail (const tNDSHeader* ndsHeader, aFile file, const u32* cheatData, u32* cheatEngineLocation, u32* cardEngineLocation) {
	u32* hookLocation = NULL;
	u32* hookAccel = NULL;
	u32* debug = (u32*)0x03786000;

	nocashMessage("hookNdsRetail");

	if (!hookLocation) {
		hookLocation = hookInterruptHandler((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
	}

	if (!hookLocation) {
		nocashMessage("ERR_HOOK");
		//return ERR_HOOK;
	}

	if(!hookLocation){

		if(*(u32*)(0x27FF03C) == 0x00022B40){
			// TODO
		}else if(*(u32*)(0x27FF03C) == 0x00022BCC){
			// TODO
		}else if(*(u32*)(0x27FF03C) == 0x00028F84){
			hookLocation = 0x2391918;
		}else if(*(u32*)(0x27FF03C) == 0x0002909C){
			// TODO
		}else if(*(u32*)(0x27FF03C) == 0x0002914C){
			// TODO
		}else if(*(u32*)(0x27FF03C) == 0x00029164){
			hookLocation = 0x2391ADC;
		}else if(*(u32*)(0x27FF03C) == 0x0002A2EC){
			// TODO
		}else if(*(u32*)(0x27FF03C) == 0x0002A318){
			// TODO
		}else if(*(u32*)(0x27FF03C) == 0x0002AF18){
			hookLocation = 0x239227C;
		}else if(*(u32*)(0x27FF03C) == 0x0002C5B4){
			// TODO
		}

	}

	if (!hookLocation) {
		nocashMessage("ERR_HOOK");
		return ERR_HOOK;
	}

	
	u32* vblankHandler = hookLocation;
	u32* ipcSyncHandler = hookLocation+16;

	debug[9] = hookLocation;

	/*hookAccel = hookAccelIPCHomebrew2007((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);

	if (!hookAccel) {
		nocashMessage("ACCEL_IPC_2007_ERR");
	} else {
		nocashMessage("ACCEL_IPC_2007_OK");
	}

	hookAccel = hookAccelIPCHomebrew2010((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);

	if (!hookAccel) {
		nocashMessage("ACCEL_IPC_2010_ERR");
	} else {
		nocashMessage("ACCEL_IPC_2010_OK");
	}*/

	cardEngineLocation[1] = *vblankHandler;
	cardEngineLocation[2] = *ipcSyncHandler;
	cardEngineLocation[4] = file.firstCluster;

	u32* patches =  (u32*) cardEngineLocation[0];

	*vblankHandler = patches[3];
	*ipcSyncHandler = patches[4];

	nocashMessage("ERR_NONE");
	return ERR_NONE;
}

