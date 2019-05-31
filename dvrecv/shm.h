#ifndef _DVRECV_SHM_H_
#define _DVRECV_SHM_H_

#include <stdio.h>

#include "param.h"

int prepare_shared_memory __P((struct dvrecv_param *));
int attach_shared_memory __P((struct dvrecv_param *));
void free_shared_memory __P((struct dvrecv_param *));

#endif /* _DVRECV_SHM_H_ */
