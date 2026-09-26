#pragma once

#include <Arduino.h>
#include <lvgl.h>

bool displayBegin();
void displaySetBrightness(uint8_t level);
void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP);
void displayFill(uint16_t color);
void *displayDrawBuffer(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *rgb565);
