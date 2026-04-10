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
#include <string.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#define DEVICES_DIR "/dev/"

int on_setting_changed(sd_bus_message *m, void *userdata,
                       sd_bus_error *ret_error) {
  (void)ret_error;
  Hardwarer *hardwarer = (Hardwarer *)userdata;

  LOG_DEBUG("GNOME Event Received\n");

  const char *namespace_id, *key;
  int r = sd_bus_message_read_basic(m, 's', &namespace_id);
  if (r <= 0) {
    LOG_ERR("Failed to read setting namespace: %d\n", r);
    return 0;
  }

  r = sd_bus_message_read_basic(m, 's', &key);
  if (r <= 0) {
    LOG_ERR("Failed to read setting key: %d\n", r);
    return 0;
  }

  LOG_INFO("GNOME Setting changed: %s %s\n", namespace_id, key);

  if (strcmp(namespace_id, "org.gnome.desktop.input-sources") != 0 ||
      strcmp(key, "mru-sources") != 0) {
    return 0;
  }

  const char *vtype;
  char ptype;
  int r_v = 0;

  while (sd_bus_message_peek_type(m, &ptype, &vtype) > 0 &&
         ptype == SD_BUS_TYPE_VARIANT) {
    if ((r_v = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, vtype)) <
        0) {
      LOG_ERR("Failed to enter setting variant: %s\n", strerror(-r_v));
      return 0;
    }
  }

  if (sd_bus_message_peek_type(m, &ptype, &vtype) > 0 &&
      ptype == SD_BUS_TYPE_ARRAY) {
    if ((r_v = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(ss)")) <
        0) {
      LOG_ERR("Failed to enter setting array: %s\n", strerror(-r_v));
      return 0;
    }

    const char *type, *layout;
    if ((r_v = sd_bus_message_read(m, "(ss)", &type, &layout)) > 0) {
      static char last_layout[64] = {0};
      if (strcmp(layout, last_layout) != 0) {
        strncpy(last_layout, layout, sizeof(last_layout) - 1);
        LOG_INFO("GNOME Active layout changed to -> %s\n", layout);
        sendActions(hardwarer, (char *)layout);
      } else {
        LOG_DEBUG("Ignoring duplicate GNOME active layout %s event\n", layout);
      }
    } else {
      LOG_WARN("Received empty mru-sources update in GNOME Settings\n");
    }

    sd_bus_message_exit_container(m);
  } else {
    LOG_ERR("Setting changed variant does not terminate in an array! "
            "Extracted type: %c contents: %s\n",
            ptype, vtype);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  sd_bus *bus = NULL;
  int r = sd_bus_default_user(&bus);
  if (r < 0) {
    LOG_ERR("Failed to connect to user bus: %s\n", strerror(-r));
    return 1;
  }

  int inotifyfd = inotify_init1(IN_NONBLOCK);
  inotify_add_watch(inotifyfd, DEVICES_DIR,
                    IN_CREATE | IN_DELETE | IN_ATTRIB | IN_ONLYDIR);

  int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

  int nfds = 0;
  struct pollfd fds[3];

  fds[0].fd = sd_bus_get_fd(bus);
  fds[0].events = POLLIN;
  nfds++;

  fds[1].fd = inotifyfd;
  fds[1].events = POLLIN;
  nfds++;

  fds[2].fd = timerfd;
  fds[2].events = POLLIN;
  nfds++;

  Hardwarer hardwarer = initHardwarer(DEVICES_DIR, timerfd, argc - 1, argv + 1);

  r = sd_bus_match_signal(bus, NULL, NULL, "/org/freedesktop/portal/desktop",
                          "org.freedesktop.portal.Settings", "SettingChanged",
                          on_setting_changed, &hardwarer);

  if (r < 0) {
    LOG_ERR("Failed to add match for SettingChanged: %s\n", strerror(-r));
    goto finish;
  }

  int triggered;
  while (1) {
    triggered = poll(fds, nfds, -1);
    if (triggered <= 0 || (fds[0].revents & POLLHUP)) {
      break;
    }

    if ((fds[0].revents & POLLIN) != 0) {
      while ((r = sd_bus_process(bus, NULL)) > 0)
        ;
      if (r < 0 && r != -ENOTCONN) {
        LOG_ERR("Failed to process DBus events: %s\n", strerror(-r));
      }
    }
    if ((fds[1].revents & POLLIN) != 0) {
      processEvents(&hardwarer, fds[1].fd);
    }
    if ((fds[2].revents & POLLIN) != 0) {
      processTimer(&hardwarer);
    }
  }

finish:
  sd_bus_unref(bus);
  close(inotifyfd);
  close(timerfd);

  return 0;
}
