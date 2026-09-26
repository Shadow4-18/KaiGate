#include <Arduino.h>
#include <LittleFS.h>

#include "ble/gatt_server.h"
#include "ble/obd_client.h"
#include "config.h"
#include "core/gear_engine.h"
#include "core/lock.h"
#include "core/safety.h"
#include "core/settings.h"
#include "core/state.h"
#include "core/timers.h"
#include "hal/audio_hal.h"
#include "hal/battery_hal.h"
#include "hal/display_hal.h"
#include "hal/haptic_hal.h"
#include "hal/i2c_bus.h"
#include "hal/imu_hal.h"
#include "hal/touch_hal.h"
#include "media/media_player.h"
#include "net/wifi_transfer.h"
#include "ui/ui.h"

static SemaphoreHandle_t s_lvglMux;

static void sensorTask(void *) {
  ImuSample imu;
  uint32_t lastTel = 0, lastImuN = 0;
  for (;;) {
    const uint32_t now = millis();
    imuRead(imu);
    AppState &st = AppState::get();
    {
      StateGuard g(st);
      st.imu.pitch = imu.pitchDeg;
      st.imu.roll = imu.rollDeg;
      st.imu.gx = imu.gx;
      st.imu.gy = imu.gy;
      st.imu.gz = imu.gz;
      st.imu.latG = imu.latG;
      st.imu.longG = imu.longG;
      st.imu.gyroMag = imu.gyroMagDps;
      st.telemetry.pitchDeg = imu.pitchDeg;
      st.telemetry.rollDeg = imu.rollDeg;
      st.telemetry.latG = imu.latG;
      st.telemetry.longG = imu.longG;
      st.telemetry.knobBatteryV = batteryVoltage();
      st.telemetry.knobBatteryPct = batteryPercent();
      gearTick(st, imu, now);
      timersTick(st, now);
      safetyTick(st, now);

      const bool movingStick = imu.gyroMagDps > STICK_WAKE_GYRO_DPS;
      if (movingStick) st.lastMotionMs = now;
      const bool stationary = st.telemetry.speedMph <= STATIONARY_SPEED_MPH &&
                              st.telemetry.confirmedGear == Gear::Neutral &&
                              st.telemetry.rpm < IDLE_RPM_MAX;
      if (!stationary) st.stationarySinceMs = now;
      if (stationary && (now - st.stationarySinceMs) > IDLE_MASCOT_MS && mediaHasIdle() &&
          !movingStick) {
        st.telemetry.idleMascot = true;
      }
      if (movingStick || st.telemetry.speedMph > STATIONARY_SPEED_MPH ||
          st.telemetry.confirmedGear != Gear::Neutral) {
        st.telemetry.idleMascot = false;
      }
    }

    if (now - lastTel >= 100) {
      lastTel = now;
      gattNotifyTelemetry();
    }
    if (st.calibrating && now - lastImuN >= (1000 / CAL_IMU_HZ)) {
      lastImuN = now;
      gattNotifyImu();
    }
    vTaskDelay(pdMS_TO_TICKS(1000 / IMU_HZ));
  }
}

static void obdTask(void *) {
  for (;;) {
    obdClientTick();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static void uiTask(void *) {
  uint32_t last = 0;
  for (;;) {
    const uint32_t now = millis();
    TouchPoint tp;
    touchRead(tp);
    lockOnTouch(tp.pressed, now);
    if (tp.pressed) AppState::get().lastInputMs = now;
    {
      StateGuard g(AppState::get());
      lockTick(g.get(), now);
    }
    audioTick(now);
    wifiTransferTick();

    if (xSemaphoreTake(s_lvglMux, pdMS_TO_TICKS(20)) == pdTRUE) {
      uiTick(now);
      xSemaphoreGive(s_lvglMux);
    }
    const uint32_t dt = millis() - last;
    last = millis();
    const int slack = static_cast<int>(1000 / DISPLAY_HZ) - static_cast<int>(dt);
    vTaskDelay(pdMS_TO_TICKS(slack > 5 ? slack : 5));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== KaiGate Shift Knob " KAIGATE_FW_VERSION " ===");

  s_lvglMux = xSemaphoreCreateMutex();
  settingsBegin();
  AppState &st = AppState::get();
  settingsLoad(st.config);
  st.telemetry.locked = true;

  i2cBusBegin();
  batteryBegin();
  displayBegin();
  displaySetBrightness(st.config.brightness);
  touchBegin();
  imuBegin();
  hapticBegin();
  audioBegin();
  mediaBegin();
  mediaPlayBoot();

  gearEngineBegin();
  lockLogicBegin();
  safetyBegin();
  timersBegin(st);

  uiBegin();
  gattServerBegin();
  obdClientBegin();

  xTaskCreatePinnedToCore(sensorTask, "sensor", 6144, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(obdTask, "obd", 8192, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(uiTask, "ui", 12288, nullptr, 3, nullptr, 1);
  Serial.println("[boot] tasks running");
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
