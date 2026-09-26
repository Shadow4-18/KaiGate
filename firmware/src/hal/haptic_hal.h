#pragma once

#include <stdint.h>

bool hapticBegin();
void hapticClick();
void hapticDoublePulse();
void hapticAlert();
void hapticBuzz(uint8_t effectId);
