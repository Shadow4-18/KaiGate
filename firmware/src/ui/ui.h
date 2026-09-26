#pragma once

#include <lvgl.h>

void uiBegin();
void uiTick(uint32_t nowMs);
void uiLockIndev(bool allowSwipe);
void uiOnThemeChanged();
