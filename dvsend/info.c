/*
 * Copyright (c) 1999-2003 WIDE Project
 * All rights reserved.
 *
 * Author : Akimichi OGAWA (akimichi@sfc.wide.ad.jp)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code MUST retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form MUST reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    MUST display the following acknowledgement:
 *      This product includes software developed by Akimichi OGAWA.
 * 4. The name of the author MAY NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifdef HAVE_CONFIG_H
#include <dvts-config.h>
#endif	/* HAVE_CONFIG_H */

#include "info.h"

void
show_usage (char *argv0)
{
  printf("\nUsage : %s [options]\n\n", argv0);

  printf("<options>\n");
  printf("-v          : show version number\n");
#ifdef ENABLE_INET6
  printf("-6          : use IPv6 (Default)\n");
#endif /* ENABLE_INET6 */
  printf("-4          : use IPv4\n");
  printf("-h hostname : sendto host \"hostname\"\n");
  printf("-f rate     : send full frame by 1/rate\n");

#ifdef	FREEBSD_4
  printf("-I 1394if   : use interface \"1394if\"\n");
  printf("               example, [ -I ohci0 ]\n");
#endif	/* FREEBSD_4 */

#ifdef	NETBSD
  printf("-D 1394dev  : use device \"1394dev\"\n");
  printf("               example, [ -D /dev/fw0 ]\n");
#endif	/* NETBSD */

  printf("-M if       : use \"if\" for sending IPv6 multicast packets\n");
  printf("-m if       : use \"if\" for sending IPv4 multicast packets\n");
  printf("-t ttl      : TTL for multicast\n");
  printf("-P port     : use UDP port \"port\"\n");
  printf("-s number   : number of DIF blocks included in one packet\n");
  printf("               packet length will be [IPhdr+UDPhdr+RTPhdr+80*number]\n");
  printf("-N          : do NOT send video\n");
  printf("-d port     : send audio and video in different stream\n");
  printf("               send audio usind port \"port\"\n");
  printf("-L          : show packet loss state of the receivers\n");
  printf("-H          : show this help message\n");
  printf("-r level    : audio redundancy level (default 0)\n");
  printf("-p          : use PAL\n");
}

void
show_version ()
{
  printf("DVTS (DV Transport System) version %s\n", VERSION);
  printf("Copyright (c) 1999-2003 WIDE Project\n");
  printf("All rights reserved\n");
  printf("Author : Akimichi OGAWA (akimichi@sfc.wide.ad.jp)\n");
  printf("   http://www.sfc.wide.ad.jp/DVTS/\n");
}
