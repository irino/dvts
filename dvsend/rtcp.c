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
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rtpvar.h>

#include "udp.h"
#include "rtcp.h"
#include "flags.h"

#define MAX_RTCP_PKT_LEN	1500

static int _process_rtcp_rr __P((struct dvsend_param *, char *, int, struct sockaddr *, int));
static int _process_rtcp_bye __P((struct dvsend_param *, char *, int, struct sockaddr *, int));

int
send_rtcp_sr (struct dvsend_param *dvsend_param)
{
  int n;
  char buf[MAX_RTCP_PKT_LEN];
  int buflen;

  rtcp_common_t *rtcp_hdr;
  struct sr {
	u_int32_t ssrc;
	u_int32_t ntp_sec;
	u_int32_t ntp_frac;
	u_int32_t rtp_ts;
	u_int32_t psent;
	u_int32_t osent;
	rtcp_rr_t rr[1];
  } *rtcp_sr;

  memset(&buf, 0, sizeof(buf));
  rtcp_hdr = (rtcp_common_t *)buf;
  rtcp_sr  = (struct sr *)&buf[sizeof(rtcp_common_t)];

  buflen = sizeof(rtcp_common_t) + sizeof(struct sr);

  rtcp_hdr->pt = RTCP_SR;

  n = send_rtcp_pkt(dvsend_param, (u_int32_t *)buf, buflen);
  if (n < 1) {
    perror("rtcp sendto");
  }

  return(1);
}

int
send_rtcp_bye (struct dvsend_param *dvsend_param)
{
  return(1);
}

/* This sees if there is a RTCP packet in the queue.      */
/* If there is no RTCP packet in the queue, this function */
/* simply returns.                                        */
int
process_rtcp (struct dvsend_param *dvsend_param)
{
  char buf[MAX_RTCP_PKT_LEN];
  rtcp_common_t *rtcp_hdr;
  int n;
  struct sockaddr_storage from;
  int length = sizeof(from);

  struct rtcp_recv_obj *rtcp_recv_obj;

  if (!(dvsend_param->flags & USE_RTCP)) {
    return(1);
  }

  memset(buf, 0, sizeof(buf));
  rtcp_hdr = (rtcp_common_t *)buf;

  rtcp_recv_obj = dvsend_param->rtcp_recv_list;
  if (rtcp_recv_obj == NULL) {
    return(1);
  }

  while (rtcp_recv_obj != NULL) {
    if ((n = recvfrom(rtcp_recv_obj->soc,
                      buf, sizeof(buf),
                      0,
                      (struct sockaddr *)&from, &length)) < 0) {
      if (errno != EAGAIN) {
        perror("rtcp recvfrom");
        return(-1);
      }
      else {
        /* no RTCP packets are in the queue */
        rtcp_recv_obj = rtcp_recv_obj->next;
        continue;
      }
    }

    switch (rtcp_hdr->pt) {
      case RTCP_RR:
        _process_rtcp_rr(dvsend_param,
                         buf, n, (struct sockaddr *)&from, length);
        break;

      case RTCP_BYE:
        _process_rtcp_bye(dvsend_param,
                          buf, n, (struct sockaddr *)&from, length);
        break;

      default:
        break;
    }

    rtcp_recv_obj = rtcp_recv_obj->next;
  }

  return(1);
}

static int
_process_rtcp_rr (struct dvsend_param *dvsend_param,
                  char *buf, int n,
                  struct sockaddr *addr, int length)
{
  rtcp_t *rtcp_rr;
  u_long frac_lost;
  int  pkt_loss_sum;
  float frac_lost_100;
  char s_addr_str[64];
#ifdef	ENABLE_INET6
  int err;
#endif	/* ENABLE_INET6 */

  memset(s_addr_str, 0, sizeof(s_addr_str));

  /* if showing number of packet loss is required */
  if (dvsend_param->flags & SHOW_PACKETLOSS) {
    rtcp_rr = (rtcp_t *)buf;

    frac_lost = rtcp_rr->r.rr.rr[0].fraction;
    pkt_loss_sum = rtcp_rr->r.rr.rr[0].lost;

    /*                                    */
    /* changes 1/255 scale to 1/100 scale */
    /*                                    */
    if (frac_lost != 0) {
      frac_lost_100 = (double)(frac_lost) * 100.0 / 255.0;
    }
    else {
      frac_lost_100 = 0;
    }

#ifdef	ENABLE_INET6
#ifndef	NO_SS_LEN
    err = getnameinfo(addr, addr->sa_len,
#else	/* NO_SS_LEN */
    err = getnameinfo(addr,
                      (addr->sa_family==AF_INET6)?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in),
#endif	/* NO_SS_LEN */
                      s_addr_str, sizeof(s_addr_str),
                      NULL, 0, NI_NUMERICHOST);
#else	/* ENABLE_INET6 */
    memcpy(s_addr_str,
           (char *)inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
           sizeof(struct in_addr));
#endif	/* ENABLE_INET6 */
                      
    printf("receiver [%s], pkt lost [%d], pkt loss (%%) [%.2f %%]\n",
           s_addr_str,
           pkt_loss_sum,
           frac_lost_100);
  }

  return(1);
}

static int
_process_rtcp_bye (struct dvsend_param *dvsend_param,
                   char *buf, int n,
                   struct sockaddr *addr, int length)
{
  printf("RECEIVED RTCP BYE\n");

  return(1);
}
