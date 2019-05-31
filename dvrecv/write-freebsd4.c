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

#include <sys/types.h>
#include <sys/socket.h>

#include <dvts.h>

#include "write.h"
#include "ieee1394.h"
#include "shm.h"

/* blocks used for [u_int32_t outbuf] */
#define		NBLOCKS		400

#define		USE_SILENT_AUDIO	0x00000001

int freebsd4_write_frame_to_ieee1394 __P((struct dvrecv_param *, u_int32_t *, int));

void
ieee1394dv_write_loop (struct dvrecv_param *dvrecv_param)
{
  u_long frac = dvrecv_param->ieee1394dv.frac;
  u_long mod = dvrecv_param->ieee1394dv.mod;

  u_long silent_audio[dvrecv_param->maxdifseq*9*80/4];
  u_long silent_audio_flag = 0;
  u_long same_audio_count = 0;

  u_int32_t outbuf[512/4 * NBLOCKS];
  u_char seq = 0;
  u_char dseq, sct, dbn;
  int i;
  int ieee1394_pkt_count = 0;
  int count;

  /* prepare DV DIF block without any sound */
  memset(silent_audio, 0, sizeof(silent_audio));

  sct = SCT_AUDIO;
  for (dseq=0; dseq<dvrecv_param->maxdifseq; dseq++) {
    for (dbn=0; dbn<9; dbn++) {
      silent_audio[(dseq*9*80 + dbn*80)/4] = htonl((sct & 0x7) << 29 | (seq & 0xf) << 24 | (dseq & 0xf) << 20 | (dbn & 0xff) << 8);
    }
  }

  memset(outbuf, 0, sizeof(outbuf));

  for (i=0; i<NBLOCKS; i++) {
    outbuf[i * (512/4) + 0] = htonl(488 << 16 | 0x00007fa0);
    outbuf[i * (512/4) + 1] = htonl(0x00780000);

    if (dvrecv_param->format == DV_FORMAT_NTSC) {
      outbuf[i * (512/4) + 2] = htonl(0x8000ffff);
    } else if (dvrecv_param->format == DV_FORMAT_PAL) {
      outbuf[i * (512/4) + 2] = htonl(0x8080ffff);
    } else {
      return;
    }
  }

  while (1) {
    ieee1394_pkt_count = 0;

    if (dvrecv_param->video_framebuf->next->framebuf->lock == DVFRAME_DATA_READY) {
      dvrecv_param->video_framebuf = dvrecv_param->video_framebuf->next;
      dvrecv_param->video_framebuf->framebuf->lock = DVFRAME_WRITING_IEEE1394;
    }

    if (dvrecv_param->audio_framebuf->next->framebuf->lock == DVFRAME_DATA_READY) {
      same_audio_count = 0;
      silent_audio_flag = 0;
      dvrecv_param->audio_framebuf = dvrecv_param->audio_framebuf->next;
      dvrecv_param->audio_framebuf->framebuf->lock = DVFRAME_WRITING_IEEE1394;
    } else {
      same_audio_count++;
      if (same_audio_count > 5) {
        if (same_audio_count == 6) {
          printf("MUTE AUDIO\n");
        }
        same_audio_count = 10;
        silent_audio_flag = USE_SILENT_AUDIO;
      }
    }

    for (dseq=0; dseq<dvrecv_param->maxdifseq; dseq++) {
      if (silent_audio_flag) {
        for (i=0; i<9; i++) {
          memcpy(&dvrecv_param->video_framebuf->framebuf->dvframe.pkt[dseq][0][0] + (6 + i*16)*80/4,
                 &silent_audio[dseq*9*(80/4) + i*(80/4)],
                 80);
        }
      } else {
        for (i=0; i<9; i++) {
          memcpy(&dvrecv_param->video_framebuf->framebuf->dvframe.pkt[dseq][0][0] + (6 + i*16)*80/4,
                 &dvrecv_param->audio_framebuf->framebuf->dseq[dseq].dbn[i],
                 80);
        }
      }
    }

    for (dseq=0; dseq<dvrecv_param->maxdifseq; dseq++) {
      for (count=0; count<IEEE1394_PKT_NUM; count++) {

        memcpy(&outbuf[ieee1394_pkt_count*(512/4) + 3],
               &dvrecv_param->video_framebuf->framebuf->dvframe.pkt[dseq][0][0] + count*480/4,
               480);

        outbuf[ieee1394_pkt_count*(512/4)] = htonl(488 << 16 | 0x00007fa0);

        ieee1394_pkt_count++;

        if (ieee1394_pkt_count % frac == mod) {
          outbuf[ieee1394_pkt_count*(512/4)] = htonl(8 << 16 | 0x00007fa0);
          ieee1394_pkt_count++;
        }
      } /* end of for(count=0; count<IEEE1394_PKT_NUM; count++) */
    } /* end of for(dseq=0; dseq<dvrecv_param->maxdifseq; dseq++) */

    if (freebsd4_write_frame_to_ieee1394(dvrecv_param,
                                         outbuf,
                                         ieee1394_pkt_count*512) < 0) {
      break;
    }

    dvrecv_param->video_framebuf->framebuf->lock = DVFRAME_READY;
    dvrecv_param->audio_framebuf->framebuf->lock = DVFRAME_READY;
  }
}
