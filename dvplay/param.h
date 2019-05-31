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

#ifndef _DVPLAY_PARAM_H_
#define _DVPLAY_PARAM_H_

#include <stdio.h>
#include <sys/types.h>

#define 	DVPLAY_FLAG_STDIN	0x00000001

#ifdef MACOS_X
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <DVComponentGlue/IsochronousDataHandler.h>
#include <DVComponentGlue/DeviceControl.h>
#endif	/* MACOS_X */

struct dvplay_param {
  int   format;
  int   maxdifseq;

  char *filename;
  FILE *fp;

  struct dv1394_pkts dvframe;

  int channel;

  u_int32_t flags;

#ifdef	FREEBSD_4
  char *ieee1394dev;
  int ieee1394_fd;
  int frac;
  int mod;
  int sync;
#endif	/* FREEBSD_4 */

#ifdef MACOS_X
  ComponentInstance theInst;
  QTAtomSpec videoConfig;
  Ptr buffer;
  IDHParameterBlock isochParamBlock;
#endif /* MACOS_X */

#ifdef	LINUX
  char *devname;
  int fd1394;
#endif	/* LINUX */
};

#endif /* _DVPLAY_PARAM_H_ */
