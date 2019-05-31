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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>

#include <dvts.h>

#include "shm.h"

int
prepare_shared_memory (struct dvrecv_param *dvrecv_param)
{
  int video_shm_size, audio_shm_size;
  int video_shmid, audio_shmid;

  video_shm_size = dvrecv_param->frames_in_buffer *
		(sizeof(struct shm_framebuf) +
		 sizeof(struct static_video_frame));
  audio_shm_size = dvrecv_param->frames_in_buffer *
		(sizeof(struct audio_shm_framebuf));

  if ((video_shmid = shmget(IPC_PRIVATE, video_shm_size, 0600)) < 0) {
    perror("shmget");
    return(-1);
  }

  /* AUDIO */
  if ((audio_shmid = shmget(IPC_PRIVATE, audio_shm_size, 0600)) < 0) {
    perror("shmget audio");
    return(-1);
  }

  /* remember the shmid */
  dvrecv_param->video_shmid = video_shmid;
  dvrecv_param->audio_shmid = audio_shmid;

  return(1);
}

int
attach_shared_memory (struct dvrecv_param *dvrecv_param)
{
  u_int32_t *video_buf, *audio_buf;

  int i;
  struct shm_frame *frame = NULL;
  struct shm_frame *frame_last = NULL;
  struct audio_shm_frame *audio_frame = NULL;
  struct audio_shm_frame *audio_frame_last = NULL;

  if ((video_buf = shmat(dvrecv_param->video_shmid, 0, 0)) == (void *)-1) {
    perror("shmat");
    return(-1);
  }

  if ((audio_buf = shmat(dvrecv_param->audio_shmid, 0, 0)) == (void *)-1) {
    perror("shmat audio");
    return(-1);
  }

  dvrecv_param->video_buf = video_buf;
  dvrecv_param->audio_buf = audio_buf;

  /***** static video *****/
  dvrecv_param->static_video_frame = (struct static_video_frame *)video_buf;
  dvrecv_param->static_video_frame->flag = 0;
  video_buf += sizeof(struct static_video_frame) / 4;

  /***** video *****/
  for (i=0; i<dvrecv_param->frames_in_buffer; i++) {
    frame = (struct shm_frame *)malloc(sizeof(struct shm_frame));
    memset(frame, 0, sizeof(struct shm_frame));

    frame->framebuf = (struct shm_framebuf *)((char *)video_buf + i*sizeof(struct shm_framebuf));

    frame->framebuf->lock = DVFRAME_READY;

    if (frame_last == NULL) {
      dvrecv_param->video_framebuf = frame;
    }
    else {
      frame_last->next = frame;
      frame_last->next->prev = frame_last;
    }
    frame_last = frame;
  }
  frame->next = dvrecv_param->video_framebuf;
  frame->next->prev = frame;

  /***** audio *****/
  for (i=0; i<dvrecv_param->frames_in_buffer; i++) {
    audio_frame = (struct audio_shm_frame *)malloc(sizeof(struct shm_frame));
    memset(audio_frame, 0, sizeof(struct shm_frame));

    audio_frame->framebuf = (struct audio_shm_framebuf *)((char *)audio_buf + i*sizeof(struct audio_shm_framebuf));

    audio_frame->framebuf->lock = DVFRAME_READY;

    if (dvrecv_param->audio_framebuf == NULL) {
      dvrecv_param->audio_framebuf = audio_frame;
    } else {
      audio_frame_last->next = audio_frame;
    }

    audio_frame_last = audio_frame;
  }
  audio_frame_last->next = dvrecv_param->audio_framebuf;

  return(1);
}

void
free_shared_memory (struct dvrecv_param *dvrecv_param)
{
  struct shmid_ds shmid_ds;

  if (shmdt(dvrecv_param->video_buf) < 0) {
    perror("shmdt");
  }
  if (shmctl(dvrecv_param->video_shmid, IPC_RMID, &shmid_ds) < 0) {
    perror("shmctl IPC_RMID");
  }

  if (shmdt(dvrecv_param->audio_buf) < 0) {
    perror("shmdt audio");
  }
  if (shmctl(dvrecv_param->audio_shmid, IPC_RMID, &shmid_ds) < 0) {
    perror("shmctl IPC_RMID");
  }
}
