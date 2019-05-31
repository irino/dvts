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
#include <string.h>
#include <assert.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "param.h"
#include "multicast.h"

static void
_IPv4_multicast_settings __P((struct dest_list_obj *));

#ifdef ENABLE_INET6
static void
_IPv6_multicast_settings __P((struct dest_list_obj *));
#endif /* ENABLE_INET6 */

void
multicast_settings (struct dvsend_param *dvsend_param)
{
  struct dest_list_obj *dest_list;
  struct sockaddr *s_addr;

  struct dest_list_obj *rtcp_out;

  assert(dvsend_param);

  dest_list = (struct dest_list_obj *)dvsend_param->dest_list;

  if (dest_list == NULL) {
    return;
  }

  while (1) {
    s_addr = (struct sockaddr *)&dest_list->s_addr;
    switch (s_addr->sa_family) {
      case AF_INET:
        if (IN_MULTICAST(ntohl(((struct sockaddr_in *)s_addr)->sin_addr.s_addr))) {
          _IPv4_multicast_settings(dest_list);
        }
        break;

#ifdef ENABLE_INET6
      case AF_INET6:
        if (IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)s_addr)->sin6_addr)) {
          _IPv6_multicast_settings(dest_list);
        }
        break;
#endif /* ENABLE_INET6 */

      default:
        printf("multicast_settings : UNKNOWN sa_family\n");
        assert(0);
        break;
    }

    dest_list = dest_list->next;
    if (dest_list == NULL) {
      break;
    }
  }

  /**********/

  rtcp_out = (struct dest_list_obj *)dvsend_param->rtcp_out_list;

  if (rtcp_out == NULL) {
    return;
  }
  while (1) {
    s_addr = (struct sockaddr *)&rtcp_out->s_addr;
    switch (s_addr->sa_family) {
      case AF_INET:
        if (IN_MULTICAST(ntohl(((struct sockaddr_in *)s_addr)->sin_addr.s_addr))) {
          _IPv4_multicast_settings(rtcp_out);
        }
        break;

#ifdef ENABLE_INET6
      case AF_INET6:
        if (IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)s_addr)->sin6_addr)) {
          _IPv6_multicast_settings(rtcp_out);
        }
        break;
#endif /* ENABLE_INET6 */

      default:
        printf("multicast_settings : UNKNOWN sa_family\n");
        assert(0);
        break;
    }

    rtcp_out = rtcp_out->next;
    if (rtcp_out == NULL) {
      break;
    }
  }
}

static void
_IPv4_multicast_settings (struct dest_list_obj *dest_list_obj)
{
  u_char ttl = dest_list_obj->multi_ttl;
  struct in_addr addr;

  if (setsockopt(dest_list_obj->soc,
                 IPPROTO_IP,
                 IP_MULTICAST_TTL,
                 (void *)&ttl,
                 sizeof(ttl)) < 0) {
    perror("setsockopt multicast ttl");
  }

  if (dest_list_obj->multi_ifname) {
    memset(&addr, 0, sizeof(addr));
    addr.s_addr = inet_addr(dest_list_obj->multi_ifname);
    if (setsockopt(dest_list_obj->soc,
                   IPPROTO_IP,
                   IP_MULTICAST_IF,
                   &addr, sizeof(addr)) < 0) {
      perror("setsockopt IPv4 multicast interface");
    }
  }
}

#ifdef ENABLE_INET6
static void
_IPv6_multicast_settings (struct dest_list_obj *dest_list_obj)
{
  /* needs this because setsockopt for IPv6 multicast requires "int" */
  int hoplimit = dest_list_obj->multi_ttl;
  int ifindex;

  if (setsockopt(dest_list_obj->soc,
                 IPPROTO_IPV6,
                 IPV6_MULTICAST_HOPS,
                 (void *)&hoplimit, sizeof(hoplimit)) < 0) {
    perror("setsockopt IPv6 multicast hops");
  }

  if (dest_list_obj->multi_ifname6) {
    ifindex = if_nametoindex(dest_list_obj->multi_ifname6);
    if (setsockopt(dest_list_obj->soc,
                   IPPROTO_IPV6,
                   IPV6_MULTICAST_IF,
                   &ifindex, sizeof(ifindex)) < 0) {
      perror("setsockopt IPv6 multicast interface");
    }
  }
}
#endif	/* ENABLE_INET6 */
