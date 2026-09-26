#pragma once

#include "core/state.h"

void timersBegin(AppState &st);
void timersTick(AppState &st, uint32_t nowMs);
void timersResetPeaks(AppState &st);
