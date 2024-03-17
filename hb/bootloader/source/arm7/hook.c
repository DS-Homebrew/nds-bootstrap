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

#include "sdengine_bin.h"
#include "sdengine_alt_bin.h"
#include "patch.h"
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
	0xE59F1008, // ldr    r1, =0x3000010   @ my custom handler
	0xE5012008, // str    r2, [r1,#-8]     @ irqhandler
	0xE501F004, // str    r0, [r1,#-4]     @ irqsig
	0xEA000000, // b      got_handler
	0x00000010  // DCD 	  0x03000010
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

// Accelerator patch for IPC_SYNC v2007
static const u32 homebrewAccelSig2007ARM[4] = {
	0xE59F316C, 0xE3A01000, 0xE0832181, 0xE5922004
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
	0x47104A00   , // LDR     R2, =0x03000020
	               // BX      R2
	0x00000020
};

static const u16 swi00Sig[2] = {
	0xDF00   , // SWI 0X00
	0x4770
};

static const u16 swi12Sig[2] = {
	0xDF12   , // SWI 0X12
	0x4770
};

static const u32 swi00Patched[3] = {
	0x68004801   , // LDR     R0, =0x02FFFE34
	               // LDR     R0, [R0]
	0x00004700   , // BX      R0
	0x02FFFE34
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

	return addr;
}

static u32* hookAccelIPCHomebrew2007ARM(u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);

	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewAccelSig2007ARM[0]) &&
			(addr[1] == homebrewAccelSig2007ARM[1]) &&
			(addr[2] == homebrewAccelSig2007ARM[2]) &&
			(addr[3] == homebrewAccelSig2007ARM[3]))
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	return addr;
}

static u16* hookSwi00(u16* addr, size_t size) {
	u16* end = addr + size/sizeof(u16);

	while (addr < end) {
		if (addr[0] == swi00Sig[0] &&
			(addr[1] == swi00Sig[1]))
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	return addr;
}

static u16* hookSwi12(u16* addr, size_t size) {
	u16* end = addr + size/sizeof(u16);

	while (addr < end) {
		if (addr[0] == swi12Sig[0] &&
			(addr[1] == swi12Sig[1]))
		{
			break;
		}
		addr++;
	}

	if (addr >= end) {
		return NULL;
	}

	return addr;
}

const u16* generateA7InstrThumb(int arg1, int arg2) {
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

/*static u32* hookSwi05(u32* addr, size_t size, u32* hookAccel, u32* sdEngineLocation) {
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
	u32* hookLocation = patchOffsetCache.a7IrqHookOffset;
	u32* hookAccel = patchOffsetCache.a7IrqHookAccelOffset;
	u16* a9Swi12Location = patchOffsetCache.a9Swi12Offset;
	u16* swi00Location = patchOffsetCache.swi00Offset;

	nocashMessage("hookNds");

	if (!patchOffsetCache.a9Swi12Checked) {
		a9Swi12Location = hookSwi12((u16*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
		if (a9Swi12Location) {
			patchOffsetCache.a9Swi12Offset = a9Swi12Location;
		}
		patchOffsetCache.a9Swi12Checked = true;
	}
	if (a9Swi12Location && !(REG_SCFG_ROM & BIT(1))) {
		// Patch SWI 0x12 to 0x02 for DSi BIOS
		*a9Swi12Location = 0xDF02;
	}

	if (!patchOffsetCache.swi00Checked) {
		swi00Location = hookSwi00((u16*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		if (swi00Location) {
			patchOffsetCache.swi00Offset = swi00Location;
		}
		patchOffsetCache.swi00Checked = true;
	}
	if (swi00Location && !(REG_SCFG_ROM & BIT(9))) {
		// Patch SWI 0x12 to 0x02 for DSi BIOS
		for (u8 i = 0; i < 0x80/2; i++) {
			if (swi00Location[i] == 0xDF12) {
				swi00Location[i] = 0xDF02;
				break;
			}
		}
	}

	if (!sdEngineLocation) {
		return ERR_NONE;
	}

	if (!hookLocation) {
		hookLocation = hookInterruptHandlerHomebrew((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		if (hookLocation) {
			patchOffsetCache.a7IrqHookOffset = hookLocation;
		}
	}

	if (hookLocation) {
		// patch the program
		hookLocation[0] = homebrewSigPatched[0];
		hookLocation[1] = homebrewSigPatched[1];
		hookLocation[2] = homebrewSigPatched[2];
		hookLocation[3] = homebrewSigPatched[3];
		hookLocation[4] = ((u32)sdEngineLocation)+homebrewSigPatched[4];
	} else {
		nocashMessage("ERR_HOOK");
		return ERR_HOOK;
	}

	if (!hookAccel) {
		hookAccel = hookAccelIPCHomebrew2007((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		if (!hookAccel) {
			hookAccel = hookAccelIPCHomebrew2010((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		}
		if (!hookAccel) {
			hookAccel = hookAccelIPCHomebrew2007ARM((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
		}
		if (hookAccel) {
			patchOffsetCache.a7IrqHookAccelOffset = hookAccel;
		}
	}

	if (!hookAccel) {
		nocashMessage("ACCEL_IPC_ERR");
	} else {
		// patch the program
		hookAccel[0] = (*hookAccel == homebrewAccelSig2007ARM[0]) ? 0xE51FF004 : homebrewAccelSigPatched[0];
		hookAccel[1] = ((u32)sdEngineLocation)+homebrewAccelSigPatched[1];

		nocashMessage("ACCEL_IPC_OK");
	}

	if (swi00Location && hookAccel) {
		u32 dstAddr = (u32)hookAccel+8;
		const u16* branchCode = generateA7InstrThumb((int)swi00Location, dstAddr);

		// patch the program
		tonccpy(swi00Location, branchCode, 4);

		tonccpy((u32*)dstAddr, swi00Patched, 0xC);
	}

	/*if (hookAccel && (u32)ndsHeader->arm7destination >= 0x037F8000) {
		hookSwi05((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize, hookAccel, sdEngineLocation);
	}*/

	tonccpy (sdEngineLocation, (sdEngineLocation == (u32*)SDENGINE_LOCATION_ALT) ? sdengine_alt_bin : sdengine_bin, (sdEngineLocation == (u32*)SDENGINE_LOCATION_ALT) ? sdengine_alt_bin_size : sdengine_bin_size);

	sdEngineLocation[1] = (u32)wordCommandAddr;

	nocashMessage("ERR_NONE");
	return ERR_NONE;
}
