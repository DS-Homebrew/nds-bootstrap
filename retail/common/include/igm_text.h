#ifndef IGM_TEXT_H
#define IGM_TEXT_H

#include "locations.h"

struct IgmText {
	char version[20];
	char ndsBootstrap[14];
	char ramViewer[20];
	char jumpAddress[20];
	char menu[20][6];
	char options[20][10];
};

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION;

#endif // IGN_TEXT_H
