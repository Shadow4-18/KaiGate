#pragma once

#include "types.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct ImuLive {
  float pitch = 0, roll = 0;
  float gx = 0, gy = 0, gz = 0;
  float latG = 0, longG = 0;
  float gyroMag = 0;
};

class AppState {
 public:
  static AppState &get();

  void lock();
  void unlock();

  Telemetry telemetry;
  UserConfig config;
  ImuLive imu;
  bool calibrating = false;
  int calibrateTarget = -1;
  bool wifiApActive = false;
  char wifiIp[16] = "0.0.0.0";
  DtcCode dtcs[16];
  uint8_t dtcCount = 0;
  ScreenId screen = ScreenId::Tach;
  uint32_t lastInputMs = 0;
  uint32_t lastMotionMs = 0;
  uint32_t stationarySinceMs = 0;
  bool swipeAllowed = true;
  NowPlaying nowPlaying;

 private:
  AppState();
  SemaphoreHandle_t mutex_;
};

class StateGuard {
 public:
  explicit StateGuard(AppState &s) : s_(s) { s_.lock(); }
  ~StateGuard() { s_.unlock(); }
  AppState *operator->() { return &s_; }
  AppState &get() { return s_; }

 private:
  AppState &s_;
};
