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

#include "param.h"
#include "write.h"
#include "ieee1394.h"
#include "shm.h"

#include <difblock.h>

/* blocks used for [u_long outbuf] */
#define		NBLOCKS		300

#define		USE_SILENT_AUDIO	0x00000001

extern struct dvrecv_param dvrecv_param;

/* frame size for PAL is larger than NTSC */
static u_long silent_audio[DIFSEQ_NUM_PAL*9*80/4];

static void
_prepare_write()
{
  u_char seq = 0;
  u_char dseq, sct, dbn;

  /* prepare DV DIF block without any sound */
  memset(silent_audio, 0, sizeof(silent_audio));

  sct = SCT_AUDIO;
  for (dseq=0; dseq<dvrecv_param.maxdifseq; dseq++) {
    for (dbn=0; dbn<9; dbn++) {
      silent_audio[(dseq*9*80 + dbn*80)/4] = htonl((sct & 0x7) << 29 | (seq & 0xf) << 24 | (dseq & 0xf) << 20 | (dbn & 0xff) << 8);
    }
  }
}

static OSStatus
_write_frame(IDHGenericEvent *eventRecord, void *userData)
{
  OSErr result = noErr;
  IDHParameterBlock *pb;
  int dseq;
  int i;
  int silent_audio_flag = 0;

  static struct shm_frame *shm_frame;
  static struct audio_shm_frame *audio_shm_frame;

  shm_frame = dvrecv_param.video_framebuf;
  audio_shm_frame = dvrecv_param.audio_framebuf;

  if (shm_frame->next->framebuf->lock == DVFRAME_DATA_READY) {
    dvrecv_param.video_framebuf = shm_frame->next;
    shm_frame->framebuf->lock = DVFRAME_WRITING_IEEE1394;
  }
  if (audio_shm_frame->next->framebuf->lock == DVFRAME_DATA_READY) {
    dvrecv_param.audio_framebuf = audio_shm_frame->next;
    audio_shm_frame->framebuf->lock = DVFRAME_WRITING_IEEE1394;
  } else {
    silent_audio_flag = USE_SILENT_AUDIO;
  }

  pb = (IDHParameterBlock *)eventRecord;

  /* prepare audio for DV frame */
  if (silent_audio_flag == USE_SILENT_AUDIO) {
    for (dseq=0; dseq<dvrecv_param.maxdifseq; dseq++) {
      for (i=0; i<9; i++) {
        memcpy(&shm_frame->framebuf->dvframe.pkt[dseq][0][0] + (6 + i*16)*80/4,
               &silent_audio[dseq*9*(80/4) + i*(80/4)],
               80);
      }
    }
  } else {
    for (dseq=0; dseq<dvrecv_param.maxdifseq; dseq++) {
      for (i=0; i<9; i++) {
        memcpy(&shm_frame->framebuf->dvframe.pkt[dseq][0][0] + (6 + i*16)*80/4,
               &audio_shm_frame->framebuf->dseq[dseq].dbn[i],
               80);
      }
    }
  }

  memcpy((char *)pb->buffer, (char *)&shm_frame->framebuf->dvframe, 120000);

  if (dvrecv_param.format == DV_FORMAT_NTSC) {
    pb->requestedCount = 120000;
  } else if (dvrecv_param.format == DV_FORMAT_PAL) {
    pb->requestedCount = 144000;
  } else {
    return(-1);
  }

  pb->actualCount = 0;
  pb->completionProc = _write_frame;

  result = IDHWrite(dvrecv_param.ieee1394dv.theInst, pb);
  if (result != noErr) {
    printf("IDHWrite error 0x%x\n", result);
  }

  shm_frame->framebuf->lock = DVFRAME_READY;
  audio_shm_frame->framebuf->lock = DVFRAME_READY;

  return(result);
}

void
ieee1394dv_write_loop (struct dvrecv_param *dv_param)
{
  IDHParameterBlock isochParamBlock;
  static Ptr myBuffer;
  OSErr err;

  _prepare_write();

  if (dv_param->format == DV_FORMAT_NTSC) {
    myBuffer = NewPtr(120000);
    isochParamBlock.requestedCount = 120000;
  } else if (dv_param->format == DV_FORMAT_PAL) {
    myBuffer = NewPtr(144000);
    isochParamBlock.requestedCount = 144000;
  } else {
    return;
  }

  isochParamBlock.buffer = myBuffer;
  isochParamBlock.actualCount = 0;
  isochParamBlock.completionProc = _write_frame;

  err = IDHWrite(dv_param->ieee1394dv.theInst, &isochParamBlock);
  if (err != noErr) {
    printf("IDHWrite error : ieee1394dv_write_loop\n");
    return;
  }

  while (1) {
    sleep(10);
  }
}
