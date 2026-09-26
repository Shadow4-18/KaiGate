#pragma once

#include <lvgl.h>

bool mediaBegin();
void mediaShowIdle(bool on);
void mediaDrawWallpaper(lv_obj_t *parent);
void mediaPlayBoot();
bool mediaHasIdle();
bool mediaHasWallpaper();
