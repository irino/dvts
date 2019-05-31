/*
 * Copyright (c) 1999-2003 WIDE Project
 * All rights reserved.
 *
 * Author : Akimichi OGAWA (akimichi@sfc.wide.ad.jp)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code MUST retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form MUST reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    MUST display the following acknowledgement:
 *      This product includes software developed by Akimichi OGAWA.
 * 4. The name of the author MAY NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <iokit/IOKitLib.h>

#include <DVComponentGlue/IsochronousDataHandler.h>
#include <DVComponentGlue/DeviceControl.h>

/********************************/
#include <rtpvar.h>

#include "param.h"
#include "flags.h"
#include "ieee1394.h"
#include "rtp.h"
#include "rtcp.h"

#define	RTCP_SR_INTERVAL	3

extern struct dvsend_param dvsend_param;

static int _macosX_open_dv __P((void));
static OSStatus _macosX_notificationProc __P((IDHGenericEvent *, void *));
static OSStatus _macosX_idh_read_next __P((IDHGenericEvent *, void *));

static struct _macosX_param {
  IDHNotificationID notificationID;
  IDHDeviceID deviceID;
  IDHParameterBlock isochParamBlock;
  int frameSize;
  ComponentInstance theInst;
  QTAtomSpec videoConfig;
} macosX_param;


int
prepare_ieee1394 (struct dvsend_param *dvsend_param)
{
  u_int32_t seed;
  u_int32_t num_components;
  ComponentDescription desc;
  Component aComponent;
  Handle aName;
  OSErr err;
  ComponentDescription aDesc;

  memset(&macosX_param, 0, sizeof(macosX_param));
  memset(&desc, 0, sizeof(desc));

  seed = GetComponentListModSeed();
  desc.componentType = 'ihlr';
  desc.componentSubType = 0;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  num_components = CountComponents(&desc);

  aComponent = 0;
  aName = NewHandleClear(200);

  aComponent = FindNextComponent(aComponent, &desc);

  err = GetComponentInfo(aComponent, &aDesc, aName, NULL, NULL);
  if (err) {
    printf("GetComponetInfo() ERROR %d\n", err);
    return(-1);
  }

  return(1);
}

int
main_loop(struct dvsend_param *dvsend_param)
{
  int ret;
  struct timeval tv, tv_prev;

  ret = _macosX_open_dv();

  if (dvsend_param->flags & USE_RTCP) {
    for (;;) {
      if (gettimeofday(&tv, NULL) < 0) {
        printf("gettimeofday failed\n");
        break;
      }
      if ((tv.tv_sec - tv_prev.tv_sec) > RTCP_SR_INTERVAL) {
        send_rtcp_sr(dvsend_param);
        memcpy(&tv_prev, &tv, sizeof(tv_prev));
      }

      process_rtcp(dvsend_param);
    }
  }
  else {
    /* loop forever */
    for (;;) {
      sleep(10);
    }
  }

  return(ret);
}

static int
_macosX_open_dv(void)
{
  ComponentResult version;
  QTAtomContainer deviceList = NULL;
  short nDVDevices, i, j;
  QTAtom deviceAtom;
  u_int32_t cmpFlag;
  u_int32_t isoversion;
  long size;
  OSErr err;

  macosX_param.theInst = OpenDefaultComponent('ihlr', 'dv  ');
  printf("Instance is 0x%x\n", macosX_param.theInst);

  version = CallComponentVersion(macosX_param.theInst);
  printf("Version is 0x%x\n", version);

  err = IDHNewNotification(macosX_param.theInst,
                           kIDHDeviceIDEveryDevice,
                           _macosX_notificationProc, macosX_param.theInst,
                           &macosX_param.notificationID);
  if (err != noErr) {
    return(-1);
  }

  err = IDHNotifyMeWhen(macosX_param.theInst,
                        macosX_param.notificationID, kIDHEventEveryEvent);
  if (err != noErr) {
    return(-1);
  }

  err = IDHGetDeviceList(macosX_param.theInst, &deviceList);
  if (err != noErr) {
    return(-1);
  }

  nDVDevices = QTCountChildrenOfType(deviceList,
                                     kParentAtomIsContainer,
                                     kIDHDeviceAtomType);
  if (nDVDevices < 1) {
    printf("no DV input !!!\n");
    return(-1);
  }

  QTLockContainer(deviceList);
  deviceAtom = QTFindChildByIndex(deviceList,
                                  kParentAtomIsContainer,
                                  kIDHUseCMPAtomType, 1, NULL);
  if (deviceAtom == NULL) {
    return(-1);
  }

  QTCopyAtomDataToPtr(deviceList,
                      deviceAtom, true, sizeof(cmpFlag), &cmpFlag, &size);

  deviceAtom = QTFindChildByIndex(deviceList,
                                  kParentAtomIsContainer,
                                  kIDHIsochVersionAtomType, 1, NULL);
  if (deviceAtom == NULL) {
    return(-1);
  }

  QTCopyAtomDataToPtr(deviceList,
                      deviceAtom, true, sizeof(isoversion), &isoversion, &size);

  for (i=0; i<nDVDevices; i++) {
    QTAtom isochAtom, dataAtom;
    u_int32_t test[2];
    int nConfigs;
    char cameraName[256];
    IDHDeviceStatus deviceStatus;

    memset(&deviceStatus, 0, sizeof(deviceStatus));

    deviceAtom = QTFindChildByIndex(deviceList,
                                    kParentAtomIsContainer,
                                    kIDHDeviceAtomType, i + 1, NULL);
    if (deviceAtom == NULL) {
      return(-1);
    }

    dataAtom = QTFindChildByIndex(deviceList,
                                  deviceAtom,
                                  kIDHUniqueIDType, 1, NULL);
    if (dataAtom == NULL) {
      return(-1);
    }

    QTCopyAtomDataToPtr(deviceList, dataAtom, true, sizeof(test), test, &size);

    dataAtom = QTFindChildByIndex(deviceList, deviceAtom,
                                  kIDHNameAtomType, 1, NULL);
    if (dataAtom == NULL) {
      return(-1);
    }

    QTCopyAtomDataToPtr(deviceList, dataAtom, true, 255, cameraName, &size);
    cameraName[size] = 0;

    dataAtom = QTFindChildByIndex(deviceList, deviceAtom,
                                  kIDHDeviceIDType, 1, NULL);
    if (dataAtom == NULL) {
      return(-1);
    }
    QTCopyAtomDataToPtr(deviceList, dataAtom,
                        true, sizeof(macosX_param.deviceID),
                        &macosX_param.deviceID, &size);

    dataAtom = QTFindChildByIndex(deviceList, deviceAtom, 'ddin', 1, NULL);
    if (dataAtom == NULL) {
      return(-1);
    }
    QTCopyAtomDataToPtr(deviceList, dataAtom,
                        true, sizeof(deviceStatus), &deviceStatus, &size);

    if (deviceStatus.inputStandard == ntscIn) {
      macosX_param.frameSize = 120000;
    }
    else if (deviceStatus.inputStandard == palIn) {
      macosX_param.frameSize = 144000;
    }
    else {
      printf("unknown DV standard : %d\n", deviceStatus.inputStandard);
      return(-1);
    }

    isochAtom = QTFindChildByIndex(deviceList, deviceAtom,
                                   kIDHIsochServiceAtomType, 1, NULL);
    if (isochAtom == NULL) {
      return(-1);
    }

    nConfigs = QTCountChildrenOfType(deviceList, isochAtom,
                                     kIDHIsochModeAtomType);
    macosX_param.videoConfig.atom = NULL;

    for (j=0; j<nConfigs; ++j) {
      OSType mediaType;
      QTAtom configAtom, mediaAtom;

      configAtom = QTFindChildByIndex(deviceList, isochAtom,
                                      kIDHIsochModeAtomType, j+1, NULL);
      if (configAtom == NULL) {
        return(-1);
      }

      mediaAtom = QTFindChildByIndex(deviceList, configAtom,
                                     kIDHIsochMediaType, 1, NULL);
      if (mediaAtom == NULL) {
        return(-1);
      }

      QTCopyAtomDataToPtr(deviceList, mediaAtom, true, sizeof(mediaType),
                          &mediaType, &size);
      if (mediaType == kIDHVideoMediaAtomType) {
        macosX_param.videoConfig.container = deviceList;
        macosX_param.videoConfig.atom = configAtom;
      }
    }
  }

  if (macosX_param.videoConfig.atom == NULL) {
    return(-1);
  }

  err = IDHSetDeviceConfiguration(macosX_param.theInst,
                                  &macosX_param.videoConfig);
  if (err != noErr) {
    return(-1);
  }

  err = IDHOpenDevice(macosX_param.theInst, kIDHOpenForReadTransactions);
  if (err != noErr) {
    return(-1);
  }

  macosX_param.isochParamBlock.buffer = NULL;
  macosX_param.isochParamBlock.requestedCount = macosX_param.frameSize;
  macosX_param.isochParamBlock.actualCount = 0;
  macosX_param.isochParamBlock.refCon = (void *)macosX_param.theInst;

  macosX_param.isochParamBlock.completionProc = _macosX_idh_read_next;

  err = IDHRead(macosX_param.theInst, &macosX_param.isochParamBlock);
  if (err != noErr) {
    return(-1);
  }

  return(1);
}

static OSStatus
_macosX_notificationProc(IDHGenericEvent *event, void *userData)
{
  IDHNotifyMeWhen(macosX_param.theInst,
                  event->eventHeader.notificationID, kIDHEventEveryEvent);

  return(noErr);
}

static OSStatus
_macosX_idh_read_next(IDHGenericEvent *eventRecord, void *userData)
{
  OSErr err;
  IDHParameterBlock *pb;
  ComponentInstance theInst;
  int len = 0;
  int offset = 0;
/*
  u_char dbn, dseq, seq, sct;
  int n;
*/
  u_int32_t *u_int32_t_ptr;

  theInst = userData;
  pb = (IDHParameterBlock *)eventRecord;

  len = pb->actualCount;
  while (len > 0) {
    u_int32_t_ptr = (u_int32_t *)((char *)pb->buffer + offset);

    proc_dvdif(&dvsend_param, u_int32_t_ptr);
    len -= 80;
    offset += 80;
  }

  err = IDHReleaseBuffer(theInst, pb);

  pb->buffer = NULL;
  pb->requestedCount = macosX_param.frameSize;
  pb->actualCount = 0;
  pb->completionProc = _macosX_idh_read_next;

  err = IDHRead(theInst, pb);
  if (err != noErr) {
    return(-1);
  }

  return(err);
}
