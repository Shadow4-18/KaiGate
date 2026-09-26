#include "core/gear_engine.h"

#include "config.h"

#include <math.h>

static Gear s_prev = Gear::Neutral;
static uint32_t s_lastChangeMs = 0;

void gearEngineBegin() {
  s_prev = Gear::Neutral;
  s_lastChangeMs = millis();
}

static float dist(const GateCluster &g, float pitch, float roll) {
  const float dp = pitch - g.pitchDeg;
  const float dr = roll - g.rollDeg;
  return sqrtf(dp * dp + dr * dr);
}

Gear gearClassify(const UserConfig &cfg, float pitch, float roll) {
  int best = -1;
  float bestD = 1e9f;
  const int maxG = maxGear(cfg.layout);
  for (int i = 0; i < 8; ++i) {
    if (i > maxG && i < 7) continue;
    if (!cfg.gates[i].valid) continue;
    const float d = dist(cfg.gates[i], pitch, roll);
    if (d < cfg.gates[i].radiusDeg && d < bestD) {
      bestD = d;
      best = i;
    }
  }
  if (best < 0) return Gear::Unknown;
  return indexToGear(best);
}

Gear gearConfirm(const UserConfig &cfg, Gear imuGear, float speedMph, uint16_t rpm) {
  if (rpm < 400) return Gear::Neutral;
  if (speedMph < STATIONARY_SPEED_MPH && rpm < IDLE_RPM_MAX) {
    if (imuGear == Gear::Neutral || imuGear == Gear::Unknown) return Gear::Neutral;
  }

  const float ratio = speedMph / static_cast<float>(rpm < 1 ? 1 : rpm);
  int bestRatio = -1;
  float bestErr = 1e9f;
  const int maxG = maxGear(cfg.layout);
  for (int i = 1; i <= maxG; ++i) {
    if (!cfg.gates[i].valid || cfg.gates[i].rpmSpeedRatio <= 0.00001f) continue;
    const float err = fabsf(ratio - cfg.gates[i].rpmSpeedRatio) / cfg.gates[i].rpmSpeedRatio;
    if (err < bestErr) {
      bestErr = err;
      bestRatio = i;
    }
  }

  if (imuGear == Gear::Reverse) return Gear::Reverse;
  if (bestRatio > 0 && bestErr < 0.18f) {
    const Gear ratioGear = static_cast<Gear>(bestRatio);
    if (imuGear == Gear::Unknown) return ratioGear;
    if (imuGear == ratioGear) return ratioGear;
    // During a shift the ratio lags; trust IMU while RPM is collapsing or climbing fast.
    return imuGear;
  }
  return imuGear == Gear::Unknown ? Gear::Neutral : imuGear;
}

void gearSaveCurrent(UserConfig &cfg, int index, float pitch, float roll, float speedMph,
                     uint16_t rpm) {
  if (index < 0 || index > 7) return;
  cfg.gates[index].pitchDeg = pitch;
  cfg.gates[index].rollDeg = roll;
  cfg.gates[index].radiusDeg = GATE_RADIUS_DEG_DEFAULT;
  if (index >= 1 && index <= 6 && rpm > 800 && speedMph > 3.0f) {
    cfg.gates[index].rpmSpeedRatio = speedMph / static_cast<float>(rpm);
  }
  cfg.gates[index].valid = true;
}

void gearTick(AppState &st, const ImuSample &imu, uint32_t nowMs) {
  const Gear imuGear = gearClassify(st.config, imu.pitchDeg, imu.rollDeg);
  const Gear confirmed = gearConfirm(st.config, imuGear, st.telemetry.speedMph, st.telemetry.rpm);
  if (confirmed != s_prev && confirmed != Gear::Unknown) {
    s_lastChangeMs = nowMs;
    s_prev = confirmed;
  }
  st.telemetry.gear = imuGear;
  st.telemetry.confirmedGear = (confirmed == Gear::Unknown) ? s_prev : confirmed;
}
