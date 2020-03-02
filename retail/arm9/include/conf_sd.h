#ifndef CONF_SD_H
#define CONF_SD_H

#include "configuration.h"

#ifdef __cplusplus
extern "C"
{
#endif

int loadFromSD(configuration* conf, const char *bootstrapPath);

#ifdef __cplusplus
}
#endif

#endif // CONF_SD_H
