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
#endif	/* HAVE_CONFIG */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "param.h"
#include "flags.h"
#include "udp.h"

static int _prepare_rtp_send  __P((struct dvsend_param *));
static int _prepare_rtcp_send __P((struct dvsend_param *));

static int _prepare_rtp_audio_send  __P((struct dvsend_param *));
static int _prepare_rtcp_audio_send __P((struct dvsend_param *));
static int _prepare_rtcp_audio_recv __P((struct dvsend_param *));


static int _prepare_send_sockaddr __P((char *, struct sockaddr_storage *, int));

static int _add_dest           __P((struct dvsend_param *,
                                    struct dest_list_obj *, char *));
static int _add_rtcp_out       __P((struct dvsend_param *,
                                    struct dest_list_obj *, char *));
static int _add_rtcp_out_audio __P((struct dvsend_param *,
                                    struct dest_list_obj *, char *));
static int _add_rtcp_recv      __P((struct dvsend_param *,
                                    struct dest_list_obj *));

static struct dest_list_obj *_dest_list_obj_alloc __P((void));
static struct dest_list_obj *_dest_list_obj_last  __P((struct dest_list_obj *));

static int _parse_dest_str __P((char *, char *, int, int *, int *, char **));

/****** static functions START ******/

static int
_parse_dest_str(char *dest_str,
                char *dest_ret, int dest_str_len,
                int *port_ret,
                int *multi_ttl_ret,
                char **multi_ifname_ret)
{
  int i;
  int number_of_slash = 0;
  char port_str[128];
  char multi_ttl_str[128];
  char multi_ifname_str[128];

  char *ptr, *ptr_prev;

  if (dest_str == NULL || dest_ret == NULL ||
      port_ret == NULL ||
      multi_ttl_ret == NULL || multi_ifname_ret == NULL) {
    return(-1);
  }

  if (strlen(dest_str) > dest_str_len) {
    return(-1);
  }

  memset(port_str, 0, sizeof(port_str));
  memset(multi_ttl_str, 0, sizeof(multi_ttl_str));
  memset(multi_ifname_str, 0, sizeof(multi_ifname_str));

  ptr = dest_str;
  ptr_prev = dest_str;
  for (i=0; i<strlen(dest_str); i++) {
    if (*ptr == '/') {
      switch (number_of_slash) {
        case 0:
          snprintf(dest_ret, ptr - ptr_prev + 1, "%s", ptr_prev);
          ptr_prev = ptr + 1;
          break;

        case 1:
          snprintf(port_str, ptr - ptr_prev + 1, "%s", ptr_prev);
          *port_ret = atoi(port_str);
          ptr_prev = ptr + 1;
          break;

        case 2:
          snprintf(multi_ttl_str, ptr - ptr_prev + 1, "%s", ptr_prev);
          ptr_prev = ptr + 1;
          *multi_ttl_ret = atoi(multi_ttl_str);
          break;

        case 3:
          snprintf(multi_ifname_str, ptr - ptr_prev + 1, "%s", ptr_prev);
          ptr_prev = ptr + 1;
          if (strlen(multi_ifname_str) < 0) {
            return(-1);
          }
          *multi_ifname_ret = (char *)malloc(strlen(multi_ifname_str) + 1);
          memset(*multi_ifname_ret, 0, strlen(multi_ifname_str) + 1);
          memcpy(*multi_ifname_ret, multi_ifname_str, strlen(multi_ifname_str));
          break;

        default:
          break;
      }

      number_of_slash++;
    }

    ptr++;
  }

  switch (number_of_slash) {
    case 0:
      snprintf(dest_ret, dest_str_len, "%s", dest_str);
      break;

    case 1:
      snprintf(port_str, ptr - ptr_prev + 1, "%s", ptr_prev);
      *port_ret = atoi(port_str);
      break;

    case 2:
      snprintf(multi_ttl_str, ptr - ptr_prev + 1, "%s", ptr_prev);
      *multi_ttl_ret = atoi(multi_ttl_str);
      break;

    case 3:
      snprintf(multi_ifname_str, ptr - ptr_prev + 1, "%s", ptr_prev);
      if (strlen(multi_ifname_str) < 0) {
        return(-1);
      }
      *multi_ifname_ret = (char *)malloc(strlen(multi_ifname_str) + 1);
      memset(*multi_ifname_ret, 0, strlen(multi_ifname_str) + 1);
      memcpy(*multi_ifname_ret, multi_ifname_str, strlen(multi_ifname_str));
      break;

    default:
      break;
  }

  return(1);
}

static struct dest_list_obj *
_dest_list_obj_alloc(void)
{
  struct dest_list_obj *dest_list;

  dest_list = malloc(sizeof(struct dest_list_obj));
  if (dest_list == NULL) {
    perror("malloc");
    assert(NULL);
  }
  memset(dest_list, 0, sizeof(struct dest_list_obj));

  return(dest_list);
}

static struct dest_list_obj *
_dest_list_obj_last(struct dest_list_obj *dest_list)
{
  struct dest_list_obj *dest_list_ret;

  dest_list_ret = dest_list;
  while (dest_list_ret->next != NULL) {
    dest_list_ret = dest_list_ret->next;
  }

  return(dest_list_ret);
}

static int
_add_dest(struct dvsend_param *dvsend_param,
          struct dest_list_obj *dest_list,
          char *buf)
{
  _parse_dest_str(buf,
                  dest_list->dest_str,
                  sizeof(dest_list->dest_str),
                  &dest_list->port,
                  &dest_list->multi_ttl,
                  &dest_list->multi_ifname);

  if (dest_list->port  < 1) {
    dest_list->port = dvsend_param->default_port;
  }
  if (dest_list->multi_ttl < 1) {
    dest_list->multi_ttl = dvsend_param->default_multi_ttl;
  }
  if (dest_list->multi_ifname == NULL) {
    dest_list->multi_ifname = dvsend_param->default_multi_ifname;
  }

#ifdef	ENABLE_INET6
  if (dvsend_param->flags & USE_IPV4_ONLY) {
    dest_list->s_addr.ss_family = AF_INET;
#ifndef	NO_SS_LEN
    dest_list->s_addr.ss_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
  }
  else if (dvsend_param->flags & USE_IPV6_ONLY) {
    dest_list->s_addr.ss_family = AF_INET6;
#ifndef	NO_SS_LEN
    dest_list->s_addr.ss_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
  }
  else {
    dest_list->s_addr.ss_family = PF_UNSPEC;
  }
#else	/* ENABLE_INET6 */
  dest_list->s_addr.ss_family = AF_INET;
#ifndef	NO_SS_LEN
  dest_list->s_addr.ss_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
#endif	/* ENABLE_INET6 */

  return(1);
}

static int
_add_rtcp_out(struct dvsend_param *dvsend_param,
              struct dest_list_obj *rtcp_out,
              char *buf)
{
  _parse_dest_str(buf,
                  rtcp_out->dest_str,
                  sizeof(rtcp_out->dest_str),
                  &rtcp_out->port,
                  &rtcp_out->multi_ttl,
                  &rtcp_out->multi_ifname);

  if (rtcp_out->port  < 1) {
    rtcp_out->port = dvsend_param->default_port + 1;
  } else {
    rtcp_out->port++;
  }
  if (rtcp_out->multi_ttl < 1) {
    rtcp_out->multi_ttl = dvsend_param->default_multi_ttl;
  }
  if (rtcp_out->multi_ifname == NULL) {
    rtcp_out->multi_ifname = dvsend_param->default_multi_ifname;
  }

#ifdef	ENABLE_INET6
  if (dvsend_param->flags & USE_IPV4_ONLY) {
    rtcp_out->s_addr.ss_family = AF_INET;
#ifndef	NO_SS_LEN
    rtcp_out->s_addr.ss_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
  } else if (dvsend_param->flags & USE_IPV6_ONLY) {
    rtcp_out->s_addr.ss_family = AF_INET6;
#ifndef	NO_SS_LEN
    rtcp_out->s_addr.ss_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
  } else {
    rtcp_out->s_addr.ss_family = PF_UNSPEC;
  }
#else	/* ENABLE_INET6 */
  rtcp_out->s_addr.ss_family = AF_INET;
#ifndef	NO_SS_LEN
  rtcp_out->s_addr.ss_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
#endif	/* ENABLE_INET6 */

  return(1);
}

static int
_add_rtcp_out_audio(struct dvsend_param *dvsend_param,
                    struct dest_list_obj *rtcp_out_audio,
                    char *buf)
{
  snprintf(rtcp_out_audio->dest_str,
           sizeof(rtcp_out_audio->dest_str), "%s", buf);
  rtcp_out_audio->port = dvsend_param->default_port + 1;

#ifdef	ENABLE_INET6
  if (dvsend_param->flags & USE_IPV4_ONLY) {
    rtcp_out_audio->s_addr.ss_family = AF_INET;
#ifndef	NO_SS_LEN
    rtcp_out_audio->s_addr.ss_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
  }
  else if (dvsend_param->flags & USE_IPV6_ONLY) {
    rtcp_out_audio->s_addr.ss_family = AF_INET6;
#ifndef	NO_SS_LEN
    rtcp_out_audio->s_addr.ss_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
  }
  else {
    rtcp_out_audio->s_addr.ss_family = PF_UNSPEC;
  }
#else	/* ENABLE_INET6 */
  rtcp_out_audio->s_addr.ss_family = AF_INET;
#ifndef	NO_SS_LEN
  rtcp_out_audio->s_addr.ss_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
#endif	/* ENABLE_INET6 */

  return(1);
}

static int
_add_rtcp_recv(struct dvsend_param *dvsend_param,
               struct dest_list_obj *rtcp_out)
{
  struct rtcp_recv_obj *new;
  struct rtcp_recv_obj *rtcp_recv;
  struct sockaddr_in *s_in, *s_in_rtcp;
  struct sockaddr_in6 *s_in6, *s_in6_rtcp;

  if (dvsend_param->rtcp_recv_list == NULL) {
    new = (struct rtcp_recv_obj *)malloc(sizeof(struct rtcp_recv_obj));
    memcpy(&new->s_addr, &rtcp_out->s_addr, sizeof(new->s_addr));
    new->port = rtcp_out->port;

    switch (new->s_addr.ss_family) {
      case AF_INET:
        s_in = (struct sockaddr_in *)&new->s_addr;
        memset(&s_in->sin_addr, 0, sizeof(s_in->sin_addr));
        break;

#ifdef	AF_INET6
      case AF_INET6:
        s_in6 = (struct sockaddr_in6 *)&new->s_addr;
        memset(&s_in6->sin6_addr, 0, sizeof(s_in6->sin6_addr));
        break;
#endif	/* AF_INET6 */

      default:
        printf("rtcp recv init error\n");
        break;
    }

    if (bind (rtcp_out->soc,
              (struct sockaddr *)&new->s_addr,
#ifndef	NO_SS_LEN
              new->s_addr.ss_len) < 0) {
#else	/* NO_SS_LEN */
              sizeof(new->s_addr)) < 0) {
#endif	/* NO_SS_LEN */
      perror("rtcp recv bind");
      return(-1);
    }

    if (fcntl(rtcp_out->soc, F_SETFL, O_NONBLOCK) < 0) {
      perror("rtcp recv fcntl O_NONBLOCK");
      return(-1);
    }

    new->soc = rtcp_out->soc;

    dvsend_param->rtcp_recv_list = new;
  } else {
    rtcp_recv = dvsend_param->rtcp_recv_list;
    while (rtcp_recv != NULL) {
      if (rtcp_recv->s_addr.ss_family == rtcp_out->s_addr.ss_family) {
        switch (rtcp_recv->s_addr.ss_family) {
          case AF_INET:
            s_in = (struct sockaddr_in *)&rtcp_recv->s_addr;
            s_in_rtcp = (struct sockaddr_in *)&rtcp_out->s_addr;
            if (s_in->sin_port == s_in_rtcp->sin_port) {
              /* TBD */
            }
            break;

#ifdef	AF_INET6
          case AF_INET6:
            s_in6 = (struct sockaddr_in6 *)&rtcp_recv->s_addr;
            s_in6_rtcp = (struct sockaddr_in6 *)&rtcp_out->s_addr;
            if (s_in6->sin6_port == s_in6_rtcp->sin6_port) {
              /* TBD */
            }
            break;
#endif	/* AF_INET6 */
        }
      }

      rtcp_recv = rtcp_recv->next;
    }
  }

  return(1);
}
/****** static functions END ******/

int
add_destination(struct dvsend_param *dvsend_param, char *buf)
{
  struct dest_list_obj *list_tmp;

  struct dest_list_obj *dest;
  struct dest_list_obj *rtcp_out;

  dest = _dest_list_obj_alloc();

  if (dvsend_param->dest_list == NULL) {
    dvsend_param->dest_list = dest;
  } else {
    list_tmp = _dest_list_obj_last(dvsend_param->dest_list);
    list_tmp->next = dest;
  }

  _add_dest(dvsend_param, dest, buf);

  /********/

  if (dvsend_param->flags & USE_RTCP) {
    rtcp_out = _dest_list_obj_alloc();

    if (dvsend_param->rtcp_out_list == NULL) {
      dvsend_param->rtcp_out_list = rtcp_out;
    } else {
      list_tmp = _dest_list_obj_last(dvsend_param->rtcp_out_list);
      list_tmp->next = rtcp_out;
    }

    _add_rtcp_out(dvsend_param, rtcp_out, buf);
  } /* if (dvsend_param->flags & USE_RTCP) */

  return(1);
}

int
add_audio_only_destination(struct dvsend_param *dvsend_param, char *buf)
{
  struct dest_list_obj *list_tmp;

  struct dest_list_obj *dest;
  struct dest_list_obj *rtcp_out_audio;

  dest = _dest_list_obj_alloc();

  if (dvsend_param->audio_dest_list == NULL) {
    dvsend_param->audio_dest_list = dest;
  } else {
    list_tmp = _dest_list_obj_last(dvsend_param->audio_dest_list);
    list_tmp->next = dest;
  }

  if (dvsend_param->flags & USE_RTCP) {
    rtcp_out_audio = _dest_list_obj_alloc();

    if (dvsend_param->rtcp_out_audio_list == NULL) {
      dvsend_param->rtcp_out_audio_list = rtcp_out_audio;
    } else {
      list_tmp = _dest_list_obj_last(dvsend_param->rtcp_out_audio_list);
      list_tmp->next = rtcp_out_audio;
    }

    _add_rtcp_out_audio(dvsend_param, rtcp_out_audio, buf);
  }

  return(1);
}

int
send_pkt(struct dvsend_param *dvsend_param, u_int32_t *buf, int buflen)
{
  int n;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->dest_list;
  while (dest_list != NULL) {
    n = sendto(dest_list->soc,
               buf,
               buflen,
               0,
               (struct sockaddr *)&dest_list->s_addr,
#ifndef	NO_SS_LEN
               dest_list->s_addr.ss_len);
#else	/* NO_SS_LEN */
               sizeof(dest_list->s_addr));
#endif	/* NO_SS_LEN */

    if (n < 1) {
      perror("send_pkt : sendto");
    }

    dest_list->pkt_count++;

    dest_list = dest_list->next;
  }

  return(1);
}

int
send_audio_pkt(struct dvsend_param *dvsend_param, u_int32_t *buf, int buflen)
{
  int n;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->audio_dest_list;
  while (dest_list != NULL) {
    n = sendto(dest_list->soc,
               buf,
               buflen,
               0,
               (struct sockaddr *)&dest_list->s_addr,
               sizeof(dest_list->s_addr));

    dest_list->pkt_count++;

    dest_list = dest_list->next;
  }

  return(1);
}

int
send_rtcp_pkt(struct dvsend_param *dvsend_param, u_int32_t *buf, int buflen)
{
  int n;
  int saddr_size = 0;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->rtcp_out_list;
  while (dest_list != NULL) {
    switch (dest_list->s_addr.ss_family) {
      case AF_INET:
        saddr_size = sizeof(struct sockaddr_in);
        break;
      case AF_INET6:
        saddr_size = sizeof(struct sockaddr_in6);
        break;
      default:
        saddr_size = 0;
        break;
    }

    n = sendto(dest_list->soc,
               buf,
               buflen,
               0,
               (struct sockaddr *)&dest_list->s_addr,
               saddr_size);
    if (n < 1) {
      perror("rtcp sendto");
    }

    dest_list = dest_list->next;
  }

  return(1);
}

int
send_rtcp_audio_pkt(struct dvsend_param *dvsend_param,
                    u_int32_t *buf,
                    int buflen)
{
  int n;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->rtcp_out_audio_list;
  while (dest_list != NULL) {
    n = sendto(dest_list->soc,
               buf,
               buflen,
               0,
               (struct sockaddr *)&dest_list->s_addr,
               sizeof(dest_list->s_addr));

    dest_list = dest_list->next;
  }

  return(1);
}

int
prepare_udp_socket (struct dvsend_param *dvsend_param)
{
  if (_prepare_rtp_send(dvsend_param) < 0) {
    printf("failed to create RTP send socket\n");
    return(-1);
  }

  if (dvsend_param->flags & SEPARATED_AUDIO_STREAM) {
    if (_prepare_rtp_audio_send(dvsend_param) < 0) {
      printf("failed to create RTP audio send socket\n");
      return(-1);
    }
  }

  if (dvsend_param->flags & USE_RTCP) {
    if (_prepare_rtcp_send(dvsend_param) < 0) {
      printf("failed to create RTCP send socket\n");
      return(-1);
    }

    if (dvsend_param->flags & SEPARATED_AUDIO_STREAM) {
      if (_prepare_rtcp_audio_send(dvsend_param) < 0) {
        printf("failed to create RTCP audio socket\n");
        return(-1);
      }
      if (_prepare_rtcp_audio_recv(dvsend_param) < 0) {
        printf("failed to create RTCP audio receive socket\n");
        return(-1);
      }
    }
  }

  return(1);
}

static int
_prepare_rtp_send (struct dvsend_param *dvsend_param)
{
  int ret;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->dest_list;
  while (dest_list != NULL) {
    ret = _prepare_send_sockaddr(dest_list->dest_str,
                                 &dest_list->s_addr,
                                 dest_list->port);
    if (ret < 0) {
      return(-1);
    }

    printf("destination : %s\n", dest_list->dest_str);
    printf("port        : %d\n", dest_list->port);
    printf("\n");

    dest_list->soc = socket(dest_list->s_addr.ss_family, SOCK_DGRAM, 0);
    if (dest_list->soc < 0) {
      perror("socket");
      return(dest_list->soc);
    }

    dest_list = dest_list->next;
  }

  return(1);
}

static int
_prepare_rtp_audio_send (struct dvsend_param *dvsend_param)
{
  int ret;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->audio_dest_list;
  while (dest_list != NULL) {
    ret = _prepare_send_sockaddr(dest_list->dest_str,
                                 &dest_list->s_addr,
                                 dest_list->port);
    if (ret < 0) {
      return(-1);
    }

    printf("AUDIO ONLY   : %s, %d\n", dest_list->dest_str, dest_list->port);

    dest_list = dest_list->next;
  }

  return(1);
}


/********** RTCP ************/

static int
_prepare_rtcp_send (struct dvsend_param *dvsend_param)
{
  int ret;
  int soc_found;
  struct dest_list_obj *dest_list_obj;

  struct rtcp_recv_obj *recv_list;

  dest_list_obj = dvsend_param->rtcp_out_list;
  recv_list = dvsend_param->rtcp_recv_list;

  while (dest_list_obj != NULL) {
    ret = _prepare_send_sockaddr(dest_list_obj->dest_str,
                                 &dest_list_obj->s_addr,
                                 dest_list_obj->port);
    if (ret < 0) {
      return(-1);
    }

    printf("RTCP dest   : %s\n", dest_list_obj->dest_str);
    printf("RTCP port   : %d\n", dest_list_obj->port);

    recv_list = dvsend_param->rtcp_recv_list;
    soc_found = 0;
    while (recv_list != NULL) {
      if (recv_list->s_addr.ss_family == dest_list_obj->s_addr.ss_family) {
        if (recv_list->port == dest_list_obj->port) {
          recv_list->soc = dest_list_obj->soc;
          soc_found = 1;
          break;
        }
      }
      recv_list = recv_list->next;
    }

    if (soc_found == 0) {
      dest_list_obj->soc = socket(dest_list_obj->s_addr.ss_family,
                                  SOCK_DGRAM, 0);
      if (dest_list_obj->soc < 0) {
        perror("socket");
        return(dest_list_obj->soc);
      }
    }

    _add_rtcp_recv(dvsend_param, dest_list_obj);

    dest_list_obj = dest_list_obj->next;
  }

  return(1);
}

static int
_prepare_rtcp_audio_send (struct dvsend_param *dvsend_param)
{
  int ret;
  struct dest_list_obj *dest_list;

  dest_list = dvsend_param->rtcp_out_audio_list;

  while (dest_list != NULL) {
    ret = _prepare_send_sockaddr(dest_list->dest_str,
                                 &dest_list->s_addr,
                                 dest_list->port);
    if (ret < 0) {
      return(-1);
    }

    printf("RTCP audio dest : %s\n", dest_list->dest_str);
    printf("RTCP audio port : %d\n", dest_list->port);

    dest_list->soc = socket(dest_list->s_addr.ss_family,
                                SOCK_DGRAM, 0);
    if (dest_list->soc < 0) {
      perror("socket");
      return(dest_list->soc);
    }
  }

  return(1);
}

static int
_prepare_rtcp_audio_recv (struct dvsend_param *dvsend_param)
{
  int soc;
  struct sockaddr_in *s_in;
#ifdef ENABLE_INET6
  struct sockaddr_in6 *s6_in;
  struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
#endif /* ENABLE_INET6 */

  struct rtcp_recv_obj *rtcp_recv;

  rtcp_recv = dvsend_param->rtcp_audio_recv_list;
  while (rtcp_recv != NULL) {
    if ((soc = socket(rtcp_recv->s_addr.ss_family,
                      SOCK_DGRAM, 0)) < 0) {
      perror("rtcp audio recv socket");
      return(-1);
    }

    s_in = (struct sockaddr_in *)&rtcp_recv->s_addr;
#ifdef ENABLE_INET6
    s6_in = (struct sockaddr_in6 *)&rtcp_recv->s_addr;
#endif /* ENABLE_INET6 */

    switch (rtcp_recv->s_addr.ss_family) {
#ifdef ENABLE_INET6
      case AF_INET6:
#ifndef	NO_SS_LEN
        s6_in->sin6_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
        s6_in->sin6_port = htons(rtcp_recv->port);
        memcpy(&s6_in->sin6_addr, &in6addr_any, sizeof(struct in6_addr));
        break;
#endif /* ENABLE_INET6 */

      case AF_INET:
#ifndef	NO_SS_LEN
        s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
        s_in->sin_port = htons(rtcp_recv->port);
        s_in->sin_addr.s_addr = htonl(INADDR_ANY);
        break;

      default:
        return(-1);
        break;
    }

    if (bind (soc,
              (struct sockaddr *)&rtcp_recv->s_addr,
#ifndef	NO_SS_LEN
              rtcp_recv->s_addr.ss_len) < 0) {
#else	/* NO_SS_LEN */
              sizeof(rtcp_recv->s_addr)) < 0) {
#endif	/* NO_SS_LEN */
      perror("rtcp recv bind");
      return(-1);
    }

    if (fcntl(soc, F_SETFL, O_NONBLOCK) < 0) {
      perror("rtcp audio recv fcntl, O_NONBLOCK");
      return(-1);
    }

    rtcp_recv->soc = soc;
    rtcp_recv = rtcp_recv->next;
  }

  return(1);
}

static int
_prepare_send_sockaddr(char *dsthost, struct sockaddr_storage *saddr, int port)
{
#ifdef ENABLE_INET6
  struct sockaddr_in *s_in = (struct sockaddr_in *)saddr;
  struct sockaddr_in6 *s6_in = (struct sockaddr_in6 *)saddr;
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  if (saddr->ss_family != 0) {
    hints.ai_family = saddr->ss_family;
  }

  if (getaddrinfo(dsthost, NULL, &hints, &res) != 0) {
    perror("getaddrinfo");
    printf("%s\n", dsthost);
    return(-1);
  }

  saddr->ss_family = ((struct sockaddr *)res->ai_addr)->sa_family;
  switch (saddr->ss_family) {
    case AF_INET6:
#ifndef	NO_SS_LEN
      s6_in->sin6_len = sizeof(struct sockaddr_in6);
#endif	/* NO_SS_LEN */
      s6_in->sin6_port = htons(port);
      memcpy(&s6_in->sin6_addr,
             &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,
             res->ai_addrlen);
      break;

    case AF_INET:
#ifndef	NO_SS_LEN
      s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
      s_in->sin_port = htons(port);
      s_in->sin_addr.s_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
      break;

    default:
      printf("Unknown address family\n");
      return(-1);
      break;
  }
#else
  struct sockaddr_in *s_in = (struct sockaddr_in *)saddr;
  struct hostent *hp;

  if ((s_in->sin_addr.s_addr = inet_addr(dsthost)) == -1) {
    if ((hp = gethostbyname(dsthost)) == NULL) {
      herror("gethostbyname");
      return(-1);
    }
    s_in->sin_addr.s_addr = **(u_int32_t **)hp->h_addr_list;
  }

  s_in->sin_family = AF_INET;
#ifndef	NO_SS_LEN
  s_in->sin_len = sizeof(struct sockaddr_in);
#endif	/* NO_SS_LEN */
  s_in->sin_port = htons(port);
#endif /* ENABLE_INET6 */

  return(1);
}
