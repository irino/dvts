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

#ifndef _DVSEND_IEEE1394_STRUCT_H_
#define _DVSEND_IEEE1394_STRUCT_H_

#ifdef	LINUX
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#endif	/* LINUX */

struct ieee1394dv {
  /* IEEE1394 isochronous channel */
  int channel;

#ifdef	FREEBSD_4
  /* for receiving data from IEEE1394 device */
  int fd;

  /* IEEE1394 interface name */
  char *ifname;
#endif	/* FREEBSD_4 */

#ifdef	MACOS_X
#endif	/* MACOS_X */

#ifdef	NETBSD
  /* for receiving data from IEEE1394 device */
  int fd;

  /* IEEE1394 interface name */
  char *ifname;
#endif	/* NETBSD */

#ifdef	LINUX
  /* IEEE1394 interface name */
  char *devname;
  int   fd1394;
#endif	/* LINUX */

#ifdef	FREEBSD_5
  char *devname;
#endif	/* FREEBSD_5 */
};

#endif /* _DVSEND_IEEE1394_STRUCT_H_ */
