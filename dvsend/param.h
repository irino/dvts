#ifndef _DVSEND_PARAM_H_
#define _DVSEND_PARAM_H_

#include "ieee1394-struct.h"

#ifdef	LINUX_OLD_SS_FAMILY
#define	ss_family	__ss_family
#endif	/* LINUX_OLD_SS_FAMILY */

struct dest_list_obj {
  /* socket fd must be here, since setsockopt is needed */
  /* for setting multicast TTL and other things. */
  int soc;

  char  dest_str[256];
  int   port;
  struct sockaddr_storage s_addr;

  /* FOR MULTICAST */
  int multi_ttl;
  char *multi_ifname;
  char *multi_ifname6;

  u_int32_t	pkt_count;

  struct dest_list_obj *next;
};

struct rtcp_recv_obj {
  int soc;
  int port;

  struct sockaddr_storage s_addr;

  struct rtcp_recv_obj *next;
};

struct dvsend_param {
  int format;

  int default_port;
  int default_multi_ttl;
  char *default_multi_ifname;
  char *default_multi_ifname6;

  /* IP destination host list */
  struct dest_list_obj *dest_list;

  struct dest_list_obj *audio_dest_list;


  struct dest_list_obj *rtcp_out_list;
  struct dest_list_obj *rtcp_out_audio_list;

  struct rtcp_recv_obj *rtcp_recv_list;
  struct rtcp_recv_obj *rtcp_audio_recv_list;


  /*--------------------*/
  /* IEEE1394 PARAMTERS */
  /*--------------------*/
  struct ieee1394dv ieee1394dv;


  /*----------------------*/
  /* DV RELATED PARAMTERS */
  /*----------------------*/
  /* frame drop rate */
  int frame_drop;

  /* DIF blocks in each RTP packet */
  int DIFblocks_in_pkt;

  int audio_redundancy;

  /*-------------------------------*/
  /*             FLAGS             */
  /* see dvsend-flags.h for detail */
  /*-------------------------------*/
  unsigned long flags;
};

#endif /* _DVSEND_PARAM_H_ */
