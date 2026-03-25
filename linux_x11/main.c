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
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define DEVICES_DIR "/dev/"

int main(int argc, char *argv[]) {
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    LOG_ERR("Failed to open X display\n");
    return 72;
  }

  int xkbError;
  int xkbOpcode;
  int xkbEventBase;
  if (!XkbQueryExtension(display, &xkbOpcode, &xkbEventBase, &xkbError, NULL,
                         NULL)) {
    LOG_ERR("Xkb extension not supported\n");
    return 1;
  }

  XkbSelectEventDetails(display, XkbUseCoreKbd, XkbStateNotify,
                        XkbAllStateComponentsMask, XkbGroupStateMask);

  int x11_fd = ConnectionNumber(display);

  int nfds = 0;
  struct pollfd fds[3];

  fds[0].fd = x11_fd;
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

  XkbDescPtr desc = XkbAllocKeyboard();
  if (desc) {
    XkbGetNames(display, XkbGroupNamesMask, desc);
  }

  while (1) {
    triggered = poll(fds, nfds, -1);
    if (triggered <= 0 || (fds[0].revents & POLLHUP)) {
      break;
    }

    if ((fds[0].revents & POLLIN) != 0) {
      while (XPending(display)) {
        XEvent e;
        XNextEvent(display, &e);
        if (e.type == xkbEventBase) {
          XkbEvent *xkbEvent = (XkbEvent *)&e;
          if (xkbEvent->any.xkb_type == XkbStateNotify) {
            int group = xkbEvent->state.group;
            if (desc && desc->names && desc->names->groups[group]) {
              char *groupName =
                  XGetAtomName(display, desc->names->groups[group]);
              if (groupName) {
                LOG_INFO("X11 layout changed to: %s\n", groupName);
                sendActions(&hardwarer, groupName);
                XFree(groupName);
              }
            } else {
              LOG_WARN("Could not determine layout name for group index %d\n",
                       group);
            }
          }
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

  if (desc) {
    XkbFreeKeyboard(desc, XkbGroupNamesMask, True);
  }
  close(inotifyfd);
  close(timerfd);
  XCloseDisplay(display);

  return 0;
}
