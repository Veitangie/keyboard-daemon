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

#include "../linux_common/log.h"
#include "hardwarer.h"
#include <dbt.h>
#include <hidsdi.h>
#include <initguid.h>
#include <stdio.h>
#include <windows.h>

#ifndef LOCALE_SISO639LANGNAME
#define LOCALE_SISO639LANGNAME 0x0059
#endif

Hardwarer g_hardwarer;
HKL lastHkl = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE:
    SetTimer(hwnd, 1, 100, NULL);
    break;
  case WM_TIMER: {
    DWORD threadId = 0;
    GUITHREADINFO gti = {0};
    gti.cbSize = sizeof(GUITHREADINFO);

    if (GetGUIThreadInfo(0, &gti)) {
      HWND hwnd = gti.hwndFocus ? gti.hwndFocus : gti.hwndActive;
      if (hwnd) {
        threadId = GetWindowThreadProcessId(hwnd, NULL);
      }
    }

    if (!threadId) {
      HWND fg = GetForegroundWindow();
      if (fg) {
        threadId = GetWindowThreadProcessId(fg, NULL);
      }
    }

    if (threadId) {
      HKL hkl = GetKeyboardLayout(threadId);
      if (hkl != lastHkl) {
        lastHkl = hkl;
        char locale[9] = {0};
        if (GetLocaleInfoA((LCID)((UINT_PTR)hkl & 0xFFFF),
                           LOCALE_SISO639LANGNAME, locale, sizeof(locale))) {
          sendActions(&g_hardwarer, locale);
        }
      }
    }
    processTimer(&g_hardwarer);
    break;
  }
  case WM_DEVICECHANGE: {
    if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
      DEV_BROADCAST_HDR *hdr = (DEV_BROADCAST_HDR *)lParam;
      if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
        DEV_BROADCAST_DEVICEINTERFACE_A *devIF =
            (DEV_BROADCAST_DEVICEINTERFACE_A *)hdr;
        if (wParam == DBT_DEVICEARRIVAL) {
          LOG_DEBUG("inotify: device created/modified '%s'\n",
                    devIF->dbcc_name);
          addDevice(&g_hardwarer, devIF->dbcc_name);
        } else {
          LOG_DEBUG("inotify: device removed '%s'\n", devIF->dbcc_name);
          removeDevice(&g_hardwarer, devIF->dbcc_name);
        }
      }
    }
    break;
  }
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char *argv[]) {
  g_hardwarer = initHardwarer(argc - 1, argv + 1);

  WNDCLASSA wc = {0};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpszClassName = "KbdDaemonHidden";
  RegisterClassA(&wc);

  HWND hwnd = CreateWindowA("KbdDaemonHidden", "KbdDaemon", 0, 0, 0, 0, 0,
                            HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

  GUID hidGuid;
  HidD_GetHidGuid(&hidGuid);

  DEV_BROADCAST_DEVICEINTERFACE_A filter = {0};
  filter.dbcc_size = sizeof(filter);
  filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  filter.dbcc_classguid = hidGuid;

  RegisterDeviceNotificationA(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
}
