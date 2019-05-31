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
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>

#include <net/if.h>
#include <dev/firewire/firewire.h>

#include <libdv/dv.h>

#include "param.h"

#define		NBLOCKS				300

static u_int32_t outbuf[512*NBLOCKS / 4];

int
open_ieee1394(struct dvplay_param *dvplay_param)
{
  char str[128];
  int i;
  int dummy;

  dvplay_param->ieee1394_fd = open(dvplay_param->ieee1394dev, O_RDWR);
  if (dvplay_param->ieee1394_fd < 0) {
    memset(str, 0, sizeof(str));
    snprintf(str, sizeof(str), "open %s", dvplay_param->ieee1394dev);
    perror(str);
    return(-1);
  }

  memset(outbuf, 0, sizeof(outbuf));

  for (i=0; i<NBLOCKS; i++) {
    outbuf[i * (512/4) + 0] = htonl(488 << 16 | 0x00007fa0);
    outbuf[i * (512/4) + 1] = htonl(0x00780000);
    outbuf[i * (512/4) + 2] = htonl(0x8000ffff);
  }

  ioctl(dvplay_param->ieee1394_fd, FW_DV_SYNC, &dvplay_param->sync);
  ioctl(dvplay_param->ieee1394_fd, FW_DV_TXSTART, &dummy);

  return(1);
}

void
close_ieee1394(struct dvplay_param *dvplay_param)
{
  int dummy;

  ioctl(dvplay_param->ieee1394_fd, FW_DV_TXSTOP, &dummy);
  close(dvplay_param->ieee1394_fd);
}

int
write_ieee1394(struct dvplay_param *dvplay_param)
{
  u_char dseq;
  int ieee1394_pkt_count;
  int count;

  ieee1394_pkt_count = 0;

  for (dseq=0; dseq<DIFSEQ_NUM; dseq++) {
    for (count=0; count<IEEE1394_PKT_NUM; count++) {
      outbuf[ieee1394_pkt_count*(512/4)] = htonl(488 << 16 | 0x00007fa0);
      memcpy(&outbuf[ieee1394_pkt_count*(512/4) + 3],
             &dvplay_param->dvframe.pkt[dseq][0][0] + count*480/4,
             480);

      if ((ieee1394_pkt_count % dvplay_param->frac) == dvplay_param->mod) {
          ieee1394_pkt_count++;
          outbuf[ieee1394_pkt_count*(512/4)] = htonl(8 << 16 | 0x00007fa0);
      }

      ieee1394_pkt_count++;
    } /* end of for(count=0; count<IEEE1394_PKT_NUM; count++) */
  } /* end of for(dseq=0; dseq<DIFSEQ_NUM; dseq++) */

  if (write(dvplay_param->ieee1394_fd, outbuf, ieee1394_pkt_count*512) < 0) {
    return(-1);
  }

  return(1);
}
