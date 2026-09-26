#pragma once

#include "core/state.h"

void safetyBegin();
uint16_t safetyEffectiveRedline(const AppState &st);
void safetyTick(AppState &st, uint32_t nowMs);
void safetyStrobeClear(AppState &st);
