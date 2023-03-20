/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014-2015, Normmatt
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#ifndef _NO_SDMMC
#include <stddef.h> // NULL
#include <string.h>
#include <nds/ndstypes.h>
#include <nds/bios.h>
#include <nds/debug.h>

#include "my_sdmmc.h"
#include "my_disc_io.h"

#define DATA32_SUPPORT


static int ndmaSlot = 0;
struct mmcdevice handleNAND;
struct mmcdevice handleSD;

mmcdevice *getMMCDevice(int drive)
{
	if(drive==0) return &handleNAND;
	return &handleSD;
}

static int get_error(struct mmcdevice *ctx)
{
	return (int)((ctx->error << 29) >> 31);
}


static void set_target(struct mmcdevice *ctx)
{
	sdmmc_mask16(REG_SDPORTSEL,0x3,(u16)ctx->devicenumber);
	setckl(ctx->clk);
	if(ctx->SDOPT == 0)
	{
		sdmmc_mask16(REG_SDOPT,0,0x8000);
	}
	else
	{
		sdmmc_mask16(REG_SDOPT,0x8000,0);
	}
}

static void sdmmc_send_command(struct mmcdevice *ctx, u32 cmd, u32 args)
{
	const bool getSDRESP = (cmd << 15) >> 31;
	u16 flags = (cmd << 15) >> 31;
	const bool readdata = cmd & 0x20000;
	const bool writedata = cmd & 0x40000;

	if(readdata || writedata)
	{
		flags |= TMIO_STAT0_DATAEND;
	}

	ctx->error = 0;
	while((sdmmc_read16(REG_SDSTATUS1) & TMIO_STAT1_CMD_BUSY)); //mmc working?
	sdmmc_write16(REG_SDIRMASK0,0);
	sdmmc_write16(REG_SDIRMASK1,0);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);
	sdmmc_mask16(REG_DATACTL32,0x1800,0x400); // Disable TX32RQ and RX32RDY IRQ. Clear fifo.
	sdmmc_write16(REG_SDCMDARG0,args &0xFFFF);
	sdmmc_write16(REG_SDCMDARG1,args >> 16);
	sdmmc_write16(REG_SDCMD,cmd &0xFFFF);

	u32 size = ctx->size;
	const u16 blkSize = sdmmc_read16(REG_SDBLKLEN32);
	u32 *rDataPtr32 = (u32*)ctx->rData;
	u8  *rDataPtr8  = ctx->rData;
	const u32 *tDataPtr32 = (u32*)ctx->tData;
	const u8  *tDataPtr8  = ctx->tData;

	bool rUseBuf = ( NULL != rDataPtr32 );
	bool tUseBuf = ( NULL != tDataPtr32 );

	u16 status0 = 0;
	while(1)
	{
		volatile u16 status1 = sdmmc_read16(REG_SDSTATUS1);
#ifdef DATA32_SUPPORT
		volatile u16 ctl32 = sdmmc_read16(REG_DATACTL32);
		if((ctl32 & 0x100))
#else
		if((status1 & TMIO_STAT1_RXRDY))
#endif
		{
			if(readdata)
			{
				if(rUseBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_RXRDY, 0);
					if(size >= blkSize)
					{
						#ifdef DATA32_SUPPORT
                        // skip startOffset bytes at the beggining of the read
                        if(ctx->startOffset>0)
                        {
                            u32 skipped=0;
                            for(u32 skipped = 0; skipped < ctx->startOffset; skipped += 4)
                            {
                                sdmmc_read32(REG_SDFIFO32);
                            }
                            u32 remain = ctx->startOffset-skipped;
                            if(remain>0)
                            {
                                u32 data = sdmmc_read32(REG_SDFIFO32);
                                u8 data8[4];
                                u8* pdata8 = data8;
                                *pdata8++ = data;
    							*pdata8++ = data >> 8;
    							*pdata8++ = data >> 16;
    							*pdata8++ = data >> 24;
    							pdata8 = data8;
                                for (int i=0; i<remain; i++)
                                {
                                    pdata8++;
                                }
                                for (int i=0; i<4-remain; i++)
                                {
                                    *rDataPtr8++ = *pdata8++;
                                }
                            }
                        }
                        // copy data
                        u32 copied = 0;
						if(!((u32)rDataPtr32 & 3))
						{
							for(int i = 0; i < blkSize-ctx->startOffset-ctx->endOffset; i += 4)
							{
								*rDataPtr32++ = sdmmc_read32(REG_SDFIFO32);
								copied+=4;
							}
						}
						else
						{
							for(int i = 0; i < blkSize-ctx->startOffset-ctx->endOffset; i += 4)
							{
								u32 data = sdmmc_read32(REG_SDFIFO32);
								*rDataPtr8++ = data;
								*rDataPtr8++ = data >> 8;
								*rDataPtr8++ = data >> 16;
								*rDataPtr8++ = data >> 24;
								copied+=4;
							}
						}
                        // skip endOffset bytes at the end of the read
                        if(ctx->endOffset>0)
                        {
                          u32 remain = blkSize-ctx->startOffset-ctx->endOffset-copied;
                          if(remain)
                          {
                                u32 data = sdmmc_read32(REG_SDFIFO32);
                                u8 data8[4];
                                u8* pdata8 = data8;
                                *pdata8++ = data;
                                *pdata8++ = data >> 8;
                                *pdata8++ = data >> 16;
                                *pdata8++ = data >> 24;
                                pdata8 = data8;
                                for (int i=0; i<remain; i++)
                                {
                                    *rDataPtr8++ = *pdata8++;
                                }
                          }
                          for(u32 skipped=4-remain; skipped < ctx->endOffset; skipped += 4)
                          {
                              sdmmc_read32(REG_SDFIFO32);
                          }
                        }
						#else
                        // startOffset & endOffset NOT IMPLEMENTED in DATA16 mode
                        // copy data : this code seems wrong
						if(!((u32)rDataPtr16 & 1))
						{
							for(u32 i = 0; i < blkSize; i += 4)
							{
								*rDataPtr16++ = sdmmc_read16(REG_SDFIFO);
							}
						}
						else
						{
							for(u32 i = 0; i < blkSize; i += 4)
							{
								u16 data = sdmmc_read16(REG_SDFIFO);
								*rDataPtr8++ = data;
								*rDataPtr8++ = data >> 8;
							}
						}
						#endif
						size -= blkSize;
					}
				}

				sdmmc_mask16(REG_DATACTL32, 0x800, 0);
			}
		}
#ifdef DATA32_SUPPORT
		if(!(ctl32 & 0x200))
#else
		if((status1 & TMIO_STAT1_TXRQ))
#endif
		{
			if(writedata)
			{
				if(tUseBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_TXRQ, 0);
					if(size >= blkSize)
					{
						#ifdef DATA32_SUPPORT
						if(!((u32)tDataPtr32 & 3))
						{
							for(u32 i = 0; i < blkSize; i += 4)
							{
								sdmmc_write32(REG_SDFIFO32, *tDataPtr32++);
							}
						}
						else
						{
							for(u32 i = 0; i < blkSize; i += 4)
							{
								u32 data = *tDataPtr8++;
								data |= (u32)*tDataPtr8++ << 8;
								data |= (u32)*tDataPtr8++ << 16;
								data |= (u32)*tDataPtr8++ << 24;
								sdmmc_write32(REG_SDFIFO32, data);
							}
						}
						#else
						if(!((u32)tDataPtr16 & 1))
						{
							for(u32 i = 0; i < blkSize; i += 2)
							{
								sdmmc_write16(REG_SDFIFO, *tDataPtr16++);
							}
						}
						else
						{
							for(u32 i = 0; i < blkSize; i += 2)
							{
								u16 data = *tDataPtr8++;
								data |= (u16)(*tDataPtr8++ << 8);
								sdmmc_write16(REG_SDFIFO, data);
							}
						}
						#endif
						size -= blkSize;
					}
				}

				sdmmc_mask16(REG_DATACTL32, 0x1000, 0);
			}
		}
		if(status1 & TMIO_MASK_GW)
		{
			ctx->error |= 4;
			break;
		}

		if(!(status1 & TMIO_STAT1_CMD_BUSY))
		{
			status0 = sdmmc_read16(REG_SDSTATUS0);
			if(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_CMDRESPEND)
			{
				ctx->error |= 0x1;
			}
			if(status0 & TMIO_STAT0_DATAEND)
			{
				ctx->error |= 0x2;
			}

			if((status0 & flags) == flags)
				break;
		}
	}
	ctx->stat0 = sdmmc_read16(REG_SDSTATUS0);
	ctx->stat1 = sdmmc_read16(REG_SDSTATUS1);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);

	if(getSDRESP != 0)
	{
		ctx->ret[0] = (u32)(sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16));
		ctx->ret[1] = (u32)(sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16));
		ctx->ret[2] = (u32)(sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16));
		ctx->ret[3] = (u32)(sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16));
	}
}

static void sdmmc_send_command_nonblocking_ndma(struct mmcdevice *ctx, u32 cmd, u32 args)
{
	*((vu32*)0x4004100) = 0x80020000; //use round robin arbitration method;

	*(vu32*)((u32)0x4004104+0x1C*ndmaSlot) = 0x0400490C;
	*(vu32*)((u32)0x4004108+0x1C*ndmaSlot) = (u32)ctx->rData;

	*(vu32*)((u32)0x400410C+0x1C*ndmaSlot) = ctx->size;

	*(vu32*)((u32)0x4004110+0x1C*ndmaSlot) = 0x80;

	*(vu32*)((u32)0x4004114+0x1C*ndmaSlot) = 0x10;

	*(vu32*)((u32)0x400411C+0x1C*ndmaSlot) = 0xC8064000;

	//const bool getSDRESP = (cmd << 15) >> 31;
	u16 flags = (cmd << 15) >> 31;
	const bool readdata = cmd & 0x20000;
	const bool writedata = cmd & 0x40000;

	if(readdata || writedata)
	{
		flags |= TMIO_STAT0_DATAEND;
	}

	ctx->error = 0;
	while((sdmmc_read16(REG_SDSTATUS1) & TMIO_STAT1_CMD_BUSY)); //mmc working?
	sdmmc_write16(REG_SDIRMASK0,0);
	sdmmc_write16(REG_SDIRMASK1,0);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);
	sdmmc_mask16(REG_DATACTL32,0x1800,0x400); // Disable TX32RQ and RX32RDY IRQ. Clear fifo.
	sdmmc_write16(REG_SDCMDARG0,args &0xFFFF);
	sdmmc_write16(REG_SDCMDARG1,args >> 16);
	sdmmc_write16(REG_SDCMD,cmd &0xFFFF);

	u32 size = ctx->size;
	const u16 blkSize = sdmmc_read16(REG_SDBLKLEN32);
	//u32 *rDataPtr32 = (u32*)ctx->rData;
	//u8  *rDataPtr8  = ctx->rData;
	const u32 *tDataPtr32 = (u32*)ctx->tData;
	const u8  *tDataPtr8  = ctx->tData;

	//bool rUseBuf = ( NULL != rDataPtr32 );
	bool tUseBuf = ( NULL != tDataPtr32 );

    //nocashMessage("main loop");

	while(1)
	{
		volatile u16 status1 = sdmmc_read16(REG_SDSTATUS1);
#ifdef DATA32_SUPPORT
		volatile u16 ctl32 = sdmmc_read16(REG_DATACTL32);
		if((ctl32 & 0x100))
#else
		if((status1 & TMIO_STAT1_RXRDY))
#endif
		{
            /*
            // should not be needed : ndma unit is taking care of data transfer
            nocashMessage("readdata check");
			if(readdata)
			{
                nocashMessage("readdata");
				if(rUseBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_RXRDY, 0);
					if(size >= blkSize)
					{
						size -= blkSize;
					}
				}

				sdmmc_mask16(REG_DATACTL32, 0x800, 0);
			}*/
		}
#ifdef DATA32_SUPPORT
		if(!(ctl32 & 0x200))
#else
		if((status1 & TMIO_STAT1_TXRQ))
#endif
		{
            //nocashMessage("writedata check");
			if(writedata)
			{
                //nocashMessage("writedata");
				if(tUseBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_TXRQ, 0);
					if(size >= blkSize)
					{
						#ifdef DATA32_SUPPORT
						if(!((u32)tDataPtr32 & 3))
						{
							for(u32 i = 0; i < blkSize; i += 4)
							{
								sdmmc_write32(REG_SDFIFO32, *tDataPtr32++);
							}
						}
						else
						{
							for(u32 i = 0; i < blkSize; i += 4)
							{
								u32 data = *tDataPtr8++;
								data |= (u32)*tDataPtr8++ << 8;
								data |= (u32)*tDataPtr8++ << 16;
								data |= (u32)*tDataPtr8++ << 24;
								sdmmc_write32(REG_SDFIFO32, data);
							}
						}
						#else
						if(!((u32)tDataPtr16 & 1))
						{
							for(u32 i = 0; i < blkSize; i += 2)
							{
								sdmmc_write16(REG_SDFIFO, *tDataPtr16++);
							}
						}
						else
						{
							for(u32 i = 0; i < blkSize; i += 2)
							{
								u16 data = *tDataPtr8++;
								data |= (u16)(*tDataPtr8++ << 8);
								sdmmc_write16(REG_SDFIFO, data);
							}
						}
						#endif
						size -= blkSize;
					}
				}

				sdmmc_mask16(REG_DATACTL32, 0x1000, 0);
			}
		}
		if(status1 & TMIO_MASK_GW)
		{
            //nocashMessage("error 4");
			ctx->error |= 4;
			break;
		}
		if(status1 & TMIO_STAT1_CMD_BUSY)
		{
            // command is ongoing : return
            //nocashMessage("cmd busy");
            break;
		} 
        else if (strncmp((const char*)0x04FFFA00, "no$gba", 6) == 0)
        {
            // command is finished already without going busy : return
            // not supposed to happen
            // needed for no$gba only
            u16 status0 = sdmmc_read16(REG_SDSTATUS0);
            //nocashMessage("already finished");
            if((status0 & flags) == flags)
            break;
		}
	}
	//ctx->stat0 = sdmmc_read16(REG_SDSTATUS0);
	//ctx->stat1 = sdmmc_read16(REG_SDSTATUS1);
	//sdmmc_write16(REG_SDSTATUS0,0);
	//sdmmc_write16(REG_SDSTATUS1,0);

	/*if(getSDRESP != 0)
	{
		ctx->ret[0] = (u32)(sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16));
		ctx->ret[1] = (u32)(sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16));
		ctx->ret[2] = (u32)(sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16));
		ctx->ret[3] = (u32)(sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16));
	}*/
}

/* return true if the command is completed and false if it is still ongoing */
static bool sdmmc_check_command_ndma(struct mmcdevice *ctx, u32 cmd)
{
	const bool getSDRESP = (cmd << 15) >> 31;
    u16 flags = (cmd << 15) >> 31;
	const bool readdata = cmd & 0x20000;
	const bool writedata = cmd & 0x40000;

	if(readdata || writedata)
	{
		flags |= TMIO_STAT0_DATAEND;
	}

    volatile u16 status1 = sdmmc_read16(REG_SDSTATUS1);

    if(status1 & TMIO_MASK_GW)
	{
		ctx->error |= 4;

        ctx->stat0 = sdmmc_read16(REG_SDSTATUS0);
      	ctx->stat1 = sdmmc_read16(REG_SDSTATUS1);
      	sdmmc_write16(REG_SDSTATUS0,0);
      	sdmmc_write16(REG_SDSTATUS1,0);

      	if(getSDRESP != 0)
      	{
      		ctx->ret[0] = (u32)(sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16));
      		ctx->ret[1] = (u32)(sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16));
      		ctx->ret[2] = (u32)(sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16));
      		ctx->ret[3] = (u32)(sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16));
      	}
        *(vu32*)((u32)0x400411C+ndmaSlot*0x1C) = 0x48064000;
        return true;
    }

    if(!(status1 & TMIO_STAT1_CMD_BUSY))
	{
		u16 status0 = sdmmc_read16(REG_SDSTATUS0);
		if(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_CMDRESPEND)
		{
			ctx->error |= 0x1;
		}
		if(status0 & TMIO_STAT0_DATAEND)
		{
			ctx->error |= 0x2;
		}

        if((status0 & flags) != flags)
        {
            return false;
        }

        ctx->stat0 = sdmmc_read16(REG_SDSTATUS0);
      	ctx->stat1 = sdmmc_read16(REG_SDSTATUS1);
      	sdmmc_write16(REG_SDSTATUS0,0);
      	sdmmc_write16(REG_SDSTATUS1,0);

      	if(getSDRESP != 0)
      	{
      		ctx->ret[0] = (u32)(sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16));
      		ctx->ret[1] = (u32)(sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16));
      		ctx->ret[2] = (u32)(sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16));
      		ctx->ret[3] = (u32)(sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16));
      	}
        *(vu32*)((u32)0x400411C+ndmaSlot*0x1C) = 0x48064000; 
        return true;
	} else return false;
}

int my_sdmmc_sdcard_writesectors(u32 sector_no, u32 numsectors, const u8 *in)
{
	if(handleSD.isSDHC == 0) sector_no <<= 9;
	set_target(&handleSD);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleSD.tData = in;
	handleSD.size = numsectors << 9;
    handleSD.startOffset = 0;
    handleSD.endOffset = 0;
	// if (ndmaSlot < 0 || ndmaSlot > 3) {
		sdmmc_send_command(&handleSD,0x52C19,sector_no);
	/* } else { // Writes are bugged with NDMA
        //nocashMessage("my_sdmmc_sdcard_writesectors");
        sdmmc_send_command_nonblocking_ndma(&handleSD,0x52C19,sector_no);
        //nocashMessage("command sent");
        while(!sdmmc_check_command_ndma(&handleSD,0x52C19)) {}
        //nocashMessage("command checked");
	} */
	return get_error(&handleSD);
}

int my_sdmmc_sdcard_readsector(u32 sector_no, u8 *out, u32 startOffset, u32 endOffset)
{
	if(handleSD.isSDHC == 0) sector_no <<= 9;
	set_target(&handleSD);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,1);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,1);
	handleSD.rData = out;
	handleSD.size = 1 << 9;
    handleSD.startOffset = startOffset;
    handleSD.endOffset = endOffset;
	sdmmc_send_command(&handleSD,0x33C12,sector_no);
	return get_error(&handleSD);
}

int my_sdmmc_sdcard_readsectors(u32 sector_no, u32 numsectors, u8 *out)
{
	if(handleSD.isSDHC == 0) sector_no <<= 9;
	set_target(&handleSD);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleSD.rData = out;
	handleSD.size = numsectors << 9;
    handleSD.startOffset = 0;
    handleSD.endOffset = 0;
	if (ndmaSlot < 0 || ndmaSlot > 3) {
		sdmmc_send_command(&handleSD,0x33C12,sector_no);
	} else {
        //nocashMessage("my_sdmmc_sdcard_readsectors");
        sdmmc_send_command_nonblocking_ndma(&handleSD,0x33C12,sector_no);
        //nocashMessage("command sent");
        while(!sdmmc_check_command_ndma(&handleSD,0x33C12)) {}
        //nocashMessage("command checked");
	}
	return get_error(&handleSD);
}

bool my_sdmmc_sdcard_check_command(int cmd)
{
    return sdmmc_check_command_ndma(&handleSD,cmd);
}

int my_sdmmc_sdcard_readsectors_nonblocking(u32 sector_no, u32 numsectors, u8 *out)
{
	if(handleSD.isSDHC == 0) sector_no <<= 9;
	set_target(&handleSD);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleSD.rData = out;
	handleSD.size = numsectors << 9;
    handleSD.startOffset = 0;
    handleSD.endOffset = 0;
	if (ndmaSlot < 0 || ndmaSlot > 3) {
        // ndmaSlot needs to be valid
        return -1;
	} else {
        //nocashMessage("my_sdmmc_sdcard_readsectors");
        sdmmc_send_command_nonblocking_ndma(&handleSD,0x33C12,sector_no);
        //nocashMessage("command sent");
        /*while(!sdmmc_check_command_ndma(&handleSD,0x33C12)) {}
        nocashMessage("command checked");*/
	}
	return 0x33C12;
}

static u32 sdmmc_calc_size(u8* csd, int type)
{
  u32 result = 0;
  if(type == -1) type = csd[14] >> 6;
  switch(type)
  {
    case 0:
      {
        u32 block_len=csd[9]&0xf;
        block_len=1u<<block_len;
        u32 mult=( u32)((csd[4]>>7)|((csd[5]&3)<<1));
        mult=1u<<(mult+2);
        result=csd[8]&3;
        result=(result<<8)|csd[7];
        result=(result<<2)|(csd[6]>>6);
        result=(result+1)*mult*block_len/512;
      }
      break;
    case 1:
      result=csd[7]&0x3f;
      result=(result<<8)|csd[6];
      result=(result<<8)|csd[5];
      result=(result+1)*1024;
      break;
    default:
      break; //Do nothing otherwise FIXME perhaps return some error?
  }
  return result;
}

void sdmmc_init(void)
{
	//SD
	handleSD.isSDHC = 0;
	handleSD.SDOPT = 0;
	handleSD.res = 0;
	handleSD.initarg = 0;
	handleSD.clk = 0x20; // 523.655968 KHz
	handleSD.devicenumber = 0;

	/* *(vu16*)0x10006100 &= 0xF7FFu; //SDDATACTL32
	*(vu16*)0x10006100 &= 0xEFFFu; //SDDATACTL32
#ifdef DATA32_SUPPORT
	*(vu16*)0x10006100 |= 0x402u; //SDDATACTL32
#else
	*(vu16*)0x10006100 |= 0x402u; //SDDATACTL32
#endif
	*(vu16*)0x100060D8 = (*(vu16*)0x100060D8 & 0xFFDD) | 2;
#ifdef DATA32_SUPPORT
	*(vu16*)0x10006100 &= 0xFFFFu; //SDDATACTL32
	*(vu16*)0x100060D8 &= 0xFFDFu; //SDDATACTL
	*(vu16*)0x10006104 = 512; //SDBLKLEN32
#else
	*(vu16*)0x10006100 &= 0xFFFDu; //SDDATACTL32
	*(vu16*)0x100060D8 &= 0xFFDDu; //SDDATACTL
	*(vu16*)0x10006104 = 0; //SDBLKLEN32
#endif
	*(vu16*)0x10006108 = 1; //SDBLKCOUNT32
	*(vu16*)0x100060E0 &= 0xFFFEu; //SDRESET
	*(vu16*)0x100060E0 |= 1u; //SDRESET
	*(vu16*)0x10006020 |= TMIO_MASK_ALL; //SDIR_MASK0
	*(vu16*)0x10006022 |= TMIO_MASK_ALL>>16; //SDIR_MASK1
	*(vu16*)0x100060FC |= 0xDBu; //SDCTL_RESERVED7
	*(vu16*)0x100060FE |= 0xDBu; //SDCTL_RESERVED8
	*(vu16*)0x10006002 &= 0xFFFCu; //SDPORTSEL
#ifdef DATA32_SUPPORT
	*(vu16*)0x10006024 = 0x20;
	*(vu16*)0x10006028 = 0x40E9;
#else
	*(vu16*)0x10006024 = 0x40; //Nintendo sets this to 0x20
	*(vu16*)0x10006028 = 0x40E9; //Nintendo sets this to 0x40EE
#endif
	*(vu16*)0x10006002 &= 0xFFFCu; ////SDPORTSEL
	*(vu16*)0x10006026 = 512; //SDBLKLEN
	*(vu16*)0x10006008 = 0; //SDSTOP */
}

int SD_Init(void)
{
	// We need to send at least 74 clock pulses.
	set_target(&handleSD);
	swiDelay(0x1980); // ~75-76 clocks

    // card reset
    sdmmc_send_command(&handleSD,0,0);

    // CMD8 0x1AA
    sdmmc_send_command(&handleSD,0x10408,0x1AA);
    u32 temp = (handleSD.error & 0x1) << 0x1E;

    u32 temp2 = 0;
    do {
        do {
            // CMD55
            sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
            // ACMD41
            sdmmc_send_command(&handleSD,0x10769,0x00FF8000 | temp);
            temp2 = 1;
        } while ( !(handleSD.error & 1) );

    } while((handleSD.ret[0] & 0x80000000) == 0);

    if(!((handleSD.ret[0] >> 30) & 1) || !temp)
        temp2 = 0;

    handleSD.isSDHC = temp2;

    sdmmc_send_command(&handleSD,0x10602,0);
    if (handleSD.error & 0x4) return -1;

    sdmmc_send_command(&handleSD,0x10403,0);
    if (handleSD.error & 0x4) return -1;
    handleSD.initarg = handleSD.ret[0] >> 0x10;

    sdmmc_send_command(&handleSD,0x10609,handleSD.initarg << 0x10);
    if (handleSD.error & 0x4) return -1;

    handleSD.total_size = sdmmc_calc_size((u8*)&handleSD.ret[0],-1);
    handleSD.clk = 1;
    setckl(1);

    sdmmc_send_command(&handleSD,0x10507,handleSD.initarg << 0x10);
    if (handleSD.error & 0x4) return -1;

    // CMD55
    sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
    if (handleSD.error & 0x4) return -1;

    // ACMD42
    sdmmc_send_command(&handleSD,0x1076A,0x0);
    if (handleSD.error & 0x4) return -1;

    // CMD55
    sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
    if (handleSD.error & 0x4) return -7;

    handleSD.SDOPT = 1;
    sdmmc_send_command(&handleSD,0x10446,0x2);
    if (handleSD.error & 0x4) return -8;

    sdmmc_send_command(&handleSD,0x1040D,handleSD.initarg << 0x10);
    if (handleSD.error & 0x4) return -9;

    sdmmc_send_command(&handleSD,0x10410,0x200);
    if (handleSD.error & 0x4) return -10;
    handleSD.clk |= 0x200;

	return 0;
}

int my_sdmmc_get_cid(bool isNand, u32 *info)
{
	struct mmcdevice *device;
	if(isNand)
		device = &handleNAND;
	else
		device = &handleSD;

	set_target(device);
	// use cmd7 to put sd card in standby mode
	// CMD7
	{
		sdmmc_send_command(device,0x10507,0);
		//if((device->error & 0x4)) return -1;
	}

	// get sd card info
	// use cmd10 to read CID
	{
		sdmmc_send_command(device,0x1060A,device->initarg << 0x10);
		//if((device->error & 0x4)) return -2;

		for( int i = 0; i < 4; ++i ) {
			info[i] = device->ret[i];
		}
	}

	// put sd card back to transfer mode
	// CMD7
	{
		sdmmc_send_command(device,0x10507,device->initarg << 0x10);
		//if((device->error & 0x4)) return -3;
	}

	return 0;
}
#endif