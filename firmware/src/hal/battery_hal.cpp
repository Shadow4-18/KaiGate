#include "hal/battery_hal.h"

#include "config.h"
#include "pins.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

static adc_oneshot_unit_handle_t s_adc = nullptr;
static adc_cali_handle_t s_cali = nullptr;
static bool s_ok = false;
static float s_filt = 4.0f;

bool batteryBegin() {
  pinMode(PIN_BAT_EN, OUTPUT);
  pinMode(PIN_PWR_KEY, INPUT_PULLUP);
  digitalWrite(PIN_BAT_EN, HIGH);

  adc_oneshot_unit_init_cfg_t unit = {.unit_id = ADC_UNIT_1};
  if (adc_oneshot_new_unit(&unit, &s_adc) != ESP_OK) return false;

  adc_oneshot_chan_cfg_t ch = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
  };
  if (adc_oneshot_config_channel(s_adc, ADC_CHANNEL_3, &ch) != ESP_OK) return false;

  adc_cali_curve_fitting_config_t cali = {};
  cali.unit_id = ADC_UNIT_1;
  cali.atten = ADC_ATTEN_DB_12;
  cali.bitwidth = ADC_BITWIDTH_12;
  if (adc_cali_create_scheme_curve_fitting(&cali, &s_cali) != ESP_OK) {
    s_cali = nullptr;
  }
  s_ok = true;
  Serial.println("[battery] ADC1_CH3 (GPIO4) ready, BAT_EN held");
  return true;
}

void batteryHoldPower(bool hold) { digitalWrite(PIN_BAT_EN, hold ? HIGH : LOW); }

float batteryVoltage() {
  if (!s_ok) return 0;
  int raw = 0;
  adc_oneshot_read(s_adc, ADC_CHANNEL_3, &raw);
  int mv = 0;
  if (s_cali) {
    adc_cali_raw_to_voltage(s_cali, raw, &mv);
  } else {
    mv = raw * 3300 / 4095;
  }
  const float v = (mv / 1000.0f) * BAT_VDIV_RATIO;
  s_filt = s_filt * 0.85f + v * 0.15f;
  return s_filt;
}

uint8_t batteryPercent() {
  const float v = batteryVoltage();
  float pct = (v - BAT_EMPTY_V) * 100.0f / (BAT_FULL_V - BAT_EMPTY_V);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return static_cast<uint8_t>(pct);
}

bool batteryPwrHeld() { return digitalRead(PIN_PWR_KEY) == LOW; }
