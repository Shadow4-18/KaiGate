import "package:flutter/material.dart";
import "package:provider/provider.dart";

import "../providers/knob_controller.dart";
import "../theme/brand.dart";

class CalibrateScreen extends StatefulWidget {
  const CalibrateScreen({super.key});
  @override
  State<CalibrateScreen> createState() => _CalibrateScreenState();
}

class _CalibrateScreenState extends State<CalibrateScreen> {
  static const steps = [
    (0, "NEUTRAL", "Rest the stick in the gate center"),
    (1, "1ST", "Pull left and forward into 1st"),
    (2, "2ND", "Hold left and back into 2nd"),
    (3, "3RD", "Center-forward into 3rd"),
    (4, "4TH", "Center-back into 4th"),
    (5, "5TH", "Right-forward into 5th"),
    (6, "6TH", "Right-back into 6th (skip on 5-speed)"),
    (7, "REVERSE", "Engage reverse with lockout if fitted"),
  ];
  int i = 0;
  KnobController? _knob;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _knob = context.read<KnobController>();
      _knob?.cmd("calibrateStart");
    });
  }

  @override
  void dispose() {
    _knob?.cmd("calibrateStop");
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final k = context.watch<KnobController>();
    final imu = k.imu;
    final step = steps[i];
    return Scaffold(
      appBar: AppBar(title: const Text("GATE CALIBRATION")),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            LinearProgressIndicator(value: (i + 1) / steps.length, color: Brand.red, backgroundColor: Colors.white12),
            const SizedBox(height: 24),
            Text("STEP ${i + 1} / ${steps.length}", style: const TextStyle(color: Brand.mist)),
            Text(step.$2, style: const TextStyle(fontSize: 42, color: Brand.white)),
            Text(step.$3, textAlign: TextAlign.center),
            const SizedBox(height: 24),
            Container(
              height: 220,
              width: 220,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                border: Border.all(color: Brand.amber, width: 2),
              ),
              child: CustomPaint(painter: _GatePainter(imu?.roll ?? 0, imu?.pitch ?? 0)),
            ),
            const SizedBox(height: 12),
            Text(
              "PITCH  ${(imu?.pitch ?? 0).toStringAsFixed(1)}°    ROLL  ${(imu?.roll ?? 0).toStringAsFixed(1)}°",
              style: const TextStyle(color: Brand.amber),
            ),
            const Spacer(),
            FilledButton(
              onPressed: () async {
                await k.saveGate(step.$1);
                if (i < steps.length - 1) {
                  setState(() => i++);
                } else if (mounted) {
                  ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Gates saved to NVS")));
                  Navigator.pop(context);
                }
              },
              child: Text("ENGAGE ${step.$2}  →  SAVE"),
            ),
            TextButton(
              onPressed: i == 6
                  ? () => setState(() => i++)
                  : null,
              child: const Text("Skip 6th (5-speed)"),
            ),
          ],
        ),
      ),
    );
  }
}

class _GatePainter extends CustomPainter {
  _GatePainter(this.roll, this.pitch);
  final double roll, pitch;
  @override
  void paint(Canvas canvas, Size size) {
    final c = Offset(size.width / 2, size.height / 2);
    final p = Paint()
      ..color = const Color(0x33FFFFFF)
      ..style = PaintingStyle.stroke;
    canvas.drawCircle(c, size.width / 2 - 8, p);
    canvas.drawLine(Offset(c.dx, 16), Offset(c.dx, size.height - 16), p);
    canvas.drawLine(Offset(16, c.dy), Offset(size.width - 16, c.dy), p);
    final dot = Offset(c.dx + roll * 3.2, c.dy - pitch * 3.2);
    canvas.drawCircle(dot, 10, Paint()..color = const Color(0xFFE10600));
  }

  @override
  bool shouldRepaint(covariant _GatePainter old) => old.roll != roll || old.pitch != pitch;
}
