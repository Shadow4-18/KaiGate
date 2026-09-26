#include "core/safety.h"

#include "config.h"
#include "hal/audio_hal.h"
#include "hal/haptic_hal.h"

static uint32_t s_strobeUntil = 0;
static uint32_t s_lastBeepMs = 0;
static bool s_wasOver = false;
static Gear s_prevGear = Gear::Neutral;

void safetyBegin() {
  s_strobeUntil = 0;
  s_lastBeepMs = 0;
  s_wasOver = false;
}

static float coolantF(const AppState &st) { return st.telemetry.coolantC * 9.0f / 5.0f + 32.0f; }

uint16_t safetyEffectiveRedline(const AppState &st) {
  if (st.telemetry.coolantC <= 1.0f) return st.config.redlineRpm;
  if (coolantF(st) < st.config.coldCoolantF) {
    return st.config.coldLimitRpm;
  }
  return st.config.redlineRpm;
}

static float expectedRpm(const UserConfig &cfg, int gearIdx, float speedMph) {
  if (gearIdx < 1 || gearIdx > 6) return 0;
  const float r = cfg.gates[gearIdx].rpmSpeedRatio;
  if (r <= 0.00001f) return 0;
  return speedMph / r;
}

void safetyTick(AppState &st, uint32_t nowMs) {
  const uint16_t redline = safetyEffectiveRedline(st);
  st.telemetry.coldLimited = redline < st.config.redlineRpm && coolantF(st) < st.config.coldCoolantF;

  const uint16_t light = st.telemetry.coldLimited ? redline : st.config.shiftLightRpm;
  const bool over = st.telemetry.rpm >= light && light > 500;

  if (over && !s_wasOver) {
    s_strobeUntil = nowMs + SHIFT_STROBE_MS * 4;
    if (st.config.hapticEnabled) hapticDoublePulse();
    if (st.config.audioEnabled) audioShiftBeep();
    s_lastBeepMs = nowMs;
  } else if (over && (nowMs - s_lastBeepMs) > 160) {
    if (st.config.audioEnabled) audioShiftBeep();
    if (st.config.hapticEnabled) hapticClick();
    s_lastBeepMs = nowMs;
    s_strobeUntil = nowMs + SHIFT_STROBE_MS;
  }
  s_wasOver = over;
  st.telemetry.shiftStrobe = nowMs < s_strobeUntil;

  // Money-shift / over-rev trajectory: a downshift whose predicted RPM exceeds redline.
  const Gear nowG = st.telemetry.confirmedGear;
  st.telemetry.moneyShiftWarn = false;
  if (s_prevGear >= Gear::G3 && nowG >= Gear::G1 && nowG < s_prevGear) {
    const float pred = expectedRpm(st.config, static_cast<int>(nowG), st.telemetry.speedMph);
    if (pred > redline * 1.02f && st.telemetry.speedMph > 20.0f) {
      st.telemetry.moneyShiftWarn = true;
      s_strobeUntil = nowMs + 600;
      if (st.config.hapticEnabled) hapticAlert();
      if (st.config.audioEnabled) audioAlert();
    }
  }
  s_prevGear = nowG;
}

void safetyStrobeClear(AppState &st) {
  st.telemetry.shiftStrobe = false;
  st.telemetry.moneyShiftWarn = false;
}
