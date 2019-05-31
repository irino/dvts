#ifndef	_DVRECV_IEEE_H_
#define	_DVRECV_IEEE_H_

#include "param.h"

int	prepare_ieee1394 __P((struct dvrecv_param *));
void	stop_ieee1394_output __P((struct dvrecv_param *));

#endif	/* _DVRECV_IEEE_H_ */
