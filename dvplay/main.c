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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>

#include <dvts.h>

#include "param.h"
#include "file.h"
#include "info.h"

#include "ieee1394.h"


struct dvplay_param dvplay_param;

int
main(int argc, char *argv[])
{
  extern char *optarg;
  extern int optind;
  int op;

  memset(&dvplay_param, 0, sizeof(dvplay_param));

  dvplay_param.filename = NULL;
  dvplay_param.channel  = 63;

#ifdef	FREEBSD_4
  dvplay_param.ieee1394dev = "/dev/dv0";
  dvplay_param.frac = 17;
  dvplay_param.mod = 0;
#endif	/* FREEBSD_4 */

#ifdef	LINUX
  dvplay_param.devname = "/dev/video1394";
#endif	/* LINUX */

  dvplay_param.format = DV_FORMAT_NTSC;
  dvplay_param.maxdifseq = DIFSEQ_NUM_NTSC;

  while ((op = getopt(argc, argv, "vD:F:p")) > 0) {
    switch (op) {
      case 'v':
        show_version();
        exit(1);
        break;

#ifdef	FREEBSD_4
      case 'D':
        dvplay_param.ieee1394dev = optarg;
        break;
#endif	/* FREEBSD_4 */

      case 'F':
        dvplay_param.filename = optarg;
        break;

      case 'p':
        dvplay_param.format = DV_FORMAT_PAL;
        dvplay_param.maxdifseq = DIFSEQ_NUM_PAL;
        break;

      default:
        show_usage(argv[0]);
        exit(0);
        break;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc > 0) {
    dvplay_param.filename = argv[0];
  }

  if (open_dv_data_file(&dvplay_param) < 0) {
    printf("failed to open file : %s\n", dvplay_param.filename);
    return(-1);
  }

  printf("reading DV data from file : %s\n", dvplay_param.filename);

  if (open_ieee1394(&dvplay_param) < 0) {
    printf("failed to open ieee1394 device\n");
    return(-1);
  }

  while (read_dv_data_from_file(&dvplay_param) > 0) {
    write_ieee1394(&dvplay_param);
  }

  close_ieee1394(&dvplay_param);
  close_dv_data_file(&dvplay_param);

  return(1);
}
