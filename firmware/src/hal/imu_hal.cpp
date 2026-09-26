#include "hal/imu_hal.h"

#include "hal/i2c_bus.h"
#include "pins.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

static Adafruit_MPU6050 s_mpu;
static bool s_ok = false;
static float s_pitch = 0;
static float s_roll = 0;
static float s_biasGx = 0, s_biasGy = 0, s_biasGz = 0;
static uint32_t s_lastUs = 0;

bool imuBegin() {
  if (!i2cLock(100)) return false;
  s_ok = s_mpu.begin(ADDR_MPU6050, &i2cBus(), 0);
  if (s_ok) {
    s_mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    s_mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  i2cUnlock();
  if (!s_ok) {
    Serial.println("[imu] MPU-6050 not found at 0x68");
    return false;
  }
  imuCalibrateBias();
  Serial.println("[imu] MPU-6050 ready");
  return true;
}

void imuCalibrateBias() {
  if (!s_ok) return;
  float sx = 0, sy = 0, sz = 0;
  const int n = 64;
  for (int i = 0; i < n; ++i) {
    sensors_event_t a, g, t;
    if (!i2cLock()) continue;
    s_mpu.getEvent(&a, &g, &t);
    i2cUnlock();
    sx += g.gyro.x;
    sy += g.gyro.y;
    sz += g.gyro.z;
    delay(8);
  }
  const float r2d = 180.0f / PI;
  s_biasGx = (sx / n) * r2d;
  s_biasGy = (sy / n) * r2d;
  s_biasGz = (sz / n) * r2d;
  s_lastUs = micros();
}

bool imuRead(ImuSample &out) {
  out.ok = false;
  if (!s_ok) return false;
  sensors_event_t a, g, t;
  if (!i2cLock()) return false;
  const bool ok = s_mpu.getEvent(&a, &g, &t);
  i2cUnlock();
  if (!ok) return false;

  const float r2d = 180.0f / PI;
  out.ax = a.acceleration.x / 9.80665f;
  out.ay = a.acceleration.y / 9.80665f;
  out.az = a.acceleration.z / 9.80665f;
  out.gx = g.gyro.x * r2d - s_biasGx;
  out.gy = g.gyro.y * r2d - s_biasGy;
  out.gz = g.gyro.z * r2d - s_biasGz;
  out.gyroMagDps = sqrtf(out.gx * out.gx + out.gy * out.gy + out.gz * out.gz);

  const uint32_t now = micros();
  float dt = (now - s_lastUs) / 1000000.0f;
  if (dt <= 0 || dt > 0.1f) dt = 0.01f;
  s_lastUs = now;

  const float accPitch = atan2f(-out.ax, sqrtf(out.ay * out.ay + out.az * out.az)) * r2d;
  const float accRoll = atan2f(out.ay, out.az) * r2d;
  const float alpha = 0.96f;
  s_pitch = alpha * (s_pitch + out.gy * dt) + (1.0f - alpha) * accPitch;
  s_roll = alpha * (s_roll + out.gx * dt) + (1.0f - alpha) * accRoll;
  out.pitchDeg = s_pitch;
  out.rollDeg = s_roll;

  // Subtract gravity in the sensor frame using the complementary angles, then
  // map remaining specific force onto vehicle lateral/longitudinal axes.
  const float pr = s_pitch * (PI / 180.0f);
  const float rr = s_roll * (PI / 180.0f);
  const float gxG = sinf(pr);
  const float gyG = -sinf(rr) * cosf(pr);
  const float gzG = -cosf(rr) * cosf(pr);
  const float lx = out.ax - gxG;
  const float ly = out.ay - gyG;
  const float lz = out.az - gzG;
  (void)lz;
  out.latG = ly;
  out.longG = -lx;
  out.ok = true;
  return true;
}
