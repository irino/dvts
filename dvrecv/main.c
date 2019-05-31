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
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef	LINUX
#include <getopt.h>
#endif	/* LINUX */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>

#include <dvts.h>

#include "signal.h"
#include "info.h"
#include "param.h"
#include "udp.h"
#include "multicast.h"
#include "shm.h"
#include "read.h"
#include "flags.h"

#include "ieee1394.h"
#include "write.h"

/* this structure contains all configuration */
struct dvrecv_param dvrecv_param;

int
main(int argc, char *argv[])
{
  /* these are for getopt() */
  extern char *optarg;
  extern int optind;
  int op;

  /* used by fork() */
  int pid;

  /************************************/
  /*       program starts here        */
  /************************************/

  /*----------------------------------*/
  /*          SET DEFAULTS            */
  /*----------------------------------*/

  memset(&dvrecv_param, 0, sizeof(dvrecv_param));

  dvrecv_param.ieee1394dv.channel = 63;

#ifdef FREEBSD_4
  dvrecv_param.ieee1394dv.ifname = "/dev/dv0";
#ifdef CANOPUS_MC
  dvrecv_param.ieee1394dv.frac = 15;
#else	/* CANOPUS_MC */
  dvrecv_param.ieee1394dv.frac = 17;
#endif	/* CANOPUS_MC */
  dvrecv_param.ieee1394dv.mod  = 0;
#endif /* FREEBSD_4 */

#ifdef	LINUX
  dvrecv_param.ieee1394dv.devname = "/dev/dv1394";
#endif	/* LINUX */

  dvrecv_param.port = 8000;
  dvrecv_param.frames_in_buffer = 3;

#ifdef ENABLE_INET6
  dvrecv_param.s_addr.ss_family = AF_INET6;
  dvrecv_param.rtcp_in_s_addr.ss_family = AF_INET6;
  dvrecv_param.rtcp_out_s_addr.ss_family = AF_INET6;

  dvrecv_param.audio_s_addr.ss_family = AF_INET6;
  dvrecv_param.audio_rtcp_in_s_addr.ss_family = AF_INET6;
  dvrecv_param.audio_rtcp_out_s_addr.ss_family = AF_INET6;

  dvrecv_param.multicast_s_addr.ss_family = AF_INET6;
#else /* ENABLE_INET6 */
  dvrecv_param.s_addr.ss_family = AF_INET;
  dvrecv_param.rtcp_in_s_addr.ss_family = AF_INET;
  dvrecv_param.rtcp_out_s_addr.ss_family = AF_INET;

  dvrecv_param.audio_s_addr.ss_family = AF_INET;
  dvrecv_param.audio_rtcp_in_s_addr.ss_family = AF_INET;
  dvrecv_param.audio_rtcp_out_s_addr.ss_family = AF_INET;

  dvrecv_param.multicast_s_addr.ss_family = AF_INET;
#endif /* ENABLE_INET6 */

  dvrecv_param.flags |= USE_RTCP;

  dvrecv_param.pkt_loss_count = 3000;

  dvrecv_param.format = DV_FORMAT_NTSC;
  dvrecv_param.maxdifseq = DIFSEQ_NUM_NTSC;

  /*----------------------------------*/
  /*       END OF SET DEFAULTS        */
  /*----------------------------------*/

  /* getting options */
  while ((op = getopt(argc, argv, "v64j:M:P:D:Ll:d:RHp")) > 0) {
    switch (op) {
      case 'v':
        show_version();
        exit(0);
        break;

#ifdef ENABLE_INET6
      case '6':
        dvrecv_param.s_addr.ss_family = AF_INET6;
        dvrecv_param.rtcp_in_s_addr.ss_family = AF_INET6;
        dvrecv_param.rtcp_out_s_addr.ss_family = AF_INET6;

        dvrecv_param.audio_s_addr.ss_family = AF_INET6;
        dvrecv_param.audio_rtcp_in_s_addr.ss_family = AF_INET6;
        dvrecv_param.audio_rtcp_out_s_addr.ss_family = AF_INET6;
        break;
#endif /* ENABLE_INET6 */

      case '4':
        dvrecv_param.s_addr.ss_family = AF_INET;
        dvrecv_param.rtcp_in_s_addr.ss_family = AF_INET;
        dvrecv_param.rtcp_out_s_addr.ss_family = AF_INET;

        dvrecv_param.audio_s_addr.ss_family = AF_INET;
        dvrecv_param.audio_rtcp_in_s_addr.ss_family = AF_INET;
        dvrecv_param.audio_rtcp_out_s_addr.ss_family = AF_INET;
        break;

      case 'j':
        dvrecv_param.multicast_addr_str = optarg;
        break;

      case 'M':
        snprintf(dvrecv_param.multicast_ifname,
                 sizeof(dvrecv_param.multicast_ifname),
                 "%s", optarg);
        break;

      case 'P':
        dvrecv_param.port = atoi(optarg);
        if (dvrecv_param.port <1) {
          printf("Invalid port number : %d\n", dvrecv_param.port);
          exit(1);
        }
        break;

#ifdef FREEBSD_4
      case 'D':
        dvrecv_param.ieee1394dv.ifname = optarg;
        break;
#endif /* FREEBSD_4 */

      case 'L':
        dvrecv_param.flags |= SHOW_PACKETLOSS;
        break;

      case 'l':
        dvrecv_param.pkt_loss_count = atoi(optarg);
        if (dvrecv_param.pkt_loss_count < 1) {
          printf("Invalid packet loss count value : %d\n",
                 dvrecv_param.pkt_loss_count);
          exit(1);
        }
        dvrecv_param.flags |= SHOW_PACKETLOSS;
        break;

      case 'd':
        dvrecv_param.flags |= SEPARATED_AUDIO_STREAM;
        dvrecv_param.audio_port = atoi(optarg);
        if (dvrecv_param.audio_port < 1 || dvrecv_param.audio_port > 65534) {
          printf("Invalid audio port : %d\n", dvrecv_param.audio_port);
          exit(1);
        }
        break;

      case 'R':
        dvrecv_param.flags &= ~USE_RTCP;
        break;

      case 'p':
        dvrecv_param.format = DV_FORMAT_PAL;
        dvrecv_param.maxdifseq = DIFSEQ_NUM_PAL;
        break;

      case 'H':
      default:
        show_usage(argv[0]);
        exit(1);
        break;
    }
  }
  argc -= optind;
  argv += optind;

  /* set signal behaviors */
  set_signal_behavior();

  /* prepare a UDP socket for DV/RTP */
  if (prepare_udp_socket(&dvrecv_param) < 0) {
    return(1);
  }

  /* setup multicast parameters */
  /* will do nothing if the destination IP address is not a multicast address */
  if (multicast_settings(&dvrecv_param) < 0) {
    return(1);
  }

  /* create shared memory and set shmid in dvrecv_param */
  if (prepare_shared_memory(&dvrecv_param) < 0) {
    return(1);
  }


  /*-------------------------------------------------------------------*/
  /* This application uses fork().                                     */
  /* This application uses two processes connected by a shared memory. */
  /* There is a DV/RTP/UDP reading process.                            */
  /* There is a DV/IEEE1394 writing process.                           */
  /*-------------------------------------------------------------------*/

  /* parent process will be write loop process */
  /* child process will be read loop process */
  dvrecv_param.write_loop_pid = getpid();

  if ((pid = fork()) == 0) {
    dvrecv_param.read_loop_pid = getpid();

    if (attach_shared_memory(&dvrecv_param) < 0) {
      return(1);
    }

    dvrtp_read_loop(&dvrecv_param);

    /* kill the other process */
    kill(dvrecv_param.write_loop_pid, SIGKILL);

    return(1);
  }
  else {
    dvrecv_param.read_loop_pid = pid;

    /* prepare a IEEE1394 output file descriptor */
    if (prepare_ieee1394(&dvrecv_param) < 0) {
      printf("Failed to initialize IEEE1394 device\n");
      printf("EXIT\n");

      /* kill the other process */
      kill(dvrecv_param.read_loop_pid, SIGKILL);

      return(1);
    }

    if (attach_shared_memory(&dvrecv_param) < 0) {
      printf("Failed to attach shared memory\n");
      printf("EXIT\n");

      /* kill the other process */
      kill(dvrecv_param.read_loop_pid, SIGKILL);

      return(1);
    }

    ieee1394dv_write_loop(&dvrecv_param);

    /* kill the other process */
    kill(dvrecv_param.read_loop_pid, SIGKILL);

    return(1);
  }

  /* It is assumed that the program will never reach this point */
  return(0);
}
