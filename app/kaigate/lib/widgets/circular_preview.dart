import "dart:typed_data";

import "package:flutter/material.dart";

class CircularLens extends StatelessWidget {
  const CircularLens({super.key, this.bytes, this.child, this.size = 280});
  final Uint8List? bytes;
  final Widget? child;
  final double size;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: size,
      height: size,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        border: Border.all(color: const Color(0xFFE10600), width: 3),
        boxShadow: const [BoxShadow(color: Color(0x55E10600), blurRadius: 18)],
      ),
      child: ClipOval(
        child: bytes != null
            ? Image.memory(bytes!, fit: BoxFit.cover, width: size, height: size)
            : Container(color: Colors.black, alignment: Alignment.center, child: child),
      ),
    );
  }
}
