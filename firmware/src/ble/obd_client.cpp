#include "ble/obd_client.h"

#include "ble/gatt_server.h"
#include "core/obd.h"
#include "core/state.h"

#include <NimBLEDevice.h>
#include <string.h>

static NimBLEClient *s_client = nullptr;
static NimBLERemoteCharacteristic *s_rx = nullptr;
static NimBLERemoteCharacteristic *s_tx = nullptr;
static bool s_connected = false;
static uint32_t s_lastScanMs = 0;
static uint32_t s_lastPollMs = 0;
static int s_pidIndex = 0;
static String s_line;
static bool s_wantDtc = false;
static bool s_wantClear = false;
static uint32_t s_initStepMs = 0;
static int s_initStep = 0;

static const char *kInit[] = {"ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP0"};

static bool nameLooksLikeObd(const std::string &name) {
  auto has = [&](const char *s) {
    return name.find(s) != std::string::npos;
  };
  return has("Vgate") || has("vgate") || has("iCar") || has("ICAR") || has("OBD") || has("OBDII") ||
         has("ELM") || has("LELink") || has("Kiwi");
}

static bool bindSerialChars(NimBLEClient *c) {
  static const char *kSvc[] = {"fff0", "ffe0", "18f0", "e7810a71-73ae-499d-8c42-d7ea0d1ff0a3"};
  static const char *kNtf[] = {"fff1", "ffe1", "2af0", "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f"};
  static const char *kWr[] = {"fff2", "ffe1", "2af1", "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f"};

  for (int i = 0; i < 4; ++i) {
    NimBLERemoteService *svc = c->getService(kSvc[i]);
    if (!svc) continue;
    auto *ntf = svc->getCharacteristic(kNtf[i]);
    auto *wr = svc->getCharacteristic(kWr[i]);
    if (!ntf) ntf = svc->getCharacteristic("fff1");
    if (!wr) wr = svc->getCharacteristic("fff2");
    if (ntf && wr) {
      s_rx = ntf;
      s_tx = wr;
      return true;
    }
  }

  auto svcs = c->getServices(true);
  for (auto *svc : svcs) {
    auto chars = svc->getCharacteristics(true);
    NimBLERemoteCharacteristic *ntf = nullptr, *wr = nullptr;
    for (auto *ch : chars) {
      if (ch->canNotify() || ch->canIndicate()) ntf = ch;
      if (ch->canWrite() || ch->canWriteNoResponse()) wr = ch;
    }
    if (ntf && wr) {
      s_rx = ntf;
      s_tx = wr;
      return true;
    }
  }
  return false;
}

static void applyLine(const String &line) {
  AppState &st = AppState::get();
  uint16_t rpm = st.telemetry.rpm;
  float spd = st.telemetry.speedMph, clt = st.telemetry.coolantC, mapv = 0, iat = st.telemetry.iatC,
        bat = st.telemetry.batteryV;
  if (obdParseLine(line.c_str(), rpm, spd, clt, mapv, iat, bat)) {
    StateGuard g(st);
    st.telemetry.rpm = rpm;
    st.telemetry.speedMph = spd;
    st.telemetry.coolantC = clt;
    if (mapv > 0) st.telemetry.boostKpa = mapv - 101.325f;
    st.telemetry.iatC = iat;
    if (bat > 0) st.telemetry.batteryV = bat;
  }
  if (line.indexOf("43") >= 0 || line.indexOf("43 ") >= 0) {
    char tmp[16][8];
    uint8_t n = 0;
    obdDecodeDtcPayload(line.c_str(), tmp, n, 16);
    StateGuard g(st);
    st.dtcCount = n;
    for (uint8_t i = 0; i < n; ++i) {
      strncpy(st.dtcs[i].code, tmp[i], sizeof(st.dtcs[i].code) - 1);
      st.dtcs[i].description[0] = 0;
    }
    gattNotifyDtc();
  }
}

static void onNotify(NimBLERemoteCharacteristic *c, uint8_t *data, size_t len, bool isNotify) {
  (void)c;
  (void)isNotify;
  for (size_t i = 0; i < len; ++i) {
    char ch = static_cast<char>(data[i]);
    if (ch == '>') {
      applyLine(s_line);
      s_line = "";
    } else if (ch != '\0') {
      s_line += ch;
      if (s_line.length() > 240) s_line.remove(0, 80);
    }
  }
}

static void writeCmd(const char *cmd) {
  if (!s_tx || !cmd) return;
  char buf[24];
  snprintf(buf, sizeof(buf), "%s\r", cmd);
  s_tx->writeValue(reinterpret_cast<uint8_t *>(buf), strlen(buf), false);
}

class ClientCb : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *c) override {
    (void)c;
    Serial.println("[obd] connected");
  }
  void onDisconnect(NimBLEClient *c, int reason) override {
    (void)c;
    (void)reason;
    s_connected = false;
    s_rx = s_tx = nullptr;
    AppState::get().telemetry.obdConnected = false;
    Serial.println("[obd] disconnected");
  }
};

static ClientCb s_clientCb;

static void startScan() {
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(140);
  scan->setWindow(80);
  scan->setMaxResults(0);
  scan->setDuplicateFilter(true);
  scan->start(3, false, true);
}

static bool tryConnect() {
  NimBLEScan *scan = NimBLEDevice::getScan();
  auto results = scan->getResults();
  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice *d = results.getDevice(i);
    const std::string name = d->getName();
    if (!nameLooksLikeObd(name) && !d->isAdvertisingService(NimBLEUUID("fff0")) &&
        !d->isAdvertisingService(NimBLEUUID("ffe0"))) {
      continue;
    }
    if (s_client) {
      NimBLEDevice::deleteClient(s_client);
      s_client = nullptr;
    }
    s_client = NimBLEDevice::createClient();
    s_client->setClientCallbacks(&s_clientCb, false);
    s_client->setConnectTimeout(5);
    Serial.printf("[obd] connecting to %s\n", name.c_str());
    if (!s_client->connect(d)) continue;
    if (!bindSerialChars(s_client)) {
      s_client->disconnect();
      continue;
    }
    if (s_rx->canNotify()) s_rx->subscribe(true, onNotify);
    s_connected = true;
    s_initStep = 0;
    s_initStepMs = millis();
    AppState::get().telemetry.obdConnected = true;
    scan->stop();
    return true;
  }
  return false;
}

void obdClientBegin() {
  s_lastScanMs = 0;
  Serial.println("[obd] BLE client ready (Vgate iCar Pro / ELM327)");
}

void obdClientQueryDtc() { s_wantDtc = true; }
void obdClientClearDtc() { s_wantClear = true; }
bool obdClientConnected() { return s_connected; }

void obdClientTick() {
  const uint32_t now = millis();
  if (!s_connected) {
    if (now - s_lastScanMs > 4000) {
      s_lastScanMs = now;
      startScan();
      tryConnect();
    }
    return;
  }
  if (s_initStep < 6) {
    if (now - s_initStepMs > 400) {
      writeCmd(kInit[s_initStep++]);
      s_initStepMs = now;
    }
    return;
  }
  if (s_wantClear) {
    s_wantClear = false;
    writeCmd("04");
    return;
  }
  if (s_wantDtc) {
    s_wantDtc = false;
    writeCmd("03");
    return;
  }
  if (now - s_lastPollMs >= 100) {
    s_lastPollMs = now;
    writeCmd(obdPidCommand(s_pidIndex++));
  }
}
