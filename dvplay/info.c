#ifdef	HAVE_CONFIG_H
#include <dvts-config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include "info.h"

void
show_usage(char *argv0)
{
  printf("Usage : %s [options]\n\n", argv0);
  printf("<options>\n");
  printf("-v          : show version number\n");
  printf("-F filename : input file\n");
#ifdef FREEBSD_4
  printf("-D 1394dev  : use interface \"1394dev\"\n");
  printf("               example, [ -D /dev/dv0 ]\n");
  printf("-p          : use PAL\n");
#endif /* FREEBSD_4 */
}

void
show_version()
{
  printf("DVTS (DV Transport System) version %s\n", VERSION);
  printf("Copyright (c) 1999-2003 WIDE Project\n");
  printf("All rights reserved\n");
  printf("Author : Akimichi OGAWA (akimichi@sfc.wide.ad.jp)\n");
  printf("   http://www.sfc.wide.ad.jp/DVTS/\n");
}
