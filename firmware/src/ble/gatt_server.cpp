#include "ble/gatt_server.h"

#include "config.h"
#include "core/gear_engine.h"
#include "core/lock.h"
#include "core/settings.h"
#include "core/state.h"
#include "core/timers.h"
#include "hal/display_hal.h"
#include "net/wifi_transfer.h"
#include "protocol.h"

#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <string.h>

static NimBLEServer *s_server = nullptr;
static NimBLECharacteristic *s_tel = nullptr;
static NimBLECharacteristic *s_cfg = nullptr;
static NimBLECharacteristic *s_cmd = nullptr;
static NimBLECharacteristic *s_imu = nullptr;
static NimBLECharacteristic *s_dtc = nullptr;
static NimBLECharacteristic *s_status = nullptr;
static NimBLECharacteristic *s_media = nullptr;
static bool s_appConnected = false;
extern void obdClientQueryDtc();
extern void obdClientClearDtc();

static String configJson() {
  AppState &st = AppState::get();
  JsonDocument doc;
  doc["redline"] = st.config.redlineRpm;
  doc["shiftLight"] = st.config.shiftLightRpm;
  doc["coldLimit"] = st.config.coldLimitRpm;
  doc["coldCoolantF"] = st.config.coldCoolantF;
  doc["theme"] = static_cast<uint8_t>(st.config.theme);
  doc["layout"] = static_cast<uint8_t>(st.config.layout);
  doc["reverseLockoutLeft"] = st.config.reverseLockoutLeft;
  doc["brightness"] = st.config.brightness;
  doc["overlay"] = st.config.floatingGearOverlay;
  doc["haptic"] = st.config.hapticEnabled;
  doc["audio"] = st.config.audioEnabled;
  JsonArray gates = doc["gates"].to<JsonArray>();
  for (int i = 0; i < 8; ++i) {
    JsonObject g = gates.add<JsonObject>();
    g["i"] = i;
    g["p"] = st.config.gates[i].pitchDeg;
    g["r"] = st.config.gates[i].rollDeg;
    g["rad"] = st.config.gates[i].radiusDeg;
    g["ratio"] = st.config.gates[i].rpmSpeedRatio;
    g["ok"] = st.config.gates[i].valid;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

static void applyConfigJson(const char *json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  AppState &st = AppState::get();
  StateGuard g(st);
  if (doc["redline"].is<int>()) {
    int v = doc["redline"].as<int>();
    st.config.redlineRpm = static_cast<uint16_t>(constrain(v, REDLINE_MIN, REDLINE_MAX));
  }
  if (doc["shiftLight"].is<int>()) {
    int v = doc["shiftLight"].as<int>();
    st.config.shiftLightRpm = static_cast<uint16_t>(constrain(v, 2000, REDLINE_MAX));
  }
  if (doc["coldLimit"].is<int>()) st.config.coldLimitRpm = doc["coldLimit"].as<uint16_t>();
  if (doc["coldCoolantF"].is<int>()) st.config.coldCoolantF = doc["coldCoolantF"].as<uint16_t>();
  if (doc["theme"].is<int>()) st.config.theme = static_cast<ThemeId>(doc["theme"].as<uint8_t>());
  if (doc["layout"].is<int>()) st.config.layout = static_cast<TransLayout>(doc["layout"].as<uint8_t>());
  if (doc["reverseLockoutLeft"].is<bool>()) st.config.reverseLockoutLeft = doc["reverseLockoutLeft"];
  if (doc["brightness"].is<int>()) {
    st.config.brightness = doc["brightness"].as<uint8_t>();
    displaySetBrightness(st.config.brightness);
  }
  if (doc["overlay"].is<bool>()) st.config.floatingGearOverlay = doc["overlay"];
  if (doc["haptic"].is<bool>()) st.config.hapticEnabled = doc["haptic"];
  if (doc["audio"].is<bool>()) st.config.audioEnabled = doc["audio"];
  settingsSave(st.config);
}

static void applyNowPlaying(JsonDocument &doc) {
  AppState &st = AppState::get();
  StateGuard g(st);
  if (!doc["title"].isNull()) {
    strncpy(st.nowPlaying.title, doc["title"] | "Not playing", sizeof(st.nowPlaying.title) - 1);
    st.nowPlaying.title[sizeof(st.nowPlaying.title) - 1] = 0;
  }
  if (!doc["artist"].isNull()) {
    strncpy(st.nowPlaying.artist, doc["artist"] | "Phone media", sizeof(st.nowPlaying.artist) - 1);
    st.nowPlaying.artist[sizeof(st.nowPlaying.artist) - 1] = 0;
  }
  if (!doc["playing"].isNull()) st.nowPlaying.playing = doc["playing"] | false;
  if (!doc["volume"].isNull()) {
    const int v = doc["volume"].as<int>();
    st.nowPlaying.volume = static_cast<uint8_t>(constrain(v, 0, 100));
  }
}

static void handleCommand(const char *json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  const char *cmd = doc["cmd"] | "";
  AppState &st = AppState::get();

  if (!strcmp(cmd, "saveGate")) {
    const int gear = doc["gear"] | 0;
    StateGuard g(st);
    gearSaveCurrent(st.config, gear, st.imu.pitch, st.imu.roll, st.telemetry.speedMph,
                    st.telemetry.rpm);
    settingsSaveGates(st.config);
  } else if (!strcmp(cmd, "calibrateStart")) {
    StateGuard g(st);
    st.calibrating = true;
  } else if (!strcmp(cmd, "calibrateStop")) {
    StateGuard g(st);
    st.calibrating = false;
  } else if (!strcmp(cmd, "lock")) {
    StateGuard g(st);
    lockSet(st, doc["on"] | true, true);
  } else if (!strcmp(cmd, "wifiStart")) {
    wifiTransferBegin();
  } else if (!strcmp(cmd, "wifiStop")) {
    wifiTransferStop();
  } else if (!strcmp(cmd, "clearDtc")) {
    obdClientClearDtc();
  } else if (!strcmp(cmd, "queryDtc")) {
    obdClientQueryDtc();
  } else if (!strcmp(cmd, "resetPeaks")) {
    StateGuard g(st);
    timersResetPeaks(st);
  } else if (!strcmp(cmd, "resetGates")) {
    StateGuard g(st);
    settingsResetGates(st.config);
  } else if (!strcmp(cmd, "setTheme")) {
    StateGuard g(st);
    st.config.theme = static_cast<ThemeId>(doc["id"] | 0);
    settingsSave(st.config);
  } else if (!strcmp(cmd, "nowPlaying")) {
    applyNowPlaying(doc);
  }
}

class ServerCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s, NimBLEConnInfo &info) override {
    (void)s;
    (void)info;
    s_appConnected = true;
    AppState::get().telemetry.appConnected = true;
    Serial.println("[ble] app connected");
  }
  void onDisconnect(NimBLEServer *s, NimBLEConnInfo &info, int reason) override {
    (void)info;
    (void)reason;
    s_appConnected = false;
    AppState::get().telemetry.appConnected = false;
    s->startAdvertising();
    Serial.println("[ble] app disconnected");
  }
};

class CmdCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    (void)info;
    std::string v = c->getValue();
    if (!v.empty()) handleCommand(v.c_str());
  }
};

class MediaCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    (void)info;
    std::string v = c->getValue();
    if (v.empty()) return;
    JsonDocument doc;
    if (deserializeJson(doc, v.c_str())) return;
    applyNowPlaying(doc);
  }
};

class CfgCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    (void)info;
    std::string v = c->getValue();
    if (!v.empty()) applyConfigJson(v.c_str());
  }
  void onRead(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    (void)info;
    String js = configJson();
    c->setValue(js.c_str());
  }
};

static ServerCb s_serverCb;
static CmdCb s_cmdCb;
static CfgCb s_cfgCb;
static MediaCb s_mediaCb;

void gattServerBegin() {
  NimBLEDevice::init(KAIGATE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setMTU(185);
  NimBLEDevice::setSecurityAuth(false, false, false);

  s_server = NimBLEDevice::createServer();
  s_server->setCallbacks(&s_serverCb);
  s_server->advertiseOnDisconnect(true);

  NimBLEService *svc = s_server->createService(KAIGATE_SERVICE_UUID);
  s_tel = svc->createCharacteristic(KAIGATE_TELEMETRY_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  s_cfg = svc->createCharacteristic(KAIGATE_CONFIG_UUID,
                                    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  s_cmd = svc->createCharacteristic(KAIGATE_COMMAND_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  s_imu = svc->createCharacteristic(KAIGATE_IMU_UUID, NIMBLE_PROPERTY::NOTIFY);
  s_dtc = svc->createCharacteristic(KAIGATE_DTC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  s_status = svc->createCharacteristic(KAIGATE_STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  s_media = svc->createCharacteristic(
      KAIGATE_MEDIA_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
  s_cfg->setCallbacks(&s_cfgCb);
  s_cmd->setCallbacks(&s_cmdCb);
  s_media->setCallbacks(&s_mediaCb);
  s_cfg->setValue(configJson().c_str());
  svc->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->setName(KAIGATE_DEVICE_NAME);
  adv->addServiceUUID(KAIGATE_SERVICE_UUID);
  adv->enableScanResponse(true);
  adv->start();
  Serial.println("[ble] GATT server advertising as KaiGate");
}

void gattServerTick() {}

bool gattAppConnected() { return s_appConnected; }

void gattNotifyTelemetry() {
  if (!s_tel || !s_appConnected) return;
  AppState &st = AppState::get();
  TelemetryPacket p{};
  p.rpm = st.telemetry.rpm;
  p.speedX10 = static_cast<uint16_t>(st.telemetry.speedMph * 10.0f);
  p.boostX10 = static_cast<int16_t>(st.telemetry.boostKpa * 10.0f);
  p.coolantX10 = static_cast<int16_t>(st.telemetry.coolantC * 10.0f);
  p.iatX10 = static_cast<int16_t>(st.telemetry.iatC * 10.0f);
  p.voltageMv = static_cast<uint16_t>(st.telemetry.batteryV * 1000.0f);
  p.gear = static_cast<int8_t>(st.telemetry.confirmedGear);
  p.flags = 0;
  if (st.telemetry.locked) p.flags |= 0x01;
  if (st.telemetry.obdConnected) p.flags |= 0x02;
  if (st.telemetry.shiftStrobe) p.flags |= 0x04;
  if (st.telemetry.moneyShiftWarn) p.flags |= 0x08;
  if (st.telemetry.coldLimited) p.flags |= 0x10;
  if (st.telemetry.idleMascot) p.flags |= 0x20;
  p.pitchX100 = static_cast<int16_t>(st.imu.pitch * 100.0f);
  p.rollX100 = static_cast<int16_t>(st.imu.roll * 100.0f);
  p.latGX100 = static_cast<int16_t>(st.telemetry.latG * 100.0f);
  p.longGX100 = static_cast<int16_t>(st.telemetry.longG * 100.0f);
  p.knobBattPct = st.telemetry.knobBatteryPct;
  p.theme = static_cast<uint8_t>(st.config.theme);
  p.lastShiftMs = static_cast<uint16_t>(st.telemetry.lastShiftMs);
  p.zeroToSixtyX100 = static_cast<uint16_t>(st.telemetry.zeroToSixtyS * 100.0f);
  s_tel->setValue(reinterpret_cast<uint8_t *>(&p), sizeof(p));
  s_tel->notify();
}

void gattNotifyImu() {
  if (!s_imu || !s_appConnected) return;
  AppState &st = AppState::get();
  ImuPacket p{};
  p.pitchX100 = static_cast<int16_t>(st.imu.pitch * 100.0f);
  p.rollX100 = static_cast<int16_t>(st.imu.roll * 100.0f);
  p.gxX100 = static_cast<int16_t>(st.imu.gx * 100.0f);
  p.gyX100 = static_cast<int16_t>(st.imu.gy * 100.0f);
  p.gzX100 = static_cast<int16_t>(st.imu.gz * 100.0f);
  s_imu->setValue(reinterpret_cast<uint8_t *>(&p), sizeof(p));
  s_imu->notify();
}

void gattNotifyDtc() {
  if (!s_dtc) return;
  AppState &st = AppState::get();
  JsonDocument doc;
  JsonArray arr = doc["dtc"].to<JsonArray>();
  for (uint8_t i = 0; i < st.dtcCount; ++i) {
    JsonObject o = arr.add<JsonObject>();
    o["code"] = st.dtcs[i].code;
    o["text"] = st.dtcs[i].description;
  }
  String out;
  serializeJson(doc, out);
  s_dtc->setValue(out.c_str());
  if (s_appConnected) s_dtc->notify();
}

void gattNotifyStatus(const char *json) {
  if (!s_status || !json) return;
  s_status->setValue(json);
  if (s_appConnected) s_status->notify();
}

void gattNotifyMediaAction(const char *action) {
  if (!s_media || !action) return;
  AppState &st = AppState::get();
  JsonDocument doc;
  doc["action"] = action;
  doc["playing"] = st.nowPlaying.playing;
  doc["volume"] = st.nowPlaying.volume;
  doc["title"] = st.nowPlaying.title;
  doc["artist"] = st.nowPlaying.artist;
  String out;
  serializeJson(doc, out);
  s_media->setValue(out.c_str());
  if (s_appConnected) s_media->notify();
}
