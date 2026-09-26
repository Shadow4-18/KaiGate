#pragma once

#include "types.h"

#include <lvgl.h>

struct ThemeColors {
  lv_color_t bg;
  lv_color_t fg;
  lv_color_t accent;
  lv_color_t accent2;
  lv_color_t warn;
  lv_color_t cold;
  lv_color_t muted;
};

ThemeColors themeColors(ThemeId id);
lv_color_t themeTachColor(ThemeId id, float t01);
