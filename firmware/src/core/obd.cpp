#include "core/obd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static int parseByte(const char *s) {
  int hi = hexNibble(s[0]);
  int lo = hexNibble(s[1]);
  if (hi < 0 || lo < 0) return -1;
  return (hi << 4) | lo;
}

static bool nextHexByte(const char *&p, int &out) {
  while (*p == ' ' || *p == '\r' || *p == '\n') ++p;
  if (!p[0] || !p[1]) return false;
  const int v = parseByte(p);
  if (v < 0) return false;
  p += 2;
  out = v;
  return true;
}

bool obdParseLine(const char *line, uint16_t &rpm, float &speedMph, float &coolantC, float &mapKpa,
                  float &iatC, float &batteryV) {
  if (!line) return false;
  const char *p = strstr(line, "41 ");
  if (!p) p = strstr(line, "41");
  if (!p) return false;
  if (p[2] == ' ') p += 3;
  else p += 2;

  int pid = 0, a = 0, b = 0;
  if (!nextHexByte(p, pid)) return false;
  if (!nextHexByte(p, a)) return false;
  switch (pid) {
    case 0x0C:  // RPM
      if (!nextHexByte(p, b)) return false;
      rpm = static_cast<uint16_t>(((a * 256) + b) / 4);
      return true;
    case 0x0D:
      speedMph = a * 0.621371f;
      return true;
    case 0x05:
      coolantC = static_cast<float>(a) - 40.0f;
      return true;
    case 0x0B:
      mapKpa = static_cast<float>(a);
      return true;
    case 0x0F:
      iatC = static_cast<float>(a) - 40.0f;
      return true;
    case 0x42:  // control module voltage
      if (!nextHexByte(p, b)) return false;
      batteryV = ((a * 256) + b) / 1000.0f;
      return true;
    default:
      return false;
  }
}

static const char *kPids[] = {"010C", "010D", "0105", "010B", "010F", "0142"};

const char *obdPidCommand(int pidIndex) {
  const int n = static_cast<int>(sizeof(kPids) / sizeof(kPids[0]));
  return kPids[(pidIndex % n + n) % n];
}

int obdBuildPoll(int pidIndex, char *out, size_t outLen) {
  return snprintf(out, outLen, "%s\r", obdPidCommand(pidIndex));
}

static char dtcChar(int v) {
  switch (v) {
    case 0: return 'P';
    case 1: return 'C';
    case 2: return 'B';
    default: return 'U';
  }
}

void obdDecodeDtcPayload(const char *hex, char codes[][8], uint8_t &count, uint8_t maxCount) {
  count = 0;
  if (!hex) return;
  const char *p = hex;
  int bytes[64];
  int n = 0;
  int v;
  while (n < 64 && nextHexByte(p, v)) bytes[n++] = v;
  int i = 0;
  if (n >= 2 && bytes[0] == 0x43) i = 2;  // 43 NN ...
  else if (n >= 1 && bytes[0] == 0x43) i = 1;
  for (; i + 1 < n && count < maxCount; i += 2) {
    const int A = bytes[i];
    const int B = bytes[i + 1];
    if (A == 0 && B == 0) continue;
    snprintf(codes[count], 8, "%c%u%02X%02X", dtcChar((A >> 6) & 0x03), (A >> 4) & 0x03, A & 0x0F, B);
    // Format is actually P0xxx: first digit is (A>>4)&3 for the numeric family after letter.
    snprintf(codes[count], 8, "%c%d%X%02X", dtcChar((A >> 6) & 0x03), (A >> 4) & 0x03, A & 0x0F, B);
    ++count;
  }
}
