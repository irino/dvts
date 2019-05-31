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
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <dvts.h>

#include "param.h"

int
ieee1394_open(struct dvsave_param *dvsave_param)
{
  int fd;

  fd = open(dvsave_param->devname, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return(-1);
  }

  dvsave_param->fd1394 = fd;

  return(1);
}

void
ieee1394_close(struct dvsave_param *dvsave_param)
{
}

int
main_loop(struct dvsave_param *dvsave_param)
{
  /* length of data received from IEEE1394 device */
  int len;

  /* buffer for receiving data from IEEE1394 device */
  u_int32_t recvbuf[512/4];

  int sct, dbn, seq, seq_prev;
  u_long frame_count = 0;
  int n;

  seq_prev = 0;

  /*************************/
  /*  program starts here  */
  /*************************/

  while (1) {
    /* receive packet from IEEE1394 device */
    len = read(dvsave_param->fd1394, recvbuf, 480);
    if (len < 1) {
      printf("ieee1394 recv len : %d\n", len);
      break;
    }
    else {
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
        if (dvsave_param->frame_max != 0 &&
            frame_count > dvsave_param->frame_max) {
          break;
        }
      }

      if (seq != 0xf) {
        seq_prev = seq;
      }

      n = write(dvsave_param->fd, recvbuf, len);
      if (n < 1) {
        perror("write");
        break;
      }
    }
  }

  return(1);
}
