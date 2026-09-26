#pragma once

#include <Arduino.h>
#include <Wire.h>

bool i2cBusBegin();
TwoWire &i2cBus();
bool i2cLock(uint32_t timeoutMs = 50);
void i2cUnlock();
bool i2cProbe(uint8_t addr);
size_t i2cWriteReg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
size_t i2cReadReg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
