import "package:flutter/material.dart";
import "package:provider/provider.dart";

import "../ble/protocol.dart";
import "../providers/knob_controller.dart";
import "../theme/brand.dart";

class TuneScreen extends StatelessWidget {
  const TuneScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final k = context.watch<KnobController>();
    final c = k.config;
    return Scaffold(
      appBar: AppBar(title: const Text("TUNE")),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          _slide("Redline", c.redline.toDouble(), 4000, 9500, "RPM", (v) {
            c.redline = v.round();
            k.notifyListeners();
          }),
          _slide("Shift light", c.shiftLight.toDouble(), 3000, 9500, "RPM", (v) {
            c.shiftLight = v.round();
            k.notifyListeners();
          }),
          _slide("Cold rev limiter", c.coldLimit.toDouble(), 2500, 5000, "RPM", (v) {
            c.coldLimit = v.round();
            k.notifyListeners();
          }),
          _slide("Cold coolant target", c.coldCoolantF.toDouble(), 140, 200, "°F", (v) {
            c.coldCoolantF = v.round();
            k.notifyListeners();
          }),
          _slide("Brightness", c.brightness.toDouble(), 20, 255, "", (v) {
            c.brightness = v.round();
            k.notifyListeners();
          }),
          const SizedBox(height: 8),
          const Text("Theme"),
          Wrap(
            spacing: 8,
            children: List.generate(
              themeNames.length,
              (i) => ChoiceChip(
                label: Text(themeNames[i]),
                selected: c.theme == i,
                selectedColor: Brand.red,
                onSelected: (_) {
                  c.theme = i;
                  k.notifyListeners();
                },
              ),
            ),
          ),
          const SizedBox(height: 16),
          const Text("Transmission"),
          Wrap(
            spacing: 8,
            children: List.generate(
              layoutNames.length,
              (i) => ChoiceChip(
                label: Text(layoutNames[i]),
                selected: c.layout == i,
                selectedColor: Brand.red,
                onSelected: (_) {
                  c.layout = i;
                  k.notifyListeners();
                },
              ),
            ),
          ),
          SwitchListTile(
            title: const Text("Reverse lockout on the left"),
            value: c.reverseLockoutLeft,
            onChanged: (v) {
              c.reverseLockoutLeft = v;
              k.notifyListeners();
            },
          ),
          SwitchListTile(
            title: const Text("Floating micro-gear overlay"),
            value: c.overlay,
            onChanged: (v) {
              c.overlay = v;
              k.notifyListeners();
            },
          ),
          SwitchListTile(
            title: const Text("Haptics"),
            value: c.haptic,
            onChanged: (v) {
              c.haptic = v;
              k.notifyListeners();
            },
          ),
          SwitchListTile(
            title: const Text("Audio alerts"),
            value: c.audio,
            onChanged: (v) {
              c.audio = v;
              k.notifyListeners();
            },
          ),
          const SizedBox(height: 20),
          FilledButton(onPressed: k.applyConfig, child: const Text("WRITE TO KNOB")),
        ],
      ),
    );
  }

  Widget _slide(String label, double v, double min, double max, String unit, ValueChanged<double> on) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text("$label  ${v.round()} $unit"),
        Slider(value: v.clamp(min, max), min: min, max: max, onChanged: on),
      ],
    );
  }
}
