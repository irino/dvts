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

#ifndef _DVRECV_IEEE1394_STRUCT_H_
#define _DVRECV_IEEE1394_STRUCT_H_

#ifdef FREEBSD_4
struct ieee1394dv {
  int channel;
  char *ifname;
  int   fd;
  int   sync;
  int   frac;
  int   mod;
};
#endif	/* FREEBSD_4 */

#ifdef MACOS_X
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <DVComponentGlue/IsochronousDataHandler.h>
#include <DVComponentGlue/DeviceControl.h>

struct ieee1394dv {
  int channel;
  ComponentInstance theInst;
  QTAtomSpec videoConfig;
};
#endif	/* MACOS_X */

#ifdef	NETBSD
struct ieee1394dv {
  int channel;
};
#endif	/* NETBSD */

#ifdef	LINUX
/* /usr/src/linux/drivers/ieee1394/dv1394.h */
#include <dv1394.h>
struct ieee1394dv {
  int channel;
  int fd1394;
  char *devname;
  unsigned char f50_60;
};
#endif	/* LINUX */

#endif /* _DVRECV_IEEE1394_STRUCT_H_ */
