/*---------------------------------------------------------------------------------

	Copyright (C) 2005
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/

#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/ipc.h>

void IntrMain();	// Prototype for assembly interrupt dispatcher

//---------------------------------------------------------------------------------
void irqDummy(void) {}
//---------------------------------------------------------------------------------

#define INT_TABLE_SECTION

struct IntTable irqTable[MAX_INTERRUPTS] INT_TABLE_SECTION;

//---------------------------------------------------------------------------------
static void __irqSet(u32 mask, IntFn handler, struct IntTable irqTable[] ) {
//---------------------------------------------------------------------------------
	if (!mask) return;

	int i;

	for	(i=0;i<MAX_INTERRUPTS;i++)
		if	(!irqTable[i].mask || irqTable[i].mask == mask) break;

	if ( i == MAX_INTERRUPTS ) return;

	irqTable[i].handler	= handler;
	irqTable[i].mask	= mask;
}

//---------------------------------------------------------------------------------
void irqSet(u32 mask, IntFn handler) {
//---------------------------------------------------------------------------------
	int oldIME = enterCriticalSection();
	__irqSet(mask,handler,irqTable);
	if(mask & IRQ_VBLANK)
		REG_DISPSTAT |= DISP_VBLANK_IRQ ;
	if(mask & IRQ_HBLANK)
		REG_DISPSTAT |= DISP_HBLANK_IRQ ;
	if(mask & IRQ_IPC_SYNC)
		REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;
	leaveCriticalSection(oldIME);
}

//---------------------------------------------------------------------------------
void irqEnable(uint32 irq) {
//---------------------------------------------------------------------------------
	int oldIME = enterCriticalSection();
	if (irq & IRQ_VBLANK)
		REG_DISPSTAT |= DISP_VBLANK_IRQ ;
	if (irq & IRQ_HBLANK)
		REG_DISPSTAT |= DISP_HBLANK_IRQ ;
	if (irq & IRQ_VCOUNT)
		REG_DISPSTAT |= DISP_YTRIGGER_IRQ;
	if(irq & IRQ_IPC_SYNC)
		REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

	REG_IE |= irq;
	leaveCriticalSection(oldIME);
}

//---------------------------------------------------------------------------------
void irqDisable(uint32 irq) {
//---------------------------------------------------------------------------------
	int oldIME = enterCriticalSection();
	if (irq & IRQ_VBLANK)
		REG_DISPSTAT &= ~DISP_VBLANK_IRQ ;
	if (irq & IRQ_HBLANK)
		REG_DISPSTAT &= ~DISP_HBLANK_IRQ ;
	if (irq & IRQ_VCOUNT)
		REG_DISPSTAT &= ~DISP_YTRIGGER_IRQ;
	if(irq & IRQ_IPC_SYNC)
		REG_IPC_SYNC &= ~IPC_SYNC_IRQ_ENABLE;

	REG_IE &= ~irq;
	leaveCriticalSection(oldIME);
}

//---------------------------------------------------------------------------------
static void __irqClear(u32 mask, struct IntTable irqTable[]) {
//---------------------------------------------------------------------------------
	int i = 0;

	for	(i=0;i<MAX_INTERRUPTS;i++)
		if	(irqTable[i].mask == mask) break;

	if ( i == MAX_INTERRUPTS ) return;

	irqTable[i].handler	= irqDummy;
}

//---------------------------------------------------------------------------------
void irqClear(u32 mask) {
//---------------------------------------------------------------------------------
	int oldIME = enterCriticalSection();
	__irqClear(mask,irqTable);
	irqDisable( mask);
	leaveCriticalSection(oldIME);
}


