#pragma once

#include <Arduino.h>

struct TouchPoint {
  bool pressed = false;
  int16_t x = 0;
  int16_t y = 0;
  uint32_t pressStartMs = 0;
};

bool touchBegin();
bool touchRead(TouchPoint &out);
void touchLvglRead(struct _lv_indev_drv_t *drv, struct _lv_indev_data_t *data);
