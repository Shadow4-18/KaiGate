#include "core/lock.h"

#include "config.h"
#include "hal/haptic_hal.h"

static bool s_down = false;
static uint32_t s_downAt = 0;
static bool s_fired = false;

void lockLogicBegin() {
  s_down = false;
  s_downAt = 0;
  s_fired = false;
}

void lockOnTouch(bool pressed, uint32_t nowMs) {
  if (pressed && !s_down) {
    s_down = true;
    s_downAt = nowMs;
    s_fired = false;
  } else if (!pressed) {
    s_down = false;
    s_fired = false;
  }
}

bool lockConsumeLongPress(uint32_t nowMs) {
  if (s_down && !s_fired && (nowMs - s_downAt) >= LOCK_LONG_PRESS_MS) {
    s_fired = true;
    return true;
  }
  return false;
}

void lockSet(AppState &st, bool locked, bool haptic) {
  if (st.telemetry.locked == locked) return;
  st.telemetry.locked = locked;
  if (haptic && st.config.hapticEnabled) hapticClick();
}

bool lockAllowsSwipe(const AppState &st) {
  if (st.telemetry.locked) return false;
  const bool moving = st.telemetry.speedMph > STATIONARY_SPEED_MPH;
  const Gear g = st.telemetry.confirmedGear;
  const bool inGear = g != Gear::Neutral && g != Gear::Unknown;
  if (moving || inGear) return false;
  return true;
}

void lockTick(AppState &st, uint32_t nowMs) {
  if (lockConsumeLongPress(nowMs)) {
    lockSet(st, !st.telemetry.locked, true);
    st.lastInputMs = nowMs;
  }

  const bool stationary = st.telemetry.speedMph <= STATIONARY_SPEED_MPH;
  if (!st.telemetry.locked && stationary &&
      (nowMs - st.lastInputMs) >= LOCK_IDLE_RELOCK_MS) {
    lockSet(st, true, false);
  }

  st.swipeAllowed = lockAllowsSwipe(st);
}
