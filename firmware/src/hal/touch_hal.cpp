#include "hal/touch_hal.h"

#include "hal/i2c_bus.h"
#include "pins.h"

#include <lvgl.h>

static TouchPoint s_last;

static void cst820Reset() {
  pinMode(PIN_TP_RST, OUTPUT);
  pinMode(PIN_TP_INT, INPUT_PULLUP);
  digitalWrite(PIN_TP_RST, LOW);
  delay(10);
  digitalWrite(PIN_TP_RST, HIGH);
  delay(50);
}

static bool cst820Wake() {
  uint8_t mode = 0x00;
  // 0xFE = 0x00 exits the CST8xx automatic sleep so the panel stays live.
  return i2cWriteReg(ADDR_CST820, 0xFE, &mode, 1) > 0;
}

bool touchBegin() {
  cst820Reset();
  delay(20);
  if (!i2cProbe(ADDR_CST820)) {
    Serial.println("[touch] CST820 not found at 0x15");
    return false;
  }
  uint8_t chip = 0, fw = 0;
  i2cReadReg(ADDR_CST820, 0xA7, &chip, 1);
  i2cReadReg(ADDR_CST820, 0xA9, &fw, 1);
  cst820Wake();
  Serial.printf("[touch] CST820 chip=0x%02X fw=0x%02X\n", chip, fw);
  return true;
}

bool touchRead(TouchPoint &out) {
  uint8_t buf[6] = {};
  if (i2cReadReg(ADDR_CST820, 0x01, buf, sizeof(buf)) != sizeof(buf)) {
    out.pressed = false;
    return false;
  }
  const uint8_t fingers = buf[0] & 0x0F;
  const uint16_t x = ((buf[1] & 0x0F) << 8) | buf[2];
  const uint16_t y = ((buf[3] & 0x0F) << 8) | buf[4];
  out.pressed = fingers > 0 && x < 1000 && y < 1000;
  out.x = static_cast<int16_t>(constrain(static_cast<int>(x), 0, LCD_WIDTH - 1));
  out.y = static_cast<int16_t>(constrain(static_cast<int>(y), 0, LCD_HEIGHT - 1));
  if (out.pressed && !s_last.pressed) out.pressStartMs = millis();
  else if (out.pressed) out.pressStartMs = s_last.pressStartMs;
  else out.pressStartMs = 0;
  s_last = out;
  return true;
}

void touchLvglRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  TouchPoint p;
  touchRead(p);
  data->point.x = p.x;
  data->point.y = p.y;
  data->state = p.pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}
