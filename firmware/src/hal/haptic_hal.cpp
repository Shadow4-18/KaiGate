#include "hal/haptic_hal.h"

#include "hal/i2c_bus.h"
#include "pins.h"

#include <Adafruit_DRV2605.h>

static Adafruit_DRV2605 s_drv;
static bool s_ok = false;

bool hapticBegin() {
  if (!i2cLock(100)) return false;
  s_ok = s_drv.begin(&i2cBus(), ADDR_DRV2605);
  if (s_ok) {
    s_drv.selectLibrary(1);
    s_drv.setMode(DRV2605_MODE_INTTRIG);
    s_drv.useERM();
  }
  i2cUnlock();
  if (!s_ok) Serial.println("[haptic] DRV2605L not found at 0x5A");
  else Serial.println("[haptic] DRV2605L ERM ready (1027 coin)");
  return s_ok;
}

static void play(uint8_t a, uint8_t b = 0, uint8_t c = 0) {
  if (!s_ok) return;
  if (!i2cLock()) return;
  s_drv.setWaveform(0, a);
  s_drv.setWaveform(1, b);
  s_drv.setWaveform(2, c);
  s_drv.setWaveform(3, 0);
  s_drv.go();
  i2cUnlock();
}

void hapticClick() { play(1); }
void hapticDoublePulse() { play(10, 1); }  // double-click + strong click
void hapticAlert() { play(47, 15, 47); }   // buzz alert
void hapticBuzz(uint8_t effectId) { play(effectId); }
