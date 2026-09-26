#include "hal/audio_hal.h"

#include "hal/i2c_bus.h"
#include "pins.h"

#include "driver/i2s_std.h"
#include <math.h>

static i2s_chan_handle_t s_tx = nullptr;
static bool s_ok = false;
static bool s_enabled = true;
static int16_t *s_tone = nullptr;
static size_t s_toneLen = 0;
static size_t s_tonePos = 0;
static uint32_t s_toneEndMs = 0;

static constexpr int kSampleRate = 16000;

static bool es8311Write(uint8_t reg, uint8_t val) {
  return i2cWriteReg(ADDR_ES8311, reg, &val, 1) > 0;
}

static bool es8311Init() {
  pinMode(PIN_CODEC_EN, OUTPUT);
  pinMode(PIN_PA_CTRL, OUTPUT);
  digitalWrite(PIN_CODEC_EN, HIGH);
  digitalWrite(PIN_PA_CTRL, HIGH);
  delay(15);

  if (!i2cProbe(ADDR_ES8311)) {
    Serial.println("[audio] ES8311 not found at 0x18");
    return false;
  }

  es8311Write(0x00, 0x1F);  // reset
  delay(5);
  es8311Write(0x00, 0x80);  // CSM on, analog on
  es8311Write(0x01, 0x3F);  // MCLK on, clock manager
  es8311Write(0x02, 0x10);
  es8311Write(0x03, 0x10);
  es8311Write(0x04, 0x10);
  es8311Write(0x05, 0x00);
  es8311Write(0x06, 0x03);  // 16 kHz family
  es8311Write(0x07, 0x00);
  es8311Write(0x08, 0xFF);
  es8311Write(0x09, 0x0C);  // I2S 16-bit slave
  es8311Write(0x0A, 0x0C);
  es8311Write(0x0B, 0x00);
  es8311Write(0x0C, 0x00);
  es8311Write(0x10, 0x03);
  es8311Write(0x11, 0x7F);
  es8311Write(0x12, 0x28);
  es8311Write(0x13, 0x00);
  es8311Write(0x14, 0x1A);
  es8311Write(0x16, 0x03);
  es8311Write(0x17, 0x18);
  es8311Write(0x1B, 0x0A);
  es8311Write(0x1C, 0x6A);
  es8311Write(0x37, 0x08);
  es8311Write(0x32, 0xBF);  // DAC volume
  es8311Write(0x31, 0x00);
  Serial.println("[audio] ES8311 codec configured");
  return true;
}

static bool i2sInit() {
  i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan.auto_clear = true;
  if (i2s_new_channel(&chan, &s_tx, nullptr) != ESP_OK) return false;

  i2s_std_config_t std = {};
  std.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kSampleRate);
  std.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  std.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  std.gpio_cfg.mclk = static_cast<gpio_num_t>(PIN_I2S_MCLK);
  std.gpio_cfg.bclk = static_cast<gpio_num_t>(PIN_I2S_BCLK);
  std.gpio_cfg.ws = static_cast<gpio_num_t>(PIN_I2S_WS);
  std.gpio_cfg.dout = static_cast<gpio_num_t>(PIN_I2S_DOUT);
  std.gpio_cfg.din = static_cast<gpio_num_t>(PIN_I2S_DIN);
  if (i2s_channel_init_std_mode(s_tx, &std) != ESP_OK) return false;
  return i2s_channel_enable(s_tx) == ESP_OK;
}

static void buildTone(uint16_t freqHz, uint16_t durationMs) {
  const size_t n = static_cast<size_t>(kSampleRate) * durationMs / 1000;
  if (s_tone) {
    free(s_tone);
    s_tone = nullptr;
  }
  s_tone = static_cast<int16_t *>(malloc(n * 2 * sizeof(int16_t)));
  if (!s_tone) {
    s_toneLen = 0;
    return;
  }
  for (size_t i = 0; i < n; ++i) {
    const float env = (i < 80) ? i / 80.0f : (i > n - 80 ? (n - i) / 80.0f : 1.0f);
    const int16_t s = static_cast<int16_t>(sinf(2.0f * PI * freqHz * i / kSampleRate) * 22000.0f * env);
    s_tone[i * 2] = s;
    s_tone[i * 2 + 1] = s;
  }
  s_toneLen = n * 2;
  s_tonePos = 0;
  s_toneEndMs = millis() + durationMs + 20;
}

bool audioBegin() {
  s_ok = es8311Init() && i2sInit();
  if (!s_ok) Serial.println("[audio] I2S/codec init failed");
  return s_ok;
}

void audioSetEnabled(bool on) {
  s_enabled = on;
  digitalWrite(PIN_PA_CTRL, on ? HIGH : LOW);
}

void audioBeep(uint16_t freqHz, uint16_t durationMs) {
  if (!s_ok || !s_enabled) return;
  buildTone(freqHz, durationMs);
}

void audioShiftBeep() { audioBeep(1760, 70); }
void audioAlert() { audioBeep(880, 220); }

void audioTick(uint32_t nowMs) {
  if (!s_ok || !s_tone || s_tonePos >= s_toneLen) {
    if (s_tone && nowMs > s_toneEndMs) {
      free(s_tone);
      s_tone = nullptr;
      s_toneLen = s_tonePos = 0;
    }
    return;
  }
  size_t written = 0;
  const size_t bytes = (s_toneLen - s_tonePos) * sizeof(int16_t);
  i2s_channel_write(s_tx, s_tone + s_tonePos, bytes, &written, 0);
  s_tonePos += written / sizeof(int16_t);
}
