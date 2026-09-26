# Hardware reference — Waveshare ESP32-S3-Touch-AMOLED-1.32

MCU: **ESP32-S3-PICO-1-N8R8** (8 MB OPI PSRAM, 8 MB flash).  
Pin numbers transcribed from  
https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.32/ESP32-S3-Touch-AMOLED-1.32-Schematic.pdf

## On-board

| Function | GPIO / addr |
| --- | --- |
| I2C SDA / SCL (400 kHz) | **47 / 48** |
| CST820 touch | 0x15, RST=7, INT=6 |
| ES8311 codec | 0x18 |
| CO5300 QSPI CS/SCLK/D0–D3 | 10 / 11 / 12 / 13 / 14 / 15 |
| LCD RST / TE | 8 / 9 |
| I2S MCLK BCLK DIN WS DOUT | 38 / 39 / 40 / 41 / 42 |
| NS4150B PA_CTRL | 46 |
| Codec enable | 16 |
| BAT_ADC (200k/200k, ADC1_CH3) | 4 |
| BAT_EN / PWR_KEY | 18 / 17 |

## Off-board (SH1.0 12PIN + shared I2C)

Both chips hang on the same I2C0 bus as the touch controller and codec.

```
3V3  → MPU-6050 VCC, DRV2605L VCC
GND  → MPU-6050 GND, DRV2605L GND
SDA  GPIO47
SCL  GPIO48
INT  GPIO5  (optional, MPU INT)
DRV2605L OUT+/OUT− → 1027 coin ERM
```

Addresses: MPU-6050 **0x68** (AD0 low), DRV2605L **0x5A**.

Speaker: MX1.25 2-pin header (already amplified by NS4150B).  
Battery: MX1.25 2-pin LiPo, held on by GPIO18 `BAT_EN`.
