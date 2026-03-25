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

#define TARGET_EVENT "activelayout"
#define BUFSIZE 1024
#define TARGET_SIZE 12

enum ParserState {
  EMPTY,
  SEARCHING,
  EMPTY_SKIPPING,
  SKIPPING,
  EMPTY_LOCALE,
  LOCALE
};

typedef struct {
  char buf[BUFSIZE];
  char layoutName[BUFSIZE / 8];
  int bufIdx;
  int layoutIdx;
  enum ParserState state;
} Parser;

char *parse(Parser *parser, int fd);
