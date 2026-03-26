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
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define DEVICES_DIR "/dev/"
#define BUFSIZE 4096

int main(int argc, char *argv[]) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    LOG_ERR("Failed to create pipe for dbus-monitor\n");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    LOG_ERR("Failed to fork dbus-monitor process\n");
    return 1;
  } else if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    prctl(PR_SET_PDEATHSIG, SIGKILL);

    execlp("stdbuf", "stdbuf", "-oL", "dbus-monitor", "--session",
           "interface='org.kde.KeyboardLayouts'", NULL);
    exit(1);
  }
  close(pipefd[1]);

  int dbus_fd = pipefd[0];

  int nfds = 0;
  struct pollfd fds[3];

  fds[0].fd = dbus_fd;
  fds[0].events = POLLIN;
  nfds++;

  int inotifyfd = inotify_init1(IN_NONBLOCK);
  inotify_add_watch(inotifyfd, DEVICES_DIR,
                    IN_CREATE | IN_DELETE | IN_ATTRIB | IN_ONLYDIR);
  fds[1].fd = inotifyfd;
  fds[1].events = POLLIN;
  nfds++;

  int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  fds[2].fd = timerfd;
  fds[2].events = POLLIN;
  nfds++;

  int triggered;
  Hardwarer hardwarer = initHardwarer(DEVICES_DIR, timerfd, argc - 1, argv + 1);

  char buf[BUFSIZE];
  int buf_len = 0;

  while (1) {
    triggered = poll(fds, nfds, -1);
    if (triggered <= 0 || (fds[0].revents & POLLHUP)) {
      break;
    }

    if ((fds[0].revents & POLLIN) != 0) {
      int n = read(dbus_fd, buf + buf_len, BUFSIZE - buf_len - 1);
      if (n > 0) {
        buf_len += n;
        buf[buf_len] = '\0';

        char *target = "string \"";
        char *start = strstr(buf, target);
        if (start) {
          start += strlen(target);
          char *end = strchr(start, '"');
          if (end) {
            *end = '\0';
            LOG_DEBUG("KDE DBus string parameter: %s\n", start);
            sendActions(&hardwarer, start);

            int remaining = buf_len - ((end + 1) - buf);
            memmove(buf, end + 1, remaining);
            buf_len = remaining;
            buf[buf_len] = '\0';
          }
        }

        if (buf_len >= BUFSIZE - 1) {
          buf_len = 0;
        }
      }
    }
    if ((fds[1].revents & POLLIN) != 0) {
      processEvents(&hardwarer, fds[1].fd);
    }
    if ((fds[2].revents & POLLIN) != 0) {
      processTimer(&hardwarer);
    }
  }

  close(inotifyfd);
  close(timerfd);
  close(dbus_fd);

  return 0;
}
