#pragma once

#include <stdint.h>

bool audioBegin();
void audioSetEnabled(bool on);
void audioBeep(uint16_t freqHz = 1320, uint16_t durationMs = 90);
void audioShiftBeep();
void audioAlert();
void audioTick(uint32_t nowMs);
