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
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <dvts.h>

#include "param.h"
#include "udp.h"
#include "flags.h"

static int _prepare_rtp_read_socket __P((struct dvrecv_param *));
static int _prepare_rtp_audio_read_socket __P((struct dvrecv_param *));
static int _prepare_rtcp_read_socket __P((struct dvrecv_param *));
static int _prepare_rtcp_write_socket __P((struct dvrecv_param *));

int
prepare_udp_socket (struct dvrecv_param *dvrecv_param)
{
  memset(&dvrecv_param->fds, 0, sizeof(dvrecv_param->fds));
  dvrecv_param->maxfds = 0;

  if (_prepare_rtp_read_socket(dvrecv_param) < 0) {
    return(-1);
  }
  printf("RTP Port : %d\n", dvrecv_param->port);

  if (dvrecv_param->flags & SEPARATED_AUDIO_STREAM) {
    if (_prepare_rtp_audio_read_socket(dvrecv_param) < 0) {
      return(-1);
    }
    printf("SEPARATED AUDIO, AUDIO RTP Port : %d\n", dvrecv_param->audio_port);
  }

  if (dvrecv_param->flags & USE_RTCP) {
    if (_prepare_rtcp_read_socket(dvrecv_param) < 0) {
      return(-1);
    }
    printf("USE RTCP, RTCP Port : %d\n", dvrecv_param->port + 1);

    if (_prepare_rtcp_write_socket(dvrecv_param) < 0) {
      return(-1);
    }
  }

  return(1);
}

static int
_prepare_rtp_read_socket (struct dvrecv_param *dvrecv_param)
{
  int soc;
  struct sockaddr_in *s_in;
#ifdef ENABLE_INET6
  struct sockaddr_in6 *s6_in;
/*
  struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
*/
#endif /* ENABLE_INET6 */

  if ((soc = socket(dvrecv_param->s_addr.ss_family, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return(-1);
  }

  s_in = (struct sockaddr_in *)&dvrecv_param->s_addr;
#ifdef ENABLE_INET6
  s6_in = (struct sockaddr_in6 *)&dvrecv_param->s_addr;
#endif /* ENABLE_INET6 */

  switch (dvrecv_param->s_addr.ss_family) {
#ifdef ENABLE_INET6
    case AF_INET6:
#ifndef	NO_SS_LEN
      s6_in->sin6_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
      s6_in->sin6_port = htons(dvrecv_param->port);
      memcpy(&s6_in->sin6_addr, &in6addr_any, sizeof(struct in6_addr));
      break;
#endif /* ENABLE_INET6 */

    case AF_INET:
#ifndef	NO_SS_LEN
      s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
      s_in->sin_port = htons(dvrecv_param->port);
      s_in->sin_addr.s_addr = htonl(INADDR_ANY);
      break;

    default:
      return(-1);
      break;
  }

  if (bind(soc,
           (struct sockaddr *)&dvrecv_param->s_addr,
#ifndef	NO_SS_LEN
           dvrecv_param->s_addr.ss_len) < 0) {
#else	/* NO_SS_LEN */
           sizeof(dvrecv_param->s_addr)) < 0) {
#endif	/* NO_SS_LEN */
    perror("RTP bind");
    return(-1);
  }

  FD_SET(soc, &dvrecv_param->fds);
  if (dvrecv_param->maxfds < soc + 1) {
    dvrecv_param->maxfds = soc + 1;
  }

  dvrecv_param->soc = soc;

  return(1);
}

static int
_prepare_rtp_audio_read_socket (struct dvrecv_param *dvrecv_param)
{
  int audio_soc;
  struct sockaddr_in *s_in;
#ifdef ENABLE_INET6
  struct sockaddr_in6 *s6_in;
/*
  struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
*/
#endif /* ENABLE_INET6 */

  if ((audio_soc = socket(dvrecv_param->audio_s_addr.ss_family,
                          SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return(-1);
  }

  s_in = (struct sockaddr_in *)&dvrecv_param->audio_s_addr;
#ifdef ENABLE_INET6
  s6_in = (struct sockaddr_in6 *)&dvrecv_param->audio_s_addr;
#endif /* ENABLE_INET6 */

  switch (dvrecv_param->audio_s_addr.ss_family) {
#ifdef ENABLE_INET6
    case AF_INET6:
#ifndef	NO_SS_LEN
      s6_in->sin6_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
      s6_in->sin6_port = htons(dvrecv_param->audio_port);
      memcpy(&s6_in->sin6_addr, &in6addr_any, sizeof(struct in6_addr));
      break;
#endif /* ENABLE_INET6 */

    case AF_INET:
#ifndef	NO_SS_LEN
      s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
      s_in->sin_port = htons(dvrecv_param->audio_port);
      s_in->sin_addr.s_addr = htonl(INADDR_ANY);
      break;

    default:
      return(-1);
      break;
  }

  if (bind(audio_soc,
           (struct sockaddr *)&dvrecv_param->audio_s_addr,
#ifndef	NO_SS_LEN
           dvrecv_param->audio_s_addr.ss_len) < 0) {
#else	/* NO_SS_LEN */
           sizeof(dvrecv_param->audio_s_addr)) < 0) {
#endif	/* NO_SS_LEN */
    perror("RTP audio bind");
    return(-1);
  }

  FD_SET(audio_soc, &dvrecv_param->fds);
  if (dvrecv_param->maxfds < audio_soc + 1) {
    dvrecv_param->maxfds = audio_soc + 1;
  }

  dvrecv_param->audio_soc = audio_soc;

  return(1);
}

static int
_prepare_rtcp_read_socket (struct dvrecv_param *dvrecv_param)
{
  int rtcp_in_soc;
  struct sockaddr_in *s_in;
#ifdef ENABLE_INET6
  struct sockaddr_in6 *s6_in;
/*
  struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
*/
#endif /* ENABLE_INET6 */

  if ((rtcp_in_soc = socket(dvrecv_param->rtcp_in_s_addr.ss_family,
                            SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return(-1);
  }

  s_in = (struct sockaddr_in *)&dvrecv_param->rtcp_in_s_addr;
#ifdef ENABLE_INET6
  s6_in = (struct sockaddr_in6 *)&dvrecv_param->rtcp_in_s_addr;
#endif /* ENABLE_INET6 */

  switch (dvrecv_param->rtcp_in_s_addr.ss_family) {
#ifdef ENABLE_INET6
    case AF_INET6:
#ifndef	NO_SS_LEN
      s6_in->sin6_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
      s6_in->sin6_port = htons(dvrecv_param->port + 1);
      memcpy(&s6_in->sin6_addr, &in6addr_any, sizeof(struct in6_addr));
      break;
#endif /* ENABLE_INET6 */

    case AF_INET:
#ifndef	NO_SS_LEN
      s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
      s_in->sin_port = htons(dvrecv_param->port + 1);
      s_in->sin_addr.s_addr = htonl(INADDR_ANY);
      break;

    default:
      return(-1);
      break;
  }

  if (bind(rtcp_in_soc,
           (struct sockaddr *)&dvrecv_param->rtcp_in_s_addr,
#ifndef	NO_SS_LEN
           dvrecv_param->rtcp_in_s_addr.ss_len) < 0) {
#else	/* NO_SS_LEN */
           sizeof(dvrecv_param->rtcp_in_s_addr)) < 0) {
#endif	/* NO_SS_LEN */
    perror("RTCP bind");
    return(-1);
  }

  FD_SET(rtcp_in_soc, &dvrecv_param->fds);
  if (dvrecv_param->maxfds < rtcp_in_soc + 1) {
    dvrecv_param->maxfds = rtcp_in_soc + 1;
  }

  dvrecv_param->rtcp_in_soc = rtcp_in_soc;

  return(1);
}

static int
_prepare_rtcp_write_socket (struct dvrecv_param *dvrecv_param)
{
  int rtcp_out_soc;
  struct sockaddr_in *s_in;
#ifdef ENABLE_INET6
  struct sockaddr_in6 *s6_in;
/*
  struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
*/
#endif /* ENABLE_INET6 */

  if ((rtcp_out_soc = socket(dvrecv_param->rtcp_out_s_addr.ss_family,
                    SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return(-1);
  }

  s_in = (struct sockaddr_in *)&dvrecv_param->rtcp_out_s_addr;
#ifdef ENABLE_INET6
  s6_in = (struct sockaddr_in6 *)&dvrecv_param->rtcp_out_s_addr;
#endif /* ENABLE_INET6 */

  switch (dvrecv_param->s_addr.ss_family) {
#ifdef ENABLE_INET6
    case AF_INET6:
#ifndef	 NO_SS_LEN
      s6_in->sin6_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
      s6_in->sin6_port = htons(dvrecv_param->port + 1);
      memcpy(&s6_in->sin6_addr, &in6addr_any, sizeof(struct in6_addr));
      break;
#endif /* ENABLE_INET6 */

    case AF_INET:
#ifndef	NO_SS_LEN
      s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
      s_in->sin_port = htons(dvrecv_param->port + 1);
      s_in->sin_addr.s_addr = htonl(INADDR_ANY);
      break;

    default:
      return(-1);
      break;
  }

  dvrecv_param->rtcp_out_soc = rtcp_out_soc;

  return(1);
}
