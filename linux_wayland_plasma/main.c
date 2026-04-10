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
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#define DEVICES_DIR "/dev/"
#define MAX_LAYOUTS 256

char *layout_mapping[MAX_LAYOUTS];
int layout_count = 0;

int update_layouts(sd_bus *bus) {
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;
  int r;

  r = sd_bus_call_method(bus, "org.kde.keyboard", "/Layouts",
                         "org.kde.KeyboardLayouts", "getLayoutsList", &error,
                         &m, "");
  if (r < 0) {
    LOG_ERR("Failed to get layout list: %s\n", error.message);
    sd_bus_error_free(&error);
    return r;
  }

  const char *sig = sd_bus_message_get_signature(m, 1);
  if (!sig) {
    LOG_ERR("Failed to get message signature\n");
    sd_bus_message_unref(m);
    return -1;
  }

  for (int i = 0; i < layout_count; i++) {
    free(layout_mapping[i]);
  }
  layout_count = 0;

  if (strcmp(sig, "as") == 0) {
    r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "s");
    if (r < 0) {
      LOG_ERR("Failed to enter str array container: %s\n", strerror(-r));
      goto cleanup;
    }
    const char *s;
    while ((r = sd_bus_message_read(m, "s", &s)) > 0) {
      if (layout_count < MAX_LAYOUTS) {
        layout_mapping[layout_count++] = strdup(s);
        LOG_DEBUG("Loaded layout[%d] = %s\n", layout_count - 1, s);
      }
    }
    sd_bus_message_exit_container(m);
  } else if (strcmp(sig, "a(sss)") == 0) {
    r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(sss)");
    if (r < 0) {
      LOG_ERR("Failed to enter tuple array container: %s\n", strerror(-r));
      goto cleanup;
    }
    const char *s1, *s2, *s3;
    while ((r = sd_bus_message_read(m, "(sss)", &s1, &s2, &s3)) > 0) {
      if (layout_count < MAX_LAYOUTS) {
        layout_mapping[layout_count++] = strdup(s3);
        LOG_DEBUG("Loaded layout[%d] = %s\n", layout_count - 1, s3);
      }
    }
    sd_bus_message_exit_container(m);
  } else {
    LOG_ERR("Unsupported getLayoutsList map signature: %s\n", sig);
  }

cleanup:
  sd_bus_message_unref(m);
  return 0;
}

int on_layout_list_changed(sd_bus_message *m, void *userdata,
                           sd_bus_error *ret_error) {
  (void)userdata;
  (void)ret_error;

  LOG_INFO("Layout list changed event triggered, refreshing mappings...\n");
  sd_bus *bus = sd_bus_message_get_bus(m);
  update_layouts(bus);

  return 0;
}

int on_layout_changed(sd_bus_message *m, void *userdata,
                      sd_bus_error *ret_error) {
  (void)ret_error;
  uint32_t idx;
  int r = sd_bus_message_read(m, "u", &idx);

  if (r < 0) {
    LOG_ERR("Failed to parse layoutChanged signal payload: %s\n", strerror(-r));
    return 0;
  }

  if (idx >= (uint32_t)layout_count) {
    LOG_WARN("Received invalid layout index %u. Current layout mapping length "
             "is %d.\n",
             idx, layout_count);
    return 0;
  }

  static uint32_t last_idx = -1;
  if (idx == last_idx) {
    LOG_DEBUG("Ignoring duplicate Plasma layout index %u event\n", idx);
    return 0;
  }

  last_idx = idx;
  Hardwarer *hardwarer = (Hardwarer *)userdata;
  LOG_INFO("Plasma layout changed to index %u -> %s\n", idx,
           layout_mapping[idx]);
  sendActions(hardwarer, layout_mapping[idx]);

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

  update_layouts(bus);

  r = sd_bus_match_signal(bus, NULL, NULL, "/Layouts",
                          "org.kde.KeyboardLayouts", "layoutChanged",
                          on_layout_changed, &hardwarer);
  if (r < 0) {
    LOG_ERR("Failed to add match for layoutChanged: %s\n", strerror(-r));
    goto finish;
  }

  r = sd_bus_match_signal(bus, NULL, NULL, "/Layouts",
                          "org.kde.KeyboardLayouts", "layoutListChanged",
                          on_layout_list_changed, NULL);
  if (r < 0) {
    LOG_ERR("Failed to add match for layoutListChanged: %s\n", strerror(-r));
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
  for (int i = 0; i < layout_count; i++) {
    free(layout_mapping[i]);
  }
  sd_bus_unref(bus);
  close(inotifyfd);
  close(timerfd);

  return 0;
}
