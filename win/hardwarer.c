// Keyboard Daemon - A cross-platform layout synchronization tool
// Copyright (C) 2026 Veitangie
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "hardwarer.h"
#include "../linux_common/log.h"
#include <hidsdi.h>
#include <initguid.h>
#include <setupapi.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

const char *DEFAULT_LAYOUT_MAP_WIN[2] = {"en", "ru"};

void addDevice(Hardwarer *hardwarer, const char *devicePath) {
  LOG_INFO("Attempting to add device '%s' (current: %d, capacity: %d)\n",
           devicePath, hardwarer->lendevices, hardwarer->capdevices);

  for (int i = 0; i < hardwarer->lendevices; i++) {
    if (strcmp(hardwarer->devices[i].devicePath, devicePath) == 0) {
      LOG_WARN("Device is already registered\n");
      return;
    }
  }

  HANDLE hFile = CreateFileA(devicePath, GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, 0, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    LOG_ERR("Failed to open device file descriptor\n");
    return;
  }

  HIDD_ATTRIBUTES attr;
  attr.Size = sizeof(HIDD_ATTRIBUTES);
  if (!HidD_GetAttributes(hFile, &attr)) {
    LOG_ERR("Failed to retrieve vendor information\n");
    CloseHandle(hFile);
    return;
  }

  if (attr.VendorID != ZSA_VENDOR_ID) {
    CloseHandle(hFile);
    return;
  }
  LOG_INFO("Vendor identified\n");

  char handshake[33] = {0};
  DWORD written;

  WriteFile(hFile, handshake, 33, &written, NULL);
  handshake[1] = 1;
  WriteFile(hFile, handshake, 33, &written, NULL);
  handshake[1] = 0xfe;
  WriteFile(hFile, handshake, 33, &written, NULL);

  LOG_INFO("Sent Keymapp handshake to device\n");
  CloseHandle(hFile);

  if (written != 33)
    return;

  if (hardwarer->lendevices == hardwarer->capdevices) {
    Device *newdev = malloc(sizeof(Device) * (hardwarer->capdevices + 1));
    if (!newdev)
      return;
    memcpy(newdev, hardwarer->devices, sizeof(Device) * hardwarer->capdevices);
    free(hardwarer->devices);
    hardwarer->devices = newdev;
    hardwarer->capdevices++;
  }

  hardwarer->devices[hardwarer->lendevices].devicePath = strdup(devicePath);
  hardwarer->lendevices++;
  hardwarer->devicesToInit++;

  LOG_DEBUG("Scheduled timer for device initialization\n");
}

void removeDevice(Hardwarer *hardwarer, const char *devicePath) {
  int i = 0;
  for (; i < hardwarer->lendevices; i++) {
    if (strcasecmp(hardwarer->devices[i].devicePath, devicePath) == 0) {
      break;
    }
  }

  if (i < hardwarer->lendevices) {
    LOG_INFO("Removed device '%s' from watchlist (remaining: %d, cap: %d)\n",
             hardwarer->devices[i].devicePath, hardwarer->lendevices - 1,
             hardwarer->capdevices);
    free(hardwarer->devices[i].devicePath);
    for (; i < hardwarer->lendevices - 1; i++) {
      hardwarer->devices[i] = hardwarer->devices[i + 1];
    }
    hardwarer->lendevices--;
    LOG_DEBUG("Device removal complete\n");
  }
}

Hardwarer initHardwarer(int layoutCount, char **layoutMap) {
  Hardwarer res = {0};
  res.capdevices = 1;
  res.devices = malloc(sizeof(Device));
  res.layoutCount = layoutCount;
  res.layoutMap = layoutMap;

  res.command[1] = 4;
  res.command[2] = 1;
  for (int i = 0; i < 29; i++) {
    res.command[i + 4] = 0;
  }
  if (layoutCount == 0) {
    res.layoutMap = (char **)DEFAULT_LAYOUT_MAP_WIN;
    res.layoutCount = 2;
  }

  GUID hidGuid;
  HidD_GetHidGuid(&hidGuid);

  HDEVINFO hDevInfo = SetupDiGetClassDevs(
      &hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (hDevInfo != INVALID_HANDLE_VALUE) {
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (int i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &hidGuid, i,
                                                &deviceInterfaceData);
         i++) {
      DWORD requiredSize = 0;
      SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0,
                                      &requiredSize, NULL);
      SP_DEVICE_INTERFACE_DETAIL_DATA_A *detailData = malloc(requiredSize);
      if (detailData) {
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if (SetupDiGetDeviceInterfaceDetailA(hDevInfo, &deviceInterfaceData,
                                             detailData, requiredSize, NULL,
                                             NULL)) {
          addDevice(&res, detailData->DevicePath);
        }
        free(detailData);
      }
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
  }
  return res;
}

void sendMsg(Hardwarer *hardwarer, int deviceIdx, char *msgs[], int nmsgs) {
  HANDLE hFile = CreateFileA(
      hardwarer->devices[deviceIdx].devicePath, GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  for (int i = 0; i < nmsgs; i++) {
    char *command = msgs[i];
    LOG_DEBUG("Sending command %02x %02x %02x %02x to %s\n",
              (unsigned char)command[0], (unsigned char)command[1],
              (unsigned char)command[2], (unsigned char)command[3],
              hardwarer->devices[deviceIdx].devicePath);
    DWORD written;
    WriteFile(hFile, command, 33, &written, NULL);
    LOG_DEBUG("Successfully wrote %lu bytes\n", written);
    Sleep(50);
  }
  CloseHandle(hFile);
}

void sendActions(Hardwarer *hardwarer, const char *locale) {
  int i;
  for (i = 0; i < hardwarer->layoutCount; i++) {
    if (strncasecmp(hardwarer->layoutMap[i], locale,
                    strlen(hardwarer->layoutMap[i])) == 0) {
      LOG_DEBUG("Found locale entry %s for locale %s, layer index: %d\n",
                hardwarer->layoutMap[i], locale, i);
      hardwarer->command[3] = i;
      break;
    }
  }
  if (i >= hardwarer->layoutCount)
    return;

  for (int j = 0; j < hardwarer->lendevices; j++) {
    char *cmd[1] = {hardwarer->command};
    sendMsg(hardwarer, j, cmd, 1);
  }
}

void processTimer(Hardwarer *hardwarer) {
  if (hardwarer->devicesToInit <= 0)
    return;
  char cmd1[33] = {0};
  char cmd2[33] = {0};
  char cmd3[33] = {0};
  cmd2[1] = 1;
  cmd3[1] = (char)254;
  char *msgs[4] = {cmd1, cmd2, cmd3, hardwarer->command};

  for (int i = 0; i < hardwarer->devicesToInit && i < hardwarer->lendevices;
       i++) {
    int deviceToInit = hardwarer->lendevices - 1 - i;
    LOG_INFO("Initializing device: %s\n",
             hardwarer->devices[deviceToInit].devicePath);
    sendMsg(hardwarer, deviceToInit, msgs, 4);
  }
  hardwarer->devicesToInit = 0;
}
