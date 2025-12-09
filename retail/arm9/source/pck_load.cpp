#include <nds.h>
#include <string>
#include <string.h>
#include <fat.h>
#include "configuration.h"
#include "nitrofs.h"

extern off_t getFileSize(const char* path);
extern off_t getFileSize(FILE* fp);

static const char* getSdatPath(const char* romTid) {
	if (strncmp(romTid, "CDZ", 3) == 0 // Dragon Ball: Origins
	 || strncmp(romTid, "BDB", 3) == 0) { // Dragon Ball: Origins 2
		return "rom:/sound/data.sdat";
	} else if (strncmp(romTid, "AOS", 3) == 0) { // Elite Beat Agents & Osu! Tatakae! Ouendan
		return "rom:/data/sound_data.sdat";
	} else if (strncmp(romTid, "ADA", 3) == 0 // Mainline Gen 4 Pokemon games
			|| strncmp(romTid, "APA", 3) == 0) {
		return "rom:/data/sound/sound_data.sdat";
	} else if (strncmp(romTid, "CPU", 3) == 0) {
		return "rom:/data/sound/pl_sound_data.sdat";
	} else if (strncmp(romTid, "IPK", 3) == 0
			|| strncmp(romTid, "IPG", 3) == 0) {
		return "rom:/data/sound/gs_sound_data.sdat";
	} else if (strncmp(romTid, "ASC", 3) == 0) { // Sonic Rush
		return "rom:/snd/sys/sound_data.sdat";
	} else if (strncmp(romTid, "BJM", 3) == 0	// Stitch Jam
			|| strncmp(romTid, "B3I", 3) == 0) { // Stitch Jam 2
		return "rom:/sound_data.sdat";
	} else if (strncmp(romTid, "AYG", 3) == 0) { // Yu-Gi-Oh!: Nightmare Troubadour
		return "rom:/sound/sound_data.sdat";
	}
	return "rom:/NULL.sdat";
}

static u32 getSdatStrmId(const char* romTid) {
	if (strncmp(romTid, "ASC", 3) == 0) { // Sonic Rush
		return 0x69;
	} else if (strcmp(romTid, "AOSJ") == 0) { // Osu! Tatakae! Ouendan
		return 0x102;
	} else if (strncmp(romTid, "AYG", 3) == 0) { // Yu-Gi-Oh!: Nightmare Troubadour
		return (romTid[3] == 'P') ? 0x102 : 0x107;
	}
	return 0xFFFFFFFF;
}

bool loadPreLoadSettings(configuration* conf, const char* pckPath, const char* romTid, const u16 headerCRC) {
	FILE *file = NULL;
	bool openSdat = false;

	if (strncmp(romTid, "CLF", 3) == 0) { // Code Lyoko: Fall of X.A.N.A.
		if (romFSInit(conf->ndsPath)) {
			file = fopen("rom:/data.bf", "rb");
		}
	} else
	// Pre-load sound data
	if (strncmp(romTid, "CDZ", 3) == 0 // Dragon Ball: Origins
	 || strncmp(romTid, "BDB", 3) == 0 // Dragon Ball: Origins 2
	 || strncmp(romTid, "ADA", 3) == 0 // Mainline Gen 4 Pokemon games
	 || strncmp(romTid, "APA", 3) == 0
	 || strncmp(romTid, "CPU", 3) == 0
	 || strncmp(romTid, "IPK", 3) == 0
	 || strncmp(romTid, "IPG", 3) == 0) {
		openSdat = true;
	} else if (conf->consoleModel > 0 &&
			  (strncmp(romTid, "BJM", 3) == 0	// Stitch Jam
			|| strncmp(romTid, "B3I", 3) == 0)) { // Stitch Jam 2
		openSdat = true;
	} else if (conf->consoleModel > 0 && strncmp(romTid, "B3R", 3) == 0) { // Pokemon Ranger: Guardian Signs
		if (romFSInit(conf->ndsPath)) {
			file = fopen((romTid[3] == 'J') ? "rom:/data/data_game.acf": "rom:/data/data_game_us.acf", "rb");
		}
	}

	if (openSdat) {
		if (romFSInit(conf->ndsPath)) {
			file = fopen(getSdatPath(romTid), "rb");
		}
	}

	if (file) {
		conf->dataToPreloadAddr[0] = offsetOfOpenedNitroFile;
		conf->dataToPreloadSize[0] = getFileSize(file);
		// conf->dataToPreloadFrame = 0;
		fclose(file);
		if (strncmp(romTid, "BDB", 3) == 0) {
			file = fopen("rom:/sound/datastr.sdat", "rb");
			if (file) {
				conf->dataToPreloadAddr[1] = offsetOfOpenedNitroFile;
				conf->dataToPreloadSize[1] = getFileSize(file);
				fclose(file);
			}
		}
		return true;
	}

	file = fopen(pckPath, "rb");
	if (!file) {
		return false;
	}

	char buf[5] = {0};
	fread(buf, 1, 4, file);
	if (strcmp(buf, ".PCK") != 0) {
		return false;
	}

	u32 fileCount;
	fread(&fileCount, 1, sizeof(fileCount), file);

	u32 offset = 0, size = 0;

	// Try binary search for the game
	int left = 0;
	int right = fileCount;

	while (left <= right) {
		int mid = left + ((right - left) / 2);
		fseek(file, 16 + mid * 16, SEEK_SET);
		fread(buf, 1, 4, file);
		int cmp = strcmp(buf, romTid);
		if (cmp == 0) { // TID matches, check CRC
			u16 crc;
			fread(&crc, 1, sizeof(crc), file);

			if (crc == headerCRC) { // CRC matches
				fread(&offset, 1, sizeof(offset), file);
				fread(&size, 1, sizeof(size), file);
				break;
			} else if (crc < headerCRC) {
				left = mid + 1;
			} else {
				right = mid - 1;
			}
		} else if (cmp < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	if (offset > 0) {
		if (size > 0x20) {
			size = 0x20;
		}
		fseek(file, offset, SEEK_SET);
		u32 *buffer = new u32[size/4];
		fread(buffer, 1, size, file);

		for (u32 i = 0; i < size/8; i++) {
			conf->dataToPreloadAddr[i] = buffer[0+(i*2)];
			conf->dataToPreloadSize[i] = buffer[1+(i*2)];
		}
		// conf->dataToPreloadFrame = (size == 0xC) ? buffer[2] : 0;
		delete[] buffer;
	} else {
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

void loadAsyncLoadSettings(configuration* conf, const char* romTid, const u16 headerCRC) {
	// Set data to be asynchrously loadable
	FILE *file = NULL;
	u32 sizeOverride = 0;
	u32 sdatFileId = 0xFFFFFFFF;
	bool readStrmFile = false;

	if (strncmp(romTid, "ASC", 3) == 0 // Sonic Rush
	 || strcmp(romTid, "AOSJ") == 0 // Osu! Tatakae! Ouendan
	 || strncmp(romTid, "AYG", 3) == 0) { // Yu-Gi-Oh!: Nightmare Troubadour
		if (romFSInit(conf->ndsPath)) {
			file = fopen(getSdatPath(romTid), "rb");
			sdatFileId = getSdatStrmId(romTid);
			readStrmFile = true;
		}
	} /* else if (strncmp(romTid, "DSY", 3) == 0) { // System Flaw
		if (romFSInit(conf->ndsPath)) {
			file = fopen("rom:/data/mobiclip/intro.avi.mods", "rb");
		}
	} */

	if (file && readStrmFile) {
		const u32 sdatSize = getFileSize(file);
		u32 sdatFatOffset = 0;
		u32 sdatFatString = 0;
		u32 sdatFatSize = 0;
		u32 sdatFatFiles = 0;

		fseek(file, 0x20, SEEK_SET);
		fread(&sdatFatOffset, 1, sizeof(u32), file);
		fseek(file, sdatFatOffset, SEEK_SET);
		fread(&sdatFatString, 1, sizeof(u32), file);
		if (sdatFatString == 0x20544146) { // 'FAT '
			fread(&sdatFatSize, 1, sizeof(u32), file);
			fread(&sdatFatFiles, 1, sizeof(u32), file);
			sdatFatOffset += 0xC;
			sdatFatSize -= 0xC;

			// Fast method
			if (sdatFileId == 0xFFFFFFFF) {
				sdatFatSize -= 0x10;
				sdatFatOffset += sdatFatSize;
			} else {
				sdatFatOffset += 0x10*sdatFileId;
			}

			fseek(file, sdatFatOffset, SEEK_SET);
			u32 info[4];
			fread(info, 4, sizeof(u32), file);
			fseek(file, info[0], SEEK_SET);
			u32 string = 0;
			fread(&string, 1, sizeof(u32), file);
			if (string == 0x4D525453) { // 'STRM'
				offsetOfOpenedNitroFile += info[0];
				sizeOverride = (sdatFileId == 0xFFFFFFFF) ? info[1] : (sdatSize-info[0]);
			}

			// Slow method
			/* for (u32 i = 0; i < sdatFatFiles; i++) {
				fseek(file, sdatFatOffset, SEEK_SET);
				u32 info[4];
				fread(info, 4, sizeof(u32), file);
				fseek(file, info[0], SEEK_SET);
				u32 string = 0;
				fread(&string, 1, sizeof(u32), file);
				if (string == 0x4D525453) { // 'STRM'
					offsetOfOpenedNitroFile += info[0];
					sizeOverride = info[1];
					break;
				}
				sdatFatOffset += 0x10;
			} */

			if (sizeOverride == 0) {
				fclose(file);
			}
		} else {
			fclose(file);
		}
	}

	if (file) {
		conf->asyncDataAddr[0] = offsetOfOpenedNitroFile;
		conf->asyncDataSize[0] = sizeOverride ? sizeOverride : getFileSize(file);
		fclose(file);
		/* if (strncmp(romTid, "DSY", 3) == 0) {
			file = fopen("rom:/data/mobiclip/outro.avi.mods", "rb");
			if (file) {
				conf->asyncDataAddr[1] = offsetOfOpenedNitroFile;
				conf->asyncDataSize[1] = getFileSize(file);
				fclose(file);
			}
		} */
		return;
	}

	/* file = fopen("nitro:/asyncLoadSettings.pck", "rb");
	if (!file) {
		return;
	}

	char buf[5] = {0};
	fread(buf, 1, 4, file);
	if (strcmp(buf, ".PCK") != 0) {
		return;
	}

	u32 fileCount;
	fread(&fileCount, 1, sizeof(fileCount), file);

	u32 offset = 0, size = 0;

	// Try binary search for the game
	int left = 0;
	int right = fileCount;

	while (left <= right) {
		int mid = left + ((right - left) / 2);
		fseek(file, 16 + mid * 16, SEEK_SET);
		fread(buf, 1, 4, file);
		int cmp = strcmp(buf, romTid);
		if (cmp == 0) { // TID matches, check CRC
			u16 crc;
			fread(&crc, 1, sizeof(crc), file);

			if (crc == headerCRC) { // CRC matches
				fread(&offset, 1, sizeof(offset), file);
				fread(&size, 1, sizeof(size), file);
				break;
			} else if (crc < headerCRC) {
				left = mid + 1;
			} else {
				right = mid - 1;
			}
		} else if (cmp < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	if (offset > 0) {
		if (size > 0x10) {
			size = 0x10;
		}
		fseek(file, offset, SEEK_SET);
		u32 *buffer = new u32[size/4];
		fread(buffer, 1, size, file);

		for (u32 i = 0; i < size/8; i++) {
			conf->asyncDataAddr[i] = buffer[0+(i*2)];
			conf->asyncDataSize[i] = buffer[1+(i*2)];
		}
		delete[] buffer;
	}

	fclose(file); */
}

void loadApFix(configuration* conf, const char* bootstrapPath, const char* romTid, const u16 headerCRC) {
	{
		std::string romFilename = conf->ndsPath;
		const size_t last_slash_idx = romFilename.find_last_of("/");
		if (std::string::npos != last_slash_idx)
		{
			romFilename.erase(0, last_slash_idx + 1);
		}

		sprintf(conf->apPatchPath, "%s:/_nds/nds-bootstrap/apFix/%s.ips", conf->bootstrapOnFlashcard ? "fat" : "sd", romFilename.c_str());
		if (access(conf->apPatchPath, F_OK) == 0) {
			conf->apPatchSize = getFileSize(conf->apPatchPath);
			return;
		}

		sprintf(conf->apPatchPath, "%s:/_nds/nds-bootstrap/apFix/%s.bin", conf->bootstrapOnFlashcard ? "fat" : "sd", romFilename.c_str());
		if (access(conf->apPatchPath, F_OK) == 0) {
			conf->apPatchSize = getFileSize(conf->apPatchPath);
			conf->valueBits |= BIT(5);
			return;
		}

		sprintf(conf->apPatchPath, "%s:/_nds/nds-bootstrap/apFix/%s-%04X.ips", conf->bootstrapOnFlashcard ? "fat" : "sd", romTid, headerCRC);
		if (access(conf->apPatchPath, F_OK) == 0) {
			conf->apPatchSize = getFileSize(conf->apPatchPath);
			return;
		}

		sprintf(conf->apPatchPath, "%s:/_nds/nds-bootstrap/apFix/%s-%04X.ips", conf->bootstrapOnFlashcard ? "fat" : "sd", romTid, 0xFFFF);
		if (access(conf->apPatchPath, F_OK) == 0) {
			conf->apPatchSize = getFileSize(conf->apPatchPath);
			return;
		}

		sprintf(conf->apPatchPath, "%s:/_nds/nds-bootstrap/apFix/%s-%04X.bin", conf->bootstrapOnFlashcard ? "fat" : "sd", romTid, headerCRC);
		if (access(conf->apPatchPath, F_OK) == 0) {
			conf->apPatchSize = getFileSize(conf->apPatchPath);
			conf->valueBits |= BIT(5);
			return;
		}

		sprintf(conf->apPatchPath, "%s:/_nds/nds-bootstrap/apFix/%s-%04X.bin", conf->bootstrapOnFlashcard ? "fat" : "sd", romTid, 0xFFFF);
		if (access(conf->apPatchPath, F_OK) == 0) {
			conf->apPatchSize = getFileSize(conf->apPatchPath);
			conf->valueBits |= BIT(5);
			return;
		}

		conf->apPatchPath[0] = 0;
	}

	FILE *file = fopen("nitro:/apfix.pck", "rb");
	if (!file) {
		return;
	}

	char buf[5] = {0};
	fread(buf, 1, 4, file);
	if (strcmp(buf, ".PCK") != 0) {
		return;
	}

	u32 fileCount;
	fread(&fileCount, 1, sizeof(fileCount), file);

	u32 offset = 0, size = 0;
	bool cheatVer = false;

	// Try binary search for the game
	int left = 0;
	int right = fileCount;
	bool tidFound = false;

	while (left <= right) {
		fseek(file, 16 + left * 16, SEEK_SET);
		fread(buf, 1, 4, file);
		int cmp = strcmp(buf, romTid);
		if (cmp == 0) { // TID matches, check other info
			tidFound = true;
			u16 crc;
			fread(&crc, 1, sizeof(crc), file);
			fread(&offset, 1, sizeof(offset), file);
			fread(&size, 1, sizeof(size), file);
			cheatVer = fgetc(file) & 1;

			if (crc == 0xFFFF || crc == headerCRC) {
				break;
			} else {
				offset = 0, size = 0;
				left++;
			}
		} else if (tidFound) {
			break;
		} else {
			left++;
		}
	}

	if (offset > 0 && size > 0) {
		sprintf(conf->apPatchPath, bootstrapPath);
		conf->apPatchOffset = offsetOfOpenedNitroFile+offset;
		conf->apPatchSize = size;
		if (cheatVer) {
			conf->valueBits |= BIT(5);
		}
	}

	fclose(file);
}

/* void loadApFixPostCardRead(configuration* conf, const char* bootstrapPath, const char* romTid, const u16 headerCRC) {
	FILE *file = fopen("nitro:/apfixPostCardRead.pck", "rb");
	if (!file) {
		return;
	}

	char buf[5] = {0};
	fread(buf, 1, 4, file);
	if (strcmp(buf, ".PCK") != 0) {
		return;
	}

	u32 offset = 0, size = 0;

	// Try binary search for the game
	int left = 0;
	bool tidFound = false;

	while (1) {
		fseek(file, 16 + left * 16, SEEK_SET);
		if (fread(buf, 1, 4, file) == 0) {
			break;
		}
		if (strcmp(buf, romTid) == 0) { // TID matches, check other info
			tidFound = true;
			u16 crc;
			fread(&crc, 1, sizeof(crc), file);
			fread(&offset, 1, sizeof(offset), file);
			fread(&size, 1, sizeof(size), file);

			if (crc == 0xFFFF || crc == headerCRC) {
				break;
			} else {
				offset = 0, size = 0;
				left++;
			}
		} else if ((strcmp(buf, "END") == 0) || tidFound) {
			break;
		} else {
			left++;
		}
	}

	if (offset > 0 && size > 0) {
		sprintf(conf->apPatchPostCardReadPath, bootstrapPath);
		conf->apPatchPostCardReadOffset = offsetOfOpenedNitroFile+offset;
		conf->apPatchPostCardReadSize = size;
	}

	fclose(file);
} */

void loadMobiclipOffsets(configuration* conf, const char* bootstrapPath, const char* romTid, const u8 romVersion, const u16 headerCRC) {
	FILE *file = fopen("nitro:/mobiclipOffsets.pck", "rb");
	if (!file) {
		return;
	}

	char buf[5] = {0};
	fread(buf, 1, 4, file);
	if (strcmp(buf, ".PCK") != 0) {
		return;
	}

	u32 fileCount;
	fread(&fileCount, 1, sizeof(fileCount), file);

	u32 offset = 0, size = 0;

	// Try binary search for the game
	int left = 0;
	int right = fileCount;
	bool tidFound = false;

	while (left <= right) {
		fseek(file, 16 + left * 16, SEEK_SET);
		fread(buf, 1, 4, file);
		int cmp = strcmp(buf, romTid);
		if (cmp == 0) { // TID matches, check other info
			tidFound = true;
			u16 crc;
			u8 ver;
			fread(&crc, 1, sizeof(crc), file);
			fread(&offset, 1, sizeof(offset), file);
			fread(&size, 1, sizeof(size), file);
			fread(&ver, 1, sizeof(ver), file);

			if ((crc == 0xFFFF || crc == headerCRC) && (ver == 0xFF || ver == romVersion)) {
				break;
			} else {
				offset = 0, size = 0;
				left++;
			}
		} else if (tidFound) {
			break;
		} else {
			left++;
		}
	}

	if (offset > 0) {
		if (size > 4) {
			size = 4;
		}

		fseek(file, offset, SEEK_SET);
		u32 buffer = 0;
		fread(&buffer, 1, size, file);

		conf->mobiclipStartOffset = buffer;
		conf->mobiclipEndOffset = buffer+0x5B0;
	}

	fclose(file);
}

void loadDSi2DSSavePatch(configuration* conf, const char* bootstrapPath, const char* romTid, const u8 romVersion, const u16 headerCRC) {
	FILE *file = fopen("nitro:/dsi2dsSavePatches.pck", "rb");
	if (!file) {
		return;
	}

	char buf[5] = {0};
	fread(buf, 1, 4, file);
	if (strcmp(buf, ".PCK") != 0) {
		return;
	}

	u32 fileCount;
	fread(&fileCount, 1, sizeof(fileCount), file);

	u32 offset = 0, size = 0;

	// Try binary search for the game
	int left = 0;
	int right = fileCount;
	bool tidFound = false;

	while (left <= right) {
		fseek(file, 16 + left * 16, SEEK_SET);
		fread(buf, 1, 4, file);
		int cmp = strcmp(buf, romTid);
		if (cmp == 0) { // TID matches, check other info
			tidFound = true;
			u16 crc;
			u8 ver;
			fread(&crc, 1, sizeof(crc), file);
			fread(&offset, 1, sizeof(offset), file);
			fread(&size, 1, sizeof(size), file);
			fread(&ver, 1, sizeof(ver), file);

			if ((crc == 0xFFFF || crc == headerCRC) && (ver == 0xFF || ver == romVersion)) {
				break;
			} else {
				offset = 0, size = 0;
				left++;
			}
		} else if (tidFound) {
			break;
		} else {
			left++;
		}
	}

	if (offset > 0) {
		sprintf(conf->dsi2dsSavePatchPath, bootstrapPath);
		conf->dsi2dsSavePatchOffset = offsetOfOpenedNitroFile+offset;
		conf->dsi2dsSavePatchSize = size;
	}

	fclose(file);
}
