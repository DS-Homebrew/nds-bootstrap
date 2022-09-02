#ifndef IGM_TEXT_H
#define IGM_TEXT_H

#include <nds/ndstypes.h>

#define IGM_TEXT_SIZE_ALIGNED ((sizeof(struct IgmText) + 0xF) & ~0xF)

struct IgmText {
	unsigned char version[20];
	unsigned char ndsBootstrap[14];
	unsigned char ramViewer[20];
	unsigned char jumpAddress[20];
	unsigned char selectBank[20];
	unsigned char count[14];
	unsigned char menu[8][20];
	unsigned char optionsLabels[5][20];
	unsigned char optionsValues[7][20];

	u8 font[256 * 8];

	bool rtl;
	u16 hotkey;
	u8 currentScreenshot;
	int manualLine;
	int manualMaxLine;
};

#ifdef __cplusplus
static_assert(sizeof(IgmText) == 0xA0C, "IgmText is too big! Allocate more space in the in-game menu header");

enum class IgmFont : u8 {
	arabic = 0,
	cyrillic = 1,
	extendedLatin = 2,
	greek = 3,
	hangul = 4,
	hebrew = 5,
	japanese = 6,
	chinese = 7
};
#endif

#endif // IGM_TEXT_H
