#pragma once

#include "core/state.h"

void lockLogicBegin();
void lockOnTouch(bool pressed, uint32_t nowMs);
bool lockConsumeLongPress(uint32_t nowMs);
void lockTick(AppState &st, uint32_t nowMs);
bool lockAllowsSwipe(const AppState &st);
void lockSet(AppState &st, bool locked, bool haptic);
