// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include "ar6k_base.h"

MK_EXTERN_C_START

typedef enum Ar6kBmiCommand {
	Ar6kBmiCmd_NoCommand          = 0,
	Ar6kBmiCmd_Done               = 1,
	Ar6kBmiCmd_ReadMemory         = 2,
	Ar6kBmiCmd_WriteMemory        = 3,
	Ar6kBmiCmd_Execute            = 4,
	Ar6kBmiCmd_SetAppStart        = 5,
	Ar6kBmiCmd_ReadSocRegister    = 6,
	Ar6kBmiCmd_WriteSocRegister   = 7,
	Ar6kBmiCmd_GetTargetInfo      = 8,
	Ar6kBmiCmd_RomPatchInstall    = 9,
	Ar6kBmiCmd_RomPatchUninstall  = 10,
	Ar6kBmiCmd_RomPatchActivate   = 11,
	Ar6kBmiCmd_RomPatchDeactivate = 12,
	Ar6kBmiCmd_LzStreamStart      = 13,
	Ar6kBmiCmd_LzData             = 14,
	Ar6kBmiCmd_NvramProcess       = 15,
} Ar6kBmiCommand;

typedef struct Ar6kBmiTargetInfo {
	u32 target_ver;
	u32 target_type;
} Ar6kBmiTargetInfo;

bool ar6kBmiBufferSend(Ar6kDev* dev, const void* buf, size_t len);
bool ar6kBmiBufferRecv(Ar6kDev* dev, void* buf, size_t len);

bool ar6kBmiDone(Ar6kDev* dev);
bool ar6kBmiReadMemory(Ar6kDev* dev, u32 addr, void* buf, size_t len);
bool ar6kBmiWriteMemory(Ar6kDev* dev, u32 addr, const void* buf, size_t len);
bool ar6kBmiReadSocReg(Ar6kDev* dev, u32 addr, u32* out);
bool ar6kBmiWriteSocReg(Ar6kDev* dev, u32 addr, u32 value);
bool ar6kBmiGetTargetInfo(Ar6kDev* dev, Ar6kBmiTargetInfo* info);

MK_INLINE bool ar6kBmiWriteMemoryWord(Ar6kDev* dev, u32 addr, u32 value)
{
	return ar6kBmiWriteMemory(dev, addr, &value, sizeof(u32));
}

MK_EXTERN_C_END
