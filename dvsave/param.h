#ifndef _DVSAVE_PARAM_H_
#define _DVSAVE_PARAM_H_

#ifdef	MACOS_X
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <iokit/IOKitLib.h>
#include <DVComponentGlue/IsochronousDataHandler.h>
#include <DVComponentGlue/DeviceControl.h>
#endif /* MACOS_X */

struct dvsave_param {
  int fd;
  int frame_max;
  u_int32_t frame_count;
  int channel;

  int format;

#ifdef FREEBSD_4
  int soc;
  char *ifname;
#endif	/* FREEBSD_4 */

#ifdef MACOS_X
  ComponentInstance theInst;
  QTAtomSpec videoConfig;
  IDHParameterBlock isochParamBlock;
  int frameSize;
  int finished;
#endif	/* MACOS_X */

#ifdef	NETBSD
  int fd1394;
  char *devname;
#endif	/* NETBSD */

#ifdef	LINUX
  int fd1394;
  char *devname;
#endif	/* LINUX */
};

#endif /* _DVSAVE_PARAM_H_ */
