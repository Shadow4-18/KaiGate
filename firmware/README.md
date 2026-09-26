# KaiGate firmware

PlatformIO project targeting Waveshare **ESP32-S3-Touch-AMOLED-1.32**.

```bash
pio run -e kaigate
pio run -e kaigate -t upload
pio run -e kaigate -t uploadfs
```

Serial is USB-CDC at 115200. Successful boot prints probe lines for CO5300, CST820, MPU-6050, DRV2605L, and ES8311.

If a sensor is unplugged the corresponding HAL logs `not found` and the rest of the product still runs (gear falls back to OBD ratio / Neutral).

LittleFS files:

| Path | Format |
| --- | --- |
| `/boot.gif` | optional circular GIF played once at boot |
| `/idle.gif` | idle mascot (>30 s stationary in N) |
| `/wallpaper.bin` | 466×466 RGB565 little-endian (434,312 bytes) |
