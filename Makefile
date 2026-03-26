# Keyboard Daemon - A cross-platform layout synchronization tool
# Copyright (C) 2026 Veitangie
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

TARGET := $(firstword $(MAKECMDGOALS))

SUPPORTED_TARGETS := hyprland gnome kde sway x11 macos win
BUILD_TARGETS := $(addprefix build-,$(SUPPORTED_TARGETS))

ifneq ($(filter $(TARGET),$(SUPPORTED_TARGETS) $(BUILD_TARGETS)),)
  DAEMON_ARGS := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  $(eval $(DAEMON_ARGS):;@:)
endif

.PHONY: all clean $(SUPPORTED_TARGETS) $(BUILD_TARGETS) $(DAEMON_ARGS)

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O3
MAC_CC ?= clang
WIN_CC ?= x86_64-w64-mingw32-gcc

hyprland: build-hyprland
	./install_linux.sh hyprland "$(DAEMON_ARGS)"

gnome: build-gnome
	./install_linux.sh gnome "$(DAEMON_ARGS)"

kde: build-kde
	./install_linux.sh kde "$(DAEMON_ARGS)"

sway: build-sway
	./install_linux.sh sway "$(DAEMON_ARGS)"

x11: build-x11
	./install_linux.sh x11 "$(DAEMON_ARGS)"

macos: build-macos
	./install_mac.sh "$(DAEMON_ARGS)"

win: build-win
	@echo "Windows build finished. To install, transfer 'kbd.exe' and 'install_win.ps1' to Windows and run:"
	@echo 'powershell -ExecutionPolicy Bypass -File install_win.ps1 "$(DAEMON_ARGS)"'

# Build-only targets
build-hyprland:
	$(CC) $(CFLAGS) linux_common/hardwarer.c linux_hypr/main.c linux_hypr/parser.c -o kbd

build-gnome:
	$(CC) $(CFLAGS) linux_common/hardwarer.c linux_wayland_gnome/main.c -o kbd

build-kde:
	$(CC) $(CFLAGS) linux_common/hardwarer.c linux_wayland_kde/main.c -o kbd

build-sway:
	$(CC) $(CFLAGS) linux_common/hardwarer.c linux_wayland_sway/main.c -o kbd

build-x11:
	$(CC) $(CFLAGS) linux_common/hardwarer.c linux_x11/main.c -lX11 -o kbd

build-macos:
	$(MAC_CC) $(CFLAGS) -framework Foundation -framework Carbon -framework IOKit macos/kbd.m -o kbd

build-win:
	$(WIN_CC) $(CFLAGS) -mwindows win/hardwarer.c win/main.c -lsetupapi -lhid -o kbd.exe

clean:
	rm -f kbd kbd.exe
