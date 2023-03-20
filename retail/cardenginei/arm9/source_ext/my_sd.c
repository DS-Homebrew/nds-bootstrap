#include <nds/bios.h>
#include <nds/system.h>
#include <nds/ipc.h>

#include "cardengine_header_arm9.h"
#include "my_disc_io.h"
#include "my_sdmmc.h"

#define isSdk5 BIT(5)

extern cardengineArm9* volatile ce9;

extern bool isDma;
extern bool dmaOn;
extern void sleepMs(int ms);

/*! \fn DC_FlushRange(const void *base, u32 size)
	\brief flush the data cache for a range of addresses to memory.
	\param base base address of the region to flush.
	\param size size of the region to flush.
*/
void	DC_FlushRange(const void *base, u32 size);


/*! \fn DC_InvalidateAll()
	\brief invalidate the entire data cache.
*/
void	DC_InvalidateAll();


/*! \fn DC_InvalidateRange(const void *base, u32 size)
	\brief invalidate the data cache for a range of addresses.
	\param base base address of the region to invalidate
	\param size size of the region to invalidate.
*/
void	DC_InvalidateRange(const void *base, u32 size);

extern vu32* volatile sharedAddr;

/*-----------------------------------------------------------------
startUp
Initialize the interface, geting it into an idle, ready state
returns true if successful, otherwise returns false
-----------------------------------------------------------------*/
bool my_sdio_Startup(void) {
	#ifdef DEBUG
	nocashMessage("startup internal");
	#endif
	return true;
}

/*-----------------------------------------------------------------
isInserted
Is a card inserted?
return true if a card is inserted and usable
-----------------------------------------------------------------*/
bool my_sdio_IsInserted(void) {
	#ifdef DEBUG
	nocashMessage("isInserted internal");
	#endif
	return true;
}

/*-----------------------------------------------------------------
readSector
Read 1 512-byte sized sectors from the card into "buffer", 
starting at "sector". 
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool my_sdio_ReadSector(sec_t sector, void* buffer, u32 startOffset, u32 endOffset) {
	#ifdef DEBUG
	nocashMessage("readSector internal");
	#endif

	#ifdef TWLSDK
	DC_InvalidateRange(buffer, 512);
	#else
	if ((ce9->valueBits & isSdk5) || ((u32)buffer >= 0x02000000 && (u32)buffer < 0x03000000)) {
		DC_InvalidateRange(buffer, 512);
	}
	#endif

	u32 commandRead = isDma ? 0x53444D31 : 0x53445231;

	sharedAddr[0] = sector;
	sharedAddr[1] = (vu32)buffer;
	sharedAddr[2] = startOffset;
	sharedAddr[3] = endOffset;
	sharedAddr[4] = commandRead;

	if (dmaOn) IPC_SendSync(0x4);
	while (sharedAddr[4] == commandRead) {
		sleepMs(1);
	}
	return sharedAddr[4] == 0;
}

/*-----------------------------------------------------------------
readSectors
Read "numSectors" 512-byte sized sectors from the card into "buffer", 
starting at "sector". 
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool my_sdio_ReadSectors(sec_t sector, sec_t numSectors, void* buffer) {
	#ifdef DEBUG
	nocashMessage("readSectors internal");
	#endif

	#ifdef TWLSDK
	DC_InvalidateRange(buffer, numSectors * 512);
	#else
	if ((ce9->valueBits & isSdk5) || ((u32)buffer >= 0x02000000 && (u32)buffer < 0x03000000)) {
		DC_InvalidateRange(buffer, numSectors * 512);
	}
	#endif

	u32 commandRead = isDma ? 0x53444D41 : 0x53445244;

	sharedAddr[0] = sector;
	sharedAddr[1] = numSectors;
	sharedAddr[2] = (vu32)buffer;
	sharedAddr[4] = commandRead;

	while (sharedAddr[4] == commandRead) {
		if (dmaOn) IPC_SendSync(0x4);
		sleepMs(1);
	}
	return sharedAddr[4] == 0;
}

/*-----------------------------------------------------------------
readSectors
Read "numSectors" 512-byte sized sectors from the card into "buffer", 
starting at "sector". 
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
int my_sdio_ReadSectors_nonblocking(sec_t sector, sec_t numSectors, void* buffer) {
	#ifdef DEBUG
	nocashMessage("my_sdio_ReadSectors_nonblocking");
	#endif

	/*#ifndef UNCACHED
	DC_InvalidateRange(buffer, numSectors * 512);
	#endif

	u32 commandRead = 0x53415244;

	sharedAddr[0] = sector;
	sharedAddr[1] = numSectors;
	sharedAddr[2] = (vu32)buffer;
	sharedAddr[4] = commandRead;

	while (sharedAddr[4] == commandRead);
	return sharedAddr[4];*/
	return false;
}

bool  my_sdio_check_command(int cmd) {
	#ifdef DEBUG
	nocashMessage("my_sdio_check_command");
	#endif

	/*u32 commandCheck = 0x53444348;

	sharedAddr[0] = cmd;
	sharedAddr[4] = commandCheck;

	while (sharedAddr[4] == commandCheck);
	return sharedAddr[4];*/
	return false;
}

/*-----------------------------------------------------------------
writeSectors
Write "numSectors" 512-byte sized sectors from "buffer" to the card, 
starting at "sector".
The buffer may be unaligned, and the driver must deal with this correctly.
return true if it was successful, false if it failed for any reason
-----------------------------------------------------------------*/
bool my_sdio_WriteSectors(sec_t sector, sec_t numSectors, const void* buffer) {
	#ifdef DEBUG
	nocashMessage("writeSectors internal");
	#endif

	u32 commandWrite = 0x53445752;

	sharedAddr[0] = sector;
	sharedAddr[1] = numSectors;
	sharedAddr[2] = (vu32)buffer;
	sharedAddr[4] = commandWrite;

	if (dmaOn) IPC_SendSync(0x4);
	while (sharedAddr[4] == commandWrite) {
		sleepMs(1);
	}
	return sharedAddr[4] == 0;
}


/*-----------------------------------------------------------------
clearStatus
Reset the card, clearing any status errors
return true if the card is idle and ready
-----------------------------------------------------------------*/
bool my_sdio_ClearStatus(void) {
	#ifdef DEBUG
	nocashMessage("clearStatus internal");
	#endif
	return true;
}

/*-----------------------------------------------------------------
shutdown
shutdown the card, performing any needed cleanup operations
Don't expect this function to be called before power off, 
it is merely for disabling the card.
return true if the card is no longer active
-----------------------------------------------------------------*/
bool my_sdio_Shutdown(void) {
	#ifdef DEBUG	
	nocashMessage("shutdown internal");
	#endif	
	return true;
}

const NEW_DISC_INTERFACE __myio_dsisd = {
	DEVICE_TYPE_DSI_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
	(FN_MEDIUM_STARTUP)&my_sdio_Startup,
	(FN_MEDIUM_ISINSERTED)&my_sdio_IsInserted,
    (FN_MEDIUM_READSECTOR)&my_sdio_ReadSector,
	(FN_MEDIUM_READSECTORS)&my_sdio_ReadSectors,
    (FN_MEDIUM_READSECTORS_NONBLOCKING)&my_sdio_ReadSectors_nonblocking,
    (FN_MEDIUM_CHECK_COMMAND)&my_sdio_check_command,
	(FN_MEDIUM_WRITESECTORS)&my_sdio_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&my_sdio_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)&my_sdio_Shutdown
};
