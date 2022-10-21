#ifndef DSI_NDMA_INCLUDE
#define DSI_NDMA_INCLUDE

#include <nds/ndstypes.h>

static inline
/*! \fn void ndmaCopyWords(uint8 channel, const void* src, void* dest, uint32 size)
    \brief copies from source to destination on one of the 4 available channels in words
    \param channel the dma channel to use (0 - 3).
    \param src the source to copy from
    \param dest the destination to copy to
    \param size the size in bytes of the data to copy.  Will be truncated to the nearest word (4 bytes)
*/
void ndmaCopyWords(uint8 ndmaSlot, const void* src, void* dest, uint32 size) {
	*(vu32*)(0x4004104+(0x1C*ndmaSlot)) = (u32)src;
	*(vu32*)(0x4004108+(0x1C*ndmaSlot)) = (u32)dest;

	*(vu32*)(0x4004110+(0x1C*ndmaSlot)) = size/4;	

    *(vu32*)(0x4004114+(0x1C*ndmaSlot)) = 0x1;

	*(vu32*)(0x400411C+(0x1C*ndmaSlot)) = 0x90070000;

	while (*(vu32*)(0x400411C+(0x1C*ndmaSlot)) & BIT(31));
}

static inline
/*! \fn void ndmaCopyWordsAsynch(uint8 channel, const void* src, void* dest, uint32 size)
\brief copies from source to destination on one of the 4 available channels in half words.  
This function returns immediately after starting the transfer.
\param channel the dma channel to use (0 - 3).
\param src the source to copy from
\param dest the destination to copy to
\param size the size in bytes of the data to copy.  Will be truncated to the nearest word (4 bytes)
*/
void ndmaCopyWordsAsynch(uint8 ndmaSlot, const void* src, void* dest, uint32 size) {
	*(vu32*)(0x4004104+(0x1C*ndmaSlot)) = (u32)src;
	*(vu32*)(0x4004108+(0x1C*ndmaSlot)) = (u32)dest;

	*(vu32*)(0x4004110+(0x1C*ndmaSlot)) = size/4;	

    *(vu32*)(0x4004114+(0x1C*ndmaSlot)) = 0x1;

	*(vu32*)(0x400411C+(0x1C*ndmaSlot)) = 0x90070000;
}

static inline 
bool ndmaBusy(uint8 ndmaSlot) {
	return (*(vu32*)(0x400411C+(0x1C*ndmaSlot)) & BIT(31));
}

/*static inline 
bool ndmaEnabled(void) {
	return (REG_SCFG_EXT & BIT(16));
}*/

#endif