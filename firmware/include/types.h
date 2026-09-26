#pragma once

#include <stdint.h>

enum class Gear : int8_t {
  Unknown = -2,
  Reverse = -1,
  Neutral = 0,
  G1 = 1,
  G2 = 2,
  G3 = 3,
  G4 = 4,
  G5 = 5,
  G6 = 6,
};

enum class TransLayout : uint8_t {
  Speed5 = 0,
  Speed6 = 1,
  Dogleg = 2,
};

enum class ThemeId : uint8_t {
  JdmAmber = 0,
  EuroSport = 1,
  Mono = 2,
  Cyberpunk = 3,
};

enum class ScreenId : uint8_t {
  Tach = 0,
  Vitals = 1,
  GForce = 2,
  Timers = 3,
  Media = 4,
  MediaControl = 5,
  Dtc = 6,
  Count = 7,
};

struct GateCluster {
  float pitchDeg = 0;
  float rollDeg = 0;
  float radiusDeg = 9.0f;
  float rpmSpeedRatio = 0;  // mph / rpm, 0 = unused
  bool valid = false;
};

struct Telemetry {
  uint16_t rpm = 0;
  float speedMph = 0;
  float boostKpa = 0;
  float boostPeakKpa = 0;
  float coolantC = 0;
  float iatC = 0;
  float batteryV = 0;
  float knobBatteryV = 0;
  uint8_t knobBatteryPct = 0;
  Gear gear = Gear::Neutral;
  Gear confirmedGear = Gear::Neutral;
  float pitchDeg = 0;
  float rollDeg = 0;
  float latG = 0;
  float longG = 0;
  float latGPeak = 0;
  float longGPeak = 0;
  float zeroToSixtyS = 0;
  float lastShiftMs = 0;
  float bestZeroToSixtyS = 0;
  float bestShiftMs = 0;
  bool obdConnected = false;
  bool appConnected = false;
  bool locked = true;
  bool idleMascot = false;
  bool shiftStrobe = false;
  bool moneyShiftWarn = false;
  bool coldLimited = false;
};

struct UserConfig {
  uint16_t redlineRpm = 7200;
  uint16_t shiftLightRpm = 6800;
  uint16_t coldLimitRpm = 3500;
  uint16_t coldCoolantF = 180;
  ThemeId theme = ThemeId::JdmAmber;
  TransLayout layout = TransLayout::Speed6;
  bool reverseLockoutLeft = true;
  uint8_t brightness = 180;
  bool floatingGearOverlay = true;
  bool hapticEnabled = true;
  bool audioEnabled = true;
  GateCluster gates[8];  // index 0 = N, 1-6, 7 = R
};

struct NowPlaying {
  char title[48] = "Not playing";
  char artist[32] = "Phone media";
  bool playing = false;
  uint8_t volume = 50;
};

struct DtcCode {
  char code[8];
  char description[96];
};

inline int gearToIndex(Gear g) {
  if (g == Gear::Reverse) return 7;
  if (g == Gear::Neutral) return 0;
  if (static_cast<int>(g) >= 1 && static_cast<int>(g) <= 6) return static_cast<int>(g);
  return -1;
}

inline Gear indexToGear(int idx) {
  if (idx == 7) return Gear::Reverse;
  if (idx == 0) return Gear::Neutral;
  if (idx >= 1 && idx <= 6) return static_cast<Gear>(idx);
  return Gear::Unknown;
}

inline const char *gearLabel(Gear g) {
  switch (g) {
    case Gear::Reverse: return "R";
    case Gear::Neutral: return "N";
    case Gear::G1: return "1";
    case Gear::G2: return "2";
    case Gear::G3: return "3";
    case Gear::G4: return "4";
    case Gear::G5: return "5";
    case Gear::G6: return "6";
    default: return "-";
  }
}

inline int maxGear(TransLayout layout) {
  return layout == TransLayout::Speed5 ? 5 : 6;
}
