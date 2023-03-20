#include "my_disc_io.h"
#include "my_sdmmc.h"

/*-----------------------------------------------------------------
startUp
Initialize the interface, geting it into an idle, ready state
returns true if successful, otherwise returns false
-----------------------------------------------------------------*/
bool my_sdio_Startup(void) {
	#ifdef DEBUG
	nocashMessage("startup internal");
	#endif
	sdmmc_init();
	return SD_Init() == 0;
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
	return my_sdmmc_sdcard_readsector(sector, buffer, startOffset, endOffset) == 0;
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
	return my_sdmmc_sdcard_readsectors(sector, numSectors, buffer) == 0;
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
	return my_sdmmc_sdcard_readsectors_nonblocking(sector, numSectors, buffer);
}

bool  my_sdio_check_command(int cmd) {
	#ifdef DEBUG
	nocashMessage("my_sdio_check_command");
	#endif
	return my_sdmmc_sdcard_check_command(cmd);
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
	return my_sdmmc_sdcard_writesectors(sector, numSectors, buffer) == 0;
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

const DISC_INTERFACE __myio_dsisd = {
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
