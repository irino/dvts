#ifndef _DVSEND_RTCP_H_
#define _DVSEND_RTCP_H_

#include "param.h"

int send_rtcp_sr __P((struct dvsend_param *));
int send_rtcp_bye __P((struct dvsend_param *));
int process_rtcp __P((struct dvsend_param *));

#endif /* _DVSEND_RTCP_H_ */
