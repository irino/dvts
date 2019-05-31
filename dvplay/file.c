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
#include <unistd.h>

#include <sys/types.h>
#include <netinet/in.h>

#include <dvts.h>

#include "file.h"

int
open_dv_data_file (struct dvplay_param *dvplay_param)
{
  FILE *fp;
  char str[128];

  /* read from STDIN if filename is NULL */
  if (dvplay_param->filename == NULL) {
    fp = stdin;
    dvplay_param->flags |= DVPLAY_FLAG_STDIN;
  }

  else if ((fp = fopen(dvplay_param->filename, "r")) == NULL) {
    memset(str, 0, sizeof(str));
    snprintf(str, sizeof(str), "fopen %s", dvplay_param->filename);
    perror(str);
    return(-1);
  }

  dvplay_param->fp = fp;

  return(1);
}

void
close_dv_data_file (struct dvplay_param *dvplay_param)
{
  fclose(dvplay_param->fp);
}

int
read_dv_data_from_file (struct dvplay_param *dvplay_param)
{
  int n;
  int n_read = 0;
  char *ptr;

  struct dv_sct scthdr;
  struct dv_difblock dvdif;

  ptr = (char *)&dvdif;
  if (dvplay_param->flags & DVPLAY_FLAG_STDIN) {
    /* need to read until 80 bytes */
    while (1) {
      n_read = 0;
      while (n_read < sizeof(struct dv_difblock)) {
        n = read(fileno(dvplay_param->fp),
                 ptr + n_read,
                 sizeof(struct dv_difblock) - n_read);
        if (n < 1) {
          return(-1);
        }
        n_read += n;
      }

      scthdr.dbn  = (ntohl(dvdif.data[0]) >> 8) & 0xff;
      scthdr.dseq = (ntohl(dvdif.data[0]) >> 20) & 0xff;
      scthdr.seq  = (ntohl(dvdif.data[0]) >> 24) & 0xff;
      scthdr.sct  = (ntohl(dvdif.data[0]) >> 29) & 0x7;

      switch (scthdr.sct) {
        case SCT_HEADER:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][0],
                 &dvdif, 80);
          break;

        case SCT_SUBCODE:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][(scthdr.dbn+1)*80/4],
                 &dvdif, 80);
          break;

        case SCT_VAUX:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][(scthdr.dbn+3)*80/4],
                 &dvdif, 80);
          break;

        case SCT_AUDIO:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][0] +
                   (scthdr.dbn*16+6)*80/4,
                 &dvdif, 80);
          break;

        case SCT_VIDEO:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][0] +
                  (scthdr.dbn+6+1+scthdr.dbn/15)*80/4,
                 &dvdif, 80);
          if (scthdr.dseq == 9 && scthdr.dbn == 134) {
            return(1);
          }
          break;

        default:
          printf("INVALID FILE FORMAT!!!\n");
          printf("SCT : %d\n", scthdr.sct);
          return(-1);
      }
    }
  } else {
    while ((n = read(fileno(dvplay_param->fp),
                     (char *)&dvdif, sizeof(struct dv_difblock))) > 0) {
      scthdr.dbn  = (ntohl(dvdif.data[0]) >> 8) & 0xff;
      scthdr.dseq = (ntohl(dvdif.data[0]) >> 20) & 0xff;
      scthdr.seq  = (ntohl(dvdif.data[0]) >> 24) & 0xff;
      scthdr.sct  = (ntohl(dvdif.data[0]) >> 29) & 0x7;

      switch (scthdr.sct) {
        case SCT_HEADER:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][0],
                 &dvdif, 80);
          break;

        case SCT_SUBCODE:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][(scthdr.dbn+1)*80/4],
                 &dvdif, 80);
          break;

        case SCT_VAUX:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][(scthdr.dbn+3)*80/4],
                 &dvdif, 80);
          break;

        case SCT_AUDIO:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][0] +
                  (scthdr.dbn*16+6)*80/4,
                 &dvdif, 80);
          break;

        case SCT_VIDEO:
          memcpy(&dvplay_param->dvframe.pkt[scthdr.dseq][0][0] +
                  (scthdr.dbn+6+1+scthdr.dbn/15)*80/4,
                 &dvdif, 80);
          if (scthdr.dseq == 9 && scthdr.dbn == 134) {
            return(1);
          }
          break;

        default:
          printf("INVALID FILE FORMAT!!!\n");
          printf("SCT : %d\n", scthdr.sct);
          return(-1);
      }
    }
  }

  return(-1);
}
