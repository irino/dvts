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
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <dvts.h>

#include "param.h"
#include "rtp.h"
#include "rtcp.h"
#include "flags.h"

#define		RTCP_SR_INTERVAL	1

int
prepare_ieee1394 (struct dvsend_param *dvsend_param)
{
  int fd;

  fd = open(dvsend_param->ieee1394dv.ifname, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return(-1);
  }

  dvsend_param->ieee1394dv.fd = fd;

  return(1);
}

int
main_loop (struct dvsend_param *dvsend_param)
{
  /* length of data received from IEEE1394 device */
  int len;

  /* buffer for receiving data from IEEE1394 device */
  u_int32_t recvbuf[512/4];

  /* used by for(i=0; i<len/80; i++) */
  int i;

  /* how many times received from IEEE1394 */
  u_long readcount = 0;

  /* to calculate the RTCP interval */
  struct timeval tv, tv_prev;

  /*************************/
  /*  program starts here  */
  /*************************/

  gettimeofday(&tv, NULL);
  gettimeofday(&tv_prev, NULL);

  while (1) {
    /* RTCP */
    readcount++;

    /* just do RTCP process sometimes */
    if ((dvsend_param->flags & USE_RTCP) && readcount % 1000) {
      if (gettimeofday(&tv, NULL) < NULL) {
        printf("gettimeofday failed\n");
        break;
      }
      if ((tv.tv_sec - tv_prev.tv_sec) > RTCP_SR_INTERVAL) {
        send_rtcp_sr(dvsend_param);
        memcpy(&tv_prev, &tv, sizeof(tv_prev));
      }

      /* process rtcp input */
      process_rtcp(dvsend_param);
    }

    /* receive packet from IEEE1394 device */
    len = read(dvsend_param->ieee1394dv.fd, recvbuf, 480);
    if (len < 1) {
      printf("ieee1394 recv len : %d\n", len);
      break;
    }
    else {
      /* process each DV DIF block */
      for (i=0; i<len/80; i++) {
        proc_dvdif(dvsend_param, &recvbuf[i * 80 / 4]);
      }
    }
  }

  return(1);
}
