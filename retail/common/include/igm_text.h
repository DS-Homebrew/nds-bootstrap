#ifndef IGM_TEXT_H
#define IGM_TEXT_H

struct IgmText {
	u16 version[20];
	u16 ndsBootstrap[14];
	u16 ramViewer[20];
	u16 jumpAddress[20];
	u16 selectBank[20];
	u16 count[14];
	u16 menu[7][20];
	u16 options[10][20];

	bool rtl;
	u16 hotkey;
	u8 currentScreenshot;
};

#ifdef __cplusplus
static_assert(sizeof(IgmText) < 0x400, "IgmText is too big! Allocate more space in the in-game menu header");
#endif

#endif // IGM_TEXT_H
