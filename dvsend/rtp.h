#ifndef	_DVSEND_RTP_H_
#define	_DVSEND_RTP_H_

#include <sys/types.h>
#include "param.h"

#define		FLUSH_DVRTP_TYPE_VIDEO	0x00000001
#define		FLUSH_DVRTP_TYPE_AUDIO	0x00000002

void	proc_dvdif_init __P((void));
void	proc_dvdif	__P((struct dvsend_param *, u_int32_t *));

#endif	/* _DVSEND_RTP_H_ */
