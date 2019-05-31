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

#ifdef  HAVE_CONFIG_H
#include <dvts-config.h>
#endif  /* HAVE_CONFIG_H */

#include <stdio.h>

#include "param.h"

static OSStatus _macosX_notificationProc __P((IDHGenericEvent *, void *));
static OSStatus _macosX_idh_read __P((IDHGenericEvent *, void *));

struct dvsave_param *dvs_param;

int
ieee1394_open(struct dvsave_param *dvsave_param)
{
  u_int32_t seed;
  u_int32_t num_components;
  ComponentDescription desc;
  Component aComponent;
  Handle aName;
  OSErr err;
  ComponentDescription aDesc;
  IDHNotificationID notificationID;
  IDHDeviceID deviceID;

  QTAtomContainer deviceList = NULL;
  QTAtom deviceAtom;

  u_int32_t cmpFlag;
  u_int32_t isoversion;

  short i,j;
  short nDVDevices;
  long size;

  dvs_param = dvsave_param;

  seed = GetComponentListModSeed();

  memset(&desc, 0, sizeof(desc));
  desc.componentType = 'ihlr';
  desc.componentSubType = 0;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  num_components = CountComponents(&desc);
  if (num_components < 1) {
    printf("No DV device found : %d\n", num_components);
    return(-1);
  }

  aComponent = 0;
  aName = NewHandleClear(200);

  aComponent = FindNextComponent(aComponent, &desc);

  err = GetComponentInfo(aComponent, &aDesc, aName, NULL, NULL);
  if (err) {
    printf("GetComponetInfo() ERROR %d\n", err);
    return(-1);
  }

  dvsave_param->theInst = OpenDefaultComponent('ihlr', 'dv  ');

  err = IDHNewNotification(dvsave_param->theInst,
                           kIDHDeviceIDEveryDevice,
                           _macosX_notificationProc, dvsave_param->theInst,
                           &notificationID);
  if (err != noErr) {
    printf("IDHNewNotification failed\n");
    return(-1);
  }

  err = IDHNotifyMeWhen(dvsave_param->theInst,
                        notificationID, kIDHEventEveryEvent);
  if (err != noErr) {
    printf("IDHNotifyMeWhen failed\n");
    return(-1);
  }

  err = IDHGetDeviceList(dvsave_param->theInst, &deviceList);
  if (err != noErr) {
    printf("IDHGetDeviceList failed\n");
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
    printf("QTFindChildByIndex failed\n");
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
                        true, sizeof(deviceID),
                        &deviceID, &size);

    dataAtom = QTFindChildByIndex(deviceList, deviceAtom, 'ddin', 1, NULL);
    if (dataAtom == NULL) {
      return(-1);
    }
    QTCopyAtomDataToPtr(deviceList, dataAtom,
                        true, sizeof(deviceStatus), &deviceStatus, &size);

    if (deviceStatus.inputStandard == ntscIn) {
      dvsave_param->frameSize = 120000;
    }
    else if (deviceStatus.inputStandard == palIn) {
      dvsave_param->frameSize = 144000;
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
    dvsave_param->videoConfig.atom = NULL;

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
        dvsave_param->videoConfig.container = deviceList;
        dvsave_param->videoConfig.atom = configAtom;
      }
    }
  }

  if (dvsave_param->videoConfig.atom == NULL) {
    return(-1);
  }

  err = IDHSetDeviceConfiguration(dvsave_param->theInst,
                                  &dvsave_param->videoConfig);
  if (err != noErr) {
    return(-1);
  }

  err = IDHOpenDevice(dvsave_param->theInst, kIDHOpenForReadTransactions);
  if (err != noErr) {
    return(-1);
  }

  dvsave_param->isochParamBlock.buffer = NULL;
  dvsave_param->isochParamBlock.requestedCount = dvsave_param->frameSize;
  dvsave_param->isochParamBlock.actualCount = 0;
  dvsave_param->isochParamBlock.refCon = (void *)dvsave_param->theInst;

  dvsave_param->isochParamBlock.completionProc = _macosX_idh_read;

  return(1);
}

void
ieee1394_close()
{
}

int
ieee1394_read(struct dvsave_param *dvsave_param)
{
  OSErr err;

  err = IDHRead(dvsave_param->theInst, &dvsave_param->isochParamBlock);
  if (err != noErr) {
    return(-1);
  }

  return(1);
}

static OSStatus
_macosX_notificationProc(IDHGenericEvent *event, void *userData)
{
  IDHNotifyMeWhen(dvs_param->theInst,
                  event->eventHeader.notificationID, kIDHEventEveryEvent);

  return(noErr);
}

static OSStatus
_macosX_idh_read(IDHGenericEvent *eventRecord, void *userData)
{
  OSErr err;
  IDHParameterBlock *pb;
  ComponentInstance theInst;
  int len;
  int n;

  theInst = userData;
  pb = (IDHParameterBlock *)eventRecord;

  len = pb->actualCount;

  n = write(dvs_param->fd, pb->buffer, len);
  if (n < 1) {
    perror("write");
    return(-1);
  }

  err = IDHReleaseBuffer(theInst, pb);

  pb->buffer = NULL;
  pb->requestedCount = dvs_param->frameSize;
  pb->actualCount = 0;
  pb->completionProc = _macosX_idh_read;

  dvs_param->frame_count++;
  if (dvs_param->frame_max != 0 &&
      dvs_param->frame_count >= dvs_param->frame_max) {
    dvs_param->finished = 1;
    return(noErr);
  }

  err = IDHRead(theInst, pb);
  if (err != noErr) {
    printf("IDHRead Error\n");
    return(-1);
  }

  return(err);
}

int
main_loop(struct dvsave_param *dvsave_param)
{
  ieee1394_read(dvsave_param);
  while (1) {
    if (dvsave_param->finished) {
      break;
    }
    sleep(1);
  }

  return(1);
}
