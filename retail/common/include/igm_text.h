#ifndef IGM_TEXT_H
#define IGM_TEXT_H

#include "locations.h"

struct IgmText {
	char version[20];
	char ndsBootstrap[14];
	char ramViewer[20];
	char jumpAddress[20];
	char menu[6][20];
	char options[10][20];
};

struct IgmText *igmText = (struct IgmText *)INGAME_MENU_LOCATION;

#endif // IGM_TEXT_H
