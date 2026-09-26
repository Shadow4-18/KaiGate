#pragma once

#include "core/state.h"
#include "hal/imu_hal.h"
#include "types.h"

void gearEngineBegin();
Gear gearClassify(const UserConfig &cfg, float pitch, float roll);
Gear gearConfirm(const UserConfig &cfg, Gear imuGear, float speedMph, uint16_t rpm);
void gearSaveCurrent(UserConfig &cfg, int index, float pitch, float roll, float speedMph,
                     uint16_t rpm);
void gearTick(AppState &st, const ImuSample &imu, uint32_t nowMs);
