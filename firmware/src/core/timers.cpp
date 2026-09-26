#include "core/timers.h"

#include "core/settings.h"

#include <math.h>

static bool s_runArmed = false;
static bool s_runActive = false;
static uint32_t s_runStartMs = 0;
static Gear s_shiftFrom = Gear::Neutral;
static uint32_t s_shiftStartMs = 0;
static uint32_t s_peakBoostUntil = 0;

void timersBegin(AppState &st) {
  settingsLoadBests(st.telemetry.bestZeroToSixtyS, st.telemetry.bestShiftMs);
  s_runArmed = true;
}

void timersResetPeaks(AppState &st) {
  st.telemetry.latGPeak = 0;
  st.telemetry.longGPeak = 0;
  st.telemetry.boostPeakKpa = st.telemetry.boostKpa;
}

void timersTick(AppState &st, uint32_t nowMs) {
  const float spd = st.telemetry.speedMph;

  if (spd < 1.0f) {
    s_runArmed = true;
    s_runActive = false;
    st.telemetry.zeroToSixtyS = 0;
  } else if (s_runArmed && spd >= 1.0f && !s_runActive) {
    s_runArmed = false;
    s_runActive = true;
    s_runStartMs = nowMs;
  }
  if (s_runActive) {
    st.telemetry.zeroToSixtyS = (nowMs - s_runStartMs) / 1000.0f;
    if (spd >= 60.0f) {
      s_runActive = false;
      if (st.telemetry.bestZeroToSixtyS <= 0.05f ||
          st.telemetry.zeroToSixtyS < st.telemetry.bestZeroToSixtyS) {
        st.telemetry.bestZeroToSixtyS = st.telemetry.zeroToSixtyS;
        settingsSaveBests(st.telemetry.bestZeroToSixtyS, st.telemetry.bestShiftMs);
      }
    }
  }

  const Gear g = st.telemetry.confirmedGear;
  if (g != s_shiftFrom && g >= Gear::G1 && s_shiftFrom >= Gear::G1) {
    if (s_shiftStartMs > 0) {
      st.telemetry.lastShiftMs = static_cast<float>(nowMs - s_shiftStartMs);
      if (st.telemetry.bestShiftMs <= 1.0f || st.telemetry.lastShiftMs < st.telemetry.bestShiftMs) {
        st.telemetry.bestShiftMs = st.telemetry.lastShiftMs;
        settingsSaveBests(st.telemetry.bestZeroToSixtyS, st.telemetry.bestShiftMs);
      }
    }
    s_shiftStartMs = nowMs;
  } else if (g != s_shiftFrom) {
    s_shiftStartMs = nowMs;
  }
  s_shiftFrom = g;

  if (fabsf(st.telemetry.latG) > fabsf(st.telemetry.latGPeak)) st.telemetry.latGPeak = st.telemetry.latG;
  if (fabsf(st.telemetry.longG) > fabsf(st.telemetry.longGPeak))
    st.telemetry.longGPeak = st.telemetry.longG;

  if (st.telemetry.boostKpa >= st.telemetry.boostPeakKpa) {
    st.telemetry.boostPeakKpa = st.telemetry.boostKpa;
    s_peakBoostUntil = nowMs + 3000;
  } else if (nowMs > s_peakBoostUntil) {
    st.telemetry.boostPeakKpa += (st.telemetry.boostKpa - st.telemetry.boostPeakKpa) * 0.02f;
  }
}
