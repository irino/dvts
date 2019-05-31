/*
  * Copyright (c) 1999-2003 WIDE Project
  * All rights reserved.
  *
  * Author : Akimichi Ogawa (akimichi@sfc.wide.ad.jp)
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  * 3. All advertising materials mentioning features or use of this software
  *    must display the following acknowledgement:
  *      This product includes software developed by Akimichi OGAWA
  * 4. The name of the author may not be used to endorse or promote products
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
  *
*/

#ifdef HAVE_CONFIG_H
#include <dvts-config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef	LINUX
#include <getopt.h>
#endif	/* LINUX */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>

#include <dvts.h>

#include "param.h"
#include "ieee1394.h"

#define RECV_CHANNEL	0x3f


int
main(int argc, char *argv[])
{
  struct dvsave_param dvsave_param;

  char        *path = NULL;

  extern char *optarg;
  extern int   optind;
  int          op;

  memset(&dvsave_param, 0, sizeof(dvsave_param));

  dvsave_param.channel = RECV_CHANNEL;

  dvsave_param.format = DV_FORMAT_NTSC;

#ifdef	FREEBSD_4
  dvsave_param.ifname = "ohci0";
#endif /* FREEBSD_4 */

#ifdef	NETBSD
  dvsave_param.devname = "/dev/fwiso0";
#endif	/* NETBSD */

#ifdef	LINUX
  dvsave_param.devname = "/dev/dv1394";
#endif	/* LINUX */

  while ((op = getopt(argc, argv, "f:o:i:c:F:D:p")) > 0) {
    switch(op) {
      case 'f':
        dvsave_param.frame_max = atoi(optarg);
        if (dvsave_param.frame_max == 0 ||
            dvsave_param.frame_max == 0xffffffff) {
          fprintf(stderr,
                  "wrong number of frames : %s, %d\n",
                  optarg, dvsave_param.frame_max);
          return(-1);
        }
        printf("saving %d DV frames\n", dvsave_param.frame_max);
        break;

      case 'o':
        path = optarg;
        break;

#ifdef FREEBSD_4
      case 'i':
        dvsave_param.ifname = optarg;
        break;
#endif /* FREEBSD_4 */

      case 'c':
        dvsave_param.channel = atoi(optarg);
        break;

      case 'F':
        path = optarg;
        break;

#ifdef	NETBSD
      case 'D':
        dvsave_param.devname = optarg;
        break;
#endif	/* NETBSD */

      case 'p':
        dvsave_param.format = DV_FORMAT_PAL;
        break;

      default:
        printf("Usage : %s [options]\n", argv[0]);
        printf("\t-f frames_to_save\n");
        printf("\t-o output_file\n");
#ifdef FREEBSD_4
        printf("\t-i fw_interface\n");
#endif /* FREEBSD_4 */
#ifdef	NETBSD
        printf("\t-D devname\n");
#endif	/* NETBSD */
        printf("\t-c fw_channel\n");
        printf("\t-p use PAL\n");
        return(-1);
        break;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc > 0) {
    path = argv[0];
  }

  if (ieee1394_open(&dvsave_param) < 0) {
    printf("Failed to open IEEE1394\n");
    return(-1);
  }

  if (path == NULL) {
    fprintf(stderr, "INVALID OUTPUT FILE\n");
    return(-1);
  }

  if (path == NULL) {
    dvsave_param.fd = fileno(stdout);
  } else {
    dvsave_param.fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  }
  if (dvsave_param.fd < 0) {
    perror("open");
    return(-1);
  }

  printf("Saving DV data in file : %s\n", path);

  main_loop(&dvsave_param);

  close(dvsave_param.fd);
  return(0);
}
