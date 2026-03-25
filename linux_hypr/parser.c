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

#include "parser.h"
#include "../linux_common/log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int findIdx(char *source, char target, int len) {
  int idx = 0;
  for (; idx < len - 1; idx++) {
    if (source[idx] == target || source[idx] == 0) {
      break;
    }
  }
  return idx;
}

char *parse(Parser *parser, int fd) {
  char *result = NULL;
  LOG_DEBUG("Parsing event [state=%d, ptr=%d, buf=%s]\n", parser->state,
            parser->bufIdx, parser->buf);
  if (parser->state % 2 == 0) {

    int needToMove = 0;
    if (parser->bufIdx < BUFSIZE && parser->buf[parser->bufIdx] != 0) {
      needToMove = strlen(parser->buf + parser->bufIdx);
      memmove(parser->buf, parser->buf + parser->bufIdx, needToMove);
    }

    LOG_DEBUG("Moving %d bytes in buffer\n", needToMove);
    int n = read(fd, parser->buf + needToMove, BUFSIZE - needToMove - 1);
    if (n <= 0) {
      return result;
    }
    LOG_DEBUG("Read %d bytes from socket\n", n);

    parser->buf[needToMove + n] = 0;
    parser->bufIdx = 0;
    parser->state++;
    LOG_DEBUG("Buffer status post-read [buf=%s, ptr=%d, state=%d]\n",
              parser->buf, parser->bufIdx, parser->state);
  }

  while (parser->state % 2 == 1) {
    switch (parser->state) {
    case SEARCHING: {
      LOG_DEBUG("Parser state: SEARCHING\n");
      if (BUFSIZE - parser->bufIdx < TARGET_SIZE + 2 ||
          parser->buf[parser->bufIdx] == 0) {
        LOG_DEBUG("Incomplete data, awaiting more\n");
        parser->state = EMPTY;
        break;
      }
      if (strncmp(TARGET_EVENT, parser->buf + parser->bufIdx, TARGET_SIZE) ==
          0) {
        parser->bufIdx += TARGET_SIZE;
        for (int i = 0; i < 2; i++) {
          if (parser->buf[parser->bufIdx] != '>') {
            goto OuttaSwitch;
          }
          parser->bufIdx++;
        }
        parser->state = SKIPPING;
        LOG_INFO("Layout switch event detected\n");
      } else {
        parser->bufIdx += findIdx(parser->buf + parser->bufIdx, '\n',
                                  BUFSIZE - parser->bufIdx);
        if (parser->buf[parser->bufIdx] == '\n') {
          parser->bufIdx++;
        }
        break;
      }
    }
    case SKIPPING: {
      LOG_DEBUG("Parser state: SKIPPING\n");
      for (; parser->bufIdx < BUFSIZE - 1; parser->bufIdx++) {
        switch (parser->buf[parser->bufIdx]) {
        case 0:
          goto OuttaInnerLoopSkipping;
        case '\n':
          parser->bufIdx++;
          parser->state = SEARCHING;
          goto OuttaSwitch;
        case ',':
          parser->bufIdx++;
          parser->state = LOCALE;
          goto OuttaSwitch;
        }
      }

    OuttaInnerLoopSkipping:
      if (parser->buf[parser->bufIdx] == 0) {
        LOG_DEBUG("Incomplete data, awaiting more\n");
        parser->state = EMPTY_SKIPPING;
        break;
      }
    }
    case LOCALE: {
      LOG_DEBUG("Parser state: LOCALE\n");
      for (; parser->bufIdx < BUFSIZE - 1; parser->bufIdx++) {
        switch (parser->buf[parser->bufIdx]) {
        case 0:
          goto OuttaInnerLoopLocale;
        case '\n':
          parser->bufIdx++;
          parser->state = SEARCHING;
          parser->layoutName[parser->layoutIdx] = 0;
          parser->layoutIdx = 0;

          if (result == NULL) {
            result = malloc(sizeof(char[BUFSIZE / 8]));
          }
          strcpy(result, parser->layoutName);
          LOG_INFO("Parsed locale: %s\n", result);
          goto OuttaSwitch;
        }
        parser->layoutName[parser->layoutIdx] = parser->buf[parser->bufIdx];
        parser->layoutIdx++;
        if (parser->layoutIdx >= BUFSIZE / 8) {
          LOG_WARN("Layout buffer overflow, reverting to SEARCHING state\n");
          parser->state = SEARCHING;
          parser->layoutName[0] = 0;
          parser->layoutIdx = 0;
          goto OuttaSwitch;
        }
      }

    OuttaInnerLoopLocale:
      if (parser->buf[parser->bufIdx] == 0) {
        LOG_DEBUG("Incomplete data, awaiting more\n");
        parser->state = EMPTY_LOCALE;
        break;
      }
    }
    default: {
    }
    }
  OuttaSwitch: {}
  }
  if (result == NULL) {
    LOG_DEBUG("No event parsed, current state: %d\n", parser->state);
  } else {
    LOG_DEBUG("Returning locale '%s', current state: %d\n", result,
              parser->state);
  }
  return result;
}
