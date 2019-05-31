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

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>

#include <dvts.h>

#include "param.h"
#include "write.h"
#include "ieee1394.h"
#include "shm.h"

#define		USE_SILENT_AUDIO	0x00000001

/* frame size of PAL is larger than NTSC */
u_int32_t dvframe_buf[144000/4];

static void _fill_buffer __P((struct dvrecv_param *));

static u_long silent_audio[DIFSEQ_NUM_PAL*9*80/4];
static u_long same_audio_count = 0;

void
ieee1394dv_write_loop (struct dvrecv_param *dvrecv_param)
{
  u_char seq = 0;
  u_char dseq, sct, dbn;
  int dvframe_size = 0;

  if (dvrecv_param->format == DV_FORMAT_PAL) {
    dvframe_size = 144000;
  } else if (dvrecv_param->format == DV_FORMAT_NTSC) {
    dvframe_size = 120000;
  }

  /* prepare DV DIF block without any sound */
  memset(silent_audio, 0, sizeof(silent_audio));

  sct = SCT_AUDIO;
  for (dseq=0; dseq<dvrecv_param->maxdifseq; dseq++) {
    for (dbn=0; dbn<9; dbn++) {
      silent_audio[(dseq*9*80 + dbn*80)/4] =
         htonl((sct & 0x7) << 29 |
           (seq & 0xf) << 24 |
             (dseq & 0xf) << 20 |
               (dbn & 0xff) << 8);
    }
  }

  while (1) {
    _fill_buffer(dvrecv_param);
    write(dvrecv_param->ieee1394dv.fd1394, dvframe_buf, dvframe_size);
  }
}

static void
_fill_buffer (struct dvrecv_param *dvrecv_param)
{
  int i;
  u_char dseq;
  u_long silent_audio_flag = 0;
  struct ieee1394dv *ieee1394dv;

  ieee1394dv = &dvrecv_param->ieee1394dv;

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

  memcpy(dvframe_buf,
         &dvrecv_param->video_framebuf->framebuf->dvframe.pkt,
         sizeof(dvrecv_param->video_framebuf->framebuf->dvframe.pkt));

  dvrecv_param->video_framebuf->framebuf->lock = DVFRAME_READY;
  dvrecv_param->audio_framebuf->framebuf->lock = DVFRAME_READY;
}
