#pragma once

// Shared BLE contract between ESP32 firmware and the KaiGate Flutter app.
// Keep this file and app/kaigate/lib/ble/protocol.dart in lock-step.

#define KAIGATE_SERVICE_UUID        "8f400001-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_TELEMETRY_UUID      "8f400002-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_CONFIG_UUID         "8f400003-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_COMMAND_UUID        "8f400004-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_IMU_UUID            "8f400005-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_DTC_UUID            "8f400006-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_STATUS_UUID         "8f400007-7a3b-4c2d-9e1f-0a1b2c3d4e5f"
#define KAIGATE_MEDIA_UUID          "8f400008-7a3b-4c2d-9e1f-0a1b2c3d4e5f"

// Packed little-endian telemetry (28 bytes). Flutter unpacks the same layout.
#ifdef __cplusplus
#include <stdint.h>
struct __attribute__((packed)) TelemetryPacket {
  uint16_t rpm;
  uint16_t speedX10;      // mph * 10
  int16_t boostX10;       // kPa * 10
  int16_t coolantX10;     // °C * 10
  int16_t iatX10;         // °C * 10
  uint16_t voltageMv;     // vehicle voltage
  int8_t gear;
  uint8_t flags;          // bit0 lock, bit1 obd, bit2 strobe, bit3 money, bit4 cold, bit5 mascot
  int16_t pitchX100;
  int16_t rollX100;
  int16_t latGX100;
  int16_t longGX100;
  uint8_t knobBattPct;
  uint8_t theme;
  uint16_t lastShiftMs;
  uint16_t zeroToSixtyX100;  // seconds * 100
};
static_assert(sizeof(TelemetryPacket) == 28, "TelemetryPacket size");

struct __attribute__((packed)) ImuPacket {
  int16_t pitchX100;
  int16_t rollX100;
  int16_t gxX100;
  int16_t gyX100;
  int16_t gzX100;
};
#endif

// Command JSON examples written to KAIGATE_COMMAND_UUID:
// {"cmd":"saveGate","gear":1}
// {"cmd":"calibrateStart"}
// {"cmd":"calibrateStop"}
// {"cmd":"clearDtc"}
// {"cmd":"queryDtc"}
// {"cmd":"wifiStart"}
// {"cmd":"wifiStop"}
// {"cmd":"lock","on":true}
// {"cmd":"resetPeaks"}
// {"cmd":"setTheme","id":0}
// {"cmd":"nowPlaying","title":"...","artist":"...","playing":true,"volume":70}
