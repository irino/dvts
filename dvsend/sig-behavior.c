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

#ifdef HAVE_CONFIG_H
#include <dvts-config.h>
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "param.h"
#include "sig-behavior.h"

extern struct dvsend_param dvsend_param;

static void
_sigint (int sig)
{
  exit(0);
}

/* increase the picture frame rate by SIGUSR1 */
static void
_sigusr1 (int sig)
{
  if (dvsend_param.frame_drop > 1) {
    printf("frame drop %d -> %d\n", dvsend_param.frame_drop,
                                    dvsend_param.frame_drop - 1);
    dvsend_param.frame_drop--;
  }
}

/* decrease the picture frame rate by SIGUSR1 */
static void
_sigusr2 (int sig)
{
  printf("frame drop %d -> %d\n", dvsend_param.frame_drop,
                                  dvsend_param.frame_drop + 1);
  dvsend_param.frame_drop++;
}

void
set_signal_behavior(void)
{
  signal (SIGUSR1, _sigusr1);
  signal (SIGUSR2, _sigusr2);
  signal (SIGINT, _sigint);
}
