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

#ifdef	HAVE_CONFIG_H
#include <dvts-config.h>
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <dv1394.h>

#include <dvts.h>

#include "param.h"

static struct dv1394_init dv1394_init_param;

int
open_ieee1394(struct dvplay_param *dvplay_param)
{
  int fd;

  fd = open(dvplay_param->devname, O_RDWR);
  if (fd < 0) {
    printf("failed to open device : %s\n", dvplay_param->devname);
    return(-1);
  }

  dv1394_init_param.api_version = DV1394_API_VERSION;
  dv1394_init_param.channel = 63;
  dv1394_init_param.n_frames = 2;
  dv1394_init_param.cip_n = 0;
  dv1394_init_param.cip_d = 0;
  dv1394_init_param.syt_offset = 0;

  if (dvplay_param->format == DV_FORMAT_PAL) {
    dv1394_init_param.format = DV1394_PAL;
  } else if (dvplay_param->format == DV_FORMAT_NTSC) {
    dv1394_init_param.format = DV1394_NTSC;
  } else {
    return(-1);
  }

  if (ioctl(fd, DV1394_INIT, &dv1394_init_param) < 0) {
    perror("ioctl DV1394_INIT");
    printf("channel  : %d\n", dv1394_init_param.channel);
    printf("n_frames : %d\n", dv1394_init_param.n_frames);
    return(-1);
  }

  dvplay_param->fd1394 = fd;

  return(1);
}

void
close_ieee1394(struct dvplay_param *dvplay_param)
{
  if (dvplay_param == NULL) {
    return;
  }

  close(dvplay_param->fd1394);
}

int
write_ieee1394(struct dvplay_param *dvplay_param)
{
  int n;

  if (dvplay_param->format == DV_FORMAT_PAL) {
    n = write(dvplay_param->fd1394, &dvplay_param->dvframe.pkt, 144000);
  } else if (dvplay_param->format == DV_FORMAT_NTSC) {
    n = write(dvplay_param->fd1394, &dvplay_param->dvframe.pkt, 120000);
  } else {
    return(-1);
  }

  if (n < 1) {
    return(-1);
  }

  return(1);
}
