#pragma once

// Waveshare ESP32-S3-Touch-AMOLED-1.32 (ESP32-S3-PICO-1-N8R8)
// Pin map transcribed from the official schematic:
// https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.32/ESP32-S3-Touch-AMOLED-1.32-Schematic.pdf

#include <stdint.h>

// ---------------------------------------------------------------------------
// Shared I2C0  @ 400 kHz  — GPIO47 SDA / GPIO48 SCL
// Devices: CST820 (0x15), ES8311 (0x18), MPU-6050 (0x68), DRV2605L (0x5A)
// ---------------------------------------------------------------------------
static constexpr int PIN_I2C_SDA = 47;
static constexpr int PIN_I2C_SCL = 48;
static constexpr uint32_t I2C_HZ = 400000;

static constexpr uint8_t ADDR_CST820 = 0x15;
static constexpr uint8_t ADDR_ES8311 = 0x18;
static constexpr uint8_t ADDR_DRV2605 = 0x5A;
static constexpr uint8_t ADDR_MPU6050 = 0x68;

// Touch CST820
static constexpr int PIN_TP_INT = 6;
static constexpr int PIN_TP_RST = 7;

// CO5300 AMOLED QSPI 466x466
static constexpr int PIN_LCD_RST = 8;
static constexpr int PIN_LCD_TE = 9;
static constexpr int PIN_LCD_CS = 10;
static constexpr int PIN_LCD_SCLK = 11;
static constexpr int PIN_LCD_D0 = 12;
static constexpr int PIN_LCD_D1 = 13;
static constexpr int PIN_LCD_D2 = 14;
static constexpr int PIN_LCD_D3 = 15;
static constexpr int LCD_WIDTH = 466;
static constexpr int LCD_HEIGHT = 466;

// ES8311 I2S + NS4150B amplifier
static constexpr int PIN_CODEC_EN = 16;
static constexpr int PIN_I2S_MCLK = 38;
static constexpr int PIN_I2S_BCLK = 39;
static constexpr int PIN_I2S_DIN = 40;   // ES8311 ASDOUT (mic)
static constexpr int PIN_I2S_WS = 41;    // LRCK
static constexpr int PIN_I2S_DOUT = 42;  // ES8311 DSDIN (speaker)
static constexpr int PIN_PA_CTRL = 46;

// Battery path (ETA6098 charger + 200k/200k divider on GPIO4 / ADC1_CH3)
static constexpr int PIN_BAT_ADC = 4;
static constexpr int PIN_PWR_KEY = 17;
static constexpr int PIN_BAT_EN = 18;
static constexpr int BAT_ADC_CHANNEL = 3;  // ADC1 channel 3 == GPIO4

// Expansion header leftovers (SH1.0 12PIN) — IMU INT optional
static constexpr int PIN_MPU_INT = 5;
static constexpr int PIN_EXPAND_IO1 = 1;
static constexpr int PIN_EXPAND_IO2 = 2;
static constexpr int PIN_EXPAND_IO3 = 3;

static constexpr int PIN_BOOT = 0;
