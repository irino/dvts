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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <dvts.h>

#include "multicast.h"

static int _multicast_str2addr __P((struct dvrecv_param *));
static int _chk_multicast_addr __P((struct dvrecv_param *));
static int _chk_multicast_family __P((struct dvrecv_param *));
static int _multicast_join __P((struct dvrecv_param *));

int
multicast_settings (struct dvrecv_param *dvrecv_param)
{
  /* no multicast address is requested */
  if (dvrecv_param->multicast_addr_str == NULL) {
    return(1);
  }

  if (_multicast_str2addr(dvrecv_param) < 0) {
    return(-1);
  }

  if (_chk_multicast_addr(dvrecv_param) < 0) {
    return(-1);
  }

  if (_chk_multicast_family(dvrecv_param) < 0) {
    return(-1);
  }

  if (_multicast_join(dvrecv_param) < 0) {
    return(-1);
  }

  return(1);
}

static int
_multicast_str2addr (struct dvrecv_param *dvrecv_param)
{
#ifdef	ENABLE_INET6
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  if (getaddrinfo(dvrecv_param->multicast_addr_str, NULL, &hints, &res) != 0) {
    perror("getaddrinfo");
    return(-1);
  }

  memcpy(&dvrecv_param->multicast_s_addr, res->ai_addr, res->ai_addrlen);
#else  /* ENABLE_INET6 */
  struct sockaddr_in *s_in = (struct sockaddr_in *)&dvrecv_param->multicast_s_addr;

  s_in->sin_addr.s_addr = inet_addr(dvrecv_param->multicast_addr_str);
  if (s_in->sin_addr.s_addr == -1) {
    return(-1);
  }
#endif /* ENABLE_INET6 */

  return(1);
}

static int
_chk_multicast_addr (struct dvrecv_param *dvrecv_param)
{
  struct sockaddr_in *s_in;
#ifdef ENABLE_INET6
  struct sockaddr_in6 *s6_in;
#endif	/* ENABLE_INET6 */

  switch (dvrecv_param->multicast_s_addr.ss_family) {
    case AF_INET:
      s_in = (struct sockaddr_in *)&dvrecv_param->multicast_s_addr;

      if (!IN_MULTICAST(ntohl(s_in->sin_addr.s_addr))) {
        printf("%s\n", dvrecv_param->multicast_addr_str);
        printf("ERROR : requested multicast join address is not a multicast address\n");
        return(-1);
      }
      break;

#ifdef ENABLE_INET6
    case AF_INET6:
      s6_in = (struct sockaddr_in6 *)&dvrecv_param->multicast_s_addr;

      if (!IN6_IS_ADDR_MULTICAST(&s6_in->sin6_addr)) {
        printf("%s\n", dvrecv_param->multicast_addr_str);
        printf("ERROR : requested multicast join address is not a multicast address\n");
        return(-1);
      }
      break;
#endif	/* ENABLE_INET6 */

    default:
      printf("Mulitcast join error : unknown address family\n");
      return(-1);
      break;
  }

  return(1);
}

static int
_chk_multicast_family (struct dvrecv_param *dvrecv_param)
{
  if (dvrecv_param->s_addr.ss_family !=
        dvrecv_param->multicast_s_addr.ss_family) {

      printf("Protocol family of DV/RTP socket and multicast join address mismatch\n");

      printf("multicast join address : %s\n", dvrecv_param->multicast_addr_str);

    switch (dvrecv_param->s_addr.ss_family) {
      case AF_INET:
        printf("DV/RTP socket family   : IPv4 (AF_INET)\n");
        break;

#ifdef ENABLE_INET6
      case AF_INET6:
        printf("DV/RTP socket family   : IPv6 (AF_INET6)\n");
        break;
#endif	/* ENABLE_INET6 */

      default:
        break;
    }
    switch (dvrecv_param->multicast_s_addr.ss_family) {
      case AF_INET:
        printf("multicast join address : IPv4 (AF_INET)\n");
        break;

#ifdef ENABLE_INET6
      case AF_INET6:
        printf("multicast join address : IPv6 (AF_INET6)\n");
        break;
#endif	/* ENABLE_INET6 */

      default:
        break;
    }

    return(-1);
  }

  return(1);
}

static int
_multicast_join (struct dvrecv_param *dvrecv_param)
{
  struct ip_mreq mreq;
#ifdef	ENABLE_INET6
  struct ipv6_mreq mreq6;

  if (dvrecv_param->multicast_s_addr.ss_family == AF_INET6) {
    memcpy((char *)&mreq6.ipv6mr_multiaddr,
           (char *)&((struct sockaddr_in6 *)&dvrecv_param->multicast_s_addr)->sin6_addr,
           sizeof(((struct sockaddr_in6 *)&dvrecv_param->multicast_s_addr)->sin6_addr));

    mreq6.ipv6mr_interface = if_nametoindex(dvrecv_param->multicast_ifname);

#ifdef	IPV6_ADD_MEMBERSHIP
    if (setsockopt(dvrecv_param->soc, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                   (char *)&mreq6, sizeof(mreq6)) < 0) {
#else	/* IPV6_ADD_MEMBERSHIP */
    if (setsockopt(dvrecv_param->soc, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                   (char *)&mreq6, sizeof(mreq6)) < 0) {
#endif	/* IPV6_ADD_MEMBERSHIP */
      perror("ip_add_membership");
    }
  }
#endif	/* ENABLE_INET6 */

  if (dvrecv_param->multicast_s_addr.ss_family == AF_INET) {
    mreq.imr_multiaddr = ((struct sockaddr_in *)&dvrecv_param->multicast_s_addr)->sin_addr;
    mreq.imr_interface = ((struct sockaddr_in *)&dvrecv_param->s_addr)->sin_addr;

    if (setsockopt(dvrecv_param->soc, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (char *)&mreq, sizeof(mreq)) < 0) {
      perror("ip_add_membership");
    }
  }

  return(1);
}
