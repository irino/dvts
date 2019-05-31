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
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <net/if.h>
#include <dev/firewire/firewire.h>

#include <dvts.h>

#include "param.h"
#include "ieee1394-struct.h"

int
prepare_ieee1394 (struct dvrecv_param *dvrecv_param)
{
  int dummy;

  if ((dvrecv_param->ieee1394dv.fd =
           open(dvrecv_param->ieee1394dv.ifname, O_RDWR)) < 0) {
    perror("open");
    return(-1);
  }

  ioctl(dvrecv_param->ieee1394dv.fd,
        FW_DV_SYNC, &dvrecv_param->ieee1394dv.sync);
  ioctl(dvrecv_param->ieee1394dv.fd,
        FW_DV_TXSTART, &dummy);

  printf("IEEE1394 device : %s\n", dvrecv_param->ieee1394dv.ifname);

  return (1);
}

int
freebsd4_write_frame_to_ieee1394 (struct dvrecv_param *dvrecv_param,
                                  unsigned long *buf,
                                  int len)
{
  return write(dvrecv_param->ieee1394dv.fd, buf, len);
}

void
stop_ieee1394_output (struct dvrecv_param *dvrecv_param)
{
  int dummy;

  ioctl(dvrecv_param->ieee1394dv.fd, FW_DV_TXSTOP, &dummy);
}
