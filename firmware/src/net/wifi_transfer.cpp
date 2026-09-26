#include "net/wifi_transfer.h"

#include "ble/gatt_server.h"
#include "config.h"
#include "core/state.h"

#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

static WebServer s_http(80);
static File s_file;
static bool s_active = false;
static size_t s_written = 0;

static void sendPortal() {
  s_http.send(200, "text/html",
              "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
              "<body style='background:#050505;color:#fff;font-family:sans-serif;text-align:center'>"
              "<h1>KaiGate <span style='color:#e10600'>改</span></h1>"
              "<p>Drop a circular 466×466 GIF or wallpaper.bin</p>"
              "<form method=POST action=/upload enctype=multipart/form-data>"
              "<input type=file name=file><br><br>"
              "<select name=dest>"
              "<option value='/idle.gif'>Idle mascot GIF</option>"
              "<option value='/boot.gif'>Boot GIF</option>"
              "<option value='/wallpaper.bin'>Wallpaper RGB565</option>"
              "</select><br><br><button>Upload</button></form></body>");
}

static String destFromUpload() {
  if (s_http.hasArg("dest")) return s_http.arg("dest");
  return LITTLEFS_IDLE_GIF;
}

static void handleUpload() {
  HTTPUpload &up = s_http.upload();
  if (up.status == UPLOAD_FILE_START) {
    String path = destFromUpload();
    if (up.filename.endsWith(".bin")) path = LITTLEFS_WALLPAPER;
    else if (up.filename.indexOf("boot") >= 0) path = LITTLEFS_BOOT_GIF;
    else if (up.filename.endsWith(".gif")) path = LITTLEFS_IDLE_GIF;
    LittleFS.remove(path);
    s_file = LittleFS.open(path, "w");
    s_written = 0;
    Serial.printf("[wifi] upload start %s\n", path.c_str());
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (s_file) s_written += s_file.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (s_file) s_file.close();
    Serial.printf("[wifi] upload done %u bytes\n", static_cast<unsigned>(s_written));
  }
}

bool wifiTransferBegin() {
  if (s_active) return true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(KAIGATE_WIFI_SSID, KAIGATE_WIFI_PASS);
  delay(120);
  s_http.on("/", HTTP_GET, sendPortal);
  s_http.on("/status", HTTP_GET, []() {
    s_http.send(200, "application/json", "{\"ok\":true,\"device\":\"KaiGate\"}");
  });
  s_http.on(
      "/upload", HTTP_POST,
      []() {
        s_http.send(200, "application/json", "{\"ok\":true}");
      },
      handleUpload);
  s_http.on("/uploadRaw", HTTP_POST, []() {
    String path = s_http.hasArg("path") ? s_http.arg("path") : String(LITTLEFS_IDLE_GIF);
    File f = LittleFS.open(path, "w");
    if (!f) {
      s_http.send(500, "text/plain", "open failed");
      return;
    }
    const int n = s_http.client().write(reinterpret_cast<const uint8_t *>(""), 0);
    (void)n;
    f.print(s_http.arg("plain"));
    f.close();
    s_http.send(200, "application/json", "{\"ok\":true}");
  });
  s_http.begin();
  s_active = true;
  AppState &st = AppState::get();
  st.wifiApActive = true;
  strncpy(st.wifiIp, KAIGATE_WIFI_IP_STR, sizeof(st.wifiIp) - 1);
  gattNotifyStatus("{\"wifi\":true,\"ip\":\"192.168.4.1\",\"ssid\":\"KaiGate_Setup\"}");
  Serial.println("[wifi] SoftAP KaiGate_Setup  / 192.168.4.1");
  return true;
}

void wifiTransferStop() {
  if (!s_active) return;
  s_http.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  s_active = false;
  AppState::get().wifiApActive = false;
  gattNotifyStatus("{\"wifi\":false}");
}

bool wifiTransferActive() { return s_active; }

void wifiTransferTick() {
  if (s_active) s_http.handleClient();
}
