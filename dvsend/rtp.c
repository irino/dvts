#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <rtpvar.h>

#include "param.h"
#include "flags.h"
#include "rtp.h"
#include "udp.h"
#include "dvts.h"

#define		MAX_PACKET_LENGTH	1500

/* buffer for UDP */
static u_int32_t outbuf[MAX_PACKET_LENGTH/4];
static int outbuf_len;

/* buffer for UDP (for separate audio) */
static u_int32_t audiobuf[MAX_PACKET_LENGTH/4];
static int audiobuf_len;

static u_int32_t r_outbuf[MAX_PACKET_LENGTH/4];
static int r_outbuf_len;

static void _add_dvdif_to_outbuf __P((struct dvsend_param *, u_int32_t *));
static void _add_dvdif_to_r_outbuf __P((struct dvsend_param *, u_int32_t *));
static void _add_dvdif_to_audio_outbuf __P((struct dvsend_param *, u_int32_t *));
static void _dvdif_flush __P((struct dvsend_param *));


/*---------------------------------------------*/
/* These functions will process each DIF block */
/*---------------------------------------------*/
static void _process_sct_header  __P((struct dvsend_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_subcode __P((struct dvsend_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_vaux    __P((struct dvsend_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_audio   __P((struct dvsend_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_video   __P((struct dvsend_param *,
                                      u_char, u_char, u_char, u_int32_t *));

/* function container */
static void (*sct_func[5]) __P((struct dvsend_param *,
                                u_char, u_char, u_char, u_int32_t *));

/* The number of frame        */
/* will be used by frame_drop */
static u_long frame = 1;

/* RTP header */
static rtp_hdr_t *rtphdr;
static rtp_hdr_t *audio_rtphdr;

void
proc_dvdif_init(void)
{
  srandom(time(0));

  memset(outbuf, 0, sizeof(outbuf));
  outbuf_len = 0;
  rtphdr = (rtp_hdr_t *)outbuf;

  memset(audiobuf, 0, sizeof(audiobuf));
  audiobuf_len = 0;
  audio_rtphdr = (rtp_hdr_t *)audiobuf;

  memset(r_outbuf, 0, sizeof(r_outbuf));
  r_outbuf_len = 0;

  sct_func[0] = _process_sct_header;
  sct_func[1] = _process_sct_subcode;
  sct_func[2] = _process_sct_vaux;
  sct_func[3] = _process_sct_audio;
  sct_func[4] = _process_sct_video;

  /* RTP version 2 */
  rtphdr->version = RTP_VERSION;
  audio_rtphdr->version = RTP_VERSION;

  /* padding flag                                              */
  /* Padding will be required if the RTP payload length is NOT */
  /* multiple of 4. There will be no padding in DV/RTP, since  */
  /* DV/RTP payload will always be multiple of 80.             */
  rtphdr->p = 0;
  audio_rtphdr->p = 0;

  /* RTP payload type */
  /* XXX */
  rtphdr->pt = 0;
  audio_rtphdr->pt = 0;

  /*
     There aren't much meaning for doing htons and htonl
     on random values. However, this is done for remebering
     to exchange host byte order to network byte order.
  */
   
  /* RTP sequence number */
  rtphdr->seq = htons(random());
  audio_rtphdr->seq = htons(random());

  /* RTP timestamp */
  rtphdr->ts  = htonl(random());
  audio_rtphdr->ts  = htonl(random());

  /* RTP SSRC */
  rtphdr->ssrc = htonl(random());
  audio_rtphdr->ssrc = htonl(random());

  memcpy(r_outbuf, outbuf, sizeof(rtp_hdr_t));
}

void
proc_dvdif(struct dvsend_param *dvsend_param,
           u_int32_t *difblock)
{
  static u_char seq, dseq, dbn, sct;

  if (difblock == NULL) {
    printf("proc_dvdif: difblock == NULL\n");
    return;
  }

  dbn  = (ntohl(difblock[0]) >>  8) & 0xff;
  dseq = (ntohl(difblock[0]) >> 20) & 0xf;
  seq  = (ntohl(difblock[0]) >> 24) & 0xf;
  sct  = (ntohl(difblock[0]) >> 29) & 0x7;

  if (sct > 0x4) {
    printf("WARNING : INVALID DV SCT [%d]\n", sct);
    return;
  }

  (sct_func[sct]) (dvsend_param, seq, dseq, dbn, difblock);
}

/* This will process DV HEADER DIF blocks */
static void
_process_sct_header (struct dvsend_param *dvsend_param,
                     u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  if (dvsend_param->frame_drop > 1 &&
      (frame % dvsend_param->frame_drop) != 0) {
    /* drop this frame */
  }
  else {
    _add_dvdif_to_outbuf(dvsend_param, difblock);
  }
}

/* This will process DV SUBCODE DIF blocks */
static void
_process_sct_subcode (struct dvsend_param *dvsend_param,
                      u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  if (dvsend_param->frame_drop > 1 &&
      (frame % dvsend_param->frame_drop) != 0) {
    /* drop this frame */
  }
  else {
    _add_dvdif_to_outbuf(dvsend_param, difblock);
  }
}

/* This will process DV VAUX DIF blocks */
static void
_process_sct_vaux (struct dvsend_param *dvsend_param,
                   u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  if (dvsend_param->frame_drop > 1 &&
      (frame % dvsend_param->frame_drop) != 0) {
    /* drop this frame */
  }
  else {
    _add_dvdif_to_outbuf(dvsend_param, difblock);
  }
}

/* This will process DV AUIDO DIF blocks */
static void
_process_sct_audio (struct dvsend_param *dvsend_param,
                    u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  if (dvsend_param->flags & NO_AUDIO_STREAM) {
    /* do not include audio data */
  } else if (dvsend_param->flags & SEPARATED_AUDIO_STREAM) {
    _add_dvdif_to_audio_outbuf(dvsend_param, difblock);
  } else {
    _add_dvdif_to_outbuf(dvsend_param, difblock);
  }

  if (dvsend_param->audio_redundancy > 0) {
    _add_dvdif_to_r_outbuf(dvsend_param, difblock);
  }
}

/* This will process DV VIDEO DIF blocks */
static void
_process_sct_video (struct dvsend_param *dvsend_param,
                    u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  if (dvsend_param->flags & NO_VIDEO_STREAM) {
    /* do not include video data */
  } else if (dvsend_param->frame_drop > 1 &&
      (frame % dvsend_param->frame_drop) != 0) {
    /* drop this frame */
  } else {
    _add_dvdif_to_outbuf(dvsend_param, difblock);
  }

  /* This DIF block is end of frame */
  if (dvsend_param->format == DV_FORMAT_NTSC) {
    if (dseq == (DIFSEQ_NUM_NTSC - 1) && dbn == 134) {
      _dvdif_flush(dvsend_param);
    }
  } else if (dvsend_param->format == DV_FORMAT_PAL) {
    if (dseq == (DIFSEQ_NUM_PAL - 1) && dbn == 134) {
      _dvdif_flush(dvsend_param);
    }
  }
}

static void
_add_dvdif_to_outbuf(struct dvsend_param *dvsend_param,
                     u_int32_t *difblock)
{
  static int n;

  memcpy(&outbuf[(outbuf_len + sizeof(rtp_hdr_t))/4], difblock, 80);
  outbuf_len += 80;

  if (outbuf_len > 80*(dvsend_param->DIFblocks_in_pkt - 1)) {
    n = send_pkt(dvsend_param, outbuf, outbuf_len + sizeof(rtp_hdr_t));

    outbuf_len = 0;
    rtphdr->seq = htons(ntohs(rtphdr->seq)+1);
  }

  return;
}

static void
_add_dvdif_to_r_outbuf(struct dvsend_param *dvsend_param,
                       u_int32_t *difblock)
{
  static int n;
  int i;

  memcpy(&r_outbuf[(r_outbuf_len + sizeof(rtp_hdr_t))/4], difblock, 80);
  r_outbuf_len += 80;

  if (r_outbuf_len > 80*(dvsend_param->DIFblocks_in_pkt - 1)) {
    for (i=0; i<dvsend_param->audio_redundancy; i++) {
      memcpy(r_outbuf, outbuf, sizeof(rtp_hdr_t));
      n = send_pkt(dvsend_param, r_outbuf, r_outbuf_len + sizeof(rtp_hdr_t));
      rtphdr->seq = htons(ntohs(rtphdr->seq)+1);
    }

    r_outbuf_len = 0;
  }

  return;
}

static void
_add_dvdif_to_audio_outbuf(struct dvsend_param *dvsend_param,
                           u_int32_t *difblock)
{
  static int n;

  memcpy(&audiobuf[(audiobuf_len + sizeof(rtp_hdr_t))/4], difblock, 80);
  audiobuf_len += 80;

  if (audiobuf_len > 80*(dvsend_param->DIFblocks_in_pkt - 1)) {
    n = send_audio_pkt(dvsend_param,
                       audiobuf,
                       audiobuf_len + sizeof(rtp_hdr_t));

    if (n < 0) {
      perror("sendto audio");
    }

    audiobuf_len = 0;
    audio_rtphdr->seq = htons(ntohs(audio_rtphdr->seq)+1);
  }

  return;
}

static void
_dvdif_flush(struct dvsend_param *dvsend_param)
{
  if (outbuf_len > 0) {
    /* Send out the RTP packet.                         */
    /* In RTP, the data from different frame can not be */
    /* included in a single packet.                     */

    send_pkt(dvsend_param, outbuf, outbuf_len + sizeof(rtp_hdr_t));
    outbuf_len = 0;
  }

  /* RTP sequence number will increase every time a RTP packet is sent */
  rtphdr->seq = htons(ntohs(rtphdr->seq)+1);

  /* RTP timestamp will increase every by frame.                     */
  /* since DV/RTP payload uses 90khz timestamp and NTSC is 29.97 hz, */
  /* the timestamp will simply increase by 3003.                     */
  rtphdr->ts = htonl(ntohl(rtphdr->ts)+3003);

  /* The variable [frame] counts the number of frames read */
  /* from the IEEE 1394 device                             */
  frame++;

  /* audio will be sent to a different UDP port */
  /* when it needs to be sent separetly.        */
  if (dvsend_param->flags & SEPARATED_AUDIO_STREAM) {
    if (audiobuf_len > 0) {
      send_audio_pkt(dvsend_param, audiobuf, audiobuf_len + sizeof(rtp_hdr_t));
      audiobuf_len = 0;
      audio_rtphdr->ts = htonl(ntohl(audio_rtphdr->ts)+3003);
      audio_rtphdr->seq = htons(ntohs(audio_rtphdr->seq)+1);
    }
  }

  return;
}
