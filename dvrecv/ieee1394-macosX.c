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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <dvts.h>

#include "param.h"
#include "ieee1394.h"

int
prepare_ieee1394 (struct dvrecv_param *dvrecv_param)
{
  u_int32_t seed;
  u_int32_t num;
  Handle aName;
  QTAtomContainer deviceList = NULL;
  QTAtom deviceAtom;
  ComponentDescription desc, aDesc;
  Component aComponent;
  long size;
  OSErr err;

  short nDVDevices, i, j;

  seed = GetComponentListModSeed();

  memset(&desc, 0, sizeof(desc));
  memset(&aDesc, 0, sizeof(aDesc));

  desc.componentType = 'ihlr';
  desc.componentSubType = 0;
  desc.componentManufacturer = 0;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  num = CountComponents(&desc);

  aComponent = 0;
  aName = NewHandleClear(200);

  while (aComponent = FindNextComponent(aComponent, &desc)) {
    err =  GetComponentInfo(aComponent, &aDesc, aName, NULL, NULL);
    if (err != noErr) {
      printf("GetComponentInfo() returned error %d\n", err);
    }
    else {
    }
  }

  dvrecv_param->ieee1394dv.theInst = OpenDefaultComponent('ihlr', 'dv  ');
  if (dvrecv_param->ieee1394dv.theInst == NULL) {
    printf("OpenDefaultComponent() failed\n");
    return(-1);
  }

  err = IDHGetDeviceList(dvrecv_param->ieee1394dv.theInst, &deviceList);
  if (err != noErr) {
    printf("IDHGetDeviceList() failed\n");
    return(-1);
  }

  nDVDevices = QTCountChildrenOfType(deviceList,
                                     kParentAtomIsContainer,
                                     kIDHDeviceAtomType);
  if (nDVDevices < 0) {
    printf("QTCountChildrenOfType() failed\n");
  }
  else if (nDVDevices == 0) {
    printf("No DV Devices found\n");
  }

  QTLockContainer(deviceList);

  for (i=0; i<nDVDevices; ++i) {
    QTAtom dataAtom, isochAtom;
    int nConfigs;
    IDHDeviceID deviceID;
    IDHDeviceStatus deviceStatus;

    deviceAtom = QTFindChildByIndex(deviceList,
                                    kParentAtomIsContainer,
                                    kIDHDeviceAtomType, i + 1, NULL);
    if (deviceAtom == NULL) {
      QTUnlockContainer(deviceList);
      return(-1);
    }

    dataAtom = QTFindChildByIndex(deviceList, deviceAtom, 'ddin', 1, NULL);
    if (dataAtom == NULL) {
      QTUnlockContainer(deviceList);
      return(-1);
    }
    QTCopyAtomDataToPtr(deviceList, dataAtom,
                        true, sizeof(deviceStatus), &deviceStatus, &size);

    printf("\nDV device status:\n");
    printf("version         : %d\n", deviceStatus.version);
    printf("current channel : %d\n", deviceStatus.currentChannel);
    printf("input standard  : %d\n", deviceStatus.inputStandard);

    isochAtom = QTFindChildByIndex(deviceList, deviceAtom,
                                   kIDHIsochServiceAtomType, 1, NULL);
    if (isochAtom == NULL) {
      QTUnlockContainer(deviceList);
      return(-1);
    }

    nConfigs = QTCountChildrenOfType(deviceList, isochAtom,
                                     kIDHIsochModeAtomType);

    dvrecv_param->ieee1394dv.videoConfig.atom = NULL;

    for (j=0; j<nConfigs; ++j) {
      OSType mediaType;
      QTAtom configAtom, mediaAtom;

      configAtom = QTFindChildByIndex(deviceList, isochAtom,
                                      kIDHIsochModeAtomType, j + 1, NULL);
      if (configAtom == NULL) {
        QTUnlockContainer(deviceList);
        return(-1);
      }

      mediaAtom = QTFindChildByIndex(deviceList, configAtom,
                                     kIDHIsochMediaType, 1, NULL);
      if (mediaAtom == NULL) {
        QTUnlockContainer(deviceList);
        return(-1);
      }
      QTCopyAtomDataToPtr(deviceList, mediaAtom,
                          true, sizeof(mediaType), &mediaType, &size);
      if (mediaType == kIDHVideoMediaAtomType) {
        /* found video device */
        dvrecv_param->ieee1394dv.videoConfig.container = deviceList;
        dvrecv_param->ieee1394dv.videoConfig.atom = configAtom;
        break;
      }
    }
  }

  if (dvrecv_param->ieee1394dv.videoConfig.atom == NULL) {
    printf("no good configs found\n");
    QTUnlockContainer(deviceList);
    return(-1);
  }
  QTUnlockContainer(deviceList);

  err = IDHSetDeviceConfiguration(dvrecv_param->ieee1394dv.theInst,
                                  &dvrecv_param->ieee1394dv.videoConfig);
  if (err != noErr) {
    return(-1);
  }

  err = IDHOpenDevice(dvrecv_param->ieee1394dv.theInst,
                      kIDHOpenForWriteTransactions);
  if (err != noErr) {
    return(-1);
  }

  return(1);
}

void
stop_ieee1394_output (struct dvrecv_param *dvrecv_param)
{
  OSErr err;

  err = IDHCloseDevice(dvrecv_param->ieee1394dv.theInst);
}
