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

#include "../linux_common/hardwarer.h"
#include "../linux_common/log.h"
#include "parser.h"
#include <bits/time.h>
#include <linux/limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/un.h>
#include <unistd.h>

#define DEVICES_DIR "/dev/"

int initInotify(char *path) {
  int inotifyfd = inotify_init1(IN_NONBLOCK);
  inotify_add_watch(inotifyfd, path,
                    IN_CREATE | IN_DELETE | IN_ATTRIB | IN_ONLYDIR);
  return inotifyfd;
}

int main(int argc, char *argv[]) {
  char *runtimeDir = getenv("XDG_RUNTIME_DIR");
  if (runtimeDir == NULL) {
    return 72;
  }

  char *his = getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (his == NULL) {
    struct pollfd fd;
    fd.fd = initInotify(runtimeDir);
    fd.events = POLLIN;
    struct inotify_event *event =
        malloc(sizeof(struct inotify_event) + NAME_MAX + 1);
    int x = 100;
    while (x) {
      poll(&fd, 1, -1);
      int total =
          read(fd.fd, event, sizeof(struct inotify_event) + NAME_MAX + 1);
      while (total > 0) {
        if ((event->mask & IN_CREATE) > 0 && strcmp(event->name, "hypr") == 0) {
          return 0;
        }

        int eventSize = sizeof(struct inotify_event) + event->len;
        char *temp = (char *)event;
        temp += eventSize;
        total -= eventSize;
        event = (struct inotify_event *)temp;
      }
      x--;
    }
    return 72;
  }

  int size = 21;
  size += strlen(runtimeDir);
  size += strlen(his);
  char path[size];
  sprintf(path, "%s/hypr/%s/.socket2.sock", runtimeDir, his);

  int nfds = 0;
  struct pollfd fds[3];
  fds[0].fd = socket(AF_UNIX, SOCK_STREAM, 0);
  fds[0].events = POLLIN;
  nfds++;

  struct sockaddr_un address;
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, path);
  int len = sizeof(address);

  if (connect(fds[0].fd, (struct sockaddr *)&address, len) == -1)
    return 0;

  fds[1].fd = initInotify(DEVICES_DIR);
  fds[1].events = POLLIN;
  nfds++;

  fds[2].fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  fds[2].events = POLLIN;
  nfds++;

  int triggered;
  Parser parser = {};
  Hardwarer hardwarer =
      initHardwarer(DEVICES_DIR, fds[2].fd, argc - 1, argv + 1);

  while (1) {
    triggered = poll(fds, nfds, -1);
    if (triggered <= 0 || fds[0].revents & POLLHUP) {
      return 1;
    }
    LOG_DEBUG("Poll awoken\n");
    if ((fds[0].revents & POLLIN) != 0) {
      LOG_DEBUG("Received Hyprland socket event\n");
      char *locale = parse(&parser, fds[0].fd);
      if (locale != NULL) {
        sendActions(&hardwarer, locale);
        free(locale);
      }
    }
    if ((fds[1].revents & POLLIN) != 0) {
      LOG_DEBUG("Received inotify event\n");
      processEvents(&hardwarer, fds[1].fd);
    }
    if ((fds[2].revents & POLLIN) != 0) {
      LOG_DEBUG("Received timer event\n");
      processTimer(&hardwarer);
    }
  }

  for (int i = 0; i < nfds; i++) {
    close(fds[i].fd);
  }

  return 0;
}
