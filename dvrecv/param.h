#ifndef _DVRECV_PARAM_H_
#define _DVRECV_PARAM_H_

#include "ieee1394-struct.h"

#ifdef	LINUX_OLD_SS_FAMILY
#define	ss_family	__ss_family
#endif	/* LINUX_OLD_SS_FAMILY */

#define		DVFRAME_READY			0x00000001
#define		DVFRAME_WRITING_DATA		0x00000002
#define		DVFRAME_WRITING_IEEE1394	0x00000004
#define		DVFRAME_DATA_READY		0x00000008

struct shm_framebuf {
  u_int32_t lock;
  struct dv1394_pkts dvframe;
  char video_recv_flag[DIFSEQ_NUM_PAL][135];
};

struct shm_frame {
  struct shm_frame *next;
  struct shm_frame *prev;
  struct shm_framebuf *framebuf;
};

struct audio_shm_framebuf {
  u_int32_t lock;
  struct dv_rawdata625_50_audio {
    struct dv_difblock dbn[9];
  } dseq[DIFSEQ_NUM_PAL];
};

struct audio_shm_frame {
  struct audio_shm_frame *next;
  struct audio_shm_framebuf *framebuf;
};

struct static_video_frame {
  u_int32_t flag;
  struct dv1394_pkts dvframe;
};

struct dvrecv_param {
  int format;
  int maxdifseq;

  struct ieee1394dv ieee1394dv;

  char *multicast_addr_str;
#ifndef IFNAMSIZ
#define	IFNAMSIZ	16
#endif	/* IFNAMSIZ */
  char multicast_ifname[IFNAMSIZ];

  int video_shmid;
  int audio_shmid;

  int frames_in_buffer;

  u_int32_t *last_dv_buffer;

  u_int32_t *video_buf;
  struct shm_frame *video_framebuf;

  u_int32_t *audio_buf;
  struct audio_shm_frame *audio_framebuf;

  struct static_video_frame *static_video_frame;

  struct sockaddr_storage s_addr;
  int port;

  struct sockaddr_storage audio_s_addr;
  int audio_port;

  int		maxfds;
  fd_set	fds;

  int soc;
  int audio_soc;

  struct sockaddr_storage rtcp_in_s_addr;
  struct sockaddr_storage rtcp_out_s_addr;
  int rtcp_out_soc;
  int rtcp_in_soc;

  struct sockaddr_storage audio_rtcp_in_s_addr;
  struct sockaddr_storage audio_rtcp_out_s_addr;
  int rtcp_out_audio_soc;
  int rtcp_in_audio_soc;

  struct sockaddr_storage multicast_s_addr;

  int read_loop_pid;
  int write_loop_pid;

  u_int32_t flags;

  /* when "show packet loss option" is set, show packet loss when */
  /* input packet number exceeds this value                       */
  u_int32_t pkt_loss_count;

  /* sum of packet loss */
  /* used by RTCP       */
  u_int32_t pkt_loss_sum;
  /* sum of input packets */
  /* used by RTCP         */
  u_int32_t pkt_count;
};

#endif /* _DVRECV_PARAM_H_ */
