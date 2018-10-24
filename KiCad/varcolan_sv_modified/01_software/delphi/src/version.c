#include "version.h"
#include ".version.h"
#include "config.h"
#include <stdlib.h>

char* version_get()
{
#if 0
#ifdef INVIOSOGLIE
  return VERSION_DAY VERSION_MONTH VERSION_YEAR VERSION_HOUR VERSION_MIN " (soglie)"
#else
  return VERSION_DAY VERSION_MONTH VERSION_YEAR VERSION_HOUR VERSION_MIN " (no soglie)"
#endif
#endif
  return "Centrale SAET Delphi v." VERSION_DAY VERSION_MONTH VERSION_YEAR VERSION_HOUR VERSION_MIN
  	/* " - TEST" */;
}

void version_data(unsigned char *ev)
{
  ev[19] = atoi(VERSION_MIN);
  ev[21] = atoi(VERSION_HOUR);
  ev[24] = atoi(VERSION_DAY);
  ev[25] = atoi(VERSION_MONTH);
  ev[26] = atoi(VERSION_YEAR);
}

short version_get_short()
{
  return atoi(VERSION_MONTH) | (atoi(VERSION_YEAR) << 4) | (atoi(VERSION_DAY) << 8);
}
