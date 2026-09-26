#include "hal/i2c_bus.h"

#include "pins.h"

static SemaphoreHandle_t s_mutex = nullptr;

bool i2cBusBegin() {
  if (!s_mutex) {
    s_mutex = xSemaphoreCreateMutex();
  }
  bool ok = Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_HZ);
  Wire.setTimeOut(30);
  return ok;
}

TwoWire &i2cBus() { return Wire; }

bool i2cLock(uint32_t timeoutMs) {
  if (!s_mutex) return true;
  return xSemaphoreTake(s_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void i2cUnlock() {
  if (s_mutex) xSemaphoreGive(s_mutex);
}

bool i2cProbe(uint8_t addr) {
  if (!i2cLock()) return false;
  Wire.beginTransmission(addr);
  uint8_t err = Wire.endTransmission();
  i2cUnlock();
  return err == 0;
}

size_t i2cWriteReg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
  if (!i2cLock()) return 0;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  size_t n = 0;
  if (data && len) n = Wire.write(data, len);
  uint8_t err = Wire.endTransmission();
  i2cUnlock();
  return err == 0 ? n + 1 : 0;
}

size_t i2cReadReg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
  if (!data || !len) return 0;
  if (!i2cLock()) return 0;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    i2cUnlock();
    return 0;
  }
  size_t got = Wire.requestFrom(static_cast<int>(addr), static_cast<int>(len));
  for (size_t i = 0; i < got; ++i) data[i] = Wire.read();
  i2cUnlock();
  return got;
}
