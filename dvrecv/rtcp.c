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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <rtpvar.h>
#include <dvts.h>

#include "flags.h"
#include "rtcp.h"

#define MAX_RTCP_PKT_LEN	1500

static int _process_rtcp_sr __P((struct dvrecv_param *, char *, int, struct sockaddr *, int));
static int _process_rtcp_bye __P((struct dvrecv_param *, char *, int, struct sockaddr *, int));

int
send_rtcp_rr (struct dvrecv_param *dvrecv_param,
              char *buf,
              int n,
              struct sockaddr *addr,
              int length)
{
  rtcp_t rtcp_rr;
  u_int32_t frac_pkt_lost;
  u_int32_t pkt_lost;
  int ret;

  struct sockaddr_storage s_addr;

  memset(&rtcp_rr, 0, sizeof(rtcp_rr));

  /* prepare struct for sending RTCP RR */
  memset(&s_addr, 0, sizeof(s_addr));
  memcpy(&s_addr, addr, length);
#ifndef	NO_SS_LEN
  s_addr.ss_len = length;
#endif	/* NO_SS_LEN */
  s_addr.ss_family = dvrecv_param->s_addr.ss_family;

  if (dvrecv_param->pkt_count < 1) {
     return(0);
  }

  if (dvrecv_param->pkt_loss_sum != 0) {
    frac_pkt_lost = (dvrecv_param->pkt_loss_sum*256) / dvrecv_param->pkt_count;
  }
  else {
    frac_pkt_lost = 0;
  }

  pkt_lost = dvrecv_param->pkt_loss_sum;

  /* reset the value set in dvrecv_param.             */
  /* the value will be used each time dvrecv receives */
  /* a RTCP SR from the dvsend.                       */
  dvrecv_param->pkt_loss_sum = 0;
  dvrecv_param->pkt_count = 0;

  rtcp_rr.common.pt = RTCP_RR;
  rtcp_rr.r.rr.rr[0].fraction = frac_pkt_lost;
  rtcp_rr.r.rr.rr[0].lost = pkt_lost;

  if (s_addr.ss_family == AF_INET) {
    struct sockaddr_in *s_in = (struct sockaddr_in *)&s_addr;
    s_in->sin_port = htons(dvrecv_param->port + 1);
  }
#ifdef	ENABLE_INET6
  else if (s_addr.ss_family == AF_INET6) {
    struct sockaddr_in6 *s6_in = (struct sockaddr_in6 *)&s_addr;
    s6_in->sin6_port = htons(dvrecv_param->port + 1);
  }
#endif	/* ENABLE_INET6 */

  if ((ret = sendto(dvrecv_param->rtcp_out_soc,
                    (char *)&rtcp_rr, sizeof(rtcp_rr), 0,
                    (struct sockaddr *)&s_addr, length)) < 1) {
    perror("RTCP RR sendto");
  }

  return(1);
}

int
send_rtcp_bye (struct dvrecv_param *dvrecv_param)
{
  return(1);
}

int
process_rtcp (struct dvrecv_param *dvrecv_param)
{
  char buf[MAX_RTCP_PKT_LEN];
  rtcp_common_t *rtcp_hdr;
  int n;
  struct sockaddr_storage from;
  int length = sizeof(from);

  if (!(dvrecv_param->flags & USE_RTCP)) {
    return(1);
  }

  memset(buf, 0, sizeof(buf));
  rtcp_hdr = (rtcp_common_t *)buf;

  if ((n = recvfrom(dvrecv_param->rtcp_in_soc,
                    buf, sizeof(buf),
                    0,
                    (struct sockaddr *)&from, &length)) < 0) {
    if (errno != EAGAIN) {
      perror("rtcp recvfrom");
      return(-1);
    }
    else {
      /* no RTCP packets are in the queue */
      return(1);
    }
  }

  switch (rtcp_hdr->pt) {
    case RTCP_SR:
      _process_rtcp_sr(dvrecv_param, buf, n, (struct sockaddr *)&from, length);
      break;

    case RTCP_BYE:
      _process_rtcp_bye(dvrecv_param, buf, n, (struct sockaddr *)&from, length);
      break;

    default:
      break;
  }

  return(1);
}

/*************************/

static int
_process_rtcp_sr (struct dvrecv_param *dvrecv_param,
                  char *buf,
                  int n,
                  struct sockaddr *addr,
                  int length)
{
  int ret;

  ret = send_rtcp_rr(dvrecv_param, buf, n, addr, length);

  return(ret);
}

static int
_process_rtcp_bye (struct dvrecv_param *dvrecv_param,
                   char *buf,
                   int n,
                   struct sockaddr *addr,
                   int length)
{
  printf("RECEIVED RTCP BYE\n");

  return(1);
}
