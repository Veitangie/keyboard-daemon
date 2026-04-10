#!/bin/bash

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

PLATFORM=$1
ARGS=$2
if [ -z "$PLATFORM" ] || [ -z "$ARGS" ]; then
    echo "=========================================="
    echo " Keyboard Daemon Installer (Linux)        "
    echo "=========================================="
fi

if [ -z "$PLATFORM" ]; then
    read -p "Which platform are you using? (hyprland/gnome/plasma/sway/x11): " PLATFORM
fi

if [ -z "$ARGS" ]; then
    read -p "Enter your layout arguments separated by space (e.g. 'en ru'): " ARGS
fi

BIN_NAME="kbd_${PLATFORM}"

if [ -f "kbd" ]; then
    SOURCE_BIN="kbd"
elif [ -f "kbd_${PLATFORM}" ]; then
    SOURCE_BIN="kbd_${PLATFORM}"
else
    echo "Error: Binary not found. Please compile it first or download the zip."
    exit 1
fi

echo ""
echo "Setting up device permissions for ZSA keyboards..."
echo "Sudo is required to write to /etc/udev/rules.d/ and reload udev rules."
sudo bash -c 'cat <<EOF > /etc/udev/rules.d/50-kbd-zsa-access.rules
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="3297", TAG+="uaccess"
EOF'

sudo udevadm control --reload-rules
sudo udevadm trigger
echo "Udev rules applied."
echo ""

mkdir -p ~/.config/systemd/user
mkdir -p ~/.local/bin

systemctl --user stop "kbd_${PLATFORM}.service" 2>/dev/null || true

cp "$SOURCE_BIN" ~/.local/bin/$BIN_NAME

COUNT=$(find ~/.local/bin -maxdepth 1 -type f -name "kbd_*" | wc -l)
if [ "$COUNT" -eq 1 ]; then
    ln -sf ~/.local/bin/$BIN_NAME ~/.local/bin/kbd
fi

cat <<EOF > ~/.config/systemd/user/kbd_${PLATFORM}.service
[Unit]
Description=Keyboard Daemon ($PLATFORM)
After=graphical-session.target

[Service]
ExecStart=%h/.local/bin/$BIN_NAME $ARGS
Restart=always
RestartSec=2

[Install]
WantedBy=default.target
EOF

systemctl --user daemon-reload
systemctl --user enable --now kbd_${PLATFORM}.service
echo "Installed and started kbd_${PLATFORM}.service!"
