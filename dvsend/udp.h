#ifndef _DVSEND_UDP_H_
#define _DVSEND_UDP_H_

#include <sys/types.h>

#include "param.h"

int prepare_udp_socket __P((struct dvsend_param *));
int send_pkt __P((struct dvsend_param *, u_int32_t *, int));
int send_audio_pkt __P((struct dvsend_param *, u_int32_t *, int));
int send_rtcp_pkt __P((struct dvsend_param *, u_int32_t *, int));
int send_rtcp_audio_pkt __P((struct dvsend_param *, u_int32_t *, int));
int add_destination __P((struct dvsend_param *, char *));
int add_audio_only_destination __P((struct dvsend_param *, char *));

#endif /* _DVSEND_UDP_H_ */
