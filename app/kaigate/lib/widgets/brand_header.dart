import "package:flutter/material.dart";

import "../theme/brand.dart";

class BrandHeader extends StatelessWidget {
  const BrandHeader({super.key, this.subtitle = "SMART SHIFT KNOB"});
  final String subtitle;

  @override
  Widget build(BuildContext context) {
    final w = MediaQuery.sizeOf(context).width;
    final sealH = (w * 0.46).clamp(168.0, 280.0);
    final wordW = (w * 0.78).clamp(240.0, 440.0);
    return Column(
      children: [
        Image.asset(
          "assets/branding/kaigate_seal.png",
          height: sealH,
          filterQuality: FilterQuality.high,
        ),
        SizedBox(height: w < 420 ? 14 : 20),
        Image.asset(
          "assets/branding/kaigate_wordmark.png",
          width: wordW,
          filterQuality: FilterQuality.high,
        ),
        const SizedBox(height: 10),
        Text(subtitle, style: const TextStyle(letterSpacing: 4, color: Brand.mist, fontSize: 12)),
      ],
    );
  }
}
