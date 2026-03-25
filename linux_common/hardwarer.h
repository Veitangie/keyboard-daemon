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

#define ZSA_VENDOR_ID 0x3297

typedef struct {
  char *filename;
} Device;

typedef struct {
  char *dir;
  int lendevices;
  int capdevices;
  Device *devices;
  char command[33];
  int timerfd;
  int devicesToInit;
  int layoutCount;
  char **layoutMap;
} Hardwarer;

Hardwarer initHardwarer(char *dir, int timerfd, int layoutCount,
                        char **layoutMap);

void processTimer(Hardwarer *hardwarer);

void processEvents(Hardwarer *hardwarer, int fd);

void sendActions(Hardwarer *hardwarer, char *locale);
