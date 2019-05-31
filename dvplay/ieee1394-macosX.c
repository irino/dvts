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

#ifdef	HAVE_CONFIG_H
#include <dvts-config.h>
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>

#include <difblock.h>
#include <dvts.h>

#include "param.h"

int
open_ieee1394(struct dvplay_param *dvplay_param)
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
    err = GetComponentInfo(aComponent, &aDesc, aName, NULL, NULL);
    if (err != noErr) {
      printf("GetComponentInfo() returned error %d\n", err);
    }
    else {
    }
  }

  dvplay_param->theInst = OpenDefaultComponent('ihlr', 'dv  ');
  if (dvplay_param->theInst == NULL) {
    printf("OpenDefaultComponent() failed\n");
    return(-1);
  }

  err = IDHGetDeviceList(dvplay_param->theInst, &deviceList);
  if (err != noErr) {
    printf("IDHGetDeviceList() failed\n");
    return(-1);
  }

  nDVDevices = QTCountChildrenOfType(deviceList,
                                     kParentAtomIsContainer,
                                     kIDHDeviceAtomType);
  if (nDVDevices < 0) {
    printf("QTCountChildrenOfType() failed\n");
    return(-1);
  }
  else if (nDVDevices == 0) {
    printf("No DV Devices found\n");
    return(-1);
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
    dvplay_param->videoConfig.atom = NULL;

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
        dvplay_param->videoConfig.container = deviceList;
        dvplay_param->videoConfig.atom = configAtom;
        break;
      }
    }
  }

  if (dvplay_param->videoConfig.atom == NULL) {
    printf("no good configs found\n");
    QTUnlockContainer(deviceList);
    return(-1);
  }
  QTUnlockContainer(deviceList);

  err = IDHSetDeviceConfiguration(dvplay_param->theInst,
                                  &dvplay_param->videoConfig);
  if (err != noErr) {
    return(-1);
  }

  err = IDHOpenDevice(dvplay_param->theInst,
                      kIDHOpenForWriteTransactions);
  if (err != noErr) {
    return(-1);
  }

  dvplay_param->buffer = NewPtr(120000);
  if (dvplay_param->buffer == NULL) {
    return(-1);
  }

  dvplay_param->isochParamBlock.buffer = dvplay_param->buffer;
  dvplay_param->isochParamBlock.actualCount = 0;
  dvplay_param->isochParamBlock.requestedCount = 120000;
  dvplay_param->isochParamBlock.completionProc = NULL;

  return(1);
}

void
close_ieee1394(struct dvplay_param *dvplay_param)
{
}

int
write_ieee1394(struct dvplay_param *dvplay_param)
{
  IDHParameterBlock *pb;
  OSErr err;
  int dseq, i;

  pb = &dvplay_param->isochParamBlock;

  /* prepare DV frame */
  memcpy((char *)pb->buffer, (char *)&dvplay_param->dvframe, 120000);

  err = IDHWrite(dvplay_param->theInst, pb);
  if (err != noErr) {
    printf("IDHWrite error : %d\n", err);
    return(-1);
  }

  return(1);
}
