#include "core/settings.h"

#include "config.h"

#include <Preferences.h>
#include <string.h>

static Preferences s_prefs;

bool settingsBegin() { return s_prefs.begin("kaigate", false); }

static void writeGate(const char *key, const GateCluster &g) {
  float blob[4] = {g.pitchDeg, g.rollDeg, g.radiusDeg, g.rpmSpeedRatio};
  s_prefs.putBytes(key, blob, sizeof(blob));
  char vk[12];
  snprintf(vk, sizeof(vk), "%sv", key);
  s_prefs.putBool(vk, g.valid);
}

static void readGate(const char *key, GateCluster &g) {
  float blob[4] = {};
  if (s_prefs.getBytes(key, blob, sizeof(blob)) == sizeof(blob)) {
    g.pitchDeg = blob[0];
    g.rollDeg = blob[1];
    g.radiusDeg = blob[2];
    g.rpmSpeedRatio = blob[3];
  }
  char vk[12];
  snprintf(vk, sizeof(vk), "%sv", key);
  g.valid = s_prefs.getBool(vk, false);
}

void settingsLoad(UserConfig &cfg) {
  cfg.redlineRpm = s_prefs.getUShort("redline", REDLINE_DEFAULT);
  cfg.shiftLightRpm = s_prefs.getUShort("shift", SHIFT_LIGHT_DEFAULT);
  cfg.coldLimitRpm = s_prefs.getUShort("coldrpm", COLD_SHIFT_RPM_DEFAULT);
  cfg.coldCoolantF = s_prefs.getUShort("coldf", COLD_COOLANT_F_DEFAULT);
  cfg.theme = static_cast<ThemeId>(s_prefs.getUChar("theme", 0));
  cfg.layout = static_cast<TransLayout>(s_prefs.getUChar("layout", 1));
  cfg.reverseLockoutLeft = s_prefs.getBool("rlock", true);
  cfg.brightness = s_prefs.getUChar("bright", 180);
  cfg.floatingGearOverlay = s_prefs.getBool("overlay", true);
  cfg.hapticEnabled = s_prefs.getBool("haptic", true);
  cfg.audioEnabled = s_prefs.getBool("audio", true);
  const char *keys[8] = {"gN", "g1", "g2", "g3", "g4", "g5", "g6", "gR"};
  for (int i = 0; i < 8; ++i) readGate(keys[i], cfg.gates[i]);
}

void settingsSave(const UserConfig &cfg) {
  s_prefs.putUShort("redline", cfg.redlineRpm);
  s_prefs.putUShort("shift", cfg.shiftLightRpm);
  s_prefs.putUShort("coldrpm", cfg.coldLimitRpm);
  s_prefs.putUShort("coldf", cfg.coldCoolantF);
  s_prefs.putUChar("theme", static_cast<uint8_t>(cfg.theme));
  s_prefs.putUChar("layout", static_cast<uint8_t>(cfg.layout));
  s_prefs.putBool("rlock", cfg.reverseLockoutLeft);
  s_prefs.putUChar("bright", cfg.brightness);
  s_prefs.putBool("overlay", cfg.floatingGearOverlay);
  s_prefs.putBool("haptic", cfg.hapticEnabled);
  s_prefs.putBool("audio", cfg.audioEnabled);
  settingsSaveGates(cfg);
}

void settingsSaveGates(const UserConfig &cfg) {
  const char *keys[8] = {"gN", "g1", "g2", "g3", "g4", "g5", "g6", "gR"};
  for (int i = 0; i < 8; ++i) writeGate(keys[i], cfg.gates[i]);
}

void settingsSaveBests(float best060, float bestShiftMs) {
  s_prefs.putFloat("b060", best060);
  s_prefs.putFloat("bsh", bestShiftMs);
}

void settingsLoadBests(float &best060, float &bestShiftMs) {
  best060 = s_prefs.getFloat("b060", 0);
  bestShiftMs = s_prefs.getFloat("bsh", 0);
}

void settingsResetGates(UserConfig &cfg) {
  for (auto &g : cfg.gates) g = GateCluster{};
  settingsSaveGates(cfg);
}
