#ifndef _DVSEND_MULTICAST_H_
#define _DVSEND_MULTICAST_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "param.h"

__BEGIN_DECLS
void multicast_settings __P((struct dvsend_param *));
__END_DECLS

#endif /* _DVSEND_MULTICAST_H_ */
