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

#ifdef  HAVE_CONFIG_H
#include <dvts-config.h>
#endif  /* HAVE_CONFIG_H */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/if_fw.h>

#include <dvts.h>

#include "param.h"

#define		BUFSIZEMAX	512

int
ieee1394_open(struct dvsave_param *dvsave_param)
{
  struct sockaddr_n1394 sfw;

  memset(&sfw, 0, sizeof(sfw));

  sfw.sn1394_family = AF_N1394;
  sfw.sn1394_fch = dvsave_param->channel;
  sfw.sn1394_ftag = 0x1;

  memcpy(&sfw.sn1394_if, dvsave_param->ifname, IFNAMSIZ);

  dvsave_param->soc = socket(AF_N1394, SOCK_DGRAM, 1);
  if (dvsave_param->soc < 0) {
    perror("socket 1394");
    return(-1);
  }

  if (connect(dvsave_param->soc,
              (struct sockaddr *)&sfw, sizeof(sfw)) < 0) {
    perror("connect 1394");
    return(-1);
  }

  return(1);
}

void
ieee1394_close()
{
}

int
ieee1394_read(struct dvsave_param *dvsave_param,
              u_int32_t *buf,
              u_long buf_size_max)
{
  int n;
  u_long tempbuf[buf_size_max];

  n = recv(dvsave_param->soc, tempbuf, buf_size_max, 0);
  if (n < 0) {
    return(-1);
  }

  if (n <= 8) {
    return(1);
  }

  memcpy(buf, &tempbuf[2], n - 8);

  return(n - 8);
}

int
main_loop(struct dvsave_param *dvsave_param)
{
  u_int32_t recvbuf[BUFSIZEMAX];
  int len;
  int sct, dbn, seq, seq_prev;
  u_long frame_count = 0;
  int n;

  seq_prev = 0;

  while((len = ieee1394_read(dvsave_param, recvbuf, BUFSIZEMAX)) > 0) {
    if (len == 0) {
      continue;
    }
    if (len <= 8) {
      continue;
    }

    sct = (ntohl(recvbuf[0]) >> 29) & 0x7;
    seq = (ntohl(recvbuf[0]) >> 24) & 0xf;
    dbn = (ntohl(recvbuf[0]) >> 8) & 0xff;

    if (sct > 0x4) {
      printf("Invalid DV SCT [%d]\n", sct);
    }
    if (seq > 11 && seq < 15) {
      printf("Invalid DV seq [%d]\n", seq);
    }
    if (dbn > 134) {
      printf("Invalid dbn [%d]\n", dbn);
    }

    if (sct != SCT_HEADER && seq != seq_prev) {
      frame_count++;
      if (dvsave_param->frame_max != 0 && frame_count > dvsave_param->frame_max) {
        break;
      }
    }

    if (seq != 0xf) {
      seq_prev = seq;
    }

    if ((n = write(dvsave_param->fd, recvbuf, len)) < 1) {
      perror("write error\n");
      fprintf(stderr, "fd : %d\nlen : %d\n%d\n", dvsave_param->fd, len, n);
      break;
    }
  }

  return(1);
}
