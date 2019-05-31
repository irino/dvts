#ifndef	_DVPLAY_IEEE1394_H_
#define	_DVPLAY_IEEE1394_H_

int open_ieee1394 __P((struct dvplay_param *));
int write_ieee1394 __P((struct dvplay_param *));
void close_ieee1394 __P((struct dvplay_param *));

#endif	/* _DVPLAY_IEEE1394_H_ */
