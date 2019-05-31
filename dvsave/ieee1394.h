#ifndef	_DVSAVE_IEEE1394_H_
#define	_DVSAVE_IEEE1394_H_

#include "param.h"

int ieee1394_open __P((struct dvsave_param *));
void ieee1394_close __P((void));
int main_loop __P((struct dvsave_param *));

#endif	/* _DVSAVE_IEEE1394_H_ */
