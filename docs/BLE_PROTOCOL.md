# KaiGate BLE protocol

Service UUID: `8f400001-7a3b-4c2d-9e1f-0a1b2c3d4e5f`  
Device name: `KaiGate`

| Characteristic | UUID suffix | Props | Payload |
| --- | --- | --- | --- |
| Telemetry | `...0002` | read, notify | 28-byte `TelemetryPacket` LE |
| Config | `...0003` | read, write | JSON `KnobConfig` |
| Command | `...0004` | write | JSON `{ "cmd": "..." }` |
| IMU | `...0005` | notify | 10-byte `ImuPacket` (during calibration) |
| DTC | `...0006` | read, notify | JSON `{ "dtc": [ { "code": "P0301" } ] }` |
| Status | `...0007` | read, notify | JSON wifi/status |
| Media | `...0008` | read, write, notify | now-playing JSON; notify `{ "action": "playPause" }` |

## TelemetryPacket (28 bytes)

```
u16 rpm
u16 speed_mph * 10
i16 boost_kPa * 10
i16 coolant_C * 10
i16 iat_C * 10
u16 module_mV
i8  gear   (-1 R, 0 N, 1-6)
u8  flags  bit0 lock,1 obd,2 strobe,3 money,4 cold,5 mascot
i16 pitch*100
i16 roll*100
i16 latG*100
i16 longG*100
u8  knob_soc
u8  theme id
u16 last_shift_ms
u16 zero_to_sixty * 100
```

C++: `firmware/include/protocol.h`  
Dart: `app/kaigate/lib/ble/protocol.dart`

## Commands

```json
{"cmd":"saveGate","gear":1}
{"cmd":"calibrateStart"}
{"cmd":"calibrateStop"}
{"cmd":"lock","on":true}
{"cmd":"wifiStart"}
{"cmd":"wifiStop"}
{"cmd":"clearDtc"}
{"cmd":"queryDtc"}
{"cmd":"resetPeaks"}
{"cmd":"resetGates"}
{"cmd":"setTheme","id":0}
{"cmd":"nowPlaying","title":"Track","artist":"Artist","playing":true,"volume":70}
```

## OBD client (Vgate)

The knob scans for advertised names containing `Vgate`, `iCar`, `OBD`, `ELM`. It binds Nordic-style serial characteristics (`fff0/fff1/fff2`, `ffe0/ffe1`, or first notify+write pair) and polls:

`010C` RPM · `010D` speed · `0105` coolant · `010B` MAP · `010F` IAT · `0142` voltage · `03` DTCs · `04` clear

## Wi-Fi fallback

SSID `KaiGate_Setup` / password `kaigate32` / portal `http://192.168.4.1`  
`POST /upload` multipart field `file`, optional `dest` = `/idle.gif` | `/boot.gif` | `/wallpaper.bin`

## Media control

Phone **writes** now-playing to `...0008`:

```json
{"title":"Track","artist":"Artist","playing":true,"volume":70}
```

Knob **notifies** transport when the glass is tapped:

```json
{"action":"playPause"}
{"action":"next"}
{"action":"prev"}
{"action":"volUp"}
{"action":"volDown"}
```

The KaiGate app turns those into Android media keys / iOS system music commands.
