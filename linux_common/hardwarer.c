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
#include "log.h"
#include <bits/types/struct_itimerspec.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <unistd.h>

const size_t EVENT_BUFFER_SIZE =
    (sizeof(struct inotify_event) + NAME_MAX + 1) * 8;

const char *DEFAULT_LAYOUT_MAP[2] = {"en", "ru"};

void addDevice(Hardwarer *hardwarer, char *filename) {
  LOG_INFO("Attempting to add device '%s' (current: %d, capacity: %d)\n",
           filename, hardwarer->lendevices, hardwarer->capdevices);
  if (strncmp(filename, "hidraw", 6) != 0) {
    return;
  }
  LOG_DEBUG("Device identified as hidraw\n");

  char fullPath[strlen(hardwarer->dir) + strlen(filename) + 1];
  strncpy(fullPath, hardwarer->dir, strlen(hardwarer->dir));
  strcpy(fullPath + strlen(hardwarer->dir), filename);

  struct hidraw_devinfo info;
  int fd = open(fullPath, O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    LOG_ERR("Failed to open device file descriptor\n");
    return;
  }
  int res = ioctl(fd, HIDIOCGRAWINFO, &info);
  if (res < 0) {
    LOG_ERR("Failed to retrieve vendor information\n");
    close(fd);
    return;
  }

  if (info.vendor != ZSA_VENDOR_ID) {
    close(fd);
    return;
  }
  LOG_INFO("Vendor identified\n");

  int size = 0;
  ioctl(fd, HIDIOCGRDESCSIZE, &size);
  struct hidraw_report_descriptor desc;
  desc.size = size;
  ioctl(fd, HIDIOCGRDESC, &desc);

  int canProceed = 0;
  for (int i = 0; i < size - 2; i++) {
    if (desc.value[i] == 0x06 && desc.value[i + 1] == 0x60 &&
        desc.value[i + 2] == 0xFF) {
      canProceed = 1;
      break;
    }
  }

  if (!canProceed) {
    close(fd);
    return;
  }
  LOG_INFO("Device matches required signature\n");

  char handshake[33] = {0};
  int lastWrite = write(fd, handshake, 33);
  handshake[1] = 1;
  lastWrite = write(fd, handshake, 33);
  handshake[1] = 0xfe;
  lastWrite = write(fd, handshake, 33);
  LOG_INFO("Sent Keymapp handshake to device\n");
  close(fd);

  if (lastWrite != 33) {
    return;
  }

  for (int i = 0; i < hardwarer->lendevices; i++) {
    if (strcmp(hardwarer->devices[i].filename, filename) == 0) {
      LOG_WARN("Device is already registered\n");
      return;
    }
  }

  char *heapified = malloc(strlen(filename) + 1);
  strcpy(heapified, filename);

  if (hardwarer->lendevices == hardwarer->capdevices) {
    Device *new = malloc(sizeof(Device) * (hardwarer->capdevices + 1));
    memcpy(new, hardwarer->devices, sizeof(Device) * hardwarer->capdevices);
    free(hardwarer->devices);
    hardwarer->devices = new;
    hardwarer->capdevices++;
  }

  LOG_INFO("Added device '%s' to watchlist (len: %d, cap: %d)\n", heapified,
           hardwarer->lendevices + 1, hardwarer->capdevices);
  hardwarer->devices[hardwarer->lendevices].filename = heapified;
  hardwarer->lendevices++;
  hardwarer->devicesToInit++;

  LOG_DEBUG("Scheduled timer for device initialization\n");
  struct itimerspec spec = {0};
  spec.it_value.tv_sec = 1;
  spec.it_value.tv_nsec = 500000000;
  timerfd_settime(hardwarer->timerfd, 0, &spec, NULL);
}

Hardwarer initHardwarer(char *dir, int timerfd, int layoutCount,
                        char **layoutMap) {
  Hardwarer res = {dir,      0,       1, malloc(sizeof(Device)),
                   {0},      timerfd, 0, layoutCount,
                   layoutMap};
  res.command[1] = 4;
  res.command[2] = 1;
  if (layoutCount == 0) {
    res.layoutMap = (char **)DEFAULT_LAYOUT_MAP;
    res.layoutCount = 2;
  }

  DIR *root = opendir(dir);
  if (root == NULL) {
    exit(1);
  }

  struct dirent *en;
  while ((en = readdir(root)) != NULL) {
    addDevice(&res, en->d_name);
  }
  return res;
}

void processEvents(Hardwarer *hardwarer, int fd) {
  LOG_DEBUG("Processing hardware events\n");
  struct inotify_event *event = malloc(EVENT_BUFFER_SIZE);
  struct inotify_event *toFree = event;
  LOG_DEBUG("Reading inotify events (blocking point)\n");
  int total = read(fd, event, EVENT_BUFFER_SIZE);

  while (total > 0) {
    LOG_DEBUG("Bytes read from inotify: %d\n", total);
    if (event->mask & (IN_CREATE | IN_ATTRIB)) {
      LOG_INFO("inotify: device created/modified '%s' (new: %d, attrib: %d)\n",
               event->name, (event->mask & IN_CREATE) > 0,
               (event->mask & IN_ATTRIB) > 0);
      addDevice(hardwarer, event->name);
    } else if (event->mask & IN_DELETE) {
      LOG_INFO("inotify: device removed '%s'\n", event->name);

      int i = 0;
      for (; i < hardwarer->lendevices; i++) {
        if (strcmp(hardwarer->devices[i].filename, event->name) == 0) {
          break;
        }
      }

      if (i < hardwarer->lendevices) {

        LOG_INFO(
            "Removed device '%s' from watchlist (remaining: %d, cap: %d)\n",
            hardwarer->devices[i].filename, hardwarer->lendevices - 1,
            hardwarer->capdevices);
        free(hardwarer->devices[i].filename);
        for (; i < hardwarer->lendevices - 1; i++) {
          hardwarer->devices[i] = hardwarer->devices[i + 1];
        }

        hardwarer->lendevices--;
        LOG_DEBUG("Device removal complete\n");
      }
      LOG_DEBUG("Finished processing device removal\n");
    }

    int eventSize = sizeof(struct inotify_event) + event->len;
    LOG_DEBUG("Processed inotify event size: %d\n", eventSize);
    char *temp = (char *)event;
    temp += eventSize;
    total -= eventSize;
    event = (struct inotify_event *)temp;
  }

  free(toFree);
  return;
}

void sendMsg(Hardwarer *hardwarer, int deviceIdx, char *msgs[], int nmsgs) {
  char fullPath[strlen(hardwarer->dir) +
                strlen(hardwarer->devices[deviceIdx].filename) + 1];
  strncpy(fullPath, hardwarer->dir, strlen(hardwarer->dir));
  strcpy(fullPath + strlen(hardwarer->dir),
         hardwarer->devices[deviceIdx].filename);
  int fd = open(fullPath, O_WRONLY | O_NONBLOCK);

  for (int i = 0; i < nmsgs; i++) {
    char *command = msgs[i];
    LOG_DEBUG("Sending command %02x %02x %02x %02x to %s\n",
              (unsigned char)command[0], (unsigned char)command[1],
              (unsigned char)command[2], (unsigned char)command[3], fullPath);
    if (fd >= 0) {
      ssize_t written = write(fd, command, 33);
      LOG_DEBUG("Successfully wrote %zd bytes\n", written);
    }
    usleep(50000);
  }

  if (fd >= 0) {
    close(fd);
  }
}

void sendActions(Hardwarer *hardwarer, char *locale) {
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
  if (i >= hardwarer->layoutCount) {
    return;
  }

  for (int i = 0; i < hardwarer->lendevices; i++) {
    char *cmd[1] = {hardwarer->command};
    sendMsg(hardwarer, i, cmd, 1);
  }
}

void processTimer(Hardwarer *hardwarer) {
  unsigned long expirations = 0;
  if (read(hardwarer->timerfd, &expirations, sizeof(expirations)) < 0) {
    LOG_DEBUG("Timer read failed\n");
  }
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
             hardwarer->devices[deviceToInit].filename);
    sendMsg(hardwarer, deviceToInit, msgs, 4);
  }
  hardwarer->devicesToInit = 0;
}
