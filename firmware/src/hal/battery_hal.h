#pragma once

#include <stdint.h>

bool batteryBegin();
void batteryHoldPower(bool hold);
float batteryVoltage();
uint8_t batteryPercent();
bool batteryPwrHeld();
