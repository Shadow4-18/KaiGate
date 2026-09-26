#pragma once

#include <stddef.h>
#include <stdint.h>

bool obdParseLine(const char *line, uint16_t &rpm, float &speedMph, float &coolantC, float &mapKpa,
                  float &iatC, float &batteryV);
int obdBuildPoll(int pidIndex, char *out, size_t outLen);
void obdDecodeDtcPayload(const char *hex, char codes[][8], uint8_t &count, uint8_t maxCount);
const char *obdPidCommand(int pidIndex);
