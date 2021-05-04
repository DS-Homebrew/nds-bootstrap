#ifndef IGM_TEXT_H
#define IGM_TEXT_H

#include "locations.h"

struct IgmText {
	u16 version[20];
	u16 ndsBootstrap[14];
	u16 ramViewer[20];
	u16 jumpAddress[20];
	u16 menu[6][20];
	u16 options[10][20];
	bool rtl;
	u16 hotkey;
};

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION;

#endif // IGM_TEXT_H
