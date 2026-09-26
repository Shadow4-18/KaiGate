import "dart:typed_data";

import "package:flutter/material.dart";
import "package:image/image.dart" as img;
import "package:image_picker/image_picker.dart";
import "package:provider/provider.dart";

import "../providers/knob_controller.dart";
import "../services/asset_export.dart";
import "../theme/brand.dart";
import "../widgets/circular_preview.dart";

class StudioScreen extends StatefulWidget {
  const StudioScreen({super.key});
  @override
  State<StudioScreen> createState() => _StudioScreenState();
}

class _StudioScreenState extends State<StudioScreen> {
  img.Image? _src;
  Uint8List? _preview;
  double _scale = 1;
  double _panX = 0;
  double _panY = 0;
  String _dest = "/idle.gif";
  bool _busy = false;
  final _xfer = WifiTransfer();

  Future<void> _pick() async {
    final x = await ImagePicker().pickImage(source: ImageSource.gallery, imageQuality: 95);
    if (x == null) return;
    final bytes = await x.readAsBytes();
    setState(() {
      _src = img.decodeImage(bytes);
      _rebuild();
    });
  }

  void _rebuild() {
    if (_src == null) return;
    _preview = toPreviewPng(_src!, scale: _scale, panX: _panX, panY: _panY);
  }

  Future<void> _push() async {
    final k = context.read<KnobController>();
    setState(() => _busy = true);
    try {
      await k.cmd("wifiStart");
      await Future<void>.delayed(const Duration(seconds: 2));
      var tries = 0;
      while (tries < 12 && !await _xfer.ping()) {
        await Future<void>.delayed(const Duration(milliseconds: 500));
        tries++;
      }
      if (_src == null) throw Exception("No image");
      if (_dest.endsWith(".bin")) {
        final bin = toRgb565Bin(_src!, scale: _scale, panX: _panX, panY: _panY);
        await _xfer.uploadBytes(bin, _dest, filename: "wallpaper.bin");
      } else {
        final png = toPreviewPng(_src!, scale: _scale, panX: _panX, panY: _panY);
        await _xfer.uploadBytes(png, _dest, filename: "idle.gif");
      }
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Pushed to LittleFS")));
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text("Join Wi-Fi 'KaiGate_Setup' / kaigate32 then retry. ($e)")),
        );
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("LENS STUDIO")),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          const Text("Live 466×466 circular mask. Pan/zoom, then push over the knob SoftAP."),
          const SizedBox(height: 16),
          Center(child: CircularLens(bytes: _preview, child: const Text("PICK"))),
          const SizedBox(height: 16),
          FilledButton.tonal(onPressed: _pick, child: const Text("PICK IMAGE / GIF FRAME")),
          const SizedBox(height: 8),
          const Text("Zoom"),
          Slider(
            value: _scale,
            min: 0.6,
            max: 3,
            onChanged: (v) => setState(() {
              _scale = v;
              _rebuild();
            }),
          ),
          const Text("Pan X"),
          Slider(
            value: _panX,
            min: -200,
            max: 200,
            onChanged: (v) => setState(() {
              _panX = v;
              _rebuild();
            }),
          ),
          const Text("Pan Y"),
          Slider(
            value: _panY,
            min: -200,
            max: 200,
            onChanged: (v) => setState(() {
              _panY = v;
              _rebuild();
            }),
          ),
          DropdownButton<String>(
            value: _dest,
            dropdownColor: Brand.card,
            items: const [
              DropdownMenuItem(value: "/idle.gif", child: Text("Idle mascot")),
              DropdownMenuItem(value: "/boot.gif", child: Text("Boot animation")),
              DropdownMenuItem(value: "/wallpaper.bin", child: Text("Static wallpaper RGB565")),
            ],
            onChanged: (v) => setState(() => _dest = v ?? _dest),
          ),
          const SizedBox(height: 8),
          FilledButton(
            onPressed: _busy ? null : _push,
            child: Text(_busy ? "TRANSFERRING…" : "WIFI PUSH TO LITTLEFS"),
          ),
          const SizedBox(height: 8),
          const Text(
            "The knob raises SoftAP SSID KaiGate_Setup (password kaigate32). Connect the phone to that network during transfer.",
            style: TextStyle(color: Brand.mist, fontSize: 12),
          ),
        ],
      ),
    );
  }
}
