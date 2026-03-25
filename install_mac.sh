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

ARGS=$1

if [ -z "$ARGS" ]; then
    echo "=========================================="
    echo " Keyboard Daemon Installer (macOS)        "
    echo "=========================================="
    read -p "Enter your layout arguments separated by space (e.g. 'ABC Russian'): " ARGS
fi

if [ -f "kbd" ]; then
    SOURCE_BIN="kbd"
elif [ -f "kbd_macos" ]; then
    SOURCE_BIN="kbd_macos"
else
    echo "Error: Binary not found. Please compile it first or download the zip."
    exit 1
fi

mkdir -p ~/Library/LaunchAgents
mkdir -p ~/.local/bin

cp "$SOURCE_BIN" ~/.local/bin/kbd

cat <<EOF > ~/Library/LaunchAgents/dev.veitangie.kbd_daemon.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>dev.veitangie.kbd_daemon</string>
    <key>ProgramArguments</key>
    <array>
        <string>$HOME/.local/bin/kbd</string>
EOF

for arg in $ARGS; do
    echo "        <string>$arg</string>" >> ~/Library/LaunchAgents/dev.veitangie.kbd_daemon.plist
done

cat <<EOF >> ~/Library/LaunchAgents/dev.veitangie.kbd_daemon.plist
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
EOF

launchctl load -w ~/Library/LaunchAgents/dev.veitangie.kbd_daemon.plist
echo "Installed and started Mac LaunchAgent!"
