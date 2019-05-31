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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <dv1394.h>

#include <dvts.h>

#include "param.h"
#include "flags.h"
#include "rtcp.h"
#include "rtp.h"

#define		RTCP_SR_INTERVAL	3

static struct dv1394_init dv1394_init_param;

int
prepare_ieee1394 (struct dvsend_param *dvsend_param)
{
  int fd;

  fd = open(dvsend_param->ieee1394dv.devname, O_RDONLY);
  if (fd < 0) {
    printf("failed to open device : %s\n", dvsend_param->ieee1394dv.devname);
    return(-1);
  }

  dv1394_init_param.api_version = DV1394_API_VERSION;
  dv1394_init_param.channel = 63;
  dv1394_init_param.n_frames = 2;
  dv1394_init_param.cip_n = 0;
  dv1394_init_param.cip_d = 0;
  dv1394_init_param.syt_offset = 0;
#ifdef	PAL
  dv1394_init_param.format = DV1394_PAL;
#else	/* PAL */
  dv1394_init_param.format = DV1394_NTSC;
#endif	/* PAL */

/*
  if (ioctl(fd, DV1394_INIT, &dv1394_init_param) < 0) {
    perror("ioctl DV1394_INIT");
    return(-1);
  }
*/

  dvsend_param->ieee1394dv.fd1394 = fd;

  return(1);
}

int
main_loop(struct dvsend_param *dvsend_param)
{
  u_int32_t readcount = 0;
  struct timeval tv, tv_prev;

  int n;
  int i;
  int readsize;

  /* frame size of PAL is larger than NTSC */
  u_int32_t dvdata[DV1394_PAL_FRAME_SIZE/4];

  gettimeofday(&tv, NULL);
  gettimeofday(&tv_prev, NULL);

  if (dvsend_param->format == DV_FORMAT_PAL) {
    readsize = 144000;
  } else if (dvsend_param->format == DV_FORMAT_NTSC) {
    readsize = 120000;
  } else {
    return(-1);
  }

  while (1) {
    n = read(dvsend_param->ieee1394dv.fd1394, dvdata, readsize);
    if (n < 1) {
      break;
    }

    for (i=0; i<n; i+=80) {
      proc_dvdif(dvsend_param, &dvdata[i/4]);

      /* RTCP */
      readcount++;
      /* just do RTCP process sometimes */
      if ((dvsend_param->flags & USE_RTCP) && readcount % 6000) {
        if (gettimeofday(&tv, NULL) < 0) {
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
    }
  }

  return(-1);
}
