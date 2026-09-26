<p align="center">
  <img src="app/kaigate/assets/branding/kaigate_seal.png#gh-dark-mode-only" width="220" alt="KaiGate seal">
  <img src="branding/kaigate_seal.jpg#gh-light-mode-only" width="220" alt="KaiGate seal">
  <br>
  <img src="app/kaigate/assets/branding/kaigate_wordmark.png#gh-dark-mode-only" width="340" alt="KaiGate">
  <img src="branding/kaigate_wordmark.jpg#gh-light-mode-only" width="340" alt="KaiGate">
</p>

<p align="center">
  <strong>SMART SHIFT KNOB</strong><br>
  ESP32-S3 firmware · 466×466 circular AMOLED · KaiGate app for Android &amp; iOS
</p>

<p align="center">
  <a href="docs/readme.html"><strong>Open the visual brief</strong></a>
  ·
  <a href="docs/HARDWARE.md">Hardware</a>
  ·
  <a href="docs/BLE_PROTOCOL.md">BLE protocol</a>
</p>

---

Universal commercial smart manual-shift knob: **ESP32-S3 firmware** (Arduino / PlatformIO + LVGL 8) plus the **KaiGate** Flutter companion. Wordmark and 改-seal live in `branding/` and ship inside the mobile app.

## What you get

| Layer | Role |
| --- | --- |
| Waveshare ESP32-S3-Touch-AMOLED-1.32 | 466×466 circular CO5300 AMOLED, CST820 touch, ES8311 speaker path, LiPo gauge |
| MPU-6050 @ `0x68` | Pitch/roll H-gate clustering + 2-axis G-meter |
| DRV2605L @ `0x5A` | 1027 coin ERM haptics (shift double-pulse, money-shift alert) |
| Vgate iCar Pro BLE | ELM327 PID client (RPM, speed, CLT, MAP/boost, IAT, voltage, DTCs) |
| KaiGate phone app | GATT configurator, calibration wizard, circular lens studio, telemetry/DTC hub |

The knob is a **BLE peripheral for the phone** and a **BLE central for the OBD dongle at the same time** (NimBLE dual role). Large GIF/wallpaper transfers raise SoftAP `KaiGate_Setup`.

## On-glass carousel

1. **Tach** — perimeter green→yellow→red arc, center gear 1–6 / R / N
2. **Vitals** — boost bar + peak hold, coolant, IAT, voltage, knob SoC
3. **G-meter** — live bubble + lateral/longitudinal peak dots
4. **Timers** — auto 0–60 mph and shift-gate time (ms)
5. **Lens** — LittleFS wallpaper / idle GIF, optional floating micro-gear
6. **Media** — play/pause, skip, volume; phone executes transport keys over BLE
7. **DTC** — stored codes, touch CLEAR (ELM mode 04)

## Repository

```
KaiGate/
├── branding/                          # official logos
├── docs/                              # hardware, BLE, visual brief
├── firmware/                          # PlatformIO + LVGL 8
│   ├── include/                       # pins, lv_conf, protocol, types
│   ├── data/                          # LittleFS (/boot.gif, /idle.gif, /wallpaper.bin)
│   └── src/                           # hal, core, ble, net, media, ui
├── app/kaigate/                       # Flutter companion
└── scripts/bootstrap_flutter.cmd
```

## Firmware

OPI PSRAM is required (`qio_opi`). Arduino-ESP32 **≥ 3.3.0** is required. `platformio.ini` pins pioarduino 53.03.13.

```bash
cd firmware
pio run -t upload
pio run -t uploadfs
pio device monitor
```

**Safety:** 2 s long-press toggles lock. Auto-relock after 10 s idle while stationary. Swipes rejected if speed > 0.5 mph or a gear is engaged. Cold limiter holds the shift light at 3,500 RPM until coolant ≥ 180 °F. Shift strobe is full-circle red/white + haptic double-pulse + beep. Money-shift compares predicted post-downshift RPM to redline.

**Gear engine:** NVS namespace `kaigate` stores eight clusters (N, 1–6, R). The phone wizard writes them with `{"cmd":"saveGate","gear":n}`.

## Flutter app

```bat
scripts\bootstrap_flutter.cmd
cd app\kaigate
flutter run
```

Or `flutter build apk` for a sideload installer at `app/kaigate/build/app/outputs/flutter-apk/app-release.apk`.

- Auto-scan for the KaiGate GATT service
- Redline 4,000–9,500, shift light, cold limiter, brightness
- Themes: JDM Amber, Euro Sport, Monochrome, Cyberpunk Neon
- Layouts: 5-speed, 6-speed, dog-leg
- Live IMU calibration wizard
- Media control with now-playing title, artist, and cover art
- 466×466 circular lens studio and Wi-Fi push to LittleFS
- 0–60 history + plain-English DTC decoder

Phone BLE UUIDs match `firmware/include/protocol.h` — keep those two files in lock-step.

## First-drive bring-up

1. Wire MPU-6050 and DRV2605L onto I2C0 (GPIO47/48) — see `docs/HARDWARE.md`.
2. Fit a 1027 coin motor to the DRV2605L and a speaker to the MX1.25 header.
3. Flash firmware, confirm serial: `CST820`, `MPU-6050`, `DRV2605L`, `ES8311`.
4. Pair Vgate iCar Pro (ignition on). Knob scans for Vgate / iCar / OBD.
5. Open KaiGate, connect, run **Calibrate** in the driveway.
6. Set redline/theme. Optional: Lens Studio → `KaiGate_Setup` / `kaigate32` → push idle GIF.

## License / product notes

This tree is the product firmware + companion source for KaiGate. Validate haptic drive current, speaker SPL, and AMOLED brightness against your enclosure thermal budget before a production spin.
