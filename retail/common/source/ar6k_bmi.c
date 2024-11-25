// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#include "ar6k_common.h"
#include <string.h>

bool ar6kBmiBufferSend(Ar6kDev* dev, const void* buffer, size_t size)
{
	u32 credits = 0;
	while (!credits) {
		if (!sdioCardReadExtended(dev->sdio, 1, 0x000450, &credits, 4)) {
			return false;
		}
		credits &= 0xff;
	}

	return sdioCardWriteExtended(dev->sdio, 1, 0x000800 + 0x800 - size, buffer, size);
}

bool ar6kBmiBufferRecv(Ar6kDev* dev, void* buffer, size_t size)
{
	// TODO: why does official code not have the credits loop?
	return sdioCardReadExtended(dev->sdio, 1, 0x000800 + 0x800 - size, buffer, size);
}

bool ar6kBmiDone(Ar6kDev* dev)
{
	struct {
		u32 id;
	} cmd = { .id = Ar6kBmiCmd_Done };

	return ar6kBmiBufferSend(dev, &cmd, sizeof(cmd));
}

typedef struct _Ar6kBmiReadWriteMemCmdHdr {
	u32 id;
	u32 addr;
	u32 len;
} _Ar6kBmiReadWriteMemCmdHdr;

bool ar6kBmiReadMemory(Ar6kDev* dev, u32 addr, void* buf, size_t len)
{
	const size_t maxsize = 0x200;
	_Ar6kBmiReadWriteMemCmdHdr cmd;

	while (len) {
		size_t cursize = len > maxsize ? maxsize : len;

		cmd.id = Ar6kBmiCmd_ReadMemory;
		cmd.addr = addr;
		cmd.len = cursize;

		if (!ar6kBmiBufferSend(dev, &cmd, sizeof(cmd)) || !ar6kBmiBufferRecv(dev, buf, cursize)) {
			return false;
		}

		addr += cursize;
		buf = (u8*)buf + cursize;
		len -= cursize;
	}
	return true;
}

bool ar6kBmiWriteMemory(Ar6kDev* dev, u32 addr, const void* buf, size_t len)
{
	const size_t maxsize = 0x200-sizeof(_Ar6kBmiReadWriteMemCmdHdr);

	struct {
		_Ar6kBmiReadWriteMemCmdHdr hdr;
		u8 payload[len > maxsize ? maxsize : len];
	} cmd;

	while (len) {
		size_t cursize = len > maxsize ? maxsize : len;

		cmd.hdr.id = Ar6kBmiCmd_WriteMemory;
		cmd.hdr.addr = addr;
		cmd.hdr.len = cursize;
		memcpy(cmd.payload, buf, cursize);

		if (!ar6kBmiBufferSend(dev, &cmd, sizeof(cmd.hdr) + cursize)) {
			return false;
		}

		addr += cursize;
		buf = (u8*)buf + cursize;
		len -= cursize;
	}
	return true;
}

bool ar6kBmiReadSocReg(Ar6kDev* dev, u32 addr, u32* out)
{
	struct {
		u32 id;
		u32 addr;
	} cmd = { .id = Ar6kBmiCmd_ReadSocRegister, .addr = addr };

	return ar6kBmiBufferSend(dev, &cmd, sizeof(cmd)) && ar6kBmiBufferRecv(dev, out, sizeof(u32));
}

bool ar6kBmiWriteSocReg(Ar6kDev* dev, u32 addr, u32 value)
{
	struct {
		u32 id;
		u32 addr;
		u32 value;
	} cmd = {
		.id = Ar6kBmiCmd_WriteSocRegister,
		.addr = addr,
		.value = value,
	};

	return ar6kBmiBufferSend(dev, &cmd, sizeof(cmd));
}

bool ar6kBmiGetTargetInfo(Ar6kDev* dev, Ar6kBmiTargetInfo* info)
{
	struct {
		u32 id;
	} cmd = { .id = Ar6kBmiCmd_GetTargetInfo };

	if (!ar6kBmiBufferSend(dev, &cmd, sizeof(cmd))) {
		return false;
	}

	if (!ar6kBmiBufferRecv(dev, &info->target_ver, sizeof(u32))) {
		return false;
	}

	if (info->target_ver == UINT32_MAX) { // TARGET_VERSION_SENTINAL (sic)
		// Read size of target info struct
		u32 total_size = 0;
		if (!ar6kBmiBufferRecv(dev, &total_size, sizeof(u32))) {
			return false;
		}

		// Ensures it matches our size
		if (total_size != sizeof(u32) + sizeof(*info)) {
			dietPrint("bmiTargetInfo bad size %lu\n", total_size);
			return false;
		}

		// Read the target info, this time for real
		return ar6kBmiBufferRecv(dev, info, sizeof(*info));
	} else {
		// Older protocol - fill in the gaps
		info->target_type = 1; // TARGET_TYPE_AR6001
		return true;
	}
}
