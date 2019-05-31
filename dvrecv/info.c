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
  printf("\nUsage : %s [options]\n", argv0);

  printf("<options>\n");
  printf("-v        : show version number\n");
#ifdef ENABLE_INET6
  printf("-6        : use IPv6 (Default)\n");
#endif /* ENABLE_INET6 */
  printf("-4        : use IPv4\n");
  printf("-j group  : join mulitcast group \"group\"\n");
  printf("             example, [-j 239.100.100.100]\n");
  printf("-M ifname : multicast join interface \"ifname\"\n");
  printf("             example, [-M fxp0]\n");
  printf("-P port   : RTP port number \"port\"\n");
  printf("             example, [-P 8100]\n");
#ifdef FREEBSD_4
  printf("-D dev    : use device \"dev\"\n");
  printf("             example, [-D /dev/dv0]\n");
#endif	/* FREEBSD_4 */
  printf("-L        : show packet loss\n");
  printf("-R        : don't use RTCP\n");
  printf("-l number : show packet loss, specify display granularity\n");
  printf("-H        : show this help message\n");
  printf("-p        : use PAL\n");
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
