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

#include <dv1394.h>

#include <dvts.h>

#include "param.h"

int
ieee1394_open(struct dvsave_param *dvsave_param)
{
  int fd;
  struct dv1394_init dv1394_init_param;

  fd = open(dvsave_param->devname, O_RDONLY);
  if (fd < 0) {
    printf("failed to open device : %s\n", dvsave_param->devname);
    return(-1);
  }

  dv1394_init_param.api_version = DV1394_API_VERSION;
  dv1394_init_param.channel = dvsave_param->channel;
  dv1394_init_param.n_frames = 2;
  dv1394_init_param.cip_n = 0;
  dv1394_init_param.cip_d = 0;
  dv1394_init_param.syt_offset = 0;

  if (dvsave_param->format == DV_FORMAT_PAL) {
    dv1394_init_param.format = DV1394_PAL;
  } else if (dvsave_param->format == DV_FORMAT_NTSC) {
    dv1394_init_param.format = DV1394_NTSC;
  } else {
    return(-1);
  }

  if (ioctl(fd, DV1394_INIT, &dv1394_init_param) < 0) {
    perror("ioctl DV1394_INIT");
    return(-1);
  }

  dvsave_param->fd1394 = fd;

  return(1);
}

void
ieee1394_close(struct dvsave_param *dvsave_param)
{
  if (dvsave_param == NULL) {
    return;
  }

  close(dvsave_param->fd1394);
}

int
main_loop(struct dvsave_param *dvsave_param)
{
  int n_in, n_out;
  int i;
  int readsize;
  int framecount = 0;

  /* frame size of PAL is larger than NTSC */
  u_int32_t dvdata[DV1394_PAL_FRAME_SIZE/4];

  if (dvsave_param->format == DV_FORMAT_PAL) {
    readsize = 144000;
  } else if (dvsave_param->format == DV_FORMAT_NTSC) {
    readsize = 120000;
  } else {
    return(-1);
  }

  while (1) {
    if (dvsave_param->frame_count != 0 &&
        framecount > dvsave_param->frame_count) {
      break;
    }

    n_in = read(dvsave_param->fd1394, dvdata, readsize);
    if (n_in < 1) {
      return(-1);
    }

    n_out = write(dvsave_param->fd, dvdata, n_in);
    if (n_out < 1) {
      return(-1);
    }

    framecount++;
  }

  return(1);
}
