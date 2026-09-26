import "package:flutter/material.dart";
import "package:provider/provider.dart";

import "../providers/knob_controller.dart";
import "../theme/brand.dart";

class TelemetryScreen extends StatelessWidget {
  const TelemetryScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final k = context.watch<KnobController>();
    final t = k.telemetry;
    return Scaffold(
      appBar: AppBar(
        title: const Text("TELEMETRY"),
        actions: [
          IconButton(onPressed: () => k.cmd("resetPeaks"), icon: const Icon(Icons.restart_alt)),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          if (t != null) ...[
            _kv("RPM", "${t.rpm}"),
            _kv("Speed", "${t.speedMph.toStringAsFixed(1)} mph"),
            _kv("Boost", "${t.boostKpa.toStringAsFixed(1)} kPa"),
            _kv("Coolant", "${(t.coolantC * 9 / 5 + 32).toStringAsFixed(0)} °F"),
            _kv("IAT", "${(t.iatC * 9 / 5 + 32).toStringAsFixed(0)} °F"),
            _kv("Module V", "${t.voltageV.toStringAsFixed(2)} V"),
            _kv("Lat G", t.latG.toStringAsFixed(2)),
            _kv("Long G", t.longG.toStringAsFixed(2)),
            _kv("0-60 (live)", "${t.zeroToSixtyS.toStringAsFixed(2)} s"),
            _kv("Last shift", "${t.lastShiftMs} ms"),
          ] else
            const Text("Connect the knob to stream live telemetry."),
          const SizedBox(height: 18),
          const Text("PERSONAL BESTS", style: TextStyle(color: Brand.amber)),
          const SizedBox(height: 8),
          if (k.history.isEmpty) const Text("No logged runs yet. 0-60 is stored when 60 mph is reached."),
          ...k.history.map(
            (r) => ListTile(
              dense: true,
              title: Text("${r.zeroToSixty.toStringAsFixed(2)} s   •   ${r.shiftMs} ms shift"),
              subtitle: Text(r.at.toLocal().toString()),
            ),
          ),
        ],
      ),
    );
  }

  Widget _kv(String k, String v) => Padding(
        padding: const EdgeInsets.symmetric(vertical: 6),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [Text(k, style: const TextStyle(color: Brand.mist)), Text(v)],
        ),
      );
}
