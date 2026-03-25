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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_INFO(fmt, ...) printf("[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) printf("[ERROR] " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)

#endif // LOG_H
