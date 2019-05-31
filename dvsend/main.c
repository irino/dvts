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

#ifdef	HAVE_CONFIG_H
#include <dvts-config.h>
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef	LINUX
#include <getopt.h>
#endif	/* LINUX */

#include "dvts.h"
#include "param.h"
#include "info.h"
#include "signal.h"
#include "flags.h"
#include "udp.h"
#include "multicast.h"
#include "rtp.h"

#include "ieee1394.h"

#define DEFAULT_DVTS_UDP_PORT	8000

/* this structure contains all configurations */
struct dvsend_param dvsend_param;

int
main(int argc, char *argv[])
{
  /* these are for getopt() */
  extern char *optarg;
  extern int optind;
  int op;

  int i;

  /* containor for the return value of each functions */
  int ret;

  /* audio redundancy value */
  int ar_val;

  /**********************************************/
  /*            program starts here             */
  /**********************************************/

  /*----------------------*/
  /*    INIT FUNCTIONS    */
  /*----------------------*/
  proc_dvdif_init();

  /*----------------------*/
  /*     SET DEFAULTS     */
  /*----------------------*/

  memset(&dvsend_param, 0, sizeof(dvsend_param));

  /* port number for DV/RTP packets */
  dvsend_param.default_port = DEFAULT_DVTS_UDP_PORT;

  /* TTL (hop limit) for multicast packets */
  dvsend_param.default_multi_ttl = 1;
  dvsend_param.default_multi_ifname = NULL;
  dvsend_param.default_multi_ifname6 = NULL;

  /* Flags (see dvsend-flags.h for detail) */
  dvsend_param.flags = USE_RTCP; /* RTCP is enabled by default */

  /* IEEE1394 isochronous channel */
  dvsend_param.ieee1394dv.channel = 0x3f;

#ifdef	FREEBSD_4
  /* IEEE1394 interface name (DEFAULT value will be ohci0) */
  dvsend_param.ieee1394dv.ifname = "ohci0";
#endif	/* FREEBSD_4 */

#ifdef	NETBSD
  /* IEEE1394 interface name (DEFAULT value will be /dev/fw0) */
  dvsend_param.ieee1394dv.ifname = "/dev/fwiso0";
#endif	/* NETBSD */

#ifdef	LINUX
  /* IEEE1394 device (DEFAULT value will be /dev/dv1394) */
  dvsend_param.ieee1394dv.devname = "/dev/dv1394";
#endif	/* LINUX */

  /* The length of each DV/RTP packet */
  /* The payload length will be (DIFblocks_in_pkt * 80) */
  dvsend_param.DIFblocks_in_pkt = 17;

  /* The video frame drop rate */
  /* (frame_drop == 1) means full rate, i.e. 100% quality */
  dvsend_param.frame_drop = 1;

  dvsend_param.format = DV_FORMAT_NTSC;

  /*---------------------*/
  /* END OF SET DEFAULTS */
  /*---------------------*/

  /* getting options */
  while ((op = getopt(argc, argv, "vh:64NI:P:s:M:m:t:d:f:LHD:C:Rr:p")) > 0) {
    switch (op) {
      case 'v':
        show_version();
        exit(1);
        break;

      case 'h':
        add_destination(&dvsend_param, optarg);
        break;

#ifdef ENABLE_INET6
      case '6':
        dvsend_param.flags |= USE_IPV6_ONLY;
        break;
#endif /* ENABLE_INET6 */

      case '4':
        dvsend_param.flags |= USE_IPV4_ONLY;
        break;

      case 'N':
        dvsend_param.flags |= NO_VIDEO_STREAM;
        break;

#ifdef	FREEBSD_4
      case 'I':
        dvsend_param.ieee1394dv.ifname = optarg;
        break;
#endif	/* FREEBSD_4 */

#ifdef	LINUX
      case 'I':
        dvsend_param.ieee1394dv.devname = optarg;
        break;
#endif	/* LINUX */

#ifdef	NETBSD
      case 'D':
        dvsend_param.ieee1394dv.ifname = optarg;
        break;
#endif	/* NETBSD */

      case 'P':
        dvsend_param.default_port = atoi(optarg);
        if (dvsend_param.default_port < 1) { 
          printf("Invalid port number\n");
          exit(-1);
        }
        break;

      case 'm':
        dvsend_param.default_multi_ifname = optarg;
        break;

      case 'M':
        dvsend_param.default_multi_ifname6 = optarg;
        break;

      case 't':
        dvsend_param.default_multi_ttl = atoi(optarg);
        break;

      case 's':
        dvsend_param.DIFblocks_in_pkt = atoi(optarg);
        if (dvsend_param.DIFblocks_in_pkt > 17) {
          printf("Number of DIF blocks in packet too large : %d\n",
                 dvsend_param.DIFblocks_in_pkt);
          printf("MUST be,  3 < DIFblocks < 18\n");
        }
        else if (dvsend_param.DIFblocks_in_pkt < 4) {
          printf("Number of DIF blocks in packet too small : %d\n",
                 dvsend_param.DIFblocks_in_pkt);
          printf("MUST be,  3 < DIFblocks < 18\n");
        }
        break;

      case 'd':
        dvsend_param.flags |= SEPARATED_AUDIO_STREAM;
        add_audio_only_destination(&dvsend_param, optarg);
        break;

      case 'f':
        dvsend_param.frame_drop = atoi(optarg);
        if (dvsend_param.frame_drop < 1) {
          printf("invalid frame drop value : %d\n", dvsend_param.frame_drop);
          exit(-1);
        }
        break;

      case 'L':
        dvsend_param.flags |= SHOW_PACKETLOSS;
        break;

      case 'R':
        dvsend_param.flags &= ~USE_RTCP;
        break;

      case 'r':
        ar_val = atoi(optarg);
        if (ar_val < 0) {
          printf("invalid audio redundancy level : %s\n", optarg);
          exit(1);
        }
        dvsend_param.audio_redundancy = ar_val;
        break;

      case 'p':
        dvsend_param.format = DV_FORMAT_PAL;
        break;

      case 'H':
      default:
        show_usage(argv[0]);
        exit(0);
        break;
    }
  }
  argc -= optind;
  argv += (optind - 1);

  for (i=argc; i>0; i--) {
    add_destination(&dvsend_param, argv[i]);
  }

  /* set signal behaviors */
  set_signal_behavior ();

  /* prepare a UDP socket for DV/RTP */
  if (prepare_udp_socket(&dvsend_param) < 0) {
    return(-1);
  }

  /* setup multicast parameters */
  /* will do nothing if the destination IP address is not a multicast address */
  multicast_settings(&dvsend_param);

  ret = prepare_ieee1394(&dvsend_param);
  if (ret < 0) {
    printf("Failed to prepare IEEE1394 input\n");
    printf("EXIT\n");
    return(-1);
  }

  /* receives DV/IEEE1394 packet, sends DV/RTP packets. */
  /* function "main_loop()" is contained within ieee1394 related file. */
  main_loop(&dvsend_param);

  return(ret);
}
