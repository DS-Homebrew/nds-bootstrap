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

#include <stdio.h>
#include <nds/system.h>
#include <nds/debug.h>

#include "hook.h"
#include "common.h"
#include "tonccpy.h"
#include "locations.h"

extern unsigned long cheat_engine_size;
extern unsigned long intr_orig_return_offset;

/*// libnds v1.5.12 2016
static const u32 homebrewStartSig_2016[1] = {
	0x04000208, 	// DCD 0x4000208
};

static const u32 homebrewEndSig_2016[2] = {
	0x04000004,		// DCD 0x4000004
	0x04000180		// DCD 0x4000180
};

// libnds v_._._ 2007 irqset
static const u32 homebrewStartSig_2007[1] = {
	0x04000208, 	// DCD 0x4000208
};

static const u32 homebrewEndSig2007[2] = {
	0x04000004,		// DCD 0x4000004
	0x04000180		// DCD 0x4000180
};*/

// interruptDispatcher.s jump_intr:
// interruptDispatcher.s jump_intr:
static const u32 homebrewSig[5] = {
	0xE5921000, // ldr    r1, [r2]        @ user IRQ handler address
	0xE3510000, // cmp    r1, #0
	0x1A000001, // bne    got_handler
	0xE1A01000, // mov    r1, r0
	0xEAFFFFF6  // b    no_handler
};

// interruptDispatcher.s jump_intr:
// Patch
static const u32 homebrewSigPatched[5] = {
	0xE59F1008, // ldr    r1, =0x3900010   @ my custom handler
	0xE5012008, // str    r2, [r1,#-8]     @ irqhandler
	0xE501F004, // str    r0, [r1,#-4]     @ irqsig
	0xEA000000, // b      got_handler
	0x037C0010  // DCD 	  0x037C0010
};

// Accelerator patch for IPC_SYNC v2007
static const u32 homebrewAccelSig2007[4] = {
	0x2401B510   , // .
	               // MOVS    R4, #1
	0xD0064220   , // .
				// .
	0x881A4B0C   , // ...
	0x430A2108   , // ...
};

// Accelerator patch for IPC_SYNC v2007
static const u32 homebrewAccelSig2007_2[4] = {
	0x2401B510   , // .
	               // MOVS    R4, #1
	0xD0064220   , // .
				// .
	0x881A4B10   , // ...
	0x430A2108   , // ...
};

// Accelerator patch for IPC_SYNC v2010 (libnds 1.4.8)
static const u32 homebrewAccelSig2010[4] = {
	0x07C3B500   , // .
	               // MOVS    R4, #1
	0x4B13D506   , // .
				// .
	0x22088819   , // ...
	0x0412430A   , // ...
};

static const u32 homebrewAccelSigPatched[2] = {
	0x47104A00   , // LDR     R2, =0x037C0020
	               // BX      R2
	0x037C0020
};

/*static const u32 swi05Sig[1] = {
	0x4770DF05   , // SWI 0X05
};*/

//static const int MAX_HANDLER_SIZE = 50;

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

static u32* hookAccelIPCHomebrew2007(u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);

	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewAccelSig2007[0]) &&
			(addr[1] == homebrewAccelSig2007[1]) &&
			((addr[2] == homebrewAccelSig2007[2]) || (addr[2] == homebrewAccelSig2007_2[2])) &&
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
	addr[0] = homebrewAccelSigPatched[0];
	addr[1] = homebrewAccelSigPatched[1];

	return addr;
}

static u32* hookAccelIPCHomebrew2010(u32* addr, size_t size) {
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
	addr[0] = homebrewAccelSigPatched[0];
	addr[1] = homebrewAccelSigPatched[1];

	return addr;
}

/*const u16* generateA7InstrThumb(int arg1, int arg2) {
	static u16 instrs[2];

	// 23 bit offset
	u32 offset = (u32)(arg2 - arg1 - 4);
	//dbg_printf("generateA7InstrThumb offset\n");
	//dbg_hexa(offset);
	
	// 1st instruction contains the upper 11 bit of the offset
	instrs[0] = ((offset >> 12) & 0x7FF) | 0xF000;

	// 2nd instruction contains the lower 11 bit of the offset
	instrs[1] = ((offset >> 1) & 0x7FF) | 0xF800;

	return instrs;
}

static u32* hookSwi05(u32* addr, size_t size, u32* hookAccel, u32* sdEngineLocation) {
	u32* end = addr + size/sizeof(u32);

	// Find the start of the handler
	while (addr < end) {
		if (addr[0] == swi05Sig[0])
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	u32 dstAddr = (u32)hookAccel+8;
	const u16* branchCode = generateA7InstrThumb((int)addr, dstAddr);

	// patch the program
	tonccpy(addr, branchCode, 4);

	tonccpy((u32*)dstAddr, (u32**)((u32)SDENGINE_BUFFER_LOCATION+4), 0x10);

	return addr;
}*/

int hookNds (const tNDSHeader* ndsHeader, u32* sdEngineLocation, u32* wordCommandAddr) {
	u32* hookLocation = NULL;
	u32* hookAccel = NULL;

	nocashMessage("hookNds");

	hookLocation = hookInterruptHandlerHomebrew((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);

	if (!hookLocation) {
		nocashMessage("ERR_HOOK");
		return ERR_HOOK;
	}

	hookAccel = hookAccelIPCHomebrew2007((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);

	if (!hookAccel) {
		hookAccel = hookAccelIPCHomebrew2010((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
	}

	if (!hookAccel) {
		nocashMessage("ACCEL_IPC_ERR");
	} else {
		nocashMessage("ACCEL_IPC_OK");
	}

	/*if (hookAccel && (u32)ndsHeader->arm7destination >= 0x037F8000) {
		hookSwi05((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize, hookAccel, sdEngineLocation);
	}*/

	tonccpy (sdEngineLocation, (u32*)SDENGINE_BUFFER_LOCATION, 0x4000);
	tonccpy ((u32*)SDENGINE_BUFFER_WRAM_LOCATION, (u32*)SDENGINE_BUFFER_LOCATION, 0x4000);
	toncset ((u32*)SDENGINE_BUFFER_LOCATION, 0, 0x4000);

	sdEngineLocation[1] = (u32)wordCommandAddr;

	nocashMessage("ERR_NONE");
	return ERR_NONE;
}
