#pragma once

struct ImuSample {
  float ax = 0, ay = 0, az = 0;          // g
  float gx = 0, gy = 0, gz = 0;          // deg/s
  float pitchDeg = 0, rollDeg = 0;
  float latG = 0, longG = 0;             // vehicle axes after gravity removal
  float gyroMagDps = 0;
  bool ok = false;
};

bool imuBegin();
bool imuRead(ImuSample &out);
void imuCalibrateBias();
