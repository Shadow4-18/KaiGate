#include "hal/display_hal.h"

#include "config.h"
#include "pins.h"

#include <Arduino_GFX_Library.h>

static Arduino_DataBus *s_bus = nullptr;
static Arduino_GFX *s_gfx = nullptr;

bool displayBegin() {
  pinMode(PIN_LCD_TE, INPUT);
  pinMode(PIN_LCD_RST, OUTPUT);
  digitalWrite(PIN_LCD_RST, LOW);
  delay(12);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(80);

  s_bus = new Arduino_ESP32QSPI(PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2,
                                PIN_LCD_D3);
  s_gfx = new Arduino_CO5300(s_bus, PIN_LCD_RST, 0 /*rotation*/, false /*ips*/, LCD_WIDTH,
                             LCD_HEIGHT, 0, 0, 0, 0);
  if (!s_gfx->begin(40000000)) {
    Serial.println("[display] CO5300 begin failed");
    return false;
  }
  s_gfx->fillScreen(BLACK);
  displaySetBrightness(180);
  Serial.printf("[display] CO5300 %dx%d ready\n", LCD_WIDTH, LCD_HEIGHT);
  return true;
}

void displaySetBrightness(uint8_t level) {
  if (!s_gfx) return;
  s_gfx->Display_Brightness(level);
}

void displayFill(uint16_t color) {
  if (s_gfx) s_gfx->fillScreen(color);
}

void *displayDrawBuffer(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *rgb565) {
  if (!s_gfx || !rgb565) return nullptr;
  s_gfx->draw16bitRGBBitmap(x, y, const_cast<uint16_t *>(rgb565), w, h);
  return nullptr;
}

void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  if (!s_gfx) {
    lv_disp_flush_ready(disp);
    return;
  }
  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;
  s_gfx->draw16bitRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(colorP), w, h);
  lv_disp_flush_ready(disp);
}
