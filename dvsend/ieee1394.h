#ifndef _DVSEND_IEEE1394_H_
#define _DVSEND_IEEE1394_H_

#include <stdio.h>

#include "param.h"

int prepare_ieee1394 __P((struct dvsend_param *));
int main_loop __P((struct dvsend_param *));

#endif  /* _DVSEND_IEEE1394_H_ */
