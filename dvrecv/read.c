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
#include <unistd.h>

#include <sys/errno.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

#include <dvts.h>

#include <rtpvar.h>

#include "read.h"
#include "shm.h"
#include "rtcp.h"
#include "flags.h"

#define MAX_PACKET_LENGTH	1500


static void _process_sct_header  __P((struct dvrecv_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_subcode __P((struct dvrecv_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_vaux    __P((struct dvrecv_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_audio   __P((struct dvrecv_param *,
                                      u_char, u_char, u_char, u_int32_t *));
static void _process_sct_video   __P((struct dvrecv_param *,
                                      u_char, u_char, u_char, u_int32_t *));

void
dvrtp_read_loop (struct dvrecv_param *dvrecv_param)
{
  struct sockaddr_storage from;
  int length = sizeof(from);
  int n;
  int i;

  int j, k;
  static struct shm_frame *shm_frame;

  u_char dbn, dseq, seq, sct;

  u_int32_t recvbuf[MAX_PACKET_LENGTH];
  rtp_hdr_t *rtphdr = (rtp_hdr_t *)&recvbuf[0];

  u_long rtp_ts_prev = 0;
  u_long rtp_audio_ts_prev = 0;

  /* These are used to calculate packet loss */
  u_long npkts = 0;
  u_long pkt_loss = 0;
  u_long rtp_seq_prev = 0;

  int ret;
  fd_set readfds;

  void (*sct_func[5]) __P((struct dvrecv_param *,
                           u_char, u_char, u_char, u_int32_t *));

  sct_func[0] = _process_sct_header;
  sct_func[1] = _process_sct_subcode;
  sct_func[2] = _process_sct_vaux;
  sct_func[3] = _process_sct_audio;
  sct_func[4] = _process_sct_video;

  while (1) {
    memcpy(&readfds, &dvrecv_param->fds, sizeof(readfds));
    ret = select(dvrecv_param->maxfds, &readfds, NULL, NULL, NULL);
    if (ret < 0) {
      perror("select");
      return;
    }

    if (FD_ISSET(dvrecv_param->soc, &readfds)) {
      if ((n = recvfrom(dvrecv_param->soc,
                        (char *)&recvbuf, sizeof(recvbuf),
                        0,
                        (struct sockaddr *)&from, &length)) < 0) {
        perror("recvfrom");
        return;
      }
    } else if (dvrecv_param->flags & SEPARATED_AUDIO_STREAM) {
      if ((n = recvfrom(dvrecv_param->audio_soc,
                        (char *)&recvbuf, sizeof(recvbuf),
                        0,
                        (struct sockaddr *)&from, &length)) < 0) {
        perror("recvfrom audio soc");
        return;
      }
    } else {
      if (process_rtcp(dvrecv_param) < 0) {
        return;
      } else {
        continue;
      }
    }

    /* counting number of packets */
    /* THIS IS FOR SHOWING NUMBER OF PACKET LOSS LOCALLY */
    npkts++;

    /* counting number of packets */
    /* THIS IS FOR RTCP */
    dvrecv_param->pkt_count++;

    /* Network byte order to host byte order */
    rtphdr->seq  = ntohs(rtphdr->seq);
    rtphdr->ts   = ntohl(rtphdr->ts);
    rtphdr->ssrc = ntohl(rtphdr->ssrc);

    if (rtp_seq_prev != 0 && rtp_seq_prev < rtphdr->seq) {
      /* calculation of packet loss number */
      /* THIS IS FOR SHOWING NUMBER OF PACKET LOSS LOCALLY */
      pkt_loss += rtphdr->seq - rtp_seq_prev - 1;

      /* calculation of packet loss number */
      /* THIS IS FOR RTCP */
      dvrecv_param->pkt_loss_sum += rtphdr->seq - rtp_seq_prev - 1;
    }
    rtp_seq_prev = rtphdr->seq;

    if (npkts >= dvrecv_param->pkt_loss_count &&
        dvrecv_param->flags & SHOW_PACKETLOSS) {
      printf("npkts : %ld,\tpkt_loss %ld\n", npkts, pkt_loss);
      npkts = 0;
      pkt_loss = 0;
    }

    for (i=0; i<(n - sizeof(rtp_hdr_t))/80; i++) {
      dbn  = (ntohl(recvbuf[(sizeof(rtp_hdr_t)+80*i)/4]) >> 8) & 0xff;
      dseq = (ntohl(recvbuf[(sizeof(rtp_hdr_t)+80*i)/4]) >> 20) & 0xf;
      seq  = (ntohl(recvbuf[(sizeof(rtp_hdr_t)+80*i)/4]) >> 24) & 0xf;
      sct  = (ntohl(recvbuf[(sizeof(rtp_hdr_t)+80*i)/4]) >> 29) & 0x7;

      if (sct > 4) {
        printf("INVALID sct : %d\n", sct);
        continue;
      }

      if (sct == SCT_AUDIO) {
        if (rtphdr->ts != rtp_audio_ts_prev) {
          dvrecv_param->audio_framebuf->framebuf->lock = DVFRAME_DATA_READY;
          dvrecv_param->audio_framebuf = dvrecv_param->audio_framebuf->next;

          rtp_audio_ts_prev = rtphdr->ts;
        }
      }
      else if (rtphdr->ts != rtp_ts_prev) {
        shm_frame = dvrecv_param->video_framebuf;
        for (j = 0; j < dvrecv_param->maxdifseq; j++) {
          for (k = 0; k < 135; k++) {
            if (!shm_frame->framebuf->video_recv_flag[j][k]) {
              memcpy(&shm_frame->framebuf->dvframe.pkt[j][0][0] + (6+1+k+k/15)*80/4,
                     &shm_frame->prev->framebuf->dvframe.pkt[j][0][0] + (6+1+k+k/15)*80/4,
                     80);
            }
          }
        }
        dvrecv_param->video_framebuf->framebuf->lock = DVFRAME_DATA_READY;
        dvrecv_param->video_framebuf = dvrecv_param->video_framebuf->next;

        memset(dvrecv_param->video_framebuf->framebuf->video_recv_flag,
               0,
               sizeof(dvrecv_param->video_framebuf->framebuf->video_recv_flag));

        rtp_ts_prev = rtphdr->ts;
      }

      (void)(*sct_func[sct])(dvrecv_param,
                             seq, dseq, dbn,
                             &recvbuf[(sizeof(rtp_hdr_t) + 80*i)/4]);
    }
  }
}

/***********************/

static void
_process_sct_header (struct dvrecv_param *dvrecv_param,
                     u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  static struct shm_frame *shm_frame;

  shm_frame = dvrecv_param->video_framebuf;

  while (1) {
    if (!(shm_frame->framebuf->lock == DVFRAME_WRITING_DATA)) {
      shm_frame->framebuf->lock = DVFRAME_WRITING_DATA;
      break;
    } else if (shm_frame->framebuf->lock & DVFRAME_WRITING_IEEE1394) {
      dvrecv_param->video_framebuf = shm_frame->next;
    } else {
      break;
    }
  }

  memcpy(&shm_frame->framebuf->dvframe.pkt[dseq][0][0],
         difblock, 80);
}

static void
_process_sct_subcode (struct dvrecv_param *dvrecv_param,
                      u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  static struct shm_frame *shm_frame;

  shm_frame = dvrecv_param->video_framebuf;

  while (1) {
    if (!(shm_frame->framebuf->lock & DVFRAME_WRITING_DATA)) {
      shm_frame->framebuf->lock = DVFRAME_WRITING_DATA;
      break;
    } else if (shm_frame->framebuf->lock & DVFRAME_WRITING_IEEE1394) {
      dvrecv_param->video_framebuf = shm_frame->next;
    } else {
      break;
    }
  }

  memcpy(&shm_frame->framebuf->dvframe.pkt[dseq][0][(dbn + 1)*80/4],
         difblock, 80);
}

static void
_process_sct_vaux (struct dvrecv_param *dvrecv_param,
                   u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  static struct shm_frame *shm_frame;

  shm_frame = dvrecv_param->video_framebuf;

  while (1) {
    if (!(shm_frame->framebuf->lock & DVFRAME_WRITING_DATA)) {
      shm_frame->framebuf->lock = DVFRAME_WRITING_DATA;
      break;
    } else if (shm_frame->framebuf->lock & DVFRAME_WRITING_IEEE1394) {
      dvrecv_param->video_framebuf = shm_frame->next;
    } else {
      break;
    }
  }

  memcpy(&shm_frame->framebuf->dvframe.pkt[dseq][0][(3+dbn)*80/4],
         difblock, 80);
}

static void
_process_sct_audio (struct dvrecv_param *dvrecv_param,
                    u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  static struct audio_shm_frame *audio_shm_frame;

  audio_shm_frame = dvrecv_param->audio_framebuf;

  while (1) {
    if (!(audio_shm_frame->framebuf->lock & DVFRAME_WRITING_DATA)) {
      audio_shm_frame->framebuf->lock = DVFRAME_WRITING_DATA;
      break;
    } else if (audio_shm_frame->framebuf->lock & DVFRAME_WRITING_IEEE1394) {
      dvrecv_param->audio_framebuf = audio_shm_frame->next;
    } else {
      break;
    }
  }

  memcpy(&audio_shm_frame->framebuf->dseq[dseq].dbn[dbn],
         difblock, 80);
}

static void
_process_sct_video (struct dvrecv_param *dvrecv_param,
                    u_char seq, u_char dseq, u_char dbn, u_int32_t *difblock)
{
  static struct shm_frame *shm_frame;

  shm_frame = dvrecv_param->video_framebuf;

  while (1) {
    if (!(shm_frame->framebuf->lock & DVFRAME_WRITING_DATA)) {
      shm_frame->framebuf->lock = DVFRAME_WRITING_DATA;
      break;
    } else if (shm_frame->framebuf->lock & DVFRAME_WRITING_IEEE1394) {
      dvrecv_param->video_framebuf = shm_frame->next;
    } else {
      break;
    }
  }

  memcpy(&shm_frame->framebuf->dvframe.pkt[dseq][0][0] + (6+1+dbn+dbn/15)*80/4,
         difblock, 80);
  shm_frame->framebuf->video_recv_flag[dseq][dbn] = 1;
}
