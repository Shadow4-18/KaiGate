import "package:flutter/material.dart";
import "package:provider/provider.dart";

import "../providers/knob_controller.dart";
import "../theme/brand.dart";
import "../widgets/brand_header.dart";
import "calibrate_screen.dart";
import "dtc_screen.dart";
import "media_control_screen.dart";
import "studio_screen.dart";
import "telemetry_screen.dart";
import "tune_screen.dart";

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final k = context.watch<KnobController>();
    final t = k.telemetry;
    return Scaffold(
      body: SafeArea(
        child: ListView(
          padding: const EdgeInsets.fromLTRB(20, 12, 20, 32),
          children: [
            const BrandHeader(),
            const SizedBox(height: 18),
            _status(k),
            if (k.devices.isNotEmpty && !k.connected) ...[
              const SizedBox(height: 8),
              ...k.devices.map(
                (d) => ListTile(
                  tileColor: Brand.card,
                  title: Text(d.device.platformName.isEmpty ? d.device.remoteId.str : d.device.platformName),
                  subtitle: Text(d.device.remoteId.str),
                  trailing: const Icon(Icons.link, color: Brand.amber),
                  onTap: () => k.connect(d.device),
                ),
              ),
            ],
            const SizedBox(height: 18),
            if (t != null) _hero(t.gearLabel, t.rpm, t.speedMph, t.locked, t.obd, t.coldLimited),
            const SizedBox(height: 22),
            _tile(context, "Tune", "Redline, shift light, themes, layout", Icons.tune, const TuneScreen()),
            _tile(context, "Calibrate", "IMU gate wizard with live pitch/roll", Icons.gps_fixed, const CalibrateScreen()),
            _tile(context, "Media", "Play, skip, and volume on the knob", Icons.play_circle, const MediaControlScreen()),
            _tile(context, "Lens Studio", "466×466 circular media → LittleFS", Icons.camera, const StudioScreen()),
            _tile(context, "Telemetry", "0-60 history and live vitals", Icons.speed, const TelemetryScreen()),
            _tile(context, "Diagnostics", "Plain-English DTCs", Icons.medical_services, const DtcScreen()),
            const SizedBox(height: 12),
            if (k.connected)
              FilledButton(
                onPressed: () => k.cmd("lock", {"on": !(t?.locked ?? true)}),
                child: Text((t?.locked ?? true) ? "UNLOCK KNOB" : "LOCK KNOB"),
              ),
          ],
        ),
      ),
    );
  }

  Widget _status(KnobController k) {
    return Container(
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(color: Brand.card, borderRadius: BorderRadius.circular(16)),
      child: Row(
        children: [
          Icon(k.connected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
              color: k.connected ? Brand.amber : Colors.white38),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              k.connected
                  ? "KaiGate knob  •  batt ${k.telemetry?.knobBattPct ?? "--"}%"
                  : "Scanning for KaiGate… tap Connect",
            ),
          ),
          if (!k.connected)
            TextButton(onPressed: k.scan, child: Text(k.scanning ? "…" : "SCAN")),
        ],
      ),
    );
  }

  Widget _hero(String gear, int rpm, double mph, bool locked, bool obd, bool cold) {
    return Container(
      height: 210,
      decoration: const BoxDecoration(
        shape: BoxShape.circle,
        gradient: RadialGradient(colors: [Color(0xFF2A0000), Color(0xFF000000)]),
        border: Border.fromBorderSide(BorderSide(color: Brand.red, width: 3)),
      ),
      alignment: Alignment.center,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(gear, style: const TextStyle(fontSize: 72, color: Brand.white, height: 1)),
          Text("$rpm RPM", style: const TextStyle(color: Brand.amber, fontSize: 18)),
          Text("${mph.toStringAsFixed(0)} mph  •  ${locked ? "LOCKED" : "OPEN"}  •  ${obd ? "OBD" : "NO OBD"}",
              style: const TextStyle(fontSize: 12, color: Brand.mist)),
          if (cold) const Text("COLD LIMITER", style: TextStyle(color: Colors.lightBlueAccent, fontSize: 11)),
        ],
      ),
    );
  }

  Widget _tile(BuildContext ctx, String t, String s, IconData i, Widget page) {
    return Card(
      color: Brand.card,
      margin: const EdgeInsets.only(bottom: 10),
      child: ListTile(
        leading: Icon(i, color: Brand.red),
        title: Text(t),
        subtitle: Text(s),
        trailing: const Icon(Icons.chevron_right),
        onTap: () => Navigator.push(ctx, MaterialPageRoute(builder: (_) => page)),
      ),
    );
  }
}
